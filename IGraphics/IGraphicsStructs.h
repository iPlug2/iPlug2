/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include <cmath>
#include <cassert>
#include <functional>
#include <algorithm>
#include <random>
#include <chrono>

#include "wdlstring.h"
#include "ptrlist.h"
#if defined OS_MAC || defined OS_LINUX
#include "swell.h"
#endif

#include "nanosvg.h"

#include "IPlugPlatform.h"
#include "IGraphicsConstants.h"

class IGraphics;
class IControl;
struct IRECT;
struct IMouseInfo;
template <typename T = double>
inline T DegToRad(T degrees);

typedef std::function<void(IControl*)> IActionFunction;
typedef std::function<void(IControl*)> IAnimationFunction;
typedef std::function<void(IControl*, IGraphics&, IRECT&, IMouseInfo&, double)> IDrawFunction;

void DefaultClickActionFunc(IControl* pCaller);
void DefaultAnimationFunc(IControl* pCaller);
void FlashCircleClickActionFunc(IControl* pCaller);
void FlashCircleClickAnimationFunc(IControl* pCaller);

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;
typedef std::chrono::duration<double, std::chrono::milliseconds::period> Milliseconds;

/**
 * \defgroup IGraphicsStructs IGraphics::Structs
 * @{
 */

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
  class LICE_IFont;
#elif defined IGRAPHICS_CANVAS
  #include <emscripten.h>
  #include <emscripten/val.h>
typedef emscripten::val* BitmapData;
#else // NO_IGRAPHICS
  typedef void* BitmapData;
#endif

/** APIBitmap is a wrapper around the different drawing backend bitmap representations.
 * In most cases it does own the bitmap data, the exception being with NanoVG, where the image is loaded onto the GPU as a texture,
 * but still needs to be freed
 */

class APIBitmap
{
public:
  APIBitmap(BitmapData pBitmap, int w, int h, int s, float ds)
  : mBitmap(pBitmap)
  , mWidth(w)
  , mHeight(h)
  , mScale(s)
  , mDrawScale(ds)
  {}

  APIBitmap()
  : mBitmap(0)
  , mWidth(0)
  , mHeight(0)
  , mScale(0)
  , mDrawScale(1.f)
  {}

  virtual ~APIBitmap() {}

  void SetBitmap(BitmapData pBitmap, int w, int h, int s, float ds)
  {
    mBitmap = pBitmap;
    mWidth = w;
    mHeight = h;
    mScale = s;
    mDrawScale = ds;
  }

  BitmapData GetBitmap() const { return mBitmap; }
  int GetWidth() const { return mWidth; }
  int GetHeight() const { return mHeight; }
  int GetScale() const { return mScale; }
  float GetDrawScale() const { return mDrawScale; }

private:
  BitmapData mBitmap; // for most drawing APIs BitmapData is a pointer. For Nanovg it is an integer index
  int mWidth;
  int mHeight;
  int mScale;
  float mDrawScale;
};

/** IBitmap is IGraphics's bitmap abstraction that you use to manage bitmap data, independant of draw class/platform.
 * IBitmap doesn't actually own the image data @see APIBitmap
 * An IBitmap's width and height are always in relation to a 1:1 (low dpi) screen. Any scaling happens at the drawing stage. */
class IBitmap
{
public:
  /** Creates a new IBitmap object
  * @param pData Pointer to the raw bitmap data
  * @param w Bitmap width (in pixels)
  * @param h Bitmap height (in pixels)
  * @param n Number of frames (for multibitmaps)
  * @param framesAreHorizontal \c true if the frames are positioned horizontally
  * @param name Resource name for the bitmap */
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

  /** @return overall bitmap width */
  inline int W() const { return mW; }

  /** @return overall bitmap height */
  inline int H() const { return mH; }

  /** @return Width of a single frame */
  inline int FW() const { return (mFramesAreHorizontal ? mW / mN : mW); }
  
  /** @return Height of a single frame */
  inline int FH() const { return (mFramesAreHorizontal ? mH : mH / mN); }
  
  /** * @return number of frames */
  inline int N() const { return mN; }
  
  /** * @return the scale of the bitmap */
  inline int GetScale() const { return mAPIBitmap->GetScale(); }

