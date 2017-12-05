#include "IPlugAU.h"
#include "Log.h"
#include "Hosts.h"

#include "dfx/dfx-au-utilities.h"
#include <algorithm>

inline CFStringRef MakeCFString(const char* cStr)
{
  return CFStringCreateWithCString(0, cStr, kCFStringEncodingUTF8);
}

struct CFStrLocal
{
  CFStringRef mCFStr;
  CFStrLocal(const char* cStr)
  {
    mCFStr = MakeCFString(cStr);
  }
  ~CFStrLocal()
  {
    CFRelease(mCFStr);
  }
};

struct CStrLocal
{
  char* mCStr;
  CStrLocal(CFStringRef cfStr)
  {
    long n = CFStringGetLength(cfStr) + 1;
    mCStr = (char*) malloc(n);
    CFStringGetCString(cfStr, mCStr, n, kCFStringEncodingUTF8);
  }
  ~CStrLocal()
  {
    FREE_NULL(mCStr);
  }
};

#define kAudioUnitRemovePropertyListenerWithUserDataSelect 0x0012

typedef AudioStreamBasicDescription STREAM_DESC;

/* inline */ void MakeDefaultASBD(STREAM_DESC* pASBD, double sampleRate, int nChannels, bool interleaved)
{
  memset(pASBD, 0, sizeof(STREAM_DESC));
  pASBD->mSampleRate = sampleRate;
  pASBD->mFormatID = kAudioFormatLinearPCM;
  pASBD->mFormatFlags = kAudioFormatFlagsCanonical;
  pASBD->mBitsPerChannel = 8 * sizeof(AudioSampleType);
  pASBD->mChannelsPerFrame = nChannels;
  pASBD->mFramesPerPacket = 1;
  int nBytes = sizeof(AudioSampleType);
  if (interleaved)
  {
    nBytes *= nChannels;
  }
  else
  {
    pASBD->mFormatFlags |= kAudioFormatFlagIsNonInterleaved;
  }
  pASBD->mBytesPerPacket = pASBD->mBytesPerFrame = nBytes;
}

template <class C>
int PtrListAddFromStack(WDL_PtrList<C>* pList, C* pStackInstance)
{
  C* pNew = new C;
  memcpy(pNew, pStackInstance, sizeof(C));
  pList->Add(pNew);
  return pList->GetSize() - 1;
}

template <class C>
int PtrListInitialize(WDL_PtrList<C>* pList, int size)
{
  for (int i = 0; i < size; ++i)
  {
    C* pNew = new C;
    memset(pNew, 0, sizeof(C));
    pList->Add(pNew);
  }
  return size;
}

#if defined(__LP64__)
  #define GET_COMP_PARAM(TYPE, IDX, NUM) *((TYPE*)&(params->params[NUM - IDX]))
#else
  #define GET_COMP_PARAM(TYPE, IDX, NUM) *((TYPE*)&(params->params[IDX]))
#endif

#define NO_OP(select) case select: return badComponentSelector;

#pragma mark COMPONENT MANAGER ENTRY POINT
// static
OSStatus IPlugAU::IPlugAUEntry(ComponentParameters *params, void* pPlug)
{
  int select = params->what;

  Trace(TRACELOC, "(%d:%s)", select, AUSelectStr(select));

  if (select == kComponentOpenSelect)
  {
    IPlugAU* _this = MakePlug();
    _this->HostSpecificInit();
    _this->PruneUninitializedPresets();
    _this->mCI = GET_COMP_PARAM(ComponentInstance, 0, 1);
    SetComponentInstanceStorage(_this->mCI, (Handle) _this);
    return noErr;
  }

  IPlugAU* _this = (IPlugAU*) pPlug;
  
  if (select == kComponentCloseSelect)
  {
    _this->ClearConnections();
    DELETE_NULL(_this);
    return noErr;
  }

  switch (select)
  {
    case kComponentVersionSelect:
    {
      return _this->GetEffectVersion(false);
    }
    case kAudioUnitInitializeSelect:
    {
      return DoInitialize(_this);
    }
    case kAudioUnitUninitializeSelect:
    {
      return DoUninitialize(_this);
    }
    case kAudioUnitGetPropertyInfoSelect:
    {
      AudioUnitPropertyID propID = GET_COMP_PARAM(AudioUnitPropertyID, 4, 5);
      AudioUnitScope scope = GET_COMP_PARAM(AudioUnitScope, 3, 5);
      AudioUnitElement element = GET_COMP_PARAM(AudioUnitElement, 2, 5);
      UInt32* pDataSize = GET_COMP_PARAM(UInt32*, 1, 5);
      Boolean* pWriteable = GET_COMP_PARAM(Boolean*, 0, 5);

      return _this->DoGetPropertyInfo(_this, propID, scope, element, pDataSize, pWriteable);
    }
    case kAudioUnitGetPropertySelect:
    {
      AudioUnitPropertyID propID = GET_COMP_PARAM(AudioUnitPropertyID, 4, 5);
      AudioUnitScope scope = GET_COMP_PARAM(AudioUnitScope, 3, 5);
      AudioUnitElement element = GET_COMP_PARAM(AudioUnitElement, 2, 5);
      void* pData = GET_COMP_PARAM(void*, 1, 5);
      UInt32* pDataSize = GET_COMP_PARAM(UInt32*, 0, 5);
      
      return _this->DoGetProperty(_this, propID, scope, element, pData, pDataSize);
    }
    case kAudioUnitSetPropertySelect:
    {
      AudioUnitPropertyID propID = GET_COMP_PARAM(AudioUnitPropertyID, 4, 5);
      AudioUnitScope scope = GET_COMP_PARAM(AudioUnitScope, 3, 5);
      AudioUnitElement element = GET_COMP_PARAM(AudioUnitElement, 2, 5);
      const void* pData = GET_COMP_PARAM(const void*, 1, 5);
      UInt32* pDataSize = GET_COMP_PARAM(UInt32*, 0, 5);
      
      return _this->DoSetProperty(_this, propID, scope, element, pData, pDataSize);
    }
    case kAudioUnitAddPropertyListenerSelect:
    {
      AudioUnitPropertyID propID = GET_COMP_PARAM(AudioUnitPropertyID, 2, 3);
      AudioUnitPropertyListenerProc proc = GET_COMP_PARAM(AudioUnitPropertyListenerProc, 1, 3);
      void* userData = GET_COMP_PARAM(void*, 0, 3);
      
      return _this->DoAddPropertyListener(_this, propID, proc, userData);
    }
    case kAudioUnitRemovePropertyListenerSelect:
    {
      AudioUnitPropertyID propID = GET_COMP_PARAM(AudioUnitPropertyID, 1, 2);
      AudioUnitPropertyListenerProc proc = GET_COMP_PARAM(AudioUnitPropertyListenerProc, 0, 2);

      return _this->DoRemovePropertyListener(_this, propID, proc);
    }
    case kAudioUnitRemovePropertyListenerWithUserDataSelect:
    {
      AudioUnitPropertyID propID = GET_COMP_PARAM(AudioUnitPropertyID, 2, 3);
      AudioUnitPropertyListenerProc proc = GET_COMP_PARAM(AudioUnitPropertyListenerProc, 1, 3);
      void* userData = GET_COMP_PARAM(void*, 0, 3);
      
      return _this->DoRemovePropertyListenerWithUserData(_this, propID, proc, userData);
    }
    case kAudioUnitAddRenderNotifySelect:
    {
      AURenderCallback proc = GET_COMP_PARAM(AURenderCallback, 1, 2);
      void* userData = GET_COMP_PARAM(void*, 0, 2);
      return _this->DoAddRenderNotify(_this, proc, userData);
    }
    case kAudioUnitRemoveRenderNotifySelect:
    {
      AURenderCallback proc = GET_COMP_PARAM(AURenderCallback, 1, 2);
      void* userData = GET_COMP_PARAM(void*, 0, 2);
      return _this->DoRemoveRenderNotify(_this, proc, userData);
    }
    case kAudioUnitGetParameterSelect:
    {
      AudioUnitParameterID paramID = GET_COMP_PARAM(AudioUnitParameterID, 3, 4);
      AudioUnitScope scope = GET_COMP_PARAM(AudioUnitScope, 2, 4);
      AudioUnitElement element = GET_COMP_PARAM(AudioUnitElement, 1, 4);
      AudioUnitParameterValue* pValue = GET_COMP_PARAM(AudioUnitParameterValue*, 0, 4);
      return _this->DoGetParameter(_this, paramID, scope, element, pValue);
    }
    case kAudioUnitSetParameterSelect:
    {
      AudioUnitParameterID paramID = GET_COMP_PARAM(AudioUnitParameterID, 4, 5);
      AudioUnitScope scope = GET_COMP_PARAM(AudioUnitScope, 3, 5);
      AudioUnitElement element = GET_COMP_PARAM(AudioUnitElement, 2, 5);
      AudioUnitParameterValue value = GET_COMP_PARAM(AudioUnitParameterValue, 1, 5);
      UInt32 offset = GET_COMP_PARAM(UInt32, 0, 5);
      return _this->DoSetParameter(_this, paramID, scope, element, value, offset);
    }
    case kAudioUnitScheduleParametersSelect:
    {
      AudioUnitParameterEvent* pEvent = GET_COMP_PARAM(AudioUnitParameterEvent*, 1, 2);
      UInt32 nEvents = GET_COMP_PARAM(UInt32, 0, 2);
      return _this->DoScheduleParameters(_this, pEvent, nEvents);
    }
    case kAudioUnitRenderSelect:
    {
      AudioUnitRenderActionFlags* pFlags = GET_COMP_PARAM(AudioUnitRenderActionFlags*, 4, 5);
      const AudioTimeStamp* pTimestamp = GET_COMP_PARAM(AudioTimeStamp*, 3, 5);
      UInt32 outputBusIdx = GET_COMP_PARAM(UInt32, 2, 5);
      UInt32 nFrames = GET_COMP_PARAM(UInt32, 1, 5);
      AudioBufferList* pBufferList = GET_COMP_PARAM(AudioBufferList*, 0, 5);
      return _this->DoRender(_this, pFlags, pTimestamp, outputBusIdx, nFrames, pBufferList);
    }
    case kAudioUnitResetSelect:
    {
      return _this->DoReset(_this);
    }
    case kMusicDeviceMIDIEventSelect:
    {
      return _this->DoMIDIEvent(_this, GET_COMP_PARAM(UInt32, 3, 4), GET_COMP_PARAM(UInt32, 2, 4), GET_COMP_PARAM(UInt32, 1, 4), GET_COMP_PARAM(UInt32, 0, 4));
    }
    case kMusicDeviceSysExSelect:
    {
      return _this->DoSysEx(_this, GET_COMP_PARAM(UInt8*, 1, 2), GET_COMP_PARAM(UInt32, 0, 2));
    }
    case kMusicDevicePrepareInstrumentSelect:
    {
      return noErr;
    }
    case kMusicDeviceReleaseInstrumentSelect:
    {
      return noErr;
    }
    case kMusicDeviceStartNoteSelect:
    {
//      MusicDeviceInstrumentID deviceID = GET_COMP_PARAM(MusicDeviceInstrumentID, 4, 5);
//      MusicDeviceGroupID groupID = GET_COMP_PARAM(MusicDeviceGroupID, 3, 5);
      NoteInstanceID* pNoteID = GET_COMP_PARAM(NoteInstanceID*, 2, 5);
      UInt32 offset = GET_COMP_PARAM(UInt32, 1, 5);
      MusicDeviceNoteParams* pNoteParams = GET_COMP_PARAM(MusicDeviceNoteParams*, 0, 5);
      int note = (int) pNoteParams->mPitch;
      *pNoteID = note;
      IMidiMsg msg;
      msg.MakeNoteOnMsg(note, (int) pNoteParams->mVelocity, offset);
      return noErr;
    }
    case kMusicDeviceStopNoteSelect:
    {
//      MusicDeviceGroupID groupID = GET_COMP_PARAM(MusicDeviceGroupID, 2, 3);
      NoteInstanceID noteID = GET_COMP_PARAM(NoteInstanceID, 1, 3);
      UInt32 offset = GET_COMP_PARAM(UInt32, 0, 3);
      // noteID is supposed to be some incremented unique ID, but we're just storing note number in it.
      IMidiMsg msg;
      msg.MakeNoteOffMsg(noteID, offset);
      return noErr;
    }
    case kComponentCanDoSelect:
    {
      switch (params->params[0])
      {
        case kAudioUnitInitializeSelect:
        case kAudioUnitUninitializeSelect:
        case kAudioUnitGetPropertyInfoSelect:
        case kAudioUnitGetPropertySelect:
        case kAudioUnitSetPropertySelect:
        case kAudioUnitAddPropertyListenerSelect:
        case kAudioUnitRemovePropertyListenerSelect:
        case kAudioUnitGetParameterSelect:
        case kAudioUnitSetParameterSelect:
        case kAudioUnitResetSelect:
        case kAudioUnitRenderSelect:
        case kAudioUnitAddRenderNotifySelect:
        case kAudioUnitRemoveRenderNotifySelect:
        case kAudioUnitScheduleParametersSelect:
          return 1;
        default:
          return 0;
      }
    }
    default: return badComponentSelector;
  }
}

