/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/*

 AGG 2.4 should be modified to avoid bringing carbon headers on mac, which can cause conflicts

 in "agg_mac_pmap.h" ...
 //#include <ApplicationServices/ApplicationServices.h>
 #include <CoreGraphics/CoreGraphics.h>

 */

#include "IGraphicsPathBase.h"
#include "IGraphicsAGG_src.h"

#include "heapbuf.h"

template <class SpanGeneratorType>
class alpha_span_generator : public SpanGeneratorType
{
public:
  
  alpha_span_generator(typename SpanGeneratorType::source_type& source, typename SpanGeneratorType::interpolator_type& interpolator, agg::cover_type a)
  : SpanGeneratorType(source, interpolator), alpha(a) {}
  
  void generate(typename SpanGeneratorType::color_type* span, int x, int y, unsigned len)
  {
    SpanGeneratorType::generate(span, x, y, len);
    
    if (alpha != 255)
    {
      for (unsigned i = 0; i < len; i++, span++)
        span->a = (span->a * alpha + SpanGeneratorType::base_mask) >> SpanGeneratorType::base_shift;
    }
  }
  
private:
  
  agg::cover_type alpha;
};

/** An AGG API bitmap
 * @ingroup APIBitmaps */
class AGGBitmap : public APIBitmap
{
public:
  AGGBitmap(agg::pixel_map* pPixMap, int scale, float drawScale, bool preMultiplied)
    : APIBitmap(pPixMap, pPixMap->width(), pPixMap->height(), scale, drawScale), mPreMultiplied(preMultiplied)
    {}
  virtual ~AGGBitmap() { delete GetBitmap(); }
  bool IsPreMultiplied() const { return mPreMultiplied; }
private:
  bool mPreMultiplied;
};

/** IGraphics draw class using Antigrain Geometry
*   @ingroup DrawClasses*/
class IGraphicsAGG : public IGraphicsPathBase
{
public:
#ifdef OS_WIN
  typedef agg::order_bgra PixelOrder;
  typedef agg::pixel_map_win32 PixelMapType;
#elif defined OS_MAC
  typedef agg::order_argb PixelOrder;
  typedef agg::pixel_map_mac PixelMapType;
#else
#error NOT IMPLEMENTED
#endif
  typedef agg::span_allocator<agg::rgba8> SpanAllocatorType;
  typedef agg::span_interpolator_linear<> InterpolatorType;
  // Pre-multiplied source types
  typedef agg::comp_op_adaptor_rgba_pre<agg::rgba8, PixelOrder> BlenderPreType;
  typedef agg::pixfmt_custom_blend_rgba<BlenderPreType, agg::rendering_buffer> PixfmtPreType;
  typedef agg::renderer_base <PixfmtPreType> RenbasePreType;
   // Non pre-multiplied source types
  typedef agg::comp_op_adaptor_rgba<agg::rgba8, PixelOrder> BlenderType;
  typedef agg::pixfmt_custom_blend_rgba<BlenderType, agg::rendering_buffer> PixfmtType;
  typedef agg::renderer_base <PixfmtType> RenbaseType;
  // Image bitmap types
  typedef agg::image_accessor_clone<PixfmtType> imgSourceType;
  typedef agg::span_image_filter_rgba_bilinear<imgSourceType, InterpolatorType> SpanGeneratorType;
  typedef agg::renderer_scanline_aa<RenbaseType, SpanAllocatorType, SpanGeneratorType> BitmapRenderType;
  // Font types
  typedef agg::font_engine_freetype_int32 FontEngineType;
  typedef agg::font_cache_manager<FontEngineType> FontManagerType;

  class Rasterizer
  {
  public:

    Rasterizer(IGraphicsAGG& graphics) : mGraphics(graphics) {}
    
    agg::rgba8 GetPixel(int x, int y) { return mRenBase.pixel(x, y); }

    void SetOutput(agg::rendering_buffer& renBuf)
    {
      mPixf = PixfmtType(renBuf);
      mRenBase = RenbaseType(mPixf);
      mPixfPre = PixfmtPreType(renBuf);
      mRenBasePre = RenbasePreType(mPixfPre);
      mRenBase.clear(agg::rgba(1, 1, 1));
    }

