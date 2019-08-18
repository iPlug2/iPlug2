/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/


#include <Shlobj.h>
#include <commctrl.h>

#include "heapbuf.h"

#include "IPlugParameter.h"
#include "IGraphicsWin.h"
#include "IPopupMenuControl.h"
#include "IPlugPaths.h"

#include <wininet.h>

using namespace iplug;
using namespace igraphics;

#pragma warning(disable:4244) // Pointer size cast mismatch.
#pragma warning(disable:4312) // Pointer size cast mismatch.
#pragma warning(disable:4311) // Pointer size cast mismatch.

static int nWndClassReg = 0;
static const char* wndClassName = "IPlugWndClass";
static double sFPS = 0.0;

#define PARAM_EDIT_ID 99
#define IPLUG_TIMER_ID 2
#define IPLUG_WIN_MAX_WIDE_PATH 4096

#pragma mark - Private Classes and Structs

// Fonts

class IGraphicsWin::InstalledFont
{
public:
  InstalledFont(void* data, int resSize)
  : mFontHandle(nullptr)
  {
    if (data)
    {
      DWORD numFonts;
      mFontHandle = AddFontMemResourceEx(data, resSize, NULL, &numFonts);
    }
  }
  
  ~InstalledFont()
  {
    if (IsValid())
      RemoveFontMemResourceEx(mFontHandle);
  }
  
  InstalledFont(const InstalledFont&) = delete;
  InstalledFont& operator=(const InstalledFont&) = delete;
    
  bool IsValid() const { return mFontHandle; }
  
private:
  HANDLE mFontHandle;
};

struct IGraphicsWin::HFontHolder
{
  HFontHolder(HFONT hfont) : mHFont(nullptr)
  {
    LOGFONT lFont = { 0 };
    GetObject(hfont, sizeof(LOGFONT), &lFont);
    mHFont = CreateFontIndirect(&lFont);
  }
  
  HFONT mHFont;
};

class IGraphicsWin::Font : public PlatformFont
{
public:
  Font(HFONT font, const char* styleName, bool system)
  : PlatformFont(system), mFont(font), mStyleName(styleName) {}
  ~Font()
  {
    DeleteObject(mFont);
  }
  
  FontDescriptor GetDescriptor() override { return mFont; }
  IFontDataPtr GetFontData() override;
  
private:
  HFONT mFont;
  WDL_String mStyleName;
};

IFontDataPtr IGraphicsWin::Font::GetFontData()
{
  HDC hdc = CreateCompatibleDC(NULL);
  IFontDataPtr fontData(new IFontData());
  
  if (hdc != NULL)
  {
    SelectObject(hdc, mFont);
    const size_t size = ::GetFontData(hdc, 0, 0, NULL, 0);

    if (size != GDI_ERROR)
    {
      fontData = std::make_unique<IFontData>(size);

      if (fontData->GetSize() == size)
      {
        size_t result = ::GetFontData(hdc, 0x66637474, 0, fontData->Get(), size);
        if (result == GDI_ERROR)
          result = ::GetFontData(hdc, 0, 0, fontData->Get(), size);
        if (result == size)
          fontData->SetFaceIdx(GetFaceIdx(fontData->Get(), fontData->GetSize(), mStyleName.Get()));
      }
    }
    
    DeleteDC(hdc);
  }

  return fontData;
}

StaticStorage<IGraphicsWin::InstalledFont> IGraphicsWin::sPlatformFontCache;
StaticStorage<IGraphicsWin::HFontHolder> IGraphicsWin::sHFontCache;

#pragma mark - DPI Helper

// DPI helper
UINT(WINAPI *__GetDpiForWindow)(HWND);

// Mouse and tablet helpers
static int GetScaleForWindow(HWND hWnd)
{
  if (hWnd && __GetDpiForWindow)
  {
    int dpi = __GetDpiForWindow(hWnd);
    if (dpi != USER_DEFAULT_SCREEN_DPI)
      return std::round(static_cast<double>(dpi) / USER_DEFAULT_SCREEN_DPI);
  }

  return 1;
}

#pragma mark -

inline IMouseInfo IGraphicsWin::GetMouseInfo(LPARAM lParam, WPARAM wParam)
{
  IMouseInfo info;
  info.x = mCursorX = GET_X_LPARAM(lParam) / (GetDrawScale() * GetScreenScale());
  info.y = mCursorY = GET_Y_LPARAM(lParam) / (GetDrawScale() * GetScreenScale());
  info.ms = IMouseMod((wParam & MK_LBUTTON), (wParam & MK_RBUTTON), (wParam & MK_SHIFT), (wParam & MK_CONTROL),
#ifdef AAX_API
    GetAsyncKeyState(VK_MENU) < 0
#else
    GetKeyState(VK_MENU) < 0
#endif
  );
  return info;
}

inline IMouseInfo IGraphicsWin::GetMouseInfoDeltas(float& dX, float& dY, LPARAM lParam, WPARAM wParam)
{
  float oldX = mCursorX;
  float oldY = mCursorY;
  
  IMouseInfo info = GetMouseInfo(lParam, wParam);

  dX = info.x - oldX;
  dY = info.y - oldY;
  
  return info;
}

void IGraphicsWin::CheckTabletInput(UINT msg)
{
  if ((msg == WM_LBUTTONDOWN) || (msg == WM_RBUTTONDOWN) || (msg == WM_MBUTTONDOWN) || (msg == WM_MOUSEMOVE)
      || (msg == WM_RBUTTONDBLCLK) || (msg == WM_LBUTTONDBLCLK) || (msg == WM_MBUTTONDBLCLK)
      || (msg == WM_RBUTTONUP) || (msg == WM_LBUTTONUP) || (msg == WM_MBUTTONUP)
      || (msg == WM_MOUSEHOVER) || (msg == WM_MOUSELEAVE))
  {
    const LONG_PTR c_SIGNATURE_MASK = 0xFFFFFF00;
    const LONG_PTR c_MOUSEEVENTF_FROMTOUCH = 0xFF515700;
    
    LONG_PTR extraInfo = GetMessageExtraInfo();
    SetTabletInput(((extraInfo & c_SIGNATURE_MASK) == c_MOUSEEVENTF_FROMTOUCH));
    mCursorLock &= !mTabletInput;
  }
}

void IGraphicsWin::DestroyEditWindow()
{
 if (mParamEditWnd)
 {
   SetWindowLongPtr(mParamEditWnd, GWLP_WNDPROC, (LPARAM) mDefEditProc);
   DestroyWindow(mParamEditWnd);
   mParamEditWnd = nullptr;
   mDefEditProc = nullptr;
   DeleteObject(mEditFont);
   mEditFont = nullptr;
 }
}

