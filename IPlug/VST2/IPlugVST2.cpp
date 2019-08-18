/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include <cstdio>
#include "IPlugVST2.h"
#include "IPlugPluginBase.h"

using namespace iplug;

static const int VST_VERSION = 2400;

static int VSTSpkrArrType(int nchan)
{
  if (!nchan) return kSpeakerArrEmpty;
  if (nchan == 1) return kSpeakerArrMono;
  if (nchan == 2) return kSpeakerArrStereo;
  return kSpeakerArrUserDefined;
}

IPlugVST2::IPlugVST2(const InstanceInfo& info, const Config& config)
  : IPlugAPIBase(config, kAPIVST2)
  , IPlugProcessor(config, kAPIVST2)
  , mHostCallback(info.mVSTHostCallback)
{
  Trace(TRACELOC, "%s", config.pluginName);

  mHasVSTExtensions = VSTEXT_NONE;

  int nInputs = MaxNChannels(ERoute::kInput), nOutputs = MaxNChannels(ERoute::kOutput);

  memset(&mAEffect, 0, sizeof(AEffect));
  mAEffect.object = this;
  mAEffect.magic = kEffectMagic;
  mAEffect.dispatcher = VSTDispatcher;
  mAEffect.getParameter = VSTGetParameter;
  mAEffect.setParameter = VSTSetParameter;
  mAEffect.numPrograms = config.nPresets;
  mAEffect.numParams = config.nParams;
  mAEffect.numInputs = nInputs;
  mAEffect.numOutputs = nOutputs;
  mAEffect.uniqueID = config.uniqueID;
  mAEffect.version = GetPluginVersion(true);
  mAEffect.__ioRatioDeprecated = 1.0f;
  mAEffect.__processDeprecated = VSTProcess;
  mAEffect.processReplacing = VSTProcessReplacing;
  mAEffect.processDoubleReplacing = VSTProcessDoubleReplacing;
  mAEffect.initialDelay = config.latency;
  mAEffect.flags = effFlagsCanReplacing | effFlagsCanDoubleReplacing;

  if (config.plugDoesChunks) { mAEffect.flags |= effFlagsProgramChunks; }
  if (LegalIO(1, -1)) { mAEffect.flags |= __effFlagsCanMonoDeprecated; }
  if (config.plugType == EIPlugPluginType::kInstrument) { mAEffect.flags |= effFlagsIsSynth; }

  memset(&mEditRect, 0, sizeof(ERect));
  memset(&mInputSpkrArr, 0, sizeof(VstSpeakerArrangement));
  memset(&mOutputSpkrArr, 0, sizeof(VstSpeakerArrangement));
  mInputSpkrArr.numChannels = nInputs;
  mOutputSpkrArr.numChannels = nOutputs;
  mInputSpkrArr.type = VSTSpkrArrType(nInputs);
  mOutputSpkrArr.type = VSTSpkrArrType(nOutputs);

  // Default everything to connected, then disconnect pins if the host says to.
  SetChannelConnections(ERoute::kInput, 0, nInputs, true);
  SetChannelConnections(ERoute::kOutput, 0, nOutputs, true);

  SetBlockSize(DEFAULT_BLOCK_SIZE);

  if(config.plugHasUI)
  {
    mAEffect.flags |= effFlagsHasEditor;
    mEditRect.left = mEditRect.top = 0;
    mEditRect.right = config.plugWidth;
    mEditRect.bottom = config.plugHeight;
  }
  
  CreateTimer();
}

void IPlugVST2::BeginInformHostOfParamChange(int idx)
{
  mHostCallback(&mAEffect, audioMasterBeginEdit, idx, 0, 0, 0.0f);
}

void IPlugVST2::InformHostOfParamChange(int idx, double normalizedValue)
{
  mHostCallback(&mAEffect, audioMasterAutomate, idx, 0, 0, (float) normalizedValue);
}

void IPlugVST2::EndInformHostOfParamChange(int idx)
{
  mHostCallback(&mAEffect, audioMasterEndEdit, idx, 0, 0, 0.0f);
}

void IPlugVST2::InformHostOfProgramChange()
{
  mHostCallback(&mAEffect, audioMasterUpdateDisplay, 0, 0, 0, 0.0f);
}

