#pragma once

#include <cmath>
#include <cassert>
#include <functional>
#include <algorithm>
#include <random>
#include <chrono>

#include "wdlstring.h"
#include "ptrlist.h"
#ifndef OS_WIN
#include "swell.h"
#endif

#include "nanosvg.h"

#include "IPlugPlatform.h"
#include "IGraphicsConstants.h"


class IControl;

typedef std::function<void(IControl*)> IActionFunction;
typedef std::function<void(IControl*)> IAnimationFunction;

typedef std::chrono::high_resolution_clock Time;
typedef std::chrono::time_point<std::chrono::high_resolution_clock> TimePoint;
typedef std::chrono::duration<double, std::chrono::milliseconds::period> Milliseconds;

class LICE_IFont;
/**
 * \defgroup IGraphicsStructs IGraphics::Structs
 * @{
 */

class APIBitmap
{
public:
  APIBitmap(void* pBitmap, int w, int h, int s)
  : mBitmap(pBitmap)
  , mWidth(w)
  , mHeight(h)
  , mScale(s) {}

  APIBitmap()
  : mBitmap(nullptr)
  , mWidth(0)
  , mHeight(0)
  , mScale(0)
  {}

  virtual ~APIBitmap() {}

  void SetBitmap(void* pBitmap, int w, int h, int s)
  {
    assert(((w % s) == 0) && ((h % s) == 0));

    mBitmap = pBitmap;
    mWidth = w;
    mHeight = h;
    mScale = s;
  }

  void* GetBitmap() const { return mBitmap; }
  int GetWidth() const { return mWidth; }
  int GetHeight() const { return mHeight; }
  int GetScale() const { return mScale; }

  private:
  void* mBitmap;
  int mWidth;
  int mHeight;
  int mScale;
};

/** Used to manage bitmap data, independant of draw class/platform.
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
  * @param name Resource name for the bitmap
  */
  IBitmap(APIBitmap* pAPIBitmap, int n, bool framesAreHorizontal, const char* name = "")
    : mAPIBitmap(pAPIBitmap)
    , mW(pAPIBitmap->GetWidth() / pAPIBitmap->GetScale())
    , mH(pAPIBitmap->GetHeight() / pAPIBitmap->GetScale())
    , mN(n)
    , mFramesAreHorizontal(framesAreHorizontal)
    , mResourceName(name, (int) strlen(name))
  {
  }

  IBitmap() : mAPIBitmap(nullptr), mW(0), mH(0), mN(0), mFramesAreHorizontal(false)
  {
  }

  /** @return overall bitmap width */
  inline int W() const { return mW; }

  /** @return overall bitmap height */
  inline int H() const { return mH; }

  /** @return Width of a single frame */
  inline int FW() const { return (mFramesAreHorizontal ? mW / mN : mW); }
  /**
  * @return Height of a single frame
  */
  inline int FH() const { return (mFramesAreHorizontal ? mH : mH / mN); }
  /**
  * @return number of frames
  */
  inline int N() const { return mN; }
  /**
   * @return the scale of the bitmap
   */
  inline int GetScale() const { return mAPIBitmap->GetScale(); }

  /**
  * @return a pointer to the referenced APIBitmap
  */
  inline APIBitmap* GetAPIBitmap() const { return mAPIBitmap; }
  /**
  * @return the raw underlying bitmap
  */
  inline void* GetRawBitmap() const { return mAPIBitmap->GetBitmap(); }
  /**
  * @return whether or not frames are stored horiztonally
  */
  inline bool GetFramesAreHorizontal() const { return mFramesAreHorizontal; }
  /**
  * @return the resource name
  */
  inline const WDL_String& GetResourceName() const { return mResourceName; }

private:

  /** Pointer to the API specific bitmap data */
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
    int A = randomAlpha ? rand() & 0xFF : 255;
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

};

const IColor COLOR_TRANSPARENT(0, 0, 0, 0);
const IColor COLOR_TRANSLUCENT(10, 0, 0, 0);
const IColor COLOR_BLACK(255, 0, 0, 0);
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
#ifndef NDEBUG
const IColor DEFAULT_BGCOLOR = COLOR_TRANSLUCENT;
#else
const IColor DEFAULT_BGCOLOR = COLOR_TRANSPARENT;
#endif
const IColor DEFAULT_FGCOLOR = COLOR_MID_GRAY;
const IColor DEFAULT_PRCOLOR = COLOR_LIGHT_GRAY;

const IColor DEFAULT_FRCOLOR = COLOR_DARK_GRAY;
const IColor DEFAULT_HLCOLOR = COLOR_TRANSLUCENT;
const IColor DEFAULT_SHCOLOR = IColor(60, 0, 0, 0);
const IColor DEFAULT_X1COLOR = COLOR_RED;
const IColor DEFAULT_X2COLOR = COLOR_GREEN;
const IColor DEFAULT_X3COLOR = COLOR_BLUE;

const IColor DEFAULT_TEXTENTRY_BGCOLOR = COLOR_WHITE;
const IColor DEFAULT_TEXTENTRY_FGCOLOR = COLOR_BLACK;

