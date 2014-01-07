#include "IPlugVST.h"
#include "IGraphics.h"
#include <stdio.h>

const int VST_VERSION = 2400;

int VSTSpkrArrType(int nchan)
{
  if (!nchan) return kSpeakerArrEmpty;
  if (nchan == 1) return kSpeakerArrMono;
  if (nchan == 2) return kSpeakerArrStereo;
  return kSpeakerArrUserDefined;
}

IPlugVST::IPlugVST(IPlugInstanceInfo instanceInfo,
                   int nParams,
                   const char* channelIOStr,
                   int nPresets,
                   const char* effectName,
                   const char* productName,
                   const char* mfrName,
                   int vendorVersion,
                   int uniqueID,
                   int mfrID,
                   int latency,
                   bool plugDoesMidi,
                   bool plugDoesChunks,
                   bool plugIsInst,
                   int plugScChans)
  : IPlugBase(nParams,
              channelIOStr,
              nPresets,
              effectName,
              productName,
              mfrName,
              vendorVersion,
              uniqueID,
              mfrID,
              latency,
              plugDoesMidi,
              plugDoesChunks,
              plugIsInst,
              kAPIVST2)
  , mHostCallback(instanceInfo.mVSTHostCallback)
  , mHostSpecificInitDone(false)
{
  Trace(TRACELOC, "%s", effectName);

  mHasVSTExtensions = VSTEXT_NONE;

  int nInputs = NInChannels(), nOutputs = NOutChannels();

  memset(&mAEffect, 0, sizeof(AEffect));
  mAEffect.object = this;
  mAEffect.magic = kEffectMagic;
  mAEffect.dispatcher = VSTDispatcher;
  mAEffect.getParameter = VSTGetParameter;
  mAEffect.setParameter = VSTSetParameter;
  mAEffect.numPrograms = nPresets;
  mAEffect.numParams = nParams;
  mAEffect.numInputs = nInputs;
  mAEffect.numOutputs = nOutputs;
  mAEffect.uniqueID = uniqueID;
  mAEffect.version = GetEffectVersion(true);
  mAEffect.__ioRatioDeprecated = 1.0f;
  mAEffect.__processDeprecated = VSTProcess;
  mAEffect.processReplacing = VSTProcessReplacing;
  mAEffect.processDoubleReplacing = VSTProcessDoubleReplacing;
  mAEffect.initialDelay = latency;
  mAEffect.flags = effFlagsCanReplacing | effFlagsCanDoubleReplacing;
  
  if (plugDoesChunks) { mAEffect.flags |= effFlagsProgramChunks; }
  if (LegalIO(1, -1)) { mAEffect.flags |= __effFlagsCanMonoDeprecated; }
  if (plugIsInst) { mAEffect.flags |= effFlagsIsSynth; }

  memset(&mEditRect, 0, sizeof(ERect));
  memset(&mInputSpkrArr, 0, sizeof(VstSpeakerArrangement));
  memset(&mOutputSpkrArr, 0, sizeof(VstSpeakerArrangement));
  mInputSpkrArr.numChannels = nInputs;
  mOutputSpkrArr.numChannels = nOutputs;
  mInputSpkrArr.type = VSTSpkrArrType(nInputs);
  mOutputSpkrArr.type = VSTSpkrArrType(nOutputs);

  // Default everything to connected, then disconnect pins if the host says to.
  SetInputChannelConnections(0, nInputs, true);
  SetOutputChannelConnections(0, nOutputs, true);

  SetBlockSize(DEFAULT_BLOCK_SIZE);
}

void IPlugVST::BeginInformHostOfParamChange(int idx)
{
  mHostCallback(&mAEffect, audioMasterBeginEdit, idx, 0, 0, 0.0f);
}

void IPlugVST::InformHostOfParamChange(int idx, double normalizedValue)
{
  mHostCallback(&mAEffect, audioMasterAutomate, idx, 0, 0, (float) normalizedValue);
}

void IPlugVST::EndInformHostOfParamChange(int idx)
{
  mHostCallback(&mAEffect, audioMasterEndEdit, idx, 0, 0, 0.0f);
}

void IPlugVST::InformHostOfProgramChange()
{
  mHostCallback(&mAEffect, audioMasterUpdateDisplay, 0, 0, 0, 0.0f);
}

