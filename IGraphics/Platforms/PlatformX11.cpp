/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "PlatformX11.hpp"
#include "IPlugStructs.h"
#include "IPlugConstants.h"
#include <string>

#include <X11/Xlib.h>
#include <X11/Xlib-xcb.h>
#include <xcb/xcb.h>
#include <xcb/xproto.h>
#include <xcb/xcb_event.h>
#include <xcb/xcb_icccm.h>
#include <xcb/xfixes.h>
#include <sys/wait.h>
#include <poll.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <vector>
#include <memory>
#include <algorithm>
#include <list>
#include <mutex.h>

#define XK_3270  // for XK_3270_BackTab
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#define VKEY_UNSUPPORTED VKEY_UNKNOWN

// use GLX loaded by GLAD, linking with glx librariy is required otherwise
#include <glad/glad.h>
#include <glad/glad_glx.h>

#undef TRACE
#ifdef NDEBUG
#define TRACE(...)
#else
#define TRACE printf
#endif

using namespace iplug::igraphics;


// forward declarations
struct XcbPlatform;
struct RealWindow;

/// @brief Size of buffers for X11 error messages
#define XERR_MSG_SIZE (256)

// Source: https://specifications.freedesktop.org/xembed-spec/latest/messages.html

/* XEMBED messages */
#define XEMBED_EMBEDDED_NOTIFY    0
#define XEMBED_WINDOW_ACTIVATE    1
#define XEMBED_WINDOW_DEACTIVATE  2
#define XEMBED_REQUEST_FOCUS      3
#define XEMBED_FOCUS_IN           4
#define XEMBED_FOCUS_OUT          5
#define XEMBED_FOCUS_NEXT         6
#define XEMBED_FOCUS_PREV         7
/* 8-9 were used for XEMBED_GRAB_KEY/XEMBED_UNGRAB_KEY */
#define XEMBED_MODALITY_ON       10
#define XEMBED_MODALITY_OFF      11
#define XEMBED_REGISTER_ACCELERATOR     12
#define XEMBED_UNREGISTER_ACCELERATOR   13
#define XEMBED_ACTIVATE_ACCELERATOR     14

// Source: https://specifications.freedesktop.org/xembed-spec/latest/lifecycle.html
/* Flags for _XEMBED_INFO */
#define XEMBED_MAPPED           (1 << 0)


/// @brief List/set of atoms we'll use
struct AtomSet
{
  // clipboard related
  xcb_atom_t CLIPBOARD = 0;
  xcb_atom_t UTF8_STRING;
  xcb_atom_t XSEL_DATA;
  xcb_atom_t STRING;
  xcb_atom_t TEXT;
  xcb_atom_t TARGETS;
  xcb_atom_t IMAGE_PNG;
  // misc for window management
  xcb_atom_t WM_PROTOCOLS;
  xcb_atom_t WM_DELETE_WINDOW;
  // xembed atoms
  xcb_atom_t _XEMBED_INFO;
  xcb_atom_t _XEMBED;
  // atoms from "extended window manager hints"
  // see <xcb/xcb_ewmh.h> for more details
  xcb_atom_t _NET_WM_WINDOW_TYPE;
  xcb_atom_t _NET_WM_PID;
  xcb_atom_t _NET_WM_WINDOW_TYPE_DESKTOP;
  xcb_atom_t _NET_WM_WINDOW_TYPE_DOCK;
  xcb_atom_t _NET_WM_WINDOW_TYPE_TOOLBAR;
  xcb_atom_t _NET_WM_WINDOW_TYPE_MENU;
  xcb_atom_t _NET_WM_WINDOW_TYPE_UTILITY;
  xcb_atom_t _NET_WM_WINDOW_TYPE_SPLASH;
  xcb_atom_t _NET_WM_WINDOW_TYPE_DIALOG;
  xcb_atom_t _NET_WM_WINDOW_TYPE_DROPDOWN_MENU;
  xcb_atom_t _NET_WM_WINDOW_TYPE_POPUP_MENU;
  xcb_atom_t _NET_WM_WINDOW_TYPE_TOOLTIP;
  xcb_atom_t _NET_WM_WINDOW_TYPE_NOTIFICATION;
  xcb_atom_t _NET_WM_WINDOW_TYPE_COMBO;
  xcb_atom_t _NET_WM_WINDOW_TYPE_DND;
  xcb_atom_t _NET_WM_WINDOW_TYPE_NORMAL;

  void load(xcb_connection_t* conn);
};

struct XcbPlatform
{
  /// @brief Loading status
  enum ELoadStatus : uint8_t {
    /// @brief Have not attempted to connect
    kNotAttempted = 0,
    /// @brief Tried to load, but failed
    kLoadFailed = 1,
    /// @brief Loaded successfully
    kLoadSuccess = 2,
  };

  /// @brief Private flags for \c WindowOptions
  enum PrivFlags {
    NO_COLORMAP = 1 << 8,
    WM_DECORATIONS = 1 << 9,
    WM_NO_DECORATIONS = 1 << 10,
  };

  //------//
  // Xlib //
  //------//

  /**
   * @brief XErrorHandler typedef
   * @remark The typedef is listed as "not part of the Xlib specification"
   *   so make sure we have our own copy, in case it's not defined?
   */
  typedef int (*_XErrorHandler) (Display*, XErrorEvent*);

  // Pointer to old error handler, returned from XSetErrorHandler
  _XErrorHandler oldErrorHandler;

  /// @brief Xlib display, used for as few things as possible
  Display *dpy;

  //-----//
  // XCB //
  //-----//

  /// @brief XCB connection
  xcb_connection_t* mConn = nullptr;

  /// @brief default X11 screen
  int mDefaultScreen;

  /// @brief Overall loading/connected status
  ELoadStatus mStatus = kNotAttempted;

  /// @brief Are we inside a VM? If so, we probably can't control the cursor.
  ELoadStatus mIsInVm = kNotAttempted;

  /// @brief Can we control the mouse cursor?
  ELoadStatus mCanMoveCursor = kNotAttempted;

  /// @brief Do we have GLX support?
  ELoadStatus mGlxLoaded = kNotAttempted;

  /// @brief Load status of glad
  ELoadStatus mGladGLLoaded = kNotAttempted;

  /// @brief List of available screens
  std::vector<xcb_screen_t*> mScreens;

  /// @brief List of known windows
  std::vector<RealWindow*> mWindows;

  /// @brief Common atoms
  AtomSet atoms;

  /// @brief Lock for all X operations, for thread-safety
  WDL_Mutex mXLock;

  //---------------------------//
  // Clipboard and other state //
  //---------------------------//

  /// @brief Contents of the clipboard, currently only supports UTF-8 / ASCII text
  WDL_TypedBuf<uint8_t> mClipboardData;
  // ID of window that owns clipboard data or 0 if not one of ours
  xcb_window_t mClipboardOwner;

  /// @brief is Ctrl pressed?
  bool mCtrlDown = false;
  /// @brief is Alt pressed?
  bool mAltDown = false;
  /// @brief is Shift pressed?
  bool mShiftDown = false;

  /// @brief Double-click timeout in milliseconds
  /// @remark Default windows double-click timeout is 500ms,
  /// source: https://learn.microsoft.com/en-us/windows/win32/controls/ttm-setdelaytime?redirectedfrom=MSDN
  uint32_t mDblClickTimeout = 400;

  //-----------//
  // Functions //
  //-----------//

  XcbPlatform();
  ~XcbPlatform();

  // for compatibility with the conn() function in RealWindow
  inline xcb_connection_t* conn() const
  { return mConn; }

  inline bool HasGLX() const
  { return mGlxLoaded == kLoadSuccess; }

  /// @brief Try to connect to the X11 server with xcb. Updates \c mStatus .
  void Connect();

  /// @brief Create a window using the provided options.
  /// @remark This delegates to the \c Create*Window methods depending on the options
  /// @param options
  /// @return A pointer to the new window, or NULL on error
  RealWindow* CreateWindow(const WindowOptions& options);

  /// @brief Create a basic window
  RealWindow* CreateBasicWindow(const WindowOptions& options);

  /// @brief Create a window with a GLX context
  RealWindow* CreateGlxWindow(const WindowOptions& options);

  /// @brief Find and return screen number for xcb <window>.
  /// @return -1 in case of errors, the <window> is not found or its screen is not known
  int WindowToScreen(xcb_window_t wnd);

  bool CheckScreenIsTrueColor(int screen);

  bool TestInVM();

  /// @brief Test if we can control the cursor.
  /// @return true if the cursor can be controlled, false if not
  /// @remark This caches the test results in \c mCanMoveCursor
  bool TestMoveCursor();

  /// @brief Move the cursor according to \c mode
  /// @param wnd window to move relative to, if \c mode==MOUSE_MOVE_WINDOW
  /// @param mode movement mode, or what (cx, cy) is relative to
  /// @param cx cursor x
  /// @param cy cursor y
  /// @return true on success, false on failure
  bool MoveCursor(RealWindow* wnd, EMouseMoveMode mode, int cx, int cy);

  bool GetCursorPosition(int* pX, int* pY);

  bool SetClipboard(EClipboardFormat format, const void* data, size_t data_len);
  bool GetClipboard(EClipboardFormat* pFormat, WDL_TypedBuf<uint8_t>* pData);

  /// @brief Process a single X event.
  /// @details The event will be propogated to the appropriate child window(s).
  void ProcessXEvent(xcb_generic_event_t* evt);

  /// @brief Process all events currently in the X event queue
  /// @param timeout if a positive integer, wait up to this many milliseconds
  /// for more data from the X server, otherwise don't wait.
  void ProcessEventQueue(int timeout);

  /**
   * @brief Check if a cookie returned an error or not, printing a message if so
   * @param ck request cookie
   * @return true ON ERROR, so callers can do ``if (CheckCookie(...)) { return ...; }``
   */
  bool CheckCookie(xcb_void_cookie_t ck, const char* prefix) const;

  /// @brief Returns the screen at index \c i
  /// @param i either a valid screen index or \c -1 for the default screen
  /// @return a pointer to a screen, or NULL if the index was invalid
  xcb_screen_t* GetScreen(int i) const
  {
    if (i == -1) {
      i = mDefaultScreen;
    }
    return i < mScreens.size() ? mScreens[i] : nullptr;
  }

  /**
   * @brief Simple wrapper around ``xcb_change_property(mConn, XCB_PROP_MODE_REPLACE, ...)`` for convenience
   */
  void ReplaceProperty(xcb_window_t window, xcb_atom_t property, xcb_atom_t type, uint8_t format, uint32_t data_len, const void* data)
  {
    xcb_change_property(mConn, XCB_PROP_MODE_REPLACE, window, property, type, format, data_len, data);
  }
};

