/*
    WDL - ptrlist.h
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

  This file provides a simple templated class for a list of pointers. By default this list
  doesn't free any of the pointers, but you can call Empty(true) or Delete(x,true) to delete the pointer,
  or you can use Empty(true,free) etc to call free (or any other function).

  Note: on certain compilers, instantiating with WDL_PtrList<void> bla; will give a warning, since
  the template will create code for "delete (void *)x;" which isn't technically valid. Oh well.

*/

#ifndef _WDL_PTRLIST_H_
#define _WDL_PTRLIST_H_

#include "heapbuf.h"

template<class PTRTYPE> class WDL_PtrList
{
  public:
    explicit WDL_PtrList(int defgran=4096, int prealloc=0) : m_buf(defgran)
    {
      if (prealloc>0) m_buf.Prealloc(prealloc);
    }

    ~WDL_PtrList()
    {
    }

    void Prealloc(int sz) { m_buf.Prealloc(sz); }
    void ResizeToCurrent() { m_buf.ResizeToCurrent(); }

    PTRTYPE **GetList() const { return m_buf.Get(); }
    PTRTYPE *Get(INT_PTR index) const
    {
      PTRTYPE **list = m_buf.Get();
      if (list && (UINT_PTR)index < (UINT_PTR)m_buf.GetSize()) return list[index];
      return NULL;
    }

    int GetSize(void) const { return m_buf.GetSize(); }

    PTRTYPE *Pop()
    {
      const int l = GetSize()-1;
      if (l<0) return NULL;
      PTRTYPE *ret = m_buf.Get()[l];
      m_buf.Resize(l,false);
      return ret;
    }
    PTRTYPE *GetLast() const { return Get(GetSize()-1); }

    int Find(const PTRTYPE *p) const
    {
      if (p)
      {
        PTRTYPE **list=m_buf.Get();
        int x;
        const int n = GetSize();
        for (x = 0; x < n; x ++) if (list[x] == p) return x;
      }
      return -1;
    }
    int FindR(const PTRTYPE *p) const
    {
      if (p)
      {
        PTRTYPE **list=m_buf.Get();
        int x = GetSize();
        while (--x >= 0) if (list[x] == p) return x;
      }
      return -1;
    }

    PTRTYPE *Add(PTRTYPE *item) { return m_buf.Add(item) ? item : NULL; }
    void Add(PTRTYPE **items, int numitems) { m_buf.Add(items, numitems); }

    PTRTYPE *Set(int index, PTRTYPE *item)
    {
      PTRTYPE **list=m_buf.Get();
      if (list && index >= 0 && index < GetSize()) return list[index]=item;
      return NULL;
    }

    PTRTYPE *Insert(int index, PTRTYPE *item)
    {
      const int s=GetSize();
      m_buf.Insert(item, wdl_clamp(index,0,s));
      return item;

    }
    template<class SB, class SB2> int FindSorted(SB p, int (*compar)(SB2 a, const PTRTYPE *b)) const
    {
      bool m;
      int i = LowerBound(p,&m,compar);
      return m ? i : -1;
    }
    PTRTYPE *InsertSorted(PTRTYPE *item, int (*compar)(const PTRTYPE *a, const PTRTYPE *b))
    {
      bool m;
      return Insert(LowerBound(item,&m,compar),item);
    }
    template<class SB, class SB2> PTRTYPE *InsertSorted(PTRTYPE *item, SB ref, int (*compar)(SB2 a, const PTRTYPE *b))
    {
      bool m;
      return Insert(LowerBound(ref,&m,compar),item);
    }

    void Delete(int index) { m_buf.Delete(index);  }
    void Delete(int index, bool wantDelete, void (*delfunc)(void *)=NULL)
    {
      PTRTYPE **list=GetList();
      int size=GetSize();
      if (list && index >= 0 && index < size)
      {
        if (wantDelete)
        {
          if (delfunc) delfunc(Get(index));
          else delete Get(index);
        }
        // if delfunc modified the list this could be unpredictable
        WDL_ASSERT(size == GetSize());
        if (index < --size) memmove(list+index,list+index+1,(unsigned int)sizeof(PTRTYPE *)*(size-index));
        m_buf.Resize(size,false);
      }
    }
    template<class PT> void Delete(int index, PT delfunc)
    {
      PTRTYPE **list=GetList();
      int size=GetSize();
      if (list && index >= 0 && index < size)
      {
        if (delfunc) delfunc(Get(index));
        // if delfunc modified the list this could be unpredictable
        WDL_ASSERT(size == GetSize());

        if (index < --size) memmove(list+index,list+index+1,(unsigned int)sizeof(PTRTYPE *)*(size-index));
        m_buf.Resize(size,false);
      }
    }
    void DeletePtr(const PTRTYPE *p) { Delete(Find(p)); }
    void DeletePtr(const PTRTYPE *p, bool wantDelete, void (*delfunc)(void *)=NULL) { Delete(Find(p),wantDelete,delfunc); }
    template<class PT> void DeletePtr(const PTRTYPE *p, PT delfunc) { Delete(Find(p),delfunc); }

    void DeleteRange(int index, int count) { m_buf.DeleteRange(index,count); }

    void Empty()
    {
      m_buf.Resize(0,false);
    }
    void Empty(bool wantDelete, void (*delfunc)(void *)=NULL)
    {
      if (wantDelete)
      {
        int x;
        for (x = GetSize()-1; x >= 0; x --)
        {
          PTRTYPE* p = Get(x);
          if (p)
          {
            if (delfunc) delfunc(p);
            else delete p;
          }
          // if delfunc modified the list this could be unpredictable
          WDL_ASSERT(x == GetSize()-1);
          m_buf.Resize(x,false);
        }
      }
      m_buf.Resize(0,false);
    }
    template<class PT> void Empty(PT delfunc)
    {
      WDL_ASSERT(delfunc != NULL);
      for (int x = GetSize()-1; x >= 0; x --)
      {
        PTRTYPE* p = Get(x);
        if (delfunc && p) delfunc(p);
        // if delfunc modified the list this could be unpredictable
        WDL_ASSERT(x == GetSize()-1);
        m_buf.Resize(x,false);
      }
    }

    template<class SB, class SB2> int LowerBound(SB key, bool* ismatch, int (*compar)(SB2 a, const PTRTYPE *b)) const
    {
      int a = 0;
      int c = GetSize();
      PTRTYPE **list=GetList();
      while (a != c)
      {
        int b = (a+c)/2;
        int cmp = compar(key, list[b]);
        if (cmp > 0) a = b+1;
        else if (cmp < 0) c = b;
        else
        {
          *ismatch = true;
          return b;
        }
      }
      *ismatch = false;
      return a;
    }

    int DeleteBatch(bool (*proc)(PTRTYPE *p, void *ctx), void *ctx=NULL) // proc returns true to remove item. returns number removed
    {
      const int sz = GetSize();
      int cnt=0;
      PTRTYPE **rd = GetList(), **wr = rd;
      for (int x = 0; x < sz; x ++)
      {
        if (!proc(*rd,ctx))
        {
          if (rd != wr) *wr=*rd;
          wr++;
          cnt++;
        }
        rd++;
      }
      if (cnt < sz) m_buf.Resize(cnt,false);
      return sz - cnt;
    }

    const PTRTYPE **begin() const { return GetList(); }
    const PTRTYPE **end() const { return GetList() + GetSize(); }
    PTRTYPE **begin() { return GetList(); }
    PTRTYPE **end() { return GetList() + GetSize(); }

  private:
    WDL_TypedBuf<PTRTYPE *> m_buf;

};


template<class PTRTYPE> class WDL_PtrList_DeleteOnDestroy : public WDL_PtrList<PTRTYPE>
{
public:
  explicit WDL_PtrList_DeleteOnDestroy(void (*delfunc)(void *)=NULL, int defgran=4096) : WDL_PtrList<PTRTYPE>(defgran), m_delfunc(delfunc) {  }
  ~WDL_PtrList_DeleteOnDestroy()
  {
    WDL_PtrList<PTRTYPE>::Empty(true,m_delfunc);
  }
private:
  void (*m_delfunc)(void *);
};

#endif