  /** * @return the draw scale of the bitmap */
  inline float GetDrawScale() const { return mAPIBitmap->GetDrawScale(); }
    
  /** * @return a pointer to the referenced APIBitmap */
  inline APIBitmap* GetAPIBitmap() const { return mAPIBitmap; }

  /** * @return whether or not frames are stored horiztonally */
  inline bool GetFramesAreHorizontal() const { return mFramesAreHorizontal; }
  
  /** * @return the resource name */
  inline const WDL_String& GetResourceName() const { return mResourceName; }

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

struct ISVG
{
  NSVGimage* mImage = nullptr;
  
  ISVG(NSVGimage* pImage)
  {
    mImage = pImage;
    assert(mImage != nullptr);
  }

  float W()
  {
    if (mImage)
      return mImage->width;
    else
      return 0;
  }

  float H()
  {
    if (mImage)
      return mImage->height;
    else
      return 0;
  }
};

/** Used to manage Color data, independant of draw class/platform.*/
struct IColor
{
  int A, R, G, B;
  IColor(int a = 255, int r = 0, int g = 0, int b = 0) : A(a), R(r), G(g), B(b) {}
  bool operator==(const IColor& rhs) { return (rhs.A == A && rhs.R == R && rhs.G == G && rhs.B == B); }
  bool operator!=(const IColor& rhs) { return !operator==(rhs); }
  bool Empty() const { return A == 0 && R == 0 && G == 0 && B == 0; }
  void Clamp() { A = std::min(A, 255); R = std::min(R, 255); G = std::min(G, 255); B = std::min(B, 255); }
  void Randomise(int alpha = 255) { A = alpha; R = std::rand() % 255; G = std::rand() % 255; B = std::rand() % 255; }

  void AddContrast(double c)
  {
    const int mod = int(c * 255.);
    R = std::min(R += mod, 255);
    G = std::min(G += mod, 255);
    B = std::min(B += mod, 255);
  }

  IColor GetContrasted(double c) const
  {
    const int mod = int(c * 255.);
    IColor n = *this;
    n.R = std::min(n.R += mod, 255);
    n.G = std::min(n.G += mod, 255);
    n.B = std::min(n.B += mod, 255);
    return n;
  }

  static IColor GetRandomColor(bool randomAlpha = false)
  {
    int A = randomAlpha ? std::rand() & 0xFF : 255;
    int R = std::rand() & 0xFF;
    int G = std::rand() & 0xFF;
    int B = std::rand() & 0xFF;

    return IColor(A, R, G, B);
  }

  int GetLuminocity() const
  {
    int min = R < G ? (R < B ? R : B) : (G < B ? G : B);
    int max = R > G ? (R > B ? R : B) : (G > B ? G : B);
    return (min + max) / 2;
  };
  
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

struct IVColorSpec
{
  IColor mBGColor = DEFAULT_BGCOLOR; // Background
  IColor mFGColor = DEFAULT_FGCOLOR; // Foreground
  IColor mPRColor = DEFAULT_PRCOLOR; // Pressed
  IColor mFRColor = DEFAULT_FRCOLOR; // Frame
  IColor mHLColor = DEFAULT_HLCOLOR; // Higlight
  IColor mSHColor = DEFAULT_SHCOLOR; // Shadow
  IColor mX1Color = DEFAULT_X1COLOR; // Extra 1
  IColor mX2Color = DEFAULT_X2COLOR; // Extra 2
  IColor mX3Color = DEFAULT_X3COLOR; // Extra 3

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

  void ResetColors() { SetColors(); }
};

const IVColorSpec DEFAULT_SPEC = IVColorSpec();

/** Used to manage composite/blend operations, independant of draw class/platform */
struct IBlend
{
  EBlendType mMethod;
  float mWeight;

  /** Creates a new IBlend
   * @param type Blend type (defaults to none)
   * \todo IBlend::weight needs documentation
   * @param weight
  */
  IBlend(EBlendType type = kBlendNone, float weight = 1.0f) : mMethod(type), mWeight(weight) {}
};

inline float BlendWeight(const IBlend* pBlend)
{
  return (pBlend ? pBlend->mWeight : 1.0f);
}

const IBlend BLEND_75 = IBlend(kBlendNone, 0.75f);
const IBlend BLEND_50 = IBlend(kBlendNone, 0.5f);
const IBlend BLEND_25 = IBlend(kBlendNone, 0.25f);
const IBlend BLEND_10 = IBlend(kBlendNone, 0.1f);

// Path related structures for patterns and fill/stroke options

struct IFillOptions
{
  IFillOptions()
  : mFillRule(kFillWinding)
  , mPreserve(false)
  {}

