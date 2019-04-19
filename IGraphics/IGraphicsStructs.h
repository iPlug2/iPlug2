/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file Structs and small classes used throughout IGraphics code
 * \addtogroup IGraphicsStructs
 * @{
 */


#include <cmath>
#include <cassert>
#include <cstdint>
#include <functional>
#include <algorithm>
#include <numeric>
#include <chrono>
#include <string>

#include "mutex.h"
#include "wdlstring.h"
#include "wdlendian.h"
#include "ptrlist.h"
#include "heapbuf.h"

#include "nanosvg.h"

#include "IPlugPlatform.h"
#include "IPlugUtilities.h"
#include "IPlugLogger.h"
#include "IGraphicsConstants.h"

class IGraphics;
class IControl;
class ILambdaControl;
struct IRECT;
struct IMouseInfo;
template <typename T = double>
inline T DegToRad(T degrees);

typedef std::function<void(IControl*)> IActionFunction;
typedef std::function<void(IControl*)> IAnimationFunction;
typedef std::function<void(ILambdaControl*, IGraphics&, IRECT&)> ILambdaDrawFunction;

void DefaultClickActionFunc(IControl* pCaller);
void DefaultAnimationFunc(IControl* pCaller);
void SplashClickActionFunc(IControl* pCaller);
void SplashAnimationFunc(IControl* pCaller);

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;
typedef std::chrono::duration<double, std::chrono::milliseconds::period> Milliseconds;

typedef WDL_TypedBuf<uint8_t> RawBitmapData;

#ifdef IGRAPHICS_AGG
  #include "IGraphicsAGG_src.h"
  typedef agg::pixel_map* BitmapData;
#elif defined IGRAPHICS_CAIRO
  #if defined OS_MAC || defined OS_LINUX
    #include "cairo/cairo.h"
  #elif defined OS_WIN
    #include "cairo/src/cairo.h"
  #else
    #error NOT IMPLEMENTED
  #endif
  typedef cairo_surface_t* BitmapData;
#elif defined IGRAPHICS_NANOVG
  typedef int BitmapData;
#elif defined IGRAPHICS_LICE
  #include "lice.h"
  typedef LICE_IBitmap* BitmapData;
#elif defined IGRAPHICS_CANVAS
  #include <emscripten.h>
  #include <emscripten/val.h>
  typedef emscripten::val* BitmapData;
#else // NO_IGRAPHICS
  typedef void* BitmapData;
#endif

#if defined OS_MAC
#include <CoreText/CoreText.h>
typedef CTFontDescriptorRef FontDescriptor;
#elif defined OS_WIN
#include "wingdi.h"
#include "Stringapiset.h"
typedef HFONT FontDescriptor;
#elif defined OS_WEB
typedef std::pair<WDL_String, WDL_String>* FontDescriptor;
#else // NO_IGRAPHICS
typedef void* BitmapData;
#endif

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

  /** @return BitmapData /todo */
  BitmapData GetBitmap() const { return mBitmap; }

  /** /todo */
  int GetWidth() const { return mWidth; }

  /** /todo */
  int GetHeight() const { return mHeight; }

  /** /todo */
  int GetScale() const { return mScale; }
  
  /** /todo */
  float GetDrawScale() const { return mDrawScale; }

private:
  BitmapData mBitmap; // for most drawing APIs BitmapData is a pointer. For Nanovg it is an integer index
  int mWidth;
  int mHeight;
  int mScale;
  float mDrawScale;
};

/** User-facing bitmap abstraction that you use to manage bitmap data, independant of draw class/platform.
 * IBitmap doesn't actually own the image data \see APIBitmap
 * An IBitmap's width and height are always in relation to a 1:1 (low dpi) screen. Any scaling happens at the drawing stage. */
class IBitmap
{
public:
  /** IBitmap Constructor
   @param pAPIBitmap Pointer to a drawing API bitmap
   @param n Number of frames (for multi frame film-strip bitmaps)
   @param framesAreHorizontal framesAreHorizontal \c true if the frames are positioned horizontally
   @param name Resource name for the bitmap */
  IBitmap(APIBitmap* pAPIBitmap, int n, bool framesAreHorizontal, const char* name = "")
    : mAPIBitmap(pAPIBitmap)
    , mW(pAPIBitmap->GetWidth() / pAPIBitmap->GetScale())
    , mH(pAPIBitmap->GetHeight() / pAPIBitmap->GetScale())
    , mN(n)
    , mFramesAreHorizontal(framesAreHorizontal)
    , mResourceName(name, (int) strlen(name))
  {
  }

  IBitmap()
  : mAPIBitmap(nullptr)
  , mW(0)
  , mH(0)
  , mN(0)
  , mFramesAreHorizontal(false)
  {
  }

  /** @return overall bitmap width in pixels */
  int W() const { return mW; }

  /** @return overall bitmap height in pixels */
  int H() const { return mH; }

  /** @return Width of a single frame in pixels */
  int FW() const { return (mFramesAreHorizontal ? mW / mN : mW); }
  
  /** @return Height of a single frame in pixels */
  int FH() const { return (mFramesAreHorizontal ? mH : mH / mN); }
  
  /** @return number of frames */
  int N() const { return mN; }
  
  /** @return the scale of the bitmap */
  int GetScale() const { return mAPIBitmap->GetScale(); }

  /** @return the draw scale of the bitmap */
  float GetDrawScale() const { return mAPIBitmap->GetDrawScale(); }
    
  /** @return a pointer to the referenced APIBitmap */
  APIBitmap* GetAPIBitmap() const { return mAPIBitmap; }

  /** @return whether or not frames are stored horizontally */
  bool GetFramesAreHorizontal() const { return mFramesAreHorizontal; }
  
  /** @return the resource name */
  const WDL_String& GetResourceName() const { return mResourceName; }
  
  /** @return \true if the bitmap has valid data */
  inline bool IsValid() const { return mAPIBitmap != nullptr; }

private:
  /** Pointer to the API specific bitmap */
  APIBitmap* mAPIBitmap;
  /** Bitmap width (in pixels) */
  int mW;
  /** Bitmap height (in pixels) */
  int mH;
  /** Number of frames (for stacked bitmaps) */
  int mN;
  /** \c true if the frames are positioned horizontally */
  bool mFramesAreHorizontal;
  /** Resource path/name for the bitmap */
  WDL_String mResourceName;
};

/** Used to manage SVG images used by the graphics context */
struct ISVG
{  
  ISVG(NSVGimage* pImage)
  {
    mImage = pImage;
  }

  /** /todo */
  float W() const
  {
    if (mImage)
      return mImage->width;
    else
      return 0;
  }

  /** /todo */
  float H() const
  {
    if (mImage)
      return mImage->height;
    else
      return 0;
  }
  
  /** @return \true if the SVG has valid data */
  inline bool IsValid() const { return mImage != nullptr; }
  
  NSVGimage* mImage = nullptr;
};

/** Used to manage color data, independent of draw class/platform. */
struct IColor
{
  int A, R, G, B;
  
  IColor(int a = 255, int r = 0, int g = 0, int b = 0) : A(a), R(r), G(g), B(b) {}

  bool operator==(const IColor& rhs) { return (rhs.A == A && rhs.R == R && rhs.G == G && rhs.B == B); }
  
  bool operator!=(const IColor& rhs) { return !operator==(rhs); }
  
  /** /todo */
  bool Empty() const { return A == 0 && R == 0 && G == 0 && B == 0; }
  
  /** /todo */
  void Clamp() { A = Clip(A, 0, 255); R = Clip(R, 0, 255); Clip(G, 0, 255); B = Clip(B, 0, 255); }
  
  /** /todo 
   * @param alpha */
  void Randomise(int alpha = 255) { A = alpha; R = std::rand() % 255; G = std::rand() % 255; B = std::rand() % 255; }

  /**  @param c /todo */
  void AddContrast(double c)
  {
    const int mod = int(c * 255.);
    R = std::min(R += mod, 255);
    G = std::min(G += mod, 255);
    B = std::min(B += mod, 255);
  }

  /** /todo 
   * @param c /todo
   * @return IColor /todo */
  IColor GetContrasted(double c) const
  {
    const int mod = int(c * 255.);
    IColor n = *this;
    n.R = std::min(n.R += mod, 255);
    n.G = std::min(n.G += mod, 255);
    n.B = std::min(n.B += mod, 255);
    return n;
  }

