#include "IGraphicsLice.h"
#include "IControl.h"
#include <cmath>
#include "Log.h"

signed int GetSystemVersion();

static StaticStorage<LICE_IBitmap> s_bitmapCache;

//todo: replace this with templated version
class FontStorage
{
public:
  
  struct FontKey
  {
    int size, orientation;
    IText::EStyle style;
    char face[FONT_LEN];
    LICE_IFont* font;
  };
  
  WDL_PtrList<FontKey> m_fonts;
  
  LICE_IFont* Find(const IText& text)
  {
    int i = 0, n = m_fonts.GetSize();
    for (i = 0; i < n; ++i)
    {
      FontKey* key = m_fonts.Get(i);
      if (key->size == text.mSize && key->orientation == text.mOrientation && key->style == text.mStyle && !strcmp(key->face, text.mFont)) return key->font;
    }
    return 0;
  }
  
  void Add(LICE_IFont* font, const IText& text)
  {
    FontKey* key = m_fonts.Add(new FontKey);
    key->size = text.mSize;
    key->orientation = text.mOrientation;
    key->style = text.mStyle;
    strcpy(key->face, text.mFont);
    key->font = font;
  }
  
  ~FontStorage()
  {
    int i, n = m_fonts.GetSize();
    for (i = 0; i < n; ++i)
    {
      delete(m_fonts.Get(i)->font);
    }
    m_fonts.Empty(true);
  }
};

static FontStorage s_fontCache;

inline LICE_pixel LiceColor(const IColor& color)
{
  return LICE_RGBA(color.R, color.G, color.B, color.A);
}

inline float LiceWeight(const IChannelBlend* pBlend)
{
  return (pBlend ? pBlend->mWeight : 1.0f);
}

inline int LiceBlendMode(const IChannelBlend* pBlend)
{
  if (!pBlend)
  {
    return LICE_BLIT_MODE_COPY | LICE_BLIT_USE_ALPHA;
  }
  switch (pBlend->mMethod)
  {
    case IChannelBlend::kBlendClobber:
    {
      return LICE_BLIT_MODE_COPY;
    }
    case IChannelBlend::kBlendAdd:
    {
      return LICE_BLIT_MODE_ADD | LICE_BLIT_USE_ALPHA;
    }
    case IChannelBlend::kBlendColorDodge:
    {
      return LICE_BLIT_MODE_DODGE | LICE_BLIT_USE_ALPHA;
    }
    case IChannelBlend::kBlendNone:
    default:
    {
      return LICE_BLIT_MODE_COPY | LICE_BLIT_USE_ALPHA;
    }
  }
}

#pragma mark -

IGraphicsLice::IGraphicsLice(IPlugBaseGraphics& plug, int w, int h, int fps)
: IGraphics(plug, w, h, fps)
, mDrawBitmap(nullptr)
, mTmpBitmap(nullptr)
#ifdef OS_OSX
, mColorSpace(nullptr)
#endif
{}

IGraphicsLice::~IGraphicsLice() 
{
#ifdef OS_OSX
  if (mColorSpace)
  {
    CFRelease(mColorSpace);
    mColorSpace = nullptr;
  }
#endif
  
  DELETE_NULL(mDrawBitmap);
  DELETE_NULL(mTmpBitmap);
}

