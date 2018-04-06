#include <cmath>

#include "IGraphicsLice.h"

extern int GetSystemVersion();

static StaticStorage<LICE_IFont> s_fontCache;

#pragma mark -

IGraphicsLice::IGraphicsLice(IDelegate& dlg, int w, int h, int fps)
: IGraphics(dlg, w, h, fps)
{}

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
}

void IGraphicsLice::SetDisplayScale(int scale)
{
  if(!mDrawBitmap)
  {
    mDrawBitmap = new LICE_SysBitmap(Width() * scale, Height() * scale);
    mTmpBitmap = new LICE_MemBitmap();
  }
  else
    mDrawBitmap->resize(Width() * scale, Height() * scale);
  
  IGraphics::SetDisplayScale(scale);
}

APIBitmap* IGraphicsLice::ScaleAPIBitmap(const APIBitmap* pBitmap, int scale)
{
  int destW = (pBitmap->GetWidth() / pBitmap->GetScale()) * scale;
  int destH = (pBitmap->GetHeight() / pBitmap->GetScale()) * scale;
    
  LICE_IBitmap* pSrc = (LICE_IBitmap*) pBitmap;
  LICE_MemBitmap* pDest = new LICE_MemBitmap(destW, destH);
  LICE_ScaledBlit(pDest, pSrc, 0, 0, destW, destH, 0.0f, 0.0f, (float) pSrc->getWidth(), (float) pSrc->getHeight(), 1.0f, LICE_BLIT_MODE_COPY | LICE_BLIT_FILTER_BILINEAR);
    
  return new LICEBitmap(pDest, scale);
}

void IGraphicsLice::DrawSVG(ISVG& svg, const IRECT& bounds, const IBlend* pBlend)
{
  //TODO:
//  LiceNanoSVGRender::RenderNanoSVG(mDrawBitmap, svg.mImage);
}

void IGraphicsLice::DrawRotatedSVG(ISVG& svg, float destCtrX, float destCtrY, float width, float height, double angle, const IBlend* pBlend)
{
  DrawSVG(svg, GetDrawRect(), pBlend);
  //TODO:
}

void IGraphicsLice::DrawBitmap(IBitmap& bitmap, const IRECT& bounds, int srcX, int srcY, const IBlend* pBlend)
{
  const float ds = GetDisplayScale();
  IRECT sr = bounds;
  sr.Scale(ds);
  
  IRECT sdr = mDrawRECT;
  sdr.Scale(ds);

  srcX *= ds;
  srcY *= ds;
  
  LICE_IBitmap* pLB = (LICE_IBitmap*) bitmap.GetRawBitmap();
  IRECT r = sr.Intersect(sdr);
  srcX += r.L - sr.L;
  srcY += r.T - sr.T;
  LICE_Blit(mDrawBitmap, pLB, r.L, r.T, srcX, srcY, r.W(), r.H(), BlendWeight(pBlend), LiceBlendMode(pBlend));
}

void IGraphicsLice::DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend)
{
  const float ds = GetDisplayScale();
  LICE_IBitmap* pLB = (LICE_IBitmap*) bitmap.GetRawBitmap();
  
  int W = bitmap.W() * ds;
  int H = bitmap.H() * ds;
  int destX = (destCtrX * ds) - W / 2;
  int destY = (destCtrY * ds) - H / 2;
  
  LICE_RotatedBlit(mDrawBitmap, pLB, destX, destY, W, H, 0.0f, 0.0f, (float) W, (float) H, (float) DegToRad(angle), false, BlendWeight(pBlend), LiceBlendMode(pBlend) | LICE_BLIT_FILTER_BILINEAR, 0.0f, (float) yOffsetZeroDeg);
}

void IGraphicsLice::DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IBlend* pBlend)
{
  LICE_IBitmap* pBase = (LICE_IBitmap*) base.GetRawBitmap();
  LICE_IBitmap* pMask = (LICE_IBitmap*) mask.GetRawBitmap();
  LICE_IBitmap* pTop = (LICE_IBitmap*) top.GetRawBitmap();
  
  double dA = angle * PI / 180.0;
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
  LICE_Blit(mDrawBitmap, mTmpBitmap, r.L, r.T, r.L - x, r.T - y, r.R - r.L, r.B - r.T, BlendWeight(pBlend), LiceBlendMode(pBlend));
}

