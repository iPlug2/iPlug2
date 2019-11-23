#include "rtaudio_c.h"
#include "RtAudio.h"

#include <cstring>

#define MAX_ERROR_MESSAGE_LENGTH 512

struct rtaudio {
  RtAudio *audio;

  rtaudio_cb_t cb;
  void *userdata;

  int has_error;
  char errmsg[MAX_ERROR_MESSAGE_LENGTH];
};

const char *rtaudio_version() { return RTAUDIO_VERSION; }

extern "C" const RtAudio::Api rtaudio_compiled_apis[];
const rtaudio_api_t *rtaudio_compiled_api() {
  return (rtaudio_api_t *) &rtaudio_compiled_apis[0];
}

extern "C" const unsigned int rtaudio_num_compiled_apis;
unsigned int rtaudio_get_num_compiled_apis(void) {
  return rtaudio_num_compiled_apis;
}

extern "C" const char* rtaudio_api_names[][2];
const char *rtaudio_api_name(rtaudio_api_t api) {
    if (api < 0 || api >= RTAUDIO_API_NUM)
        return NULL;
    return rtaudio_api_names[api][0];
}

const char *rtaudio_api_display_name(rtaudio_api_t api)
{
    if (api < 0 || api >= RTAUDIO_API_NUM)
        return "Unknown";
    return rtaudio_api_names[api][1];
}

rtaudio_api_t rtaudio_compiled_api_by_name(const char *name) {
    RtAudio::Api api = RtAudio::UNSPECIFIED;
    if (name) {
        api = RtAudio::getCompiledApiByName(name);
    }
    return (rtaudio_api_t)api;
}

const char *rtaudio_error(rtaudio_t audio) {
  if (audio->has_error) {
    return audio->errmsg;
  }
  return NULL;
}

rtaudio_t rtaudio_create(rtaudio_api_t api) {
  rtaudio_t audio = new struct rtaudio();
  try {
    audio->audio = new RtAudio((RtAudio::Api)api);
  } catch (RtAudioError &err) {
    audio->has_error = 1;
    strncpy(audio->errmsg, err.what(), sizeof(audio->errmsg) - 1);
  }
  return audio;
}

void rtaudio_destroy(rtaudio_t audio) { delete audio->audio; }

rtaudio_api_t rtaudio_current_api(rtaudio_t audio) {
  return (rtaudio_api_t)audio->audio->getCurrentApi();
}

int rtaudio_device_count(rtaudio_t audio) {
  return audio->audio->getDeviceCount();
}

rtaudio_device_info_t rtaudio_get_device_info(rtaudio_t audio, int i) {
  rtaudio_device_info_t result;
  std::memset(&result, 0, sizeof(result));
  try {
    audio->has_error = 0;
    RtAudio::DeviceInfo info = audio->audio->getDeviceInfo(i);
    result.probed = info.probed;
    result.output_channels = info.outputChannels;
    result.input_channels = info.inputChannels;
    result.duplex_channels = info.duplexChannels;
    result.is_default_output = info.isDefaultOutput;
    result.is_default_input = info.isDefaultInput;
    result.native_formats = info.nativeFormats;
    result.preferred_sample_rate = info.preferredSampleRate;
    strncpy(result.name, info.name.c_str(), sizeof(result.name) - 1);
    for (unsigned int j = 0; j < info.sampleRates.size(); j++) {
      if (j < sizeof(result.sample_rates) / sizeof(result.sample_rates[0])) {
        result.sample_rates[j] = info.sampleRates[j];
      }
    }
  } catch (RtAudioError &err) {
    audio->has_error = 1;
    strncpy(audio->errmsg, err.what(), sizeof(audio->errmsg) - 1);
  }
  return result;
}

unsigned int rtaudio_get_default_output_device(rtaudio_t audio) {
  return audio->audio->getDefaultOutputDevice();
}

unsigned int rtaudio_get_default_input_device(rtaudio_t audio) {
  return audio->audio->getDefaultInputDevice();
}

static int proxy_cb_func(void *out, void *in, unsigned int nframes, double time,
                         RtAudioStreamStatus status, void *userdata) {
  rtaudio_t audio = (rtaudio_t)userdata;
  return audio->cb(out, in, nframes, time, (rtaudio_stream_status_t)status,
                   audio->userdata);
}

