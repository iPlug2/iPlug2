/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <cstring>
#include <cstdio>
#include <cstdint>

#include "IGraphicsWeb.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

void GetScreenDimensions(int& width, int& height)
{
  width = val::global("window")["innerWidth"].as<int>();
  height = val::global("window")["innerHeight"].as<int>();
}

END_IPLUG_NAMESPACE
END_IGRAPHICS_NAMESPACE

using namespace iplug;
using namespace igraphics;
using namespace emscripten;

extern IGraphicsWeb* gGraphics;
double gPrevMouseDownTime = 0.;
bool gFirstClick = false;

#pragma mark - Private Classes and Structs

// Fonts

class IGraphicsWeb::Font : public PlatformFont
{
public:
  Font(const char* fontName, const char* fontStyle)
  : PlatformFont(true), mDescriptor{fontName, fontStyle}
  {}
  
  FontDescriptor GetDescriptor() override { return &mDescriptor; }
  
private:
  std::pair<WDL_String, WDL_String> mDescriptor;
};

class IGraphicsWeb::FileFont : public Font
{
public:
  FileFont(const char* fontName, const char* fontStyle, const char* fontPath)
  : Font(fontName, fontStyle), mPath(fontPath)
  {
    mSystem = false;
  }
  
  IFontDataPtr GetFontData() override;
  
private:
  WDL_String mPath;
};

IFontDataPtr IGraphicsWeb::FileFont::GetFontData()
{
  IFontDataPtr fontData(new IFontData());
  FILE* fp = fopen(mPath.Get(), "rb");
  
  // Read in the font data.
  if (!fp)
    return fontData;
  
  fseek(fp,0,SEEK_END);
  fontData = std::make_unique<IFontData>((int) ftell(fp));
  
  if (!fontData->GetSize())
    return fontData;
  
  fseek(fp,0,SEEK_SET);
  size_t readSize = fread(fontData->Get(), 1, fontData->GetSize(), fp);
  fclose(fp);
  
  if (readSize && readSize == fontData->GetSize())
    fontData->SetFaceIdx(0);
  
  return fontData;
}

class IGraphicsWeb::MemoryFont : public Font
{
public:
  MemoryFont(const char* fontName, const char* fontStyle, const void* pData, int dataSize)
  : Font(fontName, fontStyle)
  {
    mSystem = false;
    mData.Set((const uint8_t*)pData, dataSize);
  }

  IFontDataPtr GetFontData() override
  {
    return IFontDataPtr(new IFontData(mData.Get(), mData.GetSize(), 0));
  }

private:
  WDL_TypedBuf<uint8_t> mData;
};

#pragma mark - Utilities and Callbacks

static EM_BOOL key_callback(int eventType, const EmscriptenKeyboardEvent* pEvent, void* pUserData)
{
  IGraphicsWeb* pGraphicsWeb = (IGraphicsWeb*) pUserData;

  int VK = DOMKeyToVirtualKey(pEvent->keyCode);
  WDL_String keyUTF8;

  // filter utf8 for non ascii keys
  if ((VK >= kVK_0 && VK <= kVK_Z) || VK == kVK_NONE)
    keyUTF8.Set(pEvent->key);
  else
    keyUTF8.Set("");

  IKeyPress keyPress {keyUTF8.Get(),
                      DOMKeyToVirtualKey(pEvent->keyCode),
                      static_cast<bool>(pEvent->shiftKey),
                      static_cast<bool>(pEvent->ctrlKey || pEvent->metaKey),
                      static_cast<bool>(pEvent->altKey)};
  
  switch (eventType)
  {
    case EMSCRIPTEN_EVENT_KEYDOWN:
    {
      return pGraphicsWeb->OnKeyDown(pGraphicsWeb->mPrevX, pGraphicsWeb->mPrevY, keyPress);
    }
    case EMSCRIPTEN_EVENT_KEYUP:
    {
      return pGraphicsWeb->OnKeyUp(pGraphicsWeb->mPrevX, pGraphicsWeb->mPrevY, keyPress);
    }
    default:
      break;
  }
  
  return 0;
}

