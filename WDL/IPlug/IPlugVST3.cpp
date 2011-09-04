#include "IPlugVST3.h"
#include "IGraphics.h"
#include <stdio.h>
#include "pluginterfaces/base/ustring.h"
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"

IPlugVST3::IPlugVST3(IPlugInstanceInfo instanceInfo, int nParams, const char* channelIOStr, int nPresets,
           const char* effectName, const char* productName, const char* mfrName,
           int vendorVersion, int uniqueID, int mfrID, int latency, 
           bool plugDoesMidi, bool plugDoesChunks, bool plugIsInst, int plugScChans)
: IPlugBase(nParams, channelIOStr, nPresets, effectName, productName, mfrName, vendorVersion, uniqueID, mfrID, latency, plugDoesMidi, plugDoesChunks, plugIsInst)
{ 
  mDoesMidi = plugDoesMidi;
  mScChans = plugScChans;
  mSideChainIsConnected = false;
  SetInputChannelConnections(0, NInChannels(), true);
  SetOutputChannelConnections(0, NOutChannels(), true);
}

IPlugVST3::~IPlugVST3()
{
}

#pragma mark -
#pragma mark AudioEffect overrides

tresult PLUGIN_API IPlugVST3::initialize (FUnknown* context)
{
  tresult result = SingleComponentEffect::initialize (context);
  
  if (result == kResultOk)
  {
    addAudioInput (STR16("Audio Input"), getSpeakerArrForChans(NInChannels()) );
    addAudioOutput (STR16("Audio Output"), getSpeakerArrForChans(NOutChannels()) );
    
    if (mScChans == 1)
      addAudioInput(STR16("Sidechain Input"), SpeakerArr::kMono, kAux, 0);
    else if (mScChans >= 2)
    {
      mScChans = 2;
      addAudioInput(STR16("Sidechain Input"), SpeakerArr::kStereo, kAux, 0);
    }
        
    if(mDoesMidi) {
      addEventInput (STR16("MIDI In"), 1);
      addEventOutput(STR16("MIDI Out"), 1);
    }
    
    for (int i=0;i<NParams();i++)
    {
      IParam *p = GetParam(i);
      
      int32 flags = 0;
      
      if (p->GetCanAutomate()) {
        flags |= ParameterInfo::kCanAutomate;
      }
            
      switch (p->Type()) {
        case IParam::kTypeDouble:
        case IParam::kTypeInt:
        {
          Parameter* param = new RangeParameter ( STR16(p->GetNameForHost()), 
                                                  i, 
                                                  STR16(p->GetLabelForHost()), 
                                                  p->GetMin(), 
                                                  p->GetMax(), 
                                                  p->GetDefault(),
                                                  p->GetStep(),
                                                  flags);
          
          param->setPrecision (p->GetPrecision());
          parameters.addParameter (param);

          break;
        }
        case IParam::kTypeEnum:
        case IParam::kTypeBool: 
        {
          StringListParameter* param = new StringListParameter (STR16(p->GetNameForHost()), 
                                                                i,
                                                                STR16(p->GetLabelForHost()),
                                                                flags | ParameterInfo::kIsList);
          
          int nDisplayTexts = p->GetNDisplayTexts();
          
          assert(nDisplayTexts);

          for (int j=0; j<nDisplayTexts; j++) 
          {
            param->appendString(STR16(p->GetDisplayText(j)));
          }
          
          parameters.addParameter (param);
          break; 
        }
        default:
          break;
      }
      
    }
  }
  
  return result;
}

tresult PLUGIN_API IPlugVST3::terminate  ()
{
  viewsArray.removeAll ();
  
  return SingleComponentEffect::terminate ();
}

