/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <cstring>
#include <cstdio>

#include "IGraphicsWeb.h"
#include "IControl.h"
#include "IPopupMenuControl.h"

using namespace emscripten;

extern IGraphics* gGraphics;

// from <emscripten/key_codes.h>
#define VK_CANCEL              0x03
#define VK_HELP                0x06
#define VK_BACK                0x08
#define VK_TAB                 0x09
#define VK_CLEAR               0x0C
#define VK_RETURN              0x0D
#define VK_ENTER               0x0E
#define VK_SHIFT               0x10
#define VK_CONTROL             0x11
#define VK_ALT                 0x12
#define VK_PAUSE               0x13
#define VK_CAPS_LOCK           0x14
#define VK_KANA                0x15
#define VK_HANGUL              0x15
#define VK_EISU                0x16
#define VK_JUNJA               0x17
#define VK_FINAL               0x18
#define VK_HANJA               0x19
#define VK_KANJI               0x19
#define VK_ESCAPE              0x1B
#define VK_CONVERT             0x1C
#define VK_NONCONVERT          0x1D
#define VK_ACCEPT              0x1E
#define VK_MODECHANGE          0x1F
#define VK_SPACE               0x20
#define VK_PAGE_UP             0x21
#define VK_PAGE_DOWN           0x22
#define VK_END                 0x23
#define VK_HOME                0x24
#define VK_LEFT                0x25
#define VK_UP                  0x26
#define VK_RIGHT               0x27
#define VK_DOWN                0x28
#define VK_SELECT              0x29
#define VK_PRINT               0x2A
#define VK_EXECUTE             0x2B
#define VK_PRINTSCREEN         0x2C
#define VK_INSERT              0x2D
#define VK_DELETE              0x2E
#define VK_0                   0x30
#define VK_1                   0x31
#define VK_2                   0x32
#define VK_3                   0x33
#define VK_4                   0x34
#define VK_5                   0x35
#define VK_6                   0x36
#define VK_7                   0x37
#define VK_8                   0x38
#define VK_9                   0x39
#define VK_COLON               0x3A
#define VK_SEMICOLON           0x3B
#define VK_LESS_THAN           0x3C
#define VK_EQUALS              0x3D
#define VK_GREATER_THAN        0x3E
#define VK_QUESTION_MARK       0x3F
#define VK_AT                  0x40
#define VK_A                   0x41
#define VK_B                   0x42
#define VK_C                   0x43
#define VK_D                   0x44
#define VK_E                   0x45
#define VK_F                   0x46
#define VK_G                   0x47
#define VK_H                   0x48
#define VK_I                   0x49
#define VK_J                   0x4A
#define VK_K                   0x4B
#define VK_L                   0x4C
#define VK_M                   0x4D
#define VK_N                   0x4E
#define VK_O                   0x4F
#define VK_P                   0x50
#define VK_Q                   0x51
#define VK_R                   0x52
#define VK_S                   0x53
#define VK_T                   0x54
#define VK_U                   0x55
#define VK_V                   0x56
#define VK_W                   0x57
#define VK_X                   0x58
#define VK_Y                   0x59
#define VK_Z                   0x5A
#define VK_WIN                 0x5B
#define VK_CONTEXT_MENU        0x5D
#define VK_SLEEP               0x5F
#define VK_NUMPAD0             0x60
#define VK_NUMPAD1             0x61
#define VK_NUMPAD2             0x62
#define VK_NUMPAD3             0x63
#define VK_NUMPAD4             0x64
#define VK_NUMPAD5             0x65
#define VK_NUMPAD6             0x66
#define VK_NUMPAD7             0x67
#define VK_NUMPAD8             0x68
#define VK_NUMPAD9             0x69
#define VK_MULTIPLY            0x6A
#define VK_ADD                 0x6B
#define VK_SEPARATOR           0x6C
#define VK_SUBTRACT            0x6D
#define VK_DECIMAL             0x6E
#define VK_DIVIDE              0x6F
#define VK_F1                  0x70
#define VK_F2                  0x71
#define VK_F3                  0x72
#define VK_F4                  0x73
#define VK_F5                  0x74
#define VK_F6                  0x75
#define VK_F7                  0x76
#define VK_F8                  0x77
#define VK_F9                  0x78
#define VK_F10                 0x79
#define VK_F11                 0x7A
#define VK_F12                 0x7B
#define VK_F13                 0x7C
#define VK_F14                 0x7D
#define VK_F15                 0x7E
#define VK_F16                 0x7F
#define VK_F17                 0x80
#define VK_F18                 0x81
#define VK_F19                 0x82
#define VK_F20                 0x83
#define VK_F21                 0x84
#define VK_F22                 0x85
#define VK_F23                 0x86
#define VK_F24                 0x87
#define VK_NUM_LOCK            0x90
#define VK_SCROLL_LOCK         0x91
#define VK_WIN_OEM_FJ_JISHO    0x92
#define VK_WIN_OEM_FJ_MASSHOU  0x93
#define VK_WIN_OEM_FJ_TOUROKU  0x94
#define VK_WIN_OEM_FJ_LOYA     0x95
#define VK_WIN_OEM_FJ_ROYA     0x96
#define VK_CIRCUMFLEX          0xA0
#define VK_EXCLAMATION         0xA1
#define VK_DOUBLE_QUOTE        0xA3
#define VK_HASH                0xA3
#define VK_DOLLAR              0xA4
#define VK_PERCENT             0xA5
#define VK_AMPERSAND           0xA6
#define VK_UNDERSCORE          0xA7
#define VK_OPEN_PAREN          0xA8
#define VK_CLOSE_PAREN         0xA9
#define VK_ASTERISK            0xAA
#define VK_PLUS                0xAB
#define VK_PIPE                0xAC
#define VK_HYPHEN_MINUS        0xAD
#define VK_OPEN_CURLY_BRACKET  0xAE
#define VK_CLOSE_CURLY_BRACKET 0xAF
#define VK_TILDE               0xB0
#define VK_VOLUME_MUTE         0xB5
#define VK_VOLUME_DOWN         0xB6
#define VK_VOLUME_UP           0xB7
#define VK_COMMA               0xBC
#define VK_PERIOD              0xBE
#define VK_SLASH               0xBF
#define VK_BACK_QUOTE          0xC0
#define VK_OPEN_BRACKET        0xDB
#define VK_BACK_SLASH          0xDC
#define VK_CLOSE_BRACKET       0xDD
#define VK_QUOTE               0xDE
#define VK_META                0xE0
#define VK_ALTGR               0xE1
#define VK_WIN_ICO_HELP        0xE3
#define VK_WIN_ICO_00          0xE4
#define VK_WIN_ICO_CLEAR       0xE6
#define VK_WIN_OEM_RESET       0xE9
#define VK_WIN_OEM_JUMP        0xEA
#define VK_WIN_OEM_PA1         0xEB
#define VK_WIN_OEM_PA2         0xEC
#define VK_WIN_OEM_PA3         0xED
#define VK_WIN_OEM_WSCTRL      0xEE
#define VK_WIN_OEM_CUSEL       0xEF
#define VK_WIN_OEM_ATTN        0xF0
#define VK_WIN_OEM_FINISH      0xF1
#define VK_WIN_OEM_COPY        0xF2
#define VK_WIN_OEM_AUTO        0xF3
#define VK_WIN_OEM_ENLW        0xF4
#define VK_WIN_OEM_BACKTAB     0xF5
#define VK_ATTN                0xF6
#define VK_CRSEL               0xF7
#define VK_EXSEL               0xF8
#define VK_EREOF               0xF9
#define VK_PLAY                0xFA
#define VK_ZOOM                0xFB
#define VK_PA1                 0xFD
#define VK_WIN_OEM_CLEAR       0xFE

