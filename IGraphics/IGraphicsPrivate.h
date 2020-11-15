/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file Private structs and small classes, internal use only
 * @{
 */

#include <codecvt>
#include <string>
#include <memory>

#include "mutex.h"
#include "wdlstring.h"
#include "wdlendian.h"
#include "ptrlist.h"
#include "heapbuf.h"

#ifdef IGRAPHICS_SKIA
  #pragma warning( push )
  #pragma warning( disable : 4244 )
  #pragma warning( disable : 5030 )
  #include "experimental/svg/model/SkSVGDOM.h"
  #include "include/core/SkCanvas.h"
  #include "include/core/SkStream.h"
  #include "src/xml/SkDOM.h"
  #pragma warning( pop )
#else
  #include "nanosvg.h"
#endif

#include "IPlugPlatform.h"

#if defined IGRAPHICS_NANOVG
  #define BITMAP_DATA_TYPE int;
#elif defined IGRAPHICS_SKIA
  #pragma warning( push )
  #pragma warning( disable : 4244 )
  #include "SkImage.h"
  #include "SkSurface.h"
  #pragma warning( pop )
  struct SkiaDrawable
  {
    bool mIsSurface;
    sk_sp<SkImage> mImage;
    sk_sp<SkSurface> mSurface;
  };
  #define BITMAP_DATA_TYPE SkiaDrawable*
#elif defined IGRAPHICS_CANVAS
  #include <emscripten.h>
  #include <emscripten/val.h>
  #define BITMAP_DATA_TYPE emscripten::val*
#else // NO_IGRAPHICS
  #define BITMAP_DATA_TYPE void*;
#endif

#if defined OS_MAC || defined OS_IOS
  #include <CoreText/CoreText.h>
  #define FONT_DESCRIPTOR_TYPE CTFontDescriptorRef
#elif defined OS_WIN
  #include "wingdi.h"
  #define FONT_DESCRIPTOR_TYPE HFONT
#elif defined OS_WEB
  #define FONT_DESCRIPTOR_TYPE std::pair<WDL_String, WDL_String>*
#else 
  // NO_IGRAPHICS
#endif

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE
using BitmapData = BITMAP_DATA_TYPE;
using FontDescriptor = FONT_DESCRIPTOR_TYPE;
using RawBitmapData = WDL_TypedBuf<uint8_t>;

/** A bitmap abstraction around the different drawing back end bitmap representations.
 * In most cases it does own the bitmap data, the exception being with NanoVG, where the image is loaded onto the GPU as a texture,
 * but still needs to be freed. Most of the time  end-users will deal with IBitmap rather than APIBitmap, which is used behind the scenes. */
class APIBitmap
{
public:
  
  /** APIBitmap constructor
  * @param pBitmap pointer or integer index (NanoVG) to the image data
  * @param w The width of the bitmap
  * @param h The height of the bitmap
  * @param scale An integer representing the scale of this bitmap in relation to a 1:1 pixel screen, e.g. 2 for an @2x bitmap
  * @param drawScale The draw scale at which this API bitmap was created (used in the context of layers) */
  APIBitmap(BitmapData pBitmap, int w, int h, int scale, float drawScale)
  : mBitmap(pBitmap)
  , mWidth(w)
  , mHeight(h)
  , mScale(scale)
  , mDrawScale(drawScale)
  {}

  APIBitmap()
  : mBitmap(0)
  , mWidth(0)
  , mHeight(0)
  , mScale(0)
  , mDrawScale(1.f)
  {}

  virtual ~APIBitmap() {}

  APIBitmap(const APIBitmap&) = delete;
  APIBitmap& operator=(const APIBitmap&) = delete;
    
  /** Used to initialise the members after construction
   * @param pBitmap pointer or integer index (NanoVG) to the image data
   * @param w The width of the bitmap
   * @param h The height of the bitmap
   * @param scale An integer representing the scale of this bitmap in relation to a 1:1 pixel screen, e.g. 2 for an @2x bitmap
   * @param drawScale The draw scale at which this API bitmap was created (used in the context of layers) */
  void SetBitmap(BitmapData pBitmap, int w, int h, int scale, float drawScale)
  {
    mBitmap = pBitmap;
    mWidth = w;
    mHeight = h;
    mScale = scale;
    mDrawScale = drawScale;
  }

  /** @return BitmapData \todo */
  BitmapData GetBitmap() const { return mBitmap; }

  /** \todo */
  int GetWidth() const { return mWidth; }

  /** \todo */
  int GetHeight() const { return mHeight; }

  /** \todo */
  int GetScale() const { return mScale; }
  
  /** \todo */
  float GetDrawScale() const { return mDrawScale; }

private:
  BitmapData mBitmap; // for most drawing APIs BitmapData is a pointer. For Nanovg it is an integer index
  int mWidth;
  int mHeight;
  int mScale;
  float mDrawScale;
};

/** Used to retrieve font info directly from a raw memory buffer. */
class IFontInfo
{
public:
  IFontInfo(const void* data, uint32_t dataSize, uint32_t faceIdx)
  : mData(reinterpret_cast<const unsigned char*>(data))
  {
    if (mData)
      FindFace(faceIdx);
    
    if (mData)
    {
      mHeadLocation = LocateTable("head");
      mNameLocation = LocateTable("name");
      mHheaLocation = LocateTable("hhea");
      mFDscLocation = LocateTable("fdsc");
      
      if (IsValid())
      {
        mUnitsPerEM = GetUInt16(mHeadLocation + 18);
        mMacStyle = GetUInt16(mHeadLocation + 44);
        mFamily = SearchFontString(1);
        mStyle = SearchFontString(2);
        mAscender = GetSInt16(mHheaLocation + 4);
        mDescender = GetSInt16(mHheaLocation + 6);
        mLineGap = GetSInt16(mHheaLocation + 8);
      }
    }
  }
  
  bool IsValid() const      { return mData && mHeadLocation && mNameLocation && mHheaLocation; }
  
  const WDL_String& GetFamily() const   { return mFamily; }
  const WDL_String& GetStyle() const    { return mStyle; }
  
  bool IsBold() const       { return mMacStyle & (1 << 0); }
  bool IsItalic() const     { return mMacStyle & (1 << 1); }
  bool IsUnderline() const  { return mMacStyle & (1 << 2); }
  bool IsOutline() const    { return mMacStyle & (1 << 3); }
  bool IsShadow() const     { return mMacStyle & (1 << 4); }
  bool IsCondensed() const  { return mMacStyle & (1 << 5); }
  bool IsExpanded() const   { return mMacStyle & (1 << 6); }
  
  double GetHeightEMRatio() const { return mUnitsPerEM / static_cast<double>(mAscender - mDescender); }

  uint16_t GetUnitsPerEM() const { return mUnitsPerEM; }
  int16_t GetAscender() const    { return mAscender; }
  int16_t GetDescender() const   { return mDescender; }
  int16_t GetLineGap() const     { return mLineGap; }
  int16_t GetLineHeight() const  { return (mAscender - mDescender) + mLineGap; }
  
private:
    
  enum class EStringID { Mac, Windows };
    
  bool MatchTag(uint32_t loc, const char* tag)
  {
    return mData[loc+0] == tag[0] && mData[loc+1] == tag[1] && mData[loc+2] == tag[2] && mData[loc+3] == tag[3];
  }
  
  uint32_t LocateTable(const char *tag)
  {
    uint16_t numTables = GetUInt16(4);
    
    for (uint16_t i = 0; i < numTables; ++i)
    {
      uint32_t tableLocation = 12 + (16 * i);
      if (MatchTag(tableLocation, tag))
        return GetUInt32(tableLocation + 8);
    }
    
    return 0;
  }
  
  WDL_String SearchFontString(int nameID)
  {
    WDL_String str = GetFontString(nameID, EStringID::Windows);
    
    if (str.GetLength())
      return str;
    
    return GetFontString(nameID, EStringID::Mac);
  }
    
  WDL_String GetFontString(int nameID, EStringID stringID)
  {
    // Default to windows values
      
    int platformID = 3;
    int encodingID = 1;
    int languageID = 0x409;
      
    switch (stringID)
    {
      case EStringID::Mac:
        platformID = 1;
        encodingID = 0;
        languageID = 0;
        break;
            
      case EStringID::Windows:
        break;
    }
    
    for (uint16_t i = 0; i < GetUInt16(mNameLocation + 2); ++i)
    {
      uint32_t loc = mNameLocation + 6 + (12 * i);
      
      if (platformID == GetUInt16(loc + 0) && encodingID == GetUInt16(loc + 2)
          && languageID == GetUInt16(loc + 4) && nameID == GetUInt16(loc + 6))
      {
        uint32_t stringLocation = GetUInt16(mNameLocation + 4) + GetUInt16(loc + 10);
        uint16_t length = GetUInt16(loc + 8);
        
        switch (stringID)
        {
          case EStringID::Windows:
          {
            WDL_TypedBuf<char> utf8;
            WDL_TypedBuf<char16_t> utf16;
            utf8.Resize((length * 3) / 2);
            utf16.Resize(length / sizeof(char16_t));
            
            for (int j = 0; j < length; j++)
              utf16.Get()[j] = GetUInt16(mNameLocation + stringLocation + j * 2);
            
            std::codecvt_utf8_utf16<char16_t> conv;
            const char16_t *a;
            char *b;
            mbstate_t mbs;
            memset(&mbs, 0, sizeof(mbs));
            conv.out(mbs, utf16.Get(), utf16.Get() + utf16.GetSize(), a, utf8.Get(), utf8.Get() + utf8.GetSize(), b);
            
            return WDL_String(utf8.Get(), (int) (b - utf8.Get()));
          }
            
          case EStringID::Mac:
             return WDL_String((const char*)(mData + mNameLocation + stringLocation), length);
        }
      }
    }
    
    return WDL_String();
  }
  
  void FindFace(uint32_t faceIdx)
  {
    bool singleFont = IsSingleFont();
    
    if (singleFont && faceIdx == 0 )
      return;
    
    // Check if it's a TTC file
    if (!singleFont && MatchTag(0, "ttcf"))
    {
      // Check version
      if (GetUInt32(4) == 0x00010000 || GetUInt32(4) == 0x00020000)
      {
        if (faceIdx < GetSInt32(8))
        {
          mData += GetUInt32(12 + faceIdx * 4);
          return;
        }
      }
    }
    mData = nullptr;
  }
  
  bool IsSingleFont()
  {
    char TTV1[4] = { '1', 0, 0, 0 };
    char OTV1[4] = { 0, 1, 0, 0 };
    
    // Check the version number
    if (MatchTag(0, TTV1)) return true;   // TrueType 1
    if (MatchTag(0, "typ1")) return true; // TrueType with type 1 font -- we don't support this!
    if (MatchTag(0, "OTTO")) return true; // OpenType with CFF
    if (MatchTag(0, OTV1))  return true;  // OpenType 1.0
    
    return false;
  }
  
#if defined WDL_LITTLE_ENDIAN
  uint16_t GetUInt16(uint32_t loc) { return (((uint16_t)mData[loc + 0]) << 8) | (uint16_t)mData[loc + 1]; }
  int16_t GetSInt16(uint32_t loc) { return (((uint16_t)mData[loc + 0]) << 8) | (uint16_t)mData[loc + 1]; }
  uint32_t GetUInt32(uint32_t loc) { return (((uint32_t)GetUInt16(loc + 0)) << 16) | (uint32_t)GetUInt16(loc + 2); }
  int32_t GetSInt32(uint32_t loc) { return (((uint32_t)GetUInt16(loc + 0)) << 16) | (uint32_t)GetUInt16(loc + 2); }
#else
  uint16_t GetUInt16(uint32_t loc) { return (((uint16_t)mData[loc + 1]) << 8) | (uint16_t)mData[loc + 0]; }
  int16_t GetSInt16(uint32_t loc) { return (((uint16_t)mData[loc + 1]) << 8) | (uint16_t)mData[loc + 0]; }
  uint32_t GetUInt32(uint32_t loc) { return (((uint32_t)GetUInt16(loc + 2)) << 16) | (uint32_t)GetUInt16(loc + 0); }
  int32_t GetSInt32(uint32_t loc) { return (((uint32_t)GetUInt16(loc + 2)) << 16) | (uint32_t)GetUInt16(loc + 0); }
#endif
  
private:
  const unsigned char* mData;
  
  uint32_t mHeadLocation = 0;
  uint32_t mNameLocation = 0;
  uint32_t mHheaLocation = 0;
  uint32_t mFDscLocation = 0;
  
  // Font Identifiers
  WDL_String mFamily;
  WDL_String mStyle;
  uint16_t mMacStyle = 0;
  
  // Metrics
  uint16_t mUnitsPerEM = 0;
  int16_t mAscender = 0;
  int16_t mDescender = 0;
  int16_t mLineGap = 0;
};

/** Used to manage raw font data. */
class IFontData : public IFontInfo, private WDL_TypedBuf<unsigned char>
{
public:
  IFontData() : IFontInfo(nullptr, 0, -1), mFaceIdx(-1) {}
  
  IFontData(const void* data, int size, int faceIdx) : IFontInfo(data, size, faceIdx), mFaceIdx(faceIdx)
  {
    const unsigned char* src = reinterpret_cast<const unsigned char*>(data);
    unsigned char* dest = ResizeOK(size);
    
    if (dest)
      std::copy(src, src + size, dest);
  }
  
  IFontData(int size) : IFontInfo(nullptr, 0, -1), mFaceIdx(-1)
  {
    Resize(size);
  }
  
  void SetFaceIdx(int faceIdx)
  {
    mFaceIdx = faceIdx;
    static_cast<IFontData&>(*this) = IFontData(Get(), GetSize(), mFaceIdx);
  }
  
  bool IsValid() const { return GetSize() && mFaceIdx >= 0 && IFontInfo::IsValid(); }
  
  unsigned char* Get() { return WDL_TypedBuf<unsigned char>::Get(); }
  int GetSize() const { return WDL_TypedBuf<unsigned char>::GetSize(); }
  int GetFaceIdx() const { return mFaceIdx; }
  
private:
  int mFaceIdx;
};

/** IFontDataPtr is a managed pointer for transferring the ownership of font data */
using IFontDataPtr = std::unique_ptr<IFontData>;

/** \todo */
class PlatformFont
{
public:
  PlatformFont(bool system) : mSystem(system) {}
  virtual ~PlatformFont() {}
    
  PlatformFont(const PlatformFont&) = delete;
  PlatformFont& operator=(const PlatformFont&) = delete;

  virtual FontDescriptor GetDescriptor() { return nullptr; }
  virtual IFontDataPtr GetFontData() { return IFontDataPtr(new IFontData()); }
  bool IsSystem() { return mSystem; }
    
protected:
  int GetFaceIdx(const void* data, int dataSize, const char* styleName)
  {
    for (int idx = 0; ; idx++)
    {
      IFontInfo fontInfo(data, dataSize, idx);
      
      if (!fontInfo.IsValid())
        return -1;
      
      const WDL_String& style = fontInfo.GetStyle();
      
      if (style.GetLength() && (!styleName[0] || !strcmp(style.Get(), styleName)))
        return idx;
    }
  }

  bool mSystem;
};

using PlatformFontPtr = std::unique_ptr<PlatformFont>;

#ifdef IGRAPHICS_SKIA
struct SVGHolder
{
  SVGHolder(sk_sp<SkSVGDOM> svgDom)
  : mSVGDom(svgDom)
  {
  }
  
  ~SVGHolder()
  {
    mSVGDom = nullptr;
  }
  
  SVGHolder(const SVGHolder&) = delete;
  SVGHolder& operator=(const SVGHolder&) = delete;
  
  sk_sp<SkSVGDOM> mSVGDom;
};
#else
/** Used internally to manage SVG data*/
struct SVGHolder
{
  SVGHolder(NSVGimage* pImage)
  : mImage(pImage)
  {
  }
  
  ~SVGHolder()
  {
    if(mImage)
      nsvgDelete(mImage);
    
    mImage = nullptr;
  }
  
  SVGHolder(const SVGHolder&) = delete;
  SVGHolder& operator=(const SVGHolder&) = delete;
  
  NSVGimage* mImage = nullptr;
};
#endif

/** Used internally to store data statically, making sure memory is not wasted when there are multiple plug-in instances loaded */
template <class T>
class StaticStorage
{
public:
  /** Accessor class that mantains thread safety when using static storage via RAII */
  class Accessor : private WDL_MutexLock
  {
  public:
    Accessor(StaticStorage& storage) 
    : WDL_MutexLock(&storage.mMutex)
    , mStorage(storage) 
    {}
    
    T* Find(const char* str, double scale = 1.)               { return mStorage.Find(str, scale); }
    void Add(T* pData, const char* str, double scale = 1.)    { return mStorage.Add(pData, str, scale); }
    void Remove(T* pData)                                     { return mStorage.Remove(pData); }
    void Clear()                                              { return mStorage.Clear(); }
    void Retain()                                             { return mStorage.Retain(); }
    void Release()                                            { return mStorage.Release(); }
      
  private:
    StaticStorage& mStorage;
  };
  
  StaticStorage() {}
    
  ~StaticStorage()
  {
    Clear();
  }

  StaticStorage(const StaticStorage&) = delete;
  StaticStorage& operator=(const StaticStorage&) = delete;
    
private:
  /** \todo */
  struct DataKey
  {
    // N.B. - hashID is not guaranteed to be unique
    size_t hashID;
    WDL_String name;
    double scale;
    std::unique_ptr<T> data;
  };
  
  /** \todo 
   * @param str \todo
   * @return size_t \todo */
  size_t Hash(const char* str)
  {
    std::string string(str);
    return std::hash<std::string>()(string);
  }

  /** \todo 
   * @param str \todo
   * @param scale \todo
   * @return T* \todo */
  T* Find(const char* str, double scale = 1.)
  {
    WDL_String cacheName(str);
    cacheName.AppendFormatted((int) strlen(str) + 6, "-%.1fx", scale);
    
    size_t hashID = Hash(cacheName.Get());
    
    int i, n = mDatas.GetSize();
    for (i = 0; i < n; ++i)
    {
      DataKey* pKey = mDatas.Get(i);

      // Use the hash id for a quick search and then confirm with the scale and identifier to ensure uniqueness
      if (pKey->hashID == hashID && scale == pKey->scale && !strcmp(str, pKey->name.Get()))
        return pKey->data.get();
    }
    return nullptr;
  }

  /** \todo 
   * @param pData \todo
   * @param str \todo
   * @param scale \todo scale where 2x = retina, omit if not needed */
  void Add(T* pData, const char* str, double scale = 1.)
  {
    DataKey* pKey = mDatas.Add(new DataKey);

    WDL_String cacheName(str);
    cacheName.AppendFormatted((int) strlen(str) + 6, "-%.1fx", scale);
    
    pKey->hashID = Hash(cacheName.Get());
    pKey->data = std::unique_ptr<T>(pData);
    pKey->scale = scale;
    pKey->name.Set(str);

    //DBGMSG("adding %s to the static storage at %.1fx the original scale\n", str, scale);
  }

  /** \todo @param pData \todo */
  void Remove(T* pData)
  {
    for (int i = 0; i < mDatas.GetSize(); ++i)
    {
      if (mDatas.Get(i)->data.get() == pData)
      {
        mDatas.Delete(i, true);
        break;
      }
    }
  }

  /** \todo  */
  void Clear()
  {
    mDatas.Empty(true);
  };

  /** \todo  */
  void Retain()
  {
    mCount++;
  }
  
  /** \todo  */
  void Release()
  {
    if (--mCount == 0)
      Clear();
  }
    
  int mCount = 0;
  WDL_Mutex mMutex;
  WDL_PtrList<DataKey> mDatas;
};

/** Encapsulate an xy point in one struct */
struct IVec2
{
  float x, y;
  IVec2() = default;
  IVec2(float x, float y) : x(x), y(y) {}
  
  IVec2 operator-(const IVec2 b) { return IVec2{x-b.x, y-b.y}; }
  IVec2 operator+(const IVec2 b) { return IVec2{x+b.x, y+b.y}; }
};


END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

/**@}*/