tresult PLUGIN_API IPlugVST3::setBusArrangements(SpeakerArrangement* inputs, int32 numIns, SpeakerArrangement* outputs, int32 numOuts)
{
  //inputs
  AudioBus* bus = getAudioInput(0);
  
  if (bus && bus->getArrangement() != inputs[0])
  {
    if (inputs[0] == SpeakerArr::kMono)
    {
      //re-create the busses..
      audioInputs.remove(bus);
      
      if (mScChans)
      {
        bus = getAudioInput(0);
        if (bus && bus->getArrangement() != inputs[1]) //sidechain SpeakerArr:: must match the input SpeakerArr::
        {
          audioInputs.remove(bus);
        }
      }
      
      addAudioInput(USTRING ("Mono In"),  SpeakerArr::kMono);
      addAudioInput(USTRING ("Mono Sidechain In"), SpeakerArr::kMono, kAux, 0);
      
      //disconnect the unused pins, don't worry about sidechain yet - it will get done at process()
      SetInputChannelConnections(1, NInChannels(), false);
      mSideChainIsConnected = false;
    }
  }
  
  //outputs
  bus = getAudioOutput(0);
  if (bus && bus->getArrangement() != outputs[0])
  {
    if (outputs[0] == SpeakerArr::kMono)
    {
      audioOutputs.remove(bus);
      addAudioOutput(USTRING ("Mono Out"),  SpeakerArr::kMono);
      
      //disconnect the unused pin
      SetOutputChannelConnections(1, NOutChannels(), false);
    }
  }
  
  if (mScChans)
  {
    if (getAudioInput(0)->getArrangement() == inputs[0] && 
        getAudioOutput(0)->getArrangement() == outputs[0] && 
        getAudioInput(1)->getArrangement() == inputs[1])
      return kResultOk;
  }
  else 
  {
    if (getAudioInput(0)->getArrangement() == inputs[0] && 
        getAudioOutput(0)->getArrangement() == outputs[0])
      return kResultOk;
  }
  
  return kResultFalse;
}

tresult PLUGIN_API IPlugVST3::setActive (TBool state)
{
  OnActivate((bool) state);
  
  // TODO: check if in/out config is supported?
  return SingleComponentEffect::setActive (state);  
}

tresult PLUGIN_API IPlugVST3::setupProcessing (ProcessSetup& newSetup)
{
  mSampleRate = newSetup.sampleRate;
  IPlugBase::SetBlockSize(newSetup.maxSamplesPerBlock);
  Reset();
  
  processSetup = newSetup;

  return kResultOk;
  
  //return SingleComponentEffect::setupProcessing (newSetup); // don't think we want this because we want to be able to support 64bit (see method)
}

