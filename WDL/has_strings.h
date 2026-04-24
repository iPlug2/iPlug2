#ifndef _WDL_HASSTRINGS_H_
#define _WDL_HASSTRINGS_H_

/*
 does user defined string matching.

 searching for:
     one two three
 matches all strings that contain the substrings "one" "two" and "three".

 quotes can be used to search for strings that contain spaces:
     "one two"
 matches all strings that contain the substring "one two"

 if quotes are used on a string that does not contain spaces, a word match is used.
     "one"
 matches all strings that contain the word "one". e.g. "bone" does not match, but "thirty-one" does.

 if the quoted string has leading or trailing spaces, then they specify a word boundary:
     " one"
 will match "onerous" but not match "bone", and:
     "one "
 will match "bone" but not match "onerous"

 you can specify NOT (must be upper-case) as a prefix to a string/word:
     NOT one
 matches all strings that do not contain the substring "one"

 you can specify OR (must be upper-case) to change the default AND behavior to OR.
     one OR two
 matches all strings that contain the substring "one" or contain the substring "two"

 you can specify parentheses (must be separated by spaces) to group logic:
     one OR ( two three )
 matches all strings that contain the substring "one", or contain both of the substrings "two" and "three"

 unquoted substrings that begin with ^ will match the start of the string only:
     ^one
 will match "one time I saw" but not match "there is one thing"

 unquoted substrings that end with $ will match the end of the string only:
     one$
 will match "there can be only one" but not match "there is one thing"




 use this to preprocess a search filter:
#include "WDL/lineparse.h"
 LineParser lp;
 bool filter_en = WDL_makeSearchFilter("filter string", &lp);

 // then for each item:
 if (!filter_en || WDL_hasStrings(string_in_question, &lp)) { matches }


*/

#ifndef WDL_HASSTRINGS_EXPORT
#define WDL_HASSTRINGS_EXPORT
#endif

