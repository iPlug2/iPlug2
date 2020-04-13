/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "pluginterfaces/base/keycodes.h"

#include "IPlugStructs.h"

/** IPlug VST3 View  */
template <class T>
class IPlugVST3View : public Steinberg::CPluginView
                    , public Steinberg::IPlugViewContentScaleSupport
{
public:
  IPlugVST3View(T& owner)
  : mOwner(owner)
  {
    mOwner.addRef();
  }
  
  ~IPlugVST3View()
  {
    mOwner.release();
  }
  
  IPlugVST3View(const IPlugVST3View&) = delete;
  IPlugVST3View& operator=(const IPlugVST3View&) = delete;
  
  Steinberg::tresult PLUGIN_API isPlatformTypeSupported(Steinberg::FIDString type) override
  {
    if (mOwner.HasUI()) // for no editor plugins
    {
#ifdef OS_WIN
      if (strcmp(type, Steinberg::kPlatformTypeHWND) == 0)
        return Steinberg::kResultTrue;
      
#elif defined OS_MAC
      if (strcmp (type, Steinberg::kPlatformTypeNSView) == 0)
        return Steinberg::kResultTrue;
#endif
    }
    
    return Steinberg::kResultFalse;
  }
    
  Steinberg::tresult PLUGIN_API onSize(Steinberg::ViewRect* pSize) override
  {
    TRACE
    
    if (pSize)
      rect = *pSize;
    
    return Steinberg::kResultTrue;
  }
  
  Steinberg::tresult PLUGIN_API getSize(Steinberg::ViewRect* pSize) override
  {
    TRACE
    
    if (mOwner.HasUI())
    {
      *pSize = Steinberg::ViewRect(0, 0, mOwner.GetEditorWidth(), mOwner.GetEditorHeight());
      
      return Steinberg::kResultTrue;
    }
    else
    {
      return Steinberg::kResultFalse;
    }
  }
  
  Steinberg::tresult PLUGIN_API attached(void* pParent, Steinberg::FIDString type) override
  {
    if (mOwner.HasUI())
    {
      void* pView = nullptr;
#ifdef OS_WIN
      if (strcmp(type, Steinberg::kPlatformTypeHWND) == 0)
        pView = mOwner.OpenWindow(pParent);
#elif defined OS_MAC
      if (strcmp (type, Steinberg::kPlatformTypeNSView) == 0)
        pView = mOwner.OpenWindow(pParent);
      else // Carbon
        return Steinberg::kResultFalse;
#endif
      return Steinberg::kResultTrue;
    }
    
    return Steinberg::kResultFalse;
  }
    
  Steinberg::tresult PLUGIN_API removed() override
  {
    if (mOwner.HasUI())
      mOwner.CloseWindow();
    
    return CPluginView::removed();
  }

  Steinberg::tresult PLUGIN_API setContentScaleFactor(ScaleFactor factor) override
  {
    mOwner.SetScreenScale(factor);

    return Steinberg::kResultOk;
  }

  Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID _iid, void** obj) override
  {
    QUERY_INTERFACE(_iid, obj, IPlugViewContentScaleSupport::iid, IPlugViewContentScaleSupport)
    *obj = 0;
    return CPluginView::queryInterface(_iid, obj);
  }

  static int AsciiToVK(int ascii)
  {
  #ifdef OS_WIN
    HKL layout = GetKeyboardLayout(0);
    return VkKeyScanExA((CHAR)ascii, layout);
  #else
    // Numbers and uppercase alpha chars map directly to VK
    if ((ascii >= 0x30 && ascii <= 0x39) || (ascii >= 0x41 && ascii <= 0x5A))
    {
      return ascii;
    }

    // Lowercase alpha chars map to VK but need shifting
    if (ascii >= 0x61 && ascii <= 0x7A)
    {
      return ascii - 0x20;
    }

    return iplug::kVK_NONE;
  #endif
  }
  
  static int VSTKeyCodeToVK (Steinberg::int16 code, char ascii)
  {
    // If the keycode provided by the host is 0, we can still calculate the VK from the ascii value
    // NOTE: VKEY_EQUALS Doesn't seem to map to a Windows VK, so get the VK from the ascii char instead
    if (code == 0 || code == Steinberg::KEY_EQUALS)
    {
      return AsciiToVK(ascii);
    }
    
    using namespace Steinberg;
    using namespace iplug;

    switch (code)
    {
      case KEY_BACK: return kVK_BACK;
      case KEY_TAB: return kVK_TAB;
      case KEY_CLEAR: return kVK_CLEAR;
      case KEY_RETURN: return kVK_RETURN;
      case KEY_PAUSE: return kVK_PAUSE;
      case KEY_ESCAPE: return kVK_ESCAPE;
      case KEY_SPACE: return kVK_SPACE;
      case KEY_NEXT: return kVK_NEXT;
      case KEY_END: return kVK_END;
      case KEY_HOME: return kVK_HOME;
      case KEY_LEFT: return kVK_LEFT;
      case KEY_UP: return kVK_UP;
      case KEY_RIGHT: return kVK_RIGHT;
      case KEY_DOWN: return kVK_DOWN;
      case KEY_PAGEUP: return kVK_PRIOR;
      case KEY_PAGEDOWN: return kVK_NEXT;
      case KEY_SELECT: return kVK_SELECT;
      case KEY_PRINT: return kVK_PRINT;
      case KEY_ENTER: return kVK_RETURN;
      case KEY_SNAPSHOT: return kVK_SNAPSHOT;
      case KEY_INSERT: return kVK_INSERT;
      case KEY_DELETE: return kVK_DELETE;
      case KEY_HELP: return kVK_HELP;
      case KEY_NUMPAD0: return kVK_NUMPAD0;
      case KEY_NUMPAD1: return kVK_NUMPAD1;
      case KEY_NUMPAD2: return kVK_NUMPAD2;
      case KEY_NUMPAD3: return kVK_NUMPAD3;
      case KEY_NUMPAD4: return kVK_NUMPAD4;
      case KEY_NUMPAD5: return kVK_NUMPAD5;
      case KEY_NUMPAD6: return kVK_NUMPAD6;
      case KEY_NUMPAD7: return kVK_NUMPAD7;
      case KEY_NUMPAD8: return kVK_NUMPAD8;
      case KEY_NUMPAD9: return kVK_NUMPAD9;
      case KEY_MULTIPLY: return kVK_MULTIPLY;
      case KEY_ADD: return kVK_ADD;
      case KEY_SEPARATOR: return kVK_SEPARATOR;
      case KEY_SUBTRACT: return kVK_SUBTRACT;
      case KEY_DECIMAL: return kVK_DECIMAL;
      case KEY_DIVIDE: return kVK_DIVIDE;
      case KEY_F1: return kVK_F1;
      case KEY_F2: return kVK_F2;
      case KEY_F3: return kVK_F3;
      case KEY_F4: return kVK_F4;
      case KEY_F5: return kVK_F5;
      case KEY_F6: return kVK_F6;
      case KEY_F7: return kVK_F7;
      case KEY_F8: return kVK_F8;
      case KEY_F9: return kVK_F9;
      case KEY_F10: return kVK_F10;
      case KEY_F11: return kVK_F11;
      case KEY_F12: return kVK_F12;
      case KEY_NUMLOCK: return kVK_NUMLOCK;
      case KEY_SCROLL: return kVK_SCROLL;
      case KEY_SHIFT: return kVK_SHIFT;
      case KEY_CONTROL: return kVK_CONTROL;
      case KEY_ALT: return kVK_MENU;
      case KEY_EQUALS: return kVK_NONE;
    }

    if(code >= VKEY_FIRST_ASCII)
      return (code - VKEY_FIRST_ASCII + kVK_0);

    return kVK_NONE;
  };
  
  static iplug::IKeyPress translateKeyMessage (Steinberg::char16 key, Steinberg::int16 keyMsg, Steinberg::int16 modifiers)
  {
    char character = 0;
    
    if (key == 0)
    {
      key = Steinberg::VirtualKeyCodeToChar((Steinberg::uint8) keyMsg);
    }
    
    if (key)
    {
      Steinberg::String keyStr(STR (" "));
      keyStr.setChar16(0, key);
      keyStr.toMultiByte(Steinberg::kCP_Utf8);
      if (keyStr.length() == 1)
      {
        character = keyStr.getChar8 (0);
      }
    }

    iplug::IKeyPress keyPress {&character, VSTKeyCodeToVK(keyMsg, character),
      static_cast<bool>(modifiers & Steinberg::kShiftKey),
      static_cast<bool>(modifiers & Steinberg::kControlKey),
      static_cast<bool>(modifiers & Steinberg::kAlternateKey)};
    
    return keyPress;
  }
  
  Steinberg::tresult PLUGIN_API onKeyDown (Steinberg::char16 key, Steinberg::int16 keyMsg, Steinberg::int16 modifiers) override
  {
    return mOwner.OnKeyDown(translateKeyMessage(key, keyMsg, modifiers)) ? Steinberg::kResultTrue : Steinberg::kResultFalse;
  }
  
  Steinberg::tresult PLUGIN_API onKeyUp (Steinberg::char16 key, Steinberg::int16 keyMsg, Steinberg::int16 modifiers) override
  {
    return mOwner.OnKeyUp(translateKeyMessage(key, keyMsg, modifiers)) ? Steinberg::kResultTrue : Steinberg::kResultFalse;
  }
  
  DELEGATE_REFCOUNT(Steinberg::CPluginView)

  void resize(int w, int h)
  {
    TRACE
    
    Steinberg::ViewRect newSize = Steinberg::ViewRect(0, 0, w, h);
    plugFrame->resizeView(this, &newSize);
  }

  T& mOwner;
};
