/*
WDL - wdlutf8.h
Copyright (C) 2005 and later, Cockos Incorporated

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

#ifndef _WDLUTF8_H_
#define _WDLUTF8_H_

/* todo: handle overlongs differently? */

#include "wdltypes.h"

#ifndef WDL_WCHAR
  #ifdef _WIN32
    #define WDL_WCHAR WCHAR
  #else
    // this is often 4 bytes on macOS/linux! beware dragons!
    #define WDL_WCHAR wchar_t
  #endif
#endif


// returns size, sets cOut to code point. 
// if invalid UTF-8, sets cOut to first character (as unsigned char).
// cOut may be NULL if you only want the size of the character
static int WDL_STATICFUNC_UNUSED wdl_utf8_parsechar(const char *rd, int *cOut) 
{
  const unsigned char *p = (const unsigned char *)rd;
  const unsigned char b0 = *p;
  unsigned char b1,b2,b3;

  if (cOut) *cOut = b0;
  if (b0 < 0x80) 
  {
    return 1;
  }
  if (((b1=p[1])&0xC0) != 0x80) return 1;

  if (b0 < 0xE0)
  {
    if (!(b0&0x1E)) return 1; // detect overlong
    if (cOut) *cOut = ((b0&0x1F)<<6)|(b1&0x3F);
    return 2;
  }

  if (((b2=p[2])&0xC0) != 0x80) return 1;

  if (b0 < 0xF0)
  {
    if (!(b0&0xF) && !(b1&0x20)) return 1; // detect overlong

    if (cOut) *cOut = ((b0&0x0F)<<12)|((b1&0x3F)<<6)|(b2&0x3f);
    return 3;
  }

  if (((b3=p[3])&0xC0) != 0x80) return 1;

  if (b0 < 0xF8)
  {
    if (!(b0&0x7) && !(b1&0x30)) return 1; // detect overlong

    if (cOut) *cOut = ((b0&7)<<18)|((b1&0x3F)<<12)|((b2&0x3F)<<6)|(b3&0x3F);
    return 4;
  }

  // UTF-8 does not actually support 5-6 byte sequences as of 2003 (RFC-3629)
  // skip them and return _
  if ((p[4]&0xC0) != 0x80) return 1;
  if (b0 < 0xFC) 
  {
    if (cOut) *cOut = '_';
    return 5;
  }

  if ((p[5]&0xC0) != 0x80) return 1;
  if (cOut) *cOut = '_';
  return 6;
}


// makes a character, returns length. does NOT nul terminate.
// returns 0 if insufficient space, -1 if out of range value
static int WDL_STATICFUNC_UNUSED wdl_utf8_makechar(int c, char *dest, int dest_len)
{
  if (c < 0) return -1; // out of range character

  if (c < 0x80)
  {
    if (dest_len<1) return 0;
    dest[0]=(char)c;
    return 1;
  }  
  if (c < 0x800)
  {
    if (dest_len < 2) return 0;

    dest[0]=0xC0|(c>>6);
    dest[1]=0x80|(c&0x3F);
    return 2;
  }
  if (c < 0x10000)
  {
    if (dest_len < 3) return 0;

    dest[0]=0xE0|(c>>12);
    dest[1]=0x80|((c>>6)&0x3F);
    dest[2]=0x80|(c&0x3F);
    return 3;
  }
  if (c < 0x200000)
  {
    if (dest_len < 4) return 0;
    dest[0]=0xF0|(c>>18);
    dest[1]=0x80|((c>>12)&0x3F);
    dest[2]=0x80|((c>>6)&0x3F);
    dest[3]=0x80|(c&0x3F);
    return 4;
  }

  return -1;
}


