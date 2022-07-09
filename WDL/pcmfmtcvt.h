/*
    WDL - pcmfmtcvt.h
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

#ifndef _PCMFMTCVT_H_
#define _PCMFMTCVT_H_


#include "wdltypes.h"
#include <math.h>

#ifndef PCMFMTCVT_DBL_TYPE
#define PCMFMTCVT_DBL_TYPE double
#endif

#define INT16_TO_float INT16_TO_double
#define float_TO_INT16 double_TO_INT16
#define float_to_i32 double_to_i32
#define float_to_i24 double_to_i24
#define i24_to_float i24_to_double
#define i32_to_float i32_to_double

#define pcmToFloats pcmToDoubles
#define floatsToPcm doublesToPcm


template<class T> static void INT16_TO_double(T &out, int in)
{
  out = (T) (in/32768.0);
}
template<class T> static void double_TO_INT16(T &out, double in)
{
  in *= 32768.0;
  if (in <= -32768.0) out = -32768;
  else if (in >= 32767.0) out = 32767;
  else out = (T) floor(in + 0.5);
}

template<class T> static void i32_to_double(int i32, T *p)
{
  *p = i32 * (1.0 / 2147483648.0);
}

template<class T> static void i24_to_double(const unsigned char *i24, T *p)
{
  // little endian
  int val=(i24[0]) | (i24[1]<<8) | (i24[2]<<16);
  if (val&0x800000) val|=0xFF000000;
  *p = (T) (((double) val) * (1.0 / 8388608.0));
}

template<class T> static void double_to_i32(const T *vv, int *i32)
{
  const double v = *vv * 2147483648.0;
  if (v <= -2147483648.0) *i32 = 0x80000000;
  else if (v >= 2147483647.0) *i32 = 0x7FFFFFFF;
  else *i32 = (int) floor(v + 0.5);
}

static WDL_STATICFUNC_UNUSED int double_to_int_24(const double vv)
{
  const double v = vv * 8388608.0;
  if (v <= -8388608.0) return -0x800000;
  if (v >= 8388607.0)  return  0x7fffff;
  return (int) floor(v + 0.5);
}

static WDL_STATICFUNC_UNUSED int double_to_int_x(const double vv, int bits)
{
  const double sc = (double) (1<<(bits-1));
  const double v = vv * sc;
  if (v <= -sc) return -(1<<(bits-1));
  if (v >= sc-1.0) return (1<<(bits-1))-1;
  return (int) floor(v + 0.5);
}

template<class T> static void double_to_i24(const T *vv, unsigned char *i24)
{
  const int i = double_to_int_24(*vv);
  i24[0]=i&0xff;
  i24[1]=(i>>8)&0xff;
  i24[2]=(i>>16)&0xff;
}

template<class T> static void pcmToDoubles(void *src, int items, int bps, int src_spacing, T *dest, int dest_spacing, int byteadvancefor24=0)
{
  if (bps == 32)
  {
    int *i1=(int *)src;
    while (items--)
    {          
      i32_to_double(*i1,dest);
      i1+=src_spacing;
      dest+=dest_spacing;      
    }
  }
  else if (bps == 24)
  {
    unsigned char *i1=(unsigned char *)src;
    int adv=3*src_spacing+byteadvancefor24;
    while (items--)
    {          
      i24_to_double(i1,dest);
      dest+=dest_spacing;
      i1+=adv;
    }
  }
  else if (bps == 16)
  {
    short *i1=(short *)src;
    while (items--)
    {          
      INT16_TO_double(*dest,*i1);
      i1+=src_spacing;
      dest+=dest_spacing;
    }
  }
}

template<class T> static void doublesToPcm(const T *src, int src_spacing, int items, void *dest, int bps, int dest_spacing, int byteadvancefor24=0)
{
  if (bps==32)
  {
    int *o1=(int*)dest;
    while (items--)
    {
      double_to_i32(src,o1);
      src+=src_spacing;
      o1+=dest_spacing;
    }
  }
  else if (bps == 24)
  {
    unsigned char *o1=(unsigned char*)dest;
    int adv=dest_spacing*3+byteadvancefor24;
    while (items--)
    {
      double_to_i24(src,o1);
      src+=src_spacing;
      o1+=adv;
    }
  }
  else if (bps==16)
  {
    short *o1=(short*)dest;
    while (items--)
    {
      double_TO_INT16(*o1,*src);
      src+=src_spacing;
      o1+=dest_spacing;
    }
  }
}

#endif //_PCMFMTCVT_H_
