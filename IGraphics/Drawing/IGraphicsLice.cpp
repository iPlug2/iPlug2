/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <cmath>

#include "IGraphicsLice.h"
#include "ITextEntryControl.h"

#include "lice_combine.h"

extern int GetSystemVersion();

static StaticStorage<LICE_IFont> s_fontCache;

#pragma mark -

IGraphicsLice::IGraphicsLice(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGraphics(dlg, w, h, fps, scale)
{
  DBGMSG("IGraphics Lice @ %i FPS\n", fps);
  StaticStorage<LICE_IFont>::Accessor storage(s_fontCache);
  storage.Retain();
}

IGraphicsLice::~IGraphicsLice() 
{
#ifdef OS_MAC
  if (mColorSpace)
  {
    CFRelease(mColorSpace);
    mColorSpace = nullptr;
  }
#endif
  
  DELETE_NULL(mDrawBitmap);
  DELETE_NULL(mTmpBitmap);
#ifdef OS_WIN
  DELETE_NULL(mScaleBitmap);
#endif

  StaticStorage<LICE_IFont>::Accessor storage(s_fontCache);
  storage.Release();
}

void IGraphicsLice::DrawResize()
{
  if(!mDrawBitmap)
    mDrawBitmap = new LICE_SysBitmap(Width() * GetScreenScale(), Height() * GetScreenScale());
  else
    mDrawBitmap->resize(Width() * GetScreenScale(), Height() * GetScreenScale());

#ifdef OS_WIN
  if (GetDrawScale() == 1.0)
  {
    DELETE_NULL(mScaleBitmap);
  }
  else
  {
    if (!mScaleBitmap)
      mScaleBitmap = new LICE_SysBitmap(WindowWidth() * GetScreenScale(), WindowHeight() * GetScreenScale());
    else
      mScaleBitmap->resize(WindowWidth() * GetScreenScale(), WindowHeight() * GetScreenScale());
  }
#endif

  mRenderBitmap = mDrawBitmap;
}

void IGraphicsLice::DrawSVG(ISVG& svg, const IRECT& bounds, const IBlend* pBlend)
{
  DrawText(DEFAULT_TEXT, "UNSUPPORTED", bounds);
}

void IGraphicsLice::DrawRotatedSVG(ISVG& svg, float destCtrX, float destCtrY, float width, float height, double angle, const IBlend* pBlend)
{
  IRECT r = IRECT(destCtrX - (width/2.), destCtrY - (height/2.), destCtrX + width, destCtrY + height);
  DrawText(DEFAULT_TEXT, "UNSUPPORTED", r);
}

void IGraphicsLice::DrawBitmap(IBitmap& bitmap, const IRECT& bounds, int srcX, int srcY, const IBlend* pBlend)
{
  const int ds = GetScreenScale();
  IRECT sr = TransformRECT(bounds);
  
  IRECT sdr = mDrawRECT;
  sdr.Scale(ds);

  srcX *= ds;
  srcY *= ds;
  
  LICE_IBitmap* pLB = bitmap.GetAPIBitmap()->GetBitmap();
  IRECT r = sr.Intersect(sdr);
  srcX += r.L - sr.L;
  srcY += r.T - sr.T;
  LICE_Blit(mRenderBitmap, pLB, r.L, r.T, srcX, srcY, r.W(), r.H(), BlendWeight(pBlend), LiceBlendMode(pBlend));
}

void IGraphicsLice::DrawRotatedBitmap(IBitmap& bitmap, float destCtrX, float destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend)
{
  const int ds = GetScreenScale();
  LICE_IBitmap* pLB = bitmap.GetAPIBitmap()->GetBitmap();
  
  int W = bitmap.W() * ds;
  int H = bitmap.H() * ds;
  int destX = TransformX(destCtrX) - W / 2;
  int destY = TransformY(destCtrY) - H / 2;
  
  LICE_RotatedBlit(mRenderBitmap, pLB, destX, destY, W, H, 0.0f, 0.0f, (float) W, (float) H, (float) DegToRad(angle), false, BlendWeight(pBlend), LiceBlendMode(pBlend) | LICE_BLIT_FILTER_BILINEAR, 0.0f, (float) yOffsetZeroDeg);
}

void IGraphicsLice::DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, float x, float y, double angle, const IBlend* pBlend)
{
  x = TransformX(x);
  y = TransformY(y);
  
  LICE_IBitmap* pBase = base.GetAPIBitmap()->GetBitmap();
  LICE_IBitmap* pMask = mask.GetAPIBitmap()->GetBitmap();
  LICE_IBitmap* pTop = top.GetAPIBitmap()->GetBitmap();
  
  int W = base.W();
  int H = base.H();
  float xOffs = (W % 2 ? -0.5f : 0.0f);
  
  if (!mTmpBitmap)
    mTmpBitmap = new LICE_MemBitmap();
  
  const float angleRadians = DegToRad(angle);
  
  LICE_Copy(mTmpBitmap, pBase);
  LICE_ClearRect(mTmpBitmap, 0, 0, W, H, LICE_RGBA(255, 255, 255, 0));
  
  LICE_RotatedBlit(mTmpBitmap, pMask, 0, 0, W, H, 0.0f, 0.0f, (float) W, (float) H, angleRadians,
                   true, 1.0f, LICE_BLIT_MODE_ADD | LICE_BLIT_FILTER_BILINEAR | LICE_BLIT_USE_ALPHA, xOffs, 0.0f);
  LICE_RotatedBlit(mTmpBitmap, pTop, 0, 0, W, H, 0.0f, 0.0f, (float) W, (float) H, angleRadians,
                   true, 1.0f, LICE_BLIT_MODE_COPY | LICE_BLIT_FILTER_BILINEAR | LICE_BLIT_USE_ALPHA, xOffs, 0.0f);
  
  IRECT r = IRECT(x, y, x + W, y + H).Intersect(mDrawRECT);
  LICE_Blit(mRenderBitmap, mTmpBitmap, r.L, r.T, r.L - x, r.T - y, r.R - r.L, r.B - r.T, BlendWeight(pBlend), LiceBlendMode(pBlend));
}

