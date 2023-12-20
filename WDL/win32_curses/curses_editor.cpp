#ifdef _WIN32
#include <windows.h>
#else
#include "../swell/swell.h"
#endif
#include <stdlib.h>
#include <string.h>
#ifndef CURSES_INSTANCE
#define CURSES_INSTANCE ((win32CursesCtx*)m_cursesCtx)
#endif

#include "curses_editor.h"
#include "../wdlutf8.h"
#include "../win32_utf8.h"
#include "../wdlcstring.h"

#ifndef VALIDATE_TEXT_CHAR
#define VALIDATE_TEXT_CHAR(thischar) ((thischar) >= 0 && (thischar >= 128 || isspace(thischar) || isgraph(thischar)) && !(thischar >= KEY_DOWN && thischar <= KEY_F12))
#endif



#ifdef __APPLE__
#define CONTROL_KEY_NAME "Cmd"
#else
#define CONTROL_KEY_NAME "Ctrl"
#endif

WDL_FastString WDL_CursesEditor::s_fake_clipboard;
int WDL_CursesEditor::s_overwrite=0;
int WDL_CursesEditor::s_search_mode; // &1=case sensitive, &2=word. 5/6 = token matching

static WDL_FastString s_goto_line_buf;

static const char *searchmode_desc(int mode)
{
  if (mode == 4 || mode == 5) return (mode&1)  ? "TokenMatch":"tokenmatch";
  return (mode&2) ?
    ((mode&1) ? "WordMatch":"wordmatch") :
    ((mode&1) ? "SubString":"substring");
}


#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL 0x20A
#endif

static void __curses_onresize(win32CursesCtx *ctx)
{
  WDL_CursesEditor *p = (WDL_CursesEditor *)ctx->user_data;
  if (p)
  {
    p->draw();
    p->setCursorIfVisible();
  }
}
WDL_CursesEditor::WDL_CursesEditor(void *cursesCtx)
{ 
  m_newline_mode=0;
  m_write_leading_tabs=0;
  m_max_undo_states = 500;
  m_indent_size=2;
  m_cursesCtx = cursesCtx;

  m_top_margin=1;
  m_bottom_margin=1;

  m_selecting=0;
  m_select_x1=m_select_y1=m_select_x2=m_select_y2=0;
  m_ui_state=UI_STATE_NORMAL;
  m_offs_x=0;
  m_curs_x=m_curs_y=0;
  m_want_x=-1;
  m_undoStack_pos=-1;
  m_clean_undopos=0;

  m_curpane=0;
  m_pane_div=1.0;
  m_paneoffs_y[0]=m_paneoffs_y[1]=0;

  m_curpane=0;
  m_scrollcap=0;
  m_scrollcap_yoffs=0;
  
  m_filelastmod=0;
  m_status_lastlen=0;

#ifdef WDL_IS_FAKE_CURSES
  if (m_cursesCtx)
  {
    CURSES_INSTANCE->user_data = this;
    CURSES_INSTANCE->onMouseMessage = _onMouseMessage;
    CURSES_INSTANCE->want_scrollbar=1; // 1 or 2 chars wide
    CURSES_INSTANCE->do_update = __curses_onresize;
  }
#endif

  initscr();
  cbreak();
  noecho();
  nonl();
  intrflush(stdscr,FALSE);
  keypad(stdscr,TRUE);
  nodelay(stdscr,TRUE);
  raw(); // disable ctrl+C etc. no way to kill if allow quit isn't defined, yay.
  start_color();

#ifdef WDL_IS_FAKE_CURSES
  if (!curses_win32_global_user_colortab && (!m_cursesCtx || !CURSES_INSTANCE->user_colortab))
#endif
  {
  init_pair(1, COLOR_WHITE, COLOR_BLUE);     // COLOR_BOTTOMLINE
  init_pair(2, COLOR_BLACK, COLOR_CYAN);     // COLOR_SELECTION
  init_pair(3, RGB(0,255,255),COLOR_BLACK);  // SYNTAX_HIGHLIGHT1
  init_pair(4, RGB(0,255,0),COLOR_BLACK);    // SYNTAX_HIGHLIGHT2
  init_pair(5, RGB(96,128,192),COLOR_BLACK); // SYNTAX_COMMENT
  init_pair(6, COLOR_WHITE, COLOR_RED);      // SYNTAX_ERROR
  init_pair(7, RGB(255,255,0), COLOR_BLACK); // SYNTAX_FUNC

#ifdef WDL_IS_FAKE_CURSES
  init_pair(8, RGB(255,128,128), COLOR_BLACK);  // SYNTAX_REGVAR
  init_pair(9, RGB(0,192,255), COLOR_BLACK);    // SYNTAX_KEYWORD
  init_pair(10, RGB(255,192,192), COLOR_BLACK); // SYNTAX_STRING
  init_pair(11, RGB(192,255,128), COLOR_BLACK); // SYNTAX_STRINGVAR
  init_pair(12, COLOR_BLACK, COLOR_CYAN);       // COLOR_MESSAGE (maps to COLOR_SELECTION)
  init_pair(13, COLOR_WHITE, COLOR_RED);        // COLOR_TOPLINE (maps to SYNTAX_ERROR)
  init_pair(14, RGB(192,192,0), COLOR_BLACK);   // SYNTAX_FUNC2
#endif
  }

  erase();
  refresh();
}

int  WDL_CursesEditor::GetPaneDims(int* paney, int* paneh) // returns ypos of divider
{
  const int pane_divy=(int)(m_pane_div*(double)(LINES-m_top_margin-m_bottom_margin-1));
  if (paney)
  {
    paney[0]=m_top_margin;
    paney[1]=m_top_margin+pane_divy+1;
  }
  if (paneh)
  {
    paneh[0]=pane_divy+(m_pane_div >= 1.0 ? 1 : 0);
    paneh[1]=LINES-pane_divy-m_top_margin-m_bottom_margin-1;
    
  }
  return pane_divy;
}


int WDL_CursesEditor::getVisibleLines() const { return LINES-m_bottom_margin-m_top_margin; }