/// @brief The actual implementation of X11Window
struct RealWindow
{
  /// @brief Pointer to the platform object (matches with X11Window)
  PlatformX11* mPlatform;
  /// @brief xcb window reference
  /// @remark `xcb_window_t`
  uint32_t mWnd;
  /// @brief xcb colormap handle
  uint32_t mCmap;
  /// @brief Graphics context, created on-demand for drawing images in software
  xcb_gcontext_t mGc;
  /// @brief GL Context
  GLXContext mGlContext;
  /// @brief Tracks paired DrawBegin() and DrawEnd() calls.
  short mInDraw = 0;
  /// @brief Current screen ID this window belongs to
  short mScreen = 0;
  /// @brief Is the window currently visible?
  bool mVisible = false;
  /// @brief Are we showing the cursor (true) or hiding it (false)?
  bool mCursorVisible = true;
  /// @brief Are we locking the cursor?
  bool mCursorGrab = false;
  /// @brief If we are locking the cursor, this is where
  xcb_point_t mLockPos;
  /// @brief Current known cursor position IN the window.
  /// If this is (-1, -1) then the cursor is outside the window.
  xcb_point_t mCursorPos { -1, -1 };

  /// @brief The currently known bounds, may change during update processing
  xcb_rectangle_t mBounds;

  /// @brief Timestamp of last left-click, for checking on double-click events.
  /// @remark This is a uint32, meaning it will wrap at around 49 days. Not a huge issue,
  /// especially since the worst-case scenario is it might eat a double-click once ever 49 days.
  xcb_timestamp_t mLastLeftClickStamp = 0;

  /// @brief Event queue for this window
  std::list<SDL_Event> mEvents;

  //-----------//
  // Functions //
  //-----------//

  RealWindow(XcbPlatform* xp);
  ~RealWindow();

  /// @brief Convert the PlatformX11 pointer into an \c XcbPlatform pointer
  inline XcbPlatform* CastX() const
  { return reinterpret_cast<XcbPlatform*>(mPlatform); }

  /// @brief Get the \c xcb_connection_t* this window is bound to
  inline xcb_connection_t* conn() const
  { return CastX()->mConn; }

  /// @brief Initialize the window, filling in \c mWnd and \c mCmap
  /// @param options
  /// @param visualId
  /// @return success/failure
  bool CreateXWindow(const WindowOptions& options, int visualId);

  /// @brief Destroy the window and release associated data.
  void Destroy();

  void SetVisible(bool show);
  void SetTitle(const char* title);
  void EnableEmbed(bool on);
  void Resize(uint32_t w, uint32_t h);
  void Move(int32_t x, int32_t y);
  void RequestFocus();

  void SetCursorVisible(bool on);
  bool IsCursorVisible() const;

  void SetCursorGrabbed(bool lock, bool internal);
  bool IsCursorGrabbed() const;

  bool DrawBegin();
  void DrawEnd();

  /// @brief Draw rgba data to the window in the given area
  /// @param area the region of the window to draw in, also used to calculate the size of \c data
  /// @param format the pixel format
  /// @param data pixel data
  /// @return true on success, false on error
  bool DrawImage(const WRect& area, int format, const uint8_t *data);

  bool PutPixels(const xcb_rectangle_t& area, unsigned depth, unsigned data_length, const uint8_t *data);

  /// @brief Get the position of this window relative to the screen root
  xcb_point_t GetScreenPosition() const;

  void ProcessXEvent(xcb_generic_event_t* ev);

  bool PollEvent(SDL_Event* event);

  /// @brief Helper to set a window-manager atom property to a single atom value.
  /// @param atom property
  /// @param value value
  void SetWmAtom(xcb_atom_t atom, xcb_atom_t value);
};

#pragma region static helpers

static iplug::EVirtualKey KeyboardDetailToVirtualKey(unsigned int keysym);

static xcb_get_property_reply_t* GetProperty(
  XcbPlatform* xp, xcb_window_t window, xcb_atom_t property, xcb_atom_t type, uint32_t offset, uint32_t length)
{
  char errbuf[XERR_MSG_SIZE];
  xcb_generic_error_t* err = nullptr;

  auto cookie1 = xcb_get_property(xp->mConn, 0, window, property, type, offset, length);
  xcb_get_property_reply_t* prop = xcb_get_property_reply(xp->mConn, cookie1, &err);
  if (err) {
    XGetErrorText(xp->dpy, err->error_code, errbuf, sizeof(errbuf));
    TRACE("PX11:GetProperty: xcb_get_property failed: %s\n", errbuf);
    free(err);
    return nullptr;
  }
  return prop;
}

/// @brief Convert a \c WRect into an \c xcb_rectangle_t
static xcb_rectangle_t make_xrect(const WRect& r)
{
  xcb_rectangle_t bb;
  bb.x = (int16_t)r.x;
  bb.y = (int16_t)r.y;
  bb.width = (uint16_t)r.w;
  bb.height = (uint16_t)r.w;
  return bb;
}

/// @brief Convert an \c xcb_rectangle_t into a \c WRect
static WRect make_wrect(const xcb_rectangle_t& r)
{
  WRect b;
  b.x = r.x;
  b.y = r.y;
  b.w = r.width;
  b.h = r.height;
  return b;
}

/// @brief Cast a void pointer to an XID
static uint32_t voidp_to_xid(void* p)
{
  return (uint32_t)reinterpret_cast<uintptr_t>(p);
}

/// @brief Cast an xcb ID/XID to a void*
static void* xid_to_voidp(uint32_t x)
{
  return reinterpret_cast<void*>((uintptr_t)x);
}

static uint64_t get_time_ms()
{
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC_RAW, &t);
  return (t.tv_sec * 1000) + (t.tv_nsec / 1000000);
}

#pragma endregion static helpers

#pragma region Misc methods

void AtomSet::load(xcb_connection_t* conn)
{
  AtomSet& a = *this;

  // If this is non-zero assume we loaded already
  if (a.CLIPBOARD) {
    return;
  }

  const int ATOM_COUNT = 27;
  static const char *atom_names[ATOM_COUNT];
  xcb_atom_t* atoms[ATOM_COUNT];
  int ix = 0;
  #define DEF_ATOM2(name, strname) do{ atoms[ix] = &a. name ; atom_names[ix] = strname; ix++; }while(0)
  #define DEF_ATOM1(name) DEF_ATOM2(name, #name)
  DEF_ATOM1(CLIPBOARD);
  DEF_ATOM1(UTF8_STRING);
  DEF_ATOM1(XSEL_DATA);
  DEF_ATOM1(STRING);
  DEF_ATOM1(TEXT);
  DEF_ATOM1(TARGETS);
  DEF_ATOM2(IMAGE_PNG, "image/png");
  DEF_ATOM1(WM_PROTOCOLS);
  DEF_ATOM1(WM_DELETE_WINDOW);
  DEF_ATOM1(_XEMBED_INFO);
  DEF_ATOM1(_XEMBED);
  DEF_ATOM1(_NET_WM_WINDOW_TYPE);
  DEF_ATOM1(_NET_WM_PID);
  DEF_ATOM1(_NET_WM_WINDOW_TYPE_DESKTOP);
  DEF_ATOM1(_NET_WM_WINDOW_TYPE_DOCK);
  DEF_ATOM1(_NET_WM_WINDOW_TYPE_TOOLBAR);
  DEF_ATOM1(_NET_WM_WINDOW_TYPE_MENU);
  DEF_ATOM1(_NET_WM_WINDOW_TYPE_UTILITY);
  DEF_ATOM1(_NET_WM_WINDOW_TYPE_SPLASH);
  DEF_ATOM1(_NET_WM_WINDOW_TYPE_DIALOG);
  DEF_ATOM1(_NET_WM_WINDOW_TYPE_DROPDOWN_MENU);
  DEF_ATOM1(_NET_WM_WINDOW_TYPE_POPUP_MENU);
  DEF_ATOM1(_NET_WM_WINDOW_TYPE_TOOLTIP);
  DEF_ATOM1(_NET_WM_WINDOW_TYPE_NOTIFICATION);
  DEF_ATOM1(_NET_WM_WINDOW_TYPE_COMBO);
  DEF_ATOM1(_NET_WM_WINDOW_TYPE_DND);
  DEF_ATOM1(_NET_WM_WINDOW_TYPE_NORMAL);
  #undef DEF_ATOM1
  #undef DEF_ATOM2
  assert(ix == ATOM_COUNT);

  xcb_intern_atom_cookie_t ck[ATOM_COUNT];
  xcb_intern_atom_reply_t *ar;
  int i;
  for (i = 0; i < ATOM_COUNT; ++i){
    ck[i] = xcb_intern_atom(conn, 0, strlen(atom_names[i]), atom_names[i]);
  }

  for (i = 0; i < ATOM_COUNT; ++i){
    ar = xcb_intern_atom_reply(conn, ck[i], NULL);
    if (ar) {
      *atoms[i] = ar->atom;
    } else {
      *atoms[i] = 0;
      TRACE("ERROR: could not get atom '%s'\n", atom_names[i]);
    }
  }
}

#pragma endregion Misc methods


//-----------------------------------------------
// XcbPlatform implementation
#pragma region XcbImpl

/// Macro that returns an auto-releasing mutex lock
#define LockX() WDL_MutexLock(&mXLock)

/*
 * Log XLib errors for now
 */
static int XlibErrorHandler(Display *dpy, XErrorEvent *ev ){
  char errbuf[XERR_MSG_SIZE];
  XGetErrorText(ev->display, ev->error_code, errbuf, sizeof(errbuf));
  TRACE("XLib error: %s\n", errbuf);
  return 0;
}

XcbPlatform::XcbPlatform()
: mClipboardData{1024}
{
  XInitThreads();
  mStatus = kNotAttempted;
  mClipboardOwner = 0;
}

XcbPlatform::~XcbPlatform()
{
  // close XLib display if it was opened
  if(this->dpy){
      XCloseDisplay(this->dpy);
      this->dpy = NULL;
  }
  if (mConn) {
    xcb_disconnect(mConn);
    mConn = nullptr;
  }
}

void XcbPlatform::Connect()
{
#define LOG_PREFIX "PX11:Connect"
  // no need to LockX since we're only initializing it.

  // error pointer
  xcb_generic_error_t* err = nullptr;

  // if we already have a connection, nothing else to do
  if (mStatus != kNotAttempted) {
    return;
  }
  // track if we got glx
  mGlxLoaded = kLoadFailed;
  // default to failing to load
  mStatus = kLoadFailed;

  this->dpy = XOpenDisplay(nullptr);
  if (!this->dpy) {
    TRACE(LOG_PREFIX ": Could not open X display\n");
    return;
  }

  // Warning: this is global
  this->oldErrorHandler = XSetErrorHandler(&XlibErrorHandler);
  // Load the default screen
  this->mDefaultScreen = DefaultScreen(this->dpy);
  // Grab our xcb connection
  this->mConn = XGetXCBConnection(this->dpy);
  if (!this->mConn) {
    TRACE(LOG_PREFIX ": Could not get XCB connection for X display\n");
    return;
  }

  XSetEventQueueOwner(this->dpy, XCBOwnsEventQueue);

  // Try to load GLX, but don't exit if this fails.
  if (!gladLoadGLX(this->dpy, this->mDefaultScreen)) {
    TRACE(LOG_PREFIX ": Could not load GLX\n");
    mGlxLoaded = kLoadFailed;
  } else {
    mGlxLoaded = kLoadSuccess;
  }

  TRACE("INFO: Xlib/XCB FD: %d\n", xcb_get_file_descriptor(mConn));

  // Load atoms for future use
  atoms.load(mConn);

  // Iterate through screens
  xcb_screen_iterator_t iter = xcb_setup_roots_iterator(xcb_get_setup(mConn));
  for (; iter.rem; xcb_screen_next(&iter)) {
      this->mScreens.push_back(iter.data);
  }

  // Load xfixes extension
  xcb_xfixes_query_version_cookie_t cookie = xcb_xfixes_query_version(mConn, 4, 0);
  xcb_flush(mConn);
  xcb_xfixes_query_version_reply_t* reply = xcb_xfixes_query_version_reply(mConn, cookie, &err);
  if (!reply) {
      printf("Unable to load xfixes extension: codes %d,%d\n", err->major_code, err->minor_code);
      free(err);
      return;
  }
  free(reply);

  // Load successful
  mStatus = kLoadSuccess;

  // Test if we can move the cursor on start-up instead of dynamically.
  if (!TestMoveCursor()) {
    TRACE(LOG_PREFIX ": cannot move the cursor, grabbing and warping disabled\n");
  }
#undef LOG_PREFIX
}

