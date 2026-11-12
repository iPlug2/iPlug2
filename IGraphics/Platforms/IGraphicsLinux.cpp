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

unsigned long HoverMillis()
{
  return static_cast<unsigned long>(std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now().time_since_epoch()).count());
}

std::vector<std::string> ParseDroppedFiles(const unsigned char* data, size_t length)
{
  std::vector<std::string> paths;
  const std::string list(reinterpret_cast<const char*>(data), length);
  for (size_t start = 0; start < list.size();)
  {
    const size_t end = list.find('\n', start);
    const std::string line = list.substr(start, end == std::string::npos ? end : end - start);
    start = end == std::string::npos ? list.size() : end + 1;
    if (line.empty() || line[0] == '#') continue;
    const std::string uri = line.back() == '\r' ? line.substr(0, line.size() - 1) : line;
    if (uri.compare(0, 7, "file://") != 0) continue;
    size_t pathStart = 7;
    if (uri.compare(pathStart, 10, "localhost/") == 0) pathStart += 9;
    if (pathStart >= uri.size() || uri[pathStart] != '/') continue;
    std::string path;
    bool valid = true;
    for (size_t i = pathStart; i < uri.size(); ++i)
    {
      if (uri[i] != '%') { path += uri[i]; continue; }
      if (i + 2 >= uri.size()) { valid = false; break; }
      auto hex = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
      };
      const int hi = hex(uri[i + 1]), lo = hex(uri[i + 2]);
      if (hi < 0 || lo < 0 || (hi == 0 && lo == 0)) { valid = false; break; }
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
      mWindow = XCreateSimpleWindow(mDisplay, DefaultRootWindow(mDisplay),
                                    0, 0, 1, 1, 0, 0, 0);
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
          else if (event.type == SelectionClear &&
                   XGetSelectionOwner(self->mDisplay, self->mClipboard) != self->mWindow)
            self->mText.Set("");
        }
      }
      pollfd fd {XConnectionNumber(self->mDisplay), POLLIN, 0};
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
    xcb_selection_notify_event_t response {};
    response.response_type = XCB_SELECTION_NOTIFY;
    response.requestor = request.requestor;
    response.selection = request.selection;
    response.target = request.target;
    response.time = request.time;
    const Atom property = request.property ? request.property : request.target;
    if (request.target == mTargets)
    {
      const xcb_atom_t targets[] = {static_cast<xcb_atom_t>(mTargets),
                                   static_cast<xcb_atom_t>(mUTF8), XCB_ATOM_STRING};
      if (!CheckReply(xcb_change_property_checked(mConnection, XCB_PROP_MODE_REPLACE,
                          request.requestor, property, XCB_ATOM_ATOM, 32, 3, targets),
                      "property write"))
        return;
      response.property = property;
    }
    else if (request.target == mUTF8 || request.target == XA_STRING)
    {
      if (!CheckReply(xcb_change_property_checked(mConnection, XCB_PROP_MODE_REPLACE,
                          request.requestor, property, request.target, 8,
                          mText.GetLength(), mText.Get()), "property write"))
        return;
      response.property = property;
    }
    char event[32] {};
    static_assert(sizeof(response) <= sizeof(event), "X11 selection event must fit the wire event");
    memcpy(event, &response, sizeof(response));
    CheckReply(xcb_send_event_checked(mConnection, 0, request.requestor, XCB_EVENT_MASK_NO_EVENT, event),
               "selection notification");
  }

  bool CheckReply(xcb_void_cookie_t cookie, const char* operation)
  {
    xcb_generic_error_t* error = xcb_request_check(mConnection, cookie);
    if (error)
    {
      fprintf(stderr, "X11 clipboard %s failed (error %u, resource 0x%x)\n",
              operation, error->error_code, error->resource_id);
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
  std::atomic<bool> mRunning {false};
  pthread_t mThread = 0;
};

static LinuxClipboard& GetLinuxClipboard()
{
  // A separate connection serves selections even while the host UI thread waits for a paste.
  static LinuxClipboard clipboard;
  return clipboard;
}
}

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

  IFontDataPtr GetFontData() override
  {
    return IFontDataPtr(new IFontData(mData.Get(), mData.GetSize(), 0));
  }

private:
  WDL_TypedBuf<unsigned char> mData;
};

// ---- Runtime GTK3 (optional — loaded via dlopen so the plugin works without GTK) ----

struct GdkRGBA { double red, green, blue, alpha; };
struct GdkRectangle { int x, y, width, height; };

// GdkGravity values for gtk_menu_popup_at_rect anchoring
static constexpr int kGDK_GRAVITY_NORTH_WEST = 1;
static constexpr int kGDK_GRAVITY_SOUTH_WEST = 7;

// Opaque GTK/GDK types — only used as void* through the function pointers below.
struct _GtkWidget;

// GTK response codes (subset we need)
static constexpr int kGTK_RESPONSE_OK     = -5;
static constexpr int kGTK_RESPONSE_CANCEL = -6;
static constexpr int kGTK_RESPONSE_YES    = -8;
static constexpr int kGTK_RESPONSE_NO     = -9;
static constexpr int kGTK_RESPONSE_RETRY  = 1;

// GTK file-chooser action codes
static constexpr int kGTK_FILE_CHOOSER_ACTION_OPEN          = 0;
static constexpr int kGTK_FILE_CHOOSER_ACTION_SAVE          = 1;
static constexpr int kGTK_FILE_CHOOSER_ACTION_SELECT_FOLDER = 2;

// GTK button presets used by message dialogs
static constexpr int kGTK_BUTTONS_NONE      = 0;
static constexpr int kGTK_BUTTONS_OK        = 1;
static constexpr int kGTK_BUTTONS_YES_NO    = 4;
static constexpr int kGTK_BUTTONS_OK_CANCEL = 5;

// GTK message-dialog icon types
static constexpr int kGTK_MESSAGE_INFO     = 0;
static constexpr int kGTK_MESSAGE_QUESTION = 3;

struct GTK3
{
  void* handle = nullptr;

  // Core
  using fn_init_check               = int   (*)(int*, char***);
  using fn_events_pending           = int   (*)();
  using fn_main_iteration_do        = int   (*)(int);
  // Dialogs
  using fn_dialog_run               = int   (*)(void*);
  using fn_dialog_add_button        = void* (*)(void*, const char*, int);
  using fn_widget_destroy           = void  (*)(void*);
  using fn_window_set_title         = void  (*)(void*, const char*);
  // File chooser
  using fn_file_chooser_dialog_new  = void* (*)(const char*, void*, int,
                                                const char*, ...);
  using fn_file_chooser_get_filename        = char* (*)(void*);
  using fn_file_chooser_set_current_folder  = int   (*)(void*, const char*);
  using fn_file_chooser_set_current_name    = void  (*)(void*, const char*);
  using fn_file_filter_new          = void* (*)();
  using fn_file_filter_set_name     = void  (*)(void*, const char*);
  using fn_file_filter_add_pattern  = void  (*)(void*, const char*);
  using fn_file_chooser_add_filter  = void  (*)(void*, void*);
  // Color chooser
  using fn_color_chooser_dialog_new = void* (*)(const char*, void*);
  using fn_color_chooser_get_rgba   = void  (*)(void*, GdkRGBA*);
  using fn_color_chooser_set_rgba   = void  (*)(void*, const GdkRGBA*);
  // Message dialog
  using fn_message_dialog_new       = void* (*)(void*, int, int, int,
                                                const char*, ...);
  // Popup menu
  using fn_menu_new                 = void* (*)();
  using fn_menu_item_new_with_label = void* (*)(const char*);
  using fn_check_menu_item_new_with_label = void* (*)(const char*);
  using fn_separator_menu_item_new  = void* (*)();
  using fn_menu_shell_append        = void  (*)(void*, void*);
  using fn_menu_item_set_submenu    = void  (*)(void*, void*);
  using fn_check_menu_item_set_active = void (*)(void*, int);
  using fn_widget_set_sensitive     = void  (*)(void*, int);
  using fn_widget_show_all          = void  (*)(void*);
  using fn_menu_popup_at_pointer    = void  (*)(void*, void*);
  // gtk_menu_popup_at_rect: positions menu relative to a GdkWindow rectangle.
  // args: menu, rect_window, rect(GdkRectangle*), rect_anchor, menu_anchor, trigger_event
  using fn_menu_popup_at_rect       = void  (*)(void*, void*, const void*,
                                                int, int, void*);
  using fn_main                     = void  (*)();
  using fn_main_quit                = void  (*)();
  // g_signal_connect is a macro over g_signal_connect_data in GObject
  using fn_signal_connect_data      = unsigned long (*)(void*, const char*,
                                                        void*, void*,
                                                        void*, int);
  // Window management (for dialog parenting)
  using fn_window_set_modal         = void  (*)(void*, int);
  using fn_window_present           = void  (*)(void*);
  // GDK helpers for popup menu positioning
  using fn_gdk_display_get_default  = void* (*)();
  using fn_gdk_x11_window_foreign   = void* (*)(void*, unsigned long);
  using fn_g_object_unref           = void  (*)(void*);
  // Entry dialog
  using fn_entry_new                = void* (*)();
  using fn_entry_set_text           = void  (*)(void*, const char*);
  using fn_entry_get_text           = const char* (*)(void*);
  using fn_entry_set_max_length     = void  (*)(void*, int);
  using fn_dialog_new_with_buttons  = void* (*)(const char*, void*, int,
                                                const char*, ...);
  using fn_dialog_get_content_area  = void* (*)(void*);
  using fn_dialog_response          = void  (*)(void*, int);
  using fn_box_pack_start           = void  (*)(void*, void*, int, int, unsigned);

