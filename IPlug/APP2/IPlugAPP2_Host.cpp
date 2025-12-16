/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#include "IPlugAPP2_Host.h"
#include "IPlugAPP2.h"

#include "RtMidi.h"

#include <algorithm>
#include <sstream>

#include "config.h"

#ifdef OS_WIN
  #include <windows.h>
#else
  #include "IPlugSWELL.h"
#endif

// Global window handles
HWND gHWND = nullptr;
HINSTANCE gHINSTANCE = nullptr;

BEGIN_IPLUG_NAMESPACE

std::unique_ptr<IPlugAPP2Host> IPlugAPP2Host::sInstance = nullptr;

IPlugAPP2Host* IPlugAPP2Host::Create()
{
  sInstance = std::make_unique<IPlugAPP2Host>();
  return sInstance.get();
}

IPlugAPP2Host::IPlugAPP2Host()
{
  // Parse channel I/O configurations from config.h
  mIOConfigs = ParseChannelIOString(PLUG_CHANNEL_IO);

  // Create audio engine
  mAudioEngine = std::make_unique<AudioEngine>();

  // Get settings file path
  mSettingsPath = AppStateSerializer::GetSettingsPath(BUNDLE_NAME);
}

IPlugAPP2Host::~IPlugAPP2Host()
{
  StopAudio();

  mMidiIn.reset();
  mMidiOut.reset();
  mAudioEngine.reset();
  mPlug.reset();
}

bool IPlugAPP2Host::Init()
{
  // Create plugin instance
  InstanceInfo info;
  info.mOSWindowHandle = nullptr;
  info.mOSContextHandle = nullptr;

  mPlug.reset(MakePlugAPP2(info));
  if (!mPlug)
  {
    return false;
  }
  mPlug->SetHost(this);

  // Load saved state
  LoadState();

  // Probe devices with current API
  ProbeDevices();

  // Initialize audio
  if (!InitializeAudio())
  {
    // Audio init failed - continue anyway, user can fix in settings
  }

  // Initialize MIDI
  InitializeMidi();

  // Start audio
  StartAudio();

  // Notify plugin of initial state
  mPlug->OnParamReset(kReset);
  mPlug->OnActivate(true);

  return true;
}

bool IPlugAPP2Host::LoadState()
{
  if (mSettingsPath.empty())
    return false;

  if (!AppStateSerializer::Load(mState, mSettingsPath))
  {
    // Use defaults if load fails
    mState.Reset();

    // Set defaults based on probed devices
    if (auto defIn = mAudioEngine->GetDefaultInputDevice())
      mState.inputDeviceName = defIn->name;
    if (auto defOut = mAudioEngine->GetDefaultOutputDevice())
      mState.outputDeviceName = defOut->name;
  }

  return true;
}

bool IPlugAPP2Host::SaveState()
{
  if (mSettingsPath.empty())
    return false;

  return AppStateSerializer::Save(mState, mSettingsPath);
}

bool IPlugAPP2Host::OpenWindow(void* pParent)
{
  if (mPlug && mPlug->HasUI())
  {
    mPlug->OnWindowAttached(pParent);
    return true;
  }
  return false;
}

void IPlugAPP2Host::CloseWindow()
{
  if (mPlug)
  {
    mPlug->OnWindowDetached();
  }
}

bool IPlugAPP2Host::ShowSettingsDialog(void* pParent)
{
  // Create dialog if needed
  if (!mSettingsDialog)
  {
    mSettingsDialog = CreateSettingsDialog();
    if (!mSettingsDialog)
      return false;

    // Set up callbacks
    mSettingsDialog->OnApply = [this](const AppState& newState) {
      return ApplyAudioState(newState);
    };

    mSettingsDialog->OnDriverChange = [this](EAudioApi api) {
      mAudioEngine->SetApi(api);
      ProbeDevices();
      // Update dialog with new device lists
      mSettingsDialog->SetInputDevices(mAudioEngine->GetInputDevices());
      mSettingsDialog->SetOutputDevices(mAudioEngine->GetOutputDevices());
    };
  }

  // Update dialog with current state and device lists
  mSettingsDialog->SetState(mState);
  mSettingsDialog->SetInputDevices(mAudioEngine->GetInputDevices());
  mSettingsDialog->SetOutputDevices(mAudioEngine->GetOutputDevices());
  mSettingsDialog->SetMidiInputDevices(mMidiInputDevices);
  mSettingsDialog->SetMidiOutputDevices(mMidiOutputDevices);
  mSettingsDialog->SetIOConfigs(mIOConfigs);
  mSettingsDialog->SetAudioEngine(mAudioEngine.get());

  // Show modal dialog
  if (mSettingsDialog->ShowModal(pParent))
  {
    // User clicked OK - save state
    mState = mSettingsDialog->GetState();
    SaveState();
    return true;
  }

  return false;
}

