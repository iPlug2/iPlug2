#include <cmath>

#include "png.h"

#include "IGraphicsCairo.h"
#include "CairoNanoSVG.h"

#ifdef OS_MAC
cairo_surface_t* LoadPNGResource(void* hInst, const WDL_String& path)
{
  return cairo_image_surface_create_from_png(path.Get());
}
#else //OS_WIN
class PNGStreamReader
{
public:
  PNGStreamReader(HMODULE hInst, const WDL_String &path)
  : mData(nullptr), mSize(0), mCount(0)
  {
    HRSRC resInfo = FindResource(hInst, path.Get(), "PNG");
    if (resInfo)
    {
      HGLOBAL res = LoadResource(hInst, resInfo);
      if (res)
      {
        mData = (uint8_t *) LockResource(res);
        mSize = SizeofResource(hInst, resInfo);
      }
    }
  }

  cairo_status_t Read(uint8_t* data, uint32_t length)
  {
    mCount += length;
    if (mCount <= mSize)
    {
      memcpy(data, mData + mCount - length, length);
      return CAIRO_STATUS_SUCCESS;
    }

    return CAIRO_STATUS_READ_ERROR;
  }

  static cairo_status_t StaticRead(void *reader, uint8_t *data, uint32_t length)
  {
    return ((PNGStreamReader*)reader)->Read(data, length);
  }
  
private:
  const uint8_t* mData;
  size_t mCount;
  size_t mSize;
};

cairo_surface_t* LoadPNGResource(void* hInst, const WDL_String& path)
{
  PNGStreamReader reader((HMODULE) hInst, path);
  return cairo_image_surface_create_from_png_stream(&PNGStreamReader::StaticRead, &reader);
}
#endif //OS_WIN

struct CairoBitmap 
{
  cairo_surface_t* surface = nullptr;
  int width = 0;
  int height = 0;
  