// static
LRESULT CALLBACK IGraphicsWin::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (msg == WM_CREATE)
  {
    LPCREATESTRUCT lpcs = (LPCREATESTRUCT) lParam;
    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LPARAM) (lpcs->lpCreateParams));
    int mSec = static_cast<int>(std::round(1000.0 / (sFPS)));
    SetTimer(hWnd, IPLUG_TIMER_ID, mSec, NULL);
    SetFocus(hWnd); // gets scroll wheel working straight away
    DragAcceptFiles(hWnd, true);
    return 0;
  }

  IGraphicsWin* pGraphics = (IGraphicsWin*) GetWindowLongPtr(hWnd, GWLP_USERDATA);
  char txt[MAX_WIN32_PARAM_LEN];
  double v;

  if (!pGraphics || hWnd != pGraphics->mPlugWnd)
  {
    return DefWindowProc(hWnd, msg, wParam, lParam);
  }

  if (pGraphics->mParamEditWnd && pGraphics->mParamEditMsg == kEditing)
  {
    if (msg == WM_RBUTTONDOWN || (msg == WM_LBUTTONDOWN))
    {
      pGraphics->mParamEditMsg = kCancel;
      return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
  }

  pGraphics->CheckTabletInput(msg);
  
  switch (msg)
  {
    case WM_TIMER:
    {
      if (wParam == IPLUG_TIMER_ID)
      {
        if (pGraphics->mParamEditWnd && pGraphics->mParamEditMsg != kNone)
        {
          switch (pGraphics->mParamEditMsg)
          {
            case kCommit:
            {
              SendMessage(pGraphics->mParamEditWnd, WM_GETTEXT, MAX_WIN32_PARAM_LEN, (LPARAM) txt);
              pGraphics->SetControlValueAfterTextEdit(txt);
              pGraphics->DestroyEditWindow();
            }
            break;
                  
            case kCancel:
            {
              pGraphics->DestroyEditWindow();
            }
            break;
          }
          pGraphics->mParamEditMsg = kNone;
          return 0; // TODO: check this!
        }

        int scale = GetScaleForWindow(pGraphics->mPlugWnd);
        if (scale != pGraphics->GetScreenScale())
          pGraphics->SetScreenScale(scale);

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
            IRECT notDirtyR = pGraphics->mEditRECT;
            notDirtyR.Scale(pGraphics->GetDrawScale() * pGraphics->GetScreenScale());
            notDirtyR.PixelAlign();
            RECT r2 = { (LONG) notDirtyR.L, (LONG) notDirtyR.T, (LONG) notDirtyR.R, (LONG) notDirtyR.B };
            ValidateRect(hWnd, &r2); // make sure we dont redraw the edit box area
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
    }

    case WM_RBUTTONDOWN:
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    {
      pGraphics->HideTooltip();
      if (pGraphics->mParamEditWnd)
      {
        pGraphics->mParamEditMsg = kCommit;
        return 0;
      }
      SetFocus(hWnd); // Added to get keyboard focus again when user clicks in window
      SetCapture(hWnd);
      IMouseInfo info = pGraphics->GetMouseInfo(lParam, wParam);
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
      if (!(wParam & (MK_LBUTTON | MK_RBUTTON)))
      {
        IMouseInfo info = pGraphics->GetMouseInfo(lParam, wParam);
        if (pGraphics->OnMouseOver(info.x, info.y, info.ms))
        {
          TRACKMOUSEEVENT eventTrack = { sizeof(TRACKMOUSEEVENT), TME_LEAVE, hWnd, HOVER_DEFAULT };
          if (pGraphics->TooltipsEnabled()) 
          {
            int c = pGraphics->GetMouseOver();
            if (c != pGraphics->mTooltipIdx) 
            {
              if (c >= 0) eventTrack.dwFlags |= TME_HOVER;
              pGraphics->mTooltipIdx = c;
              pGraphics->HideTooltip();
            }
          }

          TrackMouseEvent(&eventTrack);
        }
      }
      else if (GetCapture() == hWnd && !pGraphics->IsInTextEntry())
      {
        float dX, dY;
        IMouseInfo info = pGraphics->GetMouseInfoDeltas(dX, dY, lParam, wParam);
        if (dX || dY)
        {
          pGraphics->OnMouseDrag(info.x, info.y, dX, dY, info.ms);
          if (pGraphics->MouseCursorIsLocked())
            pGraphics->MoveMouseCursor(pGraphics->mHiddenCursorX, pGraphics->mHiddenCursorY);
        }
      }

      return 0;
    }
    case WM_MOUSEHOVER: 
    {
      pGraphics->ShowTooltip();
      return 0;
    }
    case WM_MOUSELEAVE:
    {
      pGraphics->HideTooltip();
      pGraphics->OnMouseOut();
      return 0;
    }
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    {
      ReleaseCapture();
      IMouseInfo info = pGraphics->GetMouseInfo(lParam, wParam);
      pGraphics->OnMouseUp(info.x, info.y, info.ms);
      return 0;
    }
    case WM_LBUTTONDBLCLK:
    {
      IMouseInfo info = pGraphics->GetMouseInfo(lParam, wParam);
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
        IMouseInfo info = pGraphics->GetMouseInfo(lParam, wParam);
        float d = GET_WHEEL_DELTA_WPARAM(wParam) / WHEEL_DELTA;
        float scale = pGraphics->GetDrawScale() * pGraphics->GetScreenScale();
        RECT r;
        GetWindowRect(hWnd, &r);
        pGraphics->OnMouseWheel(info.x - (r.left / scale), info.y - (r.top / scale), info.ms, d);
        return 0;
      }
    }
    case WM_GETDLGCODE:
      return DLGC_WANTALLKEYS;
    case WM_KEYDOWN:
    case WM_KEYUP:
    {
      POINT p;
      GetCursorPos(&p);
      ScreenToClient(hWnd, &p);

      BYTE keyboardState[256] = {};
      GetKeyboardState(keyboardState);
      const int keyboardScanCode = (lParam >> 16) & 0x00ff;
      WORD character = 0;
      const int len = ToAscii(wParam, keyboardScanCode, keyboardState, &character, 0);
      // TODO: should get unicode?
      bool handle = false;

      // send when len is 0 because wParam might be something like VK_LEFT or VK_HOME, etc.
      if (len == 0 || len == 1)
      {
        char str[2];
        str[0] = static_cast<char>(character);
        str[1] = '\0';
          
        IKeyPress keyPress{ str, static_cast<int>(wParam),
                            static_cast<bool>(GetKeyState(VK_SHIFT) & 0x8000),
                            static_cast<bool>(GetKeyState(VK_CONTROL) & 0x8000),
                            static_cast<bool>(GetKeyState(VK_MENU) & 0x8000) };

        double scale = pGraphics->GetDrawScale() * pGraphics->GetScreenScale();

        if(msg == WM_KEYDOWN)
          handle = pGraphics->OnKeyDown(p.x / scale, p.y / scale, keyPress);
        else
          handle = pGraphics->OnKeyUp(p.x / scale, p.y / scale, keyPress);
      }

      if (!handle)
      {
        HWND rootHWnd = GetAncestor( hWnd, GA_ROOT);
        SendMessage(rootHWnd, msg, wParam, lParam);
        return DefWindowProc(hWnd, msg, wParam, lParam);
      }
      else
        return 0;
    }
    case WM_PAINT:
    {
      auto addDrawRect = [pGraphics](IRECTList& rects, RECT r) {
        IRECT ir(r.left, r.top, r.right, r.bottom);
        ir.Scale(1.f / (pGraphics->GetDrawScale() * pGraphics->GetScreenScale()));
        ir.PixelAlign();
        rects.Add(ir);
      };

      HRGN region = CreateRectRgn(0, 0, 0, 0);
      int regionType = GetUpdateRgn(hWnd, region, FALSE);

      if ((regionType == COMPLEXREGION) || (regionType == SIMPLEREGION))
      {
        #ifdef IGRAPHICS_GL
        pGraphics->ActivateGLContext();
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps); // TODO: BeginPaint/EndPaint and GetDC/ReleaseDC ? issues closing reaper without this
        #endif

        IRECTList rects;
        const int bufferSize = sizeof(RECT) * 64;
        unsigned char stackBuffer[sizeof(RGNDATA) + bufferSize];
        RGNDATA* regionData = (RGNDATA *) stackBuffer;

        if (regionType == COMPLEXREGION && GetRegionData(region, bufferSize, regionData))
        {
          for (int i = 0; i < regionData->rdh.nCount; i++)
            addDrawRect(rects, *(((RECT*) regionData->Buffer) + i));
        }
        else
        {
          RECT r;
          GetRgnBox(region, &r);
          addDrawRect(rects, r);
        }

        pGraphics->Draw(rects);

        #ifdef IGRAPHICS_GL
        SwapBuffers((HDC) pGraphics->mPlatformContext);
        pGraphics->DeactivateGLContext();
        EndPaint(hWnd, &ps);
        #endif
      }

      DeleteObject(region);
      return 0;
    }

    case WM_CTLCOLOREDIT:
    {
      if(!pGraphics->mParamEditWnd)
        return 0;

      const IText& text = pGraphics->mEditText;
      HDC dc = (HDC) wParam;
      SetBkColor(dc, RGB(text.mTextEntryBGColor.R, text.mTextEntryBGColor.G, text.mTextEntryBGColor.B));
      SetTextColor(dc, RGB(text.mTextEntryFGColor.R, text.mTextEntryFGColor.G, text.mTextEntryFGColor.B));
      SetBkMode(dc, OPAQUE);
      SetDCBrushColor(dc, RGB(text.mTextEntryBGColor.R, text.mTextEntryBGColor.G, text.mTextEntryBGColor.B));
      return (LRESULT)GetStockObject(DC_BRUSH);
    }
    case WM_DROPFILES:
    {
      HDROP hdrop = (HDROP)wParam;
      
      char pathToFile[1025];
      DragQueryFile(hdrop, 0, pathToFile, 1024);
      
      POINT point;
      DragQueryPoint(hdrop, &point);
      
      pGraphics->OnDrop(pathToFile, point.x, point.y);
      
      return 0;
    }
    case WM_CLOSE:
    {
      pGraphics->CloseWindow();
      return 0;
    }
    case WM_SETFOCUS:
    {
      return 0;
    }
    case WM_KILLFOCUS:
    {
      return 0;
    }
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}

