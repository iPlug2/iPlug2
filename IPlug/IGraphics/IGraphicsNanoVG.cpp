#include <cmath>

#include "IGraphicsNanoVG.h"
#include "NanoVGNanoSVG.h"

#pragma mark -

inline int GetBitmapIdx(APIBitmap* pBitmap) { return (int) ((long long) pBitmap->GetBitmap()); }

NanoVGBitmap::NanoVGBitmap(NVGcontext* context, const char* path, double sourceScale)
{
  mVG = context;
  int w = 0, h = 0;
  long long idx = nvgCreateImage(mVG, path, 0);
  nvgImageSize(mVG, idx, &w, &h);
      
  SetBitmap((void *) idx, w, h, sourceScale);
}

NanoVGBitmap::~NanoVGBitmap()
{
  int idx = GetBitmapIdx(this);
  nvgDeleteImage(mVG, idx);
}
  
IGraphicsNanoVG::IGraphicsNanoVG(IGraphicsDelegate& dlg, int w, int h, int fps)
: IGraphics(dlg, w, h, fps)
{
}

IGraphicsNanoVG::~IGraphicsNanoVG() 
{
  mBitmaps.Empty(true);
  
#ifdef OS_MAC
  if(mVG)
    nvgDeleteMTL(mVG);
#endif
}

IBitmap IGraphicsNanoVG::LoadBitmap(const char* name, int nStates, bool framesAreHorizontal)
{
  WDL_String fullPath;
  const int targetScale = round(GetDisplayScale());
  int sourceScale = 0;
  bool resourceFound = SearchImageResource(name, "png", fullPath, targetScale, sourceScale);
  assert(resourceFound);
    
  NanoVGBitmap* bitmap = (NanoVGBitmap*) LoadAPIBitmap(fullPath, sourceScale);
  assert(bitmap);
  mBitmaps.Add(bitmap);
  
  return IBitmap(bitmap, nStates, framesAreHorizontal, name);
}

APIBitmap* IGraphicsNanoVG::LoadAPIBitmap(const WDL_String& resourcePath, int scale)
{
  return new NanoVGBitmap(mVG, resourcePath.Get(), scale);
}

APIBitmap* IGraphicsNanoVG::ScaleAPIBitmap(const APIBitmap* pBitmap, int scale)
{
  return nullptr;
}

void IGraphicsNanoVG::RetainBitmap(const IBitmap& bitmap, const char * cacheName)
{
}

IBitmap IGraphicsNanoVG::ScaleBitmap(const IBitmap& bitmap, const char* name, int targetScale)
{
  return bitmap;
}
/*
IBitmap IGraphicsNanoVG::CropBitmap(const IBitmap& bitmap, const IRECT& rect, const char* name, int targetScale)
{
  return bitmap;
}*/

void IGraphicsNanoVG::ViewInitialized(void* layer)
{
#ifdef OS_MAC
  mVG = nvgCreateMTL(layer, NVG_ANTIALIAS | NVG_STENCIL_STROKES);
#endif
}

void IGraphicsNanoVG::BeginFrame()
{
  nvgBeginFrame(mVG, Width(), Height(), GetDisplayScale());
}

void IGraphicsNanoVG::EndFrame()
{
  nvgEndFrame(mVG);
}

void IGraphicsNanoVG::DrawSVG(ISVG& svg, const IRECT& dest, const IBlend* pBlend)
{
  nvgSave(mVG);
  nvgTranslate(mVG, dest.L, dest.T);

  float xScale = dest.W() / svg.W();
  float yScale = dest.H() / svg.H();
  float scale = xScale < yScale ? xScale : yScale;
    
  nvgScale(mVG, scale, scale);
    
  NanoVGNanoSVGRender::RenderNanoSVG(mVG, svg.mImage);

  nvgRestore(mVG);
}

void IGraphicsNanoVG::DrawRotatedSVG(ISVG& svg, float destCtrX, float destCtrY, float width, float height, double angle, const IBlend* pBlend)
{
  nvgSave(mVG);
  nvgTranslate(mVG, destCtrX, destCtrY);
  nvgRotate(mVG, DegToRad(angle));
  DrawSVG(svg, IRECT(-width * 0.5, - height * 0.5, width * 0.5, height * 0.5), pBlend);
  nvgRestore(mVG);
}

void IGraphicsNanoVG::DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend)
{
  int idx = GetBitmapIdx(bitmap.GetAPIBitmap());
  NVGpaint imgPaint = nvgImagePattern(mVG, std::round(dest.L) - srcX, std::round(dest.T) - srcY, bitmap.W(), bitmap.H(), 0.f, idx, BlendWeight(pBlend));
  nvgBeginPath(mVG);
  nvgRect(mVG, dest.L, dest.T, dest.W(), dest.H());
  nvgFillPaint(mVG, imgPaint);
  nvgFill(mVG);
}

void IGraphicsNanoVG::DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend)
{
  int idx = GetBitmapIdx(bitmap.GetAPIBitmap());
  NVGpaint imgPaint = nvgImagePattern(mVG, destCtrX, destCtrY, bitmap.W(), bitmap.H(), angle, idx, BlendWeight(pBlend));
  nvgBeginPath(mVG);
  nvgRect(mVG, destCtrX, destCtrY, destCtrX + bitmap.W(), destCtrX + bitmap.H());
  nvgFillPaint(mVG, imgPaint);
  nvgFill(mVG);
}