inline VstTimeInfo* GetTimeInfo(audioMasterCallback hostCallback, AEffect* pAEffect, int filter = 0)
{
#pragma warning(disable:4312) // Pointer size cast mismatch.
  VstTimeInfo* pTI = (VstTimeInfo*) hostCallback(pAEffect, audioMasterGetTime, 0, filter, 0, 0);
#pragma warning(default:4312)
  if (pTI && (!filter || (pTI->flags & filter)))
  {
    return pTI;
  }
  return 0;
}

int IPlugVST::GetSamplePos()
{
  VstTimeInfo* pTI = GetTimeInfo(mHostCallback, &mAEffect);
  if (pTI && pTI->samplePos >= 0.0)
  {
    return int(pTI->samplePos + 0.5);
  }
  return 0;
}

double IPlugVST::GetTempo()
{
  if (mHostCallback)
  {
    VstTimeInfo* pTI = GetTimeInfo(mHostCallback, &mAEffect, kVstTempoValid);
    if (pTI && pTI->tempo >= 0.0)
    {
      return pTI->tempo;
    }
  }
  return 0.0;
}

void IPlugVST::GetTimeSig(int* pNum, int* pDenom)
{
  *pNum = *pDenom = 0;
  VstTimeInfo* pTI = GetTimeInfo(mHostCallback, &mAEffect, kVstTimeSigValid);
  if (pTI && pTI->timeSigNumerator >= 0.0 && pTI->timeSigDenominator >= 0.0)
  {
    *pNum = pTI->timeSigNumerator;
    *pDenom = pTI->timeSigDenominator;
  }
}

void IPlugVST::GetTime(ITimeInfo* pTimeInfo)
{
  VstTimeInfo* pTI = GetTimeInfo(mHostCallback,
                                 &mAEffect,
                                 kVstPpqPosValid |
                                 kVstTempoValid |
                                 kVstBarsValid |
                                 kVstCyclePosValid |
                                 kVstTimeSigValid );

  if (pTI)
  {
    pTimeInfo->mSamplePos = pTI->samplePos;

    if ((pTI->flags & kVstPpqPosValid) && pTI->ppqPos >= 0.0) pTimeInfo->mPPQPos = pTI->ppqPos;
    if ((pTI->flags & kVstTempoValid) && pTI->tempo > 0.0) pTimeInfo->mTempo = pTI->tempo;
    if ((pTI->flags & kVstBarsValid) && pTI->barStartPos >= 0.0) pTimeInfo->mLastBar = pTI->barStartPos;
    if ((pTI->flags & kVstCyclePosValid) && pTI->cycleStartPos >= 0.0 && pTI->cycleEndPos >= 0.0)
    {
      pTimeInfo->mCycleStart = pTI->cycleStartPos;
      pTimeInfo->mCycleEnd = pTI->cycleEndPos;
    }
    if ((pTI->flags & kVstTimeSigValid) && pTI->timeSigNumerator > 0.0 && pTI->timeSigDenominator > 0.0)
    {
      pTimeInfo->mNumerator = pTI->timeSigNumerator;
      pTimeInfo->mDenominator = pTI->timeSigDenominator;
    }
    pTimeInfo->mTransportIsRunning = pTI->flags & kVstTransportPlaying;
    pTimeInfo->mTransportLoopEnabled = pTI->flags & kVstTransportCycleActive;
  }
}

EHost IPlugVST::GetHost()
{
  EHost host = IPlugBase::GetHost();

  if (host == kHostUninit)
  {
    char productStr[256];
    productStr[0] = '\0';
    int version = 0;
    mHostCallback(&mAEffect, audioMasterGetProductString, 0, 0, productStr, 0.0f);

    if (CSTR_NOT_EMPTY(productStr))
    {
      int decVer = mHostCallback(&mAEffect, audioMasterGetVendorVersion, 0, 0, 0, 0.0f);
      int ver = decVer / 10000;
      int rmaj = (decVer - 10000 * ver) / 100;
      int rmin = (decVer - 10000 * ver - 100 * rmaj);
      version = (ver << 16) + (rmaj << 8) + rmin;
    }

    SetHost(productStr, version);
    host = IPlugBase::GetHost();
  }

  return host;
}

