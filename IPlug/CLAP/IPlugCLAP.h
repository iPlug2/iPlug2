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
 * @copydoc IPlugCLAP
 */

#include "IPlugAPIBase.h"
#include "IPlugProcessor.h"
#include "plugin.hh"

#include "config.h"   // This is your plugin's config.h.

BEGIN_IPLUG_NAMESPACE

/** Used to pass various instance info to the API class */
struct InstanceInfo
{
  const clap_plugin_descriptor* mDesc;
  const clap_host* mHost;
};

// Set the level of host checking based on if this is debug build

#ifdef DEBUG
using ClapPluginHelper = clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Terminate, clap::helpers::CheckingLevel::Maximal>;
#else
using ClapPluginHelper = clap::helpers::Plugin<clap::helpers::MisbehaviourHandler::Ignore, clap::helpers::CheckingLevel::None>;
#endif

/** CLAP API base class for an IPlug plug-in
*   @ingroup APIClasses */
class IPlugCLAP : public IPlugAPIBase
  , public IPlugProcessor
  , public ClapPluginHelper
{
  struct ParamToHost
  {
    enum class Type { Begin, Value, End };

    uint32_t idx() const { return static_cast<uint32_t>(mIdx); }
    double value() const { return mValue; }
    uint16_t type() const
    {
      switch (mType)
      {
      case Type::Begin:   return CLAP_EVENT_PARAM_GESTURE_BEGIN;
      case Type::Value:   return CLAP_EVENT_PARAM_VALUE;
      case Type::End:     return CLAP_EVENT_PARAM_GESTURE_END;
      }
    }

    Type mType;
    int mIdx;
    double mValue;
  };

public:
  IPlugCLAP(const InstanceInfo& info, const Config& config);

  // IPlugAPIBase
  void BeginInformHostOfParamChange(int idx) override;
  void InformHostOfParamChange(int idx, double normalizedValue) override;
  void EndInformHostOfParamChange(int idx) override;
  //  void InformHostOfPresetChange() override;
  //  bool EditorResize(int viewWidth, int viewHeight) override;

    // IPlugProcessor
  void SetTailSize(int tailSize) override;
  void SetLatency(int samples) override;
  bool SendMidiMsg(const IMidiMsg& msg) override;
  bool SendSysEx(const ISysEx& msg) override;

  private:

    // clap_plugin
  bool init() noexcept override;
  bool activate(double sampleRate, uint32_t minFrameCount, uint32_t maxFrameCount) noexcept override;
  void deactivate() noexcept override;
  bool startProcessing() noexcept override { return true; }
  void stopProcessing() noexcept override {}
  clap_process_status process(const clap_process *process) noexcept override;

  //void onMainThread() noexcept override {}

  // clap_plugin_latency
  bool implementsLatency() const noexcept override { return true; }
  uint32_t latencyGet() const noexcept override { return GetLatency(); }

  // clap_plugin_tail
  bool implementsTail() const noexcept override { return true; }
  uint32_t tailGet(const clap_plugin_t *plugin) const noexcept override { return GetTailSize(); }

  // clap_plugin_render
  bool implementsRender() const noexcept override { return true; }
  bool renderSetMode(clap_plugin_render_mode mode) noexcept override;

  // clap_plugin_state
  bool implementsState() const noexcept override { return true; }
  bool stateSave(const clap_ostream *stream) noexcept override;
  bool stateLoad(const clap_istream *stream) noexcept override;

  /*void stateMarkDirty() const noexcept {
     if (canUseState())
        _host.MarkDirty();
  }*/

  // clap_plugin_audio_ports
  bool implementsAudioPorts() const noexcept override { return MaxNBuses(ERoute::kInput) || MaxNBuses(ERoute::kOutput); }
  uint32_t audioPortsCount(bool isInput) const noexcept override { return MaxNBuses(isInput ? ERoute::kInput : ERoute::kOutput); }
  bool audioPortsInfo(uint32_t index, bool isInput, clap_audio_port_info *info) const noexcept override;

  // clap_plugin_note_ports
  bool implementsNotePorts() const noexcept override { return true; }
  uint32_t notePortsCount(bool is_input) const noexcept override;
  bool notePortsInfo(uint32_t index, bool is_input, clap_note_port_info *info) const noexcept override;

  // clap_plugin_params
  bool implementsParams() const noexcept override { return true; }
  uint32_t paramsCount() const noexcept override { return NParams(); }

  bool paramsInfo(uint32_t paramIndex, clap_param_info *info) const noexcept override;
  
  bool paramsValue(clap_id paramId, double *value) noexcept override;
  bool paramsValueToText(clap_id paramId, double value, char *display, uint32_t size) noexcept override;
  bool paramsTextToValue(clap_id paramId, const char *display, double *value) noexcept override;
     
  void paramsFlush(const clap_input_events *input_parameter_changes, const clap_output_events *output_parameter_changes) noexcept override;
  bool isValidParamId(clap_id paramId) const noexcept override { return paramId < NParams(); }
    
  // clap_plugin_gui
#if PLUG_HAS_UI
  bool implementsGui() const noexcept override { return true; }
  bool guiCreate(const char *api, bool isFloating) noexcept override { return true; }
  void guiDestroy() noexcept override;
  
  bool guiSetScale(double scale) noexcept override;
  bool guiShow() noexcept override;
  bool guiHide() noexcept override;
  bool guiGetSize(uint32_t *width, uint32_t *height) noexcept override;
  
#if PLUG_HOST_RESIZE
  bool guiCanResize() const noexcept override { return true; }
  bool guiSetSize(uint32_t width, uint32_t height) noexcept override;
  void guiRoundSize(uint32_t *width, uint32_t *height) noexcept override;
#endif
  
#ifdef OS_WIN
  // clap_plugin_gui_win32
  bool guiIsApiSupported(const char *api, bool isFloating) noexcept override { return !strcmp(api, CLAP_WINDOW_API_WIN32); }
  bool guiSetParent(const clap_window *window) noexcept override { return GUIWindowAttach(window->win32);  }
#endif
  
#ifdef OS_MAC
  // clap_plugin_gui_cocoa
  bool guiIsApiSupported(const char *api, bool isFloating) noexcept override { return !strcmp(api, CLAP_WINDOW_API_COCOA); }
  bool guiSetParent(const clap_window *window) noexcept override { return GUIWindowAttach(window->cocoa); }
#endif
  
  // Helper to attach GUI Windows
  
  bool GUIWindowAttach(void *parent) noexcept;

  // Parameter Helpers
  
  void ProcessInputEvents(const clap_input_events *in_events) noexcept;
  void ProcessOutputParams(const clap_output_events *output_parameter_changes) noexcept;
  
  void *mWindow = nullptr;
  bool mGUIOpen = false;
#endif
  
  IPlugQueue<ParamToHost> mParamInfoToHost {PARAM_TRANSFER_SIZE};
};

IPlugCLAP* MakePlug(const InstanceInfo& info);

END_IPLUG_NAMESPACE

#endif