// static
LRESULT CALLBACK IGraphicsWin::ParamEditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  IGraphicsWin* pGraphics = (IGraphicsWin*) GetWindowLongPtr(GetParent(hWnd), GWLP_USERDATA);

  if (pGraphics && pGraphics->mParamEditWnd && pGraphics->mParamEditWnd == hWnd)
  {
    pGraphics->HideTooltip();

    switch (msg)
    {
      case WM_CHAR:
      {
        // limit to numbers for text entry on appropriate parameters
        if(pGraphics->mEditParam)
        {
          char c = wParam;

          if(c == 0x08) break; // backspace

          switch (pGraphics->mEditParam->Type())
          {
            case IParam::kTypeEnum:
            case IParam::kTypeInt:
            case IParam::kTypeBool:
              if (c >= '0' && c <= '9') break;
              else if (c == '-') break;
              else if (c == '+') break;
              else return 0;
            case IParam::kTypeDouble:
              if (c >= '0' && c <= '9') break;
              else if (c == '-') break;
              else if (c == '+') break;
              else if (c == '.') break;
              else return 0;
            default:
              break;
          }
        }
        break;
      }
      case WM_KEYDOWN:
      {
        if (wParam == VK_RETURN)
        {
          pGraphics->mParamEditMsg = kCommit;
          return 0;
        }
        else if (wParam == VK_ESCAPE)
        {
          pGraphics->mParamEditMsg = kCancel;
          return 0;
        }
        break;
      }
      case WM_SETFOCUS:
      {
        pGraphics->mParamEditMsg = kEditing;
        break;
      }
      case WM_KILLFOCUS:
      {
        pGraphics->mParamEditMsg = kCommit;
        break;
      }
      // handle WM_GETDLGCODE so that we can say that we want the return key message
      //  (normally single line edit boxes don't get sent return key messages)
      case WM_GETDLGCODE:
      {
        LPARAM lres;
        // find out if the original control wants it
        lres = CallWindowProc(pGraphics->mDefEditProc, hWnd, WM_GETDLGCODE, wParam, lParam);
        // add in that we want it if it is a return keydown
        if (lParam && ((MSG*)lParam)->message == WM_KEYDOWN  &&  wParam == VK_RETURN)
        {
          lres |= DLGC_WANTMESSAGE;
        }
        return lres;
      }
      case WM_COMMAND:
      {
        switch HIWORD(wParam)
        {
          case CBN_SELCHANGE:
          {
            if (pGraphics->mParamEditWnd)
            {
              pGraphics->mParamEditMsg = kCommit;
              return 0;
            }
          }

        }
        break;  // Else let the default proc handle it.
      }
    }
    return CallWindowProc(pGraphics->mDefEditProc, hWnd, msg, wParam, lParam);
  }
  return DefWindowProc(hWnd, msg, wParam, lParam);
}

