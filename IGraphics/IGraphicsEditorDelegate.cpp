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

void* IGEditorDelegate::OpenWindow(void* pParent)
{
  if(!mGraphics)
  {
    mGraphics = std::unique_ptr<IGraphics>(CreateGraphics());
    if (mLastWidth && mLastHeight && mLastScale)
      GetUI()->Resize(mLastWidth, mLastHeight, mLastScale);
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
      mLastWidth = mGraphics->Width();
      mLastHeight = mGraphics->Height();
      mLastScale = mGraphics->GetDrawScale();
      mGraphics->CloseWindow();
      mGraphics = nullptr;
    }
    
    mClosing = false;
  }
}

void IGEditorDelegate::SetScreenScale(float scale)
{
  if (GetUI())
    mGraphics->SetScreenScale(scale);
}

void IGEditorDelegate::SendControlValueFromDelegate(int ctrlTag, double normalizedValue)
{
  if(!mGraphics)
    return;

  IControl* pControl = mGraphics->GetControlWithTag(ctrlTag);
  
  assert(pControl);
  
  if(pControl)
  {
    pControl->SetValueFromDelegate(normalizedValue);
  }
}

void IGEditorDelegate::SendControlMsgFromDelegate(int ctrlTag, int msgTag, int dataSize, const void* pData)
{
  if(!mGraphics)
    return;
  
  IControl* pControl = mGraphics->GetControlWithTag(ctrlTag);
  
  assert(pControl);
  
  if(pControl)
  {
    pControl->OnMsgFromDelegate(msgTag, dataSize, pData);
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

bool IGEditorDelegate::SerializeEditorSize(IByteChunk& data) const
{
  bool savedOK = true;
    
  int width = mGraphics ? mGraphics->Width() : mLastWidth;
  int height = mGraphics ? mGraphics->Height() : mLastHeight;
  float scale = mGraphics ? mGraphics->GetDrawScale() : mLastScale;
    
  savedOK &= data.Put(&width);
  savedOK &= data.Put(&height);
  savedOK &= data.Put(&scale);
    
  return savedOK;
}

int IGEditorDelegate::UnserializeEditorSize(const IByteChunk& data, int startPos)
{
  int width = 0;
  int height = 0;
  float scale = 0.f;
    
  startPos = data.Get(&width, startPos);
  startPos = data.Get(&height, startPos);
  startPos = data.Get(&scale, startPos);
    
  if (GetUI())
  {
    if (width && height && scale)
      GetUI()->Resize(width, height, scale);
  }
  else
  {
    mLastWidth = width;
    mLastHeight = height;
    mLastScale = scale;
  }
    
  return startPos;
}

bool IGEditorDelegate::SerializeEditorState(IByteChunk& chunk) const
{
  return SerializeEditorSize(chunk);
}

int IGEditorDelegate::UnserializeEditorState(const IByteChunk& chunk, int startPos)
{
  return UnserializeEditorSize(chunk, startPos);
}

bool IGEditorDelegate::OnKeyDown(const IKeyPress& key)
{
  IGraphics* pGraphics = GetUI();
  
  if (pGraphics)
  {
    float x, y;
    pGraphics->GetMouseLocation(x, y);
    return pGraphics->OnKeyDown(x, y, key);
  }
  else
    return false;
}

bool IGEditorDelegate::OnKeyUp(const IKeyPress& key)
{
  IGraphics* pGraphics = GetUI();

  if (pGraphics)
  {
    float x, y;
    pGraphics->GetMouseLocation(x, y);
    return pGraphics->OnKeyUp(x, y, key);
  }
  else
    return false;
}