  CairoBitmap(cairo_surface_t* s, double scale)
  {
    surface = s;
    cairo_surface_set_device_scale(s, scale, scale);
    width = cairo_image_surface_get_width(s);
    height = cairo_image_surface_get_height(s);
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

IBitmap IGraphicsCairo::LoadBitmap(const char* name, int nStates, bool framesAreHoriztonal, double sourceScale)
{
  const double targetScale = GetDisplayScale(); // targetScale = what this screen is

  CairoBitmap* pCB = s_bitmapCache.Find(name, targetScale);

  if (!pCB) // if bitmap not in cache already at targetScale
  {
    WDL_String fullPath;
    OSFindResource(name, "png", fullPath);
    cairo_surface_t* pSurface = LoadPNGResource(GetPlatformInstance(), fullPath);

    assert(cairo_surface_status(pSurface) == CAIRO_STATUS_SUCCESS); // Protect against typos in resource.h and .rc files.
    
    pCB = new CairoBitmap(pSurface, sourceScale);

    const IBitmap bitmap(pCB->surface, pCB->width / sourceScale, pCB->height / sourceScale, nStates, framesAreHoriztonal, sourceScale, name);

    if (sourceScale != targetScale)
      return ScaleBitmap(bitmap, name, targetScale); // will add to cache
    else
      s_bitmapCache.Add(pCB, name, sourceScale);
  }

  return IBitmap(pCB->surface, pCB->width / targetScale, pCB->height / targetScale, nStates, framesAreHoriztonal, sourceScale, name);
}

void IGraphicsCairo::ReleaseBitmap(IBitmap& bitmap)
{
}

void IGraphicsCairo::RetainBitmap(IBitmap& bitmap, const char * cacheName)
{
}

IBitmap IGraphicsCairo::ScaleBitmap(const IBitmap& inBitmap, const char* name, double targetScale)
{
  int newW = (int)(inBitmap.W * targetScale);
  int newH = (int)(inBitmap.H * targetScale);
  
  // Create resources to redraw
    
  cairo_surface_t* pOutSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, newW, newH);
  cairo_t* pOutContext = cairo_create(pOutSurface);

  // Scale and paint (destroying the context / the surface is retained)
    
  cairo_scale(pOutContext, targetScale, targetScale);
  cairo_set_source_surface(pOutContext, (cairo_surface_t*)inBitmap.mData, 0, 0);
  cairo_paint(pOutContext);
  cairo_destroy(pOutContext);

 // Cache and retun as an IBitmap
    
  CairoBitmap* pCB = new CairoBitmap(pOutSurface, targetScale);
  s_bitmapCache.Add(pCB, name, targetScale);

  return IBitmap(pCB->surface, inBitmap.W, inBitmap.H, inBitmap.N, inBitmap.mFramesAreHorizontal, inBitmap.mSourceScale, name);
}

IBitmap IGraphicsCairo::CropBitmap(const IBitmap& inBitmap, const IRECT& rect, const char* name, double targetScale)
{
  int newW = (int)(inBitmap.W * targetScale);
  int newH = (int)(inBitmap.H * targetScale);
    
  uint8_t* pOutBuffer = new uint8_t[newW * newH * 4];
  
  // Convert output to cairo
  cairo_surface_t* pOutSurface = cairo_image_surface_create_for_data(pOutBuffer, CAIRO_FORMAT_ARGB32, newW, newH, 0);
  cairo_t* pOutContext = cairo_create(pOutSurface);
  
  // Paint from one surface to another
  cairo_set_source_surface(pOutContext, (cairo_surface_t*) inBitmap.mData, 0, 0);
  cairo_rectangle(pOutContext, rect.L, rect.T, rect.W(), rect.H());
  cairo_scale(pOutContext, targetScale, targetScale);
  cairo_paint(pOutContext);
  cairo_surface_set_device_scale(pOutSurface, targetScale, targetScale);
    
  // Destroy cairo stuff
  cairo_destroy(pOutContext);
  
  return IBitmap(pOutSurface, newW, newH); //TODO: surface will not be destroyed, unless this is retained
}

void IGraphicsCairo::DrawSVG(ISVG& svg, const IRECT& dest, const IBlend* pBlend)
{
  cairo_save(mContext);
  cairo_translate(mContext, dest.L, dest.T);
  cairo_rectangle(mContext, 0, 0, dest.W(), dest.H());
  cairo_clip(mContext);

  double xScale = dest.W() / svg.W();
  double yScale = dest.H() / svg.H();
  double scale = xScale < yScale ? xScale : yScale;

  cairo_scale(mContext, scale, scale);

  CairoNanoSVGRender::RenderNanoSVG(mContext, svg.mImage);

  cairo_restore(mContext);
}

void IGraphicsCairo::DrawRotatedSVG(ISVG& svg, float destCtrX, float destCtrY, float width, float height, double angle, const IBlend* pBlend)
{
  cairo_save(mContext);
  cairo_translate(mContext, destCtrX, destCtrY);
  cairo_rotate(mContext, DegToRad(angle));
  DrawSVG(svg, IRECT(-width * 0.5, - height * 0.5, width * 0.5, height * 0.5), pBlend);
  cairo_restore(mContext);
}

void IGraphicsCairo::DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend)
{
  cairo_save(mContext);
  ClipRegion(dest);
  cairo_surface_t* surface = (cairo_surface_t*) bitmap.mData;
  cairo_set_source_surface(mContext, surface, std::round(dest.L) - srcX, (int) std::round(dest.T) - srcY);
  cairo_set_operator(mContext, CairoBlendMode(pBlend));
  cairo_paint_with_alpha(mContext, BlendWeight(pBlend));
  cairo_restore(mContext);
}

void IGraphicsCairo::DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend)
{
  //TODO: offset support
    
  float width = bitmap.W;
  float height = bitmap.H;

  cairo_save(mContext);
  cairo_translate(mContext, destCtrX, destCtrY);
  cairo_rotate(mContext, DegToRad(angle));
  DrawBitmap(bitmap, IRECT(-width * 0.5, - height * 0.5, width * 0.5, height * 0.5), 0, 0, pBlend);
  cairo_restore(mContext);
}

void IGraphicsCairo::DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IBlend* pBlend)
{
  float width = base.W;
  float height = base.H;

  IBlend addBlend(kBlendAdd);
  cairo_save(mContext);
  DrawBitmap(base, IRECT(x, y, x + width, y + height), 0, 0, pBlend);
  cairo_translate(mContext, x + 0.5 * width, y + 0.5 * height);
  cairo_rotate(mContext, angle);
  DrawBitmap(mask, IRECT(-width * 0.5, - height * 0.5, width * 0.5, height * 0.5), 0, 0, &addBlend);
  DrawBitmap(top, IRECT(-width * 0.5, - height * 0.5, width * 0.5, height * 0.5), 0, 0, pBlend);
  cairo_restore(mContext);
}

void IGraphicsCairo::DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend)
{
  cairo_move_to(mContext, x + 0.5, y + 0.5);
  cairo_line_to(mContext, x + 1.5, y + 1.5);
  Stroke(color, pBlend);
}

