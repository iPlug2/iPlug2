#ifndef _WDL_HASSTRINGS_H_
#define _WDL_HASSTRINGS_H_

#ifndef WDL_HASSTRINGS_EXPORT
#define WDL_HASSTRINGS_EXPORT
#endif

WDL_HASSTRINGS_EXPORT bool hasStrings_isNonWordChar(int c)
{
  // treat as whitespace when searching for " foo "
  switch (c)
  {
    case 0:
    case 1:
    case ' ':
    case '\t':
    case '.':
    case '/':
    case '\\':
      return true;

    default:
      return false;
  }
}

// these are latin-1 supplemental (first utf-8 byte must be 0xc3), pass second byte&~0x20, second byte
#define IS_UTF8_BYTE2_LATIN1S_A(cc,ccf) ((cc) >= 0x80 && (cc) <= 0x85)
#define IS_UTF8_BYTE2_LATIN1S_C(cc,ccf) ((cc) == 0x87)
#define IS_UTF8_BYTE2_LATIN1S_E(cc,ccf) ((cc) >= 0x88 && (cc) <= 0x8b)
#define IS_UTF8_BYTE2_LATIN1S_I(cc,ccf) ((cc) >= 0x8c && (cc) <= 0x8f)
#define IS_UTF8_BYTE2_LATIN1S_N(cc,ccf) ((cc) == 0x91)
#define IS_UTF8_BYTE2_LATIN1S_O(cc,ccf) ((cc) >= 0x92 && (cc) <= 0x96)
#define IS_UTF8_BYTE2_LATIN1S_U(cc,ccf) ((cc) >= 0x99 && (cc) <= 0x9c)
#define IS_UTF8_BYTE2_LATIN1S_Y(cc,ccf) ((cc) == 0x9d || (ccf) == 0x9f)

// latin extended A
#define IS_UTF8_EXT1A_A(b1, b2) ((b1)==0xc4 && (b2) >= 0x80 && (b2) <= 0x85)
#define IS_UTF8_EXT1A_C(b1, b2) ((b1)==0xc4 && (b2) >= 0x86 && (b2) <= 0x8D)
#define IS_UTF8_EXT1A_D(b1, b2) ((b1)==0xc4 && (b2) >= 0x8E && (b2) <= 0x91)
#define IS_UTF8_EXT1A_E(b1, b2) ((b1)==0xc4 && (b2) >= 0x92 && (b2) <= 0x9B)
#define IS_UTF8_EXT1A_G(b1, b2) ((b1)==0xc4 && (b2) >= 0x9C && (b2) <= 0xa3)
#define IS_UTF8_EXT1A_H(b1, b2) ((b1)==0xc4 && (b2) >= 0xa4 && (b2) <= 0xa7)
#define IS_UTF8_EXT1A_I(b1, b2) ((b1)==0xc4 && (b2) >= 0xa8 && (b2) <= 0xb1)
#define IS_UTF8_EXT1A_J(b1, b2) ((b1)==0xc4 && (b2) >= 0xb4 && (b2) <= 0xb5)
#define IS_UTF8_EXT1A_K(b1, b2) ((b1)==0xc4 && (b2) >= 0xb6 && (b2) <= 0xb8)
#define IS_UTF8_EXT1A_L(b1, b2) ((b1)==0xc4 ? ((b2) >= 0xb9 && (b2) <= 0xbf) : \
                                ((b1)==0xc5 && (b2) >= 0x80 && (b2) <= 0x82))
