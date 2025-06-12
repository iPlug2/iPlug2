/*
 * JSFX
 * Copyright (C) 2004 and onwards, Cockos Incorporated
 * License: LGPL
 *
 * Standalone controls similar to REAPER's slider/fader controls
 */

#ifdef _WIN32
#include <windows.h>
#include <commctrl.h>
#else

#ifdef __APPLE__
#import <Carbon/Carbon.h>
#endif

#endif

#ifdef _WIN32
#include <windowsx.h>
#endif
#include "../../WDL/wingui/membitmap.h"
#include "../../WDL/db2val.h"
#include "../../WDL/wdlcstring.h"

#include "../sfxui.h"

#include <math.h>

#define Is_Key_Down(x) ((GetAsyncKeyState(x)&0x8000)!=0)

const char *g_config_slider_classname = "jsfx_slider";

static struct
{
  HBITMAP fader_bitmap_v,fader_bitmap_h;
}
standalone_icontheme;

#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x20A
#endif
#ifdef _WIN32
static UINT Scroll_Message;
#endif

static HBITMAP STYLE_GetSliderBitmap(bool isVert, int *w, int *h);


// this uses 2x the ram because on win64 the extra space is needed. We could ifdef the space in but f it.
#define OFFSET_FLAGS 0 // &1=ignore automation, &2=no center mark/snap
#define OFFSET_VALUE 8
#define OFFSET_CENTERPOS 16
#define OFFSET_VUMETERS 24
#define OFFSET_RANGE 32
#define OFFSET_ISVERT 40
#define EXTRA_SIZE 48

// these are defaults
#define H_SLIDER_W 23
#define H_SLIDER_H 14
#define V_SLIDER_W 14
#define V_SLIDER_H 26




#define CalcRanges()         \
  LPARAM RangeWord=(LPARAM)GetWindowLong(hwnd,OFFSET_RANGE); \
  int minRange=LOWORD(RangeWord); \
  int maxRange=HIWORD(RangeWord); \
  if (minRange >= maxRange) maxRange=minRange+1;


static void RefreshFader(HWND hwnd)
{
#ifdef TRANSPARENT_POO
  if (GetWindowLong(hwnd,GWL_EXSTYLE)&WS_EX_TRANSPARENT)
  {
    HWND par=GetParent(hwnd);
    RECT br;
    GetWindowRect(hwnd,&br);
    ScreenToClient(par,(LPPOINT)&br);
    ScreenToClient(par,((LPPOINT)&br)+1);
    InvalidateRect(par,&br,FALSE);

  }
  else
#endif //TRANSPARENT_POO
    InvalidateRect(hwnd,NULL,FALSE);
}


static void DrawBackground(HWND hwnd, WDL_WinMemBitmap *bm, RECT *r)
{
  HWND par=GetParent(hwnd);

#ifdef TRANSPARENT_POO
  if (GetWindowLong(hwnd,GWL_EXSTYLE)&WS_EX_TRANSPARENT)
  {
    HDC hdc=GetWindowDC(hwnd);
    BitBlt(bm->GetDC(),r->left,r->top,r->right-r->left,r->bottom-r->top,hdc,r->left,r->top,SRCCOPY);
    ReleaseDC(hwnd,hdc);
    return;
  }
#endif

  HBRUSH bg=(HBRUSH)SendMessage(par,WM_CTLCOLORDLG,(WPARAM)bm->GetDC(),(LPARAM)par);
  if (bg) FillRect(bm->GetDC(),r,bg);
  else
  {
    bg=CreateSolidBrush(GetSysColor(COLOR_3DFACE));
    FillRect(bm->GetDC(),r,bg);
    DeleteObject(bg);
  }
}


