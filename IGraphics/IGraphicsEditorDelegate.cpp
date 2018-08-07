#include "IGraphicsEditorDelegate.h"
#include "IGraphics.h"
#include "IControl.h"

IGEditorDelegate::IGEditorDelegate(int nParams)
: IEditorDelegate(nParams)
{  
}

IGEditorDelegate::~IGEditorDelegate()
{
}

void IGEditorDelegate::OnUIReady(IGraphics* pGraphics)
{
  for (auto i = 0; i < NParams(); ++i)
  {
    SendParameterValueFromDelegate(i, GetParam(i)->GetNormalized(), true);
  }

  // TODO: is it safe/sensible to do this here
  pGraphics->OnResizeOrRescale();
}

IGraphics* IGEditorDelegate::GetUI()
{
  return mGraphics;
}

void IGEditorDelegate::OnRestoreState()
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

void* IGEditorDelegate::OpenWindow(void* pParent)
{
  if(!mGraphics)
    mGraphics = CreateGraphics();
  
  if(mGraphics)
    return mGraphics->OpenWindow(pParent);
  else
    return nullptr;
}

void IGEditorDelegate::CloseWindow()
{
  if(mGraphics)
    mGraphics->CloseWindow();
  
  DELETE_NULL(mGraphics);
}

void IGEditorDelegate::SendControlValueFromDelegate(int controlTag, double normalizedValue)
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
        pControl->SetValueFromDelegate(normalizedValue);
      }
    }
  }
}

void IGEditorDelegate::SendControlMsgFromDelegate(int controlTag, int messageTag, int dataSize, const void* pData)
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

void IGEditorDelegate::SendParameterValueFromDelegate(int paramIdx, double value, bool normalized)
{
  if(mGraphics)
  {
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
  }
  
  IEditorDelegate::SendParameterValueFromDelegate(paramIdx, value, normalized);
}

void IGEditorDelegate::SendMidiMsgFromDelegate(const IMidiMsg& msg)
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

void IGEditorDelegate::ForControlWithParam(int paramIdx, std::function<void(IControl& control)> func)
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

void IGEditorDelegate::ForControlInGroup(const char* group, std::function<void(IControl& control)> func)
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
