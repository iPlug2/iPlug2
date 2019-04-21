/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <algorithm>
#include <cmath>

#include "IGraphicsAGG.h"

static StaticStorage<IFontData> sFontCache;

const bool textKerning = true;

class pixel_wrapper : public agg::pixel_map
{
public:
  pixel_wrapper(unsigned char* buf, unsigned w, unsigned h, unsigned bpp, int row_bytes)
  : m_buf(buf)
  , m_width(w)
  , m_height(h)
  , m_bpp(bpp)
  , m_row_bytes(row_bytes)
  {}
  
  unsigned char* buf() override { return m_buf; }
  unsigned width() const override { return m_width; }
  unsigned height() const override { return m_height; }
  
  int row_bytes() const override { return m_row_bytes; }
  unsigned bpp() const override  { return m_bpp; }
  
private:
  
  // Do not use!
  
  void create(unsigned width, unsigned height, unsigned clear_val=255) override {};
  void clear(unsigned clear_val=255) override {};
  void destroy() override {};
  
  unsigned char* m_buf;
  unsigned m_width;
  unsigned m_height;
  unsigned m_bpp;
  int m_row_bytes;
};

inline const agg::rgba8 AGGColor(const IColor& color, float opacity)
{
  return agg::rgba8(color.R, color.G, color.B, (opacity * color.A));
}

inline agg::comp_op_e AGGBlendMode(const IBlend* pBlend)
{
  if (!pBlend)
    return agg::comp_op_src_over;
  
  switch (pBlend->mMethod)
  {
    case kBlendDefault:         // fall through
    case kBlendClobber:         // fall through
    case kBlendSourceOver:      return agg::comp_op_src_over;
    case kBlendSourceIn:        return agg::comp_op_src_in;
    case kBlendSourceOut:       return agg::comp_op_src_out;
    case kBlendSourceAtop:      return agg::comp_op_src_atop;
    case kBlendDestOver:        return agg::comp_op_dst_over;
    case kBlendDestIn:          return agg::comp_op_dst_in;
    case kBlendDestOut:         return agg::comp_op_dst_out;
    case kBlendDestAtop:        return agg::comp_op_dst_atop;
    case kBlendAdd:             return agg::comp_op_plus;
    case kBlendXOR:             return agg::comp_op_xor;
  }
}

inline agg::cover_type AGGCover(const IBlend* pBlend = nullptr)
{
  return std::max(agg::cover_type(0), std::min(agg::cover_type(roundf(BlendWeight(pBlend) * 255.f)), agg::cover_type(255)));
}

agg::pixel_map* CreatePixmap(int w, int h)
{
  agg::pixel_map* pPixelMap = new IGraphicsAGG::PixelMapType();
  
  pPixelMap->create(w, h, 0);
  
  return pPixelMap;
}

// Rasterizing

template <typename FuncType, typename ColorArrayType>
void GradientRasterize(IGraphicsAGG::Rasterizer& rasterizer, const FuncType& gradientFunc, agg::trans_affine& xform, ColorArrayType& colorArray, agg::comp_op_e op)
{
  typedef agg::span_gradient<agg::rgba8, IGraphicsAGG::InterpolatorType, FuncType, ColorArrayType> SpanGradientType;
  
  IGraphicsAGG::InterpolatorType spanInterpolator(xform);
  SpanGradientType spanGradient(spanInterpolator, gradientFunc, colorArray, 0, 512);
  rasterizer.Rasterize(spanGradient, op);
}