WDL_HASSTRINGS_EXPORT const char *hasStrings_rewutf8(const char *str, const char *base)
{
#ifdef WDL_HASSTRINGS_REWUTF8_HOOK
  WDL_HASSTRINGS_REWUTF8_HOOK(str, base)
#endif
  while (str > base && (*(unsigned char *)str & 0xC0) == 0x80) str--;
  return str;
}
WDL_HASSTRINGS_EXPORT int hasStrings_isNonWordChar(const char *cptr)
{
  // treat non-alnum non-utf-8 as whitespace when searching for " foo "
  const unsigned char c = *(const unsigned char *)cptr;
  if (c < 128)
  {
    if ((c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        (c >= '0' && c <= '9'))
    {
      return 0;
    }

    return 1; // non-alnum ascii are non-word chars
  }

  // most UTF-8 characters are word characters
  if (c == 0xC2)
  {
    // except UTF-8 U+A1 to U+B1, U+B4, U+B6, U+B7, U+BB, U+BF
    const unsigned char c2 = ((unsigned char*)cptr)[1];
    if (c2 >= 0xa1)
    {
      switch (c2)
      {
        case 0xB4: case 0xB6: case 0xB7: case 0xBB: case 0xBF:
          return 1;
        default:
          if (c2 <= 0xB1) return 1;
        break;
      }
    }
  }
  else if (c == 0xE2)
  {
    unsigned char c2;
    // except UTF-8 U+2000-U+206F and U+20A0-U+20BF
    switch (((unsigned char*)cptr)[1])
    {
      case 0x80:
        c2 = ((unsigned char*)cptr)[2];
        if (c2 >= 0x80 && c2 <= 0xBF) return 1;
      break;
      case 0x81:
        c2 = ((unsigned char*)cptr)[2];
        if (c2 >= 0x80 && c2 <= 0xAF) return 1;
      break;
      case 0x82: // U+20A0-U+20BF (currency signs)
        c2 = ((unsigned char*)cptr)[2];
        if (c2 >= 0xa0 && c2 <= 0xBF) return 1;
      break;
    }
  }

  return 0;
}

#define WDL_IS_UTF8_CONTINUATION_BYTE(x) ((x)>=0x80 && (x)<0xC0)
#define WDL_IS_UTF8_2BYTE_PREFIX(x) ((x)>=0xC2 && (x)<0xe0)

#include "utf8_extended.h"
#include "utf8_casefold.h"

// returns negative if does not match but more of a is available to search
// returns 0 if done searching without match
// returns >0 if matches (return bytelen of match)
// note that this assumes that b was preprocessed by WDL_makeSearchFilter and that strlen(b) >= n
WDL_HASSTRINGS_EXPORT int hasStrings_utf8cmp(const unsigned char * const a, const unsigned char *b, unsigned int n)
{
  int aidx=0;
  while (n)
  {
    int ca=a[aidx], cb=b[0];
    // ca may be any character (including A-Z), utf8, cb will never be A-Z or NUL
    WDL_ASSERT(cb != 0 && !(cb >= 'A' && cb <= 'Z'));
    cb -= ca;
    // if ca is A, and cb is a, cb will be 'a'-'A'
    if (cb)
    {
      if (ca < 0x80)
      {
        if (cb != 'a'-'A' || ca < 'A' || ca > 'Z') return -ca;
      }
      else
      {
        if (WDL_IS_UTF8_2BYTE_PREFIX(ca))
        {
          unsigned char b1 = ca, b2 = a[aidx+1];

          wdl_utf8_2byte_casefold(b1,b2);
          if (b1 != b[0]) // folded, or non-folded, no match
          {
            const int skipl = WDL_IS_UTF8_SKIPPABLE(b1,b2); // if WDL_IS_UTF8_SKIPPABLE ever updated for 3 byte skips, need to move this out of here
            if (skipl)
            {
              aidx += skipl;
              continue;
            }
            return -ca;
          }
          aidx+=2;
          if (!b2)
          {
            b++;
            n--;
            continue;
          }
          if (n < 2 || b2 != b[1]) return -ca;
          b+=2;
          n-=2;
          continue;
        }
        else if (WDL_IS_UTF8_CONTINUATION_BYTE(ca) && aidx > 0 && WDL_IS_UTF8_2BYTE_PREFIX(a[aidx-1]))
        {
          // characters differ only by continuation byte, fold this to see if it matches
          unsigned char lc = a[aidx-1], b1 = lc, b2 = ca;
          wdl_utf8_2byte_casefold(b1,b2);
          WDL_ASSERT(b[-1] == lc);
          if (!b2 || lc != b1 || b[0] != b2) return -1;
        }
        else if (ca == 0xE2 && cb == ('\''-0xE2) && a[aidx+1] == 0x80 && (a[aidx+2]&~1) == 0x98)
        {
          aidx+=2;
        }
        else return -ca;
      }
    }
    ++aidx;
    ++b;
    --n;
  }
  return aidx;
}

static const char *hasStrings_scan_for_char_match(const char *p, char v)
{
  return (const char *)wdl_utf8_scan_unfolded((const unsigned char *)p,(unsigned char)v);
}

WDL_HASSTRINGS_EXPORT const char *hasStrings_skipSkippable(const char *cptr)
{
  int skip;
  while ((skip=WDL_IS_UTF8_SKIPPABLE(((unsigned char*)cptr)[0],((unsigned char*)cptr)[1]))>0) cptr+=skip;
  return cptr;
}

WDL_HASSTRINGS_EXPORT bool WDL_hasStringsEx2(const char **name_list, int name_list_size, const LineParser *lp
#ifdef WDL_HASSTRINGS_EXTRA_PARAMETERS
   WDL_HASSTRINGS_EXTRA_PARAMETERS
#endif
    )
{
  if (!lp) return true;
  const int ntok = lp->getnumtokens();
  if (ntok<1) return true;

  char stack_[1024]; // &1=not bit, 0x10 = ignoring subscopes, &2= state when 0x10 set
  int stacktop = 0, stacktop_v;
#define TOP_OF_STACK stacktop_v
#define PUSH_STACK(x) do { if (stacktop < (int)sizeof(stack_) - 1) stack_[stacktop++] = stacktop_v&0xff; stacktop_v = (x); } while(0)
#define POP_STACK() (stacktop_v = stack_[--stacktop])
  TOP_OF_STACK = 0;

  char matched_local=-1; // -1 = first eval for scope, 0=did not pass scope, 1=OK, 2=ignore rest of scope
  for (int x = 0; x < ntok; x ++)
  {
    const char *n=lp->gettoken_str(x);
    
    if (n[0] == '(' && !n[1] && !lp->gettoken_quotingchar(x))
    {
      if (!(matched_local&1))
      {
        TOP_OF_STACK |= matched_local | 0x10;
        matched_local=2; // ignore subscope
      }
      else 
      {
        matched_local = -1; // new scope
      }

      PUSH_STACK(0);
    }
    else if (n[0] == ')' && !n[1] && stacktop && !lp->gettoken_quotingchar(x))
    {
      if (POP_STACK()&0x10)
      {
        // restore state
        matched_local = TOP_OF_STACK&2;
      }
      else
      {
        matched_local = (matched_local != 0 ? 1 : 0) ^ (TOP_OF_STACK&1);
      }
      TOP_OF_STACK = 0;
    }
    else if (n[0] == 'O' && n[1] == 'R' && !n[2] && matched_local != 2 && !lp->gettoken_quotingchar(x))
    {
      matched_local = (matched_local > 0) ? 2 : -1;
      TOP_OF_STACK = 0;
    }
    else if (matched_local&1) // matches 1, -1
    {
      int ln = (int)strlen(n);
      if (ln>0)
      {
        // ^foo -- string starts (or follows \1 separator with) foo
        // foo$ -- string ends with foo (or is immediately followed by \1 separator)
        // " foo ", "foo ", " foo" include end of string/start of string has whitespace
        int wc_left = 0; // 1=require \1 or start of string, 2=require space or \1 or start
        int wc_right = 0; // 1=require \1 or \0, 2 = require space or \1 or \0
        // perhaps wc_left/wc_right of 2 should also match non-alnum characters in addition to space?
        if (ln>1)
        {
          switch (*n) 
          {
            case ' ': 
              if (*++n != ' ') wc_left=2;
              // else { multiple characters of whitespace = literal whitespace search (two spaces requires a single space, etc) }

              ln--;
            break;
            case '^': 
              ln--; 
              n++; 
              wc_left=1; 
            break;
            // upper case being here implies it is almost certainly NOT/AND due to postprocessing in WDL_makeSearchFilter
            case 'N':
              if (WDL_NORMALLY(!strcmp(n,"NOT") && !lp->gettoken_quotingchar(x)))
              {
                TOP_OF_STACK^=1;
                continue;
              }
            break;
            case 'A':
              if (WDL_NORMALLY(!strcmp(n,"AND") && !lp->gettoken_quotingchar(x)))
              {
                // ignore unquoted uppercase AND
                continue;
              }
            break;
          }
        }
        if (ln>1)
        {
          switch (n[ln-1]) 
          {
            case ' ':               
              if (n[--ln - 1] != ' ') wc_right=2;
              // else { multiple characters of whitespace = literal whitespace search (two spaces requires a single space, etc) }
            break;
            case '$': 
              ln--; 
              wc_right++; 
            break;
          }
        }

        if (!wc_left && !wc_right && *n)
        {
          switch (lp->gettoken_quotingchar(x))
          {
            case '\'':
            case '"':
              { // if a quoted string has no whitespace in it, treat as whole word search
                const char *p = n;
                while (*p && *p != ' ' && *p != '\t') p++;
                if (!*p)
                {
                  // though if leading char is non-word, don't require previous character to be nonword
                  if (!hasStrings_isNonWordChar(n)) wc_left=2;
                  // and if trailing char is non-word, don't require next character to be nonword
                  if (!hasStrings_isNonWordChar(hasStrings_rewutf8(p-1,n))) wc_right=2;
                }
              }
            break;
          }
        }

        bool matched = false;

#ifdef WDL_HASSTRINGS_PRE_MATCH
        if (!wc_left && !wc_right && WDL_HASSTRINGS_PRE_MATCH(n))
          matched = true;
        else
#endif
        for (int i = 0; i < name_list_size; i ++)
        {
          const char *name = name_list[i];
          const char *t = name;

#define MATCH_RIGHT_CHECK_WORD(SZ) \
                (wc_right == 0 || \
                  ((const unsigned char*)(t))[SZ] < 2 || \
                  (wc_right > 1 && hasStrings_isNonWordChar(hasStrings_skipSkippable((t)+(SZ)))) \
                )

#define MATCH_LEFT_SKIP_TO_WORD() do { \
                if (*(unsigned char*)t < 2) { t++; break; } \
                if (wc_left>1) { const int l = hasStrings_isNonWordChar(t); if (l > 0) { t+=l; break; } } \
                t++; \
              } while (t[0])

          {
            const char n0 = n[0];
            if (wc_left>0)
            {
              for (;;)
              {
                t = hasStrings_scan_for_char_match(t,n0);
                if (!t) break;
                if (t==name || t[-1] == 1 || (wc_left>1 && hasStrings_isNonWordChar(hasStrings_rewutf8(t-1,name))))
                {
                  const int v = hasStrings_utf8cmp((const unsigned char *)t,(const unsigned char *)n,ln);
                  if (v>=0)
                  {
                    if (!v) break;
                    if (MATCH_RIGHT_CHECK_WORD(v)) { matched = true; break; }
                  }
                }
                t++;
              }
            }
            else
            {
              for (;;)
              {
                t = hasStrings_scan_for_char_match(t,n0);
                if (!t) break;
                const int v = hasStrings_utf8cmp((const unsigned char *)t,(const unsigned char *)n,ln);
                if (v>=0)
                {
                  if (!v) break;
                  if (MATCH_RIGHT_CHECK_WORD(v)) { matched = true; break; }
                }
                t++;
              }
            }
          }
#undef MATCH_RIGHT_CHECK_WORD
#undef MATCH_LEFT_SKIP_TO_WORD
          if (matched) break;
        }

        matched_local = (matched?1:0) ^ (TOP_OF_STACK&1);
        TOP_OF_STACK=0;
      }
    }
  }
  while (stacktop > 0) 
  {
    if (POP_STACK() & 0x10) matched_local=TOP_OF_STACK&2;
    else matched_local = (matched_local > 0 ? 1 : 0) ^ (TOP_OF_STACK&1);
  }

  return matched_local!=0;
#undef TOP_OF_STACK
#undef POP_STACK
#undef PUSH_STACK
}

