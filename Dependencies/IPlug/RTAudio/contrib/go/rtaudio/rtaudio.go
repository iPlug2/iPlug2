package rtaudio

/*

#cgo CXXFLAGS: -g
#cgo LDFLAGS: -lstdc++ -g

#cgo linux CXXFLAGS: -D__LINUX_ALSA__
#cgo linux LDFLAGS: -lm -lasound -pthread

#cgo linux,pulseaudio CXXFLAGS: -D__LINUX_PULSE__
#cgo linux,pulseaudio LDFLAGS: -lpulse -lpulse-simple

#cgo jack CXXFLAGS: -D__UNIX_JACK__
#cgo jack LDFLAGS: -ljack

#cgo windows CXXFLAGS: -D__WINDOWS_WASAPI__
#cgo windows LDFLAGS: -lm -lksuser -lmfplat -lmfuuid -lwmcodecdspuuid -lwinmm -lole32 -static

#cgo darwin CXXFLAGS: -D__MACOSX_CORE__
#cgo darwin LDFLAGS: -framework CoreAudio -framework CoreFoundation

#include <stdlib.h>
#include <stdint.h>
#include "rtaudio_stub.h"

extern int goCallback(void *out, void *in, unsigned int nFrames,
	double stream_time, rtaudio_stream_status_t status, void *userdata);

static inline void cgoRtAudioOpenStream(rtaudio_t audio,
	rtaudio_stream_parameters_t *output_params,
	rtaudio_stream_parameters_t *input_params,
	rtaudio_format_t format,
	unsigned int sample_rate,
	unsigned int *buffer_frames,
	int cb_id,
	rtaudio_stream_options_t *options) {
		rtaudio_open_stream(audio, output_params, input_params,
			format, sample_rate, buffer_frames,
			goCallback, (void *)(uintptr_t)cb_id, options, NULL);
}
*/
import "C"
import (
	"errors"
	"sync"
	"time"
	"unsafe"
)

// API is an enumeration of available compiled APIs. Supported API include
// Alsa/PulseAudio/OSS, Jack, CoreAudio, WASAPI/ASIO/DS and dummy API.
type API C.rtaudio_api_t

const (
	// APIUnspecified looks for a working compiled API.
	APIUnspecified API = C.RTAUDIO_API_UNSPECIFIED
	// APILinuxALSA uses the Advanced Linux Sound Architecture API.
	APILinuxALSA = C.RTAUDIO_API_LINUX_ALSA
	// APILinuxPulse uses the Linux PulseAudio API.
	APILinuxPulse = C.RTAUDIO_API_LINUX_PULSE
	// APILinuxOSS uses the Linux Open Sound System API.
	APILinuxOSS = C.RTAUDIO_API_LINUX_OSS
	// APIUnixJack uses the Jack Low-Latency Audio Server API.
	APIUnixJack = C.RTAUDIO_API_UNIX_JACK
	// APIMacOSXCore uses Macintosh OS-X Core Audio API.
	APIMacOSXCore = C.RTAUDIO_API_MACOSX_CORE
	// APIWindowsWASAPI uses the Microsoft WASAPI API.
	APIWindowsWASAPI = C.RTAUDIO_API_WINDOWS_WASAPI
	// APIWindowsASIO uses the Steinberg Audio Stream I/O API.
	APIWindowsASIO = C.RTAUDIO_API_WINDOWS_ASIO
	// APIWindowsDS uses the Microsoft DirectSound API.
	APIWindowsDS = C.RTAUDIO_API_WINDOWS_DS
	// APIDummy is a compilable but non-functional API.
	APIDummy = C.RTAUDIO_API_DUMMY
)

func (api API) String() string {
	switch api {
	case APIUnspecified:
		return "unspecified"
	case APILinuxALSA:
		return "alsa"
	case APILinuxPulse:
		return "pulse"
	case APILinuxOSS:
		return "oss"
	case APIUnixJack:
		return "jack"
	case APIMacOSXCore:
		return "coreaudio"
	case APIWindowsWASAPI:
		return "wasapi"
	case APIWindowsASIO:
		return "asio"
	case APIWindowsDS:
		return "directsound"
	case APIDummy:
		return "dummy"
	}
	return "?"
}

