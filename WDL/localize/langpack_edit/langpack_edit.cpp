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
void WDL_fgets_as_utf8(char *linebuf, int linebuf_size, FILE *fp, int *format_flag); // defined in localize.cpp

#define WDL_HASSTRINGS_REWUTF8_HOOK(str, base) \
 if ((str) > (base)) switch ((str)[0]) { \
   case 'r': case 'n': case 't': case '0': if ((str)[-1] == '\\') (str)--; break; \
   case 'd': case 's': case 'f': case 'g': case 'c': case 'u': if ((str)[-1] == '%') (str)--; break; \
 }
#include "../../has_strings.h"

#include "resource.h"


#ifndef HDF_SORTUP
#define HDF_SORTUP 0x0400
#endif
#ifndef HDF_SORTDOWN
#define HDF_SORTDOWN 0x0200
#endif

void ListView_SetHeaderSortArrow(HWND hlist, int col, int dir)
{
  HWND hhdr=ListView_GetHeader(hlist);
  if (!hhdr) return;
  for (int i=0; i < Header_GetItemCount(hhdr); ++i)
  {
    HDITEM hi = { HDI_FORMAT, 0, };
    Header_GetItem(hhdr, i, &hi);
    hi.fmt &= ~(HDF_SORTUP|HDF_SORTDOWN);
    if (i == col) hi.fmt |= (dir > 0 ? HDF_SORTUP : HDF_SORTDOWN);
    Header_SetItem(hhdr, i, &hi);
  }
}


template<class A, class B> static void Restore_ListSelState(HWND hlist, const WDL_TypedBuf<A> *order, const B *st)
{
#ifdef __APPLE__
   SendMessage(hlist,WM_SETREDRAW,FALSE,0);
#endif
  int x;
  const int n = order->GetSize();
  int frow = -1;
  for (x = 0; x < n; x ++)
  {
    bool sel = st->Get(order->Get()[x]);
    if (frow < 0 && sel) frow = x;
    ListView_SetItemState(hlist, x, sel ? LVIS_SELECTED : 0, LVIS_SELECTED);
  }
  if (frow >= 0)
    ListView_EnsureVisible(hlist, frow, FALSE);
#ifdef __APPLE__
   SendMessage(hlist,WM_SETREDRAW,TRUE,0);
#endif
}

template<class A, class B> static void Save_ListSelState(HWND hlist, const WDL_TypedBuf<A> *order, B *st)
{
  int x;
  const int n = order->GetSize();
  for (x = 0; x < n; x ++)
  {
    if (ListView_GetItemState(hlist,x,LVIS_SELECTED)) st->AddUnsorted(order->Get()[x], true);
  }
  st->Resort();
}



#if !defined(_WIN32) && !defined(__APPLE__)
bool g_quit;
#endif

HINSTANCE g_hInstance;
WDL_FastString g_ini_file;
static bool s_comment_reent;

enum {
  COL_STATE=0, // if we edit these need to edit the IDs of ID_COL_* in resource.h
  COL_ID,
  COL_ROW_IDX,
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

static int sort_section_id(const char * const *aa, const char * const *bb)
{
  const char *a = *aa ? *aa : "", *b = *bb ? *bb : "";
  int pl=0;
  while (a[pl] && b[pl] && a[pl] != ':' && b[pl] != ':') pl++;

  if (pl && a[pl]==b[pl]) // both sections same length
  {
    int r = strnicmp(a,b,pl); // if differ by more than just case
    if (r) return r;
    r = strncmp(a,b,pl); // same but potentially different case
    if (r) return r;
    // exact same section, compare rest of string
    return a[pl] ? strcmp(a+pl,b+pl) : 0;
  }
  return stricmp(a,b);
}

struct editor_instance {
  editor_instance() : m_recs(true, pack_rec::freeptrs),
                      m_column_no_searchflags(0), m_sort_col(COL_ID), m_sort_rev(false),
                      m_hwnd(NULL), m_dirty(false)
  {
    m_recs.Resort(sort_section_id);
  }
  ~editor_instance() { }

