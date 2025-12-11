/*
    WDL - jsonparse.h
    Copyright (C) 2025 and later, Cockos Incorporated

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

  minimal JSON5 parser
  does not validate tokens
  does not validate UTF-8 codepoints

*/
#ifndef _WDL_JSON_PARSE_H_
#define _WDL_JSON_PARSE_H_

#include "ptrlist.h"
#include "wdlstring.h"
#include "wdlcstring.h"
#include "wdlutf8.h"

class wdl_json_element
{
  public:
    wdl_json_element() : m_array(NULL), m_object_names(NULL), m_value(NULL), m_value_string(false)
    {
      // caller must init
    }
    ~wdl_json_element()
    {
      if (m_array) m_array->Empty(true);
      if (m_object_names) m_object_names->Empty();
      delete m_array;
      delete m_object_names;
    }

    const char *get_string_value(bool allow_unquoted=false) const { return allow_unquoted||m_value_string ? m_value : NULL; }
    bool is_array() const { return m_array && !m_object_names; }
    bool is_object() const { return m_array && m_object_names; }
    wdl_json_element *enum_item(int x) const { return m_array ? m_array->Get(x) : NULL; }
    const char *enum_item_name(int x) const { return m_object_names ? m_object_names->Get(x) : NULL; }
    wdl_json_element *get_item_by_name(const char *name) const
    {
      if (!m_object_names || WDL_NOT_NORMALLY(!m_array)) return NULL;
      for (int x = 0; x < m_object_names->GetSize(); x ++)
        if (!strcmp(m_object_names->Get(x),name)) return m_array->Get(x);
      return NULL;
    }
    const char *get_string_by_name(const char *name, bool allow_unquoted=false) const
    {
      const wdl_json_element *hit = get_item_by_name(name);
      return hit ? hit->get_string_value(allow_unquoted) : NULL;
    }

    // either m_value or m_array will be valid
    // if m_array is valid, m_object_names will be set if it is an object rather than array
    WDL_PtrList<wdl_json_element> *m_array;
    WDL_PtrList<const char> *m_object_names;
    const char *m_value; // de-escaped string value, or raw value (true/false/null, 1, 1.234)

    bool m_value_string; // true if m_value was a string value
};

class wdl_json_parser
{
  public:
    wdl_json_parser() : m_err(NULL), m_err_rdptr(NULL), m_stringstore_pos(0) { }
    ~wdl_json_parser()
    {
      m_spare_elements.Empty(true);
      m_spare_arrays.Empty(true);
      m_spare_object_names.Empty(true);
    }

    const char *m_err;
    const char *m_err_rdptr;
    WDL_FastString m_tmp;

    // if caller wants to take ownership of its wdl_json_element tree, it will need to create a heapbuf and use
    // hb.SwapContentsWith(&parser.m_stringstore)
    WDL_HeapBuf m_stringstore;
    int m_stringstore_pos;

    wdl_json_element *parse(const char *rdptr, int rdptr_len)
    {
      m_stringstore.ResizeOK(rdptr_len,false);
      m_stringstore_pos = 0;
      m_err = m_err_rdptr = NULL;
      wdl_json_element *elem = NULL;
      parse_internal(rdptr,rdptr+rdptr_len,&elem);
      return elem;
    }

    void dispose_element(wdl_json_element *elem)
    {
      if (!elem) return;
      if (elem->m_array)
      {
        for (int x = 0; x < elem->m_array->GetSize(); x ++)
          dispose_element(elem->m_array->Get(x));
        m_spare_arrays.Add(elem->m_array);
        elem->m_array = NULL;
      }
      if (elem->m_object_names)
      {
        m_spare_object_names.Add(elem->m_object_names);
        elem->m_object_names = NULL;
      }
      m_spare_elements.Add(elem);
    }

private:
    WDL_PtrList<wdl_json_element> m_spare_elements;
    WDL_PtrList<WDL_PtrList<wdl_json_element> > m_spare_arrays;
    WDL_PtrList<WDL_PtrList<const char> > m_spare_object_names;