bool IPlugVST2::EditorResizeFromDelegate(int viewWidth, int viewHeight)
{
  bool resized = false;

  if (HasUI())
  {
    if (viewWidth != GetEditorWidth() || viewHeight != GetEditorHeight())
    {
      mEditRect.left = mEditRect.top = 0;
      mEditRect.right = viewWidth;
      mEditRect.bottom = viewHeight;
    
      resized = mHostCallback(&mAEffect, audioMasterSizeWindow, viewWidth, viewHeight, 0, 0.f);
    }
    
    IPlugAPIBase::EditorResizeFromDelegate(viewWidth, viewHeight);
  }

  return resized;
}

void IPlugVST2::SetLatency(int samples)
{
  mAEffect.initialDelay = samples;
  IPlugProcessor::SetLatency(samples);
}

bool IPlugVST2::SendVSTEvent(VstEvent& event)
{
  // It would be more efficient to bundle these and send at the end of a processed block,
  // but that would require writing OnBlockEnd and making sure it always gets called,
  // and who cares anyway, midi events aren't that dense.
  VstEvents events;
  memset(&events, 0, sizeof(VstEvents));
  events.numEvents = 1;
  events.events[0] = &event;
  return (mHostCallback(&mAEffect, audioMasterProcessEvents, 0, 0, &events, 0.0f) == 1);
}

bool IPlugVST2::SendMidiMsg(const IMidiMsg& msg)
{
  VstMidiEvent midiEvent;
  memset(&midiEvent, 0, sizeof(VstMidiEvent));

  midiEvent.type = kVstMidiType;
  midiEvent.byteSize = sizeof(VstMidiEvent);  // Should this be smaller?
  midiEvent.deltaFrames = msg.mOffset;
  midiEvent.midiData[0] = msg.mStatus;
  midiEvent.midiData[1] = msg.mData1;
  midiEvent.midiData[2] = msg.mData2;

  return SendVSTEvent((VstEvent&) midiEvent);
}

bool IPlugVST2::SendSysEx(const ISysEx& msg)
{
  VstMidiSysexEvent sysexEvent;
  memset(&sysexEvent, 0, sizeof(VstMidiSysexEvent));

  sysexEvent.type = kVstSysExType;
  sysexEvent.byteSize = sizeof(VstMidiSysexEvent);
  sysexEvent.deltaFrames = msg.mOffset;
  sysexEvent.dumpBytes = msg.mSize;
  sysexEvent.sysexDump = (char*) msg.mData;

  return SendVSTEvent((VstEvent&) sysexEvent);
}

void IPlugVST2::HostSpecificInit()
{
  switch (GetHost())
  {
    case kHostAudition:
    case kHostOrion:
    case kHostForte:
    case kHostSAWStudio:
      LimitToStereoIO(); //TODO:  is this still necessary?
      break;
    default:
      break;
  }

  // This won't always solve a picky host problem -- for example Forte
  // looks at mAEffect IO count before identifying itself.
  mAEffect.numInputs = mInputSpkrArr.numChannels = MaxNChannels(ERoute::kInput);
  mAEffect.numOutputs = mOutputSpkrArr.numChannels = MaxNChannels(ERoute::kOutput);
}

