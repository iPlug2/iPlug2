#include "IPlugBaseGraphics.h"
#include "IGraphics.h"

IPlugBaseGraphics::IPlugBaseGraphics(IPlugConfig config, EAPI plugAPI)
: IPlugBase(config, plugAPI)
{  
}

IPlugBaseGraphics::~IPlugBaseGraphics()
{
  DELETE_NULL(mGraphics);
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
    
    mGraphics = pGraphics;
    mHasUI = true;
    
    // TODO: is it safe/sensible to do this here
    pGraphics->OnDisplayScale();
  }
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

void* IPlugBaseGraphics::OpenWindow(void* pHandle)
{
  if(mGraphics)
    return mGraphics->OpenWindow(pHandle);
  else
    return nullptr;
}

void IPlugBaseGraphics::CloseWindow()
{
  if(mGraphics)
    mGraphics->CloseWindow();
}

void IPlugBaseGraphics::SendParameterValueToUIFromAPI(int paramIdx, double value, bool normalized)
{
  if(mGraphics)
    mGraphics->SetParameterFromPlug(paramIdx, value, normalized);
}

void IPlugBaseGraphics::PrintDebugInfo()
{
  if(!mGraphics)
    return IPlugBase::PrintDebugInfo();
    
  WDL_String buildInfo;
  GetBuildInfoStr(buildInfo);
  DBGMSG("\n%s\n%s Graphics %i FPS\n--------------------------------------------------\n", buildInfo.Get(), mGraphics->GetDrawingAPIStr(), mGraphics->FPS());

#if defined TRACER_BUILD && !defined TRACE_TO_STDOUT
  WDL_String pHomePath;
  mGraphics->UserHomePath(pHomePath);
  DBGMSG("Location of the Tracer Build Log: \n%s/%s\n\n", pHomePath.Get(), LOGFILE);
#endif
}
