/*
 * JSFX
 * Copyright (C) 2004 and onwards, Cockos Incorporated
 * License: LGPL
 *
 * Standalone controls similar to REAPER's meters
 */
#ifdef _WIN32
#include <windowsx.h>
#include <windows.h>
#endif

#include <math.h>
#include "../../WDL/wdlcstring.h"
#include "../../WDL/lice/lice.h"

const struct {
  int vu_dointerlace;
  int vu_bottom;
  int vu_mid;
  int vu_top;
  int vu_clip;
  int vu_intcol;
} g_ctheme = {
  1, // vu_dointerlace
  RGB(0,128,0), // vu_bottom
  RGB(192,255,96), // vu_mid
  RGB(255,128,64), // vu_top
  RGB(255,0,0), // vu_clip
  RGB(32,32,32),  // vu_intcol
};

#define VU_FONTFACE "Arial"

#define MAKERG(cp,cmp) int red,green,blue;  \
                if ((cp) cmp redPos) { red=GetRValue(g_ctheme.vu_clip); green=GetGValue(g_ctheme.vu_clip); blue=GetBValue(g_ctheme.vu_clip);  }  \
                else if (g_ctheme.vu_dointerlace && ((cp)&1)) { red=GetRValue(g_ctheme.vu_intcol); green=GetGValue(g_ctheme.vu_intcol); blue=GetBValue(g_ctheme.vu_intcol); } \
                else getColorForSc(sc,red,green,blue);


#define DIM(x) (x/4)+(x/8)

const int g_config_vumeter_segmentsize=0;

const int g_vu_minvol=-62;
const int g_vu_maxvol=6;
const int g_vu_clipall=1;
static int g_vu_needrefcnt;


static int GetMinVal(bool isGR=false)
{
  if (isGR) return 21;

  return (g_vu_minvol>-6?6:g_vu_minvol<-150?150:-g_vu_minvol);
}
static int GetMaxVal(bool isGR=false)
{
  if (isGR) return 3;

  int mv=-GetMinVal();
  return (g_vu_maxvol<mv+6?mv+6:g_vu_maxvol>60?60:g_vu_maxvol);
}

static void getColorForSc(int sc, int &red, int &green, int &blue)
{
#define VUCURVE_PT1 160
#define VUCURVE_PT2 190
  if (sc < VUCURVE_PT1)
  {
    if (sc<0)sc=0;
    int isc=VUCURVE_PT1-1-sc;
    red=(GetRValue(g_ctheme.vu_bottom)*isc + GetRValue(g_ctheme.vu_mid)*sc)/VUCURVE_PT1;
    green=(GetGValue(g_ctheme.vu_bottom)*isc + GetGValue(g_ctheme.vu_mid)*sc)/VUCURVE_PT1;
    blue=(GetBValue(g_ctheme.vu_bottom)*isc + GetBValue(g_ctheme.vu_mid)*sc)/VUCURVE_PT1;
  }
  else
  {
    if (sc > VUCURVE_PT2)
    {
      sc=VUCURVE_PT2+(sc-VUCURVE_PT2)*2;
      if (sc>255)sc=255;
    }
    sc-=VUCURVE_PT1;
    int isc=(255-VUCURVE_PT1)-sc;
    red=(GetRValue(g_ctheme.vu_mid)*isc + GetRValue(g_ctheme.vu_top)*sc)/(255-VUCURVE_PT1);
    green=(GetGValue(g_ctheme.vu_mid)*isc + GetGValue(g_ctheme.vu_top)*sc)/(255-VUCURVE_PT1);
    blue=(GetBValue(g_ctheme.vu_mid)*isc + GetBValue(g_ctheme.vu_top)*sc)/(255-VUCURVE_PT1);
  }
  if (g_ctheme.vu_dointerlace)
  {
    red=red+red/4;
    green=green+green/4;
    blue=blue+blue/4;
  }
  if (red<0)red=0; else if (red>255)red=255;
  if (green<0)green=0; else if (green>255) green=255;
  if (blue<0)blue=0; else if (blue>255)blue=255;
}


