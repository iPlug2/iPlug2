/*
    LangPackEdit
    Copyright (C) 2022 Cockos Inc

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
       claim that you wrote the original software. If you use this software
       in a product, an acknowledgment in the product documentation would be
       appreciated but is not required.
    2. Altered source versions must be plainly marked as such, and must not be
       misrepresented as being the original software.
    3. This notice may not be removed or altered from any source distribution.
*/

#ifdef _WIN32
#include <windows.h>
#include <CommCtrl.h>
#endif

#include "../../swell/swell.h"

#include "../../win32_utf8.h"
#include "../../wingui/wndsize.h"
#include "../../filebrowse.h"
#include "../../wdlstring.h"

#include "../../assocarray.h"

#include "../../localize/localize.h"
#include "../../lineparse.h"
#include "../../has_strings.h"

#include "resource.h"

#if !defined(_WIN32) && !defined(__APPLE__)
bool g_quit;
#endif

HINSTANCE g_hInstance;
WDL_FastString g_ini_file;

enum {
  COL_STATE=0,
  COL_ID,
  COL_TEMPLATE,
  COL_LOCALIZED,
  COL_COMMON_LOCALIZED,
  COL_MAX,
};


struct pack_rec {
  char *template_str, *pack_str;

  char *key_desc; // key name + comments

  int common_idx;

  static void freeptrs(pack_rec r)
  {
    free(r.key_desc);
    free(r.template_str);
    free(r.pack_str);
  }
};

struct editor_instance {
  editor_instance() : m_recs(false, pack_rec::freeptrs), m_hwnd(NULL), m_dirty(false) { }
  ~editor_instance() { }

  WDL_FastString m_pack_fn;

  WDL_StringKeyedArray2<pack_rec> m_recs;
  WDL_TypedBuf<int> m_display_order;

  HWND m_hwnd;
  bool m_dirty;

  WDL_WndSizer m_resize;

  void load_file(const char *filename, bool is_template);
  void save_file(const char *filename);

  void cull_recs();
  void refresh_list(bool refilter=true);

  const char *get_rec_value(const pack_rec *r, const char *k, int w) const
  {
    __LOCALIZE_LCACHE("(empty)","langpackedit",empt);
    switch (w)
    {
      case COL_STATE:
        {
          if (!r->template_str)
          {
            if (strstr(k,":5CA1E00000000000")) return "";
            __LOCALIZE_LCACHE("not-in-template","langpackedit",nit);
            return nit;
          }
          if (r->pack_str) return "";
          if (r->common_idx>=0)
          {
            const char *k2;
            pack_rec *r2 = m_recs.EnumeratePtr(r->common_idx,&k2);
            if (WDL_NORMALLY(r2 && k2))
            {
              WDL_ASSERT(!strcmp(strstr(k2,":"),strstr(k,":")));
              __LOCALIZE_LCACHE("localized-in-common","langpackedit",lic);
              __LOCALIZE_LCACHE("common-not-localized","langpackedit",cnl);
              if (r2->pack_str) return lic;
              return cnl;
            }
          }
          return "not-localized";
        }
      case COL_ID: return r->key_desc ? r->key_desc : k;
      case COL_TEMPLATE: return r->template_str;
      case COL_LOCALIZED: return r->pack_str == NULL ? "" : r->pack_str[0] ? r->pack_str : empt;
      case COL_COMMON_LOCALIZED:
        if (r->common_idx>=0)
        {
          const char *k2;
          pack_rec *r2 = m_recs.EnumeratePtr(r->common_idx,&k2);
          if (WDL_NORMALLY(r2 && k2))
          {
            WDL_ASSERT(!strcmp(strstr(k2,":"),strstr(k,":")));
            return r2->pack_str == NULL ? "" : r2->pack_str[0] ? r2->pack_str : empt;
          }
        }
      break;

    }
    return NULL;
  }

