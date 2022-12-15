#ifndef _EEL2_PREPROC_H_
#define _EEL2_PREPROC_H_
#include "ns-eel-int.h"

#define EEL2_PREPROCESS_OPEN_TOKEN "<?"

class EEL2_PreProcessor
{
  enum { LITERAL_BASE = 100000 };
public:
  EEL2_PreProcessor(int max_sz = 64<<20)
  {
    m_max_sz = max_sz;
    m_fsout = NULL;
    m_vm = NSEEL_VM_alloc();
    NSEEL_VM_SetCustomFuncThis(m_vm, this);
    NSEEL_VM_SetStringFunc(m_vm, addStringCallback, NULL);
    if (!m_ftab.list_size)
    {
      NSEEL_addfunc_varparm_ex("printf",1,0,NSEEL_PProc_THIS,&pp_printf,&m_ftab);
    }
    NSEEL_VM_SetFunctionTable(m_vm, &m_ftab);
    m_suppress = NSEEL_VM_regvar(m_vm, "_suppress");
  }

  ~EEL2_PreProcessor()
  {
    for (int x = 0; x < m_code_handles.GetSize(); x ++)
      NSEEL_code_free((NSEEL_CODEHANDLE) m_code_handles.Get(x));
    m_literal_strings.Empty(true,free);
    if (m_vm) NSEEL_VM_free(m_vm);
    m_suppress = NULL;
  }

  void clear_line_info()
  {
    m_line_tab.Resize(0);
  }
  const char *preprocess(const char *str, WDL_FastString *fs)
  {
    if (!m_vm || !m_suppress)
      return "preprocessor: memory error";
    m_line_tab.Resize(0);
    int input_linecnt = 0, output_linecnt = 0;
    *m_suppress = 0.0;
    for (;;)
    {
      const bool suppress = m_suppress && *m_suppress > 0.0;

      int lc = 0;
      const char *tag = str;
      while (*tag && strncmp(tag,EEL2_PREPROCESS_OPEN_TOKEN,2)) if (*tag++ == '\n') lc++;

      if (lc)
      {
        input_linecnt += lc;
        if (suppress)
          add_line_inf(output_linecnt,lc);
        else
          output_linecnt += lc;
      }

      if (!*tag)
      {
        if (!suppress) fs->Append(str);
        return NULL;
      }
      if (!suppress && tag > str) fs->Append(str,(int)(tag-str));

      tag += 2;
      while (*tag == ' ' || *tag == '\t') tag++;
      str = tag;
      lc = 0;
      while (*str && strncmp(str,"?>",2)) if (*str++ == '\n') lc++;

      if (!*str)
      {
        m_tmp.SetFormatted(512, "%d: unterminated preprocessor " EEL2_PREPROCESS_OPEN_TOKEN " block", input_linecnt+1);
        return m_tmp.Get();
      }

      if (lc)
      {
        input_linecnt += lc;
        add_line_inf(output_linecnt,lc);
      }
      if (str > tag)
      {
        m_tmp.Set(tag,(int)(str-tag));
        NSEEL_CODEHANDLE ch = NSEEL_code_compile_ex(m_vm, m_tmp.Get(), 0, NSEEL_CODE_COMPILE_FLAG_COMMONFUNCS);
        if (!ch)
        {
          const char *err = NSEEL_code_getcodeerror(m_vm);
          if (err)
          {
            const int line_ref = atoi(err);
            while (*err >= '0' && *err <= '9') err++;
            m_tmp.SetFormatted(512,"%d: preprocessor%s%s",input_linecnt+line_ref,*err && *err != ':' ? ": ":"",err);
            return m_tmp.Get();
          }
        }
        else
        {
          lc = 0;
          const int oldlen = fs->GetLength();
          m_fsout = fs;
          NSEEL_code_execute(ch);
          m_fsout = NULL;
          m_code_handles.Add(ch);
          for (int x = oldlen; x < fs->GetLength(); x ++) if (fs->Get()[x] == '\n') lc++;
          if (lc)
          {
            add_line_inf(output_linecnt,-lc);
            output_linecnt += lc;
          }
        }
      }
      str += 2;
    }
  }