void IGraphicsLice::DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend)
{
  const float ds = GetDisplayScale();
  float weight = (pBlend ? pBlend->mWeight : 1.0f);
  LICE_PutPixel(mDrawBitmap, int((x * ds) + 0.5f), int((y  * ds) + 0.5f), LiceColor(color), weight, LiceBlendMode(pBlend));
}

void IGraphicsLice::DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, float thickness)
{
  const float ds = GetDisplayScale();
  LICE_FLine(mDrawBitmap, x1 * ds, y1 * ds, x2 * ds, y2 * ds, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
}

void IGraphicsLice::DrawDottedLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, float thickness)
{
  const float ds = GetDisplayScale();
  const int dash = 2 * ds;
  LICE_DashedLine(mDrawBitmap, x1 * ds, y1 * ds, x2 * ds, y2 * ds, dash, dash, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
}

void IGraphicsLice::DrawTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend, float)
{
  const float ds = GetDisplayScale();
  LICE_FLine(mDrawBitmap, x1 * ds, y1 * ds, x2 * ds, y2 * ds, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
  LICE_FLine(mDrawBitmap, x2 * ds, y2 * ds, x3 * ds, y3 * ds, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
  LICE_FLine(mDrawBitmap, x3 * ds, y3 * ds, x1 * ds, y1 * ds, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
}

void IGraphicsLice::DrawRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float)
{
  const float ds = GetDisplayScale();
  IRECT r = bounds;
  r.Scale(ds);
    
  LICE_FLine(mDrawBitmap, r.L, r.T, r.R, r.T, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
  LICE_FLine(mDrawBitmap, r.R, r.T, r.R, r.B, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
  LICE_FLine(mDrawBitmap, r.L, r.B, r.R, r.B, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
  LICE_FLine(mDrawBitmap, r.L, r.T, r.L, r.B, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
}

void IGraphicsLice::DrawRoundRect(const IColor& color, const IRECT& bounds, float cr, const IBlend* pBlend, float)
{
  const float ds = GetDisplayScale();
  //TODO: review floating point input support
  IRECT r = bounds;
  r.Scale(ds);
  LICE_RoundRect(mDrawBitmap, r.L, r.T, r.W(), r.H(), cr * ds, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
}

void IGraphicsLice::DrawConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend, float)
{
  const float ds = GetDisplayScale();
  
  for (int i = 0; i < npoints - 1; i++)
    LICE_FLine(mDrawBitmap, x[i] * ds, y[i] * ds, x[i+1] * ds, y[i+1] * ds, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);

  LICE_FLine(mDrawBitmap, x[npoints - 1] * ds, y[npoints - 1] * ds, x[0] * ds, y[0] * ds, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
}

void IGraphicsLice::DrawArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax, const IBlend* pBlend, float thickness)
{
  const float ds = GetDisplayScale();
  LICE_Arc(mDrawBitmap, cx * ds, cy * ds, r * ds, DegToRad(aMin), DegToRad(aMax), LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
}

void IGraphicsLice::DrawCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend, float)
{
  const float ds = GetDisplayScale();
  LICE_Circle(mDrawBitmap, cx * ds, cy * ds, r * ds, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
}

void IGraphicsLice::DrawDottedRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend, float)
{
  //TODO: review floating point input support
  const float ds = GetDisplayScale();
  IRECT r = bounds;
  r.Scale(ds);
  const int dash = 2 * ds;
  LICE_DashedLine(mDrawBitmap, r.L, r.T, r.R, r.T, dash, dash, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
  LICE_DashedLine(mDrawBitmap, r.L, r.B, r.R, r.B, dash, dash, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
  LICE_DashedLine(mDrawBitmap, r.L, r.T, r.L, r.B, dash, dash, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
  LICE_DashedLine(mDrawBitmap, r.R, r.T, r.R, r.B, dash, dash, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
}

void IGraphicsLice::FillTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend)
{
  //TODO: review floating point input support
  const float ds = GetDisplayScale();
  LICE_FillTriangle(mDrawBitmap, x1 * ds, y1 * ds, x2 * ds, y2 * ds, x3 * ds, y3 * ds, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend));
}

void IGraphicsLice::FillRect(const IColor& color, const IRECT& bounds, const IBlend* pBlend)
{
  //TODO: review floating point input support
  const float ds = GetDisplayScale();
  IRECT r = bounds;
  r.Scale(ds);
  LICE_FillRect(mDrawBitmap, r.L, r.T, r.W(), r.H(), LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend));
}

void IGraphicsLice::FillRoundRect(const IColor& color, const IRECT& bounds, float cr, const IBlend* pBlend)
{
  //TODO: review floating point input support
  const float ds = GetDisplayScale();
  IRECT r = bounds;
  r.Scale(ds);
  
  float x1 = r.L;
  float y1 = r.T;
  float h = r.H();
  float w = r.W();
  
  cr *= ds;
  
  int mode = LiceBlendMode(pBlend);
  float weight = BlendWeight(pBlend);
  LICE_pixel lcolor = LiceColor(color);
  
  LICE_FillRect(mDrawBitmap, x1+cr, y1, w-2.f*cr, h, lcolor, weight, mode);
  LICE_FillRect(mDrawBitmap, x1, y1+cr, cr, h-2.f*cr,lcolor, weight, mode);
  LICE_FillRect(mDrawBitmap, x1+w-cr, y1+cr, cr, h-2*cr, lcolor, weight, mode);
  
  //void LICE_FillCircle(LICE_IBitmap* dest, float cx, float cy, float r, LICE_pixel color, float alpha, int mode)
  LICE_FillCircle(mDrawBitmap, x1+cr, y1+cr, cr, lcolor, weight, mode, true);
  LICE_FillCircle(mDrawBitmap, x1+w-cr-1, y1+h-cr-1, cr, lcolor, weight, mode, true);
  LICE_FillCircle(mDrawBitmap, x1+w-cr-1, y1+cr, cr, lcolor, weight, mode, true);
  LICE_FillCircle(mDrawBitmap, x1+cr, y1+h-cr-1, cr, lcolor, weight, mode, true);
}

void IGraphicsLice::FillConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend)
{
  const float ds = GetDisplayScale();
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
    xpoints[i] = x[i] * ds;
    ypoints[i] = y[i] * ds;
  }
    
  LICE_FillConvexPolygon(mDrawBitmap, xpoints, ypoints, npoints, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend));
    
  if (npoints > 512)
    delete[] xpoints;
}

void IGraphicsLice::FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend)
{
  const float ds = GetDisplayScale();
  LICE_FillCircle(mDrawBitmap, cx * ds, cy * ds, r * ds, LiceColor(color), BlendWeight(pBlend), LiceBlendMode(pBlend), true);
}

void IGraphicsLice::FillArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax,  const IBlend* pBlend)
{
  float xarray[361];
  float yarray[361];
  
  aMin = DegToRad(aMin-90.f);
  aMax = DegToRad(aMax-90.f);
  
  int arcpoints = 360.0 * std::min(1., aMax - aMin / 2.f * PI);
  double arcincrement = (aMax - aMin) / arcpoints;
  for(int i = 0; i < arcpoints; i++)
  {
    xarray[i] = cx + cosf(i * arcincrement + aMin) * r;
    yarray[i] = cy + sinf(i * arcincrement + aMin) * r;
  }
    
  xarray[arcpoints] = cx;
  yarray[arcpoints] = cy;

  FillConvexPolygon(color, xarray, yarray, arcpoints + 1, pBlend);
  //TODO:
}

IColor IGraphicsLice::GetPoint(int x, int y)
{
  const float ds = GetDisplayScale();
  LICE_pixel pix = LICE_GetPixel(mDrawBitmap, x * ds, y * ds);
  return IColor(LICE_GETA(pix), LICE_GETR(pix), LICE_GETG(pix), LICE_GETB(pix));
}

bool IGraphicsLice::DrawText(const IText& text, const char* str, IRECT& bounds, bool measure)
{
  const float ds = GetDisplayScale();
  if (!str || str[0] == '\0')
  {
    return true;
  }
  
  LICE_IFont* font = text.mCached;
    
  if (!font || text.mCachedScale != ds)
  {
    font = CacheFont(text, ds);
    if (!font) return false;
  }
  
  LICE_pixel color = LiceColor(text.mFGColor);
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
#ifdef OS_MAC
    font->SWELL_DrawText(mDrawBitmap, str, -1, &R, fmt);
#else
    font->DrawTextA(mDrawBitmap, str, -1, &R, fmt);
#endif
    if( text.mAlign == IText::kAlignNear)
    {
      bounds.R = R.right;
    }
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
#ifdef OS_MAC
    font->SWELL_DrawText(mDrawBitmap, str, -1, &R, fmt);
#else
    font->DrawTextA(mDrawBitmap, str, -1, &R, fmt);
#endif
  }
  
  return true;
}

LICE_IFont* IGraphicsLice::CacheFont(const IText& text, double scale)
{
  WDL_String hashStr(text.mFont);
  hashStr.AppendFormatted(50, "-%d-%d-%d", text.mSize, text.mOrientation, text.mStyle);
    
  LICE_CachedFont* font = (LICE_CachedFont*)s_fontCache.Find(hashStr.Get(), scale);
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
    s_fontCache.Add(font, hashStr.Get(), scale);
  }
  text.mCached = font;
  text.mCachedScale = scale;
  return font;
}