  const char *get_row_value(int row, int w) const
  {
    if (row < 0 || row >= m_display_order.GetSize()) return "<err>";
    const char *k;
    pack_rec *r = m_recs.EnumeratePtr(m_display_order.Get()[row],&k);
    if (!r || !k) return "<ERR>";
    const char *rv = get_rec_value(r,k,w);
    return rv ? rv : "";
  }

  bool edit_row(int rec_idx, int other_action=IDC_LOCALIZED_STRING);

  void set_dirty()
  {
    if (!m_dirty)
    {
      m_dirty=true;
      set_caption();
    }
  }
  void set_caption()
  {
    if (m_hwnd)
    {
      char tmp[512];
      snprintf(tmp,sizeof(tmp),"%s%sLangPackEdit%s",
        m_pack_fn.get_filepart(),m_pack_fn.GetLength() ? " - ": "",m_dirty ? __LOCALIZE(" (unsaved)","langpackedit") :"");
      SetWindowText(m_hwnd,tmp);
    }
  }
  bool prompt_exit()
  {
    if (m_dirty)
    {
      int a = MessageBox(m_hwnd,__LOCALIZE("LangPack is not saved, save before exiting?","langpackedit"),
          __LOCALIZE("Unsaved","langpackedit"),MB_YESNOCANCEL);
      if (a == IDCANCEL) return false;
      if (a == IDNO) m_dirty = false;
      else SendMessage(m_hwnd,WM_COMMAND,IDC_PACK_SAVE,0);
    }
    return !m_dirty;
  }
};

static void del_array(WDL_AssocArray<WDL_UINT64, char *> *d) { delete d; }

static void format_section_id(char *buf, size_t bufsz, const char *section, WDL_UINT64 id)
{
  snprintf(buf,bufsz,"%s:%08X%08X",section, (int)(id>>32),(int)(id&0xffffffff));
}

static const char *parse_section_id(const char *k, char *buf, int bufsz) // returns ID or NULL
{
  if (buf) lstrcpyn_safe(buf, k, bufsz);

  char *base = buf ? buf : (char*)k, *p = base;
  while (*p) p++;
  while (p > base && *p != ':') p--;
  if (WDL_NOT_NORMALLY(p==base)) return NULL;
  if (buf) *p=0;
  return p+1;
}

void editor_instance::save_file(const char *filename)
{
  FILE *fp = fopen(filename,"wb");
  if (!fp)
  {
    MessageBox(m_hwnd,__LOCALIZE("Error opening file for writing","langpackedit"),
        __LOCALIZE("Error","langpackedit"),MB_OK);
    return;
  }
  char buf[32768];
  buf[0]=0;
  if (WDL_NORMALLY(m_hwnd))
    GetDlgItemText(m_hwnd,IDC_COMMENTS,buf,sizeof(buf));
  fprintf(fp,"%s\r\n",buf);

  char last_sec[1024];
  last_sec[0]=0;

  for (int x = 0; x < m_recs.GetSize(); x ++)
  {
    const char *k;
    const pack_rec *rec = m_recs.EnumeratePtr(x,&k);
    if (!rec->pack_str) continue;

    char sec[256];
    const char *id = parse_section_id(k,sec,sizeof(sec));
    if (WDL_NORMALLY(id))
    {
      if (rec->common_idx>=0 && WDL_NORMALLY(stricmp(sec,"common")))
      {
        // optimize and don't write out duplicate strings which are already in [common]
        const char *k2;
        pack_rec *r2 = m_recs.EnumeratePtr(rec->common_idx,&k2);
        if (WDL_NORMALLY(r2 && k2))
        {
          if (r2->pack_str && !strcmp(r2->pack_str,rec->pack_str))
            continue;
        }
      }

      if (stricmp(last_sec,sec))
      {
        lstrcpyn_safe(last_sec,sec,sizeof(last_sec));
        const char *trail = "";
        if (rec->key_desc)
        {
          trail = strstr(rec->key_desc,"  ");
          trail = trail ? (trail+1) : "";
        }
        fprintf(fp,"\r\n[%s]%s\r\n",sec,trail);
      }
      fprintf(fp,"%s=%s\r\n",id,rec->pack_str);
    }
  }
  fclose(fp);
}