template <typename FuncType, typename ColorArrayType>
void GradientRasterizeAdapt(IGraphicsAGG::Rasterizer& rasterizer, EPatternExtend extend, const FuncType& gradientFunc, agg::trans_affine& xform, ColorArrayType& colorArray, agg::comp_op_e op)
{
  switch (extend)
  {
    case kExtendNone: //TODO:  extend none
    case kExtendPad:
      GradientRasterize(rasterizer, gradientFunc, xform, colorArray, op);
      break;
    case kExtendReflect:
      GradientRasterize(rasterizer, agg::gradient_reflect_adaptor<FuncType>(gradientFunc), xform, colorArray, op);
      break;
    case kExtendRepeat:
      GradientRasterize(rasterizer, agg::gradient_repeat_adaptor<FuncType>(gradientFunc), xform, colorArray, op);
      break;
  }
}

void IGraphicsAGG::Rasterizer::Rasterize(const IPattern& pattern, agg::comp_op_e op, float opacity, EFillRule rule)
{
  mRasterizer.filling_rule(rule == kFillWinding ? agg::fill_non_zero : agg::fill_even_odd );
  
  switch (pattern.mType)
  {
    case kSolidPattern:
      Rasterize(AGGColor(pattern.GetStop(0).mColor, opacity), op);
    break;
    case kLinearPattern:
    case kRadialPattern:
    {
      // Common gradient objects
      const IMatrix& m = pattern.mTransform;
      
      agg::trans_affine gradientMTX(m.mXX, m.mYX , m.mXY, m.mYY, m.mTX, m.mTY);
      agg::gradient_lut<agg::color_interpolator<agg::rgba8>, 512> colorArray;
      
      // Scaling
      gradientMTX = (agg::trans_affine() / mGraphics.mTransform) * gradientMTX * agg::trans_affine_scaling(512.0);
      
      // Make gradient lut
      colorArray.remove_all();
      
      for (int i = 0; i < pattern.NStops(); i++)
      {
        const IColorStop& stop = pattern.GetStop(i);
        float offset = stop.mOffset;
        colorArray.add_color(offset, AGGColor(stop.mColor, opacity));
      }
      
      colorArray.build_lut();
      
      // Rasterize
      if (pattern.mType == kLinearPattern)
        GradientRasterizeAdapt(*this, pattern.mExtend, agg::gradient_y(), gradientMTX, colorArray, op);
      else
        GradientRasterizeAdapt(*this, pattern.mExtend, agg::gradient_radial_d(), gradientMTX, colorArray, op);
    }
    break;
  }
}

#pragma mark -