IGraphicsWin::IGraphicsWin(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
  : IGRAPHICS_DRAW_CLASS(dlg, w, h, fps, scale)
{
  if (!__GetDpiForWindow)
  {
    HINSTANCE h = LoadLibrary("user32.dll");
    if (h) *(void **)&__GetDpiForWindow = GetProcAddress(h, "GetDpiForWindow");
  }

  StaticStorage<InstalledFont>::Accessor fontStorage(sPlatformFontCache);
  StaticStorage<HFontHolder>::Accessor hfontStorage(sHFontCache);
  fontStorage.Retain();
  hfontStorage.Retain();
}

IGraphicsWin::~IGraphicsWin()
{
  StaticStorage<InstalledFont>::Accessor fontStorage(sPlatformFontCache);
  StaticStorage<HFontHolder>::Accessor hfontStorage(sHFontCache);
  fontStorage.Release();
  hfontStorage.Release();
  DestroyEditWindow();
  CloseWindow();
}

static void GetWindowSize(HWND pWnd, int* pW, int* pH)
{
  if (pWnd)
  {
    RECT r;
    GetWindowRect(pWnd, &r);
    *pW = r.right - r.left;
    *pH = r.bottom - r.top;
  }
  else
  {
    *pW = *pH = 0;
  }
}

static bool IsChildWindow(HWND pWnd)
{
  if (pWnd)
  {
    int style = GetWindowLong(pWnd, GWL_STYLE);
    int exStyle = GetWindowLong(pWnd, GWL_EXSTYLE);
    return ((style & WS_CHILD) && !(exStyle & WS_EX_MDICHILD));
  }
  return false;
}

void IGraphicsWin::ForceEndUserEdit()
{
  mParamEditMsg = kCancel;
}

static UINT SETPOS_FLAGS = SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE;

void IGraphicsWin::PlatformResize(bool parentHasResized)
{
  if (WindowIsOpen())
  {
    HWND pParent = 0, pGrandparent = 0;
    int dlgW = 0, dlgH = 0, parentW = 0, parentH = 0, grandparentW = 0, grandparentH = 0;
    GetWindowSize(mPlugWnd, &dlgW, &dlgH);
    int dw = (WindowWidth() * GetScreenScale()) - dlgW, dh = (WindowHeight()* GetScreenScale()) - dlgH;
      
    if (IsChildWindow(mPlugWnd))
    {
      pParent = GetParent(mPlugWnd);
      GetWindowSize(pParent, &parentW, &parentH);

      if (IsChildWindow(pParent))
      {
        pGrandparent = GetParent(pParent);
        GetWindowSize(pGrandparent, &grandparentW, &grandparentH);
      }
    }

    if (!dw && !dh)
      return;

    SetWindowPos(mPlugWnd, 0, 0, 0, dlgW + dw, dlgH + dh, SETPOS_FLAGS);

    if(pParent && !parentHasResized)
    {
      SetWindowPos(pParent, 0, 0, 0, parentW + dw, parentH + dh, SETPOS_FLAGS);
    }

    if(pGrandparent && !parentHasResized)
    {
      SetWindowPos(pGrandparent, 0, 0, 0, grandparentW + dw, grandparentH + dh, SETPOS_FLAGS);
    }

    RECT r = { 0, 0, WindowWidth() * GetScreenScale(), WindowHeight() * GetScreenScale() };
    InvalidateRect(mPlugWnd, &r, FALSE);
  }
}

#ifdef IGRAPHICS_GL
void IGraphicsWin::DrawResize()
{
  ActivateGLContext();
  IGRAPHICS_DRAW_CLASS::DrawResize();
  DeactivateGLContext();
}
#endif

void IGraphicsWin::HideMouseCursor(bool hide, bool lock)
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

void IGraphicsWin::MoveMouseCursor(float x, float y)
{
  if (mTabletInput)
    return;
 
  float scale = GetDrawScale() * GetScreenScale();
    
  POINT p;
  p.x = std::round(x * scale);
  p.y = std::round(y * scale);
  
  ::ClientToScreen((HWND)GetWindow(), &p);
  
  if (SetCursorPos(p.x, p.y))
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

ECursor IGraphicsWin::SetMouseCursor(ECursor cursorType)
{
  HCURSOR cursor;

  switch (cursorType)
  {
    case ECursor::ARROW:            cursor = LoadCursor(NULL, IDC_ARROW);           break;
    case ECursor::IBEAM:            cursor = LoadCursor(NULL, IDC_IBEAM);           break;
    case ECursor::WAIT:             cursor = LoadCursor(NULL, IDC_WAIT);            break;
    case ECursor::CROSS:            cursor = LoadCursor(NULL, IDC_CROSS);           break;
    case ECursor::UPARROW:          cursor = LoadCursor(NULL, IDC_UPARROW);         break;
    case ECursor::SIZENWSE:         cursor = LoadCursor(NULL, IDC_SIZENWSE);        break;
    case ECursor::SIZENESW:         cursor = LoadCursor(NULL, IDC_SIZENESW);        break;
    case ECursor::SIZEWE:           cursor = LoadCursor(NULL, IDC_SIZEWE);          break;
    case ECursor::SIZENS:           cursor = LoadCursor(NULL, IDC_SIZENS);          break;
    case ECursor::SIZEALL:          cursor = LoadCursor(NULL, IDC_SIZEALL);         break;
    case ECursor::INO:              cursor = LoadCursor(NULL, IDC_NO);              break;
    case ECursor::HAND:             cursor = LoadCursor(NULL, IDC_HAND);            break;
    case ECursor::APPSTARTING:      cursor = LoadCursor(NULL, IDC_APPSTARTING);     break;
    case ECursor::HELP:             cursor = LoadCursor(NULL, IDC_HELP);            break;
    default:
      cursor = LoadCursor(NULL, IDC_ARROW);
  }

  SetCursor(cursor);
  return IGraphics::SetMouseCursor(cursorType);
}

bool IGraphicsWin::MouseCursorIsLocked()
{
  return mCursorLock;
}

#ifdef IGRAPHICS_GL
void IGraphicsWin::CreateGLContext()
{
  PIXELFORMATDESCRIPTOR pfd =
  {
    sizeof(PIXELFORMATDESCRIPTOR),
    1,
    PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, //Flags
    PFD_TYPE_RGBA, // The kind of framebuffer. RGBA or palette.
    32, // Colordepth of the framebuffer.
    0, 0, 0, 0, 0, 0,
    0,
    0,
    0,
    0, 0, 0, 0,
    24, // Number of bits for the depthbuffer
    8, // Number of bits for the stencilbuffer
    0, // Number of Aux buffers in the framebuffer.
    PFD_MAIN_PLANE,
    0,
    0, 0, 0
  };

  HDC dc = GetDC(mPlugWnd);
  int fmt = ChoosePixelFormat(dc, &pfd);
  SetPixelFormat(dc, fmt, &pfd);

  mHGLRC = wglCreateContext(dc);
  wglMakeCurrent(dc, mHGLRC);

  //TODO: return false if GL init fails?
  if (!gladLoadGL())
    DBGMSG("Error initializing glad");

  glGetError();

  ReleaseDC(mPlugWnd, dc);
}

void IGraphicsWin::DestroyGLContext()
{
  wglMakeCurrent(NULL, NULL);
  wglDeleteContext(mHGLRC);
}

void IGraphicsWin::ActivateGLContext()
{
  mStartHDC = wglGetCurrentDC();
  mStartHGLRC = wglGetCurrentContext();
  HDC dc = GetDC(mPlugWnd);
  wglMakeCurrent(dc, mHGLRC);
}

void IGraphicsWin::DeactivateGLContext()
{
  ReleaseDC(mPlugWnd, (HDC) mPlatformContext);
  wglMakeCurrent(mStartHDC, mStartHGLRC); // return current ctxt to start
}
#endif

EMsgBoxResult IGraphicsWin::ShowMessageBox(const char* text, const char* caption, EMsgBoxType type, IMsgBoxCompletionHanderFunc completionHandler)
{
  ReleaseMouseCapture();
  
  EMsgBoxResult result = static_cast<EMsgBoxResult>(MessageBox(GetMainWnd(), text, caption, static_cast<int>(type)));
  
  if(completionHandler)
    completionHandler(result);
  
  return result;
}

void* IGraphicsWin::OpenWindow(void* pParent)
{
  mParentWnd = (HWND) pParent;
  int screenScale = GetScaleForWindow(mParentWnd);
  int x = 0, y = 0, w = WindowWidth() * screenScale, h = WindowHeight() * screenScale;

  if (mPlugWnd)
  {
    RECT pR, cR;
    GetWindowRect((HWND) pParent, &pR);
    GetWindowRect(mPlugWnd, &cR);
    CloseWindow();
    x = cR.left - pR.left;
    y = cR.top - pR.top;
    w = cR.right - cR.left;
    h = cR.bottom - cR.top;
  }

  if (nWndClassReg++ == 0)
  {
    WNDCLASS wndClass = { CS_DBLCLKS | CS_OWNDC, WndProc, 0, 0, mHInstance, 0, 0, 0, 0, wndClassName };
    RegisterClass(&wndClass);
  }

  sFPS = FPS();
  mPlugWnd = CreateWindow(wndClassName, "IPlug", WS_CHILD | WS_VISIBLE, x, y, w, h, mParentWnd, 0, mHInstance, this);

  HDC dc = GetDC(mPlugWnd);
  SetPlatformContext(dc);
  ReleaseDC(mPlugWnd, dc);

#ifdef IGRAPHICS_GL
  CreateGLContext();
#endif

  OnViewInitialized((void*) dc);

  SetScreenScale(screenScale); // resizes draw context

  GetDelegate()->LayoutUI(this);

  if (!mPlugWnd && --nWndClassReg == 0)
  {
    UnregisterClass(wndClassName, mHInstance);
  }
  else
  {
    SetAllControlsDirty();
  }

  if (mPlugWnd && TooltipsEnabled())
  {
    bool ok = false;
    static const INITCOMMONCONTROLSEX iccex = { sizeof(INITCOMMONCONTROLSEX), ICC_TAB_CLASSES };

    if (InitCommonControlsEx(&iccex))
    {
      mTooltipWnd = CreateWindowEx(0, TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
                                   CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, mPlugWnd, NULL, mHInstance, NULL);
      if (mTooltipWnd)
      {
        SetWindowPos(mTooltipWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
        TOOLINFO ti = { TTTOOLINFOA_V2_SIZE, TTF_IDISHWND | TTF_SUBCLASS, mPlugWnd, (UINT_PTR)mPlugWnd };
        ti.lpszText = (LPTSTR)NULL;
        SendMessage(mTooltipWnd, TTM_ADDTOOL, 0, (LPARAM)&ti);
        ok = true;
      }
    }

    if (!ok) EnableTooltips(ok);
  }

  GetDelegate()->OnUIOpen();
  
  return mPlugWnd;
}

static void GetWndClassName(HWND hWnd, WDL_String* pStr)
{
  char cStr[MAX_CLASSNAME_LEN];
  cStr[0] = '\0';
  GetClassName(hWnd, cStr, MAX_CLASSNAME_LEN);
  pStr->Set(cStr);
}

BOOL CALLBACK IGraphicsWin::FindMainWindow(HWND hWnd, LPARAM lParam)
{
  IGraphicsWin* pGraphics = (IGraphicsWin*) lParam;
  if (pGraphics)
  {
    DWORD wPID;
    GetWindowThreadProcessId(hWnd, &wPID);
    WDL_String str;
    GetWndClassName(hWnd, &str);
    if (wPID == pGraphics->mPID && !strcmp(str.Get(), pGraphics->mMainWndClassName.Get()))
    {
      pGraphics->mMainWnd = hWnd;
      return FALSE;   // Stop enumerating.
    }
  }
  return TRUE;
}

HWND IGraphicsWin::GetMainWnd()
{
  if (!mMainWnd)
  {
    if (mParentWnd)
    {
      HWND parentWnd = mParentWnd;
      while (parentWnd)
      {
        mMainWnd = parentWnd;
        parentWnd = GetParent(mMainWnd);
      }
      
      GetWndClassName(mMainWnd, &mMainWndClassName);
    }
    else if (CStringHasContents(mMainWndClassName.Get()))
    {
      mPID = GetCurrentProcessId();
      EnumWindows(FindMainWindow, (LPARAM) this);
    }
  }
  return mMainWnd;
}

IRECT IGraphicsWin::GetWindowRECT()
{
  if (mPlugWnd)
  {
    RECT r;
    GetWindowRect(mPlugWnd, &r);
    r.right -= TOOLWIN_BORDER_W;
    r.bottom -= TOOLWIN_BORDER_H;
    return IRECT(r.left, r.top, r.right, r.bottom);
  }
  return IRECT();
}

void IGraphicsWin::SetWindowTitle(const char* str)
{
  SetWindowText(mPlugWnd, str);
}

void IGraphicsWin::CloseWindow()
{
  if (mPlugWnd)
  {
    OnViewDestroyed();

#ifdef IGRAPHICS_GL
    DestroyGLContext();
#endif

    SetPlatformContext(nullptr);

    if (mTooltipWnd)
    {
      DestroyWindow(mTooltipWnd);
      mTooltipWnd = 0;
      mShowingTooltip = false;
      mTooltipIdx = -1;
    }

    DestroyWindow(mPlugWnd);
    mPlugWnd = 0;

    if (--nWndClassReg == 0)
    {
      UnregisterClass(wndClassName, mHInstance);
    }
  }
}

IPopupMenu* IGraphicsWin::GetItemMenu(long idx, long& idxInMenu, long& offsetIdx, IPopupMenu& baseMenu)
{
  long oldIDx = offsetIdx;
  offsetIdx += baseMenu.NItems();

  if (idx < offsetIdx)
  {
    idxInMenu = idx - oldIDx;
    return &baseMenu;
  }

  IPopupMenu* pMenu = nullptr;

  for(int i = 0; i< baseMenu.NItems(); i++)
  {
    IPopupMenu::Item* pMenuItem = baseMenu.GetItem(i);
    if(pMenuItem->GetSubmenu())
    {
      pMenu = GetItemMenu(idx, idxInMenu, offsetIdx, *pMenuItem->GetSubmenu());

      if(pMenu)
        break;
    }
  }

  return pMenu;
}

HMENU IGraphicsWin::CreateMenu(IPopupMenu& menu, long* pOffsetIdx)
{
  HMENU hMenu = ::CreatePopupMenu();

  WDL_String escapedText;

  int flags = 0;
  long offset = *pOffsetIdx;
  long nItems = menu.NItems();
  *pOffsetIdx += nItems;
  long inc = 0;

  for(int i = 0; i < nItems; i++)
  {
    IPopupMenu::Item* pMenuItem = menu.GetItem(i);

    if (pMenuItem->GetIsSeparator())
    {
      AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
    }
    else
    {
      const char* str = pMenuItem->GetText();
      int maxlen = strlen(str) + menu.GetPrefix() ? 50 : 0;
      WDL_String entryText(str);

      if (menu.GetPrefix())
      {
        switch (menu.GetPrefix())
        {
          case 1:
          {
            entryText.SetFormatted(maxlen, "%1d: %s", i+1, str); break;
          }
          case 2:
          {
            entryText.SetFormatted(maxlen, "%02d: %s", i+1, str); break;
          }
          case 3:
          {
            entryText.SetFormatted(maxlen, "%03d: %s", i+1, str); break;
          }
        }
      }

      // Escape ampersands if present

      if (strchr(entryText.Get(), '&'))
      {
        for (int c = 0; c < entryText.GetLength(); c++)
          if (entryText.Get()[c] == '&')
            entryText.Insert("&", c++);
      }

      flags = MF_STRING;
      //if (nItems < 160 && pMenu->getNbItemsPerColumn () > 0 && inc && !(inc % _menu->getNbItemsPerColumn ()))
      //  flags |= MF_MENUBARBREAK;

      if (pMenuItem->GetSubmenu())
      {
        HMENU submenu = CreateMenu(*pMenuItem->GetSubmenu(), pOffsetIdx);
        if (submenu)
        {
          AppendMenu(hMenu, flags|MF_POPUP|MF_ENABLED, (UINT_PTR)submenu, (const TCHAR*)entryText.Get());
        }
      }
      else
      {
        if (pMenuItem->GetEnabled())
          flags |= MF_ENABLED;
        else
          flags |= MF_GRAYED;
        if (pMenuItem->GetIsTitle())
          flags |= MF_DISABLED;
        if (pMenuItem->GetChecked())
          flags |= MF_CHECKED;
        else
          flags |= MF_UNCHECKED;

        AppendMenu(hMenu, flags, offset + inc, entryText.Get());
      }
    }
    inc++;
  }

  return hMenu;
}

IPopupMenu* IGraphicsWin::CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT& bounds)
{
  long offsetIdx = 0;
  HMENU hMenu = CreateMenu(menu, &offsetIdx);

  if(hMenu)
  {
    long offsetIdx = 0;
    HMENU hMenu = CreateMenu(menu, &offsetIdx);
    IPopupMenu* result = nullptr;

    if (hMenu)
    {
      POINT cPos;

      cPos.x = bounds.L * (GetDrawScale() * GetScreenScale());
      cPos.y = bounds.B * (GetDrawScale() * GetScreenScale());

      ::ClientToScreen(mPlugWnd, &cPos);

      if (TrackPopupMenu(hMenu, TPM_LEFTALIGN, cPos.x, cPos.y, 0, mPlugWnd, 0))
      {
        MSG msg;
        if (PeekMessage(&msg, mPlugWnd, WM_COMMAND, WM_COMMAND, PM_REMOVE))
        {
          if (HIWORD(msg.wParam) == 0)
          {
            long res = LOWORD(msg.wParam);
            if (res != -1)
            {
              long idx = 0;
              offsetIdx = 0;
              IPopupMenu* pReturnMenu = GetItemMenu(res, idx, offsetIdx, menu);
              if (pReturnMenu)
              {
                result = pReturnMenu;
                result->SetChosenItemIdx(idx);
                
                //synchronous
                if(pReturnMenu && pReturnMenu->GetFunction())
                  pReturnMenu->ExecFunction();
              }
            }
          }
        }
      }
      DestroyMenu(hMenu);

      RECT r = { 0, 0, WindowWidth() * GetScreenScale(), WindowHeight() * GetScreenScale() };
      InvalidateRect(mPlugWnd, &r, FALSE);
    }

    return result;
  }

  return nullptr;
}

void IGraphicsWin::CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str)
{
  if (mParamEditWnd)
    return;

  DWORD editStyle;

  switch ( text.mAlign )
  {
    case EAlign::Near:   editStyle = ES_LEFT;   break;
    case EAlign::Far:    editStyle = ES_RIGHT;  break;
    case EAlign::Center:
    default:                  editStyle = ES_CENTER; break;
  }

  double scale = GetDrawScale() * GetScreenScale();
  IRECT scaledBounds = bounds.GetScaled(scale);

  mParamEditWnd = CreateWindow("EDIT", str, ES_AUTOHSCROLL /*only works for left aligned text*/ | WS_CHILD | WS_VISIBLE | ES_MULTILINE | editStyle,
    scaledBounds.L, scaledBounds.T, scaledBounds.W()+1, scaledBounds.H()+1,
    mPlugWnd, (HMENU) PARAM_EDIT_ID, mHInstance, 0);

  StaticStorage<HFontHolder>::Accessor hfontStorage(sHFontCache);

  LOGFONT lFont = { 0 };
  HFontHolder* hfontHolder = hfontStorage.Find(text.mFont);
  GetObject(hfontHolder->mHFont, sizeof(LOGFONT), &lFont);
  lFont.lfHeight = text.mSize * scale;
  mEditFont = CreateFontIndirect(&lFont);

  assert(hfontHolder && "font not found - did you forget to load it?");

  mEditParam = paramIdx > kNoParameter ? GetDelegate()->GetParam(paramIdx) : nullptr;
  mEditText = text;
  mEditRECT = bounds;

  SendMessage(mParamEditWnd, EM_LIMITTEXT, (WPARAM) length, 0);
  SendMessage(mParamEditWnd, WM_SETFONT, (WPARAM)mEditFont, 0);
  SendMessage(mParamEditWnd, EM_SETSEL, 0, -1);

  SetFocus(mParamEditWnd);

  mDefEditProc = (WNDPROC) SetWindowLongPtr(mParamEditWnd, GWLP_WNDPROC, (LONG_PTR) ParamEditProc);
  SetWindowLongPtr(mParamEditWnd, GWLP_USERDATA, 0xdeadf00b);
}

bool IGraphicsWin::RevealPathInExplorerOrFinder(WDL_String& path, bool select)
{
  bool success = false;
  
  if (path.GetLength())
  {
    WCHAR winDir[IPLUG_WIN_MAX_WIDE_PATH];
    WCHAR explorerWide[IPLUG_WIN_MAX_WIDE_PATH];
    UINT len = GetSystemDirectoryW(winDir, IPLUG_WIN_MAX_WIDE_PATH);
    
    if (len || !(len > MAX_PATH - 2))
    {
      winDir[len]   = L'\\';
      winDir[++len] = L'\0';
      
      WDL_String explorerParams;
      
      if(select)
        explorerParams.Append("/select,");
      
      explorerParams.Append("\"");
      explorerParams.Append(path.Get());
      explorerParams.Append("\\\"");
      
      UTF8ToUTF16(explorerWide, explorerParams.Get(), IPLUG_WIN_MAX_WIDE_PATH);
      HINSTANCE result;
      
      if ((result=::ShellExecuteW(NULL, L"open", L"explorer.exe", explorerWide, winDir, SW_SHOWNORMAL)) <= (HINSTANCE) 32)
        success = true;
    }
  }
  
  return success;
}

void IGraphicsWin::PromptForFile(WDL_String& fileName, WDL_String& path, EFileAction action, const char* extensions)
{
  if (!WindowIsOpen())
  {
    fileName.Set("");
    return;
  }
    
  wchar_t fnCStr[_MAX_PATH];
  wchar_t dirCStr[_MAX_PATH];
    
  if (fileName.GetLength())
    UTF8ToUTF16(fnCStr, fileName.Get(), _MAX_PATH);
  else
    fnCStr[0] = '\0';
    
  dirCStr[0] = '\0';
    
  //if (!path.GetLength())
  //  DesktopPath(path);
    
  UTF8ToUTF16(dirCStr, path.Get(), _MAX_PATH);
    
  OPENFILENAMEW ofn;
  memset(&ofn, 0, sizeof(OPENFILENAMEW));
    
  ofn.lStructSize = sizeof(OPENFILENAMEW);
  ofn.hwndOwner = (HWND) GetWindow();
  ofn.lpstrFile = fnCStr;
  ofn.nMaxFile = _MAX_PATH - 1;
  ofn.lpstrInitialDir = dirCStr;
  ofn.Flags = OFN_PATHMUSTEXIST;
    
  if (CStringHasContents(extensions))
  {
    wchar_t extStr[256];
    wchar_t defExtStr[16];
    int i, p, n = strlen(extensions);
    bool seperator = true;
        
    for (i = 0, p = 0; i < n; ++i)
    {
      if (seperator)
      {
        if (p)
          extStr[p++] = ';';
                
        seperator = false;
        extStr[p++] = '*';
        extStr[p++] = '.';
      }

      if (extensions[i] == ' ')
        seperator = true;
      else
        extStr[p++] = extensions[i];
    }
    extStr[p++] = '\0';
        
    wcscpy(&extStr[p], extStr);
    extStr[p + p] = '\0';
    ofn.lpstrFilter = extStr;
        
    for (i = 0, p = 0; i < n && extensions[i] != ' '; ++i)
      defExtStr[p++] = extensions[i];
    
    defExtStr[p++] = '\0';
    ofn.lpstrDefExt = defExtStr;
  }
    
  bool rc = false;
    
  switch (action)
  {
    case EFileAction::Save:
      ofn.Flags |= OFN_OVERWRITEPROMPT;
      rc = GetSaveFileNameW(&ofn);
      break;
            
    case EFileAction::Open:
      default:
      ofn.Flags |= OFN_FILEMUSTEXIST;
      rc = GetOpenFileNameW(&ofn);
      break;
  }
    
  if (rc)
  {
    char drive[_MAX_DRIVE];
    char directoryOutCStr[_MAX_PATH];
    
    WDL_String tempUTF8;
    UTF16ToUTF8(tempUTF8, ofn.lpstrFile);
    
    if (_splitpath_s(tempUTF8.Get(), drive, sizeof(drive), directoryOutCStr, sizeof(directoryOutCStr), NULL, 0, NULL, 0) == 0)
    {
      path.Set(drive);
      path.Append(directoryOutCStr);
    }
      
    fileName.Set(tempUTF8.Get());
  }
  else
  {
    fileName.Set("");
  }

  ReleaseMouseCapture();
}

void IGraphicsWin::PromptForDirectory(WDL_String& dir)
{
  BROWSEINFO bi;
  memset(&bi, 0, sizeof(bi));
  
  bi.ulFlags   = BIF_USENEWUI;
  bi.hwndOwner = mPlugWnd;
  bi.lpszTitle = "Choose a Directory";
  
  // must call this if using BIF_USENEWUI
  ::OleInitialize(NULL);
  LPITEMIDLIST pIDL = ::SHBrowseForFolder(&bi);
  
  if(pIDL != NULL)
  {
    char buffer[_MAX_PATH] = {'\0'};
    
    if(::SHGetPathFromIDList(pIDL, buffer) != 0)
    {
      dir.Set(buffer);
      dir.Append("\\");
    }
    
    // free the item id list
    CoTaskMemFree(pIDL);
  }
  else
  {
    dir.Set("");
  }

  ReleaseMouseCapture();
  
  ::OleUninitialize();
}

static UINT_PTR CALLBACK CCHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
  if (uiMsg == WM_INITDIALOG && lParam)
  {
    CHOOSECOLOR* cc = (CHOOSECOLOR*) lParam;
    if (cc && cc->lCustData)
    {
      char* str = (char*) cc->lCustData;
      SetWindowText(hdlg, str);
      UINT uiSetRGB;
      uiSetRGB = RegisterWindowMessage(SETRGBSTRING);
      SendMessage(hdlg, uiSetRGB, 0, (LPARAM) cc->rgbResult);
    }
  }
  return 0;
}

