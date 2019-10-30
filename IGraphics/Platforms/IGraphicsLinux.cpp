/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

/*
 * AZ TODO:
 *   * check graphic in plug-in format (if it can work at all in current context)
 *   * FONS_USE_FREETYPE and SKIA
 */

#include <swell.h>
#include <wdlutf8.h>

#include "heapbuf.h"

#include "IPlugParameter.h"
#include "IGraphicsLinux.h"
#include "IPopupMenuControl.h"
#include "IPlugPaths.h"

#ifdef OS_LINUX
  // we need GDK m_oswindow to create GLX context
  #include "swell-internal.h"

  #include "GL/glx.h"

  #include <fontconfig/fontconfig.h>
#endif

using namespace iplug;
using namespace igraphics;

#define IPLUG_TIMER_ID 2

class IGraphicsLinux::Font : public PlatformFont
{
public:
  Font(WDL_String &fileName) : PlatformFont(false), mFileName(fileName) { }
  IFontDataPtr GetFontData() override;

private:
  WDL_String mFileName;
};


void IGraphicsLinux::HideMouseCursor(bool hide, bool lock)
{
  if (mCursorHidden == hide)
    return;

  if (hide)
  {
    mHiddenCursorX = mCursorX;
    mHiddenCursorY = mCursorY;

    ShowCursor(false);
    mCursorHidden = true;
    mCursorLock = lock && !mTabletInput;
  }
  else
  {
    if (mCursorLock)
      MoveMouseCursor(mHiddenCursorX, mHiddenCursorY);

    ShowCursor(true);
    mCursorHidden = false;
    mCursorLock = false;
  }
}

void IGraphicsLinux::MoveMouseCursor(float x, float y)
{
  if (mTabletInput)
    return;

  float scale = GetDrawScale() * GetScreenScale();

  POINT p;
  p.x = std::round(x * scale);
  p.y = std::round(y * scale);

  ::ClientToScreen((HWND)GetWindow(), &p);

  if (SWELL_SetCursorPos(p.x, p.y))
  {
    GetCursorPos(&p);
    ScreenToClient((HWND)GetWindow(), &p);

    mCursorX = p.x / scale;
    mCursorY = p.y / scale;

    if (mCursorHidden && !mCursorLock)
    {
      mHiddenCursorX = p.x / scale;
      mHiddenCursorY = p.y / scale;
    }
  }
}


#ifdef IGRAPHICS_GL

/*
 * AZ: I have not found a good way to make it with SWELL. BTW it can be better to
 * avoid SWELL completely for GL based plug-ins.
 *
 * The following is ONE BIG DIRTY HACK !!!!
 */

// There is not easy way to get it from SWELL, so a hack using internal structure
static HWND getOSParentFor (HWND hWnd)
{
  while ( hWnd )
  {
    if ( hWnd->m_oswindow )
    {
      return hWnd;
    }
    hWnd = hWnd->m_parent;
  }
  return nullptr;
}

// GLX context creation is from
//    https://www.khronos.org/opengl/wiki/Tutorial:_OpenGL_3.0_Context_Creation_(GLX)


static bool ctxErrorOccurred = false;
static int ctxErrorHandler( Display *dpy, XErrorEvent *ev )
{
    ctxErrorOccurred = true;
    return 0;
}

#define GLX_CONTEXT_MAJOR_VERSION_ARB       0x2091
#define GLX_CONTEXT_MINOR_VERSION_ARB       0x2092
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display*, GLXFBConfig, GLXContext, Bool, const int*);

// Helper to check for extension string presence.  Adapted from:
//   http://www.opengl.org/resources/features/OGLextensions/
static bool isExtensionSupported(const char *extList, const char *extension)
{
  const char *start;
  const char *where, *terminator;

  /* Extension names should not have spaces. */
  where = strchr(extension, ' ');
  if (where || *extension == '\0')
    return false;

  /* It takes a bit of care to be fool-proof about parsing the
     OpenGL extensions string. Don't be fooled by sub-strings,
     etc. */
  for (start=extList;;) {
    where = strstr(start, extension);

    if (!where)
      break;

    terminator = where + strlen(extension);

    if ( where == start || *(where - 1) == ' ' )
      if ( *terminator == ' ' || *terminator == '\0' )
        return true;

    start = terminator;
  }

  return false;
}