struct AudioUnitCarbonViewCreateGluePB
{
  unsigned char componentFlags;
  unsigned char componentParamSize;
  short componentWhat;
  ControlRef* outControl;
  const Float32Point* inSize;
  const Float32Point* inLocation;
  ControlRef inParentControl;
  WindowRef inWindow;
  AudioUnit inAudioUnit;
  AudioUnitCarbonView inView;
};

struct CarbonViewInstance
{
  ComponentInstance mCI;
  IPlugAU* mPlug;
};

// static
OSStatus IPlugAU::IPlugAUCarbonViewEntry(ComponentParameters *params, void* pView)
{
  int select = params->what;

  Trace(TRACELOC, "%d:%s", select, AUSelectStr(select));

  if (select == kComponentOpenSelect)
  {
    CarbonViewInstance* pCVI = new CarbonViewInstance;
    pCVI->mCI = GET_COMP_PARAM(ComponentInstance, 0, 1);
    pCVI->mPlug = 0;
    SetComponentInstanceStorage(pCVI->mCI, (Handle) pCVI);
    return noErr;
  }

  CarbonViewInstance* pCVI = (CarbonViewInstance*) pView;

  switch (select)
  {
    case kComponentCloseSelect:
    {
      IPlugAU* _this = pCVI->mPlug;
      if (_this && _this->GetHasUI())
      {
        _this->CloseWindow();
      }
      DELETE_NULL(pCVI);
      return noErr;
    }
    case kAudioUnitCarbonViewCreateSelect:
    {
      AudioUnitCarbonViewCreateGluePB* pb = (AudioUnitCarbonViewCreateGluePB*) params;
      IPlugAU* _this = (IPlugAU*) GetComponentInstanceStorage(pb->inAudioUnit);
      pCVI->mPlug = _this;
      if (_this && _this->GetHasUI())
      {
        *(pb->outControl) = (ControlRef) (_this->OpenWindow(pb->inWindow, pb->inParentControl));
        return noErr;
      }
      return badComponentSelector;
    }
    default:  return badComponentSelector;
  }
}

//TODO: support more tags

UInt32 IPlugAU::GetTagForNumChannels(int numChannels)
{
  switch (numChannels)
  {
    case 1:
      return kAudioChannelLayoutTag_Mono;
    case 2:
      return kAudioChannelLayoutTag_Stereo;
//    case 4:
//      return kAudioChannelLayoutTag_Ambisonic_B_Format;
//    case 6:
//      return kAudioChannelLayoutTag_AudioUnit_5_1;
//    case 7:
//      return kAudioChannelLayoutTag_AudioUnit_6_1;
//    case 8:
//      return kAudioChannelLayoutTag_AudioUnit_7_1;
    case 0:
    default:
      return kAudioChannelLayoutTag_Unknown;
  }
}

UInt32 IPlugAU::GetChannelLayoutTags(AudioUnitScope scope, AudioUnitElement element, AudioChannelLayoutTag* tags)
{
  switch(scope)
  {
    case kAudioUnitScope_Input:
    {
      if (!mInBuses.Get(0)) // no inputs = synth
        return 0;

      // this stuff is not currently needed

//      if (element == 0 ) // main input
//      {
//        bool canDoMono = LegalIO(1, -1);
//        bool canDoStereo = LegalIO(2, -1);
//
//        if (canDoMono && canDoStereo)
//        {
//          if(tags)
//          {
//            tags[0] = GetTagForNumChannels(1);
//            tags[1] = GetTagForNumChannels(2);
//          }
//
//          return 2;
//        }
//        else if (canDoMono)
//        {
//          if(tags)
//          {
//            tags[0] = GetTagForNumChannels(1);
//          }
//
//          return 1;
//        }
//        else if (canDoStereo)
//        {
//          if(tags)
//          {
//            tags[0] = GetTagForNumChannels(2);
//          }
//
//          return 1;
//        }
//      }
//      else if (element == 1 ) // aux input
//      {
//        if(tags)
//        {
//          tags[0] = GetTagForNumChannels(mInBuses.Get(element)->mNPlugChannels);
//        }
//
//        return 1;
//      }
    }
    case kAudioUnitScope_Output:
    {
      if(tags)
      {
        tags[0] = GetTagForNumChannels(mOutBuses.Get(element)->mNPlugChannels);
      }

      return 1;
    }
    default:
      return 0;
  }
}

#define ASSERT_SCOPE(reqScope) if (scope != reqScope) { return kAudioUnitErr_InvalidProperty; }
#define ASSERT_ELEMENT(numElements) if (element >= numElements) { return kAudioUnitErr_InvalidElement; }
#define ASSERT_INPUT_OR_GLOBAL_SCOPE \
  if (scope != kAudioUnitScope_Input && scope != kAudioUnitScope_Global) { \
    return kAudioUnitErr_InvalidProperty; \
  }
#undef NO_OP
#define NO_OP(propID) case propID: return kAudioUnitErr_InvalidProperty;