void IGraphicsCairo::ForcePixel(const IColor& color, int x, int y)
{
  cairo_move_to(mContext, x + 0.5, y + 0.5);
  cairo_line_to(mContext, x + 1.5, y + 1.5);
  Stroke(color);
}

inline void IGraphicsCairo::CairoDrawTriangle(float x1, float y1, float x2, float y2, float x3, float y3)
{
  cairo_move_to(mContext, x1, y1);
  cairo_line_to(mContext, x2, y2);
  cairo_line_to(mContext, x3, y3);
  cairo_close_path(mContext);
}

inline void IGraphicsCairo::CairoDrawConvexPolygon(float* x, float* y, int npoints)
{
  cairo_move_to(mContext, x[0], y[0]);
  for(int i = 1; i < npoints; i++)
    cairo_line_to(mContext, x[i], y[i]);
  cairo_close_path(mContext);
}

inline void IGraphicsCairo::CairoDrawRoundRect(const IRECT& rect, float corner)
{
  const double y = rect.B - rect.H();
  cairo_new_path(mContext);
  cairo_arc(mContext, rect.L + rect.W() - corner, y + corner, corner, PI * -0.5, 0);
  cairo_arc(mContext, rect.L + rect.W() - corner, y + rect.H() - corner, corner, 0, PI * 0.5);
  cairo_arc(mContext, rect.L + corner, y + rect.H() - corner, corner, PI * 0.5, PI);
  cairo_arc(mContext, rect.L + corner, y + corner, corner, PI, PI * 1.5);
  cairo_close_path(mContext);
}

inline void IGraphicsCairo::CairoDrawCircle(float cx, float cy, float r)
{
  cairo_new_path(mContext);
  cairo_arc(mContext, cx, cy, r, 0.f, 2.f * PI);
  cairo_close_path(mContext);
}

void IGraphicsCairo::DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend)
{
  cairo_move_to(mContext, x1, y1);
  cairo_line_to(mContext, x2, y2);
  Stroke(color, pBlend);
}

void IGraphicsCairo::DrawTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend)
{
  CairoDrawTriangle(x1, y1, x2, y2, x3, y3);
  Stroke(color, pBlend);
}

void IGraphicsCairo::DrawRect(const IColor& color, const IRECT& rect, const IBlend* pBlend)
{
  CairoDrawRect(rect);
  Stroke(color, pBlend);
}

void IGraphicsCairo::DrawRoundRect(const IColor& color, const IRECT& rect, float corner, const IBlend* pBlend)
{
  CairoDrawRoundRect(rect, corner);
  Stroke(color, pBlend);
}

void IGraphicsCairo::DrawConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend)
{
  CairoDrawConvexPolygon(x, y, npoints);
  Stroke(color, pBlend);
}

void IGraphicsCairo::DrawArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax, const IBlend* pBlend)
{
  cairo_arc(mContext, cx, cy, r, DegToRad(aMin-90.f), DegToRad(aMax-90.f));
  Stroke(color, pBlend);
}

void IGraphicsCairo::DrawCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend)
{
  CairoDrawCircle(cx, cy, r);
  Stroke(color, pBlend);
}

void IGraphicsCairo::DrawDottedRect(const IColor& color, const IRECT& rect, const IBlend* pBlend)
{
  double dashLength = 2;
  cairo_set_dash(mContext, &dashLength, 1, 0.0);
  DrawRect(color, rect, pBlend);
  cairo_set_dash(mContext, nullptr, 0, 0.0);
}

void IGraphicsCairo::FillTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend)
{
  CairoDrawTriangle(x1, y1, x2, y2, x3, y3);
  Fill(color, pBlend);
}

void IGraphicsCairo::FillRect(const IColor& color, const IRECT& rect, const IBlend* pBlend)
{
  CairoDrawRect(rect);
  Fill(color, pBlend);
}

void IGraphicsCairo::FillRoundRect(const IColor& color, const IRECT& rect, float corner, const IBlend* pBlend)
{
  CairoDrawRoundRect(rect, corner);
  Fill(color, pBlend);
}

void IGraphicsCairo::FillConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend)
{
  CairoDrawConvexPolygon(x, y, npoints);
  Fill(color, pBlend);
}

void IGraphicsCairo::FillArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax, const IBlend* pBlend)
{
  cairo_move_to(mContext, cx, cy);
  cairo_arc(mContext, cx, cy, r, DegToRad(aMin-90.f), DegToRad(aMax-90.f));
  cairo_close_path(mContext);
  Fill(color, pBlend);
}