  const char *translate_error_line(const char *err_line)
  {
    if (m_line_tab.GetSize()<2) return err_line;
    int l = atoi(err_line)-1;
    if (l<0) return err_line;

    // tab is a list of pairs
    // [<output position>, delta]
    // delta>0 if input lines were skipped
    // delta<0 if output lines were added

    int nl = l;
    for (int x = m_line_tab.GetSize()-2; x >= 0; x -= 2)
    {
      int p = m_line_tab.Get()[x];
      if (nl > p)
      {
        int delta = m_line_tab.Get()[x+1];
        nl += delta;
        if (nl < p) nl = p;
      }
    }

    if (l == nl) return err_line;

    while (*err_line >= '0' && *err_line <= '9') err_line++;
    if (*err_line == ':') err_line++;
    m_tmp.SetFormatted(512,"%d:%s",1 + nl,err_line);
    return m_tmp.Get();
  }

  NSEEL_VMCTX m_vm;
  WDL_PtrList<char> m_literal_strings;
  WDL_FastString m_tmp, *m_fsout;
  WDL_TypedBuf<int> m_line_tab; // expose this in case the caller wants to keep copies around
  EEL_F *m_suppress;
  int m_max_sz;
  static eel_function_table m_ftab;
  WDL_PtrList<void> m_code_handles;

  void add_line_inf(int output_linecnt, int lc)
  {
    m_line_tab.Add(output_linecnt); // log lc lines of input skipped
    m_line_tab.Add(lc);
  }

  static EEL_F addStringCallback(void *opaque, struct eelStringSegmentRec *list)
  {
    EEL2_PreProcessor *_this = (EEL2_PreProcessor*)opaque;
    if (!_this) return -1.0;

    const int sz = nseel_stringsegments_tobuf(NULL,0,list);
    char *ns = (char *)malloc(sz+1);
    if (WDL_NOT_NORMALLY(!ns)) return -1.0;
    nseel_stringsegments_tobuf(ns,sz,list);

    const int nstr = _this->m_literal_strings.GetSize();
    for (int x=0;x<nstr;x++)
    {
      char *s = _this->m_literal_strings.Get(x);
      if (!strcmp(s,ns))
      {
        free(ns);
        return x + LITERAL_BASE;
      }
    }
    _this->m_literal_strings.Add(ns);
    return nstr + LITERAL_BASE;
  }

  const char *GetString(EEL_F v)
  {
    if (v >= LITERAL_BASE && v < LITERAL_BASE + m_literal_strings.GetSize())
      return m_literal_strings.Get((int) (v - LITERAL_BASE));
    return NULL;
  }

  static int eel_validate_format_specifier(const char *fmt_in, char *typeOut,
                                           char *fmtOut, int fmtOut_sz,
                                           char *varOut, int varOut_sz,
                                           int *varOut_used
                                          )
  {
    const char *fmt = fmt_in+1;
    int state=0;
    if (fmt_in[0] != '%') return 0; // ugh passed a non-specifier

    *varOut_used = 0;
    *varOut = 0;

    if (fmtOut_sz-- < 2) return 0;
    *fmtOut++ = '%';

    while (*fmt)
    {
      const char c = *fmt++;
      if (fmtOut_sz < 2) return 0;

      if (c == 'f'|| c=='e' || c=='E' || c=='g' || c=='G' || c == 'd' || c == 'u' ||
          c == 'x' || c == 'X' || c == 'c' || c == 'C' || c =='s' || c=='S' || c=='i')
      {
        *typeOut = c;
        fmtOut[0] = c;
        fmtOut[1] = 0;
        return (int) (fmt - fmt_in);
      }
      else if (c == '.')
      {
        *fmtOut++ = c; fmtOut_sz--;
        if (state&(2)) break;
        state |= 2;
      }
      else if (c == '+')
      {
        *fmtOut++ = c; fmtOut_sz--;
        if (state&(32|16|8|4)) break;
        state |= 8;
      }
      else if (c == '-' || c == ' ')
      {
        *fmtOut++ = c; fmtOut_sz--;
        if (state&(32|16|8|4)) break;
        state |= 16;
      }
      else if (c >= '0' && c <= '9')
      {
        *fmtOut++ = c; fmtOut_sz--;
        state|=4;
      }
      else if (c == '{')
      {
        if (state & 64) break;
        state|=64;
        if (*fmt == '.' || (*fmt >= '0' && *fmt <= '9')) return 0; // symbol name can't start with 0-9 or .

        while (*fmt != '}')
        {
          if ((*fmt >= 'a' && *fmt <= 'z') ||
              (*fmt >= 'A' && *fmt <= 'Z') ||
              (*fmt >= '0' && *fmt <= '9') ||
              *fmt == '_' || *fmt == '.' || *fmt == '#')
          {
            if (varOut_sz < 2) return 0;
            *varOut++ = *fmt++;
            varOut_sz -- ;
          }
          else
          {
            return 0; // bad character in variable name
          }
        }
        fmt++;
        *varOut = 0;
        *varOut_used=1;
      }
      else
      {
        break;
      }
    }
    return 0;
  }

