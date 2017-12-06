#include <cmath>

#include "IGraphicsCairo.h"
#include "IControl.h"
#include "Log.h"

#pragma mark -

IGraphicsCairo::IGraphicsCairo(IPlugBaseGraphics& plug, int w, int h, int fps)
: IGraphics(plug, w, h, fps)
, mSurface(nullptr)
, mContext(nullptr)
{
}

IGraphicsCairo::~IGraphicsCairo() 
{
  if (mContext)
    cairo_destroy(mContext);
  
  if (mSurface)
    cairo_surface_destroy(mSurface);
}

IBitmap IGraphicsCairo::LoadIBitmap(const char* name, int nStates, bool framesAreHoriztonal, double sourceScale)
{
}

void IGraphicsCairo::ReleaseIBitmap(IBitmap& bitmap)
{
}

void IGraphicsCairo::RetainIBitmap(IBitmap& bitmap, const char * cacheName)
{
}

IBitmap IGraphicsCairo::ScaleIBitmap(const IBitmap& bitmap, const char* name, double targetScale)
{
}

IBitmap IGraphicsCairo::CropIBitmap(const IBitmap& bitmap, const IRECT& rect, const char* name, double targetScale)
{
}

void IGraphicsCairo::PrepDraw()
{
// not sure if needed yet may change api
//  int w = Width() * mDisplayScale;
//  int h = Height() * mDisplayScale;
}

void IGraphicsCairo::ReScale()
{
  cairo_surface_set_device_scale(mSurface, 1./mDisplayScale, 1./mDisplayScale);
  IGraphics::ReScale(); // will cause all the controls to update their bitmaps
}

void IGraphicsCairo::DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IChannelBlend* pBlend)
{
}

void IGraphicsCairo::DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IChannelBlend* pBlend)
{
}

void IGraphicsCairo::DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IChannelBlend* pBlend)
{
}

void IGraphicsCairo::DrawPoint(const IColor& color, float x, float y, const IChannelBlend* pBlend, bool aa)
{
}

void IGraphicsCairo::ForcePixel(const IColor& color, int x, int y)
{
}

void IGraphicsCairo::DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IChannelBlend* pBlend, bool aa)
{
  SetCairoSourceRGBA(color);
  cairo_move_to (mContext, x1 * mDisplayScale, (Height() - y1) * mDisplayScale);
  cairo_line_to (mContext, x2 * mDisplayScale, (Height() - y2) * mDisplayScale);
  cairo_set_line_width (mContext, 2);
  cairo_stroke (mContext);
}

void IGraphicsCairo::DrawArc(const IColor& color, float cx, float cy, float r, float minAngle, float maxAngle, const IChannelBlend* pBlend, bool aa)
{
}

void IGraphicsCairo::DrawCircle(const IColor& color, float cx, float cy, float r, const IChannelBlend* pBlend, bool aa)
{
}

void IGraphicsCairo::RoundRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend, int cornerradius, bool aa)
{
}

void IGraphicsCairo::FillRoundRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend, int cornerradius, bool aa)
{
}

void IGraphicsCairo::FillIRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend)
{
  CairoDrawRect(rect);
  SetCairoSourceRGBA(color);
  cairo_fill(mContext);
}

void IGraphicsCairo::FillCircle(const IColor& color, int cx, int cy, float r, const IChannelBlend* pBlend, bool aa)
{
}

void IGraphicsCairo::FillTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IChannelBlend* pBlend)
{
}

void IGraphicsCairo::FillIConvexPolygon(const IColor& color, int* x, int* y, int npoints, const IChannelBlend* pBlend)
{
}

IColor IGraphicsCairo::GetPoint(int x, int y)
{
  return COLOR_BLACK; //TODO:
}

bool IGraphicsCairo::DrawIText(const IText& text, const char* str, IRECT& rect, bool measure)
{
  return true;
}

bool IGraphicsCairo::MeasureIText(const IText& text, const char* str, IRECT& destRect)
{
  return true;
}

void IGraphicsCairo::SetPlatformContext(void* pContext)
{
  if(!mSurface)
  {
    int w = Width() * mDisplayScale;
    int h = Height() * mDisplayScale;
    mSurface = cairo_quartz_surface_create_for_cg_context(CGContextRef(pContext), w , h);
    mContext = cairo_create(mSurface);
  }
  
  IGraphics::SetPlatformContext(pContext);
}

void IGraphicsCairo::RenderAPIBitmap(void *pContext)
{
//    TODO: bg color?
//    int w = Width() * mDisplayScale;
//    int h = Height() * mDisplayScale;
//    cairo_set_source_rgb(mContext, 0., 0., 0.);
//    cairo_rectangle(mContext, 0, 0, w, h);
//    cairo_fill(mContext);
}