  /** /todo 
   * @param randomAlpha /todo
   * @return IColor /todo */
  static IColor GetRandomColor(bool randomAlpha = false)
  {
    int A = randomAlpha ? std::rand() & 0xFF : 255;
    int R = std::rand() & 0xFF;
    int G = std::rand() & 0xFF;
    int B = std::rand() & 0xFF;

    return IColor(A, R, G, B);
  }

  /** /todo 
   * @param h /todo
   * @param s /todo
   * @param l /todo
   * @param a /todo
   * @return IColor /todo */
  static IColor GetFromHSLA(float h, float s, float l, float a = 1.)
  {
    auto hue = [](float h, float m1, float m2)
    {
      if (h < 0) h += 1;
      if (h > 1) h -= 1;
      if (h < 1.0f / 6.0f)
        return m1 + (m2 - m1) * h * 6.0f;
      else if (h < 3.0f / 6.0f)
        return m2;
      else if (h < 4.0f / 6.0f)
        return m1 + (m2 - m1) * (2.0f / 3.0f - h) * 6.0f;
      return m1;
    };

    IColor col;
    h = std::fmodf(h, 1.0f);
    if (h < 0.0f) h += 1.0f;
    s = Clip(s, 0.0f, 1.0f);
    l = Clip(l, 0.0f, 1.0f);
    float m2 = l <= 0.5f ? (l * (1 + s)) : (l + s - l * s);
    float m1 = 2 * l - m2;
    col.R = static_cast<int>(Clip(hue(h + 1.0f / 3.0f, m1, m2), 0.0f, 1.0f) * 255.f);
    col.G = static_cast<int>(Clip(hue(h, m1, m2), 0.0f, 1.0f) * 255.f);
    col.B = static_cast<int>(Clip(hue(h - 1.0f / 3.0f, m1, m2), 0.0f, 1.0f) * 255.f);
    col.A = static_cast<int>(a * 255.f);
    return col;
  }

  /** /todo 
   * @return int /todo */
  int GetLuminosity() const
  {
    int min = R < G ? (R < B ? R : B) : (G < B ? G : B);
    int max = R > G ? (R > B ? R : B) : (G > B ? G : B);
    return (min + max) / 2;
  };
  
  /** /todo 
   * @param start /todo
   * @param dest /todo
   * @param result /todo
   * @param progress /todo */
  static void LinearInterpolateBetween(const IColor& start, const IColor& dest, IColor& result, float progress)
  {
    result.A = start.A + static_cast<int>(progress * static_cast<float>(dest.A -  start.A));
    result.R = start.R + static_cast<int>(progress * static_cast<float>(dest.R -  start.R));
    result.G = start.G + static_cast<int>(progress * static_cast<float>(dest.G -  start.G));
    result.B = start.B + static_cast<int>(progress * static_cast<float>(dest.B -  start.B));
  }
};

const IColor COLOR_TRANSPARENT(0, 0, 0, 0);
const IColor COLOR_TRANSLUCENT(10, 0, 0, 0);
const IColor COLOR_BLACK(255, 0, 0, 0);
const IColor COLOR_BLACK_DROP_SHADOW(128, 0, 0, 0);
const IColor COLOR_GRAY(255, 127, 127, 127);
const IColor COLOR_LIGHT_GRAY(255, 240, 240, 240);
const IColor COLOR_MID_GRAY(255, 200, 200, 200);
const IColor COLOR_DARK_GRAY(255, 70, 70, 70);
const IColor COLOR_WHITE(255, 255, 255, 255);
const IColor COLOR_RED(255, 255, 0, 0);
const IColor COLOR_GREEN(255, 0, 255, 0);
const IColor COLOR_BLUE(255, 0, 0, 255);
const IColor COLOR_YELLOW(255, 255, 255, 0);
const IColor COLOR_ORANGE(255, 255, 127, 0);

const IColor DEFAULT_GRAPHICS_BGCOLOR = COLOR_GRAY;
const IColor DEFAULT_BGCOLOR = COLOR_TRANSPARENT;
const IColor DEFAULT_FGCOLOR = COLOR_MID_GRAY;
const IColor DEFAULT_PRCOLOR = COLOR_LIGHT_GRAY;

const IColor DEFAULT_FRCOLOR = COLOR_DARK_GRAY;
const IColor DEFAULT_HLCOLOR = COLOR_TRANSLUCENT;
const IColor DEFAULT_SHCOLOR = IColor(60, 0, 0, 0);
const IColor DEFAULT_X1COLOR = COLOR_RED;
const IColor DEFAULT_X2COLOR = COLOR_GREEN;
const IColor DEFAULT_X3COLOR = COLOR_BLUE;

const IColor DEFAULT_TEXT_FGCOLOR = COLOR_BLACK;
const IColor DEFAULT_TEXTENTRY_BGCOLOR = COLOR_WHITE;
const IColor DEFAULT_TEXTENTRY_FGCOLOR = COLOR_BLACK;

/** Contains a set of colours used to theme IVControls */
struct IVColorSpec
{
  IColor mBGColor = DEFAULT_BGCOLOR; // Background
  IColor mFGColor = DEFAULT_FGCOLOR; // Foreground
  IColor mPRColor = DEFAULT_PRCOLOR; // Pressed
  IColor mFRColor = DEFAULT_FRCOLOR; // Frame
  IColor mHLColor = DEFAULT_HLCOLOR; // Highlight
  IColor mSHColor = DEFAULT_SHCOLOR; // Shadow
  IColor mX1Color = DEFAULT_X1COLOR; // Extra 1
  IColor mX2Color = DEFAULT_X2COLOR; // Extra 2
  IColor mX3Color = DEFAULT_X3COLOR; // Extra 3

  /** /todo  */
  void SetColors(const IColor BGColor = DEFAULT_BGCOLOR,
                 const IColor FGColor = DEFAULT_FGCOLOR,
                 const IColor PRColor = DEFAULT_PRCOLOR,
                 const IColor FRColor = DEFAULT_FRCOLOR,
                 const IColor HLColor = DEFAULT_HLCOLOR,
                 const IColor SHColor = DEFAULT_SHCOLOR,
                 const IColor X1Color = DEFAULT_X1COLOR,
                 const IColor X2Color = DEFAULT_X2COLOR,
                 const IColor X3Color = DEFAULT_X3COLOR)
  {
  }

  /** /todo  */
  void ResetColors() { SetColors(); }
};

const IVColorSpec DEFAULT_SPEC = IVColorSpec();

/** Used to manage composite/blend operations, independent of draw class/platform */
struct IBlend
{
  EBlendType mMethod;
  float mWeight;

  /** Creates a new IBlend
   * @param type Blend type (defaults to none)
   * @param weight normalised alpha blending amount */
  IBlend(EBlendType type = kBlendDefault, float weight = 1.0f)
  : mMethod(type)
  , mWeight(Clip(weight, 0.f, 1.f))
  {}
};

/** /todo 
 * @param pBlend /todo
 * @return float /todo */
inline float BlendWeight(const IBlend* pBlend)
{
  return (pBlend ? pBlend->mWeight : 1.0f);
}

const IBlend BLEND_75 = IBlend(kBlendDefault, 0.75f);
const IBlend BLEND_50 = IBlend(kBlendDefault, 0.5f);
const IBlend BLEND_25 = IBlend(kBlendDefault, 0.25f);
const IBlend BLEND_10 = IBlend(kBlendDefault, 0.1f);
const IBlend BLEND_05 = IBlend(kBlendDefault, 0.05f);
const IBlend BLEND_01 = IBlend(kBlendDefault, 0.01f);

/** Used to manage fill behaviour for path based drawing back ends */
struct IFillOptions
{
  IFillOptions()
  : mFillRule(kFillWinding)
  , mPreserve(false)
  {}

  EFillRule mFillRule;
  bool mPreserve;
};

/** Used to manage stroke behaviour for path based drawing back ends */
struct IStrokeOptions
{
  /** Used to manage dashes for stroke */
  class DashOptions
  {
  public:

    /** @return int /todo */
    int GetCount() const { return mCount; }

    /** @return float  /todo */
    float GetOffset() const { return mOffset; }