tresult PLUGIN_API IPlugVST3::process(ProcessData& data)
{ 
  IMutexLock lock(this); // TODO: is this the best place to lock the mutex?
  
  memcpy(&mProcessContext, data.processContext, sizeof(ProcessContext));
  
  //process parameters
  IParameterChanges* paramChanges = data.inputParameterChanges;
  if (paramChanges)
  {
    int32 numParamsChanged = paramChanges->getParameterCount();
    
    //it is possible to get a finer resolution of control here by retrieving more values (points) from the queue
    //for now we just grab the last one
    
    for (int32 i = 0; i < numParamsChanged; i++)
    {
      IParamValueQueue* paramQueue = paramChanges->getParameterData(i);
      if (paramQueue)
      {
        int32 numPoints = paramQueue->getPointCount();
        int32 offsetSamples;
        double value;
        
        if (paramQueue->getPoint(numPoints - 1,  offsetSamples, value) == kResultTrue)
        {
          int idx = paramQueue->getParameterId();
          if (idx >= 0 && idx < NParams()) 
          {
            GetParam(idx)->SetNormalized((double)value);
            if (GetGUI()) GetGUI()->SetParameterFromPlug(idx, (double)value, true);
            OnParamChange(idx);
          }
        }
      }
    }
  }
  
  if(mDoesMidi) {
    //process events.. only midi note on and note off?
    IEventList* eventList = data.inputEvents;
    if (eventList) 
    {
      int32 numEvent = eventList->getEventCount();
      for (int32 i=0; i<numEvent; i++)
      {
        Event event;
        if (eventList->getEvent(i, event) == kResultOk)
        {
          IMidiMsg msg;
          switch (event.type)
          {
            case Event::kNoteOnEvent:
            {
              msg.MakeNoteOnMsg(event.noteOn.pitch, event.noteOn.velocity * 127, event.sampleOffset, event.noteOn.channel);
              ProcessMidiMsg(&msg);
              break;
            }
              
            case Event::kNoteOffEvent:
            {
              msg.MakeNoteOffMsg(event.noteOff.pitch, event.sampleOffset, event.noteOff.channel);
              ProcessMidiMsg(&msg);
              break;
            }
          }
        }
      }
    }
  }
  
  //process audio
  if (data.numInputs == 0 || data.numOutputs == 0)
  {
    // nothing to do
    return kResultOk;
  }
  
  if (processSetup.symbolicSampleSize == kSample32)
  {
    float** in  = data.inputs[0].channelBuffers32;
    float** out = data.outputs[0].channelBuffers32;
    
    if (mScChans) 
    {
      float** side = data.inputs[1].channelBuffers32;

      if (getAudioInput(1)->isActive()) 
      {
        int totalNInputs = data.inputs[0].numChannels + data.inputs[1].numChannels;
        
        float** allInputs = new float*[totalNInputs];

        for (int i = 0; i < data.inputs[0].numChannels; i ++) {
          allInputs[i] = in[i];
        }
        
        for (int i = 0; i < data.inputs[1].numChannels; i ++) {
          allInputs[i + data.inputs[0].numChannels] = side[i];
        }
        
        AttachInputBuffers(0, totalNInputs, allInputs, data.numSamples);
        mSideChainIsConnected = true;
        
        delete [] allInputs;
      }
      else 
      {
        AttachInputBuffers(0, data.inputs[0].numChannels, in, data.numSamples);
        mSideChainIsConnected = false;
      }
    }
    else {
      AttachInputBuffers(0, data.inputs[0].numChannels, in, data.numSamples);
    }
    
    AttachOutputBuffers(0, data.outputs[0].numChannels, out);
    ProcessBuffers(0.0f, data.numSamples);
  }
  else if (processSetup.symbolicSampleSize == kSample64)
  {
    double** in  = data.inputs[0].channelBuffers64;
    double** out = data.outputs[0].channelBuffers64;
    
    AttachInputBuffers(0, data.inputs[0].numChannels, in, data.numSamples);
    AttachOutputBuffers(0, data.outputs[0].numChannels, out);
    
    ProcessBuffers(0.0, data.numSamples);
  } 
  
  // Midi Out
//  if (mDoesMidi) {
//    IEventList eventList = data.outputEvents;
//    
//    if (eventList) 
//    {
//      Event event;
//      
//      while (!mMidiOutputQueue.Empty()) {
//        //TODO: parse events and add
//        eventList.addEvent(event);
//      }
//    }
//  }
  
  return kResultOk; 
}

#pragma mark -
#pragma mark IEditController overrides

IPlugView* PLUGIN_API IPlugVST3::createView (const char* name)
{
  if (name && strcmp (name, "editor") == 0)
  {
    IPlugVST3View* view = new IPlugVST3View (this);
    addDependentView (view);
    return view;
  }
  
  return 0;
}

tresult PLUGIN_API IPlugVST3::setEditorState(IBStream* state)
{
  ByteChunk chunk;
  SerializeState(&chunk);
  
  if (chunk.Size() >= 0)
  {
    state->read(chunk.GetBytes(), chunk.Size());
    UnserializeState(&chunk, 0);
    RedrawParamControls();
    return kResultOk;
  }
  
  return kResultFalse;
}

tresult PLUGIN_API IPlugVST3::getEditorState(IBStream* state)
{
  ByteChunk chunk;
  if (SerializeState(&chunk))
  {
    state->write(chunk.GetBytes(), chunk.Size());
    return kResultOk;
  }
  return kResultFalse;
}