bool IPlugAPP2Host::StartAudio()
{
  if (!mAudioEngine)
    return false;

  if (mAudioEngine->IsRunning())
    return true;

  if (!mAudioEngine->IsOpen())
  {
    if (!InitializeAudio())
      return false;
  }

  return mAudioEngine->Start();
}

void IPlugAPP2Host::StopAudio()
{
  if (mAudioEngine && mAudioEngine->IsRunning())
  {
    mAudioEngine->Stop();
  }
}

bool IPlugAPP2Host::IsAudioRunning() const
{
  return mAudioEngine && mAudioEngine->IsRunning();
}

const IOConfig& IPlugAPP2Host::GetCurrentIOConfig() const
{
  if (mState.selectedIOConfig >= 0 &&
      mState.selectedIOConfig < static_cast<int>(mIOConfigs.size()))
  {
    return mIOConfigs[mState.selectedIOConfig];
  }

  // Return first config or empty
  static IOConfig emptyConfig;
  return mIOConfigs.empty() ? emptyConfig : mIOConfigs[0];
}

std::vector<IOConfig> IPlugAPP2Host::ParseChannelIOString(const char* ioStr)
{
  std::vector<IOConfig> configs;

  if (!ioStr || !*ioStr)
    return configs;

  std::istringstream stream(ioStr);
  std::string configStr;

  while (stream >> configStr)
  {
    IOConfig config;

    // Parse format: "IN-OUT" or "IN-OUT.OUT2.OUT3" etc.
    size_t dashPos = configStr.find('-');
    if (dashPos == std::string::npos)
      continue;

    // Parse input buses
    std::string inputPart = configStr.substr(0, dashPos);
    config.numInputs = std::stoi(inputPart);
    if (config.numInputs > 0)
      config.inputBusSizes.push_back(config.numInputs);

    // Parse output buses (may be multiple separated by '.')
    std::string outputPart = configStr.substr(dashPos + 1);
    std::istringstream outStream(outputPart);
    std::string busStr;

    while (std::getline(outStream, busStr, '.'))
    {
      if (!busStr.empty())
      {
        int busSize = std::stoi(busStr);
        config.outputBusSizes.push_back(busSize);
        config.numOutputs += busSize;
      }
    }

    configs.push_back(config);
  }

  return configs;
}

bool IPlugAPP2Host::InitializeAudio()
{
  if (!mAudioEngine || !mPlug)
    return false;

  // Build audio configuration from state
  AudioStreamConfig config;
  config.api = mState.audioApi;
  config.inputDeviceName = mState.inputDeviceName;
  config.outputDeviceName = mState.outputDeviceName;
  config.sampleRate = mState.sampleRate;
  config.bufferSize = mState.bufferSize;

  // Get channel counts from current I/O config
  const IOConfig& ioConfig = GetCurrentIOConfig();

  // Create default mappings if not set
  if (mState.inputMappings.empty() && ioConfig.numInputs > 0)
  {
    mState.CreateDefaultInputMappings(ioConfig.numInputs);
  }
  if (mState.outputMappings.empty() && ioConfig.numOutputs > 0)
  {
    mState.CreateDefaultOutputMappings(ioConfig.numOutputs);
  }

  config.inputMappings = mState.inputMappings;
  config.outputMappings = mState.outputMappings;

  // Set up callback
  auto callback = [this](double** inputs, double** outputs,
                         int nInputs, int nOutputs,
                         int nFrames, double sampleRate, double streamTime) {
    this->AudioCallback(inputs, outputs, nInputs, nOutputs, nFrames, sampleRate, streamTime);
  };

  // Initialize
  auto result = mAudioEngine->Initialize(config, callback);

  mAudioInitialized = (result == EAudioInitResult::kSuccess ||
                       result == EAudioInitResult::kFallbackUsed ||
                       result == EAudioInitResult::kPartialChannels ||
                       result == EAudioInitResult::kNoInputDevice);

  if (mAudioInitialized)
  {
    mActiveState = mState;

    // Update plugin sample rate
    double sr = mAudioEngine->GetSampleRate();
    mPlug->SetSampleRate(sr);
    mPlug->SetBlockSize(mAudioEngine->GetBufferSize());
  }

  return mAudioInitialized;
}