static void DrawTransparentBitmap(HDC hdc, HBITMAP hBitmap, short xStart,
                           short yStart, COLORREF cTransparentColor)
{
#ifdef _WIN32
   BITMAP     bm;
   COLORREF   cColor;
   HBITMAP    bmAndBack, bmAndObject, bmAndMem, bmSave;
   HGDIOBJ    bmBackOld, bmObjectOld, bmMemOld, bmSaveOld;
   HDC        hdcMem, hdcBack, hdcObject, hdcTemp, hdcSave;
   POINT      ptSize;

   hdcTemp = CreateCompatibleDC(hdc);
   SelectObject(hdcTemp, hBitmap);   // Select the bitmap

   GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
   ptSize.x = bm.bmWidth;            // Get width of bitmap
   ptSize.y = bm.bmHeight;           // Get height of bitmap
   DPtoLP(hdcTemp, &ptSize, 1);      // Convert from device

                                     // to logical points

   // Create some DCs to hold temporary data.
   hdcBack   = CreateCompatibleDC(hdc);
   hdcObject = CreateCompatibleDC(hdc);
   hdcMem    = CreateCompatibleDC(hdc);
   hdcSave   = CreateCompatibleDC(hdc);

   // Create a bitmap for each DC. DCs are required for a number of
   // GDI functions.

   // Monochrome DC
   bmAndBack   = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

   // Monochrome DC
   bmAndObject = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);

   bmAndMem    = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);
   bmSave      = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);

   // Each DC must select a bitmap object to store pixel data.
   bmBackOld   = SelectObject(hdcBack, bmAndBack);
   bmObjectOld = SelectObject(hdcObject, bmAndObject);
   bmMemOld    = SelectObject(hdcMem, bmAndMem);
   bmSaveOld   = SelectObject(hdcSave, bmSave);

   // Set proper mapping mode.
   SetMapMode(hdcTemp, GetMapMode(hdc));

   // Save the bitmap sent here, because it will be overwritten.
   BitBlt(hdcSave, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);

   // Set the background color of the source DC to the color.
   // contained in the parts of the bitmap that should be transparent
   cColor = SetBkColor(hdcTemp, cTransparentColor);

   // Create the object mask for the bitmap by performing a BitBlt
   // from the source bitmap to a monochrome bitmap.
   BitBlt(hdcObject, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0,
          SRCCOPY);

   // Set the background color of the source DC back to the original
   // color.
   SetBkColor(hdcTemp, cColor);

   // Create the inverse of the object mask.
   BitBlt(hdcBack, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0,
          NOTSRCCOPY);

   // Copy the background of the main DC to the destination.
   BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdc, xStart, yStart,
          SRCCOPY);

   // Mask out the places where the bitmap will be placed.
   BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, SRCAND);

   // Mask out the transparent colored pixels on the bitmap.
   BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND);

   // XOR the bitmap with the background on the destination DC.
   BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCPAINT);

   // Copy the destination to the screen.
   BitBlt(hdc, xStart, yStart, ptSize.x, ptSize.y, hdcMem, 0, 0,
          SRCCOPY);

   // Place the original bitmap back into the bitmap sent here.
   BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcSave, 0, 0, SRCCOPY);

   // Delete the memory bitmaps.
   DeleteObject(SelectObject(hdcBack, bmBackOld));
   DeleteObject(SelectObject(hdcObject, bmObjectOld));
   DeleteObject(SelectObject(hdcMem, bmMemOld));
   DeleteObject(SelectObject(hdcSave, bmSaveOld));

   // Delete the memory DCs.
   DeleteDC(hdcMem);
   DeleteDC(hdcBack);
   DeleteDC(hdcObject);
   DeleteDC(hdcSave);
   DeleteDC(hdcTemp);
#else
   BITMAP bm;
   GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
   RECT r={xStart,yStart, xStart+bm.bmWidth, yStart+bm.bmHeight};

   DrawImageInRect(hdc,(HICON)hBitmap,&r);
#endif
}

#define GetHSliderPositionPixels() (((GetWindowLong(hwnd,OFFSET_VALUE)-minRange) * (r.right-bm_w))/(maxRange-minRange))

static LRESULT CALLBACK HorzSliderProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  static WDL_WinMemBitmap m_bm1;
  static int m_click_pos;
  static int m_move_offset,m_last_x;

#ifdef _WIN32
  if (Scroll_Message && msg == Scroll_Message)
  {
    msg=WM_MOUSEWHEEL;
    wParam<<=16;
  }
