#include "rtaudio_c.h"
#include "RtAudio.h"

#include <cstring>

#define MAX_ERROR_MESSAGE_LENGTH 512

struct rtaudio {
  RtAudio *audio;

  rtaudio_cb_t cb;
  void *userdata;

  rtaudio_error_t errtype;
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
  if (audio->errtype == RTAUDIO_ERROR_NONE) {
    return NULL;
  }
  return audio->errmsg;
}

rtaudio_error_t rtaudio_error_type(rtaudio_t audio) {
  return audio->errtype;
}

rtaudio_t rtaudio_create(rtaudio_api_t api) {
  rtaudio_t audio = new struct rtaudio();
  audio->errtype = RTAUDIO_ERROR_NONE;
  audio->audio = new RtAudio((RtAudio::Api)api,
    [audio](RtAudioErrorType type, const std::string &errorText){
      audio->errtype = (rtaudio_error_t)type;
      strncpy(audio->errmsg, errorText.c_str(), errorText.size() - 1);
    });
  return audio;
}

void rtaudio_destroy(rtaudio_t audio) { delete audio->audio; }

rtaudio_api_t rtaudio_current_api(rtaudio_t audio) {
  return (rtaudio_api_t)audio->audio->getCurrentApi();
}

int rtaudio_device_count(rtaudio_t audio) {
  return audio->audio->getDeviceCount();
}

unsigned int rtaudio_get_device_id(rtaudio_t audio, int i) {
  std::vector<unsigned int> deviceIds = audio->audio->getDeviceIds();
  if ( i >= 0 && i < (int) deviceIds.size() )
    return deviceIds[i];
  else
    return 0;
}

rtaudio_device_info_t rtaudio_get_device_info(rtaudio_t audio, unsigned int id) {
  rtaudio_device_info_t result;
  std::memset(&result, 0, sizeof(result));

  audio->errtype = RTAUDIO_ERROR_NONE;
  RtAudio::DeviceInfo info = audio->audio->getDeviceInfo(id);
  if (audio->errtype != RTAUDIO_ERROR_NONE)
      return result;

  result.id = info.ID;
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

rtaudio_error_t rtaudio_open_stream(rtaudio_t audio,
                        rtaudio_stream_parameters_t *output_params,
                        rtaudio_stream_parameters_t *input_params,
                        rtaudio_format_t format, unsigned int sample_rate,
                        unsigned int *buffer_frames, rtaudio_cb_t cb,
                        void *userdata, rtaudio_stream_options_t *options,
                        rtaudio_error_cb_t /*errcb*/)
{
  audio->errtype = RTAUDIO_ERROR_NONE;
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
                           buffer_frames, proxy_cb_func, (void *)audio, opts); //,  NULL);
  return audio->errtype;
}

void rtaudio_close_stream(rtaudio_t audio) { audio->audio->closeStream(); }

rtaudio_error_t rtaudio_start_stream(rtaudio_t audio) {
  audio->errtype = RTAUDIO_ERROR_NONE;
  audio->audio->startStream();
  return audio->errtype;
}

rtaudio_error_t rtaudio_stop_stream(rtaudio_t audio) {
  audio->errtype = RTAUDIO_ERROR_NONE;
  audio->audio->stopStream();
  return audio->errtype;
}

rtaudio_error_t rtaudio_abort_stream(rtaudio_t audio) {
  audio->errtype = RTAUDIO_ERROR_NONE;
  audio->audio->abortStream();
  return audio->errtype;
}

int rtaudio_is_stream_open(rtaudio_t audio) {
  return !!audio->audio->isStreamOpen();
}

int rtaudio_is_stream_running(rtaudio_t audio) {
  return !!audio->audio->isStreamRunning();
}

double rtaudio_get_stream_time(rtaudio_t audio) {
  audio->errtype = RTAUDIO_ERROR_NONE;
  return audio->audio->getStreamTime();
}

void rtaudio_set_stream_time(rtaudio_t audio, double time) {
  audio->errtype = RTAUDIO_ERROR_NONE;
  audio->audio->setStreamTime(time);
}

long rtaudio_get_stream_latency(rtaudio_t audio) {
  audio->errtype = RTAUDIO_ERROR_NONE;
  return audio->audio->getStreamLatency();
}

unsigned int rtaudio_get_stream_sample_rate(rtaudio_t audio) {
  audio->errtype = RTAUDIO_ERROR_NONE;
  return audio->audio->getStreamSampleRate();
}

void rtaudio_show_warnings(rtaudio_t audio, int show) {
  audio->audio->showWarnings(!!show);
}