// pData == 0 means return property info only.
OSStatus IPlugAU::GetProperty(AudioUnitPropertyID propID, AudioUnitScope scope, AudioUnitElement element,
                                     UInt32* pDataSize, Boolean* pWriteable, void* pData)
{
  Trace(TRACELOC, "%s(%d:%s):(%d:%s):%d", (pData ? "" : "info:"), propID, AUPropertyStr(propID), scope, AUScopeStr(scope), element);

  switch (propID)
  {
    case kIPlugObjectPropertyID:
    {
      *pDataSize = sizeof (void*);
      if (pData)
      {
        ((void**) pData)[0] = (void*) static_cast<IPlugBase*> (this);
      }
      else {
        *pWriteable = false;
      }
      return noErr;
    }
    case kAudioUnitProperty_ClassInfo:                    // 0,
    {
      *pDataSize = sizeof(CFPropertyListRef);
      if (pData)
      {
        CFPropertyListRef* pList = (CFPropertyListRef*) pData;
      *pWriteable = true;
        return GetState(pList);
      }
      return noErr;
    }
    case kAudioUnitProperty_MakeConnection:               // 1,
    {
      ASSERT_INPUT_OR_GLOBAL_SCOPE;
      *pDataSize = sizeof(AudioUnitConnection);
      *pWriteable = true;
      return noErr;
    }
    case kAudioUnitProperty_SampleRate:                  // 2,
    {
      *pDataSize = sizeof(Float64);
      *pWriteable = true;
      if (pData)
      {
        *((Float64*) pData) = GetSampleRate();
      }
      return noErr;
    }
    case kAudioUnitProperty_ParameterList:               // 3,  listenable
    {
      int n = (scope == kAudioUnitScope_Global ? NParams() : 0);
      *pDataSize = n * sizeof(AudioUnitParameterID);
      if (pData && n)
      {
        AudioUnitParameterID* pParamID = (AudioUnitParameterID*) pData;
        for (int i = 0; i < n; ++i, ++pParamID)
        {
          *pParamID = (AudioUnitParameterID) i;
        }
      }
      return noErr;
    }
    case kAudioUnitProperty_ParameterInfo:               // 4,  listenable
    {
      ASSERT_SCOPE(kAudioUnitScope_Global);
      ASSERT_ELEMENT(NParams());
      *pDataSize = sizeof(AudioUnitParameterInfo);
      if (pData)
      {
        AudioUnitParameterInfo* pInfo = (AudioUnitParameterInfo*) pData;
        memset(pInfo, 0, sizeof(AudioUnitParameterInfo));
        pInfo->flags = kAudioUnitParameterFlag_CFNameRelease |
                       kAudioUnitParameterFlag_HasCFNameString |
                       kAudioUnitParameterFlag_IsReadable;
        
        WDL_MutexLock lock(&mParams_mutex);
        IParam* pParam = GetParam(element);
        
        if (pParam->GetCanAutomate()) 
        {
          pInfo->flags = pInfo->flags | kAudioUnitParameterFlag_IsWritable;
        }
        
        if (pParam->GetIsMeta()) 
        {
          pInfo->flags |= kAudioUnitParameterFlag_IsElementMeta;
        }
        
        const char* paramName = pParam->GetNameForHost();
        pInfo->cfNameString = CFStringCreateWithCString(0, pParam->GetNameForHost(), kCFStringEncodingUTF8);
        strcpy(pInfo->name, paramName);   // Max 52.

        switch (pParam->Type())
        {
          case IParam::kTypeBool:
            pInfo->unit = kAudioUnitParameterUnit_Boolean;
            break;
          case IParam::kTypeEnum:
            //fall through
          case IParam::kTypeInt:
            pInfo->unit = kAudioUnitParameterUnit_Indexed;
            break;
          default:
          {
            const char* label = pParam->GetLabelForHost();
            if (CSTR_NOT_EMPTY(label))
            {
              pInfo->unit = kAudioUnitParameterUnit_CustomUnit;
              pInfo->unitName = CFStringCreateWithCString(0, label, kCFStringEncodingUTF8);
            }
            else
            {
              pInfo->unit = kAudioUnitParameterUnit_Generic;
            }
          }
        }
        double lo, hi;
        pParam->GetBounds(lo, hi);
        pInfo->minValue = lo;
        pInfo->maxValue = hi;
        pInfo->defaultValue = pParam->Value();
        
        const char* paramGroupName = pParam->GetParamGroupForHost();

        if (CSTR_NOT_EMPTY(paramGroupName))
        {
          int clumpID = 0;
          
          for(int i = 0; i< mParamGroups.GetSize(); i++)
          {
            if(strcmp(paramGroupName, mParamGroups.Get(i)) == 0)
            {
              clumpID = i+1;
            }
          }
          
          if (clumpID == 0) // new clump
          {
            mParamGroups.Add(paramGroupName);
            clumpID = mParamGroups.GetSize();
          }
          
          pInfo->flags = pInfo->flags | kAudioUnitParameterFlag_HasClump;
          pInfo->clumpID = clumpID;
        }
      }
      return noErr;
    }
    case kAudioUnitProperty_FastDispatch:                // 5,
    {
      return GetProc(element, pDataSize, pData);
    }
    NO_OP(kAudioUnitProperty_CPULoad);                   // 6,
    case kAudioUnitProperty_StreamFormat:                // 8,
    {
      BusChannels* pBus = GetBus(scope, element);
      if (!pBus)
      {
        return kAudioUnitErr_InvalidProperty;
      }
      *pDataSize = sizeof(STREAM_DESC);
      *pWriteable = true;
      if (pData)
      {
        int nChannels = pBus->mNHostChannels;  // Report how many channels the host has connected.
        if (nChannels < 0)    // Unless the host hasn't connected any yet, in which case report the default.
        {
          nChannels = pBus->mNPlugChannels;
        }
        STREAM_DESC* pASBD = (STREAM_DESC*) pData;
        MakeDefaultASBD(pASBD, GetSampleRate(), nChannels, false);
      }
      return noErr;
    }
    case kAudioUnitProperty_ElementCount:                // 11,
    {
      *pDataSize = sizeof(UInt32);
      if (pData)
      {
        int n = 0;
        if (scope == kAudioUnitScope_Input)
        {
          n = mInBuses.GetSize();
        }
        else if (scope == kAudioUnitScope_Output)
        {
          n = mOutBuses.GetSize();
        }
        else if (scope == kAudioUnitScope_Global)
        {
          n = 1;
        }
        *((UInt32*) pData) = n;
      }
      return noErr;
    }
    case kAudioUnitProperty_Latency:                      // 12,  // listenable
    {
      ASSERT_SCOPE(kAudioUnitScope_Global);
      *pDataSize = sizeof(Float64);
      if (pData)
      {
        *((Float64*) pData) = (double) GetLatency() / GetSampleRate();
      }
      return noErr;
    }
    case kAudioUnitProperty_SupportedNumChannels:        // 13,
    {
      ASSERT_SCOPE(kAudioUnitScope_Global);
      int n = mChannelIO.GetSize();
      *pDataSize = n * sizeof(AUChannelInfo);
      if (pData)
      {
        AUChannelInfo* pChInfo = (AUChannelInfo*) pData;
        for (int i = 0; i < n; ++i, ++pChInfo)
        {
          ChannelIO* pIO = mChannelIO.Get(i);
          pChInfo->inChannels = pIO->mIn;
          pChInfo->outChannels = pIO->mOut;
          Trace(TRACELOC, "IO:%d:%d", pIO->mIn, pIO->mOut);

        }
      }
      return noErr;
    }
    case kAudioUnitProperty_MaximumFramesPerSlice:       // 14,
    {
      ASSERT_SCOPE(kAudioUnitScope_Global);
      *pDataSize = sizeof(UInt32);
      *pWriteable = true;
      if (pData)
      {
        *((UInt32*) pData) = GetBlockSize();
      }
      return noErr;
    }
    NO_OP(kAudioUnitProperty_SetExternalBuffer);         // 15,
    case kAudioUnitProperty_ParameterValueStrings:       // 16,
    {
      ASSERT_SCOPE(kAudioUnitScope_Global);
      ASSERT_ELEMENT(NParams());
      WDL_MutexLock lock(&mParams_mutex);
      IParam* pParam = GetParam(element);
      int n = pParam->GetNDisplayTexts();
      if (!n)
      {
        *pDataSize = 0;
        return kAudioUnitErr_InvalidProperty;
      }
      *pDataSize = sizeof(CFArrayRef);
      if (pData)
      {
        CFMutableArrayRef nameArray = CFArrayCreateMutable(kCFAllocatorDefault, n, &kCFTypeArrayCallBacks);
        for (int i = 0; i < n; ++i)
        {
          const char* str = pParam->GetDisplayText(i);
          CFStrLocal cfstr = CFStrLocal(str);
          CFArrayAppendValue(nameArray, cfstr.mCFStr);
        }
        *((CFArrayRef*) pData) = nameArray;
      }
      return noErr;
    }
    case kAudioUnitProperty_GetUIComponentList:          // 18,
    {
      if (GetHasUI())
      {
        *pDataSize = sizeof(AudioComponentDescription);
        if (pData)
        {
          AudioComponentDescription* pDesc = (AudioComponentDescription*) pData;
          pDesc->componentType = kAudioUnitCarbonViewComponentType;
          pDesc->componentSubType = GetUniqueID();
          pDesc->componentManufacturer = GetMfrID();
          pDesc->componentFlags = 0;
          pDesc->componentFlagsMask = 0;
        }
        return noErr;
      }
      return kAudioUnitErr_InvalidProperty;
    }
    case kAudioUnitProperty_AudioChannelLayout:
    {
      return kAudioUnitErr_InvalidPropertyValue; // TODO: this seems wrong but works
    }
    case kAudioUnitProperty_TailTime:                    // 20,   // listenable
    {
      ASSERT_SCOPE(kAudioUnitScope_Global);
      *pWriteable = GetTailSize() > 0;
      *pDataSize = sizeof(Float64);
      
      if (pData)
      {
        *((Float64*) pData) = (double) GetTailSize() / GetSampleRate();
      }
      return noErr;
    }
    case kAudioUnitProperty_BypassEffect:                // 21,
    {
      ASSERT_SCOPE(kAudioUnitScope_Global);
      *pWriteable = true;
      *pDataSize = sizeof(UInt32);
      if (pData)
      {
        *((UInt32*) pData) = (mIsBypassed ? 1 : 0);
      }
      return noErr;
    }
    case kAudioUnitProperty_LastRenderError:             // 22,
    {
      ASSERT_SCOPE(kAudioUnitScope_Global);
      *pDataSize = sizeof(OSStatus);
      if (pData)
      {
        *((OSStatus*) pData) = noErr;
      }
      return noErr;
    }
    case kAudioUnitProperty_SetRenderCallback:           // 23,
    {
      ASSERT_INPUT_OR_GLOBAL_SCOPE;
      if (element >= mInBuses.GetSize())
      {
        return kAudioUnitErr_InvalidProperty;
      }
      *pDataSize = sizeof(AURenderCallbackStruct);
      *pWriteable = true;
      return noErr;
    }
    case kAudioUnitProperty_FactoryPresets:              // 24,   // listenable
    {
      *pDataSize = sizeof(CFArrayRef);
      if (pData)
      {
        int i, n = NPresets();
        CFMutableArrayRef presetArray = CFArrayCreateMutable(kCFAllocatorDefault, n, &kCFAUPresetArrayCallBacks);

        if (presetArray == NULL)
          return coreFoundationUnknownErr;

        for (i = 0; i < n; ++i)
        {
          CFStrLocal presetName = CFStrLocal(GetPresetName(i));
          CFAUPresetRef newPreset = CFAUPresetCreate(kCFAllocatorDefault, i, presetName.mCFStr); // todo should i be 0 based?

          if (newPreset != NULL)
          {
            CFArrayAppendValue(presetArray, newPreset);
            CFAUPresetRelease(newPreset);
          }
        }

        *((CFMutableArrayRef*) pData) = presetArray;
      }
      return noErr;
    }
    NO_OP(kAudioUnitProperty_ContextName);                // 25,
    NO_OP(kAudioUnitProperty_RenderQuality);              // 26,
    case kAudioUnitProperty_HostCallbacks:                // 27,
    {
      ASSERT_SCOPE(kAudioUnitScope_Global);
      *pDataSize = sizeof(HostCallbackInfo);
      *pWriteable = true;
      return noErr;
    }
    NO_OP(kAudioUnitProperty_InPlaceProcessing);          // 29,
    case kAudioUnitProperty_ElementName:
    {
      *pDataSize = sizeof(CFStringRef);
      *pWriteable = false;
      if (pData)
      {
        switch(scope)
        {
          case kAudioUnitScope_Input:
          {
            if (element == 0 || element == 1)
            {
              *(CFStringRef *)pData = MakeCFString(GetInputBusLabel(element)->Get());
              return noErr;
            }
            else
              return kAudioUnitErr_InvalidElement;
          }
          case kAudioUnitScope_Output:
          {
            *(CFStringRef *)pData = MakeCFString(GetOutputBusLabel(element)->Get());
            return noErr;
          }
          default:
            return kAudioUnitErr_InvalidScope;
        }
      }
      return kAudioUnitErr_InvalidProperty;
    }
    case kAudioUnitProperty_CocoaUI:                      // 31,
    {
      if (GetHasUI()) // this won't work < 10.5 SDK but barely anyone will use that these days
      {
        *pDataSize = sizeof(AudioUnitCocoaViewInfo);  // Just one view.
        if (pData)
        {
          AudioUnitCocoaViewInfo* pViewInfo = (AudioUnitCocoaViewInfo*) pData;
          CFStrLocal bundleID(mOSXBundleID.Get());
          CFBundleRef pBundle = CFBundleGetBundleWithIdentifier(bundleID.mCFStr);
          CFURLRef url = CFBundleCopyBundleURL(pBundle);
          pViewInfo->mCocoaAUViewBundleLocation = url;
          pViewInfo->mCocoaAUViewClass[0] = CFStringCreateWithCString(0, mCocoaViewFactoryClassName.Get(), kCFStringEncodingUTF8);
        }
        return noErr;
      }
      return kAudioUnitErr_InvalidProperty;
    }
    case kAudioUnitProperty_SupportedChannelLayoutTags:
    {
      // kAudioUnitProperty_SupportedChannelLayoutTags is only needed for multi-output bus instruments
      if (!IsInst() || (mOutBuses.GetSize()==1))
        return kAudioUnitErr_InvalidProperty;

      if (!pData) // GetPropertyInfo
      {
        UInt32 numLayouts = GetChannelLayoutTags(scope, element, NULL);

        if (numLayouts)
        {
          *pDataSize = numLayouts * sizeof(AudioChannelLayoutTag);
          *pWriteable = true;
          return noErr;
        }
      }
      else // GetProperty
      {
        AudioChannelLayoutTag* ptr = pData ? static_cast<AudioChannelLayoutTag*>(pData) : NULL;

        if (GetChannelLayoutTags(scope, element, ptr))
        {
          return noErr;
        }
      }

      return kAudioUnitErr_InvalidProperty;
    }
    case kAudioUnitProperty_ParameterIDName:             // 34,
    {
      *pDataSize = sizeof(AudioUnitParameterIDName);
      if (pData && scope == kAudioUnitScope_Global)
      {
        AudioUnitParameterIDName* pIDName = (AudioUnitParameterIDName*) pData;
        WDL_MutexLock lock(&mParams_mutex);
        IParam* pParam = GetParam(pIDName->inID);
        char cStr[MAX_PARAM_NAME_LEN];
        strcpy(cStr, pParam->GetNameForHost());
        if (pIDName->inDesiredLength != kAudioUnitParameterName_Full)
        {
          int n = std::min<int>(MAX_PARAM_NAME_LEN - 1, pIDName->inDesiredLength);
          cStr[n] = '\0';
        }
        pIDName->outName = CFStringCreateWithCString(0, cStr, kCFStringEncodingUTF8);
      }
      return noErr;
    }
    case kAudioUnitProperty_ParameterClumpName:          // 35,
    {
      *pDataSize = sizeof (AudioUnitParameterNameInfo);
      if (pData && scope == kAudioUnitScope_Global)
      {
        AudioUnitParameterNameInfo* parameterNameInfo = (AudioUnitParameterNameInfo *) pData;
        int clumpId = parameterNameInfo->inID;
        
        if (clumpId < 1)
          return kAudioUnitErr_PropertyNotInUse;
        
        parameterNameInfo->outName = CFStringCreateWithCString(0, mParamGroups.Get(clumpId-1), kCFStringEncodingUTF8);
      }
      return noErr;
    }
    case kAudioUnitProperty_CurrentPreset:               // 28,
    case kAudioUnitProperty_PresentPreset:               // 36,       // listenable
    {
      *pDataSize = sizeof(AUPreset);
      *pWriteable = true;
      if (pData)
      {
        AUPreset* pAUPreset = (AUPreset*) pData;
        pAUPreset->presetNumber = GetCurrentPresetIdx();
        const char* name = GetPresetName(pAUPreset->presetNumber);
        pAUPreset->presetName = CFStringCreateWithCString(0, name, kCFStringEncodingUTF8);
      }
      return noErr;
    }
    NO_OP(kAudioUnitProperty_OfflineRender);             // 37,
    case kAudioUnitProperty_ParameterStringFromValue:     // 33,
    {
      *pDataSize = sizeof(AudioUnitParameterStringFromValue);
      if (pData && scope == kAudioUnitScope_Global)
      {
        AudioUnitParameterStringFromValue* pSFV = (AudioUnitParameterStringFromValue*) pData;
        WDL_MutexLock lock(&mParams_mutex);
        IParam* pParam = GetParam(pSFV->inParamID);
        
        pParam->GetDisplayForHost(*(pSFV->inValue), false, mParamValueString);
        pSFV->outString = MakeCFString((const char*) mParamValueString);
      }
      return noErr;
    }
    case kAudioUnitProperty_ParameterValueFromString:     // 38,
    {
      *pDataSize = sizeof(AudioUnitParameterValueFromString);
      if (pData)
      {
        AudioUnitParameterValueFromString* pVFS = (AudioUnitParameterValueFromString*) pData;
        if (scope == kAudioUnitScope_Global)
        {
          CStrLocal cStr(pVFS->inString);
          WDL_MutexLock lock(&mParams_mutex);
          IParam* pParam = GetParam(pVFS->inParamID);
          if (pParam->GetNDisplayTexts())
          {
            int v;
            if (pParam->MapDisplayText(cStr.mCStr, &v))
              pVFS->outValue = (AudioUnitParameterValue) v;
          }
          else
          {
            double v = atof(cStr.mCStr);
            if (pParam->GetDisplayIsNegated()) v = -v;
            pVFS->outValue = (AudioUnitParameterValue) v;
          }
        }
      }
      return noErr;
    }
    NO_OP(kAudioUnitProperty_IconLocation);               // 39,
    NO_OP(kAudioUnitProperty_PresentationLatency);        // 40,
    NO_OP(kAudioUnitProperty_DependentParameters);        // 45,
    case kMusicDeviceProperty_InstrumentCount:
    {
      ASSERT_SCOPE(kAudioUnitScope_Global);
      if (IsInst())
      {
        *pDataSize = sizeof(UInt32);
        if (pData)
        {
          *((UInt32*) pData) = 1; //(mIsBypassed ? 1 : 0);
        }
        return noErr;
      }
      else
      {
        return kAudioUnitErr_InvalidProperty;
      }
    }

#if MAC_OS_X_VERSION_MAX_ALLOWED > 1040
    NO_OP(kAudioUnitProperty_AUHostIdentifier);           // 46,
    NO_OP(kAudioUnitProperty_MIDIOutputCallbackInfo);     // 47,
    NO_OP(kAudioUnitProperty_MIDIOutputCallback);         // 48,
    NO_OP(kAudioUnitProperty_InputSamplesInOutput);       // 49,
    NO_OP(kAudioUnitProperty_ClassInfoFromDocument);      // 50
#endif

    default:
    {
      return kAudioUnitErr_InvalidProperty;
    }
  }
}