void IGraphicsLice::DrawFittedBitmap(IBitmap& bitmap, const IRECT& bounds, const IBlend* pBlend)
{
  // TODO - clipping
  IRECT r = TransformRECT(bounds);
  LICE_IBitmap* pSrc = bitmap.GetAPIBitmap()->GetBitmap();
  LICE_ScaledBlit(mRenderBitmap, pSrc, r.L, r.T, r.W(), r.H(), 0.0f, 0.0f, (float) pSrc->getWidth(), (float) pSrc->getHeight(), BlendWeight(pBlend), LiceBlendMode(pBlend) | LICE_BLIT_FILTER_BILINEAR);
}

void IGraphicsLice::DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend)
{
  LICE_PutPixel(mRenderBitmap, int(TransformX(x) + 0.5f), int(TransformY(y) + 0.5f), LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend));
}

void IGraphicsLice::DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, float thickness)
{
  //TODO: review floating point input support

  LICE_FLine(mRenderBitmap, TransformX(x1), TransformY(y1), TransformX(x2), TransformY(y2), LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
}

void IGraphicsLice::DrawDottedLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, float thickness, float dashLen)
{
  //TODO: review floating point input support

  const int dash = 2 * GetScreenScale();
  
  LICE_DashedLine(mRenderBitmap, TransformX(x1), TransformY(y1), TransformX(x2), TransformY(y2), dash, dash, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
}

void IGraphicsLice::DrawTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend, float thickness)
{
  if (!OpacityCheck(color, pBlend))
  {
    OpacityLayer(&IGraphicsLice::DrawTriangle, pBlend, color, x1, x1, x2, y2, x3, y3, nullptr, thickness);
    return;
  }
    
  DrawLine(color, x1, y1, x2, y2, pBlend, 1.0);
  DrawLine(color, x2, y2, x3, y3, pBlend, 1.0);
  DrawLine(color, x3, y3, x1, y1, pBlend, 1.0);
}

void IGraphicsLice::DrawRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness)
{
  if (!OpacityCheck(color, pBlend))
  {
    OpacityLayer(&IGraphicsLice::DrawRect, pBlend, color, bounds, nullptr, thickness);
    return;
  }
    
  DrawLine(color, bounds.L, bounds.T, bounds.R, bounds.T, pBlend, thickness);
  DrawLine(color, bounds.L, bounds.B, bounds.R, bounds.B, pBlend, thickness);
  DrawLine(color, bounds.L, bounds.T, bounds.L, bounds.B, pBlend, thickness);
  DrawLine(color, bounds.R, bounds.T, bounds.R, bounds.B, pBlend, thickness);
}

