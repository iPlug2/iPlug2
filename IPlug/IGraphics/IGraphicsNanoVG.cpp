#include <cmath>

#include "IGraphicsNanoVG.h"
#include "Log.h"

#pragma mark -

struct NanoVGBitmap {
  int idx = -1;
  int w = 0;
  int h = 0;
  NVGcontext* mVG;
  
  NanoVGBitmap(NVGcontext* context, const char* path, double sourceScale)
  {
    mVG = context;
    
    idx = nvgCreateImage(mVG, path, 0);
    nvgImageSize(mVG, idx, &w, &h);
    w /= sourceScale;
    h /= sourceScale;
  }
  
  ~NanoVGBitmap()
  {
    nvgDeleteImage(mVG, idx);
  }
};

IGraphicsNanoVG::IGraphicsNanoVG(IPlugBaseGraphics& plug, int w, int h, int fps)
: IGraphics(plug, w, h, fps)
{
}

IGraphicsNanoVG::~IGraphicsNanoVG() 
{
  mBitmaps.Empty(true);
  
#ifdef OS_OSX
  if(mVG)
    nvgDeleteMTL(mVG);
#endif
}

IBitmap IGraphicsNanoVG::LoadIBitmap(const char* name, int nStates, bool framesAreHoriztonal, double sourceScale)
{
  WDL_String fullPath;
  OSLoadBitmap(name, fullPath);
  
  NanoVGBitmap* nvgbmp = new NanoVGBitmap(mVG, fullPath.Get(), sourceScale);
  mBitmaps.Add(nvgbmp);
  return IBitmap(nvgbmp, nvgbmp->w, nvgbmp->h, nStates, framesAreHoriztonal, sourceScale, name);
}

void IGraphicsNanoVG::ReleaseIBitmap(IBitmap& bitmap)
{
}

void IGraphicsNanoVG::RetainIBitmap(IBitmap& bitmap, const char * cacheName)
{
}

IBitmap IGraphicsNanoVG::ScaleIBitmap(const IBitmap& bitmap, const char* name, double targetScale)
{
  return bitmap;
}

IBitmap IGraphicsNanoVG::CropIBitmap(const IBitmap& bitmap, const IRECT& rect, const char* name, double targetScale)
{
  return bitmap;
}

void IGraphicsNanoVG::ViewInitialized(void* layer)
{
#ifdef OS_OSX
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

void IGraphicsNanoVG::PrepDraw()
{
}

void IGraphicsNanoVG::ReScale()
{
//   mDrawBitmap->resize(Width() * mDisplayScale, Height() * mDisplayScale);
  IGraphics::ReScale(); // will cause all the controls to update their bitmaps
}

void IGraphicsNanoVG::DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend)
{
  NanoVGBitmap* pBmp = (NanoVGBitmap*) bitmap.mData;
  NVGpaint imgPaint = nvgImagePattern(mVG, dest.L - srcX, dest.T - srcY, bitmap.W, bitmap.H, 0.f, pBmp->idx, NanoVGWeight(pBlend));
  nvgBeginPath(mVG);
  nvgRect(mVG, dest.L, dest.T, dest.W(), dest.H());
  nvgFillPaint(mVG, imgPaint);
  nvgFill(mVG);
}

void IGraphicsNanoVG::DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend)
{
  NanoVGBitmap* pBmp = (NanoVGBitmap*) bitmap.mData;
  NVGpaint imgPaint = nvgImagePattern(mVG, destCtrX, destCtrY, bitmap.W, bitmap.H, angle, pBmp->idx, NanoVGWeight(pBlend));
  nvgBeginPath(mVG);
  nvgRect(mVG, destCtrX, destCtrY, destCtrX + bitmap.W, destCtrX + bitmap.H);
  nvgFillPaint(mVG, imgPaint);
  nvgFill(mVG);
}

void IGraphicsNanoVG::DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IBlend* pBlend)
{
}

void IGraphicsNanoVG::DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend, bool aa)
{
  nvgBeginPath(mVG);
  nvgCircle(mVG, x, y, 0.01);
  nvgStrokeColor(mVG, NanoVGColor(color, pBlend));
  nvgStroke(mVG);
}

void IGraphicsNanoVG::ForcePixel(const IColor& color, int x, int y)
{
}