OSStatus IPlugAU::SetProperty(AudioUnitPropertyID propID, AudioUnitScope scope, AudioUnitElement element,
                                     UInt32* pDataSize, const void* pData)
{
  Trace(TRACELOC, "(%d:%s):(%d:%s):%d", propID, AUPropertyStr(propID), scope, AUScopeStr(scope), element);

  InformListeners(propID, scope);

  switch (propID)
  {
    case kAudioUnitProperty_ClassInfo:                  // 0,
    {
      return SetState(*((CFPropertyListRef*) pData));
    }
    case kAudioUnitProperty_MakeConnection:              // 1,
    {
      ASSERT_INPUT_OR_GLOBAL_SCOPE;
      AudioUnitConnection* pAUC = (AudioUnitConnection*) pData;
      if (pAUC->destInputNumber >= mInBusConnections.GetSize())
      {
        return kAudioUnitErr_InvalidProperty;
      }
      InputBusConnection* pInBusConn = mInBusConnections.Get(pAUC->destInputNumber);
      memset(pInBusConn, 0, sizeof(InputBusConnection));
      bool negotiatedOK = true;
      if (pAUC->sourceAudioUnit)      // Opening connection.
      {
        AudioStreamBasicDescription srcASBD;
        UInt32 size = sizeof(AudioStreamBasicDescription);
        negotiatedOK =   // Ask whoever is sending us audio what the format is.
          (AudioUnitGetProperty(pAUC->sourceAudioUnit, kAudioUnitProperty_StreamFormat,
                                kAudioUnitScope_Output, pAUC->sourceOutputNumber, &srcASBD, &size) == noErr);
        negotiatedOK &= // Try to set our own format to match.
          (SetProperty(kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input,
                       pAUC->destInputNumber, &size, &srcASBD) == noErr);
        if (negotiatedOK)     // Connection terms successfully negotiated.
        {
          pInBusConn->mUpstreamUnit = pAUC->sourceAudioUnit;
          pInBusConn->mUpstreamBusIdx = pAUC->sourceOutputNumber;
          // Will the upstream unit give us a fast render proc for input?
          AudioUnitRenderProc srcRenderProc;
          size = sizeof(AudioUnitRenderProc);
          if (AudioUnitGetProperty(pAUC->sourceAudioUnit, kAudioUnitProperty_FastDispatch, kAudioUnitScope_Global, kAudioUnitRenderSelect,
                                   &srcRenderProc, &size) == noErr)
          {
            // Yes, we got a fast render proc, and we also need to store the pointer to the upstream audio unit object.
            pInBusConn->mUpstreamRenderProc = srcRenderProc;
            pInBusConn->mUpstreamObj = GetComponentInstanceStorage(pAUC->sourceAudioUnit);
          }
          // Else no fast render proc, so leave the input bus connection struct's upstream render proc and upstream object empty,
          // and we will need to make a component call through the component manager to get input data.
        }
        // Else this is a call to close the connection, which we effectively did by clearing the InputBusConnection struct,
        // which counts as a successful negotiation.
      }
      AssessInputConnections();
      return (negotiatedOK ? noErr : (int) kAudioUnitErr_InvalidProperty);  // casting to int avoids gcc error
    }
    case kAudioUnitProperty_SampleRate:                  // 2,
    {
      SetSampleRate(*((Float64*) pData));
      Reset();
      return noErr;
    }
    NO_OP(kAudioUnitProperty_ParameterList);             // 3,
    NO_OP(kAudioUnitProperty_ParameterInfo);             // 4,
    NO_OP(kAudioUnitProperty_FastDispatch);              // 5,
    NO_OP(kAudioUnitProperty_CPULoad);                   // 6,
    case kAudioUnitProperty_StreamFormat:                // 8,
    {
      AudioStreamBasicDescription* pASBD = (AudioStreamBasicDescription*) pData;
      int nHostChannels = pASBD->mChannelsPerFrame;
      BusChannels* pBus = GetBus(scope, element);
      if (!pBus)
      {
        return kAudioUnitErr_InvalidProperty;
      }
      pBus->mNHostChannels = 0;
      // The connection is OK if the plugin expects the same number of channels as the host is attempting to connect,
      // or if the plugin supports mono channels (meaning it's flexible about how many inputs to expect)
      // and the plugin supports at least as many channels as the host is attempting to connect.
      bool connectionOK = (nHostChannels > 0);
      connectionOK &= CheckLegalIO(scope, element, nHostChannels);
      connectionOK &= (pASBD->mFormatID == kAudioFormatLinearPCM && pASBD->mFormatFlags & kAudioFormatFlagsCanonical);

      Trace(TRACELOC, "%d:%d:%s:%s:%s",
            nHostChannels, pBus->mNPlugChannels,
            (pASBD->mFormatID == kAudioFormatLinearPCM ? "linearPCM" : "notLinearPCM"),
            (pASBD->mFormatFlags & kAudioFormatFlagsCanonical ? "canonicalFormat" : "notCanonicalFormat"),
            (connectionOK ? "connectionOK" : "connectionNotOK"));

      // bool interleaved = !(pASBD->mFormatFlags & kAudioFormatFlagIsNonInterleaved);
      if (connectionOK)
      {
        pBus->mNHostChannels = nHostChannels;
        if (pASBD->mSampleRate > 0.0)
        {
          SetSampleRate(pASBD->mSampleRate);
        }
      }
      AssessInputConnections();
      return (connectionOK ? noErr : (int) kAudioUnitErr_InvalidProperty); // casting to int avoids gcc error
    }
    NO_OP(kAudioUnitProperty_ElementCount);              // 11,
    NO_OP(kAudioUnitProperty_Latency);                   // 12,
    NO_OP(kAudioUnitProperty_SupportedNumChannels);      // 13,
    case kAudioUnitProperty_MaximumFramesPerSlice:       // 14,
    {
      SetBlockSize(*((UInt32*) pData));
      Reset();
      return noErr;
    }
    NO_OP(kAudioUnitProperty_SetExternalBuffer);         // 15,
    NO_OP(kAudioUnitProperty_ParameterValueStrings);     // 16,
    NO_OP(kAudioUnitProperty_GetUIComponentList);        // 18,
    NO_OP(kAudioUnitProperty_AudioChannelLayout);        // 19, //TODO?: Set kAudioUnitProperty_AudioChannelLayout
    NO_OP(kAudioUnitProperty_TailTime);                  // 20,
    case kAudioUnitProperty_BypassEffect:                // 21,
    {
      mIsBypassed = (*((UInt32*) pData) != 0);
      OnActivate(!mIsBypassed);
      Reset();
      return noErr;
    }
    NO_OP(kAudioUnitProperty_LastRenderError);           // 22,
    case kAudioUnitProperty_SetRenderCallback:           // 23,
    {
      ASSERT_SCOPE(kAudioUnitScope_Input);    // if global scope, set all
      if (element >= mInBusConnections.GetSize())
      {
        return kAudioUnitErr_InvalidProperty;
      }
      InputBusConnection* pInBusConn = mInBusConnections.Get(element);
      memset(pInBusConn, 0, sizeof(InputBusConnection));
      AURenderCallbackStruct* pCS = (AURenderCallbackStruct*) pData;
      if (pCS->inputProc != 0)
      {
        pInBusConn->mUpstreamRenderCallback = *pCS;
      }
      AssessInputConnections();
      return noErr;
    }
    NO_OP(kAudioUnitProperty_FactoryPresets);            // 24,
    NO_OP(kAudioUnitProperty_ContextName);               // 25,
    NO_OP(kAudioUnitProperty_RenderQuality);             // 26,
    case kAudioUnitProperty_HostCallbacks:              // 27,
    {
      ASSERT_SCOPE(kAudioUnitScope_Global);
      memcpy(&mHostCallbacks, pData, sizeof(HostCallbackInfo));
      return noErr;
    }
    NO_OP(kAudioUnitProperty_InPlaceProcessing);         // 29,
    NO_OP(kAudioUnitProperty_ElementName);               // 30,
    NO_OP(kAudioUnitProperty_CocoaUI);                   // 31,
    NO_OP(kAudioUnitProperty_SupportedChannelLayoutTags); // 32,
    NO_OP(kAudioUnitProperty_ParameterIDName);           // 34,
    NO_OP(kAudioUnitProperty_ParameterClumpName);        // 35,
    case kAudioUnitProperty_CurrentPreset:               // 28,
    case kAudioUnitProperty_PresentPreset:               // 36,
    {
      int presetIdx = ((AUPreset*) pData)->presetNumber;
      RestorePreset(presetIdx);
      return noErr;
    }
    case kAudioUnitProperty_OfflineRender:                // 37,
    {
      mIsOffline = (*((UInt32*) pData) != 0);
      return noErr;
    }
    NO_OP(kAudioUnitProperty_ParameterStringFromValue);  // 33,
    NO_OP(kAudioUnitProperty_ParameterValueFromString);  // 38,
    NO_OP(kAudioUnitProperty_IconLocation);              // 39,
    NO_OP(kAudioUnitProperty_PresentationLatency);       // 40,
    NO_OP(kAudioUnitProperty_DependentParameters);       // 45,

#if MAC_OS_X_VERSION_MAX_ALLOWED > 1040
    case kAudioUnitProperty_AUHostIdentifier:            // 46,
    {
      AUHostIdentifier* pHostID = (AUHostIdentifier*) pData;
      CStrLocal hostStr(pHostID->hostName);
      int hostVer = (pHostID->hostVersion.majorRev << 16)
                    + ((pHostID->hostVersion.minorAndBugRev & 0xF0) << 4)
                    + ((pHostID->hostVersion.minorAndBugRev & 0x0F));

      SetHost(hostStr.mCStr, hostVer);
      OnHostIdentified();
      return noErr;
    }
    NO_OP(kAudioUnitProperty_MIDIOutputCallbackInfo);   // 47,
    NO_OP(kAudioUnitProperty_MIDIOutputCallback);       // 48,
    NO_OP(kAudioUnitProperty_InputSamplesInOutput);       // 49,
    NO_OP(kAudioUnitProperty_ClassInfoFromDocument)       // 50
#endif

    default:
    {
      return kAudioUnitErr_InvalidProperty;
    }
  }
}