void IGraphicsLice::DrawRoundRect(const IColor& color, const IRECT& bounds, float cr, const IBlend* pBlend, float)
{
  //TODO: review floating point input support
  
  IRECT r = TransformRECT(bounds);

  LICE_RoundRect(mRenderBitmap, r.L, r.T, r.W(), r.H(), cr * GetScreenScale(), LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
}

void IGraphicsLice::DrawConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend, float thickness)
{
  if (!OpacityCheck(color, pBlend))
  {
    OpacityLayer(&IGraphicsLice::DrawConvexPolygon, pBlend, color, x, y, npoints, nullptr, thickness);
    return;
  }
    
  for (int i = 0; i < npoints - 1; i++)
    DrawLine(color, x[i], y[i], x[i+1], y[i+1], pBlend, 1.0);
  
  DrawLine(color, x[npoints - 1], y[npoints - 1], x[0], y[0], pBlend, 1.0);
}

void IGraphicsLice::DrawArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax, const IBlend* pBlend, float thickness)
{
  //TODO: review floating point input support

  LICE_Arc(mRenderBitmap, TransformX(cx), TransformY(cy), r * GetScreenScale(), DegToRad(aMin), DegToRad(aMax), LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
}

void IGraphicsLice::DrawCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend, float)
{
  //TODO: review floating point input support

  LICE_Circle(mRenderBitmap, TransformX(cx), TransformY(cy), r * GetScreenScale(), LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
}

void IGraphicsLice::DrawDottedRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float thickness, float dashLen)
{
  if (!OpacityCheck(color, pBlend))
  {
    OpacityLayer(&IGraphicsLice::DrawDottedRect, pBlend, color, bounds, nullptr, thickness, dashLen);
    return;
  }
    
  DrawDottedLine(color, bounds.L, bounds.T, bounds.R, bounds.T, pBlend, thickness, dashLen);
  DrawDottedLine(color, bounds.L, bounds.B, bounds.R, bounds.B, pBlend, thickness, dashLen);
  DrawDottedLine(color, bounds.L, bounds.T, bounds.L, bounds.B, pBlend, thickness, dashLen);
  DrawDottedLine(color, bounds.R, bounds.T, bounds.R, bounds.B, pBlend, thickness, dashLen);
}

void IGraphicsLice::FillTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend)
{
  //TODO: review floating point input support
  
  LICE_FillTriangle(mRenderBitmap, TransformX(x1), TransformY(y1), TransformX(x2), TransformY(y2), TransformX(x3), TransformY(y3), LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend));
}

void IGraphicsLice::FillRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend)
{
  //TODO: review floating point input support
  
  IRECT r = TransformRECT(bounds);

  LICE_FillRect(mRenderBitmap, r.L, r.T, r.W(), r.H(), LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend));
}

void IGraphicsLice::FillRoundRect(const IColor& color, const IRECT& bounds, float cr, const IBlend* pBlend)
{
  //TODO: review floating point input support
  
  if (!OpacityCheck(color, pBlend))
  {
    void (IGraphicsLice::*method)(const IColor&, const IRECT&, float, const IBlend*) = &IGraphicsLice::FillRoundRect;
    OpacityLayer(method, pBlend, color, bounds, cr, nullptr);
    return;
  }

  IRECT r = TransformRECT(bounds);
  
  float x1 = r.L;
  float y1 = r.T;
  float h = r.H();
  float w = r.W();
  
  cr *= GetScreenScale();
  cr = std::abs(cr);
  cr = std::min(cr, r.W() / 2.f);
  cr = std::min(cr, r.H() / 2.f);
  
  int mode = LiceBlendMode(pBlend);
  float weight = BlendWeight(pBlend);
  LICE_pixel lcolor = LiceColor(color);
  
  LICE_FillRect(mRenderBitmap, x1+cr, y1, w-2.f*cr, h, lcolor, weight, mode);
  LICE_FillRect(mRenderBitmap, x1, y1+cr, cr, h-2.f*cr,lcolor, weight, mode);
  LICE_FillRect(mRenderBitmap, x1+w-cr, y1+cr, cr, h-2*cr, lcolor, weight, mode);
  
  LICE_FillCircle(mRenderBitmap, x1+cr, y1+cr, cr, lcolor, weight, mode, true);
  LICE_FillCircle(mRenderBitmap, x1+w-cr, y1+h-cr, cr, lcolor, weight, mode, true);
  LICE_FillCircle(mRenderBitmap, x1+w-cr, y1+cr, cr, lcolor, weight, mode, true);
  LICE_FillCircle(mRenderBitmap, x1+cr, y1+h-cr, cr, lcolor, weight, mode, true);
}

