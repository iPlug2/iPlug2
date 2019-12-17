/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include <algorithm>
#include <CoreMIDI/CoreMIDI.h>

#include "heapbuf.h"

#include "dfx-au-utilities.h"
#include "IPlugAU.h"
#include "IPlugAU_ioconfig.h"

using namespace iplug;

#pragma mark - CFString and CString Utilities

static inline CFStringRef MakeCFString(const char* cStr)
{
  return CFStringCreateWithCString(0, cStr, kCFStringEncodingUTF8);
}

class IPlugAU::CFStrLocal
{
public:
  CFStrLocal(const char* cStr)
  {
    mCFStr = MakeCFString(cStr);
  }
    
  ~CFStrLocal()
  {
    CFRelease(mCFStr);
  }
    
  CFStrLocal(const CFStrLocal&) = delete;
  CFStrLocal& operator=(const CFStrLocal&) = delete;
    
  CFStringRef Get() { return mCFStr; }
    
private:
  CFStringRef mCFStr;
};

struct IPlugAU::CStrLocal : WDL_TypedBuf<char>
{
  CStrLocal(CFStringRef cfStr)
  {
    if (cfStr)
    {
      Resize((int) CFStringGetLength(cfStr) + 1);
      CFStringGetCString(cfStr, Get(), GetSize(), kCFStringEncodingUTF8);
    }
  }
};

#pragma mark - Utilities

inline void IPlugAU::PutNumberInDict(CFMutableDictionaryRef pDict, const char* key, void* pNumber, CFNumberType type)
{
  CFStrLocal cfKey(key);
  CFNumberRef pValue = CFNumberCreate(0, type, pNumber);
  CFDictionarySetValue(pDict, cfKey.Get(), pValue);
  CFRelease(pValue);
}

inline void IPlugAU::PutStrInDict(CFMutableDictionaryRef pDict, const char* key, const char* value)
{
  CFStrLocal cfKey(key);
  CFStrLocal cfValue(value);
  CFDictionarySetValue(pDict, cfKey.Get(), cfValue.Get());
}

inline void IPlugAU::PutDataInDict(CFMutableDictionaryRef pDict, const char* key, IByteChunk* pChunk)
{
  CFStrLocal cfKey(key);
  CFDataRef pData = CFDataCreate(0, pChunk->GetData(), pChunk->Size());
  CFDictionarySetValue(pDict, cfKey.Get(), pData);
  CFRelease(pData);
}

inline bool IPlugAU::GetNumberFromDict(CFDictionaryRef pDict, const char* key, void* pNumber, CFNumberType type)
{
  CFStrLocal cfKey(key);
  CFNumberRef pValue = (CFNumberRef) CFDictionaryGetValue(pDict, cfKey.Get());
  if (pValue)
  {
    CFNumberGetValue(pValue, type, pNumber);
    return true;
  }
  return false;
}

inline bool IPlugAU::GetStrFromDict(CFDictionaryRef pDict, const char* key, char* value)
{
  CFStrLocal cfKey(key);
  CFStringRef pValue = (CFStringRef) CFDictionaryGetValue(pDict, cfKey.Get());
  if (pValue)
  {
    CStrLocal cStr(pValue);
    strcpy(value, cStr.Get());
    return true;
  }
  value[0] = '\0';
  return false;
}

inline bool IPlugAU::GetDataFromDict(CFDictionaryRef pDict, const char* key, IByteChunk* pChunk)
{
  CFStrLocal cfKey(key);
  CFDataRef pData = (CFDataRef) CFDictionaryGetValue(pDict, cfKey.Get());
  if (pData)
  {
    CFIndex n = CFDataGetLength(pData);
    pChunk->Resize((int) n);
    memcpy(pChunk->GetData(), CFDataGetBytePtr(pData), n);
    return true;
  }
  return false;
}

#define kAudioUnitRemovePropertyListenerWithUserDataSelect 0x0012

typedef AudioStreamBasicDescription STREAM_DESC;

static /* inline */ void MakeDefaultASBD(STREAM_DESC* pASBD, double sampleRate, int nChannels, bool interleaved)
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
static int PtrListAddFromStack(WDL_PtrList<C>* pList, C* pStackInstance)
{
  C* pNew = new C;
  memcpy(pNew, pStackInstance, sizeof(C));
  pList->Add(pNew);
  return pList->GetSize() - 1;
}

template <class C>
static int PtrListInitialize(WDL_PtrList<C>* pList, int size)
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