/*
 * GL Window is placed as "ClientRect" of SWELL window,
 * but position should be specified relative to the parent XWindow.
 *
 * A bit tricky, but I could not get right position from SWELL.
 */
void IGraphicsLinux::getGLRect(RECT *r)
{
  GetClientRect(mPlugWnd,  r);
  HWND hWnd = mPlugWnd;

  while ( hWnd )
  {
    NCCALCSIZE_PARAMS p = {{ hWnd->m_position, }, };
    if (hWnd->m_wndproc)
    {
      hWnd->m_wndproc(hWnd,WM_NCCALCSIZE,0,(LPARAM)&p);
    }
    r->left += p.rgrc[0].left;
    r->top += p.rgrc[0].top;
    if ( !hWnd->m_parent )
    {
      r->left -= hWnd->m_position.left;
      r->top  -= hWnd->m_position.top;
    }
    if ( hWnd == mGLParent)
    {
      break;
    }
    hWnd = hWnd->m_parent;
  }
  r->right += r->left;
  r->bottom += r->top;
}

bool IGraphicsLinux::CreateGLContext()
{
  if ( mGLWnd )
  {
    DBGMSG( "BUG: GL window was already created" );
    return false;
  }
  mGLParent = getOSParentFor( mPlugWnd );
  if ( !mGLParent )
  {
    DBGMSG( "BUG: Can not create GL context without instantiated GDK window\n");
    return false;
  }
  GdkWindow *gdkParent  = mGLParent->m_oswindow;
  mGLDisplay = GDK_WINDOW_XDISPLAY( gdkParent );
  Window xParent = GDK_WINDOW_XID( gdkParent );

  // Get a matching FB config
  static int visual_attribs[] =
    {
      GLX_X_RENDERABLE    , True,
      GLX_DRAWABLE_TYPE   , GLX_WINDOW_BIT,
      GLX_RENDER_TYPE     , GLX_RGBA_BIT,
      GLX_X_VISUAL_TYPE   , GLX_TRUE_COLOR,
      GLX_RED_SIZE        , 8,
      GLX_GREEN_SIZE      , 8,
      GLX_BLUE_SIZE       , 8,
      GLX_ALPHA_SIZE      , 8,
      GLX_DEPTH_SIZE      , 24,
      GLX_STENCIL_SIZE    , 8,
      GLX_DOUBLEBUFFER    , True,
      //GLX_SAMPLE_BUFFERS  , 1,
      //GLX_SAMPLES         , 4,
      None
    };

  int glx_major, glx_minor;

  // Need at least GLX 1.3
  if ( !glXQueryVersion( mGLDisplay, &glx_major, &glx_minor ) ||
       ( ( glx_major == 1 ) && ( glx_minor < 3 ) ) || ( glx_major < 1 ) )
  {
    DBGMSG("Invalid GLX version");
    return false;
  }
  int fbcount;
  GLXFBConfig* fbc = glXChooseFBConfig( mGLDisplay, DefaultScreen(mGLDisplay), visual_attribs, &fbcount);
  if (!fbc)
  {
    DBGMSG( "Failed to retrieve a framebuffer config\n" );
    return false;
  }

  // Pick the FB config/visual with the most samples per pixel
  //  ??? does that influence performance
  int best_fbc = -1, best_num_samp = -1;
  for (int i=0; i<fbcount; ++i)
  {
    XVisualInfo *vi = glXGetVisualFromFBConfig( mGLDisplay, fbc[i] );
    if ( vi )
    {
      int samp_buf, samples;
      glXGetFBConfigAttrib( mGLDisplay, fbc[i], GLX_SAMPLE_BUFFERS, &samp_buf );
      glXGetFBConfigAttrib( mGLDisplay, fbc[i], GLX_SAMPLES       , &samples  );

      if ( best_fbc < 0 || (samp_buf && (samples > best_num_samp) ) )
      {
        best_fbc = i, best_num_samp = samples;
      }
    }
    XFree( vi );
  }
  if (best_fbc < 0)
  {
    XFree( fbc );
    DBGMSG( "Failed to match visual, check with glx_info\n" );
    return false;
  }
  GLXFBConfig bestFbc = fbc[ best_fbc ];
  XFree( fbc );
  XVisualInfo *vi = glXGetVisualFromFBConfig( mGLDisplay, bestFbc );
  DBGMSG( "Chosen visual ID = 0x%x , %d samples\n", (unsigned int)vi->visualid, (int)best_num_samp);

  XSetWindowAttributes swa;
  swa.colormap = mGLColormap = XCreateColormap( mGLDisplay,
                                         RootWindow( mGLDisplay, vi->screen ),
                                         vi->visual, AllocNone );
  swa.background_pixmap = None ;
  swa.border_pixel      = 0;
  swa.event_mask        = StructureNotifyMask;

  RECT r;
  getGLRect(&r);
  mGLWnd = XCreateWindow( mGLDisplay, xParent,
                              r.left, r.top, r.right - r.left, r.bottom - r.top, 0, vi->depth, InputOutput,
                              vi->visual,
                              CWBackPixmap|CWBorderPixel|CWColormap|CWEventMask, &swa );
  if ( !mGLWnd )
  {
    printf( "Failed to create window.\n" );
    return false;
  }

  // Done with the visual info data
  XFree( vi );
  XMapWindow( mGLDisplay, mGLWnd );

  // Get the default screen's GLX extension list
  const char *glxExts = glXQueryExtensionsString( mGLDisplay,
                                                  DefaultScreen( mGLDisplay ) );

  // NOTE: It is not necessary to create or make current to a context before
  // calling glXGetProcAddressARB
  glXCreateContextAttribsARBProc glXCreateContextAttribsARB = 0;
  glXCreateContextAttribsARB = (glXCreateContextAttribsARBProc)
           glXGetProcAddressARB( (const GLubyte *) "glXCreateContextAttribsARB" );

  // Install an X error handler so the application won't exit if GL 3.0
  // context allocation fails.
  //
  // Note this error handler is global.  All display connections in all threads
  // of a process use the same error handler, so be sure to guard against other
  // threads issuing X commands while this code is running.
  ctxErrorOccurred = false;
  int (*oldHandler)(Display*, XErrorEvent*) =
      XSetErrorHandler(&ctxErrorHandler);

  // Check for the GLX_ARB_create_context extension string and the function.
  // If either is not present, use GLX 1.3 context creation method.
  if ( !isExtensionSupported( glxExts, "GLX_ARB_create_context" ) ||
       !glXCreateContextAttribsARB )
  {
    printf( "glXCreateContextAttribsARB() not found"
            " ... using old-style GLX context\n" );
    mGLContext = glXCreateNewContext( mGLDisplay, bestFbc, GLX_RGBA_TYPE, 0, True );
  }

 // If it does, try to get a GL 3.0 context!
  else
  {
    int context_attribs[] =
      {
        GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
        GLX_CONTEXT_MINOR_VERSION_ARB, 0,
        //GLX_CONTEXT_FLAGS_ARB        , GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
        None
      };

    printf( "Creating context\n" );
    mGLContext = glXCreateContextAttribsARB( mGLDisplay, bestFbc, 0,
                                      True, context_attribs );

    // Sync to ensure any errors generated are processed.
    XSync( mGLDisplay, False );
    if ( !ctxErrorOccurred && mGLContext )
      printf( "Created GL 3.0 context\n" );
    else
    {
      // Couldn't create GL 3.0 context.  Fall back to old-style 2.x context.
      // When a context version below 3.0 is requested, implementations will
      // return the newest context version compatible with OpenGL versions less
      // than version 3.0.
      // GLX_CONTEXT_MAJOR_VERSION_ARB = 1
      context_attribs[1] = 1;
      // GLX_CONTEXT_MINOR_VERSION_ARB = 0
      context_attribs[3] = 0;

      ctxErrorOccurred = false;

      printf( "Failed to create GL 3.0 context"
              " ... using old-style GLX context\n" );
      mGLContext = glXCreateContextAttribsARB( mGLDisplay, bestFbc, 0,
                                        True, context_attribs );
    }
  }

  // Sync to ensure any errors generated are processed.
  XSync( mGLDisplay, False );

  // Restore the original error handler
  XSetErrorHandler( oldHandler );

  if ( ctxErrorOccurred || !mGLContext )
  {
    printf( "Failed to create an OpenGL context\n" );
    exit(1);
  }

  // Verifying that context is a direct context
  if ( ! glXIsDirect ( mGLDisplay, (GLXContext)mGLContext ) )
  {
    printf( "Indirect GLX rendering context obtained\n" );
  }
  else
  {
    printf( "Direct GLX rendering context obtained\n" );
  }
  ActivateGLContext();
  if (!gladLoadGL())
  {
    DBGMSG("Error initializing glad\n");
    return false;
  }
  return true;
}

