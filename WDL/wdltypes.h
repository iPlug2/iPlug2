#ifndef _WDLTYPES_
#define _WDLTYPES_

#ifdef _MSC_VER

typedef __int64 WDL_INT64;
typedef unsigned __int64 WDL_UINT64;

#else

typedef long long WDL_INT64;
typedef unsigned long long WDL_UINT64;

#endif

#ifdef _MSC_VER
  #define WDL_UINT64_CONST(x) (x##ui64)
  #define WDL_INT64_CONST(x) (x##i64)
#else
  #define WDL_UINT64_CONST(x) (x##ULL)
  #define WDL_INT64_CONST(x) (x##LL)
#endif

#ifdef _WIN32
  #define WDL_PRI_UINT64 "I64u"
  #define WDL_PRI_INT64 "I64d"
#else
  #define WDL_PRI_UINT64 "llu"
  #define WDL_PRI_INT64 "lld"
#endif

#if !defined(_MSC_VER) ||  _MSC_VER > 1200
#define WDL_DLGRET INT_PTR CALLBACK
#else
#define WDL_DLGRET BOOL CALLBACK
#endif


#ifdef _WIN32
#include <windows.h>
#include <stdio.h>
#else
#include <stdint.h>
typedef intptr_t INT_PTR;
typedef uintptr_t UINT_PTR;
#endif
#include <string.h>

#if defined(__ppc__) || !defined(__cplusplus)
typedef char WDL_bool;
#else
typedef bool WDL_bool;
#endif

#ifndef GWLP_USERDATA
#define GWLP_USERDATA GWL_USERDATA
#define GWLP_WNDPROC GWL_WNDPROC
#define GWLP_HINSTANCE GWL_HINSTANCE
#define GWLP_HWNDPARENT GWL_HWNDPARENT
#define DWLP_USER DWL_USER
#define DWLP_DLGPROC DWL_DLGPROC
#define DWLP_MSGRESULT DWL_MSGRESULT
#define SetWindowLongPtr(a,b,c) SetWindowLong(a,b,c)
#define GetWindowLongPtr(a,b) GetWindowLong(a,b)
#define SetWindowLongPtrW(a,b,c) SetWindowLongW(a,b,c)
#define GetWindowLongPtrW(a,b) GetWindowLongW(a,b)
#define SetWindowLongPtrA(a,b,c) SetWindowLongA(a,b,c)
#define GetWindowLongPtrA(a,b) GetWindowLongA(a,b)

#define GCLP_WNDPROC GCL_WNDPROC
#define GCLP_HICON GCL_HICON
#define GCLP_HICONSM GCL_HICONSM
#define SetClassLongPtr(a,b,c) SetClassLong(a,b,c)
#define GetClassLongPtr(a,b) GetClassLong(a,b)
#endif

#if !defined(WDL_BIG_ENDIAN) && !defined(WDL_LITTLE_ENDIAN)
  #ifdef __ppc__
    #define WDL_BIG_ENDIAN
  #else
    #define WDL_LITTLE_ENDIAN
  #endif
#endif

#if defined(WDL_BIG_ENDIAN) && defined(WDL_LITTLE_ENDIAN)
#error WDL_BIG_ENDIAN and WDL_LITTLE_ENDIAN both defined
#endif


#ifdef __GNUC__
// for structures that contain doubles, or doubles in structures that are after stuff of questionable alignment (for OSX/linux)
  #define WDL_FIXALIGN  __attribute__ ((aligned (8)))
// usage: void func(int a, const char *fmt, ...) WDL_VARARG_WARN(printf,2,3); // note: if member function, this pointer is counted as well, so as member function that would be 3,4
  #define WDL_VARARG_WARN(x,n,s) __attribute__ ((format (x,n,s)))
  #define WDL_STATICFUNC_UNUSED __attribute__((unused))

#else
  #define WDL_FIXALIGN 
  #define WDL_VARARG_WARN(x,n,s)
  #define WDL_STATICFUNC_UNUSED
#endif

#ifndef WDL_WANT_NEW_EXCEPTIONS
#if defined(__cplusplus)
#include <new>
#define WDL_NEW (std::nothrow)
#endif
#else
#define WDL_NEW
#endif


#if !defined(max) && defined(WDL_DEFINE_MINMAX)
#define max(x,y) ((x)<(y)?(y):(x))
#define min(x,y) ((x)<(y)?(x):(y))
#endif

#ifndef wdl_max
#define wdl_max(x,y) ((x)<(y)?(y):(x))
#define wdl_min(x,y) ((x)<(y)?(x):(y))
#define wdl_abs(x) ((x)<0 ? -(x) : (x))
#define wdl_clamp(x,minv,maxv) (WDL_NOT_NORMALLY((maxv) < (minv)) || (x) < (minv) ? (minv) : ((x) > (maxv) ? (maxv) : (x)))
#endif