int rtaudio_open_stream(rtaudio_t audio,
                        rtaudio_stream_parameters_t *output_params,
                        rtaudio_stream_parameters_t *input_params,
                        rtaudio_format_t format, unsigned int sample_rate,
                        unsigned int *buffer_frames, rtaudio_cb_t cb,
                        void *userdata, rtaudio_stream_options_t *options,
                        rtaudio_error_cb_t /*errcb*/) {
  try {
    audio->has_error = 0;
    RtAudio::StreamParameters *in = NULL;
    RtAudio::StreamParameters *out = NULL;
    RtAudio::StreamOptions *opts = NULL;

    RtAudio::StreamParameters inparams;
    RtAudio::StreamParameters outparams;
    RtAudio::StreamOptions stream_opts;

    if (input_params != NULL) {
      inparams.deviceId = input_params->device_id;
      inparams.nChannels = input_params->num_channels;
      inparams.firstChannel = input_params->first_channel;
      in = &inparams;
    }
    if (output_params != NULL) {
      outparams.deviceId = output_params->device_id;
      outparams.nChannels = output_params->num_channels;
      outparams.firstChannel = output_params->first_channel;
      out = &outparams;
    }

    if (options != NULL) {
      stream_opts.flags = (RtAudioStreamFlags)options->flags;
      stream_opts.numberOfBuffers = options->num_buffers;
      stream_opts.priority = options->priority;
      if (strlen(options->name) > 0) {
        stream_opts.streamName = std::string(options->name);
      }
      opts = &stream_opts;
    }
    audio->cb = cb;
    audio->userdata = userdata;
    audio->audio->openStream(out, in, (RtAudioFormat)format, sample_rate,
                             buffer_frames, proxy_cb_func, (void *)audio, opts,
                             NULL);
    return 0;
  } catch (RtAudioError &err) {
    audio->has_error = 1;
    strncpy(audio->errmsg, err.what(), sizeof(audio->errmsg) - 1);
    return -1;
  }
}

void rtaudio_close_stream(rtaudio_t audio) { audio->audio->closeStream(); }

int rtaudio_start_stream(rtaudio_t audio) {
  try {
    audio->has_error = 0;
    audio->audio->startStream();
  } catch (RtAudioError &err) {
    audio->has_error = 1;
    strncpy(audio->errmsg, err.what(), sizeof(audio->errmsg) - 1);
  }
  return 0;
}

int rtaudio_stop_stream(rtaudio_t audio) {
  try {
    audio->has_error = 0;
    audio->audio->stopStream();
  } catch (RtAudioError &err) {
    audio->has_error = 1;
    strncpy(audio->errmsg, err.what(), sizeof(audio->errmsg) - 1);
  }
  return 0;
}

int rtaudio_abort_stream(rtaudio_t audio) {
  try {
    audio->has_error = 0;
    audio->audio->abortStream();
  } catch (RtAudioError &err) {
    audio->has_error = 1;
    strncpy(audio->errmsg, err.what(), sizeof(audio->errmsg) - 1);
  }
  return 0;
}

int rtaudio_is_stream_open(rtaudio_t audio) {
  return !!audio->audio->isStreamOpen();
}

int rtaudio_is_stream_running(rtaudio_t audio) {
  return !!audio->audio->isStreamRunning();
}

double rtaudio_get_stream_time(rtaudio_t audio) {
  try {
    audio->has_error = 0;
    return audio->audio->getStreamTime();
  } catch (RtAudioError &err) {
    audio->has_error = 1;
    strncpy(audio->errmsg, err.what(), sizeof(audio->errmsg) - 1);
    return 0;
  }
}

void rtaudio_set_stream_time(rtaudio_t audio, double time) {
  try {
    audio->has_error = 0;
    audio->audio->setStreamTime(time);
  } catch (RtAudioError &err) {
    audio->has_error = 1;
    strncpy(audio->errmsg, err.what(), sizeof(audio->errmsg) - 1);
  }
}

int rtaudio_get_stream_latency(rtaudio_t audio) {
  try {
    audio->has_error = 0;
    return audio->audio->getStreamLatency();
  } catch (RtAudioError &err) {
    audio->has_error = 1;
    strncpy(audio->errmsg, err.what(), sizeof(audio->errmsg) - 1);
    return -1;
  }
}

unsigned int rtaudio_get_stream_sample_rate(rtaudio_t audio) {
  try {
    return audio->audio->getStreamSampleRate();
  } catch (RtAudioError &err) {
    audio->has_error = 1;
    strncpy(audio->errmsg, err.what(), sizeof(audio->errmsg) - 1);
    return -1;
  }
}

void rtaudio_show_warnings(rtaudio_t audio, int show) {
  audio->audio->showWarnings(!!show);
}