void IPlugVST::AttachGraphics(IGraphics* pGraphics)
{
  if (pGraphics)
  {
    IPlugBase::AttachGraphics(pGraphics);
    mAEffect.flags |= effFlagsHasEditor;
    mEditRect.left = mEditRect.top = 0;
    mEditRect.right = pGraphics->Width();
    mEditRect.bottom = pGraphics->Height();
  }
}

void IPlugVST::ResizeGraphics(int w, int h)
{
  IGraphics* pGraphics = GetGUI();

  if (pGraphics)
  {
    mEditRect.left = mEditRect.top = 0;
    mEditRect.right = pGraphics->Width();
    mEditRect.bottom = pGraphics->Height();

    OnWindowResize();
  }
}

bool IPlugVST::IsRenderingOffline()
{
  return mHostCallback(&mAEffect, audioMasterGetCurrentProcessLevel, 0, 0, 0, 0.0f) == kVstProcessLevelOffline;
}

void IPlugVST::SetLatency(int samples)
{
  mAEffect.initialDelay = samples;
  IPlugBase::SetLatency(samples);
}

bool IPlugVST::SendVSTEvent(VstEvent* pEvent)
{
  // It would be more efficient to bundle these and send at the end of a processed block,
  // but that would require writing OnBlockEnd and making sure it always gets called,
  // and who cares anyway, midi events aren't that dense.
  VstEvents events;
  memset(&events, 0, sizeof(VstEvents));
  events.numEvents = 1;
  events.events[0] = pEvent;
  return (mHostCallback(&mAEffect, audioMasterProcessEvents, 0, 0, &events, 0.0f) == 1);
}

bool IPlugVST::SendMidiMsg(IMidiMsg* pMsg)
{
  VstMidiEvent midiEvent;
  memset(&midiEvent, 0, sizeof(VstMidiEvent));

  midiEvent.type = kVstMidiType;
  midiEvent.byteSize = sizeof(VstMidiEvent);  // Should this be smaller?
  midiEvent.deltaFrames = pMsg->mOffset;
  midiEvent.midiData[0] = pMsg->mStatus;
  midiEvent.midiData[1] = pMsg->mData1;
  midiEvent.midiData[2] = pMsg->mData2;

  return SendVSTEvent((VstEvent*) &midiEvent);
}

bool IPlugVST::SendSysEx(ISysEx* pSysEx)
{ 
  VstMidiSysexEvent sysexEvent;
  memset(&sysexEvent, 0, sizeof(VstMidiSysexEvent));

  sysexEvent.type = kVstSysExType;
  sysexEvent.byteSize = sizeof(VstMidiSysexEvent);
  sysexEvent.deltaFrames = pSysEx->mOffset;
  sysexEvent.dumpBytes = pSysEx->mSize;
  sysexEvent.sysexDump = (char*)pSysEx->mData;

  return SendVSTEvent((VstEvent*) &sysexEvent);
}

audioMasterCallback IPlugVST::GetHostCallback()
{
  return mHostCallback;
}

void IPlugVST::HostSpecificInit()
{
  if (!mHostSpecificInitDone)
  {
    mHostSpecificInitDone = true;
    EHost host = GetHost();
    switch (host)
    {
      case kHostAudition:
      case kHostOrion:
      case kHostForte:
      case kHostSAWStudio:
        LimitToStereoIO();
        break;
      default:
        break;
    }

    // This won't always solve a picky host problem -- for example Forte
    // looks at mAEffect IO count before identifying itself.
    mAEffect.numInputs = mInputSpkrArr.numChannels = NInChannels();
    mAEffect.numOutputs = mOutputSpkrArr.numChannels = NOutChannels();

    OnHostIdentified();
  }
}

