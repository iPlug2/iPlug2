/************************************************************************/
/*! \defgroup C-interface
    @{

    \brief C interface to realtime audio i/o C++ classes.

    RtAudio offers a C-style interface, principally for use in binding
    RtAudio to other programming languages.  All structs, enums, and
    functions listed here have direct analogs (and simply call to)
    items in the C++ RtAudio class and its supporting classes and
    types
*/
/************************************************************************/

/*!
  \file rtaudio_c.h
 */

#ifndef RTAUDIO_C_H
#define RTAUDIO_C_H

#if defined(RTAUDIO_EXPORT)
#if defined _WIN32 || defined __CYGWIN__
#define RTAUDIOAPI __declspec(dllexport)
#else
#define RTAUDIOAPI __attribute__((visibility("default")))
#endif
#else
#define RTAUDIOAPI //__declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*! \typedef typedef unsigned long rtaudio_format_t;
    \brief RtAudio data format type.

    - \e RTAUDIO_FORMAT_SINT8:   8-bit signed integer.
    - \e RTAUDIO_FORMAT_SINT16:  16-bit signed integer.
    - \e RTAUDIO_FORMAT_SINT24:  24-bit signed integer.
    - \e RTAUDIO_FORMAT_SINT32:  32-bit signed integer.
    - \e RTAUDIO_FORMAT_FLOAT32: Normalized between plus/minus 1.0.
    - \e RTAUDIO_FORMAT_FLOAT64: Normalized between plus/minus 1.0.

    See \ref RtAudioFormat.
*/
typedef unsigned long rtaudio_format_t;

#define RTAUDIO_FORMAT_SINT8 0x01
#define RTAUDIO_FORMAT_SINT16 0x02
#define RTAUDIO_FORMAT_SINT24 0x04
#define RTAUDIO_FORMAT_SINT32 0x08
#define RTAUDIO_FORMAT_FLOAT32 0x10
#define RTAUDIO_FORMAT_FLOAT64 0x20

/*! \typedef typedef unsigned long rtaudio_stream_flags_t;
    \brief RtAudio stream option flags.

    The following flags can be OR'ed together to allow a client to
    make changes to the default stream behavior:

    - \e RTAUDIO_FLAGS_NONINTERLEAVED:   Use non-interleaved buffers (default = interleaved).
    - \e RTAUDIO_FLAGS_MINIMIZE_LATENCY: Attempt to set stream parameters for lowest possible latency.
    - \e RTAUDIO_FLAGS_HOG_DEVICE:       Attempt grab device for exclusive use.
    - \e RTAUDIO_FLAGS_ALSA_USE_DEFAULT: Use the "default" PCM device (ALSA only).
    - \e RTAUDIO_FLAGS_JACK_DONT_CONNECT: Do not automatically connect ports (JACK only).

    See \ref RtAudioStreamFlags.
*/
typedef unsigned int rtaudio_stream_flags_t;

#define RTAUDIO_FLAGS_NONINTERLEAVED 0x1
#define RTAUDIO_FLAGS_MINIMIZE_LATENCY 0x2
#define RTAUDIO_FLAGS_HOG_DEVICE 0x4
#define RTAUDIO_FLAGS_SCHEDULE_REALTIME 0x8
#define RTAUDIO_FLAGS_ALSA_USE_DEFAULT 0x10
#define RTAUDIO_FLAGS_JACK_DONT_CONNECT = 0x20

/*! \typedef typedef unsigned long rtaudio_stream_status_t;
    \brief RtAudio stream status (over- or underflow) flags.

    Notification of a stream over- or underflow is indicated by a
    non-zero stream \c status argument in the RtAudioCallback function.
    The stream status can be one of the following two options,
    depending on whether the stream is open for output and/or input:

    - \e RTAUDIO_STATUS_INPUT_OVERFLOW:   Input data was discarded because of an overflow condition at the driver.
    - \e RTAUDIO_STATUS_OUTPUT_UNDERFLOW: The output buffer ran low, likely producing a break in the output sound.

    See \ref RtAudioStreamStatus.
*/
typedef unsigned int rtaudio_stream_status_t;

#define RTAUDIO_STATUS_INPUT_OVERFLOW 0x1
#define RTAUDIO_STATUS_OUTPUT_UNDERFLOW 0x2

//! RtAudio callback function prototype.
/*!
   All RtAudio clients must create a function of this type to read
   and/or write data from/to the audio stream.  When the underlying
   audio system is ready for new input or output data, this function
   will be invoked.

   See \ref RtAudioCallback.
 */
typedef int (*rtaudio_cb_t)(void *out, void *in, unsigned int nFrames,
                            double stream_time, rtaudio_stream_status_t status,
                            void *userdata);

