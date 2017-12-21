#include <cmath>

#include "png.h"

#include "IGraphicsCairo.h"
#include "IControl.h"
#include "Log.h"

struct CairoBitmap {
  cairo_surface_t* surface = nullptr;
  int width = 0;
  int height = 0;
  
  CairoBitmap(cairo_surface_t* s)
  {
    surface = s;

    width = cairo_image_surface_get_width (s);
    height = cairo_image_surface_get_height (s);
  }
  
  ~CairoBitmap()
  {
    cairo_surface_destroy(surface);
  }
};

static StaticStorage<CairoBitmap> s_bitmapCache;

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
  const double targetScale = GetDisplayScale(); // targetScale = what this screen is

  CairoBitmap* pCB = s_bitmapCache.Find(name, targetScale);

  if (!pCB) // if bitmap not in cache allready at targetScale
  {
    WDL_String fullPath;
    OSLoadBitmap(name, fullPath);

    cairo_surface_t* pSurface = cairo_image_surface_create_from_png(fullPath.Get());

#ifndef NDEBUG
    bool imgResourceFound = pSurface;
#endif
    assert(imgResourceFound); // Protect against typos in resource.h and .rc files.
    
    pCB = new CairoBitmap(pSurface);

    const IBitmap bitmap(pCB->surface, pCB->width / sourceScale, pCB->height / sourceScale, nStates, framesAreHoriztonal, sourceScale, name);

    if (sourceScale != targetScale) {
      return ScaleIBitmap(bitmap, name, targetScale); // will add to cache
    }
    else {
      s_bitmapCache.Add(pCB, name, sourceScale);
      return IBitmap(pCB->surface, pCB->width / sourceScale, pCB->height / sourceScale, nStates, framesAreHoriztonal, sourceScale, name);
    }
  }

  // if bitmap allready cached at scale
  // TODO: this is horribly hacky
  if(targetScale > 1.)
    return IBitmap(pCB->surface, pCB->width / targetScale, pCB->height / targetScale, nStates, framesAreHoriztonal, sourceScale, name);
  else
    return IBitmap(pCB->surface, pCB->width, pCB->height, nStates, framesAreHoriztonal, sourceScale, name);
}

void IGraphicsCairo::ReleaseIBitmap(IBitmap& bitmap)
{
}

void IGraphicsCairo::RetainIBitmap(IBitmap& bitmap, const char * cacheName)
{
}

IBitmap IGraphicsCairo::ScaleIBitmap(const IBitmap& inBitmap, const char* name, double targetScale)
{
  int newW = (int)(inBitmap.W * targetScale);
  int newH = (int)(inBitmap.H * targetScale);
  
  // Convert output to cairo
  cairo_surface_t* pOutSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, newW, newH);
  cairo_t* pOutContext = cairo_create(pOutSurface);

  cairo_set_source_surface(pOutContext, (cairo_surface_t*)inBitmap.mData, 0, 0);
  cairo_scale(pOutContext, 1. / targetScale, 1. / targetScale);
  cairo_paint(pOutContext);

  CairoBitmap* pCB = new CairoBitmap(pOutSurface);
  
  s_bitmapCache.Add(pCB, name, targetScale);
  
  cairo_destroy(pOutContext);

  return IBitmap(pCB->surface, inBitmap.W, inBitmap.H, inBitmap.N, inBitmap.mFramesAreHorizontal, inBitmap.mSourceScale, name);
}

IBitmap IGraphicsCairo::CropIBitmap(const IBitmap& inBitmap, const IRECT& rect, const char* name, double targetScale)
{
  int newW = (int)(inBitmap.W * targetScale);
  int newH = (int)(inBitmap.H * targetScale);
  
  unsigned char* pOutBuffer = new unsigned char[newW * newH * 4];
  IBitmap outBitmap(pOutBuffer, newW, newH);
  
  // Convert output to cairo
  cairo_surface_t* pOutSurface = cairo_image_surface_create_for_data((unsigned char*) outBitmap.mData, CAIRO_FORMAT_ARGB32, outBitmap.W, outBitmap.H, 0);
  cairo_t* pOutContext = cairo_create(pOutSurface);
  
  // Paint from one surface to another
  cairo_set_source_surface(pOutContext, (cairo_surface_t*) inBitmap.mData, 0, 0);
  cairo_rectangle(pOutContext, rect.L, rect.T, rect.W(), rect.H());
  cairo_scale(pOutContext, 1.0 / targetScale, 1.0 / targetScale);
  cairo_paint(pOutContext);
  
  // Destroy cairo stuff
  cairo_destroy(pOutContext);
  
  return outBitmap; //TODO: surface will not be destroyed, unless this is retained
}