IBitmap IGraphicsLice::LoadIBitmap(const char* name, int nStates, bool framesAreHoriztonal, double sourceScale)
{
  const double targetScale = GetDisplayScale(); // targetScale = what this screen is

  LICE_IBitmap* pLB = s_bitmapCache.Find(name, targetScale);

  if (!pLB) // if bitmap not in cache allready at targetScale
  {
    WDL_String fullPath;
    OSLoadBitmap(name, fullPath);
    pLB = LoadAPIBitmap(fullPath.Get());
#ifndef NDEBUG
    bool imgResourceFound = pLB;
#endif
    assert(imgResourceFound); // Protect against typos in resource.h and .rc files.

    const IBitmap bitmap(pLB, pLB->getWidth() / sourceScale, pLB->getHeight() / sourceScale, nStates, framesAreHoriztonal, sourceScale, name);

    if (sourceScale != targetScale) {
      return ScaleIBitmap(bitmap, name, targetScale); // will add to cache
    }
    else {
      s_bitmapCache.Add(pLB, name, sourceScale);
      return IBitmap(pLB, pLB->getWidth() / sourceScale, pLB->getHeight() / sourceScale, nStates, framesAreHoriztonal, sourceScale, name);
    }
  }

  // if bitmap allready cached at scale
  // TODO: this is horribly hacky
  if(targetScale > 1.)
    return IBitmap(pLB, pLB->getWidth() / targetScale, pLB->getHeight() / targetScale, nStates, framesAreHoriztonal, sourceScale, name);
  else
    return IBitmap(pLB, pLB->getWidth(), pLB->getHeight(), nStates, framesAreHoriztonal, sourceScale, name);
}

void IGraphicsLice::ReleaseIBitmap(IBitmap& bitmap)
{
  s_bitmapCache.Remove((LICE_IBitmap*) bitmap.mData);
}

void IGraphicsLice::RetainIBitmap(IBitmap& bitmap, const char * cacheName)
{
  s_bitmapCache.Add((LICE_IBitmap*)bitmap.mData, cacheName);
}

IBitmap IGraphicsLice::ScaleIBitmap(const IBitmap& bitmap, const char* name, double targetScale)
{
  const double scalingFactor = targetScale / bitmap.mSourceScale;
  LICE_IBitmap* pSrc = (LICE_IBitmap*) bitmap.mData;
  
  const int destW = pSrc->getWidth() * scalingFactor;
  const int destH = pSrc->getHeight() * scalingFactor;
  
  LICE_MemBitmap* pDest = new LICE_MemBitmap(destW, destH);
  LICE_ScaledBlit(pDest, pSrc, 0, 0, destW, destH, 0.0f, 0.0f, (float) pSrc->getWidth(), (float) pSrc->getHeight(), 1.0f, LICE_BLIT_MODE_COPY | LICE_BLIT_FILTER_BILINEAR);
  
  IBitmap bmp(pDest, bitmap.W, bitmap.H, bitmap.N, bitmap.mFramesAreHorizontal, bitmap.mSourceScale, name);
  s_bitmapCache.Add((LICE_IBitmap*) bmp.mData, name, targetScale);
  return bmp;
}

IBitmap IGraphicsLice::CropIBitmap(const IBitmap& bitmap, const IRECT& rect, const char* name, double targetScale)
{
  int destW = rect.W(), destH = rect.H();
  LICE_IBitmap* pSrc = (LICE_IBitmap*) bitmap.mData;
  LICE_MemBitmap* pDest = new LICE_MemBitmap(destW, destH);
  LICE_Blit(pDest, pSrc, 0, 0, rect.L, rect.T, destW, destH, 1.0f, LICE_BLIT_MODE_COPY);
  
  IBitmap bmp(pDest, destW, destH, bitmap.N, bitmap.mFramesAreHorizontal, bitmap.mSourceScale, name);
  s_bitmapCache.Add((LICE_IBitmap*) bmp.mData, name, targetScale);
  return bmp;
}

void IGraphicsLice::PrepDraw()
{
  int w = Width() * mDisplayScale;
  int h = Height() * mDisplayScale;

  mDrawBitmap = new LICE_SysBitmap(w, h);
  mTmpBitmap = new LICE_MemBitmap();
}

void IGraphicsLice::ReScale()
{
  mDrawBitmap->resize(Width() * mDisplayScale, Height() * mDisplayScale);
  IGraphics::ReScale(); // will cause all the controls to update their bitmaps
}

