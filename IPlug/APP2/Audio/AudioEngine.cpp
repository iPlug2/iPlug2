/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#include "AudioEngine.h"
#include "RtAudio.h"

#include <algorithm>
#include <cmath>
#include <cstring>

BEGIN_IPLUG_NAMESPACE

namespace
{
  // Convert EAudioApi to RtAudio::Api
  RtAudio::Api ToRtAudioApi(EAudioApi api)
  {
    switch (api)
    {
#ifdef OS_WIN
      case EAudioApi::kDirectSound: return RtAudio::WINDOWS_DS;
      case EAudioApi::kASIO: return RtAudio::WINDOWS_ASIO;
      case EAudioApi::kWASAPI: return RtAudio::WINDOWS_WASAPI;
#elif defined(OS_MAC)
      case EAudioApi::kCoreAudio: return RtAudio::MACOSX_CORE;
#elif defined(OS_LINUX)
      case EAudioApi::kALSA: return RtAudio::LINUX_ALSA;
      case EAudioApi::kJack: return RtAudio::UNIX_JACK;
      case EAudioApi::kPulseAudio: return RtAudio::LINUX_PULSE;
#endif
      default: return RtAudio::UNSPECIFIED;
    }
  }

  constexpr float kMinDb = -96.f;
  constexpr int kFadeSamples = 512;
}

// IOConfig implementation
std::string IOConfig::GetDisplayName() const
{
  std::string name;
  name += std::to_string(numInputs) + " in / " + std::to_string(numOutputs) + " out";
  if (outputBusSizes.size() > 1)
  {
    name += " (";
    for (size_t i = 0; i < outputBusSizes.size(); ++i)
    {
      if (i > 0) name += "+";
      name += std::to_string(outputBusSizes[i]);
    }
    name += " ch)";
  }
  return name;
}

int IOConfig::GetTotalInputChannels() const
{
  int total = 0;
  for (int size : inputBusSizes) total += size;
  return total;
}

int IOConfig::GetTotalOutputChannels() const
{
  int total = 0;
  for (int size : outputBusSizes) total += size;
  return total;
}

// AudioEngine implementation

AudioEngine::AudioEngine()
{
  // Initialize atomic peak levels
  for (int i = 0; i < kMaxChannels; ++i)
  {
    mInputPeaks[i].store(kMinDb, std::memory_order_relaxed);
    mOutputPeaks[i].store(kMinDb, std::memory_order_relaxed);
  }
}

AudioEngine::~AudioEngine()
{
  Close();
}

bool AudioEngine::SetApi(EAudioApi api)
{
  if (IsOpen())
  {
    mLastError = "Cannot change API while stream is open";
    return false;
  }

  mApi = api;

  try
  {
    mRtAudio = std::make_unique<RtAudio>(ToRtAudioApi(api), &AudioEngine::RtAudioErrorCallback);
  }
  catch (const std::exception& e)
  {
    mLastError = std::string("Failed to initialize audio API: ") + e.what();
    mRtAudio = nullptr;
    return false;
  }

  return true;
}

void AudioEngine::ProbeDevices()
{
  mInputDevices.clear();
  mOutputDevices.clear();

  if (!mRtAudio)
  {
    if (!SetApi(mApi))
      return;
  }

  const auto deviceCount = mRtAudio->getDeviceCount();
  auto defaultIn = mRtAudio->getDefaultInputDevice();
  auto defaultOut = mRtAudio->getDefaultOutputDevice();

  for (unsigned int i = 0; i < deviceCount; ++i)
  {
    try
    {
      auto info = mRtAudio->getDeviceInfo(i);

      AudioDeviceInfo devInfo;
      devInfo.id = i;
      devInfo.name = info.name;
      devInfo.inputChannels = info.inputChannels;
      devInfo.outputChannels = info.outputChannels;
      devInfo.sampleRates = info.sampleRates;
      devInfo.preferredSampleRate = info.preferredSampleRate;
      devInfo.isDefaultInput = (i == defaultIn);
      devInfo.isDefaultOutput = (i == defaultOut);

      if (devInfo.HasInputs())
        mInputDevices.push_back(devInfo);

      if (devInfo.HasOutputs())
        mOutputDevices.push_back(devInfo);
    }
    catch (...)
    {
      // Skip devices that fail to enumerate
    }
  }
}