void IGraphicsLice::FillConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend)
{
  //TODO: review floating point input support
  
  int xarray[512];
  int yarray[512];
  int* xpoints = xarray;
  int* ypoints = yarray;

  if (npoints > 512)
  {
    xpoints = new int[npoints * 2];
    ypoints = xpoints + npoints;
  }

  for (int i = 0; i < npoints; i++)
  {
    xpoints[i] = TransformX(x[i]);
    ypoints[i] = TransformY(y[i]);
  }
    
  LICE_FillConvexPolygon(mRenderBitmap, xpoints, ypoints, npoints, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend));
    
  if (npoints > 512)
    delete[] xpoints;
}

void IGraphicsLice::FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend)
{
  //TODO: review floating point input support

  LICE_FillCircle(mRenderBitmap, TransformX(cx), TransformY(cy), r * GetScreenScale(), LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
}

void IGraphicsLice::FillArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax,  const IBlend* pBlend)
{
  if (aMax < aMin)
    std::swap(aMin, aMax);
  
  if (aMax >= aMin + 360.f)
  {
    FillCircle(color, cx, cy, r, pBlend);
    return;
  }
  
  float xarray[181];
  float yarray[181];
  
  if (aMax > aMin + 180.f)
  {
    if (!OpacityCheck(color, pBlend))
    {
      OpacityLayer(&IGraphicsLice::FillArc, pBlend, color, cx, cy, r, aMin, aMax, nullptr);
      return;
    }
    
    FillArc(color, cx, cy, r, aMin + 178.f, aMax, pBlend);
    aMax = aMin + 180.f;
  }
  
  aMin = DegToRad(aMin-90.f);
  aMax = DegToRad(aMax-90.f);

  int arcpoints = 180.0 * std::min(1., (aMax - aMin) / PI);
  double arcincrement = (aMax - aMin) / arcpoints;
  for(int i = 0; i < arcpoints; i++)
  {
    xarray[i] = cx + cosf(i * arcincrement + aMin) * r;
    yarray[i] = cy + sinf(i * arcincrement + aMin) * r;
  }
    
  xarray[arcpoints] = cx;
  yarray[arcpoints] = cy;

  FillConvexPolygon(color, xarray, yarray, arcpoints + 1, pBlend);
}

IColor IGraphicsLice::GetPoint(int x, int y)
{
  const int ds = GetScreenScale();
  LICE_pixel pix = LICE_GetPixel(mDrawBitmap, x * ds, y * ds);
  return IColor(LICE_GETA(pix), LICE_GETR(pix), LICE_GETG(pix), LICE_GETB(pix));
}

