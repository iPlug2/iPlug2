#if WINDOWS_VERSION
#include <windows.h>
#include "Mac2Win.H"
#endif

#include "IPlugGroup.h"
#include "FicPluginEnums.h"
#include "CDAEError.h"
#include "IPlugProcess.h"
#include "CNoResourceView.h"
#include "CPlugincontrol_OnOff.h"
#include "CPluginControl_Linear.h"
#include "../IGraphics.h"
#include "IPlugCustomUI.h"

IPlugProcess::IPlugProcess(void)
: mCustomUI(0), mNoUIView(0), mModuleHandle(0), mPlug(NULL), mDirectMidiInterface(NULL)
{
  mPluginWinRect.top = 0;
  mPluginWinRect.left = 0;
  mPluginWinRect.bottom = 0;
  mPluginWinRect.right = 0;
#if WINDOWS_VERSION
  mModuleHandle = (void*) gThisModule;  // extern from DLLMain.cpp; HINSTANCE of the DLL
#else 
  mModuleHandle = 0;
#endif
}

IPlugProcess::~IPlugProcess(void)
{
  if(mCustomUI)
    DELETE_NULL(mCustomUI);

  mNoUIView = NULL;
  
  DirectMidi_FreeClient((void*)mDirectMidiInterface);
  
  DELETE_NULL(mPlug);
}
 
void IPlugProcess::EffectInit()
{
  TRACE;
  
  if (mPlug)
  {
    AddControl(new CPluginControl_OnOff('bypa', "Master Bypass\nMastrByp\nMByp\nByp", false, true)); // Default to off
    DefineMasterBypassControlIndex(1);
    
    int paramCount = mPlug->NParams();
    
    for (int i=0;i<paramCount;i++)
    {
      IParam *p = mPlug->GetParam(i);
      
      double low=0.0; double high=1.0;
      p->GetBounds(&low, &high);
      
      AddControl(new CPluginControl_Linear(' ld '+i, p->GetNameForHost(), low, high, p->GetStep(), p->GetDefault(), p->GetCanAutomate()));
    }
    
    if (PLUG_DOES_MIDI)
    {
      ComponentResult result = noErr;
      
      Cmn_Int32 requestedVersion = 7;
      
      std::string midiNodeName(PLUG_NAME" Midi");
      
      while (requestedVersion)
      {
        result = DirectMidi_RegisterClient(requestedVersion, this, reinterpret_cast<Cmn_UInt32>(this), (void **)&mDirectMidiInterface); 
        
        if (result == noErr && mDirectMidiInterface != NULL)
        {
          mDirectMidiInterface->CreateRTASBufferedMidiNode(0, const_cast<char *>(midiNodeName.c_str()), 1);
          
          break;
        }
        
        requestedVersion--;
      }
      
      mPlug->Reset();
    }
  }
}

// TODO: check this is nessecary
void IPlugProcess::DoTokenIdle (void)
{
  CEffectProcess::DoTokenIdle();  // call inherited to get tokens so we can move controls.
  
  if(mCustomUI) 
    mCustomUI->Idle();
}

//  Tells Pro Tools what size view it should create for you. 
void IPlugProcess::GetViewRect(Rect *viewRect)
{
  if (mPlug)
  { 
    if(!mCustomUI)
      mCustomUI = CreateIPlugCustomUI((void*)this);
    
    mCustomUI->GetRect(&mPluginWinRect.left, &mPluginWinRect.top, &mPluginWinRect.right, &mPluginWinRect.bottom);
    viewRect->left = mPluginWinRect.left;
    viewRect->top = mPluginWinRect.top;
    viewRect->right = mPluginWinRect.right;
    viewRect->bottom = mPluginWinRect.bottom;
  }
}

CPlugInView* IPlugProcess::CreateCPlugInView()
{
  CNoResourceView *ui = 0;
  
  if (mPlug)
  {
    try
    {
      if( !mCustomUI )
      {
        mCustomUI = CreateIPlugCustomUI((void*)this);
        mCustomUI->GetRect(&mPluginWinRect.left, &mPluginWinRect.top, &mPluginWinRect.right, &mPluginWinRect.bottom);
      }
      
      ui = new CNoResourceView;
      ui->SetSize(mPluginWinRect.right, mPluginWinRect.bottom);
      
      mNoUIView = (IPlugDigiView *) ui->AddView2("!NoUIView[('NoID')]", 0, 0, mPluginWinRect.right, mPluginWinRect.bottom, false);
      
      if( mNoUIView )
        mNoUIView->SetCustomUI( mCustomUI );
    }
    catch(...)
    {
      if(mCustomUI)
        DELETE_NULL(mCustomUI);
        
      if(ui)
        DELETE_NULL(ui);
    }
  }
  
  return ui;
}
 