void IGraphicsLinux::DestroyGLContext()
{
  if (mGLWnd)
  {
    DeactivateGLContext ();
    if ( mGLContext )
    {
      glXDestroyContext (mGLDisplay, (GLXContext)mGLContext);
      mGLContext = nullptr;
    }
    XDestroyWindow( mGLDisplay, mGLWnd );
    mGLWnd = 0;
    if ( mGLColormap )
    {
      XFreeColormap( mGLDisplay, mGLColormap );
      mGLColormap = 0;
    }
    mGLDisplay = nullptr;
  }
}

void IGraphicsLinux::ActivateGLContext()
{
  mGLOldContext = glXGetCurrentContext();
  if(mGLOldContext != mGLContext)
  {
    glXMakeCurrent(mGLDisplay, mGLWnd, (GLXContext)mGLContext);
  }
  else
  {
    mGLOldContext = nullptr;
  }
}

void IGraphicsLinux::DeactivateGLContext()
{
  if((glXGetCurrentContext() == (GLXContext)mGLContext))
  {
    glXMakeCurrent(mGLDisplay, mGLOldContext ? mGLWnd : None, (GLXContext)mGLOldContext);
  }
  mGLOldContext = nullptr;
}

/*
 * Dirty hack...
 * We create X Window and we want Expose events processing
 */
