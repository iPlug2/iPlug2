/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "IGraphicsCanvas.h"
#include <string>
#include <utility>
#include <stdio.h>
#include <emscripten.h>

#include "wdl_base64.h"

using namespace emscripten;

extern IGraphics* gGraphics;

extern val GetPreloadedImages();
extern val GetCanvas();

// Fonts

typedef std::pair<WDL_String, WDL_String> FontDescType;
StaticStorage<FontDescType> sFontCache;

// Color Utility

static std::string CanvasColor(const IColor& color, float alpha = 1.0)
{
  WDL_String str;
  str.SetFormatted(64, "rgba(%d, %d, %d, %lf)", color.R, color.G, color.B, alpha * color.A / 255.0);
  return str.Get();
}

// Bitmap

CanvasBitmap::CanvasBitmap(val imageCanvas, const char* name, int scale)
{
  SetBitmap(new val(imageCanvas), imageCanvas["width"].as<int>(), imageCanvas["height"].as<int>(), scale, 1.f);
}

CanvasBitmap::CanvasBitmap(int width, int height, int scale, float drawScale)
{
  val canvas = val::global("document").call<val>("createElement", std::string("canvas"));
  canvas.set("width", width);
  canvas.set("height", height);

  SetBitmap(new val(canvas), width, height, scale, drawScale);
}

CanvasBitmap::~CanvasBitmap()
{
  delete GetBitmap();
}

IGraphicsCanvas::IGraphicsCanvas(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGraphicsPathBase(dlg, w, h, fps, scale)
{
  StaticStorage<FontDescType>::Accessor storage(sFontCache);
  storage.Retain();
}

IGraphicsCanvas::~IGraphicsCanvas()
{
  StaticStorage<FontDescType>::Accessor storage(sFontCache);
  storage.Release();
}

void IGraphicsCanvas::DrawBitmap(const IBitmap& bitmap, const IRECT& bounds, int srcX, int srcY, const IBlend* pBlend)
{
  val context = GetContext();
  val img = *bitmap.GetAPIBitmap()->GetBitmap();
  context.call<void>("save");
  SetCanvasBlendMode(context, pBlend);
  context.set("globalAlpha", BlendWeight(pBlend));
    
  const float bs = bitmap.GetScale();
  IRECT sr = bounds;
  sr.Scale(bs * bitmap.GetDrawScale());

  PathRect(bounds);
  context.call<void>("clip");
  context.call<void>("drawImage", img, srcX * bs, srcY * bs, sr.W(), sr.H(), bounds.L, bounds.T, bounds.W(), bounds.H());
  GetContext().call<void>("restore");
}

void IGraphicsCanvas::PathClear()
{
  GetContext().call<void>("beginPath");
}

void IGraphicsCanvas::PathClose()
{
  GetContext().call<void>("closePath");
}

void IGraphicsCanvas::PathArc(float cx, float cy, float r, float aMin, float aMax)
{
  GetContext().call<void>("arc", cx, cy, r, DegToRad(aMin - 90.f), DegToRad(aMax - 90.f));
}

void IGraphicsCanvas::PathMoveTo(float x, float y)
{
  GetContext().call<void>("moveTo", x, y);
}

void IGraphicsCanvas::PathLineTo(float x, float y)
{
  GetContext().call<void>("lineTo", x, y);
}

void IGraphicsCanvas::PathCurveTo(float x1, float y1, float x2, float y2, float x3, float y3)
{
  GetContext().call<void>("bezierCurveTo", x1, y1, x2, y2, x3, y3);
}

void IGraphicsCanvas::PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend)
{
  val context = GetContext();
  
  switch (options.mCapOption)
  {
    case kCapButt: context.set("lineCap", "butt"); break;
    case kCapRound: context.set("lineCap", "round"); break;
    case kCapSquare: context.set("lineCap", "square"); break;
  }
  
  switch (options.mJoinOption)
  {
    case kJoinMiter: context.set("lineJoin", "miter"); break;
    case kJoinRound: context.set("lineJoin", "round"); break;
    case kJoinBevel: context.set("lineJoin", "bevel"); break;
  }
  
  context.set("miterLimit", options.mMiterLimit);
    
  val dashArray = val::array();
  
  for (int i = 0; i < options.mDash.GetCount(); i++)
    dashArray.call<void>("push", val(*(options.mDash.GetArray() + i)));
  
  context.call<void>("setLineDash", dashArray);
  context.set("lineDashOffset", options.mDash.GetOffset());
  context.set("lineWidth", thickness);
  
  SetCanvasSourcePattern(context, pattern, pBlend);

  context.call<void>("stroke");
  
  if (!options.mPreserve)
    PathClear();
}

