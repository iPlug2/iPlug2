/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/
#include "IPlugLV2.h"
#include <lv2/urid/urid.h>
#include <lv2/options/options.h>
#include <lv2/buf-size/buf-size.h>

#define NOTIMP printf("%s: not implemented\n", __FUNCTION__);

BEGIN_IPLUG_NAMESPACE

#ifdef IPLUG_DSP

IPlugLV2DSP::IPlugLV2DSP(const InstanceInfo &info, const Config& config)
  : IPlugAPIBase(config, kAPILV2)
  , IPlugProcessor(config, kAPILV2)
{
  // maybe: info.descriptor should match our expectation, should we check that?
 
  Trace(TRACELOC, "%s", config.pluginName);
  
  int nInputs = MaxNChannels(ERoute::kInput), nOutputs = MaxNChannels(ERoute::kOutput), nParams = NParams();
  mPorts = new void *[nInputs + nOutputs + nParams];
  
  SetSampleRate(info.rate);
  
  int block_size = DEFAULT_BLOCK_SIZE; // that can lead to allocation in RT, but there is no workaround in case host does not specify it
  
  LV2_URID_Map *urid_map = nullptr;
  //LV2_URID_Unmap *urid_unmap = nullptr;
  const LV2_Options_Option *options = nullptr;
  auto features = info.features;
  if (features)
  {
    const LV2_Feature *feature;
    while((feature = *features++))
    {
      if(!strcmp(feature->URI, LV2_OPTIONS__options))
      {
        options = (const LV2_Options_Option *)feature->data;
      } else if(!strcmp(feature->URI, LV2_URID__map))
      {
        urid_map = (LV2_URID_Map *)feature->data;
      } else if(!strcmp(feature->URI, LV2_URID__unmap))
      {
        // urid_unmap = (LV2_URID_Unmap *)feature->data;
      }
    }
  }
  
  if(options && urid_map) // options we are looking for are URID based
  {
    LV2_URID maxBlockLengthID = urid_map->map(urid_map->handle, LV2_BUF_SIZE__maxBlockLength);
    // maybe: sequenceSize
    for(; options->key ; ++options)
    {
      if((options->key == maxBlockLengthID) && (options->size == sizeof(int) && options->value))
      {
        block_size =  *(int *)options->value; // at least Arodour reports theoretical maximum here, not currently used buffer size
      }
    }
  }
  SetBlockSize(block_size);

  // Default everything to connected, maybe: support less inputs/outpus then max (with separate descriptor, like Mono/Stereo/Surround 
  SetChannelConnections(ERoute::kInput, 0, nInputs, true);
  SetChannelConnections(ERoute::kOutput, 0, nOutputs, true);
  
  // TODO: CreateTimer();
}

IPlugLV2DSP::~IPlugLV2DSP()
{
  delete mPorts;
}

// Private methods


//IPlugProcessor
bool IPlugLV2DSP::SendMidiMsg(const IMidiMsg& msg)
{
  NOTIMP
  return false;
}


//LV2 methods

void IPlugLV2DSP::connect_port(uint32_t port, void *data)
{
  // maybe : check the size
  mPorts[port] = data;
}

void IPlugLV2DSP::activate()
{
  OnActivate(true);
}

void IPlugLV2DSP::run(uint32_t n_samples)
{
  int nInputs = MaxNChannels(ERoute::kInput), nOutputs = MaxNChannels(ERoute::kOutput), nParams = NParams();

  if(GetBlockSize() < n_samples)
  {
    // if host has no maxBlockLength, we can get there. Strictly speaking we violate hardRT by allocation,
    // but what else can we do in such case? Make the feature "required" and so do not support this host at all? 
    SetBlockSize(n_samples);
  }

  AttachBuffers(ERoute::kInput, 0, nInputs, (float **)mPorts, n_samples);
  AttachBuffers(ERoute::kOutput, 0, nOutputs, (float **)(mPorts + nInputs), n_samples);

  ENTER_PARAMS_MUTEX;
  for (int i = 0; i < nParams; ++i)
  {
    float *pParam = (float *)mPorts[nInputs + nOutputs + i];
    if (pParam)
    {
      IParam *param = GetParam(i);
      if(param->Value() != *pParam){
        param->Set(*pParam);
        // SendParameterValueFromAPI make no big sense for LV2, GUI is always separate
        OnParamChange(i, kHost);
      }
    }
  }
  LEAVE_PARAMS_MUTEX;

  // TODO: parameters
  // TODO: time info
  // TODO: midi

  ProcessBuffers((float) 0.0f, n_samples);  
}

