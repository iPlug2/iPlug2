/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <mutex>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <dlfcn.h>
#include <poll.h>
#include <cerrno>
#include <climits>
#include <limits>
#include <chrono>
#include <string>
// Fontconfig is loaded via dlopen so the plugin works without it installed.
// Only the font-by-name lookup (LoadPlatformFont) needs it; embedded fonts work regardless.

// X11 headers first — they define macros (None, Bool, True, False, Status, Complex...)
// that conflict with C++ identifiers. IGraphicsLinux.h undefines them.
#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

// glad must come before GL/glx.h: glad sets __gl_h_ so that glx.h's own
// #include <GL/gl.h> is skipped. If glx.h included gl.h first, glad would
// detect __gl_h_ and throw a compile-time error.
// Note: IGRAPHICS_GL is defined inside IGraphics_select.h (not yet processed here),
// so we use the CMake-passed IGRAPHICS_GL2/GL3 defines directly.
#if defined(IGRAPHICS_GL2) || defined(IGRAPHICS_GL3)
  #include <glad/glad.h>
  // After glad, Bool is still defined (= int from X11/Xlib.h) so GLX
  // function declarations that use Bool compile correctly.
  #include <GL/glx.h>
#endif

// IGraphicsLinux.h undefs the conflicting X11 macros (None, Bool, etc.) and
// then includes IGraphics_select.h. The glad and GLX_H guards prevent those
// headers from being re-included.
#include "IGraphicsLinux.h"
#include "IGraphicsStructs.h"
#include "IControl.h"
#include "wdlutf8.h"

#if defined APP_API || defined REAPER_EXT_API
  #include "IPlugSWELLTimer.h"
#endif

using namespace iplug;
using namespace igraphics;

// Timer interval in microseconds (60 fps = ~16667 us)
static constexpr int TIMER_INTERVAL_US = 16667;

namespace
{
// Match WindowWidth()/WindowHeight() followed by the X11/display-scale conversion.
double PhysicalWindowDimension(int designDimension, float drawScale, float screenScale)
{
  // Probe large candidates without overflowing int; actual window sizes fit in int.
  const float logical = std::trunc(designDimension * drawScale);
  return std::trunc(logical * screenScale);
}

unsigned long HoverMillis() { return static_cast<unsigned long>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count()); }

std::vector<std::string> ParseDroppedFiles(const unsigned char* data, size_t length)
{
  std::vector<std::string> paths;
  const std::string list(reinterpret_cast<const char*>(data), length);
  for (size_t start = 0; start < list.size();)
  {
    const size_t end = list.find('\n', start);
    const std::string line = list.substr(start, end == std::string::npos ? end : end - start);
    start = end == std::string::npos ? list.size() : end + 1;
    if (line.empty() || line[0] == '#')
      continue;
    const std::string uri = line.back() == '\r' ? line.substr(0, line.size() - 1) : line;
    if (uri.compare(0, 7, "file://") != 0)
      continue;
    size_t pathStart = 7;
    if (uri.compare(pathStart, 10, "localhost/") == 0)
      pathStart += 9;
    if (pathStart >= uri.size() || uri[pathStart] != '/')
      continue;
    std::string path;
    bool valid = true;
    for (size_t i = pathStart; i < uri.size(); ++i)
    {
      if (uri[i] != '%')
      {
        path += uri[i];
        continue;
      }
      if (i + 2 >= uri.size())
      {
        valid = false;
        break;
      }
      auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9')
          return c - '0';
        if (c >= 'a' && c <= 'f')
          return c - 'a' + 10;
        if (c >= 'A' && c <= 'F')
          return c - 'A' + 10;
        return -1;
      };
      const int hi = hex(uri[i + 1]), lo = hex(uri[i + 2]);
      if (hi < 0 || lo < 0 || (hi == 0 && lo == 0))
      {
        valid = false;
        break;
      }
      path += static_cast<char>((hi << 4) | lo);
      i += 2;
    }
    if (valid && path.find('\0') == std::string::npos)
      paths.push_back(std::move(path));
  }
  return paths;
}

class LinuxClipboard
{
public:
  ~LinuxClipboard()
  {
    mRunning = false;
    if (mThread)
      pthread_join(mThread, nullptr);
    if (mDisplay)
    {
      XDestroyWindow(mDisplay, mWindow);
      XCloseDisplay(mDisplay);
    }
  }

  bool SetText(const char* text, const char* displayName)
  {
    std::lock_guard<std::mutex> lock(mMutex);
    if (!mDisplay)
    {
      mDisplay = XOpenDisplay(displayName);
      if (!mDisplay)
      {
        fprintf(stderr, "Could not open the X11 clipboard display\n");
        return false;
      }
      mConnection = XGetXCBConnection(mDisplay);
      if (!mConnection)
      {
        fprintf(stderr, "Could not access the XCB clipboard connection\n");
        XCloseDisplay(mDisplay);
        mDisplay = nullptr;
        return false;
      }
      mWindow = XCreateSimpleWindow(mDisplay, DefaultRootWindow(mDisplay), 0, 0, 1, 1, 0, 0, 0);
      mClipboard = XInternAtom(mDisplay, "CLIPBOARD", 0);
      mUTF8 = XInternAtom(mDisplay, "UTF8_STRING", 0);
      mTargets = XInternAtom(mDisplay, "TARGETS", 0);
      mRunning = true;
      const int error = pthread_create(&mThread, nullptr, Run, this);
      if (error)
      {
        fprintf(stderr, "Could not start the X11 clipboard service: %s\n", strerror(error));
        mRunning = false;
        mThread = 0;
        XDestroyWindow(mDisplay, mWindow);
        XCloseDisplay(mDisplay);
        mDisplay = nullptr;
        mConnection = nullptr;
        return false;
      }
    }
    if (strcmp(DisplayString(mDisplay), displayName) != 0)
    {
      fprintf(stderr, "The clipboard service is already connected to another X11 display\n");
      return false;
    }
    mText.Set(text);
    XSetSelectionOwner(mDisplay, mClipboard, mWindow, 0);
    XSync(mDisplay, 0);
    return XGetSelectionOwner(mDisplay, mClipboard) == mWindow;
  }

  bool GetOwnedText(Window owner, const char* displayName, WDL_String& text)
  {
    std::lock_guard<std::mutex> lock(mMutex);
    if (!mDisplay || owner != mWindow || strcmp(DisplayString(mDisplay), displayName) != 0)
      return false;
    text.Set(mText.Get());
    return true;
  }

private:
  static void* Run(void* context)
  {
    auto* self = static_cast<LinuxClipboard*>(context);
    while (self->mRunning)
    {
      {
        std::lock_guard<std::mutex> lock(self->mMutex);
        while (XPending(self->mDisplay))
        {
          XEvent event;
          XNextEvent(self->mDisplay, &event);
          if (event.type == SelectionRequest)
            self->Reply(event.xselectionrequest);
          else if (event.type == SelectionClear && XGetSelectionOwner(self->mDisplay, self->mClipboard) != self->mWindow)
            self->mText.Set("");
        }
      }
      pollfd fd{XConnectionNumber(self->mDisplay), POLLIN, 0};
      const int result = poll(&fd, 1, 20);
      if (result < 0 && errno != EINTR)
      {
        fprintf(stderr, "Could not poll the X11 clipboard display: %s\n", strerror(errno));
        break;
      }
    }
    return nullptr;
  }

  void Reply(const XSelectionRequestEvent& request)
  {
    XFlush(mDisplay);
    xcb_selection_notify_event_t response{};
    response.response_type = XCB_SELECTION_NOTIFY;
    response.requestor = request.requestor;
    response.selection = request.selection;
    response.target = request.target;
    response.time = request.time;
    const Atom property = request.property ? request.property : request.target;
    if (request.target == mTargets)
    {
      const xcb_atom_t targets[] = {static_cast<xcb_atom_t>(mTargets), static_cast<xcb_atom_t>(mUTF8), XCB_ATOM_STRING};
      if (!CheckReply(xcb_change_property_checked(mConnection, XCB_PROP_MODE_REPLACE, request.requestor, property, XCB_ATOM_ATOM, 32, 3, targets), "property write"))
        return;
      response.property = property;
    }
    else if (request.target == mUTF8 || request.target == XA_STRING)
    {
      if (!CheckReply(xcb_change_property_checked(mConnection, XCB_PROP_MODE_REPLACE, request.requestor, property, request.target, 8, mText.GetLength(), mText.Get()), "property write"))
        return;
      response.property = property;
    }
    char event[32]{};
    static_assert(sizeof(response) <= sizeof(event), "X11 selection event must fit the wire event");
    memcpy(event, &response, sizeof(response));
    CheckReply(xcb_send_event_checked(mConnection, 0, request.requestor, XCB_EVENT_MASK_NO_EVENT, event), "selection notification");
  }

  bool CheckReply(xcb_void_cookie_t cookie, const char* operation)
  {
    xcb_generic_error_t* error = xcb_request_check(mConnection, cookie);
    if (error)
    {
      fprintf(stderr, "X11 clipboard %s failed (error %u, resource 0x%x)\n", operation, error->error_code, error->resource_id);
      free(error);
      return false;
    }
    if (xcb_connection_has_error(mConnection))
    {
      fprintf(stderr, "The XCB clipboard connection failed during %s\n", operation);
      return false;
    }
    return true;
  }

  Display* mDisplay = nullptr;
  xcb_connection_t* mConnection = nullptr;
  Window mWindow = 0;
  Atom mClipboard = 0;
  Atom mUTF8 = 0;
  Atom mTargets = 0;
  WDL_String mText;
  std::mutex mMutex;
  std::atomic<bool> mRunning{false};
  pthread_t mThread = 0;
};

static LinuxClipboard& GetLinuxClipboard()
{
  // A separate connection serves selections even while the host UI thread waits for a paste.
  static LinuxClipboard clipboard;
  return clipboard;
}
} // namespace

// ---- Font helpers -----------------------------------------------------------

/** Loads a TTF font from a file path; GetFontData() reads the bytes on demand. */
class LinuxFileFont final : public PlatformFont
{
public:
  LinuxFileFont(const char* filePath, bool isSystem)
    : PlatformFont(isSystem)
  {
    mPath.Set(filePath);
  }