VstIntPtr VSTCALLBACK IPlugVST2::VSTDispatcher(AEffect *pEffect, VstInt32 opCode, VstInt32 idx, VstIntPtr value, void *ptr, float opt)
{
  // VSTDispatcher is an IPlugVST class member, we can access anything in IPlugVST from here.
  IPlugVST2* _this = (IPlugVST2*) pEffect->object;
  if (!_this)
  {
    return 0;
  }

  // Handle a couple of opcodes here to make debugging easier.
//  switch (opCode)
//  {
//    case effEditIdle:
//    case __effIdleDeprecated:
//    #ifdef USE_IDLE_CALLS
//    _this->OnIdle();
//    #endif
//    return 0;
//  }

  Trace(TRACELOC, "%d(%s):%d:%d", opCode, VSTOpcodeStr(opCode), idx, (int) value);

  switch (opCode)
  {
    case effOpen:
    {
      if (_this->GetHost() == kHostUninit)
      {
        char productStr[256];
        productStr[0] = '\0';
        int version = 0;
        _this->mHostCallback(&_this->mAEffect, audioMasterGetProductString, 0, 0, productStr, 0.0f);
        
        if (CStringHasContents(productStr))
        {
          int decVer = (int) _this->mHostCallback(&_this->mAEffect, audioMasterGetVendorVersion, 0, 0, 0, 0.0f);
          int ver = decVer / 10000;
          int rmaj = (decVer - 10000 * ver) / 100;
          int rmin = (decVer - 10000 * ver - 100 * rmaj);
          version = (ver << 16) + (rmaj << 8) + rmin;
        }
        
        _this->SetHost(productStr, version);
      }
      _this->OnParamReset(kReset);
      return 0;
    }
    case effClose:
    {
      delete _this;
      return 0;
    }
    case effGetParamLabel:
    {
      if (idx >= 0 && idx < _this->NParams())
      {
        ENTER_PARAMS_MUTEX_STATIC;
        strcpy((char*) ptr, _this->GetParam(idx)->GetLabelForHost());
        LEAVE_PARAMS_MUTEX_STATIC;
      }
      return 0;
    }
    case effGetParamDisplay:
    {
      if (idx >= 0 && idx < _this->NParams())
      {
        ENTER_PARAMS_MUTEX_STATIC;
        _this->GetParam(idx)->GetDisplayForHost(_this->mParamDisplayStr);
        LEAVE_PARAMS_MUTEX_STATIC;
        strcpy((char*) ptr, _this->mParamDisplayStr.Get());
      }
      return 0;
    }
    case effGetParamName:
    {
      if (idx >= 0 && idx < _this->NParams())
      {
        ENTER_PARAMS_MUTEX_STATIC;
        strcpy((char*) ptr, _this->GetParam(idx)->GetNameForHost());
        LEAVE_PARAMS_MUTEX_STATIC;
      }
      return 0;
    }
    case effGetParameterProperties:
    {
      if (idx >= 0 && idx < _this->NParams())
      {
        VstParameterProperties* props = (VstParameterProperties*) ptr;

        ENTER_PARAMS_MUTEX_STATIC;
        IParam* pParam = _this->GetParam(idx);
        switch (pParam->Type())
        {
          case IParam::kTypeInt:
          case IParam::kTypeEnum:
            props->flags = kVstParameterUsesIntStep | kVstParameterUsesIntegerMinMax;
            props->minInteger = (int) pParam->GetMin();
            props->maxInteger = (int) pParam->GetMax();
            props->stepInteger = props->largeStepInteger = 1;
            break;
          case IParam::kTypeBool:
            props->flags = kVstParameterIsSwitch;
            break;
          case IParam::kTypeDouble:
          default:
            props->flags = kVstParameterUsesFloatStep;
            props->largeStepFloat = props->smallStepFloat = props->stepFloat = (float) pParam->GetStep();
            break;
        }

        strcpy(props->label, pParam->GetLabelForHost());
        LEAVE_PARAMS_MUTEX_STATIC;

        return 1;
      }
      return 0;
    }
    case effString2Parameter:
    {
      if (idx >= 0 && idx < _this->NParams())
      {
        if (ptr)
        {
          ENTER_PARAMS_MUTEX_STATIC;
          IParam* pParam = _this->GetParam(idx);
          const double v = pParam->StringToValue((const char *)ptr);
          pParam->Set(v);
          _this->SendParameterValueFromAPI(idx, v, false);
          _this->OnParamChange(idx, kHost);
          LEAVE_PARAMS_MUTEX_STATIC;
        }
        return 1;
      }
      return 0;
    }
    case effSetSampleRate:
    {
      _this->SetSampleRate(opt);
      _this->OnReset();
      return 0;
    }
    case effSetBlockSize:
    {
      _this->SetBlockSize((int) value);
      _this->OnReset();
      return 0;
    }
    case effMainsChanged:
    {
      if (!value)
      {
        _this->OnActivate(false);
        _this->OnReset();
      }
      else
      {
        _this->OnActivate(true);
      }
      return 0;
    }
    case effEditGetRect:
    {
      if (ptr && _this->HasUI())
      {
        *(ERect**) ptr = &(_this->mEditRect);
        return 1;
      }
      ptr = 0;
      return 0;
    }
    case effEditOpen:
    {
#if defined OS_WIN || defined ARCH_64BIT
      if (_this->OpenWindow(ptr))
      {
        return 1;
      }
#else   // OSX 32 bit, check if we are in a Cocoa VST host, otherwise tough luck
      bool iscocoa = (_this->mHasVSTExtensions&VSTEXT_COCOA);
      if (iscocoa && _this->OpenWindow(ptr))
      {
        return 1; // cocoa supported open cocoa
      }
#endif
      return 0;
    }
    case effEditClose:
    {
      if (_this->HasUI())
      {
        _this->CloseWindow();
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
      uint8_t** ppData = (uint8_t**) ptr;
      if (ppData)
      {
        bool isBank = (!idx);
        IByteChunk& chunk = (isBank ? _this->mBankState : _this->mState);
        IByteChunk::InitChunkWithIPlugVer(chunk);
        bool savedOK = true;

        if (isBank)
        {
          _this->ModifyCurrentPreset();
          savedOK = static_cast<IPluginBase*>(_this)->SerializePresets(chunk);
        }
        else
        {
          savedOK = _this->SerializeState(chunk);
        }

        if (savedOK && chunk.Size())
        {
          *ppData = chunk.GetData();
          return chunk.Size();
        }
      }
      return 0;
    }
    case effSetChunk:
    {
      if (ptr)
      {
        bool isBank = (!idx);
        IByteChunk& chunk = (isBank ? _this->mBankState : _this->mState);
        chunk.Resize((int) value);
        memcpy(chunk.GetData(), ptr, value);
        int pos = 0;
        int iplugVer = IByteChunk::GetIPlugVerFromChunk(chunk, pos);
        isBank &= (iplugVer >= 0x010000);

        if (isBank)
        {
          pos = static_cast<IPluginBase*>(_this)->UnserializePresets(chunk, pos);
        }
        else
        {
          pos = _this->UnserializeState(chunk, pos);
          _this->ModifyCurrentPreset();
        }

        if (pos >= 0)
        {
          _this->OnRestoreState();
          return 1;
        }
      }
      return 0;
    }
    case effProcessEvents:
    {
      VstEvents* pEvents = (VstEvents*) ptr;
      if (pEvents)
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
              _this->ProcessMidiMsg(msg);
              _this->mMidiMsgsFromProcessor.Push(msg);

              //#ifdef TRACER_BUILD
              //  msg.LogMsg();
              //#endif
            }
            else if (pEvent->type == kVstSysExType)
            {
              VstMidiSysexEvent* pSE = (VstMidiSysexEvent*) pEvent;
              ISysEx sysex(pSE->deltaFrames, (const uint8_t*)pSE->sysexDump, pSE->dumpBytes);
              _this->ProcessSysEx(sysex);
            }
          }
        }
        return 1;
      }
      return 0;
    }
    case effCanBeAutomated:
    {
      if (idx >= 0 && idx < _this->NParams())
      {
        return _this->GetParam(idx)->GetCanAutomate();
      }
    }
    case effGetInputProperties:
    {
      if (ptr && idx >= 0 && idx < _this->MaxNChannels(ERoute::kInput))
      {
        VstPinProperties* pp = (VstPinProperties*) ptr;
        pp->flags = kVstPinIsActive;

        if (!(idx%2) && idx < _this->MaxNChannels(ERoute::kInput)-1)
          pp->flags |= kVstPinIsStereo;

        if (_this->GetChannelLabel(ERoute::kInput, idx).GetLength())
          sprintf(pp->label, "%s", _this->GetChannelLabel(ERoute::kInput, idx).Get());
        else
          sprintf(pp->label, "Input %d", idx + 1);

        return 1;
      }
      return 0;
    }
    case effGetOutputProperties:
    {
      if (ptr && idx >= 0 && idx < _this->MaxNChannels(ERoute::kOutput))
      {
        VstPinProperties* pp = (VstPinProperties*) ptr;
        pp->flags = kVstPinIsActive;

        if (!(idx%2) && idx < _this->MaxNChannels(ERoute::kOutput)-1)
          pp->flags |= kVstPinIsStereo;

        if (_this->GetChannelLabel(ERoute::kOutput, idx).GetLength())
          sprintf(pp->label, "%s", _this->GetChannelLabel(ERoute::kOutput, idx).Get());
        else
          sprintf(pp->label, "Output %d", idx + 1);

        return 1;
      }
      return 0;
    }
    case effGetPlugCategory:
    {
      if (_this->IsInstrument()) return kPlugCategSynth;
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
        _this->SetChannelConnections(ERoute::kInput, 0, n, true);
        _this->SetChannelConnections(ERoute::kInput, n, _this->MaxNChannels(ERoute::kInput) - n, false);
      }
      if (pOutputArr)
      {
        int n = pOutputArr->numChannels;
        _this->SetChannelConnections(ERoute::kOutput, 0, n, true);
        _this->SetChannelConnections(ERoute::kOutput, n, _this->MaxNChannels(ERoute::kOutput) - n, false);
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
        strcpy((char*) ptr, _this->GetPluginName());
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
    case effGetVendorVersion:
    {
      return _this->GetPluginVersion(true);
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
        if (_this->DoesMIDIIn())
        {
          if (!strcmp((char*) ptr, "receiveVstEvents") ||
              !strcmp((char*) ptr, "receiveVstMidiEvent"))
          {
            return 1;
          }
        }
        if (_this->DoesMIDIOut())
        {
          if (!strcmp((char*) ptr, "sendVstEvents") ||
              !strcmp((char*) ptr, "sendVstMidiEvent"))
          {
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
        else if (!strcmp((char*) ptr, "wantsChannelCountNotifications"))
        {
          return 1;
        }
        
        return _this->VSTCanDo((char *) ptr);
      }
      return 0;
    }
    case effGetTailSize:
    {
      return _this->GetTailSize();
    }
    case effVendorSpecific:
    {
      switch (idx)
      {
          // Mouse wheel
//        case 0x73744341:
//        {
//          if (value == 0x57686565)
//          {
//            IGraphics* pGraphics = _this->GetUI();
//            if (pGraphics) {
//              return pGraphics->ProcessMouseWheel(opt);
//            }
//          }
//          break;
//        }
          // Support Reaper VST extensions: http://www.reaper.fm/sdk/vst/
        case effGetParamDisplay:
        {
          if (ptr)
          {
            if (value >= 0 && value < _this->NParams())
            {
              _this->GetParam((int) value)->GetDisplayForHost((double) opt, true, _this->mParamDisplayStr);
              strcpy((char*) ptr, _this->mParamDisplayStr.Get());
            }
            return 0xbeef;
          }
          break;
        }
        case effString2Parameter:
        {
          if (ptr && value >= 0 && value < _this->NParams())
          {
            if (*(char*) ptr != '\0')
            {
              IParam* pParam = _this->GetParam((int) value);
              sprintf((char*) ptr, "%.17f", pParam->ToNormalized(pParam->StringToValue((const char*) ptr)));
            }
            return 0xbeef;
          }
          break;
        }
        case kVstParameterUsesIntStep:
        {
          if (value >= 0 && value < _this->NParams())
          {
            IParam* pParam = _this->GetParam((int) value);
            switch (pParam->Type())
            {
              case IParam::kTypeBool:
                return 0xbeef;
              case IParam::kTypeInt:
              case IParam::kTypeEnum:
              {
                double min, max;
                pParam->GetBounds(min, max);
                if (std::fabs(max - min) < 1.5)
                  return 0xbeef;

                break;
              }
              default:
                break;
            }
          }
          break;
        }
      }
      return _this->VSTVendorSpecific(idx, value, ptr, opt);
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
      return (CStringHasContents((char*) ptr) ? 1 : 0);
    }
    case effSetProgramName:
    {
      if (ptr)
      {
        _this->ModifyCurrentPreset((char*) ptr);
        _this->OnPresetsModified();
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
        if (_this->GetMidiNoteText(pMKN->thisKeyNumber, pMKN->keyName))
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
void IPlugVST2::VSTPreProcess(SAMPLETYPE** inputs, SAMPLETYPE** outputs, VstInt32 nFrames)
{
  if (DoesMIDIIn())
    mHostCallback(&mAEffect, __audioMasterWantMidiDeprecated, 0, 0, 0, 0.0f);

  AttachBuffers(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), inputs, nFrames);
  AttachBuffers(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), outputs, nFrames);

  VstTimeInfo* pTI = (VstTimeInfo*) mHostCallback(&mAEffect, audioMasterGetTime, 0, kVstPpqPosValid | kVstTempoValid | kVstBarsValid | kVstCyclePosValid | kVstTimeSigValid, 0, 0);

  ITimeInfo timeInfo;

  if (pTI)
  {
    timeInfo.mSamplePos = pTI->samplePos;

    if ((pTI->flags & kVstPpqPosValid) && pTI->ppqPos >= 0.0) timeInfo.mPPQPos = pTI->ppqPos;
    if ((pTI->flags & kVstTempoValid) && pTI->tempo > 0.0) timeInfo.mTempo = pTI->tempo;
    if ((pTI->flags & kVstBarsValid) && pTI->barStartPos >= 0.0) timeInfo.mLastBar = pTI->barStartPos;
    if ((pTI->flags & kVstCyclePosValid) && pTI->cycleStartPos >= 0.0 && pTI->cycleEndPos >= 0.0)
    {
      timeInfo.mCycleStart = pTI->cycleStartPos;
      timeInfo.mCycleEnd = pTI->cycleEndPos;
    }
    if ((pTI->flags & kVstTimeSigValid) && pTI->timeSigNumerator > 0.0 && pTI->timeSigDenominator > 0.0)
    {
      timeInfo.mNumerator = pTI->timeSigNumerator;
      timeInfo.mDenominator = pTI->timeSigDenominator;
    }
    timeInfo.mTransportIsRunning = pTI->flags & kVstTransportPlaying;
    timeInfo.mTransportLoopEnabled = pTI->flags & kVstTransportCycleActive;
  }

  const bool renderingOffline = mHostCallback(&mAEffect, audioMasterGetCurrentProcessLevel, 0, 0, 0, 0.0f) == kVstProcessLevelOffline;

  SetTimeInfo(timeInfo);
  SetRenderingOffline(renderingOffline);

  IMidiMsg msg;

  while (mMidiMsgsFromEditor.Pop(msg))
  {
    ProcessMidiMsg(msg);
  }
}

// Deprecated.
void VSTCALLBACK IPlugVST2::VSTProcess(AEffect* pEffect, float** inputs, float** outputs, VstInt32 nFrames)
{
  TRACE;
  IPlugVST2* _this = (IPlugVST2*) pEffect->object;
  _this->VSTPreProcess(inputs, outputs, nFrames);
  _this->ProcessBuffersAccumulating(nFrames);
  _this->OutputSysexFromEditor();
}

void VSTCALLBACK IPlugVST2::VSTProcessReplacing(AEffect* pEffect, float** inputs, float** outputs, VstInt32 nFrames)
{
  TRACE;
  IPlugVST2* _this = (IPlugVST2*) pEffect->object;
  _this->VSTPreProcess(inputs, outputs, nFrames);
  _this->ProcessBuffers((float) 0.0f, nFrames);
  _this->OutputSysexFromEditor();
}

void VSTCALLBACK IPlugVST2::VSTProcessDoubleReplacing(AEffect* pEffect, double** inputs, double** outputs, VstInt32 nFrames)
{
  TRACE;
  IPlugVST2* _this = (IPlugVST2*) pEffect->object;
  _this->VSTPreProcess(inputs, outputs, nFrames);
  _this->ProcessBuffers((double) 0.0, nFrames);
  _this->OutputSysexFromEditor();
}

float VSTCALLBACK IPlugVST2::VSTGetParameter(AEffect *pEffect, VstInt32 idx)
{
  Trace(TRACELOC, "%d", idx);
  IPlugVST2* _this = (IPlugVST2*) pEffect->object;
  if (idx >= 0 && idx < _this->NParams())
  {
    ENTER_PARAMS_MUTEX_STATIC;
    const float val = (float) _this->GetParam(idx)->GetNormalized();
    LEAVE_PARAMS_MUTEX_STATIC;

    return val;
  }
  return 0.0f;
}

void VSTCALLBACK IPlugVST2::VSTSetParameter(AEffect *pEffect, VstInt32 idx, float value)
{
  Trace(TRACELOC, "%d:%f", idx, value);
  IPlugVST2* _this = (IPlugVST2*) pEffect->object;
  if (idx >= 0 && idx < _this->NParams())
  {
    ENTER_PARAMS_MUTEX_STATIC;
    _this->GetParam(idx)->SetNormalized(value);
    _this->SendParameterValueFromAPI(idx, value, true);
    _this->OnParamChange(idx, kHost);
    LEAVE_PARAMS_MUTEX_STATIC;
  }
}

void IPlugVST2::OutputSysexFromEditor()
{
  //Output SYSEX from the editor, which has bypassed ProcessSysEx()
  if(mSysExDataFromEditor.ElementsAvailable())
  {
    while (mSysExDataFromEditor.Pop(mSysexBuf))
    {
      ISysEx smsg {mSysexBuf.mOffset, mSysexBuf.mData, mSysexBuf.mSize};
      SendSysEx(smsg);
    }
  }
}
