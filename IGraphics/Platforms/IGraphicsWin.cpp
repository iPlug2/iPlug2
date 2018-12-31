/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/


#include <Shlobj.h>
#include <Shlwapi.h>
#include <commctrl.h>

#include "IPlugParameter.h"
#include "IGraphicsWin.h"
#include "IControl.h"
#include "IPopupMenuControl.h"

#include <wininet.h>

#pragma warning(disable:4244) // Pointer size cast mismatch.
#pragma warning(disable:4312) // Pointer size cast mismatch.
#pragma warning(disable:4311) // Pointer size cast mismatch.

static int nWndClassReg = 0;
static const char* wndClassName = "IPlugWndClass";
static double sFPS = 0.0;

#define PARAM_EDIT_ID 99
#define IPLUG_TIMER_ID 2
#define IPLUG_WIN_MAX_WIDE_PATH 4096

// Unicode helpers


void UTF8ToUTF16(wchar_t* utf16Str, const char* utf8Str, int maxLen)
{
  int requiredSize = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, NULL, 0);

  if (requiredSize > 0 && requiredSize <= maxLen)
  {
    MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, utf16Str, requiredSize);
    return;
  }

  utf16Str[0] = 0;
}

void UTF16ToUTF8(WDL_String& utf8Str, const wchar_t* utf16Str)
{
  int requiredSize = WideCharToMultiByte(CP_UTF8, 0, utf16Str, -1, NULL, 0, NULL, NULL);

  if (requiredSize > 0 && utf8Str.SetLen(requiredSize))
  {
    WideCharToMultiByte(CP_UTF8, 0, utf16Str, -1, utf8Str.Get(), requiredSize, NULL, NULL);
    return;
  }

  utf8Str.Set("");
}

// Helper for getting a known folder in UTF8

void GetKnownFolder(WDL_String &path, int identifier, int flags = 0)
{
  wchar_t wideBuffer[1024];

  SHGetFolderPathW(NULL, identifier, NULL, flags, wideBuffer);
  UTF16ToUTF8(path, wideBuffer);
}