  IFontDataPtr GetFontData() override
  {
    IFontDataPtr fontData(new IFontData());
    FILE* fp = fopen(mPath.Get(), "rb");
    if (!fp)
      return fontData;

    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    if (fileSize <= 0)
    {
      fclose(fp);
      return fontData;
    }
    fontData = std::make_unique<IFontData>((int)fileSize);

    if (!fontData->GetSize())
    {
      fclose(fp);
      return fontData;
    }

    fseek(fp, 0, SEEK_SET);
    size_t readSize = fread(fontData->Get(), 1, fontData->GetSize(), fp);
    fclose(fp);

    if (readSize && readSize == (size_t)fontData->GetSize())
      fontData->SetFaceIdx(0);

    return fontData;
  }

private:
  WDL_String mPath;
};

/** Wraps pre-loaded raw font bytes; takes a copy so the caller can free its buffer. */
class LinuxMemFont final : public PlatformFont
{
public:
  LinuxMemFont(const void* pData, int dataSize)
    : PlatformFont(false)
  {
    mData.Set((const unsigned char*)pData, dataSize);
  }

  IFontDataPtr GetFontData() override { return IFontDataPtr(new IFontData(mData.Get(), mData.GetSize(), 0)); }

private:
  WDL_TypedBuf<unsigned char> mData;
};

// ---- Runtime GTK3 (optional — loaded via dlopen so the plugin works without GTK) ----

struct GdkRGBA
{
  double red, green, blue, alpha;
};
struct GdkRectangle
{
  int x, y, width, height;
};

// GdkGravity values for gtk_menu_popup_at_rect anchoring
static constexpr int kGDK_GRAVITY_NORTH_WEST = 1;
static constexpr int kGDK_GRAVITY_SOUTH_WEST = 7;

// Opaque GTK/GDK types — only used as void* through the function pointers below.
struct _GtkWidget;

// GTK response codes (subset we need)
static constexpr int kGTK_RESPONSE_OK = -5;
static constexpr int kGTK_RESPONSE_CANCEL = -6;
static constexpr int kGTK_RESPONSE_YES = -8;
static constexpr int kGTK_RESPONSE_NO = -9;
static constexpr int kGTK_RESPONSE_RETRY = 1;

// GTK file-chooser action codes
static constexpr int kGTK_FILE_CHOOSER_ACTION_OPEN = 0;
static constexpr int kGTK_FILE_CHOOSER_ACTION_SAVE = 1;
static constexpr int kGTK_FILE_CHOOSER_ACTION_SELECT_FOLDER = 2;

// GTK button presets used by message dialogs
static constexpr int kGTK_BUTTONS_NONE = 0;
static constexpr int kGTK_BUTTONS_OK = 1;
static constexpr int kGTK_BUTTONS_YES_NO = 4;
static constexpr int kGTK_BUTTONS_OK_CANCEL = 5;

// GTK message-dialog icon types
static constexpr int kGTK_MESSAGE_INFO = 0;
static constexpr int kGTK_MESSAGE_QUESTION = 3;

struct GTK3
{
  void* handle = nullptr;

  // Core
  using fn_init_check = int (*)(int*, char***);
  using fn_events_pending = int (*)();
  using fn_main_iteration_do = int (*)(int);
  // Dialogs
  using fn_dialog_run = int (*)(void*);
  using fn_dialog_add_button = void* (*)(void*, const char*, int);
  using fn_widget_destroy = void (*)(void*);
  using fn_window_set_title = void (*)(void*, const char*);
  // File chooser
  using fn_file_chooser_dialog_new = void* (*)(const char*, void*, int, const char*, ...);
  using fn_file_chooser_get_filename = char* (*)(void*);
  using fn_file_chooser_set_current_folder = int (*)(void*, const char*);
  using fn_file_chooser_set_current_name = void (*)(void*, const char*);
  using fn_file_filter_new = void* (*)();
  using fn_file_filter_set_name = void (*)(void*, const char*);
  using fn_file_filter_add_pattern = void (*)(void*, const char*);
  using fn_file_chooser_add_filter = void (*)(void*, void*);
  // Color chooser
  using fn_color_chooser_dialog_new = void* (*)(const char*, void*);
  using fn_color_chooser_get_rgba = void (*)(void*, GdkRGBA*);
  using fn_color_chooser_set_rgba = void (*)(void*, const GdkRGBA*);
  // Message dialog
  using fn_message_dialog_new = void* (*)(void*, int, int, int, const char*, ...);
  // Popup menu
  using fn_menu_new = void* (*)();
  using fn_menu_item_new_with_label = void* (*)(const char*);
  using fn_check_menu_item_new_with_label = void* (*)(const char*);
  using fn_separator_menu_item_new = void* (*)();
  using fn_menu_shell_append = void (*)(void*, void*);
  using fn_menu_item_set_submenu = void (*)(void*, void*);
  using fn_check_menu_item_set_active = void (*)(void*, int);
  using fn_widget_set_sensitive = void (*)(void*, int);
  using fn_widget_show_all = void (*)(void*);
  using fn_widget_get_mapped = int (*)(void*);
  using fn_menu_popup_at_pointer = void (*)(void*, void*);
  // gtk_menu_popup_at_rect: positions menu relative to a GdkWindow rectangle.
  // args: menu, rect_window, rect(GdkRectangle*), rect_anchor, menu_anchor, trigger_event
  using fn_menu_popup_at_rect = void (*)(void*, void*, const void*, int, int, void*);
  using fn_main = void (*)();
  using fn_main_quit = void (*)();
  // g_signal_connect is a macro over g_signal_connect_data in GObject
  using fn_signal_connect_data = unsigned long (*)(void*, const char*, void*, void*, void*, int);
  // Window management (for dialog parenting)
  using fn_window_set_modal = void (*)(void*, int);
  using fn_window_present = void (*)(void*);
  using fn_window_set_position = void (*)(void*, int);
  // GDK helpers for popup menu positioning
  using fn_gdk_display_get_default = void* (*)();
  using fn_gdk_x11_window_foreign = void* (*)(void*, unsigned long);
  using fn_g_object_unref = void (*)(void*);
  // Entry dialog
  using fn_entry_new = void* (*)();
  using fn_entry_set_text = void (*)(void*, const char*);
  using fn_entry_get_text = const char* (*)(void*);
  using fn_entry_set_max_length = void (*)(void*, int);
  using fn_dialog_new_with_buttons = void* (*)(const char*, void*, int, const char*, ...);
  using fn_dialog_get_content_area = void* (*)(void*);
  using fn_dialog_response = void (*)(void*, int);
  using fn_box_pack_start = void (*)(void*, void*, int, int, unsigned);

  fn_init_check init_check = nullptr;
  fn_events_pending events_pending = nullptr;
  fn_main_iteration_do main_iteration_do = nullptr;
  fn_dialog_run dialog_run = nullptr;
  fn_dialog_add_button dialog_add_button = nullptr;
  fn_widget_destroy widget_destroy = nullptr;
  fn_window_set_title window_set_title = nullptr;
  fn_file_chooser_dialog_new file_chooser_dialog_new = nullptr;
  fn_file_chooser_get_filename file_chooser_get_filename = nullptr;
  fn_file_chooser_set_current_folder file_chooser_set_current_folder = nullptr;
  fn_file_chooser_set_current_name file_chooser_set_current_name = nullptr;
  fn_file_filter_new file_filter_new = nullptr;
  fn_file_filter_set_name file_filter_set_name = nullptr;
  fn_file_filter_add_pattern file_filter_add_pattern = nullptr;
  fn_file_chooser_add_filter file_chooser_add_filter = nullptr;
  fn_color_chooser_dialog_new color_chooser_dialog_new = nullptr;
  fn_color_chooser_get_rgba color_chooser_get_rgba = nullptr;
  fn_color_chooser_set_rgba color_chooser_set_rgba = nullptr;
  fn_message_dialog_new message_dialog_new = nullptr;
  fn_menu_new menu_new = nullptr;
  fn_menu_item_new_with_label menu_item_new_with_label = nullptr;
  fn_check_menu_item_new_with_label check_menu_item_new_with_label = nullptr;
  fn_separator_menu_item_new separator_menu_item_new = nullptr;
  fn_menu_shell_append menu_shell_append = nullptr;
  fn_menu_item_set_submenu menu_item_set_submenu = nullptr;
  fn_check_menu_item_set_active check_menu_item_set_active = nullptr;
  fn_widget_set_sensitive widget_set_sensitive = nullptr;
  fn_widget_show_all widget_show_all = nullptr;
  fn_widget_get_mapped widget_get_mapped = nullptr;
  fn_menu_popup_at_pointer menu_popup_at_pointer = nullptr;
  fn_menu_popup_at_rect menu_popup_at_rect = nullptr;
  fn_main main = nullptr;
  fn_main_quit main_quit = nullptr;
  fn_signal_connect_data signal_connect_data = nullptr;
  fn_entry_new entry_new = nullptr;
  fn_entry_set_text entry_set_text = nullptr;
  fn_entry_get_text entry_get_text = nullptr;
  fn_entry_set_max_length entry_set_max_length = nullptr;
  fn_window_set_modal window_set_modal = nullptr;
  fn_window_present window_present = nullptr;
  fn_window_set_position window_set_position = nullptr;
  fn_gdk_display_get_default gdk_display_get_default = nullptr;
  fn_gdk_x11_window_foreign gdk_x11_window_foreign = nullptr;
  fn_g_object_unref g_object_unref = nullptr;
  fn_dialog_new_with_buttons dialog_new_with_buttons = nullptr;
  fn_dialog_get_content_area dialog_get_content_area = nullptr;
  fn_dialog_response dialog_response = nullptr;
  fn_box_pack_start box_pack_start = nullptr;
};

#define LOAD_GTK_SYM(g, name) g.name = (GTK3::fn_##name)dlsym(g.handle, "gtk_" #name)