RealWindow* XcbPlatform::CreateWindow(const WindowOptions& options)
{
#define LOG_PREFIX "PX11:Platform:CreateWindow"
  if (options.flags & WindowOptions::USE_GLES) {
    TRACE(LOG_PREFIX ": GLES not currently implemented\n");
    return nullptr;
  }
  if (options.glMajor > 0) {
    WindowOptions opts = options;
    // opts.flags |= NO_COLORMAP;
    return CreateGlxWindow(options);
  } else {
    return CreateBasicWindow(options);
  }
#undef LOG_PREFIX
}

RealWindow* XcbPlatform::CreateBasicWindow(const WindowOptions& options)
{
#define LOG_PREFIX "PX11:Platform:CreateBasicWindow"
  auto lock = LockX();
  RealWindow* w = new RealWindow(this);
  int visual_id;

  int screen = WindowToScreen(voidp_to_xid(options.parent));
  if (screen < 0) {
    TRACE(LOG_PREFIX ": could not find parent screen");
    return nullptr;
  }
  if (!CheckScreenIsTrueColor(screen)) {
    TRACE(LOG_PREFIX ":NOT IMPLEMENTED: screen visual is not 24bit RGB, not supported\n");
    return nullptr;
  }
  visual_id = mScreens[screen]->root_visual;

  if (!w->CreateXWindow(options, visual_id)) {
    delete w;
    return nullptr;
  }

  return w;
#undef LOG_PREFIX
}

RealWindow* XcbPlatform::CreateGlxWindow(const WindowOptions& options)
{
#define LOG_PREFIX "PX11:Platform:CreateWindow"
  // First we need a usable FBConfig
  static int fbcAttr[] = {
    GLX_X_RENDERABLE    , True,             // we want X able draw here
    GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,   // we are going to draw a window
    GLX_RENDER_TYPE     , GLX_RGBA_BIT,     // RGBA 8bit per color/alpha
    GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
    GLX_RED_SIZE        , 8,
    GLX_GREEN_SIZE      , 8,
    GLX_BLUE_SIZE       , 8,
    GLX_ALPHA_SIZE      , 8,
    GLX_DEPTH_SIZE      , 24,
    GLX_STENCIL_SIZE    , 8,                // in case we want use stencil (requirement for NanoVG)
    GLX_DOUBLEBUFFER    , True,             // we want double buffer
    //GLX_SAMPLE_BUFFERS  , 1,                // that is nice to have, but not critical. GLX 1.3 could have GLX_ARB_MULTISAMPLE, otherwise had no such attributes.
    //GLX_SAMPLES         , 4,
    None
  };

  auto lock = LockX();

  int attr[] = {
    GLX_CONTEXT_MAJOR_VERSION_ARB, options.glMajor,
    GLX_CONTEXT_MINOR_VERSION_ARB, options.glMinor,
    GLX_CONTEXT_FLAGS_ARB, /* GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB */ None,
    None
    // GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB | GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB, for 3.x+
  };

  // optionally, enable debug mode
  if (options.flags & WindowOptions::GL_DEBUG) {
      attr[5] |= GLX_CONTEXT_DEBUG_BIT_ARB;
  } else {
      attr[4] = 0;
  }

  // Create GLX window
  if (!HasGLX()) {
    TRACE(LOG_PREFIX ": GLX not loaded, cannot create GL window\n");
    return nullptr;
  }

  // auto-deleting FB Config array
  std::unique_ptr<GLXFBConfig, void(*)(void*)> fbcs {nullptr, &free};
  // auto-deleting window
  std::unique_ptr<RealWindow> w {new RealWindow(this)};
  // the visual ID
  int visual_id;

  int fbcCount = 0;
  fbcs.reset(glXChooseFBConfig(this->dpy, mDefaultScreen, fbcAttr, &fbcCount));
  if (!fbcs) {
    TRACE(LOG_PREFIX ": No reasonable FB Configs found\n");
    return nullptr;
  }

  // assume that fbcs[0] will work, since it matches our requirements
  if (glXGetFBConfigAttrib(dpy, fbcs.get()[0], GLX_VISUAL_ID, &visual_id) != 0) {
    TRACE(LOG_PREFIX ":BUG: best FB config has no Visual\n");
    return nullptr;
  }

  GLXContext ctx = nullptr;
  if (glXCreateContextAttribsARB) {
    ctx = glXCreateContextAttribsARB(this->dpy, fbcs.get()[0], 0, true, attr);
    if (!ctx) {
      TRACE(LOG_PREFIX ":GLX: ARB context creation failed\n");
    }
  }
  if (!ctx) {
    // Fall back to default creation method which doesn't allow attributes
    ctx = glXCreateNewContext(dpy, fbcs.get()[0], GLX_RGBA_TYPE, 0, true);
    if (ctx) {
      TRACE(LOG_PREFIX ":GLX: Legacy context creation succeeded\n");
    } else {
      TRACE(LOG_PREFIX ":GLX: Context creation failed\n");
      return nullptr;
    }
  }

  // Set the GL context for the window
  w->mGlContext = ctx;

  if (!w->CreateXWindow(options, visual_id)) {
    return nullptr;
  }

  if (xcb_flush(conn()) < 0) {
    TRACE(LOG_PREFIX ":ERR: xcb_flush failed\n");
  }

  // // Register the window with GLX
  // w->mGlWindow = glXCreateWindow(this->dpy, fbcs.get()[0], (long)w->mWnd, nullptr);
  // if (!w->mGlWindow) {
  //   TRACE(LOG_PREFIX " Could not create GL window\n");
  //   return nullptr;
  // }

  if (mGladGLLoaded == kNotAttempted) {
    mGladGLLoaded = kLoadFailed;
    glXMakeContextCurrent(dpy, w->mWnd, w->mWnd, w->mGlContext);
    if (!gladLoadGL()) {
      TRACE(LOG_PREFIX " gladLoadGL failed\n");
      return nullptr;
    }
    glXMakeContextCurrent(dpy, XCB_NONE, XCB_NONE, nullptr);
    mGladGLLoaded = kLoadSuccess;
  }

  // return the window, taking it from unique_ptr so it doesn't get free'd
  return w.release();

#undef LOG_PREFIX
}

int XcbPlatform::WindowToScreen(xcb_window_t wnd)
{
  auto lock = LockX();
  xcb_query_tree_reply_t *reply = xcb_query_tree_reply(mConn, xcb_query_tree(mConn, wnd), NULL);
  if(reply){
    xcb_window_t root = reply->root;
    free(reply);
    const xcb_setup_t *setup = xcb_get_setup(mConn);
    int32_t screen_count = xcb_setup_roots_length(setup);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    for(int i = 0; i < screen_count; ++i) {
      if(iter.data->root == root){
        return i;
      }
      xcb_screen_next(&iter);
    }
  }
  return -1;
}

bool XcbPlatform::CheckScreenIsTrueColor(int screen)
{
  auto lock = LockX();
  int vid = mScreens[screen]->root_visual;

  xcb_visualtype_t* vt = nullptr;
  xcb_depth_iterator_t dit = xcb_screen_allowed_depths_iterator(mScreens[screen]);
  for (; dit.rem && !vt; xcb_depth_next(&dit)) {
      xcb_visualtype_iterator_t vit = xcb_depth_visuals_iterator(dit.data);
      for(; vit.rem; xcb_visualtype_next(&vit)) {
          if(vid == vit.data->visual_id){
              vt = vit.data;
              break;
          }
      }
  }
  // we couldn't find the visualtype, so assume false
  return vt && (vt->_class == XCB_VISUAL_CLASS_TRUE_COLOR)
      && (vt->red_mask == 0xff0000) && (vt->green_mask == 0xff00) && (vt->blue_mask == 0xff);
}

bool XcbPlatform::TestInVM()
{
#define LOG_PREFIX "PX11:TestInVM"
  // Assume locked by caller

  if (mCanMoveCursor != 0) {
    return mCanMoveCursor == kLoadSuccess;
  }
  // Assume in a VM unless we think otherwise
  mCanMoveCursor = kLoadSuccess;

  // try to open the subprocess, looking for VMware devices
  FILE *out = popen("lsusb -d 0e0f:", "r");
  if (!out)
  {
    return true;
  }
  std::string buf;
  buf.resize(1024 * 4);
  size_t last = 0;
  do
  {
    ssize_t len = fread(buf.data(), 1, buf.size() - last, out);
    if (len < 0)
    {
      break;
    }
    last += (size_t)len;
    buf.resize(last + 1024);
  } while(1);
  pclose(out);

  // See if Virtual Mouse is in the list of devices
  if (buf.find("Virtual Mouse") != std::string::npos)
  {
    return true;
  }

  // Probably not in a VM, or at least not in a way we care about.
  mIsInVm = kLoadFailed;
  return false;
#undef LOG_PREFIX
}