struct IVColorSpec
{
  IColor mBGColor = DEFAULT_BGCOLOR;
  IColor mFGColor = DEFAULT_FGCOLOR;
  IColor mPRColor = DEFAULT_PRCOLOR;
  IColor mFRColor = DEFAULT_FRCOLOR;
  IColor mHLColor = DEFAULT_HLCOLOR;
  IColor mSHColor = DEFAULT_SHCOLOR;
  IColor mX1Color = DEFAULT_X1COLOR;
  IColor mX2Color = DEFAULT_X2COLOR;
  IColor mX3Color = DEFAULT_X3COLOR;

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

struct IColorStop
{
  IColorStop()
  : mOffset(0.0)
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
  WDL_TypedBuf<IColorStop> mStops;
  float mTransform[6];

  IPattern(const IColor& color) : mExtend(kExtendRepeat)
  {
    mType = kSolidPattern;
    mStops.Add(IColorStop(color, 0.0));
    SetTransform(1.f, 0.f, 0.f, 1.f, 0.f, 0.f);
  }

  IPattern(EPatternType type) : mExtend(kExtendNone)
  {
    mType = type;
    SetTransform(1.f, 0.f, 0.f, 1.f, 0.f, 0.f);
  }

  IPattern(float x1, float y1, float x2, float y2) : mExtend(kExtendNone)
  {
    mType = kLinearPattern;

    // Calculate the affine transform from one line segment to another!

    const float xd = x2 - x1;
    const float yd = y2 - y1;
    const float size = sqrtf(xd * xd + yd * yd);
    const float angle = -(atan2(yd, xd));
    const float sinV = sinf(angle) / size;
    const float cosV = cosf(angle) / size;

    const float xx = cosV;
    const float xy = -sinV;
    const float yx = sinV;
    const float yy = cosV;
    const float x0 = -(x1 * xx + y1 * xy);
    const float y0 = -(x1 * yx + y1 * yy);

    SetTransform(xx, yx, xy, yy, x0, y0);
  }

  IPattern(float x1, float y1, float r) : mExtend(kExtendNone)
  {
    mType = kRadialPattern;

    const float xx = 1.0 / r;
    const float yy = 1.0 / r;
    const float x0 = -(x1 * xx);
    const float y0 = -(y1 * yy);

    SetTransform(xx, 0, 0, yy, x0, y0);
  }

  int NStops() const
  {
    return mStops.GetSize();
  }

  const IColorStop& GetStop(int idx) const
  {
    return *(mStops.Get() + idx);
  }

  void AddStop(IColor color, float offset)
  {
    assert(mType != kSolidPattern);
    assert(!NStops() || GetStop(NStops() - 1).mOffset < offset);
    mStops.Add(IColorStop(color, offset));
  }

  void SetTransform(float xx, float yx, float xy, float yy, float x0, float y0)
  {
    mTransform[0] = xx;
    mTransform[1] = yx;
    mTransform[2] = xy;
    mTransform[3] = yy;
    mTransform[4] = x0;
    mTransform[5] = y0;
  }
};

/** Used to manage font and text/text entry style, independant of draw class/platform.*/
struct IText
{
  enum EStyle { kStyleNormal, kStyleBold, kStyleItalic } mStyle;
  enum EAlign { kAlignNear, kAlignCenter, kAlignFar } mAlign;
  enum EQuality { kQualityDefault, kQualityNonAntiAliased, kQualityAntiAliased, kQualityClearType } mQuality = kQualityDefault;

  IText(const IColor& color = DEFAULT_FGCOLOR,
        int size = DEFAULT_TEXT_SIZE,
        const char* font = nullptr,
        EStyle style = kStyleNormal,
        EAlign align = kAlignCenter,
        int orientation = 0,
        EQuality quality = kQualityDefault,
        const IColor& TEBGColor = DEFAULT_TEXTENTRY_BGCOLOR,
        const IColor& TEFGColor = DEFAULT_TEXTENTRY_FGCOLOR)
    : mSize(size)
    , mFGColor(color)
    , mStyle(style)
    , mAlign(align)
    , mOrientation(orientation)
    , mQuality(quality)
    , mTextEntryBGColor(TEBGColor)
    , mTextEntryFGColor(TEFGColor)
  {
    strcpy(mFont, (font ? font : DEFAULT_FONT));
  }

  char mFont[FONT_LEN];
  int mSize;
  IColor mFGColor;
  IColor mTextEntryBGColor;
  IColor mTextEntryFGColor;
  int mOrientation = 0; // Degrees ccwise from normal.
  mutable LICE_IFont* mCached = nullptr;
  mutable double mCachedScale = 1.0;
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
  
  IRECT(float x, float y, IBitmap& bitmap)
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