#ifdef WDL_IS_FAKE_CURSES
LRESULT WDL_CursesEditor::onMouseMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static int s_mousedown[2];

  switch (uMsg)
  {
    case WM_CAPTURECHANGED:
    break;

    case WM_MOUSEMOVE:
      if (GetCapture()==hwnd && CURSES_INSTANCE->m_font_w && CURSES_INSTANCE->m_font_h)
      {
        POINT pt = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
        int cx=pt.x/CURSES_INSTANCE->m_font_w;
        int cy=pt.y/CURSES_INSTANCE->m_font_h;

        int paney[2], paneh[2];
        const int pane_divy=GetPaneDims(paney, paneh);

        if (m_scrollcap)
        {
          int i=m_scrollcap-1;

          if (i == 2) // divider
          {
            int divy=cy-paney[0];
            if (divy != pane_divy)
            {
              if (divy <= 0 || divy >= LINES-m_top_margin-m_bottom_margin-1)
              {
                if (divy <= 0.0) m_paneoffs_y[0]=m_paneoffs_y[1];
                m_curpane=0;
                m_pane_div=1.0;
              }
              else
              {
                m_pane_div=(double)divy/(double)(LINES-m_top_margin-m_bottom_margin-1);
              }
              draw();
              draw_status_state();
            }
          }
          else
          {
            int prevoffs=m_paneoffs_y[i];
            int pxy=paney[i]*CURSES_INSTANCE->m_font_h;
            int pxh=paneh[i]*CURSES_INSTANCE->m_font_h;

            if (pxh > CURSES_INSTANCE->scroll_h[i])
            {
              m_paneoffs_y[i]=(m_text.GetSize()-paneh[i])*(pt.y-m_scrollcap_yoffs-pxy)/(pxh-CURSES_INSTANCE->scroll_h[i]);
              int maxscroll=m_text.GetSize()-paneh[i]+4;
              if (m_paneoffs_y[i] > maxscroll) m_paneoffs_y[i]=maxscroll;
              if (m_paneoffs_y[i] < 0) m_paneoffs_y[i]=0;
            }

            if (m_paneoffs_y[i] != prevoffs)
            {
              draw();
              draw_status_state();
              
              const int col=m_curs_x-m_offs_x;
              int line=m_curs_y+paney[m_curpane]-m_paneoffs_y[m_curpane];
              if (line >= paney[m_curpane] && line < paney[m_curpane]+paneh[m_curpane]) move(line, col);
            }
          }

          return 0;
        }

        if (!m_selecting && (cx != s_mousedown[0] || cy != s_mousedown[1]))
        {
          m_select_x2=m_select_x1=m_curs_x;
          m_select_y2=m_select_y1=m_curs_y;
          m_selecting=1;
        }

        int x=cx+m_offs_x;
        int y=cy+m_paneoffs_y[m_curpane]-paney[m_curpane];
        if (m_selecting && (m_select_x2!=x || m_select_y2 != y))
        {
          if (y < m_paneoffs_y[m_curpane] && m_paneoffs_y[m_curpane] > 0)
          {
            m_paneoffs_y[m_curpane]--;
          }
          else if (y >= m_paneoffs_y[m_curpane]+paneh[m_curpane] && m_paneoffs_y[m_curpane]+paneh[m_curpane] < m_text.GetSize())
          {
            m_paneoffs_y[m_curpane]++;
          }
          if (x < m_offs_x && m_offs_x > 0)
          {
            m_offs_x--;
          }
          else if (x > m_offs_x+COLS)
          {
            int maxlen=0;
            int a;
            for (a=0; a < paneh[m_curpane]; a++)
            {
              WDL_FastString* s=m_text.Get(m_paneoffs_y[m_curpane]+a);
              if (s)
              {
                const int l = WDL_utf8_get_charlen(s->Get());
                if (l > maxlen) maxlen=l;
              }
            }
            if (maxlen > m_offs_x+COLS-8) m_offs_x++;
          }

          m_select_y2=y;
          m_select_x2=x;
          if (m_select_y2<0) m_select_y2=0;
          else if (m_select_y2>=m_text.GetSize())
          {
            m_select_y2=m_text.GetSize()-1;
            WDL_FastString *s=m_text.Get(m_select_y2);
            if (s) m_select_x2 = WDL_utf8_get_charlen(s->Get());
          }
          if (m_select_x2<0)m_select_x2=0;
          WDL_FastString *s=m_text.Get(m_select_y2);
          if (s)
          {
            const int l = WDL_utf8_get_charlen(s->Get());
            if (m_select_x2>l) m_select_x2 = l;
          }
          draw();

          int y=m_curs_y+paney[m_curpane]-m_paneoffs_y[m_curpane];
          if (y >= paney[m_curpane] && y < paney[m_curpane]+paneh[m_curpane]) setCursor();
        }
      }
      break;
    case WM_LBUTTONDBLCLK:
      if (CURSES_INSTANCE && CURSES_INSTANCE->m_font_w && CURSES_INSTANCE->m_font_h)
      {
        const int y = ((short)HIWORD(lParam)) / CURSES_INSTANCE->m_font_h - m_top_margin;
        const int x = ((short)LOWORD(lParam)) / CURSES_INSTANCE->m_font_w + m_offs_x;
        WDL_FastString *fs=m_text.Get(y + m_paneoffs_y[m_curpane]);
        if (fs && y >= 0)
        {
          const char *url=fs->Get();
          
          while (NULL != (url = strstr(url,"http://")))
          {
            if (url != fs->Get() && url[-1] > 0 && isalnum(url[-1]))
            {
              url+=7;
            }
            else
            {
              const int soffs = (int) (url - fs->Get());
              char tmp[512];
              char *p=tmp;
              while (p < (tmp+sizeof(tmp)-1) &&
                    *url && *url != ' ' && *url != ')' && *url != '\t' && *url != '"' && *url != '\'' )
              {
                *p++ = *url++;
              }
              *p=0;
              if (strlen(tmp) >= 10 && x >= soffs && x<(url-fs->Get()))
              {
                ShellExecute(hwnd,"open",tmp,"","",0);
                return 1;
              }
            }
          }
        }
      }

    case WM_LBUTTONDOWN:
      if (CURSES_INSTANCE && CURSES_INSTANCE->m_font_w && CURSES_INSTANCE->m_font_h)
      {
        int x = ((short)LOWORD(lParam)) / CURSES_INSTANCE->m_font_w;
        int y = ((short)HIWORD(lParam)) / CURSES_INSTANCE->m_font_h;
        const int tabcnt=GetTabCount();
        if (y==0 && tabcnt>1)
        {
          int tsz=COLS/tabcnt;
          // this is duplicated in draw_top_line
          if (tsz>128)tsz=128;
          if (tsz<12) tsz=12;
          SwitchTab(x/tsz,false);

          return 1;
        }
      }

      // passthrough
    case WM_RBUTTONDOWN:

    if (CURSES_INSTANCE->m_font_w && CURSES_INSTANCE->m_font_h)
    {
      int mousex=(short)LOWORD(lParam);
      int mousey=(short)HIWORD(lParam);

      int cx=mousex/CURSES_INSTANCE->m_font_w;
      int cy=mousey/CURSES_INSTANCE->m_font_h;
      if (cx > COLS) cx=COLS;
      if (cx < 0) cx=0;
      if (cy > LINES) cy=LINES;
      if (cy < 0) cy=0;

      m_ui_state=UI_STATE_NORMAL; // any click clears the state
      s_mousedown[0]=cx;
      s_mousedown[1]=cy;

      int paney[2], paneh[2];
      const int pane_divy=GetPaneDims(paney, paneh);

      if (uMsg == WM_LBUTTONDOWN && m_pane_div > 0.0 && m_pane_div < 1.0 && cy == m_top_margin+pane_divy)
      {
        SetCapture(hwnd);
        m_scrollcap=3;
        return 0;
      }

      int pane=-1;
      if (cy >= paney[0] && cy < paney[0]+paneh[0]) pane=0;
      else if (cy >= paney[1] && cy < paney[1]+paneh[1]) pane=1;

      if ((uMsg == WM_LBUTTONDOWN || uMsg == WM_LBUTTONDBLCLK) && pane >= 0 &&
          cx >= COLS-CURSES_INSTANCE->drew_scrollbar[pane])
      {
        const int st=paney[pane]*CURSES_INSTANCE->m_font_h+CURSES_INSTANCE->scroll_y[pane];
        const int sb=st+CURSES_INSTANCE->scroll_h[pane];

        int prevoffs=m_paneoffs_y[pane];

        if (mousey < st)
        {
          m_paneoffs_y[pane] -= paneh[pane];
          if (m_paneoffs_y[pane] < 0) m_paneoffs_y[pane]=0;
        }
        else if (mousey < sb)
        {
          if (uMsg == WM_LBUTTONDOWN)
          {
            SetCapture(hwnd);
            m_scrollcap=pane+1;
            m_scrollcap_yoffs=mousey-st;
          }
        }
        else
        {
          m_paneoffs_y[pane] += paneh[pane];
          int maxscroll=m_text.GetSize()-paneh[pane]+4;
          if (m_paneoffs_y[pane] > maxscroll) m_paneoffs_y[pane]=maxscroll;
        }
        
        if (prevoffs != m_paneoffs_y[pane])
        {
          draw();
          draw_status_state();

          int y=m_curs_y+paney[m_curpane]-m_paneoffs_y[m_curpane];
          if (y >= paney[m_curpane] && y < paney[m_curpane]+paneh[m_curpane]) setCursor();
        }
        return 0;
      }

      int ox=m_curs_x , oy=m_curs_y;
      if (m_selecting && ox == m_select_x2 && oy == m_select_y2)
      {
        ox = m_select_x1;
        oy = m_select_y1;
      }

      if (uMsg == WM_LBUTTONDOWN) m_selecting=0;

      if (pane >= 0) m_curpane=pane;           

      m_curs_x=cx+m_offs_x;
      m_curs_y=cy+m_paneoffs_y[m_curpane]-paney[m_curpane];

      bool end = (m_curs_y > m_text.GetSize()-1);
      if (end) m_curs_y=m_text.GetSize()-1;
      if (m_curs_y < 0) m_curs_y = 0;

      WDL_FastString *s=m_text.Get(m_curs_y); 
      if (m_curs_x < 0) m_curs_x = 0;
      const int slen = s ? WDL_utf8_get_charlen(s->Get()) : 0;

      if (s && (end || m_curs_x > slen)) m_curs_x=slen;
      
      if (uMsg == WM_LBUTTONDOWN && !!(GetAsyncKeyState(VK_SHIFT)&0x8000) && 
        (m_curs_x != ox || m_curs_y != oy))
      {
        m_select_x1=ox;
        m_select_y1=oy;
        m_select_x2=m_curs_x;
        m_select_y2=m_curs_y;
        m_selecting=1;
      }
      else if (uMsg == WM_LBUTTONDBLCLK && s && slen)
      {
        if (m_curs_x < slen)
        {     
          int x1=WDL_utf8_charpos_to_bytepos(s->Get(),m_curs_x);
          int x2=x1+1;
          const char* p=s->Get();
          while (x1 > 0 && p[x1-1] > 0 && (isalnum(p[x1-1]) || p[x1-1] == '_')) --x1;
          while (x2 < s->GetLength() && p[x2] > 0 && (isalnum(p[x2]) || p[x2] == '_')) ++x2;
          if (x2 > x1)
          {
            m_select_x1=WDL_utf8_bytepos_to_charpos(s->Get(),x1);
            m_curs_x=m_select_x2=WDL_utf8_bytepos_to_charpos(s->Get(),x2);
            m_select_y1=m_select_y2=m_curs_y;
            m_selecting=1;
          }
        }
      }

      onChar('L'-'A'+1); // refresh, update suggestions

      if (uMsg == WM_LBUTTONDOWN) 
      {
        SetCapture(hwnd);
      }
      else if (uMsg == WM_RBUTTONDOWN)
      {
        onRightClick(hwnd);
      }
    }
    return 0;

    case WM_LBUTTONUP:
      ReleaseCapture();
      m_scrollcap=0;
      m_scrollcap_yoffs=0;
    return 0;

    case WM_MOUSEWHEEL:
      if (CURSES_INSTANCE->m_font_h)
      {
        POINT p;
        GetCursorPos(&p);
        ScreenToClient(hwnd, &p);
        p.y /= CURSES_INSTANCE->m_font_h;

        int paney[2], paneh[2];
        GetPaneDims(paney, paneh);
        int pane=-1;
        if (p.y >= paney[0] && p.y < paney[0]+paneh[0]) pane=0;
        else if (p.y >= paney[1] && p.y < paney[1]+paneh[1]) pane=1;
        if (pane < 0) pane=m_curpane;

        m_paneoffs_y[pane] -= ((short)HIWORD(wParam))/20;

        int maxscroll=m_text.GetSize()-paneh[pane]+4;
        if (m_paneoffs_y[pane] > maxscroll) m_paneoffs_y[pane]=maxscroll;
        if (m_paneoffs_y[pane] < 0) m_paneoffs_y[pane]=0;

        draw();

        int y=m_curs_y+paney[m_curpane]-m_paneoffs_y[m_curpane];
        if (y >= paney[m_curpane] && y < paney[m_curpane]+paneh[m_curpane]) setCursor();
        else draw_status_state();
      }
    break;
  }
  return 0;
}
#endif


WDL_CursesEditor::~WDL_CursesEditor()
{
  endwin();
  m_text.Empty(true);
  m_undoStack.Empty(true);
}

int WDL_CursesEditor::init(const char *fn, const char *init_if_empty)
{
  m_filename.Set(fn);
  FILE *fh=fopenUTF8(fn,"rb");

  if (!fh) 
  {
    if (init_if_empty)
    {
      fh=fopenUTF8(fn,"w+b");
      if (fh)
      {
        fwrite(init_if_empty,1,strlen(init_if_empty),fh);

        fseek(fh,0,SEEK_SET);
      }
    }
    if (!fh)
    {
      saveUndoState();
      m_clean_undopos=m_undoStack_pos;
      return 1;
    }
  }
  
  loadLines(fh);
  fclose(fh);

  return 0;  
}

int WDL_CursesEditor::reload_file(bool clearundo)
{
  FILE *fh=fopenUTF8(m_filename.Get(),"rb");
  if (fh)
  {
    if (!clearundo)
    {
      preSaveUndoState();
    }
    else
    {
      m_undoStack.Empty(true);
      m_undoStack_pos=-1;
    }

    m_text.Empty(true);
    loadLines(fh);
    fclose(fh);

    return 0;
  }
  return 1;
}

static void ReplaceTabs(WDL_FastString *str, int tabsz)
{
  int x;
  char s[128];
  // replace any \t with spaces
  int insert_sz=tabsz - 1;
  if (insert_sz<0) insert_sz=0;
  else if (insert_sz>128) insert_sz=128;

  if (insert_sz>0) memset(s,' ',insert_sz);
  for(x=0;x<str->GetLength();x++)
  {
    char *p = (char *)str->Get();
    if (p[x] == '\t')
    {
      p[x] = ' ';
      str->Insert(s,x+1,insert_sz);
      x+=insert_sz;
    }
  }
}

void WDL_CursesEditor::loadLines(FILE *fh)
{
  int crcnt = 0;
  int tabstate = 0;
  int tab_cnv_size=5;
  int rdcnt=0;
  for (;;)
  {
    WDL_FastString *fs = NULL;

    for (;;)
    {
      char line[4096];
      line[0]=0;
      fgets(line,sizeof(line),fh);
      if (!line[0]) break;

      if (!fs) fs = new WDL_FastString(line);
      else fs->Append(line);

      if (fs->Get()[fs->GetLength()-1] == '\n' || fs->GetLength() > 32*1024*1024) break;
    }
    if (!fs) break;

    if (!rdcnt++)
    {
      if ((unsigned char)fs->Get()[0] == 0xef && 
          (unsigned char)fs->Get()[1] == 0xbb && 
          (unsigned char)fs->Get()[2] == 0xbf)
      {
        // remove BOM (could track it, but currently EEL/etc don't support reading it anyway)
        fs->DeleteSub(0,3);
        if (!fs->GetLength()) break;
      }
    }

    while (fs->GetLength()>0)
    {
      char c = fs->Get()[fs->GetLength()-1];
      if (c == '\r') crcnt++;
      else if (c != '\n') break;
      fs->DeleteSub(fs->GetLength()-1,1);
    }
    m_text.Add(fs);

    if (tabstate>=0)
    {
      const char *p = fs->Get();
      if (*p == '\t' && !tabstate) tabstate=1; 
      while (*p == '\t') p++;

      int spacecnt=0;
      while (*p == ' ') { p++; spacecnt++; }
      if (*p == '\t' || spacecnt>7) tabstate=-1; // tab after space, or more than 7 spaces = dont try to preserve tabs
      else if (spacecnt+1 > tab_cnv_size) tab_cnv_size = spacecnt+1;
    }
  }
  if (tabstate>0)
  {
    m_indent_size=tab_cnv_size;
    m_write_leading_tabs=tab_cnv_size;
  }
  else
  {
    m_write_leading_tabs=0;
  }

  int x;
  for (x=0;x<m_text.GetSize();x++)
  {
    WDL_FastString *s = m_text.Get(x);
    if (s)
      ReplaceTabs(s,m_indent_size);
  }
  m_newline_mode=crcnt > m_text.GetSize()/2; // more than half of lines have crlf, then use crlf

  saveUndoState();
  m_clean_undopos=m_undoStack_pos;
  updateLastModTime();
}

void WDL_CursesEditor::draw_status_state()
{
  int paney[2], paneh[2];
  const int pane_divy=GetPaneDims(paney, paneh);

  attrset(COLOR_BOTTOMLINE);
  bkgdset(COLOR_BOTTOMLINE);

  int line=LINES-1;
  const char* whichpane="";
  if (m_pane_div > 0.0 && m_pane_div < 1.0)
  {
    whichpane=(!m_curpane ? "Upper pane: " : "Lower pane: ");
    line=m_top_margin+pane_divy;
    move(line, 0);
    clrtoeol();
  }

  char str[512];
  const int pane_offs = m_paneoffs_y[!!m_curpane];
  snprintf(str, sizeof(str), "%sLine %d/%d [%d-%d] Col %d [%s]%s",
    whichpane,
    m_curs_y+1, m_text.GetSize(),
    pane_offs+1, wdl_min(pane_offs+1+paneh[!!m_curpane],m_text.GetSize()),
    m_curs_x+1,
    (s_overwrite ? "OVR" : "INS"), (m_clean_undopos == m_undoStack_pos ? "" : "*"));

  int len=strlen(str);
  int x=COLS-len-1;
  if (!*whichpane)
  {
    if (len < m_status_lastlen)
    {
      int xpos = COLS-m_status_lastlen-1;
      if (xpos<0) xpos=0;
      move(line,xpos);
      while (xpos++ < x) addstr(" ");
    }
    m_status_lastlen = len;
  }
  else 
  {
    m_status_lastlen=0;
  }

  mvaddnstr(line, x, str, len);
  clrtoeol();

  attrset(0);
  bkgdset(0);  

  const int col=m_curs_x-m_offs_x;
  line=m_curs_y+paney[m_curpane]-m_paneoffs_y[m_curpane];
  if (line >= paney[m_curpane] && line < paney[m_curpane]+paneh[m_curpane]) move(line, col);
}

