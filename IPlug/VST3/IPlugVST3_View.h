/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once
using namespace Steinberg;
using namespace Vst;

/** IPlug VST3 View  */
template<class T>
class IPlugVST3View : public CPluginView
{
public:
  IPlugVST3View(T& owner)
  : mOwner(owner)
  {
    //mPlug.addDependentView(this);
    mOwner.addRef();
  }
  
  ~IPlugVST3View()
  {
    //mPlug.removeDependentView(this);
    mOwner.release();
  }
  
  tresult PLUGIN_API isPlatformTypeSupported(FIDString type) override
  {
    if(mOwner.HasUI()) // for no editor plugins
    {
#ifdef OS_WIN
      if (strcmp(type, kPlatformTypeHWND) == 0)
        return kResultTrue;
      
#elif defined OS_MAC
      if (strcmp (type, kPlatformTypeNSView) == 0)
        return kResultTrue;
#endif
    }
    
    return kResultFalse;
  }
    
  tresult PLUGIN_API onSize(ViewRect* pSize) override
  {
    TRACE;
    
    if (pSize)
      rect = *pSize;
    
    return kResultTrue;
  }
  
  tresult PLUGIN_API getSize(ViewRect* pSize) override
  {
    TRACE;
    
    if (mOwner.HasUI())
    {
      *pSize = ViewRect(0, 0, mOwner.GetEditorWidth(), mOwner.GetEditorHeight());
      
      return kResultTrue;
    }
    else
    {
      return kResultFalse;
    }
  }
  
  tresult PLUGIN_API attached(void* pParent, FIDString type) override
  {
    if (mOwner.HasUI())
    {
      void* pView = nullptr;
#ifdef OS_WIN
      if (strcmp(type, kPlatformTypeHWND) == 0)
        pView = mOwner.OpenWindow(pParent);
#elif defined OS_MAC
      if (strcmp (type, kPlatformTypeNSView) == 0)
        pView = mOwner.OpenWindow(pParent);
      else // Carbon
        return kResultFalse;
#endif
      if (pView)
        mOwner.OnUIOpen();
      
      return kResultTrue;
    }
    
    return kResultFalse;
  }
    
  tresult PLUGIN_API removed() override
  {
    if (mOwner.HasUI())
      mOwner.CloseWindow();
    
    return CPluginView::removed();
  }
  
  void resize(int w, int h)
  {
    TRACE;
    
    ViewRect newSize = ViewRect(0, 0, w, h);
    plugFrame->resizeView(this, &newSize);
  }
  
  T& mOwner;
};
