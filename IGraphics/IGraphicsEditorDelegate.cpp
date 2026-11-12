/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "IGraphicsEditorDelegate.h"
#include "IGraphics.h"
#include "IControl.h"
#include <algorithm>
#include <cmath>
#include <climits>

#ifdef OS_LINUX
#include "IGraphicsLinux.h"
#define IPLUG_GFX_LOCK std::lock_guard<std::recursive_mutex> gfxLock(mGfxMutex)
#else
#define IPLUG_GFX_LOCK ((void)0)
#endif

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
  IPLUG_GFX_LOCK;
#ifdef OS_LINUX
  if (mGraphics && static_cast<IGraphicsLinux*>(mGraphics.get())->IsInPlatformCallback())
  {
    fprintf(stderr, "Cannot reopen the Linux editor during graphics dispatch\n");
    return nullptr;
  }
#endif
#ifdef OS_LINUX
  if (!PrepareGraphics())
    return nullptr;
#else
  if(!mGraphics)
  {
    mGraphics = std::unique_ptr<IGraphics>(CreateGraphics());
    if (mLastWidth && mLastHeight && mLastScale)
      GetUI()->Resize(mLastWidth, mLastHeight, mLastScale);
  }
#endif
  
  if(mGraphics)
  {
    void* window = mGraphics->OpenWindow(pParent);
#ifdef OS_LINUX
    if (!window)
      CloseWindow();
#endif
    return window;
  }
  else
    return nullptr;
}

void IGEditorDelegate::CloseWindow()
{
  IPLUG_GFX_LOCK;
#ifdef OS_LINUX
  if (!mGraphics)
    return;
  auto* graphics = static_cast<IGraphicsLinux*>(mGraphics.get());
  if (graphics->IsInPlatformCallback())
  {
    graphics->RequestWindowClose();
    return;
  }
#endif
  if (!mClosing)
  {
    mClosing = true;
#ifdef OS_LINUX
    if (mGraphics->WindowIsOpen())
      IEditorDelegate::CloseWindow();
#else
    IEditorDelegate::CloseWindow();
#endif
  
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

#ifdef OS_LINUX
bool IGEditorDelegate::PrepareGraphics()
{
  IPLUG_GFX_LOCK;
  if (!mGraphics)
  {
    mGraphics.reset(CreateGraphics());
    if (!mGraphics)
      return false;
    if (mLastWidth && mLastHeight && mLastScale)
      mGraphics->Resize(mLastWidth, mLastHeight, mLastScale);
    if (mHostScreenScale > 0.f)
      mGraphics->SetScreenScale(mHostScreenScale);
    SetEditorSize(mGraphics->WindowWidth() * mGraphics->GetScreenScale(),
                  mGraphics->WindowHeight() * mGraphics->GetScreenScale());
  }
  return true;
}

bool IGEditorDelegate::SetHostScreenScale(float scale)
{
  IPLUG_GFX_LOCK;
  if (!std::isfinite(scale) || scale <= 0.f || !PrepareGraphics())
    return false;
  const double width = mGraphics->WindowWidth() * static_cast<double>(scale);
  const double height = mGraphics->WindowHeight() * static_cast<double>(scale);
  if (width < 1. || height < 1. || width >= INT_MAX || height >= INT_MAX)
    return false;
  mHostScreenScale = scale;
  static_cast<IGraphicsLinux*>(mGraphics.get())->SetScreenScaleFromHost(scale);
  return true;
}

bool IGEditorDelegate::CloseWindowIfRequested()
{
  IPLUG_GFX_LOCK;
  if (mGraphics && !static_cast<IGraphicsLinux*>(mGraphics.get())->IsInPlatformCallback() &&
      static_cast<IGraphicsLinux*>(mGraphics.get())->WindowCloseRequested())
  {
    CloseWindow();
    return true;
  }
  return false;
}
#endif

void IGEditorDelegate::OnParentWindowResize(int width, int height)
{
  IPLUG_GFX_LOCK;
  if (auto* pGraphics = GetUI())
  {
#ifdef OS_LINUX
    static_cast<IGraphicsLinux*>(pGraphics)->ResizeFromHost(width, height);
#else
    const auto scale = pGraphics->GetPlatformWindowScale();
    pGraphics->Resize(static_cast<int>(width / scale), static_cast<int>(height / scale), 1.0f, false);
#endif
  }
}

void IGEditorDelegate::SetScreenScale(float scale)
{
  IPLUG_GFX_LOCK;
#ifdef OS_LINUX
  SetHostScreenScale(scale);
#else
  if (GetUI())
    mGraphics->SetScreenScale(scale);
#endif
}

void IGEditorDelegate::SendControlValueFromDelegate(int ctrlTag, double normalizedValue)
{
  IPLUG_GFX_LOCK;
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
  IPLUG_GFX_LOCK;
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
  IPLUG_GFX_LOCK;
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
  IPLUG_GFX_LOCK;
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
  IPLUG_GFX_LOCK;
  bool savedOK = true;
    
  int width = mGraphics ? mGraphics->Width() : mLastWidth;
  int height = mGraphics ? mGraphics->Height() : mLastHeight;
  float scale = mGraphics ? mGraphics->GetDrawScale() : mLastScale;
    
  savedOK &= (data.Put(&width) > 0);
  savedOK &= (data.Put(&height) > 0);
  savedOK &= (data.Put(&scale) > 0);

  return savedOK;
}

int IGEditorDelegate::UnserializeEditorSize(const IByteChunk& data, int startPos)
{
  IPLUG_GFX_LOCK;
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
  IPLUG_GFX_LOCK;
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
  IPLUG_GFX_LOCK;
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