    const char *skip_whitespace(const char *rdptr, const char *rdptr_end)
    {
      for (;;)
      {
        while (rdptr < rdptr_end && (*rdptr>=0 && *rdptr <=' ')) rdptr++;
        if (rdptr+1 < rdptr_end && *rdptr == '/' && rdptr[1] == '/')
          while (rdptr < rdptr_end && *rdptr != '\n') rdptr++;
        else if (rdptr+1 < rdptr_end && *rdptr == '/' && rdptr[1] == '*')
        {
          while (rdptr+1 < rdptr_end && (rdptr[0] != '*' || rdptr[1] != '/')) rdptr++;
          if (rdptr+1 < rdptr_end) rdptr+=2;
        }
        else
          return rdptr;
      }
    }
    static bool token_char(char c)
    {
      return (c>='0'&&c<='9') ||
             (c>='a'&&c<='z') ||
             (c>='A'&&c<='Z') ||
             c=='+' ||
             c=='-' ||
             c=='.' ||
             c=='_';
    }
    const char *parse_internal(const char *rdptr, const char *rdptr_end, wdl_json_element **elem)
    {
      rdptr=skip_whitespace(rdptr,rdptr_end);
      if (rdptr >= rdptr_end) { eof: m_err_rdptr=rdptr; m_err="Unexpected EOF"; return NULL; }

      if (*rdptr == '[' || *rdptr == '{')
      {
        const char endchar = *rdptr == '[' ? ']' : '}';
        wdl_json_element *obj = *elem = new_element(*rdptr);

        for (;;)
        {
          rdptr = skip_whitespace(rdptr+1,rdptr_end);
          if (rdptr >= rdptr_end) goto eof;

          if (*rdptr == endchar) return rdptr+1; // [ ] and { } are valid, as are [ foo, ] and { foo: bar, }

          wdl_json_element *obj1 = NULL, *obj2 = NULL;
          const char *prevrdptr = rdptr;
          rdptr = parse_internal(rdptr, rdptr_end, &obj1);
          if (!rdptr) { dispose_element(obj1); return NULL; }
          if (!obj1) break;
          if (endchar == '}' && !obj1->m_value) { rdptr = prevrdptr; dispose_element(obj1); break; }

          rdptr = skip_whitespace(rdptr,rdptr_end);
          if (rdptr >= rdptr_end) { dispose_element(obj1); goto eof; }

          if (endchar == '}')
          {
            if (*rdptr != ':') { dispose_element(obj1); break; }
            rdptr = skip_whitespace(rdptr+1,rdptr_end);
            if (rdptr >= rdptr_end) { dispose_element(obj1); goto eof; }
            rdptr = parse_internal(rdptr, rdptr_end, &obj2);
            if (!rdptr) { dispose_element(obj1); dispose_element(obj2); return NULL; }
            if (!obj2) { dispose_element(obj1); break; }
            obj->m_object_names->Add(obj1->m_value);
            obj->m_array->Add(obj2);
          }
          else obj->m_array->Add(obj1);

          rdptr = skip_whitespace(rdptr,rdptr_end);
          if (rdptr >= rdptr_end) goto eof;
          if (*rdptr == endchar) return rdptr+1;
          if (*rdptr != ',') break;
        }
      }
      else if (*rdptr == '"' || *rdptr == '\'')
      {
        const char endchar_str = *rdptr;
        rdptr++;
        m_tmp.Set("");
        for (;;)
        {
          if (rdptr >= rdptr_end) goto eof;
          if (*rdptr == endchar_str)
          {
            *elem = new_element(endchar_str, m_tmp.Get(), m_tmp.GetLength());
            return rdptr+1;
          }
          if (*rdptr == '\r' || *rdptr == '\n') break;

          if (*rdptr == '\\')
          {
            if (++rdptr >= rdptr_end) goto eof;
            switch (*rdptr)
            {
              case '\n': break;
              case '\r': if (rdptr+1 < rdptr_end && rdptr[1] == '\n') rdptr++; break;
              case '\\': m_tmp.Append("\""); break;
              case '/':  m_tmp.Append("/"); break;
              case '"':  m_tmp.Append("\""); break;
              case '\'': m_tmp.Append("\'"); break;
              case 'b':  m_tmp.Append("\b"); break;
              case 'r':  m_tmp.Append("\r"); break;
              case 'n':  m_tmp.Append("\n"); break;
              case 't':  m_tmp.Append("\t"); break;
              case 'f':  m_tmp.Append("\f"); break;
              case 'u':
                {
                  int rv = 0;
                  for (int i = 0; i < 4; i ++)
                  {
                    if (++rdptr >= rdptr_end) goto eof;
                    const char c = *rdptr;
                    if (c >= '0' && c <= '9') rv = rv*16 + (c - '0');
                    else if (c >= 'A' && c <= 'F') rv = rv*16 + 10 + (c - 'A');
                    else if (c >= 'a' && c <= 'f') rv = rv*16 + 10 + (c - 'a');
                    else goto syntax_error;
                  }
                  char tmp[8];
                  int l = wdl_utf8_makechar(rv,tmp,sizeof(tmp));
                  if (WDL_NORMALLY(l>0)) m_tmp.Append(tmp,l);
                }
              break;
              default: m_tmp.Append(rdptr-1,2); break;
            }
          }
          else m_tmp.Append(rdptr,1);
          rdptr++;
        }
      }
      else if (token_char(*rdptr))
      {
        int l = 1;
        // does not validate the number/string/whatever
        while (rdptr+l < rdptr_end && token_char(rdptr[l])) l++;
        if (rdptr+l >= rdptr_end) goto eof;
        *elem = new_element(0, rdptr, l);
        return rdptr+l;
      }

syntax_error:
      m_err_rdptr=rdptr;
      m_err="Syntax error";
      return NULL;
    }

    wdl_json_element *new_element(char lc, const char *v=NULL, int vlen=0)
    {
      wdl_json_element *e = m_spare_elements.Pop();
      if (!e) e = new wdl_json_element;
      e->m_value_string = (lc == '"');
      e->m_value = NULL;
      e->m_array = NULL;
      e->m_object_names = NULL;
      if (lc == '[' || lc == '{')
      {
        e->m_array = m_spare_arrays.Pop();
        if (e->m_array) e->m_array->Empty();
        else e->m_array = new WDL_PtrList<wdl_json_element>;

        if (lc == '{')
        {
          e->m_object_names = m_spare_object_names.Pop();
          if (e->m_object_names) e->m_object_names->Empty();
          else e->m_object_names = new WDL_PtrList<const char>;
        }
      }

      if (v)
      {
        if (WDL_NORMALLY(vlen>=0) &&
            WDL_NORMALLY(m_stringstore_pos + vlen + 1 <= m_stringstore.GetSize()))
        {
          char *wr = (char*)m_stringstore.Get() + m_stringstore_pos;
          m_stringstore_pos += vlen + 1;
          memcpy(wr,v,vlen);
          wr[vlen]=0;
          e->m_value = wr;
        }
        else
          e->m_value = "__error";
      }
      return e;
    }
};


#endif