  inline void Constrain(float& x, float& y)
  {
    if (x < L) x = L;
    else if (x > R) x = R;

    if (y < T) y = T;
    else if (y > B) y = B;
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

  inline IRECT GetGridCell(int cellIndex, int nRows, int nColumns, EDirection = kHorizontal) const
  {
    assert(cellIndex <= nRows * nColumns);

    int cell = 0;
    for(int row = 0; row<nRows; row++)
    {
      for(int column = 0; column<nColumns; column++)
      {
        if(cell == cellIndex)
        {
          const IRECT vrect = SubRectVertical(nRows, row);
          return vrect.SubRectHorizontal(nColumns, column);
        }

        cell++;
      }
    }

    return *this;
  }
  
  bool IsPixelAligned() const
  {
    return !(L - floor(L) && T - floor(T) && R - floor(R) && B - floor(B));
  }
  
  inline IRECT Pad(float padding)
  {
    L -= padding;
    T -= padding;
    R += padding;
    B += padding;
  }
  
  inline IRECT Pad(float padL, float padT, float padR, float padB)
  {
    L += padL;
    T += padT;
    R += padR;
    B += padB;
  }
  
  inline IRECT HPad(float padding)
  {
    L -= padding;
    R += padding;
  }
  
  inline IRECT VPad(float padding)
  {
    T -= padding;
    B += padding;
  }
  
  inline IRECT MidHPad(float padding)
  {
    const float mw = MW();
    L = mw - padding;
    R = mw + padding;
  }
  
  inline IRECT MidVPad(float padding)
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
  
  static void LinearInterpolateBetween(const IRECT& start, const IRECT& dest, IRECT& result, double progress)
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

  void GetRandomPoint(double& x, double& y) const
  {
    std::random_device rd;
    std::mt19937 gen(rd()); // TODO: most sensible RNG?
    std::uniform_real_distribution<> dist(0., 1.);
    x = L + dist(gen) * W();
    y = T + dist(gen) * H();
  }

  IRECT GetRandomSubRect() const
  {
    double l, t, r, b;
    GetRandomPoint(l, t);
    IRECT tmp = IRECT(l, t, R, B);
    tmp.GetRandomPoint(r, b);

    return IRECT(l, t, r, b);
  }

  void Shift(float l, float t, float r, float b)
  {
    L += l;
    T += t;
    R += r;
    B += b;
  }
  
  void Shift(float x, float y = 0.f)
  {
    L += x;
    T += y;
    R += x;
    B += y;
  }
  
  IRECT GetShifted(float x, float y = 0.f) const
  {
    return IRECT(L + x, T + y, R + x, B + y);
  }
  
  IRECT GetShifted(float l, float t, float r, float b) const
  {
    return IRECT(L + l, T + t, R + r, B + b);
  }
  
  void ScaleBounds(float scale)
  {
    L = std::floor(L * scale);
    T = std::floor(T * scale);
    R = std::ceil(R * scale);
    B = std::ceil(B * scale);
  }

  IRECT GetFlipped(int graphicsHeight) const
  {
    return IRECT(L, graphicsHeight - T, R, graphicsHeight - B);
  }

  IRECT GetCentredInside(IRECT sr)
  {
    IRECT r;
    r.L = MW() - sr.W() / 2.;
    r.T = MH() - sr.H() / 2.;
    r.R = r.L + sr.W();
    r.B = r.T + sr.H();

    return r;
  }
  
  IRECT GetCentredInside(float w, float h = 0.f)
  {
    assert(w > 0.f);
    
    if(h <= 0.f)
      h = w;
    
    IRECT r;
    r.L = MW() - w / 2.;
    r.T = MH() - h / 2.;
    r.R = r.L + w;
    r.B = r.T + h;
    
    return r;
  }

  IRECT GetCentredInside(IBitmap bitmap)
  {
    IRECT r;
    r.L = MW() - bitmap.FW() / 2.;
    r.T = MH() - bitmap.FH() / 2.;
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
};

struct IMouseInfo
{
    float x, y;
    IMouseMod ms;
};

// TODO: static storage needs thread safety mechanism
template <class T>
class StaticStorage
{
public:

  // djb2 hash function (hash * 33 + c) - see http://www.cse.yorku.ca/~oz/hash.html
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
      DataKey* key = mDatas.Get(i);

      // Use the hash id for a quick search and then confirm with the scale and identifier to ensure uniqueness
      if (key->hashID == hashID && scale == key->scale && !strcmp(str, key->name.Get()))
        return key->data;
    }
    return nullptr;
  }

  void Add(T* data, const char* str, double scale = 1. /* scale where 2x = retina, omit if not needed */)
  {
    DataKey* key = mDatas.Add(new DataKey);

    WDL_String cacheName(str);
    cacheName.AppendFormatted((int) strlen(str) + 6, "-%.1fx", scale);
    
    key->hashID = Hash(cacheName.Get());
    key->data = data;
    key->scale = scale;
    key->name.Set(str);

    DBGMSG("adding %s to the static storage at %.1fx the original scale\n", str, scale);
  }

  void Remove(T* data)
  {
    int i, n = mDatas.GetSize();
    for (i = 0; i < n; ++i)
    {
      if (mDatas.Get(i)->data == data)
      {
        mDatas.Delete(i, true);
        delete data;
        break;
      }
    }
  }

  void Clear()
  {
    int i, n = mDatas.GetSize();
    for (i = 0; i < n; ++i)
    {
      // FIX - this doesn't work - why not?
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