    /** @return float* /todo */
    const float* GetArray() const { return mArray; }

    /** /todo 
     * @param array /todo
     * @param offset /todo
     * @param count /todo */
    void SetDash(float* array, float offset, int count)
    {
      assert(count >= 0 && count <= 8);

      mCount = count;
      mOffset = offset;

      for (int i = 0; i < count; i++)
        mArray[i] = array[i];
    }

  private:
    float mArray[8];
    float mOffset = 0;
    int mCount = 0;
  };

  float mMiterLimit = 10.f;
  bool mPreserve = false;
  ELineCap mCapOption = kCapButt;
  ELineJoin mJoinOption = kJoinMiter;
  DashOptions mDash;
};

/** Used to specify text styles when loading fonts. */
enum ETextStyle { kTextStyleNormal, kTextStyleBold, kTextStyleItalic };

static const char* TextStyleString(ETextStyle style)
{
  switch (style)
  {
    case kTextStyleNormal:  return "Regular";
    case kTextStyleBold:    return "Bold";
    case kTextStyleItalic:  return "Italic";
  }
}

/** Used to manage font and text/text entry style for a piece of text on the UI, independent of draw class/platform.*/
struct IText
{
  /** /todo */
  enum EAlign { kAlignNear, kAlignCenter, kAlignFar } mAlign;

  /** /todo */
  enum EVAlign { kVAlignTop, kVAlignMiddle, kVAlignBottom } mVAlign;

  /** /todo 
   * @param size /todo
   * @param color /todo
   * @param font /todo
   * @param align /todo
   * @param valign /todo
   * @param orientation /todo
   * @param TEBGColor /todo
   * @param TEFGColor /todo */
  IText(int size = DEFAULT_TEXT_SIZE,
        const IColor& color = DEFAULT_TEXT_FGCOLOR,
        const char* font = nullptr,
        EAlign align = kAlignCenter,
        EVAlign valign = kVAlignMiddle,
        int orientation = 0,
        const IColor& TEBGColor = DEFAULT_TEXTENTRY_BGCOLOR,
        const IColor& TEFGColor = DEFAULT_TEXTENTRY_FGCOLOR)
    : mSize(size)
    , mFGColor(color)
    , mAlign(align)
    , mVAlign(valign)
    , mOrientation(orientation)
    , mTextEntryBGColor(TEBGColor)
    , mTextEntryFGColor(TEFGColor)
  {
    strcpy(mFont, (font ? font : DEFAULT_FONT));
  }

  /** /todo 
    * @param size /todo
    * @param valign /todo */
  IText(int size, EVAlign valign)
  : IText()
  {
    mSize = size;
    mVAlign = valign;
  }
  
  /** /todo 
   * @param size /todo
   * @param align /todo */
  IText(int size, EAlign align)
  : IText()
  {
    mSize = size;
    mAlign = align;
  }
    
  char mFont[FONT_LEN];
  int mSize;
  IColor mFGColor;
  IColor mTextEntryBGColor;
  IColor mTextEntryFGColor;
  int mOrientation = 0; // Degrees ccwise from normal.
};

const IText DEFAULT_TEXT = IText();

