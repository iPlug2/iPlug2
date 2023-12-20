/*
    WDL - lineparse.h
    Copyright (C) 2005-2014 Cockos Incorporated
    Copyright (C) 1999-2004 Nullsoft, Inc. 
  
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

/*

  This file provides a simple line parsing class. This class was derived from that of NSIS,
  http://nsis.sf.net, but it is no longer compatible (escaped-encodings and multiline C-style comments
  are ignored). 
  
  In particular, it allows for multiple space delimited tokens 
  on a line, with a choice of three quotes (`bla`, 'bla', or "bla") to contain any 
  items that may have spaces.
 
*/

#ifndef WDL_LINEPARSE_H_
#define WDL_LINEPARSE_H_

#include "heapbuf.h"

#ifndef WDL_LINEPARSER_HAS_LINEPARSERINT
#define WDL_LINEPARSER_HAS_LINEPARSERINT
#endif

#ifndef WDL_LINEPARSE_IMPL_ONLY
class LineParserInt // version which does not have any temporary space for buffers (requires use of parseDestroyBuffer)
{
  public:
    int getnumtokens() const { return m_nt-m_eat; }

    #ifdef WDL_LINEPARSE_INTF_ONLY
    // parse functions return <0 on error (-1=mem, -2=unterminated quotes), ignore_commentchars = true means don't treat #; as comments
      int parseDestroyBuffer(char *line, bool ignore_commentchars = true, bool backtickquote = true, bool allowunterminatedquotes = false); 

      double gettoken_float(int token) const;
      int gettoken_int(int token) const;
      unsigned int gettoken_uint(int token) const; // deprecated
      const char *gettoken_str(int token) const;
      char gettoken_quotingchar(int token) const;
      int gettoken_enum(int token, const char *strlist) const; // null separated list
      void insert_token_raw(int token, const char *p); // first character of p is quoting character!
    #endif

    void eattoken() { if (m_eat<m_nt) m_eat++; }


    LineParserInt()
    {
      m_nt=m_eat=0;
      m_tokens=m_toklist_small;
      m_tokenbasebuffer=NULL;
    }
    
    ~LineParserInt()
    {
    }

#endif // !WDL_LINEPARSE_IMPL_ONLY


    
#ifndef WDL_LINEPARSE_INTF_ONLY
   #ifdef WDL_LINEPARSE_IMPL_ONLY
     #define WDL_LINEPARSE_PREFIX LineParserInt::
     #define WDL_LINEPARSE_DEFPARM(x)
   #else
     #define WDL_LINEPARSE_PREFIX
     #define WDL_LINEPARSE_DEFPARM(x) =(x)
   #endif

    int WDL_LINEPARSE_PREFIX parseDestroyBuffer(char *line, bool ignore_commentchars WDL_LINEPARSE_DEFPARM(true), bool backtickquote WDL_LINEPARSE_DEFPARM(true), bool allowunterminatedquotes WDL_LINEPARSE_DEFPARM(false))
    {
      m_nt=0;
      m_eat=0;
      if (!line) return -1;

      m_tokens=m_toklist_small;
      m_tokenbasebuffer = line;
      char thischar;
      while ((thischar=*line) == ' ' || thischar == '\t') line++;
      if (!thischar) return 0;

      for (;;)
      {
        static const char tab[4]={0, '"', '\'', '`'};
        int lstate=0; // 1=", 2=`, 3='
        
        switch (*line)
        {
          case ';': 
          case '#': 
            if (!ignore_commentchars) return 0; // we're done!
          break;
          case '"':  line++; lstate=1; break;
          case '\'': line++; lstate=2; break;
          case '`':  if (backtickquote) { line++; lstate=3; } break;
        }

        const char *basep = line;

        if (!lstate) while ((thischar=*line) && thischar != ' ' && thischar != '\t') line++;
        else while ((thischar=*line) && thischar != tab[lstate]) line++;

        const char oldterm = *line;
        *line=0; // null terminate this token

        if (!adding_token_alloc()) return -1;

        m_tokens[m_nt++] = basep;

        if (!oldterm) 
        {
          if (lstate && !allowunterminatedquotes)
          {
            m_nt = 0;
            return -2;
          }
          return 0;
        }

        line++;
        while ((thischar=*line) == ' ' || thischar == '\t') line++;
        if (!thischar) return 0;
      }
    }


    double WDL_LINEPARSE_PREFIX gettoken_float(int token) const
    {
      const char *t = gettoken_str(token);
      if (!*t) return 0.0;

      char buf[512];
      int ot = 0;
      while (*t&&ot<(int)sizeof(buf)-1) 
      {
        char c=*t++;
        if (c == ',') c = '.';
        buf[ot++]=c;
      }
      buf[ot] = 0;
      return atof(buf);
    }

    int WDL_LINEPARSE_PREFIX gettoken_int(int token) const
    { 
      const char *tok = gettoken_str(token);
      return tok[0] == '-' ? (int)strtol(tok,NULL,10) : (int)strtoul(tok, NULL, strnicmp(tok,"0x",2) ? 10 : 16);
    }

    unsigned int WDL_LINEPARSE_PREFIX gettoken_uint(int token) const // deprecated
    { 
      const char *tok = gettoken_str(token);
      if (WDL_NOT_NORMALLY(tok[0] == '-')) tok++; // legacy behavior, yuck
      return (unsigned int) (int) strtoul(tok, NULL, strnicmp(tok,"0x",2) ? 10 : 16);
    }

    const char * WDL_LINEPARSE_PREFIX gettoken_str(int token) const
    { 
      token+=m_eat;
      if ((unsigned int)token >= m_nt) return "";
      return m_tokens[token]; 
    }