bool IGraphicsWin::PromptForColor(IColor& color, const char* prompt, IColorPickerHandlerFunc func)
{
  ReleaseMouseCapture();

  if (!mPlugWnd)
    return false;

  const COLORREF w = RGB(255, 255, 255);
  static COLORREF customColorStorage[16] = { w, w, w, w, w, w, w, w, w, w, w, w, w, w, w, w };
  
  CHOOSECOLOR cc;
  memset(&cc, 0, sizeof(CHOOSECOLOR));
  cc.lStructSize = sizeof(CHOOSECOLOR);
  cc.hwndOwner = mPlugWnd;
  cc.rgbResult = RGB(color.R, color.G, color.B);
  cc.lpCustColors = customColorStorage;
  cc.lCustData = (LPARAM) prompt;
  cc.lpfnHook = CCHookProc;
  cc.Flags = CC_RGBINIT | CC_ANYCOLOR | CC_FULLOPEN | CC_SOLIDCOLOR | CC_ENABLEHOOK;

  if (ChooseColor(&cc))
  {
    color.R = GetRValue(cc.rgbResult);
    color.G = GetGValue(cc.rgbResult);
    color.B = GetBValue(cc.rgbResult);
    
    if(func)
      func(color);
    
    return true;
  }
  return false;
}

