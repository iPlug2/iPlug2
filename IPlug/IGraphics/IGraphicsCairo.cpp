#include <cmath>

#include "png.h"

#include "IGraphicsCairo.h"
#include "IControl.h"
#include "Log.h"

struct CairoBitmap {
  cairo_surface_t* surface = nullptr;
  int width = 0;
  int height = 0;
  
  CairoBitmap(const char* path)
  {
    surface = cairo_image_surface_create_from_png(path);
    
#ifndef NDEBUG
    bool imgResourceFound = surface;
#endif
    assert(imgResourceFound); // Protect against typos in resource.h and .rc files.
    width = cairo_image_surface_get_width (surface);
    height = cairo_image_surface_get_height (surface);
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

    pCB = new CairoBitmap(fullPath.Get());

    const IBitmap bitmap(pCB->surface, pCB->width / sourceScale, pCB->height / sourceScale, nStates, framesAreHoriztonal, sourceScale, name);

//    if (sourceScale != targetScale) {
//      return ScaleIBitmap(bitmap, name, targetScale); // will add to cache
//    }
//    else {
      s_bitmapCache.Add(pCB, name, sourceScale);
      return IBitmap(pCB->surface, pCB->width / sourceScale, pCB->height / sourceScale, nStates, framesAreHoriztonal, sourceScale, name);
//    }
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

IBitmap IGraphicsCairo::ScaleIBitmap(const IBitmap& bitmap, const char* name, double targetScale)
{
  return bitmap; //TODO:!!
}

IBitmap IGraphicsCairo::CropIBitmap(const IBitmap& bitmap, const IRECT& rect, const char* name, double targetScale)
{
  return bitmap; //TODO:!!
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
  IRECT r = dest.GetFlipped(Height());
//  cairo_rectangle(mContext, r.L, r.T, r.W(), r.H());
//  cairo_clip (mContext);
//  cairo_matrix_t x_reflection_matrix;
//  cairo_matrix_init_identity(&x_reflection_matrix);
//  x_reflection_matrix.yy = 1.0;
//  cairo_translate(mContext, 0, Height());
//  cairo_set_matrix(mContext, &x_reflection_matrix);
  cairo_surface_t* surface = (cairo_surface_t*) bitmap.mData;
  cairo_set_source_surface(mContext, surface, r.L, r.B + (Height()-srcY));
  cairo_paint(mContext);
}

void IGraphicsCairo::DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IChannelBlend* pBlend)
{
}

void IGraphicsCairo::DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IChannelBlend* pBlend)
{
}

void IGraphicsCairo::DrawPoint(const IColor& color, float x, float y, const IChannelBlend* pBlend, bool aa)
{
  SetCairoSourceRGBA(color);

}

void IGraphicsCairo::ForcePixel(const IColor& color, int x, int y)
{
  SetCairoSourceRGBA(color);

}

void IGraphicsCairo::DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IChannelBlend* pBlend, bool aa)
{
  SetCairoSourceRGBA(color);
  cairo_set_line_width(mContext, 1);
  cairo_move_to (mContext, x1, (Height() - y1));
  cairo_line_to (mContext, x2, (Height() - y2));
  cairo_set_line_width (mContext, 1);
  cairo_stroke (mContext);
}

void IGraphicsCairo::DrawRect(const IColor& color, const IRECT& rect)
{
  IRECT r = rect;
  r.Scale(mDisplayScale);
  cairo_set_line_width(mContext, 1);
  SetCairoSourceRGBA(color);
  CairoDrawRect(rect);
  cairo_stroke(mContext);
}

void IGraphicsCairo::DrawTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IChannelBlend* pBlend)
{
  SetCairoSourceRGBA(color);
  cairo_set_line_width(mContext, 1);
  cairo_new_sub_path(mContext);
  cairo_move_to(mContext, x1, (Height() - y1));
  cairo_line_to(mContext, x2, (Height() - y2));
  cairo_line_to(mContext, x3, (Height() - y3));
  cairo_stroke(mContext);
}