  WDL_FastString m_pack_fn;

  WDL_StringKeyedArray2<pack_rec> m_recs;
  WDL_TypedBuf<int> m_display_order;
  int m_column_no_searchflags; // bits for exclude from search per column
  int m_sort_col;
  bool m_sort_rev;

  HWND m_hwnd;
  bool m_dirty;

  WDL_WndSizer m_resize;

  void load_file(const char *filename, bool is_template);
  void save_file(const char *filename, bool verbose=false);
  bool import_for_view(FILE *fp);
  void export_for_view(FILE *fp, int col)
  {
    for (int i = 0; i < m_display_order.GetSize(); i ++)
      fprintf(fp,"%s\n", get_row_value(i, col));
  }

  void cull_recs();
  void refresh_list(bool refilter=true);
  void sort_display_order();
  void on_sort_change();

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
  bool on_key(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
  void item_context_menu();

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

static void del_array(WDL_KeyedArray<WDL_UINT64, char *> *d) { delete d; }

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

void editor_instance::save_file(const char *filename, bool verbose)
{
  FILE *fp = fopenUTF8(filename,"wb");
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
  WDL_remove_trailing_whitespace(buf);
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
      if (strcmp(last_sec,sec))
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
      if (verbose && rec->template_str)
        fprintf(fp,";%s=%s\r\n",id,rec->template_str);
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

  WDL_StringKeyedArray<char *> extra(true, WDL_StringKeyedArray<char>::freecharptr);
  if (*filename)
  {
    WDL_StringKeyedArray< WDL_KeyedArray<WDL_UINT64, char *> * > r(true,del_array);
    WDL_LoadLanguagePackInternal(filename,&r,NULL, is_template, true, &extra);

    for (int si = 0; si < r.GetSize(); si ++)
    {
      const char *sec_name;
      WDL_KeyedArray<WDL_UINT64, char *> *sec = r.Enumerate(si,&sec_name);
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
      if (fs.GetLength())
        WDL_remove_trailing_whitespace((char *)fs.Get());
      s_comment_reent=true;
      SetDlgItemText(m_hwnd,IDC_COMMENTS,fs.Get());
      s_comment_reent=false;
    }
  }
}

bool editor_instance::import_for_view(FILE *fp)
{
  char linebuf[16384];
  int utf8flag = -1, lcnt=0;
  for (;;)
  {
    WDL_fgets_as_utf8(linebuf,sizeof(linebuf),fp,&utf8flag);
    if (!linebuf[0]) break;
    lcnt++;
  }
  if (lcnt != m_display_order.GetSize())
  {
    snprintf(linebuf,sizeof(linebuf),__LOCALIZE_VERFMT("Text file has %d lines, editor view %d lines. Import anyway?","langpackedit"),
        lcnt, m_display_order.GetSize());
    if (MessageBox(m_hwnd,linebuf, __LOCALIZE("Error","langpackedit"),MB_YESNO) == IDNO)
    {
      return false;
    }
  }

  fseek(fp,0,SEEK_SET);
  utf8flag = -1;
  lcnt=0;
  int errcnt=0;
  for (;;)
  {
    WDL_fgets_as_utf8(linebuf,sizeof(linebuf),fp,&utf8flag);
    if (!linebuf[0]) break;
    if (lcnt < m_display_order.GetSize())
    {
      int rec_idx = m_display_order.Get()[lcnt];
      const char *k;
      pack_rec *r = m_recs.EnumeratePtr(rec_idx,&k);
      if (WDL_NORMALLY(k && r))
      {
        free(r->pack_str);
        r->pack_str = strdup(linebuf);
      }
      else
        errcnt++;
    }
    lcnt++;
  }

  refresh_list(false);
  set_dirty();
  if (errcnt)
  {
    snprintf(linebuf,sizeof(linebuf),__LOCALIZE_VERFMT("Warning: %d lines could not be imported (this should not happen!)","langpackedit"),
        errcnt);
    MessageBox(m_hwnd,linebuf, __LOCALIZE("Warning","langpackedit"),MB_OK);
  }
  return true;
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

static bool filt_amper_str(const char *v, WDL_FastString *s)
{
  const char *nv = v;
  while (*nv && *nv != '&') nv++;
  if (!*nv) return false;
  s->Set("");
  // v=leading string, nv=next ampersand
  for (;;)
  {
    if (nv > v) s->Append(v, (int) (nv-v));
    if (!*nv) return true;
    v = ++nv; // skip ampersand
    if (*nv) nv++; // allow any trailing ampersand
    while (*nv && *nv != '&') nv++;
  }
}

void editor_instance::refresh_list(bool refilter)
{
  HWND list = WDL_NORMALLY(m_hwnd) ? GetDlgItem(m_hwnd,IDC_LIST) : NULL;
  if (!refilter)
  {
    if (list) ListView_RedrawItems(list, 0, m_display_order.GetSize());
    return;
  }
  WDL_IntKeyedArray<bool> selState;
  if (list)
    Save_ListSelState(list, &m_display_order, &selState);

  m_display_order.Resize(0,false);

  LineParser lp;
  if (m_hwnd)
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

    if (strncmp(k,"common:",7))
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

    const char *strs[COL_MAX*2];
    int nc = 0;
    if (do_filt)
    {
      static WDL_FastString amp_filt[COL_MAX];
      for (int c = 0; c < COL_MAX; c ++)
      {
        if (m_column_no_searchflags & (1<<c)) continue;
        const char *v = get_rec_value(r,k,c);
        if (v)
        {
          strs[nc++] = v;
          if (filt_amper_str(v, &amp_filt[c])) strs[nc++] = amp_filt[c].Get();
        }
      }
    }

    if (!do_filt || WDL_hasStringsEx2(strs,nc,&lp))
      m_display_order.Add(&x,1);
  }

  sort_display_order();

  if (list)
  {
    ListView_SetItemCount(list, m_display_order.GetSize());
    Restore_ListSelState(list, &m_display_order, &selState);
    ListView_RedrawItems(list, 0, m_display_order.GetSize());
  }
}

static editor_instance *sort_inst;
static int sort_func(const void *a, const void *b)
{
  const int idx_a = *(const int *)a, idx_b = *(const int *)b;
  int ret = idx_a < idx_b ? -1 : idx_a > idx_b ? 1 : 0;

  const int col = sort_inst->m_sort_col;
  switch (col)
  {
    case COL_STATE:
    case COL_TEMPLATE:
    case COL_LOCALIZED:
    case COL_COMMON_LOCALIZED:
    case COL_ID:
      {
        const char *ak, *bk;
        const pack_rec *ar = sort_inst->m_recs.EnumeratePtr(idx_a,&ak);
        const pack_rec *br = sort_inst->m_recs.EnumeratePtr(idx_b,&bk);
        const char *av = ar ? sort_inst->get_rec_value(ar,ak,col) : NULL;
        const char *bv = br ? sort_inst->get_rec_value(br,bk,col) : NULL;
        if (av || bv)
        {
          if (av) while (*av == ' ') av++;
          if (bv) while (*bv == ' ') bv++;
          if (col == COL_ID) ret = sort_section_id(&av,&bv);
          else ret = WDL_strcmp_logical_ex(av?av:"",bv?bv:"",0,WDL_STRCMP_LOGICAL_EX_FLAG_UTF8CONVERT);
        }
      }
    break;
  }
  return sort_inst->m_sort_rev ? -ret : ret;
}


void editor_instance::on_sort_change()
{
  char tmp[64];
  snprintf(tmp, sizeof(tmp), "%d", m_sort_col);
  WritePrivateProfileString("LangPackEdit", "sortcol", tmp, g_ini_file.Get());
  WritePrivateProfileString("LangPackEdit", "sortrev", m_sort_rev ? "1" : "0", g_ini_file.Get());

  if (!m_hwnd) return;
  HWND list = GetDlgItem(m_hwnd,IDC_LIST);

  if (!list) return;
  WDL_IntKeyedArray<bool> selState;
  Save_ListSelState(list, &m_display_order, &selState);

  sort_display_order();

  ListView_SetHeaderSortArrow(list, m_sort_col, (m_sort_rev ? -1 : 1));
  ListView_SetItemCount(list, m_display_order.GetSize());
  Restore_ListSelState(list, &m_display_order, &selState);

  ListView_RedrawItems(list, 0, m_display_order.GetSize());
}

void editor_instance::sort_display_order()
{
  if (m_display_order.GetSize() > 1)
  {
    sort_inst = this;
    qsort(m_display_order.Get(), m_display_order.GetSize(), sizeof(m_display_order.Get()[0]), sort_func);
    sort_inst = NULL;
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
          else if (!strncmp(k,"common:",7))
          {
            ShowWindow(GetDlgItem(hwndDlg,IDC_COMMON_LABEL),SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg,IDC_COMMON_STRING),SW_HIDE);
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


bool editor_instance::on_key(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
  if (!m_hwnd || (hwnd != m_hwnd && !IsChild(m_hwnd,hwnd)))
    return false;

#ifndef __APPLE__
  if (msg == WM_KEYDOWN && (wParam=='S' || wParam == 'O' || wParam == 'T'))
  {
    bool shift = !!(GetAsyncKeyState(VK_SHIFT)&0x8000);
    bool ctrl = !!(GetAsyncKeyState(VK_CONTROL)&0x8000);
    if (!shift && ctrl)
    {
      bool alt = !!(GetAsyncKeyState(VK_MENU)&0x8000);
      int cmd = 0;
      if (wParam == 'S' && ctrl) cmd = alt ? IDC_PACK_SAVE_AS : IDC_PACK_SAVE;
      else if (wParam == 'O') cmd = IDC_PACK_LOAD;
      else if (wParam == 'T') cmd = IDC_TEMPLATE_LOAD;

      if (cmd)
      {
        SendMessage(m_hwnd,WM_COMMAND,cmd,0);
        return 1;
      }
    }
  }
#endif

  HWND hlist = GetDlgItem(m_hwnd,IDC_LIST);
  if (hwnd == hlist || IsChild(hlist,hwnd))
  {
    if (msg == WM_KEYDOWN) switch (wParam)
    {
      case VK_RETURN:
        SendMessage(m_hwnd,WM_COMMAND,IDC_LOCALIZED_STRING,0);
      return true;
      break;
#ifdef _WIN32
      case VK_APPS:
        item_context_menu();
      return true;
#endif
      case VK_BACK:
      case VK_DELETE:
        SendMessage(m_hwnd,WM_COMMAND,IDC_REMOVE_LOCALIZATION,0);
      return true;
    }
  }
  return false;
}

void editor_instance::item_context_menu()
{
  HMENU menu = LoadMenu(g_hInstance, MAKEINTRESOURCE(IDR_CONTEXTMENU));
  POINT p;
  GetCursorPos(&p);
  TrackPopupMenu(GetSubMenu(menu,0),0,p.x,p.y,0,m_hwnd,NULL);
  DestroyMenu(menu);
}

const char *COL_DESCS[COL_MAX] = {
  // !WANT_LOCALIZE_STRINGS_BEGIN:langpackedit
  "State",
  "ID",
  "Row",
  "Template",
  "Localized",
  "Common Localized",
  // !WANT_LOCALIZE_STRINGS_END
};

int COL_SIZES[COL_MAX] = {
  120,
  120,
  30,
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
      g_editor.m_resize.init_item(IDC_PACK,0,0,1,0);
      g_editor.m_resize.init_item(IDC_COMMENTS,0,0,1,0);
      g_editor.m_resize.init_item(IDC_LIST,0,0,1,1);

      {
        HWND hlist = GetDlgItem(hwndDlg, IDC_LIST);

        int s=LVS_EX_FULLROWSELECT|LVS_EX_GRIDLINES|LVS_EX_HEADERDRAGDROP;
#ifdef _WIN32
        s|=LVS_EX_DOUBLEBUFFER;
#endif
        ListView_SetExtendedListViewStyleEx(hlist, s,s );

#ifdef _WIN32
        WDL_UTF8_HookListView(hlist);
        SendMessage(hlist,LVM_SETUNICODEFORMAT,1,0);
#endif
        int colorder[COL_MAX];
        for (int x = 0; x < COL_MAX; x ++)
        {
          char buf[64];
          sprintf(buf, "colwid_%d", x);
          int colw=GetPrivateProfileInt("LangPackEdit", buf, COL_SIZES[x], g_ini_file.Get());
          sprintf(buf, "colorder_%d", x);
          colorder[x]=GetPrivateProfileInt("LangPackEdit", buf, x, g_ini_file.Get());

          LVCOLUMN lvc = { LVCF_TEXT|LVCF_WIDTH, 0, colw, (char*)__localizeFunc(COL_DESCS[x],"langpackedit",LOCALIZE_FLAG_NOCACHE) };
          ListView_InsertColumn(hlist, x, &lvc);
        }
        ListView_SetColumnOrderArray(hlist, COL_MAX, colorder);
      }

      {
        char buf[2048];
        GetPrivateProfileString("LangPackEdit","template","",buf,sizeof(buf),g_ini_file.Get());
        g_editor.load_file(buf,true);

        GetPrivateProfileString("LangPackEdit","lastpack","",buf,sizeof(buf),g_ini_file.Get());
        g_editor.load_file(buf,false);

        g_editor.m_column_no_searchflags = GetPrivateProfileInt("LangPackEdit","nosearchcols",0,g_ini_file.Get());
        g_editor.m_sort_col = GetPrivateProfileInt("LangPackEdit","sortcol",COL_ID,g_ini_file.Get());
        g_editor.m_sort_rev = GetPrivateProfileInt("LangPackEdit","sortrev",0,g_ini_file.Get()) > 0;
        ListView_SetHeaderSortArrow(GetDlgItem(hwndDlg,IDC_LIST), g_editor.m_sort_col, (g_editor.m_sort_rev ? -1 : 1));
        g_editor.m_pack_fn.Set(buf);
        g_editor.set_caption();
      }
    return 1;
    case WM_DESTROY:

      {
        HWND hlist = GetDlgItem(hwndDlg,IDC_LIST);
        int colorder[COL_MAX];
        for (int i=0; i < COL_MAX; ++i) colorder[i]=i;
        ListView_GetColumnOrderArray(hlist, COL_MAX, colorder);

        for (int i=0; i < COL_MAX; ++i)
        {
          int colw = ListView_GetColumnWidth(hlist, i);
          char buf[256], buf2[256];
          sprintf(buf2, "colwid_%d", i);
          sprintf(buf, "%d", colw);
          char* p = colw == (int)COL_SIZES[i] ? NULL : buf;
          WritePrivateProfileString("LangPackEdit", buf2, p, g_ini_file.Get());

          sprintf(buf2, "colorder_%d", i);
          sprintf(buf, "%d", colorder[i]);
          p = colorder[i] == i ? NULL : buf;
          WritePrivateProfileString("LangPackEdit", buf2, p, g_ini_file.Get());
        }
      }

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
        case IDC_COMMENTS:
          if (HIWORD(wParam) == EN_CHANGE && !s_comment_reent)
          {
            g_editor.set_dirty();
          }
        break;
        case IDC_PACK_IMPORT_CURVIEW:
          {
            char vbuf[2048];
            GetPrivateProfileString("LangPackEdit","lastexpl","",vbuf,sizeof(vbuf),g_ini_file.Get());

            char *f = WDL_ChooseFileForOpen(hwndDlg,
                __LOCALIZE("Choose text file to import localized lines","langpackedit"),
                NULL,
                vbuf,
                "Text files (*.txt)\0*.*\0All files (*.*)\0*.*\0",
                "txt",
                false, false);
            if (f)
            {
              FILE *fp = fopenUTF8(f,"rb");

              if (fp)
              {
                if (g_editor.import_for_view(fp))
                  WritePrivateProfileString("LangPackEdit","lastexpl",f,g_ini_file.Get());
                fclose(fp);
              }
              else
              {
                MessageBox(hwndDlg,__LOCALIZE("Error opening file for writing","langpackedit"),
                  __LOCALIZE("Error","langpackedit"),MB_OK);
              }
              free(f);
            }
          }

        break;

        case IDC_PACK_EXPORT_CURVIEW_TEMPLATE:
        case IDC_PACK_EXPORT_CURVIEW_LOCALIZED:
          {
            char newfn[2048], vbuf[2048];
            const char *inikey = LOWORD(wParam)==IDC_PACK_EXPORT_CURVIEW_TEMPLATE ? "lastexpt" : "lastexpl";
            GetPrivateProfileString("LangPackEdit",inikey,"",vbuf,sizeof(vbuf),g_ini_file.Get());
            if (!WDL_ChooseFileForSave(hwndDlg,
                  LOWORD(wParam) == IDC_PACK_EXPORT_CURVIEW_TEMPLATE ?  __LOCALIZE("Export current view template lines as text","langpackedit") :
                     __LOCALIZE("Export current view localized lines as text","langpackedit"),
                  NULL,
                  vbuf,
                  "Text files (*.txt)\0*.*\0All files (*.*)\0*.*\0",
                  "txt",
                  false,
                  newfn,sizeof(newfn)) || !newfn[0]) return 0;

            FILE *fp = fopenUTF8(newfn,"wb");
            if (fp)
            {
              g_editor.export_for_view(fp,LOWORD(wParam) == IDC_PACK_EXPORT_CURVIEW_TEMPLATE ? COL_TEMPLATE : COL_LOCALIZED);
              fclose(fp);
              WritePrivateProfileString("LangPackEdit",inikey,newfn,g_ini_file.Get());
            }
            else
            {
              MessageBox(hwndDlg,__LOCALIZE("Error opening file for writing","langpackedit"),
                __LOCALIZE("Error","langpackedit"),MB_OK);
            }
          }
        break;

        case IDC_PACK_SAVE_AS_VERBOSE:
        case IDC_PACK_SAVE_AS:
        case IDC_PACK_SAVE:
          if (!g_editor.m_pack_fn.GetLength() || LOWORD(wParam) == IDC_PACK_SAVE_AS || LOWORD(wParam) == IDC_PACK_SAVE_AS_VERBOSE)
          {
            char newfn[2048];
            char vbuf[2048];
            if (LOWORD(wParam) == IDC_PACK_SAVE_AS_VERBOSE)
              GetPrivateProfileString("LangPackEdit","lastpackv","",vbuf,sizeof(vbuf),g_ini_file.Get());
            else vbuf[0]=0;
            if (!WDL_ChooseFileForSave(hwndDlg,
                  LOWORD(wParam) == IDC_PACK_SAVE_AS_VERBOSE ?  __LOCALIZE("Export Verbose LangPack","langpackedit") :
                     __LOCALIZE("Save LangPack","langpackedit"),
                  NULL,
                  vbuf[0] ? vbuf : g_editor.m_pack_fn.Get(),
                  "All files (*.*)\0*.*\0",
                  "",
                  false,
                  newfn,sizeof(newfn)) || !newfn[0]) return 0;

            if (LOWORD(wParam) == IDC_PACK_SAVE_AS_VERBOSE)
            {
              g_editor.save_file(newfn, true);
              WritePrivateProfileString("LangPackEdit","lastpackv",newfn,g_ini_file.Get());
              if (!strcmp(newfn, g_editor.m_pack_fn.Get()))
              {
                g_editor.m_dirty=false;
                g_editor.set_caption();
              }
              break;
            }
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
        case IDC_LOCALIZED_STRING:
        case IDC_COMMON_STRING:
        case ID_SCALING_ADD:
        case IDC_COPY_TEMPLATE:
        case IDC_REMOVE_LOCALIZATION:
        case IDC_REMOVE_NONLOCALIZATION:
          {
            HWND list = GetDlgItem(hwndDlg,IDC_LIST);
            int cnt = 0;
            const int n = ListView_GetItemCount(list);
            for (int x = 0; x < n; x ++)
            {
              if (ListView_GetItemState(list,x,LVIS_SELECTED) & LVIS_SELECTED)
              {
                if (g_editor.edit_row(x,(int)wParam)) cnt++;
                if (wParam == ID_SCALING_ADD)
                  ListView_SetItemState(list,x,0,LVIS_SELECTED);
              }
            }
            if (cnt && wParam != IDC_LOCALIZED_STRING)
            {
              g_editor.refresh_list(false);
              g_editor.set_dirty();
            }
            if (wParam == ID_SCALING_ADD)
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
        break;
        case ID_COL_STATE:
        case ID_COL_ID:
        case ID_COL_TEMPLATE:
        case ID_COL_LOCALIZED:
        case ID_COL_COMMONLOCALIZED:
          g_editor.m_column_no_searchflags ^= (1<<(LOWORD(wParam) - ID_COL_STATE));
          {
            char tmp[64];
            snprintf(tmp,sizeof(tmp),"%d",g_editor.m_column_no_searchflags);
            WritePrivateProfileString("LangPackEdit","nosearchcols",tmp,g_ini_file.Get());
          }
          SetTimer(hwndDlg,TIMER_FILTER,100,NULL);
        break;
        case ID_SORTCOL_STATE:
        case ID_SORTCOL_ID:
        case ID_SORTCOL_TEMPLATE:
        case ID_SORTCOL_LOCALIZED:
        case ID_SORTCOL_COMMONLOCALIZED:
          g_editor.m_sort_rev = false;
          g_editor.m_sort_col = LOWORD(wParam) - ID_SORTCOL_STATE;
          g_editor.on_sort_change();
        break;
        case ID_SORTCOL_REVERSE:
          g_editor.m_sort_rev = !g_editor.m_sort_rev;
          g_editor.on_sort_change();
        break;
      }
    break;
    case WM_INITMENUPOPUP:
      if (wParam)
      {
        HMENU menu = (HMENU) wParam;
        static const unsigned short tab[]={
          IDC_LOCALIZED_STRING,
          IDC_COMMON_STRING,
          ID_SCALING_ADD,
          IDC_COPY_TEMPLATE,
          IDC_REMOVE_LOCALIZATION,
          IDC_REMOVE_NONLOCALIZATION,
        };
        bool en = ListView_GetSelectedCount(GetDlgItem(hwndDlg,IDC_LIST))>0;
        for (size_t x = 0; x < sizeof(tab)/sizeof(tab[0]); x ++)
          EnableMenuItem(menu,tab[x],MF_BYCOMMAND|(en ? 0 : MF_GRAYED));
        for (int x = 0; x < COL_MAX; x ++)
          CheckMenuItem(menu,ID_COL_STATE+x, MF_BYCOMMAND | ((g_editor.m_column_no_searchflags&(1<<x)) ? MF_UNCHECKED:MF_CHECKED));
        for (int x = 0; x < COL_MAX; x ++)
          CheckMenuItem(menu,ID_SORTCOL_STATE+x, MF_BYCOMMAND | ((g_editor.m_sort_col == x) ? MF_CHECKED:MF_UNCHECKED));
        CheckMenuItem(menu,ID_SORTCOL_REVERSE, MF_BYCOMMAND | (g_editor.m_sort_rev ? MF_CHECKED:MF_UNCHECKED));
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
              g_editor.item_context_menu();
            }
          return 0;
          case LVN_COLUMNCLICK:
            {
              int col = lv->iSubItem;
              if (col == COL_ROW_IDX) col = COL_ID;
              if (g_editor.m_sort_col == col)
              {
                g_editor.m_sort_rev = !g_editor.m_sort_rev;
              }
              else
              {
                g_editor.m_sort_rev = false;
                g_editor.m_sort_col = col;
              }
              g_editor.on_sort_change();
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
              if (lpdi->item.iSubItem == COL_ROW_IDX)
                snprintf(lpdi->item.pszText,lpdi->item.cchTextMax,"%d",lpdi->item.iItem+1);
              else
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

#ifdef _DEBUG
        extern bool g_debug_langpack_has_loaded;
        g_debug_langpack_has_loaded=true;
#endif
        WDL_LoadLanguagePack(buf,NULL);

        HWND h=CreateDialog(NULL,MAKEINTRESOURCE(IDD_DIALOG1),NULL,mainProc);
        ShowWindow(h,SW_SHOW);

#ifndef _WIN32
      {
        HMENU menu = LoadMenu(NULL,MAKEINTRESOURCE(IDR_MENU1));
#ifdef __APPLE__
        {
          HMENU sm=GetSubMenu(menu,0);
          DeleteMenu(sm,ID_QUIT,MF_BYCOMMAND); // remove QUIT from our file menu, since it is in the system menu on OSX

          // remove any trailing separators
          int a= GetMenuItemCount(sm);
          while (a > 0 && GetMenuItemID(sm,a-1)==0) DeleteMenu(sm,--a,MF_BYPOSITION);
        }

        extern HMENU SWELL_app_stocksysmenu;
        if (SWELL_app_stocksysmenu) // insert the stock system menu
        {
          HMENU nm=SWELL_DuplicateMenu(SWELL_app_stocksysmenu);
          if (nm)
          {
            MENUITEMINFO mi={sizeof(mi),MIIM_STATE|MIIM_SUBMENU|MIIM_TYPE,MFT_STRING,0,0,nm,NULL,NULL,0,(char*)"LangPackEdit"};
            InsertMenuItem(menu,0,TRUE,&mi);
          }
        }
        SetMenuItemModifier(menu,IDC_TEMPLATE_LOAD,MF_BYCOMMAND,'T',FCONTROL);
        SetMenuItemModifier(menu,IDC_PACK_LOAD,MF_BYCOMMAND,'O',FCONTROL);
        SetMenuItemModifier(menu,IDC_PACK_SAVE,MF_BYCOMMAND,'S',FCONTROL);
        SetMenuItemModifier(menu,IDC_PACK_SAVE_AS,MF_BYCOMMAND,'S',FCONTROL|FALT);
#endif

        SetMenu(h,menu);
      }
#endif

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
    case SWELLAPP_PROCESSMESSAGE:
     if (parm1)
     {
       const MSG *m = (MSG *)parm1;
       if (m->message == WM_KEYDOWN && m->hwnd)
       {
#ifndef __APPLE__
         if (m->wParam == 'Q' && 
             (GetAsyncKeyState(VK_CONTROL)&0x8000) &&
             !(GetAsyncKeyState(VK_SHIFT)&0x8000) &&
             !(GetAsyncKeyState(VK_MENU)&0x8000))
         {
           if (g_editor.m_hwnd)
             SendMessage(g_editor.m_hwnd,WM_COMMAND,ID_QUIT,0);
           return 1;
         }
#endif
         if (g_editor.on_key(m->hwnd, m->message, m->wParam, m->lParam))
           return 1;
       }
     }
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