#endif

  switch (msg)
  {
    case WM_CREATE:
      SetWindowLong(hwnd,OFFSET_CENTERPOS,-1);
      SetWindowLong(hwnd,OFFSET_RANGE,MAKELPARAM(0,1000));
    return 0;
    case TBM_SETRANGE:
      SetWindowLong(hwnd,OFFSET_RANGE,lParam);
    return 0;
    case TBM_SETTIC:
      SetWindowLong(hwnd,OFFSET_CENTERPOS,lParam);
    return 0;
    case WM_RBUTTONUP:
      SendMessage(GetParent(hwnd),WM_COMMAND,GetWindowLong(hwnd,GWL_ID),0);
    return 0;
    case WM_MOUSEWHEEL:

      {
        CalcRanges();

        int l=((short)HIWORD(wParam));
        if (!Is_Key_Down(VK_CONTROL)) l *= 16;
        l *= (maxRange-minRange);
        l/=120000;
        if (!l) { if (((short)HIWORD(wParam))<0)l=-1; else if (((short)HIWORD(wParam))>0) l=1; }

        int pos=(GetWindowLong(hwnd,OFFSET_VALUE)+l);
        if (pos < minRange)pos=minRange;
        else if (pos > maxRange)pos=maxRange;

        SetWindowLong(hwnd,OFFSET_VALUE,pos);
        RefreshFader(hwnd);

        SendMessage(GetParent(hwnd),WM_HSCROLL,SB_THUMBTRACK,(LPARAM)hwnd);

        KillTimer(hwnd,32);
        SetTimer(hwnd,32,500,NULL);
      }
    return -1;
#ifdef _WIN32
    case WM_GETDLGCODE:
      return DLGC_WANTARROWS;
#endif
    case WM_KEYDOWN:
      if (wParam == VK_UP || wParam == VK_DOWN)
      {
        SendMessage(hwnd,WM_MOUSEWHEEL,wParam == VK_UP ? (120<<16) : (-120<<16),0);
        return 0;
      }
    break;
    // todo: handle up/down arrows, handle mouse scroll messages
    case WM_TIMER:
      if (wParam == 32)
      {
        KillTimer(hwnd,wParam);
        SendMessage(GetParent(hwnd),WM_HSCROLL,SB_ENDSCROLL,(LPARAM)hwnd);
      }
    return 0;

    // todo: handle up/down arrows, handle mouse scroll messages
    case TBM_GETPOS:
      return (GetWindowLong(hwnd,OFFSET_VALUE));
    case TBM_SETPOS:
      if (GetCapture()==hwnd)
      {
    //    if (lParam != (GetWindowLong(hwnd,OFFSET_VALUE)))
      //    SendMessage(GetParent(hwnd),WM_HSCROLL,SB_THUMBTRACK,(LPARAM)hwnd);
      }
      else
      {
        CalcRanges();

        if (lParam<minRange)lParam=minRange;
        else if (lParam>maxRange)lParam=maxRange;

        if (SetWindowLong(hwnd,OFFSET_VALUE,lParam) != lParam)
          RefreshFader(hwnd);
      }
    return 0;
    case WM_LBUTTONUP:
    case WM_MOUSEMOVE:

      if (GetCapture()==hwnd)
      {
        KillTimer(hwnd,32);
        RECT r;
        GetClientRect(hwnd,&r);

        CalcRanges();

        int bm_w,bm_h;
        STYLE_GetSliderBitmap(false,&bm_w,&bm_h);
        int pos;
        if (Is_Key_Down(VK_CONTROL))
        {
          pos = GetWindowLong(hwnd,OFFSET_VALUE)+GET_X_LPARAM(lParam)-m_last_x;
        }
        else
        {
          pos=minRange + ((GET_X_LPARAM(lParam)-m_move_offset)*(maxRange-minRange))/(r.right==bm_w?1:r.right-bm_w);
        }


        if (pos < minRange)pos=minRange;
        else if (pos > maxRange)pos=maxRange;
        if (pos != (GetWindowLong(hwnd,OFFSET_VALUE))|| msg == WM_LBUTTONUP)
        {
          if (GET_X_LPARAM(lParam) == m_last_x)
            pos=GetWindowLong(hwnd,OFFSET_VALUE);

          if (Is_Key_Down(VK_MENU) && msg == WM_LBUTTONUP)
            pos=m_click_pos;
          SetWindowLong(hwnd,OFFSET_VALUE,pos);

          SendMessage(GetParent(hwnd),WM_HSCROLL,msg==WM_LBUTTONUP?SB_ENDSCROLL:SB_THUMBTRACK,(LPARAM)hwnd);
          RefreshFader(hwnd);
        }
        m_last_x=GET_X_LPARAM(lParam);
        if (msg == WM_LBUTTONUP) ReleaseCapture();
      }
    return 0;
    case WM_LBUTTONDBLCLK:
      ReleaseCapture();
      {
        int center=GetWindowLong(hwnd,OFFSET_CENTERPOS);
        SetWindowLong(hwnd,OFFSET_VALUE,center);
      }
      SendMessage(GetParent(hwnd),WM_HSCROLL,SB_ENDSCROLL,(LPARAM)hwnd);
      RefreshFader(hwnd);
    return 0;
    case WM_LBUTTONDOWN:
      {
        RECT r;
        GetClientRect(hwnd,&r);
        CalcRanges();
        int bm_w,bm_h;
        STYLE_GetSliderBitmap(false,&bm_w,&bm_h);

        m_move_offset=GET_X_LPARAM(lParam)-GetHSliderPositionPixels();
        m_click_pos=GetWindowLong(hwnd,OFFSET_VALUE);
        m_last_x=GET_X_LPARAM(lParam);
        if (m_move_offset < 0 || m_move_offset >= bm_w)
        {
          int ycent=GET_Y_LPARAM(lParam) - (r.bottom-r.top)/2;
          if (ycent >= -2 && ycent < 3 && GET_X_LPARAM(lParam) >= r.left+bm_w/3 && GET_X_LPARAM(lParam) <= r.right-bm_w/3)
          {
            m_move_offset=bm_w/2;

            int pos=minRange+((GET_X_LPARAM(lParam)-m_move_offset)*(maxRange-minRange))/(r.right==bm_h?1:r.right-bm_h);
            if (pos < minRange)pos=minRange;
            else if (pos > maxRange)pos=maxRange;
            SetWindowLong(hwnd,OFFSET_VALUE,pos);
            RefreshFader(hwnd);
          }
          else
            return SendMessage(GetParent(hwnd),msg,wParam,lParam);
        }
        SetFocus(hwnd);
        SetCapture(hwnd);
      }
    return 0;
    case WM_PAINT:
      {
        PAINTSTRUCT ps;
        if (BeginPaint(hwnd,&ps))
        {

          CalcRanges();

          int bm_w,bm_h;
          HBITMAP bm=STYLE_GetSliderBitmap(false,&bm_w,&bm_h);

          RECT r;
          GetClientRect(hwnd,&r);
          if (m_bm1.GetW() < r.right || m_bm1.GetH() < r.bottom)
          {
            m_bm1.DoSize(ps.hdc,max(m_bm1.GetW(),r.right),max(m_bm1.GetH(),r.bottom));
          }
          int pos = GetHSliderPositionPixels();


          DrawBackground(hwnd,&m_bm1,&r);

          {

            if (!(GetWindowLong(hwnd,OFFSET_FLAGS)&2))
            {
              int center=GetWindowLong(hwnd,OFFSET_CENTERPOS);
              int x=((center-minRange) * (r.right-bm_w))/(maxRange-minRange) + bm_w/2;
              HPEN pen=CreatePen(PS_SOLID,0,GetTextColor(m_bm1.GetDC()));
              HGDIOBJ oldPen=SelectObject(m_bm1.GetDC(),pen);

              MoveToEx(m_bm1.GetDC(),x,2,NULL);
              LineTo(m_bm1.GetDC(),x,r.bottom-2);


              SelectObject(m_bm1.GetDC(),oldPen);

              DeleteObject(pen);
            }

            int brcol=GetSysColor(COLOR_3DSHADOW);

            if (!(GetWindowLong(hwnd,OFFSET_FLAGS)&1))
            {
              SendMessage(GetParent(hwnd),WM_USER+5040,(WPARAM)hwnd,(LPARAM)&brcol);
            }


            HPEN pen=CreatePen(PS_SOLID,0,GetSysColor(COLOR_3DHILIGHT));
            HGDIOBJ oldPen=SelectObject(m_bm1.GetDC(),pen);
            HBRUSH br=CreateSolidBrush(brcol);
            HGDIOBJ oldBr=SelectObject(m_bm1.GetDC(),br);


            RECT r2=r;
            r2.top = (r.bottom-r.top - 4)/2;
            // white with black border, mmm
            RoundRect(m_bm1.GetDC(),r2.left+bm_w/3,r2.top,r2.right - bm_w/3,r2.top+5,2,2);

            SelectObject(m_bm1.GetDC(),oldPen);
            SelectObject(m_bm1.GetDC(),oldBr);
            DeleteObject(pen);
            DeleteObject(br);
          }


          DrawTransparentBitmap(m_bm1.GetDC(),bm,pos,(r.bottom-bm_h)/2,RGB(255,0,255));

          BitBlt(ps.hdc,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right-ps.rcPaint.left,ps.rcPaint.bottom-ps.rcPaint.top,m_bm1.GetDC(),ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);

          EndPaint(hwnd,&ps);
        }
        return 0;
      }

  }
  return DefWindowProc(hwnd,msg,wParam,lParam);
}

