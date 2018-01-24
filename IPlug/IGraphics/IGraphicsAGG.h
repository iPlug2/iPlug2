#pragma once

#include "IGraphics.h"

#ifdef OS_OSX
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-register"
#endif

//agg
#include "agg_basics.h"
#include "agg_renderer_base.h"
#include "agg_renderer_primitives.h"
#include "agg_rendering_buffer.h"
#include "agg_pixfmt_rgb.h"
#include "agg_pixfmt_rgba.h"
#include "agg_pixfmt_amask_adaptor.h"
#include "agg_renderer_scanline.h"
#include "agg_rasterizer_scanline_aa.h"
#include "agg_renderer_outline_aa.h"
#include "agg_rasterizer_outline_aa.h"
#include "agg_conv_stroke.h"
#include "agg_conv_dash.h"
#include "agg_conv_curve.h"
#include "agg_conv_contour.h"
#include "agg_conv_smooth_poly1.h"
#include "agg_conv_marker.h"
#include "agg_arrowhead.h"
#include "agg_vcgen_markers_term.h"
#include "agg_scanline_p.h"
#include "agg_renderer_scanline.h"
#include "agg_pixfmt_rgb.h"
#include "agg_pixfmt_gray.h"
#include "agg_alpha_mask_u8.h"
#include "agg_path_storage.h"
#include "agg_bounding_rect.h"
#include "agg_ellipse.h"
#include "agg_font_freetype.h"
#include "agg_pmap.h"
#include "agg_font.h"
#include "agg_image_accessors.h"
#include "agg_span_allocator.h"
#include "agg_span_interpolator_linear.h"
#include "agg_renderer_outline_image.h"
#include "agg_pattern_filters_rgba.h"
#include "agg_span_image_filter_rgba.h"
#include "agg_span_image_filter_rgb.h"
#include "agg_span_image_filter_gray.h"
#include "agg_span_interpolator_linear.h"
#include "agg_rounded_rect.h"
#include "agg_span_converter.h"
#include "agg_conv_segmentator.h"
#include "agg_trans_single_path.h"

#ifdef OS_OSX
#include "agg_mac_pmap.h"
#include "agg_mac_font.h"
#pragma clang diagnostic pop
#endif

/** IGraphics draw class using Antigrain Geometry  
*   @ingroup DrawClasses
*/
class IGraphicsAGG : public IGraphics
{
public:
  struct LineInfo
  {
    int mStartChar;
    int mEndChar;
    double mWidth;
    LineInfo() : mWidth(0.0), mStartChar(0), mEndChar(0) {}
  };
  
#ifdef OS_WIN
  typedef agg::order_bgra PixelOrder;
#else
  typedef agg::order_argb PixelOrder;
#endif
  typedef agg::comp_op_adaptor_rgba<agg::rgba8, PixelOrder> BlenderType;
  typedef agg::comp_op_adaptor_rgba_pre<agg::rgba8, PixelOrder> BlenderTypePre;
  typedef agg::pixfmt_custom_blend_rgba<BlenderType, agg::rendering_buffer> PixfmtType;
  typedef agg::pixfmt_custom_blend_rgba<BlenderTypePre, agg::rendering_buffer> PixfmtTypePre;
  
  typedef agg::renderer_base <PixfmtType> RenbaseType;
  typedef agg::font_engine_freetype_int32 FontEngineType;
  typedef agg::font_cache_manager <FontEngineType> FontManagerType;
  typedef agg::span_interpolator_linear<> interpolatorType;
  typedef agg::image_accessor_clip<PixfmtType> imgSourceType;
  typedef agg::span_image_filter_rgba_bilinear_clip <PixfmtType, interpolatorType> spanGenType;
  //typedef agg::renderer_scanline_aa_solid<RenbaseType> rendererSolid;
  //typedef agg::renderer_scanline_bin_solid<RenbaseType> rendererBin;
  typedef agg::renderer_base<agg::pixfmt_gray8> maskRenBase;
  typedef agg::scanline_u8_am<agg::alpha_mask_gray8> scanlineType;
  
  IGraphicsAGG(IPlugBaseGraphics& plug, int w, int h, int fps);
  ~IGraphicsAGG();
  
  void SetDisplayScale(int scale) override;

  void Draw(const IRECT& rect) override;
  
  void DrawSVG(ISVG& svg, const IRECT& dest, const IBlend* pBlend) override {}
    void DrawRotatedSVG(ISVG& svg, float destCtrX, float destCtrY, float width, float height, double angle, const IBlend* pBlend) override {}