  EFillRule mFillRule;
  bool mPreserve;
};

struct IStrokeOptions
{
  class DashOptions
  {
  public:
    int GetCount() const { return mCount; }
    float GetOffset() const { return mOffset; }
    const float *GetArray() const { return mArray; }

    void SetDash(float *array, float offset, int count)
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

  float mMiterLimit = 1.;
  bool mPreserve = false;
  ELineCap mCapOption = kCapButt;
  ELineJoin mJoinOption = kJoinMiter;
  DashOptions mDash;
};

/** Used to manage font and text/text entry style for a piece of text on the UI, independant of draw class/platform.*/
struct IText
{
  enum EStyle { kStyleNormal, kStyleBold, kStyleItalic } mStyle;
  enum EAlign { kAlignNear, kAlignCenter, kAlignFar } mAlign;
  enum EVAlign { kVAlignTop, kVAlignMiddle, kVAlignBottom } mVAlign;
  enum EQuality { kQualityDefault, kQualityNonAntiAliased, kQualityAntiAliased, kQualityClearType } mQuality = kQualityDefault;

  IText(int size = DEFAULT_TEXT_SIZE,
        const IColor& color = DEFAULT_TEXT_FGCOLOR,
        const char* font = nullptr,
        EStyle style = kStyleNormal,
        EAlign align = kAlignCenter,
        EVAlign valign = kVAlignMiddle,
        int orientation = 0,
        EQuality quality = kQualityDefault,
        const IColor& TEBGColor = DEFAULT_TEXTENTRY_BGCOLOR,
        const IColor& TEFGColor = DEFAULT_TEXTENTRY_FGCOLOR)
    : mSize(size)
    , mFGColor(color)
    , mStyle(style)
    , mAlign(align)
    , mVAlign(valign)
    , mOrientation(orientation)
    , mQuality(quality)
    , mTextEntryBGColor(TEBGColor)
    , mTextEntryFGColor(TEFGColor)
  {
    strcpy(mFont, (font ? font : DEFAULT_FONT));
  }

  IText(int size, EVAlign valign)
  : IText()
  {
    mSize = size;
    mVAlign = valign;
  }
  
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
  mutable double mCachedScale = 1.0;

#ifdef IGRAPHICS_LICE
  mutable LICE_IFont* mCached = nullptr;
#endif
};

const IText DEFAULT_TEXT = IText();

/** Used to manage a rectangular area, independant of draw class/platform.
 * An IRECT is always specified in 1:1 pixels, any scaling for high DPI happens in the drawing class.
 * In IGraphics 0,0 is top left. */
struct IRECT
{
  float L, T, R, B;

  IRECT()
  {
    L = T = R = B = 0.f;
  }
  
  IRECT(float l, float t, float r, float b)
  : L(l), R(r), T(t), B(b)
  {}
  
  IRECT(float x, float y, const IBitmap& bitmap)
  {
    L = x;
    T = y;
    R = L + (float) bitmap.FW();
    B = T + (float) bitmap.FH();
  }

  bool Empty() const
  {
    return (L == 0.f && T == 0.f && R == 0.f && B == 0.f);
  }

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

  inline float W() const { return R - L; }
  inline float H() const { return B - T; }
  inline float MW() const { return 0.5f * (L + R); }
  inline float MH() const { return 0.5f * (T + B); }
  inline float Area() const { return W() * H(); }
  
  inline IRECT Union(const IRECT& rhs) const
  {
    if (Empty()) { return rhs; }
    if (rhs.Empty()) { return *this; }
    return IRECT(std::min(L, rhs.L), std::min(T, rhs.T), std::max(R, rhs.R), std::max(B, rhs.B));
  }

  inline IRECT Intersect(const IRECT& rhs) const
  {
    if (Intersects(rhs))
      return IRECT(std::max(L, rhs.L), std::max(T, rhs.T), std::min(R, rhs.R), std::min(B, rhs.B));

    return IRECT();
  }

