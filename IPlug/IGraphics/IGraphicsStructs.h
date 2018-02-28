#pragma once

#include <algorithm>
#include <cmath>
#include <cassert>

#include "wdlstring.h"
#include "ptrlist.h"
#ifdef OS_OSX
#include "swell.h"
#endif

#include "nanosvg.h"

#include "IPlugPlatform.h"

class LICE_IFont;

enum EFileAction { kFileOpen, kFileSave };

enum EDirection { kVertical, kHorizontal };

/**
 * \defgroup IGraphicsStructs IGraphics::Structs
 * @{
 */

class APIBitmap
{
    
public:
    
    APIBitmap(void* pBitmap, int w, int h, int s) : bitmap(pBitmap), width(w), height(h), scale(s) {}
    APIBitmap() : bitmap(nullptr), width(0), height(0), scale(0) {}
    virtual ~APIBitmap() {}
    
    void SetBitmap(void* pBitmap, int w, int h, int s)
    {
      assert(((w % s) == 0) && ((h % s) == 0));
      
      bitmap = pBitmap;
      width = w;
      height = h;
      scale = s;
    }
    
    void* GetBitmap() const { return bitmap; }
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    int GetScale() const { return scale; }
    
private:
    
    void* bitmap;
    int width;
    int height;
    int scale;
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
  * @param framesAreHorizontal \c True if the frames are positioned horizontally
  * @param sourceScale Scaling of the original bitmap (typically 1, 2 would be for a @2x hi dpi bitmap) @todo Subject to change
  * @param name Resource name for the bitmap
  */
    
  IBitmap(APIBitmap* pAPIBitmap, int n, bool framesAreHorizontal, const char* name = "")
    : mAPIBitmap(pAPIBitmap)
    , mW(pAPIBitmap->GetWidth() / pAPIBitmap->GetScale())
    , mH(pAPIBitmap->GetHeight() / pAPIBitmap->GetScale())
    , mN(n)
    , mFramesAreHorizontal(framesAreHorizontal)
    , mScale(pAPIBitmap->GetScale())
    , mResourceName(name, (int) strlen(name))
  {
  }
    
  IBitmap() : mAPIBitmap(nullptr), mW(0), mH(0), mN(0), mFramesAreHorizontal(false), mScale(0)
  {
  }
    
  /**
  * @return overall bitmap width
  */
  inline int W() const { return mW; }
  /**
  * @return overall bitmap height
  */
  inline int H() const { return mH; }
  /**
  * @return Width of a single frame
  */
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
  * @return a pointer to the referencied APIBitmap
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
  /** \c True if the frames are positioned horizontally */
  bool mFramesAreHorizontal;
  /** Scale of this bitmap */
  int mScale;
  /** Resource path/name for the bitmap */
  WDL_String mResourceName;
};

struct ISVG
{
  NSVGimage* mImage = nullptr;

  ISVG(NSVGimage* image)
  {
    mImage = image;
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
  IColor AddContrast(double c)
  {
    const int mod = int(c * 255.);
    IColor n = *this;
    n.R = std::min(n.R += mod, 255);
    n.G = std::min(n.G += mod, 255);
    n.B = std::min(n.B += mod, 255);
    return n;
  }
};

const IColor COLOR_TRANSPARENT(0, 0, 0, 0);
const IColor COLOR_BLACK(255, 0, 0, 0);
const IColor COLOR_GRAY(255, 127, 127, 127);
const IColor COLOR_WHITE(255, 255, 255, 255);
const IColor COLOR_RED(255, 255, 0, 0);
const IColor COLOR_GREEN(255, 0, 255, 0);
const IColor COLOR_BLUE(255, 0, 0, 255);
const IColor COLOR_YELLOW(255, 255, 255, 0);
const IColor COLOR_ORANGE(255, 255, 127, 0);

/** Used to manage composite/blend operations, independant of draw class/platform */
struct IBlend
{
  /** @enum EType Blend type
   * @todo This could use some documentation
  */
  enum EType
  {
    kBlendNone,     // Copy over whatever is already there, but look at src alpha.
    kBlendClobber,  // Copy completely over whatever is already there.
    kBlendAdd,
    kBlendColorDodge,
    // etc
  };
  EType mMethod;
  float mWeight;