void editor_instance::load_file(const char *filename, bool is_template)
{
  for (int x = 0; x < m_recs.GetSize(); x ++)
  {
    pack_rec *r = m_recs.EnumeratePtr(x);
    if (is_template)
    {
      free(r->template_str);
      r->template_str = NULL;
    }
    else
    {
      free(r->pack_str);
      r->pack_str = NULL;
    }
  }

  WDL_StringKeyedArray<char *> extra(false, WDL_StringKeyedArray<char>::freecharptr);
  if (*filename)
  {
    WDL_StringKeyedArray< WDL_AssocArray<WDL_UINT64, char *> * > r(false,del_array);
    WDL_LoadLanguagePackInternal(filename,&r,NULL, is_template, true, &extra);

    for (int si = 0; si < r.GetSize(); si ++)
    {
      const char *sec_name;
      WDL_AssocArray<WDL_UINT64, char *> *sec = r.Enumerate(si,&sec_name);
      for (int i = 0; i < sec->GetSize(); i ++)
      {
        WDL_UINT64 id;
        const char *value = sec->Enumerate(i,&id);
        if (WDL_NOT_NORMALLY(!value)) break;

        char rec_buf[256], key_desc[256];
        format_section_id(rec_buf,sizeof(rec_buf),sec_name, id);

        {
          const char *p = extra.Get(sec_name);
          if (p)
            snprintf(key_desc,sizeof(key_desc),"%.100s  %s",rec_buf,p);
          else
            key_desc[0]=0;
        }

        pack_rec *rec = m_recs.GetPtr(rec_buf);
        if (!rec)
        {
          pack_rec newr = { 0 };
          if (is_template) newr.template_str = strdup(value);
          else newr.pack_str = strdup(value);

          if (key_desc[0]) newr.key_desc = strdup(key_desc);
          m_recs.Insert(rec_buf,newr);
        }
        else
        {
          if (is_template || key_desc[0])
          {
            free(rec->key_desc);
            rec->key_desc = key_desc[0] ? strdup(key_desc) : NULL;
          }
          if (is_template)
          {
            free(rec->template_str);
            rec->template_str = strdup(value);
          }
          else
          {
            free(rec->pack_str);
            rec->pack_str = strdup(value);
          }
        }
      }
    }
  }
  cull_recs();
  refresh_list();

  if (m_hwnd)
  {
    SetDlgItemText(m_hwnd,is_template ? IDC_TEMPLATE : IDC_PACK, *filename ? filename : __LOCALIZE("(none)","langpackedit"));

    if (!is_template)
    {
      WDL_FastString fs;
      for (int x=0; ; x++)
      {
        char tmp[512];
        snprintf(tmp,sizeof(tmp),"_initial_comment_%d",x);
        const char *p = extra.Get(tmp);
        if (!p) break;
        if (fs.GetLength()) fs.Append("\r\n");
        fs.Append(p);
      }
      SetDlgItemText(m_hwnd,IDC_COMMENTS,fs.Get());
    }
  }
}

void editor_instance::cull_recs()
{
  for (int x = 0; x < m_recs.GetSize(); x ++)
  {
    pack_rec *r = m_recs.EnumeratePtr(x);
    if (!r->template_str && !r->pack_str)
      m_recs.DeleteByIndex(x--);
  }
}