void IGraphicsCairo::PrepDraw()
{
// not sure if needed yet may change api
}

void IGraphicsCairo::ReScale()
{
  IGraphics::ReScale(); // will cause all the controls to update their bitmaps
}

void IGraphicsCairo::DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IChannelBlend* pBlend)
{
  ClipRegion(dest);
  cairo_save(mContext);
  cairo_surface_t* surface = (cairo_surface_t*) bitmap.mData;
  cairo_scale(mContext, 1./bitmap.mSourceScale, 1./bitmap.mSourceScale);
  
  if(bitmap.mSourceScale > 1.) //TODO: check on non retina/mixed
    cairo_translate(mContext, dest.L, dest.T);
  
  cairo_set_source_surface(mContext, surface, dest.L - (srcX * bitmap.mSourceScale), dest.T - (srcY * bitmap.mSourceScale));
  cairo_set_operator(mContext, CairoBlendMode(pBlend));
  cairo_paint_with_alpha(mContext, CairoWeight(pBlend));
//  cairo_paint(mContext);
  cairo_restore(mContext);
  ResetClipRegion();
}

void IGraphicsCairo::DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IChannelBlend* pBlend)
{
  //TODO:

}

void IGraphicsCairo::DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IChannelBlend* pBlend)
{
  //TODO:

}

void IGraphicsCairo::DrawPoint(const IColor& color, float x, float y, const IChannelBlend* pBlend, bool aa)
{
  SetCairoSourceRGBA(color, pBlend);
  
  if (!aa)
  {
    x = floor(x);
    y = floor(y);
  }
  
  cairo_move_to(mContext, x + 0.5, y + 0.5);
  cairo_line_to(mContext, x + 1.5, y + 1.5);
  cairo_set_line_width(mContext, 1);
  cairo_stroke(mContext);
}

void IGraphicsCairo::ForcePixel(const IColor& color, int x, int y)
{
  SetCairoSourceRGBA(color);
  
  cairo_move_to(mContext, x + 0.5, y + 0.5);
  cairo_line_to(mContext, x + 1.5, y + 1.5);
  cairo_set_line_width(mContext, 1);
  cairo_stroke(mContext);
}

void IGraphicsCairo::DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IChannelBlend* pBlend, bool aa)
{
  SetCairoSourceRGBA(color, pBlend);
  cairo_set_line_width(mContext, 1);
  cairo_move_to (mContext, x1, y1);
  cairo_line_to (mContext, x2, y2);
  cairo_set_line_width (mContext, 1);
  cairo_stroke (mContext);
}

void IGraphicsCairo::DrawRect(const IColor& color, const IRECT& rect)
{
  cairo_set_line_width(mContext, 1);
  SetCairoSourceRGBA(color);
  CairoDrawRect(rect);
  cairo_stroke(mContext);
}

void IGraphicsCairo::DrawTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IChannelBlend* pBlend)
{
  SetCairoSourceRGBA(color, pBlend); // TODO: should cairo_create_group?
  cairo_set_line_width(mContext, 1);
  cairo_new_sub_path(mContext);
  cairo_move_to(mContext, x1, y1);
  cairo_line_to(mContext, x2, y2);
  cairo_line_to(mContext, x3, y3);
  cairo_stroke(mContext);
}

void IGraphicsCairo::DrawArc(const IColor& color, float cx, float cy, float r, float minAngle, float maxAngle, const IChannelBlend* pBlend, bool aa)
{
  SetCairoSourceRGBA(color, pBlend);
  cairo_set_line_width(mContext, 1);
  cairo_arc (mContext, cx, cy, r, minAngle, maxAngle);
  cairo_stroke (mContext);
}

void IGraphicsCairo::DrawCircle(const IColor& color, float cx, float cy, float r, const IChannelBlend* pBlend, bool aa)
{
  SetCairoSourceRGBA(color, pBlend);
  cairo_set_line_width(mContext, 1);
  cairo_arc(mContext, cx, cy, r, 0, PI * 2.);
  cairo_stroke(mContext);
}