#ifndef WDL_HASSTRINGS_EXTRA_PARAMETERS
WDL_HASSTRINGS_EXPORT bool WDL_hasStringsEx(const char *name, const LineParser *lp)
{
  return WDL_hasStringsEx2(&name,1,lp);
}

WDL_HASSTRINGS_EXPORT bool WDL_hasStrings(const char *name, const LineParser *lp)
{
  return WDL_hasStringsEx2(&name,1,lp);
}
#endif

WDL_HASSTRINGS_EXPORT char *WDL_hasstrings_preproc_searchitem(char *wr, const char *src)
{
  while (*src)
  {
    unsigned char c = *(unsigned char*)src++;
    if (c >= 'A' && c <= 'Z') c+='a'-'A';
    else if (c == 0xE2)
    {
      // convert u+2018/2019 to '
      if (*(unsigned char*)src == 0x80 && (((unsigned char*)src)[1]&~1) == 0x98)
      {
        c = '\'';
        src+=2;
      }
    }
    else
    {
      const int skipl = WDL_IS_UTF8_SKIPPABLE(c, *(unsigned char*)src);
      if (skipl > 0)
      {
        src += skipl-1;
        continue;
      }
      if (WDL_IS_UTF8_2BYTE_PREFIX(c))
      {
        const unsigned char c2 = *(unsigned char*)src;
        unsigned char newc = c, newc2 = c2;
        wdl_utf8_2byte_casefold(newc,newc2);
        if (newc != c || newc2 != c2)
        {
          *wr++ = newc;
          if (newc2) *wr++ = newc2;
          src++;
          continue;
        }
      }
    }

    *wr++ = c;
  }
  *wr=0;
  return wr;
}

WDL_HASSTRINGS_EXPORT bool WDL_makeSearchFilter(const char *flt, LineParser *lp)
{
  if (WDL_NOT_NORMALLY(!lp)) return false;

  if (WDL_NOT_NORMALLY(!flt)) flt="";

#ifdef WDL_LINEPARSER_HAS_LINEPARSERINT
  if (lp->parse_ex(flt,true,false,true)) // allow unterminated quotes
#else
  if (lp->parse_ex(flt,true,false))
#endif
  {
    if (*flt) lp->set_one_token(flt); // failed parsing search string, search as a single token
  }
  for (int x = 0; x < lp->getnumtokens(); x ++)
  {
    char *p = (char *)lp->gettoken_str(x);
    if (lp->gettoken_quotingchar(x) || (strcmp(p,"NOT") && strcmp(p,"AND") && strcmp(p,"OR")))
    {
      WDL_hasstrings_preproc_searchitem(p, p);
    }
  }

  return lp->getnumtokens()>0;
}

#endif