void editor_instance::refresh_list(bool refilter)
{
  int *p, cnt;
  if (refilter)
  {
    p = m_display_order.Resize(m_recs.GetSize());
    cnt = 0;
  }
  else
  {
    p = m_display_order.Get();
    cnt = m_display_order.GetSize();
  }

  if (m_recs.GetSize() && WDL_NORMALLY(p))
  {
    LineParser lp;
    if (refilter && m_hwnd)
    {
      char filter[512];
      GetDlgItemText(m_hwnd,IDC_FILTER,filter,sizeof(filter));
      WDL_makeSearchFilter(filter, &lp);
    }
    const bool do_filt = lp.getnumtokens()>0;
    for (int x = 0; x < m_recs.GetSize(); x ++)
    {
      const char *k = NULL;
      pack_rec *r = m_recs.EnumeratePtr(x,&k);
      if (WDL_NOT_NORMALLY(!r)) break;

      if (strnicmp(k,"common:",7))
      {
        const char *sid = parse_section_id(k,NULL,0);
        if (WDL_NORMALLY(sid))
        {
          char tmp[256];
          snprintf(tmp,sizeof(tmp),"common:%s",sid);
          r->common_idx = m_recs.GetIdx(tmp);
        }
      }
      else
        r->common_idx = -1;

      if (refilter)
      {
        const char *strs[COL_MAX];
        int nc = 0;
        if (do_filt)
        {
          for (int c =0; c < COL_MAX; c ++)
          {
            const char *v = get_rec_value(r,k,c);
            if (v) strs[nc++] = v;
          }
        }

        if (!do_filt || WDL_hasStringsEx2(strs,nc,&lp,NULL))
          p[cnt++] = x;
      }
    }
  }

  m_display_order.Resize(cnt,false);
  if (m_hwnd)
  {
    HWND list = GetDlgItem(m_hwnd,IDC_LIST);
    ListView_SetItemCount(list,cnt);
    ListView_RedrawItems(list,0,cnt);
  }
}

WDL_DLGRET editorProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
    case WM_INITDIALOG:
      SetWindowLongPtr(hwndDlg,GWLP_USERDATA,lParam);
      {
        void **p = (void **)lParam;
        editor_instance *edit = (editor_instance *)p[0];
        int item = (int) (INT_PTR) p[1];
        const char *k;
        pack_rec *rec = edit->m_recs.EnumeratePtr(item,&k);
        if (WDL_NORMALLY(rec))
        {
          char tmp[512];
          snprintf(tmp,sizeof(tmp),__LOCALIZE_VERFMT("Localize: %s","langpackedit"),k);
          SetWindowText(hwndDlg,tmp);

          if (rec->template_str)
            SetDlgItemText(hwndDlg,IDC_TEMPLATE_STRING,rec->template_str);

          const char *common_str = NULL;
          if (rec->common_idx>=0)
          {
            const char *k2;
            pack_rec *r2 = edit->m_recs.EnumeratePtr(rec->common_idx,&k2);
            if (WDL_NORMALLY(r2 && k2))
            {
              WDL_ASSERT(!strcmp(strstr(k2,":"),strstr(k,":")));
              SetDlgItemText(hwndDlg,IDC_COMMON_STRING,r2->pack_str ? r2->pack_str : __LOCALIZE("(not yet localized)","langpackedit"));
              common_str = r2->pack_str;
            }
          }
          else
            SetDlgItemText(hwndDlg,IDC_COMMON_STRING,__LOCALIZE("(not in [common])","langpackedit"));

          if (rec->pack_str || rec->template_str)
            SetDlgItemText(hwndDlg,IDC_LOCALIZED_STRING,rec->pack_str ? rec->pack_str :
                common_str ? common_str : rec->template_str);
        }
      }
    return 0;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case IDC_REMOVE_LOCALIZATION:
        case IDC_COPY_TEMPLATE:
        case IDOK:
          {
            void **p = (void **)GetWindowLongPtr(hwndDlg,GWLP_USERDATA);
            editor_instance *edit = (editor_instance *)p[0];
            int item = (int) (INT_PTR) p[1];
            const char *k;
            pack_rec *rec = edit->m_recs.EnumeratePtr(item,&k);
            if (WDL_NORMALLY(rec))
            {
              if (LOWORD(wParam) == IDOK)
              {
                char buf[32768];
                GetDlgItemText(hwndDlg,IDC_LOCALIZED_STRING,buf,sizeof(buf));
                free(rec->pack_str);
                rec->pack_str = strdup(buf);
              }
              else
              {
                free(rec->pack_str);
                rec->pack_str = LOWORD(wParam) == IDC_COPY_TEMPLATE && rec->template_str ?
                                     strdup(rec->template_str) : NULL;
              }
            }
          }
          EndDialog(hwndDlg,1);
        break;
        case IDCANCEL:
          EndDialog(hwndDlg,0);
        break;
      }
    break;
  }
  return 0;
}