void WDL_CursesEditor::setCursor(int isVscroll, double ycenter)
{
  int maxx=m_text.Get(m_curs_y) ? m_text.Get(m_curs_y)->GetLength() : 0;

  if (isVscroll)
  {
    if (m_want_x >= 0) m_curs_x=m_want_x;
  }
  else
  {
    if (m_curs_x < maxx) m_want_x=-1;
  }

  if(m_curs_x>maxx)
  {
    if (isVscroll) m_want_x=m_curs_x;
    m_curs_x=maxx;
  }

  int redraw=0;

  if (m_curs_x < m_offs_x) 
  { 
    redraw=1; 
    m_offs_x=m_curs_x; 
  }
  else 
  {
    const int mw = COLS-3;
    if (m_curs_x >= m_offs_x + mw)
    { 
      m_offs_x=m_curs_x-mw+1; 
      redraw=1; 
    }
  }

  int paney[2], paneh[2];
  GetPaneDims(paney, paneh);
  int y;
  if (ycenter >= 0.0 && ycenter < 1.0)
  {
    y=(int)((double)paneh[m_curpane]*ycenter);
    m_paneoffs_y[m_curpane]=m_curs_y-y;
    if (m_paneoffs_y[m_curpane] < 0)
    {
      y += m_paneoffs_y[m_curpane];
      m_paneoffs_y[m_curpane]=0;
    }
    redraw=1;
  }
  else
  {
    y=m_curs_y-m_paneoffs_y[m_curpane];
    if (y < 0) 
    {
      m_paneoffs_y[m_curpane] += y;
      y=0;
      redraw=1;
    }
    else if (y >= paneh[m_curpane]) 
    {
      m_paneoffs_y[m_curpane] += y-paneh[m_curpane]+1;
      y=paneh[m_curpane]-1;
      redraw=1;
    }
  }

  if (redraw) draw();

  draw_status_state();

  y += paney[m_curpane];
  move(y, m_curs_x-m_offs_x);
}

void WDL_CursesEditor::setCursorIfVisible()
{
  if (WDL_NOT_NORMALLY(m_curpane != 0 && m_curpane != 1)) return;
  int paney[2], paneh[2];
  GetPaneDims(paney, paneh);
  int y=m_curs_y-m_paneoffs_y[m_curpane];
  if (y >= 0 && y < paneh[m_curpane])
    setCursor();
}

void WDL_CursesEditor::draw_message(const char *str)
{
  if (!CURSES_INSTANCE) return;
  
  int l=strlen(str);
  if (l && m_ui_state == UI_STATE_NORMAL) m_ui_state=UI_STATE_MESSAGE;
  if (l > COLS-2) l=COLS-2;
  if (str[0]) 
  {
    attrset(COLOR_MESSAGE);
    bkgdset(COLOR_MESSAGE);
  }
  mvaddnstr(LINES-(m_bottom_margin>1?2:1),0,str,l);
  clrtoeol();
  if (str[0])
  {
    attrset(0);
    bkgdset(0);
  }   

  int paney[2], paneh[2];
  GetPaneDims(paney, paneh);

  const int col=m_curs_x-m_offs_x;
  int line=m_curs_y+paney[m_curpane]-m_paneoffs_y[m_curpane];
  if (line >= paney[m_curpane] && line < paney[m_curpane]+paneh[m_curpane]) move(line, col);
}


void WDL_CursesEditor::draw_line_highlight(int y, const char *p, int *c_comment_state, int line_n)
{
  attrset(A_NORMAL);
  mvaddstr(y,0,p + WDL_utf8_charpos_to_bytepos(p,m_offs_x));
  clrtoeol();
}

void WDL_CursesEditor::getselectregion(int &minx, int &miny, int &maxx, int &maxy)
{
    if (m_select_y2 < m_select_y1)
    {
      miny=m_select_y2; maxy=m_select_y1;
      minx=m_select_x2; maxx=m_select_x1;
    }
    else if (m_select_y1 < m_select_y2)
    {
      miny=m_select_y1; maxy=m_select_y2;
      minx=m_select_x1; maxx=m_select_x2;
    }
    else
    {
      miny=maxy=m_select_y1;
      minx=wdl_min(m_select_x1,m_select_x2);
      maxx=wdl_max(m_select_x1,m_select_x2);
    }
}

void WDL_CursesEditor::doDrawString(int y, int line_n, const char *p, int *c_comment_state)
{
  draw_line_highlight(y,p,c_comment_state, line_n);

  if (m_selecting)
  {
    int miny,maxy,minx,maxx;
    getselectregion(minx,miny,maxx,maxy);
   
    if (line_n >= miny && line_n <= maxy && (miny != maxy || minx < maxx))
    {
      minx-=m_offs_x;
      maxx-=m_offs_x;

      const int cols = COLS;

      if (line_n > miny) minx=0;
      if (line_n < maxy) maxx=cols;

      if (minx<0)minx=0;
      if (minx > cols) minx=cols;
      if (maxx > cols) maxx=cols;

      if (maxx > minx)
      {
        attrset(COLOR_SELECTION);
        p += WDL_utf8_charpos_to_bytepos(p,m_offs_x+minx);
        mvaddnstr(y,minx, p, WDL_utf8_charpos_to_bytepos(p,maxx-minx));
        attrset(A_NORMAL);
      }
      else if (maxx==minx && !*p)
      {
        attrset(COLOR_SELECTION);
        mvaddstr(y,minx," ");
        attrset(A_NORMAL);
      }
    }
  }
}

int WDL_CursesEditor::GetCommentStateForLineStart(int line) // pass current line/col, updates with previous interesting point, returns true if start of comment, or false if end of previous comment
{
  return 0;
}


void WDL_CursesEditor::draw(int lineidx)
{
  if (m_top_margin != 0) m_top_margin = GetTabCount()>1 ? 2 : 1;

  int paney[2], paneh[2];
  const int pane_divy=GetPaneDims(paney, paneh);

#ifdef WDL_IS_FAKE_CURSES
  if (!m_cursesCtx) return;

  CURSES_INSTANCE->offs_y[0]=m_paneoffs_y[0];
  CURSES_INSTANCE->offs_y[1]=m_paneoffs_y[1];
  CURSES_INSTANCE->div_y=pane_divy;
  CURSES_INSTANCE->tot_y=m_text.GetSize();

  CURSES_INSTANCE->scrollbar_topmargin = m_top_margin;
  CURSES_INSTANCE->scrollbar_botmargin = m_bottom_margin;
#endif

  attrset(A_NORMAL);

  if (lineidx >= 0)
  {
    int comment_state = GetCommentStateForLineStart(lineidx);
    WDL_FastString *s=m_text.Get(lineidx);
    if (s)
    {
      int y=lineidx-m_paneoffs_y[0];
      if (y >= 0 && y < paneh[0])
      {
        doDrawString(paney[0]+y, lineidx, s->Get(), &comment_state);
      } 
      y=lineidx-m_paneoffs_y[1];
      if (y >= 0 && y < paneh[1])
      {
        doDrawString(paney[1]+y, lineidx, s->Get(), &comment_state);
      }
    }
    return;
  }

  __curses_invalidatefull((win32CursesCtx*)m_cursesCtx,false);

  draw_top_line();

  attrset(A_NORMAL);
  bkgdset(A_NORMAL);

  move(m_top_margin,0);
  clrtoeol();

  m_status_lastlen=0;

  int pane, i;
  for (pane=0; pane < 2; ++pane)
  {
    int ln=m_paneoffs_y[pane];
    int y=paney[pane];
    int h=paneh[pane];

    int comment_state=GetCommentStateForLineStart(ln);
 
    for(i=0; i < h; ++i, ++ln, ++y)
    { 
      WDL_FastString *s=m_text.Get(ln);
      if (!s) 
      {
        move(y,0);
        clrtoeol();
      }
      else
      {
        doDrawString(y,ln,s->Get(),&comment_state);
      }
    }
  }

  attrset(COLOR_BOTTOMLINE);
  bkgdset(COLOR_BOTTOMLINE);

  if (m_bottom_margin>0)
  {
    move(LINES-1, 0);
#define BOLD(x) { attrset(COLOR_BOTTOMLINE|A_BOLD); addstr(x); attrset(COLOR_BOTTOMLINE&~A_BOLD); }
    if (m_selecting) 
    {
      mvaddstr(LINES-1,0,"SELECTING  ESC:cancel " CONTROL_KEY_NAME "+(");
      BOLD("C"); addstr("opy ");
      BOLD("X"); addstr(":cut ");
      BOLD("V"); addstr(":paste)");
    }
    else 
    {
      mvaddstr(LINES-1, 0, CONTROL_KEY_NAME "+(");

      if (m_pane_div <= 0.0 || m_pane_div >= 1.0) 
      {
        BOLD("P"); addstr("ane ");
      }
      else
      {
        BOLD("O"); addstr("therpane ");
        addstr("no"); BOLD("P"); addstr("anes ");
      }
      BOLD("F"); addstr("ind/");
      BOLD("R"); addstr("eplace ");
      draw_bottom_line();
      addstr(")");
    }
#undef BOLD
    clrtoeol();
  }

  attrset(0);
  bkgdset(0);

  __curses_invalidatefull((win32CursesCtx*)m_cursesCtx,true);
}

void WDL_CursesEditor::draw_bottom_line()
{
  // implementers add key commands here
}

static const char *countLeadingTabs(const char *p, int *ntabs, int tabsz)
{
  if (tabsz>0) while (*p)
  {
    int i;
    for (i=0;i<tabsz;i++) if (p[i]!=' ') return p;
    p+=tabsz;
    (*ntabs) += 1;
  }
  return p;
}

int WDL_CursesEditor::updateFile()
{
  FILE *fp=fopenUTF8(m_filename.Get(),"wb");
  if (!fp) return 1;
  int x;
  for (x = 0; x < m_text.GetSize(); x ++)
  {
    WDL_FastString *s = m_text.Get(x);
    if (s)
    {
      int tabcnt=0;
      const char *p = countLeadingTabs(s->Get(),&tabcnt,m_write_leading_tabs);
      while (tabcnt-->0) fputc('\t',fp);
      fwrite(p,1,strlen(p),fp);
      if (m_newline_mode==1) fputc('\r',fp);
      fputc('\n',fp);
    }
  }
  fclose(fp);
  sync();

  updateLastModTime();
  m_clean_undopos = m_undoStack_pos;

  return 0;
}

void WDL_CursesEditor::updateLastModTime()
{
  struct stat srcstat;
  if (!statUTF8(m_filename.Get(), &srcstat))
  {
#ifndef __APPLE__
    m_filelastmod = srcstat.st_mtime;
#else
    m_filelastmod = srcstat.st_mtimespec.tv_sec;
#endif
  }
  else
  {
    m_filelastmod = 0;
  }
}

void WDL_CursesEditor::indentSelect(int amt)
{
  if (m_selecting)  // remove selected text
  {
    int miny,maxy,minx,maxx;
    int x;
    getselectregion(minx,miny,maxx,maxy);
    if (maxy >= miny)
    {
      m_select_x1 = 0;
      m_select_y1 = miny;
      m_select_y2 = maxy;
      m_select_x2 = maxx;
      if (maxx<1 && maxy>miny) maxy--; // exclude empty final line

      if (m_curs_y >= miny && m_curs_y <=maxy)
      {
        m_curs_x += amt;
        if (m_curs_x<0)m_curs_x=0;
      }

      if (amt<0)
      {
        int minspc=-amt;
        for (x = miny; x <= maxy; x ++)
        {
          WDL_FastString *s=m_text.Get(x);
          if (s)
          {
            int a=0;
            while (a<minspc && s->Get()[a]== ' ') a++;
            minspc = a;
          }
        }
        if (minspc>0 && minspc < -amt) amt = -minspc;
      }

      for (x = miny; x <= maxy; x ++)
      {
        WDL_FastString *s=m_text.Get(x);
        if (s)
        {
          if (amt>0)
          {
            int a;
            for(a=0;a<amt;a+=16)
              s->Insert("                  ",0,wdl_min(amt-a,16));
          }
          else if (amt<0)
          {
            int a=0;
            while (a<-amt && s->Get()[a]== ' ') a++;
            s->DeleteSub(0,a);
          }
        }
        if (x==m_select_y2) m_select_x2 = s ? s->GetLength() : 0;
      }
    }
  }
}