void IPlugProcess::SetViewPort (GrafPtr aPort)
{
  mMainPort = aPort;
  CEffectProcess::SetViewPort( mMainPort ); // call inherited first
  
  if(mMainPort) // opening window, set it up.
  {
    if(mCustomUI)
    {
      void *windowPtr = NULL;
#if WINDOWS_VERSION
      windowPtr = (void*)ASI_GethWnd((WindowPtr)mMainPort);
#elif MAC_VERSION
      windowPtr = (void*) GetWindowFromPort( mMainPort ); // WindowRef for Carbon, not GrafPtr (quickdraw)
#endif
      
      mCustomUI->Open(windowPtr);
        
#if WINDOWS_VERSION
      // added for Staley to overcome Windows re-draw issues  - DLM
      mCustomUI->Draw(mPluginWinRect.left, mPluginWinRect.top, mPluginWinRect.right, mPluginWinRect.bottom);
#endif
    }
  }
  else  // do clean-up
  {
    if(mCustomUI)
      mCustomUI->Close();
    
    if( mNoUIView )
      mNoUIView->SetEnable(false);
  }
  return;
}

void IPlugProcess::UpdateControlValueInAlgorithm (long idx)
{
  if (!IsValidControlIndex(idx)) return;
  if (idx==mMasterBypassIndex)  return;
  
  CPluginControl_Continuous *cc=dynamic_cast<CPluginControl_Continuous*>(GetControl(idx));
  double value=0;
  
  if (cc) 
  {
    // from the protools SDK source
    static const double kControlMin = -2147483648.0;
    static const double kControlMax = 2147483647.0;
    value = ((double)cc->GetValue() - kControlMin) / (kControlMax - kControlMin);
  }
  
  idx -= kPTParamIdxOffset;
  
  if (idx >= 0 && idx < mPlug->NParams()) 
  {
    mPlug->GetParam(idx)->SetNormalized(value);
    
    if (mPlug->GetGUI()) 
    {
      mPlug->GetGUI()->SetParameterFromPlug(idx, value, true);
      //printf("UpdateControlValueInAlgorithm %d %s %f\n", idx, mPlug->GetParam(idx)->GetNameForHost(), value);
    }
    mPlug->OnParamChange(idx);
  }
}

long IPlugProcess::SetControlValue (long idx, long aValue)
{
  CPluginControl_Continuous *cc=dynamic_cast<CPluginControl_Continuous*>(GetControl(idx));
  
  if (cc) 
    cc->SetValue(aValue);
  
  return (long)CProcess::SetControlValue(idx, aValue);;
}
 
long IPlugProcess::GetControlValue(long idx, long *aValue)
{
  return (long)CProcess::GetControlValue(idx, aValue);
}

long IPlugProcess::GetControlDefaultValue(long aControlIndex, long* aValue)
{
  return (long)CProcess::GetControlDefaultValue(aControlIndex, aValue);
}

// TODO: this caused jittery audio, is it nessecary?
//ComponentResult IPlugProcess::UpdateControlGraphic (long aControlIndex, long aValue)
//{
//  if (mCustomUI)
//    return mCustomUI->UpdateGraphicControl(aControlIndex, aValue);
//  else
//    return noErr;
//}

//Boolean IPlugProcess::HandleKeystroke(EventRecord *theEvent) 
//{
//	bool usedEvent = false;
//
//	return usedEvent;
//}

int IPlugProcess::ProcessTouchControl (long aControlIndex)
{
  return (int)CProcess::TouchControl(aControlIndex);
}

int IPlugProcess::ProcessReleaseControl (long aControlIndex)
{
  return (int)CProcess::ReleaseControl(aControlIndex);
}

void IPlugProcess::ProcessDoIdle()
{
  this->DoIdle(NULL);
  gProcessGroup->YieldCriticalSection();
}

ComponentResult IPlugProcess::SetControlHighliteInfo(long controlIndex,short  isHighlighted,short color)
{
  return noErr;
}

ComponentResult IPlugProcess::ChooseControl (Point aLocalCoord, long *aControlIndex)
{
  int lastclicked = mPlug->GetGUI()->GetLastClickedParamForPTAutomation();
 
  if(lastclicked > -1)
     *aControlIndex = (long) lastclicked + kPTParamIdxOffset;
  else
    *aControlIndex = 0;
 
  return noErr;
}

void IPlugProcess::ConnectSidechain(void)
{
  CEffectProcess::ConnectSidechain();
  mPlug->SetNumSideChainInputs(1);
}

void IPlugProcess::DisconnectSidechain(void)
{
  CEffectProcess::DisconnectSidechain();
  mPlug->SetNumSideChainInputs(0);
}

