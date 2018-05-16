#include "IGraphicsWeb.h"
#include <string>
#include <stdio.h>
#include <emscripten.h>

using namespace emscripten;

extern IGraphics* gGraphics;

void MouseHandler(std::string object, val event, double outside)
{
  IGraphicsWeb* pGraphics = (IGraphicsWeb*) stoull(object, 0, 16);
  pGraphics->OnMouseEvent(event, outside);
}

void KeyHandler(std::string object, val event)
{
  IGraphicsWeb* pGraphics = (IGraphicsWeb*) stoull(object, 0, 16);
  pGraphics->OnKeyEvent(event);
}

EMSCRIPTEN_BINDINGS(IGraphics) {
  function("mouse_web_handler", &MouseHandler);
  function("key_web_handler", &KeyHandler);
}

std::string GetColor(const IColor& color, float alpha = 1.0)
{
  char cString[64];
  
  sprintf(cString, "rgba(%d, %d, %d, %lf)", color.R, color.G, color.B, alpha * color.A / 255.0);
  
  return cString;
}

WebBitmap::WebBitmap(emscripten::val imageCanvas, const char* name, int scale)
{
  SetBitmap(new RetainVal(imageCanvas), imageCanvas["width"].as<int>(), imageCanvas["height"].as<int>(), scale);
}

IGraphicsWeb::IGraphicsWeb(IDelegate& dlg, int w, int h, int fps)
: IGraphicsPathBase(dlg, w, h, fps)
{
  printf("HELLO IGraphicsWeb!\n");

  val imgs = val::global("Module")["preloadedImages"];
  mPreloadedImages = new RetainVal(imgs);
  
  val keys = val::global("Object").call<val>("keys", imgs);
  
  DBGMSG("Preloaded %i images\n", keys["length"].as<int>());
  // Seed random number generator randomly
  
//  val randomGenerator = val::global("Math");
//  std::srand(32768 * randomGenerator.call<double>("random"));
  
  // Bind event listener to the canvas for all mouse events
  char callback[256];

  sprintf(callback, "Module.mouse_web_handler('%x', e, 0);", this);

  emscripten::val canvas = GetCanvas();
  val eventListener = val::global("Function").new_(std::string("e"), std::string(callback));
  canvas.call<void>("addEventListener", std::string("dblclick"), eventListener);
  canvas.call<void>("addEventListener", std::string("mousedown"), eventListener);
  canvas.call<void>("addEventListener", std::string("mouseup"), eventListener);
  canvas.call<void>("addEventListener", std::string("mousemove"), eventListener);
  canvas.call<void>("addEventListener", std::string("mouseover"), eventListener);
  canvas.call<void>("addEventListener", std::string("mouseout"), eventListener);
  canvas.call<void>("addEventListener", std::string("mousewheel"), eventListener);

  sprintf(callback, "Module.mouse_web_handler('%x', e, 1);", this);

  mWindowListener = new RetainVal(val::global("Function").new_(std::string("e"), std::string(callback)));

  sprintf(callback, "Module.key_web_handler('%x', e);", this);

  val eventListener2 = val::global("Function").new_(std::string("e"), std::string(callback));
  val tabIndex = GetCanvas().call<val>("setAttribute", std::string("tabindex"), 1);
  canvas.call<void>("addEventListener", std::string("keydown"), eventListener2);

  int scale = val::global("window")["devicePixelRatio"].as<int>();
  canvas["style"].set("width",val(w));
  canvas["style"].set("height",val(h));
  GetContext().call<void>("scale", scale, scale);
  
  canvas.set("width", w * scale);
  canvas.set("height", h * scale);
  
  SetDisplayScale(scale);
  PathTransformScale(scale, scale);
}

IGraphicsWeb::~IGraphicsWeb()
{
  delete mWindowListener;
  delete mPreloadedImages;
}

void IGraphicsWeb::DrawBitmap(IBitmap& bitmap, const IRECT& bounds, int srcX, int srcY, const IBlend* pBlend)
{
  val context = GetContext();
  RetainVal* rv = (RetainVal*) bitmap.GetRawBitmap();
  PathStateSave();
  SetWebBlendMode(pBlend);
  context.set("globalAlpha", BlendWeight(pBlend));
  
  const float ds = GetDisplayScale();
  IRECT sr = bounds;
  sr.Scale(ds);
  
  srcX *= ds;
  srcY *= ds;
  
  context.call<void>("drawImage", rv->mItem, srcX, srcY, sr.W(), sr.H(), bounds.L, bounds.T, bounds.W(), bounds.H());
  PathStateRestore();
}

void IGraphicsWeb::PathStroke(const IPattern& pattern, float thickness, const IStrokeOptions& options, const IBlend* pBlend)
{
  val context = GetContext();
  double dashArray[8];
  
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
    PathClear();
}

