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

namespace clap
{
#ifdef DEBUG
  template class helpers::Plugin<helpers::MisbehaviourHandler::Terminate,helpers::CheckingLevel::Maximal>;
#else
  template class helpers::Plugin<helpers::MisbehaviourHandler::Ignore,helpers::CheckingLevel::None>;
#endif
}

using namespace iplug;

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
//
//bool IPlugCLAP::EditorResize(int viewWidth, int viewHeight)
//{
//  bool resized = false;
//
//  if (HasUI())
//  {
//    if (viewWidth != GetEditorWidth() || viewHeight != GetEditorHeight())
//    {
//      SetEditorSize(viewWidth, viewHeight);
//
//    }
//  }
//
//  return resized;
//}

// IPlugProcessor

void IPlugCLAP::SetLatency(int samples)
{
  IPlugProcessor::SetLatency(samples);
}

bool IPlugCLAP::SendMidiMsg(const IMidiMsg& msg)
{
  return false;
}

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
    auto transport = process->transport;
    
    ITimeInfo timeInfo;
    
    // TODO - this upper bit all needs review
    
    timeInfo.mTempo = transport->tempo;
    timeInfo.mSamplePos = process->steady_time;
    timeInfo.mPPQPos = transport->song_pos_beats;       // TODO convert
    timeInfo.mLastBar = transport->bar_start;           // TODO convert
    timeInfo.mCycleStart = transport->loop_start_beats; // TODO convert
    timeInfo.mCycleEnd = transport->loop_end_beats;

    timeInfo.mNumerator = transport->tsig_num;
    timeInfo.mDenominator = transport->tsig_denom;

    timeInfo.mTransportIsRunning = transport->flags & CLAP_TRANSPORT_IS_PLAYING;
    timeInfo.mTransportLoopEnabled = transport->flags & CLAP_TRANSPORT_IS_LOOP_ACTIVE;
  }
  
  // Input Events
  
  ProcessInputEvents(process->in_events);
  
  while (mMidiMsgsFromEditor.Pop(msg))
  {
    ProcessMidiMsg(msg);
  }
  
  // Do Audio Processing!
  
  // TODO - can we assume IO has the same format?
  // TO - get the count from the right place...
  int nIns = process->audio_inputs->channel_count;
  int nOuts = process->audio_outputs->channel_count;
  int nFrames = process->frames_count;
  
  bool format64 = process->audio_inputs->data64;
  
  assert(format64 == (bool) process->audio_outputs->data64);
  
  SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), false);
  SetChannelConnections(ERoute::kInput, 0, nIns, true);
  
  if (format64)
    AttachBuffers(ERoute::kInput, 0, nIns, process->audio_inputs->data64, nFrames);
  else
    AttachBuffers(ERoute::kInput, 0, nIns, process->audio_inputs->data32, nFrames);

  SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), false);
  SetChannelConnections(ERoute::kOutput, 0, nOuts, true);
  
  if (format64)
    AttachBuffers(ERoute::kOutput, 0, nOuts, process->audio_outputs->data64, nFrames);
  else
    AttachBuffers(ERoute::kOutput, 0, nOuts, process->audio_outputs->data32, nFrames);

  if (format64)
    ProcessBuffers(0.0, nFrames);
  else
    ProcessBuffers(0.f, nFrames);
  
  auto out_events = process->out_events;
  
  // Send Parameter Changes
  
  ProcessOutputParams(out_events);
  
  // Send Events Out
  /*
   if (process->out_events)
   {
     auto out_events = process->out_events;

     for (int i = 0; i < in_events->size(in_events); i++)
     {
       auto event = in_events->get(in_events, i);
             
       switch (event->type)
       {
         case CLAP_EVENT_NOTE_ON:
         {
           // N.B. velocity stored 0-1
           int velocity = std::round(event->note.velocity * 127.0);
           msg.MakeNoteOnMsg(event->note.key, velocity, event->time, event->note.channel);
           ProcessMidiMsg(msg);
           mMidiMsgsFromProcessor.Push(msg);
         }
         
         case CLAP_EVENT_NOTE_OFF:
         {
           msg.MakeNoteOffMsg(event->note.key, event->time, event->note.channel);
           ProcessMidiMsg(msg);
           mMidiMsgsFromProcessor.Push(msg);
         }
           
         case CLAP_EVENT_MIDI:
         {
           msg = IMidiMsg(event->time, event->midi.data[0], event->midi.data[1], event->midi.data[2]);
           ProcessMidiMsg(msg);
           mMidiMsgsFromProcessor.Push(msg);
         }
         
         case CLAP_EVENT_MIDI_SYSEX:
         {
           ISysEx sysEx(event->time, event->midi_sysex.buffer, event->midi_sysex.size);
           ProcessSysEx(sysEx);
           //mSysExDataFromProcessor.Push(sysEx);
         }
         
         case CLAP_EVENT_PARAM_VALUE:
         {
           int paramIdx = event->param_value.param_id;
           double value = event->param_value.value;
           
           GetParam(paramIdx)->Set(value);
           SendParameterValueFromAPI(paramIdx, value, false);
           OnParamChange(paramIdx, EParamSource::kHost, event->time);
         }
           
         default:
           break;
       }
     }
   }
   */
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
      
  return UnserializeState(chunk, 0) >= 0;
}

