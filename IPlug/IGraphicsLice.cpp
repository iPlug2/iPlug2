#include "IGraphicsLice.h"
#include "IControl.h"
#include <cmath>
#include "Log.h"

class BitmapStorage
{
public:
  struct BitmapKey
  {
    int id;
    LICE_IBitmap* bitmap;
  };
  
  WDL_PtrList<BitmapKey> m_bitmaps;
  WDL_Mutex m_mutex;
  
  LICE_IBitmap* Find(int id)
  {
    WDL_MutexLock lock(&m_mutex);
    int i, n = m_bitmaps.GetSize();
    for (i = 0; i < n; ++i)
    {
      BitmapKey* key = m_bitmaps.Get(i);
      if (key->id == id) return key->bitmap;
    }
    return 0;
  }
  
  void Add(LICE_IBitmap* bitmap, int id = -1)
  {
    WDL_MutexLock lock(&m_mutex);
    BitmapKey* key = m_bitmaps.Add(new BitmapKey);
    key->id = id;
    key->bitmap = bitmap;
  }
  
  void Remove(LICE_IBitmap* bitmap)
  {
    WDL_MutexLock lock(&m_mutex);
    int i, n = m_bitmaps.GetSize();
    for (i = 0; i < n; ++i)
    {
      if (m_bitmaps.Get(i)->bitmap == bitmap)
      {
        m_bitmaps.Delete(i, true);
        delete(bitmap);
        break;
      }
    }
  }
  
  ~BitmapStorage()
  {
    int i, n = m_bitmaps.GetSize();
    for (i = 0; i < n; ++i)
    {
      delete(m_bitmaps.Get(i)->bitmap);
    }
    m_bitmaps.Empty(true);
  }
};