  inline bool Intersects(const IRECT& rhs) const
  {
    return (!Empty() && !rhs.Empty() && R >= rhs.L && L < rhs.R && B >= rhs.T && T < rhs.B);
  }

  inline bool Contains(const IRECT& rhs) const
  {
    return (!Empty() && !rhs.Empty() && rhs.L >= L && rhs.R <= R && rhs.T >= T && rhs.B <= B);
  }

  inline bool Contains(float x, float y) const
  {
    return (!Empty() && x >= L && x < R && y >= T && y < B);
  }
  
  //includes right-most and bottom-most pixels
  inline bool ContainsEdge(float x, float y) const
  {
    return (!Empty() && x >= L && x <= R && y >= T && y <= B);
  }

  inline void Constrain(float& x, float& y)
  {
    if (x < L) x = L;
    else if (x > R) x = R;

    if (y < T) y = T;
    else if (y > B) y = B;
  }
  
  //The two rects cover exactly the area returned by Union()
  bool Mergeable(const IRECT& rhs) const
  {
    if (Empty() || rhs.Empty())
      return true;
    if (L == rhs.L && R == rhs.R && ((T >= rhs.T && T <= rhs.B) || (rhs.T >= T && rhs.T <= B)))
      return true;
    return T == rhs.T && B == rhs.B && ((L >= rhs.L && L <= rhs.R) || (rhs.L >= L && rhs.L <= R));
  }
  
  inline IRECT FracRect(EDirection layoutDir, float frac, bool fromTopOrRight = false) const
  {
    if(layoutDir == EDirection::kVertical)
      return FracRectVertical(frac, fromTopOrRight);
    else
      return FracRectHorizontal(frac, fromTopOrRight);
  }
  
  inline IRECT FracRectHorizontal(float frac, bool rhs = false) const
  {
    float widthOfSubRect = W() * frac;
    
    if(rhs)
      return IRECT(R - widthOfSubRect, T, R, B);
    else
      return IRECT(L, T, L + widthOfSubRect, B);
  }
  
  inline IRECT FracRectVertical(float frac, bool fromTop = false) const
  {
    float heightOfSubRect = H() * frac;

    if(fromTop)
      return IRECT(L, T, R, T + heightOfSubRect);
    else
      return IRECT(L, B - heightOfSubRect, R, B);
  }

  inline IRECT SubRectVertical(int numSlices, int sliceIdx) const
  {
    float heightOfSubRect = H() / (float) numSlices;
    float t = heightOfSubRect * (float) sliceIdx;

    return IRECT(L, T + t, R, T + t + heightOfSubRect);
  }

  inline IRECT SubRectHorizontal(int numSlices, int sliceIdx) const
  {
    float widthOfSubRect = W() / (float) numSlices;
    float l = widthOfSubRect * (float) sliceIdx;

    return IRECT(L + l, T, L + l + widthOfSubRect, B);
  }
  
  inline IRECT SubRect(EDirection layoutDir, int numSlices, int sliceIdx) const
  {
    if(layoutDir == EDirection::kVertical)
      return SubRectVertical(numSlices, sliceIdx);
    else
      return SubRectHorizontal(numSlices, sliceIdx);
  }
  
  inline IRECT GetFromTLHC(float w, float h) const { return IRECT(L, T, L+w, T+h); }
  inline IRECT GetFromBLHC(float w, float h) const { return IRECT(L, B-h, L+w, B); }
  inline IRECT GetFromTRHC(float w, float h) const { return IRECT(R-w, T, R, T+h); }
  inline IRECT GetFromBRHC(float w, float h) const { return IRECT(R-w, B-h, R, B); }

  inline IRECT GetFromTop(float amount) const { return IRECT(L, T, R, T+amount); }
  inline IRECT GetFromBottom(float amount) const { return IRECT(L, B-amount, R, B); }
  inline IRECT GetFromLeft(float amount) const { return IRECT(L, T, L+amount, B); }
  inline IRECT GetFromRight(float amount) const { return IRECT(R-amount, T, R, B); }
  
