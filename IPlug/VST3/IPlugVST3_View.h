/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"
#include "keycodes.h"

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
    
    iplug::IKeyPress keyPress {&character, keyMsg, static_cast<bool>(modifiers & Steinberg::kShiftKey),
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