static BitmapStorage s_bitmapCache;

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
  WDL_Mutex m_mutex;
  
  LICE_IFont* Find(IText* pTxt)
  {
    WDL_MutexLock lock(&m_mutex);
    int i = 0, n = m_fonts.GetSize();
    for (i = 0; i < n; ++i)
    {
      FontKey* key = m_fonts.Get(i);
      if (key->size == pTxt->mSize && key->orientation == pTxt->mOrientation && key->style == pTxt->mStyle && !strcmp(key->face, pTxt->mFont)) return key->font;
    }
    return 0;
  }
  
  void Add(LICE_IFont* font, IText* pTxt)
  {
    WDL_MutexLock lock(&m_mutex);
    FontKey* key = m_fonts.Add(new FontKey);
    key->size = pTxt->mSize;
    key->orientation = pTxt->mOrientation;
    key->style = pTxt->mStyle;
    strcpy(key->face, pTxt->mFont);
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

inline LICE_pixel LiceColor(const IColor* pColor)
{
  return LICE_RGBA(pColor->R, pColor->G, pColor->B, pColor->A);
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

IGraphicsLice::IGraphicsLice(IPlugBase* pPlug, int w, int h, int refreshFPS)
: IGraphics(pPlug, w, h, refreshFPS)
, mDrawBitmap(0)
, mTmpBitmap(0)
{}

IGraphicsLice::~IGraphicsLice() 
{
  DELETE_NULL(mDrawBitmap);
  DELETE_NULL(mTmpBitmap);
}

IBitmap IGraphicsLice::LoadIBitmap(int ID, const char* name, int nStates, bool framesAreHoriztonal)
{
  LICE_IBitmap* lb = s_bitmapCache.Find(ID);
  if (!lb)
  {
    lb = OSLoadBitmap(ID, name);
#ifndef NDEBUG
    bool imgResourceFound = lb;
#endif
    assert(imgResourceFound); // Protect against typos in resource.h and .rc files.
    s_bitmapCache.Add(lb, ID);
  }
  return IBitmap(lb, lb->getWidth(), lb->getHeight(), nStates, framesAreHoriztonal);
}

void IGraphicsLice::RetainBitmap(IBitmap* pBitmap)
{
  s_bitmapCache.Add((LICE_IBitmap*)pBitmap->mData);
}

void IGraphicsLice::ReleaseBitmap(IBitmap* pBitmap)
{
  s_bitmapCache.Remove((LICE_IBitmap*)pBitmap->mData);
}

void IGraphicsLice::PrepDraw()
{
  mDrawBitmap = new LICE_SysBitmap(Width(), Height());
  mTmpBitmap = new LICE_MemBitmap();
}

bool IGraphicsLice::DrawBitmap(IBitmap* pIBitmap, IRECT* pDest, int srcX, int srcY, const IChannelBlend* pBlend)
{
  LICE_IBitmap* pLB = (LICE_IBitmap*) pIBitmap->mData;
  IRECT r = pDest->Intersect(&mDrawRECT);
  srcX += r.L - pDest->L;
  srcY += r.T - pDest->T;
  LICE_Blit(mDrawBitmap, pLB, r.L, r.T, srcX, srcY, r.W(), r.H(), LiceWeight(pBlend), LiceBlendMode(pBlend));
  return true;
}

bool IGraphicsLice::DrawRotatedBitmap(IBitmap* pIBitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IChannelBlend* pBlend)
{
  LICE_IBitmap* pLB = (LICE_IBitmap*) pIBitmap->mData;
  
  //double dA = angle * PI / 180.0;
  // Can't figure out what LICE_RotatedBlit is doing for irregular bitmaps exactly.
  //double w = (double) bitmap.W;
  //double h = (double) bitmap.H;
  //double sinA = fabs(sin(dA));
  //double cosA = fabs(cos(dA));
  //int W = int(h * sinA + w * cosA);
  //int H = int(h * cosA + w * sinA);
  
  int W = pIBitmap->W;
  int H = pIBitmap->H;
  int destX = destCtrX - W / 2;
  int destY = destCtrY - H / 2;
  
  LICE_RotatedBlit(mDrawBitmap, pLB, destX, destY, W, H, 0.0f, 0.0f, (float) W, (float) H, (float) angle, false, LiceWeight(pBlend), LiceBlendMode(pBlend) | LICE_BLIT_FILTER_BILINEAR, 0.0f, (float) yOffsetZeroDeg);
  
  return true;
}

bool IGraphicsLice::DrawRotatedMask(IBitmap* pIBase, IBitmap* pIMask, IBitmap* pITop, int x, int y, double angle, const IChannelBlend* pBlend)
{
  LICE_IBitmap* pBase = (LICE_IBitmap*) pIBase->mData;
  LICE_IBitmap* pMask = (LICE_IBitmap*) pIMask->mData;
  LICE_IBitmap* pTop = (LICE_IBitmap*) pITop->mData;
  
  double dA = angle * PI / 180.0;
  int W = pIBase->W;
  int H = pIBase->H;
  //  RECT srcR = { 0, 0, W, H };
  float xOffs = (W % 2 ? -0.5f : 0.0f);
  
  if (!mTmpBitmap)
  {
    mTmpBitmap = new LICE_MemBitmap();
  }
  LICE_Copy(mTmpBitmap, pBase);
  LICE_ClearRect(mTmpBitmap, 0, 0, W, H, LICE_RGBA(255, 255, 255, 0));
  
  LICE_RotatedBlit(mTmpBitmap, pMask, 0, 0, W, H, 0.0f, 0.0f, (float) W, (float) H, (float) dA,
                   true, 1.0f, LICE_BLIT_MODE_ADD | LICE_BLIT_FILTER_BILINEAR | LICE_BLIT_USE_ALPHA, xOffs, 0.0f);
  LICE_RotatedBlit(mTmpBitmap, pTop, 0, 0, W, H, 0.0f, 0.0f, (float) W, (float) H, (float) dA,
                   true, 1.0f, LICE_BLIT_MODE_COPY | LICE_BLIT_FILTER_BILINEAR | LICE_BLIT_USE_ALPHA, xOffs, 0.0f);
  
  IRECT r = IRECT(x, y, x + W, y + H).Intersect(&mDrawRECT);
  LICE_Blit(mDrawBitmap, mTmpBitmap, r.L, r.T, r.L - x, r.T - y, r.R - r.L, r.B - r.T, LiceWeight(pBlend), LiceBlendMode(pBlend));
  return true;
}

bool IGraphicsLice::DrawPoint(const IColor* pColor, float x, float y, const IChannelBlend* pBlend, bool antiAlias)
{
  float weight = (pBlend ? pBlend->mWeight : 1.0f);
  LICE_PutPixel(mDrawBitmap, int(x + 0.5f), int(y + 0.5f), LiceColor(pColor), weight, LiceBlendMode(pBlend));
  return true;
}

bool IGraphicsLice::ForcePixel(const IColor* pColor, int x, int y)
{
  LICE_pixel* px = mDrawBitmap->getBits();
  px += x + y * mDrawBitmap->getRowSpan();
  *px = LiceColor(pColor);
  return true;
}

bool IGraphicsLice::DrawLine(const IColor* pColor, float x1, float y1, float x2, float y2, const IChannelBlend* pBlend, bool antiAlias)
{
  LICE_Line(mDrawBitmap, (int) x1, (int) y1, (int) x2, (int) y2, LiceColor(pColor), LiceWeight(pBlend), LiceBlendMode(pBlend), antiAlias);
  return true;
}

bool IGraphicsLice::DrawArc(const IColor* pColor, float cx, float cy, float r, float minAngle, float maxAngle,
                        const IChannelBlend* pBlend, bool antiAlias)
{
  LICE_Arc(mDrawBitmap, cx, cy, r, minAngle, maxAngle, LiceColor(pColor), LiceWeight(pBlend), LiceBlendMode(pBlend), antiAlias);
  return true;
}

bool IGraphicsLice::DrawCircle(const IColor* pColor, float cx, float cy, float r, const IChannelBlend* pBlend, bool antiAlias)
{
  LICE_Circle(mDrawBitmap, cx, cy, r, LiceColor(pColor), LiceWeight(pBlend), LiceBlendMode(pBlend), antiAlias);
  return true;
}

bool IGraphicsLice::RoundRect(const IColor* pColor, IRECT* pR, const IChannelBlend* pBlend, int cornerradius, bool aa)
{
  LICE_RoundRect(mDrawBitmap, (float) pR->L, (float) pR->T, (float) pR->W(), (float) pR->H(), cornerradius, LiceColor(pColor), LiceWeight(pBlend), LiceBlendMode(pBlend), aa);
  return true;
}

bool IGraphicsLice::FillRoundRect(const IColor* pColor, IRECT* pR, const IChannelBlend* pBlend, int cornerradius, bool aa)
{
  int x1 = pR->L;
  int y1 = pR->T;
  int h = pR->H();
  int w = pR->W();
  
  int mode = LiceBlendMode(pBlend);
  float weight = LiceWeight(pBlend);
  LICE_pixel color = LiceColor(pColor);
  
  LICE_FillRect(mDrawBitmap, x1+cornerradius, y1, w-2*cornerradius, h, color, weight, mode);
  LICE_FillRect(mDrawBitmap, x1, y1+cornerradius, cornerradius, h-2*cornerradius,color, weight, mode);
  LICE_FillRect(mDrawBitmap, x1+w-cornerradius, y1+cornerradius, cornerradius, h-2*cornerradius, color, weight, mode);
  
  //void LICE_FillCircle(LICE_IBitmap* dest, float cx, float cy, float r, LICE_pixel color, float alpha, int mode, bool aa)
  LICE_FillCircle(mDrawBitmap, (float) x1+cornerradius, (float) y1+cornerradius, (float) cornerradius, color, weight, mode, aa);
  LICE_FillCircle(mDrawBitmap, (float) x1+w-cornerradius-1, (float) y1+h-cornerradius-1, (float) cornerradius, color, weight, mode, aa);
  LICE_FillCircle(mDrawBitmap, (float) x1+w-cornerradius-1, (float) y1+cornerradius, (float) cornerradius, color, weight, mode, aa);
  LICE_FillCircle(mDrawBitmap, (float) x1+cornerradius, (float) y1+h-cornerradius-1, (float) cornerradius, color, weight, mode, aa);
  
  return true;
}

bool IGraphicsLice::FillIRect(const IColor* pColor, IRECT* pR, const IChannelBlend* pBlend)
{
  LICE_FillRect(mDrawBitmap, pR->L, pR->T, pR->W(), pR->H(), LiceColor(pColor), LiceWeight(pBlend), LiceBlendMode(pBlend));
  return true;
}

bool IGraphicsLice::FillCircle(const IColor* pColor, int cx, int cy, float r, const IChannelBlend* pBlend, bool antiAlias)
{
  LICE_FillCircle(mDrawBitmap, (float) cx, (float) cy, r, LiceColor(pColor), LiceWeight(pBlend), LiceBlendMode(pBlend), antiAlias);
  return true;
}

bool IGraphicsLice::FillTriangle(const IColor* pColor, int x1, int y1, int x2, int y2, int x3, int y3, IChannelBlend* pBlend)
{
  LICE_FillTriangle(mDrawBitmap, x1, y1, x2, y2, x3, y3, LiceColor(pColor), LiceWeight(pBlend), LiceBlendMode(pBlend));
  return true;
}

bool IGraphicsLice::FillIConvexPolygon(const IColor* pColor, int* x, int* y, int npoints, const IChannelBlend* pBlend)
{
  LICE_FillConvexPolygon(mDrawBitmap, x, y, npoints, LiceColor(pColor), LiceWeight(pBlend), LiceBlendMode(pBlend));
  return true;
}

IColor IGraphicsLice::GetPoint(int x, int y)
{
  LICE_pixel pix = LICE_GetPixel(mDrawBitmap, x, y);
  return IColor(LICE_GETA(pix), LICE_GETR(pix), LICE_GETG(pix), LICE_GETB(pix));
}

bool IGraphicsLice::DrawVerticalLine(const IColor* pColor, int xi, int yLo, int yHi)
{
  LICE_Line(mDrawBitmap, xi, yLo, xi, yHi, LiceColor(pColor), 1.0f, LICE_BLIT_MODE_COPY, false);
  return true;
}

bool IGraphicsLice::DrawHorizontalLine(const IColor* pColor, int yi, int xLo, int xHi)
{
  LICE_Line(mDrawBitmap, xLo, yi, xHi, yi, LiceColor(pColor), 1.0f, LICE_BLIT_MODE_COPY, false);
  return true;
}

IBitmap IGraphicsLice::ScaleBitmap(IBitmap* pIBitmap, int destW, int destH)
{
  LICE_IBitmap* pSrc = (LICE_IBitmap*) pIBitmap->mData;
  LICE_MemBitmap* pDest = new LICE_MemBitmap(destW, destH);
  LICE_ScaledBlit(pDest, pSrc, 0, 0, destW, destH, 0.0f, 0.0f, (float) pIBitmap->W, (float) pIBitmap->H, 1.0f,
                  LICE_BLIT_MODE_COPY | LICE_BLIT_FILTER_BILINEAR);
  
  IBitmap bmp(pDest, destW, destH, pIBitmap->N);
  RetainBitmap(&bmp);
  return bmp;
}

IBitmap IGraphicsLice::CropBitmap(IBitmap* pIBitmap, IRECT* pR)
{
  int destW = pR->W(), destH = pR->H();
  LICE_IBitmap* pSrc = (LICE_IBitmap*) pIBitmap->mData;
  LICE_MemBitmap* pDest = new LICE_MemBitmap(destW, destH);
  LICE_Blit(pDest, pSrc, 0, 0, pR->L, pR->T, destW, destH, 1.0f, LICE_BLIT_MODE_COPY);
  
  IBitmap bmp(pDest, destW, destH, pIBitmap->N);
  RetainBitmap(&bmp);
  return bmp;
}

LICE_pixel* IGraphicsLice::GetBits()
{
  return mDrawBitmap->getBits();
}

bool IGraphicsLice::DrawIText(IText* pTxt, const char* str, IRECT* pR, bool measure)
{
  if (!str || str[0] == '\0')
  {
    return true;
  }
  
  LICE_IFont* font = pTxt->mCached;
  
  if (!font)
  {
    font = CacheFont(pTxt);
    if (!font) return false;
  }
  
  LICE_pixel color = LiceColor(&pTxt->mColor);
  font->SetTextColor(color);
  
  UINT fmt = DT_NOCLIP;
  if (LICE_GETA(color) < 255) fmt |= LICE_DT_USEFGALPHA;
  if (pTxt->mAlign == IText::kAlignNear)
    fmt |= DT_LEFT;
  else if (pTxt->mAlign == IText::kAlignCenter)
    fmt |= DT_CENTER;
  else // if (pTxt->mAlign == IText::kAlignFar)
    fmt |= DT_RIGHT;
  
  if (measure)
  {
    fmt |= DT_CALCRECT;
    RECT R = {0,0,0,0};
    font->DrawText(mDrawBitmap, str, -1, &R, fmt);
    
    if( pTxt->mAlign == IText::kAlignNear)
    {
      pR->R = R.right;
    }
    else if (pTxt->mAlign == IText::kAlignCenter)
    {
      pR->L = (int) pR->MW() - (R.right/2);
      pR->R = pR->L + R.right;
    }
    else // (pTxt->mAlign == IText::kAlignFar)
    {
      pR->L = pR->R - R.right;
      pR->R = pR->L + R.right;
    }
    
    pR->B = pR->T + R.bottom;
  }
  else
  {
    RECT R = { pR->L, pR->T, pR->R, pR->B };
    font->DrawText(mDrawBitmap, str, -1, &R, fmt);
  }
  
  return true;
}

LICE_IFont* IGraphicsLice::CacheFont(IText* pTxt)
{
  LICE_CachedFont* font = (LICE_CachedFont*)s_fontCache.Find(pTxt);
  if (!font)
  {
    font = new LICE_CachedFont;
    int h = pTxt->mSize;
    int esc = 10 * pTxt->mOrientation;
    int wt = (pTxt->mStyle == IText::kStyleBold ? FW_BOLD : FW_NORMAL);
    int it = (pTxt->mStyle == IText::kStyleItalic ? TRUE : FALSE);
    
    int q;
    if (pTxt->mQuality == IText::kQualityDefault)
      q = DEFAULT_QUALITY;
#ifdef CLEARTYPE_QUALITY
    else if (pTxt->mQuality == IText::kQualityClearType)
      q = CLEARTYPE_QUALITY;
    else if (pTxt->mQuality == IText::kQualityAntiAliased)
#else
      else if (pTxt->mQuality != IText::kQualityNonAntiAliased)
#endif
        q = ANTIALIASED_QUALITY;
      else // if (pTxt->mQuality == IText::kQualityNonAntiAliased)
        q = NONANTIALIASED_QUALITY;
    
#ifdef OS_OSX
    bool resized = false;
  Resize:
    if (h < 2) h = 2;
#endif
    HFONT hFont = CreateFont(h, 0, esc, esc, wt, it, FALSE, FALSE, ANSI_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, q, DEFAULT_PITCH, pTxt->mFont);
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
    s_fontCache.Add(font, pTxt);
  }
  pTxt->mCached = font;
  return font;
}