VstIntPtr VSTCALLBACK IPlugVST::VSTDispatcher(AEffect *pEffect, VstInt32 opCode, VstInt32 idx, VstIntPtr value, void *ptr, float opt)
{
  // VSTDispatcher is an IPlugVST class member, we can access anything in IPlugVST from here.
  IPlugVST* _this = (IPlugVST*) pEffect->object;
  if (!_this)
  {
    return 0;
  }
  IPlugBase::IMutexLock lock(_this);

  // Handle a couple of opcodes here to make debugging easier.
  switch (opCode)
  {
    case effEditIdle:
    case __effIdleDeprecated:
    #ifdef USE_IDLE_CALLS
    _this->OnIdle();
    #endif
    return 0;
  }

  Trace(TRACELOC, "%d(%s):%d:%d", opCode, VSTOpcodeStr(opCode), idx, (int) value);

  switch (opCode)
  {
    case effOpen:
    {
      _this->HostSpecificInit();
      _this->OnParamReset();
      return 0;
    }
    case effClose:
    {
      lock.Destroy();
      DELETE_NULL(_this);
      return 0;
    }
    case effGetParamLabel:
    {
      if (idx >= 0 && idx < _this->NParams())
      {
        strcpy((char*) ptr, _this->GetParam(idx)->GetLabelForHost());
      }
      return 0;
    }
    case effGetParamDisplay:
    {
      if (idx >= 0 && idx < _this->NParams())
      {
        _this->GetParam(idx)->GetDisplayForHost((char*) ptr);
      }
      return 0;
    }
    case effGetParamName:
    {
      if (idx >= 0 && idx < _this->NParams())
      {
        strcpy((char*) ptr, _this->GetParam(idx)->GetNameForHost());
      }
      return 0;
    }
      //could implement effGetParameterProperties to group parameters, but can't find a host that supports it
//    case effGetParameterProperties:
//    {
//      if (idx >= 0 && idx < _this->NParams())
//      {
//        VstParameterProperties* props = (VstParameterProperties*) ptr;
//        
//        props->flags = kVstParameterSupportsDisplayCategory;
//        props->category = idx+1;
//        props->numParametersInCategory = 1;
//        strcpy(props->categoryLabel, "test");
//      }
//      return 1;
//    }
    case effString2Parameter:
    {
      if (idx >= 0 && idx < _this->NParams())
      {
        if (ptr)
        {
          double v;
          IParam* pParam = _this->GetParam(idx);
          if (pParam->GetNDisplayTexts())
          {
            int vi;
            if (!pParam->MapDisplayText((char*)ptr, &vi)) return 0;
            v = (double)vi;
          }
          else
          {
            v = atof((char*)ptr);
            if (pParam->DisplayIsNegated()) v = -v;
          }
          if (_this->GetGUI()) _this->GetGUI()->SetParameterFromPlug(idx, v, false);
          pParam->Set(v);
          _this->OnParamChange(idx);
        }
        return 1;
      }
      return 0;
    }
    case effSetSampleRate:
    {
      _this->SetSampleRate(opt);
      _this->Reset();
      return 0;
    }
    case effSetBlockSize:
    {
      _this->SetBlockSize(value);
      _this->Reset();
      return 0;
    }
    case effMainsChanged:
    {
      if (!value)
      {
        _this->OnActivate(false);
        _this->Reset();
      }
      else
      {
        _this->OnActivate(true);
      }
      return 0;
    }
    case effEditGetRect:
    {
      if (ptr && _this->GetGUI())
      {
        *(ERect**) ptr = &(_this->mEditRect);
        return 1;
      }
      ptr = 0;
      return 0;
    }
    case effEditOpen:
    {
      IGraphics* pGraphics = _this->GetGUI();
      
      if (pGraphics)
      {
        #ifdef _WIN32
          if (!pGraphics->OpenWindow(ptr)) pGraphics=0;
        #else   // OSX, check if we are in a Cocoa VST host
          #if defined(__LP64__)
          if (!pGraphics->OpenWindow(ptr)) pGraphics=0;
          #else
          bool iscocoa = (_this->mHasVSTExtensions&VSTEXT_COCOA);
          if (iscocoa && !pGraphics->OpenWindow(ptr)) pGraphics=0;
          if (!iscocoa && !pGraphics->OpenWindow(ptr, 0)) pGraphics=0;
          #endif
        #endif
        if (pGraphics)
        {
          _this->OnGUIOpen();
          return 1;
        }
      }
      return 0;
    }
    case effEditClose:
    {
      if (_this->GetGUI())
      {
        _this->OnGUIClose();
        _this->GetGUI()->CloseWindow();
        return 1;
      }
      return 0;
    }
    case __effIdentifyDeprecated:
    {
      return 'NvEf';  // Random deprecated magic.
    }
    case effGetChunk:
    {
      BYTE** ppData = (BYTE**) ptr;
      if (ppData)
      {
        bool isBank = (!idx);
        ByteChunk* pChunk = (isBank ? &(_this->mBankState) : &(_this->mState));
        _this->InitChunkWithIPlugVer(pChunk);
        bool savedOK = true;
        
        if (isBank)
        {
          _this->ModifyCurrentPreset();
          savedOK = _this->SerializePresets(pChunk);
        }
        else
        {
          savedOK = _this->SerializeState(pChunk);
        }
        
        if (savedOK && pChunk->Size())
        {
          *ppData = pChunk->GetBytes();
          return pChunk->Size();
        }
      }
      return 0;
    }
    case effSetChunk:
    {
      if (ptr)
      {
        bool isBank = (!idx);
        ByteChunk* pChunk = (isBank ? &(_this->mBankState) : &(_this->mState));
        pChunk->Resize(value);
        memcpy(pChunk->GetBytes(), ptr, value);
        int pos = 0;
        int iplugVer = _this->GetIPlugVerFromChunk(pChunk, &pos);
        isBank &= (iplugVer >= 0x010000);
        
        if (isBank)
        {
          pos = _this->UnserializePresets(pChunk, pos);
        }
        else
        {
          pos = _this->UnserializeState(pChunk, pos);
          _this->ModifyCurrentPreset();
        }
        
        if (pos >= 0)
        {
          _this->RedrawParamControls();
          return 1;
        }
      }
      return 0;
    }
    case effProcessEvents:
    {
      VstEvents* pEvents = (VstEvents*) ptr;
      if (pEvents && pEvents->events)
      {
        for (int i = 0; i < pEvents->numEvents; ++i)
        {
          VstEvent* pEvent = pEvents->events[i];
          if (pEvent)
          {
            if (pEvent->type == kVstMidiType)
            {
              VstMidiEvent* pME = (VstMidiEvent*) pEvent;
              IMidiMsg msg(pME->deltaFrames, pME->midiData[0], pME->midiData[1], pME->midiData[2]);
              _this->ProcessMidiMsg(&msg);
              //#ifdef TRACER_BUILD
              //  msg.LogMsg();
              //#endif
            }
            else if (pEvent->type == kVstSysExType) 
            {
              VstMidiSysexEvent* pSE = (VstMidiSysexEvent*) pEvent;
              ISysEx sysex(pSE->deltaFrames, (const BYTE*)pSE->sysexDump, pSE->dumpBytes);
              _this->ProcessSysEx(&sysex);
            }
          }
        }
        return 1;
      }
      return 0;
    }
    case effCanBeAutomated:
    {
      return 1;
    }
    case effGetInputProperties:
    {
      if (ptr && idx >= 0 && idx < _this->NInChannels())
      {
        VstPinProperties* pp = (VstPinProperties*) ptr;
        pp->flags = kVstPinIsActive;
        if (!(idx%2) && idx < _this->NInChannels()-1)
        {
          pp->flags |= kVstPinIsStereo;
        }

        if (_this->GetInputLabel(idx)->GetLength())
        {
          sprintf(pp->label, "%s", _this->GetInputLabel(idx)->Get());
        }
        else
        {
          sprintf(pp->label, "Input %d", idx + 1);
        }

        return 1;
      }
      return 0;
    }
    case effGetOutputProperties:
    {
      if (ptr && idx >= 0 && idx < _this->NOutChannels())
      {
        VstPinProperties* pp = (VstPinProperties*) ptr;
        pp->flags = kVstPinIsActive;
        if (!(idx%2) && idx < _this->NOutChannels()-1)
        {
          pp->flags |= kVstPinIsStereo;
        }

        if (_this->GetOutputLabel(idx)->GetLength())
        {
          sprintf(pp->label, "%s", _this->GetOutputLabel(idx)->Get());
        }
        else
        {
          sprintf(pp->label, "Output %d", idx + 1);
        }

        return 1;
      }
      return 0;
    }
    case effGetPlugCategory:
    {
      if (_this->IsInst()) return kPlugCategSynth;
      return kPlugCategEffect;
    }
    case effProcessVarIo:
    {
      // VstVariableIo* pIO = (VstVariableIo*) ptr; // For offline processing (of audio files?)
      return 0;
    }
    case effSetSpeakerArrangement:
    {
      VstSpeakerArrangement* pInputArr = (VstSpeakerArrangement*) value;
      VstSpeakerArrangement* pOutputArr = (VstSpeakerArrangement*) ptr;
      if (pInputArr)
      {
        int n = pInputArr->numChannels;
        _this->SetInputChannelConnections(0, n, true);
        _this->SetInputChannelConnections(n, _this->NInChannels() - n, false);
      }
      if (pOutputArr)
      {
        int n = pOutputArr->numChannels;
        _this->SetOutputChannelConnections(0, n, true);
        _this->SetOutputChannelConnections(n, _this->NOutChannels() - n, false);
      }
      return 1;
    }
    case effGetSpeakerArrangement:
    {
      VstSpeakerArrangement** ppInputArr = (VstSpeakerArrangement**) value;
      VstSpeakerArrangement** ppOutputArr = (VstSpeakerArrangement**) ptr;
      if (ppInputArr)
      {
        *ppInputArr = &(_this->mInputSpkrArr);
      }
      if (ppOutputArr)
      {
        *ppOutputArr = &(_this->mOutputSpkrArr);
      }
      return 1;
    }
    case effGetEffectName:
    {
      if (ptr)
      {
        strcpy((char*) ptr, _this->GetEffectName());
        return 1;
      }
      return 0;
    }
    case effGetProductString:
    {
      if (ptr)
      {
        strcpy((char*) ptr, _this->GetProductName());
        return 1;
      }
      return 0;
    }
    case effGetVendorString:
    {
      if (ptr)
      {
        strcpy((char*) ptr, _this->GetMfrName());
        return 1;
      }
      return 0;
    }
    case effCanDo:
    {
      if (ptr)
      {
        Trace(TRACELOC, "VSTCanDo(%s)", (char*) ptr);
        if (!strcmp((char*) ptr, "receiveVstTimeInfo"))
        {
          return 1;
        }
        if (_this->DoesMIDI())
        {
          if (!strcmp((char*) ptr, "sendVstEvents") ||
              !strcmp((char*) ptr, "sendVstMidiEvent") ||
              !strcmp((char*) ptr, "receiveVstEvents") ||
              !strcmp((char*) ptr, "receiveVstMidiEvent"))   // ||
          {
            //!strcmp((char*) ptr, "midiProgramNames")) {
            return 1;
          }
        }
        // Support Reaper VST extensions: http://www.reaper.fm/sdk/vst/
        if (!strcmp((char*) ptr, "hasCockosExtensions"))
        {
          _this->mHasVSTExtensions |= VSTEXT_COCKOS;
          return 0xbeef0000;
        }
        else if (!strcmp((char*) ptr, "hasCockosViewAsConfig"))
        {
          _this->mHasVSTExtensions |= VSTEXT_COCOA;
          return 0xbeef0000;
        }
      }
      return 0;
    }
    case effVendorSpecific:
    {
      // Support Reaper VST extensions: http://www.reaper.fm/sdk/vst/
      if (idx == effGetParamDisplay && ptr)
      {
        if (value >= 0 && value < _this->NParams())
        {
          _this->GetParam(value)->GetDisplayForHost((double) opt, true, (char*) ptr);
        }
        return 0xbeef;
      }

      if (idx == kVstParameterUsesIntStep)
      {
        if (value >= 0 && value < _this->NParams())
        {
          if (_this->GetParam(value)->Type() != IParam::kTypeDouble)
          {
            return 0xbeef;
          }
        }
      }

      return 0;
    }
    case effGetProgram:
    {
      return _this->GetCurrentPresetIdx();
    }
    case effSetProgram:
    {
      if (_this->DoesStateChunks() == false)
      {
        _this->ModifyCurrentPreset(); // TODO: test, something is funny about this http://forum.cockos.com/showpost.php?p=485113&postcount=22
      }
      _this->RestorePreset((int) value);
      return 0;
    }
    case effGetProgramNameIndexed:
    {
      strcpy((char*) ptr, _this->GetPresetName(idx));
      return (CSTR_NOT_EMPTY((char*) ptr) ? 1 : 0);
    }
    case effSetProgramName:
    {
      if (ptr)
      {
        _this->ModifyCurrentPreset((char*) ptr);
        _this->PresetsChangedByHost();
      }
      return 0;
    }
    case effGetProgramName:
    {
      if (ptr)
      {
        int idx = _this->GetCurrentPresetIdx();
        strcpy((char*) ptr, _this->GetPresetName(idx));
      }
      return 0;
    }
    case effGetMidiKeyName:
    {
      if (ptr)
      {
        MidiKeyName* pMKN = (MidiKeyName*) ptr;
        pMKN->keyName[0] = '\0';
        if (_this->MidiNoteName(pMKN->thisKeyNumber, pMKN->keyName))
        {
          return 1;
        }
      }
      return 0;
    }
    case effGetVstVersion:
    {
      return VST_VERSION;
    }
    case effEndSetProgram:
    case effBeginSetProgram:
    case effGetMidiProgramName:
    case effHasMidiProgramsChanged:
    case effGetMidiProgramCategory:
    case effGetCurrentMidiProgram:
    case effSetBypass:
    default:
    {
      return 0;
    }
  }
}