IGraphicsAGG::IGraphicsAGG(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGraphicsPathBase(dlg, w, h, fps, scale)
, mRasterizer(*this)
, mFontEngine()
, mFontManager(mFontEngine)
, mFontCurves(mFontManager.path_adaptor())
, mFontCurvesTransformed(mFontCurves, mTransform)
{
  DBGMSG("IGraphics AGG @ %i FPS\n", fps);
    
  StaticStorage<IFontData>::Accessor storage(sFontCache);
  storage.Retain();
}

IGraphicsAGG::~IGraphicsAGG()
{
  StaticStorage<IFontData>::Accessor storage(sFontCache);
  storage.Release();
}

void IGraphicsAGG::DrawResize()
{
  mPixelMap.create(WindowWidth() * GetScreenScale(), WindowHeight() * GetScreenScale());
  UpdateLayer();
  mRasterizer.SetOutput(mRenBuf);
    
  mTransform = agg::trans_affine_scaling(GetBackingPixelScale(), GetBackingPixelScale());
}

void IGraphicsAGG::UpdateLayer()
{
  agg::pixel_map* pPixelMap = mLayers.empty() ? &mPixelMap : mLayers.top()->GetAPIBitmap()->GetBitmap();
  mRenBuf.attach(pPixelMap->buf(), pPixelMap->width(), pPixelMap->height(), pPixelMap->row_bytes());
}

bool IGraphicsAGG::LoadAPIFont(const char* fontID, const PlatformFontPtr& font)
{
  StaticStorage<IFontData>::Accessor storage(sFontCache);
  IFontData* cached = storage.Find(fontID);
  
  if (cached)
  {
    SetFont(fontID, cached);
    return true;
  }
  
  IFontDataPtr data = font->GetFontData();

  if (data->IsValid() && SetFont(fontID, data.get()))
  {
    storage.Add(data.release(), fontID);
    return true;
  }

  return false;
}

bool CheckTransform(const agg::trans_affine& mtx)
{
  if (!agg::is_equal_eps(mtx.tx - std::round(mtx.tx), 0.0, 1e-3))
    return false;
  if (!agg::is_equal_eps(mtx.ty - std::round(mtx.ty), 0.0, 1e-3))
    return false;

  agg::trans_affine mtx_without_translate(mtx);
  mtx_without_translate.tx = mtx_without_translate.ty = 0.0;
  
  return mtx_without_translate.is_identity(1e-3);
}

void IGraphicsAGG::DrawBitmap(const IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend)
{
  bool preMultiplied = static_cast<AGGBitmap*>(bitmap.GetAPIBitmap())->IsPreMultiplied();
  IRECT bounds = mClipRECT.Empty() ? dest : mClipRECT.Intersect(dest);
  bounds.Scale(GetBackingPixelScale());

  APIBitmap* pAPIBitmap = dynamic_cast<AGGBitmap*>(bitmap.GetAPIBitmap());
  agg::pixel_map* pSource = pAPIBitmap->GetBitmap();
  agg::rendering_buffer src(pSource->buf(), pSource->width(), pSource->height(), pSource->row_bytes());

  agg::trans_affine srcMtx;
  srcMtx /= mTransform;
  srcMtx *= agg::trans_affine_translation(srcX - dest.L, srcY - dest.T);
  srcMtx *= agg::trans_affine_scaling(bitmap.GetScale() * bitmap.GetDrawScale());
    
  if (bounds.IsPixelAligned() && CheckTransform(srcMtx))
  {
    double scale = GetScreenScale() * GetScreenScale() / pAPIBitmap->GetScale();
    IRECT destScaled = dest.GetScaled(GetBackingPixelScale());
    srcX = std::round(srcX * scale + std::max(0.f, bounds.L - destScaled.L));
    srcY = std::round(srcY * scale + std::max(0.f, bounds.T - destScaled.T));
    bounds.Translate(mTransform.tx, mTransform.ty);

    mRasterizer.BlendFrom(src, bounds, srcX, srcY, AGGBlendMode(pBlend), AGGCover(pBlend), preMultiplied);
  }
  else
  {
    agg::rounded_rect rect(dest.L, dest.T, dest.R, dest.B, 0);
    agg::conv_transform<agg::rounded_rect> tr(rect, mTransform);
      
    if (preMultiplied)
    {
      PixfmtPreType fmtSrc(src);
      mRasterizer.Rasterize(fmtSrc, tr, srcMtx, AGGBlendMode(pBlend), AGGCover(pBlend));
    }
    else
    {
      PixfmtType fmtSrc(src);
      mRasterizer.Rasterize(fmtSrc, tr, srcMtx, AGGBlendMode(pBlend), AGGCover(pBlend));
    }
  }
}

void IGraphicsAGG::PathArc(float cx, float cy, float r, float aMin, float aMax)
{
  agg::path_storage transformedPath;
    
  agg::arc arc(cx, cy, r, r, DegToRad(aMin - 90.f), DegToRad(aMax - 90.f));
  arc.approximation_scale(mTransform.scale());
    
  transformedPath.join_path(arc);
  transformedPath.transform(mTransform);
  
  mPath.join_path(transformedPath);
}

void IGraphicsAGG::PathMoveTo(float x, float y)
{
  double xd = x;
  double yd = y;
  
  mTransform.transform(&xd, &yd);
  mPath.move_to(xd, yd);
}

void IGraphicsAGG::PathLineTo(float x, float y)
{
  double xd = x;
  double yd = y;

  mTransform.transform(&xd, &yd);
  mPath.line_to(xd, yd);
}

void IGraphicsAGG::PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3)
{
  double x1d = x1;
  double y1d = y1;
  double x2d = x2;
  double y2d = y2;
  double x3d = x3;
  double y3d = y3;
  
  mTransform.transform(&x1d, &y1d);
  mTransform.transform(&x2d, &y2d);
  mTransform.transform(&x3d, &y3d);

  mPath.curve4(x1d, y1d, x2d, y2d, x3d, y3d);
}

