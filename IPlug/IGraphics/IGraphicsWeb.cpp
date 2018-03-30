#include "IGraphicsWeb.h"
#include <string>
#include <stdio.h>

using namespace emscripten;

void MouseHandler(std::string object, std::string type, double x, double y, double state)
{
  int buttonStates = state;
  IGraphicsWeb* pGraphics = (IGraphicsWeb*) stoull(object, 0, 16);
  IMouseMod modifiers(0, 0, buttonStates & 4, buttonStates & 2, buttonStates & 1);
  
  pGraphics->OnMouseEvent(type, x, y, modifiers);
}

EMSCRIPTEN_BINDINGS(IGraphics) {
  function("mouse_web_handler", &MouseHandler);
}

std::string GetColor(const IColor& color, float alpha = 1.0)
{
  char cString[64];
  
  sprintf(cString, "rgba(%d, %d, %d, %lf)", color.R, color.G, color.B, alpha * color.A / 255.0);
  
  return cString;
}

struct RetainVal
{
  RetainVal(val item) : mItem(item) {}
  val mItem;
};

WebBitmap::WebBitmap(val image, int scale)
{
  int width = image["naturalWidth"].as<int>();
  int height = image["naturalHeight"].as<int>();
  
  // TODO: this value won't be correct on load, because the bitmap isn't loaded yet...
  
  SetBitmap(new RetainVal(image), 48, 48 * 60, scale);
}

WebBitmap::~WebBitmap()
{
  RetainVal *image = (RetainVal *)GetBitmap();
  delete image;
}

IGraphicsWeb::IGraphicsWeb(IDelegate& dlg, int w, int h, int fps)
: IGraphicsPathBase(dlg, w, h, fps)
{
  printf("HELLO IGraphics!\n");

  // Seed random number generator randomly
  
  val randomGenerator = val::global("Math");
  std::srand(32768 * randomGenerator.call<double>("random"));
  
  // Bind event listener to the canvas for all mouse events
  
  char callback[256];
  
  sprintf(callback, "Module.mouse_web_handler('%x', e.type, e.offsetX, e.offsetY, e.shiftKey | e.ctrlKey << 2 | e.altKey << 3)", this);
    
  val eventListener = val::global("Function").new_(std::string("e"), std::string(callback));
  GetCanvas().call<void>("addEventListener", std::string("dblclick"), eventListener);
  GetCanvas().call<void>("addEventListener", std::string("mousedown"), eventListener);
  GetCanvas().call<void>("addEventListener", std::string("mouseup"), eventListener);
  GetCanvas().call<void>("addEventListener", std::string("mousemove"), eventListener);
  GetCanvas().call<void>("addEventListener", std::string("mouseover"), eventListener);
  GetCanvas().call<void>("addEventListener", std::string("mouseout"), eventListener);
  GetCanvas().call<void>("addEventListener", std::string("mousewheel"), eventListener);
}

IGraphicsWeb::~IGraphicsWeb()
{
}

void IGraphicsWeb::DrawBitmap(IBitmap& bitmap, const IRECT& dest, int srcX, int srcY, const IBlend* pBlend)
{
  val context = GetContext();
  
  RetainVal* img = (RetainVal*)bitmap.GetRawBitmap();
  
  PathStateSave();
  SetWebBlendMode(pBlend);
  context.set("globalAlpha", BlendWeight(pBlend));
  context.call<void>("drawImage", img->mItem, srcX, srcY, dest.W(), dest.H(), dest.L, dest.T, dest.W(), dest.H());
  PathStateRestore();
}

void IGraphicsWeb::PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend)
{
  val context = GetContext();
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
  val context = GetContext();

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
  val context = GetContext();
  
  SetWebBlendMode(pBlend);
  
  switch (pattern.mType)
  {
    case kSolidPattern:
    {
      const IColor color = pattern.GetStop(0).mColor;
      std::string colorString = GetColor(color, BlendWeight(pBlend));

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
        gradient.call<void>("addColorStop", stop.mOffset, GetColor(stop.mColor));
      }
      
      context.set("fillStyle", gradient);
      context.set("strokeStyle", gradient);
    }
    break;
  }
}

void IGraphicsWeb::SetWebBlendMode(const IBlend* pBlend)
{
  val context = GetContext();

  if (!pBlend)
  {
    context.set("globalCompositeOperation", "source-over");
  }
  switch (pBlend->mMethod)
  {
    case kBlendClobber:     context.set("globalCompositeOperation", "source-over");   break;
    case kBlendAdd:         context.set("globalCompositeOperation", "lighter");       break;
    case kBlendColorDodge:  context.set("globalCompositeOperation", "source-over");   break;
    case kBlendNone:
    default:
      context.set("globalCompositeOperation", "source-over");
  }
}

void IGraphicsWeb::Resize(int w, int h, float scale)
{
  IGraphics::Resize(w, h, scale);
  
  emscripten::val canvas = GetCanvas();

  canvas.set("width", w * scale);
  canvas.set("height", h * scale);
  
  PathTransformScale(scale, scale);
}

APIBitmap* IGraphicsWeb::LoadAPIBitmap(const WDL_String& resourcePath, int scale)
{
  val img = val::global("Image").new_(100, 100);
  img.set("src", resourcePath.Get());
  
  // TODO: make sure the image has finished loading
  
  printf("loading %s\n", resourcePath.Get());
  //while(!img["complete"].as<bool>());

  //assert(img["complete"].as<bool>());  // Protect against typos in resource.h and .rc files.

  return new WebBitmap(img, scale);
}

bool IGraphicsWeb::OSFindResource(const char* name, const char* type, WDL_String& result)
{
  if (CStringHasContents(name))
  {
    std::string url = name;
    
    // TODO: safely check if the file exists...
    
    /*val request = val::global("HttpRequest").new_();
    
    request.call<void>("open", 'HEAD', url, false);
    request.call<void>("send");

    printf("url %s\n", url.c_str());

    if (!request["status"].equals(val(0)))
      return false;
    */
    result = WDL_String(url.c_str());
    
    return true;
  }
  return false;
}

void IGraphicsWeb::OnMouseEvent(std::string& type, double x, double y, const IMouseMod& modifiers)
{
  x /= GetScale();
  y /= GetScale();
  
  if (!type.compare("mousedown"))
  {
    OnMouseDown(x, y, modifiers);
    mMouseDown = true;
  }
  else if (!type.compare("mouseup"))
  {
    OnMouseUp(x, y, modifiers);
    mMouseDown = false;
  }
  else if (!type.compare("mousemove"))
  {
    if (mMouseDown)
      OnMouseDrag(x, y, x - mLastX, y - mLastY, modifiers);
    else
      OnMouseOver(x, y, modifiers);
  }
  else if (!type.compare("mouseover"))
  {
    OnMouseOver(x, y, modifiers);
  }
  else if (!type.compare("dblclick"))
  {
    OnMouseDblClick(x, y, modifiers);
  }
  else if (!type.compare("mouseout"))
  {
    OnMouseOut();
  }
  else if (!type.compare("mousewheel"))
  {
    //OnMouseOut();
  }
  
  mLastX = x;
  mLastY = y;
  
  // TODO: timer based drawing...

  Draw(GetBounds());
}


