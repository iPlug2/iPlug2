/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

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

void IGEditorDelegate::OnUIOpen()
{
  IEditorDelegate::OnUIOpen();
  
  GetUI()->Resize(GetEditorWidth(), GetEditorHeight(), GetEditorScale());
}

void* IGEditorDelegate::OpenWindow(void* pParent)
{
  if(!mGraphics) {
    mIGraphicsTransient = true;
    mGraphics = CreateGraphics();
  }
  
  if(mGraphics)
    return mGraphics->OpenWindow(pParent);
  else
    return nullptr;
}

void IGEditorDelegate::CloseWindow()
{
  if(mGraphics)
    mGraphics->CloseWindow();
  
  if(mIGraphicsTransient)
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
      
      if (pControl->GetWantsMidi())
      {
        pControl->OnMidi(msg);
      }
    }
  }
  
  IEditorDelegate::SendMidiMsgFromDelegate(msg);
}

void IGEditorDelegate::AttachGraphics(IGraphics* pGraphics)
{
  assert(mGraphics == nullptr); // protect against calling AttachGraphics() when mGraphics allready exists

  mGraphics = pGraphics;
  mIGraphicsTransient = false;
}