#ifndef AU_NO_COMPONENT_ENTRY
#pragma mark - COMPONENT MANAGER ENTRY POINT
// static
OSStatus IPlugAU::IPlugAUEntry(ComponentParameters *params, void* pPlug)
{
  int select = params->what;

  Trace(TRACELOC, "(%d:%s)", select, AUSelectStr(select));

  if (select == kComponentOpenSelect)
  {
    // N.B. calling this with nullptr will call through to new (hence the delete below)
    IPlugAU* _this = MakePlug(nullptr);
    
    _this->PruneUninitializedPresets();
    _this->mCI = GET_COMP_PARAM(ComponentInstance, 0, 1);
    SetComponentInstanceStorage(_this->mCI, (Handle) _this);
    return noErr;
  }

  IPlugAU* _this = (IPlugAU*) pPlug;
  
  if (select == kComponentCloseSelect)
  {
    _this->ClearConnections();
    delete _this;
    return noErr;
  }

  switch (select)
  {
    case kComponentVersionSelect:
    {
      return _this->GetPluginVersion(false);
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
#endif //AU_NO_COMPONENT_ENTRY

#pragma mark - GetChannelLayoutTags

UInt32 IPlugAU::GetChannelLayoutTags(AudioUnitScope scope, AudioUnitElement element, AudioChannelLayoutTag* tags)
{
  switch(scope)
  {
    case kAudioUnitScope_Input:
    case kAudioUnitScope_Output:
    {
      ERoute dir = (ERoute) (scope - 1);
      
      WDL_TypedBuf<uint64_t> foundTags;
      
      for(auto configIdx = 0; configIdx < NIOConfigs(); configIdx++)
      {
        IOConfig* pConfig = GetIOConfig(configIdx);
        
        for(auto busIdx = 0; busIdx < pConfig->NBuses(dir); busIdx++)
        {
          WDL_TypedBuf<uint64_t> busTypes;
          GetAPIBusTypeForChannelIOConfig(configIdx, dir, busIdx, pConfig, &busTypes);
//          DBGMSG("Found %i different tags for an %s bus with %i channels\n", busTypes.GetSize(), RoutingDirStrs[dir], pConfig->GetBusInfo(dir, busIdx)->mNChans);

          for (auto tag = 0; tag < busTypes.GetSize(); tag++)
          {
            if(foundTags.Find(busTypes.Get()[tag] == -1))
               foundTags.Add(busTypes.Get()[tag]);
          }
        }
      }
      
      if(tags)
      {
        for (auto v = 0; v < foundTags.GetSize(); v++)
        {
          tags[v] = (AudioChannelLayoutTag) foundTags.Get()[v];
        }
        
        DBGMSG("Adding %i tags\n", foundTags.GetSize());
        
        return 1; // success
      }
      else
        return foundTags.GetSize();
      
//      TODO: what about wild cards?
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
        ((void**) pData)[0] = (void*) static_cast<IPlugAPIBase*> (this);
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
                       kAudioUnitParameterFlag_IsWritable |
                       kAudioUnitParameterFlag_IsReadable ;
        
        #ifndef IPLUG1_COMPATIBILITY
        pInfo->flags |= kAudioUnitParameterFlag_IsHighResolution;
        #endif
        
        ENTER_PARAMS_MUTEX
        IParam* pParam = GetParam(element);
        
        if (!pParam->GetCanAutomate())  pInfo->flags |= kAudioUnitParameterFlag_NonRealTime;
        if (pParam->GetMeta()) pInfo->flags |= kAudioUnitParameterFlag_IsElementMeta;
        if (pParam->NDisplayTexts()) pInfo->flags |= kAudioUnitParameterFlag_ValuesHaveStrings;

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
            switch (pParam->Unit())
            {
              case IParam::kUnitPercentage:     pInfo->unit = kAudioUnitParameterUnit_Percent;            break;
              case IParam::kUnitSeconds:        pInfo->unit = kAudioUnitParameterUnit_Seconds;            break;
              case IParam::kUnitMilliseconds:   pInfo->unit = kAudioUnitParameterUnit_Milliseconds;       break;
              case IParam::kUnitSamples:        pInfo->unit = kAudioUnitParameterUnit_SampleFrames;       break;
              case IParam::kUnitDB:             pInfo->unit = kAudioUnitParameterUnit_Decibels;           break;
              case IParam::kUnitLinearGain:     pInfo->unit = kAudioUnitParameterUnit_LinearGain;         break;
              case IParam::kUnitPan:            pInfo->unit = kAudioUnitParameterUnit_Pan;                break;
              case IParam::kUnitPhase:          pInfo->unit = kAudioUnitParameterUnit_Phase;              break;
              case IParam::kUnitDegrees:        pInfo->unit = kAudioUnitParameterUnit_Degrees;            break;
              case IParam::kUnitMeters:         pInfo->unit = kAudioUnitParameterUnit_Meters;             break;
              case IParam::kUnitRate:           pInfo->unit = kAudioUnitParameterUnit_Rate;               break;
              case IParam::kUnitRatio:          pInfo->unit = kAudioUnitParameterUnit_Ratio;              break;
              case IParam::kUnitFrequency:      pInfo->unit = kAudioUnitParameterUnit_Hertz;              break;
              case IParam::kUnitOctaves:        pInfo->unit = kAudioUnitParameterUnit_Octaves;            break;
              case IParam::kUnitCents:          pInfo->unit = kAudioUnitParameterUnit_Cents;              break;
              case IParam::kUnitAbsCents:       pInfo->unit = kAudioUnitParameterUnit_AbsoluteCents;      break;
              case IParam::kUnitSemitones:      pInfo->unit = kAudioUnitParameterUnit_RelativeSemiTones;  break;
              case IParam::kUnitMIDINote:       pInfo->unit = kAudioUnitParameterUnit_MIDINoteNumber;     break;
              case IParam::kUnitMIDICtrlNum:    pInfo->unit = kAudioUnitParameterUnit_MIDIController;     break;
              case IParam::kUnitBPM:            pInfo->unit = kAudioUnitParameterUnit_BPM;                break;
              case IParam::kUnitBeats:          pInfo->unit = kAudioUnitParameterUnit_Beats;              break;
                
              case IParam::kUnitCustom:
              {
                if (CStringHasContents(pParam->GetCustomUnit()))
                {
                  pInfo->unit = kAudioUnitParameterUnit_CustomUnit;
                  pInfo->unitName = CFStringCreateWithCString(0, pParam->GetCustomUnit(), kCFStringEncodingUTF8);
                }
                else
                {
                  pInfo->unit = kAudioUnitParameterUnit_Generic;
                }
                break;
              }
            }
          }
        }

        switch (pParam->DisplayType())
        {
          case IParam::kDisplayLinear:
            break;
          case IParam::kDisplaySquared:
            pInfo->flags |= kAudioUnitParameterFlag_DisplaySquared;
            break;
          case IParam::kDisplaySquareRoot:
            pInfo->flags |= kAudioUnitParameterFlag_DisplaySquareRoot;
            break;
          case IParam::kDisplayCubed:
            pInfo->flags |= kAudioUnitParameterFlag_DisplayCubed;
            break;
          case IParam::kDisplayCubeRoot:
            pInfo->flags |= kAudioUnitParameterFlag_DisplayCubeRoot;
            break;
          case IParam::kDisplayExp:
            pInfo->flags |= kAudioUnitParameterFlag_DisplayExponential;
            break;
          case IParam::kDisplayLog:
            pInfo->flags |= kAudioUnitParameterFlag_DisplayLogarithmic;
            break;
        }
        
        pInfo->minValue = pParam->GetMin();
        pInfo->maxValue = pParam->GetMax();
        pInfo->defaultValue = pParam->GetDefault();
        
        const char* paramGroupName = pParam->GetGroupForHost();

        if (CStringHasContents(paramGroupName))
        {
          int clumpID = 0;
          
          for(int i = 0; i< NParamGroups(); i++)
          {
            if(strcmp(paramGroupName, GetParamGroupName(i)) == 0)
              clumpID = i+1;
          }
          
          if (clumpID == 0) // new clump
            clumpID = AddParamGroup(paramGroupName);
          
          pInfo->flags = pInfo->flags | kAudioUnitParameterFlag_HasClump;
          pInfo->clumpID = clumpID;
        }
        LEAVE_PARAMS_MUTEX
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
          n = mInBuses.GetSize();
        else if (scope == kAudioUnitScope_Output)
          n = mOutBuses.GetSize();
        else if (scope == kAudioUnitScope_Global)
          n = 1;
        
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
      int n = NIOConfigs(); //TODO: THIS IS INCORRECT!
      *pDataSize = n * sizeof(AUChannelInfo);
      if (pData)
      {
        AUChannelInfo* pChInfo = (AUChannelInfo*) pData;
        for (int i = 0; i < n; ++i, ++pChInfo)
        {
          IOConfig* pIO = GetIOConfig(i);
          
          if(pIO->ContainsWildcard(ERoute::kInput))
             pChInfo->inChannels = -1;
          else
            pChInfo->inChannels = pIO->GetTotalNChannels(kInput);
          
          if(pIO->ContainsWildcard(ERoute::kOutput))
            pChInfo->outChannels = -1;
          else
            pChInfo->outChannels = pIO->GetTotalNChannels(kOutput);
          
          Trace(TRACELOC, "IO:%d:%d", pChInfo->inChannels, pChInfo->outChannels);

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
      ENTER_PARAMS_MUTEX
      IParam* pParam = GetParam(element);
      int n = pParam->NDisplayTexts();
      if (!n)
      {
        LEAVE_PARAMS_MUTEX
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
          CFStrLocal cfstr(str);
          CFArrayAppendValue(nameArray, cfstr.Get());
        }
        *((CFArrayRef*) pData) = nameArray;
      }
      LEAVE_PARAMS_MUTEX
      return noErr;
    }
    case kAudioUnitProperty_GetUIComponentList:          // 18,
    {
      return kAudioUnitErr_InvalidProperty;
    }
    case kAudioUnitProperty_AudioChannelLayout:
    {
      return kAudioUnitErr_InvalidPropertyValue;
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
        *((UInt32*) pData) = (GetBypassed() ? 1 : 0);
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
        #ifdef AU_NO_PRESETS
        int i, n = 0;
        #else
        int i, n = NPresets();
        #endif
        
        CFMutableArrayRef presetArray = CFArrayCreateMutable(kCFAllocatorDefault, n, &kCFAUPresetArrayCallBacks);

        if (presetArray == NULL)
          return coreFoundationUnknownErr;

        for (i = 0; i < n; ++i)
        {
          CFStrLocal presetName(GetPresetName(i));
          CFAUPresetRef newPreset = CFAUPresetCreate(kCFAllocatorDefault, i, presetName.Get()); // todo should i be 0 based?

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
              *(CFStringRef *)pData = MakeCFString("input");
              return noErr;
            }
            else
              return kAudioUnitErr_InvalidElement;
          }
          case kAudioUnitScope_Output:
          {
            //TODO: live 5.1 crash?
            *(CFStringRef *)pData = MakeCFString("output");
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
      if (HasUI()) // this won't work < 10.5 SDK but barely anyone will use that these days
      {
        *pDataSize = sizeof(AudioUnitCocoaViewInfo);  // Just one view.
        if (pData)
        {
          AudioUnitCocoaViewInfo* pViewInfo = (AudioUnitCocoaViewInfo*) pData;
          CFStrLocal bundleID(mBundleID.Get());
          CFBundleRef pBundle = CFBundleGetBundleWithIdentifier(bundleID.Get());
          CFURLRef url = CFBundleCopyBundleURL(pBundle);
          pViewInfo->mCocoaAUViewBundleLocation = url;
          pViewInfo->mCocoaAUViewClass[0] = CFStringCreateWithCString(0, mCocoaViewFactoryClassName.Get(), kCFStringEncodingUTF8);
        }
        return noErr;
      }
      return kAudioUnitErr_InvalidProperty;
    }
#pragma mark - kAudioUnitProperty_SupportedChannelLayoutTags
    case kAudioUnitProperty_SupportedChannelLayoutTags:
    {
      if (!pData) // GetPropertyInfo
      {
        UInt32 numLayouts = GetChannelLayoutTags(scope, element, NULL); // 0 = invalid scope, 1 = input

        if (numLayouts)
        {
          *pDataSize = numLayouts * sizeof(AudioChannelLayoutTag);
          *pWriteable = true;
          return noErr;
        }
        else
        {
          *pDataSize = 0;
          *pWriteable = false;
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
        char cStr[MAX_PARAM_NAME_LEN];
        ENTER_PARAMS_MUTEX
        strcpy(cStr, GetParam(pIDName->inID)->GetNameForHost());
        LEAVE_PARAMS_MUTEX
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
        
        parameterNameInfo->outName = CFStringCreateWithCString(0, GetParamGroupName(clumpId-1), kCFStringEncodingUTF8);
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
        ENTER_PARAMS_MUTEX
        GetParam(pSFV->inParamID)->GetDisplayForHost(*(pSFV->inValue), false, mParamDisplayStr);
        LEAVE_PARAMS_MUTEX
        pSFV->outString = MakeCFString((const char*) mParamDisplayStr.Get());
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
          ENTER_PARAMS_MUTEX
          const double v = GetParam(pVFS->inParamID)->StringToValue(cStr.Get());
          LEAVE_PARAMS_MUTEX
          pVFS->outValue = (AudioUnitParameterValue) v;
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
      if (IsInstrument())
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

    NO_OP(kAudioUnitProperty_AUHostIdentifier);         // 46,
    case kAudioUnitProperty_MIDIOutputCallbackInfo:     // 47,
    {
      if(pData == 0)
      {
        *pWriteable = false;
        *pDataSize = sizeof(CFArrayRef);
      }
      else
      {
        CFStringRef outputName = CFSTR("midiOut");
        CFArrayRef array = CFArrayCreate(kCFAllocatorDefault, (const void**) &outputName, 1, nullptr);
        CFRelease(outputName);
        *((CFArrayRef*) pData) = array;
      }
      return noErr;
    }
    case kAudioUnitProperty_MIDIOutputCallback:         // 48,
    {
      if(pData == 0)
      {
        *pWriteable = true;
        *pDataSize = sizeof(AUMIDIOutputCallbackStruct);
        
        return noErr;
      }
    }
    NO_OP(kAudioUnitProperty_InputSamplesInOutput);       // 49,
    NO_OP(kAudioUnitProperty_ClassInfoFromDocument);      // 50
      
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
      OnReset();
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
      ResizeScratchBuffers();
      OnReset();
      return noErr;
    }
    NO_OP(kAudioUnitProperty_SetExternalBuffer);         // 15,
    NO_OP(kAudioUnitProperty_ParameterValueStrings);     // 16,
    NO_OP(kAudioUnitProperty_GetUIComponentList);        // 18,
    NO_OP(kAudioUnitProperty_AudioChannelLayout);        // 19, //TODO?: Set kAudioUnitProperty_AudioChannelLayout
    NO_OP(kAudioUnitProperty_TailTime);                  // 20,
    case kAudioUnitProperty_BypassEffect:                // 21,
    {
      const bool bypassed = *((UInt32*) pData) != 0;
      SetBypassed(bypassed);
      
      // TODO: should the following be called here?
      OnActivate(!bypassed);
      OnReset();
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
      const bool renderingOffline = (*((UInt32*) pData) != 0);
      SetRenderingOffline(renderingOffline);
      return noErr;
    }
    NO_OP(kAudioUnitProperty_ParameterStringFromValue);  // 33,
    NO_OP(kAudioUnitProperty_ParameterValueFromString);  // 38,
    NO_OP(kAudioUnitProperty_IconLocation);              // 39,
    NO_OP(kAudioUnitProperty_PresentationLatency);       // 40,
    NO_OP(kAudioUnitProperty_DependentParameters);       // 45,
    case kAudioUnitProperty_AUHostIdentifier:            // 46,
    {
      if (GetHost() == kHostUninit)
      {
        AUHostIdentifier* pHostID = (AUHostIdentifier*) pData;
        CStrLocal hostStr(pHostID->hostName);
        int version = (pHostID->hostVersion.majorRev << 16)
                    + ((pHostID->hostVersion.minorAndBugRev & 0xF0) << 4)
                    + ((pHostID->hostVersion.minorAndBugRev & 0x0F));
        SetHost(hostStr.Get(), version);
      }
      return noErr;
    }
    NO_OP(kAudioUnitProperty_MIDIOutputCallbackInfo);   // 47,
    case kAudioUnitProperty_MIDIOutputCallback:         // 48,
    {
      mMidiCallback = *((AUMIDIOutputCallbackStruct*) pData);
      return noErr;
    }
    NO_OP(kAudioUnitProperty_InputSamplesInOutput);       // 49,
    NO_OP(kAudioUnitProperty_ClassInfoFromDocument)       // 50
    default:
    {
      return kAudioUnitErr_InvalidProperty;
    }
  }
}

//static
const char* IPlugAU::AUInputTypeStr(int type)
{
  switch ((IPlugAU::EAUInputType) type)
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
  TRACE
  SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), false);

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
      SetChannelConnections(ERoute::kInput, startChannelIdx, nConnected, true);
      SetChannelConnections(ERoute::kInput, startChannelIdx + nConnected, nUnconnected, false);
    }

    Trace(TRACELOC, "%d:%s:%d:%d:%d", i, AUInputTypeStr(pInBusConn->mInputType), startChannelIdx, pInBus->mNPlugChannels, pInBus->mNHostChannels);
  }
}