ParamValue PLUGIN_API IPlugVST3::plainParamToNormalized(ParamID tag, ParamValue plainValue)
{
  IParam* param = GetParam(tag);  
  
  if (param)
  {
    return param->GetNormalized(plainValue);
  }
  
  return plainValue;
}

ParamValue PLUGIN_API IPlugVST3::getParamNormalized(ParamID tag)
{
  IParam* param = GetParam(tag);  
  
  if (param)
  {
    return param->GetNormalized();
  }
  
  return 0.0;
}

tresult PLUGIN_API IPlugVST3::setParamNormalized(ParamID tag, ParamValue value)
{
  IParam* param = GetParam(tag);  
  
  if (param)
  {
    param->SetNormalized(value);
    return kResultOk;
  }
  
  return kResultFalse;
}

tresult PLUGIN_API IPlugVST3::getParamStringByValue(ParamID tag, ParamValue valueNormalized, String128 string)
{
  IParam* param = GetParam(tag);
  
  if (param)
  {
    char disp [MAX_PARAM_NAME_LEN];
    param->GetDisplayForHost(valueNormalized, true, disp);
    Steinberg::UString(string, 128).fromAscii(disp);
    return kResultTrue;
  }
  
  return kResultFalse;
}

tresult PLUGIN_API IPlugVST3::getParamValueByString (ParamID tag, TChar* string, ParamValue& valueNormalized)
{
  return SingleComponentEffect::getParamValueByString (tag, string, valueNormalized);
}

void IPlugVST3::addDependentView (IPlugVST3View* view)
{
  viewsArray.add (view);
}

void IPlugVST3::removeDependentView (IPlugVST3View* view)
{
  for (int32 i = 0; i < viewsArray.total (); i++)
  {
    if (viewsArray.at (i) == view)
    {
      viewsArray.removeAt (i);
      break;
    }
  }
}

tresult IPlugVST3::beginEdit(ParamID tag)
{
  if (componentHandler)
    return componentHandler->beginEdit(tag); 
  return kResultFalse;
}

tresult IPlugVST3::performEdit(ParamID tag, ParamValue valueNormalized)
{
  if (componentHandler)
    return componentHandler->performEdit(tag, valueNormalized);
  return kResultFalse;
}

tresult IPlugVST3::endEdit(ParamID tag)
{
  if (componentHandler)
    return componentHandler->endEdit(tag);
  return kResultFalse;
}

AudioBus* IPlugVST3::getAudioInput (int32 index)
{
  AudioBus* bus = FCast<AudioBus> (audioInputs.at(index));
  return bus;
}

AudioBus* IPlugVST3::getAudioOutput (int32 index)
{
  AudioBus* bus = FCast<AudioBus> (audioOutputs.at(index));
  return bus;
}

// TODO: more speaker arrs
SpeakerArrangement IPlugVST3::getSpeakerArrForChans(int32 chans)
{
  switch (chans) {
    case 1:
      return SpeakerArr::kMono;
    case 2:
      return SpeakerArr::kStereo;
    case 3:
      return SpeakerArr::k30Music;
    case 4:
      return SpeakerArr::k40Music;
    case 5:
      return SpeakerArr::k50;
    case 6:
      return SpeakerArr::k51;
    default:
      return SpeakerArr::kEmpty;
      break;
  }
}

#pragma mark -
#pragma mark IPlugBase overrides

void IPlugVST3::BeginInformHostOfParamChange(int idx)
{
  if (GetParam(idx)->GetCanAutomate()) 
  {
    beginEdit(idx);
  }
}

void IPlugVST3::InformHostOfParamChange(int idx, double normalizedValue)
{ 
  if (GetParam(idx)->GetCanAutomate()) 
  {
    performEdit(idx, normalizedValue);
  }
}

void IPlugVST3::EndInformHostOfParamChange(int idx)
{
  if (GetParam(idx)->GetCanAutomate()) 
  {
    endEdit(idx);
  }
}

