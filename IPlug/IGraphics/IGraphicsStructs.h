#pragma once

#include <algorithm>
#include <cmath>

#include "wdlstring.h"
#include "ptrlist.h"
#ifdef OS_OSX
#include "swell.h"
#endif

#include "nanosvg.h"

#include "IPlugOSDetect.h"
#include "Log.h"

class LICE_IFont;

enum EFileAction { kFileOpen, kFileSave };

enum EDirection { kVertical, kHorizontal };

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
  inline int frameWidth() const { return (mFramesAreHorizontal ? W / N : W); }
  /**
   * @return Height of a single frame
   */
  inline int frameHeight() const { return (mFramesAreHorizontal ? H : H / N); }
};

struct ISVG
{
  NSVGimage* mImage = nullptr;

  ISVG(NSVGimage* image)
  {
    mImage = image;
    assert(mImage != nullptr);
  }

  int W()
  {
    if (mImage)
      return mImage->width;
    else
      return 0;
  }

  int H()
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
  void Randomise(int alpha = 255) { A = alpha, R = std::rand() % 255, G = std::rand() % 255, B = std::rand() % 255; }
  IColor addContrast(double c)
  {
    const int mod = int(c * 255.);
    IColor n = *this;
    n.R   = std::min(n.R += mod, 255);
    n.G = std::min(n.G += mod, 255);
    n.B  = std::min(n.B += mod, 255);
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

const IColor DEFAULT_TEXT_COLOR = COLOR_BLACK;
const IColor DEFAULT_TEXT_ENTRY_BGCOLOR = COLOR_WHITE;
const IColor DEFAULT_TEXT_ENTRY_FGCOLOR = COLOR_BLACK;

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
  int L, T, R, B;

  IRECT() { L = T = R = B = 0; }
  IRECT(int l, int t, int r, int b) : L(l), R(r), T(t), B(b) {}
  
  IRECT(int x, int y, IBitmap& bitmap)
  {
    L = x;
    T = y;
    R = L + bitmap.frameWidth();
    B = T + bitmap.frameHeight();
  }

  bool Empty() const
  {
    return (L == 0 && T == 0 && R == 0 && B == 0);
  }

  void Clear()
  {
    L = T = R = B = 0;
  }

  bool operator==(const IRECT& rhs) const
  {
    return (L == rhs.L && T == rhs.T && R == rhs.R && B == rhs.B);
  }

  bool operator!=(const IRECT& rhs) const
  {
    return !(*this == rhs);
  }

  inline int W() const { return R - L; }
  inline int H() const { return B - T; }
  inline float MW() const { return 0.5f * (float) (L + R); }
  inline float MH() const { return 0.5f * (float) (T + B); }

  inline IRECT Union(const IRECT& pRHS)
  {
    if (Empty()) { return pRHS; }
    if (pRHS.Empty()) { return *this; }
    return IRECT(std::min(L, pRHS.L), std::min(T, pRHS.T), std::max(R, pRHS.R), std::max(B, pRHS.B));
  }

  inline IRECT Intersect(const IRECT& pRHS) const
  {
    if (Intersects(pRHS))
    {
      return IRECT(std::max(L, pRHS.L), std::max(T, pRHS.T), std::min(R, pRHS.R), std::min(B, pRHS.B));
    }
    return IRECT();
  }

  inline bool Intersects(const IRECT& pRHS) const
  {
    return (!Empty() && !pRHS.Empty() && R >= pRHS.L && L < pRHS.R && B >= pRHS.T && T < pRHS.B);
  }

  inline bool Contains(const IRECT& pRHS) const
  {
    return (!Empty() && !pRHS.Empty() && pRHS.L >= L && pRHS.R <= R && pRHS.T >= T && pRHS.B <= B);
  }

  inline bool Contains(int x, int y) const
  {
    return (!Empty() && x >= L && x < R && y >= T && y < B);
  }

  inline void Constrain(int* x, int* y)
  {
    if (*x < L)
    {
      *x = L;
    }
    else if (*x > R)
    {
      *x = R;
    }

    if (*y < T)
    {
      *y = T;
    }
    else if (*y > B)
    {
      *y = B;
    }
  }

  inline IRECT SubRectVertical(int numSlices, int sliceIdx)
  {
    float heightOfSubRect = (float(H()) / numSlices);
    int t = int(heightOfSubRect) * sliceIdx;

    return IRECT(L, T + t, R, T + t + (int) heightOfSubRect);
  }

  inline IRECT SubRectHorizontal(int numSlices, int sliceIdx)
  {
    float widthOfSubRect = (float(W()) / numSlices);
    int l = int(widthOfSubRect) * sliceIdx;

    return IRECT(L + l, T, L + l + (int) widthOfSubRect, B);
  }
  
  inline IRECT GetPadded(int padding)
  {
    return IRECT(L-padding, T-padding, R+padding, B+padding);
  }
  
  inline IRECT GetPadded(int padL, int padT, int padR, int padB)
  {
    return IRECT(L+padL, T+padT, R+padR, B+padB);
  }
  
  inline IRECT GetHPadded(int padding)
  {
    return IRECT(L-padding, T, R+padding, B);
  }

  inline IRECT GetVPadded(int padding)
  {
    return IRECT(L, T-padding, R, B+padding);
  }
  
  void Clank(const IRECT& pRHS)
  {
    if (L < pRHS.L)
    {
      R = std::min(pRHS.R - 1, R + pRHS.L - L);
      L = pRHS.L;
    }
    if (T < pRHS.T)
    {
      B = std::min(pRHS.B - 1, B + pRHS.T - T);
      T = pRHS.T;
    }
    if (R >= pRHS.R)
    {
      L = std::max(pRHS.L, L - (R - pRHS.R + 1));
      R = pRHS.R - 1;
    }
    if (B >= pRHS.B)
    {
      T = std::max(pRHS.T, T - (B - pRHS.B + 1));
      B = pRHS.B - 1;
    }
  }
  
  void Scale(double scale)
  {
    L = (int) std::floor(0.5 + (L * scale));
    T = (int) std::floor(0.5 + (T * scale));
    R = (int) std::floor(0.5 + (R * scale));
    B = (int) std::floor(0.5 + (B * scale));
  }
  
  IRECT GetScaled(double scale) const
  {
    IRECT r = *this;
    r.Scale(scale);
    return r;
  }

  void ScaleBounds(double scale)
  {
    L = (int)std::floor(L * scale);
    T = (int)std::floor(T * scale);
    R = (int)std::ceil(R * scale);
    B = (int)std::ceil(B * scale);
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