bool IGraphicsLice::DoDrawMeasureText(const IText& text, const char* str, IRECT& bounds, const IBlend* pBlend, bool measure)
{
  const int ds = GetScreenScale();
  if (!str || str[0] == '\0')
  {
    return true;
  }
  
  LICE_IFont* font = text.mCached;
  bounds.Translate(-mDrawOffsetX, -mDrawOffsetY);
    
  if (!font || text.mCachedScale != ds)
  {
    font = CacheFont(text, ds);
    if (!font) return false;
  }
  
  LICE_pixel color;
  
  if (GetTextEntryControl() && GetTextEntryControl()->GetRECT() == bounds)
    color = LiceColor(text.mTextEntryFGColor, pBlend);
  else
    color = LiceColor(text.mFGColor, pBlend);
  
  font->SetTextColor(color);
  
  UINT fmt = DT_NOCLIP;
  if (LICE_GETA(color) < 255) fmt |= LICE_DT_USEFGALPHA;
  if (text.mAlign == IText::kAlignNear)
    fmt |= DT_LEFT;
  else if (text.mAlign == IText::kAlignCenter)
    fmt |= DT_CENTER;
  else // if (text.mAlign == IText::kAlignFar)
    fmt |= DT_RIGHT;
  
  if (text.mVAlign == IText::kVAlignTop)
    fmt |= DT_TOP;
  else if (text.mVAlign == IText::kVAlignBottom)
    fmt |= DT_BOTTOM;
  else if (text.mVAlign == IText::kVAlignMiddle)
    fmt |= DT_VCENTER;
  
  if (measure)
  {
    fmt |= DT_CALCRECT;
    RECT R = {0,0,0,0};
#if defined OS_MAC || defined OS_LINUX
    font->SWELL_DrawText(mRenderBitmap, str, -1, &R, fmt);
#elif defined OS_WIN
    font->DrawTextA(mRenderBitmap, str, -1, &R, fmt);
#else
  #error NOT IMPLEMENTED
#endif
    if( text.mAlign == IText::kAlignNear)
      bounds.R = R.right;
    else if (text.mAlign == IText::kAlignCenter)
    {
      bounds.L = (int) bounds.MW() - (R.right/2);
      bounds.R = bounds.L + R.right;
    }
    else // (text.mAlign == IText::kAlignFar)
    {
      bounds.L = bounds.R - R.right;
      bounds.R = bounds.L + R.right;
    }
    
    bounds.B = bounds.T + R.bottom;
      
    bounds.Scale(1.0 / ds);
  }
  else
  {
    IRECT r = bounds;
    r.Scale(ds);
    RECT R = { (LONG) r.L, (LONG) r.T, (LONG) r.R, (LONG) r.B };
#if defined OS_MAC || defined OS_LINUX
    font->SWELL_DrawText(mRenderBitmap, str, -1, &R, fmt);
#elif defined OS_WIN
    font->DrawTextA(mRenderBitmap, str, -1, &R, fmt);
#else
  #error NOT IMPLEMENTED
#endif
  }
  
  return true;
}

bool OpacityCheck(const IBlend* pBlend)
{
  return BlendWeight(pBlend) >= 1.f;
}

template<typename T, typename... Args>
void IGraphicsLice::OpacityLayer(T method, const IBlend* pBlend, const IColor& color, Args... args)
{
  IColor drawColor(color);
  IBlend blend = pBlend ? *pBlend : IBlend();
  blend.mWeight *= (color.A / 255.0);
  drawColor.A = 255;
  StartLayer(mDrawRECT);
  (this->*method)(drawColor, args...);
  ILayerPtr layer = EndLayer();
  DrawLayer(layer, &blend);
}

void IGraphicsLice::UpdateLayer()
{
  IRECT r = mLayers.empty() ? IRECT() : mLayers.top()->Bounds();
  mRenderBitmap = mLayers.empty() ? mDrawBitmap : mLayers.top()->GetAPIBitmap()->GetBitmap();
  mDrawRECT = mLayers.empty() ? mClipRECT : IRECT(0, 0, r.W(), r.H());
  mDrawOffsetX = mLayers.empty() ? 0 : r.L;
  mDrawOffsetY = mLayers.empty() ? 0 : r.T;
}

LICE_IFont* IGraphicsLice::CacheFont(const IText& text, double scale)
{
  StaticStorage<LICE_IFont>::Accessor storage(s_fontCache);
  WDL_String hashStr(text.mFont);
  hashStr.AppendFormatted(50, "-%d-%d-%d", text.mSize, text.mOrientation, text.mStyle);
    
  LICE_CachedFont* font = (LICE_CachedFont*) storage.Find(hashStr.Get(), scale);
  if (!font)
  {
    font = new LICE_CachedFont;
    int h = round(text.mSize * scale);
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
    
#ifdef OS_MAC
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
#ifdef OS_MAC
    if (!resized && font->GetLineHeight() != h)
    {
      h = int((double)(h * h) / (double)font->GetLineHeight() + 0.5);
      resized = true;
      goto Resize;
    }
#endif
    storage.Add(font, hashStr.Get(), scale);
  }
  text.mCached = font;
  text.mCachedScale = scale;
  return font;
}

bool IGraphicsLice::BitmapExtSupported(const char* ext)
{
  char extLower[32];
  ToLower(extLower, ext);
  
  bool ispng = strstr(extLower, "png") != nullptr;
  
  if (ispng)
    return true;

#ifdef LICE_JPEG_SUPPORT
  bool isjpg = (strstr(extLower, "jpg") != nullptr) || (strstr(extLower, "jpeg") != nullptr);

  if (isjpg)
    return true;
#endif

  return false;
}