void WDL_CursesEditor::removeSelect()
{
  if (m_selecting)  // remove selected text
  {
    int miny,maxy,minx,maxx;
    int x;
    getselectregion(minx,miny,maxx,maxy);
    m_curs_x = minx;
    m_curs_y = miny;
    if (m_curs_y < 0) m_curs_y=0;
          
      if (minx != maxx|| miny != maxy) 
      {
        int fht=0,lht=0;
        for (x = miny; x <= maxy; x ++)
        {
          WDL_FastString *s=m_text.Get(x);
          if (s)
          {
            const int sx=x == miny ? WDL_utf8_charpos_to_bytepos(s->Get(),minx) : 0;
            const int ex=x == maxy ? WDL_utf8_charpos_to_bytepos(s->Get(),maxx) : s->GetLength();

            if (x != maxy && sx == 0 && ex == s->GetLength()) // remove entire line
            {
              m_text.Delete(x,true);
              if (x==miny) miny--;
              x--;
              maxy--;
            }
            else { if (x==miny) fht=1; if (x == maxy) lht=1; s->DeleteSub(sx,ex-sx); }
          }
        }
        if (fht && lht && miny+1 == maxy)
        {
          m_text.Get(miny)->Append(m_text.Get(maxy)->Get());
          m_text.Delete(maxy,true);
        }

      }
      m_selecting=0;
      if (m_curs_y >= m_text.GetSize())
      {
        m_curs_y = m_text.GetSize();
        m_text.Add(new WDL_FastString);
      }
    }

}

static const char *skip_indent(const char *tstr, int skipsz)
{
  while (*tstr == ' ' && skipsz-->0) tstr++;
  return tstr;
}

static WDL_FastString *newIndentedFastString(const char *tstr, int indent_to_pos)
{
  WDL_FastString *s=new WDL_FastString;
  if (indent_to_pos>=0) 
  {
    int x;
    for (x=0;x<indent_to_pos;x++) s->Append(" ");
  }
  s->Append(tstr);
  return s;

}

void WDL_CursesEditor::highlight_line(int line)
{ 
  if (line >= 0 && line <= m_text.GetSize())
  {
    bool nosel=false;
    if (line == m_text.GetSize()) { line--; nosel=true; }

    m_curs_x=0;
    m_curs_y=line;

    WDL_FastString* s=m_text.Get(line);
    if (s && s->GetLength())
    {
      if (nosel) m_curs_x = WDL_utf8_get_charlen(s->Get());
      else
      {
        m_select_x1=0;
        m_select_x2=WDL_utf8_get_charlen(s->Get());
        m_select_y1=m_select_y2=m_curs_y;
        m_selecting=1;
      }
      draw();
    }

    setCursor();
  }
}

void WDL_CursesEditor::GoToLine(int line, bool dosel)
{
  WDL_FastString *fs = m_text.Get(line);
  m_curs_y=line;
  m_select_x1=0;
  m_curs_x = m_select_x2 = WDL_NORMALLY(fs) ? WDL_utf8_get_charlen(fs->Get()) : 0;
  m_select_y1=m_select_y2=line;
  m_selecting=dosel?1:0;
  setCursor(0,0.25);
}

// tweak tokens to make them more useful in searching
static void tweak_tok(const char *tok, const char **np, int *len)
{
  int l = *len;
  if (WDL_NOT_NORMALLY(*np != tok+l)) return;
  if (WDL_NOT_NORMALLY(l < 1)) return;

  if (l == 1)
  {
    if ((tok[0] == '=' || tok[0] == '!' || tok[0] == '<' || tok[0] == '>') && tok[1] == '=')
    {
      l = (tok[0] == '=' || tok[0] == '!') && tok[2] == '=' ? 3 : 2;
      *np = tok + l;
      *len = l;
    }
  }
  else if (*tok == '.')
  {
    *len = 1;
    *np = tok + 1;
  }
  else
  {
    const char *p = tok;
    while (++p < tok+l)
    {
      if (*p == '.')
      {
        *len = (int) (p-tok);
        *np = p;
        break;
      }
    }
  }
}

int WDL_CursesEditor::search_line(const char *str, const WDL_FastString *line, int startx, bool backwards, int *match_len) // returns offset of next match, or -1 if none
{
  const char *p = line->Get();
  const int linelen = line->GetLength();
  const int srchlen = (int) strlen(str);

  if (s_search_mode == 4 || s_search_mode == 5)
  {
    const char *lptr = p;
    int best_match = -1, best_match_len = 0, lstate = 0;
    for (;;)
    {
      int llen=0;
      const char *ltok = sh_tokenize(&lptr, p + linelen, &llen,&lstate);
      if (!ltok) break;
      if (!lstate) tweak_tok(ltok,&lptr,&llen);
      const int ltok_offs = (int) (ltok - p);
      if (backwards && ltok_offs > startx) break;

      // see if the sequence of tokens at ltok matches
      const char *aptr = str, *bptr = ltok;
      int astate=0, bstate=lstate;
      int last_endtok_offs = ltok_offs;
      for (;;)
      {
        int alen=0, blen=0;
        const char *atok = sh_tokenize(&aptr, str+srchlen, &alen,&astate);

        if (!atok) // end of search term
        {
          if (backwards || ltok_offs >= startx)
          {
            best_match = ltok_offs;
            best_match_len = last_endtok_offs - ltok_offs;
          }
          break;
        }

        const char *btok = sh_tokenize(&bptr, p+linelen, &blen, &bstate);
        if (!btok) break; // end of line without match

        if (!astate) tweak_tok(atok,&aptr,&alen);
        if (!bstate) tweak_tok(btok,&bptr,&blen);

        if (alen != blen ||
            ((s_search_mode&1) ? strncmp(atok,btok,alen) : strnicmp(atok,btok,alen)))
          break;

        last_endtok_offs = (int) (btok + blen - p);
      }
      if (!backwards && best_match>=0) break;
    }
    if (match_len) *match_len = best_match_len;
    return best_match;
  }
  if (match_len) *match_len = srchlen;

  const int dstartx = backwards?-1:1;
  for (; backwards ? (startx>=0) : (startx <= linelen-srchlen); startx+=dstartx)
    if ((s_search_mode&1) ? !strncmp(p+startx,str,srchlen) : !strnicmp(p+startx,str,srchlen))
    {
      if (!(s_search_mode&2)) return startx;
      if ((startx==0 || p[startx-1]<0 || !isalnum(p[startx-1])) &&
          (p[startx+srchlen]<=0 || !isalnum(p[startx+srchlen]))) return startx;
    }
  return -1;
}

static const char *ellipsify(const char *str, char *buf, int bufsz)
{
  int str_len = (int) strlen(str);
  if (str_len < bufsz-8 || bufsz < 8) return str;
  int l1 = 0;
  while (l1 < bufsz/2)
  {
    if (WDL_NOT_NORMALLY(!str[l1])) return str;
    int sz=wdl_utf8_parsechar(str+l1,NULL);
    l1+=sz;
  }
  int l2 = l1;
  while (l1 + (str_len - l2) > bufsz-4)
  {
    if (WDL_NOT_NORMALLY(!str[l2])) return str;
    int sz=wdl_utf8_parsechar(str+l2,NULL);
    l2+=sz;
  }
  snprintf(buf,bufsz,"%.*s...%s",l1,str,str+l2);
  return buf;
}

void WDL_CursesEditor::runSearch(bool backwards, bool replaceAll)
{
  char buf[512];
  buf[0]=0;
  if (replaceAll && m_search_string.GetLength())
  {
    preSaveUndoState();
    int cnt = 0, linecnt = 0;
    const int numlines = m_text.GetSize();
    for (int line = 0; line < numlines; line ++)
    {
      WDL_FastString *tl = m_text.Get(line);
      if (WDL_NOT_NORMALLY(!tl)) continue;

      bool repl = false;
      int startx = 0;

      while (startx < tl->GetLength())
      {
        int matchlen=0;
        int bytepos = search_line(m_search_string.Get(),tl, startx, false, &matchlen);
        if (bytepos < 0) break;

        int tbp, cursx_bytepos = -1, selx1_bytepos=-1, selx2_bytepos=-1;
#define PRECHK_PAIR(py, px, bpv) \
        if ((py) == line && (tbp=WDL_utf8_charpos_to_bytepos(tl->Get(),(px))) >= bytepos) { \
          if (tbp < bytepos + matchlen) (px) = WDL_utf8_bytepos_to_charpos(tl->Get(),bytepos); \
          else (bpv) = tbp; \
        }

        PRECHK_PAIR(m_curs_y,m_curs_x,cursx_bytepos)
        PRECHK_PAIR(m_select_y1,m_select_x1,selx1_bytepos)
        PRECHK_PAIR(m_select_y2,m_select_x2,selx2_bytepos)

        tl->DeleteSub(bytepos,matchlen);
        tl->Insert(m_replace_string.Get(),bytepos);

#define POSTCHK_PAIR(px, bpv) \
        if ((bpv) >= 0) (px) = WDL_utf8_bytepos_to_charpos(tl->Get(),(bpv) - matchlen + m_replace_string.GetLength());
        POSTCHK_PAIR(m_curs_x,cursx_bytepos)
        POSTCHK_PAIR(m_select_x1,selx1_bytepos);
        POSTCHK_PAIR(m_select_x2,selx2_bytepos);
#undef PRECHK_PAIR
#undef POSTCHK_PAIR

        startx = bytepos + m_replace_string.GetLength();
        repl = true;
        cnt++;
      }
      if (repl) linecnt++;
    }
    if (cnt)
      snprintf(buf,sizeof(buf),"Replaced %d instance%s on %d line%s",cnt, cnt==1?"":"s", linecnt, linecnt==1?"":"s");
    saveUndoState();
  }
  else if (m_search_string.GetLength())
  {
    char elbuf[50];
    const int numlines = m_text.GetSize();
    if (numlines) for (int y = 0; y <= numlines; y ++)
    {
      int line = m_curs_y + (backwards ? -y : y), wrapflag=0;
      if (line >= numlines) { line -= numlines; wrapflag = 1; }
      else if (line < 0) { line += numlines; wrapflag = 1; }

      WDL_FastString *tl = m_text.Get(line);
      if (WDL_NOT_NORMALLY(!tl)) continue;

      const int startx = y==0 ? (backwards ? m_curs_x-1 : m_curs_x+1) : (backwards ? tl->GetLength()-1 : 0);
      if (backwards && startx<0) continue;

      int matchlen=0;
      int bytepos = search_line(m_search_string.Get(),tl, startx > 0 ? WDL_utf8_charpos_to_bytepos(tl->Get(),startx) : 0, backwards, &matchlen);
      if (bytepos >= 0)
      {
        m_select_y1=m_select_y2=m_curs_y=line;
        m_select_x1=m_curs_x=WDL_utf8_bytepos_to_charpos(tl->Get(),bytepos);
        m_select_x2=WDL_utf8_bytepos_to_charpos(tl->Get(),bytepos+matchlen);
        m_selecting=1;

        // make sure the end is on screen
        m_offs_x = 0;
        m_curs_x = wdl_max(m_select_x1,m_select_x2) + COLS/4;
        setCursor();

        m_curs_x = m_select_x1;
        snprintf(buf,sizeof(buf),"Found @ Line %d Col %d %s'%s' (Shift+)F3|" CONTROL_KEY_NAME "+G:(prev)next",m_curs_y+1,m_curs_x+1,wrapflag?"(wrapped) ":"",ellipsify(m_search_string.Get(),elbuf,sizeof(elbuf)));
        break;
      }
    }
    if (!buf[0])
      snprintf(buf,sizeof(buf),"%s '%s' not found",searchmode_desc(s_search_mode),ellipsify(m_search_string.Get(),elbuf,sizeof(elbuf)));
  }

  draw();
  setCursor();
  if (buf[0])
    draw_message(buf);
}