void IGraphicsCanvas::PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend)
{
  val context = GetContext();
  std::string fillRule(options.mFillRule == kFillWinding ? "nonzero" : "evenodd");
  
  SetCanvasSourcePattern(context, pattern, pBlend);

  context.call<void>("fill", fillRule);

  if (!options.mPreserve)
    PathClear();
}

void IGraphicsCanvas::SetCanvasSourcePattern(val& context, const IPattern& pattern, const IBlend* pBlend)
{
  SetCanvasBlendMode(context, pBlend);
  
  switch (pattern.mType)
  {
    case kSolidPattern:
    {
      const IColor color = pattern.GetStop(0).mColor;
      std::string colorString = CanvasColor(color, BlendWeight(pBlend));

      context.set("fillStyle", colorString);
      context.set("strokeStyle", colorString);
    }
    break;
      
    case kLinearPattern:
    case kRadialPattern:
    {
      double x, y;
      IMatrix m = IMatrix(pattern.mTransform).Invert();
      m.TransformPoint(x, y, 0.0, 1.0);
        
      val gradient = (pattern.mType == kLinearPattern) ?
        context.call<val>("createLinearGradient", m.mTX, m.mTY, x, y) :
        context.call<val>("createRadialGradient", m.mTX, m.mTY, 0.0, m.mTX, m.mTY, m.mXX);
      
      for (int i = 0; i < pattern.NStops(); i++)
      {
        const IColorStop& stop = pattern.GetStop(i);
        gradient.call<void>("addColorStop", stop.mOffset, CanvasColor(stop.mColor));
      }
      
      context.set("fillStyle", gradient);
      context.set("strokeStyle", gradient);
    }
    break;
  }
}

void IGraphicsCanvas::SetCanvasBlendMode(val& context, const IBlend* pBlend)
{
  if (!pBlend)
    context.set("globalCompositeOperation", "source-over");
  
  switch (pBlend->mMethod)
  {
    case kBlendDefault:       // fall through
    case kBlendClobber:       // fall through
    case kBlendSourceOver:    context.set("globalCompositeOperation", "source-over");        break;
    case kBlendSourceIn:      context.set("globalCompositeOperation", "source-in");          break;
    case kBlendSourceOut:     context.set("globalCompositeOperation", "source-out");         break;
    case kBlendSourceAtop:    context.set("globalCompositeOperation", "source-atop");        break;
    case kBlendDestOver:      context.set("globalCompositeOperation", "destination-over");   break;
    case kBlendDestIn:        context.set("globalCompositeOperation", "destination-in");     break;
    case kBlendDestOut:       context.set("globalCompositeOperation", "destination-out");    break;
    case kBlendDestAtop:      context.set("globalCompositeOperation", "destination-atop");   break;
    case kBlendAdd:           context.set("globalCompositeOperation", "lighter");            break;
    case kBlendXOR:           context.set("globalCompositeOperation", "xor");                break;
  }
}