void IGraphicsLice::DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IChannelBlend* pBlend)
{
  IRECT rect = dest;
  rect.Scale(mDisplayScale);
  
  IRECT sdr = mDrawRECT;
  sdr.Scale(mDisplayScale);

  srcX *= mDisplayScale;
  srcY *= mDisplayScale;
  
  LICE_IBitmap* pLB = (LICE_IBitmap*) bitmap.mData;
  IRECT r = rect.Intersect(sdr);
  srcX += r.L - rect.L;
  srcY += r.T - rect.T;
  LICE_Blit(mDrawBitmap, pLB, r.L, r.T, srcX, srcY, r.W(), r.H(), LiceWeight(pBlend), LiceBlendMode(pBlend));
}

void IGraphicsLice::DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IChannelBlend* pBlend)
{
  LICE_IBitmap* pLB = (LICE_IBitmap*) bitmap.mData;
  
  int W = bitmap.W * mDisplayScale;
  int H = bitmap.H * mDisplayScale;
  int destX = (destCtrX * mDisplayScale) - W / 2;
  int destY = (destCtrY * mDisplayScale) - H / 2;
  
  LICE_RotatedBlit(mDrawBitmap, pLB, destX, destY, W, H, 0.0f, 0.0f, (float) W, (float) H, (float) angle, false, LiceWeight(pBlend), LiceBlendMode(pBlend) | LICE_BLIT_FILTER_BILINEAR, 0.0f, (float) yOffsetZeroDeg);
}

void IGraphicsLice::DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IChannelBlend* pBlend)
{
  LICE_IBitmap* pBase = (LICE_IBitmap*) base.mData;
  LICE_IBitmap* pMask = (LICE_IBitmap*) mask.mData;
  LICE_IBitmap* pTop = (LICE_IBitmap*) top.mData;
  
  double dA = angle * PI / 180.0;
  int W = base.W;
  int H = base.H;
  float xOffs = (W % 2 ? -0.5f : 0.0f);
  
  if (!mTmpBitmap)
    mTmpBitmap = new LICE_MemBitmap();
  
  LICE_Copy(mTmpBitmap, pBase);
  LICE_ClearRect(mTmpBitmap, 0, 0, W, H, LICE_RGBA(255, 255, 255, 0));
  
  LICE_RotatedBlit(mTmpBitmap, pMask, 0, 0, W, H, 0.0f, 0.0f, (float) W, (float) H, (float) dA,
                   true, 1.0f, LICE_BLIT_MODE_ADD | LICE_BLIT_FILTER_BILINEAR | LICE_BLIT_USE_ALPHA, xOffs, 0.0f);
  LICE_RotatedBlit(mTmpBitmap, pTop, 0, 0, W, H, 0.0f, 0.0f, (float) W, (float) H, (float) dA,
                   true, 1.0f, LICE_BLIT_MODE_COPY | LICE_BLIT_FILTER_BILINEAR | LICE_BLIT_USE_ALPHA, xOffs, 0.0f);
  
  IRECT r = IRECT(x, y, x + W, y + H).Intersect(mDrawRECT);
  LICE_Blit(mDrawBitmap, mTmpBitmap, r.L, r.T, r.L - x, r.T - y, r.R - r.L, r.B - r.T, LiceWeight(pBlend), LiceBlendMode(pBlend));
}

void IGraphicsLice::DrawPoint(const IColor& color, float x, float y, const IChannelBlend* pBlend, bool aa)
{
  float weight = (pBlend ? pBlend->mWeight : 1.0f);
  LICE_PutPixel(mDrawBitmap, int((x * mDisplayScale) + 0.5f), int((y  * mDisplayScale) + 0.5f), LiceColor(color), weight, LiceBlendMode(pBlend));
}

void IGraphicsLice::DrawTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IChannelBlend* pBlend)
{
  //TODO:
}

void IGraphicsLice::ForcePixel(const IColor& color, int x, int y)
{
  LICE_pixel* px = mDrawBitmap->getBits();
  px += int(x * mDisplayScale) + int(y  * mDisplayScale) * mDrawBitmap->getRowSpan();
  *px = LiceColor(color);
}

