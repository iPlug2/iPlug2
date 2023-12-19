#ifndef _WDL_ASSOCARRAY_H_
#define _WDL_ASSOCARRAY_H_

#include "heapbuf.h"
#include "mergesort.h"
#include "wdlcstring.h"

// on all of these, if valdispose is set, the array will dispose of values as needed.
// if keydup/keydispose are set, copies of (any) key data will be made/destroyed as necessary


// WDL_AssocArrayImpl can be used on its own, and can contain structs for keys or values
template <class KEY, class VAL> class WDL_AssocArrayImpl
{
  WDL_AssocArrayImpl(const WDL_AssocArrayImpl &cp) { CopyContents(cp); }

  WDL_AssocArrayImpl &operator=(const WDL_AssocArrayImpl &cp) { CopyContents(cp); return *this; }

public:

  explicit WDL_AssocArrayImpl(int (*keycmp)(KEY *k1, KEY *k2),
                              KEY (*keydup)(KEY)=NULL,
                              void (*keydispose)(KEY)=NULL,
                              void (*valdispose)(VAL)=NULL)
  {
    m_keycmp = keycmp;
    m_keydup = keydup;
    m_keydispose = keydispose;
    m_valdispose = valdispose;
  }

  ~WDL_AssocArrayImpl()
  {
    DeleteAll();
  }

  VAL* GetPtr(KEY key, KEY *keyPtrOut=NULL) const
  {
    bool ismatch = false;
    int i = LowerBound(key, &ismatch);
    if (ismatch)
    {
      KeyVal* kv = m_data.Get()+i;
      if (keyPtrOut) *keyPtrOut = kv->key;
      return &(kv->val);
    }
    return 0;
  }

  bool Exists(KEY key) const
  {
    bool ismatch = false;
    LowerBound(key, &ismatch);
    return ismatch;
  }

  int Insert(KEY key, VAL val)
  {
    bool ismatch = false;
    int i = LowerBound(key, &ismatch);
    if (ismatch)
    {
      KeyVal* kv = m_data.Get()+i;
      if (m_valdispose) m_valdispose(kv->val);
      kv->val = val;
    }
    else
    {
      KeyVal* kv = m_data.Resize(m_data.GetSize()+1)+i;
      memmove(kv+1, kv, (m_data.GetSize()-i-1)*(unsigned int)sizeof(KeyVal));
      if (m_keydup) key = m_keydup(key);
      kv->key = key;
      kv->val = val;      
    }
    return i;
  }

  void Delete(KEY key) 
  {
    bool ismatch = false;
    int i = LowerBound(key, &ismatch);
    if (ismatch)
    {
      KeyVal* kv = m_data.Get()+i;
      if (m_keydispose) m_keydispose(kv->key);
      if (m_valdispose) m_valdispose(kv->val);
      m_data.Delete(i);
    }
  }

  void DeleteByIndex(int idx)
  {
    if (idx >= 0 && idx < m_data.GetSize())
    {
      KeyVal* kv = m_data.Get()+idx;
      if (m_keydispose) m_keydispose(kv->key);
      if (m_valdispose) m_valdispose(kv->val);
      m_data.Delete(idx);
    }
  }

  void DeleteAll(bool resizedown=false)
  {
    if (m_keydispose || m_valdispose)
    {
      int i;
      for (i = 0; i < m_data.GetSize(); ++i)
      {
        KeyVal* kv = m_data.Get()+i;
        if (m_keydispose) m_keydispose(kv->key);
        if (m_valdispose) m_valdispose(kv->val);
      }
    }
    m_data.Resize(0, resizedown);
  }

  int GetSize() const
  {
    return m_data.GetSize();
  }

  VAL* EnumeratePtr(int i, KEY* key=NULL) const
  {
    if (i >= 0 && i < m_data.GetSize()) 
    {
      KeyVal* kv = m_data.Get()+i;
      if (key) *key = kv->key;
      return &(kv->val);
    }
    return 0;
  }
  
  KEY* ReverseLookupPtr(VAL val) const
  {
    int i;
    for (i = 0; i < m_data.GetSize(); ++i)
    {
      KeyVal* kv = m_data.Get()+i;
      if (kv->val == val) return &kv->key;
    }
    return 0;    
  }

  void ChangeKey(KEY oldkey, KEY newkey)
  {
    bool ismatch=false;
    int i=LowerBound(oldkey, &ismatch);
    if (ismatch) ChangeKeyByIndex(i, newkey, true);
  }

  void ChangeKeyByIndex(int idx, KEY newkey, bool needsort)
  {
    if (idx >= 0 && idx < m_data.GetSize())
    {
      KeyVal* kv=m_data.Get()+idx;
      if (!needsort)
      {
        if (m_keydispose) m_keydispose(kv->key);
        if (m_keydup) newkey=m_keydup(newkey);
        kv->key=newkey;
      }
      else
      {
        VAL val=kv->val;
        m_data.Delete(idx);
        Insert(newkey, val);
      }
    }
  }

  // fast add-block mode
  void AddUnsorted(KEY key, VAL val)
  {
    int i=m_data.GetSize();
    KeyVal* kv = m_data.Resize(i+1)+i;
    if (m_keydup) key = m_keydup(key);
    kv->key = key;
    kv->val = val;
  }

  void Resort(int (*new_keycmp)(KEY *k1, KEY *k2)=NULL)
  {
    if (new_keycmp) m_keycmp = new_keycmp;
    if (m_data.GetSize() > 1 && m_keycmp)
    {
      qsort(m_data.Get(), m_data.GetSize(), sizeof(KeyVal),
        (int(*)(const void*, const void*))m_keycmp);
      if (!new_keycmp)
        RemoveDuplicateKeys();
    }
  }

  void ResortStable()
  {
    if (m_data.GetSize() > 1 && m_keycmp)
    {
      char *tmp=(char*)malloc(m_data.GetSize()*sizeof(KeyVal));
      if (WDL_NORMALLY(tmp))
      {
        WDL_mergesort(m_data.Get(), m_data.GetSize(), sizeof(KeyVal),
          (int(*)(const void*, const void*))m_keycmp, tmp);
        free(tmp);
      }
      else
      {
        qsort(m_data.Get(), m_data.GetSize(), sizeof(KeyVal),
          (int(*)(const void*, const void*))m_keycmp);
      }

      RemoveDuplicateKeys();
    }
  }

  int LowerBound(KEY key, bool* ismatch) const
  {
    int a = 0;
    int c = m_data.GetSize();
    while (a != c)
    {
      int b = (a+c)/2;
      KeyVal* kv=m_data.Get()+b;
      int cmp = m_keycmp(&key, &kv->key);
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

  int GetIdx(KEY key) const
  {
    bool ismatch=false;
    int i = LowerBound(key, &ismatch);
    if (ismatch) return i;
    return -1;
  }

  void SetGranul(int gran)
  {
    m_data.SetGranul(gran);
  }

  void CopyContents(const WDL_AssocArrayImpl &cp)
  {
    m_data=cp.m_data;
    m_keycmp = cp.m_keycmp;
    m_keydup = cp.m_keydup; 
    m_keydispose = m_keydup ? cp.m_keydispose : NULL;
    m_valdispose = NULL; // avoid disposing of values twice, since we don't have a valdup, we can't have a fully valid copy
    if (m_keydup)
    {
      const int n=m_data.GetSize();
      for (int x=0;x<n;x++)
      {
        KeyVal *kv=m_data.Get()+x;
        kv->key = m_keydup(kv->key);
      }
    }
  }

  void CopyContentsAsReference(const WDL_AssocArrayImpl &cp)
  {
    DeleteAll(true);
    m_keycmp = cp.m_keycmp;
    m_keydup = NULL;  // this no longer can own any data
    m_keydispose = NULL;
    m_valdispose = NULL; 

    m_data=cp.m_data;
  }


// private data, but exposed in case the caller wants to manipulate at its own risk
  struct KeyVal
  {
    KEY key;
    VAL val;
  };
  WDL_TypedBuf<KeyVal> m_data;

  static int keycmp_ptr(KEY *a, KEY *b) { return (INT_PTR)*a > (INT_PTR)*b ? 1 : (INT_PTR)*a < (INT_PTR)*b ? -1 : 0; }

protected:

  int (*m_keycmp)(KEY *k1, KEY *k2);
  KEY (*m_keydup)(KEY);
  void (*m_keydispose)(KEY);
  void (*m_valdispose)(VAL);

private:

  void RemoveDuplicateKeys() // after resorting
  {
    const int sz = m_data.GetSize();

    int cnt = 1;
    KeyVal *rd = m_data.Get() + 1, *wr = rd;
    for (int x = 1; x < sz; x ++)
    {
      if (m_keycmp(&rd->key, &wr[-1].key))
      {
        if (rd != wr) *wr=*rd;
        wr++;
        cnt++;
      }
      else
      {
        if (m_keydispose) m_keydispose(rd->key);
        if (m_valdispose) m_valdispose(rd->val);
      }
      rd++;
    }
    if (cnt < sz) m_data.Resize(cnt,false);
  }
};


// WDL_AssocArray adds useful functions but cannot contain structs for keys or values
template <class KEY, class VAL> class WDL_AssocArray : public WDL_AssocArrayImpl<KEY, VAL>
{
public:

  explicit WDL_AssocArray(int (*keycmp)(KEY *k1, KEY *k2),
                          KEY (*keydup)(KEY)=NULL,
                          void (*keydispose)(KEY)=NULL, void (*valdispose)(VAL)=NULL)
    : WDL_AssocArrayImpl<KEY, VAL>(keycmp, keydup, keydispose, valdispose)
  { 
  }

  VAL Get(KEY key, VAL notfound=0) const
  {
    VAL* p = this->GetPtr(key);
    if (p) return *p;
    return notfound;
  }

  VAL Enumerate(int i, KEY* key=NULL, VAL notfound=0) const
  {
    VAL* p = this->EnumeratePtr(i, key);
    if (p) return *p;
    return notfound; 
  }

  KEY ReverseLookup(VAL val, KEY notfound=0) const
  {
    KEY* p=this->ReverseLookupPtr(val);
    if (p) return *p;
    return notfound;
  }
};


template <class VAL> class WDL_IntKeyedArray : public WDL_AssocArray<int, VAL>
{
public:

  explicit WDL_IntKeyedArray(void (*valdispose)(VAL)=NULL)
    : WDL_AssocArray<int, VAL>(cmpint, NULL, NULL, valdispose) {}

  static int cmpint(int *a, int *b) { return *a > *b ? 1 : *a < *b ? -1 : 0; }
};

template <class VAL> class WDL_IntKeyedArray2 : public WDL_AssocArrayImpl<int, VAL>
{
public:

  explicit WDL_IntKeyedArray2(void (*valdispose)(VAL)=NULL)
    : WDL_AssocArrayImpl<int, VAL>(cmpint, NULL, NULL, valdispose) {}

  static int cmpint(int *a, int *b) { return *a > *b ? 1 : *a < *b ? -1 : 0; }
};

template <class VAL> class WDL_StringKeyedArray : public WDL_AssocArray<const char *, VAL>
{
public:

  explicit WDL_StringKeyedArray(bool caseSensitive=true, void (*valdispose)(VAL)=NULL)
    : WDL_AssocArray<const char*, VAL>(caseSensitive?cmpstr:cmpistr, dupstr, freestr, valdispose) {}

  static const char *dupstr(const char *s) { return strdup(s);  } // these might not be necessary but depending on the libc maybe...
  static int cmpstr(const char **s1, const char **s2) { return strcmp(*s1, *s2); }
  static int cmpistr(const char **a, const char **b) { return stricmp(*a,*b); }
  static void freestr(const char* s) { free((void*)s); }
  static void freecharptr(char *p) { free(p); }
};


template <class VAL> class WDL_StringKeyedArray2 : public WDL_AssocArrayImpl<const char *, VAL>
{
public:

  explicit WDL_StringKeyedArray2(bool caseSensitive=true, void (*valdispose)(VAL)=NULL)
    : WDL_AssocArrayImpl<const char*, VAL>(caseSensitive?cmpstr:cmpistr, dupstr, freestr, valdispose) {}
  
  ~WDL_StringKeyedArray2() { }

  static const char *dupstr(const char *s) { return strdup(s);  } // these might not be necessary but depending on the libc maybe...
  static int cmpstr(const char **s1, const char **s2) { return strcmp(*s1, *s2); }
  static int cmpistr(const char **a, const char **b) { return stricmp(*a,*b); }
  static void freestr(const char* s) { free((void*)s); }
  static void freecharptr(char *p) { free(p); }
};

// sorts text as text, sorts anything that looks like a number as a number
template <class VAL> class WDL_LogicalSortStringKeyedArray : public WDL_StringKeyedArray<VAL>
{
public:

  explicit WDL_LogicalSortStringKeyedArray(bool caseSensitive=true, void (*valdispose)(VAL)=NULL)
    : WDL_StringKeyedArray<VAL>(caseSensitive, valdispose)
  {
    WDL_StringKeyedArray<VAL>::m_keycmp = caseSensitive?cmpstr:cmpistr; // override
  }
  
  ~WDL_LogicalSortStringKeyedArray() { }

  static int cmpstr(const char **a, const char **b)
  {
    int r=WDL_strcmp_logical_ex(*a, *b, 1, WDL_STRCMP_LOGICAL_EX_FLAG_UTF8CONVERT);
    return r?r:strcmp(*a,*b);
  }
  static int cmpistr(const char **a, const char **b)
  {
    int r=WDL_strcmp_logical_ex(*a, *b, 0, WDL_STRCMP_LOGICAL_EX_FLAG_UTF8CONVERT);
    return r?r:stricmp(*a,*b);
  }
};


template <class VAL> class WDL_PtrKeyedArray : public WDL_AssocArray<INT_PTR, VAL>
{
public:
  explicit WDL_PtrKeyedArray(void (*valdispose)(VAL)=NULL)
    : WDL_AssocArray<INT_PTR, VAL>(WDL_AssocArray<INT_PTR, VAL>::keycmp_ptr, NULL, NULL, valdispose) {}
};

template <class KEY, class VAL> class WDL_PointerKeyedArray : public WDL_AssocArray<KEY, VAL>
{
public:
  explicit WDL_PointerKeyedArray(void (*valdispose)(VAL)=NULL)
    : WDL_AssocArray<KEY, VAL>(WDL_AssocArray<KEY, VAL>::keycmp_ptr, NULL, NULL, valdispose) {}
};

struct WDL_Set_DummyRec { };
template <class KEY> class WDL_Set : public WDL_AssocArrayImpl<KEY,WDL_Set_DummyRec>
{
  public:
  explicit WDL_Set(int (*keycmp)(KEY *k1, KEY *k2),
                            KEY (*keydup)(KEY)=NULL,
                            void (*keydispose)(KEY)=NULL
      )
    : WDL_AssocArrayImpl<KEY, WDL_Set_DummyRec>(keycmp,keydup,keydispose)
  {
  }

  int Insert(KEY key)
  {
    WDL_Set_DummyRec r;
    return WDL_AssocArrayImpl<KEY, WDL_Set_DummyRec>::Insert(key,r);
  }
  void AddUnsorted(KEY key)
  {
    WDL_Set_DummyRec r;
    WDL_AssocArrayImpl<KEY, WDL_Set_DummyRec>::AddUnsorted(key,r);
  }

  bool Get(KEY key) const
  {
    return WDL_AssocArrayImpl<KEY, WDL_Set_DummyRec>::Exists(key);
  }
  bool Enumerate(int i, KEY *key=NULL)
  {
    return WDL_AssocArrayImpl<KEY, WDL_Set_DummyRec>::EnumeratePtr(i,key) != NULL;
  }
};

template <class KEY> class WDL_PtrSet : public WDL_Set<KEY>
{
public:
  explicit WDL_PtrSet() : WDL_Set<KEY>( WDL_AssocArray<KEY, WDL_Set_DummyRec>::keycmp_ptr ) { }
};



#endif