void IPlugVST3::GetTime(ITimeInfo* pTimeInfo)
{
  //TODO: check these are all valid
  pTimeInfo->mSamplePos = (double) mProcessContext.projectTimeSamples;
  pTimeInfo->mPPQPos = mProcessContext.projectTimeMusic;
  pTimeInfo->mTempo = mProcessContext.tempo;
  pTimeInfo->mLastBar = mProcessContext.barPositionMusic;
  pTimeInfo->mCycleStart = mProcessContext.cycleStartMusic;
  pTimeInfo->mCycleEnd = mProcessContext.cycleEndMusic;
  pTimeInfo->mNumerator = mProcessContext.timeSigNumerator;
  pTimeInfo->mDenominator = mProcessContext.timeSigDenominator;
  pTimeInfo->mTransportIsRunning = mProcessContext.state & ProcessContext::kPlaying;
  pTimeInfo->mTransportLoopEnabled = mProcessContext.state & ProcessContext::kCycleActive;
}

double IPlugVST3::GetTempo()
{
  return mProcessContext.tempo;
}

void IPlugVST3::GetTimeSig(int* pNum, int* pDenom)
{
  *pNum = mProcessContext.timeSigNumerator;
  *pDenom = mProcessContext.timeSigDenominator;
}

int IPlugVST3::GetSamplePos()
{
  return (int) mProcessContext.projectTimeSamples;
}

// Only add note messages, because vst3 can't handle others
bool IPlugVST3::SendMidiMsg(IMidiMsg* pMsg)
{
  int status = pMsg->StatusMsg();
  
  switch (status)
  {
    case IMidiMsg::kNoteOn:
    case IMidiMsg::kNoteOff:
      mMidiOutputQueue.Add(pMsg);
      return true;
    default:
      return false;
  }
}

#pragma mark -
#pragma mark IPlugVST3View
IPlugVST3View::IPlugVST3View(IPlugVST3* pPlug)
: mPlug(pPlug)
{  
  rect.right = 700;
  rect.bottom = 300;
  
  if (mPlug)
    mPlug->addRef();
}

IPlugVST3View::~IPlugVST3View ()
{
  if (mPlug)
  {
    mPlug->removeDependentView (this);
    mPlug->release ();
  }
}

tresult PLUGIN_API IPlugVST3View::isPlatformTypeSupported (FIDString type)
{
  if(mPlug->GetGUI()) // for no editor plugins
  {
    #ifdef OS_WIN
    if (strcmp (type, kPlatformTypeHWND) == 0)
      return kResultTrue;
    
    #elif defined OS_OSX
    if (strcmp (type, kPlatformTypeNSView) == 0)
      return kResultTrue;
    else if (strcmp (type, kPlatformTypeHIView) == 0)
      return kResultTrue;
    #endif
  }
  
  return kResultFalse;
}

tresult PLUGIN_API IPlugVST3View::onSize (ViewRect* newSize)
{
  return CPluginView::onSize (newSize);
}

tresult PLUGIN_API IPlugVST3View::getSize (ViewRect* size) 
{
  *size = ViewRect(0, 0, mPlug->GetGUI()->Width(), mPlug->GetGUI()->Height());
  return kResultTrue;
}

tresult PLUGIN_API IPlugVST3View::attached (void* parent, FIDString type)
{
  if (mPlug->GetGUI()) 
  {
#ifdef OS_WIN
    if (strcmp (type, kPlatformTypeHWND) == 0)
      mPlug->GetGUI()->OpenWindow(parent);
#elif defined OS_OSX
    if (strcmp (type, kPlatformTypeNSView) == 0)
      mPlug->GetGUI()->OpenWindow(parent);
    else // Carbon
      mPlug->GetGUI()->OpenWindow(parent, 0);
#endif
    mPlug->OnGUIOpen();
  }
  return kResultTrue;
}

tresult PLUGIN_API IPlugVST3View::removed ()
{
  if (mPlug->GetGUI()) 
  {
    mPlug->OnGUIClose();
    mPlug->GetGUI()->CloseWindow(); 
  }
  
  return CPluginView::removed ();
}