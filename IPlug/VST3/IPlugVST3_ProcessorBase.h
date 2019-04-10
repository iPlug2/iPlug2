
/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

#pragma once

#include "public.sdk/source/vst/vstbus.h"
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/vst/ivstevents.h"

#include "IPlugAPIBase.h"
#include "IPlugProcessor.h"

using namespace Steinberg;

#ifndef CUSTOM_BUSTYPE_FUNC
uint64_t GetAPIBusTypeForChannelIOConfig(int configIdx, ERoute dir, int busIdx, IOConfig* pConfig);
#else
extern uint64_t GetAPIBusTypeForChannelIOConfig(int configIdx, ERoutingDir dir, int busIdx, IOConfig* pConfig);
#endif

class IPlugVST3ProcessorBase : public IPlugProcessor<PLUG_SAMPLE_DST>
{
public:
  IPlugVST3ProcessorBase(IPlugConfig c, IPlugAPIBase& plug);
  
  template <class T>
  void Initialize(T* plug)
  {
    Vst::String128 tmpStringBuf;
    
//  for(auto configIdx = 0; configIdx < NIOConfigs(); configIdx++)
//  {
    int configIdx = NIOConfigs() - 1;
    
    IOConfig* pConfig = GetIOConfig(configIdx);
    
    assert(pConfig);
    for (auto busIdx = 0; busIdx < pConfig->NBuses(ERoute::kInput); busIdx++)
    {
      uint64_t busType = GetAPIBusTypeForChannelIOConfig(configIdx, ERoute::kInput, busIdx, pConfig);
      
      int flags = 0; //busIdx == 0 ? flags = Steinberg::Vst::BusInfo::BusFlags::kDefaultActive : flags = 0;
      UString(tmpStringBuf, 128).fromAscii(pConfig->GetBusInfo(ERoute::kInput, busIdx)->mLabel.Get(), 128);
      plug->addAudioInput(tmpStringBuf, busType, (Vst::BusTypes) busIdx > 0, flags);
    }
    
    for (auto busIdx = 0; busIdx < pConfig->NBuses(ERoute::kOutput); busIdx++)
    {
      uint64_t busType = GetAPIBusTypeForChannelIOConfig(configIdx, ERoute::kOutput, busIdx, pConfig);
      
      int flags = 0; //busIdx == 0 ? flags = Steinberg::Vst::BusInfo::BusFlags::kDefaultActive : flags = 0;
      UString(tmpStringBuf, 128).fromAscii(pConfig->GetBusInfo(ERoute::kOutput, busIdx)->mLabel.Get(), 128);
      plug->addAudioOutput(tmpStringBuf, busType, (Vst::BusTypes) busIdx > 0, flags);
    }
//  }
    
    if (DoesMIDIIn())
      plug->addEventInput(STR16("MIDI Input"), 1);
    
    if (DoesMIDIOut())
      plug->addEventOutput(STR16("MIDI Output"), 1);
  }
  
  // MIDI Processing
  void ProcessMidiIn(Vst::IEventList* eventList, IPlugQueue<IMidiMsg>& editorQueue, IPlugQueue<IMidiMsg>& processorQueue);
  void ProcessMidiOut(IPlugQueue<SysExData>& sysExQueue, SysExData& sysExBuf, Vst::IEventList* outputEvents, int32 numSamples);
  
  // Audio Processing Setup
  void SetBusArrangments(Vst::SpeakerArrangement* pInputBusArrangements, int32 numInBuses, Vst::SpeakerArrangement* pOutputBusArrangements, int32 numOutBuses);
  void AttachBuffers(ERoute direction, int idx, int n, Vst::AudioBusBuffers& pBus, int nFrames, int32 sampleSize);
  bool SetupProcessing(const Vst::ProcessSetup& setup, Vst::ProcessSetup& storedSetup);
  bool CanProcessSampleSize(int32 symbolicSampleSize);

  // Audio Processing
  void PrepareProcessContext(Vst::ProcessData& data, Vst::ProcessSetup& setup);
  void ProcessParameterChanges(Vst::ProcessData& data);
  void ProcessAudio(Vst::ProcessData& data, Vst::ProcessSetup& setup, const Vst::BusList& ins, const Vst::BusList& outs);
  void Process(Vst::ProcessData& data, Vst::ProcessSetup& setup, const Vst::BusList& ins, const Vst::BusList& outs, IPlugQueue<IMidiMsg>& fromEditor, IPlugQueue<IMidiMsg>& fromProcessor, IPlugQueue<SysExData>& sysExFromEditor, SysExData& sysExBuf);
  
  // IPlugProcessor overrides
  bool SendMidiMsg(const IMidiMsg& msg) override;

private:
  IPlugAPIBase& mPlug;
  Vst::ProcessContext mProcessContext;
  IMidiQueue mMidiOutputQueue;
  bool mSidechainActive = false;
};
