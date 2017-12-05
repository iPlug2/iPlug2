#ifndef _IPLUGAPI_
#define _IPLUGAPI_
// Only load one API class!

#include <CoreServices/CoreServices.h>
#include <AudioUnit/AUComponent.h>
#include <AudioUnit/AudioUnitProperties.h>
#include <AudioToolbox/AudioUnitUtilities.h>
#include <AudioUnit/AudioUnitCarbonView.h>
#include <AvailabilityMacros.h>

#ifdef NO_IGRAPHICS
#include "IPlugBase.h"
typedef IPlugBase IPLUG_BASE_CLASS;
#else
#include "IPlugBaseGraphics.h"
typedef IPlugBaseGraphics IPLUG_BASE_CLASS;
#endif

// Argh!
#if MAC_OS_X_VERSION_MAX_ALLOWED <= 1040
  typedef Float32 AudioSampleType;
  typedef	Float32	AudioUnitParameterValue;
  typedef OSStatus (*AUMIDIOutputCallback)(void*, const AudioTimeStamp*, UInt32, const struct MIDIPacketList*);
  struct AUMIDIOutputCallbackStruct { AUMIDIOutputCallback midiOutputCallback; void* userData; };
  #define kAudioFormatFlagsCanonical (kAudioFormatFlagIsFloat|kAudioFormatFlagsNativeEndian|kAudioFormatFlagIsPacked)
#elif MAC_OS_X_VERSION_MAX_ALLOWED <= 1050
 typedef ComponentInstance AudioComponentInstance;
 typedef ComponentDescription AudioComponentDescription;
#elif MAC_OS_X_VERSION_MAX_ALLOWED == 1060
//TODO
#else
struct AudioComponentPlugInInstance {
  AudioComponentPlugInInterface   mPlugInInterface;
  void * (*mConstruct)(void *memory, AudioComponentInstance ci);
  void (*mDestruct)(void *memory);
  void *mPad[2];
  UInt32 mInstanceStorage;
};
#endif

#define MAX_IO_CHANNELS 128

static const AudioUnitPropertyID kIPlugObjectPropertyID = UINT32_MAX-100;

struct IPlugInstanceInfo
{
  WDL_String mOSXBundleID, mCocoaViewFactoryClassName;
};

class IPlugAU : public IPLUG_BASE_CLASS
{
  friend class IPlugAUFactory;
public:
  IPlugAU(IPlugInstanceInfo instanceInfo,
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
          int plugScChans);

  virtual ~IPlugAU();

  // ----------------------------------------
  // See IPlugBase for the full list of methods that your plugin class can implement.

  void BeginInformHostOfParamChange(int idx);
  void InformHostOfParamChange(int idx, double normalizedValue);
  void EndInformHostOfParamChange(int idx);

  void InformHostOfProgramChange();

  int GetSamplePos();   // Samples since start of project.
  double GetTempo();
  void GetTimeSig(int& numerator, int& denominator);
  void GetTime(ITimeInfo& timeinfo);
  EHost GetHost();  // GetHostVersion() is inherited.

  // Tell the host that the graphics resized.
  // Should be called only by the graphics object when it resizes itself.
  void ResizeGraphics(int w, int h);

  bool IsRenderingOffline();

  enum EAUInputType
  {
    eNotConnected = 0,
    eDirectFastProc,
    eDirectNoFastProc,
    eRenderCallback
  };

protected:
  void SetBlockSize(int blockSize);
  void SetLatency(int samples);
  bool SendMidiMsg(IMidiMsg& msg);
  void HostSpecificInit();
  
private:
  WDL_String mOSXBundleID;
  WDL_String mCocoaViewFactoryClassName;
  char mParamValueString[MAX_PARAM_DISPLAY_LEN];
  AudioComponentInstance mCI;
  bool mActive, mIsOffline;
  double mRenderTimestamp, mTempo;
  HostCallbackInfo mHostCallbacks;

// InScratchBuf is only needed if the upstream connection is a callback.
// OutScratchBuf is only needed if the downstream connection fails to give us a buffer.
  WDL_TypedBuf<AudioSampleType> mInScratchBuf, mOutScratchBuf;
  WDL_PtrList<AURenderCallbackStruct> mRenderNotify;
  AUMIDIOutputCallbackStruct mMidiCallback;