bool IGraphicsWin::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
  if (confirmMsg && MessageBox(mPlugWnd, confirmMsg, msgWindowTitle, MB_YESNO) != IDYES)
  {
    return false;
  }
  DWORD inetStatus = 0;
  if (InternetGetConnectedState(&inetStatus, 0))
  {
    WCHAR urlWide[IPLUG_WIN_MAX_WIDE_PATH];
    UTF8ToUTF16(urlWide, url, IPLUG_WIN_MAX_WIDE_PATH);
    if ((int) ShellExecuteW(mPlugWnd, L"open", urlWide, 0, 0, SW_SHOWNORMAL) > MAX_INET_ERR_CODE)
    {
      return true;
    }
  }
  if (errMsgOnFailure)
  {
    MessageBox(mPlugWnd, errMsgOnFailure, msgWindowTitle, MB_OK);
  }
  return false;
}

void IGraphicsWin::SetTooltip(const char* tooltip)
{
  TOOLINFO ti = { TTTOOLINFOA_V2_SIZE, 0, mPlugWnd, (UINT_PTR)mPlugWnd };
  ti.lpszText = (LPTSTR)tooltip;
  SendMessage(mTooltipWnd, TTM_UPDATETIPTEXT, 0, (LPARAM)&ti);
}

void IGraphicsWin::ShowTooltip()
{
  if (mTooltipIdx > -1)
  {
    const char* tooltip = GetControl(mTooltipIdx)->GetTooltip();
    if (tooltip)
    {
      SetTooltip(tooltip);
      mShowingTooltip = true;
    }
  }
}