#define IS_UTF8_EXT1A_N(b1, b2) ((b1)==0xc5 && (b2) >= 0x83 && (b2) <= 0x89)
#define IS_UTF8_EXT1A_O(b1, b2) ((b1)==0xc5 && (b2) >= 0x8c && (b2) <= 0x91)
#define IS_UTF8_EXT1A_R(b1, b2) ((b1)==0xc5 && (b2) >= 0x94 && (b2) <= 0x99)
#define IS_UTF8_EXT1A_S(b1, b2) ((b1)==0xc5 && (b2) >= 0x9a && (b2) <= 0xa1)
#define IS_UTF8_EXT1A_T(b1, b2) ((b1)==0xc5 && (b2) >= 0xa2 && (b2) <= 0xa7)
#define IS_UTF8_EXT1A_U(b1, b2) ((b1)==0xc5 && (b2) >= 0xa8 && (b2) <= 0xb3)
#define IS_UTF8_EXT1A_W(b1, b2) ((b1)==0xc5 && (b2) >= 0xb4 && (b2) <= 0xb5)
#define IS_UTF8_EXT1A_Y(b1, b2) ((b1)==0xc5 && (b2) >= 0xb6 && (b2) <= 0xb8)
#define IS_UTF8_EXT1A_Z(b1, b2) ((b1)==0xc5 && (b2) >= 0xb9 && (b2) <= 0xbe)

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
      if (cb != 'a'-'A')
      {
        if (ca < 0xc3 || ca > 0xc5) return -ca;

        const int ccf = a[++aidx];
        if (ccf < 0x80) return -ca;

        if (ca == 0xc3)
        {
          // latin-1 supplemental
          const int cc = ccf & ~0x20;
          switch (*b)
          {
#define SCAN(ch, CH) case ch: if (!IS_UTF8_BYTE2_LATIN1S_##CH(cc,ccf)) return -ca; break;
          SCAN('a',A)
          SCAN('c',C)
          SCAN('e',E)
          SCAN('i',I)
          SCAN('n',N)
          SCAN('o',O)
          SCAN('u',U)
          SCAN('y',Y)
#undef SCAN
          }
        }
        else
        {
          // latin extended A
          switch (*b)
          {
#define SCAN(ch, CH) case ch: if (!IS_UTF8_EXT1A_##CH(ca,ccf)) return -ca; break;
          SCAN('a',A)
          SCAN('c',C)
          SCAN('d',D)
          SCAN('e',E)
          SCAN('g',G)
          SCAN('h',H)
          SCAN('i',I)
          SCAN('j',J)
          SCAN('k',K)
          SCAN('l',L)
          SCAN('n',N)
          SCAN('o',O)
          SCAN('r',R)
          SCAN('s',S)
          SCAN('t',T)
          SCAN('u',U)
          SCAN('w',W)
          SCAN('y',Y)
          SCAN('z',Z)
#undef SCAN
          }
        }
      }
      else if (ca < 'A' || ca > 'Z') return -ca;
    }
    ++aidx;
    ++b;
    --n;
  }
  return aidx;
}