static GTK3* LoadGTK3()
{
  static GTK3 gtk;
  static bool sTried = false;
  if (sTried)
    return gtk.handle ? &gtk : nullptr;
  sTried = true;

  // GTK 3 aborts the process if it is initialised alongside GTK 2, so do
  // without native dialogs in a host that has already loaded GTK 2.
  if (dlsym(RTLD_DEFAULT, "gtk_progress_get_type"))
    return nullptr;

  // RTLD_LOCAL keeps GTK's symbols out of the host's global namespace; all
  // GTK calls go through the explicit function-pointer table below.
  gtk.handle = dlopen("libgtk-3.so.0", RTLD_NOW | RTLD_LOCAL);
  if (!gtk.handle)
    return nullptr;

  LOAD_GTK_SYM(gtk, init_check);
  LOAD_GTK_SYM(gtk, events_pending);
  LOAD_GTK_SYM(gtk, main_iteration_do);
  LOAD_GTK_SYM(gtk, dialog_run);
  LOAD_GTK_SYM(gtk, dialog_add_button);
  LOAD_GTK_SYM(gtk, widget_destroy);
  LOAD_GTK_SYM(gtk, window_set_title);
  LOAD_GTK_SYM(gtk, file_chooser_dialog_new);
  LOAD_GTK_SYM(gtk, file_chooser_get_filename);
  LOAD_GTK_SYM(gtk, file_chooser_set_current_folder);
  LOAD_GTK_SYM(gtk, file_chooser_set_current_name);
  LOAD_GTK_SYM(gtk, file_filter_new);
  LOAD_GTK_SYM(gtk, file_filter_set_name);
  LOAD_GTK_SYM(gtk, file_filter_add_pattern);
  LOAD_GTK_SYM(gtk, file_chooser_add_filter);
  LOAD_GTK_SYM(gtk, color_chooser_dialog_new);
  LOAD_GTK_SYM(gtk, color_chooser_get_rgba);
  LOAD_GTK_SYM(gtk, color_chooser_set_rgba);
  LOAD_GTK_SYM(gtk, message_dialog_new);
  LOAD_GTK_SYM(gtk, menu_new);
  LOAD_GTK_SYM(gtk, menu_item_new_with_label);
  LOAD_GTK_SYM(gtk, check_menu_item_new_with_label);
  LOAD_GTK_SYM(gtk, separator_menu_item_new);
  LOAD_GTK_SYM(gtk, menu_shell_append);
  LOAD_GTK_SYM(gtk, menu_item_set_submenu);
  LOAD_GTK_SYM(gtk, check_menu_item_set_active);
  LOAD_GTK_SYM(gtk, widget_set_sensitive);
  LOAD_GTK_SYM(gtk, widget_show_all);
  LOAD_GTK_SYM(gtk, widget_get_mapped);
  LOAD_GTK_SYM(gtk, menu_popup_at_pointer);
  LOAD_GTK_SYM(gtk, menu_popup_at_rect);
  LOAD_GTK_SYM(gtk, main);
  LOAD_GTK_SYM(gtk, main_quit);
  // g_signal_connect_data is in libgobject, but GTK re-exports it
  gtk.signal_connect_data = (GTK3::fn_signal_connect_data)dlsym(gtk.handle, "g_signal_connect_data");
  LOAD_GTK_SYM(gtk, entry_new);
  LOAD_GTK_SYM(gtk, entry_set_text);
  LOAD_GTK_SYM(gtk, entry_get_text);
  LOAD_GTK_SYM(gtk, entry_set_max_length);
  LOAD_GTK_SYM(gtk, window_set_modal);
  LOAD_GTK_SYM(gtk, window_present);
  LOAD_GTK_SYM(gtk, window_set_position);
  // GDK/GLib symbols (not gtk_ prefixed — load directly)
  gtk.gdk_display_get_default = (GTK3::fn_gdk_display_get_default)dlsym(gtk.handle, "gdk_display_get_default");
  gtk.gdk_x11_window_foreign = (GTK3::fn_gdk_x11_window_foreign)dlsym(gtk.handle, "gdk_x11_window_foreign_new_for_display");
  gtk.g_object_unref = (GTK3::fn_g_object_unref)dlsym(gtk.handle, "g_object_unref");
  LOAD_GTK_SYM(gtk, dialog_new_with_buttons);
  LOAD_GTK_SYM(gtk, dialog_get_content_area);
  LOAD_GTK_SYM(gtk, dialog_response);
  LOAD_GTK_SYM(gtk, box_pack_start);

  if (!gtk.init_check || !gtk.dialog_run || !gtk.widget_destroy || !gtk.file_chooser_dialog_new || !gtk.file_chooser_get_filename)
  {
    dlclose(gtk.handle);
    gtk.handle = nullptr;
    return nullptr;
  }

  // gtk_init_check() would otherwise call setlocale(LC_ALL, ""), changing
  // e.g. the decimal separator for the host and every other plugin.
  if (auto disableSetLocale = reinterpret_cast<void (*)()>(dlsym(gtk.handle, "gtk_disable_setlocale")))
    disableSetLocale();

  if (!gtk.init_check(nullptr, nullptr))
  {
    dlclose(gtk.handle);
    gtk.handle = nullptr;
    return nullptr;
  }

  // The standalone runs GDK at window scale 1 (SWELL draws its own HiDPI UI),
  // so GTK dialogs would be drawn at 1x. Scale their fonts by the remaining
  // factor instead; in plugin hosts GDK already applies the monitor scale.
  using fn_screen_get_default = void* (*)();
  using fn_monitor_scale = int (*)(void*, int);
  using fn_settings_get_default = void* (*)();
  using fn_object_get = void (*)(void*, const char*, ...);
  using fn_object_set = void (*)(void*, const char*, ...);
  auto screen_get_default = (fn_screen_get_default)dlsym(gtk.handle, "gdk_screen_get_default");
  auto monitor_scale = (fn_monitor_scale)dlsym(gtk.handle, "gdk_screen_get_monitor_scale_factor");
  auto settings_get_default = (fn_settings_get_default)dlsym(gtk.handle, "gtk_settings_get_default");
  auto object_get = (fn_object_get)dlsym(gtk.handle, "g_object_get");
  auto object_set = (fn_object_set)dlsym(gtk.handle, "g_object_set");
  const char* str = getenv("IPLUG2_SCREEN_SCALE");
  char* pEnd = nullptr;
  const double screenScale = str ? strtod(str, &pEnd) : 0.;
  void* pScreen = screen_get_default ? screen_get_default() : nullptr;
  void* pSettings = settings_get_default ? settings_get_default() : nullptr;
  if (str && pEnd != str && *pEnd == '\0' && screenScale >= 1. && screenScale <= 8. && pScreen && pSettings && monitor_scale && object_get && object_set)
  {
    const double remaining = screenScale / std::max(1, monitor_scale(pScreen, 0));
    if (remaining > 1.01)
    {
      int dpi = -1; // in 1/1024ths of a dot per inch; -1 when unset
      object_get(pSettings, "gtk-xft-dpi", &dpi, nullptr);
      if (dpi <= 0)
        dpi = 96 * 1024;
      object_set(pSettings, "gtk-xft-dpi", static_cast<int>(std::lround(dpi * remaining)), nullptr);
    }
  }
  return &gtk;
}

static void GtkDrainEvents(GTK3* pGTK)
{
  if (pGTK->events_pending && pGTK->main_iteration_do)
    while (pGTK->events_pending())
      pGTK->main_iteration_do(0 /* non-blocking */);
}

// Present a GTK dialog as modal so the WM grants it focus and input grabs.
// Without this, file-chooser dropdowns and color-picker sub-dialogs fail
// on compositing WMs (Mutter, KWin) because GTK can't acquire a GDK grab.
static constexpr int kGTK_WIN_POS_MOUSE = 2;

static void GtkPresentDialog(GTK3* pGTK, void* dialog)
{
  if (pGTK->window_set_modal)
    pGTK->window_set_modal(dialog, 1);
  // The dialog has no GTK parent to be placed relative to, so without a hint
  // it opens at the screen origin. Open it at the pointer, where the user clicked.
  if (pGTK->window_set_position)
    pGTK->window_set_position(dialog, kGTK_WIN_POS_MOUSE);
  if (pGTK->window_present)
    pGTK->window_present(dialog);
}

// Launch a detached external process. Double-forks so the grandchild is
// reparented to init and the intermediate child is reaped here — neither
// becomes a zombie. Returns true if the launch was initiated.
static bool LaunchDetached(const char* program, const char* arg)
{
  pid_t pid = fork();
  if (pid == 0)
  {
    pid_t grandchild = fork();
    if (grandchild == 0)
    {
      execlp(program, program, arg, (char*)nullptr);
      _exit(127);
    }
    _exit(grandchild > 0 ? 0 : 1);
  }
  if (pid > 0)
  {
    int status;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR)
    {
    }
    return true;
  }
  return false;
}

static bool GtkAddFileFilters(GTK3* pGTK, void* dialog, const char* ext)
{
  WDL_String extensions(ext ? ext : "");
  char* context = nullptr;
  char* token = strtok_r(extensions.Get(), " \t\r\n", &context);
  if (!token)
    return true;

  if (!pGTK->file_filter_new || !pGTK->file_filter_add_pattern || !pGTK->file_chooser_add_filter)
  {
    fprintf(stderr, "Cannot apply file dialog filters: required GTK functions are unavailable\n");
    return false;
  }

  void* filter = pGTK->file_filter_new();
  if (!filter)
  {
    fprintf(stderr, "Could not create the file extension filter\n");
    return false;
  }
  if (pGTK->file_filter_set_name)
    pGTK->file_filter_set_name(filter, ext);
  do
  {
    WDL_String pattern;
    if (token[0] == '*')
      pattern.Set(token);
    else
    {
      pattern.Set("*.");
      pattern.Append(token[0] == '.' ? token + 1 : token);
    }
    pGTK->file_filter_add_pattern(filter, pattern.Get());
    token = strtok_r(nullptr, " \t\r\n", &context);
  } while (token);
  pGTK->file_chooser_add_filter(dialog, filter);

  void* all = pGTK->file_filter_new();
  if (!all)
  {
    fprintf(stderr, "Could not create the all-files filter\n");
    return false;
  }
  if (pGTK->file_filter_set_name)
    pGTK->file_filter_set_name(all, "All Files");
  pGTK->file_filter_add_pattern(all, "*");
  pGTK->file_chooser_add_filter(dialog, all);
  return true;
}

