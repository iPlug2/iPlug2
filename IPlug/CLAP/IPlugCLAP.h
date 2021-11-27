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

/** CLAP API base class for an IPlug plug-in
*   @ingroup APIClasses */
class IPlugCLAP : public IPlugAPIBase
                , public IPlugProcessor
                , public clap::Plugin
{
public:
  IPlugCLAP(const InstanceInfo& info, const Config& config);

  //IPlugAPIBase
//  void BeginInformHostOfParamChange(int idx) override;
//  void InformHostOfParamChange(int idx, double normalizedValue) override;
//  void EndInformHostOfParamChange(int idx) override;
//  void InformHostOfPresetChange() override;
//  void HostSpecificInit() override;
//  bool EditorResize(int viewWidth, int viewHeight) override;

  //IPlugProcessor
//  void SetLatency(int samples) override;
  bool SendMidiMsg(const IMidiMsg& msg) override;
//  bool SendSysEx(const ISysEx& msg) override;

private:
  
  // clap_plugin
  bool init() noexcept override;
  bool activate(double sampleRate) noexcept override;
  void deactivate() noexcept override;
  //bool startProcessing() noexcept override { return true; }
  //void stopProcessing() noexcept override {}
  clap_process_status process(const clap_process *process) noexcept override;
  
  //void onMainThread() noexcept override {}
  //const void *extension(const char *id) noexcept override { return nullptr; }

  // clap_plugin_latency
  bool implementsLatency() const noexcept override { return true; }
  uint32_t latencyGet() const noexcept override { return GetLatency(); }
  
  // clap_plugin_render
  bool implementsRender() const noexcept override { return true; }
  void renderSetMode(clap_plugin_render_mode mode) noexcept override;
  
  // clap_plugin_state
  bool implementsState() const noexcept override { return true; }
  bool stateSave(clap_ostream *stream) noexcept override;
  bool stateLoad(clap_istream *stream) noexcept override;
  
  /*void stateMarkDirty() const noexcept {
     if (canUseState())
        _hostState->mark_dirty(_host);
  }*/
  
  // clap_plugin_params
  bool implementsParams() const noexcept override { return true; }
  uint32_t paramsCount() const noexcept override { return NParams(); }
  
  bool paramsInfo(int32_t paramIndex, clap_param_info *info) const noexcept override;
  
  bool paramsValue(clap_id paramId, double *value) noexcept override;
  bool paramsValueToText(clap_id paramId, double value, char *display, uint32_t size) noexcept override;
  bool paramsTextToValue(clap_id paramId, const char *display, double *value) noexcept override;
     
  void paramsFlush(const clap_event_list *input_parameter_changes, const clap_event_list *output_parameter_changes) noexcept override;
  bool isValidParamId(clap_id paramId) const noexcept override { return paramId < NParams(); }
    
  // clap_plugin_gui
#if PLUG_HAS_UI
  bool implementsGui() const noexcept override { return true; }
  bool guiCreate() noexcept override { return true; }
  void guiDestroy() noexcept override { CloseWindow(); }
  
  void guiSetScale(double scale) noexcept override { SetScreenScale(scale); }
  void guiShow() noexcept override { OpenWindow(mWindow); }
  void guiHide() noexcept override { CloseWindow(); }
  bool guiSize(uint32_t *width, uint32_t *height) noexcept override;
  
#if PLUG_HOST_RESIZE
  bool guiCanResize() const noexcept override { return true; }
  bool guiSetSize(uint32_t width, uint32_t height) noexcept override;
  void guiRoundSize(uint32_t *width, uint32_t *height) noexcept override;
#endif
  
#ifdef OS_WIN
  // clap_plugin_gui_win32
  bool implementsGuiWin32() const noexcept override { return false; }
  bool guiWin32Attach(clap_hwnd window) noexcept override { return GUIWindowAttach(window);  }
#endif
  
#ifdef OS_MAC
  // clap_plugin_gui_cocoa
  bool implementsGuiCocoa() const noexcept override { return true; }
  bool guiCocoaAttach(void *nsView) noexcept override { return GUIWindowAttach(nsView); }
#endif
  
  // Helper to attach GUI Windows
  
  bool GUIWindowAttach(void *parent) noexcept;

  void *mWindow = nullptr;
#endif
};

IPlugCLAP* MakePlug(const InstanceInfo& info);

END_IPLUG_NAMESPACE

#endif
