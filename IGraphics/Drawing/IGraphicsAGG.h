/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IGraphicsPathBase.h"
#include "IGraphicsAGG_src.h"

#include "heapbuf.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** IGraphics draw class using Antigrain Geometry
*   @ingroup DrawClasses*/
class IGraphicsAGG : public IGraphicsPathBase
{
private:
  class Bitmap;
  
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
  
#ifdef OS_WIN
  using PixelOrder = agg::order_bgra;
  using PixelMapType = agg::pixel_map_win32;
#elif defined OS_MAC
  using PixelOrder = agg::order_argb;
  using PixelMapType = agg::pixel_map_mac;
#else
#error NOT IMPLEMENTED
#endif
  using SpanAllocatorType = agg::span_allocator<agg::rgba8>;
  using InterpolatorType = agg::span_interpolator_linear<>;
  // Pre-multiplied source types
  using BlenderPreType = agg::comp_op_adaptor_rgba_pre<agg::rgba8, PixelOrder>;
  using PixfmtPreType = agg::pixfmt_custom_blend_rgba<BlenderPreType, agg::rendering_buffer>;
  using RenbasePreType = agg::renderer_base <PixfmtPreType>;
   // Non pre-multiplied source types
  using BlenderType = agg::comp_op_adaptor_rgba<agg::rgba8, PixelOrder>;
  using PixfmtType = agg::pixfmt_custom_blend_rgba<BlenderType, agg::rendering_buffer>;
  using RenbaseType = agg::renderer_base <PixfmtType>;
  // Image bitmap types
  using ImgSourceType = agg::image_accessor_clone<PixfmtType>;
  using SpanGeneratorType = agg::span_image_filter_rgba_bilinear<ImgSourceType, InterpolatorType>;
  using BitmapRenderType = agg::renderer_scanline_aa<RenbaseType, SpanAllocatorType, SpanGeneratorType>;
  // Font types
  using FontEngineType = agg::font_engine_freetype_int32;
  using FontManagerType = agg::font_cache_manager<FontEngineType>;

  class Rasterizer
  {
  public:
    Rasterizer(IGraphicsAGG& graphics) : mGraphics(graphics) {}
    
    Rasterizer(const Rasterizer&) = delete;
    Rasterizer& operator=(const Rasterizer&) = delete;
      
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
      using RenderType = agg::renderer_scanline_aa_solid<RenbaseType>;
      
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
      using RendererType = agg::renderer_scanline_aa<RenbaseType, SpanAllocatorType, CustomSpanGeneratorType>;
      
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
    void Rasterize(VertexSourceType& path, const IPattern& pattern, agg::comp_op_e op, float opacity, EFillRule rule = EFillRule::Winding)
    {
      SetPath(path);
      Rasterize(pattern, op, opacity, rule);
    }
    
    void Rasterize(const IPattern& pattern, agg::comp_op_e op, float opacity, EFillRule rule = EFillRule::Winding);

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
      using ImgSrcType = agg::image_accessor_clone<PixSourceType>;
      using FilterType = agg::span_image_filter_rgba_bilinear<ImgSrcType, InterpolatorType>;
      using CustomSpanGeneratorType = alpha_span_generator<FilterType>;
      using RendererType = agg::renderer_scanline_aa<RenderBaseType, SpanAllocatorType, CustomSpanGeneratorType>;
      
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
public:
  IGraphicsAGG(IGEditorDelegate& dlg, int w, int h, int fps, float scale);
  ~IGraphicsAGG();

  const char* GetDrawingAPIStr() override { return "AGG"; }

  void DrawResize() override;

  void DrawBitmap(const IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend) override;

  void PathClear() override { mPath.remove_all(); }
  void PathClose() override { mPath.close_polygon(); }
  void PathArc(float cx, float cy, float r, float a1, float a2, EWinding winding) override;
  void PathMoveTo(float x, float y) override;
  void PathLineTo(float x, float y) override;
  void PathCubicBezierTo(float c1x, float c1y, float c2x, float c2y, float x2, float y2) override;
  void PathQuadraticBezierTo(float cx, float cy, float x2, float y2) override;
  void PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend) override;
  void PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend) override;
    
  IColor GetPoint(int x, int y) override;
  void* GetDrawContext() override { return nullptr; } //TODO

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

  void DoMeasureText(const IText& text, const char* str, IRECT& bounds) const override;
  void DoDrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend) override;

private:
  void PrepareAndMeasureText(const IText& text, const char* str, IRECT& r, double& x, double & y) const;
  bool SetFont(const char* fontID, IFontData* pFont) const;

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
  mutable FontEngineType mFontEngine;
  mutable FontManagerType mFontManager;
  agg::rendering_buffer mRenBuf;
  agg::path_storage mPath;
  agg::trans_affine mTransform;
  PixelMapType mPixelMap;
  Rasterizer mRasterizer;

  //pipeline to process the vectors glyph paths(curves + contour)
  agg::conv_curve<FontManagerType::path_adaptor_type> mFontCurves;
  agg::conv_transform<agg::conv_curve<FontManagerType::path_adaptor_type>> mFontCurvesTransformed;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

