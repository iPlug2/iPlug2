/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>
#include <mutex>
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <dlfcn.h>
// Fontconfig is loaded via dlopen so the plugin works without it installed.
// Only the font-by-name lookup (LoadPlatformFont) needs it; embedded fonts work regardless.

// X11 headers first — they define macros (None, Bool, True, False, Status, Complex...)
// that conflict with C++ identifiers. IGraphicsLinux.h undefines them.
#include <X11/Xlib.h>
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

using namespace iplug;
using namespace igraphics;

// Timer interval in microseconds (60 fps = ~16667 us)
static constexpr int TIMER_INTERVAL_US = 16667;

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

// GTK file-chooser action codes
static constexpr int kGTK_FILE_CHOOSER_ACTION_OPEN          = 0;
static constexpr int kGTK_FILE_CHOOSER_ACTION_SAVE          = 1;
static constexpr int kGTK_FILE_CHOOSER_ACTION_SELECT_FOLDER = 2;

// GTK button presets used by message dialogs
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

  gtk.handle = dlopen("libgtk-3.so.0", RTLD_NOW | RTLD_GLOBAL);
  if (!gtk.handle)
    return nullptr;

  LOAD_GTK_SYM(gtk, init_check);
  LOAD_GTK_SYM(gtk, events_pending);
  LOAD_GTK_SYM(gtk, main_iteration_do);
  LOAD_GTK_SYM(gtk, dialog_run);
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
  // Required before any Xlib calls from multiple threads
  XInitThreads();
}

IGraphicsLinux::~IGraphicsLinux()
{
  CloseWindow();
}

static int sIgnoreX11Error(Display*, XErrorEvent*) { return 0; }