// ---- Runtime fontconfig (optional — loaded via dlopen so plugins work without it) ----

// Fontconfig opaque types — only used through function pointers below.
struct _FcPattern;
struct _FcConfig;

// Fontconfig constants we need (match the enum values from fontconfig.h)
static constexpr int kFC_WEIGHT_REGULAR = 80;
static constexpr int kFC_WEIGHT_BOLD = 200;
static constexpr int kFC_SLANT_ROMAN = 0;
static constexpr int kFC_SLANT_ITALIC = 100;
static constexpr int kFC_MatchPattern = 0; // FcMatchKind
static constexpr int kFC_ResultMatch = 0;  // FcResult

struct Fontconfig
{
  void* handle = nullptr;

  using fn_PatternCreate = _FcPattern* (*)();
  using fn_PatternAddString = int (*)(_FcPattern*, const char*, const unsigned char*);
  using fn_PatternAddInteger = int (*)(_FcPattern*, const char*, int);
  using fn_ConfigSubstitute = int (*)(_FcConfig*, _FcPattern*, int);
  using fn_DefaultSubstitute = void (*)(_FcPattern*);
  using fn_FontMatch = _FcPattern* (*)(_FcConfig*, _FcPattern*, int*);
  using fn_PatternDestroy = void (*)(_FcPattern*);
  using fn_PatternGetString = int (*)(_FcPattern*, const char*, int, unsigned char**);

  fn_PatternCreate PatternCreate = nullptr;
  fn_PatternAddString PatternAddString = nullptr;
  fn_PatternAddInteger PatternAddInteger = nullptr;
  fn_ConfigSubstitute ConfigSubstitute = nullptr;
  fn_DefaultSubstitute DefaultSubstitute = nullptr;
  fn_FontMatch FontMatch = nullptr;
  fn_PatternDestroy PatternDestroy = nullptr;
  fn_PatternGetString PatternGetString = nullptr;
};

#define LOAD_FC_SYM(fc, name) fc.name = (Fontconfig::fn_##name)dlsym(fc.handle, "Fc" #name)

static Fontconfig* LoadFontconfig()
{
  static Fontconfig fc;
  static bool sTried = false;

  if (sTried)
    return fc.handle ? &fc : nullptr;

  sTried = true;
  fc.handle = dlopen("libfontconfig.so.1", RTLD_NOW);
  if (!fc.handle)
    return nullptr;

  LOAD_FC_SYM(fc, PatternCreate);
  LOAD_FC_SYM(fc, PatternAddString);
  LOAD_FC_SYM(fc, PatternAddInteger);
  LOAD_FC_SYM(fc, ConfigSubstitute);
  LOAD_FC_SYM(fc, DefaultSubstitute);
  LOAD_FC_SYM(fc, FontMatch);
  LOAD_FC_SYM(fc, PatternDestroy);
  LOAD_FC_SYM(fc, PatternGetString);

  if (!fc.PatternCreate || !fc.FontMatch || !fc.PatternGetString)
  {
    dlclose(fc.handle);
    fc.handle = nullptr;
    return nullptr;
  }

  return &fc;
}

