#pragma once

#include <cmath>
#include <cassert>
#include <functional>
#include <algorithm>

#include "wdlstring.h"
#include "ptrlist.h"
#ifdef OS_MAC
#include "swell.h"
#endif

#include "nanosvg.h"

#include "IPlugPlatform.h"
#include "IGraphicsConstants.h"


class IControl;

typedef std::function<void(IControl*)> IActionFunction;

class LICE_IFont;
/**
 * \defgroup IGraphicsStructs IGraphics::Structs
 * @{
 */

/** Used to manage bitmap data, independant of draw class/platform.
 * An IBitmap's width and height are always in relation to a 1:1 (low dpi) screen. Any scaling happens at the drawing stage. */
struct IBitmap
{
  /** Pointer to the raw bitmap data */
  void* mData;
  /** Bitmap width (in pixels) */
  int W;
  /** Bitmap height (in pixels) */
  int H;
  /** Number of frames (for stacked bitmaps) */
  int N;
  /** \c True if the frames are positioned horizontally */
  bool mFramesAreHorizontal;
  /** Maximum scaling allowed for the bitmap (typically 1) */
  /** @todo Subject to change */
  double mSourceScale;
  /** Resource path/name for the bitmap */
  WDL_String mResourceName;
  /** Creates a new IBitmap object
   * @param pData Pointer to the raw bitmap data
   * @param w Bitmap width (in pixels)
   * @param h Bitmap height (in pixels)
   * @param n Number of frames (for multibitmaps)
   * @param framesAreHorizontal \c True if the frames are positioned horizontally
   * @param sourceScale Scaling of the original bitmap (typically 1, 2 would be for a @2x hi dpi bitmap) @todo Subject to change
   * @param name Resource name for the bitmap
   */
  IBitmap(void* pData = nullptr, int w = 0, int h = 0, int n = 1, bool framesAreHorizontal = false, double sourceScale = 1., const char* name = "")
    : mData(pData)
    , W(w)
    , H(h)
    , N(n)
    , mFramesAreHorizontal(framesAreHorizontal)
    , mSourceScale(sourceScale)
    , mResourceName(name, (int) strlen(name))
  {
  }

  /**
   * @return Width of a single frame
  */
  inline int FW() const { return (mFramesAreHorizontal ? W / N : W); }
  /**
   * @return Height of a single frame
   */
  inline int FH() const { return (mFramesAreHorizontal ? H : H / N); }
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
  EBlendType mMethod;
  float mWeight;

  /** Creates a new IBlend
   * @param type Blend type (defaults to none)
   * @todo IBlend::weight needs documentation
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

const IColor DEFAULT_BGCOLOR = COLOR_WHITE;
const IColor DEFAULT_FGCOLOR = COLOR_BLACK;

/** Used to manage font and text/text entry style, independant of draw class/platform.*/
struct IText
{
  char mFont[FONT_LEN];
  int mSize = DEFAULT_TEXT_SIZE;
  IColor mColor;
  IColor mTextEntryBGColor = DEFAULT_BGCOLOR;
  IColor mTextEntryFGColor = DEFAULT_FGCOLOR;
  enum EStyle { kStyleNormal, kStyleBold, kStyleItalic } mStyle = kStyleNormal;
  enum EAlign { kAlignNear, kAlignCenter, kAlignFar } mAlign = kAlignCenter;
  int mOrientation = 0; // Degrees ccwise from normal.
  enum EQuality { kQualityDefault, kQualityNonAntiAliased, kQualityAntiAliased, kQualityClearType } mQuality = kQualityDefault;
  mutable LICE_IFont* mCached = nullptr;
  mutable double mCachedScale = 1.0;

  IText(int size = DEFAULT_TEXT_SIZE,
        const IColor& color = DEFAULT_FGCOLOR,
        const char* font = nullptr,
        EStyle style = kStyleNormal,
        EAlign align = kAlignCenter,
        int orientation = 0,
        EQuality quality = kQualityDefault,
        const IColor& teBGColor = DEFAULT_BGCOLOR,
        const IColor& teFGColor = DEFAULT_FGCOLOR)
    : mSize(size)
    , mColor(color)
    , mStyle(style)
    , mAlign(align)
    , mOrientation(orientation)
    , mQuality(quality)
    , mTextEntryBGColor(teBGColor)
    , mTextEntryFGColor(teFGColor)
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
    assert(cellIndex < nRows * nColumns);
    
    int cell = 0;
    for(int column = 0; column<nColumns; column++)
    {
      for(int row = 0; row<nRows; row++)
      {
        if(cell == cellIndex)
        {
          const IRECT hrect = SubRectHorizontal(nRows, row);
          return hrect.SubRectVertical(nColumns, column);
        }
        
        cell++;
      }
    }
    
    return *this;
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
  
  unsigned long hash(const char* str)
  {
    unsigned long hash = 5381; // TODO: CHECK THIS
    int c;
    
    while ((c = *str++))
    {
      hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    
    return hash;
  }
  
  struct DataKey
  {
    unsigned long id;
    WDL_String path;
    double scale;
    T* data;
  };
  
  WDL_PtrList<DataKey> mDatas;
  
  T* Find(const char* str, double scale = 1.)
  {
    WDL_String cacheName(str);
    cacheName.AppendFormatted((int) strlen(str) + 6, "-%.1fx", scale);
    
    unsigned long id = hash(cacheName.Get());
    
    int i, n = mDatas.GetSize();
    for (i = 0; i < n; ++i)
    {
      DataKey* key = mDatas.Get(i);
      
      if (key->id == id) {
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
    
    key->id = hash(cacheName.Get());
    key->data = data;
    key->scale = scale;
    key->path.Set(str);
    
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
  
  ~StaticStorage()
  {
    int i, n = mDatas.GetSize();
    for (i = 0; i < n; ++i)
    {
      delete(mDatas.Get(i)->data);
    }
    mDatas.Empty(true);
  }
};

/**@}*/