#ifndef _WIN32

static HWND sliderControlCreator(HWND parent, const char *cname, int idx, const char *classname, int style, int x, int y, int w, int h)
{
  HWND hw=0;
  if (!strcmp(classname,"jsfx_slider"))
  {
    hw=CreateDialog(NULL,0,parent,(DLGPROC)VertSliderProc);
  }

  if (hw)
  {
    SetWindowLong(hw,GWL_ID,idx);
    SetWindowPos(hw,HWND_TOP,x,y,w,h,SWP_NOZORDER|SWP_NOACTIVATE);
    ShowWindow(hw,SW_SHOWNA);
    return hw;
  }

  return 0;
}
#endif

void Sliders_Init(HINSTANCE hInst, bool reg, int hslider_bitmap_id)
{
  if (!reg)
  {
#ifdef _WIN32
    UnregisterClass("jsfx_slider",hInst);
#else
    SWELL_UnregisterCustomControlCreator(sliderControlCreator);
#endif
    return;
  }
#ifdef _WIN32
  if (!standalone_icontheme.fader_bitmap_h && hslider_bitmap_id) 
    standalone_icontheme.fader_bitmap_h = (HBITMAP)LoadImage(hInst,MAKEINTRESOURCE(hslider_bitmap_id),
        IMAGE_BITMAP,0,0,LR_CREATEDIBSECTION);

  if (!standalone_icontheme.fader_bitmap_h) 
  {
    char buf[1024];
    GetModuleFileName(hInst,buf,sizeof(buf));
    WDL_remove_filepart(buf);
    lstrcatn(buf,"\\cockos_hslider.bmp",sizeof(buf));
    standalone_icontheme.fader_bitmap_h = (HBITMAP)LoadImage(NULL,buf,IMAGE_BITMAP,0,0,
        LR_CREATEDIBSECTION|LR_LOADFROMFILE);
  }
#else
    Dl_info inf={0,};
    dladdr((void *)VST_Standalone_Init,&inf);
    if (inf.dli_fname)
    {
      char buf[1024];
      lstrcpyn(buf,inf.dli_fname,sizeof(buf));
      WDL_remove_filepart(buf); // remove executable name
      WDL_remove_filepart(buf); // remove /MacOS
      lstrcatn(buf,"/Resources/cockos_hslider.bmp",sizeof(buf));
      if (!standalone_icontheme.fader_bitmap_h)
        standalone_icontheme.fader_bitmap_h = (HBITMAP)LoadNamedImage(buf,true);
    }

    SWELL_RegisterCustomControlCreator(sliderControlCreator);
#endif

#ifdef _WIN32
  Scroll_Message = RegisterWindowMessage("MSWHEEL_ROLLMSG");
  {
    WNDCLASS wc={CS_DBLCLKS|CS_HREDRAW|CS_VREDRAW,HorzSliderProc,};
    wc.cbWndExtra = EXTRA_SIZE;
    wc.lpszClassName="jsfx_slider";
    wc.hInstance=hInst;
    wc.hCursor=LoadCursor(NULL,IDC_ARROW);
    RegisterClass(&wc);
  }
#endif
}




HBITMAP STYLE_GetSliderBitmap(bool isVert, int *w, int *h)
{
  if (isVert)
  {
    *w=V_SLIDER_W;
    *h=V_SLIDER_H;
    if (standalone_icontheme.fader_bitmap_v)
    {
      BITMAP bm;
      GetObject(standalone_icontheme.fader_bitmap_v, sizeof(BITMAP), (LPSTR)&bm);
      if (bm.bmWidth) *w=bm.bmWidth;
      if (bm.bmHeight) *h=bm.bmHeight;
    }
    return standalone_icontheme.fader_bitmap_v;
  }
  else
  {
    *w=H_SLIDER_W;
    *h=H_SLIDER_H;
    if (standalone_icontheme.fader_bitmap_h)
    {
      BITMAP bm;
      GetObject(standalone_icontheme.fader_bitmap_h, sizeof(BITMAP), (LPSTR)&bm);
      if (bm.bmWidth) *w=bm.bmWidth;
      if (bm.bmHeight) *h=bm.bmHeight;
    }
    return standalone_icontheme.fader_bitmap_h;
  }
  return 0;

}
