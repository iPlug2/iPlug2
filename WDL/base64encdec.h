#ifndef _BASE64ENCDEC_H_
#define _BASE64ENCDEC_H_

static void base64encode(const unsigned char *in, char *out, int len)
{
  char alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  int shift = 0;
  int accum = 0;

  while (len>0)
  {
    len--;
    accum <<= 8;
    shift += 8;
    accum |= *in++;
    while ( shift >= 6 )
    {
      shift -= 6;
      *out++ = alphabet[(accum >> shift) & 0x3F];
    }
  }
  if (shift == 4)
  {
    *out++ = alphabet[(accum & 0xF)<<2];
    *out++='=';  
  }
  else if (shift == 2)
  {
    *out++ = alphabet[(accum & 0x3)<<4];
    *out++='=';  
    *out++='=';  
  }

  *out++=0;
}

static int base64decode(const char *src, unsigned char *dest, int destsize)
{
  unsigned char *olddest=dest;

  int accum=0;
  int nbits=0;
  while (*src)
  {
    int x=0;
    char c=*src++;
    if (c >= 'A' && c <= 'Z') x=c-'A';
    else if (c >= 'a' && c <= 'z') x=c-'a' + 26;
    else if (c >= '0' && c <= '9') x=c-'0' + 52;
    else if (c == '+') x=62;
    else if (c == '/') x=63;
    else break;

    accum <<= 6;
    accum |= x;
    nbits += 6;   

    while (nbits >= 8)
    {
      if (--destsize<0) break;
      nbits-=8;
      *dest++ = (char)((accum>>nbits)&0xff);
    }

  }
  return dest-olddest;
}

#endif // _BASE64ENCDEC_H_