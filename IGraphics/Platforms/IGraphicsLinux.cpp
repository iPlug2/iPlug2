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