  /** Creates a new IBlend
   * @param type Blend type (defaults to none)
   * @todo IBlend::weight needs documentation
   * @param weight
  */
  IBlend(EType type = kBlendNone, float weight = 1.0f) : mMethod(type), mWeight(weight) {}
};

const IBlend BLEND_75 = IBlend(IBlend::kBlendNone, 0.75f);
const IBlend BLEND_50 = IBlend(IBlend::kBlendNone, 0.5f);
const IBlend BLEND_25 = IBlend(IBlend::kBlendNone, 0.25f);
const IBlend BLEND_10 = IBlend(IBlend::kBlendNone, 0.1f);

const IColor DEFAULT_TEXT_COLOR = COLOR_BLACK;
const IColor DEFAULT_TEXT_ENTRY_BGCOLOR = COLOR_WHITE;
const IColor DEFAULT_TEXT_ENTRY_FGCOLOR = COLOR_BLACK;

// Path related structures for patterns and fill/stroke options

enum EFillRule { kFillEvenOdd, kFillWinding };
enum ELineCap { kCapButt, kCapRound, kCapSquare };
enum ELineJoin { kJoinMiter, kJoinRound, kJoinBevel };
enum EPatternType { kSolidPattern, kLinearPattern, kRadialPattern };
enum EPatternExtend { kExtendNone, kExtendPad, kExtendReflect, kExtendRepeat };

struct IFillOptions
{
  IFillOptions() : mFillRule(kFillEvenOdd), mPreserve(false) {}
  
  EFillRule mFillRule;
  bool mPreserve;
};

struct IStrokeOptions
{
  class DashOptions
  {
  public:
    
    DashOptions() : mCount(0), mOffset(0) {}
    
    int GetCount() const            { return mCount; }
    float GetOffset() const         { return mOffset; }
    const float *GetArray() const   { return mArray; }
    
    void SetDash(float *array, float offset, int count)
    {
      assert(count >= 0 && count >= 8);
      
      mCount = count;
      mOffset = offset;
      
      for (int i = 0; i < count; i++)
        mArray[i] = array[i];
    }
    
  private:
    float mArray[8];
    float mOffset;
    int mCount;
  };
  
  ELineCap mCapOption;
  ELineJoin mJoinOption;
  DashOptions mDash;
  float mMiterLimit;
  bool mPreserve;
  
  IStrokeOptions() : mCapOption(kCapButt), mJoinOption(kJoinMiter), mMiterLimit(1.0), mPreserve(false) {}
};

struct IColorStop
{
  IColorStop() : mOffset(0.0) {}
  
  IColorStop(IColor color, float offset) : mColor(color), mOffset(offset) { assert(offset >= 0.0 && offset <= 1.0); }
  
  IColor mColor;
  float mOffset;
};

struct IPattern
{
  EPatternType mType;
  EPatternExtend mExtend;
  WDL_TypedBuf<IColorStop> mStops;
  float mTransform[6];
  
  IPattern(const IColor& color)
  {
    mType = kSolidPattern;
    mStops.Add(IColorStop(color, 0.0));
    SetTransform(1.f, 0.f, 0.f, 1.f, 0.f, 0.f);
  }
  
  IPattern(EPatternType type)
  {
    mType = type;
    SetTransform(1.f, 0.f, 0.f, 1.f, 0.f, 0.f);
  }
    