#ifndef _WIN32
  #ifndef strnicmp 
    #define strnicmp(x,y,z) strncasecmp(x,y,z)
  #endif
  #ifndef stricmp 
    #define stricmp(x,y) strcasecmp(x,y)
  #endif
#endif

#ifdef WDL_BACKSLASHES_ARE_ORDINARY
#define WDL_IS_DIRCHAR(x) ((x) == '/')
#else
// for multi-platform applications it seems better to treat backslashes as directory separators even if it
// isn't supported by the underying system (for resolving filenames, etc)
  #ifdef _WIN32
    #define WDL_IS_DIRCHAR(x) ((x) == '\\' || (x) == '/')
  #else
    #define WDL_IS_DIRCHAR(x) ((x) == '/' || (x) == '\\')
  #endif
#endif

#if defined(_WIN32) && !defined(WDL_BACKSLASHES_ARE_ORDINARY)
#define WDL_DIRCHAR '\\'
#define WDL_DIRCHAR_STR "\\"
#else
#define WDL_DIRCHAR '/'
#define WDL_DIRCHAR_STR "/"
#endif

#if defined(_WIN32) || defined(__APPLE__)
  // on __APPLE__ we should ideally check the filesystem for case-sensitivity, assuming a case-insensitive-only match
  #define wdl_filename_cmp(x,y) stricmp(x,y)
  #define wdl_filename_cmpn(x,y,n) strnicmp(x,y,n)
#else
  #define wdl_filename_cmp(x,y) strcmp(x,y)
  #define wdl_filename_cmpn(x,y,n) strncmp(x,y,n)
#endif

#if defined(__GNUC__) || defined(__INTEL_COMPILER)
  #define WDL_likely(x) (__builtin_expect(!!(x),1))
  #define WDL_unlikely(x) (__builtin_expect(!!(x),0))
#else
  #define WDL_likely(x) (!!(x))
  #define WDL_unlikely(x) (!!(x))
#endif

#if defined(_DEBUG) || defined(DEBUG)
#include <assert.h>

  #ifdef _MSC_VER
    // msvc assert failure allows message loop to run, potentially resulting in recursive asserts
    static LONG WDL_ASSERT_INTERNALCNT;
    static int WDL_ASSERT_END() { WDL_ASSERT_INTERNALCNT=0; return 0; }
    static int WDL_ASSERT_BEGIN() { return InterlockedCompareExchange(&WDL_ASSERT_INTERNALCNT,1,0) == 0; }
    #define WDL_ASSERT(x) do { if (WDL_ASSERT_BEGIN()) { assert(x); WDL_ASSERT_END(); } } while(0)
  #else
    #define WDL_ASSERT_BEGIN() (1)
    #define WDL_ASSERT_END() (0)
    #define WDL_ASSERT(x) assert(x)
  #endif
  #define WDL_NORMALLY(x)     ((x) ? 1 : (WDL_ASSERT_BEGIN() && (assert(0/*ignorethis*/ && (x)),WDL_ASSERT_END())))
  #define WDL_NOT_NORMALLY(x) ((x) ? !WDL_ASSERT_BEGIN() || (assert(0/*ignorethis*/ && !(x)),!WDL_ASSERT_END()) : 0)
#else
  #define WDL_ASSERT(x)
  #define WDL_NORMALLY(x) WDL_likely(x)
  #define WDL_NOT_NORMALLY(x) WDL_unlikely(x)
#endif


typedef unsigned int WDL_TICKTYPE;

static WDL_bool WDL_STATICFUNC_UNUSED WDL_TICKS_IN_RANGE(WDL_TICKTYPE current,  WDL_TICKTYPE refstart, int len) // current >= refstart && current < refstart+len
{
  WDL_ASSERT(len > 0);
  return (current - refstart) < (WDL_TICKTYPE)len;
}

static WDL_bool WDL_STATICFUNC_UNUSED WDL_TICKS_IN_RANGE_ENDING_AT(WDL_TICKTYPE current,  WDL_TICKTYPE refend, int len) // current >= refend-len && current < refend
{
  const WDL_TICKTYPE refstart = refend - len;
  WDL_ASSERT(len > 0);
  return (current - refstart) < (WDL_TICKTYPE)len;
  //return ((refend-1) - current) < (WDL_TICKTYPE)len;
}

// use this if you want validate that nothing that includes wdltypes.h calls fopen() directly on win32
// #define WDL_CHECK_FOR_NON_UTF8_FOPEN