static const char *hasStrings_scan_for_char_match(const char *p, char v)
{
  if (v < 'a' || v > 'z')
    for (;;)
    {
      char c = *p;
      if (!c) return NULL;
      if (c == v) return p;
      p++;
    }

  switch (v)
  {
#define SCAN(ch, CH) case (ch): for (;;) { \
      unsigned char c = *(const unsigned char *)p; \
      if (!c) return NULL; \
      if ((c|0x20) == (ch)) return p; \
      if (c >= 0xc3) { \
        if (c == 0xc3) { \
          const unsigned char ccf = ((const unsigned char*)p)[1]; \
          const unsigned char cc = ccf & ~0x20; \
          if (IS_UTF8_BYTE2_LATIN1S_##CH(cc,ccf)) return p; \
        } else { \
          if (IS_UTF8_EXT1A_##CH(c, ((const unsigned char*)p)[1])) return p; \
        } \
      } \
      p++; \
    }
    SCAN('a',A)
    SCAN('c',C)
    SCAN('e',E)
    SCAN('i',I)
    SCAN('n',N)
    SCAN('o',O)
    SCAN('u',U)
    SCAN('y',Y)
#undef SCAN

    // latin extended A only
#define SCAN(ch, CH) case (ch): for (;;) { \
      unsigned char c = *(const unsigned char *)p; \
      if (!c) return NULL; \
      if ((c|0x20) == (ch)) return p; \
      if (IS_UTF8_EXT1A_##CH(c, ((const unsigned char*)p)[1])) return p; \
      p++; \
    }

    SCAN('d',D)
    SCAN('g',G)
    SCAN('h',H)
    SCAN('j',J)
    SCAN('k',K)
    SCAN('l',L)
    SCAN('r',R)
    SCAN('s',S)
    SCAN('t',T)
    SCAN('w',W)
    SCAN('z',Z)
#undef SCAN

#undef SCAN
  }

  for (;;)
  {
    char c = *p;
    if (!c) return NULL;
    if ((c|0x20) == v) return p;
    p++;
  }
}

WDL_HASSTRINGS_EXPORT bool WDL_hasStringsEx2(const char **name_list, int name_list_size,
    const LineParser *lp,
    int (*cmp_func)(const char *a, int apos, const char *b, int blen)  // if set, returns length of matched string (0 if no match)
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

        bool use_cmp_func = cmp_func != NULL && !(TOP_OF_STACK&1);

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
                  wc_left=wc_right=2;
                  use_cmp_func = false;
                }
              }
            break;
          }
        }

        bool matched = false;
        for (int i = 0; i < name_list_size; i ++)
        {
          const char *name = name_list[i];
          const char *t = name;

#define MATCH_RIGHT_CHECK_WORD(SZ) \
                (wc_right == 0 || ((const unsigned char*)(t))[SZ] < 2 || (wc_right > 1 && hasStrings_isNonWordChar((t)[SZ])))

#define MATCH_LEFT_SKIP_TO_WORD() do { \
                unsigned char lastchar = *(unsigned char*)t++; \
                if (lastchar < 2 || (wc_left>1 && hasStrings_isNonWordChar(lastchar))) break; \
              } while (t[0])

          if (use_cmp_func)
          {
            while (t[0])
            {
              const int lln = cmp_func(t,(int) (t-name),n,ln);
              if (lln && MATCH_RIGHT_CHECK_WORD(lln)) { matched = true; break; }
              if (wc_left > 0)
                MATCH_LEFT_SKIP_TO_WORD();
              else
                t++;
            }
          }
          else
          {
            const char n0 = n[0];
            if (wc_left>0)
            {
              for (;;)
              {
                t = hasStrings_scan_for_char_match(t,n0);
                if (!t) break;
                if (t==name || t[-1] == 1 || (wc_left>1 && hasStrings_isNonWordChar(t[-1])))
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

WDL_HASSTRINGS_EXPORT bool WDL_hasStringsEx(const char *name, const LineParser *lp,
     int (*cmp_func)(const char *a, int apos, const char *b, int blen)  // if set, returns length of matched string (0 if no match)
    )
{
  return WDL_hasStringsEx2(&name,1,lp,cmp_func);
}

WDL_HASSTRINGS_EXPORT bool WDL_hasStrings(const char *name, const LineParser *lp)
{
  return WDL_hasStringsEx(name,lp,NULL);
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
      char *wr = p;
      while (*p)
      {
        unsigned char c = *(unsigned char*)p++;
        if (c >= 'A' && c <= 'Z') c+='a'-'A';
        else if (c == 0xC3)
        {
          const unsigned char ccf = *(unsigned char*)p;
          const unsigned char cc = ccf & ~0x20;
          if (IS_UTF8_BYTE2_LATIN1S_A(cc,ccf)) c = 'a';
          else if (IS_UTF8_BYTE2_LATIN1S_C(cc,ccf)) c = 'c';
          else if (IS_UTF8_BYTE2_LATIN1S_E(cc,ccf)) c = 'e';
          else if (IS_UTF8_BYTE2_LATIN1S_I(cc,ccf)) c = 'i';
          else if (IS_UTF8_BYTE2_LATIN1S_N(cc,ccf)) c = 'n';
          else if (IS_UTF8_BYTE2_LATIN1S_O(cc,ccf)) c = 'o';
          else if (IS_UTF8_BYTE2_LATIN1S_U(cc,ccf)) c = 'u';
          else if (IS_UTF8_BYTE2_LATIN1S_Y(cc,ccf)) c = 'y';

          if (c != 0xC3) p++;
        }
        // we could also convert latin extended A characters to ascii here, but meh
        *wr++ = c;
      }
      *wr=0;
    }
  }

  return lp->getnumtokens()>0;
}

#endif