IGraphicsLinux::IGraphicsLinux(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
  : IGRAPHICS_DRAW_CLASS(dlg, w, h, fps, scale)
{
#if defined VST3_API || defined VST3C_API || defined CLAP_API
  mHostDriven = true;
#endif
  // Required before any Xlib calls from multiple threads
  XInitThreads();
}

IGraphicsLinux::~IGraphicsLinux() { CloseWindow(); }

void* IGraphicsLinux::OpenWindow(void* pParent)
{
  StopTimer();
  mCloseRequested = false;
  mDisplay = XOpenDisplay(nullptr);
  if (!mDisplay)
    return nullptr;

  int screen = DefaultScreen(mDisplay);
  Window root = RootWindow(mDisplay, screen);

  mParentWnd = root;
  if (pParent)
  {
    auto* connection = XGetXCBConnection(mDisplay);
    if (!connection)
    {
      fprintf(stderr, "Could not access the XCB connection to validate the editor parent\n");
      XCloseDisplay(mDisplay);
      mDisplay = nullptr;
      return nullptr;
    }
    const uintptr_t parent = reinterpret_cast<uintptr_t>(pParent);
    xcb_generic_error_t* error = nullptr;
    xcb_get_window_attributes_reply_t* attributes = nullptr;
    if (parent <= UINT32_MAX)
      attributes = xcb_get_window_attributes_reply(connection, xcb_get_window_attributes(connection, static_cast<xcb_window_t>(parent)), &error);
    if (attributes && !error)
      mParentWnd = static_cast<Window>(parent);
    else
      fprintf(stderr, "Invalid X11 editor parent 0x%lx; using a top-level window\n", static_cast<unsigned long>(parent));
    free(attributes);
    free(error);
    if (xcb_connection_has_error(connection))
    {
      fprintf(stderr, "The XCB connection failed while validating the editor parent\n");
      XCloseDisplay(mDisplay);
      mDisplay = nullptr;
      return nullptr;
    }
  }

  // Detect HiDPI scale factor. The standalone APP sets IPLUG2_SCREEN_SCALE after
  // querying gdk_window_get_scale_factor(). Plugin hosts may set GDK_SCALE.
  // Note: on Xwayland the X11 coordinate space uses physical pixels, not logical
  // pixels, so XDisplayWidth returns the logical (scaled-down) extent. Do NOT
  // compare physW against XDisplayWidth — it will always exceed the logical extent
  // on HiDPI displays. Trust gdk_window_get_scale_factor instead.
  {
    float gdkScale = 1.f;
    const char* s = getenv("IPLUG2_SCREEN_SCALE");
    if (!s)
      s = getenv("GDK_SCALE");
    if (s)
    {
      char* end = nullptr;
      const float parsed = strtof(s, &end);
      if (end != s && *end == '\0' && std::isfinite(parsed) && parsed >= 1.f && parsed <= 8.f)
        gdkScale = parsed;
    }
    if (gdkScale > 1.f && !GetDelegate()->HasHostScreenScale())
      SetScreenScale(gdkScale);
    // SetScreenScale is safe here: mPlugWnd==0 so PlatformResize skips,
    // and mVG==nullptr so DrawResize skips.
  }

  // In standalone mode the parent window is SWELL's GDK window which includes
  // the non-client area (menu bar). IPlugAPP_host exports the menu bar height
  // so we can offset the plugin window below it. Querying XGetWindowAttributes
  // on a separate Display connection returns stale geometry, so we use the env
  // var set by the host instead.
  int clientOffY = 0;
  {
    const char* menuOff = getenv("IPLUG2_MENU_OFFSET");
    if (menuOff)
      clientOffY = std::clamp(atoi(menuOff), 0, 100);
  }

#ifdef IGRAPHICS_GL
  XVisualInfo* pVisualInfo = CreateGLContext();
  if (!mGLContext)
  {
    XCloseDisplay(mDisplay);
    mDisplay = nullptr;
    return nullptr;
  }
  // Create a colormap compatible with the GL visual so the window and context
  // share the same visual — required for glXMakeCurrent to succeed.
  mGLColormap = XCreateColormap(mDisplay, root, pVisualInfo->visual, AllocNone);

  XSetWindowAttributes attrs = {};
  attrs.event_mask = ExposureMask | StructureNotifyMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask | KeyReleaseMask | EnterWindowMask | LeaveWindowMask | FocusChangeMask
                     | PropertyChangeMask;
  attrs.colormap = mGLColormap;
  // Required when the GL visual's depth differs from the parent's: without a
  // border pixel XCreateWindow fails with BadMatch, which is fatal by default.
  attrs.border_pixel = 0;

  // X11 window must be in physical pixels (WindowWidth/Height are logical).
  const auto physW = (unsigned)(WindowWidth() * GetScreenScale());
  const auto physH = (unsigned)(WindowHeight() * GetScreenScale());

  mPlugWnd = XCreateWindow(mDisplay, mParentWnd, 0, clientOffY, physW, physH, 0, pVisualInfo->depth, InputOutput, pVisualInfo->visual, CWColormap | CWBorderPixel | CWEventMask, &attrs);
  XFree(pVisualInfo);
#else
  XSetWindowAttributes attrs = {};
  attrs.event_mask = ExposureMask | StructureNotifyMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | KeyPressMask | KeyReleaseMask | EnterWindowMask | LeaveWindowMask | FocusChangeMask
                     | PropertyChangeMask;
  attrs.background_pixel = BlackPixel(mDisplay, screen);

  const auto physW = (unsigned)(WindowWidth() * GetScreenScale());
  const auto physH = (unsigned)(WindowHeight() * GetScreenScale());

  mPlugWnd = XCreateWindow(mDisplay, mParentWnd, 0, clientOffY, physW, physH, 0, CopyFromParent, InputOutput, CopyFromParent, CWEventMask | CWBackPixel, &attrs);
#endif

  if (!mPlugWnd)
  {
#ifdef IGRAPHICS_GL
    DestroyGLContext();
    if (mGLColormap)
    {
      XFreeColormap(mDisplay, mGLColormap);
      mGLColormap = 0;
    }
#endif
    XCloseDisplay(mDisplay);
    mDisplay = nullptr;
    return nullptr;
  }

  mWMDeleteMessage = XInternAtom(mDisplay, "WM_DELETE_WINDOW", 0 /* False */);
  mClipboardAtom = XInternAtom(mDisplay, "CLIPBOARD", 0 /* False */);
  mUTF8StringAtom = XInternAtom(mDisplay, "UTF8_STRING", 0 /* False */);
  mSelDataAtom = XInternAtom(mDisplay, "IPLUG2_SEL_DATA", 0 /* False */);
  mXdndAware = XInternAtom(mDisplay, "XdndAware", 0);
  mXdndEnter = XInternAtom(mDisplay, "XdndEnter", 0);
  mXdndPosition = XInternAtom(mDisplay, "XdndPosition", 0);
  mXdndStatus = XInternAtom(mDisplay, "XdndStatus", 0);
  mXdndLeave = XInternAtom(mDisplay, "XdndLeave", 0);
  mXdndDrop = XInternAtom(mDisplay, "XdndDrop", 0);
  mXdndFinished = XInternAtom(mDisplay, "XdndFinished", 0);
  mXdndSelection = XInternAtom(mDisplay, "XdndSelection", 0);
  mXdndTypeList = XInternAtom(mDisplay, "XdndTypeList", 0);
  mXdndActionCopy = XInternAtom(mDisplay, "XdndActionCopy", 0);
  mUriList = XInternAtom(mDisplay, "text/uri-list", 0);
  mDropProperty = XInternAtom(mDisplay, "IPLUG2_DROP_DATA", 0);
  const unsigned long xdndVersion = 5;
  XChangeProperty(mDisplay, mPlugWnd, mXdndAware, XA_ATOM, 32, PropModeReplace, reinterpret_cast<const unsigned char*>(&xdndVersion), 1);

  // WM protocols and size hints are only appropriate for top-level windows.
  // When embedded in a host parent, setting them causes the window manager to
  // adopt and reparent the plugin window, making it appear as a second window.
  bool isTopLevel = (mParentWnd == (Window)XRootWindow(mDisplay, DefaultScreen(mDisplay)));
  if (isTopLevel)
  {
    XSetWMProtocols(mDisplay, mPlugWnd, &mWMDeleteMessage, 1);

    // Tell the WM the fixed (or resizable) size range so it doesn't arbitrarily
    // resize the plugin window to the screen dimensions on first map.
    XSizeHints hints = {};
    hints.flags = PSize | PMinSize;
    hints.width = WindowWidth();
    hints.height = WindowHeight();
    hints.min_width = WindowWidth();
    hints.min_height = WindowHeight();
    XSetWMNormalHints(mDisplay, mPlugWnd, &hints);
  }

  XMapWindow(mDisplay, mPlugWnd);
  XFlush(mDisplay);

#ifdef IGRAPHICS_GL
  bool initialized = false;
  {
    ScopedGLContext scopedGLCtx{this};
    if (glXGetCurrentContext() == mGLContext && glXGetCurrentDrawable() == mPlugWnd)
    {
      gladLoadGL();
      OnViewInitialized(nullptr);
      GetDelegate()->LayoutUI(this);
      SetAllControlsDirty();
      DrawResize();
      initialized = true;
    }
  }
  if (!initialized)
  {
    DestroyGLContext();
    if (mGLColormap)
    {
      XFreeColormap(mDisplay, mGLColormap);
      mGLColormap = 0;
    }
    XDestroyWindow(mDisplay, mPlugWnd);
    mPlugWnd = 0;
    XCloseDisplay(mDisplay);
    mDisplay = nullptr;
    return nullptr;
  }
#else
  OnViewInitialized(nullptr);
  GetDelegate()->LayoutUI(this);
  SetAllControlsDirty();
  DrawResize();
#endif

  CreateInputContext();
  UpdateTooltips();
  GetDelegate()->OnUIOpen();

  // Mark dirty so the first timer tick renders immediately, even if no
  // Expose event arrives before the initial frame.
  mNeedsRedraw = true;
  if (!StartTimer())
    return nullptr;

  return (void*)(uintptr_t)mPlugWnd;
}

void IGraphicsLinux::CloseWindow()
{
  if (IsInPlatformCallback())
  {
    RequestWindowClose();
    return;
  }
  StopTimer();
  ResetDrag();
  HideTooltip();
  if (!mDisplay)
    return;

  {
    ScopedGLContext scopedGLCtx{this};
    OnViewDestroyed();
  }

#ifdef IGRAPHICS_GL
  DestroyGLContext();
#endif

  if (mBlankCursor)
  {
    XFreeCursor(mDisplay, mBlankCursor);
    mBlankCursor = 0;
  }

  DestroyInputContext();
  if (mTooltipFont)
  {
    XFreeFontSet(mDisplay, mTooltipFont);
    mTooltipFont = nullptr;
  }
  if (mPlugWnd)
  {
    XDestroyWindow(mDisplay, mPlugWnd);
    mPlugWnd = 0;
  }

#ifdef IGRAPHICS_GL
  if (mGLColormap)
  {
    XFreeColormap(mDisplay, mGLColormap);
    mGLColormap = 0;
  }
#endif

  XCloseDisplay(mDisplay);
  mDisplay = nullptr;
}

void IGraphicsLinux::ResetDrag()
{
  mDragSource = 0;
  mDragSupported = false;
}

void IGraphicsLinux::HideTooltip()
{
  if (mDisplay && mTooltipWindow)
  {
    XDestroyWindow(mDisplay, mTooltipWindow);
    mTooltipWindow = 0;
  }
}

void IGraphicsLinux::UpdateTooltips()
{
  HideTooltip();
  mTooltipControl = -1;
  mTooltipHoverTime = HoverMillis();
}

void IGraphicsLinux::ShowTooltip(float x, float y)
{
  if (!mDisplay || !mPlugWnd || !TooltipsEnabled() || mTooltipControl < 0 || mTooltipWindow)
    return;
  IControl* pControl = GetControl(mTooltipControl);
  if (!pControl || pControl->IsHidden() || !pControl->GetTooltip() || !*pControl->GetTooltip())
    return;
  if (!mTooltipFont)
  {
    char** missing = nullptr;
    int count = 0;
    char* defaultString = nullptr;
    mTooltipFont = XCreateFontSet(mDisplay, "-*-sans-medium-r-normal--14-*-*-*-*-*-*-*", &missing, &count, &defaultString);
    if (missing)
      XFreeStringList(missing);
    // Minimal Xvfb installations (and some desktops) have no "sans" XLFD.
    // The core fixed font is present with the standard X11 font packages.
    if (!mTooltipFont)
    {
      missing = nullptr;
      mTooltipFont = XCreateFontSet(mDisplay, "fixed", &missing, &count, &defaultString);
      if (missing)
        XFreeStringList(missing);
    }
  }
  if (!mTooltipFont)
    return;
  const char* label = pControl->GetTooltip();
  const int len = static_cast<int>(strlen(label));
  XRectangle ink{}, logical{};
  Xutf8TextExtents(mTooltipFont, label, len, &ink, &logical);
  const int width = std::max(20, static_cast<int>(logical.width) + 12);
  const int height = std::max(20, static_cast<int>(logical.height) + 8);
  int rootX = 0, rootY = 0;
  Window child = 0;
  XTranslateCoordinates(mDisplay, mPlugWnd, DefaultRootWindow(mDisplay), static_cast<int>(x * GetTotalScale()) + 12, static_cast<int>(y * GetTotalScale()) + 20, &rootX, &rootY, &child);
  const int screen = DefaultScreen(mDisplay);
  rootX = std::max(0, std::min(rootX, DisplayWidth(mDisplay, screen) - width));
  rootY = std::max(0, std::min(rootY, DisplayHeight(mDisplay, screen) - height));
  XSetWindowAttributes attrs{};
  attrs.override_redirect = 1;
  attrs.background_pixel = WhitePixel(mDisplay, screen);
  attrs.event_mask = ExposureMask;
  mTooltipWindow =
    XCreateWindow(mDisplay, DefaultRootWindow(mDisplay), rootX, rootY, width, height, 1, CopyFromParent, InputOutput, CopyFromParent, CWOverrideRedirect | CWBackPixel | CWEventMask, &attrs);
  if (mTooltipWindow)
    XMapRaised(mDisplay, mTooltipWindow);
}

void IGraphicsLinux::DrawTooltip()
{
  if (!mTooltipWindow || mTooltipControl < 0 || !mTooltipFont)
    return;
  IControl* pControl = GetControl(mTooltipControl);
  if (!pControl || !pControl->GetTooltip())
    return;
  const char* label = pControl->GetTooltip();
  XRectangle ink{}, logical{};
  Xutf8TextExtents(mTooltipFont, label, static_cast<int>(strlen(label)), &ink, &logical);
  XWindowAttributes attrs{};
  if (!XGetWindowAttributes(mDisplay, mTooltipWindow, &attrs))
    return;
  GC gc = XCreateGC(mDisplay, mTooltipWindow, 0, nullptr);
  XSetForeground(mDisplay, gc, BlackPixel(mDisplay, DefaultScreen(mDisplay)));
  XDrawRectangle(mDisplay, mTooltipWindow, gc, 0, 0, attrs.width - 1, attrs.height - 1);
  Xutf8DrawString(mDisplay, mTooltipWindow, mTooltipFont, gc, 6, 4 - logical.y, label, static_cast<int>(strlen(label)));
  XFreeGC(mDisplay, gc);
}

void IGraphicsLinux::PlatformResize(bool parentHasResized)
{
  // When handling a ConfigureNotify, the X server already sized our window.
  // Calling XResizeWindow here would create an oscillation.
  if (mInConfigureNotify)
    return;

  if (mDisplay && mPlugWnd)
  {
    // X11 window must be in physical pixels (WindowWidth/Height are logical).
    auto w = (unsigned)(WindowWidth() * GetScreenScale());
    auto h = (unsigned)(WindowHeight() * GetScreenScale());
    XResizeWindow(mDisplay, mPlugWnd, w, h);
  }
}

void IGraphicsLinux::SetScreenScaleFromHost(float scale)
{
  if (scale == GetScreenScale())
    return;
  if (mDisplay && mPlugWnd)
  {
    // Complete older native resize requests before changing the coordinate
    // conversion. Their ConfigureNotify events describe the previous scale.
    XSync(mDisplay, 0);
    XEvent event;
    while (XCheckTypedWindowEvent(mDisplay, mPlugWnd, ConfigureNotify, &event))
    {
    }
  }
  SetScreenScale(scale);
}

void IGraphicsLinux::DrawResize()
{
  std::lock_guard<std::recursive_mutex> lock(GetDelegate()->GfxMutex());
  IGRAPHICS_DRAW_CLASS::DrawResize();
}

void IGraphicsLinux::ResizeFromHost(int width, int height)
{
  std::lock_guard<std::recursive_mutex> lock(GetDelegate()->GfxMutex());
  if (width <= 0 || height <= 0)
    return;

  int designWidth, designHeight;
  float scale;
  CalculateHostResize(width, height, designWidth, designHeight, scale);
  Resize(designWidth, designHeight, scale, false);
}

void IGraphicsLinux::CalculateHostResize(int width, int height, int& designWidth, int& designHeight, float& scale)
{
  const float screenScale = GetScreenScale();
  const auto pixels = [screenScale](int dimension, float candidateScale) { return PhysicalWindowDimension(dimension, candidateScale, screenScale); };
  if (GetResizerMode() == EUIResizerMode::Scale)
  {
    designWidth = Width();
    designHeight = Height();
    GetDelegate()->ConstrainEditorResize(designWidth, designHeight);

    const auto fits = [&](float candidateScale) { return pixels(designWidth, candidateScale) <= width && pixels(designHeight, candidateScale) <= height; };
    float lo = ConstrainDrawScale(0.f);
    float hi = ConstrainDrawScale(std::numeric_limits<float>::max());
    if (fits(hi))
      lo = hi;
    else if (fits(lo))
    {
      // Find the last representable scale that fits the requested pixel bounds.
      // Inverting their lower edges loses a pixel at each integer conversion.
      // Querying the resulting dimensions admits this same scale again.
      for (;;)
      {
        const float mid = lo + (hi - lo) * .5f;
        if (mid == lo || mid == hi)
          break;
        if (fits(mid))
          lo = mid;
        else
          hi = mid;
      }
    }
    scale = lo;
    // Keep exact scales such as 1.5 when they produce the same pixel dimensions.
    const float nominalScale = ConstrainDrawScale(std::min(width / screenScale / designWidth, height / screenScale / designHeight));
    if (pixels(designWidth, nominalScale) == pixels(designWidth, scale) && pixels(designHeight, nominalScale) == pixels(designHeight, scale))
      scale = nominalScale;
  }
  else
  {
    scale = ConstrainDrawScale(GetDrawScale());
    // Use the same upper-bound search for integer design pixels in Size mode.
    const auto fitDimension = [&](int requested, int maximum) {
      int lo = 0, hi = maximum;
      while (lo < hi)
      {
        const int mid = hi - (hi - lo) / 2;
        if (pixels(mid, scale) <= requested)
          lo = mid;
        else
          hi = mid - 1;
      }
      const int nominal = static_cast<int>(std::lround(std::min<double>(maximum, requested / screenScale / scale)));
      return pixels(nominal, scale) == pixels(lo, scale) ? nominal : lo;
    };
    designWidth = fitDimension(width, GetDelegate()->GetMaxWidth());
    designHeight = fitDimension(height, GetDelegate()->GetMaxHeight());
    // Editor constraints describe dimensions before drawing/display scaling.
    GetDelegate()->ConstrainEditorResize(designWidth, designHeight);
  }
}

void IGraphicsLinux::ConstrainHostResize(int& width, int& height)
{
  std::lock_guard<std::recursive_mutex> lock(GetDelegate()->GfxMutex());
  int designWidth, designHeight;
  float scale;
  CalculateHostResize(width, height, designWidth, designHeight, scale);
  width = static_cast<int>(PhysicalWindowDimension(designWidth, scale, GetScreenScale()));
  height = static_cast<int>(PhysicalWindowDimension(designHeight, scale, GetScreenScale()));
}

void IGraphicsLinux::ForceEndUserEdit() {}

void IGraphicsLinux::HideMouseCursor(bool hide, bool lock)
{
  if (!mDisplay || !mPlugWnd)
    return;

  if (hide)
  {
    if (!mBlankCursor)
    {
      static const char emptyData[] = {0, 0, 0, 0, 0, 0, 0, 0};
      XColor black = {};
      Pixmap emptyBitmap = XCreateBitmapFromData(mDisplay, mPlugWnd, emptyData, 8, 8);
      mBlankCursor = XCreatePixmapCursor(mDisplay, emptyBitmap, emptyBitmap, &black, &black, 0, 0);
      XFreePixmap(mDisplay, emptyBitmap);
    }
    XDefineCursor(mDisplay, mPlugWnd, mBlankCursor);
    mCursorHidden = true;
    mCursorLock = lock;
    if (lock)
    {
      mHiddenCursorX = mCursorX;
      mHiddenCursorY = mCursorY;
    }
  }
  else
  {
    XUndefineCursor(mDisplay, mPlugWnd);
    if (mCursorLock)
      MoveMouseCursor(mHiddenCursorX, mHiddenCursorY);
    mCursorHidden = false;
    mCursorLock = false;
  }
}

void IGraphicsLinux::MoveMouseCursor(float x, float y)
{
  if (mDisplay && mPlugWnd)
  {
    const float scale = GetTotalScale();
    XWarpPointer(mDisplay, 0 /* None */, mPlugWnd, 0, 0, 0, 0, (int)(x * scale), (int)(y * scale));
  }
}

ECursor IGraphicsLinux::SetMouseCursor(ECursor cursorType)
{
  if (!mDisplay || !mPlugWnd)
    return cursorType;

  unsigned int shape = XC_left_ptr;
  switch (cursorType)
  {
  case ECursor::ARROW:
    shape = XC_left_ptr;
    break;
  case ECursor::IBEAM:
    shape = XC_xterm;
    break;
  case ECursor::WAIT:
    shape = XC_watch;
    break;
  case ECursor::CROSS:
    shape = XC_crosshair;
    break;
  case ECursor::UPARROW:
    shape = XC_sb_up_arrow;
    break;
  case ECursor::SIZENWSE:
    shape = XC_sizing;
    break;
  case ECursor::SIZENESW:
    shape = XC_sizing;
    break;
  case ECursor::SIZEWE:
    shape = XC_sb_h_double_arrow;
    break;
  case ECursor::SIZENS:
    shape = XC_sb_v_double_arrow;
    break;
  case ECursor::SIZEALL:
    shape = XC_fleur;
    break;
  case ECursor::INO:
    shape = XC_X_cursor;
    break;
  case ECursor::HAND:
    shape = XC_hand2;
    break;
  case ECursor::APPSTARTING:
    shape = XC_watch;
    break;
  case ECursor::HELP:
    shape = XC_question_arrow;
    break;
  default:
    shape = XC_left_ptr;
    break;
  }

  Cursor cursor = XCreateFontCursor(mDisplay, shape);
  XDefineCursor(mDisplay, mPlugWnd, cursor);
  XFreeCursor(mDisplay, cursor);
  return IGraphics::SetMouseCursor(cursorType);
}

void IGraphicsLinux::GetMouseLocation(float& x, float& y) const
{
  if (!mDisplay || !mPlugWnd)
  {
    x = y = 0.f;
    return;
  }

  Window root, child;
  int rootX, rootY, winX, winY;
  unsigned int mask;
  if (!XQueryPointer(mDisplay, mPlugWnd, &root, &child, &rootX, &rootY, &winX, &winY, &mask))
  {
    x = y = 0.f;
    return;
  }
  const float scale = GetTotalScale();
  x = winX / scale;
  y = winY / scale;
}

EMsgBoxResult IGraphicsLinux::ShowMessageBox(const char* str, const char* title, EMsgBoxType type, IMsgBoxCompletionHandlerFunc completionHandler)
{
  auto complete = [&](EMsgBoxResult result) {
    if (completionHandler)
      completionHandler(result);
    return result;
  };
  GTK3* pGTK = LoadGTK3();
  const bool customButtons = type == kMB_YESNOCANCEL || type == kMB_RETRYCANCEL;
  if (!pGTK || !pGTK->message_dialog_new || (customButtons && !pGTK->dialog_add_button))
  {
    fprintf(stderr, "Cannot show message box: required GTK functions are unavailable\n");
    return complete(kCANCEL);
  }

  int gtkMsgType = kGTK_MESSAGE_INFO;
  int gtkBtnType = kGTK_BUTTONS_OK;
  switch (type)
  {
  case kMB_OK:
    break;
  case kMB_OKCANCEL:
    gtkMsgType = kGTK_MESSAGE_QUESTION;
    gtkBtnType = kGTK_BUTTONS_OK_CANCEL;
    break;
  case kMB_YESNO:
    gtkMsgType = kGTK_MESSAGE_QUESTION;
    gtkBtnType = kGTK_BUTTONS_YES_NO;
    break;
  case kMB_YESNOCANCEL:
  case kMB_RETRYCANCEL:
    gtkMsgType = kGTK_MESSAGE_QUESTION;
    gtkBtnType = kGTK_BUTTONS_NONE;
    break;
  default:
    fprintf(stderr, "Cannot show message box: unsupported button type %d\n", type);
    return complete(kCANCEL);
  }

  void* dialog = pGTK->message_dialog_new(nullptr, 0, gtkMsgType, gtkBtnType, "%s", str ? str : "");
  if (!dialog)
  {
    fprintf(stderr, "Could not create the message box\n");
    return complete(kCANCEL);
  }
  if (type == kMB_YESNOCANCEL)
  {
    pGTK->dialog_add_button(dialog, "_Cancel", kGTK_RESPONSE_CANCEL);
    pGTK->dialog_add_button(dialog, "_No", kGTK_RESPONSE_NO);
    pGTK->dialog_add_button(dialog, "_Yes", kGTK_RESPONSE_YES);
  }
  else if (type == kMB_RETRYCANCEL)
  {
    pGTK->dialog_add_button(dialog, "_Cancel", kGTK_RESPONSE_CANCEL);
    pGTK->dialog_add_button(dialog, "_Retry", kGTK_RESPONSE_RETRY);
  }
  if (title && pGTK->window_set_title)
    pGTK->window_set_title(dialog, title);
  GtkPresentDialog(pGTK, dialog);

  int resp = pGTK->dialog_run(dialog);
  pGTK->widget_destroy(dialog);
  GtkDrainEvents(pGTK);

  EMsgBoxResult result = kCANCEL;
  switch (resp)
  {
  case kGTK_RESPONSE_OK:
    result = kOK;
    break;
  case kGTK_RESPONSE_YES:
    result = kYES;
    break;
  case kGTK_RESPONSE_NO:
    result = kNO;
    break;
  case kGTK_RESPONSE_CANCEL:
    result = kCANCEL;
    break;
  case kGTK_RESPONSE_RETRY:
    result = kRETRY;
    break;
  default:
    result = kCANCEL;
    break;
  }

  return complete(result);
}

bool IGraphicsLinux::RevealPathInExplorerOrFinder(WDL_String& path, bool select)
{
  // Open parent directory with xdg-open
  WDL_String dir(path);
  if (select)
  {
    // Truncate to the parent dir via WDL_String, not by writing '\0' through
    // Get() — that desyncs the cached length and is UB on the const buffer.
    if (strrchr(dir.Get(), '/'))
      dir.remove_filepart(false);
  }

  return LaunchDetached("xdg-open", dir.Get());
}

void IGraphicsLinux::PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* ext, IFileDialogCompletionHandlerFunc completionHandler)
{
  WDL_String outFile, outPath;
  auto complete = [&]() {
    fileName.Set(outFile.Get());
    path.Set(outPath.Get());
    if (completionHandler)
      completionHandler(fileName, path);
  };
  GTK3* pGTK = LoadGTK3();
  if (!pGTK)
  {
    fprintf(stderr, "Cannot show file dialog: GTK is unavailable\n");
    complete();
    return;
  }

  const bool isSave = (action == EFileAction::Save);
  const int gtkAction = isSave ? kGTK_FILE_CHOOSER_ACTION_SAVE : kGTK_FILE_CHOOSER_ACTION_OPEN;
  const char* acceptBtn = isSave ? "_Save" : "_Open";

  void* dialog = pGTK->file_chooser_dialog_new(isSave ? "Save File" : "Open File", nullptr, gtkAction, "_Cancel", kGTK_RESPONSE_CANCEL, acceptBtn, kGTK_RESPONSE_OK, nullptr);

  if (!dialog)
  {
    fprintf(stderr, "Could not create the file dialog\n");
    complete();
    return;
  }
  if (!GtkAddFileFilters(pGTK, dialog, ext))
  {
    pGTK->widget_destroy(dialog);
    GtkDrainEvents(pGTK);
    complete();
    return;
  }

  // Set initial directory
  if (path.GetLength() && pGTK->file_chooser_set_current_folder)
    pGTK->file_chooser_set_current_folder(dialog, path.Get());

  // For save dialogs, suggest filename
  if (isSave && fileName.GetLength() && pGTK->file_chooser_set_current_name)
    pGTK->file_chooser_set_current_name(dialog, fileName.Get());
  GtkPresentDialog(pGTK, dialog);

  if (pGTK->dialog_run(dialog) == kGTK_RESPONSE_OK)
  {
    char* chosen = pGTK->file_chooser_get_filename(dialog);
    if (chosen)
    {
      outFile.Set(chosen);
      const char* slash = strrchr(chosen, '/');
      if (slash)
        outPath.Set(chosen, (int)(slash - chosen + 1));
      free(chosen);
    }
    else
      fprintf(stderr, "File dialog accepted without a selected file\n");
  }

  pGTK->widget_destroy(dialog);
  GtkDrainEvents(pGTK);

  complete();
}

