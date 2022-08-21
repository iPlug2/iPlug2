/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#include <cstdio>
#include "IPlugCLAP.h"
#include "IPlugPluginBase.h"
#include "plugin.hxx"
#include "host-proxy.hxx"

// Ensure that the template is defined here

using namespace iplug;

void ClapNameCopy(char *destination, const char *source)
{
  strncpy(destination, source, CLAP_NAME_SIZE);
  destination[CLAP_NAME_SIZE - 1] = 0;
}

IPlugCLAP::IPlugCLAP(const InstanceInfo& info, const Config& config)
  : IPlugAPIBase(config, kAPICLAP)
  , IPlugProcessor(config, kAPICLAP)
  , ClapPluginHelper(info.mDesc, info.mHost)
{
  Trace(TRACELOC, "%s", config.pluginName);
  
  int version = 0;
  
  if (CStringHasContents(info.mHost->version))
  {
    // TODO - check host version string
    int ver, rmaj, rmin;
    sscanf(info.mHost->version, "%d.%d.%d", &ver, &rmaj, &rmin);
    version = (ver << 16) + (rmaj << 8) + rmin;
  }
  
  // Create space to store audio pointers
  
  int nChans = std::max(MaxNChannels(kInput), MaxNChannels(kOutput));
  mAudioIO32.Resize(nChans);
  mAudioIO64.Resize(nChans);
  
  SetHost(info.mHost->name, version);
  CreateTimer();
}

void IPlugCLAP::BeginInformHostOfParamChange(int idx)
{
  ParamToHost change { ParamToHost::Type::Begin, idx, GetParam(idx)->Value() };
  mParamInfoToHost.Push(change);
}

void IPlugCLAP::InformHostOfParamChange(int idx, double normalizedValue)
{
  const IParam *pParam = GetParam(idx);
  const double value = pParam->FromNormalized(normalizedValue);
  ParamToHost change { ParamToHost::Type::Value, idx, value };
  mParamInfoToHost.Push(change);
}

void IPlugCLAP::EndInformHostOfParamChange(int idx)
{
  ParamToHost change { ParamToHost::Type::End, idx, GetParam(idx)->Value() };
  mParamInfoToHost.Push(change);
}

//
//void IPlugCLAP::InformHostOfPresetChange()
//{
//}

bool IPlugCLAP::EditorResize(int viewWidth, int viewHeight)
{
  if (HasUI())
  {
    if (viewWidth != GetEditorWidth() || viewHeight != GetEditorHeight())
    {
      _host.guiRequestResize(viewWidth, viewHeight);
    }
    
    SetEditorSize(viewWidth, viewHeight);
  }
  
  return true;
}

// IPlugProcessor

void IPlugCLAP::SetTailSize(int samples)
{
  IPlugProcessor::SetTailSize(samples);
  if (_host.canUseTail())
    _host.tailChanged();
}

void IPlugCLAP::SetLatency(int samples)
{
  IPlugProcessor::SetLatency(samples);
}

bool IPlugCLAP::SendMidiMsg(const IMidiMsg& msg)
{
  mMidiOutputQueue.Add(msg);
  return true;}

bool IPlugCLAP::SendSysEx(const ISysEx& msg)
{
  return false;
}

// clap_plugin

bool IPlugCLAP::init() noexcept
{
  return true;
}

bool IPlugCLAP::activate(double sampleRate, uint32_t minFrameCount, uint32_t maxFrameCount) noexcept
{
  SetBlockSize(maxFrameCount);
  SetSampleRate(sampleRate);
  OnActivate(true);
  OnParamReset(kReset);
  OnReset();

  return true;
}

void IPlugCLAP::deactivate() noexcept
{
  OnActivate(false);
}

template <typename T>
const T* ClapEventCast(const clap_event_header_t *event)
{
  return reinterpret_cast<const T*>(event);
}