template<typename StrokeType>
void StrokeOptions(StrokeType& strokes, double thickness, const IStrokeOptions& options)
{
  strokes.width(thickness);
  
  switch (options.mCapOption)
  {
    case kCapButt:   strokes.line_cap(agg::butt_cap);     break;
    case kCapRound:  strokes.line_cap(agg::round_cap);    break;
    case kCapSquare: strokes.line_cap(agg::square_cap);   break;
  }
  
  switch (options.mJoinOption)
  {
    case kJoinMiter:   strokes.line_join(agg::miter_join);   break;
    case kJoinRound:   strokes.line_join(agg::round_join);   break;
    case kJoinBevel:   strokes.line_join(agg::bevel_join);   break;
  }
  
  strokes.miter_limit(options.mMiterLimit);
}

void IGraphicsAGG::PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend)
{
  typedef agg::conv_curve<agg::path_storage>    CPType;
  typedef agg::conv_transform<CPType>           S1Type;
  typedef agg::conv_stroke<S1Type>              S2Type;
  typedef agg::conv_transform<S2Type>           S3Type;
  typedef agg::conv_dash<S1Type>                D2Type;
  typedef agg::conv_stroke<D2Type>              D3Type;
  typedef agg::conv_transform<D3Type>           D4Type;

  agg::trans_affine tranform(mTransform);
  CPType curvedPath(mPath);
  S1Type basePath(curvedPath, tranform.invert());

  if (options.mDash.GetCount())
  {
    D2Type dashedPath(basePath);
    D3Type strokedDashedPath(dashedPath);
    D4Type finalPath(strokedDashedPath, mTransform);
      
    // Set the dashes (N.B. - for odd counts the array is read twice)
    int dashCount = options.mDash.GetCount();
    int dashMax = dashCount & 1 ? dashCount * 2 : dashCount;
    const float* dashArray = options.mDash.GetArray();
    
    dashedPath.remove_all_dashes();
    dashedPath.dash_start(options.mDash.GetOffset());
    
    for (int i = 0; i < dashMax; i += 2)
      dashedPath.add_dash(dashArray[i % dashCount], dashArray[(i + 1) % dashCount]);
    
    StrokeOptions(strokedDashedPath, thickness, options);
    mRasterizer.Rasterize(finalPath, pattern, AGGBlendMode(pBlend), BlendWeight(pBlend));
  }
  else
  {
    S2Type strokedPath(basePath);
    S3Type finalPath(strokedPath, mTransform);
      
    StrokeOptions(strokedPath, thickness, options);
    mRasterizer.Rasterize(finalPath, pattern, AGGBlendMode(pBlend), BlendWeight(pBlend));
  }
  
  if (!options.mPreserve)
    mPath.remove_all();
}

void IGraphicsAGG::PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend)
{
  agg::conv_curve<agg::path_storage> curvedPath(mPath);
  mRasterizer.Rasterize(curvedPath, pattern, AGGBlendMode(pBlend), BlendWeight(pBlend), options.mFillRule);
  if (!options.mPreserve)
    mPath.remove_all();
}

IColor IGraphicsAGG::GetPoint(int x, int y)
{
  agg::rgba8 point = mRasterizer.GetPixel(x, y);
  IColor color(point.a, point.r, point.g, point.b);
  return color;
}