    template <typename VertexSourceType>
    void Rasterize(VertexSourceType& path, agg::rgba8 color, agg::comp_op_e op)
    {
      SetPath(path);
      Rasterize(color, op);
    }
    
    void Rasterize(agg::rgba8 color, agg::comp_op_e op)
    {
      typedef agg::renderer_scanline_aa_solid<RenbaseType> RenderType;
      
      RenderType renderer(mRenBase);
      renderer.color(color);
      Render(renderer, op);
    }
    
    template <typename VertexSourceType, typename CustomSpanGeneratorType>
    void Rasterize(VertexSourceType& path, CustomSpanGeneratorType spanGenerator, agg::comp_op_e op)
    {
      SetPath(path);
      Rasterize(spanGenerator, op);
    }
    
    template <typename CustomSpanGeneratorType>
    void Rasterize(CustomSpanGeneratorType spanGenerator, agg::comp_op_e op)
    {
      typedef agg::renderer_scanline_aa<RenbaseType, SpanAllocatorType, CustomSpanGeneratorType> RendererType;
      
      SpanAllocatorType spanAllocator;
      RendererType renderer(mRenBase, spanAllocator, spanGenerator);
      Render(renderer, op);
    }
    
    template <typename VertexSourceType>
    void Rasterize(PixfmtType& src, VertexSourceType& path, agg::trans_affine& srcMtx, agg::comp_op_e op, agg::cover_type cover)
    {
      SetPath(path);
      Rasterize(src, srcMtx, op, cover);
    }
    
    void Rasterize(PixfmtType& src, agg::trans_affine& srcMtx, agg::comp_op_e op, agg::cover_type cover)
    {
      RenderBitmap(src, mRenBase, srcMtx, op, cover);
    }
    
    template <typename VertexSourceType>
    void Rasterize(PixfmtPreType& src, VertexSourceType& path, agg::trans_affine& srcMtx, agg::comp_op_e op, agg::cover_type cover)
    {
      SetPath(path);
      Rasterize(src, srcMtx, op, cover);
    }
    
    void Rasterize(PixfmtPreType& src, agg::trans_affine& srcMtx, agg::comp_op_e op, agg::cover_type cover)
    {
      RenderBitmap(src, mRenBasePre, srcMtx, op, cover);
    }
    
    void BlendFrom(agg::rendering_buffer& renBuf, const IRECT& bounds, int srcX, int srcY, agg::comp_op_e op, agg::cover_type cover, bool preMultiplied)
    {
      // N.B. blend_from/rect_i is inclusive, hence -1 on each dimension here
      
      agg::rect_i r(srcX, srcY, srcX + std::round(bounds.W()) - 1, srcY + std::round(bounds.H()) - 1);
      int x = std::round(bounds.L) - srcX;
      int y = std::round(bounds.T) - srcY;
      
      if (preMultiplied)
      {
        mPixfPre.comp_op(op);
        mRenBasePre.blend_from(PixfmtType(renBuf), &r, x, y, cover);
      }
      else
      {
        mPixf.comp_op(op);
        mRenBase.blend_from(PixfmtType(renBuf), &r, x, y, cover);
      }
    }
    
    template <typename VertexSourceType>
    void Rasterize(VertexSourceType& path, const IPattern& pattern, agg::comp_op_e op, float opacity, EFillRule rule = kFillWinding)
    {
      SetPath(path);
      Rasterize(pattern, op, opacity, rule);
    }
    
    void Rasterize(const IPattern& pattern, agg::comp_op_e op, float opacity, EFillRule rule = kFillWinding);

    template <typename VertexSourceType>
    void SetPath(VertexSourceType& path)
    {
      // Clip
      
      IRECT clip = mGraphics.mClipRECT.Empty() ? mGraphics.GetBounds() : mGraphics.mClipRECT;
      clip.Translate(mGraphics.XTranslate(), mGraphics.YTranslate());
      clip.Scale(mGraphics.GetBackingPixelScale());
      mRasterizer.clip_box(clip.L, clip.T, clip.R, clip.B);
      
      // Add path
      
      mRasterizer.reset();
      mRasterizer.add_path(path);
    }

  private:
    