// StreamStatus defines over- or underflow flags in the audio callback.
type StreamStatus C.rtaudio_stream_status_t

const (
	// StatusInputOverflow indicates that data was discarded because of an
	// overflow condition at the driver.
	StatusInputOverflow StreamStatus = C.RTAUDIO_STATUS_INPUT_OVERFLOW
	// StatusOutputUnderflow indicates that the output buffer ran low, likely
	// producing a break in the output sound.
	StatusOutputUnderflow StreamStatus = C.RTAUDIO_STATUS_OUTPUT_UNDERFLOW
)

// Version returns current RtAudio library version string.
func Version() string {
	return C.GoString(C.rtaudio_version())
}

// CompiledAPI determines the available compiled audio APIs.
func CompiledAPI() (apis []API) {
	capis := (*[1 << 27]C.rtaudio_api_t)(unsafe.Pointer(C.rtaudio_compiled_api()))
	for i := 0; ; i++ {
		api := capis[i]
		if api == C.RTAUDIO_API_UNSPECIFIED {
			break
		}
		apis = append(apis, API(api))
	}
	return apis
}

// DeviceInfo is the public device information structure for returning queried values.
type DeviceInfo struct {
	Name              string
	Probed            bool
	NumOutputChannels int
	NumInputChannels  int
	NumDuplexChannels int
	IsDefaultOutput   bool
	IsDefaultInput    bool

	//rtaudio_format_t native_formats;

	PreferredSampleRate uint
	SampleRates         []int
}

// StreamParams is the structure for specifying input or output stream parameters.
type StreamParams struct {
	DeviceID     uint
	NumChannels  uint
	FirstChannel uint
}

// StreamFlags is a set of RtAudio stream option flags.
type StreamFlags C.rtaudio_stream_flags_t

const (
	// FlagsNoninterleaved is set to use non-interleaved buffers (default = interleaved).
	FlagsNoninterleaved = C.RTAUDIO_FLAGS_NONINTERLEAVED
	// FlagsMinimizeLatency when set attempts to configure stream parameters for lowest possible latency.
	FlagsMinimizeLatency = C.RTAUDIO_FLAGS_MINIMIZE_LATENCY
	// FlagsHogDevice when set attempts to grab device for exclusive use.
	FlagsHogDevice = C.RTAUDIO_FLAGS_HOG_DEVICE
	// FlagsScheduleRealtime is set in attempt to select realtime scheduling (round-robin) for the callback thread.
	FlagsScheduleRealtime = C.RTAUDIO_FLAGS_SCHEDULE_REALTIME
	// FlagsAlsaUseDefault is set to use the "default" PCM device (ALSA only).
	FlagsAlsaUseDefault = C.RTAUDIO_FLAGS_ALSA_USE_DEFAULT
)

// StreamOptions is the structure for specifying stream options.
type StreamOptions struct {
	Flags      StreamFlags
	NumBuffers uint
	Priotity   int
	Name       string
}

// RtAudio is a "controller" used to select an available audio i/o interface.
type RtAudio interface {
	Destroy()
	CurrentAPI() API
	Devices() ([]DeviceInfo, error)
	DefaultOutputDevice() int
	DefaultInputDevice() int

	Open(out, in *StreamParams, format Format, sampleRate uint, frames uint, cb Callback, opts *StreamOptions) error
	Close()
	Start() error
	Stop() error
	Abort() error

	IsOpen() bool
	IsRunning() bool

	Latency() (int, error)
	SampleRate() (uint, error)
	Time() (time.Duration, error)
	SetTime(time.Duration) error

	ShowWarnings(bool)
}

type rtaudio struct {
	audio          C.rtaudio_t
	cb             Callback
	inputChannels  int
	outputChannels int
	format         Format
}

