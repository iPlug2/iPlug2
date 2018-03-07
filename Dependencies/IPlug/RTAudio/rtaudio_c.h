#ifndef RTAUDIO_C_H
#define RTAUDIO_C_H

#if defined(RTAUDIO_EXPORT)
#define RTAUDIOAPI __declspec(dllexport)
#else
#define RTAUDIOAPI //__declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long rtaudio_format_t;

#define RTAUDIO_FORMAT_SINT8 0x01
#define RTAUDIO_FORMAT_SINT16 0x02
#define RTAUDIO_FORMAT_SINT24 0x04
#define RTAUDIO_FORMAT_SINT32 0x08
#define RTAUDIO_FORMAT_FLOAT32 0x10
#define RTAUDIO_FORMAT_FLOAT64 0x20

typedef unsigned int rtaudio_stream_flags_t;

#define RTAUDIO_FLAGS_NONINTERLEAVED 0x1
#define RTAUDIO_FLAGS_MINIMIZE_LATENCY 0x2
#define RTAUDIO_FLAGS_HOG_DEVICE 0x4
#define RTAUDIO_FLAGS_SCHEDULE_REALTIME 0x8
#define RTAUDIO_FLAGS_ALSA_USE_DEFAULT 0x10

typedef unsigned int rtaudio_stream_status_t;

#define RTAUDIO_STATUS_INPUT_OVERFLOW 0x1
#define RTAUDIO_STATUS_OUTPUT_UNDERFLOW 0x2

typedef int (*rtaudio_cb_t)(void *out, void *in, unsigned int nFrames,
                            double stream_time, rtaudio_stream_status_t status,
                            void *userdata);

typedef enum rtaudio_error {
  RTAUDIO_ERROR_WARNING,
  RTAUDIO_ERROR_DEBUG_WARNING,
  RTAUDIO_ERROR_UNSPECIFIED,
  RTAUDIO_ERROR_NO_DEVICES_FOUND,
  RTAUDIO_ERROR_INVALID_DEVICE,
  RTAUDIO_ERROR_MEMORY_ERROR,
  RTAUDIO_ERROR_INVALID_PARAMETER,
  RTAUDIO_ERROR_INVALID_USE,
  RTAUDIO_ERROR_DRIVER_ERROR,
  RTAUDIO_ERROR_SYSTEM_ERROR,
  RTAUDIO_ERROR_THREAD_ERROR,
} rtaudio_error_t;

typedef void (*rtaudio_error_cb_t)(rtaudio_error_t err, const char *msg);

typedef enum rtaudio_api {
  RTAUDIO_API_UNSPECIFIED,
  RTAUDIO_API_LINUX_ALSA,
  RTAUDIO_API_LINUX_PULSE,
  RTAUDIO_API_LINUX_OSS,
  RTAUDIO_API_UNIX_JACK,
  RTAUDIO_API_MACOSX_CORE,
  RTAUDIO_API_WINDOWS_WASAPI,
  RTAUDIO_API_WINDOWS_ASIO,
  RTAUDIO_API_WINDOWS_DS,
  RTAUDIO_API_DUMMY,
} rtaudio_api_t;

#define NUM_SAMPLE_RATES 16
#define MAX_NAME_LENGTH 512
typedef struct rtaudio_device_info {
  int probed;
  unsigned int output_channels;
  unsigned int input_channels;
  unsigned int duplex_channels;

  int is_default_output;
  int is_default_input;

  rtaudio_format_t native_formats;

  unsigned int preferred_sample_rate;
  int sample_rates[NUM_SAMPLE_RATES];

  char name[MAX_NAME_LENGTH];
} rtaudio_device_info_t;

typedef struct rtaudio_stream_parameters {
  unsigned int device_id;
  unsigned int num_channels;
  unsigned int first_channel;
} rtaudio_stream_parameters_t;

typedef struct rtaudio_stream_options {
  rtaudio_stream_flags_t flags;
  unsigned int num_buffers;
  int priority;
  char name[MAX_NAME_LENGTH];
} rtaudio_stream_options_t;

typedef struct rtaudio *rtaudio_t;

RTAUDIOAPI const char *rtaudio_version();
RTAUDIOAPI const rtaudio_api_t *rtaudio_compiled_api();

RTAUDIOAPI const char *rtaudio_error(rtaudio_t audio);

RTAUDIOAPI rtaudio_t rtaudio_create(rtaudio_api_t api);
RTAUDIOAPI void rtaudio_destroy(rtaudio_t audio);

RTAUDIOAPI rtaudio_api_t rtaudio_current_api(rtaudio_t audio);

RTAUDIOAPI int rtaudio_device_count(rtaudio_t audio);
RTAUDIOAPI rtaudio_device_info_t rtaudio_get_device_info(rtaudio_t audio,
                                                         int i);
RTAUDIOAPI unsigned int rtaudio_get_default_output_device(rtaudio_t audio);
RTAUDIOAPI unsigned int rtaudio_get_default_input_device(rtaudio_t audio);

RTAUDIOAPI int
rtaudio_open_stream(rtaudio_t audio, rtaudio_stream_parameters_t *output_params,
                    rtaudio_stream_parameters_t *input_params,
                    rtaudio_format_t format, unsigned int sample_rate,
                    unsigned int *buffer_frames, rtaudio_cb_t cb,
                    void *userdata, rtaudio_stream_options_t *options,
                    rtaudio_error_cb_t errcb);
RTAUDIOAPI void rtaudio_close_stream(rtaudio_t audio);
RTAUDIOAPI int rtaudio_start_stream(rtaudio_t audio);
RTAUDIOAPI int rtaudio_stop_stream(rtaudio_t audio);
RTAUDIOAPI int rtaudio_abort_stream(rtaudio_t audio);

RTAUDIOAPI int rtaudio_is_stream_open(rtaudio_t audio);
RTAUDIOAPI int rtaudio_is_stream_running(rtaudio_t audio);

RTAUDIOAPI double rtaudio_get_stream_time(rtaudio_t audio);
RTAUDIOAPI void rtaudio_set_stream_time(rtaudio_t audio, double time);
RTAUDIOAPI int rtaudio_get_stream_latency(rtaudio_t audio);
RTAUDIOAPI unsigned int rtaudio_get_stream_sample_rate(rtaudio_t audio);

RTAUDIOAPI void rtaudio_show_warnings(rtaudio_t audio, int show);

#ifdef __cplusplus
}
#endif
#endif /* RTAUDIO_C_H */