std::optional<AudioDeviceInfo> AudioEngine::FindDevice(const std::string& name) const
{
  // Search in both input and output lists
  for (const auto& dev : mInputDevices)
  {
    if (dev.name == name)
      return dev;
  }
  for (const auto& dev : mOutputDevices)
  {
    if (dev.name == name)
      return dev;
  }
  return std::nullopt;
}

std::optional<AudioDeviceInfo> AudioEngine::GetDefaultInputDevice() const
{
  for (const auto& dev : mInputDevices)
  {
    if (dev.isDefaultInput)
      return dev;
  }
  // Fallback to first input device
  if (!mInputDevices.empty())
    return mInputDevices[0];
  return std::nullopt;
}

std::optional<AudioDeviceInfo> AudioEngine::GetDefaultOutputDevice() const
{
  for (const auto& dev : mOutputDevices)
  {
    if (dev.isDefaultOutput)
      return dev;
  }
  // Fallback to first output device
  if (!mOutputDevices.empty())
    return mOutputDevices[0];
  return std::nullopt;
}

EAudioInitResult AudioEngine::Initialize(const AudioStreamConfig& config, AudioCallback callback)
{
  Close();
  mCallback = callback;
  mLastError.clear();
  mLastInitResult = EAudioInitResult::kSuccess;

  // Ensure RtAudio is initialized with correct API
  if (!mRtAudio || mApi != config.api)
  {
    if (!SetApi(config.api))
    {
      mLastInitResult = EAudioInitResult::kDriverError;
      return mLastInitResult;
    }
  }

  ProbeDevices();

  // Find input device (optional for instruments)
  std::optional<AudioDeviceInfo> inputDev;
  if (!config.inputDeviceName.empty())
  {
    inputDev = FindDevice(config.inputDeviceName);
    if (!inputDev)
    {
      inputDev = GetDefaultInputDevice();
      if (inputDev)
        mLastInitResult = EAudioInitResult::kFallbackUsed;
    }
  }

  // Find output device (required)
  std::optional<AudioDeviceInfo> outputDev = FindDevice(config.outputDeviceName);
  if (!outputDev)
  {
    outputDev = GetDefaultOutputDevice();
    if (!outputDev)
    {
      mLastError = "No output device available";
      mLastInitResult = EAudioInitResult::kNoOutputDevice;
      return mLastInitResult;
    }
    mLastInitResult = EAudioInitResult::kFallbackUsed;
  }

  // Determine channel counts
  int nInputChannels = 0;
  int nOutputChannels = 0;

  if (!config.inputMappings.empty())
  {
    for (const auto& mapping : config.inputMappings)
    {
      if (mapping.enabled)
        nInputChannels = std::max(nInputChannels, mapping.deviceChannel + 1);
    }
  }
  else if (inputDev)
  {
    // Default: use max available up to device limit
    nInputChannels = inputDev->inputChannels;
  }

  if (!config.outputMappings.empty())
  {
    for (const auto& mapping : config.outputMappings)
    {
      if (mapping.enabled)
        nOutputChannels = std::max(nOutputChannels, mapping.deviceChannel + 1);
    }
  }
  else
  {
    // Default: use max available up to device limit
    nOutputChannels = outputDev->outputChannels;
  }

  // Clamp to device capabilities
  if (inputDev)
    nInputChannels = std::min(nInputChannels, inputDev->inputChannels);
  if (outputDev)
    nOutputChannels = std::min(nOutputChannels, outputDev->outputChannels);

  // Validate sample rate
  uint32_t sampleRate = config.sampleRate;
  bool srSupported = false;
  for (uint32_t sr : outputDev->sampleRates)
  {
    if (sr == sampleRate)
    {
      srSupported = true;
      break;
    }
  }
  if (!srSupported)
  {
    // Use device preferred rate
    sampleRate = outputDev->preferredSampleRate;
    if (sampleRate == 0 && !outputDev->sampleRates.empty())
      sampleRate = outputDev->sampleRates[0];
  }

  // Setup stream parameters
  RtAudio::StreamParameters outParams;
  outParams.deviceId = outputDev->id;
  outParams.nChannels = nOutputChannels;
  outParams.firstChannel = 0;

  RtAudio::StreamParameters* pInParams = nullptr;
  RtAudio::StreamParameters inParams;
  if (inputDev && nInputChannels > 0)
  {
    inParams.deviceId = inputDev->id;
    inParams.nChannels = nInputChannels;
    inParams.firstChannel = 0;
    pInParams = &inParams;
  }

  RtAudio::StreamOptions options;
  options.flags = RTAUDIO_NONINTERLEAVED;
  options.numberOfBuffers = 0; // Use default
  options.streamName = "iPlug2";

  unsigned int bufferFrames = config.bufferSize;

  try
  {
    mRtAudio->openStream(&outParams, pInParams, RTAUDIO_FLOAT64,
                         sampleRate, &bufferFrames,
                         &AudioEngine::RtAudioCallback, this, &options);
  }
  catch (const std::exception& e)
  {
    mLastError = std::string("Failed to open audio stream: ") + e.what();
    mLastInitResult = EAudioInitResult::kFailed;
    return mLastInitResult;
  }

  // Store active configuration
  mActiveConfig = config;
  mActiveConfig.sampleRate = sampleRate;
  mActiveConfig.bufferSize = bufferFrames;
  if (inputDev)
    mActiveConfig.inputDeviceName = inputDev->name;
  if (outputDev)
    mActiveConfig.outputDeviceName = outputDev->name;

  mSampleRate = static_cast<double>(sampleRate);
  mBufferSize = bufferFrames;

  // Prepare buffer pointers
  mInputPtrs.resize(nInputChannels);
  mOutputPtrs.resize(nOutputChannels);

  // Reset levels
  ResetPeakLevels();

  return mLastInitResult;
}