EM_BOOL key_callback(int eventType, const EmscriptenKeyboardEvent* pEvent, void* pUserData)
{
  IGraphicsWeb* pGraphicsWeb = (IGraphicsWeb*) pUserData;

  switch (eventType) {
    case EMSCRIPTEN_EVENT_KEYDOWN: pGraphicsWeb->OnKeyDown(pGraphicsWeb->mPrevX, pGraphicsWeb->mPrevY, atoi(pEvent->key)); break;
    default:
      break;
  }
  
  return 0;
}

EM_BOOL outside_mouse_callback(int eventType, const EmscriptenMouseEvent* pEvent, void* pUserData)
{
  IGraphicsWeb* pGraphics = (IGraphicsWeb*) pUserData;
  
  IMouseMod modifiers(0, 0, pEvent->shiftKey, pEvent->ctrlKey, pEvent->altKey);
  
  double x = pEvent->targetX;
  double y = pEvent->targetY;
  
  val rect = GetCanvas().call<val>("getBoundingClientRect");
  x -= rect["left"].as<double>();
  y -= rect["top"].as<double>();

  x /= pGraphics->GetDrawScale();
  y /= pGraphics->GetDrawScale();
  
  switch (eventType) {
    case EMSCRIPTEN_EVENT_MOUSEUP: pGraphics->OnMouseUp(x, y, modifiers);
      pGraphics->OnMouseUp(x, y, modifiers); break;
      emscripten_set_mousemove_callback("#window", pGraphics, 1, nullptr);
      emscripten_set_mouseup_callback("#window", pGraphics, 1, nullptr);
      break;
    case EMSCRIPTEN_EVENT_MOUSEMOVE:
      if(pEvent->buttons != 0)
        pGraphics->OnMouseDrag(x, y, pEvent->movementX, pEvent->movementY, modifiers);
      break;
    default:
      break;
  }
  
  pGraphics->mPrevX = x;
  pGraphics->mPrevY = y;
    
  return true;
}