bool IGraphicsLinux::GLFilter(XEvent *xev, void *, void *me){
  const XEvent *xevent = (XEvent *)xev;
  IGraphicsLinux* pGraphics = (IGraphicsLinux*)me;

  if ( pGraphics && xevent && xevent->type == Expose )
  {
    pGraphics->Paint();
    //return GDK_FILTER_REMOVE;
  }
  return GDK_FILTER_CONTINUE;
}

#endif

bool IGraphicsLinux::ActivateContext()
{
  if (OpenWindowBH() ){
#ifdef IGRAPHICS_GL
    ActivateGLContext();
#else
    HDC dc = GetDC(mPlugWnd);
    SetPlatformContext(dc);
    BeginPaint(mPlugWnd, &mPS); // TODO: BeginPaint/EndPaint and GetDC/ReleaseDC ? issues closing reaper ?
#endif
    return true;
  }
  return false;
}

void IGraphicsLinux::DeactivateContext()
{
  if ( mOpenBHDone )
  {
#ifdef IGRAPHICS_GL
      glXSwapBuffers ( mGLDisplay, mGLWnd );
      DeactivateGLContext();
#else
      EndPaint(mPlugWnd, &mPS);
      HDC dc = (HDC)GetPlatformContext();
      if ( dc )
      {
	ReleaseDC(mPlugWnd, dc);
	SetPlatformContext( nullptr );
      }
#endif
  }
}

/*
 * For GL we need X/Gdk windows set. But till WindowShow SWELL does not
 * set m_oswindow and getting it other way is too dirty (private structure in SWELL GDK)
 */

bool IGraphicsLinux::OpenWindowBH(){
  if (mOpenBHDone)
  {
    return true;
  }

  #ifdef IGRAPHICS_GL
  if (!CreateGLContext())
  {
    return false;
  }
  #endif

  OnViewInitialized( nullptr );

  SetScreenScale(1); // resizes draw context

  GetDelegate()->LayoutUI(this);

  SetAllControlsDirty();

  if (mPlugWnd && TooltipsEnabled())
  {
    bool ok = false;
    // mTooltipWnd TODO
    if (!ok) EnableTooltips(ok);
  }

  GetDelegate()->OnUIOpen();

  #ifdef IGRAPHICS_GL
  DeactivateGLContext();
  #endif

  mOpenBHDone = true;
  return true;
}

void IGraphicsLinux::Paint()
{
  IRECTList rects;
  RECT r;
  GetClientRect(mPlugWnd, &r);
  IRECT ir(r.left, r.top, r.right, r.bottom);
  rects.Add(ir);

  if (ActivateContext() )
  {
    Draw(rects);
    DeactivateContext();
  }

}