void IGraphicsLice::DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IChannelBlend* pBlend, bool aa)
{
  LICE_Line(mDrawBitmap, (int) x1 * mDisplayScale, (int) y1 * mDisplayScale, (int) x2 * mDisplayScale, (int) y2 * mDisplayScale, LiceColor(color), LiceWeight(pBlend), LiceBlendMode(pBlend), aa);
}

void IGraphicsLice::DrawArc(const IColor& color, float cx, float cy, float r, float minAngle, float maxAngle, const IChannelBlend* pBlend, bool aa)
{
  LICE_Arc(mDrawBitmap, cx * mDisplayScale, cy * mDisplayScale, r * mDisplayScale, minAngle, maxAngle, LiceColor(color), LiceWeight(pBlend), LiceBlendMode(pBlend), aa);
}

void IGraphicsLice::DrawCircle(const IColor& color, float cx, float cy, float r, const IChannelBlend* pBlend, bool aa)
{
  LICE_Circle(mDrawBitmap, cx * mDisplayScale, cy * mDisplayScale, r * mDisplayScale, LiceColor(color), LiceWeight(pBlend), LiceBlendMode(pBlend), aa);
}

void IGraphicsLice::DrawRoundRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend, int cornerradius, bool aa)
{
  IRECT r = rect;
  r.Scale(mDisplayScale);
  
  LICE_RoundRect(mDrawBitmap, (float) r.L, (float) r.T, (float) r.W(), (float) r.H(), cornerradius * mDisplayScale, LiceColor(color), LiceWeight(pBlend), LiceBlendMode(pBlend), aa);
}

void IGraphicsLice::FillRoundRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend, int cornerradius, bool aa)
{
  IRECT r = rect;
  r.Scale(mDisplayScale);
  
  int x1 = r.L;
  int y1 = r.T;
  int h = r.H();
  int w = r.W();
  
  cornerradius *= mDisplayScale;
  
  int mode = LiceBlendMode(pBlend);
  float weight = LiceWeight(pBlend);
  LICE_pixel lcolor = LiceColor(color);
  
  LICE_FillRect(mDrawBitmap, x1+cornerradius, y1, w-2*cornerradius, h, lcolor, weight, mode);
  LICE_FillRect(mDrawBitmap, x1, y1+cornerradius, cornerradius, h-2*cornerradius,lcolor, weight, mode);
  LICE_FillRect(mDrawBitmap, x1+w-cornerradius, y1+cornerradius, cornerradius, h-2*cornerradius, lcolor, weight, mode);
  
  //void LICE_FillCircle(LICE_IBitmap* dest, float cx, float cy, float r, LICE_pixel color, float alpha, int mode, bool aa)
  LICE_FillCircle(mDrawBitmap, (float) x1+cornerradius, (float) y1+cornerradius, (float) cornerradius, lcolor, weight, mode, aa);
  LICE_FillCircle(mDrawBitmap, (float) x1+w-cornerradius-1, (float) y1+h-cornerradius-1, (float) cornerradius, lcolor, weight, mode, aa);
  LICE_FillCircle(mDrawBitmap, (float) x1+w-cornerradius-1, (float) y1+cornerradius, (float) cornerradius, lcolor, weight, mode, aa);
  LICE_FillCircle(mDrawBitmap, (float) x1+cornerradius, (float) y1+h-cornerradius-1, (float) cornerradius, lcolor, weight, mode, aa);
}

void IGraphicsLice::FillIRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend)
{
  IRECT r = rect;
  r.Scale(mDisplayScale);
  
  LICE_FillRect(mDrawBitmap, r.L, r.T, r.W(), r.H(), LiceColor(color), LiceWeight(pBlend), LiceBlendMode(pBlend));
}