bool XcbPlatform::TestMoveCursor()
{
#define LOG_PREFIX "PX11:TestMoveCursor"
  // assume locked by caller

  // If we've already tested, just return the results of the test
  if (mCanMoveCursor != 0) {
    return mCanMoveCursor == kLoadSuccess;
  }
  // Assume we failed unless we say otherwise
  mCanMoveCursor = kLoadFailed;

  // If we're in a VM, assume we can't control the cursor.
  if (TestInVM())
  {
    return false;
  }

  // To test, we create a window, grab the cursor, move it, and check if we got any events.
  xcb_screen_t* defScreen = mScreens[mDefaultScreen];
  WindowOptions opts;
  opts.bounds = WRect{.x = 0, .y = 0, .w = 20, .h = 20};
  opts.parent = xid_to_voidp(defScreen->root);
  opts.flags |= WM_NO_DECORATIONS;
  std::unique_ptr<RealWindow> wnd {CreateWindow(opts)};
  if (!wnd) {
    return false;
  }

  // how long to run the test, about ~1/3 of a second
  const int TEST_TIME = 300;
  // current time
  uint64_t tNow = get_time_ms();
  // test end time
  uint64_t tEnd = tNow + TEST_TIME;
  SDL_Event event;

  // original X and Y of the cursor
  int originalX, originalY;
  if (!GetCursorPosition(&originalX, &originalY)) {
    return false;
  }

  auto isNear = [](int a, int b, int max) -> bool {
    int d = a - b;
    if (d < 0) d = -d;
    return d <= max;
  };

  // set visible
  wnd->SetVisible(true);
  wnd->Move(0, 0);
  wnd->Resize(20, 20);
  ProcessEventQueue(TEST_TIME);
  // Move pointer to our window
  xcb_warp_pointer(mConn, XCB_NONE, wnd->mWnd, 0,0,0,0, 5, wnd->mBounds.height - 5);

  // Work through the events until our test timeout ends.
  // If we hit state 3, then we can't control the mouse.
  int state = 0;
  while (tNow < tEnd) {
    ProcessEventQueue(TEST_TIME);
    while (wnd->PollEvent(&event)) {
      int lastState = state;
      if (state == 0 && event.type == SDL_EVENT_WINDOW_SHOWN) {
        state = 2;
      } else if (state == 2 && event.type == SDL_EVENT_MOUSE_MOTION) {
        state = 3;
      } else if (state == 3 && event.type == SDL_EVENT_WINDOW_MOUSE_LEAVE) {
        state = 4;
      } else if (state == 4) {
        // If the position of the mouse is where it was initially, then there's
        // something forcing the mouse to that spot (e.g. VM mouse integration).
        // It could be another program grabbing the mouse, but better safe than sorry.
        int mX, mY;
        if (GetCursorPosition(&mX, &mY)) {
          if (isNear(mX, originalX, 5) && isNear(mY, originalY, 5)) {
            // failure state
            state = 5;
          } else {
            state = 6;
          }
        } else {
          // go back to step 1
          state = 1;
        }
      }
      if (state != lastState) {
        TRACE(LOG_PREFIX ": state = %d\n", state);
      }
    }
    tNow = get_time_ms();
  }

  // Move cursor back to where it started
  xcb_warp_pointer(mConn, XCB_NONE, mScreens[mDefaultScreen]->root, 0,0,0,0, (int16_t)originalX, (int16_t)originalY);

  // flush the connection
  xcb_flush(mConn);
  // only if we got to state 2, but not state 3, did this work
  if (state == 3 || state == 6) {
    mCanMoveCursor = kLoadSuccess;
  }

  // Return success or failure
  return mCanMoveCursor == kLoadSuccess;
#undef LOG_PREFIX
}

bool XcbPlatform::MoveCursor(RealWindow* wnd, EMouseMoveMode mode, int cx, int cy)
{
  auto lock = LockX();
  if (!TestMoveCursor()) {
    return false;
  }

  int screenIx = WindowToScreen(wnd->mWnd);
  xcb_screen_t* screen = mScreens[screenIx];
  xcb_void_cookie_t ck;
  int16_t cx16 = (int16_t)cx;
  int16_t cy16 = (int16_t)cy;
  if (mode == MOUSE_MOVE_WINDOW)
  {
    auto& re = wnd->mBounds;
    ck = xcb_warp_pointer(mConn, wnd->mWnd, wnd->mWnd, 0, 0, re.width, re.height, cx16, re.height - cy16);
  }
  else if (mode == MOUSE_MOVE_RELATIVE)
  {
    ck = xcb_warp_pointer(mConn, XCB_NONE, XCB_NONE, 0, 0, 0, 0, cx16, cy16);
  }
  else if (mode == MOUSE_MOVE_SCREEN)
  {
    ck = xcb_warp_pointer(mConn, XCB_NONE, screen->root, 0, 0, 0, 0, cx16, cy16);
  }
  /*
  xcb_flush(mConn);
  xcb_generic_error_t* err = xcb_request_check(mConn, ck);
  if (err)
  {
    char msg[XERR_MSG_SIZE];
    XGetErrorText(dpy, err->error_code, msg, XERR_MSG_SIZE);
    TRACE("PX11:MoveCursor: error %s\n", msg);
    free(err);
    return false;
  }
    */
  return true;
}

bool XcbPlatform::GetCursorPosition(int* pX, int* pY)
{
  auto lock = LockX();
  const xcb_setup_t *setup = xcb_get_setup(mConn);
  xcb_screen_iterator_t screen_iter = xcb_setup_roots_iterator(setup);
  xcb_screen_t *screen = screen_iter.data;
  xcb_window_t root_window = screen->root;

  xcb_query_pointer_cookie_t cookie = xcb_query_pointer(mConn, root_window);
  xcb_query_pointer_reply_t *reply = xcb_query_pointer_reply(mConn, cookie, NULL);
  if (reply) {
      *pX = reply->root_x;
      *pY = reply->root_y;
      free(reply); // Important: Free the reply!
      return true;
  } else {
      return false;
  }
}

bool XcbPlatform::SetClipboard(EClipboardFormat format, const void* data, size_t data_len)
{
/*
  This probably needs to be done on a root window, not embedded.

  xcb_void_cookie_t cookie1 = xcb_set_selection_owner_checked(
      x->conn, xw->wnd, XCBT_ATOM_CLIPBOARD(x), XCB_CURRENT_TIME);
  xcb_void_cookie_t cookie2 = xcb_set_selection_owner_checked(
      x->conn, xw->wnd, XCB_ATOM_PRIMARY, XCB_CURRENT_TIME);
  xcbt_flush(x);
  xcb_generic_error_t* err1 = xcb_request_check(x->conn, cookie1);
  xcb_generic_error_t* err2 = xcb_request_check(x->conn, cookie2);
  int ok = !err1 && !err2;
  if (ok)
  {
    // Data length includes terminating null
    x->clipboard_length = strlen(str) + 1;
    x->clipboard_data = calloc(1, x->clipboard_length);
    memcpy(x->clipboard_data, str, x->clipboard_length);
    x->clipboard_owner = xw->wnd;
  }
  free(err1);
  free(err2);
  return ok;
  */

  // currently only support UTF8
  if (format != CLIPBOARD_FORMAT_UTF8) {
    return false;
  }

  // For now we implement this using xclip.
  FILE* fd = popen("xclip -i -selection c", "w");
  if (fd)
  {
    fwrite(data, 1, data_len, fd);
    pclose(fd);
    return true;
  }
  else
  {
    return false;
  }
}

bool XcbPlatform::GetClipboard(EClipboardFormat *pFormat, WDL_TypedBuf<uint8_t>* pData)
{
#define LOG_PREFIX "PX11:GetClipboard"
  if (!pFormat) {
    TRACE(LOG_PREFIX ": pFormat cannot be NULL\n");
    return false;
  }
  // TODO check formats for real
  // Do that either using xclip -o -t TARGETS and string search, or
  // implementing it ourselves (the preferred method)
  if (*pFormat == CLIPBOARD_FORMAT_UNKNOWN) {
    *pFormat = CLIPBOARD_FORMAT_UTF8;
  }

  if (pData) {
    const int BUF_INCREMENT = 2048;
    // For now we implement this using xclip.
    FILE* fd = popen("xclip -o -selection c", "r");
    if (fd) {
      pData->Resize(0);
      while (true) {
        // resize buffer and set pointer to append to it
        pData->Resize(pData->GetSize() + BUF_INCREMENT);
        void* dst_ptr = pData->Get() + pData->GetSize() - BUF_INCREMENT;
        size_t amount = fread(dst_ptr, 1, BUF_INCREMENT, fd);
        if ((int)amount < BUF_INCREMENT) {
          // resize buffer to real size
          pData->Resize(pData->GetSize() + (int)amount);
          break;
        }
      }

      int exitCode = pclose(fd);
      return exitCode == 0;
    } else {
      return false;
    }
  }
  return true;

  /*
  // Either we don't own the window with the clipboard, or we don't know who does
  x->clipboard_owner = 0;
  x->clipboard_length = 0;
  free(x->clipboard_data);
  x->clipboard_data = NULL;

  xcb_convert_selection(x->conn, xw->wnd,
      XCBT_ATOM_CLIPBOARD(x), XCBT_ATOM_UTF8_STRING(x), XCBT_ATOM_CLIPBOARD(x), XCB_CURRENT_TIME);
  xcbt_flush(x);

  struct timespec now;
  struct timespec until;
  clock_gettime(CLOCK_MONOTONIC_RAW, &now);
  until = now;
  // Default timeout is 1 second
  until.tv_sec += 1;

  // We have a timeout because getting the clipboard might fail.
  // https://jtanx.github.io/2016/08/19/a-cross-platform-clipboard-library/#linux
  while (x->clipboard_length == 0 && timespec_cmp(&now, &until) < 0)
  {
    xcbt_process((xcbt)x);
    clock_gettime(CLOCK_MONOTONIC_RAW, &now);
  }
  if (x->clipboard_length > 0)
  {
    *length = x->clipboard_length;
    return x->clipboard_data;
  }
  else
  {
    *length = 0;
    return NULL;
  }
  */
#undef LOG_PREFIX
}