inline IMouseInfo IGraphicsLinux::GetMouseInfo(LPARAM lParam)
{
  IMouseInfo info;
  info.x = mCursorX = GET_X_LPARAM(lParam) / (GetDrawScale() * GetScreenScale());
  info.y = mCursorY = GET_Y_LPARAM(lParam) / (GetDrawScale() * GetScreenScale());
  info.ms = IMouseMod((mModKeys & MK_LBUTTON), (mModKeys & MK_RBUTTON), (mModKeys & MK_SHIFT), (mModKeys & MK_CONTROL), false);
  return info;
}

inline IMouseInfo IGraphicsLinux::GetMouseInfoDeltas(float& dX, float& dY, LPARAM lParam)
{
  float oldX = mCursorX;
  float oldY = mCursorY;

  IMouseInfo info = GetMouseInfo(lParam);

  dX = info.x - oldX;
  dY = info.y - oldY;

  return info;
}

// SWELL does not device that for now, but use fixed numbers...
#ifndef GET_WHEEL_DELTA_WPARAM
#define GET_WHEEL_DELTA_WPARAM(wParam) (((int)wParam)>>16)
#endif
#ifndef WHEEL_DELTA
#define WHEEL_DELTA 120
#endif

LRESULT IGraphicsLinux::PlugWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
  if (uMsg == WM_CREATE)
  {
    SetWindowLongPtr(hWnd, GWLP_USERDATA, lParam); // SWELL specific use if lParam
    SetFocus(hWnd); // gets scroll wheel working straight away
    int mSec = static_cast<int>(std::round(1000.0 / ((IGraphicsLinux *)lParam)->FPS()));
    SetTimer(hWnd, IPLUG_TIMER_ID, mSec, NULL);

    // DragAcceptFiles(hWnd, true); TODO
  }
  IGraphicsLinux* pGraphics = (IGraphicsLinux*) GetWindowLongPtr(hWnd, GWLP_USERDATA);

  if (!pGraphics || hWnd != pGraphics->mPlugWnd)
  {
    return FALSE;
  }

  if (pGraphics->mParamEditWnd && pGraphics->mParamEditMsg == kEditing)
  {
    if (uMsg == WM_RBUTTONDOWN || (uMsg == WM_LBUTTONDOWN))
    {
      pGraphics->mParamEditMsg = kCancel;
    }
    return FALSE;
  }

  switch(uMsg)
  {
    case WM_TIMER:
      if (wParam == IPLUG_TIMER_ID) {
        if (pGraphics->mParamEditWnd && pGraphics->mParamEditMsg != kNone)
	{ // TODO
	}
        IRECTList rects;
        if (pGraphics->IsDirty(rects))
        {
          pGraphics->SetAllControlsClean();

          for (int i = 0; i < rects.Size(); i++)
          {
            IRECT dirtyR = rects.Get(i);
            dirtyR.Scale(pGraphics->GetDrawScale() * pGraphics->GetScreenScale());
            dirtyR.PixelAlign();
            RECT r = { (LONG)dirtyR.L, (LONG)dirtyR.T, (LONG)dirtyR.R, (LONG)dirtyR.B };

            InvalidateRect(hWnd, &r, FALSE);
          }

          if (pGraphics->mParamEditWnd)
          {
	    // TODO
            UpdateWindow(hWnd);
            pGraphics->mParamEditMsg = kUpdate;
          }
          else
          {
            UpdateWindow(hWnd);
          }
        }
      }
      return 0;

    case WM_PAINT:
      pGraphics->Paint();
      return 0;

    case WM_RBUTTONDOWN:
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    {
      switch(uMsg)
      {
	case WM_RBUTTONDOWN:
	  pGraphics->mModKeys |= MK_RBUTTON;
	  break;
	case WM_LBUTTONDOWN:
	  pGraphics->mModKeys |= MK_LBUTTON;
	  break;
	case WM_MBUTTONDOWN:
	  pGraphics->mModKeys |= MK_MBUTTON;
      }
      pGraphics->HideTooltip();
      if (pGraphics->mParamEditWnd)
      {
        pGraphics->mParamEditMsg = kCommit;
        return 0;
      }
      SetFocus(hWnd); // Added to get keyboard focus again when user clicks in window
      SetCapture(hWnd);
      ShowCursor(false);
      IMouseInfo info = pGraphics->GetMouseInfo(lParam);
      pGraphics->OnMouseDown(info.x, info.y, info.ms);
      return 0;
    }
    case WM_SETCURSOR:
    {
      pGraphics->OnSetCursor();
      return 0;
    }
    case WM_MOUSEMOVE:
    {
      if (!(pGraphics->mModKeys & (MK_LBUTTON | MK_RBUTTON)))
      {
        IMouseInfo info = pGraphics->GetMouseInfo(lParam);
        if (pGraphics->OnMouseOver(info.x, info.y, info.ms))
        {
	  // TODO tooltips processing
        }
      }
      else if (GetCapture() == hWnd && !pGraphics->IsInTextEntry())
      {
        float dX, dY;
        IMouseInfo info = pGraphics->GetMouseInfoDeltas(dX, dY, lParam);
        if (dX || dY)
        {
          pGraphics->OnMouseDrag(info.x, info.y, dX, dY, info.ms);
        }
      }

      return 0;
    }
    // case WM_MOUSEHOVER: TODO, not exists in SWELL
    //{
    //  pGraphics->ShowTooltip();
    //  return 0;
    //}
    // case WM_MOUSELEAVE: TODO, not exists in SWELL
    //{
    //  pGraphics->HideTooltip();
    //  pGraphics->OnMouseOut();
    //  return 0;
    //}
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    {
      switch(uMsg)
      {
	case WM_RBUTTONUP:
	  pGraphics->mModKeys &= ~MK_RBUTTON;
	  break;
	case WM_LBUTTONUP:
	  pGraphics->mModKeys &= ~MK_LBUTTON;
	  break;
	case WM_MBUTTONUP:
	  pGraphics->mModKeys &= ~MK_MBUTTON;
      }
      ReleaseCapture();
      ShowCursor(true);
      IMouseInfo info = pGraphics->GetMouseInfo(lParam);
      pGraphics->OnMouseUp(info.x, info.y, info.ms);
      return 0;
    }
    case WM_LBUTTONDBLCLK:
    {
      IMouseInfo info = pGraphics->GetMouseInfo(lParam);
      if (pGraphics->OnMouseDblClick(info.x, info.y, info.ms))
      {
        SetCapture(hWnd);
      }
      return 0;
    }
    case WM_MOUSEWHEEL:
    {
      if (pGraphics->mParamEditWnd)
      {
        pGraphics->mParamEditMsg = kCancel;
        return 0;
      }
      else
      {
        IMouseInfo info = pGraphics->GetMouseInfo(lParam);
        float d = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        float scale = pGraphics->GetDrawScale() * pGraphics->GetScreenScale();
        RECT r;
        GetWindowRect(hWnd, &r);
        pGraphics->OnMouseWheel(info.x - (r.left / scale), info.y - (r.top / scale), info.ms, d);
        return 0;
      }
    }
    default:
      break;
  }
  return DefWindowProc(hWnd,uMsg,wParam,lParam);
}

