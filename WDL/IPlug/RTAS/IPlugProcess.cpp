#if WINDOWS_VERSION
#include <windows.h>
#include "Mac2Win.H"
#endif
#include <assert.h>
#include "IPlugGroup.h"
#include "FicPluginEnums.h"
#include "FicProcessTokens.h"
#include "CDAEError.h"
#include "IPlugProcess.h"
#include "CNoResourceView.h"
#include "SliderConversions.h"
#include "CPlugincontrol_OnOff.h"
#include "CPluginControl_Discrete.h"
#include "CPluginControl_Linear.h"
#include "CPluginControl_List.h"
#include "../IGraphics.h"
#include "IPlugCustomUI.h"
#include "Fic.h"

IPlugProcess::IPlugProcess(OSType type)
: mCustomUI(0)
, mView(0)
, mModuleHandle(0)
, mPlug(NULL)
, mDirectMidiInterface(NULL)
, mPluginID(type)
, mLeftOffset(0)
, mTopOffset(0)
{
  TRACE;

  mPluginWinRect.top = 0;
  mPluginWinRect.left = 0;
  mPluginWinRect.bottom = 0;
  mPluginWinRect.right = 0;
#if WINDOWS_VERSION
  mModuleHandle = (void*) gThisModule;  // extern from DLLMain.cpp; HINSTANCE of the DLL
#else 
  mModuleHandle = 0;
#endif
  
  mPlug = MakePlug();
#if PLUG_DOES_STATE_CHUNKS
  AddChunk(mPluginID, PLUG_MFR);
#endif
}

IPlugProcess::~IPlugProcess(void)
{
  if(mCustomUI)
    DELETE_NULL(mCustomUI);

  mView = NULL;
  
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
      
      switch (p->Type()) {
        case IParam::kTypeDouble:
          AddControl(new CPluginControl_Linear(' ld '+i, p->GetNameForHost(), p->GetMin(), p->GetMax(), p->GetStep(), p->GetDefault(), p->GetCanAutomate()));
          break;
        case IParam::kTypeInt:
          AddControl(new CPluginControl_Discrete(' ld '+i, p->GetNameForHost(), (long) p->GetMin(), (long) p->GetMax(), (long) p->GetDefault(), p->GetCanAutomate()));
          break;
        case IParam::kTypeEnum:
        case IParam::kTypeBool: {
          std::vector<std::string> displayTexts;
          for (int j=0; j<p->GetNDisplayTexts(); j++) {
            displayTexts.push_back(p->GetDisplayText(j));
          }
          
          assert(displayTexts.size());
          AddControl(new CPluginControl_List(' ld '+i, p->GetNameForHost(), displayTexts, (long) p->GetDefault(), p->GetCanAutomate()));
          break; }
        default:
          break;
      }

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
    }
    
    mPlug->SetIO(GetNumInputs(), GetNumOutputs());
    mPlug->SetSampleRate(GetSampleRate());
    mPlug->Reset();
  }
}

void IPlugProcess::DoTokenIdle (void)
{
  CEffectProcess::DoTokenIdle();  // call inherited to get tokens so we can move controls.
  
  // TODO: check this... idle not implemented in AU and not widely supported in vst... do we need it in PT?
#ifdef USE_IDLE_CALLS
  mPlug->OnIdle();
#endif

  // Disable metering for AS during Preview mode because doidle is called too frequently during
	// preview when the mouse is moving.
//	if(!IsAS()) 
//		UpdateMeters();
//	else if (GetASPreviewState() == previewState_Off)
//		UpdateMeters();
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
      if( !mCustomUI ) {
        mCustomUI = CreateIPlugCustomUI((void*)this);
        mCustomUI->GetRect(&mPluginWinRect.left, &mPluginWinRect.top, &mPluginWinRect.right, &mPluginWinRect.bottom);
      }
      
      ui = new CNoResourceView;
      ui->SetSize(mPluginWinRect.right, mPluginWinRect.bottom);
      
      mView = (IPlugDigiView *) ui->AddView2("!IPlugDigiView[('NoID')]", 0, 0, mPluginWinRect.right, mPluginWinRect.bottom, false);
      
      if( mView )
        mView->SetCustomUI(mCustomUI);
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
  
  if(mMainPort) {
    if(mCustomUI) {
      void *windowPtr = NULL;
      #if WINDOWS_VERSION
      windowPtr = (void*)ASI_GethWnd((WindowPtr)mMainPort);
      #elif MAC_VERSION
      windowPtr = (void*)GetWindowFromPort(mMainPort); // WindowRef for Carbon, not GrafPtr (quickdraw)
      
      // https://developer.digidesign.com/index.php?L1=5&L2=13&L3=56
      Rect aRect;
      GetPortBounds(aPort, &aRect);
      if ((aRect.left <= 0 && aRect.top <= 0) &&    // Sanity check
          (mLeftOffset == 0 && mTopOffset == 0))    // Skip if offset was already set
      {
        mLeftOffset = -aRect.left;
        mTopOffset = -aRect.top;
      }
      #endif
      mCustomUI->Open(windowPtr, mLeftOffset, mTopOffset);
      #if WINDOWS_VERSION
      mCustomUI->Draw(mPluginWinRect.left, mPluginWinRect.top, mPluginWinRect.right, mPluginWinRect.bottom);
      #endif
    }
  }
  else 
  {
    if(mCustomUI)
      mCustomUI->Close();
    
    if( mView )
      mView->SetEnable(false);
  }
  return;
}