void IGraphicsWeb::PathFill(const IPattern& pattern, const IFillOptions& options, const IBlend* pBlend)
{
  val context = GetContext();

  // FIX - fill rules?
  //options.mFillRule
  
  SetWebSourcePattern(pattern, pBlend);

  context.call<void>("fill");

  if (!options.mPreserve)
    PathClear();
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

bool IGraphicsWeb::DrawText(const IText& text, const char* str, IRECT& bounds, bool measure)
{
  // TODO: orientation
  
  val context = GetContext();
  std::string textString(str);
  
  char fontString[FONT_LEN + 64];
  char* styles[] = { "normal", "bold", "italic" };
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

    PathStateSave();
    PathRect(bounds);
    GetContext().call<void>("clip");
    PathClear();
    SetWebSourcePattern(text.mFGColor);
    context.call<void>("fillText", textString, x, y);
    PathStateRestore();
  }
  
  return true;
}

bool IGraphicsWeb::MeasureText(const IText& text, const char* str, IRECT& bounds)
{
  return DrawText(text, str, bounds, true);
}

void IGraphicsWeb::ClipRegion(const IRECT& r)
{
  PathStateSave();
  PathRect(r);
  GetContext().call<void>("clip");
  PathClear();
}

void IGraphicsWeb::ResetClipRegion()
{
  PathStateRestore();
}

void IGraphicsWeb::Resize(int w, int h, float scale)
{
  IGraphics::Resize(w, h, scale);
  
  val canvas = GetCanvas();

  canvas.set("width", w * scale);
  canvas.set("height", h * scale);
  
  PathTransformScale(scale, scale);
}

APIBitmap* IGraphicsWeb::LoadAPIBitmap(const WDL_String& resourcePath, int scale)
{
  return new WebBitmap(mPreloadedImages->mItem[resourcePath.Get()], resourcePath.Get() + 1, scale);
}

APIBitmap* IGraphicsWeb::ScaleAPIBitmap(const APIBitmap* pBitmap, int scale)
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

  return new WebBitmap(mPreloadedImages->mItem[""], "", scale);
}

bool IGraphicsWeb::OSFindResource(const char* name, const char* type, WDL_String& result)
{
  if (CStringHasContents(name))
  {
    WDL_String plugSlash;
    plugSlash.SetFormatted(strlen(name) + 1, "/%s", name);

//    val canvas = mPreloadedImages->mItem[plugSlash.Get()];
    
    if(true)//TODO: should check for existence!
    {
      result.Set(plugSlash.Get());
      return true;
    }
  }
  return false;
}

void IGraphicsWeb::OnMouseEvent(val event, bool outside)
{
  std::string type = event["type"].as<std::string>();
  double x, y = -1.0;
  
  if (outside)
  {
    x = event["pageX"].as<double>() - mPositionL;
    y = event["pageY"].as<double>() - mPositionT;
  }
  else
  {
    x = event["offsetX"].as<double>();
    y = event["offsetY"].as<double>();
    
    mPositionL = event["pageX"].as<double>() - x;
    mPositionT = event["pageY"].as<double>() - y;
  }
  
  if (mMouseState == kMouseStateDownOutside && (!outside || !type.compare("mouseup")))
  {
    mMouseState = kMouseStateDownInside;

    val::global("window").call<void>("removeEventListener", std::string("mousemove"), mWindowListener->mItem, true);
    val::global("window").call<void>("removeEventListener", std::string("mouseup"), mWindowListener->mItem, true);
  }
  
  GetCanvas().call<void>("focus");
  event.call<void>("stopImmediatePropagation");
  event.call<void>("preventDefault");

  IMouseMod modifiers(0, 0, event["shiftKey"].as<bool>(), event["ctrlKey"].as<bool>(), event["altKey"].as<bool>());

  x /= GetScale();
  y /= GetScale();
  
  if (!type.compare("mousedown"))
  {
    OnMouseDown(x, y, modifiers);
    mMouseState = kMouseStateDownInside;
  }
  else if (!type.compare("mouseup"))
  {
    OnMouseUp(x, y, modifiers);
    mMouseState = kMouseStateUp;
  }
  else if (!type.compare("mousemove"))
  {
    if (mLastX != x || mLastY != y)
    {
      if (mMouseState != kMouseStateUp)
        OnMouseDrag(x, y, x - mLastX, y - mLastY, modifiers);
      else
        OnMouseOver(x, y, modifiers);
    }
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
    
    if (mMouseState == kMouseStateDownInside)
    {
      mMouseState = kMouseStateDownOutside;
      
      val::global("window").call<void>("addEventListener", std::string("mousemove"), mWindowListener->mItem, true);
      val::global("window").call<void>("addEventListener", std::string("mouseup"), mWindowListener->mItem, true);
    }
  }
  else if (!type.compare("mousewheel"))
  {
    //OnMouseOut();
  }
  
  mLastX = x;
  mLastY = y;
}

void IGraphicsWeb::OnKeyEvent(val event)
{
  // TODO: correct key codes
  int key = event["keyCode"].as<int>();
  OnKeyDown(mLastX, mLastY, key);
}

//static
void IGraphicsWeb::OnMainLoopTimer()
{
  IRECT r;
  
  if (gGraphics->IsDirty(r))
    gGraphics->Draw(r);
}