  fn_init_check               init_check               = nullptr;
  fn_events_pending           events_pending           = nullptr;
  fn_main_iteration_do        main_iteration_do        = nullptr;
  fn_dialog_run               dialog_run               = nullptr;
  fn_dialog_add_button        dialog_add_button        = nullptr;
  fn_widget_destroy           widget_destroy           = nullptr;
  fn_window_set_title         window_set_title         = nullptr;
  fn_file_chooser_dialog_new  file_chooser_dialog_new  = nullptr;
  fn_file_chooser_get_filename       file_chooser_get_filename       = nullptr;
  fn_file_chooser_set_current_folder file_chooser_set_current_folder = nullptr;
  fn_file_chooser_set_current_name   file_chooser_set_current_name   = nullptr;
  fn_file_filter_new          file_filter_new          = nullptr;
  fn_file_filter_set_name     file_filter_set_name     = nullptr;
  fn_file_filter_add_pattern  file_filter_add_pattern  = nullptr;
  fn_file_chooser_add_filter  file_chooser_add_filter  = nullptr;
  fn_color_chooser_dialog_new color_chooser_dialog_new = nullptr;
  fn_color_chooser_get_rgba   color_chooser_get_rgba   = nullptr;
  fn_color_chooser_set_rgba   color_chooser_set_rgba   = nullptr;
  fn_message_dialog_new       message_dialog_new       = nullptr;
  fn_menu_new                 menu_new                 = nullptr;
  fn_menu_item_new_with_label menu_item_new_with_label = nullptr;
  fn_check_menu_item_new_with_label check_menu_item_new_with_label = nullptr;
  fn_separator_menu_item_new  separator_menu_item_new  = nullptr;
  fn_menu_shell_append        menu_shell_append        = nullptr;
  fn_menu_item_set_submenu    menu_item_set_submenu    = nullptr;
  fn_check_menu_item_set_active check_menu_item_set_active = nullptr;
  fn_widget_set_sensitive     widget_set_sensitive     = nullptr;
  fn_widget_show_all          widget_show_all          = nullptr;
  fn_menu_popup_at_pointer    menu_popup_at_pointer    = nullptr;
  fn_menu_popup_at_rect       menu_popup_at_rect       = nullptr;
  fn_main                     main                     = nullptr;
  fn_main_quit                main_quit                = nullptr;
  fn_signal_connect_data      signal_connect_data      = nullptr;
  fn_entry_new                entry_new                = nullptr;
  fn_entry_set_text           entry_set_text           = nullptr;
  fn_entry_get_text           entry_get_text           = nullptr;
  fn_entry_set_max_length     entry_set_max_length     = nullptr;
  fn_window_set_modal         window_set_modal         = nullptr;
  fn_window_present           window_present           = nullptr;
  fn_gdk_display_get_default  gdk_display_get_default  = nullptr;
  fn_gdk_x11_window_foreign   gdk_x11_window_foreign   = nullptr;
  fn_g_object_unref           g_object_unref           = nullptr;
  fn_dialog_new_with_buttons  dialog_new_with_buttons  = nullptr;
  fn_dialog_get_content_area  dialog_get_content_area  = nullptr;
  fn_dialog_response          dialog_response          = nullptr;
  fn_box_pack_start           box_pack_start           = nullptr;
};

#define LOAD_GTK_SYM(g, name) \
  g.name = (GTK3::fn_##name)dlsym(g.handle, "gtk_" #name)

static GTK3* LoadGTK3()
{
  static GTK3 gtk;
  static bool sTried = false;
  if (sTried)
    return gtk.handle ? &gtk : nullptr;
  sTried = true;

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
  // GDK/GLib symbols (not gtk_ prefixed — load directly)
  gtk.gdk_display_get_default = (GTK3::fn_gdk_display_get_default)
    dlsym(gtk.handle, "gdk_display_get_default");
  gtk.gdk_x11_window_foreign = (GTK3::fn_gdk_x11_window_foreign)
    dlsym(gtk.handle, "gdk_x11_window_foreign_new_for_display");
  gtk.g_object_unref = (GTK3::fn_g_object_unref)
    dlsym(gtk.handle, "g_object_unref");
  LOAD_GTK_SYM(gtk, dialog_new_with_buttons);
  LOAD_GTK_SYM(gtk, dialog_get_content_area);
  LOAD_GTK_SYM(gtk, dialog_response);
  LOAD_GTK_SYM(gtk, box_pack_start);

  if (!gtk.init_check || !gtk.dialog_run || !gtk.widget_destroy ||
      !gtk.file_chooser_dialog_new || !gtk.file_chooser_get_filename)
  {
    dlclose(gtk.handle);
    gtk.handle = nullptr;
    return nullptr;
  }

  if (!gtk.init_check(nullptr, nullptr))
  {
    dlclose(gtk.handle);
    gtk.handle = nullptr;
    return nullptr;
  }
  return &gtk;
}

static void GtkDrainEvents(GTK3* g)
{
  if (g->events_pending && g->main_iteration_do)
    while (g->events_pending())
      g->main_iteration_do(0 /* non-blocking */);
}

// Present a GTK dialog as modal so the WM grants it focus and input grabs.
// Without this, file-chooser dropdowns and color-picker sub-dialogs fail
// on compositing WMs (Mutter, KWin) because GTK can't acquire a GDK grab.
static void GtkPresentDialog(GTK3* g, void* dialog)
{
  if (g->window_set_modal)
    g->window_set_modal(dialog, 1);
  if (g->window_present)
    g->window_present(dialog);
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
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {}
    return true;
  }
  return false;
}

static bool GtkAddFileFilters(GTK3* g, void* dialog, const char* ext)
{
  WDL_String extensions(ext ? ext : "");
  char* context = nullptr;
  char* token = strtok_r(extensions.Get(), " \t\r\n", &context);
  if (!token)
    return true;

  if (!g->file_filter_new || !g->file_filter_add_pattern || !g->file_chooser_add_filter)
  {
    fprintf(stderr, "Cannot apply file dialog filters: required GTK functions are unavailable\n");
    return false;
  }

  void* filter = g->file_filter_new();
  if (!filter)
  {
    fprintf(stderr, "Could not create the file extension filter\n");
    return false;
  }
  if (g->file_filter_set_name)
    g->file_filter_set_name(filter, ext);
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
    g->file_filter_add_pattern(filter, pattern.Get());
    token = strtok_r(nullptr, " \t\r\n", &context);
  } while (token);
  g->file_chooser_add_filter(dialog, filter);

  void* all = g->file_filter_new();
  if (!all)
  {
    fprintf(stderr, "Could not create the all-files filter\n");
    return false;
  }
  if (g->file_filter_set_name)
    g->file_filter_set_name(all, "All Files");
  g->file_filter_add_pattern(all, "*");
  g->file_chooser_add_filter(dialog, all);
  return true;
}

// ---- Runtime fontconfig (optional — loaded via dlopen so plugins work without it) ----

// Fontconfig opaque types — only used through function pointers below.
struct _FcPattern;
struct _FcConfig;

// Fontconfig constants we need (match the enum values from fontconfig.h)
static constexpr int kFC_WEIGHT_REGULAR = 80;
static constexpr int kFC_WEIGHT_BOLD    = 200;
static constexpr int kFC_SLANT_ROMAN    = 0;
static constexpr int kFC_SLANT_ITALIC   = 100;
static constexpr int kFC_MatchPattern   = 0;   // FcMatchKind
static constexpr int kFC_ResultMatch    = 0;   // FcResult

struct Fontconfig
{
  void* handle = nullptr;

  using fn_PatternCreate     = _FcPattern* (*)();
  using fn_PatternAddString  = int (*)(_FcPattern*, const char*, const unsigned char*);
  using fn_PatternAddInteger = int (*)(_FcPattern*, const char*, int);
  using fn_ConfigSubstitute  = int (*)(_FcConfig*, _FcPattern*, int);
  using fn_DefaultSubstitute = void (*)(_FcPattern*);
  using fn_FontMatch         = _FcPattern* (*)(_FcConfig*, _FcPattern*, int*);
  using fn_PatternDestroy    = void (*)(_FcPattern*);
  using fn_PatternGetString  = int (*)(_FcPattern*, const char*, int, unsigned char**);

  fn_PatternCreate     PatternCreate     = nullptr;
  fn_PatternAddString  PatternAddString  = nullptr;
  fn_PatternAddInteger PatternAddInteger = nullptr;
  fn_ConfigSubstitute  ConfigSubstitute  = nullptr;
  fn_DefaultSubstitute DefaultSubstitute = nullptr;
  fn_FontMatch         FontMatch         = nullptr;
  fn_PatternDestroy    PatternDestroy    = nullptr;
  fn_PatternGetString  PatternGetString  = nullptr;
};

