/*
    WDL - queue.h
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

/*

  This file provides a simple class for a FIFO queue of bytes. It uses a simple buffer,
  so should not generally be used for large quantities of data (it can advance the queue 
  pointer, but Compact() needs to be called regularly to keep memory usage down, and when
  it is called, there's a memcpy() penalty for the remaining data. oh well, is what it is).

  You may also wish to look at fastqueue.h or circbuf.h if these limitations aren't acceptable.

*/

#ifndef _WDL_QUEUE_H_
#define _WDL_QUEUE_H_

#include "heapbuf.h"


class WDL_Queue 
{
public:
  WDL_Queue(int hbgran=4096) : m_hb(hbgran), m_pos(0) { }
  ~WDL_Queue() { }

  template <class T> void* AddT(T* buf)
  {
    return Add(buf, sizeof(T));
  }

  void *Add(const void *buf, int len)
  {
    int olen=m_hb.GetSize();
    if (m_pos >= olen) m_pos=olen=0; // if queue is empty then autoreset it

    char *newbuf=(char *)m_hb.ResizeOK(olen+len,false);
    if (newbuf)
    {
      newbuf += olen;
      if (buf) memcpy(newbuf,buf,len);
    }
    return newbuf; 
  }

  template <class T> T* GetT(T* val=0)
  {
    T* p = (T*) Get(sizeof(T));
    if (val && p) *val = *p;
    return p;
  }
  
  void* Get(int size)
  {
    void* p = Get();
    if (p) Advance(size);
    return p;
  }
    
  void *Get() const
  {
    if (m_pos >= 0 && m_pos < m_hb.GetSize()) return (char *)m_hb.Get()+m_pos;
    return NULL;
  }
  
  void* Rewind()
  {
    m_pos = 0;
    return m_hb.Get();
  }

  int GetSize() const
  {
    return m_hb.GetSize()-m_pos;
  }
  int Available() const { return GetSize(); }

  void Clear()
  {
    m_pos=0;
    m_hb.Resize(0,false);
  }

  void Advance(int bytecnt) 
  { 
    m_pos+=bytecnt; 
    if (m_pos<0)m_pos=0;
    else if (m_pos > m_hb.GetSize()) m_pos=m_hb.GetSize();
  }

  void Compact(bool allocdown=false, bool force=false)
  {
    int olen=m_hb.GetSize();
    if (m_pos > (force ? 0 : olen/2))
    {
      olen -= m_pos;
      if (olen > 0)
      {
        char *a=(char*)m_hb.Get();
        memmove(a,a+m_pos,olen);
      }
      else 
      {
        olen = 0;
      }
      m_hb.Resize(olen,allocdown);
      m_pos=0;
    }
  }

  void SetGranul(int granul) { m_hb.SetGranul(granul); }

  void Prealloc(int sz) { m_hb.Prealloc(sz); }



  // endian-management stuff 

  static void WDL_Queue__bswap_buffer(void *buf, int len) // poorly named! only bswaps on BE, deprecated, use wdl_memcpy_le()
  {
    wdl_memcpy_le(buf,buf,1,len);
  }

  // older API of static functions (that ended up warning a bit anyway)
#define WDL_Queue__AddToLE(q, v) (q)->AddToLE(v)
#define WDL_Queue__AddDataToLE(q,d,ds,us) (q)->AddDataToLE(d,ds,us)
#define WDL_Queue__GetTFromLE(q,v) (q)->GetTFromLE(v)
#define WDL_Queue__GetDataFromLE(q,ds,us) (q)->GetDataFromLE(ds,us)

  template<class T> void AddToLE(T *val)
  {
    void *w = Add(NULL, sizeof(T));
    if (WDL_NORMALLY(w != NULL) && val != NULL)
      wdl_memcpy_le(w, val, 1, sizeof(T));
  }
  void AddDataToLE(const void *data, int datasize, int unitsize)
  {
    char *w = (char *)Add(NULL,datasize);
    if (WDL_NOT_NORMALLY(w == NULL)) return;

    if (WDL_NOT_NORMALLY(unitsize<1)) unitsize=1;
    WDL_ASSERT((datasize % unitsize) == 0);
    if (data) wdl_memcpy_le(w,data,datasize/unitsize,unitsize);
  }


  // NOTE: these thrash the contents of the queue if on BE systems, do not use if you
  // will rewind etc. better to use Get() and wdl_memcpy_le().
  template<class T> T *GetTFromLE(T* val=0)
  {
    T *p = GetT(val);
    if (p)
    {
      wdl_memcpy_le(p,p,1,sizeof(T));
      if (val) *val = *p;
    }
    return p;
  }

  void *GetDataFromLE(int datasize, int unitsize)
  {
    void *data=Get(datasize);
    WDL_ASSERT((datasize % unitsize) == 0);
    if (data && WDL_NORMALLY(unitsize>0)) wdl_memcpy_le(data,data,datasize/unitsize,unitsize);
    return data;
  }


private:
  WDL_HeapBuf m_hb;
  int m_pos;
public:
  int __pad; // keep 8 byte aligned
};

template <class T> class WDL_TypedQueue
{
public:
  WDL_TypedQueue() : m_hb(4096), m_pos(0) { }
  ~WDL_TypedQueue() { }

  T *Add(const T *buf, int len)
  {
    int olen=m_hb.GetSize();
    if (m_pos >= olen) olen=m_pos=0;
    len *= (int)sizeof(T);

    char *newbuf=(char*)m_hb.ResizeOK(olen+len,false);
    if (newbuf)
    {
      newbuf += olen;
      if (buf) memcpy(newbuf,buf,len);
    }
    return (T*) newbuf;
  }

  T *Get() const
  {
    if (m_pos >= 0 && m_pos < m_hb.GetSize()) return (T*)((char *)m_hb.Get()+m_pos);
    return NULL;
  }

  int GetSize() const
  {
    return m_pos < m_hb.GetSize() ? (m_hb.GetSize()-m_pos)/sizeof(T) : 0;
  }
  int Available() const { return GetSize(); }

  void Clear()
  {
    m_pos=0;
    m_hb.Resize(0,false);
  }

  void Advance(int cnt) 
  { 
    m_pos+=cnt*(int)sizeof(T); 
    if (m_pos<0)m_pos=0;
    else if (m_pos > m_hb.GetSize()) m_pos=m_hb.GetSize();
  }

  void Compact(bool allocdown=false, bool force=false)
  {
    int olen=m_hb.GetSize();
    if (m_pos >= (force ? 0 : olen/2))
    {
      olen -= m_pos;
      if (olen > 0)
      {
        char *a=(char*)m_hb.Get();
        memmove(a,a+m_pos,olen);
      }
      else
      {
        olen = 0;
      }
      m_hb.Resize(olen,allocdown);
      m_pos=0;
    }
  }

  void SetGranul(int granul) { m_hb.SetGranul(granul); }
  void Prealloc(int sz) { m_hb.Prealloc(sz * sizeof(T)); }

private:
  WDL_HeapBuf m_hb;
  int m_pos;
public:
  int __pad; // keep 8 byte aligned
};

#endif