bool IPlugAPP2Host::InitializeMidi()
{
  mMidiInputDevices.clear();
  mMidiOutputDevices.clear();

  // Create MIDI input
  try
  {
    mMidiIn = std::make_unique<RtMidiIn>();
    mMidiIn->setCallback(&IPlugAPP2Host::MidiCallback, this);
    mMidiIn->ignoreTypes(false, false, false);

    // Enumerate input ports
    mMidiInputDevices.push_back("off");
    unsigned int nPorts = mMidiIn->getPortCount();
    for (unsigned int i = 0; i < nPorts; ++i)
    {
      mMidiInputDevices.push_back(mMidiIn->getPortName(i));
    }

#ifdef OS_MAC
    // Add virtual port option for macOS
    mMidiInputDevices.push_back(BUNDLE_NAME " Virtual Input");
#endif
  }
  catch (const std::exception&)
  {
    mMidiIn.reset();
  }

  // Create MIDI output
  try
  {
    mMidiOut = std::make_unique<RtMidiOut>();

    // Enumerate output ports
    mMidiOutputDevices.push_back("off");
    unsigned int nPorts = mMidiOut->getPortCount();
    for (unsigned int i = 0; i < nPorts; ++i)
    {
      mMidiOutputDevices.push_back(mMidiOut->getPortName(i));
    }

#ifdef OS_MAC
    // Add virtual port option for macOS
    mMidiOutputDevices.push_back(BUNDLE_NAME " Virtual Output");
#endif
  }
  catch (const std::exception&)
  {
    mMidiOut.reset();
  }

  return ApplyMidiState(mState);
}

void IPlugAPP2Host::ProbeDevices()
{
  if (mAudioEngine)
  {
    mAudioEngine->ProbeDevices();
  }
}

void IPlugAPP2Host::AudioCallback(double** inputs, double** outputs,
                                   int nInputs, int nOutputs,
                                   int nFrames, double sampleRate, double streamTime)
{
  if (mExiting || !mPlug)
    return;

  // Process plugin
  mPlug->AppProcess(inputs, outputs, nInputs, nOutputs, nFrames, sampleRate);
}

void IPlugAPP2Host::MidiCallback(double deltaTime, std::vector<uint8_t>* pMsg, void* pUserData)
{
  auto* host = static_cast<IPlugAPP2Host*>(pUserData);
  if (!host || !host->mPlug || pMsg->size() < 1)
    return;

  // Parse MIDI message
  uint8_t status = (*pMsg)[0];
  uint8_t data1 = pMsg->size() > 1 ? (*pMsg)[1] : 0;
  uint8_t data2 = pMsg->size() > 2 ? (*pMsg)[2] : 0;

  // TODO: Channel filtering based on mState.midiInputChannel
  // TODO: Queue message for processing on audio thread

  IMidiMsg msg;
  msg.mStatus = status;
  msg.mData1 = data1;
  msg.mData2 = data2;
  msg.mOffset = 0;

  // Direct call for now - should be queued for thread safety
  // host->mPlug->ProcessMidiMsg(msg);
}

bool IPlugAPP2Host::ApplyAudioState(const AppState& newState)
{
  bool needsRestart = mState.AudioSettingsDiffer(newState);

  if (needsRestart)
  {
    // Stop current audio
    StopAudio();

    // Update state
    mState = newState;

    // Reinitialize with new settings
    if (!InitializeAudio())
    {
      // Restore old state if init fails
      mState = mActiveState;
      InitializeAudio();
      return false;
    }

    // Restart
    StartAudio();
  }
  else
  {
    mState = newState;
  }

  // Apply MIDI changes
  if (mState.MidiSettingsDiffer(mActiveState))
  {
    ApplyMidiState(mState);
  }

  mActiveState = mState;
  return true;
}

bool IPlugAPP2Host::ApplyMidiState(const AppState& newState)
{
  // Close current MIDI connections
  if (mMidiIn && mMidiIn->isPortOpen())
    mMidiIn->closePort();
  if (mMidiOut && mMidiOut->isPortOpen())
    mMidiOut->closePort();

  // Open MIDI input
  if (mMidiIn && newState.midiInputDevice != "off")
  {
    try
    {
#ifdef OS_MAC
      if (newState.midiInputDevice == BUNDLE_NAME " Virtual Input")
      {
        mMidiIn->openVirtualPort(BUNDLE_NAME);
      }
      else
#endif
      {
        // Find port by name
        unsigned int nPorts = mMidiIn->getPortCount();
        for (unsigned int i = 0; i < nPorts; ++i)
        {
          if (mMidiIn->getPortName(i) == newState.midiInputDevice)
          {
            mMidiIn->openPort(i);
            break;
          }
        }
      }
    }
    catch (const std::exception&)
    {
      // MIDI input failed - continue
    }
  }

  // Open MIDI output
  if (mMidiOut && newState.midiOutputDevice != "off")
  {
    try
    {
#ifdef OS_MAC
      if (newState.midiOutputDevice == BUNDLE_NAME " Virtual Output")
      {
        mMidiOut->openVirtualPort(BUNDLE_NAME);
      }
      else
#endif
      {
        unsigned int nPorts = mMidiOut->getPortCount();
        for (unsigned int i = 0; i < nPorts; ++i)
        {
          if (mMidiOut->getPortName(i) == newState.midiOutputDevice)
          {
            mMidiOut->openPort(i);
            break;
          }
        }
      }
    }
    catch (const std::exception&)
    {
      // MIDI output failed - continue
    }
  }

  return true;
}

END_IPLUG_NAMESPACE
