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
      SendParameterValueToUIFromDelegate(i, GetParam(i)->GetNormalized(), true);
    }
        
    // TODO: is it safe/sensible to do this here
    pGraphics->OnDisplayScale();
  }
}

void IGraphicsEditorDelegate::OnRestoreState()
{
  if (mGraphics)
  {
    int i, n = mParams.GetSize();
    for (i = 0; i < n; ++i)
    {
      double v = mParams.Get(i)->Value();
      SendParameterValueToUIFromDelegate(i, v, false);
    }
  }
}

void* IGraphicsEditorDelegate::OpenWindow(void* pHandle)
{
  if(mGraphics)
    return mGraphics->OpenWindow(pHandle);
  else
    return nullptr;
}

void IGraphicsEditorDelegate::CloseWindow()
{
  if(mGraphics)
    mGraphics->CloseWindow();
}

//void IGraphicsEditorDelegate::PrintDebugInfo() const
//{
//  assert(mGraphics != nullptr);
//
//  if(!mGraphics)
//    return IPlugBase::PrintDebugInfo();
//
//  WDL_String buildInfo;
//  GetBuildInfoStr(buildInfo);
//  DBGMSG("\n%s\n%s Graphics %i FPS\n--------------------------------------------------\n", buildInfo.Get(), mGraphics->GetDrawingAPIStr(), mGraphics->FPS());
//
//#if defined TRACER_BUILD && !defined TRACE_TO_STDOUT
//  WDL_String pHomePath;
//  mGraphics->UserHomePath(pHomePath);
//  DBGMSG("Location of the Tracer Build Log: \n%s/%s\n\n", pHomePath.Get(), LOGFILE);
//#endif
//}

void IGraphicsEditorDelegate::SetControlValueFromDelegate(int controlTag, double normalizedValue)
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
  assert(mGraphics != nullptr);
  
  if (controlTag > kNoTag)
  {
    for (auto c = 0; c < mGraphics->NControls(); c++)
    {
      IControl* pControl = mGraphics->GetControl(c);
      
      if (pControl->GetTag() == controlTag)
      {
        pControl->OnDataFromDelegate(messageTag, dataSize, pData);
      }
    }
  }
}

void IGraphicsEditorDelegate::SendParameterValueToUIFromDelegate(int paramIdx, double value, bool normalized)
{
  assert(mGraphics != nullptr);
  
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
  
  IEditorDelegate::SendParameterValueToUIFromDelegate(paramIdx, value, normalized);
}

// TODO: ResizeGraphicsFromUI
void IGraphicsEditorDelegate::ResizeGraphicsFromUI()
{
  assert(mGraphics != nullptr);
//
//  mWidth = mGraphics->WindowWidth();
//  mHeight = mGraphics->WindowHeight();
//  ResizeGraphics();
}

void IGraphicsEditorDelegate::OnMidiMsgUI(const IMidiMsg& msg)
{
  assert(mGraphics != nullptr);
  
  for (auto c = 0; c < mGraphics->NControls(); c++) // TODO: could keep a map
  {
    IControl* pControl = mGraphics->GetControl(c);
    
    if (pControl->WantsMIDI())
    {
      pControl->OnMidi(msg);
    }
  }
}