  inline IRECT GetReducedFromTop(float amount) const { return IRECT(L, T+amount, R, B); }
  inline IRECT GetReducedFromBottom(float amount) const { return IRECT(L, T, R, B-amount); }
  inline IRECT GetReducedFromLeft(float amount) const { return IRECT(L+amount, T, R, B); }
  inline IRECT GetReducedFromRight(float amount) const { return IRECT(L, T, R-amount, B); }
  
  inline IRECT GetGridCell(int row, int col, int nRows, int nColumns/*, EDirection = kHorizontal*/) const
  {
    assert(row * col <= nRows * nColumns); // not enough cells !
    
    const IRECT vrect = SubRectVertical(nRows, row);
    return vrect.SubRectHorizontal(nColumns, col);
  }
  
  inline IRECT GetGridCell(int cellIndex, int nRows, int nColumns/*, EDirection = kHorizontal*/) const
  {
    assert(cellIndex <= nRows * nColumns); // not enough cells !

    int cell = 0;
    for(int row = 0; row<nRows; row++)
    {
      for(int col = 0; col<nColumns; col++)
      {
        if(cell == cellIndex)
        {
          const IRECT vrect = SubRectVertical(nRows, row);
          return vrect.SubRectHorizontal(nColumns, col);
        }

        cell++;
      }
    }

    return *this;
  }
  
  bool IsPixelAligned() const
  {
    return !(L - std::floor(L) && T - std::floor(T) && R - std::floor(R) && B - std::floor(B));
  }
  
  // Pixel aligns in an inclusive manner (moves all points outwards)
  inline void PixelAlign() 
  {
    L = std::floor(L);
    T = std::floor(T);
    R = std::ceil(R);
    B = std::ceil(B);
  }
  
  inline void Pad(float padding)
  {
    L -= padding;
    T -= padding;
    R += padding;
    B += padding;
  }
  
  inline void Pad(float padL, float padT, float padR, float padB)
  {
    L += padL;
    T += padT;
    R += padR;
    B += padB;
  }
  
  inline void HPad(float padding)
  {
    L -= padding;
    R += padding;
  }
  
  inline void VPad(float padding)
  {
    T -= padding;
    B += padding;
  }
  
  inline void MidHPad(float padding)
  {
    const float mw = MW();
    L = mw - padding;
    R = mw + padding;
  }
  
  inline void MidVPad(float padding)
  {
    const float mh = MH();
    T = mh - padding;
    B = mh + padding;
  }

  inline IRECT GetPadded(float padding) const
  {
    return IRECT(L-padding, T-padding, R+padding, B+padding);
  }

  inline IRECT GetPadded(float padL, float padT, float padR, float padB) const
  {
    return IRECT(L+padL, T+padT, R+padR, B+padB);
  }

  inline IRECT GetHPadded(float padding) const
  {
    return IRECT(L-padding, T, R+padding, B);
  }

  inline IRECT GetVPadded(float padding) const
  {
    return IRECT(L, T-padding, R, B+padding);
  }

  inline IRECT GetMidHPadded(float padding) const
  {
    return IRECT(MW()-padding, T, MW()+padding, B);
  }

  inline IRECT GetMidVPadded(float padding) const
  {
    return IRECT(L, MH()-padding, R, MH()+padding);
  }

  inline IRECT GetHSliced(float w, bool rhs = false) const
  {
    if(rhs)
      return IRECT(R - w, T, R, B);
    else
      return IRECT(L, T, L + w, B);
  }
  
  inline IRECT GetVSliced(float h, bool bot = false) const
  {
    if(bot)
      return IRECT(L, B - h, R, B);
    else
      return IRECT(L, T + h, R, B);
  }
  
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
  
  void Scale(float scale)
  {
    L = std::floor(0.5f + (L * scale));
    T = std::floor(0.5f + (T * scale));
    R = std::floor(0.5f + (R * scale));
    B = std::floor(0.5f + (B * scale));
  }
  
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
  
  IRECT GetScaledAboutCentre(float scale)
  {
    const float x = MW();
    const float y = MH();
    const float hw = W() / 2.f;
    const float hh = H() / 2.f;
    
    return IRECT(x - (hw * scale), y - (hh * scale), x + (hw * scale), y + (hh * scale));
  }
  