OSStatus IPlugAU::GetState(CFPropertyListRef* ppPropList)
{
  int plugType = GetAUPluginType();
  int plugSubType = GetUniqueID();
  int plugManID = GetMfrID();

  CFMutableDictionaryRef pDict = CFDictionaryCreateMutable(0, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  int version = GetPluginVersion(false);
  PutNumberInDict(pDict, kAUPresetVersionKey, &version, kCFNumberSInt32Type);
  PutNumberInDict(pDict, kAUPresetTypeKey, &(plugType), kCFNumberSInt32Type);
  PutNumberInDict(pDict, kAUPresetSubtypeKey, &(plugSubType), kCFNumberSInt32Type);
  PutNumberInDict(pDict, kAUPresetManufacturerKey, &(plugManID), kCFNumberSInt32Type);
  PutStrInDict(pDict, kAUPresetNameKey, GetPresetName(GetCurrentPresetIdx()));

  IByteChunk chunk;
  //InitChunkWithIPlugVer(&IPlugChunk); // TODO: IPlugVer should be in chunk!

  if (SerializeState(chunk))
  {
    PutDataInDict(pDict, kAUPresetDataKey, &chunk);
  }

  *ppPropList = pDict;
  TRACE
  return noErr;
}

OSStatus IPlugAU::SetState(CFPropertyListRef pPropList)
{
  CFDictionaryRef pDict = (CFDictionaryRef) pPropList;
  int version, type, subtype, mfr;
  char presetName[64];
  if (!GetNumberFromDict(pDict, kAUPresetVersionKey, &version, kCFNumberSInt32Type) ||
      !GetNumberFromDict(pDict, kAUPresetTypeKey, &type, kCFNumberSInt32Type) ||
      !GetNumberFromDict(pDict, kAUPresetSubtypeKey, &subtype, kCFNumberSInt32Type) ||
      !GetNumberFromDict(pDict, kAUPresetManufacturerKey, &mfr, kCFNumberSInt32Type) ||
      !GetStrFromDict(pDict, kAUPresetNameKey, presetName) ||
      //version != GetPluginVersion(false) ||
      type != GetAUPluginType() ||
      subtype != GetUniqueID() ||
      mfr != GetMfrID())
  {
    return kAudioUnitErr_InvalidPropertyValue;
  }
  
  RestorePreset(presetName);

  IByteChunk chunk;

  if (!GetDataFromDict(pDict, kAUPresetDataKey, &chunk))
  {
    return kAudioUnitErr_InvalidPropertyValue;
  }

  // TODO: IPlugVer should be in chunk!
  //  int pos;
  //  IByteChunk::GetIPlugVerFromChunk(chunk, pos)
  
  if (!UnserializeState(chunk, 0))
  {
    return kAudioUnitErr_InvalidPropertyValue;
  }

  OnRestoreState();
  return noErr;
}

// pData == 0 means return property info only.
OSStatus IPlugAU::GetProc(AudioUnitElement element, UInt32* pDataSize, void* pData)
{
  Trace(TRACELOC, "%s:(%d:%s)", (pData ? "" : "Info"), element, AUSelectStr(element));

  switch (element)
  {
      //TODO: WHAT ABOUT THESE!!!
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
  ENTER_PARAMS_MUTEX_STATIC
  *pValue = _this->GetParam(paramID)->Value();
  LEAVE_PARAMS_MUTEX_STATIC
  return noErr;
}

// static
OSStatus IPlugAU::SetParamProc(void* pPlug, AudioUnitParameterID paramID, AudioUnitScope scope, AudioUnitElement element, AudioUnitParameterValue value, UInt32 offsetFrames)
{
  Trace(TRACELOC, "%d:(%d:%s):%d", paramID, scope, AUScopeStr(scope), element);

  // In the SDK, offset frames is only looked at in group scope.
  ASSERT_SCOPE(kAudioUnitScope_Global);
  IPlugAU* _this = (IPlugAU*) pPlug;
  ENTER_PARAMS_MUTEX_STATIC
  _this->GetParam(paramID)->Set(value);
  _this->SendParameterValueFromAPI(paramID, value, false);
  _this->OnParamChange(paramID, kHost);
  LEAVE_PARAMS_MUTEX_STATIC
  return noErr;
}

static inline OSStatus RenderCallback(AURenderCallbackStruct* pCB, AudioUnitRenderActionFlags* pFlags, const AudioTimeStamp* pTimestamp, UInt32 inputBusIdx, UInt32 nFrames, AudioBufferList* pOutBufList)
{
  TRACE
  return pCB->inputProc(pCB->inputProcRefCon, pFlags, pTimestamp, inputBusIdx, nFrames, pOutBufList);
}

// static
OSStatus IPlugAU::RenderProc(void* pPlug, AudioUnitRenderActionFlags* pFlags, const AudioTimeStamp* pTimestamp,
                                    UInt32 outputBusIdx, UInt32 nFrames, AudioBufferList* pOutBufList)
{
  Trace(TRACELOC, "%d:%d:%d", outputBusIdx, pOutBufList->mNumberBuffers, nFrames);

  IPlugAU* _this = (IPlugAU*) pPlug;
  
  _this->mLastRenderTimeStamp = *pTimestamp;

  if (!(pTimestamp->mFlags & kAudioTimeStampSampleTimeValid) /*|| outputBusIdx >= _this->mOutBuses.GetSize()*/ || nFrames > _this->GetBlockSize())
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

  int lastConnectedOutputBus = -1;

  if(!_this->IsMidiEffect())
  {
    double renderSampleTime = pTimestamp->mSampleTime;

    // Pull input buffers.
    if (renderSampleTime != _this->mLastRenderSampleTime)
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
            _this->AttachBuffers(ERoute::kInput, chIdx, 1, (AudioSampleType**) &(pInBufList->mBuffers[i].mData), nFrames);
          }
        }
      }
      _this->mLastRenderSampleTime = renderSampleTime;
    }
  
    BusChannels* pOutBus = _this->mOutBuses.Get(outputBusIdx);

    // if this bus is not connected OR the number of buffers that the host has given are not equal to the number the bus expects
    if (!(pOutBus->mConnected) || pOutBus->mNHostChannels != pOutBufList->mNumberBuffers)
    {
      int startChannelIdx = pOutBus->mPlugChannelStartIdx;
      int nConnected = std::min<int>(pOutBus->mNHostChannels, pOutBufList->mNumberBuffers);
      int nUnconnected = std::max(pOutBus->mNPlugChannels - nConnected, 0);
      
      assert(nConnected > -1);
      _this->SetChannelConnections(ERoute::kOutput, startChannelIdx, nConnected, true);
      _this->SetChannelConnections(ERoute::kOutput, startChannelIdx + nConnected, nUnconnected, false); // This will disconnect the right handle channel on a single stereo bus
      pOutBus->mConnected = true;
    }

    for (int i = 0, chIdx = pOutBus->mPlugChannelStartIdx; i < pOutBufList->mNumberBuffers; ++i, ++chIdx)
    {
      if (!(pOutBufList->mBuffers[i].mData)) // Downstream unit didn't give us buffers.
        pOutBufList->mBuffers[i].mData = _this->mOutScratchBuf.Get() + chIdx * nFrames;

      _this->AttachBuffers(ERoute::kOutput, chIdx, 1, (AudioSampleType**) &(pOutBufList->mBuffers[i].mData), nFrames);
    }

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
  }

  if (_this->IsMidiEffect() || outputBusIdx == lastConnectedOutputBus)
  {
    int busIdx1based = outputBusIdx+1;

    if (busIdx1based < _this->mOutBuses.GetSize() /*&& (_this->GetHost() != kHostAbletonLive)*/)
    {
      int totalNumChans = _this->mOutBuses.GetSize() * 2; // stereo only for the time being
      int nConnected = busIdx1based * 2;
      _this->SetChannelConnections(ERoute::kOutput, nConnected, totalNumChans - nConnected, false); // this will disconnect the channels that are on the unconnected buses
    }

    if (_this->GetBypassed())
    {
      _this->PassThroughBuffers((AudioSampleType) 0, nFrames);
    }
    else
    {
      if(_this->mMidiMsgsFromEditor.ElementsAvailable())
      {
        IMidiMsg msg;
        
        while (_this->mMidiMsgsFromEditor.Pop(msg))
        {
          _this->ProcessMidiMsg(msg);
        }
      }
      
      _this->PreProcess();
      ENTER_PARAMS_MUTEX
      _this->ProcessBuffers((AudioSampleType) 0, nFrames);
      LEAVE_PARAMS_MUTEX
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
  
  _this->OutputSysexFromEditor();

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

#pragma mark - IPlugAU Constructor

IPlugAU::IPlugAU(const InstanceInfo& info, const Config& config)
: IPlugAPIBase(config, kAPIAU)
, IPlugProcessor(config, kAPIAU)
{
  Trace(TRACELOC, "%s", config.pluginName);

  memset(&mHostCallbacks, 0, sizeof(HostCallbackInfo));
  memset(&mMidiCallback, 0, sizeof(AUMIDIOutputCallbackStruct));

  mCocoaViewFactoryClassName.Set(info.mCocoaViewFactoryClassName.Get());

  const int maxNIBuses = MaxNBuses(ERoute::kInput);
  const int maxNOBuses = MaxNBuses(ERoute::kOutput);

  PtrListInitialize(&mInBusConnections, maxNIBuses);
  PtrListInitialize(&mInBuses, maxNIBuses);
  
  for (auto bus = 0; bus < maxNIBuses; bus++)
  {
    BusChannels* pInBus = mInBuses.Get(bus);
    pInBus->mNHostChannels = -1;
    pInBus->mPlugChannelStartIdx = 0;
    pInBus->mNPlugChannels = std::abs(MaxNChannelsForBus(ERoute::kInput, bus));
  }
  
  PtrListInitialize(&mOutBuses, maxNOBuses);
  
  for (auto bus = 0; bus < maxNOBuses; bus++)
  {
    BusChannels* pOutBus = mOutBuses.Get(bus);
    pOutBus->mNHostChannels = -1;
    pOutBus->mPlugChannelStartIdx = 0;
    pOutBus->mNPlugChannels = std::abs(MaxNChannelsForBus(ERoute::kOutput, bus));
  }

  AssessInputConnections();

  SetBlockSize(DEFAULT_BLOCK_SIZE);
  ResizeScratchBuffers();
  
  CreateTimer();
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

void IPlugAU::InformHostOfParameterDetailsChange()
{
  InformListeners(kAudioUnitProperty_ParameterList, kAudioUnitScope_Global);
  InformListeners(kAudioUnitProperty_ParameterInfo, kAudioUnitScope_Global);
}

void IPlugAU::PreProcess()
{
  ITimeInfo timeInfo;
  
  if (mHostCallbacks.beatAndTempoProc)
  {
    double currentBeat = 0.0, tempo = 0.0;
    mHostCallbacks.beatAndTempoProc(mHostCallbacks.hostUserData, &currentBeat, &tempo);

    if (tempo > 0.0)
      timeInfo.mTempo = tempo;
    
    if (currentBeat >= 0.0)
      timeInfo.mPPQPos = currentBeat;
  }

  if (mHostCallbacks.transportStateProc)
  {
    double samplePos = 0.0, loopStartBeat=0.0, loopEndBeat=0.0;
    Boolean playing, changed, looping;
    mHostCallbacks.transportStateProc(mHostCallbacks.hostUserData, &playing, &changed, &samplePos, &looping, &loopStartBeat, &loopEndBeat);

    if (samplePos > 0.0)
      timeInfo.mSamplePos = samplePos;
    
    if (loopStartBeat > 0.0)
      timeInfo.mCycleStart = loopStartBeat;
    
    if (loopEndBeat > 0.0)
      timeInfo.mCycleEnd = loopEndBeat;
    
    timeInfo.mTransportIsRunning = playing;
    timeInfo.mTransportLoopEnabled = looping;
  }

  UInt32 sampleOffsetToNextBeat = 0, tsDenom = 0;
  float tsNum = 0.0f;
  double currentMeasureDownBeat = 0.0;
  
  if (mHostCallbacks.musicalTimeLocationProc)
  {
    mHostCallbacks.musicalTimeLocationProc(mHostCallbacks.hostUserData, &sampleOffsetToNextBeat, &tsNum, &tsDenom, &currentMeasureDownBeat);

    timeInfo.mNumerator = (int) tsNum;
    timeInfo.mDenominator = (int) tsDenom;
    if (currentMeasureDownBeat>0.0)
      timeInfo.mLastBar=currentMeasureDownBeat;
  }
    
  SetTimeInfo(timeInfo);
}

void IPlugAU::ResizeScratchBuffers()
{
  TRACE
  int NInputs = MaxNChannels(ERoute::kInput) * GetBlockSize();
  int NOutputs = MaxNChannels(ERoute::kOutput) * GetBlockSize();
  mInScratchBuf.Resize(NInputs);
  mOutScratchBuf.Resize(NOutputs);
  memset(mInScratchBuf.Get(), 0, NInputs * sizeof(AudioSampleType));
  memset(mOutScratchBuf.Get(), 0, NOutputs * sizeof(AudioSampleType));
}

void IPlugAU::InformListeners(AudioUnitPropertyID propID, AudioUnitScope scope)
{
  TRACE
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
  TRACE
  int i, n = mPropertyListeners.GetSize();
  
  for (i = 0; i < n; ++i)
  {
    PropertyListener* pListener = mPropertyListeners.Get(i);
    if (pListener->mPropID == kAudioUnitProperty_Latency)
    {
      pListener->mListenerProc(pListener->mProcArgs, mCI, kAudioUnitProperty_Latency, kAudioUnitScope_Global, 0);
    }
  }
  
  IPlugProcessor::SetLatency(samples);
}

bool IPlugAU::SendMidiMsg(const IMidiMsg& msg)
{
  if(mMidiCallback.midiOutputCallback == nullptr)
    return false;
  
  MIDIPacketList packetList;
  
  packetList.packet[0].data[0] = msg.mStatus;
  packetList.packet[0].data[1] = msg.mData1;
  packetList.packet[0].data[2] = msg.mData2;
  packetList.packet[0].length = 3;
  packetList.packet[0].timeStamp = msg.mOffset;
  packetList.numPackets = 1;
  
  if(mMidiCallback.midiOutputCallback)
  {
    OSStatus status = mMidiCallback.midiOutputCallback(mMidiCallback.userData, &mLastRenderTimeStamp, 0, &packetList);
    
    if (status == noErr)
      return true;
  }
  
  return false;
}

bool IPlugAU::SendMidiMsgs(WDL_TypedBuf<IMidiMsg>& msgs)
{
  bool result = false;
  
  if(mMidiCallback.midiOutputCallback == nullptr)
    return false;
  
  WDL_HeapBuf heapBuf;
  MIDIPacketList* pPktlist = (MIDIPacketList*) heapBuf.ResizeOK(msgs.GetSize() * 3);
  MIDIPacket* pPkt = MIDIPacketListInit(pPktlist);
  ByteCount listSize = heapBuf.GetSize();
    
  IMidiMsg* pMsg = msgs.Get();
  for (int i = 0; i < msgs.GetSize(); ++i, ++pMsg)
  {
    pPkt = MIDIPacketListAdd(pPktlist, listSize, pPkt, pMsg->mOffset /* TODO: is this correct? */, 1, &pMsg->mStatus);
    pPkt = MIDIPacketListAdd(pPktlist, listSize, pPkt, pMsg->mOffset /* TODO: is this correct? */, 1, &pMsg->mData1);
    pPkt = MIDIPacketListAdd(pPktlist, listSize, pPkt, pMsg->mOffset /* TODO: is this correct? */, 1, &pMsg->mData2);
  }
  
  if(mMidiCallback.midiOutputCallback)
  {
    OSStatus status = mMidiCallback.midiOutputCallback(mMidiCallback.userData, &mLastRenderTimeStamp, 0, pPktlist);
    
    if (status == noErr)
      result = true;
  }
  
  return result;
}

bool IPlugAU::SendSysEx(const ISysEx& sysEx)
{
  bool result = false;

  if(mMidiCallback.midiOutputCallback == nullptr)
    return false;

  assert(sysEx.mSize <= 65536); // maximum packet list size
  
  WDL_HeapBuf heapBuf;
  MIDIPacketList* pPktlist = (MIDIPacketList*) heapBuf.ResizeOK(sysEx.mSize);
  MIDIPacket* pPkt = MIDIPacketListInit(pPktlist);
  
  ByteCount listSize = heapBuf.GetSize();
  ByteCount bytesLeft = listSize;
  
  while (bytesLeft) {
    ByteCount packetSize = listSize < 256 ? listSize : 256;
    pPkt = MIDIPacketListAdd(pPktlist, listSize, pPkt, sysEx.mOffset /* TODO: is this correct? */, packetSize, sysEx.mData);
    bytesLeft -= packetSize;
  }
  
  assert(pPkt != nullptr);
  
  if(mMidiCallback.midiOutputCallback)
  {
    OSStatus status = mMidiCallback.midiOutputCallback(mMidiCallback.userData, &mLastRenderTimeStamp, 0, pPktlist);
    
    if (status == noErr)
      result = true;
  }
  
  return result;
}

void IPlugAU::OutputSysexFromEditor()
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

#pragma mark - IPlugAU Dispatch

#if MAC_OS_X_VERSION_MAX_ALLOWED >= 1070

static IPlugAU* GetPlug(void *x)
{
  return (IPlugAU*) &((AudioComponentPlugInInstance *) x)->mInstanceStorage;
}

//static
OSStatus IPlugAU::AUMethodInitialize(void* pSelf)
{
  return DoInitialize(GetPlug(pSelf));
}

//static
OSStatus IPlugAU::AUMethodUninitialize(void* pSelf)
{
  return DoUninitialize(GetPlug(pSelf));
}

//static
OSStatus IPlugAU::AUMethodGetPropertyInfo(void* pSelf, AudioUnitPropertyID prop, AudioUnitScope scope, AudioUnitElement elem, UInt32* outDataSize, Boolean* outWritable)
{
  return DoGetPropertyInfo(GetPlug(pSelf), prop, scope, elem, outDataSize, outWritable);
}

//static
OSStatus IPlugAU::AUMethodGetProperty(void* pSelf, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, void* outData, UInt32* ioDataSize)
{
  return DoGetProperty(GetPlug(pSelf), inID, inScope, inElement, outData, ioDataSize);
}

//static
OSStatus IPlugAU::AUMethodSetProperty(void* pSelf, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, const void* inData, UInt32* inDataSize)
{
  return DoSetProperty(GetPlug(pSelf), inID, inScope, inElement, inData, inDataSize);
}

//static
OSStatus IPlugAU::AUMethodAddPropertyListener(void* pSelf, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc, void* userData)
{
  return DoAddPropertyListener(GetPlug(pSelf), prop, proc, userData);
}

//static
OSStatus IPlugAU::AUMethodRemovePropertyListener(void* pSelf, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc)
{
  return DoRemovePropertyListener(GetPlug(pSelf), prop, proc);
}

//static
OSStatus IPlugAU::AUMethodRemovePropertyListenerWithUserData(void* pSelf, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc, void* userData)
{
  return DoRemovePropertyListenerWithUserData(GetPlug(pSelf), prop, proc, userData);
}

//static
OSStatus IPlugAU::AUMethodAddRenderNotify(void* pSelf, AURenderCallback proc, void* userData)
{
  return DoAddRenderNotify(GetPlug(pSelf), proc, userData);
}

//static
OSStatus IPlugAU::AUMethodRemoveRenderNotify(void* pSelf, AURenderCallback proc, void* userData)
{
  return DoRemoveRenderNotify(GetPlug(pSelf), proc, userData);
}

//static
OSStatus IPlugAU::AUMethodGetParameter(void* pSelf, AudioUnitParameterID param, AudioUnitScope scope, AudioUnitElement elem, AudioUnitParameterValue* value)
{
  return DoGetParameter(GetPlug(pSelf), param, scope, elem, value);
}

//static
OSStatus IPlugAU::AUMethodSetParameter(void* pSelf, AudioUnitParameterID param, AudioUnitScope scope, AudioUnitElement elem, AudioUnitParameterValue value, UInt32 bufferOffset)
{
  return DoSetParameter(GetPlug(pSelf), param, scope, elem, value, bufferOffset);
}

//static
OSStatus IPlugAU::AUMethodScheduleParameters(void* pSelf, const AudioUnitParameterEvent *pEvent, UInt32 nEvents)
{
  return DoScheduleParameters(GetPlug(pSelf), pEvent, nEvents);
}

//static
OSStatus IPlugAU::AUMethodRender(void* pSelf, AudioUnitRenderActionFlags* ioActionFlags, const AudioTimeStamp* inTimeStamp, UInt32 inOutputBusNumber, UInt32 inNumberFrames, AudioBufferList* ioData)
{
  return DoRender(GetPlug(pSelf), ioActionFlags, inTimeStamp, inOutputBusNumber, inNumberFrames, ioData);
}

//static
OSStatus IPlugAU::AUMethodReset(void* pSelf, AudioUnitScope scope, AudioUnitElement elem)
{
  return DoReset(GetPlug(pSelf));
}

//static
OSStatus IPlugAU::AUMethodMIDIEvent(void* pSelf, UInt32 inStatus, UInt32 inData1, UInt32 inData2, UInt32 inOffsetSampleFrame)
{
  return DoMIDIEvent(GetPlug(pSelf), inStatus, inData1, inData2, inOffsetSampleFrame);
}

//static
OSStatus IPlugAU::AUMethodSysEx(void* pSelf, const UInt8* inData, UInt32 inLength)
{
  return DoSysEx(GetPlug(pSelf), inData, inLength);
}
#endif

//static
OSStatus IPlugAU::DoInitialize(IPlugAU* _this)
{
  if (_this->GetHost() == kHostUninit)
  {
    CFBundleRef mainBundle = CFBundleGetMainBundle();
    CFStringRef id = nullptr;
    int version = 0;
      
    if (mainBundle)
    {
      id = CFBundleGetIdentifier(mainBundle);
      CStrLocal versionStr((CFStringRef) CFBundleGetValueForInfoDictionaryKey(mainBundle, kCFBundleVersionKey));
      
      char *pStr;
      long ver = versionStr.Get() ? strtol(versionStr.Get(), &pStr, 10) : 0;
      long verRevMaj = versionStr.Get() && *pStr ? strtol(pStr + 1, &pStr, 10) : 0;
      long verRevMin = versionStr.Get() && *pStr ? strtol(pStr + 1, &pStr, 10) : 0;

      version = (int) (((ver & 0xFFFF) << 16) | ((verRevMaj &  0xFF) << 8) | (verRevMin & 0xFF));
    }

    _this->SetHost(id ? CStrLocal(id).Get() : "", version);
  }
    
  if (!(_this->CheckLegalIO()))
  {
    return badComponentSelector;
  }

  _this->mActive = true;
  _this->OnParamReset(kReset);
  _this->OnActivate(true);
  
  return noErr;
}

//static
OSStatus IPlugAU::DoUninitialize(IPlugAU* _this)
{
  _this->mActive = false;
  _this->OnActivate(false);
  return noErr;
}

//static
OSStatus IPlugAU::DoGetPropertyInfo(IPlugAU* _this, AudioUnitPropertyID prop, AudioUnitScope scope, AudioUnitElement elem, UInt32* outDataSize, Boolean* outWritable)
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
OSStatus IPlugAU::DoGetProperty(IPlugAU* _this, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, void* outData, UInt32 *ioDataSize)
{
  UInt32 dataSize = 0;
  
  if (!ioDataSize)
    ioDataSize = &dataSize;
  
  Boolean writeable = false;
  
  return _this->GetProperty(inID, inScope, inElement, ioDataSize, &writeable, outData);
}

//static
OSStatus IPlugAU::DoSetProperty(IPlugAU* _this, AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement, const void* inData, UInt32* inDataSize)
{
  return _this->SetProperty(inID, inScope, inElement, inDataSize, inData);
}

//static
OSStatus IPlugAU::DoAddPropertyListener(IPlugAU* _this, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc, void* userData)
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
OSStatus IPlugAU::DoRemovePropertyListener(IPlugAU* _this, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc)
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
OSStatus IPlugAU::DoRemovePropertyListenerWithUserData(IPlugAU* _this, AudioUnitPropertyID prop, AudioUnitPropertyListenerProc proc, void* userData)
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
OSStatus IPlugAU::DoAddRenderNotify(IPlugAU* _this, AURenderCallback proc, void* userData)
{
  AURenderCallbackStruct acs;
  acs.inputProc = proc;
  acs.inputProcRefCon = userData;
  
  PtrListAddFromStack(&(_this->mRenderNotify), &acs);
  return noErr;
}

//static
OSStatus IPlugAU::DoRemoveRenderNotify(IPlugAU* _this, AURenderCallback proc, void* userData)
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
OSStatus IPlugAU::DoGetParameter(IPlugAU* _this, AudioUnitParameterID param, AudioUnitScope scope, AudioUnitElement elem, AudioUnitParameterValue *value)
{
  //mutex locked below
  return _this->GetParamProc(_this, param, scope, elem, value);
}

//static
OSStatus IPlugAU::DoSetParameter(IPlugAU* _this, AudioUnitParameterID param, AudioUnitScope scope, AudioUnitElement elem, AudioUnitParameterValue value, UInt32 bufferOffset)
{
  //mutex locked below
  return _this->SetParamProc(_this, param, scope, elem, value, bufferOffset);
}

//static
OSStatus IPlugAU::DoScheduleParameters(IPlugAU* _this, const AudioUnitParameterEvent *pEvent, UInt32 nEvents)
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
OSStatus IPlugAU::DoRender(IPlugAU* _this, AudioUnitRenderActionFlags* ioActionFlags, const AudioTimeStamp* inTimeStamp, UInt32 inOutputBusNumber, UInt32 inNumberFrames, AudioBufferList* ioData)
{
  return RenderProc(_this, ioActionFlags, inTimeStamp, inOutputBusNumber, inNumberFrames, ioData);
}

//static
OSStatus IPlugAU::DoReset(IPlugAU* _this)
{
  _this->OnReset();
  return noErr;
}

//static
OSStatus IPlugAU::DoMIDIEvent(IPlugAU* _this, UInt32 inStatus, UInt32 inData1, UInt32 inData2, UInt32 inOffsetSampleFrame)
{
  if(_this->DoesMIDIIn())
  {
    IMidiMsg msg;
    msg.mStatus = inStatus;
    msg.mData1 = inData1;
    msg.mData2 = inData2;
    msg.mOffset = inOffsetSampleFrame;
    _this->ProcessMidiMsg(msg);
    _this->mMidiMsgsFromProcessor.Push(msg);
    return noErr;
  }
  else
    return badComponentSelector;
}

//static
OSStatus IPlugAU::DoSysEx(IPlugAU* _this, const UInt8* inData, UInt32 inLength)
{
  if(_this->DoesMIDIIn())
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