const char* AUInputTypeStr(IPlugAU::EAUInputType type)
{
  switch (type)
  {
    case IPlugAU::eDirectFastProc:     return "DirectFastProc";
    case IPlugAU::eDirectNoFastProc:   return "DirectNoFastProc";
    case IPlugAU::eRenderCallback:     return "RenderCallback";
    case IPlugAU::eNotConnected:
    default:                           return "NotConnected";
  }
}

int IPlugAU::NHostChannelsConnected(WDL_PtrList<BusChannels>* pBuses, int excludeIdx)
{
  bool init = false;
  int nCh = 0, n = pBuses->GetSize();
  
  for (int i = 0; i < n; ++i)
  {
    if (i != excludeIdx)
    {
      int nHostChannels = pBuses->Get(i)->mNHostChannels;
      if (nHostChannels >= 0)
      {
        nCh += nHostChannels;
        init = true;
      }
    }
  }
  
  if (init)
  {
    return nCh;
  }
  
  return -1;
}

bool IPlugAU::CheckLegalIO(AudioUnitScope scope, int busIdx, int nChannels)
{
  if (scope == kAudioUnitScope_Input)
  {
    int nIn = std::max(NHostChannelsConnected(&mInBuses, busIdx), 0);
    int nOut = (mActive ? NHostChannelsConnected(&mOutBuses) : -1);
    return LegalIO(nIn + nChannels, nOut);
  }
  else
  {
    int nIn = (mActive ? NHostChannelsConnected(&mInBuses) : -1);
    int nOut = std::max(NHostChannelsConnected(&mOutBuses, busIdx), 0);
    return LegalIO(nIn, nOut + nChannels);
  }
}

bool IPlugAU::CheckLegalIO()
{
  int nIn = NHostChannelsConnected(&mInBuses);
  int nOut = NHostChannelsConnected(&mOutBuses);
  return ((!nIn && !nOut) || LegalIO(nIn, nOut));
}

void IPlugAU::AssessInputConnections()
{
  TRACE;
  SetInputChannelConnections(0, NInChannels(), false);

  int nIn = mInBuses.GetSize();
  for (int i = 0; i < nIn; ++i)
  {
    BusChannels* pInBus = mInBuses.Get(i);
    InputBusConnection* pInBusConn = mInBusConnections.Get(i);

    // AU supports 3 ways to get input from the host (or whoever is upstream).
    if (pInBusConn->mUpstreamRenderProc && pInBusConn->mUpstreamObj)
    {
      // 1: direct input connection with fast render proc (and buffers) supplied by the upstream unit.
      pInBusConn->mInputType = eDirectFastProc;
    }
    else if (pInBusConn->mUpstreamUnit)
    {
      // 2: direct input connection with no render proc, buffers supplied by the upstream unit.
      pInBusConn->mInputType = eDirectNoFastProc;
    }
    else if (pInBusConn->mUpstreamRenderCallback.inputProc)
    {
      // 3: no direct connection, render callback, buffers supplied by us.
      pInBusConn->mInputType = eRenderCallback;
    }
    else
    {
      pInBusConn->mInputType = eNotConnected;
    }
    pInBus->mConnected = (pInBusConn->mInputType != eNotConnected);

    int startChannelIdx = pInBus->mPlugChannelStartIdx;
    if (pInBus->mConnected)
    {
      // There's an input connection, so we need to tell the plug to expect however many channels
      // are in the negotiated host stream format.
      if (pInBus->mNHostChannels < 0)
      {
        // The host set up a connection without specifying how many channels in the stream.
        // Assume the host will send all the channels the plugin asks for, and hope for the best.
        Trace(TRACELOC, "AssumeChannels:%d", pInBus->mNPlugChannels);
        pInBus->mNHostChannels = pInBus->mNPlugChannels;
      }
      int nConnected = pInBus->mNHostChannels;
      int nUnconnected = std::max(pInBus->mNPlugChannels - nConnected, 0);
      SetInputChannelConnections(startChannelIdx, nConnected, true);
      SetInputChannelConnections(startChannelIdx + nConnected, nUnconnected, false);
    }

    Trace(TRACELOC, "%d:%s:%d:%d:%d", i, AUInputTypeStr(pInBusConn->mInputType), startChannelIdx, pInBus->mNPlugChannels, pInBus->mNHostChannels);
  }
}

inline void PutNumberInDict(CFMutableDictionaryRef pDict, const char* key, void* pNumber, CFNumberType type)
{
  CFStrLocal cfKey(key);
  CFNumberRef pValue = CFNumberCreate(0, type, pNumber);
  CFDictionarySetValue(pDict, cfKey.mCFStr, pValue);
  CFRelease(pValue);
}

inline void PutStrInDict(CFMutableDictionaryRef pDict, const char* key, const char* value)
{
  CFStrLocal cfKey(key);
  CFStrLocal cfValue(value);
  CFDictionarySetValue(pDict, cfKey.mCFStr, cfValue.mCFStr);
}

inline void PutDataInDict(CFMutableDictionaryRef pDict, const char* key, ByteChunk* pChunk)
{
  CFStrLocal cfKey(key);
  CFDataRef pData = CFDataCreate(0, pChunk->GetBytes(), pChunk->Size());
  CFDictionarySetValue(pDict, cfKey.mCFStr, pData);
  CFRelease(pData);
}

inline bool GetNumberFromDict(CFDictionaryRef pDict, const char* key, void* pNumber, CFNumberType type)
{
  CFStrLocal cfKey(key);
  CFNumberRef pValue = (CFNumberRef) CFDictionaryGetValue(pDict, cfKey.mCFStr);
  if (pValue)
  {
    CFNumberGetValue(pValue, type, pNumber);
    return true;
  }
  return false;
}

inline bool GetStrFromDict(CFDictionaryRef pDict, const char* key, char* value)
{
  CFStrLocal cfKey(key);
  CFStringRef pValue = (CFStringRef) CFDictionaryGetValue(pDict, cfKey.mCFStr);
  if (pValue)
  {
    CStrLocal cStr(pValue);
    strcpy(value, cStr.mCStr);
    return true;
  }
  value[0] = '\0';
  return false;
}

inline bool GetDataFromDict(CFDictionaryRef pDict, const char* key, ByteChunk* pChunk)
{
  CFStrLocal cfKey(key);
  CFDataRef pData = (CFDataRef) CFDictionaryGetValue(pDict, cfKey.mCFStr);
  if (pData)
  {
    CFIndex n = CFDataGetLength(pData);
    pChunk->Resize((int) n);
    memcpy(pChunk->GetBytes(), CFDataGetBytePtr(pData), n);
    return true;
  }
  return false;
}

OSStatus IPlugAU::GetState(CFPropertyListRef* ppPropList)
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
  AudioComponentDescription cd;
  AudioComponent comp = AudioComponentInstanceGetComponent(mCI);
  OSStatus r = AudioComponentGetDescription(comp, &cd);
//#elif MAC_OS_X_VERSION_MAX_ALLOWED == 1060
//  ComponentDescription cd;
//  OSStatus r = GetComponentInfo((Component) mCI, &cd, 0, 0, 0);
#else
  AudioComponentDescription cd;
  OSStatus r = GetComponentInfo((Component) mCI, &cd, 0, 0, 0);
#endif
  
  if (r != noErr)
  {
    return r;
  }

  CFMutableDictionaryRef pDict = CFDictionaryCreateMutable(0, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  int version = GetEffectVersion(false);
  PutNumberInDict(pDict, kAUPresetVersionKey, &version, kCFNumberSInt32Type);
  PutNumberInDict(pDict, kAUPresetTypeKey, &(cd.componentType), kCFNumberSInt32Type);
  PutNumberInDict(pDict, kAUPresetSubtypeKey, &(cd.componentSubType), kCFNumberSInt32Type);
  PutNumberInDict(pDict, kAUPresetManufacturerKey, &(cd.componentManufacturer), kCFNumberSInt32Type);
  PutStrInDict(pDict, kAUPresetNameKey, GetPresetName(GetCurrentPresetIdx()));

  ByteChunk chunk;

  if (SerializeState(chunk))
  {
    PutDataInDict(pDict, kAUPresetDataKey, &chunk);
  }

  *ppPropList = pDict;
  TRACE;
  return noErr;
}

OSStatus IPlugAU::SetState(CFPropertyListRef pPropList)
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
  AudioComponentDescription cd;
  AudioComponent comp = AudioComponentInstanceGetComponent(mCI);
  OSStatus r = AudioComponentGetDescription(comp, &cd);
//#elif MAC_OS_X_VERSION_MAX_ALLOWED == 1060
//  ComponentDescription cd;
//  OSStatus r = GetComponentInfo((Component) mCI, &cd, 0, 0, 0);
#else
  AudioComponentDescription cd;
  OSStatus r = GetComponentInfo((Component) mCI, &cd, 0, 0, 0);
#endif
  
  if (r != noErr)
  {
    return r;
  }

  CFDictionaryRef pDict = (CFDictionaryRef) pPropList;
  int version, type, subtype, mfr;
  char presetName[64];
  if (!GetNumberFromDict(pDict, kAUPresetVersionKey, &version, kCFNumberSInt32Type) ||
      !GetNumberFromDict(pDict, kAUPresetTypeKey, &type, kCFNumberSInt32Type) ||
      !GetNumberFromDict(pDict, kAUPresetSubtypeKey, &subtype, kCFNumberSInt32Type) ||
      !GetNumberFromDict(pDict, kAUPresetManufacturerKey, &mfr, kCFNumberSInt32Type) ||
      !GetStrFromDict(pDict, kAUPresetNameKey, presetName) ||
      //version != GetEffectVersion(false) ||
      type != cd.componentType ||
      subtype != cd.componentSubType ||
      mfr != cd.componentManufacturer)
  {
    return kAudioUnitErr_InvalidPropertyValue;
  }
  
  RestorePreset(presetName);

  ByteChunk chunk;
  if (!GetDataFromDict(pDict, kAUPresetDataKey, &chunk))
  {
    return kAudioUnitErr_InvalidPropertyValue;
  }

  if (!UnserializeState(chunk, 0))
  {
    return kAudioUnitErr_InvalidPropertyValue;
  }

  RedrawParamControls();
  return noErr;
}