void* IGraphicsLinux::OpenWindow(void* pParent)
{
  mParentWnd = (HWND) pParent;
  RECT r = {0, 0, WindowWidth(), WindowHeight()};

  if (mPlugWnd)
  {
    GetWindowRect(mPlugWnd, &r);
    CloseWindow();
  }

  //TODO sFPS = FPS();
  mOpenBHDone = false;
  mPlugWnd = SWELL_CreateDialog(NULL, NULL, mParentWnd, (DLGPROC)PlugWndProc, (LPARAM)this); // Window, not a dialog

  SetWindowPos(mPlugWnd, NULL, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_NOZORDER|SWP_NOACTIVATE);
  ShowWindow(mPlugWnd, SW_SHOW);

  return mPlugWnd;
}

void IGraphicsLinux::CloseWindow()
{
  if (mPlugWnd)
  {
    OnViewDestroyed();

    /* TODO
    if (mTooltipWnd)
    {
      DestroyWindow(mTooltipWnd);
      mTooltipWnd = 0;
      mShowingTooltip = false;
      mTooltipIdx = -1;
    }
    */

#ifdef IGRAPHICS_GL
    //DestroyWindow(mGLWnd);
    DestroyGLContext();
#endif

    DestroyWindow(mPlugWnd);
    mPlugWnd = 0;
  }
}