#if defined(WDL_CHECK_FOR_NON_UTF8_FOPEN) && !defined(_WDL_WIN32_UTF8_H_)
  #ifdef fopen
    #undef fopen
  #endif
  #include <stdio.h>
  static WDL_STATICFUNC_UNUSED FILE *WDL_fopenA(const char *fn, const char *mode) { return fopen(fn,mode); }
  #define fopen this_should_be_fopenUTF8_include_win32_utf8.h
#else
  // callers of WDL_fopenA don't mind being non-UTF8-compatible on win32
  // (this could map to either fopen() or fopenUTF8()
  #define WDL_fopenA(fn,mode) fopen(fn,mode)
#endif

#ifndef WDL_ALLOW_UNSIGNED_DEFAULT_CHAR
typedef char wdl_assert_failed_unsigned_char[((char)-1) > 0 ? -1 : 1];
#endif

// wdl_log() / printf() wrapper. no-op on release builds
#if !defined(_DEBUG) && !defined(WDL_LOG_ON_RELEASE)
  static void WDL_STATICFUNC_UNUSED WDL_VARARG_WARN(printf,1,2) wdl_log(const char *format, ...) { }
#elif defined(_WIN32)
  static void WDL_STATICFUNC_UNUSED WDL_VARARG_WARN(printf,1,2) wdl_log(const char *format, ...)
  {
    int rv;
    va_list va;

    char tmp[3800];
    va_start(va,format);
    tmp[0]=0;
    rv=_vsnprintf(tmp,sizeof(tmp),format,va); // returns -1  if over, and does not null terminate, ugh
    va_end(va);

    if (rv < 0 || rv>=(int)sizeof(tmp)-1) tmp[sizeof(tmp)-1]=0;
    OutputDebugStringA(tmp);
  }
#else
  #define wdl_log printf
#endif

static void WDL_STATICFUNC_UNUSED wdl_bswap_copy(void *bout, const void *bin, size_t nelem, size_t elemsz)
{
  char p[8], po[8];
  WDL_ASSERT(elemsz > 0);
  if (elemsz > 1 && WDL_NORMALLY(elemsz <= sizeof(p)))
  {
    size_t i,x;
    for (i = 0; i < nelem; i ++)
    {
      memcpy(p,bin,elemsz);
      for (x = 0; x < elemsz; x ++) po[x]=p[elemsz-1-x];
      memcpy(bout,po,elemsz);
      bin = (const char *)bin + elemsz;
      bout = (char *)bout + elemsz;
    }
  }
  else if (bout != bin)
    memmove(bout,bin,elemsz * nelem);
}

static void WDL_STATICFUNC_UNUSED wdl_memcpy_le(void *bout, const void *bin, size_t nelem, size_t elemsz)
{
  WDL_ASSERT(elemsz > 0 && elemsz <= 8);
#ifdef WDL_BIG_ENDIAN
  if (elemsz > 1) wdl_bswap_copy(bout,bin,nelem,elemsz);
  else
#endif
  if (bout != bin) memmove(bout,bin,elemsz * nelem);
}

static void WDL_STATICFUNC_UNUSED wdl_memcpy_be(void *bout, const void *bin, size_t nelem, size_t elemsz)
{
  WDL_ASSERT(elemsz > 0 && elemsz <= 8);
#ifdef WDL_LITTLE_ENDIAN
  if (elemsz > 1) wdl_bswap_copy(bout,bin,nelem,elemsz);
  else
#endif
  if (bout != bin) memmove(bout,bin,elemsz * nelem);
}

static void WDL_STATICFUNC_UNUSED wdl_mem_store_int(void *bout, int v)
{
  memcpy(bout,&v,sizeof(v));
}

static void WDL_STATICFUNC_UNUSED wdl_mem_store_int_le(void *bout, int v)
{
  wdl_memcpy_le(bout,&v,1,sizeof(v));
}

static void WDL_STATICFUNC_UNUSED wdl_mem_store_int_be(void *bout, int v)
{
  wdl_memcpy_be(bout,&v,1,sizeof(v));
}

static int WDL_STATICFUNC_UNUSED wdl_mem_load_int(const void *rd)
{
  int v;
  memcpy(&v,rd,sizeof(v));
  return v;
}

static int WDL_STATICFUNC_UNUSED wdl_mem_load_int_le(const void *rd)
{
  int v;
  wdl_memcpy_le(&v,rd,1,sizeof(v));
  return v;
}

static int WDL_STATICFUNC_UNUSED wdl_mem_load_int_be(const void *rd)
{
  int v;
  wdl_memcpy_be(&v,rd,1,sizeof(v));
  return v;
}


#endif