void IGraphicsNanoVG::DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend, bool aa)
{
  nvgBeginPath(mVG);
  nvgMoveTo(mVG, x1, y1);
  nvgLineTo(mVG, x2, y2);
  nvgStrokeColor(mVG, NanoVGColor(color, pBlend));
  nvgStroke(mVG);
}

void IGraphicsNanoVG::DrawArc(const IColor& color, float cx, float cy, float r, float minAngle, float maxAngle, const IBlend* pBlend, bool aa)
{
  nvgBeginPath(mVG);
  nvgArc(mVG, cx, cy, r, minAngle, maxAngle, 0);
  nvgStrokeColor(mVG, NanoVGColor(color, pBlend));
  nvgStroke(mVG);
}

void IGraphicsNanoVG::DrawCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend, bool aa)
{
  nvgBeginPath(mVG);
  nvgCircle(mVG, cx, cy, r);
  nvgStrokeColor(mVG, NanoVGColor(color, pBlend));
  nvgStroke(mVG);
}

void IGraphicsNanoVG::DrawRoundRect(const IColor& color, const IRECT& rect, const IBlend* pBlend, int cr, bool aa)
{
  nvgBeginPath(mVG);
  nvgRoundedRect(mVG, rect.L, rect.T, rect.W(), rect.H(), cr);
  nvgStrokeColor(mVG, NanoVGColor(color, pBlend));
  nvgStroke(mVG);
}

void IGraphicsNanoVG::FillRoundRect(const IColor& color, const IRECT& rect, const IBlend* pBlend, int cr, bool aa)
{
  nvgBeginPath(mVG);
  nvgRoundedRect(mVG, rect.L, rect.T, rect.W(), rect.H(), cr);
  nvgFillColor(mVG, NanoVGColor(color, pBlend));
  nvgFill(mVG);
}

void IGraphicsNanoVG::FillIRect(const IColor& color, const IRECT& rect, const IBlend* pBlend)
{
  nvgBeginPath(mVG);
  nvgRect(mVG, rect.L, rect.T, rect.W(), rect.H());
  nvgFillColor(mVG, NanoVGColor(color, pBlend));
  nvgFill(mVG);
}

void IGraphicsNanoVG::FillCircle(const IColor& color, int cx, int cy, float r, const IBlend* pBlend, bool aa)
{
  nvgBeginPath(mVG);
  nvgCircle(mVG, cx, cy, r);
  nvgFillColor(mVG, NanoVGColor(color, pBlend));
  nvgFill(mVG);
}

void IGraphicsNanoVG::FillTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IBlend* pBlend)
{
//  nvgFillColor(mVG, NanoVGColor(color, pBlend));
//  nvgFill(mVG);
}

void IGraphicsNanoVG::FillIConvexPolygon(const IColor& color, int* x, int* y, int npoints, const IBlend* pBlend)
{
  nvgBeginPath(mVG);
  nvgMoveTo(mVG, x[0], y[0]);
  
  for(int i = 1; i < npoints; i++)
    nvgLineTo(mVG, x[i], y[i]);
  
  nvgFillColor(mVG, NanoVGColor(color, pBlend));
  nvgFill(mVG);
}

IColor IGraphicsNanoVG::GetPoint(int x, int y)
{
  return COLOR_BLACK; //TODO:
}

void IGraphicsNanoVG::DrawVerticalLine(const IColor& color, int xi, int yLo, int yHi)
{
  nvgBeginPath(mVG);
  nvgMoveTo(mVG, xi, yLo);
  nvgLineTo(mVG, xi, yHi);
  nvgStrokeColor(mVG, NanoVGColor(color));
  nvgStroke(mVG);
}

void IGraphicsNanoVG::DrawHorizontalLine(const IColor& color, int yi, int xLo, int xHi)
{
  nvgBeginPath(mVG);
  nvgMoveTo(mVG, xLo, yi);
  nvgLineTo(mVG, xHi, yi);
  nvgStrokeColor(mVG, NanoVGColor(color));
  nvgStroke(mVG);
}

bool IGraphicsNanoVG::DrawIText(const IText& text, const char* str, IRECT& rect, bool measure)
{
  return true;
}

bool IGraphicsNanoVG::MeasureIText(const IText& text, const char* str, IRECT& destRect)
{
  return true;
}

void IGraphicsNanoVG::RenderAPIBitmap(void *pContext)
{
  //TODO: change this api
}