EM_BOOL mouse_callback(int eventType, const EmscriptenMouseEvent* pEvent, void* pUserData)
{
  IGraphicsWeb* pGraphics = (IGraphicsWeb*) pUserData;
  
  IMouseMod modifiers(pEvent->buttons == 1, pEvent->buttons == 2, pEvent->shiftKey, pEvent->ctrlKey, pEvent->altKey);
  
  double x = pEvent->targetX;
  double y = pEvent->targetY;
  
  x /= pGraphics->GetDrawScale();
  y /= pGraphics->GetDrawScale();
  
  switch (eventType) {
    case EMSCRIPTEN_EVENT_CLICK: break;
    case EMSCRIPTEN_EVENT_MOUSEDOWN: pGraphics->OnMouseDown(x, y, modifiers); break;
    case EMSCRIPTEN_EVENT_MOUSEUP: pGraphics->OnMouseUp(x, y, modifiers); break;
    case EMSCRIPTEN_EVENT_DBLCLICK: pGraphics->OnMouseDblClick(x, y, modifiers);break;
    case EMSCRIPTEN_EVENT_MOUSEMOVE:
      if(pEvent->buttons == 0)
        pGraphics->OnMouseOver(x, y, modifiers);
      else
        pGraphics->OnMouseDrag(x, y, pEvent->movementX, pEvent->movementY, modifiers);
      break;
    case EMSCRIPTEN_EVENT_MOUSEENTER:
      pGraphics->OnMouseOver(x, y, modifiers);
      emscripten_set_mousemove_callback("#window", pGraphics, 1, nullptr);
      break;
    case EMSCRIPTEN_EVENT_MOUSELEAVE:
      if(pEvent->buttons != 0) {
        emscripten_set_mousemove_callback("#window", pGraphics, 1, outside_mouse_callback);
        emscripten_set_mouseup_callback("#window", pGraphics, 1, outside_mouse_callback);
      }

      pGraphics->OnMouseOut(); break;
    default:
      break;
  }
  
  pGraphics->mPrevX = x;
  pGraphics->mPrevY = y;

  return true;
}