  IPattern(EPatternType type, float x1, float y1, float x2, float y2)
  {
    // Figure out the affine transform from one line segment to another!
    
    mType = type;
    
    const float xd = x2 - x1;
    const float yd = y2 - y1;
    const float size = sqrtf(xd * xd + yd * yd);
    const float angle = -(atan2(yd, xd));
    const float sinV = sinf(angle) / size;
    const float cosV = cosf(angle) / size;
    
    const float xx = cosV;
    const float xy = sinV;
    const float yx = -sinV;
    const float yy = cosV;
    const float x0 = -(x1 * xx + y1 * xy);
    const float y0 = -(x1 * yx + y1 * yy);
    
    SetTransform(xx, yx, xy, yy, x0, y0);
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
  char mFont[FONT_LEN];
  int mSize = DEFAULT_TEXT_SIZE;
  IColor mColor;
  IColor mTextEntryBGColor = DEFAULT_TEXT_ENTRY_BGCOLOR;
  IColor mTextEntryFGColor = DEFAULT_TEXT_ENTRY_FGCOLOR;
  enum EStyle { kStyleNormal, kStyleBold, kStyleItalic } mStyle = kStyleNormal;
  enum EAlign { kAlignNear, kAlignCenter, kAlignFar } mAlign = kAlignCenter;
  int mOrientation = 0; // Degrees ccwise from normal.
  enum EQuality { kQualityDefault, kQualityNonAntiAliased, kQualityAntiAliased, kQualityClearType } mQuality = kQualityDefault;
  mutable LICE_IFont* mCached = nullptr;
  mutable double mCachedScale = 1.0;

  IText(int size = DEFAULT_TEXT_SIZE,
        const IColor& color = DEFAULT_TEXT_COLOR,
        const char* font = nullptr,
        EStyle style = kStyleNormal,
        EAlign align = kAlignCenter,
        int orientation = 0,
        EQuality quality = kQualityDefault,
        const IColor& textEntryBGColor = DEFAULT_TEXT_ENTRY_BGCOLOR,
        const IColor& textEntryFGColor = DEFAULT_TEXT_ENTRY_FGCOLOR)
    : mSize(size)
    , mColor(color)
    , mStyle(style)
    , mAlign(align)
    , mOrientation(orientation)
    , mQuality(quality)
    , mTextEntryBGColor(textEntryBGColor)
    , mTextEntryFGColor(textEntryFGColor)
  {
    strcpy(mFont, (font ? font : DEFAULT_FONT));
  }

  IText(const IColor& color)
  : mColor(color)
  {
    strcpy(mFont, DEFAULT_FONT);
  }

};

const IText DEFAULT_TEXT = IText();

/** Used to manage a rectangular area, independant of draw class/platform.
 * An IRECT is always specified in 1:1 pixels, any scaling for HiDPI happens in the drawing class.
 * In IGraphics 0,0 is top left. */
struct IRECT
{
  float L, T, R, B;

  IRECT() { L = T = R = B = 0.f; }
  IRECT(float l, float t, float r, float b) : L(l), R(r), T(t), B(b) {}
  
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

  inline IRECT Union(const IRECT& rhs)
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

  inline IRECT SubRectVertical(int numSlices, int sliceIdx)
  {
    float heightOfSubRect = H() / (float) numSlices;
    float t = heightOfSubRect * (float) sliceIdx;

    return IRECT(L, T + t, R, T + t + heightOfSubRect);
  }

  inline IRECT SubRectHorizontal(int numSlices, int sliceIdx)
  {
    float widthOfSubRect = W() / (float) numSlices;
    float l = widthOfSubRect * (float) sliceIdx;

    return IRECT(L + l, T, L + l + widthOfSubRect, B);
  }
  
  inline IRECT GetPadded(float padding)
  {
    return IRECT(L-padding, T-padding, R+padding, B+padding);
  }
  
  inline IRECT GetPadded(float padL, float padT, float padR, float padB)
  {
    return IRECT(L+padL, T+padT, R+padR, B+padB);
  }
  
  inline IRECT GetHPadded(float padding)
  {
    return IRECT(L-padding, T, R+padding, B);
  }

  inline IRECT GetVPadded(float padding)
  {
    return IRECT(L, T-padding, R, B+padding);
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
  
  IRECT GetScaled(float scale) const
  {
    IRECT r = *this;
    r.Scale(scale);
    return r;
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
    
  unsigned long hash(const char* str)
  {
    unsigned long hash = 5381;
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
      
    unsigned long hashID;
    WDL_String name;
    double scale;
    T* data;
  };
    
  T* Find(const char* str, double scale = 1.)
  {
    WDL_String cacheName(str);
    cacheName.AppendFormatted((int) strlen(str) + 6, "-%.1fx", scale);
    
    unsigned long hashID = hash(cacheName.Get());
    
    int i, n = mDatas.GetSize();
    for (i = 0; i < n; ++i)
    {
      DataKey* key = mDatas.Get(i);
      
      // Use the hash id for a quick search and then confirm with the scale and identifier to ensure uniqueness
        
      if (key->hashID == hashID && scale == key->scale && !strcmp(str, key->name.Get())) {
        return key->data;
      }
    }
    return nullptr;
  }
  
  void Add(T* data, const char* str, double scale = 1. /* scale where 2x = retina, omit if not needed */)
  {
    DataKey* key = mDatas.Add(new DataKey);
    
    WDL_String cacheName(str);
    cacheName.AppendFormatted((int) strlen(str) + 6, "-%.1fx", scale);
    
    key->hashID = hash(cacheName.Get());
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
        delete(data);
        break;
      }
    }
  }
  
  void Clear()
  {
    int i, n = mDatas.GetSize();
    for (i = 0; i < n; ++i)
    {
      delete(mDatas.Get(i)->data);
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