static EM_BOOL outside_mouse_callback(int eventType, const EmscriptenMouseEvent* pEvent, void* pUserData)
{
  IGraphicsWeb* pGraphics = (IGraphicsWeb*) pUserData;

  IMouseInfo info;
  val rect = GetCanvas().call<val>("getBoundingClientRect");
  info.x = (pEvent->targetX - rect["left"].as<double>()) / pGraphics->GetDrawScale();
  info.y = (pEvent->targetY - rect["top"].as<double>()) / pGraphics->GetDrawScale();
  info.dX = pEvent->movementX;
  info.dY = pEvent->movementY;
  info.ms = {(pEvent->buttons & 1) != 0, (pEvent->buttons & 2) != 0, static_cast<bool>(pEvent->shiftKey), static_cast<bool>(pEvent->ctrlKey), static_cast<bool>(pEvent->altKey)};
  std::vector<IMouseInfo> list {info};
  
  switch (eventType)
  {
    case EMSCRIPTEN_EVENT_MOUSEUP:
    {
      // Get button states based on what caused the mouse up (nothing in buttons)
      list[0].ms.L = pEvent->button == 0;
      list[0].ms.R = pEvent->button == 2;
      pGraphics->OnMouseUp(list);
      emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, pGraphics, 1, nullptr);
      emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, pGraphics, 1, nullptr);
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEMOVE:
    {
      if(pEvent->buttons != 0 && !pGraphics->IsInPlatformTextEntry())
        pGraphics->OnMouseDrag(list);
      break;
    }
    default:
      break;
  }
  
  pGraphics->mPrevX = info.x;
  pGraphics->mPrevY = info.y;
    
  return true;
}

static EM_BOOL mouse_callback(int eventType, const EmscriptenMouseEvent* pEvent, void* pUserData)
{
  IGraphicsWeb* pGraphics = (IGraphicsWeb*) pUserData;
  
  IMouseInfo info;
  info.x = pEvent->targetX / pGraphics->GetDrawScale();
  info.y = pEvent->targetY / pGraphics->GetDrawScale();
  info.dX = pEvent->movementX;
  info.dY = pEvent->movementY;
  info.ms = {(pEvent->buttons & 1) != 0,
             (pEvent->buttons & 2) != 0,
             static_cast<bool>(pEvent->shiftKey),
             static_cast<bool>(pEvent->ctrlKey),
             static_cast<bool>(pEvent->altKey)};
  
  std::vector<IMouseInfo> list {info};
  switch (eventType)
  {
    case EMSCRIPTEN_EVENT_MOUSEDOWN:
    {
      const double timestamp = GetTimestamp();
      const double timeDiff = timestamp - gPrevMouseDownTime;
      
      if (gFirstClick && timeDiff < 0.3)
      {
        gFirstClick = false;
        pGraphics->OnMouseDblClick(info.x, info.y, info.ms);
      }
      else
      {
        gFirstClick = true;
        pGraphics->OnMouseDown(list);
      }
        
      gPrevMouseDownTime = timestamp;
      
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEUP:
    {
      // Get button states based on what caused the mouse up (nothing in buttons)
      list[0].ms.L = pEvent->button == 0;
      list[0].ms.R = pEvent->button == 2;
      pGraphics->OnMouseUp(list);
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEMOVE:
    {
      gFirstClick = false;
      
      if(pEvent->buttons == 0)
        pGraphics->OnMouseOver(info.x, info.y, info.ms);
      else
      {
        if(!pGraphics->IsInPlatformTextEntry())
          pGraphics->OnMouseDrag(list);
      }
      break;
    }
    case EMSCRIPTEN_EVENT_MOUSEENTER:
      pGraphics->OnSetCursor();
      pGraphics->OnMouseOver(info.x, info.y, info.ms);
      emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, pGraphics, 1, nullptr);
      break;
    case EMSCRIPTEN_EVENT_MOUSELEAVE:
      if(pEvent->buttons != 0)
      {
        emscripten_set_mousemove_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, pGraphics, 1, outside_mouse_callback);
        emscripten_set_mouseup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, pGraphics, 1, outside_mouse_callback);
      }
      pGraphics->OnMouseOut(); break;
    default:
      break;
  }
  
  pGraphics->mPrevX = info.x;
  pGraphics->mPrevY = info.y;

  return true;
}

