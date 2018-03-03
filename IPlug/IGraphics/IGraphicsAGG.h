#pragma once

/*

 AGG 2.4 should be modified to avoid bringing carbon headers on mac, which can cause conflicts

 in "agg_mac_pmap.h" ...
 //#include <ApplicationServices/ApplicationServices.h>
 #include <CoreGraphics/CoreGraphics.h>

 */

#include "IGraphicsPathBase.h"
#include "IGraphicsAGG_src.h"

class AGGBitmap : public APIBitmap
{
public:
  AGGBitmap(agg::pixel_map* pPixMap, int scale) : APIBitmap (pPixMap, pPixMap->width(), pPixMap->height(), scale) {}
  virtual ~AGGBitmap() { delete ((agg::pixel_map*) GetBitmap()); }
};

inline const agg::rgba8 AGGColor(const IColor& color, const IBlend* pBlend = nullptr)
{
  return agg::rgba8(color.R, color.G, color.B, (BlendWeight(pBlend) * color.A));
}

inline agg::comp_op_e AGGBlendMode(const IBlend* pBlend)
{
  if (!pBlend)
    return agg::comp_op_src;

  switch (pBlend->mMethod)
  {
    case kBlendClobber: return agg::comp_op_src_over;
    case kBlendAdd: return agg::comp_op_plus;
    case kBlendColorDodge: return agg::comp_op_color_dodge;
    case kBlendNone:
    default:
      return agg::comp_op_src_over;
  }
}

inline const agg::cover_type AGGCover(const IBlend* pBlend = nullptr)
{
  if (!pBlend)
    return 255;

  return std::max(agg::cover_type(0), std::min(agg::cover_type(roundf(pBlend->mWeight * 255.f)), agg::cover_type(255)));
}

/** IGraphics draw class using Antigrain Geometry
*   @ingroup DrawClasses*/
class IGraphicsAGG : public IGraphicsPathBase
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
  typedef agg::renderer_scanline_aa_solid<RenbaseType> RendererSolid;
  typedef agg::renderer_scanline_bin_solid<RenbaseType> RendererBin;
  typedef agg::font_engine_freetype_int32 FontEngineType;
  typedef agg::font_cache_manager <FontEngineType> FontManagerType;
  typedef agg::span_interpolator_linear<> InterpolatorType;
  typedef agg::span_allocator<agg::rgba8> SpanAllocatorType;
  typedef agg::gradient_lut<agg::color_interpolator<agg::rgba8>, 512> ColorArrayType;
  typedef agg::image_accessor_clip<PixfmtType> imgSourceType;
  typedef agg::span_image_filter_rgba_bilinear_clip <PixfmtType, InterpolatorType> spanGenType;
  
  typedef agg::renderer_base<agg::pixfmt_gray8> maskRenBase;
  typedef agg::scanline_u8_am<agg::alpha_mask_gray8> scanlineType;

  IGraphicsAGG(IDelegate& dlg, int w, int h, int fps);
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

  void DrawDottedRect(const IColor& color, const IRECT& rect, const IBlend* pBlend) override;

  void PathClear() override { mPath.remove_all(); }
  void PathStart() override { mPath.start_new_path(); }
  void PathClose() override { mPath.close_polygon(); }
  
  void PathArc(float cx, float cy, float r, float aMin, float aMax) override;

  void PathMoveTo(float x, float y) override { mPath.move_to(x * GetDisplayScale(), y * GetDisplayScale()); }
  void PathLineTo(float x, float y) override { mPath.line_to(x * GetDisplayScale(), y * GetDisplayScale());}
  void PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3) override;
  
  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override;
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override;
  
  bool DrawText(const IText& text, const char* str, IRECT& rect, bool measure = false) override;
  bool MeasureText(const IText& text, const char* str, IRECT& destRect) override;

  IColor GetPoint(int x, int y) override;
  void* GetData() override { return 0; } //todo
  const char* GetDrawingAPIStr() override { return "AGG"; }

 // IBitmap CropBitmap(const IBitmap& bitmap, const IRECT& rect, const char* cacheName, int scale) override;
 //  IBitmap CreateIBitmap(const char * cacheName, int w, int h) override;

  void RenderDrawBitmap() override;