void IGraphicsLinux::PromptForDirectory(WDL_String& dir, IFileDialogCompletionHandlerFunc completionHandler)
{
  WDL_String outDir;
  auto complete = [&]() {
    dir.Set(outDir.Get());
    if (completionHandler)
      completionHandler(WDL_String{}, dir);
  };
  GTK3* pGTK = LoadGTK3();
  if (!pGTK)
  {
    fprintf(stderr, "Cannot show directory dialog: GTK is unavailable\n");
    complete();
    return;
  }

  void* dialog = pGTK->file_chooser_dialog_new("Select Folder", nullptr, kGTK_FILE_CHOOSER_ACTION_SELECT_FOLDER, "_Cancel", kGTK_RESPONSE_CANCEL, "_Select", kGTK_RESPONSE_OK, nullptr);

  if (!dialog)
  {
    fprintf(stderr, "Could not create the directory dialog\n");
    complete();
    return;
  }

  if (dir.GetLength() && pGTK->file_chooser_set_current_folder)
    pGTK->file_chooser_set_current_folder(dialog, dir.Get());
  GtkPresentDialog(pGTK, dialog);

  if (pGTK->dialog_run(dialog) == kGTK_RESPONSE_OK)
  {
    char* chosen = pGTK->file_chooser_get_filename(dialog);
    if (chosen)
    {
      outDir.Set(chosen);
      if (outDir.GetLength() && outDir.Get()[outDir.GetLength() - 1] != '/')
        outDir.Append("/");
      free(chosen);
    }
    else
      fprintf(stderr, "Directory dialog accepted without a selected directory\n");
  }

  pGTK->widget_destroy(dialog);
  GtkDrainEvents(pGTK);

  complete();
}

