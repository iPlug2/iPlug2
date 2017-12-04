#include "IPlugBaseGraphics.h"
#include "IGraphics.h"
#include "IControl.h"

IPlugBaseGraphics::IPlugBaseGraphics(int nParams, const char* channelIOStr, int nPresets, const char* effectName, const char* productName, const char* mfrName, int vendorVersion, int uniqueID, int mfrID, int latency, bool plugDoesMidi, bool plugDoesChunks, bool plugIsInst, EAPI plugAPI)
: IPlugBase(nParams, channelIOStr, nPresets, effectName, productName, mfrName, vendorVersion, uniqueID, mfrID, latency, plugDoesMidi, plugDoesChunks, plugIsInst, plugAPI)
{  
}

IPlugBaseGraphics::~IPlugBaseGraphics()
{
  DELETE_NULL(mGraphics);
}

int IPlugBaseGraphics::GetUIWidth()
{
  return mGraphics->Width();
}

int IPlugBaseGraphics::GetUIHeight()
{
  return mGraphics->Height();
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

void* IPlugBaseGraphics::OpenWindow(void* handle, void* control)
{
  return mGraphics->OpenWindow(handle, control);
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