  void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;
  void DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IBlend* pBlend) override;
  void DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IBlend* pBlend) override;
  void DrawPoint(const IColor& color, float x, float y, const IBlend* pBlend) override;
  void ForcePixel(const IColor& color, int x, int y) override;
    
  void DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IBlend* pBlend) override;
  void DrawTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend) override;
  void DrawRect(const IColor& color, const IRECT& rect, const IBlend* pBlend) override;
  void DrawRoundRect(const IColor& color, const IRECT& rect, float cr, const IBlend* pBlend) override;
  void DrawConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend = 0) override;
  void DrawArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax,  const IBlend* pBlend) override;
  void DrawCircle(const IColor& color, float cx, float cy, float r,const IBlend* pBlend) override;
    
  void DrawDottedRect(const IColor& color, const IRECT& rect, const IBlend* pBlend) override;

  void FillTriangle(const IColor& color, float x1, float y1, float x2, float y2, float x3, float y3, const IBlend* pBlend) override;
  void FillRect(const IColor& color, const IRECT& rect, const IBlend* pBlend) override;
  void FillRoundRect(const IColor& color, const IRECT& rect, float cr, const IBlend* pBlend) override;
  void FillConvexPolygon(const IColor& color, float* x, float* y, int npoints, const IBlend* pBlend) override;
  void FillArc(const IColor& color, float cx, float cy, float r, float aMin, float aMax,  const IBlend* pBlend) override;
  void FillCircle(const IColor& color, float cx, float cy, float r, const IBlend* pBlend) override;
  
  bool DrawText(const IText& text, const char* str, IRECT& rect, bool measure = false) override;
  bool MeasureText(const IText& text, const char* str, IRECT& destRect) override;
  
  IColor GetPoint(int x, int y) override;
  void* GetData() override { return 0; } //todo
  const char* GetDrawingAPIStr() override { return "AGG"; }

  IBitmap LoadBitmap(const char* name, int nStates, bool framesAreHoriztonal, double scale) override;
  IBitmap ScaleBitmap(const IBitmap& bitmap, const char* cacheName, double scale) override;
  IBitmap CropBitmap(const IBitmap& bitmap, const IRECT& rect, const char* cacheName, double scale) override;
  void RetainBitmap(IBitmap& bitmap, const char* cacheName) override {};
  void ReleaseBitmap(IBitmap& bitmap) override {};
//  IBitmap CreateIBitmap(const char * cacheName, int w, int h) override;

  void RenderDrawBitmap() override;
  
  inline const agg::rgba8 IColorToAggColor(const IColor& color)
  {
    return agg::rgba8(color.R, color.G, color.B, color.A);
  }
private:
    
  template <typename pathType>
  void Rasterize(const IColor& color, pathType& path)
  {
    agg::rasterizer_scanline_aa<> rasterizer;
    rasterizer.reset();
    
    agg::scanline_p8 scanline;
    
    agg::renderer_scanline_aa_solid<RenbaseType> renderer(mRenBase);
    
    renderer.color(IColorToAggColor(color));
    rasterizer.filling_rule(agg::fill_non_zero);
    
    rasterizer.add_path(path);
    agg::render_scanlines(rasterizer, scanline, renderer);
  }
    
  template <typename pathType>
  void Fill(const IColor& color, pathType& path)
  {
    Rasterize(color, path);
  }
    
  template <typename pathType>
  void Stroke(const IColor& color, pathType& path)
  {
    agg::conv_stroke<pathType> strokes(path);
    strokes.width(1.0 * GetDisplayScale());
    Rasterize(color, strokes);
  }

  RenbaseType mRenBase;
  PixfmtType mPixf;
  FontEngineType mFontEngine;
  FontManagerType mFontManager;
  agg::rendering_buffer mRenBuf;
#ifdef OS_OSX
  agg::pixel_map_mac mPixelMap;
#else
#endif
  
private:
  agg::pixel_map* LoadAPIBitmap(const char* pPath);
  agg::pixel_map* CreateAPIBitmap(int w, int h);
  agg::pixel_map* ScaleAPIBitmap(agg::pixel_map* pixel_map, int destW, int destH);

  //pipeline to process the vectors glyph paths(curves + contour)
  agg::conv_curve<FontManagerType::path_adaptor_type> mFontCurves;
  agg::conv_contour<agg::conv_curve<FontManagerType::path_adaptor_type> > mFontContour;
  
  void CalculateTextLines(WDL_TypedBuf<LineInfo> * lines, const IRECT& rect, const char * str, FontManagerType& manager);
  void ToPixel(float & pixel);
};