// invalid UTF-8 are now treated as ANSI characters for this function
// always writes utf-16, even if WDL_WCHAR is 4 bytes
static int WDL_STATICFUNC_UNUSED WDL_MBtoWideStr(WDL_WCHAR *dest, const char *src, int destlenbytes)
{
  WDL_WCHAR *w = dest, *dest_endp = dest+(size_t)destlenbytes/sizeof(WDL_WCHAR)-1;
  if (!dest || destlenbytes < 1) return 0;

  if (src) for (; *src && w < dest_endp; )
  {
    int c,sz=wdl_utf8_parsechar(src,&c);
    if (c >= 0x10000 && c < 0x10FFFF)
    {
      if (w+1 >= dest_endp) break;
      *w++ = 0xD800 + (((c-0x10000)>>10)&0x3FF);
      *w++ = 0xDC00 + (((c-0x10000)&0x3FF));
    }
    else
      *w++ = c;
    src+=sz;
  }
  *w=0; 
  return (int)(w-dest);
}


// like wdl_utf8_makechar, except nul terminates and handles errors differently (returns _ and 1 on errors)
// negative values for character are treated as 0.
static int WDL_STATICFUNC_UNUSED WDL_MakeUTFChar(char* dest, int c, int destlen)
{
  if (destlen < 2)
  {
    if (destlen == 1) dest[0]=0;
    return 0;
  }
  else
  {
    const int v = wdl_utf8_makechar(c>0?c:0,dest,destlen-1);
    if (v < 1) // implies either insufficient space or out of range character
    {
      dest[0]='_';
      dest[1]=0;
      return 1;
    }
    dest[v]=0;
    return v;
  }
}

static int WDL_STATICFUNC_UNUSED WDL_WideToMBStr(char *dest, const WDL_WCHAR *src, int destlenbytes)
{
  char *p = dest, *dest_endp = dest + destlenbytes - 1;
  if (!dest || destlenbytes < 1) return 0;

  if (src) while (*src && p < dest_endp)
  {
    int ch = *src++, v;
    if (ch >= 0xD800 && ch <= 0xD800 + 0x3FF && *src >= 0xDC00 && *src <= 0xDC00 + 0x3FF)
    {
      // surrogate pair
      ch = 0x10000 + ((ch-0xD800)<<10) + (*src++ - 0xDC00);
    }
    v = wdl_utf8_makechar(ch,p,(int)(dest_endp-p));
    if (v > 0)
    {
      p += v;
    }
    else if (v == 0) break; // out of space
  }
  *p=0;
  return (int)(p-dest);
}

// returns >0 if UTF-8, -1 if 8-bit chars occur that are not UTF-8, or 0 if ASCII
static int WDL_STATICFUNC_UNUSED WDL_DetectUTF8(const char *str)
{
  int hasUTF=0;

  if (!str) return 0;
  
  for (;;)
  {
    const unsigned char c = *(const unsigned char *)str;

    if (c < 0xC2 || c > 0xF7) 
    {
      if (!c) return hasUTF;
      if (c >= 0x80) return -1;
      str++;
    }
    else
    {
      const int l = wdl_utf8_parsechar(str,NULL);
      if (l < 2) return -1; // wdl_utf8_parsechar returns length=1 if it couldn't parse UTF-8 properly
      str+=l;
      hasUTF=1;
    }
  }
}


static int WDL_STATICFUNC_UNUSED WDL_utf8_charpos_to_bytepos(const char *str, int charpos)
{
  int bpos = 0;
  while (charpos-- > 0 && str[bpos])
  {
    bpos += wdl_utf8_parsechar(str+bpos,NULL);
  }
  return bpos;
}
static int WDL_STATICFUNC_UNUSED WDL_utf8_bytepos_to_charpos(const char *str, int bytepos)
{
  int bpos = 0, cpos=0;
  while (bpos < bytepos && str[bpos])
  {
    bpos += wdl_utf8_parsechar(str+bpos,NULL);
    cpos++;
  }
  return cpos;
}

#define WDL_utf8_get_charlen(rd) WDL_utf8_bytepos_to_charpos((rd), 0x7fffffff)