static int categorizeCharForWordNess(int c)
{
  if (c >= 0 && c < 256)
  {
    if (isspace(c)) return 0;
    if (isalnum(c) || c == '_') return 1;
    if (c == ';') return 2; // I prefer this, since semicolons are somewhat special
  }
  return 3;
}

#define CTRL_KEY_DOWN (GetAsyncKeyState(VK_CONTROL)&0x8000)
#define SHIFT_KEY_DOWN (GetAsyncKeyState(VK_SHIFT)&0x8000)
#define ALT_KEY_DOWN (GetAsyncKeyState(VK_MENU)&0x8000)


void WDL_CursesEditor::getLinesFromClipboard(WDL_FastString &buf, WDL_PtrList<const char> &lines)
{
#ifdef WDL_IS_FAKE_CURSES
  if (CURSES_INSTANCE)
  {
    HANDLE h;
    OpenClipboard(CURSES_INSTANCE->m_hwnd);
#ifdef CF_UNICODETEXT
    h=GetClipboardData(CF_UNICODETEXT);
    if (h)
    {
      wchar_t *t=(wchar_t *)GlobalLock(h);
      int s=(int)(GlobalSize(h)/2);
      while (s-- > 0)
      {
        char b[32];
        if (!*t) break;
        WDL_MakeUTFChar(b,*t++,sizeof(b));
        buf.Append(b);
      }

      GlobalUnlock(t);
    }

#endif
    if (!buf.GetLength())
    {
      h=GetClipboardData(CF_TEXT);
      if (h)
      {
        char *t=(char *)GlobalLock(h);
        int s=(int)(GlobalSize(h));
        buf.Set(t,s);
        GlobalUnlock(t);
      }
    }
    CloseClipboard();
  }
  else
#endif
  {
    buf.Set(s_fake_clipboard.Get());
  }

  if (buf.Get() && buf.Get()[0])
  {
    ReplaceTabs(&buf,m_indent_size);

    char *src=(char*)buf.Get();
    while (*src)
    {
      char *seek=src;
      while (*seek && *seek != '\r' && *seek != '\n') seek++;
      char hadclr=*seek;
      if (*seek) *seek++=0;
      lines.Add(src);

      if (hadclr == '\r' && *seek == '\n') seek++;

      if (hadclr && !*seek)
      {
        lines.Add("");
      }
      src=seek;
    }
  }
}

void WDL_CursesEditor::run_line_editor(int c, WDL_FastString *fs)
{
  char tmp[512];
  switch (c)
  {
    case -1: break;
    case 0:
      m_line_editor_edited=false;
      draw_top_line();
    break;
    case 27:
      draw();
      setCursor();
      // fallthrough
    case '\r': case '\n':
      m_ui_state=UI_STATE_NORMAL;
    return;
    case KEY_BACKSPACE:
      if (!fs->GetLength()) return;

      {
        const char *p = fs->Get();
        for (int x = 0;;)
        {
          int sz=wdl_utf8_parsechar(p+x,NULL);
          if (!p[x+sz])
          {
            fs->SetLen(x);
            m_line_editor_edited=true;
            break;
          }
          x+=sz;
        }
      }
    break;
    case KEY_IC:
      if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN) return;
    case 'V'-'A'+1:
      {
        WDL_PtrList<const char> lines;
        WDL_FastString buf;
        getLinesFromClipboard(buf,lines);
        if (!lines.Get(0)) return;
        if (!m_line_editor_edited)
        {
          fs->Set("");
          m_line_editor_edited=true;
        }
        fs->Append(lines.Get(0));
      }
    break;
    case KEY_UP:
      if (m_ui_state == UI_STATE_REPLACE || m_ui_state == UI_STATE_SEARCH)
      {
        if (--s_search_mode<0) s_search_mode=5;
      }
    break;
    case KEY_DOWN:
      if (m_ui_state == UI_STATE_REPLACE || m_ui_state == UI_STATE_SEARCH)
      {
        if (++s_search_mode>5) s_search_mode=0;
      }
    break;
    default:
      if (!VALIDATE_TEXT_CHAR(c)) return;

      if (!m_line_editor_edited) fs->Set("");
      m_line_editor_edited=true;
      WDL_MakeUTFChar(tmp,c,sizeof(tmp));
      fs->Append(tmp);
    break;
  }

  const char *search_help = COLS > 70 ? " (Up/Down:mode " CONTROL_KEY_NAME "+R:replace ESC:cancel)" :
                            COLS > 40 ? "(U/D:mode " CONTROL_KEY_NAME "+R:repl)" :
                            "";
  char elbuf[50];
  switch (m_ui_state)
  {
    case UI_STATE_REPLACE:
      snprintf(tmp,sizeof(tmp),"Replace all (%s) '%s': ",searchmode_desc(s_search_mode),ellipsify(m_search_string.Get(),elbuf,sizeof(elbuf)));
    break;
    case UI_STATE_SEARCH:
      snprintf(tmp,sizeof(tmp),"Find %s%s: ",searchmode_desc(s_search_mode), search_help);
    break;
    case UI_STATE_GOTO_LINE:
      lstrcpyn_safe(tmp, "Go to line (ESC:cancel): ",sizeof(tmp));
    break;
    default:
    return;
  }

  attrset(COLOR_MESSAGE);
  bkgdset(COLOR_MESSAGE);
  mvaddstr(LINES-1,0,tmp);
  addstr(fs->Get());
  clrtoeol();
  attrset(0);
  bkgdset(0);
}