  static void LinearInterpolateBetween(const IRECT& start, const IRECT& dest, IRECT& result, float progress)
  {
    result.L = start.L + progress * (dest.L -  start.L);
    result.T = start.T + progress * (dest.T -  start.T);
    result.R = start.R + progress * (dest.R -  start.R);
    result.B = start.B + progress * (dest.B -  start.B);
  }

  IRECT GetScaled(float scale) const
  {
    IRECT r = *this;
    r.Scale(scale);
    return r;
  }

  void GetRandomPoint(float& x, float& y) const
  {
    std::random_device rd;
    std::mt19937 gen(rd()); // TODO: most sensible RNG?
    std::uniform_real_distribution<float> dist(0., 1.);
    x = L + dist(gen) * W();
    y = T + dist(gen) * H();
  }

  IRECT GetRandomSubRect() const
  {
    float l, t, r, b;
    GetRandomPoint(l, t);
    IRECT tmp = IRECT(l, t, R, B);
    tmp.GetRandomPoint(r, b);

    return IRECT(l, t, r, b);
  }

  void Alter(float l, float t, float r, float b)
  {
    L += l;
    T += t;
    R += r;
    B += b;
  }
  
  IRECT GetAltered(float l, float t, float r, float b) const
  {
    return IRECT(L + l, T + t, R + r, B + b);
  }
  
  void Translate(float x, float y)
  {
    L += x;
    T += y;
    R += x;
    B += y;
  }
  
  IRECT GetTranslated(float x, float y) const
  {
    return IRECT(L + x, T + y, R + x, B + y);
  }
  
  IRECT GetHShifted(float x) const
  {
    return GetTranslated(x, 0.f);
  }
  
  IRECT GetVShifted(float y) const
  {
    return GetTranslated(0.f, y);
  }
  
  void ScaleBounds(float scale)
  {
    L = std::floor(L * scale);
    T = std::floor(T * scale);
    R = std::ceil(R * scale);
    B = std::ceil(B * scale);
  }

  IRECT GetCentredInside(IRECT sr) const
  {
    IRECT r;
    r.L = MW() - sr.W() / 2.f;
    r.T = MH() - sr.H() / 2.f;
    r.R = r.L + sr.W();
    r.B = r.T + sr.H();

    return r;
  }
  
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

  IRECT GetCentredInside(IBitmap bitmap)
  {
    IRECT r;
    r.L = MW() - bitmap.FW() / 2.f;
    r.T = MH() - bitmap.FH() / 2.f;
    r.R = r.L + (float) bitmap.FW();
    r.B = r.T + (float) bitmap.FH();

    return r;
  }
  
  float GetLengthOfShortestSide() const
  {
    if(W() < H())
       return W();
    else
      return H();
  }
};

/** Used to manage mouse modifiers i.e. right click and shift/control/alt keys. */
struct IMouseMod
{
  bool L, R, S, C, A;
  IMouseMod(bool l = false, bool r = false, bool s = false, bool c = false, bool a = false)
    : L(l), R(r), S(s), C(c), A(a) {}
  
  void DBGPrint() { DBGMSG("L: %i, R: %i, S: %i, C: %i,: A: %i\n", L, R, S, C, A); }
};

struct IMouseInfo
{
  float x, y;
  IMouseMod ms;
};

/** Used to manage a list of rectangular areas and optimize them for drawing to the screen. */
class IRECTList
{
public:
  int Size() const { return mRects.GetSize(); }
  
  void Add(const IRECT rect)
  {
    mRects.Add(rect);
  }
  
  void Set(int idx, const IRECT rect)
  {
    *(mRects.GetFast() + idx) = rect;
  }
  
  const IRECT& Get(int idx) const
  {
    return *(mRects.GetFast() + idx);
  }
  
  void Clear()
  {
    mRects.Resize(0);
  }
  
  IRECT Bounds()
  {
    IRECT r = Get(0);
    for (auto i = 1; i < mRects.GetSize(); i++)
      r = r.Union(Get(i));
    return r;
  }
  
  void PixelAlign()
  {
    for (auto i = 0; i < Size(); i++)
    {
      IRECT r = Get(i);
      r.PixelAlign();
      Set(i, r);
    }
  }
  
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

/** Used to store transformation matrices**/
struct IMatrix
{
  IMatrix(double xx, double yx, double xy, double yy, double tx, double ty)
  : mXX(xx), mYX(yx), mXY(xy), mYY(yy), mTX(tx), mTY(ty)
  {}
  