void IGraphicsCairo::DrawRoundRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend, int corner, bool aa)
{
  const double y = rect.B - rect.H(); // TODO: should cairo_create_group?
  SetCairoSourceRGBA(color, pBlend);
  cairo_new_sub_path(mContext);
  cairo_arc(mContext, rect.L + rect.W() - corner, y + corner, corner, PI * -0.5, 0);
  cairo_arc(mContext, rect.L + rect.W() - corner, y + rect.H() - corner, corner, 0, PI * 0.5);
  cairo_arc(mContext, rect.L + corner, y + rect.H() - corner, corner, PI * 0.5, PI);
  cairo_arc(mContext, rect.L + corner, y + corner, corner, PI, PI * 1.25);
  cairo_stroke(mContext);
}

void IGraphicsCairo::FillRoundRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend, int corner, bool aa)
{
  const double y = rect.B - rect.H();
  SetCairoSourceRGBA(color, pBlend); // TODO: should cairo_create_group?
  cairo_new_sub_path(mContext);
  cairo_arc(mContext, rect.L + rect.W() - corner, y + corner, corner, PI * -0.5, 0);
  cairo_arc(mContext, rect.L + rect.W() - corner, y + rect.H() - corner, corner, 0, PI * 0.5);
  cairo_arc(mContext, rect.L + corner, y + rect.H() - corner, corner, PI * 0.5, PI);
  cairo_arc(mContext, rect.L + corner, y + corner, corner, PI, PI * 1.25);
  cairo_fill(mContext);
}

void IGraphicsCairo::FillIRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend)
{
  SetCairoSourceRGBA(color, pBlend);
  CairoDrawRect(rect);
  cairo_fill(mContext);
}

void IGraphicsCairo::FillCircle(const IColor& color, int cx, int cy, float r, const IChannelBlend* pBlend, bool aa)
{
  SetCairoSourceRGBA(color, pBlend);
  cairo_arc(mContext, cx, cy, r, 0, 2 * PI);
  cairo_fill(mContext);
}

void IGraphicsCairo::FillTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IChannelBlend* pBlend)
{
  SetCairoSourceRGBA(color, pBlend); // TODO: should cairo_create_group?
  cairo_new_sub_path(mContext);
  cairo_move_to(mContext, x1, y1);
  cairo_line_to(mContext, x2, y2);
  cairo_line_to(mContext, x3, y3);
  cairo_fill(mContext);
}

void IGraphicsCairo::FillIConvexPolygon(const IColor& color, int* x, int* y, int npoints, const IChannelBlend* pBlend)
{
  SetCairoSourceRGBA(color, pBlend); // TODO: should cairo_create_group?

  cairo_new_sub_path(mContext);
  cairo_move_to(mContext, x[0], y[0]);
  
  for(int i = 1; i < npoints; i++)
    cairo_line_to(mContext, x[i], y[i]);
  
  cairo_fill(mContext);
}

IColor IGraphicsCairo::GetPoint(int x, int y)
{
  // Convert suface to cairo image surface
  cairo_surface_t* pOutSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, Width(), Height());
  cairo_t* pOutContext = cairo_create(pOutSurface);
  cairo_set_source_surface(pOutContext, mSurface, 0, 0);
  cairo_paint(pOutContext);
  
  unsigned char* pData = cairo_image_surface_get_data(pOutSurface);
  int stride = cairo_image_surface_get_stride(pOutSurface);
  
  unsigned int* pPixel = (unsigned int*)(pData + y * stride);
  pPixel += x;
  
  int A = ((*pPixel) >> 0) & 0xff;
  int R = ((*pPixel) >> 8) & 0xff;
  int G = ((*pPixel) >> 16) & 0xff;
  int B = ((*pPixel) >> 24) & 0xff;
  
  return IColor(A, R, G, B);
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
#ifdef OS_OSX
    mSurface = cairo_quartz_surface_create_for_cg_context(CGContextRef(pContext), Width() , Height());
#endif
    mContext = cairo_create(mSurface);
    cairo_scale(mContext, 1, -1);
    cairo_translate(mContext, 0., -Height());
  }
  
  IGraphics::SetPlatformContext(pContext);
}

