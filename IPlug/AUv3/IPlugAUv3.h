 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#ifndef _IPLUGAPI_
#define _IPLUGAPI_
// Only load one API class!

/**
 * @file
 * @copydoc IPlugAUv3
 */

#include <cstring>

#include <CoreAudio/CoreAudioTypes.h>

#include "wdlstring.h"
#include "assocarray.h"

#include "IPlugAPIBase.h"
#include "IPlugProcessor.h"

union AURenderEvent;
struct AUMIDIEvent;

BEGIN_IPLUG_NAMESPACE

/** Used to pass various instance info to the API class */
struct InstanceInfo
{
};

/**  AudioUnit v3 API base class for an IPlug plug-in
 *   @ingroup APIClasses */
class IPlugAUv3 : public IPlugAPIBase
                , public IPlugProcessor
{
public:
  IPlugAUv3(const InstanceInfo& info, const Config& config);
  
  //IPlugAPIBase
  void BeginInformHostOfParamChange(int idx) override {};
  void InformHostOfParamChange(int idx, double normalizedValue) override;
  void EndInformHostOfParamChange(int idx) override {};
  void InformHostOfProgramChange() override {};

  //IPlugProcessor
  bool SendMidiMsg(const IMidiMsg& msg) override;
//  bool SendMidiMsgs(WDL_TypedBuf<IMidiMsg>& msgs) override;
  bool SendSysEx(const ISysEx& msg) override;

  //IPlugAUv3
  void ProcessWithEvents(AudioTimeStamp const* timestamp, uint32_t frameCount, AURenderEvent const* events, ITimeInfo& timeInfo);
  void SetParameterFromValueObserver(uint64_t address, float value);
  void SendParameterValueFromObserver(uint64_t address, float value);
  float GetParameter(uint64_t address);
  const char* GetParamDisplayForHost(uint64_t address, float value);
  float GetParamStringToValue(uint64_t address, const char* str);
  void SetBuffers(AudioBufferList* pInBufferList, AudioBufferList* pOutBufferList);
  void Prepare(double sampleRate, uint32_t blockSize);
  void AddParamAddress(int paramIdx, uint64_t paramAddress) { mParamAddressMap.Insert(paramIdx, paramAddress); }
  uint64_t GetParamAddress(int paramIdx) { return mParamAddressMap.Get(paramIdx); }
  int GetParamIdx(uint64_t paramAddress) { return mParamAddressMap.ReverseLookup(paramAddress); }
  
  void SetAUAudioUnit(void* pAUAudioUnit);

  void SetOffline(bool renderingOffline) { IPlugProcessor::SetRenderingOffline(renderingOffline); }

private:
//  void HandleOneEvent(AURenderEvent const* event, int64_t startTime);
//  void PerformAllSimultaneousEvents(int64_t now, AURenderEvent const*& event);
  WDL_IntKeyedArray<uint64_t> mParamAddressMap;
  void* mAUAudioUnit = nullptr;
  AudioTimeStamp mLastTimeStamp;
};

IPlugAUv3* MakePlug(const InstanceInfo& info);

END_IPLUG_NAMESPACE

#endif //_IPLUGAPI_