void IGraphicsWin::HideTooltip()
{
  if (mShowingTooltip)
  {
    SetTooltip(NULL);
    mShowingTooltip = false;
  }
}

bool IGraphicsWin::GetTextFromClipboard(WDL_String& str)
{
  int numChars = 0;
  
  if (IsClipboardFormatAvailable(CF_UNICODETEXT))
  {
    if(OpenClipboard(0))
    {
      HGLOBAL hglb = GetClipboardData(CF_UNICODETEXT);
      
      if (hglb != NULL)
      {
        WCHAR *origStr = (WCHAR*)GlobalLock(hglb);
        
        if (origStr != NULL)
        {
          // Find out how much space is needed

          int newLen = WideCharToMultiByte(CP_UTF8, 0, origStr, -1, 0, 0, NULL, NULL);
          
          if (newLen > 0)
          {
            WDL_TypedBuf<char> utf8;
            utf8.Resize(newLen);
            numChars = WideCharToMultiByte(CP_UTF8, 0, origStr, -1, utf8.Get(), utf8.GetSize(), NULL, NULL);
            str.Set(utf8.Get());
          }
          
          GlobalUnlock(hglb);
        }
      }
    }
    
    CloseClipboard();
  }
  
  if (!numChars)
    str.Set("");
  
  return numChars;
}