clap_process_status IPlugCLAP::process(const clap_process *process) noexcept
{
  IMidiMsg msg;

  // Transport Info
  
  if (process->transport)
  {
    ITimeInfo timeInfo;

    auto &transport = process->transport;

    constexpr double beatFactor = static_cast<double>(CLAP_BEATTIME_FACTOR);
    constexpr double secFactor = static_cast<double>(CLAP_SECTIME_FACTOR);
            
    timeInfo.mTempo = transport->tempo;
    timeInfo.mSamplePos = (GetSampleRate() * static_cast<double>(transport->song_pos_seconds) / secFactor);
    timeInfo.mPPQPos = static_cast<double>(transport->song_pos_beats) / beatFactor;
    timeInfo.mLastBar = static_cast<double>(transport->bar_start) / beatFactor;
    timeInfo.mCycleStart = static_cast<double>(transport->loop_start_beats) / beatFactor;
    timeInfo.mCycleEnd = static_cast<double>(transport->loop_end_beats) / beatFactor;

    timeInfo.mNumerator = static_cast<int>(transport->tsig_num);
    timeInfo.mDenominator = static_cast<int>(transport->tsig_denom);

    timeInfo.mTransportIsRunning = transport->flags & CLAP_TRANSPORT_IS_PLAYING;
    timeInfo.mTransportLoopEnabled = transport->flags & CLAP_TRANSPORT_IS_LOOP_ACTIVE;
    
    SetTimeInfo(timeInfo);
  }
  
  // Input Events
  
  ProcessInputEvents(process->in_events);
  
  while (mMidiMsgsFromEditor.Pop(msg))
  {
    ProcessMidiMsg(msg);
  }
  
  // Do Audio Processing!
  
  int nIns = 0;
  int nOuts = 0;
  int nFrames = process->frames_count;

  // Sum IO channels
  
  for (uint32_t i = 0; i < process->audio_inputs_count; i++)
    nIns += static_cast<int>(process->audio_inputs[i].channel_count);
  
  for (uint32_t i = 0; i < process->audio_outputs_count; i++)
    nOuts += static_cast<int>(process->audio_outputs[i].channel_count);
  
  // Check the format
  
  bool format64 = false;
  
  if (process->audio_inputs_count && process->audio_inputs && process->audio_inputs[0].channel_count)
    format64 = process->audio_inputs[0].data64;
  else if (process->audio_outputs_count && process->audio_outputs && process->audio_outputs[0].channel_count)
    format64 = process->audio_outputs[0].data64;
      
  // Assert that all formats match
  
#ifndef NDEBUG
  for (uint32_t i = 0; i < process->audio_inputs_count; i++)
  {
    auto bus = process->audio_inputs[i];
    assert(!bus.channel_count || format64 == static_cast<bool>(bus.data64));
  }
  
  for (uint32_t i = 0; i < process->audio_outputs_count; i++)
  {
    auto bus = process->audio_outputs[i];
    assert(!bus.channel_count ||  format64 == static_cast<bool>(bus.data64));
  }
#endif
  
  for (uint32_t i = 0; i < process->audio_outputs_count; i++)
    nIns += static_cast<int>(process->audio_outputs->channel_count);

  SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), false);
  SetChannelConnections(ERoute::kInput, 0, nIns, true);

  if (nIns > 0)
  {
    // Copy and attach buffer pointers
    
    if (format64)
    {
      for (uint32_t i = 0, k = 0; i < process->audio_inputs_count; i++)
      {
        auto bus = process->audio_inputs[i];
        for (uint32_t j = 0; j < bus.channel_count; j++, k++)
          mAudioIO64.Get()[k] = bus.data64[j];
      }
      
      AttachBuffers(ERoute::kInput, 0, nIns, mAudioIO64.Get(), nFrames);
    }
    else
    {
      for (uint32_t i = 0, k = 0; i < process->audio_inputs_count; i++)
      {
        auto bus = process->audio_inputs[i];
        for (uint32_t j = 0; j < bus.channel_count; j++, k++)
          mAudioIO32.Get()[k] = bus.data32[j];
      }
      
      AttachBuffers(ERoute::kInput, 0, nIns, mAudioIO32.Get(), nFrames);
    }
  }

  SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), false);
  SetChannelConnections(ERoute::kOutput, 0, nOuts, true);

  if (nOuts > 0)
  {
    // Copy and attach buffer pointers

    if (format64)
    {
      for (uint32_t i = 0, k = 0; i < process->audio_outputs_count; i++)
      {
        auto bus = process->audio_outputs[i];
        for (uint32_t j = 0; j < bus.channel_count; j++, k++)
          mAudioIO64.Get()[k] = bus.data64[j];
      }
      
      AttachBuffers(ERoute::kOutput, 0, nOuts, mAudioIO64.Get(), nFrames);
    }
    else
    {
      for (uint32_t i = 0, k = 0; i < process->audio_outputs_count; i++)
      {
        auto bus = process->audio_outputs[i];
        for (uint32_t j = 0; j < bus.channel_count; j++, k++)
          mAudioIO32.Get()[k] = bus.data32[j];
      }

      AttachBuffers(ERoute::kOutput, 0, nOuts, mAudioIO32.Get(), nFrames);
    }
  }

  if (format64)
    ProcessBuffers(0.0, nFrames);
  else
    ProcessBuffers(0.f, nFrames);
    
  // Send Events Out (Parameters and Midi)
    
  ProcessOutputEvents(process->out_events, nFrames);
  
  return CLAP_PROCESS_CONTINUE;
}