// pData == 0 means return property info only.
OSStatus IPlugAU::GetProc(AudioUnitElement element, UInt32* pDataSize, void* pData)
{
  Trace(TRACELOC, "%s:(%d:%s)", (pData ? "" : "Info"), element, AUSelectStr(element));

  switch (element)
  {
      //TODO: - WHAT ABOUT THESE!!!
//    case kAudioUnitGetParameterSelect:
//    {
//      *pDataSize = sizeof(AudioUnitGetParameterProc);
//      if (pData)
//      {
//        *((AudioUnitGetParameterProc*) pData) = (AudioUnitGetParameterProc) IPlugAU::GetParamProc;
//      }
//      return noErr;
//    }
//    case kAudioUnitSetParameterSelect:
//    {
//      *pDataSize = sizeof(AudioUnitSetParameterProc);
//      if (pData)
//      {
//        *((AudioUnitSetParameterProc*) pData) = (AudioUnitSetParameterProc) IPlugAU::SetParamProc;
//      }
//      return noErr;
//    }
//    case kAudioUnitRenderSelect:
//    {
//      *pDataSize = sizeof(AudioUnitRenderProc);
//      if (pData)
//      {
//        *((AudioUnitRenderProc*) pData) = (AudioUnitRenderProc) IPlugAU::RenderProc;
//      }
//      return noErr;
//    }
    default:
      return kAudioUnitErr_InvalidElement;
  }
}

// static
OSStatus IPlugAU::GetParamProc(void* pPlug, AudioUnitParameterID paramID, AudioUnitScope scope, AudioUnitElement element, AudioUnitParameterValue* pValue)
{
  Trace(TRACELOC, "%d:(%d:%s):%d", paramID, scope, AUScopeStr(scope), element);

  ASSERT_SCOPE(kAudioUnitScope_Global);
  IPlugAU* _this = (IPlugAU*) pPlug;
  assert(_this != NULL);
  WDL_MutexLock lock(&_this->mParams_mutex);
  *pValue = _this->GetParam(paramID)->Value();
  return noErr;
}

// static
OSStatus IPlugAU::SetParamProc(void* pPlug, AudioUnitParameterID paramID, AudioUnitScope scope, AudioUnitElement element, AudioUnitParameterValue value, UInt32 offsetFrames)
{
  Trace(TRACELOC, "%d:(%d:%s):%d", paramID, scope, AUScopeStr(scope), element);

  // In the SDK, offset frames is only looked at in group scope.
  ASSERT_SCOPE(kAudioUnitScope_Global);
  IPlugAU* _this = (IPlugAU*) pPlug;
  WDL_MutexLock lock(&_this->mParams_mutex);
  IParam* pParam = _this->GetParam(paramID);
  pParam->Set(value);
  _this->SetParameterInUIFromAPI(paramID, value, false);
  _this->OnParamChange(paramID);
  return noErr;
}

struct BufferList
{
  int mNumberBuffers;
  AudioBuffer mBuffers[MAX_IO_CHANNELS];
};

inline OSStatus RenderCallback(AURenderCallbackStruct* pCB, AudioUnitRenderActionFlags* pFlags, const AudioTimeStamp* pTimestamp, UInt32 inputBusIdx, UInt32 nFrames, AudioBufferList* pOutBufList)
{
  TRACE_PROCESS;

  return pCB->inputProc(pCB->inputProcRefCon, pFlags, pTimestamp, inputBusIdx, nFrames, pOutBufList);
}

// static
OSStatus IPlugAU::RenderProc(void* pPlug, AudioUnitRenderActionFlags* pFlags, const AudioTimeStamp* pTimestamp,
                                    UInt32 outputBusIdx, UInt32 nFrames, AudioBufferList* pOutBufList)
{
  TRACE_PROCESS(TRACELOC, "%d:%d:%d", outputBusIdx, pOutBufList->mNumberBuffers, nFrames);

  IPlugAU* _this = (IPlugAU*) pPlug;

  if (!(pTimestamp->mFlags & kAudioTimeStampSampleTimeValid) || outputBusIdx >= _this->mOutBuses.GetSize() || nFrames > _this->GetBlockSize())
  {
    return kAudioUnitErr_InvalidPropertyValue;
  }

  int nRenderNotify = _this->mRenderNotify.GetSize();

  if (nRenderNotify)
  {
    for (int i = 0; i < nRenderNotify; ++i)
    {
      AURenderCallbackStruct* pRN = _this->mRenderNotify.Get(i);
      AudioUnitRenderActionFlags flags = kAudioUnitRenderAction_PreRender;
      RenderCallback(pRN, &flags, pTimestamp, outputBusIdx, nFrames, pOutBufList);
    }
  }

  double renderTimestamp = pTimestamp->mSampleTime;

  // Pull input buffers.
  if (renderTimestamp != _this->mRenderTimestamp)
  {
    BufferList bufList;
    AudioBufferList* pInBufList = (AudioBufferList*) &bufList;

    int nIn = _this->mInBuses.GetSize();

    for (int i = 0; i < nIn; ++i)
    {
      BusChannels* pInBus = _this->mInBuses.Get(i);
      InputBusConnection* pInBusConn = _this->mInBusConnections.Get(i);

      if (pInBus->mConnected)
      {
        pInBufList->mNumberBuffers = pInBus->mNHostChannels;

        for (int b = 0; b < pInBufList->mNumberBuffers; ++b)
        {
          AudioBuffer* pBuffer = &(pInBufList->mBuffers[b]);
          pBuffer->mNumberChannels = 1;
          pBuffer->mDataByteSize = nFrames * sizeof(AudioSampleType);
          pBuffer->mData = 0;
        }

        AudioUnitRenderActionFlags flags = 0;
        OSStatus r = noErr;

        switch (pInBusConn->mInputType)
        {
          case eDirectFastProc:
          {
            r = pInBusConn->mUpstreamRenderProc(pInBusConn->mUpstreamObj, &flags, pTimestamp, pInBusConn->mUpstreamBusIdx, nFrames, pInBufList);
            break;
          }
          case eDirectNoFastProc:
          {
            r = AudioUnitRender(pInBusConn->mUpstreamUnit, &flags, pTimestamp, pInBusConn->mUpstreamBusIdx, nFrames, pInBufList);
            break;
          }
          case eRenderCallback:
          {
            AudioSampleType* pScratchInput = _this->mInScratchBuf.Get() + pInBus->mPlugChannelStartIdx * nFrames;

            for (int b = 0; b < pInBufList->mNumberBuffers; ++b, pScratchInput += nFrames)
            {
              pInBufList->mBuffers[b].mData = pScratchInput;
            }

            r = RenderCallback(&(pInBusConn->mUpstreamRenderCallback), &flags, pTimestamp, i, nFrames, pInBufList);
            break;
          }
          default:
            break;
        }
        if (r != noErr)
        {
          return r;   // Something went wrong upstream.
        }

        for (int i = 0, chIdx = pInBus->mPlugChannelStartIdx; i < pInBus->mNHostChannels; ++i, ++chIdx)
        {
          _this->AttachInputBuffers(chIdx, 1, (AudioSampleType**) &(pInBufList->mBuffers[i].mData), nFrames);
        }
      }
    }
    _this->mRenderTimestamp = renderTimestamp;
  }

  BusChannels* pOutBus = _this->mOutBuses.Get(outputBusIdx);

  // if this bus is not connected OR the number of buffers that the host has given are not equal to the number the bus expects
  if (!(pOutBus->mConnected) || pOutBus->mNHostChannels != pOutBufList->mNumberBuffers)
  {
    int startChannelIdx = pOutBus->mPlugChannelStartIdx;
    int nConnected = std::min<int>(pOutBus->mNHostChannels, pOutBufList->mNumberBuffers);
    int nUnconnected = std::max(pOutBus->mNPlugChannels - nConnected, 0);
    _this->SetOutputChannelConnections(startChannelIdx, nConnected, true);
    _this->SetOutputChannelConnections(startChannelIdx + nConnected, nUnconnected, false); // This will disconnect the right handle channel on a single stereo bus
    pOutBus->mConnected = true;
  }

  for (int i = 0, chIdx = pOutBus->mPlugChannelStartIdx; i < pOutBufList->mNumberBuffers; ++i, ++chIdx)
  {
    if (!(pOutBufList->mBuffers[i].mData)) // Downstream unit didn't give us buffers.
    {
      pOutBufList->mBuffers[i].mData = _this->mOutScratchBuf.Get() + chIdx * nFrames;
    }
    _this->AttachOutputBuffers(chIdx, 1, (AudioSampleType**) &(pOutBufList->mBuffers[i].mData));
  }

  int lastConnectedOutputBus = -1;

  for(int i = 0; i < _this->mOutBuses.GetSize(); i++)
  {
    if(!_this->mOutBuses.Get(i)->mConnected)
    {
      break;
    }
    else
    {
      lastConnectedOutputBus++;
    }
  }

  if (outputBusIdx == lastConnectedOutputBus)
  {
    int busIdx1based = outputBusIdx+1;

    if (busIdx1based < _this->mOutBuses.GetSize() /*&& (_this->GetHost() != kHostAbletonLive)*/)
    {
      int totalNumChans = _this->mOutBuses.GetSize() * 2; // stereo only for the time being
      int nConnected = busIdx1based * 2;
      _this->SetOutputChannelConnections(nConnected, totalNumChans - nConnected, false); // this will disconnect the channels that are on the unconnected buses
    }

    if (_this->mIsBypassed)
    {
      _this->PassThroughBuffers((AudioSampleType) 0, nFrames);
    }
    else
    {
      _this->ProcessBuffers((AudioSampleType) 0, nFrames);
    }
  }

  if (nRenderNotify)
  {
    for (int i = 0; i < nRenderNotify; ++i)
    {
      AURenderCallbackStruct* pRN = _this->mRenderNotify.Get(i);
      AudioUnitRenderActionFlags flags = kAudioUnitRenderAction_PostRender;
      RenderCallback(pRN, &flags, pTimestamp, outputBusIdx, nFrames, pOutBufList);
    }
  }

  return noErr;
}

IPlugAU::BusChannels* IPlugAU::GetBus(AudioUnitScope scope, AudioUnitElement busIdx)
{
  if (scope == kAudioUnitScope_Input && busIdx < mInBuses.GetSize())
  {
    return mInBuses.Get(busIdx);
  }
  if (scope == kAudioUnitScope_Output && busIdx < mOutBuses.GetSize())
  {
    return mOutBuses.Get(busIdx);
  }
  // Global bus is an alias for output bus zero.
  if (scope == kAudioUnitScope_Global && mOutBuses.GetSize())
  {
    return mOutBuses.Get(busIdx);
  }
  return 0;
}

void IPlugAU::ClearConnections()
{
  int nInBuses = mInBuses.GetSize();
  for (int i = 0; i < nInBuses; ++i)
  {
    BusChannels* pInBus = mInBuses.Get(i);
    pInBus->mConnected = false;
    pInBus->mNHostChannels = -1;
    InputBusConnection* pInBusConn = mInBusConnections.Get(i);
    memset(pInBusConn, 0, sizeof(InputBusConnection));
  }
  int nOutBuses = mOutBuses.GetSize();
  for (int i = 0; i < nOutBuses; ++i)
  {
    BusChannels* pOutBus = mOutBuses.Get(i);
    pOutBus->mConnected = false;
    pOutBus->mNHostChannels = -1;
  }
}
#pragma mark IPlugAU Constructor