void IGraphicsLinux::ForceEndUserEdit()
{
  mParamEditMsg = kCancel;
}

bool IGraphicsLinux::GetTextFromClipboard(WDL_String& str)
{
  str.Set("");
  HANDLE h;
  OpenClipboard(this->mPlugWnd);
  h=GetClipboardData(CF_TEXT);
  if (h)
  {
    char *t=(char *)GlobalLock(h);
    int s=(int)(GlobalSize(h));
    str.Set(t,s);
    GlobalUnlock(t);
  }
  CloseClipboard();
  return str.GetLength();
}

EMsgBoxResult IGraphicsLinux::ShowMessageBox(const char* text, const char* caption, EMsgBoxType type, IMsgBoxCompletionHanderFunc completionHandler)
{
  ReleaseMouseCapture();

  EMsgBoxResult result = static_cast<EMsgBoxResult>(MessageBox(mPlugWnd, text, caption, static_cast<int>(type)));

  if(completionHandler)
    completionHandler(result);

  return result;
}

void IGraphicsLinux::PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* extensions)
{
  if (!WindowIsOpen())
  {
    fileName.Set("");
    return;
  }

  char extlist[256], *pextlist = extlist;

  if(extensions && *extensions){
    int n = strlen(extensions);
    if(n + 2 > sizeof(extlist))
      n = sizeof(extlist) - 2;
    memcpy(extlist, extensions, n);
    extlist[n] = 0;
    extlist[n+1] = 0;
    char *sep = extlist;
    while((sep = strchr(sep, ' ')))
      *sep++ = 0;
  } else
    pextlist = nullptr;

  WDL_String idir(fileName);
  idir.remove_filepart();
  WDL_String ifile(fileName.get_filepart());

  char fn[MAX_PATH], *pfn;

  bool rc = false;

  switch (action)
  {
    case EFileAction::Save:
      if((rc = BrowseForSaveFile("Save into", idir.Get(), ifile.Get(), pextlist, fn, sizeof(fn))))
      {
	path.Set(fn);
	fileName.Set(path.get_filepart());
	path.remove_filepart();
	rc = true;
      }
      break;

    case EFileAction::Open:
      if((pfn = BrowseForFiles("Open file", idir.Get(), ifile.Get(), false, pextlist)))
      {
	path.Set(pfn);
	fileName.Set(path.get_filepart());
	path.remove_filepart();
	free(pfn);
	rc = true;
      }
      break;
  }

  if (!rc)
  {
    fileName.Set("");
    path.Set("");
  }

  ReleaseMouseCapture();
}

void IGraphicsLinux::PromptForDirectory(WDL_String& dir){
  char dn[MAX_PATH];
  if(BrowseForDirectory("Select directory", dir.Get(), dn, sizeof(dn)))
  {
    dir.Set(dn);
  } else {
    dir.Set("");
  }
}

void IGraphicsLinux::PlatformResize(bool parentHasResized)
{
  if (WindowIsOpen())
  {
    HWND pParent = 0, pGrandparent = 0;
    int dlgW = 0, dlgH = 0, parentW = 0, parentH = 0, grandparentW = 0, grandparentH = 0;
    //GetWindowSize(mPlugWnd, &dlgW, &dlgH);  ??? Not exist in SWELL
    RECT r;
    GetWindowRect(mPlugWnd, &r);
    dlgW = r.right - r.left;
    dlgH = r.bottom - r.top;


    int dw = (WindowWidth() * GetScreenScale()) - dlgW, dh = (WindowHeight()* GetScreenScale()) - dlgH;

    // ??? Can plug-in be top window
    pParent = GetParent(mPlugWnd);
    //GetWindowSize(pParent, &parentW, &parentH);
    GetWindowRect(pParent, &r);
    parentW = r.right - r.left;
    parentH = r.bottom - r.top;

    if (!dw && !dh)
      return;

    SetWindowPos(mPlugWnd, 0, 0, 0, dlgW + dw, dlgH + dh, SWP_NOZORDER|SWP_NOACTIVATE);

    if(pParent && !parentHasResized)
    {
      SetWindowPos(pParent, 0, 0, 0, parentW + dw, parentH + dh, SWP_NOZORDER|SWP_NOACTIVATE);
    }

    if(pGrandparent && !parentHasResized)
    {
      SetWindowPos(pGrandparent, 0, 0, 0, grandparentW + dw, grandparentH + dh, SWP_NOZORDER|SWP_NOACTIVATE);
    }

    RECT ir = { 0, 0, WindowWidth() * GetScreenScale(), WindowHeight() * GetScreenScale() };
    InvalidateRect(mPlugWnd, &ir, FALSE);

    // Fix white background while resizing
    UpdateWindow(mPlugWnd);
  }
}