void IGraphicsCairo::FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend)
{
  CairoDrawCircle(cx, cy, r);
  Fill(color, pBlend);
}

IColor IGraphicsCairo::GetPoint(int x, int y)
{
  // Convert suface to cairo image surface of one pixel (avoid copying the whole surface)
    
  cairo_surface_t* pOutSurface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
  cairo_t* pOutContext = cairo_create(pOutSurface);
  cairo_set_source_surface(pOutContext, mSurface, -x, -y);
  cairo_paint(pOutContext);
  cairo_surface_flush(pOutSurface);

  uint8_t* pData = cairo_image_surface_get_data(pOutSurface);
  uint32_t px = *((uint32_t*)(pData));
  
  cairo_surface_destroy(pOutSurface);
  cairo_destroy(pOutContext);
    
  int A = (px >> 0) & 0xFF;
  int R = (px >> 8) & 0xFF;
  int G = (px >> 16) & 0xFF;
  int B = (px >> 24) & 0xFF;
    
  return IColor(A, R, G, B);
}

bool IGraphicsCairo::DrawText(const IText& text, const char* str, IRECT& rect, bool measure)
{
#ifdef OS_WIN
  // TODO: lots!
  LoadFont("C:/Windows/Fonts/Verdana.ttf");

  assert(mFTFace != nullptr);

  cairo_font_face_t* pFace = cairo_ft_font_face_create_for_ft_face(mFTFace, 0);
  cairo_text_extents_t textExtents;
  cairo_font_extents_t fontExtents;

  cairo_set_font_face(mContext, pFace);

  cairo_font_extents(mContext, &fontExtents);
  cairo_text_extents(mContext, str, &textExtents);

  if (measure)
  {
    rect = IRECT(0, 0, textExtents.width, fontExtents.height);
    cairo_font_face_destroy(pFace);
    return true;
  }

  double x = 0., y = 0.;

  switch (text.mAlign)
  {
    case IText::EAlign::kAlignNear:x = rect.L; break;
    case IText::EAlign::kAlignFar: x = rect.R - textExtents.width - textExtents.x_bearing; break;
    case IText::EAlign::kAlignCenter: x = rect.L + ((rect.W() - textExtents.width - textExtents.x_bearing) / 2.0); break;
    default: break;
  }

  // TODO: Add vertical alignment
  y = rect.T + fontExtents.ascent;

  cairo_move_to(mContext, x, y);
  SetCairoSourceRGBA(text.mColor);
  cairo_show_text(mContext, str);
  cairo_font_face_destroy(pFace);
#endif
  
	return true;
}

bool IGraphicsCairo::MeasureText(const IText& text, const char* str, IRECT& destRect)
{
  return DrawText(text, str, destRect, true);
}

void IGraphicsCairo::SetPlatformContext(void* pContext)
{
  if (!pContext)
  {
    if (mContext)
      cairo_destroy(mContext);
    if (mSurface)
      cairo_surface_destroy(mSurface);
      
    mContext = nullptr;
    mSurface = nullptr;
  }
  else if(!mSurface)
  {
#ifdef OS_MAC
    mSurface = cairo_quartz_surface_create_for_cg_context(CGContextRef(pContext), Width(), Height());
    mContext = cairo_create(mSurface);
    cairo_surface_set_device_scale(mSurface, 1, -1);
    cairo_surface_set_device_offset(mSurface, 0, Height());
#else
    HDC dc = (HDC) pContext;
    mSurface = cairo_win32_surface_create_with_ddb(dc, CAIRO_FORMAT_ARGB32, Width(), Height());
    mContext = cairo_create(mSurface);
    cairo_surface_set_device_scale(mSurface, GetScale(),GetScale());
#endif
  }
  
  IGraphics::SetPlatformContext(pContext);
}

void IGraphicsCairo::RenderDrawBitmap()
{
  cairo_surface_flush(mSurface);

#ifdef OS_WIN
  PAINTSTRUCT ps;
  HWND hWnd = (HWND) GetWindow();
  HDC dc = BeginPaint(hWnd, &ps);
  HDC cdc = cairo_win32_surface_get_dc(mSurface);
  
  if (GetScale() == 1.f)
    BitBlt(dc, 0, 0, Width(), Height(), cdc, 0, 0, SRCCOPY);
  else
    StretchBlt(dc, 0, 0, WindowWidth(), WindowHeight(), cdc, 0, 0, Width(), Height(), SRCCOPY);

  EndPaint(hWnd, &ps);
#endif
}