// clap_plugin_params

bool IPlugCLAP::paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept
{
  assert(MAX_PARAM_NAME_LEN <= CLAP_NAME_SIZE && "iPlug parameter name size exceeds CLAP maximum");
  assert(MAX_PARAM_GROUP_LEN <= CLAP_MODULE_SIZE && "iPlug group name size exceeds CLAP maximum");

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
  // TODO - check if any of the input events are not param changes?
  
  ProcessInputEvents(input_parameter_changes);
  ProcessOutputParams(output_parameter_changes);
}

void IPlugCLAP::ProcessInputEvents(const clap_input_events *in_events) noexcept
{
  IMidiMsg msg;

  if (in_events)
  {
    for (int i = 0; i < in_events->size(in_events); i++)
    {
      auto event = in_events->get(in_events, i);
      
      if (event->space_id != CLAP_CORE_EVENT_SPACE_ID)
        continue;
      
      switch (event->type)
      {
        case CLAP_EVENT_NOTE_ON:
        {
          // N.B. velocity stored 0-1
          // TODO - check velocity
          auto note = ClapEventCast<clap_event_note>(event);
          int velocity = std::round(note->velocity * 127.0);
          msg.MakeNoteOnMsg(note->key, velocity, event->time, note->channel);
          ProcessMidiMsg(msg);
          mMidiMsgsFromProcessor.Push(msg);
        }
          
        case CLAP_EVENT_NOTE_OFF:
        {
          auto note = ClapEventCast<clap_event_note>(event);
          msg.MakeNoteOffMsg(note->key, event->time, note->channel);
          ProcessMidiMsg(msg);
          mMidiMsgsFromProcessor.Push(msg);
        }
          
        case CLAP_EVENT_MIDI:
        {
          auto midi = ClapEventCast<clap_event_midi>(event);
          msg = IMidiMsg(event->time, midi->data[0], midi->data[1], midi->data[2]);
          ProcessMidiMsg(msg);
          mMidiMsgsFromProcessor.Push(msg);
        }
          
        case CLAP_EVENT_MIDI_SYSEX:
        {
          auto midiSysex = ClapEventCast<clap_event_midi_sysex>(event);
          
          ISysEx sysEx(event->time, midiSysex->buffer, midiSysex->size);
          ProcessSysEx(sysEx);
          //mSysExDataFromProcessor.Push(sysEx);
        }
          
        case CLAP_EVENT_PARAM_VALUE:
        {
          auto paramValue = ClapEventCast<clap_event_param_value>(event);
          
          int paramIdx = paramValue->param_id;
          double value = paramValue->value;
          
          GetParam(paramIdx)->Set(value);
          SendParameterValueFromAPI(paramIdx, value, false);
          OnParamChange(paramIdx, EParamSource::kHost, event->time);
        }
          
        default:
          break;
      }
    }
  }
}
  
void IPlugCLAP::ProcessOutputParams(const clap_output_events *output_parameter_changes) noexcept
{
  ParamToHost change;
  
  while (mParamInfoToHost.Pop(change))
  {
    // Construct output stream
    
    clap_event_header_t header;
    
    header.size = sizeof(clap_event_param_value);
    header.time = 0;
    header.space_id = CLAP_CORE_EVENT_SPACE_ID;
    header.type = change.type();
    header.flags = 0;
    
    clap_event_param_value event { header, change.idx(), nullptr, -1, -1, -1, change.value() };
    
    // FIX - respond to a situation in which parameters can't be pushed
    
    output_parameter_changes->try_push(output_parameter_changes, &event.header);
  }
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
  SetScreenScale(scale);
  return true;
}

bool IPlugCLAP::guiGetSize(uint32_t *width, uint32_t *height) noexcept
{
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
  SetEditorSize(width, height);
  
  return true;
}

void IPlugCLAP::guiRoundSize(uint32_t *width, uint32_t *height) noexcept
{
  if (HasUI())
  {
    int w = *width;
    int h = *height;
    ConstrainEditorResize(w, h);
    *width = w;
    *height = h;
  }
}

#endif /* PLUG_HOST_RESIZE */

#endif /* PLUG_HAS_UI */