EM_BOOL wheel_callback(int eventType, const EmscriptenWheelEvent* pEvent, void* pUserData)
{
  IGraphics* pGraphics = (IGraphics*) pUserData;
  
  IMouseMod modifiers(0, 0, pEvent->mouse.shiftKey, pEvent->mouse.ctrlKey, pEvent->mouse.altKey);
  
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

IGraphicsWeb::IGraphicsWeb(IGEditorDelegate& dlg, int w, int h, int fps, float scale)
: IGRAPHICS_DRAW_CLASS(dlg, w, h, fps, scale)
{
  val keys = val::global("Object").call<val>("keys", GetPreloadedImages());
  
  DBGMSG("Preloaded %i images\n", keys["length"].as<int>());
  
  emscripten_set_click_callback("canvas", this, 1, mouse_callback);
  emscripten_set_mousedown_callback("canvas", this, 1, mouse_callback);
  emscripten_set_mouseup_callback("canvas", this, 1, mouse_callback);
  emscripten_set_dblclick_callback("canvas", this, 1, mouse_callback);
  emscripten_set_mousemove_callback("canvas", this, 1, mouse_callback);
  emscripten_set_mouseenter_callback("canvas", this, 1, mouse_callback);
  emscripten_set_mouseleave_callback("canvas", this, 1, mouse_callback);
  emscripten_set_wheel_callback("canvas", this, 1, wheel_callback);
  emscripten_set_keydown_callback("#window", this, 1, key_callback);
}

IGraphicsWeb::~IGraphicsWeb()
{
}

void* IGraphicsWeb::OpenWindow(void* pHandle)
{
  OnViewInitialized(nullptr /* not used */);

  SetScreenScale(val::global("window")["devicePixelRatio"].as<int>());

  GetDelegate()->LayoutUI(this);
  
  return nullptr;
}

void IGraphicsWeb::HideMouseCursor(bool hide, bool lock)
{
  if (hide)
  {
    if (lock)
      emscripten_request_pointerlock("canvas", EM_FALSE);
    else
      val::global("document")["body"]["style"].set("cursor", std::string("none"));
    
    mCursorLock = lock;
  }
  else
  {
    if (mCursorLock)
      emscripten_exit_pointerlock();
    else
      val::global("document")["body"]["style"].set("cursor", std::string("auto"));
      
    mCursorLock = false;
  }
}

void IGraphicsWeb::SetMouseCursor(ECursor cursor)
{
  std::string cursorType("pointer");
  
  switch (cursor)
  {
    case ARROW:         cursorType = "default";       break;
    case IBEAM:         cursorType = "text";          break;
    case WAIT:          cursorType = "wait";          break;
    case CROSS:         cursorType = "crosshair";     break;
    case UPARROW:       cursorType = "n-resize";      break;
    case SIZENWSE:      cursorType = "nwse-resize";   break;
    case SIZENESW:      cursorType = "nesw-resize";   break;
    case SIZEWE:        cursorType = "ew-resize";     break;
    case SIZENS:        cursorType = "ns-resize";     break;
    case SIZEALL:       cursorType = "move";          break;
    case INO:           cursorType = "not-allowed";   break;
    case HAND:          cursorType = "grab";          break;
    case APPSTARTING:   cursorType = "progress";      break;
    case HELP:          cursorType = "help";          break;
  }
  
  val::global("document")["body"]["style"].set("cursor", cursorType);
}

bool IGraphicsWeb::OSFindResource(const char* name, const char* type, WDL_String& result)
{
  if (CStringHasContents(name))
  {
    WDL_String plusSlash;
    
    bool foundResource = false;
    
    if(strcmp(type, "png") == 0) {
      plusSlash.SetFormatted(strlen("/resources/img/") + strlen(name) + 1, "/resources/img/%s", name);
      foundResource = GetPreloadedImages().call<bool>("hasOwnProperty", std::string(plusSlash.Get()));
    }
    else if(strcmp(type, "ttf") == 0) {
      plusSlash.SetFormatted(strlen("/resources/fonts/") + strlen(name) + 1, "/resources/fonts/%s", name);
      foundResource = true; // TODO: check ttf
    }
    else if(strcmp(type, "svg") == 0) {
      plusSlash.SetFormatted(strlen("/resources/img/") + strlen(name) + 1, "/resources/img/%s", name);
      foundResource = true; // TODO: check svg
    }
    
    if(foundResource)
    {
      result.Set(plusSlash.Get());
      return true;
    }
  }
  return false;
}

//static
void IGraphicsWeb::OnMainLoopTimer()
{
  IRECTList rects;

  if (gGraphics->IsDirty(rects))
  {
    gGraphics->SetAllControlsClean();
    gGraphics->Draw(rects);
  }
}

bool IGraphicsWeb::GetTextFromClipboard(WDL_String& str)
{
  val clipboardText = val::global("window")["clipboardData"].call<val>("getData", std::string("Text"));
  
  str.Set(clipboardText.as<std::string>().c_str());

  return true; // TODO: return?
}

int IGraphicsWeb::ShowMessageBox(const char* str, const char* caption, EMessageBoxType type)
{
  switch (type)
  {
    case kMB_OK: val::global("window").call<val>("alert", std::string(str)); return 0;
    case kMB_YESNO:
    case kMB_OKCANCEL:
      return val::global("window").call<val>("confirm", std::string(str)).as<int>();
    // case MB_CANCEL:
    //   break;
    default: return 0;
  }
}

void IGraphicsWeb::PromptForFile(WDL_String& filename, WDL_String& path, EFileAction action, const char* ext)
{
  val inputEl = val::global("document").call<val>("getElementById", std::string("pluginInput"));
  
  inputEl.call<void>("setAttribute", std::string("accept"), std::string(ext));
  inputEl.call<void>("click");
}

void IGraphicsWeb::PromptForDirectory(WDL_String& path)
{
  val inputEl = val::global("document").call<val>("getElementById", std::string("pluginInput"));
  
  inputEl.call<void>("setAttribute", std::string("directory"));
  inputEl.call<void>("setAttribute", std::string("webkitdirectory"));
  inputEl.call<void>("click");
}

void IGraphicsWeb::CreatePlatformTextEntry(IControl& control, const IText& text, const IRECT& bounds, const char* str)
{
    ShowMessageBox("Warning", "Text entry not yet implemented", kMB_OK);
//  val input = val::global("document").call<val>("createElement", std::string("input"));
//  
//  val rect = GetCanvas().call<val>("getBoundingClientRect");
//  
//  WDL_String dimstr;
//  
//  input["style"].set("position", val("fixed"));
//  dimstr.SetFormatted(32, "%fpx",  rect["left"].as<double>() + bounds.L);
//  input["style"].set("left", std::string(dimstr.Get()));
//  dimstr.SetFormatted(32, "%fpx",  rect["top"].as<double>() + bounds.T);
//  input["style"].set("top", std::string(dimstr.Get()));
//  dimstr.SetFormatted(32, "%fpx",  bounds.W());
//  input["style"].set("width", std::string(dimstr.Get()));
//  dimstr.SetFormatted(32, "%fpx",  bounds.H());
//  input["style"].set("height", std::string(dimstr.Get()));
//  
//  if (control.ParamIdx() > kNoParameter)
//  {
//    const IParam* pParam = control.GetParam();
//    
//    switch ( pParam->Type() )
//    {
//      case IParam::kTypeEnum:
//      case IParam::kTypeInt:
//      case IParam::kTypeBool:
//        input.set("type", val("number"));
//        break;
//      case IParam::kTypeDouble:
//        input.set("type", val("number")); // TODO
//        break;
//      default:
//        break;
//    }
//  }
//  else
//  {
//    input.set("type", val("text"));
//  }
//
//  val::global("document")["body"].call<void>("appendChild", input);
//  
//  input.call<void>("focus");
}

IPopupMenu* IGraphicsWeb::CreatePlatformPopupMenu(IPopupMenu& menu, const IRECT& bounds, IControl* pCaller)
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

#if defined IGRAPHICS_CANVAS
#include "IGraphicsCanvas.cpp"
#elif defined IGRAPHICS_NANOVG
#include "IGraphicsNanoVG.cpp"

#ifdef IGRAPHICS_FREETYPE
#define FONS_USE_FREETYPE
#endif

#include "nanovg.c"
#endif
