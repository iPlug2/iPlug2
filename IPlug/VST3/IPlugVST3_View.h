/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once
#include "pluginterfaces/gui/iplugviewcontentscalesupport.h"

/** IPlug VST3 View  */
template <class T>
class IPlugVST3View : public Steinberg::Vst::EditorView
                    , public Steinberg::IPlugViewContentScaleSupport
{
public:
  IPlugVST3View(T& owner)
  : EditorView(&owner, nullptr)
  , mOwner(owner)
  {
  }
  
  ~IPlugVST3View()
  {
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

  void resize(int w, int h)
  {
    TRACE
    
    Steinberg::ViewRect newSize = Steinberg::ViewRect(0, 0, w, h);

    /* For some unknown reason plugFrame is sometime null */
    if(plugFrame)
      plugFrame->resizeView(this, &newSize);
  }
  
  OBJ_METHODS(IPlugVST3View, Steinberg::Vst::EditorView)
  DEFINE_INTERFACES
  DEF_INTERFACE(IPlugViewContentScaleSupport)
  END_DEFINE_INTERFACES(Steinberg::Vst::EditorView)
  REFCOUNT_METHODS(Steinberg::Vst::EditorView)

private:
  T& mOwner;
};