static EM_BOOL wheel_callback(int eventType, const EmscriptenWheelEvent* pEvent, void* pUserData)
{
  IGraphics* pGraphics = (IGraphics*) pUserData;
  
  IMouseMod modifiers(false, false, pEvent->mouse.shiftKey, pEvent->mouse.ctrlKey, pEvent->mouse.altKey);
  
  double x = pEvent->mouse.targetX;
  double y = pEvent->mouse.targetY;
  
  x /= pGraphics->GetDrawScale();
  y /= pGraphics->GetDrawScale();
  
  switch (eventType) {
    case EMSCRIPTEN_EVENT_WHEEL: pGraphics->OnMouseWheel(x, y, modifiers, pEvent->deltaY);
    default:
      break;
  }
  
  return true;
}

EM_BOOL touch_callback(int eventType, const EmscriptenTouchEvent* pEvent, void* pUserData)
{
  IGraphics* pGraphics = (IGraphics*) pUserData;
  const float drawScale = pGraphics->GetDrawScale();

  std::vector<IMouseInfo> points;

  static EmscriptenTouchPoint previousTouches[32];
  
  for (auto i = 0; i < pEvent->numTouches; i++)
  {
    IMouseInfo info;
    info.x = pEvent->touches[i].targetX / drawScale;
    info.y = pEvent->touches[i].targetY / drawScale;
    info.dX = info.x - (previousTouches[i].targetX / drawScale);
    info.dY = info.y - (previousTouches[i].targetY / drawScale);
    info.ms = {true,
              false,
              static_cast<bool>(pEvent->shiftKey),
              static_cast<bool>(pEvent->ctrlKey),
              static_cast<bool>(pEvent->altKey),
              static_cast<ITouchID>(pEvent->touches[i].identifier)
    };
    
    if(pEvent->touches[i].isChanged)
      points.push_back(info);
  }

  memcpy(previousTouches, pEvent->touches, sizeof(previousTouches));
  
  switch (eventType)
  {
    case EMSCRIPTEN_EVENT_TOUCHSTART:
      pGraphics->OnMouseDown(points);
      return true;
    case EMSCRIPTEN_EVENT_TOUCHEND:
      pGraphics->OnMouseUp(points);
      return true;
    case EMSCRIPTEN_EVENT_TOUCHMOVE:
      pGraphics->OnMouseDrag(points);
      return true;
   case EMSCRIPTEN_EVENT_TOUCHCANCEL:
      pGraphics->OnTouchCancelled(points);
      return true;
    default:
      return false;
  }
}

static EM_BOOL complete_text_entry(int eventType, const EmscriptenFocusEvent* focusEvent, void* pUserData)
{
  IGraphicsWeb* pGraphics = (IGraphicsWeb*) pUserData;
  
  val input = val::global("document").call<val>("getElementById", std::string("textEntry"));
  std::string str = input["value"].as<std::string>();
  val::global("document")["body"].call<void>("removeChild", input);
  pGraphics->SetControlValueAfterTextEdit(str.c_str());
  
  return true;
}

static EM_BOOL text_entry_keydown(int eventType, const EmscriptenKeyboardEvent* pEvent, void* pUserData)
{
  IGraphicsWeb* pGraphicsWeb = (IGraphicsWeb*) pUserData;
  
  IKeyPress keyPress {pEvent->key, DOMKeyToVirtualKey(pEvent->keyCode),
    static_cast<bool>(pEvent->shiftKey),
    static_cast<bool>(pEvent->ctrlKey),
    static_cast<bool>(pEvent->altKey)};
  
  if (keyPress.VK == kVK_RETURN || keyPress.VK ==  kVK_TAB)
    return complete_text_entry(0, nullptr, pUserData);
  
  return false;
}

static EM_BOOL uievent_callback(int eventType, const EmscriptenUiEvent* pEvent, void* pUserData)
{
  IGraphicsWeb* pGraphics = (IGraphicsWeb*) pUserData;

  if (eventType == EMSCRIPTEN_EVENT_RESIZE)
  {
    pGraphics->GetDelegate()->OnParentWindowResize(pEvent->windowInnerWidth, pEvent->windowInnerHeight);

    return true;
  }
  
  return false;
}

IColorPickerHandlerFunc gColorPickerHandlerFunc = nullptr;