bool IGraphicsLice::MeasureText(const IText& text, const char* str, IRECT& bounds)
{
  return DrawText(text, str, bounds, true);
}

APIBitmap* IGraphicsLice::LoadAPIBitmap(const WDL_String& resourcePath, int scale)
{
  const char *path = resourcePath.Get();
    
  bool ispng = strstr(path, "png") != nullptr;
#ifdef OS_MAC
  if (ispng) return new LICEBitmap(LICE_LoadPNG(path), scale);
#else //OS_WIN
  if (ispng) return new LICEBitmap(LICE_LoadPNGFromResource((HINSTANCE) GetPlatformInstance(), path, 0), scale);
#endif

#ifdef IPLUG_JPEG_SUPPORT
  bool isjpg = (strstr(path, "jpg") != nullptr) && (strstr(path, "jpeg") != nullptr);
#ifdef OS_MAC
  if (isjpg) return new LICEBitmap(LICE_LoadJPG(path), scale);
#else //OS_WIN
  if (isjpg) return new LICEBitmap(LICE_LoadJPGFromResource((HINSTANCE)GetPlatformInstance(), path, 0), scale);
#endif
#endif

  return new APIBitmap();
}

void IGraphicsLice::RenderDrawBitmap()
{
#ifdef OS_MAC

#ifdef IGRAPHICS_MAC_BLIT_BENCHMARK
  double tm=gettm();
#endif
    
  CGImageRef img = NULL;
  CGRect r = CGRectMake(0, 0, Width(), Height());

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
    CGContextDrawImage((CGContext*) GetPlatformContext(), r, img);
    CGImageRelease(img);
  }
    
#ifdef IGRAPHICS_MAC_BLIT_BENCHMARK
    printf("blit %fms\n",(gettm()-tm)*1000.0);
#endif
    
#else // OS_WIN
  PAINTSTRUCT ps;
  HWND hWnd = (HWND) GetWindow();
  HDC dc = BeginPaint(hWnd, &ps);
  
  if (GetScale() == 1.0)
 	  BitBlt(dc, 0, 0, Width(), Height(), mDrawBitmap->getDC(), 0, 0, SRCCOPY);
  else
	  StretchBlt(dc, 0, 0, WindowWidth(), WindowHeight(), mDrawBitmap->getDC(), 0, 0, Width(), Height(), SRCCOPY);
  
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