void XcbPlatform::ProcessXEvent(xcb_generic_event_t* evt)
{
#define LOG_PREFIX "PX11:Platform:ProcessXEvent"
  // assume locked by caller
  if (!evt) {
    return;
  }

  // The target window for this event, if applicable.
  // Updated by each event type accordingly.
  xcb_window_t destWnd = 0;

  switch(evt->response_type & ~0x80) {
    case 0:
    {
      // type 0 means error
      auto err = (xcb_value_error_t*) evt;
      auto msg = xcb_event_get_error_label(err->error_code);
      // ignore errors for certain opcodes
      if (err->major_opcode != XCB_BUTTON_PRESS) {
        TRACE(LOG_PREFIX ":XCB ERROR: code %d, sequence %d, value %d, opcode %d:%d\n  message: %s\n",
          err->error_code, err->sequence, err->bad_value, err->minor_opcode, err->major_opcode, msg);
      }

      break;
    }

    // These all have the window as the "event" field.
    case XCB_KEY_PRESS:
    case XCB_KEY_RELEASE:
    case XCB_BUTTON_PRESS:
    case XCB_BUTTON_RELEASE:
    case XCB_MOTION_NOTIFY:
    case XCB_ENTER_NOTIFY:
    case XCB_LEAVE_NOTIFY:
    {
      auto e = (xcb_enter_notify_event_t*)evt;
      destWnd = e->event;
      break;
    }

    // the window is still in the "event" field, but it's at a different offset
    case XCB_FOCUS_IN:
    case XCB_FOCUS_OUT:
    {
      auto e = (xcb_focus_in_event_t*) evt;
      destWnd = e->event;
      break;
    }

    // not sure what to do with this, no window so just ignore it for now
    case XCB_KEYMAP_NOTIFY:
      break;

    // These are all window events with the same general layout,
    // so we can grab the window ID the same way from all of them.
    case XCB_EXPOSE:
    case XCB_DESTROY_NOTIFY:
    case XCB_UNMAP_NOTIFY:
    case XCB_MAP_NOTIFY:
    case XCB_MAP_REQUEST:
    case XCB_REPARENT_NOTIFY:
    case XCB_CONFIGURE_NOTIFY:
    case XCB_GRAVITY_NOTIFY:
    case XCB_CIRCULATE_NOTIFY:
    case XCB_CIRCULATE_REQUEST:
    case XCB_CLIENT_MESSAGE:
    {
      auto e = (xcb_configure_notify_event_t*)evt;
      // double-check that it's actually for the correct window
      if (e->event == e->window) {
        destWnd = e->window;
      }
      break;
    }

    case XCB_PROPERTY_NOTIFY:
    {
      auto e = (xcb_property_notify_event_t*) evt;
      destWnd = e->window;
      // This is another way in which we can receive clipboard data.
      // Maybe because of Xwayland?
      if (e->atom == atoms.CLIPBOARD) {
        // TODO receive clipboard
        // xcbt_receive_clipboard(xw, XCBT_ATOM_CLIPBOARD(x));
      }
      break;
    }
    case XCB_SELECTION_CLEAR: {
      // A window has been asked to not be the clipboard owner anymore.
      // We don't always receive this event, maybe due to Xwayland or XEmbed?
      auto e = (xcb_selection_clear_event_t*) evt;
      if (e->owner != mClipboardOwner) {
        mClipboardOwner = 0;
        mClipboardData.Resize(0);
      }
      // TODO Now we want to know who the new owner is.
      break;
    }
    case XCB_SELECTION_REQUEST:
    {
      // This is clipboard-related request
      xcb_selection_request_event_t* e = (xcb_selection_request_event_t*) evt;
      if (e->property == XCB_NONE) {
        e->property = e->target;
      }

      bool valid = true;
      if (e->target == atoms.UTF8_STRING
          || e->target == atoms.STRING
          || e->target == atoms.TEXT) {
        // Request clipboard contents
        ReplaceProperty(
          e->requestor, e->property, e->target, 8, mClipboardData.GetSize(), mClipboardData.GetFast());
      }
      else if (e->target == atoms.TARGETS) {
        // Request to see what type of targets we support
        xcb_atom_t targets[] = {
          atoms.TARGETS,
          atoms.UTF8_STRING,
          atoms.STRING,
          atoms.TEXT,
        };
        ReplaceProperty(
          // window, property, type
          e->requestor, e->property, XCB_ATOM_ATOM,
          // format
          sizeof(xcb_atom_t) * 8,
          // size, data
          sizeof(targets) / sizeof(xcb_atom_t), targets);
      } else {
        // not a request we can process
        valid = false;
      }

      xcb_selection_notify_event_t notify = {0};
      notify.response_type = XCB_SELECTION_NOTIFY;
      notify.time = XCB_CURRENT_TIME;
      notify.requestor = e->requestor;
      notify.selection = e->selection;
      notify.target = e->target;
      notify.property = valid ? e->property : XCB_NONE;
      xcb_send_event(mConn, 0, e->requestor, XCB_EVENT_MASK_PROPERTY_CHANGE, (char*)&notify);
      xcb_flush(mConn);
      break;
    }
    case XCB_SELECTION_NOTIFY:
    {
      // We requested clipoard data and it has arrived.
      // For now we assume the data is in the format requested, but potentially it's not?
      // Not sure what to do with that yet.
      auto e = (xcb_selection_notify_event_t*) evt;
      if (e->selection == atoms.CLIPBOARD && e->property != XCB_NONE) {
        xcb_icccm_get_text_property_reply_t prop;
        xcb_get_property_cookie_t cookie =
            xcb_icccm_get_text_property(mConn, e->requestor, e->property);

        if (xcb_icccm_get_text_property_reply(mConn, cookie, &prop, NULL)) {
          // resize clipboard data and copy it
          mClipboardData.Resize(prop.name_len + 1);
          memcpy(mClipboardData.Get(), prop.name, prop.name_len);
          // ensure we have a null-terminator
          mClipboardData.Get()[prop.name_len] = 0;
          // Wipe the property and then delete it. Not sure why deleting is
          // required, but that's what the examples I've seen do.
          xcb_icccm_get_text_property_reply_wipe(&prop);
          xcb_delete_property(mConn, e->requestor, e->property);
        } else {
          TRACE(LOG_PREFIX ": failed to get clipboard reply\n");
        }
      }
      break;
    }
    default:
      TRACE(LOG_PREFIX ": Event %d (%s)\n", evt->response_type & ~0x80, evt->response_type & 0x80 ? "Synthetic" : "Native");
      return;
  }

  for (unsigned i = 0; i < mWindows.size(); i++) {
    auto wnd = mWindows[i];
    if (wnd->mWnd == destWnd) {
      wnd->ProcessXEvent(evt);
      break;
    }
  }
#undef LOG_PREFIX
}

void XcbPlatform::ProcessEventQueue(int timeout)
{
  xcb_generic_event_t* evt;
  xcb_flush(mConn);
  if (timeout > 0) {
    // make it so we can wait for events from XCB
    pollfd pfd;
    pfd.fd = xcb_get_file_descriptor(mConn);
    pfd.events = POLL_IN;
    // Wait for events from the xcb file descriptor
    poll(&pfd, 1, timeout);
  }
  auto lock = LockX();
  while((evt = xcb_poll_for_event(mConn))) {
    ProcessXEvent(evt);
    free(evt);
  }
}

bool XcbPlatform::CheckCookie(xcb_void_cookie_t ck, const char* prefix) const
{
#ifdef NDEBUG
  (void)prefix;
  xcb_generic_error_t* err = xcb_request_check(mConn, ck);
  if (err) {
    free(err);
    return true;
  }
  return false;
#else
  char msgbuf[XERR_MSG_SIZE];
  xcb_generic_error_t* err = xcb_request_check(mConn, ck);
  if (err)
  {
    XGetErrorText(dpy, err->error_code, msgbuf, sizeof(msgbuf));
    TRACE("%s:XCB: %s\n", prefix, msgbuf);
    free(err);
    return true;
  }
  return false;
#endif
}

#undef LockX
#pragma endregion XcbImpl

//---------------------------//
// RealWindow Implementation //
//---------------------------//
#pragma region RealWindow
#define LockX() WDL_MutexLock(&CastX()->mXLock)

RealWindow::RealWindow(XcbPlatform* xp)
: mWnd(0)
, mCmap(0)
, mGc(0)
, mGlContext(nullptr)
, mInDraw(0)
, mVisible(false)
{
  mPlatform = reinterpret_cast<PlatformX11*>(xp);
}

RealWindow::~RealWindow()
{
  Destroy();
}

bool RealWindow::CreateXWindow(const WindowOptions& options, int visual_id)
{
#define LOG_PREFIX "PX11:Window:CreateXWindow"
  uint32_t eventmask =
    XCB_EVENT_MASK_EXPOSURE | // we want to know when we need to redraw
    XCB_EVENT_MASK_STRUCTURE_NOTIFY | // get varius notification messages like configure, reparent, etc.
    XCB_EVENT_MASK_PROPERTY_CHANGE | // useful when something will change our property
    XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE  |  // mouse clicks
    XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE  |      // keyboard is questionable accordung to XEMBED
    XCB_EVENT_MASK_ENTER_WINDOW | XCB_EVENT_MASK_LEAVE_WINDOW |   // mouse entering/leaving
    XCB_EVENT_MASK_POINTER_MOTION // mouse motion
    ;

  const auto xp = CastX();
  char errbuf[XERR_MSG_SIZE];

  bool hasCmap = !(options.flags & XcbPlatform::NO_COLORMAP);
  xcb_generic_error_t* err = nullptr;


  mWnd = xcb_generate_id(conn());
  uint32_t cmap = hasCmap ? xcb_generate_id(conn()) : xp->mScreens[0]->default_colormap;
  if (hasCmap) {
    mCmap = cmap;
  }
  // window attributes
  uint32_t wa[] = { eventmask, cmap, 0 };
  // window's value mask.
  uint32_t value_mask = XCB_CW_EVENT_MASK | XCB_CW_COLORMAP;
  // parent XID
  uint32_t parent = voidp_to_xid(options.parent);

  if (hasCmap) {
    auto ck = xcb_create_colormap_checked(conn(), XCB_COLORMAP_ALLOC_NONE, mCmap, parent, visual_id);
    if (xp->CheckCookie(ck, LOG_PREFIX)) {
      return false;
    }
  }
  xcb_rectangle_t bb = make_xrect(options.bounds);
  xcb_void_cookie_t create_ok = xcb_create_window_checked(
    conn(), XCB_COPY_FROM_PARENT, mWnd,
    parent, bb.x, bb.y, bb.width, bb.height, 0,
    XCB_WINDOW_CLASS_INPUT_OUTPUT, visual_id, value_mask, wa);
  err = xcb_request_check(conn(), create_ok);
  if (err)
  {
    XGetErrorText(xp->dpy, err->error_code, errbuf, sizeof(errbuf));
    TRACE(LOG_PREFIX ": xcb_create_window failed: %s\n", errbuf);
    return false;
  }

  // Window manager hints
  if (options.flags & XcbPlatform::WM_DECORATIONS) {
    SetWmAtom(xp->atoms.WM_PROTOCOLS, xp->atoms.WM_DELETE_WINDOW);
  }

  if (options.flags & XcbPlatform::WM_NO_DECORATIONS) {
    SetWmAtom(xp->atoms._NET_WM_WINDOW_TYPE, xp->atoms._NET_WM_WINDOW_TYPE_SPLASH);
  }

  // enable embedding by default
  this->EnableEmbed(true);

  // Add this to the list of managed windows.
  xp->mWindows.push_back(this);

  return true;
#undef LOG_PREFIX
}

void RealWindow::Destroy()
{
  auto lock = LockX();
  auto xp = CastX();
  if (mGlContext) {
    if (glXGetCurrentContext() == mGlContext) {
      // Set the context to not be the one we're about to destroy
      glXMakeContextCurrent(xp->dpy, 0, 0, nullptr);
    }
    glXDestroyContext(xp->dpy, mGlContext);
    mGlContext = nullptr;
  }
  if (mWnd) {
    xcb_destroy_window(conn(), mWnd);
    mWnd = 0;
  }
  if (mCmap) {
    xcb_free_colormap(conn(), mCmap);
    mCmap = 0;
  }
  if (mGc) {
    xcb_free_gc(conn(), mGc);
    mGc = 0;
  }
  // Remove self from list of known windows
  for (auto it = xp->mWindows.begin(); it != xp->mWindows.end(); ++it) {
    if (*it == this) {
      xp->mWindows.erase(it);
      break;
    }
  }
}

bool RealWindow::DrawBegin()
{
#define LOG_PREFIX "PX11:Window:DrawBegin"
  auto lock = LockX();
  auto xp = CastX();
  mInDraw++;
  if (mInDraw == 1 && mGlContext) {
    if (!glXMakeContextCurrent(xp->dpy, mWnd, mWnd, mGlContext)) {
      TRACE(LOG_PREFIX ":BUG: glXMakeContextCurrent failed\n");
      return false;
    }
  }
  if (mInDraw > 1 && mGlContext) {
    GLXContext old = glXGetCurrentContext();
    if (old != mGlContext) {
      TRACE(LOG_PREFIX ":BUG: reentrant draw, but different GL context\n");
      return false;
    }
  }
  return true;
#undef LOG_PREFIX
}

