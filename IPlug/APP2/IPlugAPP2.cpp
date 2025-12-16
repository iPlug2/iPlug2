/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#include "IPlugAPP2.h"
#include "IPlugAPP2_Host.h"

BEGIN_IPLUG_NAMESPACE

IPlugAPP2::IPlugAPP2(const InstanceInfo& info, const Config& config)
  : IPlugAPIBase(config, kAPIAPP)
  , IPlugProcessor(config, kAPIAPP)
{
  Trace(TRACELOC, "%s", config.pluginName);

  int nInputs = MaxNChannels(ERoute::kInput);
  int nOutputs = MaxNChannels(ERoute::kOutput);

  SetChannelConnections(ERoute::kInput, 0, nInputs, true);
  SetChannelConnections(ERoute::kOutput, 0, nOutputs, true);

  mSampleRate = DEFAULT_SAMPLE_RATE;
  mBlockSize = 512;

  CreateTimer();
}

IPlugAPP2::~IPlugAPP2()
{
  Trace(TRACELOC, "");
}

bool IPlugAPP2::EditorResize(int viewWidth, int viewHeight)
{
  if (HasUI())
  {
    // TODO: Notify host to resize window
    return true;
  }
  return false;
}

void IPlugAPP2::AppProcess(double** inputs, double** outputs,
                            int nInputChannels, int nOutputChannels,
                            int nFrames, double sampleRate)
{
  // Update sample rate if changed
  if (sampleRate != mSampleRate)
  {
    mSampleRate = sampleRate;
    OnReset();
  }

  // Set up channel connections based on actual available channels
  SetChannelConnections(ERoute::kInput, 0, nInputChannels, true);
  SetChannelConnections(ERoute::kOutput, 0, nOutputChannels, true);

  // Disable any channels beyond what we have
  int maxIn = MaxNChannels(ERoute::kInput);
  int maxOut = MaxNChannels(ERoute::kOutput);

  if (nInputChannels < maxIn)
    SetChannelConnections(ERoute::kInput, nInputChannels, maxIn - nInputChannels, false);
  if (nOutputChannels < maxOut)
    SetChannelConnections(ERoute::kOutput, nOutputChannels, maxOut - nOutputChannels, false);

  // Attach buffers
  AttachBuffers(ERoute::kInput, 0, nInputChannels, inputs, nFrames);
  AttachBuffers(ERoute::kOutput, 0, nOutputChannels, outputs, nFrames);

  // Clear unconnected output buffers
  for (int ch = nOutputChannels; ch < maxOut; ++ch)
  {
    // These channels aren't connected, nothing to clear
  }

  // Process MIDI (host handles queuing from MIDI thread)
  // TODO: ProcessQueuedMessages()

  // Process audio
  ProcessBuffers(0.f, nFrames);
}

const IOConfig& IPlugAPP2::GetCurrentIOConfig() const
{
  if (mHost)
    return mHost->GetCurrentIOConfig();

  // Fallback - return static empty config
  static IOConfig emptyConfig;
  return emptyConfig;
}

void IPlugAPP2::OnWindowAttached(void* pParent)
{
  if (HasUI())
  {
    OpenWindow(pParent);
  }
}

void IPlugAPP2::OnWindowDetached()
{
  if (HasUI())
  {
    CloseWindow();
  }
}

bool IPlugAPP2::OnHostRequestingQuit()
{
  // Plugins can override to perform cleanup or prompt for unsaved changes
  return true; // Allow quit
}

bool IPlugAPP2::OnHostRequestingHelp()
{
  // Return true if plugin handles help, false for default behavior
  return false;
}

bool IPlugAPP2::OnHostRequestingAboutBox()
{
  // Return true if plugin handles about box, false for default behavior
  return false;
}

END_IPLUG_NAMESPACE