void IGraphicsLice::FillCircle(const IColor& color, int cx, int cy, float r, const IChannelBlend* pBlend, bool aa)
{
  LICE_FillCircle(mDrawBitmap, (float) cx * mDisplayScale, (float) cy * mDisplayScale, r * mDisplayScale, LiceColor(color), LiceWeight(pBlend), LiceBlendMode(pBlend), aa);
}

void IGraphicsLice::FillTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IChannelBlend* pBlend)
{
  LICE_FillTriangle(mDrawBitmap, x1 * mDisplayScale, y1 * mDisplayScale, x2 * mDisplayScale, y2 * mDisplayScale, x3 * mDisplayScale, y3 * mDisplayScale, LiceColor(color), LiceWeight(pBlend), LiceBlendMode(pBlend));
}

void IGraphicsLice::FillIConvexPolygon(const IColor& color, int* x, int* y, int npoints, const IChannelBlend* pBlend)
{
  //TODO: FillIConvexPolygon won't scale
  LICE_FillConvexPolygon(mDrawBitmap, x, y, npoints, LiceColor(color), LiceWeight(pBlend), LiceBlendMode(pBlend));
}

IColor IGraphicsLice::GetPoint(int x, int y)
{
  LICE_pixel pix = LICE_GetPixel(mDrawBitmap, x * mDisplayScale, y * mDisplayScale);
  return IColor(LICE_GETA(pix), LICE_GETR(pix), LICE_GETG(pix), LICE_GETB(pix));
}

bool IGraphicsLice::DrawIText(const IText& text, const char* str, IRECT& rect, bool measure)
{
  if (!str || str[0] == '\0')
  {
    return true;
  }
  
  LICE_IFont* font = text.mCached;
  
  if (!font)
  {
    font = CacheFont(const_cast<IText&>(text));
    if (!font) return false;
  }
  
  LICE_pixel color = LiceColor(text.mColor);
  font->SetTextColor(color);
  
  UINT fmt = DT_NOCLIP;
  if (LICE_GETA(color) < 255) fmt |= LICE_DT_USEFGALPHA;
  if (text.mAlign == IText::kAlignNear)
    fmt |= DT_LEFT;
  else if (text.mAlign == IText::kAlignCenter)
    fmt |= DT_CENTER;
  else // if (text.mAlign == IText::kAlignFar)
    fmt |= DT_RIGHT;
  
  if (measure)
  {
    fmt |= DT_CALCRECT;
    RECT R = {0,0,0,0};
    font->DrawText(mDrawBitmap, str, -1, &R, fmt);
    
    if( text.mAlign == IText::kAlignNear)
    {
      rect.R = R.right;
    }
    else if (text.mAlign == IText::kAlignCenter)
    {
      rect.L = (int) rect.MW() - (R.right/2);
      rect.R = rect.L + R.right;
    }
    else // (text.mAlign == IText::kAlignFar)
    {
      rect.L = rect.R - R.right;
      rect.R = rect.L + R.right;
    }
    
    rect.B = rect.T + R.bottom;
  }
  else
  {
    RECT R = { rect.L, rect.T, rect.R, rect.B };
    font->DrawText(mDrawBitmap, str, -1, &R, fmt);
  }
  
  return true;
}