class VuState
{
public:
  VuState()
  {
    m_last_vumode=0;
    m_mode=0;
    m_font=0;
    m_peaktime[0]=m_peaktime[1]=0;
    m_vols[0]=m_vols[1]=m_peakpos[0]=m_peakhold[0]=m_peakpos[1]=m_peakhold[1]=-10000;
  }
  ~VuState()
  {
    if (m_font)
      DeleteObject(m_font);
    m_font=0;
  }
  HFONT m_font;
  LICE_SysBitmap m_bm1, m_bm2;

  int m_mode,m_last_vumode;

  int m_peakhold[2],m_peakpos[2],m_vols[2];
  DWORD m_peaktime[2];
};


static LRESULT CALLBACK VertVuProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  VuState *st=(VuState *)GetWindowLongPtr(hwnd,GWLP_USERDATA);
  switch (msg)
  {
    case WM_CREATE:
      st=new VuState;
      SetWindowLongPtr(hwnd,GWLP_USERDATA,(INT_PTR)st);
    return 0;
    case WM_DESTROY:
      SetWindowLongPtr(hwnd,GWLP_USERDATA,0);
      delete st;
    return 0;
    case WM_USER+6900:
      if (st)
      {
        st->m_mode = lParam==1;
        int v=st->m_mode==1 ? 0 : -120000;
        st->m_peakpos[0] =
        st->m_peakpos[1] =
        st->m_vols[0] =
        st->m_vols[1] =
        st->m_peakhold[1] =
        st->m_peakhold[0] = v;
      }
    return 0;
    case WM_LBUTTONDOWN:
      {
        RECT r;
        GetClientRect(hwnd,&r);

        int MIN_VAL=GetMinVal(st && st->m_mode==1);
        int MAX_VAL=GetMaxVal(st && st->m_mode==1);
        int redPos=r.bottom - ((MIN_VAL*10-1)*(r.bottom-r.top))/((MIN_VAL+MAX_VAL)*10);

        if (GET_Y_LPARAM(lParam) < redPos  || (st && st->m_mode==1))
        {
          if (st)
          {
            int v=st->m_mode==1 ? 0 : -120000;
            st->m_vols[0]=
            st->m_vols[1]=
            st->m_peakpos[0]=
            st->m_peakpos[1]=
            st->m_peakhold[0]=
            st->m_peakhold[1]=v;
            InvalidateRect(hwnd,NULL,FALSE);
          }
        }
      }
    return 0;
    case WM_USER+1011:
      {
        if (st && lParam)
        {
          double *t = (double *)lParam;
          st->m_vols[0]= (int) floor(wdl_clamp(t[0], -1200000.0, 1200000.0) + 0.5);
          st->m_vols[1]= (int) floor(wdl_clamp(t[1], -1200000.0, 1200000.0) + 0.5);
          DWORD now=GetTickCount();
          for (int x = 0; x < 2; x ++)
          {
            if (st->m_vols[x] == -120000) st->m_peakhold[x]=-120000;
            if (st->m_vols[x] == -120000 || now-st->m_peaktime[x] > 1000 || (st->m_mode == 1 && st->m_vols[x] < st->m_peakpos[x]) || (st->m_mode != 1 && st->m_vols[x] > st->m_peakpos[x]) )
            {
              st->m_peakpos[x]=st->m_vols[x];
              st->m_peaktime[x]=now;
            }
            if (st->m_mode == 1 && st->m_vols[x] < st->m_peakhold[x]) st->m_peakhold[x] = st->m_vols[x];
            if (st->m_mode == 0 && st->m_vols[x] > st->m_peakhold[x]) st->m_peakhold[x] = st->m_vols[x];
          }
          InvalidateRect(hwnd,NULL,FALSE);
        }
      }
    return 0;
    case WM_PAINT:
      {
        {
          PAINTSTRUCT ps;
          RECT r;
          GetClientRect(hwnd,&r);
          if (r.right > r.left && r.bottom > r.top && BeginPaint(hwnd,&ps))
          {

            int bm_w=r.right,bm_h=r.bottom;
            int vuwid=(r.right-r.left)/2;

            int diff=st->m_bm1.resize(bm_w,bm_h)|st->m_bm2.resize(bm_w*2,bm_h)|(st->m_last_vumode != g_vu_needrefcnt);
            int MIN_VAL=GetMinVal(st && st->m_mode==1);
            int MAX_VAL=GetMaxVal(st && st->m_mode==1);
            int oredPos=r.bottom - ((MIN_VAL*10-1)*(r.bottom-r.top))/((MIN_VAL+MAX_VAL)*10);
            int redPos=0;
            if (st->m_mode != 1) redPos=oredPos;

            if (diff)
            {
              //OutputDebugString("rerendering user vert VU\n");
              st->m_last_vumode=g_vu_needrefcnt;
              if (!st->m_font)
              {
                static LOGFONT fontlog={
                  12,
                  0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,
                        OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,
                        VU_FONTFACE
                };

                st->m_font=CreateFontIndirect(&fontlog);

              }
              SelectObject(st->m_bm1.getDC(),st->m_font);
              SelectObject(st->m_bm2.getDC(),st->m_font);
              int cury = r.bottom;
              {
                RECT r2=r;
                r2.right *= 2;
                HBRUSH tb=CreateSolidBrush(g_ctheme.vu_intcol);
                FillRect(st->m_bm2.getDC(),&r2,(HBRUSH)tb);
                DeleteObject(tb);
              }

              int isc=(r.bottom-r.top-redPos);
              if (isc<1) isc=1;
              while (cury >= r.top)
              {
                int sc=(256 * (r.bottom-cury))/isc;
                int col;

                MAKERG(cury,<)

                if (st->m_mode)
                {
                  red=255-sc/3;
                  if (red<192) red=192;
                  if (red>255)red=255;
                  green=blue=0;
                  if (g_ctheme.vu_dointerlace && (cury&1))
                  {
                    red=GetRValue(g_ctheme.vu_intcol); green=GetGValue(g_ctheme.vu_intcol); blue=GetBValue(g_ctheme.vu_intcol);
                  }
                  col=RGB(red,green,blue);
                }
                else
                  col=RGB(DIM(red),DIM(green),DIM(blue));


                RECT lr;
                lr.left = 0;
                lr.right=r.right/2-1;
                if (cury < redPos) lr.right++;
                lr.top=cury;
                lr.bottom=cury+1;
                HBRUSH tb=CreateSolidBrush(col);
                FillRect(st->m_bm2.getDC(),&lr,tb);
                lr.left=r.right/2+1;
                if (cury < redPos) lr.left--;
                lr.right=r.right;
                FillRect(st->m_bm2.getDC(),&lr,tb);
                DeleteObject(tb);


                if (st->m_mode)
                {
                  red=80-sc/5;
                  if (red<0)red=0;
                  green=blue=0;
                  if (g_ctheme.vu_dointerlace && (cury&1))
                  {
                    red=GetRValue(g_ctheme.vu_intcol); green=GetGValue(g_ctheme.vu_intcol); blue=GetBValue(g_ctheme.vu_intcol);
                  }
                }

                col=RGB(red,green,blue);

                tb=CreateSolidBrush(col);
                lr.left=r.right;
                lr.right=r.right+r.right/2-1;
                if (cury < redPos) lr.right++;
                FillRect(st->m_bm2.getDC(),&lr,tb);
                lr.left=r.right+r.right/2+1;
                if (cury < redPos) lr.left--;
                lr.right=r.right*2;
                FillRect(st->m_bm2.getDC(),&lr,tb);
                DeleteObject(tb);

                cury--;
              }
              SetBkMode(st->m_bm2.getDC(),TRANSPARENT);

              int x=0;
              int dv=6;
              int mv=MIN_VAL;
              if (r.bottom - r.top < 200) { dv=12; }
              if (st->m_mode==1) { mv-=dv; x+=dv ;}
              if (r.bottom - r.top < 240)
              {
                if (!x) x += 6;
              }
              int maxv=st->m_mode==1?0:MAX_VAL;


              while (x < mv)
              {
                char buf[512];
                int a = 255 - ((255-100)*x)/MIN_VAL;
                if (a < 0) a=0;
                else if (a>255)a=255;
                if (st->m_mode==1) a=255;

                RECT tr=r;
                tr.top=((maxv+x)*r.bottom)/(MIN_VAL+maxv) - 5;
                if (tr.top > 10)
                {
                  snprintf(buf,sizeof(buf),"-%d-",x);
                  SetTextColor(st->m_bm2.getDC(),RGB(a,a,a));
                  tr.left+=1;
                  DrawText(st->m_bm2.getDC(),buf,-1,&tr,DT_SINGLELINE|DT_TOP|DT_CENTER|DT_NOPREFIX);
                  tr.left = tr.right+1;
                  tr.right *= 2;
                  if (x)
                  {
                    tr.left+=2;
                    tr.top++;
                    SetTextColor(st->m_bm2.getDC(),RGB(0,0,0));
                    DrawText(st->m_bm2.getDC(),buf,-1,&tr,DT_SINGLELINE|DT_TOP|DT_CENTER|DT_NOPREFIX);
                    tr.left-=2;
                    tr.top--;
                  }
                  SetTextColor(st->m_bm2.getDC(),RGB(255,255,255));
                  DrawText(st->m_bm2.getDC(),buf,-1,&tr,DT_SINGLELINE|DT_TOP|DT_CENTER|DT_NOPREFIX);
                }
                x+=dv;
              }

            }



            {
              int x;

              char buf[64];
              RECT r2={vuwid-1,r.top,r.right-r.left-vuwid+1,r.bottom};
              HBRUSH blackBrush=CreateSolidBrush(0);
              FillRect(st->m_bm1.getDC(),&r2,blackBrush);
              DeleteObject(blackBrush);
              int t;

              if (st->m_mode==1)
                t=min(st->m_peakhold[0],st->m_peakhold[1]);
              else
                t=max(st->m_peakhold[0],st->m_peakhold[1]);

              int maxv=st->m_mode==1?0:MAX_VAL;

              for (x = 0; x < 2; x ++)
              {
                int ypos=st->m_vols[x]+MIN_VAL*10;

                int wasClip=ypos >= MIN_VAL*10&&g_vu_clipall;

                if (ypos < 0) ypos=0;
                else if (ypos > MIN_VAL*10+maxv*10) ypos=MIN_VAL*10+maxv*10;
                ypos = r.bottom - (ypos * (r.bottom-r.top))/(MIN_VAL*10+maxv*10);


                if (wasClip && ypos >= redPos) ypos=redPos-1;

                int ypos2=st->m_peakpos[x]+MIN_VAL*10;
                wasClip=ypos2 >= MIN_VAL*10&&g_vu_clipall;
                if (ypos2 > MIN_VAL*10+maxv*10) ypos2=MIN_VAL*10+maxv*10;
                ypos2 = r.bottom - (ypos2 * (r.bottom-r.top))/(MIN_VAL*10+maxv*10);
                if (st->m_mode != 1 && wasClip && ypos2 >= redPos) ypos2=redPos-1;

                int xp=x?vuwid:0;
                int xof=xp+vuwid*2;

                BitBlt(st->m_bm1.getDC(),xp,0,vuwid,ypos,st->m_bm2.getDC(),xp,0,SRCCOPY);
                BitBlt(st->m_bm1.getDC(),xp,ypos,vuwid,r.bottom-r.top-ypos,st->m_bm2.getDC(),xof,ypos,SRCCOPY);

                if (t >= 0&&st->m_mode!=1&&g_vu_clipall)
                {
                  BitBlt(st->m_bm1.getDC(),xp,0,vuwid,redPos,st->m_bm2.getDC(),xof,0,SRCCOPY);
                }
                if ((st->m_mode==1 && ypos2 > ypos && ypos2 > oredPos) || (st->m_mode!=1 && ypos2 < ypos))
                {
                  int dpos;
                  if (st->m_mode==1)
                    dpos=ypos2;
                  else
                    dpos=max(ypos2,redPos-1);
                  BitBlt(st->m_bm1.getDC(),xp,ypos2,vuwid,dpos-ypos2+1,st->m_bm2.getDC(),st->m_mode==1?xp:xof,g_ctheme.vu_dointerlace&&ypos2>redPos?(ypos2&~1):ypos2,SRCCOPY);
                }

              }


              SetTextColor(st->m_bm1.getDC(),RGB(255,255,255));
              SetBkMode(st->m_bm1.getDC(),TRANSPARENT);


              if (st->m_mode==1)
              {
                GetClientRect(hwnd,&r);
                if (t < -999) strcpy(buf,"-inf");
                else
                {
                  char s='+';
                  if (t<0) { s='-'; t=-t; }
                  snprintf(buf,sizeof(buf),"%c%d.%01d",s,t/10,t%10);
                }
                DrawText(st->m_bm1.getDC(),buf,-1,&r,DT_SINGLELINE|DT_BOTTOM|DT_CENTER|DT_NOPREFIX);

#if 1
                t=min(st->m_peakpos[0],st->m_peakpos[1]);
                r.bottom=oredPos;
                if (t < -999) strcpy(buf,"-inf");
                else
                {
                  char s='+';
                  if (t<0) { s='-'; t=-t; }
                  snprintf(buf,sizeof(buf),"%c%d.%01d",s,t/10,t%10);
                }
                DrawText(st->m_bm1.getDC(),buf,-1,&r,DT_SINGLELINE|DT_VCENTER|DT_CENTER|DT_NOPREFIX);
#endif
              }
              else
              {
                r.bottom=oredPos;
                if (t < -999) strcpy(buf,"-inf");
                else
                {
                  char s='+';
                  if (t<0) { s='-'; t=-t; }
                  snprintf(buf,sizeof(buf),"%c%d.%01d",s,t/10,t%10);
                }
                if (r.bottom < r.top+10) r.bottom=r.top+10;
                DrawText(st->m_bm1.getDC(),buf,-1,&r,DT_SINGLELINE|DT_VCENTER|DT_CENTER|DT_NOPREFIX);

              }
            }

            BitBlt(ps.hdc,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right-ps.rcPaint.left,ps.rcPaint.bottom-ps.rcPaint.top,st->m_bm1.getDC(),ps.rcPaint.left,ps.rcPaint.top,SRCCOPY);

            EndPaint(hwnd,&ps);
          }
        }
      }

    return 0;
  }
  return DefWindowProc(hwnd,msg,wParam,lParam);
}


#ifndef _WIN32

static HWND meterControlCreator(HWND parent, const char *cname, int idx, const char *classname, int style, int x, int y, int w, int h)
{
  HWND hw=0;
  if (!strcmp(classname,"jsfx_meter"))
  {
    hw=CreateDialog(NULL,0,parent,(DLGPROC)VertVuProc);
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

void Meters_Init(HINSTANCE hInst, bool reg)
{
  if (!reg)
  {
#ifdef _WIN32
    UnregisterClass("jsfx_meter",hInst);
#else
    SWELL_UnregisterCustomControlCreator(meterControlCreator);
#endif
    return;
  }

#ifndef _WIN32
  SWELL_RegisterCustomControlCreator(meterControlCreator);
#else
    {
      LRESULT CALLBACK VertVuProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
      WNDCLASS wc={CS_HREDRAW|CS_VREDRAW,VertVuProc,};
      wc.lpszClassName="jsfx_meter";
      wc.hInstance=hInst;
      wc.hCursor=LoadCursor(NULL,IDC_ARROW);
      RegisterClass(&wc);
    }
#endif
}