// clap_plugin_render

bool IPlugCLAP::renderSetMode(clap_plugin_render_mode mode) noexcept
{
  SetRenderingOffline(mode == CLAP_RENDER_OFFLINE);
  return true;
}

// clap_plugin_state

bool IPlugCLAP::stateSave(const clap_ostream *stream) noexcept
{
  IByteChunk chunk;
  
  if (!SerializeState(chunk))
    return false;
  
  return stream->write(stream, chunk.GetData(), chunk.Size()) == chunk.Size();
}

bool IPlugCLAP::stateLoad(const clap_istream *stream) noexcept
{
  constexpr int bytesPerBlock = 256;
  char buffer[bytesPerBlock];
  int64_t bytesRead = 0;
  
  IByteChunk chunk;
    
  while ((bytesRead = stream->read(stream, buffer, bytesPerBlock)) > 0)
    chunk.PutBytes(buffer, static_cast<int>(bytesRead));

  if (bytesRead != 0)
    return false;
      
  bool restoredOK = UnserializeState(chunk, 0) >= 0;
  
  if (restoredOK)
    OnRestoreState();
  
  return restoredOK;
}

// clap_plugin_params

bool IPlugCLAP::paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept
{
  assert(MAX_PARAM_NAME_LEN <= CLAP_NAME_SIZE && "iPlug parameter name size exceeds CLAP maximum");
  assert(MAX_PARAM_GROUP_LEN <= CLAP_PATH_SIZE && "iPlug group name size exceeds CLAP maximum");

  const IParam *pParam = GetParam(paramIndex);
  
  clap_param_info_flags flags = 0;

  if (pParam->GetStepped())
    flags |= CLAP_PARAM_IS_STEPPED;
  if (pParam->GetCanAutomate())
    flags |= CLAP_PARAM_IS_AUTOMATABLE;
  
  info->id = paramIndex;
  info->flags = flags;
  info->cookie = nullptr;

  strcpy(info->name, pParam->GetName());
  strcpy(info->module, pParam->GetGroup());

  // Values
  
  info->min_value = pParam->GetMin();
  info->max_value = pParam->GetMax();
  info->default_value = pParam->GetDefault();
  
  return true;
}

bool IPlugCLAP::paramsValue(clap_id paramId, double *value) noexcept
{
  const IParam *pParam = GetParam(paramId);
  *value = pParam->Value();
  return true;
}

bool IPlugCLAP::paramsValueToText(clap_id paramId, double value, char *display, uint32_t size) noexcept
{
  const IParam *pParam = GetParam(paramId);
  WDL_String str;
  
  pParam->GetDisplay(value, false, str);
  
  // Add Label
  
  if (CStringHasContents(pParam->GetLabel()))
  {
    str.Append(" ");
    str.Append(pParam->GetLabel());
  }
  
  if (size < str.GetLength())
    return false;
    
  strcpy(display, str.Get());
  return true;
}

bool IPlugCLAP::paramsTextToValue(clap_id paramId, const char *display, double *value) noexcept
{
  const IParam *pParam = GetParam(paramId);
  *value = pParam->StringToValue(display);
  return true;
}

