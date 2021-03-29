 /*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#import <AVFoundation/AVFoundation.h>
#import <CoreAudioKit/AUViewController.h>
#include "BufferedAudioBus.hpp"

#import "IPlugAUAudioUnit.h"
#include "IPlugAUv3.h"
#include "AUv2/IPlugAU_ioconfig.h"

#if !__has_feature(objc_arc)
#error This file must be compiled with Arc. Use -fobjc-arc flag
#endif

using namespace iplug;

@interface IPlugAUAudioUnit ()

@property AUAudioUnitBusArray* mInputBusArray;
@property AUAudioUnitBusArray* mOutputBusArray;
@end

static AUAudioUnitPreset* NewAUPreset(NSInteger number, NSString* pName)
{
  AUAudioUnitPreset* pPreset = [AUAudioUnitPreset new];
  pPreset.number = number;
  pPreset.name = pName;
  return pPreset;
}

@implementation IPlugAUAudioUnit
{
  IPlugAUv3* mPlug;
  WDL_PtrList<BufferedInputBus> mBufferedInputBuses;
  WDL_PtrList<BufferedOutputBus> mBufferedOutputBuses;
  NSArray<NSNumber*>* mChannelCapabilitiesArray;
  NSArray<NSString*>* mMidiOutputNames;
  AUMIDIOutputEventBlock mMidiOutputEventBlock;
  AUParameterObserverToken mUIUpdateParamObserverToken;
  NSArray<AUAudioUnitPreset*>* mPresets;
  AUAudioUnitPreset *mCurrentPreset;
  NSInteger mCurrentFactoryPresetIndex;
}

@synthesize parameterTree = mParameterTree;
@synthesize factoryPresets = mPresets;

- (NSUInteger)getChannelLayoutTags: (int) dir : (AudioChannelLayoutTag*) pTags
{
  WDL_TypedBuf<uint64_t> foundTags;
  
  for(auto configIdx = 0; configIdx < mPlug->NIOConfigs(); configIdx++)
  {
    const IOConfig* pConfig = mPlug->GetIOConfig(configIdx);
    
    for(auto busIdx = 0; busIdx < pConfig->NBuses((ERoute) dir); busIdx++)
    {
      WDL_TypedBuf<uint64_t> busTypes;
      GetAPIBusTypeForChannelIOConfig(configIdx, (ERoute) dir, busIdx, pConfig, &busTypes);
      //          DBGMSG("Found %i different tags for an %s bus with %i channels\n", busTypes.GetSize(), RoutingDirStrs[dir], pConfig->GetBusInfo(dir, busIdx)->NChans());
      
      for (auto tag = 0; tag < busTypes.GetSize(); tag++)
      {
        if(foundTags.Find(busTypes.Get()[tag] == -1))
          foundTags.Add(busTypes.Get()[tag]);
      }
    }
  }
  
  if(pTags)
  {
    for (auto v = 0; v < foundTags.GetSize(); v++)
    {
      pTags[v] = (AudioChannelLayoutTag) foundTags.Get()[v];
    }
    
    DBGMSG("Adding %i tags\n", foundTags.GetSize());
    
    return 1; // success
  }
  else
    return foundTags.GetSize();
}

- (void)populateChannelCapabilitesArray: (NSMutableArray*) pArray
{
  for (int i = 0; i < mPlug->NIOConfigs(); i++)
  {
    const IOConfig* pIO = mPlug->GetIOConfig(i);
    
    if(pIO->ContainsWildcard(ERoute::kInput))
      [pArray addObject: [NSNumber numberWithInt:-1]];
    else
      [pArray addObject: [NSNumber numberWithInt:pIO->GetTotalNChannels(kInput)]];

    if(pIO->ContainsWildcard(ERoute::kOutput))
      [pArray addObject: [NSNumber numberWithInt:-1]];
    else
      [pArray addObject: [NSNumber numberWithInt:pIO->GetTotalNChannels(kOutput)]];
  }
}

- (instancetype)initWithComponentDescription:(AudioComponentDescription)componentDescription
                                     options:(AudioComponentInstantiationOptions)options
                                       error:(NSError **)ppOutError {
  
  self = [super initWithComponentDescription:componentDescription
                                     options:options
                                       error:ppOutError];
  
  if (self == nil) { return nil; }
  
  // Create a DSP kernel to handle the signal processing.
  mPlug = MakePlug(InstanceInfo());
  
  assert(mPlug);
  
  mPlug->SetAUAudioUnit((__bridge void*) self);
  
#pragma mark Audio I/O
  
  NSMutableArray* pChannelCapabilities = [[NSMutableArray alloc] init];
  [self populateChannelCapabilitesArray: pChannelCapabilities];
  mChannelCapabilitiesArray = pChannelCapabilities;
  
  int nInputBuses = mPlug->MaxNBuses(ERoute::kInput);
  int nOutputBuses = mPlug->MaxNBuses(ERoute::kOutput);

  
  if(nOutputBuses == 0) // MIDI FX
  {
    NSMutableArray* pOutputBusses = [[NSMutableArray alloc] init];
    BufferedOutputBus* pBufferedOutputBus = new BufferedOutputBus();
    int busChans = 2;
    AVAudioFormat* pOutputBusFormat = nil;
    AVAudioChannelLayout* pChannelLayout = [[AVAudioChannelLayout alloc] initWithLayoutTag: kAudioChannelLayoutTag_Stereo];
    pOutputBusFormat = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:DEFAULT_SAMPLE_RATE channelLayout:pChannelLayout ];
    
    if(pOutputBusFormat)
      pBufferedOutputBus->init(pOutputBusFormat, busChans);
    
    [pOutputBusses addObject:pBufferedOutputBus->bus];
    
    mBufferedOutputBuses.Add(pBufferedOutputBus);
    _mOutputBusArray  = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self busType:AUAudioUnitBusTypeOutput busses: pOutputBusses];
  }
  
  if(nInputBuses)
  {
    NSMutableArray* pInputBusses = [[NSMutableArray alloc] init];
    
    for(int busIdx = 0; busIdx < nInputBuses; busIdx++)
    {
      BufferedInputBus* pBufferedInputBus = new BufferedInputBus();
      int busChans = mPlug->MaxNChannelsForBus(ERoute::kInput, busIdx);
      
      AVAudioFormat* pInputBusFormat = nil;
      AVAudioChannelLayout* pChannelLayout = [[AVAudioChannelLayout alloc] initWithLayoutTag: kAudioChannelLayoutTag_Stereo]; // TODO: get tag
      pInputBusFormat = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:DEFAULT_SAMPLE_RATE channelLayout:pChannelLayout ];
      if(pInputBusFormat)
        pBufferedInputBus->init(pInputBusFormat, busChans);
      
      [pInputBusses addObject:pBufferedInputBus->bus];

      mBufferedInputBuses.Add(pBufferedInputBus);
    }
    
    _mInputBusArray  = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self busType:AUAudioUnitBusTypeInput busses: pInputBusses];
  }
  
  if(nOutputBuses)
  {
    NSMutableArray* pOutputBusses = [[NSMutableArray alloc] init];
    
    for(int busIdx = 0; busIdx < nOutputBuses; busIdx++)
    {
      BufferedOutputBus* pBufferedOutputBus = new BufferedOutputBus();
      int busChans = mPlug->MaxNChannelsForBus(ERoute::kOutput, busIdx);
      
      AVAudioFormat* pOutputBusFormat = nil;
      AVAudioChannelLayout* pChannelLayout = [[AVAudioChannelLayout alloc] initWithLayoutTag: kAudioChannelLayoutTag_Stereo]; // TODO: get tag
      pOutputBusFormat = [[AVAudioFormat alloc] initStandardFormatWithSampleRate:DEFAULT_SAMPLE_RATE channelLayout:pChannelLayout ];
      if(pOutputBusFormat)
      {
        pBufferedOutputBus->init(pOutputBusFormat, busChans);
      }
      
      [pOutputBusses addObject:pBufferedOutputBus->bus];
      
      mBufferedOutputBuses.Add(pBufferedOutputBus);
    }
    
    _mOutputBusArray  = [[AUAudioUnitBusArray alloc] initWithAudioUnit:self busType:AUAudioUnitBusTypeOutput busses: pOutputBusses];
  }
  
#pragma mark MIDI
  
  NSMutableArray* midiOutputNames = [[NSMutableArray<NSString*> alloc] init];
  
  if(mPlug->DoesMIDIOut())
    [midiOutputNames addObject:@"MIDI Output"];
  
  mMidiOutputNames = midiOutputNames;

#pragma mark parameters
  
  NSMutableArray* treeArray = [[NSMutableArray<AUParameter*> alloc] init];
  
  [treeArray addObject:[[NSMutableArray<AUParameter*> alloc] init]]; // ROOT

  for (int paramIdx = 0; paramIdx < mPlug->NParams(); paramIdx++)
  {
    IParam* pParam = mPlug->GetParam(paramIdx);
    NSString* pUnitName = nil;
    AudioUnitParameterUnit unit;
    AudioUnitParameterOptions options = 0;
    
    options |= kAudioUnitParameterFlag_IsReadable;
    options |= kAudioUnitParameterFlag_IsWritable;
    
    #ifndef IPLUG1_COMPATIBILITY // unfortunately this flag was not set for IPlug1, and it breaks state
    options |= kAudioUnitParameterFlag_IsHighResolution;
    #endif
    
    if (!pParam->GetCanAutomate()) options |= kAudioUnitParameterFlag_NonRealTime;
    if (pParam->GetMeta()) options |= kAudioUnitParameterFlag_IsElementMeta;

    switch (pParam->Type())
    {
      case IParam::kTypeBool:
        unit = kAudioUnitParameterUnit_Boolean;
        break;
      case IParam::kTypeEnum:
        unit = kAudioUnitParameterUnit_Indexed;
        break;
      case IParam::kTypeInt:
//        unit = kAudioUnitParameterUnit_Indexed; //TODO: this was the case for AUv2
//        break;
        unit = kAudioUnitParameterUnit_CustomUnit;
        pUnitName = @"";
        break;
      default:
      {
        switch (pParam->Unit())
        {
          case IParam::kUnitPercentage:     unit = kAudioUnitParameterUnit_Percent;            break;
          case IParam::kUnitSeconds:        unit = kAudioUnitParameterUnit_Seconds;            break;
          case IParam::kUnitMilliseconds:   unit = kAudioUnitParameterUnit_Milliseconds;       break;
          case IParam::kUnitSamples:        unit = kAudioUnitParameterUnit_SampleFrames;       break;
          case IParam::kUnitDB:             unit = kAudioUnitParameterUnit_Decibels;           break;
          case IParam::kUnitLinearGain:     unit = kAudioUnitParameterUnit_LinearGain;         break;
          case IParam::kUnitPan:            unit = kAudioUnitParameterUnit_Pan;                break;
          case IParam::kUnitPhase:          unit = kAudioUnitParameterUnit_Phase;              break;
          case IParam::kUnitDegrees:        unit = kAudioUnitParameterUnit_Degrees;            break;
          case IParam::kUnitMeters:         unit = kAudioUnitParameterUnit_Meters;             break;
          case IParam::kUnitRate:           unit = kAudioUnitParameterUnit_Rate;               break;
          case IParam::kUnitRatio:          unit = kAudioUnitParameterUnit_Ratio;              break;
          case IParam::kUnitFrequency:      unit = kAudioUnitParameterUnit_Hertz;              break;
          case IParam::kUnitOctaves:        unit = kAudioUnitParameterUnit_Octaves;            break;
          case IParam::kUnitCents:          unit = kAudioUnitParameterUnit_Cents;              break;
          case IParam::kUnitAbsCents:       unit = kAudioUnitParameterUnit_AbsoluteCents;      break;
          case IParam::kUnitSemitones:      unit = kAudioUnitParameterUnit_RelativeSemiTones;  break;
          case IParam::kUnitMIDINote:       unit = kAudioUnitParameterUnit_MIDINoteNumber;     break;
          case IParam::kUnitMIDICtrlNum:    unit = kAudioUnitParameterUnit_MIDIController;     break;
          case IParam::kUnitBPM:            unit = kAudioUnitParameterUnit_BPM;                break;
          case IParam::kUnitBeats:          unit = kAudioUnitParameterUnit_Beats;              break;
            
          case IParam::kUnitCustom:
            
            if (CStringHasContents(pParam->GetCustomUnit()))
            {
              unit = kAudioUnitParameterUnit_CustomUnit;
              pUnitName = [NSString stringWithCString:pParam->GetCustomUnit() encoding:NSUTF8StringEncoding];
            }
            else
            {
              unit = kAudioUnitParameterUnit_Generic;
            }
            break;
        }
      }
    }
    
    switch (pParam->DisplayType())
    {
      case IParam::kDisplayLinear: break;
      case IParam::kDisplaySquared: options |= kAudioUnitParameterFlag_DisplaySquared; break;
      case IParam::kDisplaySquareRoot: options |= kAudioUnitParameterFlag_DisplaySquareRoot; break;
      case IParam::kDisplayCubed: options |= kAudioUnitParameterFlag_DisplayCubed; break;
      case IParam::kDisplayCubeRoot: options |= kAudioUnitParameterFlag_DisplayCubeRoot; break;
      case IParam::kDisplayExp: options |= kAudioUnitParameterFlag_DisplayExponential; break;
      case IParam::kDisplayLog: options |= kAudioUnitParameterFlag_DisplayLogarithmic; break;
    }
    
    NSMutableArray* pValueStrings = nil;
    
    if(pParam->NDisplayTexts())
    {
      options |= kAudioUnitParameterFlag_ValuesHaveStrings;

      pValueStrings = [[NSMutableArray alloc] init];
      
      for(auto dt = 0; dt < pParam->NDisplayTexts(); dt++)
      {
        [pValueStrings addObject:[NSString stringWithCString:pParam->GetDisplayText(dt) encoding:NSUTF8StringEncoding]];
      }
    }
    
    const char* paramGroupName = pParam->GetGroup();
    auto clumpID = 0;

    if (CStringHasContents(paramGroupName))
    {
      options |= kAudioUnitParameterFlag_HasClump;
      
      for(auto groupIdx = 0; groupIdx< mPlug->NParamGroups(); groupIdx++)
      {
        if(strcmp(paramGroupName, mPlug->GetParamGroupName(groupIdx)) == 0)
        {
          clumpID = groupIdx+1;
        }
      }
      
      if (clumpID == 0) // a brand new clump
      {
        clumpID = mPlug->AddParamGroup(paramGroupName);
        [treeArray addObject:[[NSMutableArray<AUParameter*> alloc] init]]; // ROOT
      }
    }
    
    AUParameterAddress address = AUParameterAddress(paramIdx);

    AUParameter *pAUParam = [AUParameterTree createParameterWithIdentifier:    [NSString stringWithFormat:@"%d", paramIdx ]
                                                                         name: [NSString stringWithCString:pParam->GetName() encoding:NSUTF8StringEncoding]
                                                                      address: address
                                                                          min: pParam->GetMin()
                                                                          max: pParam->GetMax()
                                                                         unit: unit
                                                                     unitName: pUnitName
                                                                        flags: options
                                                                 valueStrings: pValueStrings
                                                          dependentParameters: nil];
    
    pAUParam.value = pParam->GetDefault();
    mPlug->AddParamAddress(paramIdx, address);
    
    [[treeArray objectAtIndex:clumpID] addObject:pAUParam];
  }
  
  NSMutableArray* rootNodeArray = [[NSMutableArray alloc] init];

  for (auto p = 0; p < [treeArray count]; p++)
  {
    if (p == 0)
    {
      for (auto j = 0; j < [[treeArray objectAtIndex:p] count]; j++)
      {
        [rootNodeArray addObject:treeArray[p][j]];
      }
    }
    else
    {
      AUParameterGroup* pGroup = [AUParameterTree createGroupWithIdentifier:[NSString stringWithString:[NSString stringWithFormat:@"Group %d", p-1]]
                                                                       name:[NSString stringWithCString:mPlug->GetParamGroupName(p-1) encoding:NSUTF8StringEncoding]
                                                                   children:treeArray[p]];

      [rootNodeArray addObject:pGroup];
    }
  }
  
  mParameterTree = [AUParameterTree createTreeWithChildren:rootNodeArray];
  
  #pragma mark presets
  // Create factory preset array.
  NSMutableArray* pPresets = [[NSMutableArray alloc] init];
  
  if(mPlug->NPresets() == 0 )
  {
    [pPresets addObject:NewAUPreset(0, @"Default")];
  }
  else
  {
    for(auto i = 0; i < mPlug->NPresets(); i++)
    {
      [pPresets addObject:NewAUPreset(i, [NSString stringWithCString: mPlug->GetPresetName(i) encoding:NSUTF8StringEncoding])];
    }
  }
  
  mCurrentFactoryPresetIndex = 0;
  mPresets = pPresets;
  mCurrentPreset = [mPresets objectAtIndex: 0];
  
  __block IPlugAUv3* pPlug = mPlug;

#pragma mark blocks
  
  // start timer on main thread
  dispatch_async(dispatch_get_main_queue(), ^{
    pPlug->CreateTimer();
  });
  
  mParameterTree.implementorValueObserver = ^(AUParameter *param, AUValue value) {
    pPlug->SetParameterFromValueObserver(param.address, value); // Set Processor Parameter
  };

  mParameterTree.implementorValueProvider = ^(AUParameter *param) {
    return pPlug->GetParameter(param.address);
  };

  mParameterTree.implementorStringFromValueCallback = ^(AUParameter *param, const AUValue *__nullable valuePtr) {
    AUValue value = valuePtr == nil ? param.value : *valuePtr;
    return [NSString stringWithCString:pPlug->GetParamDisplay(param.address, value) encoding:NSUTF8StringEncoding];
  };

  mParameterTree.implementorValueFromStringCallback = ^(AUParameter* param, NSString* string) {
    return (AUValue) pPlug->GetParamStringToValue(param.address, [string UTF8String]);
  };
  
  if (mPlug->HasUI())
  {
    mUIUpdateParamObserverToken = [mParameterTree tokenByAddingParameterObserver:^(AUParameterAddress address, AUValue value) {
                              dispatch_async(dispatch_get_main_queue(), ^{
                                pPlug->SendParameterValueFromObserver(address, value);
                              });
                            }];
    
//  [self addObserver:self forKeyPath:@"allParameterValues"
//                  options:NSKeyValueObservingOptionNew
//                  context:mUIUpdateParamObserverToken];
  }
  
//  self.maximumFramesToRender = 512;
  
  self.currentPreset = mPresets.firstObject;
  
  return self;
}

-(void)dealloc
{
  mBufferedInputBuses.Empty(true);
  mBufferedOutputBuses.Empty(true);
  
  if (mUIUpdateParamObserverToken != nullptr)
  {
    [mParameterTree removeParameterObserver:mUIUpdateParamObserverToken];
    mUIUpdateParamObserverToken = nullptr;
  }
  
  // delete on the main thread, otherwise get main thread warnings
//  dispatch_sync(dispatch_get_main_queue(), ^{
    delete self->mPlug;
//  });
}

#pragma mark - AUAudioUnit (Overrides)

- (AUAudioUnitBusArray *)inputBusses
{
  return _mInputBusArray;
}

- (AUAudioUnitBusArray *)outputBusses
{
  return _mOutputBusArray;
}

- (NSArray<NSString*> *) MIDIOutputNames
{
  return mMidiOutputNames;
}

- (BOOL)allocateRenderResourcesAndReturnError:(NSError **)ppOutError
{
  if (![super allocateRenderResourcesAndReturnError:ppOutError])
  {
    return NO;
  }
  
  uint32_t reqNumInputChannels = 0;
  uint32_t reqNumOutputChannels = 0;
  
  if(mBufferedInputBuses.GetSize())
    reqNumInputChannels = mBufferedInputBuses.Get(0)->bus.format.channelCount;
  
  if(mBufferedOutputBuses.GetSize())
    reqNumOutputChannels = mBufferedOutputBuses.Get(0)->bus.format.channelCount;
  
//  // TODO: legal io doesn't consider sidechain inputs
//  if (!mPlug->LegalIO(reqNumInputChannels, reqNumOutputChannels))
//  {
//    if (ppOutError)
//      *ppOutError = [NSError errorWithDomain:NSOSStatusErrorDomain code:kAudioUnitErr_FailedInitialization userInfo:nil];
//
//    // Notify superclass that initialization was not successful
//    self.renderResourcesAllocated = NO;
//
//    return NO;
//  }
  
  if (@available(macOS 10.13, *))
  {
    if(self.MIDIOutputEventBlock)
      mMidiOutputEventBlock = self.MIDIOutputEventBlock;
    else
      mMidiOutputEventBlock = nil;
  } else
  {
    mMidiOutputEventBlock = nil;
  }
  
    
  for (auto bufIdx = 0; bufIdx < mBufferedInputBuses.GetSize(); bufIdx++)
  {
    mBufferedInputBuses.Get(bufIdx)->allocateRenderResources(self.maximumFramesToRender);
  }
  
  for (auto bufIdx = 0; bufIdx < mBufferedOutputBuses.GetSize(); bufIdx++)
  {
    mBufferedOutputBuses.Get(bufIdx)->allocateRenderResources(self.maximumFramesToRender);
  }

  double sr = mBufferedOutputBuses.Get(0)->bus.format.sampleRate;
  mPlug->Prepare(sr, self.maximumFramesToRender);
  mPlug->OnReset();
  
  return YES;
}

- (void)deallocateRenderResources
{
  for (auto bufIdx = 0; bufIdx < mBufferedInputBuses.GetSize(); bufIdx++)
  {
    mBufferedInputBuses.Get(bufIdx)->deallocateRenderResources();
  }
  
  for (auto bufIdx = 0; bufIdx < mBufferedOutputBuses.GetSize(); bufIdx++)
  {
    mBufferedOutputBuses.Get(bufIdx)->deallocateRenderResources();
  }
  
  mMidiOutputEventBlock = nil;
  
  [super deallocateRenderResources];
}

#pragma mark - AUAudioUnit (AUAudioUnitImplementation)

- (AUInternalRenderBlock)internalRenderBlock
{

  __block IPlugAUv3* pPlug = mPlug;
  __block WDL_PtrList<BufferedInputBus>* inputBuses = &mBufferedInputBuses;
  __block WDL_PtrList<BufferedOutputBus>* outputBuses = &mBufferedOutputBuses;
  __block AUHostMusicalContextBlock _musicalContextCapture = self.musicalContextBlock;
  __block AUHostTransportStateBlock _transportStateCapture = self.transportStateBlock;
  
  return ^AUAudioUnitStatus(AudioUnitRenderActionFlags *actionFlags,
                            const AudioTimeStamp       *timestamp,
                            AVAudioFrameCount           frameCount,
                            NSInteger                   outputBusNumber,
                            AudioBufferList            *outputData,
                            const AURenderEvent        *realtimeEventListHead,
                            AURenderPullInputBlock      pullInputBlock) {
        
    AudioUnitRenderActionFlags pullFlags = 0;
    AUAudioUnitStatus err = 0;
    
    for (auto busIdx = 0; busIdx < inputBuses->GetSize(); busIdx++) {
      err = inputBuses->Get(busIdx)->pullInput(&pullFlags, timestamp, frameCount, busIdx, pullInputBlock);
    }

    if (err != 0) { return err; }
    
    AudioBufferList* pInAudioBufferList = nil;
    
    if(inputBuses->GetSize())
      pInAudioBufferList = inputBuses->Get(0)->mutableAudioBufferList; // TODO: buses > 0
    
    outputBuses->Get(0)->prepareOutputBufferList(outputData, frameCount, true); // TODO: buses > 0
    pPlug->SetBuffers(pInAudioBufferList, outputData);
    
    ITimeInfo timeInfo;

    if(_musicalContextCapture)
    {
      Float64 tempo; Float64 ppqPos; double numerator; NSInteger denominator; double currentMeasureDownbeatPosition; NSInteger sampleOffsetToNextBeat;

      _musicalContextCapture(&tempo, &numerator, &denominator, &ppqPos, &sampleOffsetToNextBeat, &currentMeasureDownbeatPosition);

      timeInfo.mTempo = tempo;
      timeInfo.mPPQPos = ppqPos;
      timeInfo.mLastBar = currentMeasureDownbeatPosition; //TODO: is that correct?
      timeInfo.mNumerator = (int) numerator; //TODO: update ITimeInfo precision?
      timeInfo.mDenominator = (int) denominator; //TODO: update ITimeInfo precision?
    }

    if(_transportStateCapture)
    {
      double samplePos; double cycleStart; double cycleEnd; AUHostTransportStateFlags transportStateFlags;

      _transportStateCapture(&transportStateFlags, &samplePos, &cycleStart, &cycleEnd);

      timeInfo.mSamplePos = samplePos;
      timeInfo.mCycleStart = cycleStart;
      timeInfo.mCycleEnd = cycleEnd;
      timeInfo.mTransportIsRunning = transportStateFlags == AUHostTransportStateMoving || transportStateFlags == AUHostTransportStateRecording;
      timeInfo.mTransportLoopEnabled = transportStateFlags == AUHostTransportStateCycling;
    }

    pPlug->ProcessWithEvents(timestamp, frameCount, realtimeEventListHead, timeInfo);
    
    return noErr;
  };
}

#pragma mark- AUAudioUnit (Optional Properties)

- (AUAudioUnitPreset *)currentPreset
{
  if (mCurrentPreset.number >= 0)
    return [mPresets objectAtIndex:mPlug->GetCurrentPresetIdx()];
  else // non factory preset
    return mCurrentPreset;
}

- (void)setCurrentPreset:(AUAudioUnitPreset *)currentPreset
{
  if (nil == currentPreset)
  {
    NSLog(@"nil passed to setCurrentPreset!");
    return;
  }

  dispatch_async(dispatch_get_main_queue(), ^{
    if (currentPreset.number >= 0)
    {
      // factory preset
      for (AUAudioUnitPreset* pFactoryPreset in self->mPresets)
      {
        if (currentPreset.number == pFactoryPreset.number)
        {
          self->mPlug->RestorePreset(int(pFactoryPreset.number));
          self->mCurrentPreset = pFactoryPreset;
          
          for (int paramIdx = 0; paramIdx < self->mPlug->NParams(); paramIdx++) {
            AUParameter* parameterToChange = [self->mParameterTree parameterWithAddress:self->mPlug->GetParamAddress(paramIdx)];
            parameterToChange.value = self->mPlug->GetParam(paramIdx)->Value();
          }
          
          break;
        }
      }
    }
    else if (currentPreset.name != nil)
    {
      self->mCurrentPreset = currentPreset;
    }
  });
}

- (BOOL)canProcessInPlace
{
  return NO;
}

- (NSArray<NSNumber*>*)channelCapabilities
{
  return mChannelCapabilitiesArray;
}

- (BOOL)shouldChangeToFormat:(AVAudioFormat*)format forBus:(AUAudioUnitBus*)bus
{
  //TODO: test io configs?
  return [super shouldChangeToFormat:format forBus:bus];
}

- (NSArray<NSNumber*>*)parametersForOverviewWithCount:(NSInteger)count
{
  WDL_TypedBuf<int> results;
  NSMutableArray<NSNumber*>* overviewParams = [[NSMutableArray<NSNumber*> alloc] init];

  mPlug->OnHostRequestingImportantParameters((int) count, results);
  
  for (auto i = 0; i < results.GetSize(); i++)
  {
    [overviewParams addObject:[NSNumber numberWithUnsignedLongLong: mPlug->GetParamAddress(results.Get()[i])]];
  }
  
  return overviewParams;
}

- (void)setRenderingOffline:(BOOL)renderingOffline
{
  mPlug->SetOffline(renderingOffline);
}

- (NSTimeInterval)latency
{
  return mPlug->GetLatency();
}

- (NSTimeInterval)tailTime
{
  return (double) mPlug->GetTailSize() / mPlug->GetSampleRate();
}

- (NSDictionary<NSString*, id>*)fullState
{
  NSMutableDictionary<NSString*, id>* pDict = [[NSMutableDictionary<NSString*, id> alloc] init];

#ifdef STEINBERG_AUWRAPPER_COMPATIBLE
  NSMutableData* pProcessorData = [[NSMutableData alloc] init];
  NSMutableData* pControllerData = [[NSMutableData alloc] init];
  
  [pDict setValue:pProcessorData forKey:@"Processor State"];
  [pDict setValue:pControllerData forKey:@"Controller State"];
  mPlug->SerializeState();
  mPlug->SerializeVST3CtrlrState();
#else
  [pDict setValue:[NSNumber numberWithInt: mPlug->GetPluginVersion(false)] forKey:[NSString stringWithUTF8String: kAUPresetVersionKey]];
  [pDict setValue:[NSNumber numberWithInt: mPlug->GetAUPluginType()] forKey:[NSString stringWithUTF8String: kAUPresetTypeKey]];
  [pDict setValue:[NSNumber numberWithInt: mPlug->GetUniqueID()] forKey:[NSString stringWithUTF8String: kAUPresetSubtypeKey]];
  [pDict setValue:[NSNumber numberWithInt: mPlug->GetMfrID()] forKey:[NSString stringWithUTF8String: kAUPresetManufacturerKey]];
  [pDict setValue:[NSString stringWithUTF8String: mPlug->GetPresetName(mPlug->GetCurrentPresetIdx())] forKey:[NSString stringWithUTF8String: kAUPresetNameKey]];

  IByteChunk chunk;
//  IByteChunk::InitChunkWithIPlugVer(chunk);
  mPlug->SerializeState(chunk);
  NSMutableData* pData = [[NSMutableData alloc] init];
  [pData replaceBytesInRange:NSMakeRange (0, chunk.Size()) withBytes:chunk.GetData()];
  [pDict setValue:pData forKey:[NSString stringWithUTF8String: kAUPresetDataKey]];
#endif
  return pDict;
}

- (void)setFullState:(NSDictionary<NSString*, id>*)newFullState
{
  NSMutableDictionary<NSString*, id>* modifiedState = [[NSMutableDictionary<NSString*, id> alloc] init];
  [modifiedState addEntriesFromDictionary:newFullState];
#ifndef STEINBERG_AUWRAPPER_COMPATIBLE
  NSData* pData = [newFullState valueForKey:[NSString stringWithUTF8String: kAUPresetDataKey]];
  IByteChunk chunk;
  chunk.PutBytes([pData bytes], static_cast<int>([pData length]));
  int pos = 0;
//  IByteChunk::GetIPlugVerFromChunk(chunk, pos);
  mPlug->UnserializeState(chunk, pos);
#endif
  
//  [super setFullState: newFullState]; // this hangs auval
}

- (NSIndexSet *)supportedViewConfigurations:(NSArray<AUAudioUnitViewConfiguration *> *)availableViewConfigurations  API_AVAILABLE(macos(10.13), ios(11))
{
  TRACE

  NSMutableIndexSet* pSet = [[NSMutableIndexSet alloc] init];
  
  for (AUAudioUnitViewConfiguration* viewConfig in availableViewConfigurations) {
    if(mPlug->OnHostRequestingSupportedViewConfiguration((int) [viewConfig width], (int) [viewConfig height])) {
      [pSet addIndex:[availableViewConfigurations indexOfObject:viewConfig]];
    }
  }
  
  return pSet;
}

- (void)selectViewConfiguration:(AUAudioUnitViewConfiguration *)viewConfiguration  API_AVAILABLE(macos(10.13), ios(11))
{
  TRACE
  mPlug->OnHostSelectedViewConfiguration((int) [viewConfiguration width], (int) [viewConfiguration height]);
}

#pragma mark - IPlugAUAudioUnit

- (void) beginInformHostOfParamChange: (uint64_t) address;
{
  AUParameter* parameterToChange = [mParameterTree parameterWithAddress:address];
  AUValue exitingValue = [parameterToChange value];
  
  [parameterToChange setValue:exitingValue originator:mUIUpdateParamObserverToken atHostTime:0 eventType:AUParameterAutomationEventTypeTouch]; // TODO: atHostTime:0 ?
}

- (void) informHostOfParamChange: (AUParameterAddress) address : (float) realValue
{
  AUParameter* parameterToChange = [mParameterTree parameterWithAddress:address];
  
  if (mUIUpdateParamObserverToken != nullptr)
    [parameterToChange setValue:realValue originator:mUIUpdateParamObserverToken];
  else
    [parameterToChange setValue:realValue];
}

- (void) endInformHostOfParamChange: (uint64_t) address;
{
  AUParameter* parameterToChange = [mParameterTree parameterWithAddress:address];
  AUValue exitingValue = [parameterToChange value];
  
  [parameterToChange setValue:exitingValue originator:mUIUpdateParamObserverToken atHostTime:0 eventType:AUParameterAutomationEventTypeRelease]; // TODO: atHostTime:0 ?
}

- (PLATFORM_VIEW*) openWindow: (PLATFORM_VIEW*) pParent
{
  PLATFORM_VIEW* pView = (__bridge PLATFORM_VIEW*) mPlug->OpenWindow((__bridge void*) pParent);

  return pView;
}

- (void)closeWindow
{
  mPlug->CloseWindow();
}

- (NSInteger)width
{
  return mPlug->GetEditorWidth();
}

- (NSInteger)height
{
  return mPlug->GetEditorHeight();
}

- (void) hostResized:(CGSize) newSize
{
  mPlug->OnParentWindowResize(newSize.width, newSize.height);
}

- (bool) sendMidiData:(int64_t) sampleTime : (NSInteger) length : (const uint8_t*) midiBytes
{
  if(mMidiOutputEventBlock)
  {
    OSStatus status = mMidiOutputEventBlock(sampleTime, 0, length, midiBytes);
    
    if(status == noErr)
      return true;
  }

  return false;
}

- (BOOL)supportsMPE
{
  return mPlug->DoesMPE() ? YES : NO;
}

@end