void RealWindow::DrawEnd()
{
#define LOG_PREFIX "PX11:Window:DrawEnd"
  auto lock = LockX();
  if (mInDraw > 0) {
    mInDraw--;
    if (mInDraw == 0 && mGlContext) {
      // Swap buffers
      glXSwapBuffers(CastX()->dpy, mWnd);
      // Unset the context. IMPORTANT!! If we don't do this every time, then
      // we can't render off-thread which causes problems.
      glXMakeContextCurrent(CastX()->dpy, 0, 0, nullptr);
    }
  } else {
    TRACE(LOG_PREFIX ":BUG: draw end without begin\n");
  }
#undef LOG_PREFIX
}

void RealWindow::SetTitle(const char* title)
{
  auto lock = LockX();
  size_t title_len = strlen(title);
  CastX()->ReplaceProperty(mWnd, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8, title_len, title);
  CastX()->ReplaceProperty(mWnd, XCB_ATOM_WM_ICON_NAME, XCB_ATOM_STRING, 8, title_len, title);
}

void RealWindow::SetVisible(bool show)
{
  auto lock = LockX();
  // if there's nothing to do, skip
  if (show == this->mVisible) {
    return;
  }
  if (show) {
    xcb_map_window(conn(), mWnd);
  } else {
    xcb_unmap_window(conn(), mWnd);
  }
  // Do NOT update mVisible, we'll do that when we receive the notification
  // from the X server directly.
}

