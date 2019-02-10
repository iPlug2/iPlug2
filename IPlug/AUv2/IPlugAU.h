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
 * @copydoc IPlugAU
 */

#include <CoreServices/CoreServices.h>
#include <AudioUnit/AUComponent.h>
#include <AudioUnit/AudioUnitProperties.h>
#include <AudioToolbox/AudioUnitUtilities.h>
#include <AvailabilityMacros.h>

#include "IPlugAPIBase.h"
#include "IPlugProcessor.h"

struct AudioComponentPlugInInstance
{
  AudioComponentPlugInInterface mPlugInInterface;
  void* (*mConstruct)(void* pMemory, AudioComponentInstance ci);
  void (*mDestruct)(void* pMemory);
  void* mPad[2];
  UInt32 mInstanceStorage;
};

static const AudioUnitPropertyID kIPlugObjectPropertyID = UINT32_MAX-100;

/** Used to pass various instance info to the API class */
struct IPlugInstanceInfo
{
  WDL_String mBundleID;
  WDL_String mCocoaViewFactoryClassName;
};

/**  AudioUnit v2 API base class for an IPlug plug-in
*   @ingroup APIClasses */
class IPlugAU : public IPlugAPIBase
              , public IPlugProcessor<PLUG_SAMPLE_DST>
{
public:
  IPlugAU(IPlugInstanceInfo instanceInfo, IPlugConfig config);
  ~IPlugAU();

//IPlugAPIBase
  void BeginInformHostOfParamChange(int idx) override;
  void InformHostOfParamChange(int idx, double normalizedValue) override;
  void EndInformHostOfParamChange(int idx) override;
  void InformHostOfProgramChange() override;
  void InformHostOfParameterDetailsChange() override;

//IPlugProcessor
  bool SendMidiMsg(const IMidiMsg& msg) override;
  bool SendMidiMsgs(WDL_TypedBuf<IMidiMsg>& msgs) override;
  bool SendSysEx(const ISysEx& msg) override;
  void SetLatency(int samples) override;

//IPlugAU
  void OutputSysexFromEditor();
  void PreProcess();
  void ResizeScratchBuffers();
  static const char* AUInputTypeStr(int type);
#ifndef AU_NO_COMPONENT_ENTRY
  static OSStatus IPlugAUEntry(ComponentParameters* pParams, void* pPlug);
#endif
private:

  enum EAUInputType
  {
    eNotConnected = 0,
    eDirectFastProc,
    eDirectNoFastProc,
    eRenderCallback
  };
  
  struct BusChannels
  {
    bool mConnected;
    int mNHostChannels;
    int mNPlugChannels;
    int mPlugChannelStartIdx;
  };
  
  struct BufferList
  {
    int mNumberBuffers;
    AudioBuffer mBuffers[AU_MAX_IO_CHANNELS];
  };

  struct InputBusConnection
  {
    void* mUpstreamObj;
    AudioUnit mUpstreamUnit;
    int mUpstreamBusIdx;
    AudioUnitRenderProc mUpstreamRenderProc;
    AURenderCallbackStruct mUpstreamRenderCallback;
    EAUInputType mInputType;
  };
  
  struct PropertyListener
  {
    AudioUnitPropertyID mPropID;
    AudioUnitPropertyListenerProc mListenerProc;
    void* mProcArgs;
  };
  
  int NHostChannelsConnected(WDL_PtrList<BusChannels>* pBuses, int excludeIdx = -1);
  void ClearConnections();
  BusChannels* GetBus(AudioUnitScope scope, AudioUnitElement busIdx);
  bool CheckLegalIO(AudioUnitScope scope, int busIdx, int nChannels);
  bool CheckLegalIO();
  void AssessInputConnections();

  UInt32 GetTagForNumChannels(int numChannels);
  UInt32 GetChannelLayoutTags(AudioUnitScope scope, AudioUnitElement element, AudioChannelLayoutTag* pTags);
  
#pragma mark - Component Manager Methods
  OSStatus GetPropertyInfo(AudioUnitPropertyID propID, AudioUnitScope scope, AudioUnitElement element, UInt32* pDataSize, Boolean* pWriteable);
  OSStatus GetProperty(AudioUnitPropertyID propID, AudioUnitScope scope, AudioUnitElement element, UInt32* pDataSize, Boolean* pWriteable, void* pData);
  OSStatus SetProperty(AudioUnitPropertyID propID, AudioUnitScope scope, AudioUnitElement element, UInt32* pDataSize, const void* pData);
  
  OSStatus GetProc(AudioUnitElement element, UInt32* pDataSize, void* pData);
  OSStatus GetState(CFPropertyListRef* ppPropList);
  OSStatus SetState(CFPropertyListRef pPropList);
  void InformListeners(AudioUnitPropertyID propID, AudioUnitScope scope);
  void SendAUEvent(AudioUnitEventType type, AudioComponentInstance ci, int idx);
  
  static OSStatus GetParamProc(void* pPlug, AudioUnitParameterID paramID, AudioUnitScope scope, AudioUnitElement element, AudioUnitParameterValue* pValue);
  static OSStatus SetParamProc(void* pPlug, AudioUnitParameterID paramID, AudioUnitScope scope, AudioUnitElement element, AudioUnitParameterValue value, UInt32 offsetFrames);
  static OSStatus RenderProc(void* pPlug, AudioUnitRenderActionFlags* pFlags, const AudioTimeStamp* pTimestamp, UInt32 outputBusIdx, UInt32 nFrames, AudioBufferList* pBufferList);
  
#pragma mark - Dispatch Methods
  static OSStatus AUMethodInitialize(void* pSelf);
  static OSStatus AUMethodUninitialize(void* pSelf);
  static OSStatus AUMethodGetPropertyInfo(void* pSelf, AudioUnitPropertyID prop, AudioUnitScope scope, AudioUnitElement elem, UInt32* pOutDataSize, Boolean* pOutWritable);
  static OSStatus AUMethodGetProperty(void* pSelf, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, void* pOutData, UInt32* pIODataSize);
  static OSStatus AUMethodSetProperty(void* pSelf, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, const void* pInData, UInt32* pInDataSize);
  static OSStatus AUMethodAddPropertyListener(void* pSelf, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc, void* pUserData);
  static OSStatus AUMethodRemovePropertyListener(void* pSelf, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc);
  static OSStatus AUMethodRemovePropertyListenerWithUserData(void* pSelf, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc, void* pUserData);
  static OSStatus AUMethodAddRenderNotify(void* pSelf, AURenderCallback proc, void* pUserData);
  static OSStatus AUMethodRemoveRenderNotify(void* pSelf, AURenderCallback proc, void* pUserData);
  static OSStatus AUMethodGetParameter(void* pSelf, AudioUnitParameterID param, AudioUnitScope scope, AudioUnitElement elem, AudioUnitParameterValue *value);
  static OSStatus AUMethodSetParameter(void* pSelf, AudioUnitParameterID param, AudioUnitScope scope, AudioUnitElement elem, AudioUnitParameterValue value, UInt32 bufferOffset);
  static OSStatus AUMethodScheduleParameters(void* pSelf, const AudioUnitParameterEvent *pEvent, UInt32 nEvents);
  static OSStatus AUMethodRender(void* pSelf, AudioUnitRenderActionFlags* pIOActionFlags, const AudioTimeStamp* pInTimeStamp, UInt32 inOutputBusNumber, UInt32 inNumberFrames, AudioBufferList* pIOData);
  static OSStatus AUMethodReset(void* pSelf, AudioUnitScope scope, AudioUnitElement elem);
  static OSStatus AUMethodMIDIEvent(void* pSelf, UInt32 inStatus, UInt32 inData1, UInt32 inData2, UInt32 inOffsetSampleFrame);
  static OSStatus AUMethodSysEx(void* pSelf, const UInt8* pInData, UInt32 inLength);
  
#pragma mark - Implementation Methods
  static OSStatus DoInitialize(IPlugAU* pPlug);
  static OSStatus DoUninitialize(IPlugAU* pPlug);
  static OSStatus DoGetPropertyInfo(IPlugAU* pPlug, AudioUnitPropertyID prop, AudioUnitScope scope, AudioUnitElement elem, UInt32 *pOutDataSize, Boolean* pOutWritable);
  static OSStatus DoGetProperty(IPlugAU* pPlug, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, void *pOutData, UInt32* pIODataSize);
  static OSStatus DoSetProperty(IPlugAU* pPlug, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, const void* pInData, UInt32* pInDataSize);
  static OSStatus DoAddPropertyListener(IPlugAU* pPlug, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc, void* pUserData);
  static OSStatus DoRemovePropertyListener(IPlugAU* pPlug, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc);
  static OSStatus DoRemovePropertyListenerWithUserData(IPlugAU* pPlug, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc, void* pUserData);
  static OSStatus DoAddRenderNotify(IPlugAU* pPlug, AURenderCallback proc, void* pUserData);
  static OSStatus DoRemoveRenderNotify(IPlugAU* pPlug, AURenderCallback proc, void* pUserData);
  static OSStatus DoGetParameter(IPlugAU* pPlug, AudioUnitParameterID param, AudioUnitScope scope, AudioUnitElement elem, AudioUnitParameterValue* pValue);
  static OSStatus DoSetParameter(IPlugAU* pPlug, AudioUnitParameterID param, AudioUnitScope scope, AudioUnitElement elem, AudioUnitParameterValue value, UInt32 bufferOffset);
  static OSStatus DoScheduleParameters(IPlugAU* pPlug, const AudioUnitParameterEvent *pEvent, UInt32 nEvents);
  static OSStatus DoRender(IPlugAU* pPlug, AudioUnitRenderActionFlags* pIOActionFlags, const AudioTimeStamp* pInTimeStamp, UInt32 inOutputBusNumber, UInt32 inNumberFrames, AudioBufferList* pIOData);
  static OSStatus DoReset(IPlugAU* pPlug);
  static OSStatus DoMIDIEvent(IPlugAU* pPlug, UInt32 inStatus, UInt32 inData1, UInt32 inData2, UInt32 inOffsetSampleFrame);
  static OSStatus DoSysEx(IPlugAU* pPlug, const UInt8 *inData, UInt32 inLength);
  
#pragma mark -
private:
  bool mActive = false; // TODO: is this necessary? is it correct?
  double mLastRenderSampleTime = -1.0;
  WDL_String mCocoaViewFactoryClassName;
  AudioComponentInstance mCI = nullptr;
  HostCallbackInfo mHostCallbacks;
  WDL_PtrList<BusChannels> mInBuses, mOutBuses;
  WDL_PtrList<InputBusConnection> mInBusConnections;
  WDL_PtrList<PropertyListener> mPropertyListeners;
  WDL_TypedBuf<AudioSampleType> mInScratchBuf;
  WDL_TypedBuf<AudioSampleType> mOutScratchBuf;
  WDL_PtrList<AURenderCallbackStruct> mRenderNotify;
  AUMIDIOutputCallbackStruct mMidiCallback;
  AudioTimeStamp mLastRenderTimeStamp;

  friend class IPlugAUFactory;
};

IPlugAU* MakePlug(void* memory = 0);

#endif