bool AudioEngine::Start()
{
  if (!mRtAudio || !IsOpen())
  {
    mLastError = "Stream not open";
    return false;
  }

  try
  {
    mFadeCounter = kFadeSamples;
    mFadingIn = true;
    mFadingOut = false;
    mRtAudio->startStream();
  }
  catch (const std::exception& e)
  {
    mLastError = std::string("Failed to start stream: ") + e.what();
    return false;
  }

  return true;
}

bool AudioEngine::Stop()
{
  if (!mRtAudio || !IsRunning())
    return true;

  try
  {
    mRtAudio->stopStream();
  }
  catch (const std::exception& e)
  {
    mLastError = std::string("Failed to stop stream: ") + e.what();
    return false;
  }

  return true;
}

void AudioEngine::Close()
{
  if (mRtAudio && IsOpen())
  {
    if (IsRunning())
      Stop();
    mRtAudio->closeStream();
  }
}

bool AudioEngine::IsOpen() const
{
  return mRtAudio && mRtAudio->isStreamOpen();
}

bool AudioEngine::IsRunning() const
{
  return mRtAudio && mRtAudio->isStreamRunning();
}

float AudioEngine::GetInputPeakLevel(int channel) const
{
  if (channel < 0 || channel >= kMaxChannels)
    return kMinDb;
  return mInputPeaks[channel].load(std::memory_order_relaxed);
}

float AudioEngine::GetOutputPeakLevel(int channel) const
{
  if (channel < 0 || channel >= kMaxChannels)
    return kMinDb;
  return mOutputPeaks[channel].load(std::memory_order_relaxed);
}

void AudioEngine::ResetPeakLevels()
{
  for (int i = 0; i < kMaxChannels; ++i)
  {
    mInputPeaks[i].store(kMinDb, std::memory_order_relaxed);
    mOutputPeaks[i].store(kMinDb, std::memory_order_relaxed);
  }
}