bool editor_instance::edit_row(int row, int other_action)
{
  if (row < 0 || row >= m_display_order.GetSize()) return false;
  int rec_idx = m_display_order.Get()[row];

  WDL_ASSERT(rec_idx>=0 && rec_idx < m_recs.GetSize());

  if (other_action && other_action != IDC_LOCALIZED_STRING)
  {
    const char *k;
    pack_rec *r = m_recs.EnumeratePtr(rec_idx,&k);
    if (WDL_NORMALLY(r))
    {
      switch (other_action)
      {
        case IDC_COMMON_STRING:
          if (r->common_idx>=0 && r->common_idx < m_recs.GetSize())
            rec_idx = r->common_idx;
        break;
        case IDC_REMOVE_NONLOCALIZATION:
          if (r->pack_str)
          {
            if (!r->template_str) return false;
            if (strcmp(r->template_str,r->pack_str)) return false;
          }
          // fall through
        case IDC_REMOVE_LOCALIZATION:
        case IDC_COPY_TEMPLATE:
          if (other_action == IDC_COPY_TEMPLATE && r->pack_str) return false; // do not modify already-localized strings

          free(r->pack_str);
          r->pack_str = other_action == IDC_COPY_TEMPLATE && r->template_str ? strdup(r->template_str) : NULL;
        return true;
        case ID_SCALING_ADD:
          {
            char sec[256];
            if (WDL_NORMALLY(parse_section_id(k,sec,sizeof(sec))))
            {
              lstrcatn(sec,":5CA1E00000000000",sizeof(sec));
              if (!m_recs.GetPtr(sec))
              {
                pack_rec newr = { 0 };
                m_recs.Insert(sec,newr);
                int idx = m_recs.GetIdx(sec);
                if (WDL_NORMALLY(idx>=0))
                {
                  // added a rec, adjust indices and stick us at the end
                  for (int x = 0; x < m_display_order.GetSize(); x ++)
                  {
                    if (m_display_order.Get()[x] >= idx)
                      m_display_order.Get()[x]++;
                  }
                  m_display_order.Add(idx);
                  return true;
                }

              }
            }
          }
        return false;
      }
    }
  }

  void *p[2] = { this, (void *)(INT_PTR) rec_idx };
  if (DialogBoxParam(g_hInstance, MAKEINTRESOURCE(IDD_RENAME), m_hwnd, editorProc, (LPARAM)p))
  {
    refresh_list(false);
    set_dirty();
    return true;
  }
  return false;
}

const char *COL_DESCS[COL_MAX] = {
  // !WANT_LOCALIZE_STRINGS_BEGIN:langpackedit
  "State",
  "ID",
  "Template",
  "Localized",
  "Common Localized",
  // !WANT_LOCALIZE_STRINGS_END
};

int COL_SIZES[COL_MAX] = {
  120,
  120,
  240,
  240,
  240,
};