bool IGraphicsLinux::PromptForColor(IColor& color, const char* str, IColorPickerHandlerFunc func)
{
  GTK3* pGTK = LoadGTK3();
  if (!pGTK || !pGTK->color_chooser_dialog_new)
    return false;

  void* dialog = pGTK->color_chooser_dialog_new(str ? str : "Choose Color", nullptr);

  if (pGTK->color_chooser_set_rgba)
  {
    GdkRGBA rgba = {color.R / 255.0, color.G / 255.0, color.B / 255.0, color.A / 255.0};
    pGTK->color_chooser_set_rgba(dialog, &rgba);
  }
  GtkPresentDialog(pGTK, dialog);

  const bool accepted = (pGTK->dialog_run(dialog) == kGTK_RESPONSE_OK);

  if (accepted && pGTK->color_chooser_get_rgba)
  {
    GdkRGBA rgba = {};
    pGTK->color_chooser_get_rgba(dialog, &rgba);
    color.R = (int)(rgba.red * 255.0 + 0.5);
    color.G = (int)(rgba.green * 255.0 + 0.5);
    color.B = (int)(rgba.blue * 255.0 + 0.5);
    color.A = (int)(rgba.alpha * 255.0 + 0.5);
    if (func)
      func(color);
  }

  pGTK->widget_destroy(dialog);
  GtkDrainEvents(pGTK);
  return accepted;
}

bool IGraphicsLinux::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure) { return LaunchDetached("xdg-open", url); }

bool IGraphicsLinux::GetTextFromClipboard(WDL_String& str)
{
  std::lock_guard<std::recursive_mutex> lock(GetDelegate()->GfxMutex());
  if (!mDisplay || !mPlugWnd || !mClipboardAtom)
    return false;

  const Window owner = XGetSelectionOwner(mDisplay, mClipboardAtom);
  if (GetLinuxClipboard().GetOwnedText(owner, DisplayString(mDisplay), str))
    return true;
  if (!owner)
    return false;

  // Native clipboard owners in the host may need the same GTK main thread to
  // answer. Like modal dialogs, this wait can reenter host callbacks: defer any
  // editor teardown until the synchronous paste has returned to its caller.
  DispatchScope dispatch(mDispatchDepth);
  GTK3* pGTK = LoadGTK3();

  // Ask the clipboard owner to convert to UTF-8 and store in our scratch property
  XConvertSelection(mDisplay, mClipboardAtom, mUTF8StringAtom, mSelDataAtom, mPlugWnd, 0L /* CurrentTime */);
  XFlush(mDisplay);

  // Keep the event consumer locked out until we have read SelectionNotify.
  for (int i = 0; i < 50; ++i)
  {
    if (pGTK && pGTK->events_pending && pGTK->main_iteration_do && pGTK->events_pending())
      pGTK->main_iteration_do(0);
    if (mCloseRequested)
      return false;
    usleep(10000); // 10 ms

    XEvent ev;
    if (XCheckTypedWindowEvent(mDisplay, mPlugWnd, SelectionNotify, &ev))
    {
      if (ev.xselection.selection != mClipboardAtom)
        continue;

      if (ev.xselection.property == 0 /* None */)
        return false; // owner couldn't convert

      Atom type;
      int fmt;
      unsigned long nitems, remaining;
      unsigned char* data = nullptr;

      const int status = XGetWindowProperty(mDisplay, mPlugWnd, mSelDataAtom, 0, 65536, 1 /* delete after read */, AnyPropertyType, &type, &fmt, &nitems, &remaining, &data);
      // Require remaining == 0 so we never report success on a truncated
      // read, and bound nitems before the int cast to avoid UB.
      if (status == 0 && fmt == 8 && (type == mUTF8StringAtom || type == XA_STRING) && data && remaining == 0 && nitems <= 0x7FFFFFFFUL)
      {
        str.Set((const char*)data, (int)nitems);
        XFree(data);
        return true;
      }
      if (data)
        XFree(data);
      return false;
    }
  }
  fprintf(stderr, "Timed out waiting for the X11 clipboard owner\n");
  return false;
}

bool IGraphicsLinux::SetTextInClipboard(const char* str)
{
  std::lock_guard<std::recursive_mutex> lock(GetDelegate()->GfxMutex());
  if (!mDisplay || !mPlugWnd || !mClipboardAtom)
    return false;

  return GetLinuxClipboard().SetText(str, DisplayString(mDisplay));
}

// State passed through GTK menu-item activation callbacks.
struct GtkMenuState
{
  IPopupMenu* pMenu = nullptr;
  int chosenIdx = -1;
};

/** Recursively builds a GtkMenu from an IPopupMenu.
 *  Each leaf item's "activate" signal stores the selected index. */