// Static callback
int AudioEngine::RtAudioCallback(void* outputBuffer, void* inputBuffer,
                                  unsigned int nFrames, double streamTime,
                                  unsigned int status, void* userData)
{
  auto* engine = static_cast<AudioEngine*>(userData);

  const int nInputs = static_cast<int>(engine->mInputPtrs.size());
  const int nOutputs = static_cast<int>(engine->mOutputPtrs.size());

  // Setup buffer pointers (non-interleaved)
  auto* inBuf = static_cast<double*>(inputBuffer);
  auto* outBuf = static_cast<double*>(outputBuffer);

  for (int ch = 0; ch < nInputs; ++ch)
    engine->mInputPtrs[ch] = inBuf ? inBuf + ch * nFrames : nullptr;

  for (int ch = 0; ch < nOutputs; ++ch)
    engine->mOutputPtrs[ch] = outBuf + ch * nFrames;

  // Clear output buffers
  for (int ch = 0; ch < nOutputs; ++ch)
    std::memset(engine->mOutputPtrs[ch], 0, nFrames * sizeof(double));

  // Call user callback
  if (engine->mCallback)
  {
    engine->mCallback(
      engine->mInputPtrs.data(),
      engine->mOutputPtrs.data(),
      nInputs, nOutputs,
      static_cast<int>(nFrames),
      engine->mSampleRate,
      streamTime
    );
  }

  // Apply fade-in/out
  if (engine->mFadingIn && engine->mFadeCounter > 0)
  {
    for (unsigned int i = 0; i < nFrames; ++i)
    {
      float gain = 1.f - static_cast<float>(engine->mFadeCounter) / kFadeSamples;
      for (int ch = 0; ch < nOutputs; ++ch)
        engine->mOutputPtrs[ch][i] *= gain;

      if (engine->mFadeCounter > 0)
        engine->mFadeCounter--;
    }
    if (engine->mFadeCounter == 0)
      engine->mFadingIn = false;
  }

  // Compute levels for metering
  engine->ComputeLevels(engine->mInputPtrs.data(), engine->mOutputPtrs.data(),
                        nInputs, nOutputs, static_cast<int>(nFrames));

  return 0;
}

void AudioEngine::RtAudioErrorCallback(int type, const std::string& errorText)
{
  // Log error - could be extended to call user callback
  // For now, just store in static for debugging
}

void AudioEngine::ComputeLevels(double** inputs, double** outputs,
                                 int nInputs, int nOutputs, int nFrames)
{
  // Compute peak levels for metering
  for (int ch = 0; ch < nInputs && ch < kMaxChannels; ++ch)
  {
    if (!inputs[ch]) continue;

    float peak = 0.f;
    for (int i = 0; i < nFrames; ++i)
    {
      float sample = static_cast<float>(std::abs(inputs[ch][i]));
      if (sample > peak)
        peak = sample;
    }

    float peakDb = LinearToDb(peak);
    float current = mInputPeaks[ch].load(std::memory_order_relaxed);
    // Peak hold with decay
    if (peakDb > current)
      mInputPeaks[ch].store(peakDb, std::memory_order_relaxed);
    else
      mInputPeaks[ch].store(current - 0.5f, std::memory_order_relaxed); // Decay
  }

  for (int ch = 0; ch < nOutputs && ch < kMaxChannels; ++ch)
  {
    if (!outputs[ch]) continue;

    float peak = 0.f;
    for (int i = 0; i < nFrames; ++i)
    {
      float sample = static_cast<float>(std::abs(outputs[ch][i]));
      if (sample > peak)
        peak = sample;
    }

    float peakDb = LinearToDb(peak);
    float current = mOutputPeaks[ch].load(std::memory_order_relaxed);
    if (peakDb > current)
      mOutputPeaks[ch].store(peakDb, std::memory_order_relaxed);
    else
      mOutputPeaks[ch].store(current - 0.5f, std::memory_order_relaxed);
  }
}

float AudioEngine::LinearToDb(float linear)
{
  if (linear <= 0.f)
    return kMinDb;
  float db = 20.f * std::log10(linear);
  return std::max(db, kMinDb);
}

END_IPLUG_NAMESPACE