void IGraphicsNanoVG::DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IBlend* pBlend)
{
  //TODO:
}

inline void IGraphicsNanoVG::NVGDrawTriangle(float x1, float y1, float x2, float y2, float x3, float y3)
{
  nvgBeginPath(mVG);
  nvgMoveTo(mVG, x1, y1);
  nvgLineTo(mVG, x2, y2);
  nvgLineTo(mVG, x3, y3);
  nvgClosePath(mVG);
}

inline void IGraphicsNanoVG::NVGDrawConvexPolygon(float* x, float* y, int npoints)
{
  nvgBeginPath(mVG);
  nvgMoveTo(mVG, x[0], y[0]);
  for(int i = 1; i < npoints; i++)
    nvgLineTo(mVG, x[i], y[i]);
  nvgClosePath(mVG);
}

void IGraphicsNanoVG::DrawDottedRect(const IColor& color, const IRECT& rect, const IBlend* pBlend)
{
  //TODO: NanoVG doesn't do dots
  DrawRect(color, rect, pBlend);
}

void IGraphicsNanoVG::DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend)
{
  nvgBeginPath(mVG);
  nvgCircle(mVG, x, y, 0.01); // TODO:  0.01 - is there a better way to draw a point?
  Stroke(color, pBlend);
}

void IGraphicsNanoVG::ForcePixel(const IColor& color, int x, int y)
{
  //TODO:
}

void IGraphicsNanoVG::DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend)
{
  nvgBeginPath(mVG);
  nvgMoveTo(mVG, x1, y1);
  nvgLineTo(mVG, x2, y2);
  Stroke(color, pBlend);
}

void IGraphicsNanoVG::DrawTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend)
{
  NVGDrawTriangle(x1, y1, x2, y2, x3, y3);
  Stroke(color, pBlend);
}

void IGraphicsNanoVG::DrawRect(const IColor &color, const IRECT &rect, const IBlend *pBlend)
{
  nvgBeginPath(mVG);
  nvgRect(mVG, rect.L, rect.T, rect.W(), rect.H());
  Stroke(color, pBlend);
}

void IGraphicsNanoVG::DrawRoundRect(const IColor& color, const IRECT& rect, float cr, const IBlend* pBlend)
{
  nvgBeginPath(mVG);
  nvgRoundedRect(mVG, rect.L, rect.T, rect.W(), rect.H(), cr);
  Stroke(color, pBlend);
}

void IGraphicsNanoVG::DrawConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend)
{
  NVGDrawConvexPolygon(x, y, npoints);
  Stroke(color, pBlend);
}

void IGraphicsNanoVG::DrawArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax, const IBlend* pBlend)
{
  nvgBeginPath(mVG);
  nvgArc(mVG, cx, cy, r, DegToRad(aMin-90.f), DegToRad(aMax-90.f), NVG_CW);
  Stroke(color, pBlend);
}

void IGraphicsNanoVG::DrawCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend)
{
  nvgBeginPath(mVG);
  nvgCircle(mVG, cx, cy, r);
  Stroke(color, pBlend);
}

void IGraphicsNanoVG::FillTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend)
{
  NVGDrawTriangle(x1, y1, x2, y2, x3, y3);
  Fill(color, pBlend);
}

void IGraphicsNanoVG::FillRect(const IColor& color, const IRECT& rect, const IBlend* pBlend)
{
  nvgBeginPath(mVG);
  nvgRect(mVG, rect.L, rect.T, rect.W(), rect.H());
  Fill(color, pBlend);
}

void IGraphicsNanoVG::FillRoundRect(const IColor& color, const IRECT& rect, float cr, const IBlend* pBlend)
{
  nvgBeginPath(mVG);
  nvgRoundedRect(mVG, rect.L, rect.T, rect.W(), rect.H(), cr);
  Fill(color, pBlend);
}

void IGraphicsNanoVG::FillConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend)
{
  NVGDrawConvexPolygon(x, y, npoints);
  Fill(color, pBlend);
}

void IGraphicsNanoVG::FillArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax, const IBlend* pBlend)
{
  nvgBeginPath(mVG);
  nvgMoveTo(mVG, cx, cy);
  nvgArc(mVG, cx, cy, r, DegToRad(aMin-90.f), DegToRad(aMax-90.f), NVG_CW);
  nvgClosePath(mVG);
  Fill(color, pBlend);
}

void IGraphicsNanoVG::FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend)
{
  nvgBeginPath(mVG);
  nvgCircle(mVG, cx, cy, r);
  Fill(color, pBlend);
}

IColor IGraphicsNanoVG::GetPoint(int x, int y)
{
  return COLOR_BLACK; //TODO:
}

bool IGraphicsNanoVG::DrawText(const IText& text, const char* str, IRECT& rect, bool measure)
{
  return true;
}

bool IGraphicsNanoVG::MeasureText(const IText& text, const char* str, IRECT& destRect)
{
  return DrawText(text, str, destRect, true);
}