static void* BuildGtkMenu(GTK3* pGTK, IPopupMenu& menu, GtkMenuState* pState)
{
  void* gtkMenu = pGTK->menu_new();

  for (int i = 0; i < menu.NItems(); ++i)
  {
    IPopupMenu::Item* pItem = menu.GetItem(i);
    void* gtkItem = nullptr;

    if (pItem->GetIsSeparator())
    {
      gtkItem = pGTK->separator_menu_item_new();
    }
    else if (pItem->GetChecked() && pGTK->check_menu_item_new_with_label)
    {
      gtkItem = pGTK->check_menu_item_new_with_label(pItem->GetText());
      if (pGTK->check_menu_item_set_active)
        pGTK->check_menu_item_set_active(gtkItem, 1);
    }
    else
    {
      gtkItem = pGTK->menu_item_new_with_label(pItem->GetText());
    }

    if (!gtkItem)
      continue;

    // Disabled or title items are insensitive
    if (!pItem->GetEnabled() || pItem->GetIsTitle())
    {
      if (pGTK->widget_set_sensitive)
        pGTK->widget_set_sensitive(gtkItem, 0);
    }

    // Submenus
    if (pItem->GetSubmenu() && pGTK->menu_item_set_submenu)
    {
      void* sub = BuildGtkMenu(pGTK, *pItem->GetSubmenu(), pState);
      pGTK->menu_item_set_submenu(gtkItem, sub);
    }
    else if (pItem->GetEnabled() && !pItem->GetIsTitle() && !pItem->GetIsSeparator() && pGTK->signal_connect_data)
    {
      // Store the menu pointer and index in the state for the callback.
      // We use a small lambda-like static function with user data.
      struct ActivateData
      {
        GtkMenuState* state;
        IPopupMenu* menu;
        int idx;
      };
      auto* ad = new ActivateData{pState, &menu, i};

      // GCallback signature: void callback(GtkMenuItem*, gpointer)
      auto activateCb = +[](void*, void* userData) {
        auto* d = static_cast<ActivateData*>(userData);
        d->state->pMenu = d->menu;
        d->state->chosenIdx = d->idx;
      };

      // GDestroyNotify frees the ActivateData when the signal is disconnected.
      auto destroyCb = +[](void* userData) { delete static_cast<ActivateData*>(userData); };

      pGTK->signal_connect_data(gtkItem, "activate", (void*)activateCb, ad, (void*)destroyCb, 0);
    }

    pGTK->menu_shell_append(gtkMenu, gtkItem);
  }

  if (pGTK->widget_show_all)
    pGTK->widget_show_all(gtkMenu);

  return gtkMenu;
}

IPopupMenu* IGraphicsLinux::CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT bounds, bool& isAsync)
{
  isAsync = false;

  GTK3* pGTK = LoadGTK3();
  if (!pGTK || !pGTK->menu_new || !pGTK->menu_item_new_with_label || !pGTK->menu_shell_append || !pGTK->main || !pGTK->main_quit)
    return nullptr;

  GtkMenuState state;
  void* gtkMenu = BuildGtkMenu(pGTK, menu, &state);

  // Quit the GTK main loop when the menu is dismissed (deactivate signal).
  if (pGTK->signal_connect_data)
  {
    auto deactivateCb = +[](void*, void*) {
      // LoadGTK3() returns cached pointer; safe to call again
      GTK3* pGTK = LoadGTK3();
      if (pGTK && pGTK->main_quit)
        pGTK->main_quit();
    };
    pGTK->signal_connect_data(gtkMenu, "deactivate", (void*)deactivateCb, nullptr, nullptr, 0);
  }

  // The button press that opened the menu holds an implicit pointer grab on
  // our X connection. GTK needs its own grab to show the menu and silently
  // abandons the popup when it cannot get one, so release ours first.
  if (mDisplay)
  {
    XUngrabPointer(mDisplay, 0 /* CurrentTime */);
    XSync(mDisplay, 0);
  }

  // Position the popup menu anchored to the control's bounding rectangle.
  // gtk_menu_popup_at_rect is the non-deprecated GTK3 API (since 3.22).
  // It needs a GdkWindow — we create a "foreign" wrapper around mPlugWnd.
  // Falls back to gtk_menu_popup_at_pointer(NULL) on older GTK or if the
  // foreign window can't be created (e.g. Wayland without X11 backend).
  bool popped = false;

  if (pGTK->menu_popup_at_rect && pGTK->gdk_display_get_default && pGTK->gdk_x11_window_foreign && mPlugWnd)
  {
    void* gdkDisplay = pGTK->gdk_display_get_default();
    if (gdkDisplay)
    {
      void* gdkWindow = pGTK->gdk_x11_window_foreign(gdkDisplay, static_cast<unsigned long>(mPlugWnd));

      if (gdkWindow)
      {
        // Control coordinates to window pixels (draw and screen scale), then
        // to GDK units, which GDK multiplies by its own window scale.
        using fn_window_scale = int (*)(void*);
        auto window_scale = (fn_window_scale)dlsym(pGTK->handle, "gdk_window_get_scale_factor");
        float s = GetTotalScale() / static_cast<float>(window_scale ? std::max(1, window_scale(gdkWindow)) : 1);
        GdkRectangle rect;
        rect.x = static_cast<int>(bounds.L * s);
        rect.y = static_cast<int>(bounds.B * s);
        rect.width = static_cast<int>(bounds.W() * s);
        rect.height = 1;

        pGTK->menu_popup_at_rect(gtkMenu, gdkWindow, &rect, kGDK_GRAVITY_NORTH_WEST, kGDK_GRAVITY_NORTH_WEST, nullptr);
        popped = true;

        if (pGTK->g_object_unref)
          pGTK->g_object_unref(gdkWindow);
      }
    }
  }

  if (!popped && pGTK->menu_popup_at_pointer)
    pGTK->menu_popup_at_pointer(gtkMenu, nullptr);

  // Run a nested GTK main loop — blocks until the menu closes. If GTK did not
  // show the menu (e.g. its pointer grab failed) "deactivate" never fires, and
  // the loop would block the application forever, so skip it.
  if (!pGTK->widget_get_mapped || pGTK->widget_get_mapped(gtkMenu))
    pGTK->main();
  else
    fprintf(stderr, "IGraphicsLinux: GTK did not show the popup menu\n");

  // GTK owns the menu widget tree; destroy it.
  pGTK->widget_destroy(gtkMenu);
  GtkDrainEvents(pGTK);

  if (state.pMenu && state.chosenIdx >= 0)
  {
    state.pMenu->SetChosenItemIdx(state.chosenIdx);

    if (state.pMenu->GetFunction())
      state.pMenu->ExecFunction();

    return state.pMenu;
  }

  return nullptr;
}

void IGraphicsLinux::CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str)
{
  // GTK dialog flags: GTK_DIALOG_MODAL = 1, GTK_DIALOG_DESTROY_WITH_PARENT = 2
  static constexpr int kGTK_DIALOG_MODAL = 1;

  GTK3* pGTK = LoadGTK3();
  if (!pGTK || !pGTK->dialog_new_with_buttons || !pGTK->dialog_get_content_area || !pGTK->entry_new || !pGTK->entry_get_text || !pGTK->box_pack_start)
  {
    ClearInTextEntryControl();
    return;
  }

  void* dialog = pGTK->dialog_new_with_buttons("Edit Value", nullptr, kGTK_DIALOG_MODAL, "_Cancel", kGTK_RESPONSE_CANCEL, "_OK", kGTK_RESPONSE_OK, nullptr);

  void* entry = pGTK->entry_new();
  if (pGTK->entry_set_text && str)
    pGTK->entry_set_text(entry, str);
  if (pGTK->entry_set_max_length && length > 0)
    pGTK->entry_set_max_length(entry, length);

  // Wire Enter key in the entry to trigger the OK response
  if (pGTK->signal_connect_data && pGTK->dialog_response)
  {
    auto activateCb = +[](void*, void* userData) {
      GTK3* pGTK = LoadGTK3();
      if (pGTK && pGTK->dialog_response)
        pGTK->dialog_response(userData, kGTK_RESPONSE_OK);
    };
    pGTK->signal_connect_data(entry, "activate", (void*)activateCb, dialog, nullptr, 0);
  }

  void* contentArea = pGTK->dialog_get_content_area(dialog);
  pGTK->box_pack_start(contentArea, entry, 1, 1, 0);

  if (pGTK->widget_show_all)
    pGTK->widget_show_all(dialog);
  GtkPresentDialog(pGTK, dialog);

  int response = pGTK->dialog_run(dialog);

  if (response == kGTK_RESPONSE_OK)
  {
    const char* result = pGTK->entry_get_text(entry);
    SetControlValueAfterTextEdit(result ? result : "");
  }
  else
  {
    ClearInTextEntryControl();
  }

  pGTK->widget_destroy(dialog);
  GtkDrainEvents(pGTK);
}

PlatformFontPtr IGraphicsLinux::LoadPlatformFont(const char* fontID, const char* fileNameOrResID)
{
  WDL_String fullPath;
  const EResourceLocation loc = LocateResource(fileNameOrResID, "ttf", fullPath, GetBundleID(), nullptr, nullptr);
  if (loc == EResourceLocation::kNotFound)
    return nullptr;

  return PlatformFontPtr(new LinuxFileFont(fullPath.Get(), false));
}

PlatformFontPtr IGraphicsLinux::LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style)
{
  Fontconfig* pFontconfig = LoadFontconfig();
  if (!pFontconfig)
    return nullptr;

  _FcPattern* pattern = pFontconfig->PatternCreate();
  if (!pattern)
    return nullptr;

  pFontconfig->PatternAddString(pattern, "family", (const unsigned char*)fontName);
  pFontconfig->PatternAddInteger(pattern, "weight", style == ETextStyle::Bold ? kFC_WEIGHT_BOLD : kFC_WEIGHT_REGULAR);
  pFontconfig->PatternAddInteger(pattern, "slant", style == ETextStyle::Italic ? kFC_SLANT_ITALIC : kFC_SLANT_ROMAN);

  pFontconfig->ConfigSubstitute(nullptr, pattern, kFC_MatchPattern);
  pFontconfig->DefaultSubstitute(pattern);

  int result = 1; // FcResultNoMatch
  _FcPattern* matched = pFontconfig->FontMatch(nullptr, pattern, &result);
  pFontconfig->PatternDestroy(pattern);

  if (!matched)
    return nullptr;

  PlatformFontPtr ret;
  unsigned char* path = nullptr;
  if (pFontconfig->PatternGetString(matched, "file", 0, &path) == kFC_ResultMatch && path)
    ret = PlatformFontPtr(new LinuxFileFont((const char*)path, true));

  pFontconfig->PatternDestroy(matched);
  return ret;
}

PlatformFontPtr IGraphicsLinux::LoadPlatformFont(const char* fontID, void* pData, int dataSize) { return PlatformFontPtr(new LinuxMemFont(pData, dataSize)); }

void IGraphicsLinux::CachePlatformFont(const char* fontID, const PlatformFontPtr& font)
{
  // NanoVG caches loaded fonts in its own atlas; no additional platform cache needed.
}

