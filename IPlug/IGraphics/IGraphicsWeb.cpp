#include "IGraphicsWeb.h"
#include <string>
#include <stdio.h>

using namespace emscripten;

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

WebBitmap::WebBitmap(val image, int scale)
{
  std::string url = image["src"].as<std::string>();
  int width, height;
  
  if(url.find("knob.png") != std::string::npos) {   width = 48; height = 2880; }
  if(url.find("Background_Main.png") != std::string::npos) {   width = 980; height = 580; }
  if(url.find("AboutBox.png") != std::string::npos) {   width = 980; height = 581; }
  if(url.find("Detune_Oct.png") != std::string::npos) {   width = 41; height = 112; }
  if(url.find("Detune_Polarity.png") != std::string::npos) {   width = 41; height = 36; }
  if(url.find("Detune_Semi.png") != std::string::npos) {   width = 41; height = 336; }
  if(url.find("Env_ADSRPanel.png") != std::string::npos) {   width = 872; height = 364; }
  if(url.find("Env_EndPoint.png") != std::string::npos) {   width = 45; height = 140; }
  if(url.find("Env_Loop.png") != std::string::npos) {   width = 19; height = 30; }
  if(url.find("Env_Slider.png") != std::string::npos) {   width = 25; height = 42; }
  if(url.find("Env_SustainPoint.png") != std::string::npos) {   width = 43; height = 160; }
  if(url.find("Font_LCD.png") != std::string::npos) {   width = 1045; height = 16; }
  if(url.find("Font_Presets.png") != std::string::npos) {   width = 855; height = 15; }
  if(url.find("Master_EnvMode.png") != std::string::npos) {   width = 21; height = 338; }
  if(url.find("Master_MixMode.png") != std::string::npos) {   width = 44; height = 180; }
  if(url.find("Master_ModMode.png") != std::string::npos) {   width = 44; height = 135; }
  if(url.find("Master_PitchbendRange.png") != std::string::npos) {   width = 41; height = 360; }
  if(url.find("Master_PolyMode.png") != std::string::npos) {   width = 87; height = 162; }
  if(url.find("MIDIActivityLed.png") != std::string::npos) {   width = 16; height = 26; }
  if(url.find("OctaveShift.png") != std::string::npos) {   width = 41; height = 90; }
  if(url.find("Osc_Shape.png") != std::string::npos) {   width = 41; height = 240; }
  if(url.find("Osc_Shape_Dual.png") != std::string::npos) {   width = 34; height = 24; }
  if(url.find("Pan_Lock.png") != std::string::npos) {   width = 20; height = 38; }
  if(url.find("Preset_File.png") != std::string::npos) {   width = 47; height = 72; }
  if(url.find("Preset_Next.png") != std::string::npos) {   width = 28; height = 72; }
  if(url.find("Preset_Prev.png") != std::string::npos) {   width = 29; height = 72; }
  if(url.find("Preset_Store.png") != std::string::npos) {   width = 28; height = 72; }
  if(url.find("Preset_Tools.png") != std::string::npos) {   width = 58; height = 72; }
  if(url.find("Scaling_AftertouchMode.png") != std::string::npos) {   width = 99; height = 98; }
  if(url.find("Scaling_Curve.png") != std::string::npos) {   width = 42; height = 62; }
  if(url.find("Scaling_Down.png") != std::string::npos) {   width = 34; height = 30; }
  if(url.find("Scaling_Edit.png") != std::string::npos) {   width = 47; height = 96; }
  if(url.find("Scaling_Left.png") != std::string::npos) {   width = 18; height = 62; }
  if(url.find("Scaling_Lin.png") != std::string::npos) {   width = 36; height = 62; }
  if(url.find("Scaling_LoadTun.png") != std::string::npos) {   width = 91; height = 56; }
  if(url.find("Scaling_Lock.png") != std::string::npos) {   width = 28; height = 54; }
  if(url.find("Scaling_Max.png") != std::string::npos) {   width = 37; height = 62; }
  if(url.find("Scaling_Panel.png") != std::string::npos) {   width = 980; height = 370; }
  if(url.find("Scaling_ResetTun.png") != std::string::npos) {   width = 91; height = 56; }
  if(url.find("Scaling_Right.png") != std::string::npos) {   width = 18; height = 62; }
  if(url.find("Scaling_Up.png") != std::string::npos) {   width = 34; height = 30; }
  if(url.find("SmallKnob.png") != std::string::npos) {   width = 32; height = 2145; }
  if(url.find("SmallKnobRim.png") != std::string::npos) {   width = 32; height = 2145; }
  if(url.find("Vibrato_RateMode.png") != std::string::npos) {   width = 41; height = 60; }
  if(url.find("Vibrato_Shape.png") != std::string::npos) {   width = 41; height = 210; }
  if(url.find("Vibrato_SyncMode.png") != std::string::npos) {   width = 41; height = 60; }
  if(url.find("LCDBG.png") != std::string::npos) {   width = 201; height = 110; }
  
  //  int width = image["naturalWidth"].as<int>();
  //  int height = image["naturalHeight"].as<int>();
  
  // TODO: this value won't be correct on load, because the bitmap isn't loaded yet...
  
  SetBitmap(new RetainVal(image), width, height, scale);
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
  
  sprintf(callback, "Module.mouse_web_handler('%x', e, 0);", this);

  val eventListener = val::global("Function").new_(std::string("e"), std::string(callback));
  GetCanvas().call<void>("addEventListener", std::string("dblclick"), eventListener);
  GetCanvas().call<void>("addEventListener", std::string("mousedown"), eventListener);
  GetCanvas().call<void>("addEventListener", std::string("mouseup"), eventListener);
  GetCanvas().call<void>("addEventListener", std::string("mousemove"), eventListener);
  GetCanvas().call<void>("addEventListener", std::string("mouseover"), eventListener);
  GetCanvas().call<void>("addEventListener", std::string("mouseout"), eventListener);
  GetCanvas().call<void>("addEventListener", std::string("mousewheel"), eventListener);
  
  sprintf(callback, "Module.mouse_web_handler('%x', e, 1);", this);
  
  mWindowListener = new RetainVal(val::global("Function").new_(std::string("e"), std::string(callback)));
  
  sprintf(callback, "Module.key_web_handler('%x', e);", this);
  
  val eventListener2 = val::global("Function").new_(std::string("e"), std::string(callback));
  val tabIndex = GetCanvas().call<val>("setAttribute", std::string("tabindex"), 1);
  GetCanvas().call<void>("addEventListener", std::string("keydown"), eventListener2);
}