var _ RtAudio = &rtaudio{}

// Create a new RtAudio instance using the given API.
func Create(api API) (RtAudio, error) {
	audio := C.rtaudio_create(C.rtaudio_api_t(api))
	if C.rtaudio_error(audio) != nil {
		return nil, errors.New(C.GoString(C.rtaudio_error(audio)))
	}
	return &rtaudio{audio: audio}, nil
}

func (audio *rtaudio) Destroy() {
	C.rtaudio_destroy(audio.audio)
}

func (audio *rtaudio) CurrentAPI() API {
	return API(C.rtaudio_current_api(audio.audio))
}

func (audio *rtaudio) DefaultInputDevice() int {
	return int(C.rtaudio_get_default_input_device(audio.audio))
}

func (audio *rtaudio) DefaultOutputDevice() int {
	return int(C.rtaudio_get_default_output_device(audio.audio))
}

func (audio *rtaudio) Devices() ([]DeviceInfo, error) {
	n := C.rtaudio_device_count(audio.audio)
	devices := []DeviceInfo{}
	for i := C.int(0); i < n; i++ {
		cinfo := C.rtaudio_get_device_info(audio.audio, i)
		if C.rtaudio_error(audio.audio) != nil {
			return nil, errors.New(C.GoString(C.rtaudio_error(audio.audio)))
		}
		sr := []int{}
		for _, r := range cinfo.sample_rates {
			if r == 0 {
				break
			}
			sr = append(sr, int(r))
		}
		devices = append(devices, DeviceInfo{
			Name:                C.GoString(&cinfo.name[0]),
			Probed:              cinfo.probed != 0,
			NumInputChannels:    int(cinfo.input_channels),
			NumOutputChannels:   int(cinfo.output_channels),
			NumDuplexChannels:   int(cinfo.duplex_channels),
			IsDefaultOutput:     cinfo.is_default_output != 0,
			IsDefaultInput:      cinfo.is_default_input != 0,
			PreferredSampleRate: uint(cinfo.preferred_sample_rate),
			SampleRates:         sr,
		})
		// TODO: formats
	}
	return devices, nil
}

// Format defines RtAudio data format type.
type Format int

const (
	// FormatInt8 uses 8-bit signed integer.
	FormatInt8 Format = C.RTAUDIO_FORMAT_SINT8
	// FormatInt16 uses 16-bit signed integer.
	FormatInt16 = C.RTAUDIO_FORMAT_SINT16
	// FormatInt24 uses 24-bit signed integer.
	FormatInt24 = C.RTAUDIO_FORMAT_SINT24
	// FormatInt32 uses 32-bit signed integer.
	FormatInt32 = C.RTAUDIO_FORMAT_SINT32
	// FormatFloat32 uses 32-bit floating point values normalized between (-1..1).
	FormatFloat32 = C.RTAUDIO_FORMAT_FLOAT32
	// FormatFloat64 uses 64-bit floating point values normalized between (-1..1).
	FormatFloat64 = C.RTAUDIO_FORMAT_FLOAT64
)

// Buffer is a common interface for audio buffers of various data format types.
type Buffer interface {
	Len() int
	Int8() []int8
	Int16() []int16
	Int24() []Int24
	Int32() []int32
	Float32() []float32
	Float64() []float64
}

// Int24 is a helper type to convert int32 values to int24 and back.
type Int24 [3]byte

// Set Int24 value using the least significant bytes of the given number n.
func (i *Int24) Set(n int32) {
	(*i)[0], (*i)[1], (*i)[2] = byte(n&0xff), byte((n&0xff00)>>8), byte((n&0xff0000)>>16)
}

// Get Int24 value as int32.
func (i Int24) Get() int32 {
	n := int32(i[0]) | int32(i[1])<<8 | int32(i[2])<<16
	if n&0x800000 != 0 {
		n |= ^0xffffff
	}
	return n
}

type buffer struct {
	format      Format
	length      int
	numChannels int
	ptr         unsafe.Pointer
}