    char WDL_LINEPARSE_PREFIX gettoken_quotingchar(int token) const
    {
      token+=m_eat;
      if ((unsigned int)token >= m_nt) return 0;

      const char *tok = m_tokens[token];
      if (tok != m_tokenbasebuffer) switch (tok[-1])
      {
        case '"':  return '"';
        case '`':  return '`';
        case '\'': return '\'';
      }
      return 0;
    }

    int WDL_LINEPARSE_PREFIX gettoken_enum(int token, const char *strlist) const // null separated list
    {
      int x=0;
      const char *tt=gettoken_str(token);
      if (*tt) while (*strlist)
      {
        if (!stricmp(tt,strlist)) return x;
        while (*strlist) strlist++;
        strlist++;
        x++;
      }
      return -1;
    }

    void WDL_LINEPARSE_PREFIX insert_token_raw(int token, const char *p) // first character of p is quoting character!
    {
      if (WDL_NOT_NORMALLY((unsigned int)token > m_nt)) return;
      if (WDL_NOT_NORMALLY(!adding_token_alloc())) return;
      if ((unsigned int)token < m_nt)
        memmove(m_tokens + token + 1, m_tokens + token, (m_nt-token) * sizeof(const char *));
      m_tokens[token] = p+1;
      m_nt++;
    }

#ifndef WDL_LINEPARSE_IMPL_ONLY
  private:
#endif


   #undef WDL_LINEPARSE_PREFIX
   #undef WDL_LINEPARSE_DEFPARM
#endif // ! WDL_LINEPARSE_INTF_ONLY
    
#ifndef WDL_LINEPARSE_IMPL_ONLY
  protected:

    bool adding_token_alloc()
    {
      if (m_nt < (int) (sizeof(m_toklist_small)/sizeof(m_toklist_small[0])))
        return true;

      m_tokens = m_toklist_big.ResizeOK(m_nt+1,false);
      if (!m_tokens)
      {
        m_nt=0;
        return false;
      }
      if (m_nt == (int) (sizeof(m_toklist_small)/sizeof(m_toklist_small[0])))
        memcpy(m_tokens,m_toklist_small,m_nt*sizeof(const char *));

      return true;
    }

    WDL_TypedBuf<const char *> m_toklist_big;

    unsigned int m_nt, m_eat;

    const char *m_tokenbasebuffer; // points to (mangled) caller's buffer
    const char **m_tokens; // points to m_toklist_small or m_toklist_big

    const char *m_toklist_small[64];
};
#endif//!WDL_LINEPARSE_IMPL_ONLY






// derived 

#ifndef WDL_LINEPARSE_IMPL_ONLY
class LineParser : public LineParserInt
{
  public:
    int parse(const char *line) { return parse_ex(line,false); } // <0 on error, old style (;# starting tokens means comment to EOL)

    #ifdef WDL_LINEPARSE_INTF_ONLY
    // parse functions return <0 on error (-1=mem, -2=unterminated quotes), ignore_commentchars = true means don't treat #; as comments
      int parse_ex(const char *line, bool ignore_commentchars = true, bool backtickquote = true, bool allowunterminatedquotes = false); 
      void set_one_token(const char *ptr);
      char *__get_tmpbuf(const char *line);
    #endif


    LineParser(bool ignoredLegacyValue=false) { }
    
#endif // !WDL_LINEPARSE_IMPL_ONLY


    
#ifndef WDL_LINEPARSE_INTF_ONLY
   #ifdef WDL_LINEPARSE_IMPL_ONLY
     #define WDL_LINEPARSE_PREFIX LineParser::
     #define WDL_LINEPARSE_DEFPARM(x)
   #else
     #define WDL_LINEPARSE_PREFIX
     #define WDL_LINEPARSE_DEFPARM(x) =(x)
   #endif

    int WDL_LINEPARSE_PREFIX parse_ex(const char *line, bool ignore_commentchars WDL_LINEPARSE_DEFPARM(true), bool backtickquote WDL_LINEPARSE_DEFPARM(true), bool allowunterminatedquotes WDL_LINEPARSE_DEFPARM(false))
    {
      return parseDestroyBuffer(__get_tmpbuf(line), ignore_commentchars, backtickquote, allowunterminatedquotes);
    }

    void WDL_LINEPARSE_PREFIX set_one_token(const char *line)
    { 
      m_tokens=m_toklist_small;
      m_tokens[0] = m_tokenbasebuffer = __get_tmpbuf(line);
      m_eat=0;
      m_nt=m_tokenbasebuffer?1:0;
    }

    char * WDL_LINEPARSE_PREFIX __get_tmpbuf(const char *line)
    {
      int linelen = (int)strlen(line);
      
      char *usebuf=m_tmpbuf;
      if (linelen >= (int)sizeof(m_tmpbuf))
      {
        usebuf = (char *)m_tmpbuf_big.ResizeOK(linelen+1,false);
        if (!usebuf) 
        {
          m_nt=0;
          return NULL;
        }
      }
      memcpy(usebuf,line,linelen+1);
      return usebuf;
    }

   #undef WDL_LINEPARSE_PREFIX
   #undef WDL_LINEPARSE_DEFPARM
#endif // ! WDL_LINEPARSE_INTF_ONLY
    
#ifndef WDL_LINEPARSE_IMPL_ONLY
  private:    

    WDL_HeapBuf m_tmpbuf_big;
    char m_tmpbuf[2048];
};
#endif//!WDL_LINEPARSE_IMPL_ONLY







#endif//WDL_LINEPARSE_H_