void IGraphicsCairo::DrawArc(const IColor& color, float cx, float cy, float r, float minAngle, float maxAngle, const IChannelBlend* pBlend, bool aa)
{
  SetCairoSourceRGBA(color);
  cairo_set_line_width(mContext, 1);
  cairo_arc (mContext, cx, cy, r, minAngle, maxAngle);
  cairo_stroke (mContext);
}

void IGraphicsCairo::DrawCircle(const IColor& color, float cx, float cy, float r, const IChannelBlend* pBlend, bool aa)
{
  SetCairoSourceRGBA(color);
  cairo_set_line_width(mContext, 1);
  cairo_arc(mContext, cx, cy, r, 0, PI * 2.);
  cairo_stroke(mContext);
}

void IGraphicsCairo::RoundRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend, int corner, bool aa)
{
  IRECT r = rect;
  r.Scale(mDisplayScale);
  const double y = r.B - r.H();
  SetCairoSourceRGBA(color);
  //cairo_set_line_width(mContext, mDisplayScale);
  cairo_new_sub_path(mContext);
  cairo_arc(mContext, r.L + r.W() - corner, y + corner, corner, PI * -0.5, 0);
  cairo_arc(mContext, r.L + r.W() - corner, y + r.H() - corner, corner, 0, PI * 0.5);
  cairo_arc(mContext, r.L + corner, y + r.H() - corner, corner, PI * 0.5, PI);
  cairo_arc(mContext, r.L + corner, y + corner, corner, PI, PI * 1.25);
  cairo_stroke(mContext);
}

void IGraphicsCairo::FillRoundRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend, int corner, bool aa)
{
  IRECT r = rect;
  r.Scale(mDisplayScale);
  const double y = r.B - r.H();
  SetCairoSourceRGBA(color);
  cairo_new_sub_path(mContext);
  cairo_arc(mContext, r.L + r.W() - corner, y + corner, corner, PI * -0.5, 0);
  cairo_arc(mContext, r.L + r.W() - corner, y + r.H() - corner, corner, 0, PI * 0.5);
  cairo_arc(mContext, r.L + corner, y + r.H() - corner, corner, PI * 0.5, PI);
  cairo_arc(mContext, r.L + corner, y + corner, corner, PI, PI * 1.25);
  cairo_fill(mContext);
}

void IGraphicsCairo::FillIRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend)
{
  IRECT r = rect;
  r.Scale(mDisplayScale);
  SetCairoSourceRGBA(color);
  CairoDrawRect(rect);
  cairo_fill(mContext);
}

void IGraphicsCairo::FillCircle(const IColor& color, int cx, int cy, float r, const IChannelBlend* pBlend, bool aa)
{
  SetCairoSourceRGBA(color);
  cairo_arc(mContext, cx, cy, r, 0, 2 * PI);
  cairo_fill(mContext);
}

void IGraphicsCairo::FillTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IChannelBlend* pBlend)
{
  SetCairoSourceRGBA(color);
  cairo_new_sub_path(mContext);
  cairo_move_to(mContext, x1, (Height() - y1));
  cairo_line_to(mContext, x2, (Height() - y2));
  cairo_line_to(mContext, x3, (Height() - y3));
  cairo_fill(mContext);
}

void IGraphicsCairo::FillIConvexPolygon(const IColor& color, int* x, int* y, int npoints, const IChannelBlend* pBlend)
{
  SetCairoSourceRGBA(color);

  cairo_new_sub_path(mContext);
  cairo_move_to(mContext, x[0], (Height() - y[0]));
  
  for(int i = 1; i < npoints; i++)
    cairo_line_to(mContext, x[i], (Height() - y[i]));
  
  cairo_fill(mContext);
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
    int w = Width();
    int h = Height();
    mSurface = cairo_quartz_surface_create_for_cg_context(CGContextRef(pContext), w , h);
    mContext = cairo_create(mSurface);
  }
  
  IGraphics::SetPlatformContext(pContext);
}

void IGraphicsCairo::RenderAPIBitmap(void *pContext)
{
//    TODO: bg color?
//    int w = Width();
//    int h = Height();
//    cairo_set_source_rgb(mContext, 1., 1., 1.);
//    cairo_rectangle(mContext, 0, 0, w, h);
//    cairo_fill(mContext);
}


