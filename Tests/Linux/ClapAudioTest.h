#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

// Exercise the public API of IPlugEffect; no framework implementation is linked
// into this host. Processing runs on a worker distinct from the host main thread.
static thread_local bool sAudioThread = false;

static void TestAudio(const clap_plugin_t* plugin)
{
  const auto* params = static_cast<const clap_plugin_params_t*>(plugin->get_extension(plugin, CLAP_EXT_PARAMS));
  const auto* state = static_cast<const clap_plugin_state_t*>(plugin->get_extension(plugin, CLAP_EXT_STATE));
  Check(params && state, "CLAP provides parameters and state");
  if (!params || !state)
    return;
  clap_event_param_value_t value {};
  value.header.size = sizeof(value);
  value.header.type = CLAP_EVENT_PARAM_VALUE;
  value.param_id = 0;
  value.note_id = value.port_index = value.channel = value.key = -1;
  clap_input_events_t inputEvents {
    &value,
    [](const clap_input_events_t*) -> uint32_t { return 1; },
    [](const clap_input_events_t* events, uint32_t) -> const clap_event_header_t* {
      return &static_cast<clap_event_param_value_t*>(events->ctx)->header;
    }
  };
  clap_output_events_t outputEvents {nullptr,
    [](const clap_output_events_t*, const clap_event_header_t*) { return true; }};
  auto setGain = [&](double gain) {
    value.value = gain;
    params->flush(plugin, &inputEvents, &outputEvents);
  };
  setGain(.25);
  std::vector<char> saved;
  clap_ostream_t output {&saved, [](const clap_ostream_t* stream, const void* data, uint64_t bytes) -> int64_t {
    auto& saved = *static_cast<std::vector<char>*>(stream->ctx);
    const auto count = std::min<uint64_t>(bytes, 7); // partial writes are valid
    const auto* text = static_cast<const char*>(data);
    saved.insert(saved.end(), text, text + count);
    return count;
  }};
  Check(state->save(plugin, &output) && !saved.empty(), "save CLAP state with partial stream writes");
  setGain(.75);
  double gain = 0.;
  Check(params->get_value(plugin, 0, &gain) && std::fabs(gain - .75) < 1e-8, "CLAP parameter change applied");
  struct Reader { const std::vector<char>& data; size_t position = 0; } reader {saved};
  clap_istream_t input {&reader, [](const clap_istream_t* stream, void* data, uint64_t bytes) -> int64_t {
    auto& reader = *static_cast<Reader*>(stream->ctx);
    const auto count = std::min<uint64_t>({bytes, 7, reader.data.size() - reader.position});
    if (count)
      memcpy(data, reader.data.data() + reader.position, count);
    reader.position += count;
    return count;
  }};
  Check(state->load(plugin, &input), "restore CLAP state with partial stream reads");
  Check(params->get_value(plugin, 0, &gain) && std::fabs(gain - .25) < 1e-8, "CLAP restores parameter value");

  for (double rate : {44100., 48000.})
  {
    if (!plugin->activate(plugin, rate, 1, 512))
    {
      Check(false, "activate CLAP audio");
      return;
    }
    std::thread audio([&]() {
      sAudioThread = true;
      const bool started = plugin->start_processing(plugin);
      Check(started, "start CLAP processing");
      if (!started)
        return;
      float input[2][512], output[2][512];
      float* inputs[] = {input[0], input[1]};
      float* outputs[] = {output[0], output[1]};
      clap_audio_buffer_t in {}, out {};
      in.data32 = inputs;
      out.data32 = outputs;
      in.channel_count = out.channel_count = 2;
      clap_input_events_t empty {nullptr,
        [](const clap_input_events_t*) -> uint32_t { return 0; },
        [](const clap_input_events_t*, uint32_t) -> const clap_event_header_t* { return nullptr; }};
      clap_process_t process {};
      process.audio_inputs = &in;
      process.audio_outputs = &out;
      process.audio_inputs_count = process.audio_outputs_count = 1;
      process.in_events = &empty;
      process.out_events = &outputEvents;
      process.steady_time = 0;
      for (unsigned frames : {1u, 64u, 127u, 512u})
      {
        for (unsigned c = 0; c < 2; ++c)
          for (unsigned s = 0; s < 512; ++s)
          {
            input[c][s] = float((int(s) - 256) * (c ? -1. : 1.) / 512.);
            output[c][s] = -999.f;
          }
        process.frames_count = frames;
        Check(plugin->process(plugin, &process) != CLAP_PROCESS_ERROR, "process CLAP block");
        bool correct = true;
        for (unsigned c = 0; c < 2; ++c)
          for (unsigned s = 0; s < 512; ++s)
            correct &= s < frames ? std::fabs(output[c][s] - input[c][s] * .25f) < 1e-6f : output[c][s] == -999.f;
        Check(correct, "CLAP output matches restored gain and respects block boundaries");
        process.steady_time += frames;
      }
      plugin->stop_processing(plugin);
    });
    audio.join();
    plugin->deactivate(plugin);
  }
}