int WDL_CursesEditor::onChar(int c)
{
  switch (m_ui_state)
  {
    case UI_STATE_MESSAGE:
      m_ui_state=UI_STATE_NORMAL;
      draw();
      setCursor();
    break;
    case UI_STATE_SAVE_ON_CLOSE:
      if (c>=0 && (isalnum(c) || isprint(c) || c==27))
      {
        if (c == 27)
        {
          m_ui_state=UI_STATE_NORMAL;
          draw();
          draw_message("Cancelled close of file.");
          setCursor();
          return 0;
        }
        if (toupper(c) == 'N' || toupper(c) == 'Y')
        {
          if (toupper(c) == 'Y')
          {
            if(updateFile())
            {
              m_ui_state=UI_STATE_NORMAL;
              draw();
              draw_message("Error writing file, changes not saved!");
              setCursor();
              return 0;
            }
          }
          CloseCurrentTab();

          delete this;
          // this no longer valid, return 1 to avoid future calls in onChar()

          return 1;
        }
      }
    return 0;
    case UI_STATE_SAVE_AS_NEW:
      if (c>=0 && (isalnum(c) || isprint(c) || c==27 || c == '\r' || c=='\n'))
      {
        m_ui_state=UI_STATE_NORMAL;
        if (toupper(c) == 'N' || c == 27)
        {
          draw();
          draw_message("Cancelled create new file.");
          setCursor();
          return 0;
        }

        AddTab(m_newfn.Get());
      }
    return 0;
    case UI_STATE_SEARCH:
    case UI_STATE_REPLACE:
      {
        uiState chgstate = c == 1+'R'-'A' ? UI_STATE_REPLACE : c == 1+'F'-'A' ? UI_STATE_SEARCH : UI_STATE_NORMAL;
        if (chgstate != UI_STATE_NORMAL)
        {
          if (chgstate == UI_STATE_REPLACE && !m_search_string.GetLength()) chgstate = UI_STATE_SEARCH;
          c = m_ui_state == chgstate ? 27 : 0;
          m_ui_state = chgstate;
        }
        const bool is_replace = m_ui_state == UI_STATE_REPLACE;
        run_line_editor(c,is_replace ? &m_replace_string : &m_search_string);
        if (c == '\r' || c == '\n') runSearch(false, is_replace);
      }
    return 0;
    case UI_STATE_GOTO_LINE:
      if (c == 1+'J'-'A') c=27;
      run_line_editor(c,&s_goto_line_buf);
      if (c == '\r' || c == '\n')
      {
        const char *p = s_goto_line_buf.Get();
        while (*p == ' ' || *p == '\t') p++;
        int rel = 0;
        if (*p == '+') rel=1;
        else if (*p == '-') rel=-1;
        if (rel) p++;
        while (*p == ' ' || *p == '\t') p++;
        int a = atoi(p);
        if (a > 0)
        {
          if (rel) a = m_curs_y + a*rel;
          else a--;

          m_curs_y = wdl_clamp(a,0,m_text.GetSize());
          WDL_FastString *fs = m_text.Get(m_curs_y);
          m_curs_x = fs ? WDL_utf8_get_charlen(fs->Get()) : 0;
          m_selecting=0;
          draw();
          setCursor(0,-1.0);
        }
        else
        {
          draw();
          setCursor();
        }
      }
    return 0;
    case UI_STATE_NORMAL:
    break;
  }

  // UI_STATE_NORMAL
  WDL_ASSERT(m_ui_state == UI_STATE_NORMAL /* unhandled state? */);

  if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN && c =='W'-'A'+1)
  {
    if (GetTab(0) == this) return 0; // first in list = do nothing

    if (IsDirty())
    {
      m_ui_state=UI_STATE_SAVE_ON_CLOSE;
      attrset(COLOR_MESSAGE);
      bkgdset(COLOR_MESSAGE);
      mvaddstr(LINES-1,0,"Save file before closing (y/N)? ");
      clrtoeol();
      attrset(0);
      bkgdset(0);
    }
    else
    {
      CloseCurrentTab();

      delete this;
      // context no longer valid!
      return 1;
    }
    return 0;
  }
  if ((c==27 || c==29 || (c >= KEY_F1 && c<=KEY_F10)) && CTRL_KEY_DOWN)
  {
    int idx=c-KEY_F1;
    bool rel=true;
    if (c==27) idx=-1;
    else if (c==29) idx=1;
    else rel=false;
    SwitchTab(idx,rel);
    return 1;
  }

  if (c==KEY_DOWN || c==KEY_UP || c==KEY_PPAGE||c==KEY_NPAGE || c==KEY_RIGHT||c==KEY_LEFT||c==KEY_HOME||c==KEY_END)
  {
    if (SHIFT_KEY_DOWN)
    {
      if (!m_selecting)
      {
        m_select_x2=m_select_x1=m_curs_x; m_select_y2=m_select_y1=m_curs_y;
        m_selecting=1;
      }
    }
    else if (m_selecting) { m_selecting=0; draw(); }
  }

  switch(c)
  {
    case 'O'-'A'+1:
      if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN)
      {
        if (m_pane_div <= 0.0 || m_pane_div >= 1.0)
        {
          onChar('P'-'A'+1);
        }
        if (m_pane_div > 0.0 && m_pane_div < 1.0) 
        {
          m_curpane=!m_curpane;
          draw();
          draw_status_state();
          int paney[2], paneh[2];
          GetPaneDims(paney, paneh);
          if (m_curs_y-m_paneoffs_y[m_curpane] < 0) m_curs_y=m_paneoffs_y[m_curpane];
          else if (m_curs_y-m_paneoffs_y[m_curpane] >= paneh[m_curpane]) m_curs_y=paneh[m_curpane]+m_paneoffs_y[m_curpane]-1;
          setCursor();
        }
      }
    break;
    case 'P'-'A'+1:
      if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN)
      {
        if (m_pane_div <= 0.0 || m_pane_div >= 1.0) 
        {
          m_pane_div=0.5;
          m_paneoffs_y[1]=m_paneoffs_y[0];
        }
        else 
        {
          m_pane_div=1.0;
          if (m_curpane) m_paneoffs_y[0]=m_paneoffs_y[1];
          m_curpane=0;
        }
        draw();
        draw_status_state();

        int paney[2], paneh[2];
        GetPaneDims(paney, paneh);
        setCursor();
      }
    break;
    
    case 407:
    case 'Z'-'A'+1:
      if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN)
      {
        if (m_undoStack_pos > 0)
        {
           m_undoStack_pos--;
           loadUndoState(m_undoStack.Get(m_undoStack_pos),1);
           draw();
           setCursor();
           char buf[512];
           snprintf(buf,sizeof(buf),"Undid action - %d items in undo buffer",m_undoStack_pos);
           draw_message(buf);
        }
        else 
        {
          draw_message("Can't Undo");
        }   
        break;
      }
    // fall through
    case 'Y'-'A'+1:
      if ((c == 'Z'-'A'+1 || !SHIFT_KEY_DOWN) && !ALT_KEY_DOWN)
      {
        if (m_undoStack_pos < m_undoStack.GetSize()-1)
        {
          m_undoStack_pos++;
          loadUndoState(m_undoStack.Get(m_undoStack_pos),0);
          draw();
          setCursor();
          char buf[512];
          snprintf(buf,sizeof(buf),"Redid action - %d items in redo buffer",m_undoStack.GetSize()-m_undoStack_pos-1);
          draw_message(buf);
        }
        else 
        {
          draw_message("Can't Redo");  
        }
      }
    break;
    case KEY_IC:
      if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN)
      {
        s_overwrite=!s_overwrite;
        setCursor();
        break;
      }
      // fall through
    case 'V'-'A'+1:
      if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN)
      {
        // generate a m_clipboard using win32 clipboard data
        WDL_PtrList<const char> lines;
        WDL_FastString buf;
        getLinesFromClipboard(buf,lines);

        if (lines.GetSize())
        {
          removeSelect();
          // insert lines at m_curs_y,m_curs_x
          if (m_curs_y > m_text.GetSize()) m_curs_y=m_text.GetSize();
          if (m_curs_y < 0) m_curs_y=0;

          preSaveUndoState();

          do_paste_lines(lines);
          draw();
          setCursor();
          draw_message("Pasted");
          saveUndoState();
        }
        else 
        {
          setCursor();
          draw_message("Clipboard empty");
        }
      }
  break;

  case KEY_DC:
    if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN)
    {
      WDL_FastString *s;
      if (m_selecting)
      {
        preSaveUndoState();
        removeSelect();
        draw();
        saveUndoState();
        setCursor();
      }
      else if ((s=m_text.Get(m_curs_y)))
      {
        const int xbyte = WDL_utf8_charpos_to_bytepos(s->Get(),m_curs_x);
        if (xbyte < s->GetLength())
        {
          preSaveUndoState();

          const int xbytesz=WDL_utf8_charpos_to_bytepos(s->Get()+xbyte,1);
          bool hadCom = LineCanAffectOtherLines(s->Get(),xbyte,xbytesz); 
          s->DeleteSub(xbyte,xbytesz);
          if (!hadCom) hadCom = LineCanAffectOtherLines(s->Get(),xbyte,0);
          draw(hadCom ? -1 : m_curs_y);
          saveUndoState();
          setCursor();
        }
        else // append next line to us
        {
          if (m_curs_y < m_text.GetSize()-1)
          {
            preSaveUndoState();

            WDL_FastString *nl=m_text.Get(m_curs_y+1);
            if (nl)
            {
              const char *p = nl->Get();
              while ((*p == ' ' || *p == '\t') && (p[1] == ' ' || p[1] == '\t')) p++;
              s->Append(p);
            }
            m_text.Delete(m_curs_y+1,true);

            draw();
            saveUndoState();
            setCursor();
          }
        }
      }
      break;
    }
  case 'C'-'A'+1:
  case 'X'-'A'+1:
    if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN && m_selecting)
    {
      if (c!= 'C'-'A'+1) m_selecting=0;
      int miny,maxy,minx,maxx;
      int x;
      getselectregion(minx,miny,maxx,maxy);
      const char *status="";
      char statusbuf[512];

      if (minx != maxx|| miny != maxy) 
      {
        s_fake_clipboard.Set("");

        int lht=0,fht=0;
        if (c != 'C'-'A'+1) preSaveUndoState();

        for (x = miny; x <= maxy; x ++)
        {
          WDL_FastString *s=m_text.Get(x);
          if (s) 
          {
            const char *str=s->Get();
            int sx=x == miny ? WDL_utf8_charpos_to_bytepos(s->Get(),minx) : 0;
            const int ex=x == maxy ? WDL_utf8_charpos_to_bytepos(s->Get(),maxx) : s->GetLength();
      
            if (x != miny) s_fake_clipboard.Append("\r\n");

            const int oldlen = s_fake_clipboard.GetLength();
            if (x == miny && maxy > miny && sx > 0)
            {
              // first line of multi-line copy, not starting at first byte of line
              int icnt = 0;
              while (str[icnt] == ' ') icnt++;

              for (int nl = x+1; icnt > 0 && nl <= maxy; nl ++)
              {
                WDL_FastString *cs = m_text.Get(nl);
                if (cs)
                {
                  int c = 0;
                  while (c < icnt && cs->Get()[c] == ' ') c++;
                  if (c < icnt && cs->Get()[c])
                  {
                    if (nl == maxy && c >= WDL_utf8_charpos_to_bytepos(cs->Get(),maxx)) break; // last line, ignore if whitespace exceeds what we're copying anyway
                    icnt = 0;
                  }
                }
              }

              // if all later copied lines do not have non-whitespace in the first line's indentation
              if (icnt > 0)
              {
                // then we'll copy that line's indentation
                s_fake_clipboard.Append(str,icnt);
                // and skip any whitespace in our selection
                while (str[sx] == ' ') sx++;
              }
            }

            if (ex>sx) s_fake_clipboard.Append(str+sx,ex-sx);

            if (m_write_leading_tabs>0 && sx==0 && oldlen < s_fake_clipboard.GetLength())
            {
              const char *p = s_fake_clipboard.Get() + oldlen;
              int nt=0;
              const char *sp = countLeadingTabs(p,&nt,m_write_leading_tabs);
              if (nt && sp > p)
              {
                s_fake_clipboard.DeleteSub(oldlen,(int)(sp-p));
                while (nt>0) 
                {
                  int c = nt;
                  if (c > 8) c=8;
                  nt -= c;
                  s_fake_clipboard.Insert("\t\t\t\t\t\t\t\t",oldlen,c);
                }
              }
            }

            if (c != 'C'-'A'+1)
            {
              if (sx == 0 && ex == s->GetLength()) // remove entire line
              {
                m_text.Delete(x,true);
                if (x==miny) miny--;
                x--;
                maxy--;
              }
              else { if (x==miny) fht=1; if (x == maxy) lht=1; s->DeleteSub(sx,ex-sx); }
            }
          }
        }
        if (fht && lht && miny+1 == maxy)
        {
          m_text.Get(miny)->Append(m_text.Get(maxy)->Get());
          m_text.Delete(maxy,true);
        }
        if (c != 'C'-'A'+1)
        {
          m_curs_y=miny;
          if (m_curs_y < 0) m_curs_y=0;
          m_curs_x=minx;
          saveUndoState();
          snprintf(statusbuf,sizeof(statusbuf),"Cut %d bytes",s_fake_clipboard.GetLength());
        }
        else
          snprintf(statusbuf,sizeof(statusbuf),"Copied %d bytes",s_fake_clipboard.GetLength());

#ifdef WDL_IS_FAKE_CURSES
        if (CURSES_INSTANCE)
        {
#ifdef CF_UNICODETEXT
          const int l=(WDL_utf8_get_charlen(s_fake_clipboard.Get())+1)*sizeof(wchar_t);
          HANDLE h=GlobalAlloc(GMEM_MOVEABLE,l);
          wchar_t *t=(wchar_t*)GlobalLock(h);
          if (t)
          {
            WDL_MBtoWideStr(t,s_fake_clipboard.Get(),l);
            GlobalUnlock(h);
          }
          OpenClipboard(CURSES_INSTANCE->m_hwnd);
          EmptyClipboard();
          SetClipboardData(CF_UNICODETEXT,h);
#else
          int l=s_fake_clipboard.GetLength()+1;
          HANDLE h=GlobalAlloc(GMEM_MOVEABLE,l);
          void *t=GlobalLock(h);
          if (t)
          {
            memcpy(t,s_fake_clipboard.Get(),l);
            GlobalUnlock(h);
          }
          OpenClipboard(CURSES_INSTANCE->m_hwnd);
          EmptyClipboard();
          SetClipboardData(CF_TEXT,h);
#endif
          CloseClipboard();
        }
#endif

        status=statusbuf;
      }
      else status="No selection";

      draw();
      setCursor();
      draw_message(status);
    }
  break;
  case 'D'-'A'+1:
    if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN)
    {
      if (m_selecting)
      {
        int miny,maxy,minx,maxx;
        getselectregion(minx,miny,maxx,maxy);

        if (minx != maxx|| miny != maxy) 
        {
          WDL_FastString dup;

          int x;
          for (x = miny; x <= maxy; x ++)
          {
            WDL_FastString *s=m_text.Get(x);
            if (s) 
            {
              const char *str=s->Get();
              const int sx=x == miny ? WDL_utf8_charpos_to_bytepos(str,minx) : 0;
              const int ex=x == maxy ? WDL_utf8_charpos_to_bytepos(str,maxx) : s->GetLength();
              if (dup.GetLength()) dup.Append("\n");
              dup.Append(ex-sx?str+sx:"",ex-sx);
            }
          }

          if (dup.GetLength())
          {
            preSaveUndoState();
            WDL_PtrList<const char> lines;
            char *p = (char *)dup.Get();
            for (;;)
            {
              const char *basep = p;
              while (*p && *p != '\n') p++;
              lines.Add(basep);
              if (!*p) break;
              *p++=0;
            }
            m_curs_x=maxx;
            m_curs_y=maxy;
            do_paste_lines(lines);
            m_curs_x=maxx;
            m_curs_y=maxy;

            draw();
            setCursor();
            draw_message("Duplicated selection");
            saveUndoState();
          }
        }
      }
      else if (m_text.Get(m_curs_y))
      {
        preSaveUndoState();
        m_text.Insert(m_curs_y, new WDL_FastString(m_text.Get(m_curs_y)));
        draw();
        setCursor();
        draw_message("Duplicated line");
        saveUndoState();
      }
    }
  break;

  case 'A'-'A'+1:
    if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN)
    {
      m_selecting=1;
      m_select_x1=0;
      m_select_y1=0;
      m_select_y2=m_text.GetSize()-1;
      m_select_x2=0;
      if (m_text.Get(m_select_y2))
        m_select_x2=m_text.Get(m_select_y2)->GetLength();
      draw();
      setCursor();
    }
  break;
  case 27:
    if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN && m_selecting)
    {
      m_selecting=0;
      draw();
      setCursor();
      break;
    }
  break;
  case KEY_F3:
  case 'G'-'A'+1:
    if (!ALT_KEY_DOWN && m_search_string.GetLength())
    {
      runSearch(SHIFT_KEY_DOWN != 0, false);
      return 0;
    }
  // fall through
  case 'R'-'A'+1:
  case 'F'-'A'+1:
    if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN)
    {
      if (m_selecting && m_select_y1==m_select_y2)
      {
        WDL_FastString* s=m_text.Get(m_select_y1);
        if (s)
        {
          const char* p=s->Get();
          int xlo=wdl_min(m_select_x1, m_select_x2);
          int xhi=wdl_max(m_select_x1, m_select_x2);
          xlo = WDL_utf8_charpos_to_bytepos(p,xlo);
          xhi = WDL_utf8_charpos_to_bytepos(p,xhi);
          if (xhi > xlo)
          {
            m_search_string.Set(p+xlo, xhi-xlo);
          }
        }
      }
      if (c == 'R'-'A'+1 && !m_search_string.GetLength())
      {
        draw_message("Use " CONTROL_KEY_NAME "+F first, or run " CONTROL_KEY_NAME "+R from Find prompt");
      }
      else
      {
        m_ui_state=c == 'R'-'A'+1 ? UI_STATE_REPLACE : UI_STATE_SEARCH;
        run_line_editor(0,m_ui_state == UI_STATE_REPLACE ? &m_replace_string : &m_search_string);
      }
    }
  break;
  case KEY_DOWN:
    {
      if (CTRL_KEY_DOWN)
      {
        int paney[2], paneh[2];
        GetPaneDims(paney, paneh);
        int maxscroll=m_text.GetSize()-paneh[m_curpane]+4;
        if (m_paneoffs_y[m_curpane] < maxscroll-1)
        {
          m_paneoffs_y[m_curpane]++;
          if (m_curs_y < m_paneoffs_y[m_curpane]) m_curs_y=m_paneoffs_y[m_curpane];
          draw();
        }
      }
      else
      {
        m_curs_y++;
        if (m_curs_y>=m_text.GetSize()) m_curs_y=m_text.GetSize()-1;
        if (m_curs_y < 0) m_curs_y=0;
      }
      if (m_selecting) { setCursor(1); m_select_x2=m_curs_x; m_select_y2=m_curs_y; draw(); }
      setCursor(1);
    }
  break;
  case KEY_UP:
    {
      if (CTRL_KEY_DOWN)
      {
        if (m_paneoffs_y[m_curpane] > 0)
        {
          int paney[2], paneh[2];
          GetPaneDims(paney, paneh);
          m_paneoffs_y[m_curpane]--;
          if (m_curs_y >  m_paneoffs_y[m_curpane]+paneh[m_curpane]-1) m_curs_y = m_paneoffs_y[m_curpane]+paneh[m_curpane]-1;
          if (m_curs_y < 0) m_curs_y=0;
          draw();
        }
      }
      else
      {
        if(m_curs_y>0) m_curs_y--;
      }
      if (m_selecting) { setCursor(1); m_select_x2=m_curs_x; m_select_y2=m_curs_y; draw(); }
      setCursor(1);
    }
  break;
  case KEY_PPAGE:
    {
      if (m_curs_y > m_paneoffs_y[m_curpane])
      {
        m_curs_y=m_paneoffs_y[m_curpane];
        if (m_curs_y < 0) m_curs_y=0;
      }
      else 
      {
        int paney[2], paneh[2];
        GetPaneDims(paney, paneh);
        m_curs_y -= paneh[m_curpane];
        if (m_curs_y < 0) m_curs_y=0;
        m_paneoffs_y[m_curpane]=m_curs_y;
      }
      if (m_selecting) { setCursor(1); m_select_x2=m_curs_x; m_select_y2=m_curs_y; }
      draw();
      setCursor(1);
    }
  break; 
  case KEY_NPAGE:
    {
      int paney[2], paneh[2]; 
      GetPaneDims(paney, paneh);
      if (m_curs_y >= m_paneoffs_y[m_curpane]+paneh[m_curpane]-1) m_paneoffs_y[m_curpane]=m_curs_y-1;
      m_curs_y = m_paneoffs_y[m_curpane]+paneh[m_curpane]-1;
      if (m_curs_y >= m_text.GetSize()) m_curs_y=m_text.GetSize()-1;
      if (m_curs_y < 0) m_curs_y=0;
      if (m_selecting) { setCursor(1); m_select_x2=m_curs_x; m_select_y2=m_curs_y; }
      draw();
      setCursor(1);
    }
  break;
  case KEY_RIGHT:
    {
      if (1) // wrap across lines
      {
        WDL_FastString *s = m_text.Get(m_curs_y);
        if (s && m_curs_x >= WDL_utf8_get_charlen(s->Get()) && m_curs_y < m_text.GetSize()) { m_curs_y++; m_curs_x = -1; }
      }

      if(m_curs_x<0) 
      {
        m_curs_x=0;
      }
      else
      {
        if (CTRL_KEY_DOWN)
        {
          WDL_FastString *s = m_text.Get(m_curs_y);
          if (!s) break;
          int bytepos = WDL_utf8_charpos_to_bytepos(s->Get(),m_curs_x);

          if (bytepos >= s->GetLength()) break;
          int lastType = categorizeCharForWordNess(s->Get()[bytepos++]);
          while (bytepos < s->GetLength())
          {
            int thisType = categorizeCharForWordNess(s->Get()[bytepos]);
            if (thisType != lastType && thisType != 0) break;
            lastType=thisType;
            bytepos++;
          }
          m_curs_x = WDL_utf8_bytepos_to_charpos(s->Get(),bytepos);
        }
        else 
        {
          m_curs_x++;
        }
      }
      if (m_selecting) { setCursor(); m_select_x2=m_curs_x; m_select_y2=m_curs_y; draw(); }
      setCursor();
    }
  break;
  case KEY_LEFT:
    {
      bool doMove=true;
      if (1) // wrap across lines
      {
        WDL_FastString *s = m_text.Get(m_curs_y);
        if (s && m_curs_y>0 && m_curs_x == 0) 
        { 
          s = m_text.Get(--m_curs_y);
          if (s) 
          {
            m_curs_x = WDL_utf8_get_charlen(s->Get());
            doMove=false;
          }
        }
      }

      if(m_curs_x>0 && doMove) 
      {
        if (CTRL_KEY_DOWN)
        {
          WDL_FastString *s = m_text.Get(m_curs_y);
          if (!s) break;
          int bytepos = WDL_utf8_charpos_to_bytepos(s->Get(),m_curs_x);
          if (bytepos > s->GetLength()) bytepos = s->GetLength();
          bytepos--;

          int lastType = categorizeCharForWordNess(s->Get()[bytepos--]);
          while (bytepos >= 0)
          {
            int thisType = categorizeCharForWordNess(s->Get()[bytepos]);
            if (thisType != lastType && lastType != 0) break;
            lastType=thisType;
            bytepos--;
          }
          m_curs_x = WDL_utf8_bytepos_to_charpos(s->Get(),bytepos+1);
        }
        else 
        {
          m_curs_x--;
        }
      }
      if (m_selecting) { setCursor(); m_select_x2=m_curs_x; m_select_y2=m_curs_y; draw(); }
      setCursor();
    }
  break;
  case KEY_HOME:
    {
      const int old_x = m_curs_x;
      m_curs_x=0;
      if (!CTRL_KEY_DOWN)
      {
        const WDL_FastString *ln = m_text.Get(m_curs_y);
        if (ln)
        {
          int i = 0;
          while (ln->Get()[i] == ' ' || ln->Get()[i] == '\t') i++;
          if (ln->Get()[i] && i != old_x) m_curs_x = i;
        }
      }
      else m_curs_y=0;

      if (m_selecting) { setCursor(); m_select_x2=m_curs_x; m_select_y2=m_curs_y; draw(); }
      m_want_x=-1; // clear in case we're already at the start of the line
      setCursor();
    }
  break;
  case KEY_END:
    {
      if (CTRL_KEY_DOWN) m_curs_y=wdl_max(m_text.GetSize()-1,0);
      const WDL_FastString *ln = m_text.Get(m_curs_y);
      if (ln) m_curs_x=WDL_utf8_get_charlen(ln->Get());
      if (m_selecting) { setCursor(); m_select_x2=m_curs_x; m_select_y2=m_curs_y; draw(); }
      m_want_x=-1; // clear in case we're already at the end of the line
      setCursor();
    }
  break;
  case KEY_BACKSPACE: // backspace, baby
    if (m_selecting)
    {
      preSaveUndoState();
      removeSelect();
      draw();
      saveUndoState();
      setCursor();
    }
    else if (m_curs_x > 0)
    {
      WDL_FastString *tl=m_text.Get(m_curs_y);
      if (tl)
      {
        preSaveUndoState();

        int del_sz=1;
        if (m_indent_size > 0 && !(m_curs_x % m_indent_size))
        {
          const char *p = tl->Get();
          int i=0;
          while (i<m_curs_x && p[i]== ' ') i++;
          if (i == m_curs_x && m_curs_x>=m_indent_size)
          {
            del_sz=m_indent_size;
          }
        }

        const int xbyte = WDL_utf8_charpos_to_bytepos(tl->Get(),m_curs_x - del_sz);
        const int xbytesz=WDL_utf8_charpos_to_bytepos(tl->Get()+xbyte,del_sz);

        bool hadCom = LineCanAffectOtherLines(tl->Get(), xbyte,xbytesz);
        tl->DeleteSub(xbyte,xbytesz);
        m_curs_x-=del_sz;

        if (!hadCom) hadCom = LineCanAffectOtherLines(tl->Get(),xbyte,0);
        draw(hadCom?-1:m_curs_y);
        saveUndoState();
        setCursor();
      }
    }
    else // append current line to previous line
    {
      WDL_FastString *fl=m_text.Get(m_curs_y-1), *tl=m_text.Get(m_curs_y);
      if (!tl) 
      {
        m_curs_y--;
        if (fl) m_curs_x=WDL_utf8_get_charlen(fl->Get());
        draw();
        saveUndoState();
        setCursor();
      }
      else if (fl)
      {
        preSaveUndoState();
        m_curs_x=WDL_utf8_get_charlen(fl->Get());

        // if tl is all whitespace, don't bother appending to the previous line
        const char *p = tl->Get();
        while (*p == ' ' || *p == '\t') p++;
        if (*p) fl->Append(tl->Get());

        m_text.Delete(m_curs_y--,true);
        draw();
        saveUndoState();
        setCursor();
      }
    }
  break;
  case 'L'-'A'+1:
    draw();
    setCursor();
  break;
  case 'J'-'A'+1:
    if (!SHIFT_KEY_DOWN && !ALT_KEY_DOWN)
    {
      s_goto_line_buf.Set("");
      m_ui_state=UI_STATE_GOTO_LINE;
      run_line_editor(0,&s_goto_line_buf);
    }
  break;
  case '\r': //KEY_ENTER:
    //insert newline
    preSaveUndoState();

    if (m_selecting) { removeSelect(); draw(); setCursor(); }
    if (m_curs_y >= m_text.GetSize())
    {
      m_curs_y=m_text.GetSize();
      m_text.Add(new WDL_FastString);
    }
    if (s_overwrite)
    {
      WDL_FastString *s = m_text.Get(m_curs_y);
      int plen=0;
      const char *pb=NULL;
      if (s)
      {
        pb = s->Get();
        while (plen < m_curs_x && (pb[plen]== ' ' || pb[plen] == '\t')) plen++;
      }
      if (++m_curs_y >= m_text.GetSize())
      {
        m_curs_y = m_text.GetSize();
        WDL_FastString *ns=new WDL_FastString;
        if (plen>0) ns->Set(pb,plen);
        m_text.Insert(m_curs_y,ns);
      }
      s = m_text.Get(m_curs_y);
      if (s && plen > s->GetLength()) plen=s->GetLength();
      m_curs_x=s ? WDL_utf8_bytepos_to_charpos(s->Get(),plen) : plen;
    }
    else 
    {
      WDL_FastString *s = m_text.Get(m_curs_y);
      if (s)
      {
        int bytepos = WDL_utf8_charpos_to_bytepos(s->Get(),m_curs_x);
        if (CTRL_KEY_DOWN || bytepos > s->GetLength()) bytepos = s->GetLength();
        WDL_FastString *nl = new WDL_FastString();
        int plen=0;
        const char *pb = s->Get();
        while (pb[plen]== ' ' || pb[plen] == '\t') plen++;

        if (plen>0) nl->Set(pb,plen);

        const char *insert = pb+bytepos;
        while (*insert == ' ' || *insert == '\t') insert++;
        nl->Append(insert);
        m_text.Insert(++m_curs_y,nl);
        s->SetLen(bytepos);
        m_curs_x=WDL_utf8_bytepos_to_charpos(nl->Get(),plen);
      }
    }
    m_offs_x=0;

    draw();
    saveUndoState();
    setCursor();
  break;
  case '\t':
    if (m_selecting)
    {
      preSaveUndoState();

      bool isRev = !!(GetAsyncKeyState(VK_SHIFT)&0x8000);
      indentSelect(isRev?-m_indent_size:m_indent_size);
      // indent selection:
      draw();
      setCursor();
      saveUndoState();
      break;
    }
    else if (GetAsyncKeyState(VK_SHIFT)&0x8000)
    {
      if (m_curs_x > 0)
      {
        WDL_FastString *tl=m_text.Get(m_curs_y);
        if (tl)
        {
          int del_pos = m_curs_x, del_sz;

          for (del_sz = 0; del_sz < m_curs_x && tl->Get()[del_sz] == ' '; del_sz++);

          if (del_sz < m_curs_x && tl->Get()[WDL_utf8_charpos_to_bytepos(tl->Get(),m_curs_x - 1)] == ' ')
          {
            // spaces before cursor but not at start of line
            for (del_sz = 1;
                m_curs_x - del_sz - 1 >= 0 &&
                del_sz < wdl_max(m_indent_size,1) &&
                tl->Get()[WDL_utf8_charpos_to_bytepos(tl->Get(),m_curs_x - del_sz - 1)] == ' ';
                del_sz++);
          }
          else if (del_sz > 0)
          {
            // adjust leading indentation
            int rem = 1;
            if (m_indent_size > 0)
            {
              rem = del_sz % m_indent_size;
              if (!rem) rem = m_indent_size;
            }
            if (del_sz > rem) del_sz = rem;
            del_pos = del_sz;
          }

          if (del_sz > 0)
          {
            preSaveUndoState();
            const int xbyte = WDL_utf8_charpos_to_bytepos(tl->Get(),del_pos - del_sz);
            const int xbytesz=WDL_utf8_charpos_to_bytepos(tl->Get()+xbyte,del_sz);

            bool hadCom = LineCanAffectOtherLines(tl->Get(), xbyte,xbytesz);
            tl->DeleteSub(xbyte,xbytesz);
            m_curs_x-=del_sz;

            if (!hadCom) hadCom = LineCanAffectOtherLines(tl->Get(),xbyte,0);
            draw(hadCom?-1:m_curs_y);
            saveUndoState();
            setCursor();
          }
        }
      }
      break;
    }
  default:
    //insert char
    if(VALIDATE_TEXT_CHAR(c))
    { 
      preSaveUndoState();

      if (m_selecting) { removeSelect(); draw(); setCursor(); }
      if (!m_text.Get(m_curs_y)) m_text.Insert(m_curs_y,new WDL_FastString);

      WDL_FastString *ss;
      if ((ss=m_text.Get(m_curs_y)))
      {
        char str[64];
        int slen=1,slen_bytes=1;
        if (c == '\t') 
        {
          slen = wdl_min(m_indent_size,64);
          if (slen<1) slen=1;
          
          const int partial = m_curs_x % slen;
          if (partial) slen -= partial;

          int x; 
          for(x=0;x<slen;x++) str[x]=' ';
          slen_bytes=slen;
        }
#ifdef __APPLE__
        else if (c == 'n' && !SHIFT_KEY_DOWN && ALT_KEY_DOWN) 
        {
          str[0]='~';
        }
#endif
        else
        {
          slen_bytes = WDL_MakeUTFChar(str,c,32);
          str[slen_bytes]=0;
        }


        const int xbyte = WDL_utf8_charpos_to_bytepos(ss->Get(),m_curs_x);

        bool hadCom = LineCanAffectOtherLines(ss->Get(),xbyte,0);
        if (s_overwrite)
        {
          const int xbytesz_del=WDL_utf8_charpos_to_bytepos(ss->Get()+xbyte,slen);
          if (!hadCom) hadCom = LineCanAffectOtherLines(ss->Get(),xbyte,xbytesz_del);
          ss->DeleteSub(xbyte,xbytesz_del);
        }

        ss->Insert(str,xbyte,slen_bytes);
        if (!hadCom) hadCom = LineCanAffectOtherLines(ss->Get(),xbyte,slen_bytes);

        m_curs_x += slen;

        draw(hadCom ? -1 : m_curs_y);
      }
      saveUndoState();
      setCursor();
    }
    break;
  }
  return 0;
}