editor_instance g_editor;
WDL_DLGRET mainProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  enum {
    TIMER_FILTER=1
  };
  switch (uMsg)
  {
    case WM_INITDIALOG:
      g_editor.m_hwnd = hwndDlg;
#ifdef _WIN32
      {
        HICON icon=LoadIcon(g_hInstance,MAKEINTRESOURCE(IDI_ICON1));
        SetClassLongPtr(hwndDlg,GCLP_HICON,(LPARAM)icon);
      }
#endif

      g_editor.m_resize.init(hwndDlg);
      g_editor.m_resize.init_item(IDC_FILTER,0,0,1,0);
      g_editor.m_resize.init_item(IDC_TEMPLATE,0,0,1,0);
      g_editor.m_resize.init_item(IDC_TEMPLATE_LOAD,1,0,1,0);
      g_editor.m_resize.init_item(IDC_PACK,0,0,1,0);
      g_editor.m_resize.init_item(IDC_PACK_LOAD,1,0,1,0);
      g_editor.m_resize.init_item(IDC_PACK_SAVE,1,0,1,0);
      g_editor.m_resize.init_item(IDC_PACK_SAVE_AS,1,0,1,0);
      g_editor.m_resize.init_item(IDC_COMMENTS,0,0,1,0);
      g_editor.m_resize.init_item(IDC_LIST,0,0,1,1);

      {
        HWND hlist = GetDlgItem(hwndDlg, IDC_LIST);

        int s=LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES;
#ifdef _WIN32
        s|=LVS_EX_DOUBLEBUFFER;
#endif
        ListView_SetExtendedListViewStyleEx(hlist, s,s );

#ifdef _WIN32
        WDL_UTF8_HookListView(hlist);
        SendMessage(hlist,LVM_SETUNICODEFORMAT,1,0);
#endif
        for (int x = 0; x < COL_MAX; x ++)
        {
          LVCOLUMN lvc = { LVCF_TEXT|LVCF_WIDTH, 0, COL_SIZES[x], (char*)__localizeFunc(COL_DESCS[x],"langpackedit",LOCALIZE_FLAG_NOCACHE) };
          ListView_InsertColumn(hlist, x, &lvc);
        }
      }

      {
        char buf[2048];
        GetPrivateProfileString("LangPackEdit","template","",buf,sizeof(buf),g_ini_file.Get());
        g_editor.load_file(buf,true);

        GetPrivateProfileString("LangPackEdit","lastpack","",buf,sizeof(buf),g_ini_file.Get());
        g_editor.load_file(buf,false);
        g_editor.m_pack_fn.Set(buf);
        g_editor.set_caption();
      }
    return 1;
    case WM_DESTROY:
      g_editor.m_hwnd = NULL;
#ifdef __APPLE__
      SWELL_PostQuitMessage(0);
#elif defined(_WIN32)
      PostQuitMessage(0);
#else
      g_quit = true;
#endif
    break;
    case WM_SIZE:
      if (wParam != SIZE_MINIMIZED)
        g_editor.m_resize.onResize();
    break;
    case WM_TIMER:
      switch (wParam)
      {
        case TIMER_FILTER:
          KillTimer(hwndDlg,TIMER_FILTER);
          g_editor.refresh_list();
        break;
      }
    break;
    case WM_CLOSE:
      if (g_editor.prompt_exit())
        DestroyWindow(hwndDlg);
    break;
    case WM_COMMAND:
      switch (LOWORD(wParam))
      {
        case ID_QUIT:
          if (g_editor.prompt_exit())
            DestroyWindow(hwndDlg);
        break;
        case IDC_FILTER:
          if (HIWORD(wParam) == EN_CHANGE)
          {
            SetTimer(hwndDlg,TIMER_FILTER,100,NULL);
          }
        break;
        case IDC_PACK_SAVE_AS:
        case IDC_PACK_SAVE:

          if (!g_editor.m_pack_fn.GetLength() || LOWORD(wParam) == IDC_PACK_SAVE_AS)
          {
            char newfn[2048];
            if (!WDL_ChooseFileForSave(hwndDlg, __LOCALIZE("Save LangPack as...","langpackedit"),
                  NULL,
                  g_editor.m_pack_fn.Get(),
                  "All files (*.*)\0*.*\0",
                  "",
                  false,
                  newfn,sizeof(newfn)) || !newfn[0]) return 0;

            g_editor.m_pack_fn.Set(newfn);
            WritePrivateProfileString("LangPackEdit","lastpack",newfn,g_ini_file.Get());
            SetDlgItemText(hwndDlg,IDC_PACK,newfn);
          }

          g_editor.save_file(g_editor.m_pack_fn.Get());
          g_editor.m_dirty=false;
          g_editor.set_caption();
        break;

        case IDC_PACK_LOAD:
        case IDC_TEMPLATE_LOAD:
          {
            char buf[2048];
            const bool is_template = LOWORD(wParam) == IDC_TEMPLATE_LOAD;
            const char *inikey = is_template ? "template" : "lastpack";
            GetPrivateProfileString("LangPackEdit",inikey,"",buf,sizeof(buf),g_ini_file.Get());
            char *f = WDL_ChooseFileForOpen(hwndDlg,
                is_template ? __LOCALIZE("Choose LangPack Template","langpackedit") :
                              __LOCALIZE("Load LangPack","langpackedit"),
                NULL,
                buf,
                "All files (*.*)\0*.*\0"
                ,
                "",
                false, false);
            if (f)
            {
              WritePrivateProfileString("LangPackEdit",inikey,f,g_ini_file.Get());
              g_editor.load_file(f,is_template);
              if (!is_template) g_editor.m_pack_fn.Set(f);
              free(f);
              if (!is_template)
              {
                g_editor.m_dirty=false;
                g_editor.set_caption();
              }
            }
          }
        break;
      }
    break;
    case WM_NOTIFY:
      {
        NMLISTVIEW* lv = (NMLISTVIEW*)lParam;
        if (lv->hdr.idFrom == IDC_LIST) switch (lv->hdr.code)
        {
          case NM_DBLCLK:
            g_editor.edit_row(lv->iItem);
          return 0;
          case NM_RCLICK:
            if (ListView_GetSelectedCount(lv->hdr.hwndFrom)>0)
            {
              HMENU menu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_CONTEXTMENU));

              POINT p;
              GetCursorPos(&p);
              int ret = TrackPopupMenu(menu,TPM_NONOTIFY|TPM_RETURNCMD,p.x,p.y,0,hwndDlg,NULL);
              DestroyMenu(menu);

              if (ret)
              {
                int cnt = 0;
                HWND list = lv->hdr.hwndFrom;
                const int n = ListView_GetItemCount(list);
                for (int x = 0; x < n; x ++)
                {
                  if (ListView_GetItemState(list,x,LVIS_SELECTED) & LVIS_SELECTED)
                  {
                    if (g_editor.edit_row(x,ret)) cnt++;
                    if (ret == ID_SCALING_ADD)
                      ListView_SetItemState(list,x,0,LVIS_SELECTED);
                  }
                }
                if (cnt && ret != IDC_LOCALIZED_STRING)
                {
                  g_editor.refresh_list(false);
                  g_editor.set_dirty();
                }
                if (ret == ID_SCALING_ADD)
                {
                  const int nn = ListView_GetItemCount(list);
                  if (n < nn)
                  {
                    ListView_EnsureVisible(list,n,false);
                    for (int i = n; i < nn; i ++)
                      ListView_SetItemState(list,i,LVIS_SELECTED,LVIS_SELECTED);
                  }
                }
              }

            }
          return 0;
          case LVN_GETDISPINFO:
#ifdef _WIN32
          case LVN_GETDISPINFOW:
#endif
          {
            NMLVDISPINFO *lpdi = (NMLVDISPINFO*) lParam;
            if (lpdi->item.mask & LVIF_TEXT)
            {
              lpdi->item.pszText = (char*) g_editor.get_row_value(lpdi->item.iItem, lpdi->item.iSubItem);
#ifdef _WIN32
              if (lv->hdr.code == LVN_GETDISPINFOW)
                WDL_UTF8_ListViewConvertDispInfoToW(lpdi);
#endif
            }
          }
        }
      }
    break;
  }
  return 0;
}