bool IGraphicsWin::SetTextInClipboard(const WDL_String& str)
{
  if (!OpenClipboard(mMainWnd))
    return false;

  EmptyClipboard();

  const int len = str.GetLength();
  if (len > 0)
  {
    // figure out how much memory we need for the wide version of this string
    int wchar_len = MultiByteToWideChar(CP_UTF8, 0, str.Get(), -1, NULL, 0);

    // allocate global memory object for the text
    HGLOBAL hglbCopy = GlobalAlloc(GMEM_MOVEABLE, wchar_len*sizeof(WCHAR));
    if (hglbCopy == NULL)
    {
      CloseClipboard();
      return false;
    }

    // lock the handle and copy the string into the buffer
    LPWSTR lpstrCopy = (LPWSTR)GlobalLock(hglbCopy);
    MultiByteToWideChar(CP_UTF8, 0, str.Get(), -1, lpstrCopy, wchar_len);
    GlobalUnlock(hglbCopy);

    // place the handle on the clipboard
    SetClipboardData(CF_UNICODETEXT, hglbCopy);
  }

  CloseClipboard();

  return len > 0;
}

static HFONT GetHFont(const char* fontName, int weight, bool italic, bool underline, DWORD quality = DEFAULT_QUALITY, bool enumerate = false)
{
  HDC hdc = GetDC(NULL);
  HFONT font = nullptr;
  LOGFONT lFont;

  lFont.lfHeight = 0;
  lFont.lfWidth = 0;
  lFont.lfEscapement = 0;
  lFont.lfOrientation = 0;
  lFont.lfWeight = weight;
  lFont.lfItalic = italic;
  lFont.lfUnderline = underline;
  lFont.lfStrikeOut = false;
  lFont.lfCharSet = DEFAULT_CHARSET;
  lFont.lfOutPrecision = OUT_TT_PRECIS;
  lFont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
  lFont.lfQuality = quality;
  lFont.lfPitchAndFamily = DEFAULT_PITCH;

  strncpy(lFont.lfFaceName, fontName, LF_FACESIZE);

  auto enumProc = [](const LOGFONT* pLFont, const TEXTMETRIC* pTextMetric, DWORD FontType, LPARAM lParam)
  {
    return -1;
  };

  if ((!enumerate || EnumFontFamiliesEx(hdc, &lFont, enumProc, NULL, 0) == -1))
    font = CreateFontIndirect(&lFont);

  if (font)
  {
    char selectedFontName[64];

    SelectFont(hdc, font);
    GetTextFace(hdc, 64, selectedFontName);
    if (strcmp(selectedFontName, fontName))
    {
      DeleteObject(font);
      return nullptr;
    }
  }

  ReleaseDC(NULL, hdc);

  return font;
}

PlatformFontPtr IGraphicsWin::LoadPlatformFont(const char* fontID, const char* fileNameOrResID)
{
  StaticStorage<InstalledFont>::Accessor fontStorage(sPlatformFontCache);

  std::unique_ptr<InstalledFont> pFont;
  void* pFontMem = nullptr;
  int resSize = 0;
  WDL_String fullPath;
 
  const EResourceLocation fontLocation = LocateResource(fileNameOrResID, "ttf", fullPath, GetBundleID(), GetWinModuleHandle());

  if (fontLocation == kNotFound)
    return nullptr;

  switch (fontLocation)
  {
    case kAbsolutePath:
    {
      HANDLE file = CreateFile(fullPath.Get(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
      HANDLE mapping = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
      pFontMem = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
      pFont = std::make_unique<InstalledFont>(pFontMem, resSize);
      UnmapViewOfFile(pFontMem);
      CloseHandle(mapping);
      CloseHandle(file);
    }
    break;
    case kWinBinary:
    {
      pFontMem = const_cast<void *>(LoadWinResource(fullPath.Get(), "ttf", resSize, GetWinModuleHandle()));
      pFont = std::make_unique<InstalledFont>(pFontMem, resSize);
    }
    break;
  } 

  if (pFontMem && pFont && pFont->IsValid())
  {
    IFontInfo fontInfo(pFontMem, resSize, 0);
    WDL_String family = fontInfo.GetFamily();
    int weight = fontInfo.IsBold() ? FW_BOLD : FW_REGULAR;
    bool italic = fontInfo.IsItalic();
    bool underline = fontInfo.IsUnderline();

    HFONT font = GetHFont(family.Get(), weight, italic, underline);

    if (font)
    {
      fontStorage.Add(pFont.release(), fileNameOrResID);
      return PlatformFontPtr(new Font(font, "", false));
    }
  }

  return nullptr;
}

PlatformFontPtr IGraphicsWin::LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style)
{
  int weight = style == ETextStyle::Bold ? FW_BOLD : FW_REGULAR;
  bool italic = style == ETextStyle::Italic;
  bool underline = false;
  DWORD quality = DEFAULT_QUALITY;

  HFONT font = GetHFont(fontName, weight, italic, underline, quality, true);

  return PlatformFontPtr(font ? new Font(font, TextStyleString(style), true) : nullptr);
}

void IGraphicsWin::CachePlatformFont(const char* fontID, const PlatformFontPtr& font)
{
  StaticStorage<HFontHolder>::Accessor hfontStorage(sHFontCache);

  HFONT hfont = font->GetDescriptor();

  if (!hfontStorage.Find(fontID))
    hfontStorage.Add(new HFontHolder(hfont), fontID);
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