IGraphicsWeb::~IGraphicsWeb()
{
  delete mWindowListener;
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
  
  emscripten::val canvas = GetCanvas();

  canvas.set("width", w * scale);
  canvas.set("height", h * scale);
  
  PathTransformScale(scale, scale);
}

APIBitmap* IGraphicsWeb::LoadAPIBitmap(const WDL_String& resourcePath, int scale)
{
  val img = val::global("Image").new_(100, 100);
  img.set("src", resourcePath.Get());
  
  // Use XMLHttpRequest to allow synchronous loading
  val request = val::global("XMLHttpRequest").new_();
  request.call<void>("open", std::string("GET"), std::string(resourcePath.Get()), false);
  request.call<void>("send");
  
  assert(request["status"].equals(val(200)));
  
  //printf("Object %s\n", request["response"].as<std::string>().c_str());
  // Now load the image from a generated URL
  //val blob = val::global("Blob").new_(request["response"]);
  //val url = val::global("window")["URL"].call<val>("createObjectURL", blob);
  //img.set("src", url);
  
  //assert(img["complete"].as<bool>());
  
  return new WebBitmap(img, scale);
}

APIBitmap* IGraphicsWeb::ScaleAPIBitmap(const APIBitmap* pBitmap, int scale)
{
  int destW = (pBitmap->GetWidth() / pBitmap->GetScale()) * scale;
  int destH = (pBitmap->GetHeight() / pBitmap->GetScale()) * scale;
  
  RetainVal* imgSrc = (RetainVal*)pBitmap->GetBitmap();
  
  // Make an offscreen canvas and resize
  val documentHead = val::global("document")["head"];
  val canvas = val::global("document").call<val>("createElement", std::string("canvas"));
  documentHead.call<val>("appendChild", canvas);
  val canvasNode = documentHead["lastChild"];
  val context = canvas.call<emscripten::val>("getContext", std::string("2d"));
  context.set("width", destW);
  context.set("height", destH);
  
  // Scale and draw
  context.call<void>("scale", scale / pBitmap->GetScale(), scale / pBitmap->GetScale());
  context.call<void>("drawImage", imgSrc->mItem, 0, 0);
  
  // Copy to an image
  val img = val::global("Image").new_();
  img.set("src", canvas.call<val>("toDataURL"));
  
  // Delete the canvas
  documentHead.call<val>("removeChild", canvasNode);

  return new WebBitmap(img, scale);
}

bool IGraphicsWeb::OSFindResource(const char* name, const char* type, WDL_String& result)
{
  if (CStringHasContents(name))
  {
    std::string url = name;
    
    // N.B. this is synchronous, which is badness in webland
    
    val request = val::global("XMLHttpRequest").new_();
    request.call<void>("open", std::string("HEAD"), url, false);
    request.call<void>("send");
    
    if (!request["status"].equals(val(200)))
      return false;

    result = WDL_String(url.c_str());
    
    return true;
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