void IPlugLV2DSP::deactivate()
{
  OnActivate(false);
  OnReset();
}

#endif

#ifdef IPLUG_EDITOR

IPlugLV2Editor::IPlugLV2Editor(const InstanceInfo &info, const Config& config) : IPlugAPIBase(config, kAPILV2)
, mHostSupportIdle(false), mHostWidget(nullptr), mHostResize(nullptr)
{ 
  Trace(TRACELOC, "%s", config.pluginName);

  WDL_PtrList<IOConfig> IOConfigs;
  int totalNInChans, totalNOutChans;
  int totalNInBuses, totalNOutBuses;
  IPlugProcessor::ParseChannelIOStr(config.channelIOStr, IOConfigs, totalNInChans, totalNOutChans, totalNInBuses, totalNOutBuses);
  
  mParameterPortOffset = totalNInChans + totalNOutChans;

  mEmbed = xcbt_embed_idle();
 
  mHostWrite      = info.write_function;
  mHostController = info.controller;

  auto features = info.features;
  if (features)
  {
    const LV2_Feature *feature;
    while((feature = *features++))
    {
      if(!strcmp(feature->URI, LV2_UI__parent))
      {
        mHostWidget = (LV2UI_Widget)feature->data;
      } else if(!strcmp(feature->URI, LV2_UI__idleInterface))
      {
        mHostSupportIdle = true;
      } else if(!strcmp(feature->URI, LV2_UI__resize))
      {
        mHostResize = (LV2UI_Resize *)feature->data;
      } else
      {
        // printf("Host feature: %s\n", feature->URI);
      }
    }
  }

     
  // TODO: CreateTimer();
}

LV2UI_Widget IPlugLV2Editor::CreateUI()
{
  // we can not do this in constructor, user code is not yet executed and so graphics can not be created
  SetIntegration(mEmbed);
  auto widget = reinterpret_cast<LV2UI_Widget>(OpenWindow(mHostWidget));
  EditorResize();
  return widget;
}

IPlugLV2Editor::~IPlugLV2Editor()
{
  CloseWindow();
  xcbt_embed_dtor(mEmbed);
}

void IPlugLV2Editor::InformHostOfParamChange(int idx, double normalizedValue)
{
  // I use original (not normilized) value in LV2
  ENTER_PARAMS_MUTEX_STATIC;
  float value = GetParam(idx)->Value();
  LEAVE_PARAMS_MUTEX_STATIC;
  
  uint32_t port_index = mParameterPortOffset + idx;
  
  if (mHostWrite)
  {
    mHostWrite(mHostController, port_index, sizeof(float), 0, &value);
  }
}


void IPlugLV2Editor::port_event(uint32_t port_index, uint32_t buffer_size, uint32_t format, const void*  buffer)
{
  if ((format == 0) && (buffer_size == sizeof(float)) && buffer)
  {
    float value = *((float *)buffer);
    if (port_index >= mParameterPortOffset)
    {
      int idx = port_index - mParameterPortOffset;
      // printf("  Param %d = %f\n", idx, value);
      if (idx < NParams())
      {
        ENTER_PARAMS_MUTEX_STATIC;
        GetParam(idx)->Set(value);
        SendParameterValueFromDelegate(idx, value, false);
        OnParamChange(idx, kHost);
        LEAVE_PARAMS_MUTEX_STATIC;
      }
    }
  }
}

int IPlugLV2Editor::ui_idle()
{
  xcbt_embed_idle_cb(mEmbed);
  return 0;
}

bool IPlugLV2Editor::EditorResizeFromUI(int viewWidth, int viewHeight)
{
  if (mHostResize)
  {
    return mHostResize->ui_resize(mHostResize->handle, viewWidth, viewHeight) == 0;
  }
  return false;
}


#endif

END_IPLUG_NAMESPACE
