#pragma once

#include "IControl.h"

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

class IGraphicsAGG : public IGraphics
{
public:
  const char* GetDrawingAPIStr() override { return "AGG"; }

  struct LineInfo {
    int start_char;
    int end_char;
    double width;
    LineInfo() : width(0.0), start_char(0), end_char(0) {}
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
  
  IGraphicsAGG(IPlugBaseGraphics& plug, int w, int h, int fps);
  ~IGraphicsAGG();

  void PrepDraw() override;
  void Draw(const IRECT& rect) override;

  void DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IChannelBlend* pBlend) override;
  void DrawRotatedBitmap(IBitmap& bitmap, int destCtrX, int destCtrY, double angle, int yOffsetZeroDeg, const IChannelBlend* pBlend) override;
  void DrawRotatedMask(IBitmap& base, IBitmap& mask, IBitmap& top, int x, int y, double angle, const IChannelBlend* pBlend) override;
  void DrawPoint(const IColor& color, float x, float y, const IChannelBlend* pBlend, bool aa) override;
  void ForcePixel(const IColor& color, int x, int y) override;
  void DrawLine(const IColor& color, float x1, float y1, float x2, float y2, const IChannelBlend* pBlend, bool aa) override;
  void DrawArc(const IColor& color, float cx, float cy, float r, float minAngle, float maxAngle,  const IChannelBlend* pBlend, bool aa) override;
  void DrawCircle(const IColor& color, float cx, float cy, float r,const IChannelBlend* pBlend, bool aa) override;
  void DrawTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IChannelBlend* pBlend = nullptr) override {}
  void FillCircle(const IColor& color, int cx, int cy, float r, const IChannelBlend* pBlend, bool aa) override;
  void FillIRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend) override;
  void DrawRoundRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend, int cornerradius, bool aa) override;
  void FillRoundRect(const IColor& color, const IRECT& rect, const IChannelBlend* pBlend, int cornerradius, bool aa) override;
  void FillIConvexPolygon(const IColor& color, int* x, int* y, int npoints, const IChannelBlend* pBlend) override;
  void FillTriangle(const IColor& color, int x1, int y1, int x2, int y2, int x3, int y3, const IChannelBlend* pBlend) override;
  IColor GetPoint(int x, int y) override;
  void* GetData() override { return 0; } //todo
  
  bool DrawIText(const IText& text, const char* str, IRECT& rect, bool measure = false) override;
  bool MeasureIText(const IText& text, const char* str, IRECT& destRect) override;

  IBitmap LoadIBitmap(const char* name, int nStates, bool framesAreHoriztonal, double scale) override;
  IBitmap ScaleIBitmap(const IBitmap& bitmap, const char* cacheName, double scale) override;
  IBitmap CropIBitmap(const IBitmap& bitmap, const IRECT& rect, const char* cacheName, double scale) override;
  void RetainIBitmap(IBitmap& bitmap, const char* cacheName) override {};
  void ReleaseIBitmap(IBitmap& bitmap) override {};
//  IBitmap CreateIBitmap(const char * cacheName, int w, int h) override;
  void ReScale() override;

  void RenderAPIBitmap(void* pContext) override;

  IFontData LoadIFont(const char * name, const int size);
  
  inline const agg::rgba8 IColorToAggColor(const IColor& color)
  {
    return agg::rgba8(color.R, color.G, color.B, color.A);
  }
private:
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
