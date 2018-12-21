/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "IGraphicsCanvas.h"
#include <string>
#include <stdio.h>
#include <emscripten.h>

using namespace emscripten;

extern IGraphics* gGraphics;

extern val GetPreloadedImages();
extern val GetCanvas();

static std::string CanvasColor(const IColor& color, float alpha = 1.0)
{
  WDL_String str;
  str.SetFormatted(64, "rgba(%d, %d, %d, %lf)", color.R, color.G, color.B, alpha * color.A / 255.0);
  return str.Get();
}

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
}

IGraphicsCanvas::~IGraphicsCanvas()
{
}

void IGraphicsCanvas::DrawBitmap(IBitmap& bitmap, const IRECT& bounds, int srcX, int srcY, const IBlend* pBlend)
{
  val context = GetContext();
  val img = *bitmap.GetAPIBitmap()->GetBitmap();
  GetContext().call<void>("save");
  SetCanvasBlendMode(pBlend);
  context.set("globalAlpha", BlendWeight(pBlend));
    
  const float bs = bitmap.GetScale();
  IRECT sr = bounds;
  sr.Scale(bs * bitmap.GetDrawScale());

  context.call<void>("drawImage", img, srcX * bs, srcY * bs, sr.W(), sr.H(), floor(bounds.L), floor(bounds.T), floor(bounds.W()), floor(bounds.H()));
  GetContext().call<void>("restore");
}

void IGraphicsCanvas::DrawRotatedBitmap(IBitmap& bitmap, float destCentreX, float destCentreY, double angle, int yOffsetZeroDeg, const IBlend* pBlend)
{
  IGraphicsPathBase::DrawRotatedBitmap(bitmap, destCentreX, destCentreY, DegToRad(angle), yOffsetZeroDeg, pBlend);
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
  
  SetCanvasSourcePattern(pattern, pBlend);

  context.call<void>("stroke");
  
  if (!options.mPreserve)
    PathClear();
}

void IGraphicsCanvas::PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend)
{
  val context = GetContext();
  std::string fillRule(options.mFillRule == kFillWinding ? "nonzero" : "evenodd");
  
  SetCanvasSourcePattern(pattern, pBlend);

  context.call<void>("fill", fillRule);

  if (!options.mPreserve)
    PathClear();
}

void IGraphicsCanvas::SetCanvasSourcePattern(const IPattern& pattern, const IBlend* pBlend)
{
  val context = GetContext();
  
  SetCanvasBlendMode(pBlend);
  
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
        context.call<val>("createRadialGradient", m.mTX, m.mTY, 0.0, m.mTY, m.mTY, ml.mXX);
        
      /*
      switch (pattern.mExtend)
      {
        case kExtendNone:      cairo_pattern_set_extend(cairoPattern, CAIRO_EXTEND_NONE);      break;
        case kExtendPad:       cairo_pattern_set_extend(cairoPattern, CAIRO_EXTEND_PAD);       break;
        case kExtendReflect:   cairo_pattern_set_extend(cairoPattern, CAIRO_EXTEND_REFLECT);   break;
        case kExtendRepeat:    cairo_pattern_set_extend(cairoPattern, CAIRO_EXTEND_REPEAT);    break;
      }*/
      
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

void IGraphicsCanvas::SetCanvasBlendMode(const IBlend* pBlend)
{
  if (!pBlend)
    GetContext().set("globalCompositeOperation", "source-over");
  
  switch (pBlend->mMethod)
  {
    case kBlendClobber:     GetContext().set("globalCompositeOperation", "source-over");   break;
    case kBlendAdd:         GetContext().set("globalCompositeOperation", "lighter");       break;
    case kBlendColorDodge:  GetContext().set("globalCompositeOperation", "source-over");   break;
    case kBlendNone:
    default:
      GetContext().set("globalCompositeOperation", "source-over");
  }
}

bool IGraphicsCanvas::DoDrawMeasureText(const IText& text, const char* str, IRECT& bounds, const IBlend* pBlend, bool measure)
{
  // TODO: orientation
  val context = GetContext();
  std::string textString(str);
  
  char fontString[FONT_LEN + 64];
  const char* styles[] = { "normal", "bold", "italic" };
  context.set("textBaseline", std::string("top"));
  val font = context["font"];
  sprintf(fontString, "%s %dpx %s", styles[text.mStyle], text.mSize, text.mFont);
  context.set("font", std::string(fontString));
  double textWidth = context.call<val>("measureText", textString)["width"].as<double>();
  double textHeight = EM_ASM_DOUBLE({
    return parseFloat(document.getElementById("canvas").getContext("2d").font);
  });
  
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
      case IText::EVAlign::kVAlignTop: y = bounds.T; break;
      case IText::EVAlign::kVAlignMiddle: y = bounds.MH() - (textHeight/2.); break;
      case IText::EVAlign::kVAlignBottom: y = bounds.B - textHeight; break;
      default: break;
    }

    context.call<void>("save");
    PathRect(bounds);
    context.call<void>("clip");
    PathClear();
    SetCanvasSourcePattern(text.mFGColor, pBlend);
    context.call<void>("fillText", textString, x, y);
    context.call<void>("restore");
  }
  
  return true;
}

void IGraphicsCanvas::PathTransformSetMatrix(const IMatrix& m)
{
  const double scale = GetDrawScale() * GetScreenScale();
  IMatrix t = IMatrix().Scale(scale, scale).Translate(XTranslate(), YTranslate()).Transform(m);

  GetContext().call<void>("setTransform", t.mXX, t.mYX, t.mYX, t.mYY, t.mTX, t.mTY);
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

APIBitmap* IGraphicsCanvas::LoadAPIBitmap(const WDL_String& resourcePath, int scale)
{
  return new CanvasBitmap(GetPreloadedImages()[resourcePath.Get()], resourcePath.Get() + 1, scale);
}

APIBitmap* IGraphicsCanvas::ScaleAPIBitmap(const APIBitmap* pBitmap, int scale)
{
//  int destW = (pBitmap->GetWidth() / pBitmap->GetScale()) * scale;
//  int destH = (pBitmap->GetHeight() / pBitmap->GetScale()) * scale;
//
//  val imgSrc = *pBitmap->GetBitmap();
//
//  // Make an offscreen canvas and resize
//  val documentHead = val::global("document")["head"];
//  val canvas = GetCanvas();
//  documentHead.call<val>("appendChild", canvas);
//  val canvasNode = documentHead["lastChild"];
//  val context = canvas.call<val>("getContext", std::string("2d"));
//  context.set("width", destW);
//  context.set("height", destH);
//
//  // Scale and draw
//  context.call<void>("scale", scale / pBitmap->GetScale(), scale / pBitmap->GetScale());
//  context.call<void>("drawImage", imgSrc, 0, 0);
//
//  // Copy to an image
//  val img = val::global("Image").new_();
//  img.set("src", canvas.call<val>("toDataURL"));
//
//  // Delete the canvas
//  documentHead.call<val>("removeChild", canvasNode);

  return new CanvasBitmap(GetPreloadedImages()[""], "", scale);
}

APIBitmap* IGraphicsCanvas::CreateAPIBitmap(int width, int height)
{
  const double scale = GetDrawScale() * GetScreenScale();
  return new CanvasBitmap(width * scale, height * scale, GetScreenScale(), GetDrawScale());
}