  // Every stereo pair of plugin input or output is a bus.
  // Buses can have zero host channels if the host hasn't connected the bus at all,
  // one host channel if the plugin supports mono and the host has supplied a mono stream,
  // or two host channels if the host has supplied a stereo stream.
  struct BusChannels
  {
    bool mConnected;
    int mNHostChannels, mNPlugChannels, mPlugChannelStartIdx;
  };
  
  WDL_PtrList<BusChannels> mInBuses, mOutBuses;
  BusChannels* GetBus(AudioUnitScope scope, AudioUnitElement busIdx);
  int NHostChannelsConnected(WDL_PtrList<BusChannels>* pBuses, int excludeIdx = -1);
  void ClearConnections();

  struct InputBusConnection
  {
    void* mUpstreamObj;
    AudioUnit mUpstreamUnit;
    int mUpstreamBusIdx;
    AudioUnitRenderProc mUpstreamRenderProc;
    AURenderCallbackStruct mUpstreamRenderCallback;
    EAUInputType mInputType;
  };
  
  WDL_PtrList<InputBusConnection> mInBusConnections;

  bool CheckLegalIO(AudioUnitScope scope, int busIdx, int nChannels);
  bool CheckLegalIO();
  void AssessInputConnections();

  struct PropertyListener
  {
    AudioUnitPropertyID mPropID;
    AudioUnitPropertyListenerProc mListenerProc;
    void* mProcArgs;
  };
  
  WDL_PtrList<PropertyListener> mPropertyListeners;

  UInt32 GetTagForNumChannels(int numChannels);
  
  UInt32 GetChannelLayoutTags(AudioUnitScope scope, 
                              AudioUnitElement element, 
                              AudioChannelLayoutTag* tags);

  OSStatus GetPropertyInfo(AudioUnitPropertyID propID, 
                                  AudioUnitScope scope, 
                                  AudioUnitElement element,
                                  UInt32* pDataSize, 
                                  Boolean* pWriteable);
  
  OSStatus GetProperty(AudioUnitPropertyID propID, 
                              AudioUnitScope scope, 
                              AudioUnitElement element,
                              UInt32* pDataSize, 
                              Boolean* pWriteable, 
                              void* pData);
  
  OSStatus SetProperty(AudioUnitPropertyID propID, 
                              AudioUnitScope scope,
                              AudioUnitElement element,
                              UInt32* pDataSize, 
                              const void* pData);
  
  OSStatus GetProc(AudioUnitElement element, UInt32* pDataSize, void* pData);
  OSStatus GetState(CFPropertyListRef* ppPropList);
  OSStatus SetState(CFPropertyListRef pPropList);
  void InformListeners(AudioUnitPropertyID propID, AudioUnitScope scope);
  void SendAUEvent(AudioUnitEventType type, AudioComponentInstance ci, int idx);
  
public:
  static OSStatus IPlugAUEntry(ComponentParameters *params, void* pVPlug);
  static OSStatus IPlugAUCarbonViewEntry(ComponentParameters *params, void* pView);
  
  static OSStatus GetParamProc(void* pPlug, 
                                      AudioUnitParameterID paramID, 
                                      AudioUnitScope scope, 
                                      AudioUnitElement element,
                                      AudioUnitParameterValue* pValue);
  
  static OSStatus SetParamProc(void* pPlug, 
                                      AudioUnitParameterID paramID, 
                                      AudioUnitScope scope, 
                                      AudioUnitElement element,
                                      AudioUnitParameterValue value, 
                                      UInt32 offsetFrames);
  
  static OSStatus RenderProc(void* pPlug,
                                    AudioUnitRenderActionFlags* pFlags, 
                                    const AudioTimeStamp* pTimestamp,
                                    UInt32 outputBusIdx, 
                                    UInt32 nFrames, 
                                    AudioBufferList* pBufferList);
  
#pragma mark Dispatch Methods
  