func (b *buffer) Len() int {
	if b.ptr == nil {
		return 0
	}
	return b.length
}

func (b *buffer) Int8() []int8 {
	if b.format != FormatInt8 {
		return nil
	}
	if b.ptr == nil {
		return nil
	}
	return (*[1 << 30]int8)(b.ptr)[:b.length*b.numChannels : b.length*b.numChannels]
}

func (b *buffer) Int16() []int16 {
	if b.format != FormatInt16 {
		return nil
	}
	if b.ptr == nil {
		return nil
	}
	return (*[1 << 29]int16)(b.ptr)[:b.length*b.numChannels : b.length*b.numChannels]
}

func (b *buffer) Int24() []Int24 {
	if b.format != FormatInt24 {
		return nil
	}
	if b.ptr == nil {
		return nil
	}
	return (*[1 << 28]Int24)(b.ptr)[:b.length*b.numChannels : b.length*b.numChannels]
}

func (b *buffer) Int32() []int32 {
	if b.format != FormatInt32 {
		return nil
	}
	if b.ptr == nil {
		return nil
	}
	return (*[1 << 27]int32)(b.ptr)[:b.length*b.numChannels : b.length*b.numChannels]
}

func (b *buffer) Float32() []float32 {
	if b.format != FormatFloat32 {
		return nil
	}
	if b.ptr == nil {
		return nil
	}
	return (*[1 << 27]float32)(b.ptr)[:b.length*b.numChannels : b.length*b.numChannels]
}

func (b *buffer) Float64() []float64 {
	if b.format != FormatFloat64 {
		return nil
	}
	if b.ptr == nil {
		return nil
	}
	return (*[1 << 23]float64)(b.ptr)[:b.length*b.numChannels : b.length*b.numChannels]
}

// Callback is a client-defined function that will be invoked when input data
// is available and/or output data is needed.
type Callback func(out Buffer, in Buffer, dur time.Duration, status StreamStatus) int

var (
	mu     sync.Mutex
	audios = map[int]*rtaudio{}
)

func registerAudio(a *rtaudio) int {
	mu.Lock()
	defer mu.Unlock()
	for i := 0; ; i++ {
		if _, ok := audios[i]; !ok {
			audios[i] = a
			return i
		}
	}
}

func unregisterAudio(a *rtaudio) {
	mu.Lock()
	defer mu.Unlock()
	for i := 0; i < len(audios); i++ {
		if audios[i] == a {
			delete(audios, i)
			return
		}
	}
}

func findAudio(k int) *rtaudio {
	mu.Lock()
	defer mu.Unlock()
	return audios[k]
}

//export goCallback
func goCallback(out, in unsafe.Pointer, frames C.uint, sec C.double,
	status C.rtaudio_stream_status_t, userdata unsafe.Pointer) C.int {

	k := int(uintptr(userdata))
	audio := findAudio(k)
	dur := time.Duration(time.Microsecond * time.Duration(sec*1000000.0))
	inbuf := &buffer{audio.format, int(frames), audio.inputChannels, in}
	outbuf := &buffer{audio.format, int(frames), audio.outputChannels, out}
	return C.int(audio.cb(outbuf, inbuf, dur, StreamStatus(status)))
}

