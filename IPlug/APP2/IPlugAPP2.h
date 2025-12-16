/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#pragma once

#include "IPlugAPIBase.h"
#include "IPlugProcessor.h"

#include "Audio/AudioDeviceInfo.h"

BEGIN_IPLUG_NAMESPACE

class IPlugAPP2Host;

/**
 * IPlugAPP2 - Plugin wrapper for standalone application
 *
 * Provides the IPlug API interface for a plugin running as a standalone app.
 * Handles parameter updates, editor attachment, and audio processing.
 */
class IPlugAPP2 : public IPlugAPIBase, public IPlugProcessor
{
public:
  IPlugAPP2(const InstanceInfo& info, const Config& config);
  virtual ~IPlugAPP2();

  // IPlugAPIBase overrides
  void BeginInformHostOfParamChange(int idx) override {}
  void InformHostOfParamChange(int idx, double normalizedValue) override {}
  void EndInformHostOfParamChange(int idx) override {}
  void InformHostOfPresetChange() override {}
  bool EditorResize(int viewWidth, int viewHeight) override;

  // IPlugProcessor overrides
  void SetLatency(int samples) override {}

  //--- APP2-specific methods ---

  /** Process audio block with channel mappings */
  void AppProcess(double** inputs, double** outputs,
                  int nInputChannels, int nOutputChannels,
                  int nFrames, double sampleRate);

  /** Get the host instance */
  IPlugAPP2Host* GetHost() { return mHost; }

  /** Set the host instance (called by host during creation) */
  void SetHost(IPlugAPP2Host* pHost) { mHost = pHost; }

  /** Get the current I/O configuration in use */
  const IOConfig& GetCurrentIOConfig() const;

  /** Called when window is attached */
  void OnWindowAttached(void* pParent);

  /** Called when window is closing */
  void OnWindowDetached();

  /** Called when the app is about to quit */
  bool OnHostRequestingQuit();

  /** Called when Help menu item is selected */
  bool OnHostRequestingHelp();

  /** Called when About menu item is selected */
  bool OnHostRequestingAboutBox();

private:
  IPlugAPP2Host* mHost = nullptr;
};

/** Factory function - implemented by plugin */
IPlugAPP2* MakePlugAPP2(const InstanceInfo& info);

END_IPLUG_NAMESPACE