IPlugAU::IPlugAU(IPlugInstanceInfo instanceInfo,
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
  : IPLUG_BASE_CLASS(nParams,
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
              kAPIAU)
  , mCI(0)
  , mIsOffline(false)
  , mRenderTimestamp(-1.0)
  , mTempo(DEFAULT_TEMPO)
  , mActive(false)
{
  Trace(TRACELOC, "%s", effectName);

  memset(&mHostCallbacks, 0, sizeof(HostCallbackInfo));
  memset(&mMidiCallback, 0, sizeof(AUMIDIOutputCallbackStruct));
  memset(mParamValueString, 0, MAX_PARAM_DISPLAY_LEN * sizeof(char));

  mOSXBundleID.Set(instanceInfo.mOSXBundleID.Get());
  mCocoaViewFactoryClassName.Set(instanceInfo.mCocoaViewFactoryClassName.Get());

  if (plugScChans && NInChannels()) // effect with side chain input... 2 input buses
  {
    int nNonScInputChans = NInChannels() - plugScChans;

    PtrListInitialize(&mInBusConnections, 2);
    PtrListInitialize(&mInBuses, 2);

    BusChannels* pInBus = mInBuses.Get(0);
    pInBus->mNHostChannels = -1;
    pInBus->mPlugChannelStartIdx = 0;
    pInBus->mNPlugChannels = nNonScInputChans;

    BusChannels* pInBus2 = mInBuses.Get(1);
    pInBus2->mNHostChannels = -1;
    pInBus2->mPlugChannelStartIdx = nNonScInputChans;
    pInBus2->mNPlugChannels = plugScChans;

    SetInputBusLabel(0, "main input");
    SetInputBusLabel(1, "aux input");
  }
  else if (NInChannels()) // effect with no side chain... 1 bus
  {
    PtrListInitialize(&mInBusConnections, 1);
    PtrListInitialize(&mInBuses, 1);

    BusChannels* pInBus = mInBuses.Get(0);
    pInBus->mNHostChannels = -1;
    pInBus->mPlugChannelStartIdx = 0;
    pInBus->mNPlugChannels = NInChannels();

    SetInputBusLabel(0, "input");
    SetInputBusLabel(1, "aux input"); // Ableton Live seems to think a 4-2 audiounit has a sidechain input, even if it is not meant to, so name it just in case
  }
  else   // synth = no inputs // TODO: support synths with SC inputs?
  {
    PtrListInitialize(&mInBusConnections, 0);
    PtrListInitialize(&mInBuses, 0);
  }

  if(plugIsInst) // TODO: support instruments with multichannel outputs, i.e. 5.1?
  {
    int nOutBuses = (int) ceil(NOutChannels() / 2.);

    PtrListInitialize(&mOutBuses, nOutBuses);
    char label[32];

    for (int i = 0, startCh = 0; i < nOutBuses; ++i, startCh += 2)
    {
      BusChannels* pOutBus = mOutBuses.Get(i);
      pOutBus->mNHostChannels = -1;
      pOutBus->mPlugChannelStartIdx = startCh;
      pOutBus->mNPlugChannels = std::min(NOutChannels() - startCh, 2);

      sprintf(label, "output %i", i+1);
      SetOutputBusLabel(i, label);
    }
  }
  else
  {
    // one output bus
    int nOutBuses = 1;
    PtrListInitialize(&mOutBuses, nOutBuses);

    BusChannels* pOutBus = mOutBuses.Get(0);
    pOutBus->mNHostChannels = -1;
    pOutBus->mPlugChannelStartIdx = 0;
    pOutBus->mNPlugChannels = NOutChannels();

    SetOutputBusLabel(0, "output");
  }

  AssessInputConnections();

  SetBlockSize(DEFAULT_BLOCK_SIZE);
}

IPlugAU::~IPlugAU()
{
  mRenderNotify.Empty(true);
  mInBuses.Empty(true);
  mOutBuses.Empty(true);
  mInBusConnections.Empty(true);
  mPropertyListeners.Empty(true);
}

void IPlugAU::SendAUEvent(AudioUnitEventType type, AudioComponentInstance ci, int idx)
{
  AudioUnitEvent auEvent;
  memset(&auEvent, 0, sizeof(AudioUnitEvent));
  auEvent.mEventType = type;
  auEvent.mArgument.mParameter.mAudioUnit = ci;
  auEvent.mArgument.mParameter.mParameterID = idx;
  auEvent.mArgument.mParameter.mScope = kAudioUnitScope_Global;
  auEvent.mArgument.mParameter.mElement = 0;
  AUEventListenerNotify(0, 0, &auEvent);
}

void IPlugAU::BeginInformHostOfParamChange(int idx)
{
  Trace(TRACELOC, "%d", idx);
  SendAUEvent(kAudioUnitEvent_BeginParameterChangeGesture, mCI, idx);
}

void IPlugAU::InformHostOfParamChange(int idx, double normalizedValue)
{
  Trace(TRACELOC, "%d:%f", idx, normalizedValue);
  SendAUEvent(kAudioUnitEvent_ParameterValueChange, mCI, idx);
}

void IPlugAU::EndInformHostOfParamChange(int idx)
{
  Trace(TRACELOC, "%d", idx);
  SendAUEvent(kAudioUnitEvent_EndParameterChangeGesture, mCI, idx);
}

void IPlugAU::InformHostOfProgramChange()
{
  //InformListeners(kAudioUnitProperty_CurrentPreset, kAudioUnitScope_Global);
  InformListeners(kAudioUnitProperty_PresentPreset, kAudioUnitScope_Global);
}

bool IPlugAU::IsRenderingOffline()
{
  return mIsOffline;
}

// Samples since start of project.
int IPlugAU::GetSamplePos()
{
  if (mHostCallbacks.transportStateProc)
  {
    double samplePos = 0.0, loopStartBeat, loopEndBeat;
    Boolean playing, changed, looping;
    mHostCallbacks.transportStateProc(mHostCallbacks.hostUserData, &playing, &changed, &samplePos,
                                      &looping, &loopStartBeat, &loopEndBeat);
    return int (samplePos+0.5);
  }
  return 0;
}

double IPlugAU::GetTempo()
{
  if (mHostCallbacks.beatAndTempoProc)
  {
    double currentBeat = 0.0, tempo = 0.0;
    mHostCallbacks.beatAndTempoProc(mHostCallbacks.hostUserData, &currentBeat, &tempo);
    if (tempo > 0.0)
    {
      mTempo = tempo;
    }
  }
  return mTempo;
}

void IPlugAU::GetTimeSig(int& numerator, int& denominator)
{
  UInt32 sampleOffsetToNextBeat = 0, tsDenom = 0;
  float tsNum = 0.0f;
  double currentMeasureDownBeat = 0.0;
  if (mHostCallbacks.musicalTimeLocationProc)
  {
    mHostCallbacks.musicalTimeLocationProc(mHostCallbacks.hostUserData, &sampleOffsetToNextBeat,
                                           &tsNum, &tsDenom, &currentMeasureDownBeat);
    numerator = (int) tsNum;
    denominator = (int) tsDenom;
  }
}

void IPlugAU::GetTime(ITimeInfo& timeInfo)
{
  if (mHostCallbacks.beatAndTempoProc)
  {
    double currentBeat = 0.0, tempo = 0.0;
    mHostCallbacks.beatAndTempoProc(mHostCallbacks.hostUserData, &currentBeat, &tempo);

    if (tempo > 0.0) timeInfo.mTempo = tempo;
    if (currentBeat> 0.0) timeInfo.mPPQPos = currentBeat;
  }

  if (mHostCallbacks.transportStateProc)
  {
    double samplePos = 0.0, loopStartBeat=0.0, loopEndBeat=0.0;
    Boolean playing, changed, looping;
    mHostCallbacks.transportStateProc(mHostCallbacks.hostUserData, &playing, &changed, &samplePos,
                                      &looping, &loopStartBeat, &loopEndBeat);

    if (samplePos>0.0)timeInfo.mSamplePos = samplePos;
    if (loopStartBeat>0.0) timeInfo.mCycleStart = loopStartBeat;
    if (loopEndBeat>0.0) timeInfo.mCycleEnd = loopEndBeat;
    timeInfo.mTransportIsRunning = playing;
    timeInfo.mTransportLoopEnabled = looping;
  }

  UInt32 sampleOffsetToNextBeat = 0, tsDenom = 0;
  float tsNum = 0.0f;
  double currentMeasureDownBeat = 0.0;
  
  if (mHostCallbacks.musicalTimeLocationProc)
  {
    mHostCallbacks.musicalTimeLocationProc(mHostCallbacks.hostUserData, &sampleOffsetToNextBeat,
                                           &tsNum, &tsDenom, &currentMeasureDownBeat);

    timeInfo.mNumerator = (int) tsNum;
    timeInfo.mDenominator = (int) tsDenom;
    if (currentMeasureDownBeat>0.0) timeInfo.mLastBar=currentMeasureDownBeat;
  }
}

EHost IPlugAU::GetHost()
{
  EHost host = IPLUG_BASE_CLASS::GetHost();
  if (host == kHostUninit)
  {
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    
    if (mainBundle)
    {
      CFStringRef id = CFBundleGetIdentifier(mainBundle);
      if (id)
      {
        CStrLocal str(id);
        //CFStringRef versStr = (CFStringRef) CFBundleGetValueForInfoDictionaryKey(mainBundle, kCFBundleVersionKey);
        SetHost(str.mCStr, 0);
        host = IPLUG_BASE_CLASS::GetHost();
      }
    }
    
    if (host == kHostUninit)
    {
      SetHost("", 0);
      host = IPLUG_BASE_CLASS::GetHost();
    }
  }
  return host;
}

void IPlugAU::HostSpecificInit()
{
  GetHost();
  OnHostIdentified(); // might get called again
}

void IPlugAU::ResizeGraphics(int w, int h)
{
  if (GetHasUI())
  {
    OnWindowResize();
  }
}

void IPlugAU::SetBlockSize(int blockSize)
{
  TRACE;
  int nIn = NInChannels() * blockSize;
  int nOut = NOutChannels() * blockSize;
  mInScratchBuf.Resize(nIn);
  mOutScratchBuf.Resize(nOut);
  memset(mInScratchBuf.Get(), 0, nIn * sizeof(AudioSampleType));
  memset(mOutScratchBuf.Get(), 0, nOut * sizeof(AudioSampleType));
  IPLUG_BASE_CLASS::SetBlockSize(blockSize);
}

void IPlugAU::InformListeners(AudioUnitPropertyID propID, AudioUnitScope scope)
{
  TRACE;
  int i, n = mPropertyListeners.GetSize();
  
  for (i = 0; i < n; ++i)
  {
    PropertyListener* pListener = mPropertyListeners.Get(i);
    
    if (pListener->mPropID == propID)
    {
      pListener->mListenerProc(pListener->mProcArgs, mCI, propID, scope, 0);
    }
  }
}

void IPlugAU::SetLatency(int samples)
{
  TRACE;
  int i, n = mPropertyListeners.GetSize();
  
  for (i = 0; i < n; ++i)
  {
    PropertyListener* pListener = mPropertyListeners.Get(i);
    if (pListener->mPropID == kAudioUnitProperty_Latency)
    {
      pListener->mListenerProc(pListener->mProcArgs, mCI, kAudioUnitProperty_Latency, kAudioUnitScope_Global, 0);
    }
  }
  
  IPLUG_BASE_CLASS::SetLatency(samples);
}

// TODO: SendMidiMsg
bool IPlugAU::SendMidiMsg(IMidiMsg& msg)
{
  return false;
}

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
#define GETINSTANCE(x)  (IPlugAU*) &((AudioComponentPlugInInstance *) x)->mInstanceStorage
//static
OSStatus IPlugAU::AUMethodInitialize(void *self)
{
  IPlugAU* _this = GETINSTANCE(self);
  return _this->DoInitialize(_this);
}

//static
OSStatus IPlugAU::AUMethodUninitialize(void *self)
{
  IPlugAU* _this = GETINSTANCE(self);
  return _this->DoUninitialize(_this);
}

//static
OSStatus IPlugAU::AUMethodGetPropertyInfo(void *self, AudioUnitPropertyID prop, AudioUnitScope scope, AudioUnitElement elem, UInt32 *outDataSize, Boolean *outWritable)
{
  IPlugAU* _this = GETINSTANCE(self);
  return _this->DoGetPropertyInfo(_this, prop, scope, elem, outDataSize, outWritable);
}

//static
OSStatus IPlugAU::AUMethodGetProperty(void *self, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, void *outData, UInt32 *ioDataSize)
{
  IPlugAU* _this = GETINSTANCE(self);
  return _this->DoGetProperty(_this, inID, inScope, inElement, outData, ioDataSize);
}

//static
OSStatus IPlugAU::AUMethodSetProperty(void *self, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, const void *inData, UInt32 *inDataSize)
{
  IPlugAU* _this = GETINSTANCE(self);
  return _this->DoSetProperty(_this, inID, inScope, inElement, inData, inDataSize);
}

//static
OSStatus IPlugAU::AUMethodAddPropertyListener(void *self, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc, void *userData)
{
  IPlugAU* _this = GETINSTANCE(self);
  return _this->DoAddPropertyListener(_this, prop, proc, userData);
}

//static
OSStatus IPlugAU::AUMethodRemovePropertyListener(void *self, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc)
{
  IPlugAU* _this = GETINSTANCE(self);
  return _this->DoRemovePropertyListener(_this, prop, proc);
}

//static
OSStatus IPlugAU::AUMethodRemovePropertyListenerWithUserData(void *self, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc, void *userData)
{
  IPlugAU* _this = GETINSTANCE(self);
  return _this->DoRemovePropertyListenerWithUserData(_this, prop, proc, userData);
}

//static
OSStatus IPlugAU::AUMethodAddRenderNotify(void *self, AURenderCallback proc, void *userData)
{
  IPlugAU* _this = GETINSTANCE(self);
  return _this->DoAddRenderNotify(_this, proc, userData);
}

//static
OSStatus IPlugAU::AUMethodRemoveRenderNotify(void *self, AURenderCallback proc, void *userData)
{
  IPlugAU* _this = GETINSTANCE(self);
  return _this->DoRemoveRenderNotify(_this, proc, userData);
}

//static
OSStatus IPlugAU::AUMethodGetParameter(void *self, AudioUnitParameterID param, AudioUnitScope scope, AudioUnitElement elem, AudioUnitParameterValue *value)
{
  IPlugAU* _this = GETINSTANCE(self);
  return _this->DoGetParameter(_this, param, scope, elem, value);
}

//static
OSStatus IPlugAU::AUMethodSetParameter(void *self, AudioUnitParameterID param, AudioUnitScope scope, AudioUnitElement elem, AudioUnitParameterValue value, UInt32 bufferOffset)
{
  IPlugAU* _this = GETINSTANCE(self);
  return _this->DoSetParameter(_this, param, scope, elem, value, bufferOffset);
}

//static
OSStatus IPlugAU::AUMethodScheduleParameters(void *self, const AudioUnitParameterEvent *pEvent, UInt32 nEvents)
{
  IPlugAU* _this = GETINSTANCE(self);
  return _this->DoScheduleParameters(_this, pEvent, nEvents);
}

//static
OSStatus IPlugAU::AUMethodRender(void *self, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inOutputBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
  IPlugAU* _this = GETINSTANCE(self);
  return _this->DoRender(_this, ioActionFlags, inTimeStamp, inOutputBusNumber, inNumberFrames, ioData);
}

//static
OSStatus IPlugAU::AUMethodReset(void *self, AudioUnitScope scope, AudioUnitElement elem)
{
  IPlugAU* _this = GETINSTANCE(self);
  return _this->DoReset(_this);
}

//static
OSStatus IPlugAU::AUMethodMIDIEvent(void *self, UInt32 inStatus, UInt32 inData1, UInt32 inData2, UInt32 inOffsetSampleFrame)
{
  IPlugAU* _this = GETINSTANCE(self);
  return _this->DoMIDIEvent(_this, inStatus, inData1, inData2, inOffsetSampleFrame);
}

//static
OSStatus IPlugAU::AUMethodSysEx(void *self, const UInt8 *inData, UInt32 inLength)
{
  IPlugAU* _this = GETINSTANCE(self);
  return _this->DoSysEx(_this, inData, inLength);
}
#endif

//static
OSStatus IPlugAU::DoInitialize(IPlugAU *_this)
{
  if (!(_this->CheckLegalIO()))
  {
    return badComponentSelector;
  }
  _this->mActive = true;
  _this->OnParamReset();
  _this->OnActivate(true);
  
  return noErr;
}

//static
OSStatus IPlugAU::DoUninitialize(IPlugAU *_this)
{
  _this->mActive = false;
  _this->OnActivate(false);
  return noErr;
}

//static
OSStatus IPlugAU::DoGetPropertyInfo(IPlugAU *_this, AudioUnitPropertyID prop, AudioUnitScope scope, AudioUnitElement elem, UInt32 *outDataSize, Boolean *outWritable)
{
  UInt32 dataSize = 0;
  
  if (!outDataSize)
    outDataSize = &dataSize;
  
  Boolean writeable;
  
  if (!outWritable)
    outWritable = &writeable;
  
  *outWritable = false;
  
  return _this->GetProperty(prop, scope, elem, outDataSize, outWritable, 0 /* indicates info */);
}

//static
OSStatus IPlugAU::DoGetProperty(IPlugAU *_this, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, void *outData, UInt32 *ioDataSize)
{
  UInt32 dataSize = 0;
  
  if (!ioDataSize)
    ioDataSize = &dataSize;
  
  Boolean writeable = false;
  
  return _this->GetProperty(inID, inScope, inElement, ioDataSize, &writeable, outData);
}

//static
OSStatus IPlugAU::DoSetProperty(IPlugAU *_this, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, const void *inData, UInt32* inDataSize)
{
  return _this->SetProperty(inID, inScope, inElement, inDataSize, inData);
}

//static
OSStatus IPlugAU::DoAddPropertyListener(IPlugAU *_this, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc, void *userData)
{
  PropertyListener listener;
  listener.mPropID = prop;
  listener.mListenerProc = proc;
  listener.mProcArgs = userData;
  int i, n = _this->mPropertyListeners.GetSize();
  for (i = 0; i < n; ++i)
  {
    PropertyListener* pListener = _this->mPropertyListeners.Get(i);
    if (listener.mPropID == pListener->mPropID && listener.mListenerProc == pListener->mListenerProc)
    {
      return noErr;
    }
  }
  PtrListAddFromStack(&(_this->mPropertyListeners), &listener);
  return noErr;
}

//static
OSStatus IPlugAU::DoRemovePropertyListener(IPlugAU *_this, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc)
{
  PropertyListener listener;
  listener.mPropID = prop;
  listener.mListenerProc = proc;
  int i, n = _this->mPropertyListeners.GetSize();
  for (i = 0; i < n; ++i)
  {
    PropertyListener* pListener = _this->mPropertyListeners.Get(i);
    if (listener.mPropID == pListener->mPropID && listener.mListenerProc == pListener->mListenerProc)
    {
      _this->mPropertyListeners.Delete(i, true);
      break;
    }
  }
  return noErr;
}

//static
OSStatus IPlugAU::DoRemovePropertyListenerWithUserData(IPlugAU *_this, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc, void *userData)
{
  PropertyListener listener;
  listener.mPropID = prop;
  listener.mListenerProc = proc;
  listener.mProcArgs = userData;
  int i, n = _this->mPropertyListeners.GetSize();
  for (i = 0; i < n; ++i)
  {
    PropertyListener* pListener = _this->mPropertyListeners.Get(i);
    if (listener.mPropID == pListener->mPropID &&
        listener.mListenerProc == pListener->mListenerProc && listener.mProcArgs == pListener->mProcArgs)
    {
      _this->mPropertyListeners.Delete(i, true);
      break;
    }
  }
  return noErr;
}

//static
OSStatus IPlugAU::DoAddRenderNotify(IPlugAU *_this, AURenderCallback proc, void *userData)
{
  AURenderCallbackStruct acs;
  acs.inputProc = proc;
  acs.inputProcRefCon = userData;
  
  PtrListAddFromStack(&(_this->mRenderNotify), &acs);
  return noErr;
}

//static
OSStatus IPlugAU::DoRemoveRenderNotify(IPlugAU *_this, AURenderCallback proc, void *userData)
{
  
  AURenderCallbackStruct acs;
  acs.inputProc = proc;
  acs.inputProcRefCon = userData;
  
  int i, n = _this->mRenderNotify.GetSize();
  for (i = 0; i < n; ++i)
  {
    AURenderCallbackStruct* pACS = _this->mRenderNotify.Get(i);
    if (acs.inputProc == pACS->inputProc)
    {
      _this->mRenderNotify.Delete(i, true);
      break;
    }
  }
  return noErr;
}

//static
OSStatus IPlugAU::DoGetParameter(IPlugAU *_this, AudioUnitParameterID param, AudioUnitScope scope, AudioUnitElement elem, AudioUnitParameterValue *value)
{
  //mutex locked below
  return _this->GetParamProc(_this, param, scope, elem, value);
}

//static
OSStatus IPlugAU::DoSetParameter(IPlugAU *_this, AudioUnitParameterID param, AudioUnitScope scope, AudioUnitElement elem, AudioUnitParameterValue value, UInt32 bufferOffset)
{
  //mutex locked below
  return _this->SetParamProc(_this, param, scope, elem, value, bufferOffset);
}

//static
OSStatus IPlugAU::DoScheduleParameters(IPlugAU *_this, const AudioUnitParameterEvent *pEvent, UInt32 nEvents)
{
  //mutex locked below
  for (int i = 0; i < nEvents; ++i, ++pEvent)
  {
    if (pEvent->eventType == kParameterEvent_Immediate)
    {
      OSStatus r = SetParamProc(_this, pEvent->parameter, pEvent->scope, pEvent->element,
                                pEvent->eventValues.immediate.value, pEvent->eventValues.immediate.bufferOffset);
      if (r != noErr)
      {
        return r;
      }
    }
  }
  return noErr;
}

//static
OSStatus IPlugAU::DoRender(IPlugAU *_this, AudioUnitRenderActionFlags *ioActionFlags, const AudioTimeStamp *inTimeStamp, UInt32 inOutputBusNumber, UInt32 inNumberFrames, AudioBufferList *ioData)
{
  return RenderProc(_this, ioActionFlags, inTimeStamp, inOutputBusNumber, inNumberFrames, ioData);
}

//static
OSStatus IPlugAU::DoReset(IPlugAU *_this)
{
  _this->Reset();
  return noErr;
}

//static
OSStatus IPlugAU::DoMIDIEvent(IPlugAU *_this, UInt32 inStatus, UInt32 inData1, UInt32 inData2, UInt32 inOffsetSampleFrame)
{
  if(_this->DoesMIDI())
  {
    IMidiMsg msg;
    msg.mStatus = inStatus;
    msg.mData1 = inData1;
    msg.mData2 = inData2;
    msg.mOffset = inOffsetSampleFrame;
    _this->ProcessMidiMsg(msg);
    return noErr;
  }
  else
    return badComponentSelector;
}

//static
OSStatus IPlugAU::DoSysEx(IPlugAU *_this, const UInt8 *inData, UInt32 inLength)
{
  if(_this->DoesMIDI())
  {
    ISysEx sysex;
    sysex.mData = inData;
    sysex.mSize = inLength;
    sysex.mOffset = 0;
    _this->ProcessSysEx(sysex);
    return noErr;
  }
  else
    return badComponentSelector;
}