#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

IFontDataPtr IGraphicsLinux::Font::GetFontData()
{
  IFontDataPtr pData;
  int file = open(mFileName.Get(), O_RDONLY);
  if (file >= 0)
  {
    struct stat sb;
    if (fstat(file, &sb) == 0)
    {
      int fontSize = static_cast<int>(sb.st_size);
      void *pFontMem = mmap(NULL, fontSize, PROT_READ, MAP_PRIVATE | MAP_POPULATE, file, 0);
      if (pFontMem != MAP_FAILED)
      {
	pData = std::make_unique<IFontData>(pFontMem, fontSize, 0);
	munmap(pFontMem, fontSize);
      }
    }
    close(file);
  }
  return pData;
}


PlatformFontPtr IGraphicsLinux::LoadPlatformFont(const char* fontID, const char* fileNameOrResID)
{
  WDL_String fullPath;
  const EResourceLocation fontLocation = LocateResource(fileNameOrResID, "ttf", fullPath, GetBundleID(), GetWinModuleHandle(), nullptr);

  if ((fontLocation == kNotFound) || (fontLocation != kAbsolutePath) )
  {
    return nullptr;
  }

  return PlatformFontPtr(new Font(fullPath));
}

PlatformFontPtr IGraphicsLinux::LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style)
{
  WDL_String fullPath;
  const char *styleString;
  switch ( style )
  {
    case ETextStyle::Bold: styleString = "bold"; break;
    case ETextStyle::Italic: styleString = "italic"; break;
    default: styleString = "regular";
  }

  FcConfig  *config = FcInitLoadConfigAndFonts(); // TODO: init/fini for plug-in lifetime
  FcPattern *pat = FcPatternBuild(nullptr, FC_FAMILY, FcTypeString, fontName, FC_STYLE, FcTypeString, styleString, nullptr);
  FcConfigSubstitute(config, pat, FcMatchPattern);
  FcResult result;
  FcPattern *font = FcFontMatch(config, pat, &result);
  if ( font )
  {
    FcChar8 *file;
    if ( FcPatternGetString(font, FC_FILE, 0, &file) == FcResultMatch )
    {
      fullPath.Set((const char *)file);
    }
    FcPatternDestroy( font );
  }

  FcPatternDestroy(pat);
  FcConfigDestroy(config);

  return PlatformFontPtr(fullPath.Get()[0] ? new Font(fullPath) : nullptr);
}



IGraphicsLinux::IGraphicsLinux(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
  : IGRAPHICS_DRAW_CLASS(dlg, w, h, fps, scale)
{
  //FcInit();
#ifdef IGRAPHICS_GL
  gdk_window_add_filter(nullptr, (GdkFilterFunc) GLFilter, this);
#endif
}

IGraphicsLinux::~IGraphicsLinux()
{
#ifdef IGRAPHICS_GL
  gdk_window_remove_filter(nullptr, (GdkFilterFunc) GLFilter, this);
#endif
  CloseWindow();
  // FcFini();
}


#ifndef NO_IGRAPHICS
#if defined IGRAPHICS_AGG
  #include "IGraphicsAGG.cpp"
#elif defined IGRAPHICS_CAIRO
  #include "IGraphicsCairo.cpp"
#elif defined IGRAPHICS_LICE
  #include "IGraphicsLice.cpp"
#elif defined IGRAPHICS_SKIA
  #include "IGraphicsSkia.cpp"
  #ifdef IGRAPHICS_GL
    #include "glad.c"
  #endif
#elif defined IGRAPHICS_NANOVG
  #include "IGraphicsNanoVG.cpp"
#ifdef IGRAPHICS_FREETYPE
#define FONS_USE_FREETYPE
#endif
  #include "nanovg.c"
  #include "glad.c"
#else
  #error
#endif
#endif