template <class SAMPLETYPE>
void IPlugVST::VSTPrepProcess(SAMPLETYPE** inputs, SAMPLETYPE** outputs, VstInt32 nFrames)
{
  if (DoesMIDI())
  {
    mHostCallback(&mAEffect, __audioMasterWantMidiDeprecated, 0, 0, 0, 0.0f);
  }
  AttachInputBuffers(0, NInChannels(), inputs, nFrames);
  AttachOutputBuffers(0, NOutChannels(), outputs);
}

// Deprecated.
void VSTCALLBACK IPlugVST::VSTProcess(AEffect* pEffect, float** inputs, float** outputs, VstInt32 nFrames)
{
  TRACE_PROCESS;
  IPlugVST* _this = (IPlugVST*) pEffect->object;
  IMutexLock lock(_this);
  _this->VSTPrepProcess(inputs, outputs, nFrames);
  _this->ProcessBuffersAccumulating((float) 0.0f, nFrames);
}

void VSTCALLBACK IPlugVST::VSTProcessReplacing(AEffect* pEffect, float** inputs, float** outputs, VstInt32 nFrames)
{
  TRACE_PROCESS;
  IPlugVST* _this = (IPlugVST*) pEffect->object;
  IMutexLock lock(_this);
  _this->VSTPrepProcess(inputs, outputs, nFrames);
  _this->ProcessBuffers((float) 0.0f, nFrames);
}