INT_PTR SWELLAppMain(int msg, INT_PTR parm1, INT_PTR parm2)
{
  switch (msg)
  {
    case SWELLAPP_ONLOAD:
      {
      }
    break;
    case SWELLAPP_LOADED:
      {
        char buf[2048];
        GetModuleFileName(NULL,buf,sizeof(buf));
        WDL_remove_filepart(buf);
        lstrcatn(buf,WDL_DIRCHAR_STR "LangPackEdit.ini",sizeof(buf));
        g_ini_file.Set(buf);

        WDL_remove_filepart(buf);
        lstrcatn(buf,WDL_DIRCHAR_STR "LangPackEdit.LangPack",sizeof(buf));

        extern bool g_debug_langpack_has_loaded;
        g_debug_langpack_has_loaded=true;
        WDL_LoadLanguagePack(buf,NULL);

        HWND h=CreateDialog(NULL,MAKEINTRESOURCE(IDD_DIALOG1),NULL,mainProc);
        ShowWindow(h,SW_SHOW);
      }
    break;
    case SWELLAPP_DESTROY:
      if (g_editor.m_hwnd) DestroyWindow(g_editor.m_hwnd);
    break;
    case SWELLAPP_SHOULDDESTROY:
      return g_editor.prompt_exit() ? 0 : 1;
    case SWELLAPP_ONCOMMAND:
      if (g_editor.m_hwnd)
        SendMessage(g_editor.m_hwnd,WM_COMMAND,parm1,0);
    return 0;
  }
  return 0;
}



