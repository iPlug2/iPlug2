#include "IGraphicsEditorDelegate.h"
#include "IGraphics.h"
#include "IControl.h"

IGraphicsEditorDelegate::IGraphicsEditorDelegate(int nParams)
: IEditorDelegate(nParams)
{  
}

IGraphicsEditorDelegate::~IGraphicsEditorDelegate()
{
  DELETE_NULL(mGraphics);
}

void IGraphicsEditorDelegate::AttachGraphics(IGraphics* pGraphics)
{
  if (pGraphics)
  {
    mGraphics = pGraphics;

    for (auto i = 0; i < NParams(); ++i)
    {
      SendParameterValueFromDelegate(i, GetParam(i)->GetNormalized(), true);
    }
        
    // TODO: is it safe/sensible to do this here
    pGraphics->OnResizeOrRescale();
  }
}

IGraphics* IGraphicsEditorDelegate::GetUI()
{
  assert(mGraphics); // can't call this till AttachGraphics() has been called
  return mGraphics;
}

void IGraphicsEditorDelegate::OnRestoreState()
{
  if (mGraphics)
  {
    int i, n = mParams.GetSize();
    for (i = 0; i < n; ++i)
    {
      double v = mParams.Get(i)->Value();
      SendParameterValueFromDelegate(i, v, false);
    }
  }
}

void* IGraphicsEditorDelegate::OpenWindow(void* pParent)
{
  if(!mGraphics)
    CreateUI();
  
  if(mGraphics)
    return mGraphics->OpenWindow(pParent);
  else
    return nullptr;
}

void IGraphicsEditorDelegate::CloseWindow()
{
  if(mGraphics)
    mGraphics->CloseWindow();
}

void IGraphicsEditorDelegate::SendControlValueFromDelegate(int controlTag, double normalizedValue)
{
  assert(mGraphics != nullptr);

  if (controlTag > kNoTag)
  {
    for (auto c = 0; c < mGraphics->NControls(); c++)
    {
      IControl* pControl = mGraphics->GetControl(c);
      
      if (pControl->GetTag() == controlTag)
      {
        pControl->SetValueFromDelegate(normalizedValue);
      }
    }
  }
}

void IGraphicsEditorDelegate::SendControlMsgFromDelegate(int controlTag, int messageTag, int dataSize, const void* pData)
{
  if(!mGraphics)
    return;
  
  if (controlTag > kNoTag)
  {
    for (auto c = 0; c < mGraphics->NControls(); c++)
    {
      IControl* pControl = mGraphics->GetControl(c);
      
      if (pControl->GetTag() == controlTag)
      {
        pControl->OnMsgFromDelegate(messageTag, dataSize, pData);
      }
    }
  }
}

void IGraphicsEditorDelegate::SendParameterValueFromDelegate(int paramIdx, double value, bool normalized)
{
  assert(mGraphics != nullptr); // Did you attach an IGraphics yet?
  //- maybe you wanted to #define NO_IGRAPHICS or set PLUG_HAS_UI to 0?
  
  if (!normalized)
    value = GetParam(paramIdx)->ToNormalized(value);

  for (auto c = 0; c < mGraphics->NControls(); c++)
  {
    IControl* pControl = mGraphics->GetControl(c);
    
    if (pControl->ParamIdx() == paramIdx)
    {
      pControl->SetValueFromDelegate(value);
      // Could be more than one, don't break until we check them all.
    }
  }
  
  //TODO: aux params disabled
//  int i, n = mControls.GetSize();
//  IControl** ppControl = mControls.GetList();
//  for (i = 0; i < n; ++i, ++ppControl)
//  {
//    IControl* pControl = *ppControl;
//    if (pControl->ParamIdx() == paramIdx)
//    {
//      pControl->SetValueFromDelegate(value);
//      // Could be more than one, don't break until we check them all.
//    }
//
//    // now look for any auxilliary parameters
//    // BULL SHIP this only works with 1
//    int auxParamIdx = pControl->GetAuxParamIdx(paramIdx);
//    
//    if (auxParamIdx > -1) // there are aux params
//    {
//      pControl->SetAuxParamValueFromPlug(auxParamIdx, value);
//    }
//  }
  
  IEditorDelegate::SendParameterValueFromDelegate(paramIdx, value, normalized);
}

void IGraphicsEditorDelegate::SendMidiMsgFromDelegate(const IMidiMsg& msg)
{
  if(mGraphics)
  {
    for (auto c = 0; c < mGraphics->NControls(); c++) // TODO: could keep a map
    {
      IControl* pControl = mGraphics->GetControl(c);
      
      if (pControl->WantsMidi())
      {
        pControl->OnMidi(msg);
      }
    }
  }
  
  IEditorDelegate::SendMidiMsgFromDelegate(msg);
}

void IGraphicsEditorDelegate::ForControlWithParam(int paramIdx, std::function<void(IControl& control)> func)
{
  for (auto c = 0; c < mGraphics->NControls(); c++)
  {
    IControl* pControl = mGraphics->GetControl(c);
    
    if (pControl->ParamIdx() == paramIdx)
    {
      func(*pControl);
      // Could be more than one, don't break until we check them all.
    }
  }
}

void IGraphicsEditorDelegate::ForControlInGroup(const char* group, std::function<void(IControl& control)> func)
{
  for (auto c = 0; c < mGraphics->NControls(); c++)
  {
    IControl* pControl = mGraphics->GetControl(c);
    
    if (CStringHasContents(pControl->GetGroup()))
    {
      if(strcmp(pControl->GetGroup(), group) == 0)
        func(*pControl);
      // Could be more than one, don't break until we check them all.
    }
  }
}
