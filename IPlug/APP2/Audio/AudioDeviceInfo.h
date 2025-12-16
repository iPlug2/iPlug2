/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "IPlugPlatform.h"

BEGIN_IPLUG_NAMESPACE

/** Identifies an audio API/driver type */
enum class EAudioApi
{
  kUnknown = 0,
#ifdef OS_WIN
  kDirectSound,
  kASIO,
  kWASAPI,
#elif defined(OS_MAC)
  kCoreAudio,
#elif defined(OS_LINUX)
  kALSA,
  kJack,
  kPulseAudio,
#endif
};

/** Returns the display name for an audio API */
inline const char* GetAudioApiName(EAudioApi api)
{
  switch (api)
  {
#ifdef OS_WIN
    case EAudioApi::kDirectSound: return "DirectSound";
    case EAudioApi::kASIO: return "ASIO";
    case EAudioApi::kWASAPI: return "WASAPI";
#elif defined(OS_MAC)
    case EAudioApi::kCoreAudio: return "CoreAudio";
#elif defined(OS_LINUX)
    case EAudioApi::kALSA: return "ALSA";
    case EAudioApi::kJack: return "Jack";
    case EAudioApi::kPulseAudio: return "PulseAudio";
#endif
    default: return "Unknown";
  }
}

/** Returns available audio APIs for the current platform */
inline std::vector<EAudioApi> GetAvailableAudioApis()
{
  std::vector<EAudioApi> apis;
#ifdef OS_WIN
  apis.push_back(EAudioApi::kDirectSound);
  apis.push_back(EAudioApi::kASIO);
  // apis.push_back(EAudioApi::kWASAPI); // Future
#elif defined(OS_MAC)
  apis.push_back(EAudioApi::kCoreAudio);
#elif defined(OS_LINUX)
  apis.push_back(EAudioApi::kALSA);
  apis.push_back(EAudioApi::kJack);
#endif
  return apis;
}

/** Returns the default audio API for the current platform */
inline EAudioApi GetDefaultAudioApi()
{
#ifdef OS_WIN
  return EAudioApi::kDirectSound;
#elif defined(OS_MAC)
  return EAudioApi::kCoreAudio;
#elif defined(OS_LINUX)
  return EAudioApi::kALSA;
#else
  return EAudioApi::kUnknown;
#endif
}

/** Information about an audio device */
struct AudioDeviceInfo
{
  uint32_t id = 0;                       // RTAudio device ID
  std::string name;                       // Display name
  int inputChannels = 0;                  // Number of input channels
  int outputChannels = 0;                 // Number of output channels
  std::vector<uint32_t> sampleRates;      // Supported sample rates
  uint32_t preferredSampleRate = 44100;   // Device's preferred sample rate
  bool isDefaultInput = false;            // Is system default input
  bool isDefaultOutput = false;           // Is system default output

  bool HasInputs() const { return inputChannels > 0; }
  bool HasOutputs() const { return outputChannels > 0; }
  bool IsDuplex() const { return HasInputs() && HasOutputs(); }
};

/** Maps a plugin channel to a device channel */
struct ChannelMapping
{
  int pluginChannel = 0;     // Channel index in plugin (0-based)
  int deviceChannel = 0;     // Channel index on device (0-based)
  int busIndex = 0;          // For multi-bus plugins, which bus
  bool enabled = true;       // Can disable unused channels

  ChannelMapping() = default;
  ChannelMapping(int plug, int dev, int bus = 0)
    : pluginChannel(plug), deviceChannel(dev), busIndex(bus), enabled(true) {}
};

/** Represents an I/O configuration from PLUG_CHANNEL_IO */
struct IOConfig
{
  int numInputs = 0;
  int numOutputs = 0;
  std::vector<int> inputBusSizes;   // e.g., {2} for stereo, {2, 2} for two stereo buses
  std::vector<int> outputBusSizes;  // e.g., {2, 2, 2, 2} for quad stereo output

  std::string GetDisplayName() const;
  int GetTotalInputChannels() const;
  int GetTotalOutputChannels() const;
};

END_IPLUG_NAMESPACE