APIBitmap* IGraphicsAGG::LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext)
{
  std::unique_ptr<PixelMapType> pixelMap(new PixelMapType());
  bool ispng = strstr(fileNameOrResID, "png") != nullptr;

#if defined OS_WIN
  if (location != EResourceLocation::kNotFound && ispng)
  {
    if (pixelMap->load_img((HINSTANCE)GetWinModuleHandle(), fileNameOrResID, agg::pixel_map::format_png))
      return new AGGBitmap(pixelMap.release(), scale, 1.f, false);
  }
#else
  if (location == EResourceLocation::kAbsolutePath && ispng)
  {
    if (pixelMap->load_img(fileNameOrResID, agg::pixel_map::format_png))
      return new AGGBitmap(pixelMap.release(), scale, 1.f, false);
  }
#endif

  return new APIBitmap();
}

APIBitmap* IGraphicsAGG::CreateAPIBitmap(int width, int height, int scale, double drawScale)
{
  return new AGGBitmap(CreatePixmap(width, height), scale, drawScale, true);
}

bool IGraphicsAGG::BitmapExtSupported(const char* ext)
{
  char extLower[32];
  ToLower(extLower, ext);
  return (strstr(extLower, "png") != nullptr) /*|| (strstr(extLower, "jpg") != nullptr) || (strstr(extLower, "jpeg") != nullptr)*/;
}

void IGraphicsAGG::GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data)
{
  const APIBitmap* pBitmap = layer->GetAPIBitmap();
  int size = pBitmap->GetBitmap()->height() * pBitmap->GetBitmap()->row_bytes();
    
  data.Resize(size);
    
  if (data.GetSize() >= size)
    memcpy(data.Get(), pBitmap->GetBitmap()->buf(), size);
}

void IGraphicsAGG::ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow)
{
  const APIBitmap* pBitmap = layer->GetAPIBitmap();
  agg::pixel_map* pPixMap = pBitmap->GetBitmap();
  int size = pPixMap->height() * pPixMap->row_bytes();
    
  if (mask.GetSize() >= size)
  {
    if (!shadow.mDrawForeground)
    {
      pBitmap->GetBitmap()->clear(0);
    }
    
    IRECT bounds(layer->Bounds());
    pixel_wrapper* shadowSource = new pixel_wrapper(mask.Get(), pPixMap->width(), pPixMap->height(), pPixMap->bpp(), pPixMap->row_bytes());
    APIBitmap* shadowBitmap = new AGGBitmap(shadowSource, pBitmap->GetScale(), pBitmap->GetDrawScale(), true);
    IBitmap bitmap(shadowBitmap, 1, false);
    ILayer shadowLayer(shadowBitmap, layer->Bounds());
      
    PathTransformSave();
    PushLayer(layer.get());
    PushLayer(&shadowLayer);
    PathRect(layer->Bounds());
    IBlend blend1(kBlendSourceIn, 1.0);
    PathTransformTranslate(-shadow.mXOffset, -shadow.mYOffset);
    PathFill(shadow.mPattern, IFillOptions(), &blend1);
    PopLayer();
    IBlend blend2(kBlendDestOver, shadow.mOpacity);
    bounds.Translate(shadow.mXOffset, shadow.mYOffset);
    DrawBitmap(bitmap, bounds, 0, 0, &blend2);
    PopLayer();
    PathTransformRestore();
  }
}

void IGraphicsAGG::EndFrame()
{
#ifdef OS_MAC
  CGContext* pCGContext = (CGContext*) GetPlatformContext();
  CGContextSaveGState(pCGContext);
  CGContextTranslateCTM(pCGContext, 0.0, WindowHeight());
  CGContextScaleCTM(pCGContext, 1.0, -1.0);
  mPixelMap.draw(pCGContext, GetScreenScale());
  CGContextRestoreGState(pCGContext);
#else
  PAINTSTRUCT ps;
  HWND hWnd = (HWND) GetWindow();
  HDC dc = BeginPaint(hWnd, &ps);
  mPixelMap.draw(dc, 0, 0, 1.0);
  EndPaint(hWnd, &ps);
#endif
}