func (audio *rtaudio) Open(out, in *StreamParams, format Format, sampleRate uint,
	frames uint, cb Callback, opts *StreamOptions) error {
	var (
		cInPtr   *C.rtaudio_stream_parameters_t
		cOutPtr  *C.rtaudio_stream_parameters_t
		cOptsPtr *C.rtaudio_stream_options_t
		cIn      C.rtaudio_stream_parameters_t
		cOut     C.rtaudio_stream_parameters_t
		cOpts    C.rtaudio_stream_options_t
	)

	audio.inputChannels = 0
	audio.outputChannels = 0
	if out != nil {
		audio.outputChannels = int(out.NumChannels)
		cOut.device_id = C.uint(out.DeviceID)
		cOut.num_channels = C.uint(out.NumChannels)
		cOut.first_channel = C.uint(out.FirstChannel)
		cOutPtr = &cOut
	}
	if in != nil {
		audio.inputChannels = int(in.NumChannels)
		cIn.device_id = C.uint(in.DeviceID)
		cIn.num_channels = C.uint(in.NumChannels)
		cIn.first_channel = C.uint(in.FirstChannel)
		cInPtr = &cIn
	}
	if opts != nil {
		cOpts.flags = C.rtaudio_stream_flags_t(opts.Flags)
		cOpts.num_buffers = C.uint(opts.NumBuffers)
		cOpts.priority = C.int(opts.Priotity)
		cOptsPtr = &cOpts
	}
	framesCount := C.uint(frames)
	audio.format = format
	audio.cb = cb

	k := registerAudio(audio)
	C.cgoRtAudioOpenStream(audio.audio, cOutPtr, cInPtr,
		C.rtaudio_format_t(format), C.uint(sampleRate), &framesCount, C.int(k), cOptsPtr)
	if C.rtaudio_error(audio.audio) != nil {
		return errors.New(C.GoString(C.rtaudio_error(audio.audio)))
	}
	return nil
}

func (audio *rtaudio) Close() {
	unregisterAudio(audio)
	C.rtaudio_close_stream(audio.audio)
}

func (audio *rtaudio) Start() error {
	C.rtaudio_start_stream(audio.audio)
	if C.rtaudio_error(audio.audio) != nil {
		return errors.New(C.GoString(C.rtaudio_error(audio.audio)))
	}
	return nil
}

func (audio *rtaudio) Stop() error {
	C.rtaudio_stop_stream(audio.audio)
	if C.rtaudio_error(audio.audio) != nil {
		return errors.New(C.GoString(C.rtaudio_error(audio.audio)))
	}
	return nil
}

func (audio *rtaudio) Abort() error {
	C.rtaudio_abort_stream(audio.audio)
	if C.rtaudio_error(audio.audio) != nil {
		return errors.New(C.GoString(C.rtaudio_error(audio.audio)))
	}
	return nil
}

func (audio *rtaudio) IsOpen() bool {
	return C.rtaudio_is_stream_open(audio.audio) != 0
}

func (audio *rtaudio) IsRunning() bool {
	return C.rtaudio_is_stream_running(audio.audio) != 0
}

func (audio *rtaudio) Latency() (int, error) {
	latency := C.rtaudio_get_stream_latency(audio.audio)
	if C.rtaudio_error(audio.audio) != nil {
		return 0, errors.New(C.GoString(C.rtaudio_error(audio.audio)))
	}
	return int(latency), nil
}

func (audio *rtaudio) SampleRate() (uint, error) {
	sampleRate := C.rtaudio_get_stream_sample_rate(audio.audio)
	if C.rtaudio_error(audio.audio) != nil {
		return 0, errors.New(C.GoString(C.rtaudio_error(audio.audio)))
	}
	return uint(sampleRate), nil
}

func (audio *rtaudio) Time() (time.Duration, error) {
	sec := C.rtaudio_get_stream_time(audio.audio)
	if C.rtaudio_error(audio.audio) != nil {
		return 0, errors.New(C.GoString(C.rtaudio_error(audio.audio)))
	}
	return time.Duration(time.Microsecond * time.Duration(sec*1000000.0)), nil
}

func (audio *rtaudio) SetTime(t time.Duration) error {
	sec := float64(t) * 1000000.0 / float64(time.Microsecond)
	C.rtaudio_set_stream_time(audio.audio, C.double(sec))
	if C.rtaudio_error(audio.audio) != nil {
		return errors.New(C.GoString(C.rtaudio_error(audio.audio)))
	}
	return nil
}

func (audio *rtaudio) ShowWarnings(show bool) {
	if show {
		C.rtaudio_show_warnings(audio.audio, 1)
	} else {
		C.rtaudio_show_warnings(audio.audio, 0)
	}
}