static void color_picker_callback(val e)
{
  if(gColorPickerHandlerFunc)
  {
    std::string colorStrHex = e["target"]["value"].as<std::string>();
    
    if (colorStrHex[0] == '#')
      colorStrHex = colorStrHex.erase(0, 1);
    
    IColor result;
    result.A = 255;
    sscanf(colorStrHex.c_str(), "%02x%02x%02x", &result.R, &result.G, &result.B);
    
    gColorPickerHandlerFunc(result);
  }
}

static void file_dialog_callback(val e)
{
  // DBGMSG(e["files"].as<std::string>().c_str());
}

EMSCRIPTEN_BINDINGS(events) {
  function("color_picker_callback", color_picker_callback);
  function("file_dialog_callback", file_dialog_callback);
}

#pragma mark -

IGraphicsWeb::IGraphicsWeb(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGRAPHICS_DRAW_CLASS(dlg, w, h, fps, scale)
{
  val keys = val::global("Object").call<val>("keys", GetPreloadedImages());
  
  DBGMSG("Preloaded %i images\n", keys["length"].as<int>());
  
  emscripten_set_mousedown_callback("#canvas", this, 1, mouse_callback);
  emscripten_set_mouseup_callback("#canvas", this, 1, mouse_callback);
  emscripten_set_mousemove_callback("#canvas", this, 1, mouse_callback);
  emscripten_set_mouseenter_callback("#canvas", this, 1, mouse_callback);
  emscripten_set_mouseleave_callback("#canvas", this, 1, mouse_callback);
  emscripten_set_wheel_callback("#canvas", this, 1, wheel_callback);
  emscripten_set_keydown_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, 1, key_callback);
  emscripten_set_keyup_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, 1, key_callback);
  emscripten_set_touchstart_callback("#canvas", this, 1, touch_callback);
  emscripten_set_touchend_callback("#canvas", this, 1, touch_callback);
  emscripten_set_touchmove_callback("#canvas", this, 1, touch_callback);
  emscripten_set_touchcancel_callback("#canvas", this, 1, touch_callback);
  emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, this, 1, uievent_callback);
}

IGraphicsWeb::~IGraphicsWeb()
{
}

void* IGraphicsWeb::OpenWindow(void* pHandle)
{
#ifdef IGRAPHICS_GL
  EmscriptenWebGLContextAttributes attr;
  emscripten_webgl_init_context_attributes(&attr);
  attr.stencil = true;
  attr.depth = true;
//  attr.explicitSwapControl = 1;
  
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context("#canvas", &attr);
  emscripten_webgl_make_context_current(ctx);
#endif
  
  OnViewInitialized(nullptr /* not used */);

  SetScreenScale(std::ceil(std::max(emscripten_get_device_pixel_ratio(), 1.)));

  GetDelegate()->LayoutUI(this);
  GetDelegate()->OnUIOpen();
  
  return nullptr;
}

void IGraphicsWeb::HideMouseCursor(bool hide, bool lock)
{
  if (mCursorHidden == hide)
    return;
    
  if (hide)
  {
#ifdef IGRAPHICS_WEB_POINTERLOCK
    if (lock)
      emscripten_request_pointerlock("#canvas", EM_FALSE);
    else
#endif
      val::global("document")["body"]["style"].set("cursor", "none");
    
    mCursorHidden = true;
    mCursorLock = lock;
  }
  else
  {
#ifdef IGRAPHICS_WEB_POINTERLOCK
    if (mCursorLock)
      emscripten_exit_pointerlock();
    else
#endif
    OnSetCursor();
      
    mCursorHidden = false;
    mCursorLock = false;
  }
}

ECursor IGraphicsWeb::SetMouseCursor(ECursor cursorType)
{
  std::string cursor("pointer");
  
  switch (cursorType)
  {
    case ECursor::ARROW:            cursor = "default";         break;
    case ECursor::IBEAM:            cursor = "text";            break;
    case ECursor::WAIT:             cursor = "wait";            break;
    case ECursor::CROSS:            cursor = "crosshair";       break;
    case ECursor::UPARROW:          cursor = "n-resize";        break;
    case ECursor::SIZENWSE:         cursor = "nwse-resize";     break;
    case ECursor::SIZENESW:         cursor = "nesw-resize";     break;
    case ECursor::SIZEWE:           cursor = "ew-resize";       break;
    case ECursor::SIZENS:           cursor = "ns-resize";       break;
    case ECursor::SIZEALL:          cursor = "move";            break;
    case ECursor::INO:              cursor = "not-allowed";     break;
    case ECursor::HAND:             cursor = "pointer";         break;
    case ECursor::APPSTARTING:      cursor = "progress";        break;
    case ECursor::HELP:             cursor = "help";            break;
  }
  
  val::global("document")["body"]["style"].set("cursor", cursor);
  return IGraphics::SetMouseCursor(cursorType);
}