void RealWindow::Resize(uint32_t w, uint32_t h)
{
  auto lock = LockX();
  uint32_t values[] = { w, h };
  auto cookie1 = xcb_configure_window(
    conn(), mWnd, XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
  xcb_flush(conn());
  CastX()->CheckCookie(cookie1, "X11Window:Resize:");
}

void RealWindow::Move(int32_t x, int32_t y)
{
  if (x < 0) x = 0;
  if (y < 0) y = 0;
  uint32_t values[] = { (uint32_t)x, (uint32_t)y };
  auto lock = LockX();
  xcb_configure_window(conn(), mWnd, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, values);
  xcb_flush(conn());
}

void RealWindow::RequestFocus()
{
  auto lock = LockX();
  xcb_set_input_focus(conn(), XCB_INPUT_FOCUS_POINTER_ROOT, mWnd, XCB_CURRENT_TIME);
  // xcb_set_input_focus_checked(xcbt_conn(mX), XCB_INPUT_FOCUS_POINTER_ROOT, mPlugWnd->wnd, XCB_CURRENT_TIME);
}

void RealWindow::SetCursorVisible(bool show)
{
  if (mCursorVisible == show) {
    // nothing to do
    return;
  }
  mCursorVisible = show;
  auto lock = LockX();
  if (!CastX()->TestMoveCursor()) {
    return;
  }
  // https://stackoverflow.com/questions/57841785/how-to-hide-cursor-in-xcb
  if (mCursorVisible) {
    xcb_xfixes_show_cursor(conn(), mWnd);
  } else {
    xcb_xfixes_hide_cursor(conn(), mWnd);
  }
}

bool RealWindow::IsCursorVisible() const
{
  return mCursorVisible;
}

void RealWindow::SetCursorGrabbed(bool locked, bool internal)
{
  if (mCursorGrab == locked) {
    // nothing to do
    return;
  }
  auto lock = LockX();
  auto xp = CastX();
  mCursorGrab = locked;
  if (mCursorGrab) {
    mLockPos = mCursorPos;
  }
  // only do this if we're running it internally, or if we're safe to do so
  if (internal || xp->TestMoveCursor()) {
    if (mCursorGrab) {
      auto ck = xcb_grab_pointer(
        conn(),
        true,
        mWnd,
        XCB_EVENT_MASK_BUTTON_MOTION | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE,
        XCB_GRAB_MODE_ASYNC,
        XCB_GRAB_MODE_ASYNC,
        mWnd,
        XCB_NONE,
        XCB_CURRENT_TIME
      );
      auto reply = xcb_grab_pointer_reply(conn(), ck, NULL);
      if (reply && (reply->status == XCB_GRAB_STATUS_SUCCESS)) {
        free(reply);
      }
    } else {
      xcb_ungrab_pointer(conn(), XCB_CURRENT_TIME);
    }
  }
}

bool RealWindow::IsCursorGrabbed() const
{
  return mCursorGrab;
}

void RealWindow::EnableEmbed(bool on)
{
  // assume locked by caller
  auto xp = CastX();
  if (on) {
    // fields: version, flags
    // version is always 0, flags is a bitmask
    uint32_t info[] = { 0, 0 }; // version 0, not mapped
    if (mVisible) {
      // request to be visible
      info[1] |= XEMBED_MAPPED;
    }
    xp->ReplaceProperty(mWnd, xp->atoms._XEMBED_INFO, xp->atoms._XEMBED_INFO, 32, 2, info);
  } else {
    xcb_delete_property(conn(), mWnd, xp->atoms._XEMBED_INFO);
  }
}

xcb_point_t RealWindow::GetScreenPosition() const
{
  xcb_point_t pt { -1, -1 };

  // Get the window position relative to the screen root.
  xcb_query_tree_cookie_t cookie1 = xcb_query_tree(conn(), mWnd);
  xcb_query_tree_reply_t* tree = xcb_query_tree_reply(conn(), cookie1, NULL);
  if (!tree) {
    return pt;
  }

  xcb_translate_coordinates_cookie_t cookie2 = xcb_translate_coordinates(
      conn(), mWnd, tree->root, mBounds.x, mBounds.y);
  xcb_translate_coordinates_reply_t* trans = xcb_translate_coordinates_reply(conn(), cookie2, NULL);
  if (!trans) {
    free(tree);
    return pt;
  }

  pt.x = trans->dst_x;
  pt.y = trans->dst_y;
  free(tree);
  free(trans);
  return pt;
}

bool RealWindow::DrawImage(const WRect& area, int format, const uint8_t* data)
{
  if (format != kPF_RGBA8) {
    return false;
  }
  xcb_rectangle_t bounds = make_xrect(area);
  unsigned dataLen = area.w * area.h * 4;
  unsigned depth = CastX()->GetScreen(-1)->root_depth;
  return PutPixels(bounds, depth, dataLen, data);
}

bool RealWindow::PutPixels(const xcb_rectangle_t& bounds, unsigned depth, unsigned data_length, const uint8_t* data)
{
#define LOG_PREFIX "PX11:Window:DrawImage"
  auto lock = LockX();

  // no reason we can't, but better to not allow bugs to creep in.
  if (!mInDraw) {
    TRACE(LOG_PREFIX ":BUG: call to PutPixels outside of DrawBegin()/DrawEnd()\n");
    return false;
  }

  auto xp = CastX();

  // create a gcontext on-demand
  if (!mGc) {
    xcb_gcontext_t gc = xcb_generate_id(conn());
    xcb_screen_t *si = xp->GetScreen(mScreen);
    if (si) {
      uint32_t value_list[2] = { si->white_pixel, si->black_pixel };
      auto ck = xcb_create_gc_checked(conn(), gc, mWnd, XCB_GC_FOREGROUND | XCB_GC_BACKGROUND, value_list);
      if (xp->CheckCookie(ck, LOG_PREFIX)) {
        mGc = 0;
        return false;
      }
    } else {
      TRACE(LOG_PREFIX ":BUG: Window not on a known screen\n");
      return false;
    }
  }

  // We know mGc is valid now, or we would have returned earlier.
  // Don't check the error here, let the event handler deal with it.
  // If this image doesn't get rendered, we can just fail later.
  xcb_put_image(
    conn(), XCB_IMAGE_FORMAT_Z_PIXMAP, mWnd, mGc,
    bounds.width, bounds.height, bounds.x, bounds.y, 0, depth, data_length, data);

  return true;

#undef LOG_PREFIX
}

void RealWindow::ProcessXEvent(xcb_generic_event_t* evt)
{
#define LOG_PREFIX "PX11:Window:ProcessXEvent"
  // assume locked by caller
  if (!evt) {
    return;
  }

  // Event to be appended to the queue, if type is not 0.
  SDL_Event qevent;
  qevent.type = 0;

  auto xp = CastX();
  uint8_t type = evt->response_type & ~0x80;
  switch(type) {

    // Key press and release
    case XCB_KEY_PRESS:
    case XCB_KEY_RELEASE:
    {
      using iplug::EVirtualKey;
      auto kp = (xcb_key_press_event_t*) evt;

      bool isDown = type == XCB_KEY_PRESS;
      EVirtualKey vk = KeyboardDetailToVirtualKey(kp->detail);

      // Update the global internal state
      if (vk == EVirtualKey::kVK_SHIFT) {
        xp->mShiftDown = isDown;
      } else if (vk == EVirtualKey::kVK_CONTROL) {
        xp->mCtrlDown = isDown;
      } else if (vk == EVirtualKey::kVK_MENU) {
        xp->mAltDown = isDown;
      }

      #if 0
      // use Xlib to lookup the key event information
      XKeyEvent keyev;
      keyev.display = dpy;
      keyev.keycode = kp->detail;
      keyev.state = kp->state;
      char buf[16]{};
      KeySym keysym;
      if (XLookupString(&keyev, buf, sizeof(buf), &keysym, nullptr)) {
      }
      #endif

      // info.ms = IMouseMod((state & XCB_BUTTON_MASK_1), (state & XCB_BUTTON_MASK_3), // Note "2" is the middle button
      //     (state & XCB_KEY_BUT_MASK_SHIFT), (state & XCB_KEY_BUT_MASK_CONTROL), (state & XCB_KEY_BUT_MASK_MOD_1) // shift, ctrl, alt
      //   );

      qevent.type = isDown ? SDL_EVENT_KEY_DOWN : SDL_EVENT_KEY_UP;
      qevent.key.down = isDown;
      qevent.key.key = vk;
      qevent.key.windowID = mWnd;
      qevent.key.mod = 0
        | (kp->state & XCB_MOD_MASK_CONTROL ? SDL_KMOD_CTRL : 0)
        | (kp->state & XCB_MOD_MASK_SHIFT ? SDL_KMOD_SHIFT : 0)
        | (xp->mAltDown ? SDL_KMOD_ALT : 0);
      // ideally parse kp->state for KMOD_ALT, but I can't figure it out right now
      qevent.key.repeat = false;
      break;
    }

    case XCB_BUTTON_PRESS:
    case XCB_BUTTON_RELEASE:
    {
      auto e = (xcb_button_press_event_t*) evt;
      int btn = e->detail;
      bool isDown = e->response_type == XCB_BUTTON_PRESS;

      // 1 = left click, 2 = middle click, 3 = right click
      if (btn == 1 || btn == 2 || btn == 3) {
        // most of the event fields are the same
        qevent.type = SDL_EVENT_MOUSE_BUTTON_DOWN;
        qevent.button.clicks = 1;
        qevent.button.down = isDown;
        qevent.button.which = 0;
        qevent.button.windowID = mWnd;
        qevent.button.x = e->event_x;
        qevent.button.y = e->event_y;

        if (btn == 1) {
          qevent.button.button = SDL_BUTTON_LEFT;

          // special handling for double-click
          if (isDown && (e->time - mLastLeftClickStamp) < xp->mDblClickTimeout) {
            qevent.button.clicks = 2;
            // reset so we don't spam double-click events accidentally
            mLastLeftClickStamp = 0;
          } else {
            mLastLeftClickStamp = e->time;
          }
        } else if (btn == 2) {
          qevent.button.button = SDL_BUTTON_MIDDLE;
        } else if (btn == 3) {
          qevent.button.button = SDL_BUTTON_RIGHT;
        }
      }

      // 4 = wheel up, 5 = wheel down
      // Also, we don't care about wheel release events.
      if (isDown && (btn == 4 || btn == 5)) {
        qevent.type = SDL_EVENT_MOUSE_WHEEL;
        qevent.wheel.which = 0;
        qevent.wheel.windowID = mWnd;
        qevent.wheel.mouse_x = e->event_x;
        qevent.wheel.mouse_y = e->event_y;
        qevent.wheel.direction = SDL_MOUSEWHEEL_NORMAL;
        qevent.wheel.x = 0;
        qevent.wheel.y = (btn == 4) ? 1.f : -1.f;
      }
      break;
    }

    case XCB_MOTION_NOTIFY:
    {
      auto e = (xcb_motion_notify_event_t*) evt;

      // has to be coming from the same screen
      if (!e->same_screen) {
        break;
      }

      // Update known cursor position
      mCursorPos.x = e->event_x;
      mCursorPos.y = e->event_y;

      // Ingore messages of the cursor returning to the normal position
      if (mCursorGrab && e->event_x == mLockPos.x && e->event_y == mLockPos.y) {
        break;
      }

      qevent.type = SDL_EVENT_MOUSE_MOTION;
      qevent.motion.state = 0
        | (e->state & XCB_BUTTON_MASK_1 ? SDL_BUTTON_LMASK : 0)
        | (e->state & XCB_BUTTON_MASK_2 ? SDL_BUTTON_MMASK : 0)
        | (e->state & XCB_BUTTON_MASK_3 ? SDL_BUTTON_RMASK : 0);
      qevent.motion.which = 0;
      qevent.motion.windowID = mWnd;
      qevent.motion.x = (float)e->event_x;
      qevent.motion.y = (float)e->event_y;
      qevent.motion.xrel = (float)(e->event_x); //- mCursorPos.x);
      qevent.motion.yrel = (float)(e->event_y); // - mCursorPos.y);

      if (mCursorGrab) {
        xp->MoveCursor(this, MOUSE_MOVE_WINDOW, mLockPos.x, mLockPos.y);
      }
      break;
    }

    case XCB_ENTER_NOTIFY:
    {
      qevent.type = SDL_EVENT_WINDOW_MOUSE_ENTER;
      qevent.window.windowID = mWnd;
      break;
    }
    case XCB_LEAVE_NOTIFY:
    {
      qevent.type = SDL_EVENT_WINDOW_MOUSE_LEAVE;
      qevent.window.windowID = mWnd;
      break;
    }
    case XCB_FOCUS_IN:
    {
      qevent.type = SDL_EVENT_WINDOW_FOCUS_GAINED;
      qevent.window.windowID = mWnd;
      break;
    }
    case XCB_FOCUS_OUT:
    {
      qevent.type = SDL_EVENT_WINDOW_FOCUS_LOST;
      qevent.window.windowID = mWnd;
      break;
    }

    case XCB_EXPOSE:
    {
      auto e = (xcb_expose_event_t *)evt;
      // MAYBE: can collect and use invalidated areas
      // If this is more than 0, then it's multiple invalid areas. For now,
      // just assume the whole UI is invalid if we get an expose request.
      if (e->count == 0)
      {
        qevent.type = SDL_EVENT_WINDOW_EXPOSED;
        qevent.window.windowID = mWnd;
      }
      break;
    }

    case XCB_DESTROY_NOTIFY:
    {
      auto e = (xcb_destroy_notify_event_t*) evt;
      qevent.type = SDL_EVENT_WINDOW_DESTROYED;
      qevent.window.windowID = mWnd;
      break;
    }

    case XCB_UNMAP_NOTIFY:
    {
      xcb_unmap_notify_event_t *mn = (xcb_unmap_notify_event_t *)evt;
      if(mn->event != mn->window) {
        break;
      }
      if (mVisible) {
        mVisible = false;
        qevent.type = SDL_EVENT_WINDOW_HIDDEN;
        qevent.window.windowID = mWnd;
      }
      break;
    }

    case XCB_MAP_NOTIFY:
    {
      xcb_map_notify_event_t *mn = (xcb_map_notify_event_t *)evt;
      if(mn->event != mn->window) {
        break;
      }
      if (!mVisible) {
        mVisible = true;
        qevent.type = SDL_EVENT_WINDOW_SHOWN;
        qevent.window.windowID = mWnd;
      }
      break;
    }

    case XCB_REPARENT_NOTIFY:
      break;

    case XCB_CONFIGURE_NOTIFY:
    {
      auto e = (xcb_configure_notify_event_t *)evt;
      if(e->event != e->window) {
        break;
      }

      // ignore synthetic, they are from WM about screen position
      if( !(evt->response_type & 0x80) ) {
        if((e->width != mBounds.width) || (e->height != mBounds.height)){
          TRACE("Size is changed to %ux%u\n", e->width, e->height);
          mBounds.width = e->width;
          mBounds.height = e->height;

          // create event and add it to the queue to notify the client
          SDL_Event ev1;
          ev1.type = SDL_EVENT_WINDOW_RESIZED;
          ev1.window.windowID = mWnd;
          ev1.window.data1 = mBounds.width;
          ev1.window.data2 = mBounds.height;
          mEvents.push_back(ev1);
        }

        if (e->x != mBounds.x || e->y != mBounds.y) {
          mBounds.x = e->x;
          mBounds.y = e->y;

          // create event and add it to queue to notify client
          SDL_Event ev1;
          ev1.type = SDL_EVENT_WINDOW_MOVED;
          ev1.window.windowID = mWnd;
          ev1.window.data1 = mBounds.x;
          ev1.window.data2 = mBounds.y;
          mEvents.push_back(ev1);
        }
      }
      break;
    }

    case XCB_PROPERTY_NOTIFY:
    {
      auto e = (xcb_property_notify_event_t*) evt;
      xcb_atom_t atomXEMBED = xp->atoms._XEMBED_INFO;
      if (e->atom == atomXEMBED) {
        // While we SHOULD check the property value, it's sometimes glitchy?
        // So for now just always set to true.
        SetVisible(true);
        #if 0
        // This indicates we need to change mapping status.
        // Just to be sure, let's get _XEMBED_INFO and check.
        auto prop = GetProperty(xp, mWnd, atomXEMBED, atomXEMBED, 0, 2);
        if (!prop) {
          // Failed to get property somehow!? Oh well, just map it anways.
          SetVisible(true);
          break;
        }

        auto embedInfo = (uint32_t*)xcb_get_property_value(prop);
        SetVisible((embedInfo[1] & XEMBED_MAPPED) != 0);
        free(prop);
        #endif
      }
      break;
    }

    case XCB_CLIENT_MESSAGE:
    {
      auto e = (xcb_client_message_event_t*) evt;
      if (e->type == xp->atoms._XEMBED) {
        // TODO: process different xembed messages
        uint32_t op = e->data.data32[1];
        TRACE("Received _XEMBED message opcode: %u\n", op);
      }
      break;
    }
  }

  if (qevent.type != 0) {
    mEvents.push_back(qevent);
  }

#undef LOG_PREFIX
}

bool RealWindow::PollEvent(SDL_Event* event)
{
  auto lock = LockX();
  if (mEvents.empty()) {
    return false;
  }
  *event = mEvents.front();
  mEvents.pop_front();
  return true;
}

void RealWindow::SetWmAtom(xcb_atom_t property, xcb_atom_t value)
{
  xcb_atom_t values[] = {value};
  xcb_change_property(conn(), XCB_PROP_MODE_REPLACE, mWnd, property, XCB_ATOM_ATOM, 32, 1, values);
}

#pragma endregion RealWindow

//----------------------------------//
// Implementation of the public API //
//----------------------------------//

#pragma region Public implementation


PlatformX11* PlatformX11::Create()
{
  XcbPlatform* xp = new XcbPlatform();
  xp->Connect();
  if (!(xp->mStatus & XcbPlatform::kLoadSuccess)) {
    delete xp;
    return nullptr;
  }
  return reinterpret_cast<PlatformX11*>(xp);
}

#define PSELF reinterpret_cast<XcbPlatform*>(this)

// dummy constructor
PlatformX11::PlatformX11() {}
PlatformX11::~PlatformX11()
{
  PSELF->~XcbPlatform();
}

X11Window* PlatformX11::CreateWindow(const WindowOptions& options)
{
  return reinterpret_cast<X11Window*>(PSELF->CreateWindow(options));
}

bool PlatformX11::SetClipboard(EClipboardFormat format, const void* data, uint32_t data_len)
{ return PSELF->SetClipboard(format, data, data_len); }

bool PlatformX11::GetClipboard(EClipboardFormat* pFormat, WDL_TypedBuf<uint8_t>* pData)
{ return PSELF->GetClipboard(pFormat, pData); }

void PlatformX11::Flush()
{
  xcb_flush(PSELF->mConn);
}

void PlatformX11::ProcessEvents()
{
  PSELF->ProcessEventQueue(-1);
}


#undef PSELF

#define PSELF reinterpret_cast<RealWindow*>(this)
#define PCSELF reinterpret_cast<const RealWindow*>(this)

void X11Window::Close()
{
  // have to cast to RealWindow for delete, otherwise it won't delete correctly.
  delete PSELF;
}

bool X11Window::DrawBegin()
{ return PSELF->DrawBegin(); }

void X11Window::DrawEnd()
{ PSELF->DrawEnd(); }

bool X11Window::DrawImage(const WRect& bounds, int format, const uint8_t* data)
{ return PSELF->DrawImage(bounds, format, data); }

void X11Window::SetVisible(bool show)
{ PSELF->SetVisible(show); }

bool X11Window::IsVisible() const
{ return PCSELF->mVisible; }

void X11Window::SetTitle(const char* title)
{ PSELF->SetTitle(title); }

void X11Window::Resize(uint32_t w, uint32_t h)
{ PSELF->Resize(w, h); }

void X11Window::Move(int32_t x, int32_t y)
{ PSELF->Move(x, y); }

void X11Window::RequestFocus()
{ PSELF->RequestFocus(); }

void X11Window::SetCursorVisible(bool show)
{ PSELF->SetCursorVisible(show); }

bool X11Window::IsCursorVisible() const
{ return PCSELF->IsCursorVisible(); }

void X11Window::SetCursorGrabbed(bool lock)
{ PSELF->SetCursorGrabbed(lock, false); }

bool X11Window::IsCursorGrabbed() const
{ return PCSELF->mCursorGrab; }

void X11Window::MoveMouse(EMouseMoveMode mode, int x, int y)
{
  auto xp = PSELF->CastX();
  xp->MoveCursor(PSELF, mode, x, y);
}

void* X11Window::GetHandle() const
{ return xid_to_voidp(PCSELF->mWnd); }

bool X11Window::PollEvent(SDL_Event* event)
{ return PSELF->PollEvent(event); }

#pragma endregion Public implementation


#pragma region KeyMap

iplug::EVirtualKey KeyboardDetailToVirtualKey(unsigned int keysym) {
  // TODO(sad): Have |keysym| go through the X map list?
  using EK = iplug::EVirtualKey;
  // [a-z] cases.
  if (keysym >= XK_a && keysym <= XK_z)
    return static_cast<EK>(EK::kVK_A + keysym - XK_a);
  // [0-9] cases.
  if (keysym >= XK_0 && keysym <= XK_9)
    return static_cast<EK>(EK::kVK_0 + keysym - XK_0);

  switch (keysym) {
    case XK_BackSpace:
      return EK::kVK_BACK;
    case XK_Delete:
    case XK_KP_Delete:
      return EK::kVK_DELETE;
    case XK_Tab:
    case XK_KP_Tab:
    case XK_ISO_Left_Tab:
    case XK_3270_BackTab:
      return EK::kVK_TAB;
    case XK_Linefeed:
    case XK_Return:
    case XK_KP_Enter:
    case XK_ISO_Enter:
      return EK::kVK_RETURN;
    case XK_Clear:
    case XK_KP_Begin:  // NumPad 5 without Num Lock, for crosbug.com/29169.
      return EK::kVK_CLEAR;
    case XK_KP_Space:
    case XK_space:
      return EK::kVK_SPACE;
    case XK_Home:
    case XK_KP_Home:
      return EK::kVK_HOME;
    case XK_End:
    case XK_KP_End:
      return EK::kVK_END;
    case XK_Page_Up:
    case XK_KP_Page_Up:  // aka XK_KP_Prior
      return EK::kVK_PRIOR;
    case XK_Page_Down:
    case XK_KP_Page_Down:  // aka XK_KP_Next
      return EK::kVK_NEXT;
    case XK_Left:
    case XK_KP_Left:
      return EK::kVK_LEFT;
    case XK_Right:
    case XK_KP_Right:
      return EK::kVK_RIGHT;
    case XK_Down:
    case XK_KP_Down:
      return EK::kVK_DOWN;
    case XK_Up:
    case XK_KP_Up:
      return EK::kVK_UP;
    case XK_Escape:
      return EK::kVK_ESCAPE;
    case XK_Kana_Lock:
    case XK_Kana_Shift:
      return EK::kVK_KANA;
    case XK_Hangul:
      return EK::kVK_HANGUL;
    case XK_Hangul_Hanja:
      return EK::kVK_HANJA;
    case XK_Kanji:
      return EK::kVK_KANJI;
    case XK_Henkan:
      return EK::kVK_CONVERT;
    case XK_Muhenkan:
      return EK::kVK_NONCONVERT;
    // Unimplemented, no matching key code on Windows
    // case XK_Zenkaku_Hankaku:
    //   return EK::kVK_DBE_DBCSCHAR;
    case XK_KP_0:
    case XK_KP_1:
    case XK_KP_2:
    case XK_KP_3:
    case XK_KP_4:
    case XK_KP_5:
    case XK_KP_6:
    case XK_KP_7:
    case XK_KP_8:
    case XK_KP_9:
      return static_cast<EK>(EK::kVK_NUMPAD0 + (keysym - XK_KP_0));
    case XK_multiply:
    case XK_KP_Multiply:
      return EK::kVK_MULTIPLY;
    case XK_KP_Add:
      return EK::kVK_ADD;
    case XK_KP_Separator:
      return EK::kVK_SEPARATOR;
    case XK_KP_Subtract:
      return EK::kVK_SUBTRACT;
    case XK_KP_Decimal:
      return EK::kVK_DECIMAL;
    case XK_KP_Divide:
      return EK::kVK_DIVIDE;
    case XK_KP_Equal:
    case XK_equal:
    case XK_plus:
      return EK::kVK_OEM_PLUS;
    case XK_comma:
    case XK_less:
      return EK::kVK_OEM_COMMA;
    case XK_minus:
    case XK_underscore:
      return EK::kVK_OEM_MINUS;
    case XK_greater:
    case XK_period:
      return EK::kVK_OEM_PERIOD;
    case XK_colon:
    case XK_semicolon:
      return EK::kVK_OEM_1;
    case XK_question:
    case XK_slash:
      return EK::kVK_OEM_2;
    case XK_asciitilde:
    case XK_quoteleft:
      return EK::kVK_OEM_3;
    case XK_bracketleft:
    case XK_braceleft:
      return EK::kVK_OEM_4;
    case XK_backslash:
    case XK_bar:
      return EK::kVK_OEM_5;
    case XK_bracketright:
    case XK_braceright:
      return EK::kVK_OEM_6;
    case XK_quoteright:
    case XK_quotedbl:
      return EK::kVK_OEM_7;
    case XK_ISO_Level5_Shift:
      return EK::kVK_OEM_8;
    case XK_Shift_L:
    case XK_Shift_R:
      return EK::kVK_SHIFT;
    case XK_Control_L:
    case XK_Control_R:
      return EK::kVK_CONTROL;
    case XK_Meta_L:
    case XK_Meta_R:
    case XK_Alt_L:
    case XK_Alt_R:
      return EK::kVK_MENU;
    case XK_ISO_Level3_Shift:
    case XK_Mode_switch:
      return EK::kVK_ALTGR;
    case XK_Multi_key:
      return EK::kVK_COMPOSE;
    case XK_Pause:
      return EK::kVK_PAUSE;
    case XK_Caps_Lock:
      return EK::kVK_CAPITAL;
    case XK_Num_Lock:
      return EK::kVK_NUMLOCK;
    case XK_Scroll_Lock:
      return EK::kVK_SCROLL;
    case XK_Select:
      return EK::kVK_SELECT;
    case XK_Print:
      return EK::kVK_PRINT;
    case XK_Execute:
      return EK::kVK_EXECUTE;
    case XK_Insert:
    case XK_KP_Insert:
      return EK::kVK_INSERT;
    case XK_Help:
      return EK::kVK_HELP;
    case XK_Super_L:
      return EK::kVK_LWIN;
    case XK_Super_R:
      return EK::kVK_RWIN;
    case XK_Menu:
      return EK::kVK_APPS;
    case XK_F1:
    case XK_F2:
    case XK_F3:
    case XK_F4:
    case XK_F5:
    case XK_F6:
    case XK_F7:
    case XK_F8:
    case XK_F9:
    case XK_F10:
    case XK_F11:
    case XK_F12:
    case XK_F13:
    case XK_F14:
    case XK_F15:
    case XK_F16:
    case XK_F17:
    case XK_F18:
    case XK_F19:
    case XK_F20:
    case XK_F21:
    case XK_F22:
    case XK_F23:
    case XK_F24:
      return static_cast<EK>(EK::kVK_F1 + (keysym - XK_F1));
    case XK_KP_F1:
    case XK_KP_F2:
    case XK_KP_F3:
    case XK_KP_F4:
      return static_cast<EK>(EK::kVK_F1 + (keysym - XK_KP_F1));
    case XK_guillemotleft:
    case XK_guillemotright:
    case XK_degree:
    // In the case of canadian multilingual keyboard layout, VKEY_OEM_102 is
    // assigned to ugrave key.
    case XK_ugrave:
    case XK_Ugrave:
    case XK_brokenbar:
      return EK::kVK_OEM_102;  // international backslash key in 102 keyboard.
    // When evdev is in use, /usr/share/X11/xkb/symbols/inet maps F13-18 keys
    // to the special XF86XK symbols to support Microsoft Ergonomic keyboards:
    // https://bugs.freedesktop.org/show_bug.cgi?id=5783
    // In Chrome, we map these X key symbols back to F13-18 since we don't have
    // VKEYs for these XF86XK symbols.
    case XF86XK_Launch5:
      return EK::kVK_F14;
    case XF86XK_Launch6:
      return EK::kVK_F15;
    case XF86XK_Launch7:
      return EK::kVK_F16;
    case XF86XK_Launch8:
      return EK::kVK_F17;
    case XF86XK_Launch9:
      return EK::kVK_F18;
    // For supporting multimedia buttons on a USB keyboard.
    case XF86XK_Back:
      return EK::kVK_BROWSER_BACK;
    case XF86XK_Forward:
      return EK::kVK_BROWSER_FORWARD;
    case XF86XK_Reload:
      return EK::kVK_BROWSER_REFRESH;
    case XF86XK_Stop:
      return EK::kVK_BROWSER_STOP;
    case XF86XK_Search:
      return EK::kVK_BROWSER_SEARCH;
    case XF86XK_Favorites:
      return EK::kVK_BROWSER_FAVORITES;
    case XF86XK_HomePage:
      return EK::kVK_BROWSER_HOME;
    case XF86XK_AudioMute:
      return EK::kVK_VOLUME_MUTE;
    case XF86XK_AudioLowerVolume:
      return EK::kVK_VOLUME_DOWN;
    case XF86XK_AudioRaiseVolume:
      return EK::kVK_VOLUME_UP;
    case XF86XK_AudioNext:
      return EK::kVK_MEDIA_NEXT_TRACK;
    case XF86XK_AudioPrev:
      return EK::kVK_MEDIA_PREV_TRACK;
    case XF86XK_AudioStop:
      return EK::kVK_MEDIA_STOP;
    case XF86XK_AudioPlay:
      return EK::kVK_MEDIA_PLAY_PAUSE;
    case XF86XK_Mail:
      return EK::kVK_LAUNCH_MAIL;
    case XF86XK_LaunchA:  // F3 on an Apple keyboard.
      return EK::kVK_LAUNCH_APP1;
    case XF86XK_LaunchB:  // F4 on an Apple keyboard.
    case XF86XK_Calculator:
      return EK::kVK_LAUNCH_APP2;
    // XF86XK_Tools is generated from HID Usage AL_CONSUMER_CONTROL_CONFIG
    // (Usage 0x0183, Page 0x0C) and most commonly launches the OS default
    // media player (see crbug.com/398345).
    #if 0
    case XF86XK_Tools:
      return EK::kVK_MEDIA_LAUNCH_MEDIA_SELECT;
    case XF86XK_WLAN:
      return EK::kVK_WLAN;
    case XF86XK_PowerOff:
      return EK::kVK_POWER;
    case XF86XK_MonBrightnessDown:
      return EK::kVK_BRIGHTNESS_DOWN;
    case XF86XK_MonBrightnessUp:
      return EK::kVK_BRIGHTNESS_UP;
    case XF86XK_KbdBrightnessDown:
      return EK::kVK_KBD_BRIGHTNESS_DOWN;
    case XF86XK_KbdBrightnessUp:
      return EK::kVK_KBD_BRIGHTNESS_UP;
    #endif
   // TODO(sad): some keycodes are still missing.
  }
  return (EK)0;
}

#pragma endregion KeyMap