void WDL_CursesEditor::preSaveUndoState() 
{
  editUndoRec *rec= m_undoStack.Get(m_undoStack_pos);
  if (rec)
  {
    rec->m_offs_x[1]=m_offs_x;
    rec->m_curs_x[1]=m_curs_x;
    rec->m_curs_y[1]=m_curs_y;
    rec->m_curpaneoffs_y[1]=m_paneoffs_y[m_curpane];
  }
}
void WDL_CursesEditor::saveUndoState() 
{
  editUndoRec *rec=new editUndoRec;
  rec->m_offs_x[1]=rec->m_offs_x[0]=m_offs_x;
  rec->m_curs_x[1]=rec->m_curs_x[0]=m_curs_x;
  rec->m_curs_y[1]=rec->m_curs_y[0]=m_curs_y;
  rec->m_curpaneoffs_y[1]=rec->m_curpaneoffs_y[0]=m_paneoffs_y[m_curpane];

  editUndoRec *lrec[5];
  lrec[0] = m_undoStack.Get(m_undoStack_pos);
  lrec[1] = m_undoStack.Get(m_undoStack_pos+1);
  lrec[2] = m_undoStack.Get(m_undoStack_pos-1);
  lrec[3] = m_undoStack.Get(m_undoStack_pos-2);
  lrec[4] = m_undoStack.Get(m_undoStack_pos-3);

  int x;
  for (x = 0; x < m_text.GetSize(); x ++)
  {
    refcntString *ns = NULL;

    const WDL_FastString *s=m_text.Get(x);
    const int s_len=s?s->GetLength():0;
    
    if (s_len>0)
    {
      const char *cs=s?s->Get():"";
      int w;
      for(w=0;w<5 && !ns;w++)
      {
        if (lrec[w])
        {
          int y;
          for (y=0; y<15; y ++)
          {
            refcntString *a = lrec[w]->m_htext.Get(x + ((y&1) ? -(y+1)/2 : y/2));
            if (a && a->getStrLen() == s_len && !strcmp(a->getStr(),cs))
            {
              ns = a;
              break;
            }
          }
        }
      }

      if (!ns) ns = new refcntString(cs,s_len);

      ns->AddRef();
    }
    else
    {
      // ns is NULL, blank line
    }

    rec->m_htext.Add(ns);
  }

  for (x = m_undoStack.GetSize()-1; x > m_undoStack_pos; x --)
  {
    m_undoStack.Delete(x,true);
  }
  if (m_clean_undopos > m_undoStack_pos) 
    m_clean_undopos=-1;
  
  if (m_undoStack.GetSize() > m_max_undo_states) 
  {
    for (x = 1; x < m_undoStack.GetSize()/2; x += 2)
    {
      if (x != m_clean_undopos)// don't delete our preferred special (last saved) one
      {
        m_undoStack.Delete(x,true);
        if (m_clean_undopos > x) m_clean_undopos--;
        x--;
      }
    }

    m_undoStack_pos = m_undoStack.GetSize()-1;
  }
  m_undoStack_pos++;
  m_undoStack.Add(rec);
}