/*! \brief Error codes for RtAudio.

    See \ref RtAudioError.
*/
typedef enum rtaudio_error {
  RTAUDIO_ERROR_WARNING,           /*!< A non-critical error. */
  RTAUDIO_ERROR_DEBUG_WARNING,     /*!< A non-critical error which might be useful for debugging. */
  RTAUDIO_ERROR_UNSPECIFIED,       /*!< The default, unspecified error type. */
  RTAUDIO_ERROR_NO_DEVICES_FOUND,  /*!< No devices found on system. */
  RTAUDIO_ERROR_INVALID_DEVICE,    /*!< An invalid device ID was specified. */
  RTAUDIO_ERROR_MEMORY_ERROR,      /*!< An error occured during memory allocation. */
  RTAUDIO_ERROR_INVALID_PARAMETER, /*!< An invalid parameter was specified to a function. */
  RTAUDIO_ERROR_INVALID_USE,       /*!< The function was called incorrectly. */
  RTAUDIO_ERROR_DRIVER_ERROR,      /*!< A system driver error occured. */
  RTAUDIO_ERROR_SYSTEM_ERROR,      /*!< A system error occured. */
  RTAUDIO_ERROR_THREAD_ERROR,      /*!< A thread error occured. */
} rtaudio_error_t;

//! RtAudio error callback function prototype.
/*!
    \param err Type of error.
    \param msg Error description.

    See \ref RtAudioErrorCallback.
 */
typedef void (*rtaudio_error_cb_t)(rtaudio_error_t err, const char *msg);

//! Audio API specifier.  See \ref RtAudio::Api.
typedef enum rtaudio_api {
  RTAUDIO_API_UNSPECIFIED,    /*!< Search for a working compiled API. */
  RTAUDIO_API_LINUX_ALSA,     /*!< The Advanced Linux Sound Architecture API. */
  RTAUDIO_API_LINUX_PULSE,    /*!< The Linux PulseAudio API. */
  RTAUDIO_API_LINUX_OSS,      /*!< The Linux Open Sound System API. */
  RTAUDIO_API_UNIX_JACK,      /*!< The Jack Low-Latency Audio Server API. */
  RTAUDIO_API_MACOSX_CORE,    /*!< Macintosh OS-X Core Audio API. */
  RTAUDIO_API_WINDOWS_WASAPI, /*!< The Microsoft WASAPI API. */
  RTAUDIO_API_WINDOWS_ASIO,   /*!< The Steinberg Audio Stream I/O API. */
  RTAUDIO_API_WINDOWS_DS,     /*!< The Microsoft DirectSound API. */
  RTAUDIO_API_DUMMY,          /*!< A compilable but non-functional API. */
  RTAUDIO_API_NUM,            /*!< Number of values in this enum. */
} rtaudio_api_t;

#define NUM_SAMPLE_RATES 16
#define MAX_NAME_LENGTH 512

//! The public device information structure for returning queried values.
//! See \ref RtAudio::DeviceInfo.
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

//! The structure for specifying input or ouput stream parameters.
//! See \ref RtAudio::StreamParameters.
typedef struct rtaudio_stream_parameters {
  unsigned int device_id;
  unsigned int num_channels;
  unsigned int first_channel;
} rtaudio_stream_parameters_t;

//! The structure for specifying stream options.
//! See \ref RtAudio::StreamOptions.
typedef struct rtaudio_stream_options {
  rtaudio_stream_flags_t flags;
  unsigned int num_buffers;
  int priority;
  char name[MAX_NAME_LENGTH];
} rtaudio_stream_options_t;

typedef struct rtaudio *rtaudio_t;

//! Determine the current RtAudio version.  See \ref RtAudio::getVersion().
RTAUDIOAPI const char *rtaudio_version(void);

//! Determine the number of available compiled audio APIs, the length
//! of the array returned by rtaudio_compiled_api().  See \ref
//! RtAudio::getCompiledApi().
RTAUDIOAPI unsigned int rtaudio_get_num_compiled_apis(void);

//! Return an array of rtaudio_api_t compiled into this instance of
//! RtAudio.  This array is static (do not free it) and has the length
//! returned by rtaudio_get_num_compiled_apis().  See \ref
//! RtAudio::getCompiledApi().
RTAUDIOAPI const rtaudio_api_t *rtaudio_compiled_api(void);

//! Return the name of a specified rtaudio_api_t.  This string can be
//! used to look up an API by rtaudio_compiled_api_by_name().  See
//! \ref RtAudio::getApiName().
RTAUDIOAPI const char *rtaudio_api_name(rtaudio_api_t api);