void IPlugCLAP::paramsFlush(const clap_input_events *input_parameter_changes, const clap_output_events *output_parameter_changes) noexcept
{
  ProcessInputEvents(input_parameter_changes);
  ProcessOutputParams(output_parameter_changes);
}

void IPlugCLAP::ProcessInputEvents(const clap_input_events *inputEvents) noexcept
{
  IMidiMsg msg;

  if (inputEvents)
  {
    for (int i = 0; i < inputEvents->size(inputEvents); i++)
    {
      auto event = inputEvents->get(inputEvents, i);
      
      if (event->space_id != CLAP_CORE_EVENT_SPACE_ID)
        continue;
      
      switch (event->type)
      {
        case CLAP_EVENT_NOTE_ON:
        {
          // N.B. velocity stored 0-1
          auto note = ClapEventCast<clap_event_note>(event);
          auto velocity = static_cast<int>(std::round(note->velocity * 127.0));
          msg.MakeNoteOnMsg(note->key, velocity, event->time, note->channel);
          ProcessMidiMsg(msg);
          mMidiMsgsFromProcessor.Push(msg);
          break;
        }
          
        case CLAP_EVENT_NOTE_OFF:
        {
          auto note = ClapEventCast<clap_event_note>(event);
          msg.MakeNoteOffMsg(note->key, event->time, note->channel);
          ProcessMidiMsg(msg);
          mMidiMsgsFromProcessor.Push(msg);
          break;
        }
          
        case CLAP_EVENT_MIDI:
        {
          auto midi = ClapEventCast<clap_event_midi>(event);
          msg = IMidiMsg(event->time, midi->data[0], midi->data[1], midi->data[2]);
          ProcessMidiMsg(msg);
          mMidiMsgsFromProcessor.Push(msg);
          break;
        }
          
        case CLAP_EVENT_MIDI_SYSEX:
        {
          auto midiSysex = ClapEventCast<clap_event_midi_sysex>(event);
          
          ISysEx sysEx(event->time, midiSysex->buffer, midiSysex->size);
          ProcessSysEx(sysEx);
          //mSysExDataFromProcessor.Push(sysEx);
          break;
        }
          
        case CLAP_EVENT_PARAM_VALUE:
        {
          auto paramValue = ClapEventCast<clap_event_param_value>(event);
          
          int paramIdx = paramValue->param_id;
          double value = paramValue->value;
          
          GetParam(paramIdx)->Set(value);
          SendParameterValueFromAPI(paramIdx, value, false);
          OnParamChange(paramIdx, EParamSource::kHost, event->time);
          break;
        }
          
        default:
          break;
      }
    }
  }
}
  
void IPlugCLAP::ProcessOutputParams(const clap_output_events *outputParamChanges) noexcept
{
  ParamToHost change;
  
  while (mParamInfoToHost.Pop(change))
  {
    // Construct output stream
    
    clap_event_header_t header;
    
    header.size = sizeof(clap_event_param_value);
    header.time = 0; // TODO - check this
    header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    header.type = change.type();
    header.flags = 0; // TODO - check this
    
    // TODO - respond to situations in which parameters can't be pushed
    
    clap_event_param_value event { header, change.idx(), nullptr, -1, -1, -1, -1, change.value() };
    outputParamChanges->try_push(outputParamChanges, &event.header);
  }
}