void VSTCALLBACK IPlugVST::VSTProcessDoubleReplacing(AEffect* pEffect, double** inputs, double** outputs, VstInt32 nFrames)
{
  TRACE_PROCESS;
  IPlugVST* _this = (IPlugVST*) pEffect->object;
  IMutexLock lock(_this);
  _this->VSTPrepProcess(inputs, outputs, nFrames);
  _this->ProcessBuffers((double) 0.0, nFrames);
}

float VSTCALLBACK IPlugVST::VSTGetParameter(AEffect *pEffect, VstInt32 idx)
{
  Trace(TRACELOC, "%d", idx);
  IPlugVST* _this = (IPlugVST*) pEffect->object;
  IMutexLock lock(_this);
  if (idx >= 0 && idx < _this->NParams())
  {
    return (float) _this->GetParam(idx)->GetNormalized();
  }
  return 0.0f;
}

void VSTCALLBACK IPlugVST::VSTSetParameter(AEffect *pEffect, VstInt32 idx, float value)
{
  Trace(TRACELOC, "%d:%f", idx, value);
  IPlugVST* _this = (IPlugVST*) pEffect->object;
  IMutexLock lock(_this);
  if (idx >= 0 && idx < _this->NParams())
  {
    if (_this->GetGUI())
    {
      _this->GetGUI()->SetParameterFromPlug(idx, value, true);
    }
    _this->GetParam(idx)->SetNormalized(value);
    _this->OnParamChange(idx);
  }
}