static void WDL_STATICFUNC_UNUSED wdl_utf8_set_char_case(char *p, int upper) // upper 1 or -1 only
{
  const unsigned char c1 = (unsigned char)*p;
  WDL_ASSERT(upper == 1 || upper == -1);
  if (c1 >= 'a' && c1 <= 'z')
  {
    if (upper>0) *p += 'A'-'a';
  }
  else if (c1 >= 'A' && c1 <= 'Z')
  {
    if (upper<0) *p -= 'A'-'a';
  }
  else if (c1 >= 0x80)
  {
    const unsigned char cc = (unsigned char)p[1] - 0x80;
    switch (c1)
    {
      case 0xc3: // u+0c0 to u+0ff as 0..0x3f
        if ((cc&~0x20) != 0x17) // all values except 0xc7 and 0xf7
        {
          if (upper>0) p[1] &= ~0x20;
          else p[1] |= 0x20;
        }
      break;
      case 0xc4: // u+100 to u+13f
        if (cc <= 0x37)
        {
          // u+100 to u+137 low bit is lowercase
          if (upper>0) p[1] &= ~1;
          else p[1] |= 1;
        }
        // u+138 is not cased
        else if (cc >= 0x39 && cc < 0x3f)
        {
          // u+139 to u+13e, odd is uppercase
          if ((cc & 1) != (upper>0)) p[1] -= upper;
        }
        else if (cc == 0x3f && upper<0) // u+139 convert to u+140
        {
          p[0]++;
          p[1] -= 0x3f;
        }
      break;
      case 0xc5: // u+140 to u+17f
        // u+149 and u+178 and u+17f are not cased
        if (cc == 0 && upper>0) // u+140 -> u+13f
        {
          p[0]--;
          p[1] |= 0x3f;
        }
        else if (cc >= 0xa && cc <= 0x37) // u+14a to u+177 low bit is lowercase
        {
          if (upper>0) p[1] &= ~1;
          else p[1] |= 1;
        }
        else if ((cc > 0 && cc <= 8) || (cc >= 0x39 && cc <= 0x3e))
        {
          // u+141 to u+148 and u+179 to u+17e have odd=uppercase
          if ((cc & 1) != (upper>0)) p[1] -= upper;
        }
      break;
    }
  }
}

static void WDL_STATICFUNC_UNUSED WDL_utf8_cleanup_truncation(char *buf, size_t bufsz)
{
  unsigned char * const ubuf = (unsigned char *)buf;
  unsigned char *ep = ubuf + bufsz - 2; // point to the last byte before the NUL terminator
  int contcnt = 1;

  // only does anything if strlen(buf)==bufsz-1, removes any trailing partial UTF-8 sequences
  if (!buf || bufsz<2 || strlen(buf) != bufsz-1) return;

  if (*ep < 0x80) return; // last byte is normal ASCII, done
  if (*ep >= 0xC0)
  {
    // last byte could be the start of a UTF-8 sequence, remove it
    *ep = 0;
    return;
  }

  // last byte is a UTF-8 continuation byte
  for (;;)
  {
    unsigned char c;
    if (--ep < ubuf) return; // continuation bytes at start of string, do nothing
    c = *ep;
    if (c >= 0xC0)
    {
      const int needcont = c < 0xE0 ? 1 : c < 0xF0 ? 2 : c < 0xF8 ? 3 : 0;
      if (contcnt < needcont) // insufficient continuation bytes, truncate
        *ep = 0;
      return;
    }
    if (c < 0x80 || ++contcnt > 3) return; // if ascii or more than 3 continuation bytes, not valid UTF-8, do nothing
  }
}