inline IMouseInfo IGraphicsWin::GetMouseInfo(LPARAM lParam, WPARAM wParam)
{
  IMouseInfo info;
  info.x = mCursorX = GET_X_LPARAM(lParam) / GetDrawScale();
  info.y = mCursorY = GET_Y_LPARAM(lParam) / GetDrawScale();
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

              const IParam* pParam = pGraphics->mEdControl->GetParam();
              
              if(pParam)
              {
                if (pParam->Type() == IParam::kTypeEnum || pParam->Type() == IParam::kTypeBool)
                {
                  double vi = 0.;
                  pParam->MapDisplayText(txt, &vi);
                  v = (double) vi;
                }
                else
                {
                  v = atof(txt);
                  if (pParam->GetNegateDisplay())
                  {
                    v = -v;
                  }
                }
                pGraphics->mEdControl->SetValueFromUserInput(pParam->ToNormalized(v));
              }
              else
              {
                pGraphics->mEdControl->OnTextEntryCompletion(txt);
              }
              // Fall through.
            }
            case kCancel:
            {
              SetWindowLongPtr(pGraphics->mParamEditWnd, GWLP_WNDPROC, (LPARAM) pGraphics->mDefEditProc);
              DestroyWindow(pGraphics->mParamEditWnd);
              pGraphics->mParamEditWnd = nullptr;
              pGraphics->mEdControl = nullptr;
              pGraphics->mDefEditProc = nullptr;
            }
            break;
          }
          pGraphics->mParamEditMsg = kNone;
          return 0; // TODO: check this!
        }

        IRECTList rects;
         
        if (pGraphics->IsDirty(rects))
        {
          pGraphics->SetAllControlsClean();
          IRECT dirtyR = rects.Bounds();
          dirtyR.Scale(pGraphics->GetDrawScale());
          dirtyR.PixelAlign();
          RECT r = { (LONG) dirtyR.L, (LONG) dirtyR.T, (LONG) dirtyR.R, (LONG) dirtyR.B };

          InvalidateRect(hWnd, &r, FALSE);

          if (pGraphics->mParamEditWnd)
          {
            IRECT notDirtyR = pGraphics->mEdControl->GetRECT();
            notDirtyR.Scale(pGraphics->GetDrawScale());
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
      else if (GetCapture() == hWnd && !pGraphics->mParamEditWnd)
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
        RECT r;
        GetWindowRect(hWnd, &r);
        pGraphics->OnMouseWheel(info.x - r.left, info.y - r.top, info.ms, d);
        return 0;
      }
    }
    case WM_GETDLGCODE:
      return DLGC_WANTALLKEYS;
    case WM_KEYDOWN:
    {
      POINT p;
      GetCursorPos(&p);
      ScreenToClient(hWnd, &p);

      BYTE keyboardState[256];
      GetKeyboardState(keyboardState);
      const int keyboardScanCode = (lParam >> 16) & 0x00ff;
      WORD ascii = 0;
      const int len = ToAscii(wParam, keyboardScanCode, keyboardState, &ascii, 0);

      bool handle = false;

      if (len == 1)
      {
        IKeyPress keyPress{ static_cast<char>(ascii), static_cast<int>(wParam), static_cast<bool>(GetKeyState(VK_SHIFT) & 0x8000),
                                                                                static_cast<bool>(GetKeyState(VK_CONTROL) & 0x8000),
                                                                                static_cast<bool>(GetKeyState(VK_MENU) & 0x8000) };

        handle = pGraphics->OnKeyDown(p.x, p.y, keyPress);
      }

      if (!handle)
      {
        HWND rootHWnd = GetAncestor( hWnd, GA_ROOT);
        SendMessage(rootHWnd, WM_KEYDOWN, wParam, lParam);
        return DefWindowProc(hWnd, msg, wParam, lParam);
      }
      else
        return 0;
    }
    case WM_KEYUP:
    {
      HWND rootHWnd = GetAncestor(hWnd, GA_ROOT);
      SendMessage(rootHWnd, msg, wParam, lParam);
      return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    case WM_PAINT:
    {
      RECT r;
      if (GetUpdateRect(hWnd, &r, FALSE))
      {
        #ifdef IGRAPHICS_NANOVG
        PAINTSTRUCT ps;
        BeginPaint(hWnd, &ps);
        #endif
        IRECT ir(r.left, r.top, r.right, r.bottom);
        IRECTList rects;
        ir.Scale(1. / pGraphics->GetDrawScale());
        ir.PixelAlign();
        rects.Add(ir);
        pGraphics->Draw(rects);
        #ifdef IGRAPHICS_NANOVG
        SwapBuffers((HDC)pGraphics->mPlatformContext);
        EndPaint(hWnd, &ps);
        #endif
      }
      return 0;
    }

    case WM_CTLCOLOREDIT:
    {
      if(!pGraphics->mEdControl)
        return 0;

      const IText& text = pGraphics->mEdControl->GetText();
      HDC dc = (HDC) wParam;
      SetBkColor(dc, RGB(text.mTextEntryBGColor.R, text.mTextEntryBGColor.G, text.mTextEntryBGColor.B));
      SetTextColor(dc, RGB(text.mTextEntryFGColor.R, text.mTextEntryFGColor.G, text.mTextEntryFGColor.B));
      SetBkMode(dc, OPAQUE);
      return (BOOL) GetStockObject(DC_BRUSH);
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
        const IParam* pParam = pGraphics->mEdControl->GetParam();
        // limit to numbers for text entry on appropriate parameters
        if(pParam)
        {
          char c = wParam;

          if(c == 0x08) break; // backspace

          switch ( pParam->Type() )
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
        if (pGraphics->mEdControl->GetParam()) break;
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
{}

IGraphicsWin::~IGraphicsWin()
{
  CloseWindow();
  FREE_NULL(mCustomColorStorage);
}

void GetWindowSize(HWND pWnd, int* pW, int* pH)
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

bool IsChildWindow(HWND pWnd)
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

#define SETPOS_FLAGS SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE

void IGraphicsWin::PlatformResize()
{
  if (WindowIsOpen())
  {
    HWND pParent = 0, pGrandparent = 0;
    int dlgW = 0, dlgH = 0, parentW = 0, parentH = 0, grandparentW = 0, grandparentH = 0;
    GetWindowSize(mPlugWnd, &dlgW, &dlgH);
    int dw = WindowWidth() - dlgW, dh = WindowHeight() - dlgH;
      
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

    SetWindowPos(mPlugWnd, 0, 0, 0, dlgW + dw, dlgH + dh, SETPOS_FLAGS);

    // don't want to touch the host window in VST3
#ifndef VST3_API
    if(pParent)
    {
      SetWindowPos(pParent, 0, 0, 0, parentW + dw, parentH + dh, SETPOS_FLAGS);
    }

    if(pGrandparent)
    {
      SetWindowPos(pGrandparent, 0, 0, 0, grandparentW + dw, grandparentH + dh, SETPOS_FLAGS);
    }
#endif

    RECT r = { 0, 0, WindowWidth(), WindowHeight() };
    InvalidateRect(mPlugWnd, &r, FALSE);
  }
}

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

void IGraphicsWin::SetMouseCursor(ECursor cursor)
{
  HCURSOR cursorType;
    
  switch (cursor)
  {
    case ECursor::ARROW:            cursorType = LoadCursor(NULL, IDC_ARROW);           break;
    case ECursor::IBEAM:            cursorType = LoadCursor(NULL, IDC_IBEAM);           break;
    case ECursor::WAIT:             cursorType = LoadCursor(NULL, IDC_WAIT);            break;
    case ECursor::CROSS:            cursorType = LoadCursor(NULL, IDC_CROSS);           break;
    case ECursor::UPARROW:          cursorType = LoadCursor(NULL, IDC_UPARROW);         break;
    case ECursor::SIZENWSE:         cursorType = LoadCursor(NULL, IDC_SIZENWSE);        break;
    case ECursor::SIZENESW:         cursorType = LoadCursor(NULL, IDC_SIZENESW);        break;
    case ECursor::SIZEWE:           cursorType = LoadCursor(NULL, IDC_SIZEWE);          break;
    case ECursor::SIZENS:           cursorType = LoadCursor(NULL, IDC_SIZENS);          break;
    case ECursor::SIZEALL:          cursorType = LoadCursor(NULL, IDC_SIZEALL);         break;
    case ECursor::INO:              cursorType = LoadCursor(NULL, IDC_NO);              break;
    case ECursor::HAND:             cursorType = LoadCursor(NULL, IDC_HAND);            break;
    case ECursor::APPSTARTING:      cursorType = LoadCursor(NULL, IDC_APPSTARTING);     break;
    case ECursor::HELP:             cursorType = LoadCursor(NULL, IDC_HELP);            break;
    default:
      cursorType = LoadCursor(NULL, IDC_ARROW);
  }

  SetCursor(cursorType);
}

bool IGraphicsWin::MouseCursorIsLocked()
{
  return mCursorLock;
}

int IGraphicsWin::ShowMessageBox(const char* text, const char* caption, EMessageBoxType type)
{
  ReleaseMouseCapture();
  return MessageBox(GetMainWnd(), text, caption, (int) type);
}

void* IGraphicsWin::OpenWindow(void* pParent)
{
  int x = 0, y = 0, w = WindowWidth(), h = WindowHeight();
  mParentWnd = (HWND) pParent;

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
    WNDCLASS wndClass = { CS_DBLCLKS | CS_OWNDC, WndProc, 0, 0, mHInstance, 0, LoadCursor(NULL, IDC_ARROW), 0, 0, wndClassName };
    RegisterClass(&wndClass);
  }

  sFPS = FPS();
  mPlugWnd = CreateWindow(wndClassName, "IPlug", WS_CHILD | WS_VISIBLE, x, y, w, h, mParentWnd, 0, mHInstance, this);

  HDC dc = GetDC(mPlugWnd);
  SetPlatformContext(dc);
  ReleaseDC(mPlugWnd, dc);

  OnViewInitialized((void*) dc);
  
  SetScreenScale(1); // CHECK!

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

  return mPlugWnd;
}

void GetWndClassName(HWND hWnd, WDL_String* pStr)
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

IPopupMenu* IGraphicsWin::GetItemMenu(long idx, long& idxInMenu, long& offsetIdx, const IPopupMenu& baseMenu)
{
  long oldIDx = offsetIdx;
  offsetIdx += baseMenu.NItems();

  if (idx < offsetIdx)
  {
    idxInMenu = idx - oldIDx;
    return &const_cast<IPopupMenu&>(baseMenu);
  }

  IPopupMenu* pMenu = nullptr;

  for(int i = 0; i< baseMenu.NItems(); i++)
  {
    IPopupMenu::Item* pMenuItem = const_cast<IPopupMenu&>(baseMenu).GetItem(i);
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

  for(int i = 0; i< nItems; i++)
  {
    IPopupMenu::Item* pMenuItem = const_cast<IPopupMenu&>(menu).GetItem(i);

    if (pMenuItem->GetIsSeparator())
    {
      AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
    }
    else
    {
      const char* str = pMenuItem->GetText();
      char* titleWithPrefixNumbers = 0;

      if (menu.GetPrefix())
      {
        titleWithPrefixNumbers = (char*)malloc(strlen(str) + 50);

        switch (menu.GetPrefix())
        {
          case 1:
          {
            sprintf(titleWithPrefixNumbers, "%1d: %s", i+1, str); break;
          }
          case 2:
          {
            sprintf(titleWithPrefixNumbers, "%02d: %s", i+1, str); break;
          }
          case 3:
          {
            sprintf(titleWithPrefixNumbers, "%03d: %s", i+1, str); break;
          }
        }
      }

      const char* entryText(titleWithPrefixNumbers ? titleWithPrefixNumbers : str);

      // Escape ampersands if present

      if (strchr(entryText, '&'))
      {
        escapedText = WDL_String(entryText);

        for (int c = 0; c < escapedText.GetLength(); c++)
          if (escapedText.Get()[c] == '&')
            escapedText.Insert("&", c++);

         entryText = escapedText.Get();
      }

      flags = MF_STRING;
      //if (nItems < 160 && pMenu->getNbItemsPerColumn () > 0 && inc && !(inc % _menu->getNbItemsPerColumn ()))
      //  flags |= MF_MENUBARBREAK;

      if (pMenuItem->GetSubmenu())
      {
        HMENU submenu = CreateMenu(*pMenuItem->GetSubmenu(), pOffsetIdx);
        if (submenu)
        {
          AppendMenu(hMenu, flags|MF_POPUP|MF_ENABLED, (UINT_PTR)submenu, (const TCHAR*)entryText);
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

        AppendMenu(hMenu, flags, offset + inc, entryText);
      }

      if(titleWithPrefixNumbers)
        FREE_NULL(titleWithPrefixNumbers);
    }
    inc++;
  }

  return hMenu;
}

IPopupMenu* IGraphicsWin::CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT& bounds, IControl* pCaller)
{
  long offsetIdx = 0;
  HMENU hMenu = CreateMenu(menu, &offsetIdx);
  IPopupMenu* result = nullptr;

  if(hMenu)
  {
    long offsetIdx = 0;
    HMENU hMenu = CreateMenu(menu, &offsetIdx);
    IPopupMenu* result = nullptr;

    if (hMenu)
    {
      POINT cPos;

      cPos.x = bounds.L * GetDrawScale();
      cPos.y = bounds.B * GetDrawScale();

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
              IPopupMenu* resultMenu = GetItemMenu(res, idx, offsetIdx, menu);
              if (resultMenu)
              {
                result = resultMenu;
                result->SetChosenItemIdx(idx);
              }
            }
          }
        }
      }
      DestroyMenu(hMenu);

      RECT r = { 0, 0, WindowWidth(), WindowHeight() };
      InvalidateRect(mPlugWnd, &r, FALSE);
    }

    if (pCaller)
      pCaller->OnPopupMenuSelection(result);

    return result;
  }

  return nullptr;
}