bool IGraphicsCanvas::DoDrawMeasureText(const IText& text, const char* str, IRECT& bounds, const IBlend* pBlend, bool measure)
{
  StaticStorage<FontDescType>::Accessor storage(sFontCache);
  FontDescType* descriptor = storage.Find(text.mFont);
    
  assert(descriptor && "No font found - did you forget to load it?");
    
  // TODO: orientation
  val context = GetContext();
  std::string textString(str);
  
  char fontString[FONT_LEN + 64];
  context.set("textBaseline", std::string("top"));
  sprintf(fontString, "%s %dpx %s", descriptor->second.Get(), text.mSize, descriptor->first.Get());
  context.set("font", std::string(fontString));
  val metrics = context.call<val>("measureText", textString);
  double textWidth = metrics["width"].as<double>();
  double textHeight = text.mSize;
  //EM_ASM_DOUBLE({return parseFloat(document.getElementById("canvas").getContext("2d").font);});

  if (measure)
  {
    bounds = IRECT(0, 0, (float) textWidth, (float) textHeight);
    return true;
  }
  else
  {
    double x = bounds.L;
    double y = bounds.T;
    
    switch (text.mAlign)
    {
      case IText::kAlignNear:     break;
      case IText::kAlignCenter:   x = bounds.MW() - (textWidth / 2.0);    break;
      case IText::kAlignFar:      x = bounds.R - textWidth;               break;
    }
    
    switch (text.mVAlign)
    {
      case IText::EVAlign::kVAlignTop:
        y = bounds.T;
        context.set("textBaseline", std::string("top"));
        break;
      case IText::EVAlign::kVAlignMiddle:
        y = bounds.MH();
        context.set("textBaseline", std::string("middle"));
        break;
      case IText::EVAlign::kVAlignBottom:
        y = bounds.B;
        context.set("textBaseline", std::string("bottom"));
        break;
    }

    context.call<void>("save");
    PathRect(bounds);
    context.call<void>("clip");
    PathClear();
    SetCanvasSourcePattern(context, text.mFGColor, pBlend);
    context.call<void>("fillText", textString, x, y);
    context.call<void>("restore");
  }
  
  return true;
}

void IGraphicsCanvas::PathTransformSetMatrix(const IMatrix& m)
{
  const double scale = GetBackingPixelScale();
  IMatrix t = IMatrix().Scale(scale, scale).Translate(XTranslate(), YTranslate()).Transform(m);

  GetContext().call<void>("setTransform", t.mXX, t.mYX, t.mXY, t.mYY, t.mTX, t.mTY);
}

void IGraphicsCanvas::SetClipRegion(const IRECT& r)
{
  val context = GetContext();
  context.call<void>("restore");
  context.call<void>("save");
  if (!r.Empty())
  {
    context.call<void>("beginPath");
    context.call<void>("rect", r.L, r.T, r.W(), r.H());
    context.call<void>("clip");
    context.call<void>("beginPath");
  }
}

bool IGraphicsCanvas::BitmapExtSupported(const char* ext)
{
  char extLower[32];
  ToLower(extLower, ext);
  return (strstr(extLower, "png") != nullptr) || (strstr(extLower, "jpg") != nullptr) || (strstr(extLower, "jpeg") != nullptr);
}

APIBitmap* IGraphicsCanvas::LoadAPIBitmap(const char* fileNameOrResID, int scale, EResourceLocation location, const char* ext)
{
  return new CanvasBitmap(GetPreloadedImages()[fileNameOrResID], fileNameOrResID + 1, scale);
}

APIBitmap* IGraphicsCanvas::CreateAPIBitmap(int width, int height, int scale, double drawScale)
{
  return new CanvasBitmap(width, height, scale, drawScale);
}

bool IGraphicsCanvas::CompareFontMetrics(const char* style, const char* font1, const char* font2, int size)
{
  val context = GetContext();
  std::string textString("@BmwdWMoqPYyzZr1234567890.+-=_~'");
  
  char fontString[FONT_LEN + 64];
  
  sprintf(fontString, "%s %dpx %s", style, size, font2);
  context.set("font", std::string(fontString));
  val metrics1 = context.call<val>("measureText", textString);

  sprintf(fontString, "%s %dpx %s, %s", style, size, font1, font2);
  context.set("font", std::string(fontString));
  val metrics2 = context.call<val>("measureText", textString);
  
  return metrics1["width"].as<double>() == metrics2["width"].as<double>();
}

