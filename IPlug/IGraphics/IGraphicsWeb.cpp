#include "IGraphicsWeb.h"
#include <string>
#include <stdio.h>

using namespace emscripten;

std::string getColor(const IColor& color, float alpha = 1.0)
{
  char cString[64];
  
  sprintf(cString, "rgba(%d, %d, %d, %lf)", color.R, color.G, color.B, alpha * color.A / 255.0);
  
  return cString;
}

IGraphicsWeb::IGraphicsWeb(IDelegate& dlg, int w, int h, int fps)
: IGraphicsPathBase(dlg, w, h, fps)
{
  val randomGenerator = val::global("Math");
  std::srand(32768 * randomGenerator.call<double>("random"));
  printf("HELLO IGraphics!\n");
}

IGraphicsWeb::~IGraphicsWeb()
{
}

void IGraphicsWeb::PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend)
{
  val context = getContext();
  double dashArray[8];
  
  // First set options

  switch (options.mCapOption)
  {
    case kCapButt:    context.set("lineCap", "butt");      break;
    case kCapRound:   context.set("lineCap", "round");     break;
    case kCapSquare:  context.set("lineCap", "square");    break;
  }
  
  switch (options.mJoinOption)
  {
    case kJoinMiter:  context.set("lineJoin", "miter");      break;
    case kJoinRound:  context.set("lineJoin", "round");      break;
    case kJoinBevel:  context.set("lineJoin", "bevel");      break;
  }
  
  context.set("miterLimit", options.mMiterLimit);
  /*
  for (int i = 0; i < options.mDash.GetCount(); i++)
    dashArray[i] = *(options.mDash.GetArray() + i);
  
  cairo_set_dash(mContext, dashArray, options.mDash.GetCount(), options.mDash.GetOffset());*/
  context.set("lineWidth", thickness);
  
  SetWebSourcePattern(pattern, pBlend);

  context.call<void>("stroke");
  
  if (!options.mPreserve)
    PathStart();
}

void IGraphicsWeb::PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend)
{
  val context = getContext();

  // FIX - fill rules?
  //options.mFillRule
  
  SetWebSourcePattern(pattern, pBlend);

  context.call<void>("fill");

  if (!options.mPreserve)
    PathStart();
}

void invertTransform(float *xform, const float *xformIn)
{
    double d = 1.0 / (xformIn[0] * xformIn[3] - xformIn[1] * xformIn[2]);
  
    xform[0] = xformIn[3] * d;
    xform[1] = -xformIn[2] * d;
    xform[2] = -xformIn[1] * d;
    xform[3] =  xformIn[0] * d;
    xform[4] = (-xformIn[4] * xformIn[3] * d) - (xformIn[5] * (-xformIn[2] * d));
    xform[5] = (-xformIn[4] * (xformIn[1] * d)) - (xformIn[5] * (xformIn[0] * d));
}

void IGraphicsWeb::SetWebSourcePattern(const IPattern& pattern, const IBlend* pBlend)
{
  //cairo_set_operator(mContext, CairoBlendMode(pBlend));
  val context = getContext();
  
  switch (pattern.mType)
  {
    case kSolidPattern:
    {
      const IColor color = pattern.GetStop(0).mColor;
      std::string colorString = getColor(color, BlendWeight(pBlend));

      context.set("fillStyle", colorString);
      context.set("strokeStyle", colorString);
    }
    break;
      
    case kLinearPattern:
    case kRadialPattern:
    {
      float xform[6];
      invertTransform(xform, pattern.mTransform);
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
        gradient.call<void>("addColorStop", stop.mOffset, getColor(stop.mColor));
      }
      
      context.set("fillStyle", gradient);
      context.set("strokeStyle", gradient);
    }
    break;
  }
}

void IGraphicsWeb::Resize(int w, int h, float scale)
{
  emscripten::val canvas = getCanvas();

  canvas.set("width", w * scale);
  canvas.set("height", h * scale);
  
  PathTransformScale(scale, scale);
}