/** Used to retrieve font info directly from a raw memory buffer. */
class IFontInfo
{
public:
  IFontInfo(const void* data, uint32_t dataSize, uint32_t faceIdx)
  : mData(reinterpret_cast<const unsigned char*>(data)), mHeadLocation(0), mNameLocation(0), mHheaLocation(0), mMacStyle(0), mUnitsPerEM(0), mAscender(0), mDescender(0), mLineGap(0)
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
        mFamily = GetFontString(1);
        mStyle = GetFontString(2);
        mAscender = GetSInt16(mHheaLocation + 4);
        mDescender = GetSInt16(mHheaLocation + 6);
        mLineGap = GetSInt16(mHheaLocation + 8);
      }
    }
  }
  
  bool IsValid() const       { return mData && mHeadLocation && mNameLocation && mHheaLocation; }
  
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
  
  WDL_String GetFontString(int nameID)
  {
#ifdef OS_WIN
    int platformID = 3;
    int encodingID = 1;
    int languageID = 0x409;
#else
    int platformID = 1;
    int encodingID = 0;
    int languageID = 0;
#endif
    
    for (uint16_t i = 0; i < GetUInt16(mNameLocation + 2); ++i)
    {
      uint32_t loc = mNameLocation + 6 + (12 * i);
      
      if (platformID == GetUInt16(loc + 0) && encodingID == GetUInt16(loc + 2)
          && languageID == GetUInt16(loc + 4) && nameID == GetUInt16(loc + 6))
      {
        uint32_t stringLocation = GetUInt16(mNameLocation + 4) + GetUInt16(loc + 10);
        uint16_t length = GetUInt16(loc + 8);
        
#ifdef OS_WIN
        WDL_TypedBuf<WCHAR> utf16;
        WDL_TypedBuf<char> utf8;
        
        utf16.Resize(length / sizeof(WCHAR));
        
        for (int j = 0; j < length; j++)
          utf16.Get()[j] = GetUInt16(mNameLocation + stringLocation + j * 2);
        
        int convertedLength = WideCharToMultiByte(CP_UTF8, 0, utf16.Get(), utf16.GetSize(), 0, 0, NULL, NULL);
        utf8.Resize(convertedLength);
        WideCharToMultiByte(CP_UTF8, 0, utf16.Get(), utf16.GetSize(), utf8.Get(), utf8.GetSize(), NULL, NULL);
        return WDL_String(utf8.Get(), convertedLength);
#else
        return WDL_String((const char*)(mData + mNameLocation + stringLocation), length);
#endif
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
  uint16_t   GetUInt16(uint32_t loc)  { return (((uint16_t)mData[loc + 0]) << 8) | (uint16_t)mData[loc + 1]; }
  int16_t    GetSInt16(uint32_t loc)  { return (((uint16_t)mData[loc + 0]) << 8) | (uint16_t)mData[loc + 1]; }
  uint32_t   GetUInt32(uint32_t loc)  { return (((uint32_t)GetUInt16(loc + 0)) << 16) | (uint32_t)GetUInt16(loc + 2); }
  int32_t    GetSInt32(uint32_t loc)  { return (((uint32_t)GetUInt16(loc + 0)) << 16) | (uint32_t)GetUInt16(loc + 2); }
#else
  uint16_t   GetUInt16(uint32_t loc)  { return (((uint16_t)mData[loc + 1]) << 8) | (uint16_t)mData[loc + 0]; }
  int16_t    GetSInt16(uint32_t loc)  { return (((uint16_t)mData[loc + 1]) << 8) | (uint16_t)mData[loc + 0]; }
  uint32_t   GetUInt32(uint32_t loc)  { return (((uint32_t)GetUInt16(loc + 2)) << 16) | (uint32_t)GetUInt16(loc + 0); }
  int32_t    GetSInt32(uint32_t loc)  { return (((uint32_t)GetUInt16(loc + 2)) << 16) | (uint32_t)GetUInt16(loc + 0); }
#endif
  
  // Data
  const unsigned char* mData;
  
  uint32_t mHeadLocation;
  uint32_t mNameLocation;
  uint32_t mHheaLocation;
  uint32_t mFDscLocation;
  
  // Font Identifiers
  WDL_String mFamily;
  WDL_String mStyle;
  uint16_t mMacStyle;
  
  // Metrics
  uint16_t mUnitsPerEM;
  int16_t mAscender;
  int16_t mDescender;
  int16_t mLineGap;
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
typedef std::unique_ptr<IFontData> IFontDataPtr;

/** /todo */
class PlatformFont
{
public:
  PlatformFont(bool system) : mSystem(system) {}
  virtual ~PlatformFont() {}
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

typedef std::unique_ptr<PlatformFont> PlatformFontPtr;

/** Used to manage a rectangular area, independent of draw class/platform.
 * An IRECT is always specified in 1:1 pixels, any scaling for high DPI happens in the drawing class.
 * In IGraphics 0,0 is top left. */
struct IRECT
{
  float L, T, R, B;

  /** /todo  */
  IRECT()
  {
    L = T = R = B = 0.f;
  }
  
  /** /todo 
   * @param l /todo
   * @param t /todo
   * @param r /todo
   * @param b /todo */
  IRECT(float l, float t, float r, float b)
  : L(l), R(r), T(t), B(b)
  {}
  
  /** /todo 
   * @param x /todo
   * @param y /todo
   * @param bitmap /todo */
  IRECT(float x, float y, const IBitmap& bitmap)
  {
    L = x;
    T = y;
    R = L + (float) bitmap.FW();
    B = T + (float) bitmap.FH();
  }

  /** @return true */
  bool Empty() const
  {
    return (L == 0.f && T == 0.f && R == 0.f && B == 0.f);
  }

  /** /todo  */
  void Clear()
  {
    L = T = R = B = 0.f;
  }

  bool operator==(const IRECT& rhs) const
  {
    return (L == rhs.L && T == rhs.T && R == rhs.R && B == rhs.B);
  }

  bool operator!=(const IRECT& rhs) const
  {
    return !(*this == rhs);
  }

  /** @return float /todo  */
  inline float W() const { return R - L; }

  /** @return float /todo  */
  inline float H() const { return B - T; }

  /** @return float /todo  */
  inline float MW() const { return 0.5f * (L + R); }

  /** @return float /todo  */
  inline float MH() const { return 0.5f * (T + B); }

  /** @return float /todo  */
  inline float Area() const { return W() * H(); }
  
  /** /todo 
   * @param rhs /todo
   * @return IRECT /todo*/
  inline IRECT Union(const IRECT& rhs) const
  {
    if (Empty()) { return rhs; }
    if (rhs.Empty()) { return *this; }
    return IRECT(std::min(L, rhs.L), std::min(T, rhs.T), std::max(R, rhs.R), std::max(B, rhs.B));
  }

  /** /todo 
   * @param rhs /todo
   * @return IRECT /todo */
  inline IRECT Intersect(const IRECT& rhs) const
  {
    if (Intersects(rhs))
      return IRECT(std::max(L, rhs.L), std::max(T, rhs.T), std::min(R, rhs.R), std::min(B, rhs.B));

    return IRECT();
  }

  /** /todo 
   * @param rhs /todo
   * @return true /todo
   * @return false /todo */
  inline bool Intersects(const IRECT& rhs) const
  {
    return (!Empty() && !rhs.Empty() && R >= rhs.L && L < rhs.R && B >= rhs.T && T < rhs.B);
  }

  /** /todo 
   * @param rhs /todo
   * @return true /todo
   * @return false /todo */
  inline bool Contains(const IRECT& rhs) const
  {
    return (!Empty() && !rhs.Empty() && rhs.L >= L && rhs.R <= R && rhs.T >= T && rhs.B <= B);
  }

  /** /todo 
   * @param x /todo
   * @param y /todo
   * @return true /todo
   * @return false /todo */
  inline bool Contains(float x, float y) const
  {
    return (!Empty() && x >= L && x < R && y >= T && y < B);
  }
  
  /** /todo
   * includes right-most and bottom-most pixels
   * @param x /todo
   * @param y /todo
   * @return true /todo
   * @return false /todo */
  inline bool ContainsEdge(float x, float y) const
  {
    return (!Empty() && x >= L && x <= R && y >= T && y <= B);
  }

  /** /todo 
   * @param x /todo
   * @param y /todo */
  inline void Constrain(float& x, float& y)
  {
    if (x < L) x = L;
    else if (x > R) x = R;

    if (y < T) y = T;
    else if (y > B) y = B;
  }
  
  /** /todo
   * The two rects cover exactly the area returned by Union()
   * @param rhs /todo
   * @return true /todo */
  bool Mergeable(const IRECT& rhs) const
  {
    if (Empty() || rhs.Empty())
      return true;
    if (L == rhs.L && R == rhs.R && ((T >= rhs.T && T <= rhs.B) || (rhs.T >= T && rhs.T <= B)))
      return true;
    return T == rhs.T && B == rhs.B && ((L >= rhs.L && L <= rhs.R) || (rhs.L >= L && rhs.L <= R));
  }
  
  /** /todo 
   * @param layoutDir /todo
   * @param frac /todo
   * @param fromTopOrRight /todo
   * @return IRECT /todo */
  inline IRECT FracRect(EDirection layoutDir, float frac, bool fromTopOrRight = false) const
  {
    if(layoutDir == EDirection::kVertical)
      return FracRectVertical(frac, fromTopOrRight);
    else
      return FracRectHorizontal(frac, fromTopOrRight);
  }
  
  /** /todo 
   * @param frac /todo
   * @param rhs /todo
   * @return IRECT /todo */
  inline IRECT FracRectHorizontal(float frac, bool rhs = false) const
  {
    float widthOfSubRect = W() * frac;
    
    if(rhs)
      return IRECT(R - widthOfSubRect, T, R, B);
    else
      return IRECT(L, T, L + widthOfSubRect, B);
  }
  
  /** /todo 
   * @param frac /todo
   * @param fromTop /todo
   * @return IRECT /todo */
  inline IRECT FracRectVertical(float frac, bool fromTop = false) const
  {
    float heightOfSubRect = H() * frac;

    if(fromTop)
      return IRECT(L, T, R, T + heightOfSubRect);
    else
      return IRECT(L, B - heightOfSubRect, R, B);
  }

  /** /todo 
   * @param numSlices /todo
   * @param sliceIdx /todo
   * @return IRECT /todo */
  inline IRECT SubRectVertical(int numSlices, int sliceIdx) const
  {
    float heightOfSubRect = H() / (float) numSlices;
    float t = heightOfSubRect * (float) sliceIdx;

    return IRECT(L, T + t, R, T + t + heightOfSubRect);
  }

  /** /todo 
   * @param numSlices /todo
   * @param sliceIdx /todo
   * @return IRECT /todo */
  inline IRECT SubRectHorizontal(int numSlices, int sliceIdx) const
  {
    float widthOfSubRect = W() / (float) numSlices;
    float l = widthOfSubRect * (float) sliceIdx;

    return IRECT(L + l, T, L + l + widthOfSubRect, B);
  }
  
  /** /todo 
   * @param layoutDir /todo
   * @param numSlices /todo
   * @param sliceIdx /todo
   * @return IRECT /todo */
  inline IRECT SubRect(EDirection layoutDir, int numSlices, int sliceIdx) const
  {
    if(layoutDir == EDirection::kVertical)
      return SubRectVertical(numSlices, sliceIdx);
    else
      return SubRectHorizontal(numSlices, sliceIdx);
  }
  
  /** /todo 
   * @param w /todo
   * @param h /todo
   * @return IRECT /todo */
  inline IRECT GetFromTLHC(float w, float h) const { return IRECT(L, T, L+w, T+h); }

  /** /todo 
   * @param w /todo
   * @param h /todo
   * @return IRECT /todo */
  inline IRECT GetFromBLHC(float w, float h) const { return IRECT(L, B-h, L+w, B); }

  /** /todo 
   * @param w /todo
   * @param h /todo
   * @return IRECT /todo */
  inline IRECT GetFromTRHC(float w, float h) const { return IRECT(R-w, T, R, T+h); }

  /** /todo 
   * @param w /todo
   * @param h /todo
   * @return IRECT /todo */
  inline IRECT GetFromBRHC(float w, float h) const { return IRECT(R-w, B-h, R, B); }

  /** Get a subrect of this IRECT bounded in Y by the top edge and 'amount'
   * @param amount Size in Y of the desired IRECT
   * @return IRECT The resulting subrect */
  inline IRECT GetFromTop(float amount) const { return IRECT(L, T, R, T+amount); }

  /** Get a subrect of this IRECT bounded in Y by 'amount' and the bottom edge
   * @param amount Size in Y of the desired IRECT
   * @return IRECT The resulting subrect */
  inline IRECT GetFromBottom(float amount) const { return IRECT(L, B-amount, R, B); }

  /** Get a subrect of this IRECT bounded in X by the left edge and 'amount'
   * @param amount Size in X of the desired IRECT
   * @return IRECT The resulting subrect */
  inline IRECT GetFromLeft(float amount) const { return IRECT(L, T, L+amount, B); }

  /** Get a subrect of this IRECT bounded in X by 'amount' and the right edge
   * @param amount Size in X of the desired IRECT
   * @return IRECT The resulting subrect */
  inline IRECT GetFromRight(float amount) const { return IRECT(R-amount, T, R, B); }
  
  /** Get a subrect of this IRECT reduced in height from the top edge by 'amount'
   * @param amount Size in Y to reduce by
   * @return IRECT The resulting subrect */
  inline IRECT GetReducedFromTop(float amount) const { return IRECT(L, T+amount, R, B); }

  /** Get a subrect of this IRECT reduced in height from the bottom edge by 'amount'
   * @param amount Size in Y to reduce by
   * @return IRECT The resulting subrect */
  inline IRECT GetReducedFromBottom(float amount) const { return IRECT(L, T, R, B-amount); }

  /** Get a subrect of this IRECT reduced in width from the left edge by 'amount'
   * @param amount Size in X to reduce by
   * @return IRECT The resulting subrect */
  inline IRECT GetReducedFromLeft(float amount) const { return IRECT(L+amount, T, R, B); }

  /** Get a subrect of this IRECT reduced in width from the right edge by 'amount'
   * @param amount Size in X to reduce by
   * @return IRECT The resulting subrect */
  inline IRECT GetReducedFromRight(float amount) const { return IRECT(L, T, R-amount, B); }
  
  /** Get a subrect (by row, column) of this IRECT which is a cell in a grid of size (nRows * nColumns)
   * @param row Row index of the desired subrect
   * @param col Column index of the desired subrect
   * @param nRows Number of rows in the cell grid
   * @param nColumns Number of columns in the cell grid
   * @return IRECT The resulting subrect */
  inline IRECT GetGridCell(int row, int col, int nRows, int nColumns/*, EDirection = kHorizontal*/) const
  {
    assert(row * col <= nRows * nColumns); // not enough cells !
    
    const IRECT vrect = SubRectVertical(nRows, row);
    return vrect.SubRectHorizontal(nColumns, col);
  }
  
  /** Get a subrect (by index) of this IRECT which is a cell in a grid of size (nRows * nColumns)
   * @param cellIndex Index of the desired cell in the cell grid
   * @param nRows Number of rows in the cell grid
   * @param nColumns Number of columns in the cell grid
   * @param dir Desired direction of indexing, by row (kHorizontal) or by column (kVertical)
   * @return IRECT The resulting subrect */
  inline IRECT GetGridCell(int cellIndex, int nRows, int nColumns, EDirection dir = kHorizontal) const
  {
    assert(cellIndex <= nRows * nColumns); // not enough cells !

    int cell = 0;
    
    if(dir == kHorizontal)
    {
      for(int row = 0; row < nRows; row++)
      {
        for(int col = 0; col < nColumns; col++)
        {
          if(cell == cellIndex)
          {
            const IRECT vrect = SubRectVertical(nRows, row);
            return vrect.SubRectHorizontal(nColumns, col);
          }

          cell++;
        }
      }
    }
    else
    {
      for(int col = 0; col < nColumns; col++)
      {
        for(int row = 0; row < nRows; row++)
        {
          if(cell == cellIndex)
          {
            const IRECT hrect = SubRectHorizontal(nColumns, col);
            return hrect.SubRectVertical(nRows, row);
          }
          
          cell++;
        }
      }
    }
    
    return *this;
  }
  
  /** @return true /todo */
  bool IsPixelAligned() const
  {
    // If all values are within 1/1000th of a pixel of an integer the IRECT is considered pixel aligned
      
    auto isInteger = [](float x){ return std::fabs(x - std::round(x)) <= static_cast<float>(1e-3); };
      
    return isInteger(L) && isInteger(T) && isInteger(R) && isInteger(B);
  }
  
  /** /todo
   * @param scale /todo
   * @return false /todo */
  bool IsPixelAligned(float scale) const
  {
    IRECT r = *this;
    r.Scale(scale);
    return r.IsPixelAligned();
  }
  
  /** Pixel aligns the rect in an inclusive manner (moves all points outwards) */
  inline void PixelAlign() 
  {
    L = std::floor(L);
    T = std::floor(T);
    R = std::ceil(R);
    B = std::ceil(B);
  }

  /** /todo 
   * @param scale /todo */
  inline void PixelAlign(float scale)
  {
    // N.B. - double precision is *required* for accuracy of the reciprocal
    Scale(scale);
    PixelAlign();
    Scale(static_cast<float>(1.0/static_cast<double>(scale)));
  }
  
  /** /todo 
   * @return IRECT /todo  */
  inline IRECT GetPixelAligned() const
  {
    IRECT r = *this;
    r.PixelAlign();
    return r;
  }
  
  /** /todo 
   * @param scale /todo
   * @return IRECT /todo */
  inline IRECT GetPixelAligned(float scale) const
  {
    IRECT r = *this;
    r.PixelAlign(scale);
    return r;
  }
    
  /** Pixel aligns to nearest pixels */
  inline void PixelSnap()
  {
    L = std::round(L);
    T = std::round(T);
    R = std::round(R);
    B = std::round(B);
  }
  
  /** /todo 
   * @param scale /todo */
  inline void PixelSnap(float scale)
  {
    // N.B. - double precision is *required* for accuracy of the reciprocal
    Scale(scale);
    PixelSnap();
    Scale(static_cast<float>(1.0/static_cast<double>(scale)));
  }
  
  /** @return IRECT /todo */
  inline IRECT GetPixelSnapped() const
  {
    IRECT r = *this;
    r.PixelSnap();
    return r;
  }
  
  /** /todo 
   * @param scale /todo
   * @return IRECT /todo */
  inline IRECT GetPixelSnapped(float scale) const
  {
    IRECT r = *this;
    r.PixelSnap(scale);
    return r;
  }
  
  /** /todo 
   * @param padding /todo */
  inline void Pad(float padding)
  {
    L -= padding;
    T -= padding;
    R += padding;
    B += padding;
  }
  
  /** /todo 
   * @param padL /todo
   * @param padT /todo
   * @param padR /todo
   * @param padB /todo */
  inline void Pad(float padL, float padT, float padR, float padB)
  {
    L -= padL;
    T -= padT;
    R += padR;
    B += padB;
  }
  
  /** /todo 
  * @param padding /todo */
  inline void HPad(float padding)
  {
    L -= padding;
    R += padding;
  }
  
  /** /todo 
  * @param padding /todo */
  inline void VPad(float padding)
  {
    T -= padding;
    B += padding;
  }
  
  /** /todo 
   * @param padding /todo */
  inline void MidHPad(float padding)
  {
    const float mw = MW();
    L = mw - padding;
    R = mw + padding;
  }
  
  /** /todo 
   * @param padding /todo */
  inline void MidVPad(float padding)
  {
    const float mh = MH();
    T = mh - padding;
    B = mh + padding;
  }

  /** /todo 
   * @param padding /todo
   * @return IRECT /todo */
  inline IRECT GetPadded(float padding) const
  {
    return IRECT(L-padding, T-padding, R+padding, B+padding);
  }

  /** /todo 
   * @param padding /todo
   * @return IRECT /todo */
  inline IRECT GetPadded(float padL, float padT, float padR, float padB) const
  {
    return IRECT(L-padL, T-padT, R+padR, B+padB);
  }

  /** /todo 
   * @param padding /todo
   * @return IRECT /todo */
  inline IRECT GetHPadded(float padding) const
  {
    return IRECT(L-padding, T, R+padding, B);
  }

  /** /todo 
   * @param padding /todo
   * @return IRECT /todo */
  inline IRECT GetVPadded(float padding) const
  {
    return IRECT(L, T-padding, R, B+padding);
  }

  /** /todo 
   * @param padding /todo
   * @return IRECT /todo */
  inline IRECT GetMidHPadded(float padding) const
  {
    return IRECT(MW()-padding, T, MW()+padding, B);
  }

  /** /todo 
   * @param padding /todo
   * @return IRECT /todo */
  inline IRECT GetMidVPadded(float padding) const
  {
    return IRECT(L, MH()-padding, R, MH()+padding);
  }

  /** /todo 
   * @param w /todo
   * @param rhs /todo
   * @return IRECT /todo */
  inline IRECT GetHSliced(float w, bool rhs = false) const
  {
    if(rhs)
      return IRECT(R - w, T, R, B);
    else
      return IRECT(L, T, L + w, B);
  }
  
  /** /todo 
   * @param h /todo
   * @param bot /todo
   * @return IRECT /todo */
  inline IRECT GetVSliced(float h, bool bot = false) const
  {
    if(bot)
      return IRECT(L, B - h, R, B);
    else
      return IRECT(L, T + h, R, B);
  }
  
  /** /todo 
   * @param rhs /todo */
  void Clank(const IRECT& rhs)
  {
    if (L < rhs.L)
    {
      R = std::min(rhs.R - 1, R + rhs.L - L);
      L = rhs.L;
    }
    if (T < rhs.T)
    {
      B = std::min(rhs.B - 1, B + rhs.T - T);
      T = rhs.T;
    }
    if (R >= rhs.R)
    {
      L = std::max(rhs.L, L - (R - rhs.R + 1));
      R = rhs.R - 1;
    }
    if (B >= rhs.B)
    {
      T = std::max(rhs.T, T - (B - rhs.B + 1));
      B = rhs.B - 1;
    }
  }
  
  /** /todo 
   * @param scale /todo */
  void Scale(float scale)
  {
    L *= scale;
    T *= scale;
    R *= scale;
    B *= scale;
  }
  
  /** /todo 
   * @param scale /todo  */
  void ScaleAboutCentre(float scale)
  {
    float x = MW();
    float y = MH();
    float hw = W() / 2.f;
    float hh = H() / 2.f;
    L = x - (hw * scale);
    T = y - (hh * scale);
    R = x + (hw * scale);
    B = y + (hh * scale);
  }
  
  /** /todo 
   * @param scale /todo
   * @return IRECT /todo */
  IRECT GetScaledAboutCentre(float scale)
  {
    const float x = MW();
    const float y = MH();
    const float hw = W() / 2.f;
    const float hh = H() / 2.f;
    
    return IRECT(x - (hw * scale), y - (hh * scale), x + (hw * scale), y + (hh * scale));
  }
  
  /** /todo 
   * @param start /todo
   * @param dest /todo
   * @param result /todo
   * @param progress /todo */
  static void LinearInterpolateBetween(const IRECT& start, const IRECT& dest, IRECT& result, float progress)
  {
    result.L = start.L + progress * (dest.L -  start.L);
    result.T = start.T + progress * (dest.T -  start.T);
    result.R = start.R + progress * (dest.R -  start.R);
    result.B = start.B + progress * (dest.B -  start.B);
  }

  /** /todo 
   * @param scale /todo
   * @return IRECT /todo */
  IRECT GetScaled(float scale) const
  {
    IRECT r = *this;
    r.Scale(scale);
    return r;
  }

  /** /todo 
   * @param x /todo
   * @param y /todo */
  void GetRandomPoint(float& x, float& y) const
  {
    const float r1 = static_cast<float>(std::rand()/(RAND_MAX+1.f));
    const float r2 = static_cast<float>(std::rand()/(RAND_MAX+1.f));

    x = L + r1 * W();
    y = T + r2 * H();
  }

  /** @return IRECT /todo */
  IRECT GetRandomSubRect() const
  {
    float l, t, r, b;
    GetRandomPoint(l, t);
    IRECT tmp = IRECT(l, t, R, B);
    tmp.GetRandomPoint(r, b);

    return IRECT(l, t, r, b);
  }

  /** /todo 
   * @param l /todo
   * @param t /todo
   * @param r /todo
   * @param b /todo */
  void Alter(float l, float t, float r, float b)
  {
    L += l;
    T += t;
    R += r;
    B += b;
  }
  
  /** /todo 
   * @param l /todo
   * @param t /todo
   * @param r /todo
   * @param b /todo
   * @return IRECT /todo  */
  IRECT GetAltered(float l, float t, float r, float b) const
  {
    return IRECT(L + l, T + t, R + r, B + b);
  }
  
  /** /todo 
   * @param x /todo
   * @param y /todo */
  void Translate(float x, float y)
  {
    L += x;
    T += y;
    R += x;
    B += y;
  }
  
  /** /todo 
   * @param x /todo
   * @param y /todo
   * @return IRECT /todo */
  IRECT GetTranslated(float x, float y) const
  {
    return IRECT(L + x, T + y, R + x, B + y);
  }
  
  /** /todo 
   * @param x /todo
   * @return IRECT /todo */
  IRECT GetHShifted(float x) const
  {
    return GetTranslated(x, 0.f);
  }
  
  /** /todo 
   * @param y /todo
   * @return IRECT /todo */
  IRECT GetVShifted(float y) const
  {
    return GetTranslated(0.f, y);
  }

  /** /todo 
   * @param sr /todo
   * @return IRECT /todo */
  IRECT GetCentredInside(IRECT sr) const
  {
    IRECT r;
    r.L = MW() - sr.W() / 2.f;
    r.T = MH() - sr.H() / 2.f;
    r.R = r.L + sr.W();
    r.B = r.T + sr.H();

    return r;
  }
  
  /** /todo 
   * @param w /todo
   * @param h /todo
   * @return IRECT /todo */
  IRECT GetCentredInside(float w, float h = 0.f) const
  {
    assert(w > 0.f);
    
    if(h <= 0.f)
      h = w;
    
    IRECT r;
    r.L = MW() - w / 2.f;
    r.T = MH() - h / 2.f;
    r.R = r.L + w;
    r.B = r.T + h;
    
    return r;
  }

  /** /todo 
   * @param bitmap /todo
   * @return IRECT /todo */
  IRECT GetCentredInside(IBitmap bitmap)
  {
    IRECT r;
    r.L = MW() - bitmap.FW() / 2.f;
    r.T = MH() - bitmap.FH() / 2.f;
    r.R = r.L + (float) bitmap.FW();
    r.B = r.T + (float) bitmap.FH();

    return r;
  }
  
  /** /todo 
   * @return float /todo */
  float GetLengthOfShortestSide() const
  {
    if(W() < H())
       return W();
    else
      return H();
  }
};

/** Used for key press info, such as ASCII representation, virtual key (mapped to win32 codes) and modifiers */
struct IKeyPress
{
  int VK; // Windows VK_XXX
  char utf8[5] = {0}; // UTF8 key
  bool S, C, A; // SHIFT / CTRL(WIN) or CMD (MAC) / ALT
  
  /** /todo 
   * @param unichar /todo
   * @param vk /todo
   * @param s /todo
   * @param c /todo
   * @param a /todo */
  IKeyPress(const char* _utf8, int vk, bool s = false, bool c = false, bool a = false)
  : VK(vk)
  , S(s), C(c), A(a)
  {
    strcpy(utf8, _utf8);
  }
};

/** Used to manage mouse modifiers i.e. right click and shift/control/alt keys. */
struct IMouseMod
{
  bool L, R, S, C, A;

  /** /todo 
   * @param l /todo
   * @param r /todo
   * @param s /todo
   * @param c /todo
   * @param a /todo */
  IMouseMod(bool l = false, bool r = false, bool s = false, bool c = false, bool a = false)
    : L(l), R(r), S(s), C(c), A(a) 
    {}
  
  /** /todo */
  void DBGPrint() { DBGMSG("L: %i, R: %i, S: %i, C: %i,: A: %i\n", L, R, S, C, A); }
};

/** Used to group mouse coordinates with mouse modifier information */
struct IMouseInfo
{
  float x, y;
  IMouseMod ms;
};

/** Used to manage a list of rectangular areas and optimize them for drawing to the screen. */
class IRECTList
{
public:
  /** /todo
   * @return int /todo */
  int Size() const { return mRects.GetSize(); }
  
  /** /todo 
   * @param rect /todo */
  void Add(const IRECT rect)
  {
    mRects.Add(rect);
  }
  
  /** /todo 
   * @param idx /todo
   * @param rect /todo */
  void Set(int idx, const IRECT rect)
  {
    *(mRects.GetFast() + idx) = rect;
  }
  
  /** /todo 
   * @param idx /todo
   * @return const IRECT& /todo */
  const IRECT& Get(int idx) const
  {
    return *(mRects.GetFast() + idx);
  }
  
  /** /todo */
  void Clear()
  {
    mRects.Resize(0);
  }
  
  /** /todo * @return IRECT /todo */
  IRECT Bounds()
  {
    IRECT r = Get(0);
    for (auto i = 1; i < mRects.GetSize(); i++)
      r = r.Union(Get(i));
    return r;
  }
  
  /** /todo */
  void PixelAlign()
  {
    for (auto i = 0; i < Size(); i++)
    {
      IRECT r = Get(i);
      r.PixelAlign();
      Set(i, r);
    }
  }

  /** /todo 
   * @param scale /todo */
  void PixelAlign(float scale)
  {
    for (auto i = 0; i < Size(); i++)
    {
      IRECT r = Get(i);
      r.PixelAlign(scale);
      Set(i, r);
    }
  }
  
  /** /todo 
   * @param input /todo
   * @param rects /todo
   * @param rowFractions /todo
   * @param colFractions /todo
   * @return true /todo
   * @return false /todo */
  static bool GetFracGrid(const IRECT& input, IRECTList& rects, const std::initializer_list<float>& rowFractions, const std::initializer_list<float>& colFractions)
  {
    IRECT rowsLeft = input;
    float y = 0.;
    float x = 0.;

    if(std::accumulate(rowFractions.begin(), rowFractions.end(), 0.f) != 1.)
      return false;
    
    if(std::accumulate(colFractions.begin(), colFractions.end(), 0.f) != 1.)
      return false;

    for (auto& rowFrac : rowFractions)
    {
      IRECT thisRow = input.FracRectVertical(rowFrac, true).GetTranslated(0, y);
      
      x = 0.;

      for (auto& colFrac : colFractions)
      {
        IRECT thisCell = thisRow.FracRectHorizontal(colFrac).GetTranslated(x, 0);
        
        rects.Add(thisCell);
        
        x += thisCell.W();
      }
      
      rowsLeft.Intersect(thisRow);
      
      y = rects.Bounds().H();
    }
    
    return true;
  }
  
  /** /todo  */
  void Optimize()
  {
    // Remove rects that are contained by other rects and intersections
    for (int i = 0; i < Size(); i++)
    {
      for (int j = i + 1; j < Size(); j++)
      {
        if (Get(i).Contains(Get(j)))
        {
          mRects.Delete(j);
          j--;
        }
        else if (Get(j).Contains(Get(i)))
        {
          mRects.Delete(i);
          i--;
          break;
        }
        else if (Get(i).Intersects(Get(j)))
        {
          IRECT intersection = Get(i).Intersect(Get(j));
            
          if (Get(i).Mergeable(intersection))
            Set(i, Shrink(Get(i), intersection));
          else if (Get(j).Mergeable(intersection))
            Set(j, Shrink(Get(j), intersection));
          else if (Get(i).Area() < Get(j).Area())
            Set(i, Split(Get(i), intersection));
          else
            Set(j, Split(Get(j), intersection));
        }
      }
    }
    
    // Merge any rects that can be merged
    for (int i = 0; i < Size(); i++)
    {
      for (int j = i + 1; j < Size(); j++)
      {
        if (Get(i).Mergeable(Get(j)))
        {
          Set(j, Get(i).Union(Get(j)));
          mRects.Delete(i);
          i = -1;
          break;
        }
      }
    }
  }
  
private:
  /** /todo 
   * @param r /todo
   * @param i /todo
   * @return IRECT /todo */
  IRECT Shrink(const IRECT &r, const IRECT &i)
  {
    if (i.L != r.L)
      return IRECT(r.L, r.T, i.L, r.B);
    if (i.T != r.T)
      return IRECT(r.L, r.T, r.R, i.T);
    if (i.R != r.R)
      return IRECT(i.R, r.T, r.R, r.B);
    return IRECT(r.L, i.B, r.R, r.B);
  }
  
  /** /todo 
   * @param r /todo
   * @param i /todo
   * @return IRECT /todo */
  IRECT Split(const IRECT r, const IRECT &i)
  {
    if (r.L == i.L)
    {
      if (r.T == i.T)
      {
        Add(IRECT(i.R, r.T, r.R, i.B));
        return IRECT(r.L, i.B, r.R, r.B);
      }
      else
      {
        Add(IRECT(r.L, r.T, r.R, i.T));
        return IRECT(i.R, i.T, r.R, r.B);
      }
    }
    
    if (r.T == i.T)
    {
      Add(IRECT(r.L, r.T, i.L, i.B));
      return IRECT(r.L, i.B, r.R, r.B);
    }
    else
    {
      Add(IRECT(r.L, r.T, r.R, i.T));
      return IRECT(r.L, i.T, i.L, r.B);
    }
  }
  
  WDL_TypedBuf<IRECT> mRects;
};

/** Used to store transformation matrices **/
struct IMatrix
{
  /** /todo 
   * @param xx /todo
   * @param yx /todo
   * @param xy /todo
   * @param yy /todo
   * @param tx /todo
   * @param ty /todo */
  IMatrix(double xx, double yx, double xy, double yy, double tx, double ty)
  : mXX(xx), mYX(yx), mXY(xy), mYY(yy), mTX(tx), mTY(ty)
  {}
  
  /** /todo */
  IMatrix() : IMatrix(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
  {}
  
  /** /todo 
   * @param x /todo
   * @param y /todo
   * @return IMatrix& /todo */
  IMatrix& Translate(float x, float y)
  {
    return Transform(IMatrix(1.0, 0.0, 0.0, 1.0, x, y));
  }
  
  /** /todo 
   * @param x /todo
   * @param y /todo
   * @return IMatrix& /todo */
  IMatrix& Scale(float x, float y)
  {
    return Transform(IMatrix(x, 0.0, 0.0, y, 0.0, 0.0));
  }
  
  /** /todo 
   * @param a /todo
   * @return IMatrix& /todo */
  IMatrix& Rotate(float a)
  {
    const double rad = DegToRad(a);
    const double c = std::cos(rad);
    const double s = std::sin(rad);
    
    return Transform(IMatrix(c, s, -s, c, 0.0, 0.0));
  }
  
  /** /todo 
   * @param xa /todo
   * @param ya /todo
   * @return IMatrix& /todo */
  IMatrix& Skew(float xa, float ya)
  {
    return Transform(IMatrix(1.0, std::tan(DegToRad(ya)), std::tan(DegToRad(xa)), 1.0, 0.0, 0.0));
  }
  
  /** /todo 
   * @param x /todo
   * @param y /todo
   * @param x0 /todo
   * @param y0 /todo */
  void TransformPoint(double& x, double& y, double x0, double y0)
  {
    x = x0 * mXX + y0 * mXY + mTX;
    y = x0 * mYX + y0 * mYY + mTY;
  };
  
  /** /todo 
   * @param x /todo
   * @param y /todo */
  void TransformPoint(double& x, double& y)
  {
    TransformPoint(x, y, x, y);
  };
  
  /** /todo 
   * @param before /todo
   * @param after /todo
   * @return IMatrix& /todo */
  IMatrix& Transform(const IRECT& before, const IRECT& after)
  {
    const double sx = after.W() / before.W();
    const double sy = after.H() / before.H();
    const double tx = after.L - before.L * sx;
    const double ty = after.T - before.T * sy;
    
    return *this = IMatrix(sx, 0.0, 0.0, sy, tx, ty);
  }
  
  /** /todo 
   * @param m /todo
   * @return IMatrix& /todo */
  IMatrix& Transform(const IMatrix& m)
  {
    IMatrix p = *this;
    
    mXX = m.mXX * p.mXX + m.mYX * p.mXY;
    mYX = m.mXX * p.mYX + m.mYX * p.mYY;
    mXY = m.mXY * p.mXX + m.mYY * p.mXY;
    mYY = m.mXY * p.mYX + m.mYY * p.mYY;
    mTX = m.mTX * p.mXX + m.mTY * p.mXY + p.mTX;
    mTY = m.mTX * p.mYX + m.mTY * p.mYY + p.mTY;
    
    return *this;
  }
  
  /** /todo 
   * @return IMatrix& /todo */
  IMatrix& Invert()
  {
    IMatrix m = *this;
    
    double d = 1.0 / (m.mXX * m.mYY - m.mYX * m.mXY);
    
    mXX =  m.mYY * d;
    mYX = -m.mYX * d;
    mXY = -m.mXY * d;
    mYY =  m.mXX * d;
    mTX = (-(m.mTX * mXX) - (m.mTY * mXY));
    mTY = (-(m.mTX * mYX) - (m.mTY * mYY));
    
    return *this;
  }
  
  double mXX, mYX, mXY, mYY, mTX, mTY;
};

/** Used to represent a point/stop in a gradient **/
struct IColorStop
{
  IColorStop()
  : mOffset(0.f)
  {}
  
  /** /todo 
   * @param color /todo
   * @param offset /todo */
  IColorStop(IColor color, float offset)
  : mColor(color)
  , mOffset(offset)
  {
    assert(offset >= 0.0 && offset <= 1.0);
  }
  
  IColor mColor;
  float mOffset;
};

/** Used to store pattern information for gradients **/
struct IPattern
{
  EPatternType mType;
  EPatternExtend mExtend;
  IColorStop mStops[16];
  int mNStops;
  IMatrix mTransform;
  
  /** /todo 
   * @param type /todo */
  IPattern(EPatternType type)
  : mType(type), mExtend(kExtendPad), mNStops(0)
  {}
  
  /** /todo 
   * @param color /todo */
  IPattern(const IColor& color)
  : mType(kSolidPattern), mExtend(kExtendPad), mNStops(1)
  {
    mStops[0] = IColorStop(color, 0.0);
  }
  
  /** /todo 
   * @param x1 /todo
   * @param y1 /todo
   * @param x2 /todo
   * @param y2 /todo
   * @param stops /todo
   * @return IPattern /todo */
  static IPattern CreateLinearGradient(float x1, float y1, float x2, float y2, const std::initializer_list<IColorStop>& stops = {})
  {
    IPattern pattern(kLinearPattern);
    
    // Calculate the affine transform from one line segment to another!
    const double xd = x2 - x1;
    const double yd = y2 - y1;
    const double d = sqrt(xd * xd + yd * yd);
    const double a = atan2(xd, yd);
    const double s = std::sin(a) / d;
    const double c = std::cos(a) / d;
      
    const double x0 = -(x1 * c - y1 * s);
    const double y0 = -(x1 * s + y1 * c);
    
    pattern.SetTransform(static_cast<float>(c),
                         static_cast<float>(s),
                         static_cast<float>(-s),
                         static_cast<float>(c),
                         static_cast<float>(x0),
                         static_cast<float>(y0));
    
    for (auto& stop : stops)
      pattern.AddStop(stop.mColor, stop.mOffset);
    
    return pattern;
  }
  
  /** /todo 
   * @param bounds /todo
   * @param direction /todo
   * @param stops /todo
   * @return IPattern /todo */
  static IPattern CreateLinearGradient(const IRECT& bounds, EDirection direction, const std::initializer_list<IColorStop>& stops = {})
  {
    float x1, y1, x2, y2;
    
    if(direction == kHorizontal)
    {
      y1 = bounds.MH(); y2 = y1;
      x1 = bounds.L;
      x2 = bounds.R;
    }
    else//(direction == kVertical)
    {
      x1 = bounds.MW(); x2 = x1;
      y1 = bounds.T;
      y2 = bounds.B;
    }
    
    return CreateLinearGradient(x1, y1, x2, y2, stops);
  }
  
  /** /todo 
   * @param x1 /todo
   * @param y1 /todo
   * @param r /todo
   * @param stops /todo
   * @return IPattern /todo */
  static IPattern CreateRadialGradient(float x1, float y1, float r, const std::initializer_list<IColorStop>& stops = {})
  {
    IPattern pattern(kRadialPattern);
    
    const float s = 1.f / r;

    pattern.SetTransform(s, 0, 0, s, -(x1 * s), -(y1 * s));
    
    for (auto& stop : stops)
      pattern.AddStop(stop.mColor, stop.mOffset);
    
    return pattern;
  }
  
  /** /todo 
   * @return int /todo */
  int NStops() const
  {
    return mNStops;
  }
  
  /** /todo 
   * @param idx /todo
   * @return const IColorStop& /todo */
  const IColorStop& GetStop(int idx) const
  {
    return mStops[idx];
  }
  
  /** /todo 
   * @param color /todo
   * @param offset /todo */
  void AddStop(IColor color, float offset)
  {
    assert(mType != kSolidPattern && mNStops < 16);
    assert(!mNStops || GetStop(mNStops - 1).mOffset < offset);
    if (mNStops < 16)
      mStops[mNStops++] = IColorStop(color, offset);
  }
  
  /** /todo 
   * @param xx /todo
   * @param yx /todo
   * @param xy /todo
   * @param yy /todo
   * @param x0 /todo
   * @param y0 /todo */
  void SetTransform(float xx, float yx, float xy, float yy, float x0, float y0)
  {
    mTransform = IMatrix(xx, yx, xy, yy, x0, y0);
  }
  
  /** /todo 
   * @param transform /todo */
  void SetTransform(const IMatrix& transform)
  {
    mTransform = transform;
  }
};

/** An abstraction that is used to store a temporary raster image/framebuffer.
 * The layer is drawn with a specific offset to the graphics context.
 * ILayers take ownership of the underlying bitmaps
 * In GPU-based backends (NanoVG), this is a texture. */
class ILayer
{
  friend IGraphics;
  
public:
  /** /todo 
   * @param pBitmap /todo
   * @param r /todo */
  ILayer(APIBitmap* pBitmap, IRECT r)
  : mBitmap(pBitmap)
  , mRECT(r)
  , mInvalid(false)
  {}

  ILayer(const ILayer&) = delete;
  ILayer operator =(const ILayer&) = delete;
  
  /** /todo */
  void Invalidate() { mInvalid = true; }

  /**  @return const APIBitmap* /todo */
  const APIBitmap* GetAPIBitmap() const { return mBitmap.get(); }

  /** @return IBitmap /todo */
  IBitmap GetBitmap() const { return IBitmap(mBitmap.get(), 1, false); }

  /** @return const IRECT& /todo*/
  const IRECT& Bounds() const { return mRECT; }
  
private:
  APIBitmap* AccessAPIBitmap() { return mBitmap.get(); }
  
  std::unique_ptr<APIBitmap> mBitmap;
  IRECT mRECT;
  bool mInvalid;
};

/** ILayerPtr is a managed pointer for transferring the ownership of layers */
typedef std::unique_ptr<ILayer> ILayerPtr;

/** Used to specify a gaussian drop-shadow. */
struct IShadow
{
  IShadow(){}

  /** /todo 
   * @param pattern /todo
   * @param blurSize /todo
   * @param xOffset /todo
   * @param yOffset /todo
   * @param opacity /todo
   * @param drawForeground /todo */
  IShadow(const IPattern& pattern, float blurSize, float xOffset, float yOffset, float opacity, bool drawForeground = true)
  : mPattern(pattern)
  , mBlurSize(blurSize)
  , mXOffset(xOffset)
  , mYOffset(yOffset)
  , mOpacity(opacity)
  , mDrawForeground(drawForeground)
  {}
    
  IPattern mPattern = COLOR_BLACK;
  float mBlurSize = 0.f;
  float mXOffset = 0.f;
  float mYOffset = 0.f;
  float mOpacity = 1.f;
  bool mDrawForeground = true;
};

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
  
  ~StaticStorage()
  {
    Clear();
  }

private:
  /** /todo */
  struct DataKey
  {
    // N.B. - hashID is not guaranteed to be unique
    size_t hashID;
    WDL_String name;
    double scale;
    std::unique_ptr<T> data;
  };
  
  /** /todo 
   * @param str /todo
   * @return size_t /todo */
  size_t Hash(const char* str)
  {
    std::string string(str);
    return std::hash<std::string>()(string);
  }

  /** /todo 
   * @param str /todo
   * @param scale /todo
   * @return T* /todo */
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

  /** /todo 
   * @param pData /todo
   * @param str /todo
   * @param scale /todo scale where 2x = retina, omit if not needed */
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

  /** /todo @param pData /todo */
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

  /** /todo  */
  void Clear()
  {
    mDatas.Empty(true);
  };

  /** /todo  */
  void Retain()
  {
    mCount++;
  }
  
  /** /todo  */
  void Release()
  {
    if (--mCount == 0)
      Clear();
  }
    
  int mCount;
  WDL_Mutex mMutex;
  WDL_PtrList<DataKey> mDatas;
};

/**@}*/