LICE_IFont* IGraphicsLice::CacheFont(IText& text)
{
  LICE_CachedFont* font = (LICE_CachedFont*)s_fontCache.Find(text);
  if (!font)
  {
    font = new LICE_CachedFont;
    int h = text.mSize;
    int esc = 10 * text.mOrientation;
    int wt = (text.mStyle == IText::kStyleBold ? FW_BOLD : FW_NORMAL);
    int it = (text.mStyle == IText::kStyleItalic ? TRUE : FALSE);
    
    int q;
    if (text.mQuality == IText::kQualityDefault)
      q = DEFAULT_QUALITY;
#ifdef CLEARTYPE_QUALITY
    else if (text.mQuality == IText::kQualityClearType)
      q = CLEARTYPE_QUALITY;
    else if (text.mQuality == IText::kQualityAntiAliased)
#else
      else if (text.mQuality != IText::kQualityNonAntiAliased)
#endif
        q = ANTIALIASED_QUALITY;
      else // if (text.mQuality == IText::kQualityNonAntiAliased)
        q = NONANTIALIASED_QUALITY;
    
#ifdef OS_OSX
    bool resized = false;
  Resize:
    if (h < 2) h = 2;
#endif
    HFONT hFont = CreateFont(h, 0, esc, esc, wt, it, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, q, DEFAULT_PITCH, text.mFont);
    if (!hFont)
    {
      delete(font);
      return 0;
    }
    font->SetFromHFont(hFont, LICE_FONT_FLAG_OWNS_HFONT | LICE_FONT_FLAG_FORCE_NATIVE);
#ifdef OS_OSX
    if (!resized && font->GetLineHeight() != h)
    {
      h = int((double)(h * h) / (double)font->GetLineHeight() + 0.5);
      resized = true;
      goto Resize;
    }
#endif
    s_fontCache.Add(font, text);
  }
  text.mCached = font;
  return font;
}

bool IGraphicsLice::MeasureIText(const IText& text, const char* str, IRECT& destRect)
{
  return IGraphicsLice::DrawIText(text, str, destRect, true);
}

LICE_IBitmap* IGraphicsLice::LoadAPIBitmap(const char* path)
{
  if (CSTR_NOT_EMPTY(path))
  {
    const char* ext = path +strlen(path)-1;
    while (ext >= path && *ext != '.') --ext;
    ++ext;
    
    bool ispng = !stricmp(ext, "png");
#ifndef IPLUG_JPEG_SUPPORT
    if (!ispng) return 0;
#else
    bool isjpg = !stricmp(ext, "jpg");
    if (!isjpg && !ispng) return 0;
#endif
    
    if (ispng) return LICE_LoadPNG(path);
#ifdef IPLUG_JPEG_SUPPORT
    if (isjpg) return LICE_LoadJPG(path);
#endif
  }
    
  return 0;
}

void IGraphicsLice::RenderAPIBitmap(void *pContext)
{
#ifdef OS_OSX
  CGImageRef img = NULL;
  CGRect r = CGRectMake(0, 0, Width(), Height());

  if (!mColorSpace)
  {
    SInt32 v = GetSystemVersion();
    
    if (v >= 0x1070)
    {
#ifdef MAC_OS_X_VERSION_10_11
      mColorSpace = CGDisplayCopyColorSpace(CGMainDisplayID());
#else
      CMProfileRef systemMonitorProfile = NULL;
      CMError getProfileErr = CMGetSystemProfile(&systemMonitorProfile);
      if(noErr == getProfileErr)
      {
        mColorSpace = CGColorSpaceCreateWithPlatformColorSpace(systemMonitorProfile);
        CMCloseProfile(systemMonitorProfile);
      }
#endif
    }
    if (!mColorSpace)
      mColorSpace = CGColorSpaceCreateDeviceRGB();
  }

#ifdef IGRAPHICS_MAC_OLD_IMAGE_DRAWING
  img = CGBitmapContextCreateImage(mDrawBitmap->getDC()->ctx); // ARGH .. access to incomplete strut
#else
  const unsigned char *p = (const unsigned char *) mDrawBitmap->getBits();

  int sw = mDrawBitmap->getRowSpan();
  int h = mDrawBitmap->getHeight();
  int w = mDrawBitmap->getWidth();

  CGDataProviderRef provider = CGDataProviderCreateWithData(NULL, p, 4 * sw * h, NULL);
  img = CGImageCreate(w, h, 8, 32, 4 * sw,(CGColorSpaceRef) mColorSpace, kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Host, provider, NULL, false, kCGRenderingIntentDefault);
  CGDataProviderRelease(provider);
#endif

  if (img)
  {
    CGContextDrawImage((CGContext*) pContext, r, img);
    CGImageRelease(img);
  }
#else // OS_WIN
#endif
}