bool IGraphicsCanvas::LoadAPIFont(const char* fontID, const PlatformFontPtr& font)
{
  StaticStorage<FontDescType>::Accessor storage(sFontCache);

  if (storage.Find(fontID))
    return true;

  IFontDataPtr data = font->GetFontData();
    
  if (data->IsValid())
  {
    // Embed the font data in base64 format as CSS in the head of the html
    
    WDL_TypedBuf<char> base64Encoded;
    
    if (!base64Encoded.ResizeOK(((data->GetSize() * 4) + 3) / 3 + 1))
      return false;
    
    wdl_base64encode(data->Get(), base64Encoded.Get(), data->GetSize());
    std::string htmlText("@font-face { font-family: '");
    htmlText.append(fontID);
    htmlText.append("'; src: url(data:font/ttf;base64,");
    htmlText.append(base64Encoded.Get());
    htmlText.append(") format('truetype'); }");
    val document = val::global("document");
    val documentHead = document["head"];
    val css = document.call<val>("createElement", std::string("style"));
    css.set("type", std::string("text/css"));
    css.set("innerHTML", htmlText);
    document["head"].call<void>("appendChild", css);
      
    const FontDescType* descriptor = reinterpret_cast<const FontDescType*>(font->GetDescriptor());
    storage.Add(new FontDescType{descriptor->first, descriptor->second}, fontID);
      
    return true;
  }
  
  bool found = false;
  const FontDescType* descriptor = reinterpret_cast<const FontDescType*>(font->GetDescriptor());
  const char* fontName = descriptor->first.Get();
  const char* styleName = descriptor->second.Get();
  
  if (!CompareFontMetrics(styleName, fontName, "monospace", 72))
    found = true;
  if (!found && !CompareFontMetrics(styleName, fontName, "sans-serif", 72))
    found = true;
  if (!found && !CompareFontMetrics(styleName, fontName, "serif", 72))
    found = true;
  
  if (found)
  {
    storage.Add(new FontDescType{descriptor->first, descriptor->second}, fontID);
  }
    
  return found;
}

void IGraphicsCanvas::GetLayerBitmapData(const ILayerPtr& layer, RawBitmapData& data)
{
  const APIBitmap* pBitmap = layer->GetAPIBitmap();
  int size = pBitmap->GetWidth() * pBitmap->GetHeight() * 4;
  val context = pBitmap->GetBitmap()->call<val>("getContext", std::string("2d"));
  val imageData = context.call<val>("getImageData", 0, 0, pBitmap->GetWidth(), pBitmap->GetHeight());
  val pixelData = imageData["data"];
  data.Resize(size);
  
  // Copy pixels from context
  
  if (data.GetSize() >= size)
  {
    unsigned char* out = data.Get();
    
    for (auto i = 0; i < size; i++)
      out[i] = pixelData[i].as<unsigned char>();
  }
}

void IGraphicsCanvas::ApplyShadowMask(ILayerPtr& layer, RawBitmapData& mask, const IShadow& shadow)
{
  const APIBitmap* pBitmap = layer->GetAPIBitmap();
  int size = pBitmap->GetWidth() * pBitmap->GetHeight() * 4;
  
  if (mask.GetSize() >= size)
  {
    int width = pBitmap->GetWidth();
    int height = pBitmap->GetHeight();
    double scale = pBitmap->GetScale() * pBitmap->GetDrawScale();
    double x = shadow.mXOffset * scale;
    double y = shadow.mYOffset * scale;
    val layerCanvas = *pBitmap->GetBitmap();
    val layerContext = layerCanvas.call<val>("getContext", std::string("2d"));
    layerContext.call<void>("setTransform");
    
    if (!shadow.mDrawForeground)
    {
      layerContext.call<void>("clearRect", 0, 0, width, height);
    }
    
    CanvasBitmap localBitmap(width, height, pBitmap->GetScale(), pBitmap->GetDrawScale());
    val localCanvas = *localBitmap.GetBitmap();
    val localContext = localCanvas.call<val>("getContext", std::string("2d"));
    val imageData = localContext.call<val>("createImageData", width, height);
    val pixelData = imageData["data"];
    unsigned char* in = mask.Get();
    
    for (auto i = 0; i < size; i++)
      pixelData.set(i, in[i]);
    
    localContext.call<void>("putImageData", imageData, 0, 0);
    IBlend blend(kBlendSourceIn, shadow.mOpacity);
    localContext.call<void>("rect", 0, 0, width, height);
    localContext.call<void>("scale", scale, scale);
    localContext.call<void>("translate", -(layer->Bounds().L + shadow.mXOffset), -(layer->Bounds().T + shadow.mYOffset));
    SetCanvasSourcePattern(localContext, shadow.mPattern, &blend);
    localContext.call<void>("fill");
    
    layerContext.set("globalCompositeOperation", "destination-over");
    layerContext.call<void>("drawImage", localCanvas, 0, 0, width, height, x, y, width, height);
  }
}