void IPlugProcess::UpdateControlValueInAlgorithm (long idx)
{
  TRACE;
  
  if (!IsValidControlIndex(idx)) return;
  if (idx == mMasterBypassIndex)  return;
    
  mPlug->SetParameter(idx);
}

long IPlugProcess::SetControlValue(long aControlIndex, long aValue)
{
  TRACE;
  CPluginControl *cc=dynamic_cast<CPluginControl*>(GetControl(aControlIndex));
    
  if (cc)
    cc->SetValue(aValue);
  
  return (long)CProcess::SetControlValue(aControlIndex, aValue);
}
 
long IPlugProcess::GetControlValue(long aControlIndex, long *aValue)
{  
  return (long)CProcess::GetControlValue(aControlIndex, aValue);
}

long IPlugProcess::GetControlDefaultValue(long aControlIndex, long* aValue)
{
  return (long)CProcess::GetControlDefaultValue(aControlIndex, aValue);
}

int IPlugProcess::ProcessTouchControl (long aControlIndex)
{
  TRACE;
  return (int)CProcess::TouchControl(aControlIndex);
}

int IPlugProcess::ProcessReleaseControl (long aControlIndex)
{
  TRACE;
  return (int)CProcess::ReleaseControl(aControlIndex);
}

void IPlugProcess::ProcessDoIdle()
{
  this->DoIdle(NULL);
  gProcessGroup->YieldCriticalSection();
}

ComponentResult IPlugProcess::SetControlHighliteInfo(long controlIndex, short isHighlighted, short color)
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
  mPlug->SetSideChainConnected(true);
}

void IPlugProcess::DisconnectSidechain(void)
{
  CEffectProcess::DisconnectSidechain();
  mPlug->SetSideChainConnected(false);
}

ComponentResult IPlugProcess::GetChunkSize(OSType chunkID, long *size)
{
  TRACE;
  
  if (chunkID == mPluginID) {
    *size = sizeof(SFicPlugInChunkHeader);
    ByteChunk IPlugChunk;
    
    if (mPlug->SerializeState(&IPlugChunk))
      *size = IPlugChunk.Size() + sizeof(SFicPlugInChunkHeader);
    
    return noErr;
  }
  else 
    return CEffectProcess::GetChunkSize(chunkID, size); // Not our chunk
}

ComponentResult IPlugProcess::SetChunk(OSType chunkID, SFicPlugInChunk *chunk)
{
  TRACE;
  
  //called when project is loaded from save
  if (chunkID == mPluginID) {
    const int dataSize = chunk->fSize - sizeof(SFicPlugInChunkHeader);
    
    ByteChunk IPlugChunk;
    IPlugChunk.PutBytes(chunk->fData, dataSize);  
    mPlug->UnserializeState(&IPlugChunk, 0);
    return noErr;
  }
  else 
    return CEffectProcess::SetChunk(chunkID, chunk); // Not our chunk
}

ComponentResult IPlugProcess::GetChunk(OSType chunkID, SFicPlugInChunk *chunk)
{
  TRACE;
  
  //called when project is saved
  if (chunkID == mPluginID) {
    ByteChunk IPlugChunk;
    if (mPlug->SerializeState(&IPlugChunk)) {
      chunk->fSize = IPlugChunk.Size() + sizeof(SFicPlugInChunkHeader);
      memcpy(chunk->fData, IPlugChunk.GetBytes(), IPlugChunk.Size());
      return noErr;
    }
    return 1;
  }
  else 
    return CEffectProcess::GetChunk(chunkID, chunk); // Not our chunk
}

void IPlugProcess::ResizeGraphics(int w, int h)
{
  mCustomUI->Close();
  mPlug->OnWindowResize();
  SSetProcessWindowResizeToken theToken(fRootNameId, fRootNameId);
  FicSDSDispatchToken (&theToken);
}

int IPlugProcess::GetHostVersion()
{
  //0xVVRM: PT
  //0xVVVVRRMM: IPLUG
  
  short version = FicGetVersion();
  
  int pVer = version & 0xFF00;
  int pMaj = version & 0x00F0;
  int pMin = version & 0x000F;
  
  return ((pVer << 4) * 10) + (pMaj << 4) + (pMin << 4);
}
