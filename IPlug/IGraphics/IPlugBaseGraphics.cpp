#include "IPlugBaseGraphics.h"
#include "IGraphics.h"
#include "IControl.h"

IPlugBaseGraphics::IPlugBaseGraphics(IPlugConfig config, EAPI plugAPI)
: IPlugBase(config, plugAPI)
{  
}

IPlugBaseGraphics::~IPlugBaseGraphics()
{
  DELETE_NULL(mGraphics);
}

int IPlugBaseGraphics::GetUIWidth()
{
  return mGraphics->WindowWidth();
}

int IPlugBaseGraphics::GetUIHeight()
{
  return mGraphics->WindowHeight();
    
}

void IPlugBaseGraphics::AttachGraphics(IGraphics* pGraphics)
{
  if (pGraphics)
  {
    int i, n = mParams.GetSize();
    
    for (i = 0; i < n; ++i)
    {
      pGraphics->SetParameterFromPlug(i, GetParam(i)->GetNormalized(), true);
    }
    
    pGraphics->PrepDraw();
    mGraphics = pGraphics;
    mHasUI = true;
  }
  
  OnGUICreated();
}

void IPlugBaseGraphics::RedrawParamControls()
{
  if (mGraphics)
  {
    int i, n = mParams.GetSize();
    for (i = 0; i < n; ++i)
    {
      double v = mParams.Get(i)->Value();
      mGraphics->SetParameterFromPlug(i, v, false);
    }
  }
}

void* IPlugBaseGraphics::OpenWindow(void* handle)
{
  return mGraphics->OpenWindow(handle);
}

void IPlugBaseGraphics::CloseWindow()
{
  mGraphics->CloseWindow();
}

void IPlugBaseGraphics::SetParameterInUIFromAPI(int paramIdx, double value, bool normalized)
{
  if(mGraphics)
    mGraphics->SetParameterFromPlug(paramIdx, value, normalized);
}
