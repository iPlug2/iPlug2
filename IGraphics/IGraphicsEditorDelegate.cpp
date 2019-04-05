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
  
  int width = GetEditorWidth();
  int height = GetEditorHeight();
  float scale = 1.f;
    
  // Recall size data (if not present use the defaults above)
    
  const IByteChunk& data = GetEditorData();
    
  int pos = data.Get(&width, 0);
  pos = data.Get(&height, pos);
  pos = data.Get(&scale, pos);
    
  if (pos > 0)
    GetUI()->Resize(width, height, scale);
    
  pos = UnSerializeEditorProperties(data, pos);
}

void* IGEditorDelegate::OpenWindow(void* pParent)
{
  if(!mGraphics) {
    mIGraphicsTransient = true;
    mGraphics.reset(CreateGraphics());
  }
  
  if(mGraphics)
    return mGraphics->OpenWindow(pParent);
  else
    return nullptr;
}

void IGEditorDelegate::CloseWindow()
{
  if (!mClosing)
  {
    mClosing = true;
    IEditorDelegate::CloseWindow();
  
    if (mGraphics)
    {
    
      mGraphics->CloseWindow();
    
      if (mIGraphicsTransient)
      {
        mGraphics.reset(nullptr);
      }
    }
    mClosing = false;
  }
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
  assert(!mGraphics); // protect against calling AttachGraphics() when mGraphics already exists

  mGraphics.reset(pGraphics);
  mIGraphicsTransient = false;
}

void IGEditorDelegate::EditorPropertiesModified()
{
  IByteChunk data;
    
  int width = mGraphics->Width();
  int height = mGraphics->Height();
  float scale = mGraphics->GetDrawScale();
    
  data.Put(&width);
  data.Put(&height);
  data.Put(&scale);
    
  SerializeEditorProperties(data);
    
  EditorPropertiesChangedFromUI(mGraphics->WindowWidth(), mGraphics->WindowHeight(), data);
}