//! Return the display name of a specified rtaudio_api_t.  See \ref
//! RtAudio::getApiDisplayName().
RTAUDIOAPI const char *rtaudio_api_display_name(rtaudio_api_t api);

//! Return the rtaudio_api_t having the given name.  See \ref
//! RtAudio::getCompiledApiByName().
RTAUDIOAPI rtaudio_api_t rtaudio_compiled_api_by_name(const char *name);

RTAUDIOAPI const char *rtaudio_error(rtaudio_t audio);

//! Create an instance of struct rtaudio.
RTAUDIOAPI rtaudio_t rtaudio_create(rtaudio_api_t api);

//! Free an instance of struct rtaudio.
RTAUDIOAPI void rtaudio_destroy(rtaudio_t audio);

//! Returns the audio API specifier for the current instance of
//! RtAudio.  See RtAudio::getCurrentApi().
RTAUDIOAPI rtaudio_api_t rtaudio_current_api(rtaudio_t audio);

//! Queries for the number of audio devices available.  See \ref
//! RtAudio::getDeviceCount().
RTAUDIOAPI int rtaudio_device_count(rtaudio_t audio);

//! Return a struct rtaudio_device_info for a specified device number.
//! See \ref RtAudio::getDeviceInfo().
RTAUDIOAPI rtaudio_device_info_t rtaudio_get_device_info(rtaudio_t audio,
                                                         int i);

//! Returns the index of the default output device.  See \ref
//! RtAudio::getDefaultOutputDevice().
RTAUDIOAPI unsigned int rtaudio_get_default_output_device(rtaudio_t audio);

//! Returns the index of the default input device.  See \ref
//! RtAudio::getDefaultInputDevice().
RTAUDIOAPI unsigned int rtaudio_get_default_input_device(rtaudio_t audio);

//! Opens a stream with the specified parameters.  See \ref RtAudio::openStream().
//! \return an \ref rtaudio_error.
RTAUDIOAPI int
rtaudio_open_stream(rtaudio_t audio, rtaudio_stream_parameters_t *output_params,
                    rtaudio_stream_parameters_t *input_params,
                    rtaudio_format_t format, unsigned int sample_rate,
                    unsigned int *buffer_frames, rtaudio_cb_t cb,
                    void *userdata, rtaudio_stream_options_t *options,
                    rtaudio_error_cb_t errcb);

//! Closes a stream and frees any associated stream memory.  See \ref RtAudio::closeStream().
RTAUDIOAPI void rtaudio_close_stream(rtaudio_t audio);

//! Starts a stream.  See \ref RtAudio::startStream().
RTAUDIOAPI int rtaudio_start_stream(rtaudio_t audio);

//! Stop a stream, allowing any samples remaining in the output queue
//! to be played.  See \ref RtAudio::stopStream().
RTAUDIOAPI int rtaudio_stop_stream(rtaudio_t audio);

//! Stop a stream, discarding any samples remaining in the
//! input/output queue.  See \ref RtAudio::abortStream().
RTAUDIOAPI int rtaudio_abort_stream(rtaudio_t audio);

//! Returns 1 if a stream is open and false if not.  See \ref RtAudio::isStreamOpen().
RTAUDIOAPI int rtaudio_is_stream_open(rtaudio_t audio);

//! Returns 1 if a stream is running and false if it is stopped or not
//! open.  See \ref RtAudio::isStreamRunning().
RTAUDIOAPI int rtaudio_is_stream_running(rtaudio_t audio);

//! Returns the number of elapsed seconds since the stream was
//! started.  See \ref RtAudio::getStreamTime().
RTAUDIOAPI double rtaudio_get_stream_time(rtaudio_t audio);

//! Set the stream time to a time in seconds greater than or equal to
//! 0.0.  See \ref RtAudio::setStreamTime().
RTAUDIOAPI void rtaudio_set_stream_time(rtaudio_t audio, double time);

//! Returns the internal stream latency in sample frames.  See \ref
//! RtAudio::getStreamLatency().
RTAUDIOAPI int rtaudio_get_stream_latency(rtaudio_t audio);

//! Returns actual sample rate in use by the stream.  See \ref
//! RtAudio::getStreamSampleRate().
RTAUDIOAPI unsigned int rtaudio_get_stream_sample_rate(rtaudio_t audio);

//! Specify whether warning messages should be printed to stderr.  See
//! \ref RtAudio::showWarnings().
RTAUDIOAPI void rtaudio_show_warnings(rtaudio_t audio, int show);

#ifdef __cplusplus
}
#endif
#endif /* RTAUDIO_C_H */

/*! }@ */
