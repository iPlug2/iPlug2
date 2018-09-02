#include "IGraphicsCanvas.h"
#include <string>
#include <stdio.h>
#include <emscripten.h>

using namespace emscripten;

extern IGraphics* gGraphics;

extern val GetPreloadedImages();

WebBitmap::WebBitmap(emscripten::val imageCanvas, const char* name, int scale)
{
  SetBitmap(new RetainVal(imageCanvas), imageCanvas["width"].as<int>(), imageCanvas["height"].as<int>(), scale);
}

IGraphicsCanvas::IGraphicsCanvas(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGraphicsPathBase(dlg, w, h, fps, scale)
{
  //emscripten_set_canvas_size ?
  val canvas = GetCanvas();

  int displayScale = val::global("window")["devicePixelRatio"].as<int>();
  canvas["style"].set("width", val(w));
  canvas["style"].set("height", val(h));
  GetContext().call<void>("scale", displayScale, displayScale);
  
  canvas.set("width", w * displayScale);
  canvas.set("height", h * displayScale);
  
  SetDisplayScale(displayScale);
  PathTransformScale(displayScale, displayScale);
}

IGraphicsCanvas::~IGraphicsCanvas()
{
}

void IGraphicsCanvas::DrawBitmap(IBitmap& bitmap, const IRECT& bounds, int srcX, int srcY, const IBlend* pBlend)
{
  val context = GetContext();
  RetainVal* pRV = (RetainVal*) bitmap.GetAPIBitmap()->GetBitmap();
  PathStateSave();
  SetWebBlendMode(pBlend);
  context.set("globalAlpha", BlendWeight(pBlend));
  
  const float ds = GetDisplayScale();
  IRECT sr = bounds;
  sr.Scale(ds);
  
  srcX *= ds;
  srcY *= ds;
  
  context.call<void>("drawImage", pRV->mItem, srcX, srcY, sr.W(), sr.H(), floor(bounds.L), floor(bounds.T), floor(bounds.W()), floor(bounds.H()));
  PathStateRestore();
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
    dashArray[i] = val(*(options.mDash.GetArray() + i));
  
  context.call<void>("setLineDash", dashArray);
  context.set("context.lineDashOffset", options.mDash.GetOffset());
  context.set("lineWidth", thickness);
  
  SetWebSourcePattern(pattern, pBlend);

  context.call<void>("stroke");
  
  if (!options.mPreserve)
    PathClear();
}

void IGraphicsCanvas::PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend)
{
  val context = GetContext();

  // FIX - fill rules?
  //options.mFillRule
  
  SetWebSourcePattern(pattern, pBlend);

  context.call<void>("fill");

  if (!options.mPreserve)
    PathClear();
}

void IGraphicsCanvas::SetWebSourcePattern(const IPattern& pattern, const IBlend* pBlend)
{
  auto InvertTransform = [](float *xform, const float *xformIn)
  {
    double d = 1.0 / (xformIn[0] * xformIn[3] - xformIn[1] * xformIn[2]);
    
    xform[0] = xformIn[3] * d;
    xform[1] = -xformIn[2] * d;
    xform[2] = -xformIn[1] * d;
    xform[3] =  xformIn[0] * d;
    xform[4] = (-xformIn[4] * xformIn[3] * d) - (xformIn[5] * (-xformIn[2] * d));
    xform[5] = (-xformIn[4] * (xformIn[1] * d)) - (xformIn[5] * (xformIn[0] * d));
  };
  
  val context = GetContext();
  
  SetWebBlendMode(pBlend);
  
  switch (pattern.mType)
  {
    case kSolidPattern:
    {
      const IColor color = pattern.GetStop(0).mColor;
      std::string colorString = ToCanvasColor(color, BlendWeight(pBlend));

      context.set("fillStyle", colorString);
      context.set("strokeStyle", colorString);
    }
    break;
      
    case kLinearPattern:
    case kRadialPattern:
    {
      float xform[6];
      InvertTransform(xform, pattern.mTransform);
      val gradient = (pattern.mType == kLinearPattern) ?
        context.call<val>("createLinearGradient", xform[4], xform[5], xform[0] + xform[4], xform[1] + xform[5]) :
        context.call<val>("createRadialGradient", xform[4], xform[5], 0.0, xform[4], xform[5], xform[0]);
      
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
        gradient.call<void>("addColorStop", stop.mOffset, ToCanvasColor(stop.mColor));
      }
      
      context.set("fillStyle", gradient);
      context.set("strokeStyle", gradient);
    }
    break;
  }
}