  static OSStatus AUMethodInitialize(void *self);
  static OSStatus AUMethodUninitialize(void *self);  
  static OSStatus AUMethodGetPropertyInfo(void *self, AudioUnitPropertyID prop, AudioUnitScope scope, AudioUnitElement elem, UInt32 *outDataSize, Boolean *outWritable);
  static OSStatus AUMethodGetProperty(void *self, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, void *outData, UInt32 *ioDataSize);  
  static OSStatus AUMethodSetProperty(void *self, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, const void *inData, UInt32 *inDataSize);
  static OSStatus AUMethodAddPropertyListener(void *self, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc, void *userData);
  static OSStatus AUMethodRemovePropertyListener(void *self, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc);
  static OSStatus AUMethodRemovePropertyListenerWithUserData(void *self, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc, void *userData);
  static OSStatus AUMethodAddRenderNotify(void *self, AURenderCallback proc, void *userData);
  static OSStatus AUMethodRemoveRenderNotify(void *self, AURenderCallback proc, void *userData);
  static OSStatus AUMethodGetParameter(void *self, AudioUnitParameterID param, AudioUnitScope scope, AudioUnitElement elem, AudioUnitParameterValue *value);
  static OSStatus AUMethodSetParameter(void *self, AudioUnitParameterID param, AudioUnitScope scope, AudioUnitElement elem, AudioUnitParameterValue value, UInt32 bufferOffset);
  static OSStatus AUMethodScheduleParameters(void *self, const AudioUnitParameterEvent *pEvent, UInt32 nEvents);  
  static OSStatus AUMethodRender(void *self, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inOutputBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData);
  static OSStatus AUMethodReset(void *self, AudioUnitScope scope, AudioUnitElement elem);
  static OSStatus AUMethodMIDIEvent(void *self, UInt32 inStatus, UInt32 inData1, UInt32 inData2, UInt32 inOffsetSampleFrame);
  static OSStatus AUMethodSysEx(void *self, const UInt8 *inData, UInt32 inLength);
  
  //Actuall Impl
  static OSStatus DoInitialize(IPlugAU *pPlug);
  static OSStatus DoUninitialize(IPlugAU *pPlug);
  static OSStatus DoGetPropertyInfo(IPlugAU *pPlug, AudioUnitPropertyID prop, AudioUnitScope scope, AudioUnitElement elem, UInt32 *outDataSize, Boolean *outWritable);
  static OSStatus DoGetProperty(IPlugAU *pPlug, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, void *outData, UInt32 *ioDataSize);
  static OSStatus DoSetProperty(IPlugAU *pPlug, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, const void *inData, UInt32 *inDataSize);
  static OSStatus DoAddPropertyListener(IPlugAU *pPlug, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc, void *userData);
  static OSStatus DoRemovePropertyListener(IPlugAU *pPlug, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc);
  static OSStatus DoRemovePropertyListenerWithUserData(IPlugAU *pPlug, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc, void *userData);
  static OSStatus DoAddRenderNotify(IPlugAU *pPlug, AURenderCallback proc, void *userData);
  static OSStatus DoRemoveRenderNotify(IPlugAU *pPlug, AURenderCallback proc, void *userData);
  static OSStatus DoGetParameter(IPlugAU *pPlug, AudioUnitParameterID param, AudioUnitScope scope, AudioUnitElement elem, AudioUnitParameterValue *value);
  static OSStatus DoSetParameter(IPlugAU *pPlug, AudioUnitParameterID param, AudioUnitScope scope, AudioUnitElement elem, AudioUnitParameterValue value, UInt32 bufferOffset);
  static OSStatus DoScheduleParameters(IPlugAU *pPlug, const AudioUnitParameterEvent *pEvent, UInt32 nEvents);
  static OSStatus DoRender(IPlugAU *pPlug, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inOutputBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData);
  static OSStatus DoReset(IPlugAU *pPlug);
  static OSStatus DoMIDIEvent(IPlugAU *pPlug, UInt32 inStatus, UInt32 inData1, UInt32 inData2, UInt32 inOffsetSampleFrame);
  static OSStatus DoSysEx(IPlugAU *pPlug, const UInt8 *inData, UInt32 inLength);
};

IPlugAU* MakePlug(void* memory = 0);

#endif