static char *WDL_utf8_cleanup_bad_codepoints(const char *str, char *tmpbuf, int tmpbufsz, int flags)
{
  // flags&1= force
  // drops invalid codepoints if either 'force' is specified or if the ratio of UTF-8 characters to invalid
  // bytes is high enough. returns either buf or a malloc'd buffer (or NULL if no fixing is necessary)

  int utf8bytes = 0, dropbytes = 0, slen = 0, wpos = 0, spos = 0, wbuf_sz;
  char *wbuf = tmpbuf;
  while (str[slen])
  {
    int l;
    if (str[slen] > 0) { slen++; continue; } // skip ascii
    l = wdl_utf8_parsechar(str+slen,NULL);
    if (l == 1 || l > 4) dropbytes += l;
    else utf8bytes += l;
    slen += l;
  }
  if (!dropbytes) return NULL;

  if (!(flags&1) && utf8bytes < dropbytes*5 && utf8bytes < slen*3/4)
    return NULL;

  wbuf_sz = (slen-dropbytes) + 1;
  if (wbuf_sz > tmpbufsz || !tmpbuf) wbuf = (char *)malloc(wbuf_sz);
  if (WDL_NOT_NORMALLY(!wbuf)) return NULL;

  while (str[spos])
  {
    int addl = 1;
    if (str[spos] < 0)
    {
      addl = wdl_utf8_parsechar(str+spos,NULL);
      spos += addl;
      if (addl == 1 || addl > 4) continue;
    }
    else spos++;

    if (WDL_NOT_NORMALLY(wpos+addl >= wbuf_sz)) break;
    memcpy(wbuf+wpos, str+spos-addl, addl);
    wpos += addl;
  }
  wbuf[wpos] = 0;
  WDL_ASSERT(wpos == wbuf_sz-1);
  return wbuf;
}


static WDL_WCHAR *WDL_utf8_to_utf16(const char *str, WDL_WCHAR *tmpbuf, int tmpbufsz_bytes, int flags)
{
  // flags&1= force utf-8 even if likely ansi
  // flagS&2= do not malloc()
  // drops invalid codepoints if either 'force' is specified or if the ratio of UTF-8 characters to invalid
  // bytes is high enough.
  //
  // may return tmpbuf, or a malloc()'d buffer, or NULL.
  //
  // if determines that source is unlikely be to UTF-8, returns NULL (!)

  int utf8bytes = 0, dropbytes = 0, slen = 0, wpos = 0, spos = 0, wbuf_sz = 0;
  WDL_WCHAR *wbuf = tmpbuf;
  while (str[slen])
  {
    int c,l;
    if (str[slen] > 0) { wbuf_sz++; slen++; continue; } // skip ascii
    l = wdl_utf8_parsechar(str+slen,&c);
    if (l == 1 || l > 4) dropbytes += l;
    else
    {
      utf8bytes += l;
      wbuf_sz += c>=0x10000 ? 2 : 1;
    }
    slen += l;
  }

  if (!(flags&1) && utf8bytes < dropbytes*5 && utf8bytes < slen*3/4) // if this passes, string is probably not UTF-8!
    return NULL;

  wbuf_sz++; // terminating nul
  if (wbuf_sz*(int)sizeof(WDL_WCHAR) > tmpbufsz_bytes || !tmpbuf)
  {
    if (!(flags&2))
      wbuf = (WDL_WCHAR *)malloc(wbuf_sz * sizeof(WDL_WCHAR));
    else
      wbuf_sz = tmpbufsz_bytes / sizeof(WDL_WCHAR);
  }
  if (WDL_NOT_NORMALLY(!wbuf)) return NULL;

  while (str[spos])
  {
    int c = str[spos], addl = 1;
    if (c < 0)
    {
      addl = wdl_utf8_parsechar(str+spos,&c);
      spos += addl;
      if (addl == 1 || addl > 4) continue;
      addl = c >= 0x10000 ? 2 : 1;
    }
    else spos++;

    if (wpos+addl >= wbuf_sz)
    {
      WDL_ASSERT(flags & 2);
      break;
    }
    if (addl == 1) wbuf[wpos] = c;
    else
    {
      wbuf[wpos] = 0xD800 + (((c-0x10000)>>10)&0x3FF);
      wbuf[wpos+1] = 0xDC00 + (((c-0x10000)&0x3FF));
    }
    wpos += addl;
  }
  wbuf[wpos] = 0;
  WDL_ASSERT(wpos == wbuf_sz-1);
  return wbuf;
}

#endif