void WDL_CursesEditor::loadUndoState(editUndoRec *rec, int idx)
{
  int x;
  m_text.Empty(true);
  for (x = 0; x < rec->m_htext.GetSize(); x ++)
  {
    refcntString *s=rec->m_htext.Get(x);
    m_text.Add(new WDL_FastString(s?s->getStr():""));
  }

  m_offs_x=rec->m_offs_x[idx];
  m_curs_x=rec->m_curs_x[idx];
  m_curs_y=rec->m_curs_y[idx];

  m_paneoffs_y[m_curpane]=rec->m_curpaneoffs_y[idx];
  m_selecting=false;
}


void WDL_CursesEditor::RunEditor()
{
  WDL_DestroyCheck chk(&destroy_check);

  int x;
  for(x=0;x<16;x++)
  {
    if (!chk.isOK() || !CURSES_INSTANCE) break;

    int thischar = getch();
    if (thischar==ERR) break;

    if (onChar(thischar)) break;
  }
}

void WDL_CursesEditor::draw_top_line()
{
  int ypos=0;
  if (m_top_margin > 1)
  {
    int xpos=0;
    int x;
    move(ypos++,0);
    const int cnt= GetTabCount();
    int tsz=16;
    // this is duplicated in onMouseMessage
    if (cnt>0) tsz=COLS/cnt;
    if (tsz>128)tsz=128;
    if (tsz<12) tsz=12;

    for (x= 0; x < cnt && xpos < COLS; x ++)
    {
      WDL_CursesEditor *ed = GetTab(x);
      if (ed)
      {
        char buf[128 + 8];
        memset(buf,' ',tsz);
        const char *p = WDL_get_filepart(ed->GetFileName());
        const int lp=strlen(p);
        int skip=0;        
        if (x<9) 
        { 
          if (tsz>16)
          {
            const char *s = "<" CONTROL_KEY_NAME "+";
            memcpy(buf,s,skip = (int)strlen(s));
          }
          buf[skip++]='F'; 
          buf[skip++] = '1'+x; 
          buf[skip++] = '>';
          skip++;
        }
        memcpy(buf+skip,p,wdl_min(tsz-1-skip,lp));
        buf[tsz]=0;
        int l = tsz;
        if (l > COLS-xpos) l = COLS-xpos;
        if (ed == this)
        {
          attrset(SYNTAX_HIGHLIGHT2|A_BOLD);
        }
        else
        {
          attrset(A_NORMAL);
        }
        addnstr(buf,l);
        xpos += l;
      }
    }
    if (xpos < COLS) clrtoeol();
  }
  attrset(COLOR_TOPLINE|A_BOLD);
  bkgdset(COLOR_TOPLINE);
  const char *p=GetFileName();
  move(ypos,0);
  if (COLS>4)
  {
    const int pl = (int) strlen(p);
    if (pl > COLS-1 && COLS > 4)
    {
      addstr("...");
      p+=pl - (COLS-1) + 4;
    }
    addstr(p);
  }
  clrtoeol();
}

void WDL_CursesEditor::OpenFileInTab(const char *fnp)
{
  if (!fnp[0]) return;

  FILE *fp = fopen(fnp,"rb");
  if (!fp)
  {
    WDL_FastString s(fnp);
    m_newfn.Set(fnp);

    if (COLS > 25)
    {
      int allowed = COLS-25;
      if (s.GetLength()>allowed)
      {
        s.DeleteSub(0,s.GetLength() - allowed + 3);
        s.Insert("...",0);
      }
      s.Insert("Create new file '",0);
      s.Append("' (Y/n)? ");
    }
    else
      s.Set("Create new file (Y/n)? ");

    m_ui_state=UI_STATE_SAVE_AS_NEW;
    attrset(COLOR_MESSAGE);
    bkgdset(COLOR_MESSAGE);
    mvaddstr(LINES-1,0,s.Get());
    clrtoeol();
    attrset(0);
    bkgdset(0);
  }
  else
  {
    fclose(fp);
    int x;
    for (x=0;x<GetTabCount();x++)
    {
      WDL_CursesEditor *e = GetTab(x);
      if (e && !stricmp(e->GetFileName(),fnp))
      {
        SwitchTab(x,false);
        return;
      }
    }
    AddTab(fnp);
  }
}

void WDL_CursesEditor::do_paste_lines(WDL_PtrList<const char> &lines)
{
  WDL_FastString poststr;
  int x;
  int indent_to_pos = 0;
  int skip_source_indent=0; // number of characters of whitespace to (potentially) ignore when pasting

  for (x = 0; x < lines.GetSize(); x ++)
  {
    WDL_FastString *str=m_text.Get(m_curs_y);
    const char *tstr=lines.Get(x);
    if (!tstr) tstr="";
    if (!x)
    {
      if (str)
      {
        int bytepos = WDL_utf8_charpos_to_bytepos(str->Get(),m_curs_x);

        if (bytepos < 0) bytepos=0;
        int tmp=str->GetLength();
        if (bytepos > tmp) bytepos=tmp;

        poststr.Set(str->Get()+bytepos);
        str->SetLen(bytepos);

        if (bytepos > 0 && lines.GetSize()>1)
        {
          indent_to_pos = 0;
          while (str->Get()[indent_to_pos] == ' ') indent_to_pos++;

          skip_source_indent=1024;
          for (int i = 0; skip_source_indent > 0 && i < lines.GetSize(); i ++)
          {
            int a=0;
            const char *p = lines.Get(i);
            while (a < skip_source_indent && p[a] == ' ') a++;
            if (a < skip_source_indent && p[a]) skip_source_indent=a;
          }
        }

        str->Append(skip_indent(tstr,skip_source_indent));
      }
      else
      {
        m_text.Insert(m_curs_y,(str=new WDL_FastString(skip_indent(tstr,skip_source_indent))));
      }

      if (lines.GetSize() > 1)
      {
        m_curs_y++;
      }
      else
      {
        m_curs_x = WDL_utf8_get_charlen(str->Get());
        str->Append(poststr.Get());
      }
    }
    else if (x == lines.GetSize()-1)
    {
      WDL_FastString *s=newIndentedFastString(skip_indent(tstr,skip_source_indent),indent_to_pos);
      m_curs_x = WDL_utf8_get_charlen(s->Get());
      s->Append(poststr.Get());
      m_text.Insert(m_curs_y,s);
    }
    else
    {
      m_text.Insert(m_curs_y,newIndentedFastString(skip_indent(tstr,skip_source_indent),indent_to_pos));
      m_curs_y++;
    }
  }
}