void IGraphicsWin::CreatePlatformTextEntry(IControl& control, const IText& text, const IRECT& bounds, const char* str)
{
  if (mParamEditWnd)
    return;

  DWORD editStyle;

  switch ( text.mAlign )
  {
    case IText::kAlignNear:   editStyle = ES_LEFT;   break;
    case IText::kAlignFar:    editStyle = ES_RIGHT;  break;
    case IText::kAlignCenter:
    default:                  editStyle = ES_CENTER; break;
  }

  IRECT scaledBounds = bounds.GetScaled(GetDrawScale());

  mParamEditWnd = CreateWindow("EDIT", str, ES_AUTOHSCROLL /*only works for left aligned text*/ | WS_CHILD | WS_VISIBLE | ES_MULTILINE | editStyle,
    scaledBounds.L, scaledBounds.T, scaledBounds.W()+1, scaledBounds.H()+1,
    mPlugWnd, (HMENU) PARAM_EDIT_ID, mHInstance, 0);

  HFONT font = CreateFont(text.mSize, 0, 0, 0, text.mStyle == IText::kStyleBold ? FW_BOLD : 0, text.mStyle == IText::kStyleItalic ? TRUE : 0, 0, 0, 0, 0, 0, 0, 0, text.mFont);

  SendMessage(mParamEditWnd, EM_LIMITTEXT, (WPARAM) control.GetTextEntryLength(), 0);
  SendMessage(mParamEditWnd, WM_SETFONT, (WPARAM) font, 0);
  SendMessage(mParamEditWnd, EM_SETSEL, 0, -1);

  SetFocus(mParamEditWnd);

  mDefEditProc = (WNDPROC) SetWindowLongPtr(mParamEditWnd, GWLP_WNDPROC, (LONG_PTR) ParamEditProc);
  SetWindowLongPtr(mParamEditWnd, GWLP_USERDATA, 0xdeadf00b);

  //DeleteObject(font);

  mEdControl = &control;
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

//TODO: this method needs rewriting
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
    case kFileSave:
      ofn.Flags |= OFN_OVERWRITEPROMPT;
      rc = GetSaveFileNameW(&ofn);
      break;
            
    case kFileOpen:
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

UINT_PTR CALLBACK CCHookProc(HWND hdlg, UINT uiMsg, WPARAM wParam, LPARAM lParam)
{
  if (uiMsg == WM_INITDIALOG && lParam)
  {
    CHOOSECOLOR* cc = (CHOOSECOLOR*) lParam;
    if (cc && cc->lCustData)
    {
      char* str = (char*) cc->lCustData;
      SetWindowText(hdlg, str);
    }
  }
  return 0;
}

bool IGraphicsWin::PromptForColor(IColor& color, const char* prompt)
{
  if (!mPlugWnd)
  {
    return false;
  }
  if (!mCustomColorStorage)
  {
    mCustomColorStorage = (COLORREF*) calloc(16, sizeof(COLORREF));
  }
  CHOOSECOLOR cc;
  memset(&cc, 0, sizeof(CHOOSECOLOR));
  cc.lStructSize = sizeof(CHOOSECOLOR);
  cc.hwndOwner = mPlugWnd;
  cc.rgbResult = RGB(color.R, color.G, color.B);
  cc.lpCustColors = mCustomColorStorage;
  cc.lCustData = (LPARAM) prompt;
  cc.lpfnHook = CCHookProc;
  cc.Flags = CC_RGBINIT | CC_ANYCOLOR | CC_FULLOPEN | CC_SOLIDCOLOR | CC_ENABLEHOOK;

  if (ChooseColor(&cc))
  {
    color.R = GetRValue(cc.rgbResult);
    color.G = GetGValue(cc.rgbResult);
    color.B = GetBValue(cc.rgbResult);
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
  bool success = false;
  HGLOBAL hglb;
  
  if (IsClipboardFormatAvailable(CF_UNICODETEXT))
  {
    if(OpenClipboard(0))
    {
      hglb = GetClipboardData(CF_UNICODETEXT);
      
      if(hglb != NULL)
      {
        WCHAR *orig_str = (WCHAR*)GlobalLock(hglb);
        
        if (orig_str != NULL)
        {
          int orig_len = (int) wcslen(orig_str);
          
          orig_len += 1;
          
          // find out how much space is needed
          int new_len = WideCharToMultiByte(CP_UTF8,
                                            0,
                                            orig_str,
                                            orig_len,
                                            0,
                                            0,
                                            NULL,
                                            NULL);
          
          if (new_len > 0)
          {
            char *new_str = new char[new_len + 1];
            
            int num_chars = WideCharToMultiByte(CP_UTF8,
                                                0,
                                                orig_str,
                                                orig_len,
                                                new_str,
                                                new_len,
                                                NULL,
                                                NULL);
            
            if (num_chars > 0)
            {
              success = true;
              str.Set(new_str);
            }
            
            delete [] new_str;
          }
          
          GlobalUnlock(hglb);
        }
      }
    }
    
    CloseClipboard();
  }
  
  if(!success)
    str.Set("");
  
  return success;
}

BOOL IGraphicsWin::EnumResNameProc(HANDLE module, LPCTSTR type, LPTSTR name, LONG_PTR param)
{
  if (IS_INTRESOURCE(name)) return true; // integer resources not wanted
  else {
    WDL_String* search = (WDL_String*) param;
    if (search != 0 && name != 0)
    {
      //strip off extra quotes
      WDL_String strippedName(strlwr(name+1)); 
      strippedName.SetLen(strippedName.GetLength() - 1);

      if (strcmp(strlwr(search->Get()), strippedName.Get()) == 0) // if we are looking for a resource with this name
      {
        search->SetFormatted(strippedName.GetLength() + 7, "found: %s", strippedName.Get());
        return false;
      }
    }
  }

  return true; // keep enumerating
}

EResourceLocation IGraphicsWin::OSFindResource(const char* name, const char* type, WDL_String& result)
{
  if (CStringHasContents(name))
  {
    WDL_String search(name);
    WDL_String typeUpper(type);

    EnumResourceNames(mHInstance, _strupr(typeUpper.Get()), (ENUMRESNAMEPROC)EnumResNameProc, (LONG_PTR)&search);

    if (strstr(search.Get(), "found: ") != 0)
    {
      result.SetFormatted(MAX_PATH, "\"%s\"", search.Get() + 7, search.GetLength() - 7); // 7 = strlen("found: ")
      return EResourceLocation::kWinBinary;
    }
    else
    {
      if (PathFileExists(name))
      {
        result.Set(name);
        return EResourceLocation::kAbsolutePath;
      }
    }
  }
  return EResourceLocation::kNotFound;
}

const void* IGraphicsWin::LoadWinResource(const char* resid, const char* type, int& sizeInBytes)
{
  WDL_String typeUpper(type);

  HRSRC hResource = FindResource(mHInstance, resid, _strupr(typeUpper.Get()));

  if (!hResource)
    return NULL;

  DWORD size = SizeofResource(mHInstance, hResource);

  if (size < 8)
    return NULL;

  HGLOBAL res = LoadResource(mHInstance, hResource);

  const void* pResourceData = LockResource(res);

  if (!pResourceData)
  {
    sizeInBytes = 0;
    return NULL;
  }
  else
  {
    sizeInBytes = size;
    return pResourceData;
  }
}

//TODO: THIS IS TEMPORARY, TO EASE DEVELOPMENT
#ifndef NO_IGRAPHICS
#if defined IGRAPHICS_AGG
  #include "IGraphicsAGG.cpp"
  #include "agg_win32_pmap.cpp"
  #include "agg_win32_font.cpp"
#elif defined IGRAPHICS_CAIRO
  #include "IGraphicsCairo.cpp"
#elif defined IGRAPHICS_LICE
  #include "IGraphicsLice.cpp"
#elif defined IGRAPHICS_NANOVG
  #include "IGraphicsNanoVG.cpp"
#ifdef IGRAPHICS_FREETYPE
#define FONS_USE_FREETYPE
#endif
  #include "nanovg.c"
  #include "glad.c"
#else
  #include "IGraphicsCairo.cpp"
#endif
#endif
