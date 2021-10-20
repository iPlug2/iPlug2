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
#include "clap-plugin.hh"

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
  
private:
};

IPlugCLAP* MakePlug(const InstanceInfo& info);

END_IPLUG_NAMESPACE

#endif
