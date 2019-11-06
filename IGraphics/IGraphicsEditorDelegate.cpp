/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "IGraphicsEditorDelegate.h"
#include "IGraphics.h"
#include "IControl.h"

using namespace iplug;
using namespace igraphics;

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
  UpdateData(GetEditorData(), 0);
}

void* IGEditorDelegate::OpenWindow(void* pParent)
{
  if(!mGraphics) {
    mIGraphicsTransient = true;
    mGraphics = std::unique_ptr<IGraphics>(CreateGraphics());
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
        mGraphics = nullptr;
      }
    }
    mClosing = false;
  }
}

void IGEditorDelegate::SetScreenScale(double scale)
{
  if (GetUI())
    mGraphics->SetScreenScale(static_cast<int>(std::round(scale)));
}

int IGEditorDelegate::SetEditorData(const IByteChunk& data, int startPos)
{
  int endPos = UpdateData(data, startPos);

  mEditorData.Clear();
    
  if (endPos > 0)
   mEditorData.PutBytes(data.GetData() +  startPos, endPos - startPos);
    
  return endPos;
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

    for (int c = 0; c < mGraphics->NControls(); c++)
    {
      IControl* pControl = mGraphics->GetControl(c);
      
      int nVals = pControl->NVals();
      
      for(int v = 0; v < nVals; v++)
      {
        if (pControl->GetParamIdx(v) == paramIdx)
        {
          pControl->SetValueFromDelegate(value, v);
          // Could be more than one, don't break until we check them all.
        }
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

  mGraphics = std::unique_ptr<IGraphics>(pGraphics);
  mIGraphicsTransient = false;
}

bool IGEditorDelegate::EditorResize()
{
  int scale = mGraphics->GetPlatformWindowScale();
  EditorDataModified();
  return EditorResizeFromUI(mGraphics->WindowWidth() * scale, mGraphics->WindowHeight() * scale);
}

void IGEditorDelegate::EditorDataModified()
{
  IByteChunk data;
    
  int width = mGraphics->Width();
  int height = mGraphics->Height();
  float scale = mGraphics->GetDrawScale();
    
  data.Put(&width);
  data.Put(&height);
  data.Put(&scale);
    
  SerializeCustomEditorData(data);
    
  EditorDataChangedFromUI(data);
}

int IGEditorDelegate::UpdateData(const IByteChunk& data, int startPos)
{
  int width = GetEditorWidth();
  int height = GetEditorHeight();
  float scale = 1.f;
    
  // Recall size data (if not present use the defaults above)
    
  startPos = data.Get(&width, startPos);
  startPos = data.Get(&height, startPos);
  startPos = data.Get(&scale, startPos);
    
  // This may resize the editor
    
  if (startPos > 0 && GetUI())
    GetUI()->Resize(width, height, scale);
    
  return UnserializeCustomEditorData(data, startPos);
}