  IMatrix() : IMatrix(1.0, 0.0, 0.0, 1.0, 0.0, 0.0)
  {}
  
  IMatrix& Translate(float x, float y)
  {
    return Transform(IMatrix(1.0, 0.0, 0.0, 1.0, x, y));
  }
  
  IMatrix& Scale(float x, float y)
  {
    return Transform(IMatrix(x, 0.0, 0.0, y, 0.0, 0.0));
  }
  
  IMatrix& Rotate(float a)
  {
    const double rad = DegToRad(a);
    const double c = std::cos(rad);
    const double s = std::sin(rad);
    
    return Transform(IMatrix(c, s, -s, c, 0.0, 0.0));
  }
  
  IMatrix& Skew(float xa, float ya)
  {
    return Transform(IMatrix(1.0, std::tan(DegToRad(ya)), std::tan(DegToRad(xa)), 1.0, 0.0, 0.0));
  }
  
  void TransformPoint(double& x, double& y, double x0, double y0)
  {
    x = x0 * mXX + y0 * mXY + mTX;
    y = x0 * mYX + y0 * mYY + mTY;
  };
  
  void TransformPoint(double& x, double& y)
  {
    TransformPoint(x, y, x, y);
  };
  
  IMatrix& Transform(const IRECT& before, const IRECT& after)
  {
    const double sx = after.W() / before.W();
    const double sy = after.H() / before.H();
    const double tx = after.L - before.L * sx;
    const double ty = after.T - before.T * sy;
    
    return *this = IMatrix(sx, 0.0, 0.0, sy, tx, ty);
  }
  
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

struct IColorStop
{
  IColorStop()
  : mOffset(0.f)
  {}
  
  IColorStop(IColor color, float offset)
  : mColor(color)
  , mOffset(offset)
  {
    assert(offset >= 0.0 && offset <= 1.0);
  }
  
  IColor mColor;
  float mOffset;
};

struct IPattern
{
  EPatternType mType;
  EPatternExtend mExtend;
  IColorStop mStops[16];
  int mNStops;
  IMatrix mTransform;
  
  IPattern(EPatternType type)
  : mType(type), mExtend(kExtendPad), mNStops(0)
  {}
  
  IPattern(const IColor& color)
  : mType(kSolidPattern), mExtend(kExtendPad), mNStops(1)
  {
    mStops[0] = IColorStop(color, 0.0);
  }
  
  static IPattern CreateLinearGradient(float x1, float y1, float x2, float y2)
  {
    IPattern pattern(kLinearPattern);
    
    // Calculate the affine transform from one line segment to another!
    
    const double xd = x2 - x1;
    const double yd = y2 - y1;
    const double d = sqrt(xd * xd + yd * yd);
    const double a = atan2(xd, yd);
    const double s = sin(a) / d;
    const double c = cos(a) / d;
      
    const double x0 = -(x1 * c - y1 * s);
    const double y0 = -(x1 * s + y1 * c);
    
    pattern.SetTransform(c, s, -s, c, x0, y0);
    
    return pattern;
  }
  
  static IPattern CreateLinearGradient(float x1, float y1, float x2, float y2, std::initializer_list<IColorStop> stops)
  {
    IPattern pattern = CreateLinearGradient(x1, y1, x2, y2);
    
    for (auto& stop : stops)
      pattern.AddStop(stop.mColor, stop.mOffset);
    
    return pattern;
  }
  
  static IPattern CreateRadialGradient(float x1, float y1, float r)
  {
    IPattern pattern(kRadialPattern);
    
    const double s = 1.f / r;

    pattern.SetTransform(s, 0, 0, s, -(x1 * s), -(y1 * s));
    
    return pattern;
  }
  
  static IPattern CreateRadialGradient(float x1, float y1, float r, std::initializer_list<IColorStop> stops)
  {
    IPattern pattern = CreateRadialGradient(x1, y1, r);
    
    for (auto& stop : stops)
      pattern.AddStop(stop.mColor, stop.mOffset);
    
    return pattern;
  }
  
  int NStops() const
  {
    return mNStops;
  }
  
  const IColorStop& GetStop(int idx) const
  {
    return mStops[idx];
  }
  
