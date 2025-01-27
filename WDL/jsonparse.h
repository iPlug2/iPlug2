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
    wdl_json_element(char lc, const char *v=NULL, int vlen=0) :
        m_array(NULL), m_object_names(NULL), m_value_string(lc == '"')
    {
      m_array = lc == '[' || lc == '{' ? new WDL_PtrList<wdl_json_element> : NULL;
      m_object_names = lc == '{' ? new WDL_PtrList<char> : NULL;
      m_value = (char*) (v ? malloc(vlen+1) : NULL);
      if (m_value)
      {
        memcpy(m_value,v,vlen);
        m_value[vlen]=0;
      }
    }
    ~wdl_json_element()
    {
      free(m_value);
      if (m_array) m_array->Empty(true);
      if (m_object_names) m_object_names->Empty(true,free);
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
    WDL_PtrList<char> *m_object_names;
    char *m_value; // de-escaped string value, or raw value (true/false/null, 1, 1.234)

    bool m_value_string; // true if m_value was a string value
};

class wdl_json_parser
{
  public:
    wdl_json_parser() : m_err(NULL), m_err_rdptr(NULL) { }

    const char *m_err;
    const char *m_err_rdptr;
    WDL_FastString m_tmp;

    wdl_json_element *parse(const char *rdptr, int rdptr_len)
    {
      m_err = m_err_rdptr = NULL;
      wdl_json_element *elem = NULL;
      parse_internal(rdptr,rdptr+rdptr_len,&elem);
      return elem;
    }
private:
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
        wdl_json_element *obj = *elem = new wdl_json_element(*rdptr);

        for (;;)
        {
          rdptr = skip_whitespace(rdptr+1,rdptr_end);
          if (rdptr >= rdptr_end) goto eof;

          if (*rdptr == endchar) return rdptr+1; // [ ] and { } are valid, as are [ foo, ] and { foo: bar, }

          wdl_json_element *obj1 = NULL, *obj2 = NULL;
          const char *prevrdptr = rdptr;
          rdptr = parse_internal(rdptr, rdptr_end, &obj1);
          if (!rdptr) { delete obj1; return NULL; }
          if (!obj1) break;
          if (endchar == '}' && !obj1->m_value) { rdptr = prevrdptr; delete obj1; break; }

          rdptr = skip_whitespace(rdptr,rdptr_end);
          if (rdptr >= rdptr_end) { delete obj1; goto eof; }

          if (endchar == '}')
          {
            if (*rdptr != ':') { delete obj1; break; }
            rdptr = skip_whitespace(rdptr+1,rdptr_end);
            if (rdptr >= rdptr_end) { delete obj1; goto eof; }
            rdptr = parse_internal(rdptr, rdptr_end, &obj2);
            if (!rdptr) { delete obj1; delete obj2; return NULL; }
            if (!obj2) { delete obj1; break; }
            obj->m_object_names->Add(strdup(obj1->m_value));
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
            *elem = new wdl_json_element(endchar_str, m_tmp.Get(), m_tmp.GetLength());
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
        *elem = new wdl_json_element(0, rdptr, l);
        return rdptr+l;
      }

syntax_error:
      m_err_rdptr=rdptr;
      m_err="Syntax error";
      return NULL;
    }
};


#endif
