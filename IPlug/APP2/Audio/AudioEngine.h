/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "IPlugPlatform.h"
#include "AudioDeviceInfo.h"

// Forward declare RTAudio to avoid header dependency
class RtAudio;

BEGIN_IPLUG_NAMESPACE

/** Configuration for audio stream initialization */
struct AudioStreamConfig
{
  EAudioApi api = GetDefaultAudioApi();
  std::string inputDeviceName;
  std::string outputDeviceName;
  uint32_t sampleRate = 44100;
  uint32_t bufferSize = 512;
  std::vector<ChannelMapping> inputMappings;
  std::vector<ChannelMapping> outputMappings;
};

/** Result of audio initialization attempt */
enum class EAudioInitResult
{
  kSuccess,           // Initialized as requested
  kFallbackUsed,      // Initialized with fallback device(s)
  kPartialChannels,   // Fewer channels than requested
  kNoInputDevice,     // No input device available (may be OK for instruments)
  kNoOutputDevice,    // No output device available
  kDriverError,       // Audio API error
  kDeviceNotFound,    // Requested device not found
  kSampleRateNotSupported,
  kBufferSizeNotSupported,
  kFailed             // General failure
};

/** Callback signature for audio processing */
using AudioCallback = std::function<void(
  double** inputs,      // Array of input channel buffers
  double** outputs,     // Array of output channel buffers
  int nInputChannels,
  int nOutputChannels,
  int nFrames,
  double sampleRate,
  double streamTime
)>;

/**
 * AudioEngine - Manages audio I/O via RTAudio
 *
 * Provides a clean interface for audio device enumeration, stream management,
 * and flexible channel routing that respects plugin I/O configurations.
 */
class AudioEngine
{
public:
  AudioEngine();
  ~AudioEngine();

  // Non-copyable
  AudioEngine(const AudioEngine&) = delete;
  AudioEngine& operator=(const AudioEngine&) = delete;

  //--- API Selection ---

  /** Set the audio API to use. Must be called before Initialize. */
  bool SetApi(EAudioApi api);

  /** Get currently selected API */
  EAudioApi GetApi() const { return mApi; }

  //--- Device Enumeration ---

  /** Probe available audio devices for current API */
  void ProbeDevices();

  /** Get list of available input devices */
  const std::vector<AudioDeviceInfo>& GetInputDevices() const { return mInputDevices; }

  /** Get list of available output devices */
  const std::vector<AudioDeviceInfo>& GetOutputDevices() const { return mOutputDevices; }

  /** Find a device by name. Returns nullopt if not found. */
  std::optional<AudioDeviceInfo> FindDevice(const std::string& name) const;

  /** Get the default input device, if any */
  std::optional<AudioDeviceInfo> GetDefaultInputDevice() const;

  /** Get the default output device, if any */
  std::optional<AudioDeviceInfo> GetDefaultOutputDevice() const;

  //--- Stream Management ---

  /** Initialize audio stream with given configuration */
  EAudioInitResult Initialize(const AudioStreamConfig& config, AudioCallback callback);

  /** Start the audio stream */
  bool Start();

  /** Stop the audio stream */
  bool Stop();

  /** Close the audio stream */
  void Close();

  /** Check if stream is open */
  bool IsOpen() const;

  /** Check if stream is running */
  bool IsRunning() const;

  //--- Active Configuration ---

  /** Get the active configuration (may differ from requested) */
  const AudioStreamConfig& GetActiveConfig() const { return mActiveConfig; }

  /** Get actual sample rate (may differ from requested) */
  double GetSampleRate() const { return mSampleRate; }

  /** Get actual buffer size */
  uint32_t GetBufferSize() const { return mBufferSize; }

  //--- Level Metering ---

  /** Get peak level for an input channel (dB, -96 to 0) */
  float GetInputPeakLevel(int channel) const;

  /** Get peak level for an output channel (dB, -96 to 0) */
  float GetOutputPeakLevel(int channel) const;

  /** Reset peak hold levels */
  void ResetPeakLevels();

  //--- Error Handling ---

  /** Get last error message */
  const std::string& GetLastError() const { return mLastError; }

  /** Get detailed result of last initialization */
  EAudioInitResult GetLastInitResult() const { return mLastInitResult; }

private:
  // RTAudio callback (static for C callback interface)
  static int RtAudioCallback(void* outputBuffer, void* inputBuffer,
                             unsigned int nFrames, double streamTime,
                             unsigned int status, void* userData);

  // Error callback
  static void RtAudioErrorCallback(int type, const std::string& errorText);

  // Compute levels from buffer
  void ComputeLevels(double** inputs, double** outputs,
                     int nInputs, int nOutputs, int nFrames);

  // Convert linear to dB
  static float LinearToDb(float linear);

  std::unique_ptr<RtAudio> mRtAudio;
  EAudioApi mApi = GetDefaultAudioApi();
  AudioCallback mCallback;

  std::vector<AudioDeviceInfo> mInputDevices;
  std::vector<AudioDeviceInfo> mOutputDevices;

  AudioStreamConfig mActiveConfig;
  double mSampleRate = 44100.0;
  uint32_t mBufferSize = 512;

  // Level metering (lock-free atomic access)
  static constexpr int kMaxChannels = 32;
  std::atomic<float> mInputPeaks[kMaxChannels];
  std::atomic<float> mOutputPeaks[kMaxChannels];

  // Intermediate buffers for channel remapping
  std::vector<double*> mInputPtrs;
  std::vector<double*> mOutputPtrs;

  std::string mLastError;
  EAudioInitResult mLastInitResult = EAudioInitResult::kSuccess;

  // Fade-in/out to prevent clicks
  uint32_t mFadeCounter = 0;
  bool mFadingIn = false;
  bool mFadingOut = false;
};

END_IPLUG_NAMESPACE