#ifdef _WIN32

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
  g_hInstance = hInstance;

  SWELLAppMain(SWELLAPP_ONLOAD,0,0);
  SWELLAppMain(SWELLAPP_LOADED,0,0);

  for(;;)
  {
    MSG msg={0,};
    int vvv = GetMessage(&msg,NULL,0,0);
    if (!vvv) break;

    if (vvv<0)
    {
      Sleep(10);
      continue;
    }
    if (!msg.hwnd)
    {
      DispatchMessage(&msg);
      continue;
    }
    if (SWELLAppMain(SWELLAPP_PROCESSMESSAGE, (INT_PTR) &msg, 0)) continue;

    if (g_editor.m_hwnd && IsDialogMessage(g_editor.m_hwnd,&msg)) continue;

    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  SWELLAppMain(SWELLAPP_DESTROY,0,0);

  ExitProcess(0);

  return 0;
}

#else

/************** SWELL stuff ********** */

#ifdef __APPLE__
extern "C" {
#endif

const char **g_argv;
int g_argc;

#ifdef __APPLE__
};
#endif


#ifndef __APPLE__

int main(int argc, const char **argv)
{
  g_argc=argc;
  g_argv=argv;
  SWELL_initargs(&argc,(char***)&argv);
  SWELL_Internal_PostMessage_Init();
  SWELL_ExtendedAPI("APPNAME",(void*)"LangPackEdit");
  SWELLAppMain(SWELLAPP_ONLOAD,0,0);
  SWELLAppMain(SWELLAPP_LOADED,0,0);
  while (!g_quit) {
    SWELL_RunMessageLoop();
    Sleep(10);
  }
  SWELLAppMain(SWELLAPP_DESTROY,0,0);
  return 0;
}

#endif


#include "../../swell/swell-dlggen.h"
#include "res.rc_mac_dlg"
#undef BEGIN
#undef END
#include "../../swell/swell-menugen.h"
#include "res.rc_mac_menu"

#endif