void IGraphicsCanvas::SetWebBlendMode(const IBlend* pBlend)
{
  if (!pBlend)
  {
    GetContext().set("globalCompositeOperation", "source-over");
  }
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

bool IGraphicsCanvas::DrawText(const IText& text, const char* str, IRECT& bounds, const IBlend* pBlend, bool measure)
{
  // TODO: orientation
  
  val context = GetContext();
  std::string textString(str);
  
  char fontString[FONT_LEN + 64];
  const char* styles[] = { "normal", "bold", "italic" };
  double textHeight = text.mSize;
  context.set("textBaseline", std::string("top"));
  val font = context["font"];
  sprintf(fontString, "%s %lfpx %s", styles[text.mStyle], textHeight, text.mFont);
  context.set("font", std::string(fontString));
  double textWidth = GetContext().call<val>("measureText", textString)["width"].as<double>();
  
  if (measure)
  {
    bounds = IRECT(0, 0, textWidth, textHeight);
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

    PathStateSave();
    PathRect(bounds);
    GetContext().call<void>("clip");
    PathClear();
    SetWebSourcePattern(text.mFGColor, pBlend);
    context.call<void>("fillText", textString, x, y);
    PathStateRestore();
  }
  
  return true;
}

bool IGraphicsCanvas::MeasureText(const IText& text, const char* str, IRECT& bounds)
{
  return DrawText(text, str, bounds, 0, true);
}

void IGraphicsCanvas::ClipRegion(const IRECT& r)
{
  PathStateSave();
  PathRect(r);
  GetContext().call<void>("clip");
  PathClear();
}

void IGraphicsCanvas::ResetClipRegion()
{
  PathStateRestore();
}

//void IGraphicsCanvas::Resize(int w, int h, float scale)
//{
//  IGraphics::Resize(w, h, scale);
//
//  val canvas = GetCanvas();
//
//  canvas.set("width", w * scale);
//  canvas.set("height", h * scale);
//
//  PathTransformScale(scale, scale);
//}

APIBitmap* IGraphicsCanvas::LoadAPIBitmap(const WDL_String& resourcePath, int scale)
{
  return new WebBitmap(GetPreloadedImages()[resourcePath.Get()], resourcePath.Get() + 1, scale);
}

APIBitmap* IGraphicsCanvas::ScaleAPIBitmap(const APIBitmap* pBitmap, int scale)
{
//  int destW = (pBitmap->GetWidth() / pBitmap->GetScale()) * scale;
//  int destH = (pBitmap->GetHeight() / pBitmap->GetScale()) * scale;
//
//  RetainVal* imgSrc = (RetainVal*)pBitmap->GetBitmap();
//
//  // Make an offscreen canvas and resize
//  val documentHead = val::global("document")["head"];
//  val canvas = GetCanvas();
//  documentHead.call<val>("appendChild", canvas);
//  val canvasNode = documentHead["lastChild"];
//  val context = canvas.call<emscripten::val>("getContext", std::string("2d"));
//  context.set("width", destW);
//  context.set("height", destH);
//
//  // Scale and draw
//  context.call<void>("scale", scale / pBitmap->GetScale(), scale / pBitmap->GetScale());
//  context.call<void>("drawImage", imgSrc->mItem, 0, 0);
//
//  // Copy to an image
//  val img = val::global("Image").new_();
//  img.set("src", canvas.call<val>("toDataURL"));
//
//  // Delete the canvas
//  documentHead.call<val>("removeChild", canvasNode);

  return new WebBitmap(GetPreloadedImages()[""], "", scale);
}