bool IGraphicsAGG::SetFont(const char* fontID, IFontData* pFont) const
{
  agg::glyph_rendering render = agg::glyph_ren_outline;
  return mFontEngine.load_font(fontID, pFont->GetFaceIdx(), render, (char*) pFont->Get(), pFont->GetSize());
}

void IGraphicsAGG::PrepareAndMeasureText(const IText& text, const char* str, IRECT& r, double& x, double & y) const
{
  StaticStorage<IFontData>::Accessor storage(sFontCache);
  IFontData* pFont = storage.Find(text.mFont);
  
  if (!pFont || !SetFont(text.mFont, pFont))
  {
    assert(0 && "No font found - did you forget to load it?");
  }
    
  const bool textHinting = false;
    
  // Set dpi to 72 to allow finer resolution of text sizes
  mFontEngine.resolution(72);
  mFontEngine.hinting(textHinting);
  mFontEngine.height(text.mSize * pFont->GetHeightEMRatio());
  mFontEngine.flip_y(true);
  
  const double textHeight = text.mSize;
  const double EMHeight = pFont->GetAscender() - pFont->GetDescender();
  const double ascender = text.mSize * pFont->GetAscender() / EMHeight;
  const double descender = text.mSize * pFont->GetDescender() / EMHeight;
  
  mFontManager.reset_last_glyph();
  double textWidth = 0.0;
  
  for (int i = 0; str[i]; i++)
  {
    const agg::glyph_cache* pGlyph = mFontManager.glyph(str[i]);
    
    if (textKerning)
    {
      double dx = 0.0;
      double dy = 0.0;
      mFontManager.add_kerning(&dx, &dy);
      textWidth += dx;
    }
    
    textWidth += pGlyph->advance_x;
  }
  
  switch (text.mAlign)
  {
    case IText::kAlignNear:     x = r.L;                          break;
    case IText::kAlignCenter:   x = r.MW() - (textWidth / 2.0);   break;
    case IText::kAlignFar:      x = r.R - textWidth;              break;
  }
  
  switch (text.mVAlign)
  {
    case IText::kVAlignTop:      y = r.T + ascender;                            break;
    case IText::kVAlignMiddle:   y = r.MH() + descender + (textHeight / 2.0);   break;
    case IText::kVAlignBottom:   y = r.B + descender;                           break;
  }
  
  r = IRECT((float) x, (float) y - ascender, (float) (x + textWidth), (float) (y + textHeight - ascender));
}

void IGraphicsAGG::DoMeasureText(const IText& text, const char* str, IRECT& bounds) const
{
  IRECT r = bounds;
  double x, y;
  PrepareAndMeasureText(text, str, bounds, x, y);
  DoMeasureTextRotation(text, r, bounds);
}

void IGraphicsAGG::DoDrawText(const IText& text, const char* str, const IRECT& bounds, const IBlend* pBlend)
{
  IRECT measured = bounds;
  double x, y;
  
  agg::rgba8 color(AGGColor(text.mFGColor, BlendWeight(pBlend)));
  mFontManager.reset_last_glyph();
  
  PrepareAndMeasureText(text, str, measured, x, y);
  PathTransformSave();
  DoTextRotation(text, bounds, measured);

  for (size_t c = 0; str[c]; c++)
  {
    const agg::glyph_cache* pGlyph = mFontManager.glyph(str[c]);
    
    if (pGlyph)
    {
      if (textKerning)
      {
        mFontManager.add_kerning(&x, &y);
      }
      
      mFontManager.init_embedded_adaptors(pGlyph, x, y);
      mRasterizer.Rasterize(mFontCurvesTransformed, color, AGGBlendMode(pBlend));
    }
    x += pGlyph->advance_x;
    y += pGlyph->advance_y;
  }
  PathTransformRestore();
}

#include "IGraphicsAGG_src.cpp"