void IGraphicsWeb::GetMouseLocation(float& x, float&y) const
{
  x = mPrevX;
  y = mPrevY;
}

//static
void IGraphicsWeb::OnMainLoopTimer()
{
  IRECTList rects;
  int screenScale = (int) std::ceil(std::max(emscripten_get_device_pixel_ratio(), 1.));
  
  // Don't draw if there are no graphics or if assets are still loading
  if (!gGraphics || !gGraphics->AssetsLoaded())
    return;
  
  if (screenScale != gGraphics->GetScreenScale())
  {
    gGraphics->SetScreenScale(screenScale);
  }

  if (gGraphics->IsDirty(rects))
  {
    gGraphics->SetAllControlsClean();
    gGraphics->Draw(rects);
  }
}

EMsgBoxResult IGraphicsWeb::ShowMessageBox(const char* str, const char* /*title*/, EMsgBoxType type, IMsgBoxCompletionHandlerFunc completionHandler)
{
  ReleaseMouseCapture();
  
  EMsgBoxResult result = kNoResult;
  
  switch (type)
  {
    case kMB_OK:
    {
      val::global("window").call<val>("alert", std::string(str));
      result = EMsgBoxResult::kOK;
      break;
    }
    case kMB_YESNO:
    case kMB_OKCANCEL:
    {
      result = static_cast<EMsgBoxResult>(val::global("window").call<val>("confirm", std::string(str)).as<int>());
    }
    // case MB_CANCEL:
    //   break;
    default:
      return result = kNoResult;
  }
  
  if(completionHandler)
    completionHandler(result);
  
  return result;
}

void IGraphicsWeb::PromptForFile(WDL_String& filename, WDL_String& path, EFileAction action, const char* ext, IFileDialogCompletionHandlerFunc completionHandler)
{
  //TODO
  // val inputEl = val::global("document").call<val>("createElement", std::string("input"));
  
  // inputEl.call<void>("setAttribute", std::string("type"), std::string("file"));
  // inputEl.call<void>("setAttribute", std::string("accept"), std::string(ext));
  // inputEl.call<void>("click");
  // inputEl.call<void>("addEventListener", std::string("input"), val::module_property("file_dialog_callback"), false);
  // inputEl.call<void>("addEventListener", std::string("onChange"), val::module_property("file_dialog_callback"), false);
}

void IGraphicsWeb::PromptForDirectory(WDL_String& path, IFileDialogCompletionHandlerFunc completionHandler)
{
  //TODO
  // val inputEl = val::global("document").call<val>("createElement", std::string("input"));

  // inputEl.call<void>("setAttribute", std::string("type"), std::string("file"));
  // inputEl.call<void>("setAttribute", std::string("directory"), true);
  // inputEl.call<void>("setAttribute", std::string("webkitdirectory"), true);
  // inputEl.call<void>("click");
  // inputEl.call<void>("addEventListener", std::string("input"), val::module_property("file_dialog_callback"), false);
  // inputEl.call<void>("addEventListener", std::string("onChange"), val::module_property("file_dialog_callback"), false);
}

bool IGraphicsWeb::PromptForColor(IColor& color, const char* str, IColorPickerHandlerFunc func)
{
  ReleaseMouseCapture();

  gColorPickerHandlerFunc = func;

  val inputEl = val::global("document").call<val>("createElement", std::string("input"));
  inputEl.call<void>("setAttribute", std::string("type"), std::string("color"));
  WDL_String colorStr;
  colorStr.SetFormatted(64, "#%02x%02x%02x", color.R, color.G, color.B);
  inputEl.call<void>("setAttribute", std::string("value"), std::string(colorStr.Get()));
  inputEl.call<void>("click");
  inputEl.call<void>("addEventListener", std::string("input"), val::module_property("color_picker_callback"), false);
  inputEl.call<void>("addEventListener", std::string("onChange"), val::module_property("color_picker_callback"), false);

  return false;
}