void IPlugCLAP::ProcessOutputEvents(const clap_output_events *outputEvents, int nFrames) noexcept
{
  // TODO - ordering of events!!!
  
  ProcessOutputParams(outputEvents);
  
  if (outputEvents)
  {
    while (mMidiOutputQueue.ToDo())
    {
      auto msg = mMidiOutputQueue.Peek();
      auto status = msg.mStatus;
      
      // Construct output stream
      
      clap_event_header_t header;
      
      header.size = sizeof(clap_event_param_value);
      header.time = msg.mOffset;
      header.space_id = CLAP_CORE_EVENT_SPACE_ID;
      header.type = CLAP_EVENT_MIDI;
      header.flags = 0; // TODO - check this
      
      if (msg.StatusMsg() == IMidiMsg::kNoteOn)
        header.type = CLAP_EVENT_NOTE_ON;
      
      if (msg.StatusMsg() == IMidiMsg::kNoteOff)
        header.type = CLAP_EVENT_NOTE_OFF;
      
      // TODO - respond to situations in which parameters can't be pushed

      if (header.type == CLAP_EVENT_NOTE_ON || header.type == CLAP_EVENT_NOTE_OFF)
      {
        int16_t channel = static_cast<int16_t>(msg.Channel());
        clap_event_note note_event { header, -1, 0,  channel, msg.mData1, static_cast<double>(msg.mData2) / 127.0};
        outputEvents->try_push(outputEvents, &note_event.header);
      }
      else
      {
        clap_event_midi midi_event { header, 0, { status, msg.mData1, msg.mData2 } };
        outputEvents->try_push(outputEvents, &midi_event.header);
      }
      
      mMidiOutputQueue.Remove();
    }
    
    mMidiOutputQueue.Flush(nFrames);

    /*
      case CLAP_EVENT_MIDI_SYSEX:
      {
        auto midiSysex = ClapEventCast<clap_event_midi_sysex>(event);
        
        ISysEx sysEx(event->time, midiSysex->buffer, midiSysex->size);
        ProcessSysEx(sysEx);
        //mSysExDataFromProcessor.Push(sysEx);
      }
    */
  }
}

const char *ClapPortType(uint32_t nChans)
{
  return nChans == 2 ? CLAP_PORT_STEREO : (nChans == 1 ? CLAP_PORT_MONO : nullptr);
}

bool IPlugCLAP::implementsAudioPorts() const noexcept
{
  return MaxNBuses(ERoute::kInput) || MaxNBuses(ERoute::kOutput);
}

uint32_t IPlugCLAP::audioPortsCount(bool isInput) const noexcept
{
  return MaxNBuses(isInput ? ERoute::kInput : ERoute::kOutput);
}

bool IPlugCLAP::audioPortsInfo(uint32_t index, bool isInput, clap_audio_port_info *info) const noexcept
{
  // TODO - wildcards return as -1 chans...
  // TODO - what if the config hasn't been set??
  // TODO - both sets of ids below
  
  const auto direction = isInput ? ERoute::kInput : ERoute::kOutput;
  const auto nBuses = MaxNBuses(direction);
  const auto nChans = mConfigIdx < 0 ? MaxNChannelsForBus(direction, index) : GetIOConfig(mConfigIdx)->NChansOnBusSAFE(direction, static_cast<int>(index));
  WDL_String busName;
  GetBusName(direction, index, nBuses, busName);
  
  constexpr uint32_t bitFlags = CLAP_AUDIO_PORT_SUPPORTS_64BITS
                              | CLAP_AUDIO_PORT_PREFERS_64BITS
                              | CLAP_AUDIO_PORT_REQUIRES_COMMON_SAMPLE_SIZE;
  
  info->id = index;
  ClapNameCopy(info->name, busName.Get());
  info->flags = !index ? bitFlags | CLAP_AUDIO_PORT_IS_MAIN : bitFlags;
  info->channel_count = static_cast<uint32_t>(nChans);
  info->port_type = ClapPortType(info->channel_count);
  info->in_place_pair = CLAP_INVALID_ID;
  return true;
}

bool IPlugCLAP::implementsAudioPortsConfig() const noexcept
{
  return audioPortsConfigCount();
}

uint32_t IPlugCLAP::audioPortsConfigCount() const noexcept
{
  return static_cast<uint32_t>(NIOConfigs());
}