private:

  template <typename GradientFuncType>
  void GradientRasterize(agg::rasterizer_scanline_aa<>& rasterizer, GradientFuncType& gradientFunc, InterpolatorType& spanInterpolator, ColorArrayType& colorArray)
  {
    agg::scanline_p8 scanline;
    SpanAllocatorType spanAllocator;

    // Gradient types
    
    typedef agg::span_gradient<agg::rgba8, InterpolatorType, GradientFuncType, ColorArrayType> SpanGradientType;
    typedef agg::renderer_scanline_aa<RenbaseType, SpanAllocatorType, SpanGradientType> RendererGradientType;
    
    // Gradient objects
    
    SpanGradientType spanGradient(spanInterpolator, gradientFunc, colorArray, 0, 512);
    RendererGradientType renderer(mRenBase, spanAllocator, spanGradient);
    
    agg::render_scanlines(rasterizer, scanline, renderer);
  }
  
  template <typename pathType>
  void Rasterize(const IPattern& pattern, pathType& path, const IBlend* pBlend = nullptr, EFillRule rule = kFillWinding)
  {
    agg::rasterizer_scanline_aa<> rasterizer;
    rasterizer.reset();
    rasterizer.filling_rule(rule == kFillWinding ? agg::fill_non_zero : agg::fill_even_odd );
    rasterizer.add_path(path);
    
    switch (pattern.mType)
    {
      case kSolidPattern:
      {
        agg::scanline_p8 scanline;
        RendererSolid renderer(mRenBase);
        
        const IColor &color = pattern.GetStop(0).mColor;
        renderer.color(AGGColor(color, pBlend));
        
        // Rasterize

        agg::render_scanlines(rasterizer, scanline, renderer);
      }
        break;
        
      case kLinearPattern:
      case kRadialPattern:
      {
        // Common gradient objects
        
        const float* xform = pattern.mTransform;
        
        agg::trans_affine       gradientMTX(xform[0], xform[1] , xform[2], xform[3], xform[4], xform[5]);
        ColorArrayType          colorArray;
        InterpolatorType        spanInterpolator(gradientMTX);
       
        // Scaling
        
        gradientMTX = agg::trans_affine_scaling(1.0 / GetDisplayScale()) * gradientMTX * agg::trans_affine_scaling(512.0);

        // Make gradient lut
        
        colorArray.remove_all();
        
        for (int i = 0; i < pattern.NStops(); i++)
        {
          const IColorStop& stop = pattern.GetStop(i);
          colorArray.add_color(stop.mOffset, AGGColor(stop.mColor, pBlend));
        }
        
        colorArray.build_lut();
        
        // Rasterize
        
        if (pattern.mType == kLinearPattern)
        {
          agg::gradient_x gradientFunc;
          GradientRasterize(rasterizer, gradientFunc, spanInterpolator, colorArray);
        }
        else
        {
          agg::gradient_radial_d gradientFunc;
          GradientRasterize(rasterizer, gradientFunc, spanInterpolator, colorArray);
        }
        
        // FIX extensions...
        
        /*
        switch (pattern.mExtend)
        {
        case kExtendNone:      cairo_pattern_set_extend(cairoPattern, CAIRO_EXTEND_NONE);      break;
        case kExtendPad:       cairo_pattern_set_extend(cairoPattern, CAIRO_EXTEND_PAD);       break;
        case kExtendReflect:   cairo_pattern_set_extend(cairoPattern, CAIRO_EXTEND_REFLECT);   break;
        case kExtendRepeat:    cairo_pattern_set_extend(cairoPattern, CAIRO_EXTEND_REPEAT);    break;
        }
        */
     
      }
      break;
    }
  }

  template <typename pathType>
  void Fill(const IColor& color, pathType& path, const IBlend* pBlend = nullptr)
  {
    Rasterize(color, path, pBlend);
  }

  template <typename pathType>
  void Stroke(const IColor& color, pathType& path, const IBlend* pBlend = nullptr)
  {
    agg::conv_stroke<pathType> strokes(path);
    strokes.width(1.0 * GetDisplayScale());
    Rasterize(color, strokes, pBlend);
  }

  RenbaseType mRenBase;
  PixfmtType mPixf;
  FontEngineType mFontEngine;
  FontManagerType mFontManager;
  agg::rendering_buffer mRenBuf;
  agg::path_storage mPath;
#ifdef OS_MAC
  agg::pixel_map_mac mPixelMap;
#else
  //TODO:
#endif
  
  void SetAGGSourcePattern(RendererSolid &renderer, const IPattern& pattern, const IBlend* pBlend);

  APIBitmap* LoadAPIBitmap(const WDL_String& resourcePath, int scale) override;
  agg::pixel_map* CreateAPIBitmap(int w, int h);
  APIBitmap* ScaleAPIBitmap(const APIBitmap* pBitmap, int s) override;

  //pipeline to process the vectors glyph paths(curves + contour)
  agg::conv_curve<FontManagerType::path_adaptor_type> mFontCurves;
  agg::conv_contour<agg::conv_curve<FontManagerType::path_adaptor_type> > mFontContour;

  
  void CalculateTextLines(WDL_TypedBuf<LineInfo>* pLines, const IRECT& rect, const char* str, FontManagerType& manager);
  void ToPixel(float & pixel);
};