void IGraphicsWeb::CreatePlatformTextEntry(int paramIdx, const IText& text, const IRECT& bounds, int length, const char* str)
{
  val input = val::global("document").call<val>("createElement", std::string("input"));
  val rect = GetCanvas().call<val>("getBoundingClientRect");

  auto setDim = [&input](const char *dimName, double pixels)
  {
    WDL_String dimstr;
    dimstr.SetFormatted(32, "%fpx",  pixels);
    input["style"].set(dimName, std::string(dimstr.Get()));
  };
  
  auto setColor = [&input](const char *colorName, IColor color)
  {
    WDL_String str;
    str.SetFormatted(64, "rgba(%d, %d, %d, %d)", color.R, color.G, color.B, color.A);
    input["style"].set(colorName, std::string(str.Get()));
  };

  input.set("id", std::string("textEntry"));
  input["style"].set("position", val("fixed"));
  setDim("left", rect["left"].as<double>() + bounds.L);
  setDim("top", rect["top"].as<double>() + bounds.T);
  setDim("width", bounds.W());
  setDim("height", bounds.H());
  
  setColor("color", text.mTextEntryFGColor);
  setColor("background-color", text.mTextEntryBGColor);
  if (paramIdx > kNoParameter)
  {
    const IParam* pParam = GetDelegate()->GetParam(paramIdx);

    switch (pParam->Type())
    {
      case IParam::kTypeEnum:
      case IParam::kTypeInt:
      case IParam::kTypeBool:
        input.set("type", val("number")); // TODO
        break;
      case IParam::kTypeDouble:
        input.set("type", val("number"));
        break;
      default:
        break;
    }
  }
  else
  {
    input.set("type", val("text"));
  }

  val::global("document")["body"].call<void>("appendChild", input);
  input.call<void>("focus");
  emscripten_set_focusout_callback("textEntry", this, 1, complete_text_entry);
  emscripten_set_keydown_callback("textEntry", this, 1, text_entry_keydown);
}

IPopupMenu* IGraphicsWeb::CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT bounds, bool& isAsync)
{
  return nullptr;
}

bool IGraphicsWeb::OpenURL(const char* url, const char* msgWindowTitle, const char* confirmMsg, const char* errMsgOnFailure)
{
  val::global("window").call<val>("open", std::string(url), std::string("_blank"));
  
  return true;
}

void IGraphicsWeb::DrawResize()
{
  val canvas = GetCanvas();
  
  canvas["style"].set("width", val(Width() * GetDrawScale()));
  canvas["style"].set("height", val(Height() * GetDrawScale()));
  
  canvas.set("width", Width() * GetBackingPixelScale());
  canvas.set("height", Height() * GetBackingPixelScale());
  
  IGRAPHICS_DRAW_CLASS::DrawResize();
}

PlatformFontPtr IGraphicsWeb::LoadPlatformFont(const char* fontID, const char* fileNameOrResID)
{
  WDL_String fullPath;
  const EResourceLocation fontLocation = LocateResource(fileNameOrResID, "ttf", fullPath, GetBundleID(), nullptr, nullptr);
  
  if (fontLocation == kNotFound)
    return nullptr;

  return PlatformFontPtr(new FileFont(fontID, "", fullPath.Get()));
}

PlatformFontPtr IGraphicsWeb::LoadPlatformFont(const char* fontID, const char* fontName, ETextStyle style)
{
  const char* styles[] = { "normal", "bold", "italic" };
  
  return PlatformFontPtr(new Font(fontName, styles[static_cast<int>(style)]));
}

PlatformFontPtr IGraphicsWeb::LoadPlatformFont(const char* fontID, void* pData, int dataSize)
{
  return PlatformFontPtr(new MemoryFont(fontID, "", pData, dataSize));
}

#if defined IGRAPHICS_CANVAS
#include "IGraphicsCanvas.cpp"
#elif defined IGRAPHICS_NANOVG
#include "IGraphicsNanoVG.cpp"

#ifdef IGRAPHICS_FREETYPE
#define FONS_USE_FREETYPE
#endif

#include "nanovg.c"
#endif