bool IPlugCLAP::audioPortsGetConfig(uint32_t index, clap_audio_ports_config *config) const noexcept
{
  const IOConfig* ioConfig = GetIOConfig(index);

  WDL_String configName;

  // TODO - wildcards return as -1 chans...
  // TODO - configs in iPlug have no names so we reconstruct the iplug io style strings for now...

  auto getChans = [&](ERoute direction, int idx)
  {
    return static_cast<uint32_t>(ioConfig->GetBusInfo(direction, idx)->NChans());
  };
  
  auto getDirectionName = [&](ERoute direction)
  {
    configName.AppendFormatted(CLAP_NAME_SIZE, "%d", getChans(kInput, 0));
    
    for (int i = 0; i < ioConfig->NBuses(direction); i++)
      configName.AppendFormatted(CLAP_NAME_SIZE, ".%d", getChans(kInput, i));
  };
  
  if (index >= audioPortsConfigCount())
    return false;
  
  getDirectionName(kInput);
  configName.Append("-");
  getDirectionName(kOutput);

  config->id = index;
  ClapNameCopy(config->name, configName.Get());
 
  config->input_port_count = static_cast<uint32_t>(ioConfig->NBuses(kInput));
  config->output_port_count = static_cast<uint32_t>(ioConfig->NBuses(kInput));

  config->has_main_input = config->input_port_count > 1;
  config->main_input_channel_count = config->has_main_input ? getChans(kInput, 0) : 0;
  config->main_input_port_type = ClapPortType(config->main_input_channel_count);
  
  config->has_main_input = config->output_port_count > 1;
  config->main_output_channel_count = config->has_main_input ? getChans(kOutput, 0) : 0;
  config->main_output_port_type = ClapPortType(config->main_output_channel_count);

  return true;
}

bool IPlugCLAP::audioPortsSetConfig(clap_id configId) noexcept
{
  if (configId >= audioPortsConfigCount())
    return false;
  
  mConfigIdx = static_cast<int>(configId);
  
  return true;
}

uint32_t IPlugCLAP::notePortsCount(bool is_input) const noexcept
{
  if (is_input)
    return PLUG_DOES_MIDI_IN ? 1 : 0;
  else
    return PLUG_DOES_MIDI_OUT ? 1 : 0;
}

bool IPlugCLAP::notePortsInfo(uint32_t index, bool is_input, clap_note_port_info *info) const noexcept
{
  if (is_input)
  {
    info->id = 0;
    info->supported_dialects = CLAP_NOTE_DIALECT_MIDI;
    info->preferred_dialect = CLAP_NOTE_DIALECT_MIDI;
    ClapNameCopy(info->name, "MIDI Input");
  }
  else
  {
    info->id = 0;
    info->supported_dialects = CLAP_NOTE_DIALECT_MIDI;
    info->preferred_dialect = CLAP_NOTE_DIALECT_MIDI;
    ClapNameCopy(info->name, "MIDI Output");
  }
  return true;
}

#if PLUG_HAS_UI

// clap_plugin_gui

void IPlugCLAP::guiDestroy() noexcept
{
  CloseWindow();
  mGUIOpen = false;
}

bool IPlugCLAP::guiShow() noexcept
{
  if (!mGUIOpen)
  {
    OpenWindow(mWindow);
    return true;
  }
  
  return false;
}

bool IPlugCLAP::guiHide() noexcept
{
  guiDestroy();
  return true;
}

bool IPlugCLAP::guiSetScale(double scale) noexcept
{
  SetScreenScale(static_cast<float>(scale));
  return true;
}

bool IPlugCLAP::guiGetSize(uint32_t* width, uint32_t* height) noexcept
{
  TRACE
  
  if (HasUI())
  {
    *width = GetEditorWidth();
    *height = GetEditorHeight();
    
    return true;
  }
  
  return false;
}

// clap_plugin_gui_cocoa

bool IPlugCLAP::GUIWindowAttach(void *window) noexcept
{
  OpenWindow(window);
  mWindow = window;
  mGUIOpen = true;
  return true;
}

#if PLUG_HOST_RESIZE
bool IPlugCLAP::guiSetSize(uint32_t width, uint32_t height) noexcept
{
  Trace(TRACELOC, "width:%i height:%i\n", width, height);

  OnParentWindowResize(width, height);
  
  return true;
}

bool IPlugCLAP::guiAdjustSize(uint32_t* width, uint32_t* height) noexcept
{
  Trace(TRACELOC, "width:%i height:%i\n", *width, *height);

  if (HasUI())
  {
    int w = *width;
    int h = *height;
    ConstrainEditorResize(w, h);
    *width = w;
    *height = h;

    return true;
  }
  
  return false;
}
#endif /* PLUG_HOST_RESIZE */

#endif /* PLUG_HAS_UI */