APIBitmap* IGraphicsLice::LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext)
{
  char extLower[32];
  ToLower(extLower, ext);
  
  bool ispng = (strcmp(extLower, "png") == 0);

  if (ispng)
  {
#if defined OS_WIN
    if (location == EResourceLocation::kWinBinary)
      return new LICEBitmap(LICE_LoadPNGFromResource((HINSTANCE) GetWinModuleHandle(), fileNameOrResID, 0), scale);
    else
#endif
      return new LICEBitmap(LICE_LoadPNG(fileNameOrResID), scale);
  }

#ifdef LICE_JPEG_SUPPORT
  bool isjpg = (strcmp(extLower, "jpg") == 0) || (strcmp(extLower, "jpeg") == 0);

  if (isjpg)
  {
    #if defined OS_WIN
    if (location == EResourceLocation::kWinBinary)
      return new LICEBitmap(LICE_LoadJPGFromResource((HINSTANCE)GetWinModuleHandle(), fileNameOrResID, 0), scale);
    else
    #endif
      return new LICEBitmap(LICE_LoadJPG(fileNameOrResID), scale);
  }
#endif

  return nullptr;
}

APIBitmap* IGraphicsLice::ScaleAPIBitmap(const APIBitmap* pBitmap, int scale)
{
  int destW = (pBitmap->GetWidth() / pBitmap->GetScale()) * scale;
  int destH = (pBitmap->GetHeight() / pBitmap->GetScale()) * scale;
  
  LICE_IBitmap* pSrc = pBitmap->GetBitmap();
  LICE_MemBitmap* pDest = new LICE_MemBitmap(destW, destH);
  LICE_ScaledBlit(pDest, pSrc, 0, 0, destW, destH, 0.0f, 0.0f, (float) pSrc->getWidth(), (float) pSrc->getHeight(), 1.0f, LICE_BLIT_MODE_COPY | LICE_BLIT_FILTER_BILINEAR);
  
  return new LICEBitmap(pDest, scale);
}

APIBitmap* IGraphicsLice::CreateAPIBitmap(int width, int height)
{
  const int scale = GetScreenScale();
  LICE_IBitmap* pBitmap = new LICE_MemBitmap(width * scale, height * scale);
  memset(pBitmap->getBits(), 0, pBitmap->getRowSpan() * pBitmap->getHeight() * sizeof(LICE_pixel));
  return new LICEBitmap(pBitmap, scale);
}

void IGraphicsLice::GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data)
{
  const APIBitmap* pBitmap = layer->GetAPIBitmap();
  int size = pBitmap->GetBitmap()->getHeight() * pBitmap->GetBitmap()->getRowSpan() * sizeof(LICE_pixel);
  
  data.Resize(size);
  
  if (data.GetSize() >= size)
    memcpy(data.Get(), pBitmap->GetBitmap()->getBits(), size);
}