    template <typename RendererType>
    void Render(RendererType& renderer, agg::comp_op_e op)
    {
      agg::scanline_p8 scanline;
      mPixf.comp_op(op);
      mPixfPre.comp_op(op);
      agg::render_scanlines(mRasterizer, scanline, renderer);
    }
    
    template <typename PixSourceType, typename RenderBaseType>
    void RenderBitmap(PixSourceType& src, RenderBaseType& renderbase, agg::trans_affine& srcMtx, agg::comp_op_e op, agg::cover_type cover)
    {
      typedef agg::image_accessor_clone<PixSourceType> ImgSrcType;
      typedef agg::span_image_filter_rgba_bilinear<ImgSrcType, InterpolatorType> FilterType;
      typedef alpha_span_generator<FilterType> CustomSpanGeneratorType;
      typedef agg::renderer_scanline_aa<RenderBaseType, SpanAllocatorType, CustomSpanGeneratorType> RendererType;
      
      SpanAllocatorType spanAllocator;
      InterpolatorType interpolator(srcMtx);
      ImgSrcType imgSrc(src);
      CustomSpanGeneratorType spanGenerator(imgSrc, interpolator, cover);
      RendererType renderer(renderbase, spanAllocator, spanGenerator);
      
      Render(renderer, op);
    }

    IGraphicsAGG& mGraphics;
    RenbaseType mRenBase;
    PixfmtType mPixf;
    RenbasePreType mRenBasePre;
    PixfmtPreType mPixfPre;
    agg::rasterizer_scanline_aa<> mRasterizer;
  };

  IGraphicsAGG(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  ~IGraphicsAGG();

  void DrawResize() override;

  void DrawBitmap(const IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;

  void PathClear() override { mPath.remove_all(); }
  void PathClose() override { mPath.close_polygon(); }

  void PathArc(float cx, float cy, float r, float aMin, float aMax) override;

  void PathMoveTo(float x, float y) override;
  void PathLineTo(float x, float y) override;
  void PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3) override;

  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override;
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override;
    
  IColor GetPoint(int x, int y) override;
  void* GetDrawContext() override { return nullptr; } //TODO
  const char* GetDrawingAPIStr() override { return "AGG"; }

  void UpdateLayer() override;
    
  void EndFrame() override;
  
  bool BitmapExtSupported(const char* ext) override;

protected:
  APIBitmap* LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext) override;
  APIBitmap* CreateAPIBitmap(int width, int height, int scale, double drawScale) override;

  bool LoadAPIFont(const char* fontID, const PlatformFontPtr& font) override;

  int AlphaChannel() const override { return PixelOrder().A; }
  bool FlippedBitmap() const override { return false; }

  void GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data) override;
  void ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow) override;

  bool DoDrawMeasureText(const IText& text, const char* str, IRECT& bounds, const IBlend* pBlend = 0, bool measure = false) override;

private:
  
  bool SetFont(const char* fontID, IFontData* pFont);

  double XTranslate()  { return mLayers.empty() ? 0 : -mLayers.top()->Bounds().L; }
  double YTranslate()  { return mLayers.empty() ? 0 : -mLayers.top()->Bounds().T; }
  
  void PathTransformSetMatrix(const IMatrix& m) override
  {
    const double scale = GetBackingPixelScale();
    IMatrix t = IMatrix().Scale(scale, scale).Translate(XTranslate(), YTranslate()).Transform(m);
      
    mTransform = agg::trans_affine(t.mXX, t.mYX, t.mXY, t.mYY, t.mTX, t.mTY);
  }
  
  void SetClipRegion(const IRECT& r) override { mClipRECT = r; }

  IRECT mClipRECT;
  FontEngineType mFontEngine;
  FontManagerType mFontManager;
  agg::rendering_buffer mRenBuf;
  agg::path_storage mPath;
  agg::trans_affine mTransform;
  PixelMapType mPixelMap;
  Rasterizer mRasterizer;
    
  //pipeline to process the vectors glyph paths(curves + contour)
  agg::conv_curve<FontManagerType::path_adaptor_type> mFontCurves;
  agg::conv_transform<agg::conv_curve<FontManagerType::path_adaptor_type>> mFontCurvesTransformed;
};