void* IGraphicsLinux::OpenWindow(void* pParent)
{
  mDisplay = XOpenDisplay(nullptr);
  if (!mDisplay)
    return nullptr;

  int screen = DefaultScreen(mDisplay);
  Window root = RootWindow(mDisplay, screen);

  // Validate pParent is an actual X11 Window, not a SWELL generic HWND pointer.
  // In standalone mode the APP host passes gHWND, which is a heap pointer —
  // XGetWindowAttributes will fail for it and we fall back to root.
  mParentWnd = root;
  if (pParent)
  {
    XErrorHandler prev = XSetErrorHandler(sIgnoreX11Error);
    XWindowAttributes wa = {};
    int ok = XGetWindowAttributes(mDisplay, (Window)(uintptr_t)pParent, &wa);
    XSync(mDisplay, 0 /* discard=False */);
    XSetErrorHandler(prev);
    if (ok)
      mParentWnd = (Window)(uintptr_t)pParent;
  }

  // Detect HiDPI scale factor. The standalone APP sets IPLUG2_SCREEN_SCALE after
  // querying gdk_window_get_scale_factor(). Plugin hosts may set GDK_SCALE.
  // Note: on Xwayland the X11 coordinate space uses physical pixels, not logical
  // pixels, so XDisplayWidth returns the logical (scaled-down) extent. Do NOT
  // compare physW against XDisplayWidth — it will always exceed the logical extent
  // on HiDPI displays. Trust gdk_window_get_scale_factor instead.
  {
    int gdkScale = 1;
    const char* s = getenv("IPLUG2_SCREEN_SCALE");
    if (!s) s = getenv("GDK_SCALE");
    if (s) gdkScale = std::clamp(atoi(s), 1, 8);
    if (gdkScale > 1)
      SetScreenScale(static_cast<float>(gdkScale));
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
  mLastPhysW = physW;
  mLastPhysH = physH;
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
  mLastPhysW = physW;
  mLastPhysH = physH;
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
  mTargetsAtom      = XInternAtom(mDisplay, "TARGETS",           0 /* False */);
  mSelDataAtom      = XInternAtom(mDisplay, "IPLUG2_SEL_DATA",   0 /* False */);

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
  if (!glXMakeCurrent(mDisplay, mPlugWnd, (GLXContext)mGLContext))
  {
    DestroyGLContext();
    if (mGLColormap) { XFreeColormap(mDisplay, mGLColormap); mGLColormap = 0; }
    XDestroyWindow(mDisplay, mPlugWnd); mPlugWnd = 0;
    XCloseDisplay(mDisplay); mDisplay = nullptr;
    return nullptr;
  }
  gladLoadGL();
  // Keep GL context current through OnViewInitialized, LayoutUI, and DrawResize —
  // NanoVG context creation, font loading, and FBO initialisation all need active GL.
  OnViewInitialized(nullptr);
  GetDelegate()->LayoutUI(this);
  SetAllControlsDirty();
  DrawResize();
  glXMakeCurrent(mDisplay, 0, nullptr);
#else
  OnViewInitialized(nullptr);
  GetDelegate()->LayoutUI(this);
  SetAllControlsDirty();
  DrawResize();
#endif

  GetDelegate()->OnUIOpen();

  // Mark dirty so the first timer tick renders immediately, even if no
  // Expose event arrives before the initial frame.
  mNeedsRedraw = true;
  StartTimer();

  return (void*)(uintptr_t)mPlugWnd;
}

void IGraphicsLinux::CloseWindow()
{
  if (!mDisplay)
    return;

  StopTimer();
  OnViewDestroyed();

#ifdef IGRAPHICS_GL
  DestroyGLContext();
#endif

  if (mBlankCursor)
  {
    XFreeCursor(mDisplay, mBlankCursor);
    mBlankCursor = 0;
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

void IGraphicsLinux::PlatformResize(bool parentHasResized)
{
  // When handling a ConfigureNotify or host-driven resize, the X server (or
  // host) already sized our window. Calling XResizeWindow here would create an
  // oscillation or fight the host's window manager.
  if (mInConfigureNotify || mInHostResize || mHostDidResize)
  {
    mHostDidResize = false;
    return;
  }

  if (mDisplay && mPlugWnd)
  {
    // X11 window must be in physical pixels (WindowWidth/Height are logical).
    auto w = (unsigned)(WindowWidth()  * GetScreenScale());
    auto h = (unsigned)(WindowHeight() * GetScreenScale());
    XResizeWindow(mDisplay, mPlugWnd, w, h);
    mLastPhysW = w;
    mLastPhysH = h;
  }
}

void IGraphicsLinux::DrawResize()
{
  std::lock_guard<std::recursive_mutex> lock(GetDelegate()->GfxMutex());
  IGRAPHICS_DRAW_CLASS::DrawResize();
}

void IGraphicsLinux::OnBeginHostResize(int physW, int physH)
{
  mInHostResize = true;
  mHostPhysW = static_cast<unsigned>(physW);
  mHostPhysH = static_cast<unsigned>(physH);
}

void IGraphicsLinux::OnEndHostResize()
{
  // Resize our X11 window to match the host's requested size exactly.
  // In Scale mode, WindowWidth()/WindowHeight() may differ from the
  // host's size due to aspect ratio correction (max(scaleX, scaleY)),
  // so use the stored host size to avoid oscillation.
  if (mDisplay && mPlugWnd && mHostPhysW && mHostPhysH)
  {
    XResizeWindow(mDisplay, mPlugWnd, mHostPhysW, mHostPhysH);
    mLastPhysW = mHostPhysW;
    mLastPhysH = mHostPhysH;
  }

  mInHostResize = false;
  mHostDidResize = true;
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
  XQueryPointer(mDisplay, mPlugWnd, &root, &child, &rootX, &rootY, &winX, &winY, &mask);
  x = (float)winX;
  y = (float)winY;
}

EMsgBoxResult IGraphicsLinux::ShowMessageBox(const char* str, const char* title,
                                             EMsgBoxType type,
                                             IMsgBoxCompletionHandlerFunc completionHandler)
{
  GTK3* g = LoadGTK3();
  if (!g || !g->message_dialog_new)
  {
    if (completionHandler)
      completionHandler(EMsgBoxResult::kOK);
    return EMsgBoxResult::kOK;
  }

  int gtkMsgType  = kGTK_MESSAGE_INFO;
  int gtkBtnType  = kGTK_BUTTONS_OK;
  switch (type)
  {
    case kMB_OKCANCEL:    gtkMsgType = kGTK_MESSAGE_QUESTION; gtkBtnType = kGTK_BUTTONS_OK_CANCEL; break;
    case kMB_YESNO:       gtkMsgType = kGTK_MESSAGE_QUESTION; gtkBtnType = kGTK_BUTTONS_YES_NO;    break;
    case kMB_YESNOCANCEL: gtkMsgType = kGTK_MESSAGE_QUESTION; gtkBtnType = kGTK_BUTTONS_OK_CANCEL; break;
    default:              break;
  }

  void* dialog = g->message_dialog_new(nullptr, 0, gtkMsgType, gtkBtnType, "%s", str);
  if (title && g->window_set_title)
    g->window_set_title(dialog, title);
  GtkPresentDialog(g, dialog);

  int resp = g->dialog_run(dialog);
  g->widget_destroy(dialog);
  GtkDrainEvents(g);

  EMsgBoxResult result = kOK;
  switch (resp)
  {
    case kGTK_RESPONSE_OK:     result = kOK;     break;
    case kGTK_RESPONSE_YES:    result = kYES;    break;
    case kGTK_RESPONSE_NO:     result = kNO;     break;
    case kGTK_RESPONSE_CANCEL: result = kCANCEL; break;
    default:                   result = kCANCEL; break;
  }

  if (completionHandler)
    completionHandler(result);
  return result;
}

bool IGraphicsLinux::RevealPathInExplorerOrFinder(WDL_String& path, bool select)
{
  // Open parent directory with xdg-open
  WDL_String dir(path);
  if (select)
  {
    char* lastSlash = strrchr(dir.Get(), '/');
    if (lastSlash)
      *lastSlash = '\0';
  }

  pid_t pid = fork();
  if (pid == 0)
  {
    execlp("xdg-open", "xdg-open", dir.Get(), nullptr);
    _exit(127);
  }
  return pid > 0;
}

void IGraphicsLinux::PromptForFile(WDL_String& fileName, WDL_String& path,
                                   EFileAction action, const char* ext,
                                   IFileDialogCompletionHandlerFunc completionHandler)
{
  GTK3* g = LoadGTK3();
  if (!g)
  {
    if (completionHandler)
      completionHandler(WDL_String{}, WDL_String{});
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

  // Add extension filter
  if (ext && ext[0] && g->file_filter_new)
  {
    void* filter = g->file_filter_new();
    WDL_String pat("*.");
    pat.Append(ext);
    if (g->file_filter_set_name)  g->file_filter_set_name(filter, ext);
    if (g->file_filter_add_pattern) g->file_filter_add_pattern(filter, pat.Get());
    if (g->file_chooser_add_filter) g->file_chooser_add_filter(dialog, filter);

    void* all = g->file_filter_new();
    if (g->file_filter_set_name)    g->file_filter_set_name(all, "All Files");
    if (g->file_filter_add_pattern) g->file_filter_add_pattern(all, "*");
    if (g->file_chooser_add_filter) g->file_chooser_add_filter(dialog, all);
  }

  // Set initial directory
  if (path.GetLength() && g->file_chooser_set_current_folder)
    g->file_chooser_set_current_folder(dialog, path.Get());

  // For save dialogs, suggest filename
  if (isSave && fileName.GetLength() && g->file_chooser_set_current_name)
    g->file_chooser_set_current_name(dialog, fileName.Get());
  GtkPresentDialog(g, dialog);

  WDL_String outFile, outPath;
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
  }

  g->widget_destroy(dialog);
  GtkDrainEvents(g);

  if (completionHandler)
    completionHandler(outFile, outPath);
}

void IGraphicsLinux::PromptForDirectory(WDL_String& dir,
                                        IFileDialogCompletionHandlerFunc completionHandler)
{
  GTK3* g = LoadGTK3();
  if (!g)
  {
    if (completionHandler)
      completionHandler(WDL_String{}, WDL_String{});
    return;
  }

  void* dialog = g->file_chooser_dialog_new(
    "Select Folder", nullptr, kGTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
    "_Cancel", kGTK_RESPONSE_CANCEL,
    "_Select", kGTK_RESPONSE_OK,
    nullptr
  );

  if (dir.GetLength() && g->file_chooser_set_current_folder)
    g->file_chooser_set_current_folder(dialog, dir.Get());
  GtkPresentDialog(g, dialog);

  WDL_String outDir;
  if (g->dialog_run(dialog) == kGTK_RESPONSE_OK)
  {
    char* chosen = g->file_chooser_get_filename(dialog);
    if (chosen)
    {
      outDir.Set(chosen);
      free(chosen);
    }
  }

  g->widget_destroy(dialog);
  GtkDrainEvents(g);

  if (completionHandler)
    completionHandler(outDir, WDL_String{});
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
  pid_t pid = fork();
  if (pid == 0)
  {
    execlp("xdg-open", "xdg-open", url, nullptr);
    _exit(127);
  }
  return pid > 0;
}

bool IGraphicsLinux::GetTextFromClipboard(WDL_String& str)
{
  if (!mDisplay || !mPlugWnd || !mClipboardAtom)
    return false;

  // Ask the clipboard owner to convert to UTF-8 and store in our scratch property
  XConvertSelection(mDisplay, mClipboardAtom, mUTF8StringAtom, mSelDataAtom,
                    mPlugWnd, 0L /* CurrentTime */);
  XFlush(mDisplay);

  // Poll up to 500 ms; the timer thread processes SelectionNotify for us
  for (int i = 0; i < 50; ++i)
  {
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

      XGetWindowProperty(mDisplay, mPlugWnd, mSelDataAtom,
                         0, 65536, 1 /* delete after read */,
                         AnyPropertyType, &type, &fmt,
                         &nitems, &remaining, &data);
      if (data && nitems > 0)
      {
        str.Set((const char*)data, (int)nitems);
        XFree(data);
        return true;
      }
      if (data) XFree(data);
      return false;
    }
  }
  return false;
}

bool IGraphicsLinux::SetTextInClipboard(const char* str)
{
  if (!mDisplay || !mPlugWnd || !mClipboardAtom)
    return false;

  mClipboardText.Set(str);
  XSetSelectionOwner(mDisplay, mClipboardAtom, mPlugWnd, 0L /* CurrentTime */);
  XFlush(mDisplay);
  return XGetSelectionOwner(mDisplay, mClipboardAtom) == mPlugWnd;
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
  if (!mDisplay || !mGLContext) return;
  glXMakeCurrent(mDisplay, mPlugWnd, (GLXContext)mGLContext);
#endif
}

void IGraphicsLinux::DeactivateGLContext()
{
#ifdef IGRAPHICS_GL
  if (!mDisplay) return;
  glXMakeCurrent(mDisplay, 0, nullptr);
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
    glXMakeCurrent(mDisplay, 0, nullptr);
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

void IGraphicsLinux::ProcessX11Events()
{
  if (!mDisplay || !mPlugWnd)
    return;

  const float scale = GetTotalScale();

  while (XPending(mDisplay))
  {
    XEvent ev;
    XNextEvent(mDisplay, &ev);

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

        // Update tracking to the actual X11 window size.
        mLastPhysW = (unsigned)ce.width;
        mLastPhysH = (unsigned)ce.height;

        const float ss = GetScreenScale();
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

        if (GetResizerMode() == EUIResizerMode::Scale)
        {
          // Scale mode: keep logical dimensions, adjust drawScale
          const int logW = Width();
          const int logH = Height();
          const float scaleX = static_cast<float>(physW) / (logW * ss);
          const float scaleY = static_cast<float>(physH) / (logH * ss);
          const float newScale = std::max(scaleX, scaleY);
          Resize(logW, logH, newScale, false);
        }
        else
        {
          // Size mode: physical pixels to logical
          const int w = static_cast<int>(physW / ss);
          const int h = static_cast<int>(physH / ss);
          Resize(w, h, 1.0f, false);
        }

        mInConfigureNotify = false;
        break;
      }

      case ButtonPress:
      case ButtonRelease:
      {
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
          // X11 has no native double-click event; detect via timestamp.
          static constexpr unsigned long kDblClickTimeoutMs = 400;
          if (left && (be.time - mLastLeftClickTime) < kDblClickTimeoutMs)
          {
            OnMouseDblClick(x, y, mod);
            mLastLeftClickTime = 0;  // reset so triple-click doesn't fire again
          }
          else
          {
            OnMouseDown(list);
            if (left)
              mLastLeftClickTime = be.time;
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
          OnMouseDrag(list);
          mCursorX = x;
          mCursorY = y;
        }
        else
        {
          OnMouseOver(x, y, mod);
          mCursorX = x;
          mCursorY = y;
        }
        break;
      }

      case LeaveNotify:
        OnMouseOut();
        break;

      case KeyPress:
      case KeyRelease:
      {
        XKeyEvent& ke = ev.xkey;
        char buf[8] = {};
        KeySym ks = 0;
        XLookupString(&ke, buf, sizeof(buf) - 1, &ks, nullptr);

        const bool shift = (ke.state & ShiftMask)   != 0;
        const bool ctrl  = (ke.state & ControlMask) != 0;
        const bool alt   = (ke.state & Mod1Mask)    != 0;

        IKeyPress keyPress(buf, KeysymToVK(ks), shift, ctrl, alt);
        const float mx = mCursorX;
        const float my = mCursorY;

        if (ev.type == KeyPress)
          OnKeyDown(mx, my, keyPress);
        else
          OnKeyUp(mx, my, keyPress);
        break;
      }

      case ClientMessage:
      {
        if ((Atom)ev.xclient.data.l[0] == mWMDeleteMessage)
          CloseWindow();
        break;
      }

      // Another app is asking us to hand over our clipboard contents
      case SelectionRequest:
      {
        XSelectionRequestEvent& req = ev.xselectionrequest;
        XEvent resp = {};
        resp.xselection.type      = SelectionNotify;
        resp.xselection.display   = mDisplay;
        resp.xselection.requestor = req.requestor;
        resp.xselection.selection = req.selection;
        resp.xselection.target    = req.target;
        resp.xselection.time      = req.time;

        if (req.target == mTargetsAtom)
        {
          Atom targets[] = { mTargetsAtom, mUTF8StringAtom, XA_STRING };
          XChangeProperty(mDisplay, req.requestor, req.property, XA_ATOM, 32,
                          PropModeReplace, (unsigned char*)targets, 3);
          resp.xselection.property = req.property;
        }
        else if (req.target == mUTF8StringAtom || req.target == XA_STRING)
        {
          XChangeProperty(mDisplay, req.requestor, req.property, req.target, 8,
                          PropModeReplace,
                          (unsigned char*)mClipboardText.Get(),
                          mClipboardText.GetLength());
          resp.xselection.property = req.property;
        }
        else
        {
          resp.xselection.property = 0 /* None */;
        }
        XSendEvent(mDisplay, req.requestor, 0 /* False */, 0, &resp);
        XFlush(mDisplay);
        break;
      }

      // We've lost clipboard ownership; discard our stored text
      case SelectionClear:
        mClipboardText.Set("");
        break;

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

void IGraphicsLinux::StartTimer()
{
  if (mHostDriven)
    return;  // host provides timer/fd callbacks; no internal thread needed
  mTimerRunning = true;
  pthread_create(&mTimerThread, nullptr, TimerThreadProc, this);
}

void IGraphicsLinux::StopTimer()
{
  if (mTimerRunning)
  {
    mTimerRunning = false;
    pthread_join(mTimerThread, nullptr);
    mTimerThread = 0;
  }
}

void IGraphicsLinux::OnDisplayTimer()
{
  if (!mDisplay || !mPlugWnd)
    return;

  // Hold the GfxMutex for the entire timer tick so that host-thread calls
  // (SendParameterValueFromDelegate, CloseWindow, resize, etc.) cannot
  // concurrently modify IGraphics state while we process events or draw.
  // The mutex is recursive so ProcessX11Events() -> DrawResize() can
  // re-enter safely.
  std::lock_guard<std::recursive_mutex> lock(GetDelegate()->GfxMutex());

  ProcessX11Events();

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
    ActivateGLContext();
    Draw(rects);
#if defined(IGRAPHICS_GL2) || defined(IGRAPHICS_GL3)
    glXSwapBuffers(mDisplay, mPlugWnd);
#endif
    DeactivateGLContext();
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