void IGraphicsLice::ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow)
{
  const APIBitmap* pBitmap = layer->GetAPIBitmap();
  LICE_IBitmap* pLayerBitmap = pBitmap->GetBitmap();

  int stride = pLayerBitmap->getRowSpan() * sizeof(LICE_pixel);
  int size = pLayerBitmap->getHeight() * stride;

  if (mask.GetSize() >= size)
  {
    int x = std::round(shadow.mXOffset * GetScreenScale());
    int y = std::round(shadow.mYOffset * GetScreenScale());
    int nRows = pBitmap->GetHeight() - std::abs(y);
    int nCols = pBitmap->GetWidth() - std::abs(x);
    LICE_pixel_chan* in = mask.Get() + (std::max(-x, 0) * 4) + (std::max(-y, 0) * stride);
    LICE_pixel_chan* out = ((LICE_pixel_chan*) pLayerBitmap->getBits()) + (std::max(x, 0) * 4) + (std::max(y, 0) * stride);
    
    // Pre-multiply color components
    
    IColor color = shadow.mPattern.GetStop(0).mColor;
    color.Clamp();
    unsigned int ia = (color.A * static_cast<int>(Clip(shadow.mOpacity, 0.f, 1.f) * 255.0));
    unsigned int ir = (color.R * ia);
    unsigned int ig = (color.G * ia);
    unsigned int ib = (color.B * ia);
    
    if (!shadow.mDrawForeground)
    {
      LICE_Clear(pLayerBitmap, 0);
    
      for (int i = 0 ; i < nRows; i++, in += stride, out += stride)
      {
        LICE_pixel_chan* chans = out;

        for (int j = 0 ; j < nCols; j++, chans += 4)
        {
          unsigned int maskAlpha = in[j * 4 + LICE_PIXEL_A];
        
          unsigned int A = (ia * maskAlpha) >> 16;
          unsigned int R = (ir * maskAlpha) >> 16;
          unsigned int G = (ig * maskAlpha) >> 16;
          unsigned int B = (ib * maskAlpha) >> 16;
      
          _LICE_MakePixelNoClamp(chans, R, G, B, A);
        }
      }
    }
    else
    {
      for (int i = 0 ; i < nRows; i++, in += stride, out += stride)
      {
        LICE_pixel_chan* chans = out;
        
        for (int j = 0 ; j < nCols; j++, chans += 4)
        {
          unsigned int maskAlpha = in[j * 4 + LICE_PIXEL_A];
          unsigned int alphaCmp = 255 - chans[LICE_PIXEL_A];
          
          unsigned int A = chans[LICE_PIXEL_A] + ((alphaCmp * ia * maskAlpha) >> 24);
          unsigned int R = chans[LICE_PIXEL_R] + ((alphaCmp * ir * maskAlpha) >> 24);
          unsigned int G = chans[LICE_PIXEL_G] + ((alphaCmp * ig * maskAlpha) >> 24);
          unsigned int B = chans[LICE_PIXEL_B] + ((alphaCmp * ib * maskAlpha) >> 24);
          
          _LICE_MakePixelClamp(chans, R, G, B, A);
        }
      }
    }
  }
}

void IGraphicsLice::EndFrame()
{
#ifdef OS_MAC

#ifdef IGRAPHICS_MAC_BLIT_BENCHMARK
  double tm=gettm();
#endif
    
  CGImageRef img = NULL;
  CGRect r = CGRectMake(0, 0, WindowWidth(), WindowHeight());

  if (!mColorSpace)
  {
    int v = GetSystemVersion();
    
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
    CGContextSaveGState((CGContext*) GetPlatformContext());
    CGContextTranslateCTM((CGContext*) GetPlatformContext(), 0.0, WindowHeight());
    CGContextScaleCTM((CGContext*) GetPlatformContext(), 1.0, -1.0);
    CGContextDrawImage((CGContext*) GetPlatformContext(), r, img);
    CGContextRestoreGState((CGContext*) GetPlatformContext());
    CGImageRelease(img);
  }
    
#ifdef IGRAPHICS_MAC_BLIT_BENCHMARK
    printf("blit %fms\n",(gettm()-tm)*1000.0);
#endif
    
#else // OS_WIN
  PAINTSTRUCT ps;
  HWND hWnd = (HWND) GetWindow();
  HDC dc = BeginPaint(hWnd, &ps);
  
  if (GetDrawScale() == 1.0)
  {
    BitBlt(dc, 0, 0, Width(), Height(), mDrawBitmap->getDC(), 0, 0, SRCCOPY);
  }
  else
  {
    LICE_ScaledBlit(mScaleBitmap, mDrawBitmap, 0, 0, WindowWidth(), WindowHeight(), 0, 0, Width(), Height(), 1.0, LICE_BLIT_MODE_COPY | LICE_BLIT_FILTER_BILINEAR
    );
    BitBlt(dc, 0, 0, WindowWidth(), WindowHeight(), mScaleBitmap->getDC(), 0, 0, SRCCOPY);
  }
  
  EndPaint(hWnd, &ps);
#endif
}

#ifdef OS_MAC
#ifdef FillRect
#undef FillRect
#endif
#ifdef DrawText
#undef DrawText
#endif
#ifdef Polygon
#undef Polygon
#endif

#define DrawText SWELL_DrawText
#define FillRect SWELL_FillRect
#define LineTo SWELL_LineTo
#define SetPixel SWELL_SetPixel
#define Polygon(a,b,c) SWELL_Polygon(a,b,c)
#endif

#include "IGraphicsLice_src.cpp"