  static int eel_format_strings(void *opaque, const char *fmt, const char *fmt_end, char *buf, int buf_sz, int num_fmt_parms, EEL_F **fmt_parms)
  {
    EEL2_PreProcessor *_this = (EEL2_PreProcessor*)opaque;
    int fmt_parmpos = 0;
    char *op = buf;
    while ((fmt_end ? fmt < fmt_end : *fmt) && op < buf+buf_sz-128)
    {
      if (fmt[0] == '%' && fmt[1] == '%')
      {
        *op++ = '%';
        fmt+=2;
      }
      else if (fmt[0] == '%')
      {
        char ct=0;
        char fs[128];
        char varname[128];
        int varname_used=0;
        const int l=eel_validate_format_specifier(fmt,&ct,fs,sizeof(fs),varname,sizeof(varname),&varname_used);
        if (!l || !ct)
        {
          *op=0;
          return -1;
        }

        const EEL_F *varptr = NULL;
        if (!varname_used)
        {
          if (fmt_parmpos < num_fmt_parms) varptr = fmt_parms[fmt_parmpos];
          fmt_parmpos++;
        }
        double v = varptr ? (double)*varptr : 0.0;

        if (ct == 's' || ct=='S')
        {
          const char *str = _this->GetString(v);
          const int maxl=(int) (buf+buf_sz - 2 - op);
          snprintf(op,maxl,fs,str ? str : "");
        }
        else
        {
          if (ct == 'x' || ct == 'X' || ct == 'd' || ct == 'u' || ct=='i')
          {
            snprintf(op,64,fs,(int) (v));
          }
          else if (ct == 'c')
          {
            *op++=(char) (int)v;
            *op=0;
          }
          else if (ct == 'C')
          {
            const unsigned int iv = (unsigned int) v;
            int bs = 0;
            if (iv &      0xff000000) bs=24;
            else if (iv & 0x00ff0000) bs=16;
            else if (iv & 0x0000ff00) bs=8;
            while (bs>=0)
            {
              const char c=(char) (iv>>bs);
              *op++=c?c:' ';
              bs-=8;
            }
            *op=0;
          }
          else
          {
            snprintf(op,64,fs,v);
          }
        }

        while (*op) op++;

        fmt += l;
      }
      else
      {
        *op++ = *fmt++;
      }

    }
    *op=0;
    return (int) (op - buf);
  }

  static EEL_F NSEEL_CGEN_CALL pp_printf(void *opaque, INT_PTR num_param, EEL_F **parms)
  {
    if (num_param>0 && opaque)
    {
      EEL2_PreProcessor *_this = (EEL2_PreProcessor*)opaque;
      const char *fmt = _this->GetString(parms[0][0]);
      if (fmt)
      {
        char buf[16384];
        const int len = eel_format_strings(opaque,fmt,NULL,buf,(int)sizeof(buf), (int)num_param-1, parms+1);

        if (len >= 0)
        {
          if (_this->m_fsout && _this->m_fsout->GetLength() < _this->m_max_sz)
          {
            _this->m_fsout->Append(buf,len);
          }
          return 1.0;
        }
      }
    }
    return 0.0;
  }

};

eel_function_table EEL2_PreProcessor::m_ftab;

#endif