#define LOAD_FC_SYM(fc, name) \
  fc.name = (Fontconfig::fn_##name)dlsym(fc.handle, "Fc" #name)

static Fontconfig* LoadFontconfig()
{
  static Fontconfig fc;

  if (fc.handle || fc.PatternCreate)
    return fc.handle ? &fc : nullptr;

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

IGraphicsLinux::~IGraphicsLinux()
{
  CloseWindow();
}

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
      attributes = xcb_get_window_attributes_reply(connection,
        xcb_get_window_attributes(connection, static_cast<xcb_window_t>(parent)), &error);
    if (attributes && !error)
      mParentWnd = static_cast<Window>(parent);
    else
      fprintf(stderr, "Invalid X11 editor parent 0x%lx; using a top-level window\n",
              static_cast<unsigned long>(parent));
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
    if (!s) s = getenv("GDK_SCALE");
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
  XVisualInfo* vi = CreateGLContext();
  if (!mGLContext)
  {
    XCloseDisplay(mDisplay);
    mDisplay = nullptr;
    return nullptr;
  }
  // Create a colormap compatible with the GL visual so the window and context
  // share the same visual — required for glXMakeCurrent to succeed.
  mGLColormap = XCreateColormap(mDisplay, root, vi->visual, AllocNone);

  XSetWindowAttributes attrs = {};
  attrs.event_mask = ExposureMask | StructureNotifyMask | ButtonPressMask |
                     ButtonReleaseMask | PointerMotionMask | KeyPressMask |
                     KeyReleaseMask | EnterWindowMask | LeaveWindowMask |
                     FocusChangeMask | PropertyChangeMask;
  attrs.colormap = mGLColormap;

  // X11 window must be in physical pixels (WindowWidth/Height are logical).
  const auto physW = (unsigned)(WindowWidth()  * GetScreenScale());
  const auto physH = (unsigned)(WindowHeight() * GetScreenScale());

  mPlugWnd = XCreateWindow(
    mDisplay, mParentWnd,
    0, clientOffY, physW, physH,
    0, vi->depth, InputOutput, vi->visual,
    CWColormap | CWEventMask, &attrs
  );
  XFree(vi);
#else
  XSetWindowAttributes attrs = {};
  attrs.event_mask = ExposureMask | StructureNotifyMask | ButtonPressMask |
                     ButtonReleaseMask | PointerMotionMask | KeyPressMask |
                     KeyReleaseMask | EnterWindowMask | LeaveWindowMask |
                     FocusChangeMask | PropertyChangeMask;
  attrs.background_pixel = BlackPixel(mDisplay, screen);

  const auto physW = (unsigned)(WindowWidth()  * GetScreenScale());
  const auto physH = (unsigned)(WindowHeight() * GetScreenScale());

  mPlugWnd = XCreateWindow(
    mDisplay, mParentWnd,
    0, clientOffY, physW, physH,
    0, CopyFromParent, InputOutput, CopyFromParent,
    CWEventMask | CWBackPixel, &attrs
  );
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

  mWMDeleteMessage  = XInternAtom(mDisplay, "WM_DELETE_WINDOW",  0 /* False */);
  mClipboardAtom    = XInternAtom(mDisplay, "CLIPBOARD",         0 /* False */);
  mUTF8StringAtom   = XInternAtom(mDisplay, "UTF8_STRING",       0 /* False */);
  mSelDataAtom      = XInternAtom(mDisplay, "IPLUG2_SEL_DATA",   0 /* False */);
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
  XChangeProperty(mDisplay, mPlugWnd, mXdndAware, XA_ATOM, 32, PropModeReplace,
                  reinterpret_cast<const unsigned char*>(&xdndVersion), 1);

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
    hints.flags  = PSize | PMinSize;
    hints.width  = WindowWidth();
    hints.height = WindowHeight();
    hints.min_width  = WindowWidth();
    hints.min_height = WindowHeight();
    XSetWMNormalHints(mDisplay, mPlugWnd, &hints);
  }

  XMapWindow(mDisplay, mPlugWnd);
  XFlush(mDisplay);

#ifdef IGRAPHICS_GL
  bool initialized = false;
  {
    ScopedGLContext scopedGLCtx {this};
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
    if (mGLColormap) { XFreeColormap(mDisplay, mGLColormap); mGLColormap = 0; }
    XDestroyWindow(mDisplay, mPlugWnd); mPlugWnd = 0;
    XCloseDisplay(mDisplay); mDisplay = nullptr;
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
    ScopedGLContext scopedGLCtx {this};
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
  IControl* control = GetControl(mTooltipControl);
  if (!control || control->IsHidden() || !control->GetTooltip() || !*control->GetTooltip())
    return;
  if (!mTooltipFont)
  {
    char** missing = nullptr;
    int count = 0;
    char* defaultString = nullptr;
    mTooltipFont = XCreateFontSet(mDisplay, "-*-sans-medium-r-normal--14-*-*-*-*-*-*-*",
                                  &missing, &count, &defaultString);
    if (missing) XFreeStringList(missing);
    // Minimal Xvfb installations (and some desktops) have no "sans" XLFD.
    // The core fixed font is present with the standard X11 font packages.
    if (!mTooltipFont)
    {
      missing = nullptr;
      mTooltipFont = XCreateFontSet(mDisplay, "fixed", &missing, &count, &defaultString);
      if (missing) XFreeStringList(missing);
    }
  }
  if (!mTooltipFont) return;
  const char* label = control->GetTooltip();
  const int len = static_cast<int>(strlen(label));
  XRectangle ink {}, logical {};
  Xutf8TextExtents(mTooltipFont, label, len, &ink, &logical);
  const int width = std::max(20, static_cast<int>(logical.width) + 12);
  const int height = std::max(20, static_cast<int>(logical.height) + 8);
  int rootX = 0, rootY = 0;
  Window child = 0;
  XTranslateCoordinates(mDisplay, mPlugWnd, DefaultRootWindow(mDisplay),
                        static_cast<int>(x * GetTotalScale()) + 12,
                        static_cast<int>(y * GetTotalScale()) + 20,
                        &rootX, &rootY, &child);
  const int screen = DefaultScreen(mDisplay);
  rootX = std::max(0, std::min(rootX, DisplayWidth(mDisplay, screen) - width));
  rootY = std::max(0, std::min(rootY, DisplayHeight(mDisplay, screen) - height));
  XSetWindowAttributes attrs {};
  attrs.override_redirect = 1;
  attrs.background_pixel = WhitePixel(mDisplay, screen);
  attrs.event_mask = ExposureMask;
  mTooltipWindow = XCreateWindow(mDisplay, DefaultRootWindow(mDisplay), rootX, rootY,
                                 width, height, 1, CopyFromParent, InputOutput,
                                 CopyFromParent, CWOverrideRedirect | CWBackPixel | CWEventMask, &attrs);
  if (mTooltipWindow)
    XMapRaised(mDisplay, mTooltipWindow);
}

void IGraphicsLinux::DrawTooltip()
{
  if (!mTooltipWindow || mTooltipControl < 0 || !mTooltipFont) return;
  IControl* control = GetControl(mTooltipControl);
  if (!control || !control->GetTooltip()) return;
  const char* label = control->GetTooltip();
  XRectangle ink {}, logical {};
  Xutf8TextExtents(mTooltipFont, label, static_cast<int>(strlen(label)), &ink, &logical);
  XWindowAttributes attrs {};
  if (!XGetWindowAttributes(mDisplay, mTooltipWindow, &attrs)) return;
  GC gc = XCreateGC(mDisplay, mTooltipWindow, 0, nullptr);
  XSetForeground(mDisplay, gc, BlackPixel(mDisplay, DefaultScreen(mDisplay)));
  XDrawRectangle(mDisplay, mTooltipWindow, gc, 0, 0, attrs.width - 1, attrs.height - 1);
  Xutf8DrawString(mDisplay, mTooltipWindow, mTooltipFont, gc, 6, 4 - logical.y,
                  label, static_cast<int>(strlen(label)));
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
    auto w = (unsigned)(WindowWidth()  * GetScreenScale());
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
    while (XCheckTypedWindowEvent(mDisplay, mPlugWnd, ConfigureNotify, &event)) {}
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
  const auto pixels = [screenScale](int dimension, float candidateScale) {
    return PhysicalWindowDimension(dimension, candidateScale, screenScale);
  };
  if (GetResizerMode() == EUIResizerMode::Scale)
  {
    designWidth = Width();
    designHeight = Height();
    GetDelegate()->ConstrainEditorResize(designWidth, designHeight);

    const auto fits = [&](float candidateScale) {
      return pixels(designWidth, candidateScale) <= width &&
             pixels(designHeight, candidateScale) <= height;
    };
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
    const float nominalScale = ConstrainDrawScale(std::min(width / screenScale / designWidth,
                                                            height / screenScale / designHeight));
    if (pixels(designWidth, nominalScale) == pixels(designWidth, scale) &&
        pixels(designHeight, nominalScale) == pixels(designHeight, scale))
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
      const int nominal = static_cast<int>(std::lround(std::min<double>(maximum,
                                                    requested / screenScale / scale)));
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

void IGraphicsLinux::ForceEndUserEdit()
{
}

void IGraphicsLinux::HideMouseCursor(bool hide, bool lock)
{
  if (!mDisplay || !mPlugWnd)
    return;

  if (hide)
  {
    if (!mBlankCursor)
    {
      static const char emptyData[] = { 0, 0, 0, 0, 0, 0, 0, 0 };
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
    XWarpPointer(mDisplay, 0 /* None */, mPlugWnd,
                 0, 0, 0, 0,
                 (int)(x * scale), (int)(y * scale));
  }
}

ECursor IGraphicsLinux::SetMouseCursor(ECursor cursorType)
{
  if (!mDisplay || !mPlugWnd)
    return cursorType;

  unsigned int shape = XC_left_ptr;
  switch (cursorType)
  {
    case ECursor::ARROW:       shape = XC_left_ptr;      break;
    case ECursor::IBEAM:       shape = XC_xterm;         break;
    case ECursor::WAIT:        shape = XC_watch;         break;
    case ECursor::CROSS:       shape = XC_crosshair;     break;
    case ECursor::UPARROW:     shape = XC_sb_up_arrow;   break;
    case ECursor::SIZENWSE:    shape = XC_sizing;        break;
    case ECursor::SIZENESW:    shape = XC_sizing;        break;
    case ECursor::SIZEWE:      shape = XC_sb_h_double_arrow; break;
    case ECursor::SIZENS:      shape = XC_sb_v_double_arrow; break;
    case ECursor::SIZEALL:     shape = XC_fleur;         break;
    case ECursor::INO:         shape = XC_X_cursor;      break;
    case ECursor::HAND:        shape = XC_hand2;         break;
    case ECursor::APPSTARTING: shape = XC_watch;         break;
    case ECursor::HELP:        shape = XC_question_arrow; break;
    default:                   shape = XC_left_ptr;      break;
  }

  Cursor cursor = XCreateFontCursor(mDisplay, shape);
  XDefineCursor(mDisplay, mPlugWnd, cursor);
  XFreeCursor(mDisplay, cursor);
  return cursorType;
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

EMsgBoxResult IGraphicsLinux::ShowMessageBox(const char* str, const char* title,
                                             EMsgBoxType type,
                                             IMsgBoxCompletionHandlerFunc completionHandler)
{
  auto complete = [&](EMsgBoxResult result) {
    if (completionHandler)
      completionHandler(result);
    return result;
  };
  GTK3* g = LoadGTK3();
  const bool customButtons = type == kMB_YESNOCANCEL || type == kMB_RETRYCANCEL;
  if (!g || !g->message_dialog_new || (customButtons && !g->dialog_add_button))
  {
    fprintf(stderr, "Cannot show message box: required GTK functions are unavailable\n");
    return complete(kCANCEL);
  }

  int gtkMsgType  = kGTK_MESSAGE_INFO;
  int gtkBtnType  = kGTK_BUTTONS_OK;
  switch (type)
  {
    case kMB_OK:          break;
    case kMB_OKCANCEL:    gtkMsgType = kGTK_MESSAGE_QUESTION; gtkBtnType = kGTK_BUTTONS_OK_CANCEL; break;
    case kMB_YESNO:       gtkMsgType = kGTK_MESSAGE_QUESTION; gtkBtnType = kGTK_BUTTONS_YES_NO;    break;
    case kMB_YESNOCANCEL:
    case kMB_RETRYCANCEL: gtkMsgType = kGTK_MESSAGE_QUESTION; gtkBtnType = kGTK_BUTTONS_NONE; break;
    default:
      fprintf(stderr, "Cannot show message box: unsupported button type %d\n", type);
      return complete(kCANCEL);
  }

  void* dialog = g->message_dialog_new(nullptr, 0, gtkMsgType, gtkBtnType, "%s", str ? str : "");
  if (!dialog)
  {
    fprintf(stderr, "Could not create the message box\n");
    return complete(kCANCEL);
  }
  if (type == kMB_YESNOCANCEL)
  {
    g->dialog_add_button(dialog, "_Cancel", kGTK_RESPONSE_CANCEL);
    g->dialog_add_button(dialog, "_No", kGTK_RESPONSE_NO);
    g->dialog_add_button(dialog, "_Yes", kGTK_RESPONSE_YES);
  }
  else if (type == kMB_RETRYCANCEL)
  {
    g->dialog_add_button(dialog, "_Cancel", kGTK_RESPONSE_CANCEL);
    g->dialog_add_button(dialog, "_Retry", kGTK_RESPONSE_RETRY);
  }
  if (title && g->window_set_title)
    g->window_set_title(dialog, title);
  GtkPresentDialog(g, dialog);

  int resp = g->dialog_run(dialog);
  g->widget_destroy(dialog);
  GtkDrainEvents(g);

  EMsgBoxResult result = kCANCEL;
  switch (resp)
  {
    case kGTK_RESPONSE_OK:     result = kOK;     break;
    case kGTK_RESPONSE_YES:    result = kYES;    break;
    case kGTK_RESPONSE_NO:     result = kNO;     break;
    case kGTK_RESPONSE_CANCEL: result = kCANCEL; break;
    case kGTK_RESPONSE_RETRY:  result = kRETRY;  break;
    default:                   result = kCANCEL; break;
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

void IGraphicsLinux::PromptForFile(WDL_String& fileName, WDL_String& path,
                                   EFileAction action, const char* ext,
                                   IFileDialogCompletionHandlerFunc completionHandler)
{
  WDL_String outFile, outPath;
  auto complete = [&]() {
    fileName.Set(outFile.Get());
    path.Set(outPath.Get());
    if (completionHandler)
      completionHandler(fileName, path);
  };
  GTK3* g = LoadGTK3();
  if (!g)
  {
    fprintf(stderr, "Cannot show file dialog: GTK is unavailable\n");
    complete();
    return;
  }

  const bool isSave = (action == EFileAction::Save);
  const int  gtkAction  = isSave ? kGTK_FILE_CHOOSER_ACTION_SAVE : kGTK_FILE_CHOOSER_ACTION_OPEN;
  const char* acceptBtn = isSave ? "_Save" : "_Open";

  void* dialog = g->file_chooser_dialog_new(
    isSave ? "Save File" : "Open File", nullptr, gtkAction,
    "_Cancel", kGTK_RESPONSE_CANCEL,
    acceptBtn,  kGTK_RESPONSE_OK,
    nullptr
  );

  if (!dialog)
  {
    fprintf(stderr, "Could not create the file dialog\n");
    complete();
    return;
  }
  if (!GtkAddFileFilters(g, dialog, ext))
  {
    g->widget_destroy(dialog);
    GtkDrainEvents(g);
    complete();
    return;
  }

  // Set initial directory
  if (path.GetLength() && g->file_chooser_set_current_folder)
    g->file_chooser_set_current_folder(dialog, path.Get());

  // For save dialogs, suggest filename
  if (isSave && fileName.GetLength() && g->file_chooser_set_current_name)
    g->file_chooser_set_current_name(dialog, fileName.Get());
  GtkPresentDialog(g, dialog);

  if (g->dialog_run(dialog) == kGTK_RESPONSE_OK)
  {
    char* chosen = g->file_chooser_get_filename(dialog);
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

  g->widget_destroy(dialog);
  GtkDrainEvents(g);

  complete();
}

void IGraphicsLinux::PromptForDirectory(WDL_String& dir,
                                        IFileDialogCompletionHandlerFunc completionHandler)
{
  WDL_String outDir;
  auto complete = [&]() {
    dir.Set(outDir.Get());
    if (completionHandler)
      completionHandler(WDL_String{}, dir);
  };
  GTK3* g = LoadGTK3();
  if (!g)
  {
    fprintf(stderr, "Cannot show directory dialog: GTK is unavailable\n");
    complete();
    return;
  }

  void* dialog = g->file_chooser_dialog_new(
    "Select Folder", nullptr, kGTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
    "_Cancel", kGTK_RESPONSE_CANCEL,
    "_Select", kGTK_RESPONSE_OK,
    nullptr
  );

  if (!dialog)
  {
    fprintf(stderr, "Could not create the directory dialog\n");
    complete();
    return;
  }

  if (dir.GetLength() && g->file_chooser_set_current_folder)
    g->file_chooser_set_current_folder(dialog, dir.Get());
  GtkPresentDialog(g, dialog);

  if (g->dialog_run(dialog) == kGTK_RESPONSE_OK)
  {
    char* chosen = g->file_chooser_get_filename(dialog);
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

  g->widget_destroy(dialog);
  GtkDrainEvents(g);

  complete();
}

bool IGraphicsLinux::PromptForColor(IColor& color, const char* str,
                                    IColorPickerHandlerFunc func)
{
  GTK3* g = LoadGTK3();
  if (!g || !g->color_chooser_dialog_new)
    return false;

  void* dialog = g->color_chooser_dialog_new(str ? str : "Choose Color", nullptr);

  if (g->color_chooser_set_rgba)
  {
    GdkRGBA rgba = { color.R / 255.0, color.G / 255.0,
                     color.B / 255.0, color.A / 255.0 };
    g->color_chooser_set_rgba(dialog, &rgba);
  }
  GtkPresentDialog(g, dialog);

  const bool accepted = (g->dialog_run(dialog) == kGTK_RESPONSE_OK);

  if (accepted && g->color_chooser_get_rgba)
  {
    GdkRGBA rgba = {};
    g->color_chooser_get_rgba(dialog, &rgba);
    color.R = (int)(rgba.red   * 255.0 + 0.5);
    color.G = (int)(rgba.green * 255.0 + 0.5);
    color.B = (int)(rgba.blue  * 255.0 + 0.5);
    color.A = (int)(rgba.alpha * 255.0 + 0.5);
    if (func)
      func(color);
  }

  g->widget_destroy(dialog);
  GtkDrainEvents(g);
  return accepted;
}

bool IGraphicsLinux::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
  return LaunchDetached("xdg-open", url);
}

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
  GTK3* gtk = LoadGTK3();

  // Ask the clipboard owner to convert to UTF-8 and store in our scratch property
  XConvertSelection(mDisplay, mClipboardAtom, mUTF8StringAtom, mSelDataAtom,
                    mPlugWnd, 0L /* CurrentTime */);
  XFlush(mDisplay);

  // Keep the event consumer locked out until we have read SelectionNotify.
  for (int i = 0; i < 50; ++i)
  {
    if (gtk && gtk->events_pending && gtk->main_iteration_do && gtk->events_pending())
      gtk->main_iteration_do(0);
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

      const int status = XGetWindowProperty(mDisplay, mPlugWnd, mSelDataAtom,
                         0, 65536, 1 /* delete after read */,
                         AnyPropertyType, &type, &fmt,
                         &nitems, &remaining, &data);
      // Require remaining == 0 so we never report success on a truncated
      // read, and bound nitems before the int cast to avoid UB.
      if (status == 0 && fmt == 8 && (type == mUTF8StringAtom || type == XA_STRING) &&
          data && remaining == 0 && nitems <= 0x7FFFFFFFUL)
      {
        str.Set((const char*)data, (int)nitems);
        XFree(data);
        return true;
      }
      if (data) XFree(data);
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
static void* BuildGtkMenu(GTK3* g, IPopupMenu& menu, GtkMenuState* pState)
{
  void* gtkMenu = g->menu_new();

  for (int i = 0; i < menu.NItems(); ++i)
  {
    IPopupMenu::Item* pItem = menu.GetItem(i);
    void* gtkItem = nullptr;

    if (pItem->GetIsSeparator())
    {
      gtkItem = g->separator_menu_item_new();
    }
    else if (pItem->GetChecked() && g->check_menu_item_new_with_label)
    {
      gtkItem = g->check_menu_item_new_with_label(pItem->GetText());
      if (g->check_menu_item_set_active)
        g->check_menu_item_set_active(gtkItem, 1);
    }
    else
    {
      gtkItem = g->menu_item_new_with_label(pItem->GetText());
    }

    if (!gtkItem)
      continue;

    // Disabled or title items are insensitive
    if (!pItem->GetEnabled() || pItem->GetIsTitle())
    {
      if (g->widget_set_sensitive)
        g->widget_set_sensitive(gtkItem, 0);
    }

    // Submenus
    if (pItem->GetSubmenu() && g->menu_item_set_submenu)
    {
      void* sub = BuildGtkMenu(g, *pItem->GetSubmenu(), pState);
      g->menu_item_set_submenu(gtkItem, sub);
    }
    else if (pItem->GetEnabled() && !pItem->GetIsTitle()
             && !pItem->GetIsSeparator() && g->signal_connect_data)
    {
      // Store the menu pointer and index in the state for the callback.
      // We use a small lambda-like static function with user data.
      struct ActivateData { GtkMenuState* state; IPopupMenu* menu; int idx; };
      auto* ad = new ActivateData{ pState, &menu, i };

      // GCallback signature: void callback(GtkMenuItem*, gpointer)
      auto activateCb = +[](void*, void* userData) {
        auto* d = static_cast<ActivateData*>(userData);
        d->state->pMenu = d->menu;
        d->state->chosenIdx = d->idx;
      };

      // GDestroyNotify frees the ActivateData when the signal is disconnected.
      auto destroyCb = +[](void* userData) {
        delete static_cast<ActivateData*>(userData);
      };

      g->signal_connect_data(gtkItem, "activate",
                             (void*)activateCb, ad,
                             (void*)destroyCb, 0);
    }

    g->menu_shell_append(gtkMenu, gtkItem);
  }

  if (g->widget_show_all)
    g->widget_show_all(gtkMenu);

  return gtkMenu;
}

IPopupMenu* IGraphicsLinux::CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT bounds, bool& isAsync)
{
  isAsync = false;

  GTK3* g = LoadGTK3();
  if (!g || !g->menu_new || !g->menu_item_new_with_label
      || !g->menu_shell_append || !g->main || !g->main_quit)
    return nullptr;

  GtkMenuState state;
  void* gtkMenu = BuildGtkMenu(g, menu, &state);

  // Quit the GTK main loop when the menu is dismissed (deactivate signal).
  if (g->signal_connect_data)
  {
    auto deactivateCb = +[](void*, void*) {
      // LoadGTK3() returns cached pointer; safe to call again
      GTK3* gInner = LoadGTK3();
      if (gInner && gInner->main_quit)
        gInner->main_quit();
    };
    g->signal_connect_data(gtkMenu, "deactivate",
                           (void*)deactivateCb, nullptr, nullptr, 0);
  }

  // Position the popup menu anchored to the control's bounding rectangle.
  // gtk_menu_popup_at_rect is the non-deprecated GTK3 API (since 3.22).
  // It needs a GdkWindow — we create a "foreign" wrapper around mPlugWnd.
  // Falls back to gtk_menu_popup_at_pointer(NULL) on older GTK or if the
  // foreign window can't be created (e.g. Wayland without X11 backend).
  bool popped = false;

  if (g->menu_popup_at_rect && g->gdk_display_get_default
      && g->gdk_x11_window_foreign && mPlugWnd)
  {
    void* gdkDisplay = g->gdk_display_get_default();
    if (gdkDisplay)
    {
      void* gdkWindow = g->gdk_x11_window_foreign(
        gdkDisplay, static_cast<unsigned long>(mPlugWnd));

      if (gdkWindow)
      {
        float s = GetScreenScale();
        GdkRectangle rect;
        rect.x      = static_cast<int>(bounds.L * s);
        rect.y      = static_cast<int>(bounds.B * s);
        rect.width  = static_cast<int>(bounds.W() * s);
        rect.height = 1;

        g->menu_popup_at_rect(gtkMenu, gdkWindow, &rect,
                              kGDK_GRAVITY_NORTH_WEST,
                              kGDK_GRAVITY_NORTH_WEST,
                              nullptr);
        popped = true;

        if (g->g_object_unref)
          g->g_object_unref(gdkWindow);
      }
    }
  }

  if (!popped && g->menu_popup_at_pointer)
    g->menu_popup_at_pointer(gtkMenu, nullptr);

  // Run a nested GTK main loop — blocks until the menu closes.
  g->main();

  // GTK owns the menu widget tree; destroy it.
  g->widget_destroy(gtkMenu);
  GtkDrainEvents(g);

  if (state.pMenu && state.chosenIdx >= 0)
  {
    state.pMenu->SetChosenItemIdx(state.chosenIdx);

    if (state.pMenu->GetFunction())
      state.pMenu->ExecFunction();

    return state.pMenu;
  }

  return nullptr;
}

void IGraphicsLinux::CreatePlatformTextEntry(int paramIdx, const IText& text,
                                              const IRECT& bounds, int length,
                                              const char* str)
{
  // GTK dialog flags: GTK_DIALOG_MODAL = 1, GTK_DIALOG_DESTROY_WITH_PARENT = 2
  static constexpr int kGTK_DIALOG_MODAL = 1;

  GTK3* g = LoadGTK3();
  if (!g || !g->dialog_new_with_buttons || !g->dialog_get_content_area
      || !g->entry_new || !g->entry_get_text || !g->box_pack_start)
  {
    SetControlValueAfterTextEdit(str);
    return;
  }

  void* dialog = g->dialog_new_with_buttons(
    "Edit Value", nullptr, kGTK_DIALOG_MODAL,
    "_Cancel", kGTK_RESPONSE_CANCEL,
    "_OK", kGTK_RESPONSE_OK,
    nullptr
  );

  void* entry = g->entry_new();
  if (g->entry_set_text && str)
    g->entry_set_text(entry, str);
  if (g->entry_set_max_length && length > 0)
    g->entry_set_max_length(entry, length);

  // Wire Enter key in the entry to trigger the OK response
  if (g->signal_connect_data && g->dialog_response)
  {
    auto activateCb = +[](void*, void* userData) {
      GTK3* gInner = LoadGTK3();
      if (gInner && gInner->dialog_response)
        gInner->dialog_response(userData, kGTK_RESPONSE_OK);
    };
    g->signal_connect_data(entry, "activate",
                           (void*)activateCb, dialog, nullptr, 0);
  }

  void* contentArea = g->dialog_get_content_area(dialog);
  g->box_pack_start(contentArea, entry, 1, 1, 0);

  if (g->widget_show_all)
    g->widget_show_all(dialog);
  GtkPresentDialog(g, dialog);

  int response = g->dialog_run(dialog);

  if (response == kGTK_RESPONSE_OK)
  {
    const char* result = g->entry_get_text(entry);
    SetControlValueAfterTextEdit(result ? result : "");
  }
  else
  {
    SetControlValueAfterTextEdit(str);
  }

  g->widget_destroy(dialog);
  GtkDrainEvents(g);
}

PlatformFontPtr IGraphicsLinux::LoadPlatformFont(const char* fontID, const char* fileNameOrResID)
{
  WDL_String fullPath;
  const EResourceLocation loc = LocateResource(fileNameOrResID, "ttf", fullPath,
                                               GetBundleID(), nullptr, nullptr);
  if (loc == EResourceLocation::kNotFound)
    return nullptr;

  return PlatformFontPtr(new LinuxFileFont(fullPath.Get(), false));
}

PlatformFontPtr IGraphicsLinux::LoadPlatformFont(const char* fontID, const char* fontName,
                                                 ETextStyle style)
{
  Fontconfig* fc = LoadFontconfig();
  if (!fc)
    return nullptr;

  _FcPattern* pattern = fc->PatternCreate();
  if (!pattern)
    return nullptr;

  fc->PatternAddString(pattern, "family", (const unsigned char*)fontName);
  fc->PatternAddInteger(pattern, "weight",
                        style == ETextStyle::Bold ? kFC_WEIGHT_BOLD : kFC_WEIGHT_REGULAR);
  fc->PatternAddInteger(pattern, "slant",
                        style == ETextStyle::Italic ? kFC_SLANT_ITALIC : kFC_SLANT_ROMAN);

  fc->ConfigSubstitute(nullptr, pattern, kFC_MatchPattern);
  fc->DefaultSubstitute(pattern);

  int result = 1; // FcResultNoMatch
  _FcPattern* matched = fc->FontMatch(nullptr, pattern, &result);
  fc->PatternDestroy(pattern);

  if (!matched)
    return nullptr;

  PlatformFontPtr ret;
  unsigned char* path = nullptr;
  if (fc->PatternGetString(matched, "file", 0, &path) == kFC_ResultMatch && path)
    ret = PlatformFontPtr(new LinuxFileFont((const char*)path, true));

  fc->PatternDestroy(matched);
  return ret;
}

PlatformFontPtr IGraphicsLinux::LoadPlatformFont(const char* fontID, void* pData, int dataSize)
{
  return PlatformFontPtr(new LinuxMemFont(pData, dataSize));
}

void IGraphicsLinux::CachePlatformFont(const char* fontID, const PlatformFontPtr& font)
{
  // NanoVG caches loaded fonts in its own atlas; no additional platform cache needed.
}

void IGraphicsLinux::ActivateGLContext()
{
#ifdef IGRAPHICS_GL
  mGLContextStack.push_back({glXGetCurrentDisplay(), glXGetCurrentContext(),
                            glXGetCurrentDrawable(), glXGetCurrentReadDrawable()});
  if (!mDisplay || !mGLContext) return;
  if (!glXMakeCurrent(mDisplay, mPlugWnd, (GLXContext)mGLContext))
    fprintf(stderr, "Could not activate the Linux graphics GLX context\n");
#endif
}

void IGraphicsLinux::DeactivateGLContext()
{
#ifdef IGRAPHICS_GL
  assert(!mGLContextStack.empty());
  const auto state = mGLContextStack.back();
  mGLContextStack.pop_back();
  if (glXGetCurrentDisplay() == state.display && glXGetCurrentContext() == state.context &&
      glXGetCurrentDrawable() == state.drawable && glXGetCurrentReadDrawable() == state.readDrawable)
    return;

  Display* display = state.context ? state.display : glXGetCurrentDisplay();
  if (display && !glXMakeContextCurrent(display, state.drawable, state.readDrawable,
                                       static_cast<GLXContext>(state.context)))
    fprintf(stderr, "Could not restore the previous GLX context\n");
#endif
}

#ifdef IGRAPHICS_GL
XVisualInfo* IGraphicsLinux::CreateGLContext()
{
  // Classic GLX 1.2 API — compatible with all Mesa drivers including software
  // renderers (llvmpipe, softpipe) on Xwayland where the GLX 1.3 FBConfig +
  // glXCreateNewContext path fails to create DRI drawables.
  int screen = DefaultScreen(mDisplay);

  int attribs[] = {
    GLX_RGBA,
    GLX_DOUBLEBUFFER,
    GLX_RED_SIZE,   8,
    GLX_GREEN_SIZE, 8,
    GLX_BLUE_SIZE,  8,
    GLX_DEPTH_SIZE, 24,
    0 /* None */
  };

  XVisualInfo* vi = glXChooseVisual(mDisplay, screen, attribs);
  if (!vi)
    return nullptr;

  mGLContext = (void*)glXCreateContext(mDisplay, vi, nullptr, GL_TRUE);
  if (!mGLContext)
  {
    XFree(vi);
    return nullptr;
  }
  // Caller owns vi and must XFree() it after creating the X window
  return vi;
}

void IGraphicsLinux::DestroyGLContext()
{
  if (mDisplay && mGLContext)
  {
    if (glXGetCurrentContext() == mGLContext)
      if (!glXMakeCurrent(mDisplay, 0, nullptr))
        fprintf(stderr, "Could not release the Linux graphics GLX context\n");
    glXDestroyContext(mDisplay, (GLXContext)mGLContext);
    mGLContext = nullptr;
  }
}
#endif

// Map X11 keysym to Windows virtual key code (used by IKeyPress).
// Covers the subset that iPlug2 controls actually handle.
static int KeysymToVK(KeySym ks)
{
  switch (ks)
  {
    case XK_BackSpace:    return kVK_BACK;
    case XK_Tab:          return kVK_TAB;
    case XK_Return:       return kVK_RETURN;
    case XK_Escape:       return kVK_ESCAPE;
    case XK_space:        return kVK_SPACE;
    case XK_Prior:        return kVK_PRIOR;
    case XK_Next:         return kVK_NEXT;
    case XK_End:          return kVK_END;
    case XK_Home:         return kVK_HOME;
    case XK_Left:         return kVK_LEFT;
    case XK_Up:           return kVK_UP;
    case XK_Right:        return kVK_RIGHT;
    case XK_Down:         return kVK_DOWN;
    case XK_Insert:       return kVK_INSERT;
    case XK_Delete:       return kVK_DELETE;
    case XK_F1:           return kVK_F1;
    case XK_F2:           return kVK_F2;
    case XK_F3:           return kVK_F3;
    case XK_F4:           return kVK_F4;
    case XK_F5:           return kVK_F5;
    case XK_F6:           return kVK_F6;
    case XK_F7:           return kVK_F7;
    case XK_F8:           return kVK_F8;
    case XK_F9:           return kVK_F9;
    case XK_F10:          return kVK_F10;
    case XK_F11:          return kVK_F11;
    case XK_F12:          return kVK_F12;
    case XK_Shift_L:
    case XK_Shift_R:      return kVK_SHIFT;
    case XK_Control_L:
    case XK_Control_R:    return kVK_CONTROL;
    case XK_Alt_L:
    case XK_Alt_R:        return kVK_MENU;
    default:
      if (ks >= XK_0 && ks <= XK_9) return kVK_0 + (int)(ks - XK_0);
      if (ks >= XK_a && ks <= XK_z) return kVK_A + (int)(ks - XK_a);
      if (ks >= XK_A && ks <= XK_Z) return kVK_A + (int)(ks - XK_A);
      return kVK_NONE;
  }
}

void IGraphicsLinux::CreateInputContext()
{
  mInputMethodDisconnected = false;
  // Use the host's locale/modifiers; changing them here would affect every plugin.
  mInputMethod = XOpenIM(mDisplay, nullptr, nullptr, nullptr);
  if (mInputMethod)
  {
    XIMCallback destroyed {};
    destroyed.client_data = reinterpret_cast<XPointer>(this);
    destroyed.callback = [](XIM, XPointer data, XPointer) {
      auto* graphics = reinterpret_cast<IGraphicsLinux*>(data);
      graphics->mInputContext = nullptr;
      graphics->mInputMethod = nullptr;
      graphics->mInputMethodDisconnected = true;
      fprintf(stderr, "The Linux input method disconnected; Compose and IME input are unavailable\n");
    };
    if (XSetIMValues(mInputMethod, XNDestroyCallback, &destroyed, nullptr))
    {
      fprintf(stderr, "Could not register the Linux input method disconnect callback; Compose and IME input are unavailable\n");
      DestroyInputContext();
      return;
    }
    mInputContext = XCreateIC(mInputMethod,
      XNInputStyle, XIMPreeditNothing | XIMStatusNothing,
      XNClientWindow, mPlugWnd, XNFocusWindow, mPlugWnd, nullptr);
  }
  if (!mInputContext)
  {
    fprintf(stderr, "Could not create the Linux input context; Compose and IME input are unavailable\n");
    DestroyInputContext();
    return;
  }
  long filterEvents = 0;
  if (XGetICValues(mInputContext, XNFilterEvents, &filterEvents, nullptr) == nullptr)
  {
    XWindowAttributes attributes {};
    if (XGetWindowAttributes(mDisplay, mPlugWnd, &attributes))
      XSelectInput(mDisplay, mPlugWnd, attributes.your_event_mask | filterEvents);
  }
}

void IGraphicsLinux::DestroyInputContext()
{
  if (mInputContext)
  {
    XDestroyIC(mInputContext);
    mInputContext = nullptr;
  }
  if (mInputMethod)
  {
    XCloseIM(mInputMethod);
    mInputMethod = nullptr;
  }
}

void IGraphicsLinux::DispatchKeyEvent(XKeyEvent& event)
{
  KeySym symbol = 0;
  WDL_TypedBuf<char> text;
  text.Resize(64);
  if (!text.Get())
  {
    fprintf(stderr, "Could not allocate the Linux keyboard input buffer\n");
    return;
  }
  int length = 0;
  if (event.type == KeyPress && mInputContext)
  {
    int status = 0;
    length = Xutf8LookupString(mInputContext, &event, text.Get(), text.GetSize() - 1, &symbol, &status);
    if (status == XBufferOverflow && length > 0 && length < INT_MAX)
    {
      text.Resize(length + 1);
      if (text.GetSize() != length + 1)
      {
        fprintf(stderr, "Could not allocate the Linux input method commit buffer\n");
        return;
      }
      length = Xutf8LookupString(mInputContext, &event, text.Get(), text.GetSize() - 1, &symbol, &status);
    }
    if (status != XLookupChars && status != XLookupKeySym && status != XLookupBoth)
    {
      if (status != XLookupNone)
        fprintf(stderr, "Could not read the Linux input method commit (status %d)\n", status);
      return;
    }
    if (status == XLookupChars)
      symbol = 0;
    if (status == XLookupKeySym)
      length = 0;
  }
  else
  {
    // Key releases cannot use Xutf8LookupString. Encode printable keysyms rather
    // than assuming XLookupString's locale-dependent bytes are Latin-1 or UTF-8.
    length = XLookupString(&event, text.Get(), text.GetSize() - 1, &symbol, nullptr);
    const bool ascii = length == 1 && static_cast<unsigned char>(text.Get()[0]) < 0x80;
    if (!ascii && symbol >= 0xA0 && symbol <= 0xFF)
      length = wdl_utf8_makechar(static_cast<int>(symbol), text.Get(), text.GetSize() - 1);
    else if (!ascii && (symbol & 0xFF000000UL) == 0x01000000UL)
    {
      const auto codepoint = static_cast<int>(symbol & 0xFFFFFFUL);
      if (codepoint <= 0x10FFFF && !(codepoint >= 0xD800 && codepoint <= 0xDFFF))
        length = wdl_utf8_makechar(codepoint, text.Get(), text.GetSize() - 1);
    }
  }
  if (length < 0 || length >= text.GetSize())
  {
    fprintf(stderr, "Invalid Linux input method commit length %d\n", length);
    return;
  }
  text.Get()[length] = '\0';
  // A phrase commit can accompany the Return keysym used to accept an IME candidate.
  if (length > 1)
    symbol = 0;

  int offset = 0;
  do
  {
    char character[5] {};
    int size = 0;
    if (offset < length)
    {
      int codepoint = 0;
      size = wdl_utf8_parsechar(text.Get() + offset, &codepoint);
      char encoded[4] {};
      if (size > 4 || offset + size > length || codepoint > 0x10FFFF ||
          (codepoint >= 0xD800 && codepoint <= 0xDFFF) ||
          wdl_utf8_makechar(codepoint, encoded, sizeof(encoded)) != size ||
          memcmp(encoded, text.Get() + offset, size) != 0)
      {
        fprintf(stderr, "The Linux input method returned invalid UTF-8\n");
        return;
      }
      memcpy(character, text.Get() + offset, size);
    }
    // IKeyPress holds one UTF-8 code point, while an IME can commit an entire phrase.
    IKeyPress key(character, offset == 0 ? KeysymToVK(symbol) : kVK_NONE,
                  (event.state & ShiftMask) != 0, (event.state & ControlMask) != 0,
                  (event.state & Mod1Mask) != 0);
    if (event.type == KeyPress)
      OnKeyDown(mCursorX, mCursorY, key);
    else
      OnKeyUp(mCursorX, mCursorY, key);
    offset += size;
  } while (offset < length && !mCloseRequested);
}

void IGraphicsLinux::ProcessX11Events()
{
  if (IsInPlatformCallback())
    return;
  DispatchScope dispatch(mDispatchDepth);
  DispatchX11Events();
}

void IGraphicsLinux::DispatchX11Events()
{
  if (!mDisplay || !mPlugWnd || mCloseRequested)
    return;

  while (!mCloseRequested && XPending(mDisplay))
  {
    XEvent ev;
    XNextEvent(mDisplay, &ev);
    if (ev.type == Expose && ev.xexpose.window == mTooltipWindow)
    {
      DrawTooltip();
      continue;
    }
    if (XFilterEvent(&ev, mPlugWnd))
      continue;

    const float scale = GetTotalScale();
    switch (ev.type)
    {
      case Expose:
      {
        // Handled by the render path below; drain duplicate exposures
        while (XCheckTypedWindowEvent(mDisplay, mPlugWnd, Expose, &ev))
          ;
        mNeedsRedraw = true;
        break;
      }

      case ConfigureNotify:
      {
        XConfigureEvent& ce = ev.xconfigure;
        if (ce.window != mPlugWnd)
          break;

        // Drain all pending ConfigureNotify events and use the latest.
        // During rapid resizing (corner-resizer drag or host resize),
        // multiple ConfigureNotify events queue up. Processing stale
        // intermediate sizes would cause unnecessary FBO recreations
        // and potential size oscillations. Only the final size matters.
        {
          XEvent next;
          while (XCheckTypedWindowEvent(mDisplay, mPlugWnd,
                                        ConfigureNotify, &next))
          {
            ce = next.xconfigure;
          }
        }

        const int physW = ce.width;
        const int physH = ce.height;
        // Check if the internal state already matches this physical
        // size (e.g. OnParentWindowResize already processed it).
        const int curPhysW = static_cast<int>(
          WindowWidth() * GetScreenScale());
        const int curPhysH = static_cast<int>(
          WindowHeight() * GetScreenScale());
        if (physW == curPhysW && physH == curPhysH)
        {
          break;
        }

        // The X server resized our window. Suppress PlatformResize to
        // avoid a resize oscillation.
        mInConfigureNotify = true;

        ResizeFromHost(physW, physH);

        mInConfigureNotify = false;
        // A WM may ignore the aspect ratio or scale limits. Reconcile the child
        // once; the resulting ConfigureNotify already matches our state.
        if (physW != static_cast<int>(WindowWidth() * GetScreenScale()) ||
            physH != static_cast<int>(WindowHeight() * GetScreenScale()))
          PlatformResize(false);
        break;
      }

      case ButtonPress:
      case ButtonRelease:
      {
        HideTooltip();
        mTooltipControl = -1;
        XButtonEvent& be = ev.xbutton;
        const float x = be.x / scale;
        const float y = be.y / scale;
        const bool shift   = (be.state & ShiftMask)   != 0;
        const bool ctrl    = (be.state & ControlMask) != 0;
        const bool alt     = (be.state & Mod1Mask)    != 0;

        if (be.button == Button4 || be.button == Button5)
        {
          // Scroll wheel — only fire on press
          if (ev.type == ButtonPress)
          {
            XSetInputFocus(mDisplay, mPlugWnd, RevertToParent, be.time);
            const float delta = (be.button == Button4) ? 1.f : -1.f;
            IMouseMod mod(false, false, shift, ctrl, alt);
            OnMouseWheel(x, y, mod, delta);
          }
          break;
        }

        const bool left   = (be.button == Button1);
        const bool right  = (be.button == Button3);
        const bool middle = (be.button == Button2);
        IMouseMod mod(left, right, shift, ctrl, alt);

        mCursorX = x;
        mCursorY = y;

        IMouseInfo info;
        info.x  = x;
        info.y  = y;
        info.dX = 0.f;
        info.dY = 0.f;
        info.ms = mod;
        std::vector<IMouseInfo> list{ info };

        if (ev.type == ButtonPress)
        {
          // A child of the host's window does not gain focus automatically.
          // Only an explicit click transfers focus; mapping/hovering never does.
          XSetInputFocus(mDisplay, mPlugWnd, RevertToParent, be.time);
          XFlush(mDisplay);
          // Measure distance in screen-logical pixels, independent of UI zoom.
          static constexpr unsigned long kDblClickTimeoutMs = 400;
          static constexpr float kDblClickDistance = 4.f;
          const float clickX = be.x / GetScreenScale();
          const float clickY = be.y / GetScreenScale();
          if (left && mLastLeftClickTime &&
              static_cast<uint32_t>(be.time - mLastLeftClickTime) < kDblClickTimeoutMs &&
              std::fabs(clickX - mLastLeftClickX) <= kDblClickDistance &&
              std::fabs(clickY - mLastLeftClickY) <= kDblClickDistance)
          {
            OnMouseDblClick(x, y, mod);
            mLastLeftClickTime = 0;  // reset so triple-click doesn't fire again
          }
          else
          {
            OnMouseDown(list);
            mLastLeftClickTime = left ? be.time : 0;
            mLastLeftClickX = clickX;
            mLastLeftClickY = clickY;
          }
        }
        else
        {
          OnMouseUp(list);
        }
        break;
      }

      case MotionNotify:
      {
        XMotionEvent& me = ev.xmotion;
        const float x     = me.x / scale;
        const float y     = me.y / scale;
        const bool shift  = (me.state & ShiftMask)   != 0;
        const bool ctrl   = (me.state & ControlMask) != 0;
        const bool alt    = (me.state & Mod1Mask)    != 0;
        const bool lbtn   = (me.state & Button1Mask) != 0;
        const bool rbtn   = (me.state & Button3Mask) != 0;
        IMouseMod mod(lbtn, rbtn, shift, ctrl, alt);

        IMouseInfo info;
        info.x  = x;
        info.y  = y;
        info.dX = x - mCursorX;
        info.dY = y - mCursorY;
        info.ms = mod;
        std::vector<IMouseInfo> list{ info };

        if (lbtn || rbtn)
        {
          HideTooltip();
          mTooltipControl = -1;
          OnMouseDrag(list);
          mCursorX = x;
          mCursorY = y;
        }
        else
        {
          OnMouseOver(x, y, mod);
          mCursorX = x;
          mCursorY = y;
          const int hovered = TooltipsEnabled() ? GetMouseOver() : -1;
          if (hovered != mTooltipControl || std::fabs(mTooltipX - x) > 4.f || std::fabs(mTooltipY - y) > 4.f)
          {
            HideTooltip();
            mTooltipControl = hovered;
            mTooltipX = x;
            mTooltipY = y;
            mTooltipHoverTime = HoverMillis();
          }
        }
        break;
      }

      case LeaveNotify:
        HideTooltip();
        mTooltipControl = -1;
        OnMouseOut();
        break;

      case FocusIn:
        if (mInputMethodDisconnected)
          CreateInputContext();
        if (mInputContext)
          XSetICFocus(mInputContext);
        break;

      case FocusOut:
        HideTooltip();
        if (mInputContext)
          XUnsetICFocus(mInputContext);
        break;

      case KeyPress:
      case KeyRelease:
        DispatchKeyEvent(ev.xkey);
        break;

      case ClientMessage:
      {
        if (ev.xclient.window == mPlugWnd && ev.xclient.format == 32)
        {
          auto& msg = ev.xclient;
          if (msg.message_type == mXdndEnter)
          {
            HideTooltip();
            mTooltipControl = -1;
            ResetDrag();
            const int version = (msg.data.l[1] >> 24) & 0xff;
            if (version >= 3)
            {
              mDragSource = static_cast<Window>(msg.data.l[0]);
              if (msg.data.l[1] & 1)
              {
                Atom type = 0; int format = 0; unsigned long count = 0, remaining = 0;
                unsigned char* data = nullptr;
                if (XGetWindowProperty(mDisplay, mDragSource, mXdndTypeList, 0, 1024,
                      0, XA_ATOM, &type, &format, &count, &remaining, &data) == 0 &&
                    type == XA_ATOM && format == 32 && remaining == 0 && data)
                {
                  const auto* types = reinterpret_cast<const Atom*>(data);
                  for (unsigned long i = 0; i < count; ++i)
                    if (types[i] == mUriList) mDragSupported = true;
                }
                if (data) XFree(data);
              }
              else
                for (int i = 2; i < 5; ++i)
                  if (static_cast<Atom>(msg.data.l[i]) == mUriList) mDragSupported = true;
            }
            break;
          }
          if (msg.message_type == mXdndLeave && static_cast<Window>(msg.data.l[0]) == mDragSource)
          {
            ResetDrag();
            break;
          }
          if (msg.message_type == mXdndPosition && static_cast<Window>(msg.data.l[0]) == mDragSource)
          {
            int x = 0, y = 0; Window child = 0;
            const int rootX = static_cast<short>(msg.data.l[2] >> 16);
            const int rootY = static_cast<short>(msg.data.l[2] & 0xffff);
            XTranslateCoordinates(mDisplay, DefaultRootWindow(mDisplay), mPlugWnd,
                                  rootX, rootY, &x, &y, &child);
            mDropX = x / GetTotalScale();
            mDropY = y / GetTotalScale();
            XEvent reply {};
            reply.xclient.type = ClientMessage;
            reply.xclient.display = mDisplay;
            reply.xclient.window = mDragSource;
            reply.xclient.message_type = mXdndStatus;
            reply.xclient.format = 32;
            reply.xclient.data.l[0] = mPlugWnd;
            reply.xclient.data.l[1] = mDragSupported ? 1 : 0;
            reply.xclient.data.l[4] = mDragSupported ? mXdndActionCopy : 0;
            XSendEvent(mDisplay, mDragSource, 0, NoEventMask, &reply);
            XFlush(mDisplay);
            break;
          }
          if (msg.message_type == mXdndDrop && static_cast<Window>(msg.data.l[0]) == mDragSource)
          {
            if (mDragSupported)
              XConvertSelection(mDisplay, mXdndSelection, mUriList, mDropProperty,
                                mPlugWnd, msg.data.l[2]);
            else
            {
              XEvent reply {};
              reply.xclient.type = ClientMessage;
              reply.xclient.display = mDisplay;
              reply.xclient.window = mDragSource;
              reply.xclient.message_type = mXdndFinished;
              reply.xclient.format = 32;
              reply.xclient.data.l[0] = mPlugWnd;
              XSendEvent(mDisplay, mDragSource, 0, NoEventMask, &reply);
              ResetDrag();
            }
            XFlush(mDisplay);
            break;
          }
        }
        if (ev.xclient.message_type == XInternAtom(mDisplay, "WM_PROTOCOLS", 0) &&
            ev.xclient.format == 32 && (Atom)ev.xclient.data.l[0] == mWMDeleteMessage)
        {
          mCloseRequested = true;
          mTimerRunning = false;
          return;
        }
        break;
      }

      case SelectionNotify:
      {
        if (ev.xselection.selection != mXdndSelection || !mDragSource) break;
        bool accepted = false;
        const Window dragSource = mDragSource;
        const Window targetWindow = mPlugWnd;
        if (ev.xselection.property == mDropProperty)
        {
          Atom type = 0; int format = 0; unsigned long count = 0, remaining = 0;
          unsigned char* data = nullptr;
          if (XGetWindowProperty(mDisplay, mPlugWnd, mDropProperty, 0, 262144, 1,
                mUriList, &type, &format, &count, &remaining, &data) == 0 &&
              type == mUriList && format == 8 && remaining == 0 && data)
          {
            const auto paths = ParseDroppedFiles(data, count);
            XFree(data);
            data = nullptr;
            if (!paths.empty())
            {
              accepted = true;
              if (paths.size() == 1) OnDrop(paths[0].c_str(), mDropX, mDropY);
              else
              {
                std::vector<const char*> pointers;
                for (const auto& path : paths) pointers.push_back(path.c_str());
                OnDropMultiple(pointers, mDropX, mDropY);
              }
            }
          }
          if (data) XFree(data);
        }
        XEvent reply {};
        reply.xclient.type = ClientMessage;
        reply.xclient.display = mDisplay;
        reply.xclient.window = dragSource;
        reply.xclient.message_type = mXdndFinished;
        reply.xclient.format = 32;
        reply.xclient.data.l[0] = targetWindow;
        reply.xclient.data.l[1] = accepted ? 1 : 0;
        reply.xclient.data.l[2] = accepted ? mXdndActionCopy : 0;
        if (mDisplay && mPlugWnd == targetWindow)
        {
          XSendEvent(mDisplay, dragSource, 0, NoEventMask, &reply);
          XFlush(mDisplay);
        }
        ResetDrag();
        break;
      }

      default:
        break;
    }
  }
}

void* IGraphicsLinux::TimerThreadProc(void* pParam)
{
  IGraphicsLinux* pGraphics = reinterpret_cast<IGraphicsLinux*>(pParam);
  while (pGraphics->mTimerRunning)
  {
    usleep(TIMER_INTERVAL_US);
    if (pGraphics->mTimerRunning)
      pGraphics->OnDisplayTimer();
  }
  return nullptr;
}

bool IGraphicsLinux::StartTimer()
{
  if (mHostDriven || mTimerRunning)
    return true;

#if defined APP_API || defined REAPER_EXT_API
  mMainLoopTimer.reset(SWELLTimer::Create([this](Timer&) { OnDisplayTimer(); },
    std::max(1, 1000 / FPS())));
  if (!mMainLoopTimer)
  {
    fprintf(stderr, "Could not start Linux graphics main-loop timer\n");
    return false;
  }
  mTimerRunning = true;
  return true;
#else
  mTimerRunning = true;
  const int error = pthread_create(&mTimerThread, nullptr, TimerThreadProc, this);
  if (error)
  {
    mTimerRunning = false;
    mTimerThread = 0;
    fprintf(stderr, "Could not start Linux graphics timer: %s\n", strerror(error));
    return false;
  }

  return true;
#endif
}

void IGraphicsLinux::StopTimer()
{
  mTimerRunning = false;
  mMainLoopTimer.reset();
  if (mTimerThread && !pthread_equal(mTimerThread, pthread_self()))
  {
    pthread_join(mTimerThread, nullptr);
    mTimerThread = 0;
  }
}

void IGraphicsLinux::OnDisplayTimer()
{
  if (IsInPlatformCallback())
    return;
  DispatchScope dispatch(mDispatchDepth);
  // Teardown joins this timer while holding GfxMutex; skip busy frames instead of waiting.
  std::unique_lock<std::recursive_mutex> lock(GetDelegate()->GfxMutex(), std::try_to_lock);
  if (!lock.owns_lock())
    return;
  if (!mDisplay || !mPlugWnd || mCloseRequested)
    return;

  DispatchX11Events();
  if (!mDisplay || !mPlugWnd || mCloseRequested)
    return;

  if (!TooltipsEnabled() && mTooltipWindow)
    HideTooltip();
  if (mTooltipControl >= 0 && !mTooltipWindow && TooltipsEnabled() &&
      HoverMillis() - mTooltipHoverTime >= 700)
    ShowTooltip(mTooltipX, mTooltipY);

  IRECTList rects;
  if (mNeedsRedraw || IsDirty(rects))
  {
    mNeedsRedraw = false;
    if (!rects.Size())
    {
      // Full redraw (e.g. from Expose with no dirty rects yet)
      const float scale = GetBackingPixelScale();
      IRECT full(0, 0, (float)WindowWidth() / scale, (float)WindowHeight() / scale);
      rects.Add(full);
    }
    ScopedGLContext scopedGLCtx {this};
    Draw(rects);
#if defined(IGRAPHICS_GL2) || defined(IGRAPHICS_GL3)
    glXSwapBuffers(mDisplay, mPlugWnd);
#endif
  }
}

#ifndef NO_IGRAPHICS
#if defined IGRAPHICS_NANOVG
  // Unity-build the NanoVG drawing backend into the platform translation unit.
  // nanovg.c and glad.c are compiled separately via CMake sources.
  #include "IGraphicsNanoVG.cpp"
#elif defined IGRAPHICS_SKIA
  #include "IGraphicsSkia.cpp"
#else
  #error "Either NO_IGRAPHICS or a drawing backend (IGRAPHICS_NANOVG, IGRAPHICS_SKIA) must be defined"
#endif
#endif
