#pragma once
#include "pluginterfaces/base/ibstream.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/vstspeaker.h"
#include <cmath>
#include <type_traits>

class StateStream final : public IBStream
{
public:
  tresult PLUGIN_API queryInterface(const TUID id, void** object) override
  {
    QUERY_INTERFACE(id, object, FUnknown::iid, IBStream)
    QUERY_INTERFACE(id, object, IBStream::iid, IBStream)
    *object = nullptr;
    return kNoInterface;
  }
  uint32 PLUGIN_API addRef() override { return 1; }
  uint32 PLUGIN_API release() override { return 1; }
  tresult PLUGIN_API read(void* buffer, int32 bytes, int32* count) override
  {
    if (bytes < 0)
      return kInvalidArgument;
    const auto size = std::min<size_t>(bytes, data.size() - position);
    if (size)
      memcpy(buffer, data.data() + position, size);
    position += size;
    if (count) *count = static_cast<int32>(size);
    return size || bytes == 0 ? kResultOk : kResultFalse;
  }
  tresult PLUGIN_API write(void* buffer, int32 bytes, int32* count) override
  {
    if (bytes < 0)
      return kInvalidArgument;
    data.resize(std::max(data.size(), position + bytes));
    if (bytes)
      memcpy(data.data() + position, buffer, bytes);
    position += bytes;
    if (count) *count = bytes;
    return kResultOk;
  }
  tresult PLUGIN_API seek(int64 offset, int32 mode, int64* result) override
  {
    const int64 next = offset + (mode == kIBSeekCur ? position : mode == kIBSeekEnd ? data.size() : 0);
    if (next < 0 || static_cast<uint64>(next) > data.size())
      return kInvalidArgument;
    position = next;
    if (result) *result = position;
    return kResultOk;
  }
  tresult PLUGIN_API tell(int64* result) override
  {
    if (!result) return kInvalidArgument;
    *result = position;
    return kResultOk;
  }
  std::vector<char> data;
  size_t position = 0;
};

template <typename Sample>
static void TestAudioBuffers(IAudioProcessor* processor)
{
  Sample input[2][512], output[2][512];
  Sample* inputs[] = {input[0], input[1]};
  Sample* outputs[] = {output[0], output[1]};
  AudioBusBuffers in {}, out {};
  in.numChannels = out.numChannels = 2;
  if constexpr (std::is_same_v<Sample, float>)
  {
    in.channelBuffers32 = inputs;
    out.channelBuffers32 = outputs;
  }
  else
  {
    in.channelBuffers64 = inputs;
    out.channelBuffers64 = outputs;
  }
  ProcessData process {};
  process.processMode = kRealtime;
  process.symbolicSampleSize = std::is_same_v<Sample, float> ? kSample32 : kSample64;
  process.inputs = &in;
  process.outputs = &out;
  process.numInputs = process.numOutputs = 1;
  for (int frames : {1, 64, 127, 512})
  {
    for (int c = 0; c < 2; ++c)
      for (int s = 0; s < 512; ++s)
      {
        input[c][s] = Sample((s - 256) * (c ? -1. : 1.) / 512.);
        output[c][s] = Sample(-999.);
      }
    process.numSamples = frames;
    Check(processor->process(process) == kResultOk, "process VST3 block");
    bool correct = true;
    for (int c = 0; c < 2; ++c)
      for (int s = 0; s < 512; ++s)
        correct &= s < frames ? std::fabs(output[c][s] - input[c][s] * .25) < 1e-6 : output[c][s] == Sample(-999.);
    Check(correct, "VST3 output matches restored gain and respects block boundaries");
  }
}

static void TestAudio(IComponent* component)
{
  FUnknownPtr<IAudioProcessor> processor(component);
  FUnknownPtr<IEditController> controller(component);
  Check(processor && controller, "VST3 provides processor and controller");
  if (!processor || !controller)
    return;
  Check(controller->setParamNormalized(0, .25) == kResultOk, "set VST3 gain");
  StateStream saved;
  Check(component->getState(&saved) == kResultOk && !saved.data.empty(), "save VST3 component state");
  Check(controller->setParamNormalized(0, .75) == kResultOk &&
        std::fabs(controller->getParamNormalized(0) - .75) < 1e-8, "change VST3 parameter after save");
  saved.position = 0;
  Check(component->setState(&saved) == kResultOk, "restore VST3 component state");
  Check(std::fabs(controller->getParamNormalized(0) - .25) < 1e-8, "VST3 restores controller parameter");
  SpeakerArrangement stereo = SpeakerArr::kStereo;
  Check(processor->setBusArrangements(&stereo, 1, &stereo, 1) == kResultOk, "configure stereo VST3 buses");
  Check(component->activateBus(kAudio, kInput, 0, true) == kResultOk &&
        component->activateBus(kAudio, kOutput, 0, true) == kResultOk, "activate VST3 audio buses");
  for (int sampleSize : {kSample32, kSample64})
    for (double rate : {44100., 48000.})
    {
      ProcessSetup setup {kRealtime, sampleSize, 512, rate};
      Check(processor->setupProcessing(setup) == kResultOk, "configure VST3 sample format and rate");
      Check(component->setActive(true) == kResultOk, "activate VST3");
      std::thread audio([&]() {
        Check(processor->setProcessing(true) == kResultOk, "start VST3 processing");
        if (sampleSize == kSample32)
          TestAudioBuffers<float>(processor);
        else
          TestAudioBuffers<double>(processor);
        Check(processor->setProcessing(false) == kResultOk, "stop VST3 processing");
      });
      audio.join();
      Check(component->setActive(false) == kResultOk, "deactivate VST3");
    }
}