  void AddStop(IColor color, float offset)
  {
    assert(mType != kSolidPattern && mNStops < 16);
    assert(!mNStops || GetStop(mNStops - 1).mOffset < offset);
    if (mNStops < 16)
      mStops[mNStops++] = IColorStop(color, offset);
  }
  
  void SetTransform(float xx, float yx, float xy, float yy, float x0, float y0)
  {
    mTransform = IMatrix(xx, yx, xy, yy, x0, y0);
  }
  
  void SetTransform(const IMatrix& transform)
  {
    mTransform = transform;
  }
};

/** ILayer is IGraphics's layer abstraction that you use to store temporary APIBitmap to draw with a specific offset to the interface. ILayers take ownership of the underlying bitmaps */

class ILayer
{
  friend IGraphics;
  
public:
  ILayer(APIBitmap* pBitmap, IRECT r)
  : mBitmap(pBitmap)
  , mRECT(r)
  , mInvalid(false)
  {}
  
  ILayer(const ILayer&) = delete;
  ILayer operator =(const ILayer&) = delete;
  
  void Invalidate() { mInvalid = true; }
  const APIBitmap* GetAPIBitmap() const { return mBitmap.get(); }
  IBitmap GetBitmap() const { return IBitmap(mBitmap.get(), 1, false); }
  const IRECT& Bounds() const { return mRECT; }
  
private:
  std::unique_ptr<APIBitmap> mBitmap;
  IRECT mRECT;
  bool mInvalid;
};

/** ILayerPtr is a manged pointer for transferring the ownership of layers */
typedef std::unique_ptr<ILayer> ILayerPtr;

// TODO: static storage needs thread safety mechanism
template <class T>
class StaticStorage
{
public:
  // djb2 hash function (hash * 33 + c) - see http://www.cse.yorku.ca/~oz/hash.html // TODO: can we use C++11 std::hash instead of this?
  uint32_t Hash(const char* str)
  {
    uint32_t hash = 5381;
    int c;

    while ((c = *str++))
    {
      hash = ((hash << 5) + hash) + c;
    }

    return hash;
  }

  struct DataKey
  {
    // N.B. - hashID is not guaranteed to be unique
    uint32_t hashID;
    WDL_String name;
    double scale;
    T* data;
  };

  T* Find(const char* str, double scale = 1.)
  {
    WDL_String cacheName(str);
    cacheName.AppendFormatted((int) strlen(str) + 6, "-%.1fx", scale);
    
    uint32_t hashID = Hash(cacheName.Get());
    
    int i, n = mDatas.GetSize();
    for (i = 0; i < n; ++i)
    {
      DataKey* pKey = mDatas.Get(i);

      // Use the hash id for a quick search and then confirm with the scale and identifier to ensure uniqueness
      if (pKey->hashID == hashID && scale == pKey->scale && !strcmp(str, pKey->name.Get()))
        return pKey->data;
    }
    return nullptr;
  }

  void Add(T* pData, const char* str, double scale = 1. /* scale where 2x = retina, omit if not needed */)
  {
    DataKey* pKey = mDatas.Add(new DataKey);

    WDL_String cacheName(str);
    cacheName.AppendFormatted((int) strlen(str) + 6, "-%.1fx", scale);
    
    pKey->hashID = Hash(cacheName.Get());
    pKey->data = pData;
    pKey->scale = scale;
    pKey->name.Set(str);

    //DBGMSG("adding %s to the static storage at %.1fx the original scale\n", str, scale);
  }

  void Remove(T* pData)
  {
    int i, n = mDatas.GetSize();
    for (i = 0; i < n; ++i)
    {
      if (mDatas.Get(i)->data == pData)
      {
        mDatas.Delete(i, true);
        delete pData;
        break;
      }
    }
  }

  void Clear()
  {
    int i, n = mDatas.GetSize();
    for (i = 0; i < n; ++i)
    {
      // FIXME: - this doesn't work - why not?
      /*
      DataKey* key = mDatas.Get(i);
      T* data = key->data;
      delete data;*/
    }
    mDatas.Empty(true);
  };

  ~StaticStorage()
  {
    Clear();
  }

private:
  WDL_PtrList<DataKey> mDatas;
};

/**@}*/
