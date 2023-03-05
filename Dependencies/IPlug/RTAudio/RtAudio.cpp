/************************************************************************/
/*! \class RtAudio
    \brief Realtime audio i/o C++ classes.

    RtAudio provides a common API (Application Programming Interface)
    for realtime audio input/output across Linux (native ALSA, Jack,
    and OSS), Macintosh OS X (CoreAudio and Jack), and Windows
    (DirectSound, ASIO and WASAPI) operating systems.

    RtAudio GitHub site: https://github.com/thestk/rtaudio
    RtAudio WWW site: http://www.music.mcgill.ca/~gary/rtaudio/

    RtAudio: realtime audio i/o C++ classes
    Copyright (c) 2001-2023 Gary P. Scavone

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation files
    (the "Software"), to deal in the Software without restriction,
    including without limitation the rights to use, copy, modify, merge,
    publish, distribute, sublicense, and/or sell copies of the Software,
    and to permit persons to whom the Software is furnished to do so,
    subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    Any person wishing to distribute modifications to the Software is
    asked to send the modifications to the original developer so that
    they can be incorporated into the canonical version.  This is,
    however, not a binding provision of this license.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
    IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
    ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
    CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
    WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/************************************************************************/

// RtAudio: Version 6.0.1

#include "RtAudio.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <cmath>
#include <algorithm>
#include <codecvt>
#include <locale>

#if defined(_WIN32)
#include <windows.h>
#endif

// Static variable definitions.
const unsigned int RtApi::MAX_SAMPLE_RATES = 14;
const unsigned int RtApi::SAMPLE_RATES[] = {
  4000, 5512, 8000, 9600, 11025, 16000, 22050,
  32000, 44100, 48000, 88200, 96000, 176400, 192000
};

template<typename T> inline
std::string convertCharPointerToStdString(const T *text);

template<> inline
std::string convertCharPointerToStdString(const char *text)
{
  return text;
}

template<> inline
std::string convertCharPointerToStdString(const wchar_t *text)
{
  return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>{}.to_bytes(text);
}

#if defined(_MSC_VER)
  #define MUTEX_INITIALIZE(A) InitializeCriticalSection(A)
  #define MUTEX_DESTROY(A)    DeleteCriticalSection(A)
  #define MUTEX_LOCK(A)       EnterCriticalSection(A)
  #define MUTEX_UNLOCK(A)     LeaveCriticalSection(A)
#else
  #define MUTEX_INITIALIZE(A) pthread_mutex_init(A, NULL)
  #define MUTEX_DESTROY(A)    pthread_mutex_destroy(A)
  #define MUTEX_LOCK(A)       pthread_mutex_lock(A)
  #define MUTEX_UNLOCK(A)     pthread_mutex_unlock(A)
#endif

// *************************************************** //
//
// RtApi subclass prototypes.
//
// *************************************************** //

#if defined(__MACOSX_CORE__)

#include <CoreAudio/AudioHardware.h>

class RtApiCore: public RtApi
{
public:

  RtApiCore();
  ~RtApiCore();
  RtAudio::Api getCurrentApi( void ) override { return RtAudio::MACOSX_CORE; }
  unsigned int getDefaultOutputDevice( void ) override;
  unsigned int getDefaultInputDevice( void ) override;
  void closeStream( void ) override;
  RtAudioErrorType startStream( void ) override;
  RtAudioErrorType stopStream( void ) override;
  RtAudioErrorType abortStream( void ) override;

  // This function is intended for internal use only.  It must be
  // public because it is called by an internal callback handler,
  // which is not a member of RtAudio.  External use of this function
  // will most likely produce highly undesirable results!
  bool callbackEvent( AudioDeviceID deviceId,
                      const AudioBufferList *inBufferList,
                      const AudioBufferList *outBufferList );

 private:
  void probeDevices( void ) override;
  bool probeDeviceInfo( AudioDeviceID id, RtAudio::DeviceInfo &info );
  bool probeDeviceOpen( unsigned int deviceId, StreamMode mode, unsigned int channels, 
                        unsigned int firstChannel, unsigned int sampleRate,
                        RtAudioFormat format, unsigned int *bufferSize,
                        RtAudio::StreamOptions *options ) override;
  static const char* getErrorCode( OSStatus code );
  std::vector< AudioDeviceID > deviceIds_;
};

#endif

#if defined(__UNIX_JACK__)

#include <jack/jack.h>

class RtApiJack: public RtApi
{
public:

  RtApiJack();
  ~RtApiJack();
  RtAudio::Api getCurrentApi( void ) override { return RtAudio::UNIX_JACK; }
  void closeStream( void ) override;
  RtAudioErrorType startStream( void ) override;
  RtAudioErrorType stopStream( void ) override;
  RtAudioErrorType abortStream( void ) override;

  // This function is intended for internal use only.  It must be
  // public because it is called by the internal callback handler,
  // which is not a member of RtAudio.  External use of this function
  // will most likely produce highly undesirable results!
  bool callbackEvent( unsigned long nframes );

  private:
  void probeDevices( void ) override;
  bool probeDeviceInfo( RtAudio::DeviceInfo &info, jack_client_t *client );
  bool probeDeviceOpen( unsigned int deviceId, StreamMode mode, unsigned int channels, 
                        unsigned int firstChannel, unsigned int sampleRate,
                        RtAudioFormat format, unsigned int *bufferSize,
                        RtAudio::StreamOptions *options ) override;

  bool shouldAutoconnect_;
};

#endif

#if defined(__WINDOWS_ASIO__)

class RtApiAsio: public RtApi
{
public:

  RtApiAsio();
  ~RtApiAsio();
  RtAudio::Api getCurrentApi( void ) override { return RtAudio::WINDOWS_ASIO; }
  void closeStream( void ) override;
  RtAudioErrorType startStream( void ) override;
  RtAudioErrorType stopStream( void ) override;
  RtAudioErrorType abortStream( void ) override;

  // This function is intended for internal use only.  It must be
  // public because it is called by the internal callback handler,
  // which is not a member of RtAudio.  External use of this function
  // will most likely produce highly undesirable results!
  bool callbackEvent( long bufferIndex );

  private:

  bool coInitialized_;
  void probeDevices( void ) override;
  bool probeDeviceInfo( RtAudio::DeviceInfo &info );
  bool probeDeviceOpen( unsigned int device, StreamMode mode, unsigned int channels, 
                        unsigned int firstChannel, unsigned int sampleRate,
                        RtAudioFormat format, unsigned int *bufferSize,
                        RtAudio::StreamOptions *options ) override;
};

#endif

#if defined(__WINDOWS_DS__)

class RtApiDs: public RtApi
{
public:

  RtApiDs();
  ~RtApiDs();
  RtAudio::Api getCurrentApi( void ) override { return RtAudio::WINDOWS_DS; }
  void closeStream( void ) override;
  RtAudioErrorType startStream( void ) override;
  RtAudioErrorType stopStream( void ) override;
  RtAudioErrorType abortStream( void ) override;

  // This function is intended for internal use only.  It must be
  // public because it is called by the internal callback handler,
  // which is not a member of RtAudio.  External use of this function
  // will most likely produce highly undesirable results!
  void callbackEvent( void );

  private:
  
  bool coInitialized_;
  bool buffersRolling;
  long duplexPrerollBytes;
  std::vector<struct DsDevice> dsDevices_;
  
  void probeDevices( void ) override;
  bool probeDeviceInfo( RtAudio::DeviceInfo &info, DsDevice &dsDevice );
  bool probeDeviceOpen( unsigned int deviceId, StreamMode mode, unsigned int channels, 
                        unsigned int firstChannel, unsigned int sampleRate,
                        RtAudioFormat format, unsigned int *bufferSize,
                        RtAudio::StreamOptions *options ) override;
};

#endif

#if defined(__WINDOWS_WASAPI__)

struct IMMDeviceEnumerator;

class RtApiWasapi : public RtApi
{
public:
  RtApiWasapi();
  virtual ~RtApiWasapi();
  RtAudio::Api getCurrentApi( void ) override { return RtAudio::WINDOWS_WASAPI; }
  unsigned int getDefaultOutputDevice( void ) override;
  unsigned int getDefaultInputDevice( void ) override;
  void closeStream( void ) override;
  RtAudioErrorType startStream( void ) override;
  RtAudioErrorType stopStream( void ) override;
  RtAudioErrorType abortStream( void ) override;

private:
  bool coInitialized_;
  IMMDeviceEnumerator* deviceEnumerator_;
  std::vector< std::pair< std::string, bool> > deviceIds_;

  void probeDevices( void ) override;
  bool probeDeviceInfo( RtAudio::DeviceInfo &info, LPWSTR deviceId, bool isCaptureDevice );
  bool probeDeviceOpen( unsigned int deviceId, StreamMode mode, unsigned int channels,
                        unsigned int firstChannel, unsigned int sampleRate,
                        RtAudioFormat format, unsigned int* bufferSize,
                        RtAudio::StreamOptions* options ) override;

  static DWORD WINAPI runWasapiThread( void* wasapiPtr );
  static DWORD WINAPI stopWasapiThread( void* wasapiPtr );
  static DWORD WINAPI abortWasapiThread( void* wasapiPtr );
  void wasapiThread();
};

#endif

#if defined(__LINUX_ALSA__)

class RtApiAlsa: public RtApi
{
public:

  RtApiAlsa();
  ~RtApiAlsa();
  RtAudio::Api getCurrentApi() override { return RtAudio::LINUX_ALSA; }
  void closeStream( void ) override;
  RtAudioErrorType startStream( void ) override;
  RtAudioErrorType stopStream( void ) override;
  RtAudioErrorType abortStream( void ) override;

  // This function is intended for internal use only.  It must be
  // public because it is called by the internal callback handler,
  // which is not a member of RtAudio.  External use of this function
  // will most likely produce highly undesirable results!
  void callbackEvent( void );

  private:
  std::vector<std::pair<std::string, unsigned int>> deviceIdPairs_;
  
  void probeDevices( void ) override;
  bool probeDeviceInfo( RtAudio::DeviceInfo &info, std::string name );
  bool probeDeviceOpen( unsigned int deviceId, StreamMode mode, unsigned int channels, 
                        unsigned int firstChannel, unsigned int sampleRate,
                        RtAudioFormat format, unsigned int *bufferSize,
                        RtAudio::StreamOptions *options ) override;
};

#endif

#if defined(__LINUX_PULSE__)

#include <pulse/pulseaudio.h>

class RtApiPulse: public RtApi
{
public:
  ~RtApiPulse();
  RtAudio::Api getCurrentApi() override { return RtAudio::LINUX_PULSE; }
  void closeStream( void ) override;
  RtAudioErrorType startStream( void ) override;
  RtAudioErrorType stopStream( void ) override;
  RtAudioErrorType abortStream( void ) override;

  // This function is intended for internal use only.  It must be
  // public because it is called by the internal callback handler,
  // which is not a member of RtAudio.  External use of this function
  // will most likely produce highly undesirable results!
  void callbackEvent( void );

  struct PaDeviceInfo {
    std::string sinkName;
    std::string sourceName;
  };

 private:
  std::vector< PaDeviceInfo > paDeviceList_;

  void probeDevices( void ) override;
  bool probeDeviceOpen( unsigned int deviceId, StreamMode mode, unsigned int channels,
                        unsigned int firstChannel, unsigned int sampleRate,
                        RtAudioFormat format, unsigned int *bufferSize,
                        RtAudio::StreamOptions *options ) override;
};

#endif

#if defined(__LINUX_OSS__)

#include <sys/soundcard.h>

class RtApiOss: public RtApi
{
public:

  RtApiOss();
  ~RtApiOss();
  RtAudio::Api getCurrentApi() override { return RtAudio::LINUX_OSS; }
  void closeStream( void ) override;
  RtAudioErrorType startStream( void ) override;
  RtAudioErrorType stopStream( void ) override;
  RtAudioErrorType abortStream( void ) override;

  // This function is intended for internal use only.  It must be
  // public because it is called by the internal callback handler,
  // which is not a member of RtAudio.  External use of this function
  // will most likely produce highly undesirable results!
  void callbackEvent( void );

  private:

  void probeDevices( void ) override;
  bool probeDeviceInfo( RtAudio::DeviceInfo &info, oss_audioinfo &ainfo );
  bool probeDeviceOpen( unsigned int deviceId, StreamMode mode, unsigned int channels, 
                        unsigned int firstChannel, unsigned int sampleRate,
                        RtAudioFormat format, unsigned int *bufferSize,
                        RtAudio::StreamOptions *options ) override;
};

#endif

#if defined(__RTAUDIO_DUMMY__)

class RtApiDummy: public RtApi
{
public:

  RtApiDummy() { errorText_ = "RtApiDummy: This class provides no functionality."; error( RTAUDIO_WARNING ); }
  RtAudio::Api getCurrentApi( void ) override { return RtAudio::RTAUDIO_DUMMY; }
  void closeStream( void ) override {}
  RtAudioErrorType startStream( void ) override { return RTAUDIO_NO_ERROR; }
  RtAudioErrorType stopStream( void ) override { return RTAUDIO_NO_ERROR; }
  RtAudioErrorType abortStream( void ) override { return RTAUDIO_NO_ERROR; }

  private:

  bool probeDeviceOpen( unsigned int /*deviceId*/, StreamMode /*mode*/, unsigned int /*channels*/, 
                        unsigned int /*firstChannel*/, unsigned int /*sampleRate*/,
                        RtAudioFormat /*format*/, unsigned int * /*bufferSize*/,
                        RtAudio::StreamOptions * /*options*/ ) override { return false; }
};

#endif

// *************************************************** //
//
// RtAudio definitions.
//
// *************************************************** //

std::string RtAudio :: getVersion( void )
{
  return RTAUDIO_VERSION;
}

// Define API names and display names.
// Must be in same order as API enum.
extern "C" {
const char* rtaudio_api_names[][2] = {
  { "unspecified" , "Unknown" },
  { "core"        , "CoreAudio" },
  { "alsa"        , "ALSA" },
  { "jack"        , "Jack" },
  { "pulse"       , "Pulse" },
  { "oss"         , "OpenSoundSystem" },
  { "asio"        , "ASIO" },
  { "wasapi"      , "WASAPI" },
  { "ds"          , "DirectSound" },
  { "dummy"       , "Dummy" },
};

const unsigned int rtaudio_num_api_names = 
  sizeof(rtaudio_api_names)/sizeof(rtaudio_api_names[0]);

// The order here will control the order of RtAudio's API search in
// the constructor.
extern "C" const RtAudio::Api rtaudio_compiled_apis[] = {
#if defined(__MACOSX_CORE__)
  RtAudio::MACOSX_CORE,
#endif
#if defined(__LINUX_ALSA__)
  RtAudio::LINUX_ALSA,
#endif
#if defined(__UNIX_JACK__)
  RtAudio::UNIX_JACK,
#endif
#if defined(__LINUX_PULSE__)
  RtAudio::LINUX_PULSE,
#endif
#if defined(__LINUX_OSS__)
  RtAudio::LINUX_OSS,
#endif
#if defined(__WINDOWS_ASIO__)
  RtAudio::WINDOWS_ASIO,
#endif
#if defined(__WINDOWS_WASAPI__)
  RtAudio::WINDOWS_WASAPI,
#endif
#if defined(__WINDOWS_DS__)
  RtAudio::WINDOWS_DS,
#endif
#if defined(__RTAUDIO_DUMMY__)
  RtAudio::RTAUDIO_DUMMY,
#endif
  RtAudio::UNSPECIFIED,
};

extern "C" const unsigned int rtaudio_num_compiled_apis =
  sizeof(rtaudio_compiled_apis)/sizeof(rtaudio_compiled_apis[0])-1;
}

// This is a compile-time check that rtaudio_num_api_names == RtAudio::NUM_APIS.
// If the build breaks here, check that they match.
template<bool b> class StaticAssert { private: StaticAssert() {} };
template<> class StaticAssert<true>{ public: StaticAssert() {} };
class StaticAssertions { StaticAssertions() {
  StaticAssert<rtaudio_num_api_names == RtAudio::NUM_APIS>();
}};

void RtAudio :: getCompiledApi( std::vector<RtAudio::Api> &apis )
{
  apis = std::vector<RtAudio::Api>(rtaudio_compiled_apis,
                                   rtaudio_compiled_apis + rtaudio_num_compiled_apis);
}

std::string RtAudio :: getApiName( RtAudio::Api api )
{
  if (api < 0 || api >= RtAudio::NUM_APIS)
    return "";
  return rtaudio_api_names[api][0];
}

std::string RtAudio :: getApiDisplayName( RtAudio::Api api )
{
  if (api < 0 || api >= RtAudio::NUM_APIS)
    return "Unknown";
  return rtaudio_api_names[api][1];
}

RtAudio::Api RtAudio :: getCompiledApiByName( const std::string &name )
{
  unsigned int i=0;
  for (i = 0; i < rtaudio_num_compiled_apis; ++i)
    if (name == rtaudio_api_names[rtaudio_compiled_apis[i]][0])
      return rtaudio_compiled_apis[i];
  return RtAudio::UNSPECIFIED;
}

RtAudio::Api RtAudio :: getCompiledApiByDisplayName( const std::string &name )
{
  unsigned int i=0;
  for (i = 0; i < rtaudio_num_compiled_apis; ++i)
    if (name == rtaudio_api_names[rtaudio_compiled_apis[i]][1])
      return rtaudio_compiled_apis[i];
  return RtAudio::UNSPECIFIED;
}

void RtAudio :: openRtApi( RtAudio::Api api )
{
  if ( rtapi_ )
    delete rtapi_;
  rtapi_ = 0;

#if defined(__UNIX_JACK__)
  if ( api == UNIX_JACK )
    rtapi_ = new RtApiJack();
#endif
#if defined(__LINUX_ALSA__)
  if ( api == LINUX_ALSA )
    rtapi_ = new RtApiAlsa();
#endif
#if defined(__LINUX_PULSE__)
  if ( api == LINUX_PULSE )
    rtapi_ = new RtApiPulse();
#endif
#if defined(__LINUX_OSS__)
  if ( api == LINUX_OSS )
    rtapi_ = new RtApiOss();
#endif
#if defined(__WINDOWS_ASIO__)
  if ( api == WINDOWS_ASIO )
    rtapi_ = new RtApiAsio();
#endif
#if defined(__WINDOWS_WASAPI__)
  if ( api == WINDOWS_WASAPI )
    rtapi_ = new RtApiWasapi();
#endif
#if defined(__WINDOWS_DS__)
  if ( api == WINDOWS_DS )
    rtapi_ = new RtApiDs();
#endif
#if defined(__MACOSX_CORE__)
  if ( api == MACOSX_CORE )
    rtapi_ = new RtApiCore();
#endif
#if defined(__RTAUDIO_DUMMY__)
  if ( api == RTAUDIO_DUMMY )
    rtapi_ = new RtApiDummy();
#endif
}

RtAudio :: RtAudio( RtAudio::Api api, RtAudioErrorCallback&& errorCallback )
{
  rtapi_ = 0;

  std::string errorMessage;
  if ( api != UNSPECIFIED ) {
    // Attempt to open the specified API.
    openRtApi( api );

    if ( rtapi_ ) {
      if ( errorCallback ) rtapi_->setErrorCallback( errorCallback );
      return;
    }

    // No compiled support for specified API value.  Issue a warning
    // and continue as if no API was specified.
    errorMessage = "RtAudio: no compiled support for specified API argument!";
    if ( errorCallback )
      errorCallback( RTAUDIO_INVALID_USE, errorMessage );
    else
      std::cerr << '\n' << errorMessage << '\n' << std::endl;
  }

  // Iterate through the compiled APIs and return as soon as we find
  // one with at least one device or we reach the end of the list.
  std::vector< RtAudio::Api > apis;
  getCompiledApi( apis );
  for ( unsigned int i=0; i<apis.size(); i++ ) {
    openRtApi( apis[i] );
    if ( rtapi_ && (rtapi_->getDeviceNames()).size() > 0 )
      break;
  }

  if ( rtapi_ ) {
    if ( errorCallback ) rtapi_->setErrorCallback( errorCallback );
    return;
  }

  // It should not be possible to get here because the preprocessor
  // definition __RTAUDIO_DUMMY__ is automatically defined in RtAudio.h
  // if no API-specific definitions are passed to the compiler. But just
  // in case something weird happens, issue an error message and abort.
  errorMessage = "RtAudio: no compiled API support found ... critical error!";
  if ( errorCallback )
    errorCallback( RTAUDIO_INVALID_USE, errorMessage );
  else
    std::cerr << '\n' << errorMessage << '\n' << std::endl;
  abort();
}

RtAudio :: ~RtAudio()
{
  if ( rtapi_ )
    delete rtapi_;
}

RtAudioErrorType RtAudio :: openStream( RtAudio::StreamParameters *outputParameters,
                                        RtAudio::StreamParameters *inputParameters,
                                        RtAudioFormat format, unsigned int sampleRate,
                                        unsigned int *bufferFrames,
                                        RtAudioCallback callback, void *userData,
                                        RtAudio::StreamOptions *options )
{
  return rtapi_->openStream( outputParameters, inputParameters, format,
                             sampleRate, bufferFrames, callback,
                             userData, options );
}

// *************************************************** //
//
// Public RtApi definitions (see end of file for
// private or protected utility functions).
//
// *************************************************** //

RtApi :: RtApi()
{
  clearStreamInfo();
  MUTEX_INITIALIZE( &stream_.mutex );
  errorCallback_ = 0;
  showWarnings_ = true;
  currentDeviceId_ = 129;
}

RtApi :: ~RtApi()
{
  MUTEX_DESTROY( &stream_.mutex );
}

RtAudioErrorType RtApi :: openStream( RtAudio::StreamParameters *oParams,
                                      RtAudio::StreamParameters *iParams,
                                      RtAudioFormat format, unsigned int sampleRate,
                                      unsigned int *bufferFrames,
                                      RtAudioCallback callback, void *userData,
                                      RtAudio::StreamOptions *options )
{
  if ( stream_.state != STREAM_CLOSED ) {
    errorText_ = "RtApi::openStream: a stream is already open!";
    return error( RTAUDIO_INVALID_USE );
  }

  // Clear stream information potentially left from a previously open stream.
  clearStreamInfo();

  if ( oParams && oParams->nChannels < 1 ) {
    errorText_ = "RtApi::openStream: a non-NULL output StreamParameters structure cannot have an nChannels value less than one.";
    return error( RTAUDIO_INVALID_PARAMETER );
  }

  if ( iParams && iParams->nChannels < 1 ) {
    errorText_ = "RtApi::openStream: a non-NULL input StreamParameters structure cannot have an nChannels value less than one.";
    return error( RTAUDIO_INVALID_PARAMETER );
  }

  if ( oParams == NULL && iParams == NULL ) {
    errorText_ = "RtApi::openStream: input and output StreamParameters structures are both NULL!";
    return error( RTAUDIO_INVALID_PARAMETER );
  }

  if ( formatBytes(format) == 0 ) {
    errorText_ = "RtApi::openStream: 'format' parameter value is undefined.";
    return error( RTAUDIO_INVALID_PARAMETER );
  }

  // Scan devices if none currently listed.
  if ( deviceList_.size() == 0 ) probeDevices();
  
  unsigned int m, oChannels = 0;
  if ( oParams ) {
    oChannels = oParams->nChannels;
    // Verify that the oParams->deviceId is found in our list
    for ( m=0; m<deviceList_.size(); m++ ) {
      if ( deviceList_[m].ID == oParams->deviceId ) break;
    }
    if ( m == deviceList_.size() ) {
      errorText_ = "RtApi::openStream: output device ID is invalid.";
      return error( RTAUDIO_INVALID_PARAMETER );
    }
  }

  unsigned int iChannels = 0;
  if ( iParams ) {
    iChannels = iParams->nChannels;
    for ( m=0; m<deviceList_.size(); m++ ) {
      if ( deviceList_[m].ID == iParams->deviceId ) break;
    }
    if ( m == deviceList_.size() ) {
      errorText_ = "RtApi::openStream: input device ID is invalid.";
      return error( RTAUDIO_INVALID_PARAMETER );
    }
  }

  bool result;

  if ( oChannels > 0 ) {

    result = probeDeviceOpen( oParams->deviceId, OUTPUT, oChannels, oParams->firstChannel,
                              sampleRate, format, bufferFrames, options );
    if ( result == false )
      return error( RTAUDIO_SYSTEM_ERROR );
  }

  if ( iChannels > 0 ) {

    result = probeDeviceOpen( iParams->deviceId, INPUT, iChannels, iParams->firstChannel,
                              sampleRate, format, bufferFrames, options );
    if ( result == false )
      return error( RTAUDIO_SYSTEM_ERROR );
  }

  stream_.callbackInfo.callback = (void *) callback;
  stream_.callbackInfo.userData = userData;

  if ( options ) options->numberOfBuffers = stream_.nBuffers;
  stream_.state = STREAM_STOPPED;
  return RTAUDIO_NO_ERROR;
}

void RtApi :: probeDevices( void )
{
  // This function MUST be implemented in all subclasses! Within each
  // API, this function will be used to:
  // - enumerate the devices and fill or update our
  //   std::vector< RtAudio::DeviceInfo> deviceList_ class variable
  // - store corresponding (usually API-specific) identifiers that
  //   are needed to open each device
  // - make sure that the default devices are properly identified
  //   within the deviceList_ (unless API-specific functions are
  //   available for this purpose).
  //
  // The function should not reprobe devices that have already been
  // found. The function must properly handle devices that are removed
  // or added.
  //
  // Ideally, we would also configure callback functions to be invoked
  // when devices are added or removed (which could be used to inform
  // clients about changes). However, none of the APIs currently
  // support notification of _new_ devices and I don't see the
  // usefulness of having this work only for device removal.
  return;
}

unsigned int RtApi :: getDeviceCount( void )
{
  probeDevices();
  return (unsigned int)deviceList_.size();
}

std::vector<unsigned int> RtApi :: getDeviceIds( void )
{
  probeDevices();

  // Copy device IDs into output vector.
  std::vector<unsigned int> deviceIds;
  for ( unsigned int m=0; m<deviceList_.size(); m++ )
    deviceIds.push_back( deviceList_[m].ID );

  return deviceIds;
}

std::vector<std::string> RtApi :: getDeviceNames( void )
{
  probeDevices();

  // Copy device names into output vector.
  std::vector<std::string> deviceNames;
  for ( unsigned int m=0; m<deviceList_.size(); m++ )
    deviceNames.push_back( deviceList_[m].name );

  return deviceNames;
}

unsigned int RtApi :: getDefaultInputDevice( void )
{
  // Should be reimplemented in subclasses if necessary.
  if ( deviceList_.size() == 0 ) probeDevices();
  for ( unsigned int i = 0; i < deviceList_.size(); i++ ) {
    if ( deviceList_[i].isDefaultInput )
      return deviceList_[i].ID;
  }

  // If not found, find the first device with input channels, set it
  // as the default, and return the ID.
  for ( unsigned int i = 0; i < deviceList_.size(); i++ ) {
    if ( deviceList_[i].inputChannels > 0 ) {
      deviceList_[i].isDefaultInput = true;
      return deviceList_[i].ID;
    }
  }

  return 0;
}

unsigned int RtApi :: getDefaultOutputDevice( void )
{
  // Should be reimplemented in subclasses if necessary.
  if ( deviceList_.size() == 0 ) probeDevices();
  for ( unsigned int i = 0; i < deviceList_.size(); i++ ) {
    if ( deviceList_[i].isDefaultOutput )
      return deviceList_[i].ID;
  }

  // If not found, find the first device with output channels, set it
  // as the default, and return the ID.
  for ( unsigned int i = 0; i < deviceList_.size(); i++ ) {
    if ( deviceList_[i].outputChannels > 0 ) {
      deviceList_[i].isDefaultOutput = true;
      return deviceList_[i].ID;
    }
  }

  return 0;
}

RtAudio::DeviceInfo RtApi :: getDeviceInfo( unsigned int deviceId )
{
  if ( deviceList_.size() == 0 ) probeDevices();
  for ( unsigned int m=0; m<deviceList_.size(); m++ ) {
    if ( deviceList_[m].ID == deviceId )
      return deviceList_[m];
  }

  errorText_ = "RtApi::getDeviceInfo: deviceId argument not found.";
  error( RTAUDIO_INVALID_PARAMETER );
  return RtAudio::DeviceInfo();
}

void RtApi :: closeStream( void )
{
  // MUST be implemented in subclasses!
  return;
}

bool RtApi :: probeDeviceOpen( unsigned int /*deviceId*/, StreamMode /*mode*/, unsigned int /*channels*/,
                               unsigned int /*firstChannel*/, unsigned int /*sampleRate*/,
                               RtAudioFormat /*format*/, unsigned int * /*bufferSize*/,
                               RtAudio::StreamOptions * /*options*/ )
{
  // MUST be implemented in subclasses!
  return FAILURE;
}

void RtApi :: tickStreamTime( void )
{
  // Subclasses that do not provide their own implementation of
  // getStreamTime should call this function once per buffer I/O to
  // provide basic stream time support.

  stream_.streamTime += ( stream_.bufferSize * 1.0 / stream_.sampleRate );

  /*
#if defined( HAVE_GETTIMEOFDAY )
  gettimeofday( &stream_.lastTickTimestamp, NULL );
#endif
  */
}

long RtApi :: getStreamLatency( void )
{
  long totalLatency = 0;
  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX )
    totalLatency = stream_.latency[0];
  if ( stream_.mode == INPUT || stream_.mode == DUPLEX )
    totalLatency += stream_.latency[1];

  return totalLatency;
}

/*
double RtApi :: getStreamTime( void )
{
#if defined( HAVE_GETTIMEOFDAY )
  // Return a very accurate estimate of the stream time by
  // adding in the elapsed time since the last tick.
  struct timeval then;
  struct timeval now;

  if ( stream_.state != STREAM_RUNNING || stream_.streamTime == 0.0 )
    return stream_.streamTime;

  gettimeofday( &now, NULL );
  then = stream_.lastTickTimestamp;
  return stream_.streamTime +
    ((now.tv_sec + 0.000001 * now.tv_usec) -
     (then.tv_sec + 0.000001 * then.tv_usec));     
#else
  return stream_.streamTime;
  #endif
}
*/

void RtApi :: setStreamTime( double time )
{
  if ( time >= 0.0 )
    stream_.streamTime = time;
  /*
#if defined( HAVE_GETTIMEOFDAY )
  gettimeofday( &stream_.lastTickTimestamp, NULL );
#endif
  */
}

unsigned int RtApi :: getStreamSampleRate( void )
{
  if ( isStreamOpen() ) return stream_.sampleRate;
  else return 0;
}


// *************************************************** //
//
// OS/API-specific methods.
//
// *************************************************** //

#if defined(__MACOSX_CORE__)

#include <unistd.h>

// The OS X CoreAudio API is designed to use a separate callback
// procedure for each of its audio devices.  A single RtAudio duplex
// stream using two different devices is supported here, though it
// cannot be guaranteed to always behave correctly because we cannot
// synchronize these two callbacks.
//
// A property listener is installed for over/underrun information.
// However, no functionality is currently provided to allow property
// listeners to trigger user handlers because it is unclear what could
// be done if a critical stream parameter (buffer size, sample rate,
// device disconnect) notification arrived.  The listeners entail
// quite a bit of extra code and most likely, a user program wouldn't
// be prepared for the result anyway.  However, we do provide a flag
// to the client callback function to inform of an over/underrun.

// A structure to hold various information related to the CoreAudio API
// implementation.
struct CoreHandle {
  AudioDeviceID id[2];    // device ids
#if defined( MAC_OS_X_VERSION_10_5 ) && ( MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5 )
  AudioDeviceIOProcID procId[2];
#endif
  UInt32 iStream[2];      // device stream index (or first if using multiple)
  UInt32 nStreams[2];     // number of streams to use
  bool xrun[2];
  char *deviceBuffer;
  pthread_cond_t condition;
  int drainCounter;       // Tracks callback counts when draining
  bool internalDrain;     // Indicates if stop is initiated from callback or not.
  bool xrunListenerAdded[2];
  bool disconnectListenerAdded[2];

  CoreHandle()
    :deviceBuffer(0), drainCounter(0), internalDrain(false) { nStreams[0] = 1; nStreams[1] = 1; id[0] = 0; id[1] = 0; procId[0] = 0; procId[1] = 0; xrun[0] = false; xrun[1] = false; xrunListenerAdded[0] = false; xrunListenerAdded[1] = false; disconnectListenerAdded[0] = false; disconnectListenerAdded[1] = false; }
};

#if defined( MAC_OS_VERSION_12_0 ) && ( MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_VERSION_12_0 )
  #define KAUDIOOBJECTPROPERTYELEMENT kAudioObjectPropertyElementMain
#else
  #define KAUDIOOBJECTPROPERTYELEMENT kAudioObjectPropertyElementMaster // deprecated with macOS 12
#endif

RtApiCore:: RtApiCore()
{
#if defined( AVAILABLE_MAC_OS_X_VERSION_10_6_AND_LATER )
  // This is a largely undocumented but absolutely necessary
  // requirement starting with OS-X 10.6.  If not called, queries and
  // updates to various audio device properties are not handled
  // correctly.
  CFRunLoopRef theRunLoop = NULL;
  AudioObjectPropertyAddress property = { kAudioHardwarePropertyRunLoop,
                                          kAudioObjectPropertyScopeGlobal,
                                          KAUDIOOBJECTPROPERTYELEMENT };
  OSStatus result = AudioObjectSetPropertyData( kAudioObjectSystemObject, &property, 0, NULL, sizeof(CFRunLoopRef), &theRunLoop);
  if ( result != noErr ) {
    errorText_ = "RtApiCore::RtApiCore: error setting run loop property!";
    error( RTAUDIO_SYSTEM_ERROR );
  }
#endif
}

RtApiCore :: ~RtApiCore()
{
  // The subclass destructor gets called before the base class
  // destructor, so close an existing stream before deallocating
  // apiDeviceId memory.
  if ( stream_.state != STREAM_CLOSED ) closeStream();
}

unsigned int RtApiCore :: getDefaultOutputDevice( void )
{
  AudioDeviceID id;
  UInt32 dataSize = sizeof( AudioDeviceID );
  AudioObjectPropertyAddress property = { kAudioHardwarePropertyDefaultOutputDevice, kAudioObjectPropertyScopeGlobal, KAUDIOOBJECTPROPERTYELEMENT };
  OSStatus result = AudioObjectGetPropertyData( kAudioObjectSystemObject, &property, 0, NULL, &dataSize, &id );
  if ( result != noErr ) {
    errorText_ = "RtApiCore::getDefaultOutputDevice: OS-X system error getting device.";
    error( RTAUDIO_SYSTEM_ERROR );
    return 0;
  }

  for ( unsigned int m=0; m<deviceIds_.size(); m++ ) {
    if ( deviceIds_[m] == id ) {
      if ( deviceList_[m].isDefaultOutput == false ) {
        deviceList_[m].isDefaultOutput = true;
        for ( unsigned int j=m+1; j<deviceIds_.size(); j++ ) {
          // make sure any remaining devices are not listed as the default
          deviceList_[j].isDefaultOutput = false;
        }
      }
      return deviceList_[m].ID;
    }
    deviceList_[m].isDefaultOutput = false;
  }

  // If not found above, then do system probe of devices and try again.
  probeDevices();
  for ( unsigned int m=0; m<deviceIds_.size(); m++ ) {
    if ( deviceIds_[m] == id ) return deviceList_[m].ID;
  }
  return 0;
}

unsigned int RtApiCore :: getDefaultInputDevice( void )
{
  AudioDeviceID id;
  UInt32 dataSize = sizeof( AudioDeviceID );
  AudioObjectPropertyAddress property = { kAudioHardwarePropertyDefaultInputDevice, kAudioObjectPropertyScopeGlobal, KAUDIOOBJECTPROPERTYELEMENT };
  OSStatus result = AudioObjectGetPropertyData( kAudioObjectSystemObject, &property, 0, NULL, &dataSize, &id );
  if ( result != noErr ) {
    errorText_ = "RtApiCore::getDefaultInputDevice: OS-X system error getting device.";
    error( RTAUDIO_SYSTEM_ERROR );
    return 0;
  }

  for ( unsigned int m=0; m<deviceIds_.size(); m++ ) {
    if ( deviceIds_[m] == id ) {
      if ( deviceList_[m].isDefaultInput == false ) {
        deviceList_[m].isDefaultInput = true;
        for ( unsigned int j=m+1; j<deviceIds_.size(); j++ ) {
          // make sure any remaining devices are not listed as the default
          deviceList_[j].isDefaultInput = false;
        }
      }
      return deviceList_[m].ID;
    }
    deviceList_[m].isDefaultInput = false;
  }

  // If not found above, then do system probe of devices and try again.
  probeDevices();
  for ( unsigned int m=0; m<deviceIds_.size(); m++ ) {
    if ( deviceIds_[m] == id ) return deviceList_[m].ID;
  }
  return 0;
}

// If a device used in an open stream is disconnected, close the stream.
static OSStatus streamDisconnectListener( AudioObjectID /*id*/,
                                          UInt32 nAddresses,
                                          const AudioObjectPropertyAddress properties[],
                                          void* infoPointer )
{
  for ( UInt32 i=0; i<nAddresses; i++ ) {
    if ( properties[i].mSelector == kAudioDevicePropertyDeviceIsAlive ) {
      CallbackInfo *info = (CallbackInfo *) infoPointer;
      RtApiCore *object = (RtApiCore *) info->object;
      info->deviceDisconnected = true;
      object->closeStream();
      return kAudioHardwareUnspecifiedError;
    }
  }
  
  return kAudioHardwareNoError;
}

void RtApiCore :: probeDevices( void )
{
  // See list of required functionality in RtApi::probeDevices().
  
  // Find out how many audio devices there are.
  UInt32 dataSize;
  AudioObjectPropertyAddress property = { kAudioHardwarePropertyDevices, kAudioObjectPropertyScopeGlobal, KAUDIOOBJECTPROPERTYELEMENT };
  OSStatus result = AudioObjectGetPropertyDataSize( kAudioObjectSystemObject, &property, 0, NULL, &dataSize );
  if ( result != noErr ) {
    errorText_ = "RtApiCore::probeDevices: OS-X system error getting device info!";
    error( RTAUDIO_SYSTEM_ERROR );
    return;
  }

  unsigned int nDevices = dataSize / sizeof( AudioDeviceID );
  if ( nDevices == 0 ) {
    deviceList_.clear();
    deviceIds_.clear();
    return;
  }

  AudioDeviceID ids[ nDevices ];
  property.mSelector = kAudioHardwarePropertyDevices;
  result = AudioObjectGetPropertyData( kAudioObjectSystemObject, &property, 0, NULL, &dataSize, (void *) &ids );
  if ( result != noErr ) {
    errorText_ = "RtApiCore::probeDevices: OS-X system error getting device IDs.";
    error( RTAUDIO_SYSTEM_ERROR );
    return;
  }

  // Fill or update the deviceList_ and also save a corresponding list of Ids.
  for ( unsigned int n=0; n<nDevices; n++ ) {
    if ( std::find( deviceIds_.begin(), deviceIds_.end(), ids[n] ) != deviceIds_.end() ) {
      continue; // We already have this device.
    }
    else { // There is a new device to probe.
      RtAudio::DeviceInfo info;
      if ( probeDeviceInfo( ids[n], info ) == false ) continue; // ignore if probe fails
      deviceIds_.push_back( ids[n] );
      info.ID = currentDeviceId_++;  // arbitrary internal device ID
      deviceList_.push_back( info );
      // We could set a property listener here for each device to know
      // if it is removed. However, we cannot detect (AFAIK) when a new
      // device is plugged in. If we cannot detect BOTH cases, I'm not
      // going to bother with only the one.
    }
  }

  // Remove any devices left in the list that are no longer available.
  unsigned int m;
  for ( std::vector<AudioDeviceID>::iterator it=deviceIds_.begin(); it!=deviceIds_.end(); ) {
    for ( m=0; m<nDevices; m++ ) {
      if ( ids[m] == *it ) {
        ++it;
        break;
      }
    }
    if ( m == nDevices ) { // not found so remove it from our two lists
      it = deviceIds_.erase(it);
      deviceList_.erase( deviceList_.begin() + distance(deviceIds_.begin(), it ) );
    }
  }

  // Get default devices and set flags in deviceList_.
  AudioDeviceID defaultOutputId, defaultInputId;
  dataSize = sizeof( AudioDeviceID );
  property.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
  result = AudioObjectGetPropertyData( kAudioObjectSystemObject, &property, 0, NULL, &dataSize, &defaultOutputId );
  if ( result != noErr ) {
    errorText_ = "RtApiCore::probeDeviceInfo: OS-X system error getting default output device.";
    error( RTAUDIO_WARNING );
    defaultOutputId = 0;
  }

  property.mSelector = kAudioHardwarePropertyDefaultInputDevice;
  result = AudioObjectGetPropertyData( kAudioObjectSystemObject, &property, 0, NULL, &dataSize, &defaultInputId );
  if ( result != noErr ) {
    errorText_ = "RtApiCore::probeDeviceInfo: OS-X system error getting default input device.";
    error( RTAUDIO_WARNING );
    defaultInputId = 0;
  }

  for ( m=0; m<deviceList_.size(); m++ ) {
    if ( deviceIds_[m] == defaultOutputId )
      deviceList_[m].isDefaultOutput = true;
    else
      deviceList_[m].isDefaultOutput = false;
    if ( deviceIds_[m] == defaultInputId )
      deviceList_[m].isDefaultInput = true;
    else
      deviceList_[m].isDefaultInput = false;
  }
}

bool RtApiCore :: probeDeviceInfo( AudioDeviceID id, RtAudio::DeviceInfo& info )
{
  // Get the device name.
  info.name.erase();
  CFStringRef cfname;
  UInt32 dataSize = sizeof( CFStringRef );
  AudioObjectPropertyAddress property = { kAudioObjectPropertyManufacturer,
                                          kAudioObjectPropertyScopeGlobal,
                                          KAUDIOOBJECTPROPERTYELEMENT };
  OSStatus result = AudioObjectGetPropertyData( id, &property, 0, NULL, &dataSize, &cfname );
  if ( result != noErr ) {
    errorStream_ << "RtApiCore::probeDeviceInfo: system error (" << getErrorCode( result ) << ") getting device manufacturer.";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  long length = CFStringGetLength(cfname);
  char *mname = (char *)malloc(length * 3 + 1);
#if defined( UNICODE ) || defined( _UNICODE )
  CFStringGetCString(cfname, mname, length * 3 + 1, kCFStringEncodingUTF8);
#else
  CFStringGetCString(cfname, mname, length * 3 + 1, CFStringGetSystemEncoding());
#endif
  info.name.append( (const char *)mname, strlen(mname) );
  info.name.append( ": " );
  CFRelease( cfname );
  free(mname);

  property.mSelector = kAudioObjectPropertyName;
  result = AudioObjectGetPropertyData( id, &property, 0, NULL, &dataSize, &cfname );
  if ( result != noErr ) {
    errorStream_ << "RtApiCore::probeDeviceInfo: system error (" << getErrorCode( result ) << ") getting device name.";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  length = CFStringGetLength(cfname);
  char *name = (char *)malloc(length * 3 + 1);
#if defined( UNICODE ) || defined( _UNICODE )
  CFStringGetCString(cfname, name, length * 3 + 1, kCFStringEncodingUTF8);
#else
  CFStringGetCString(cfname, name, length * 3 + 1, CFStringGetSystemEncoding());
#endif
  info.name.append( (const char *)name, strlen(name) );
  CFRelease( cfname );
  free(name);

  // Get the output stream "configuration".
  AudioBufferList	*bufferList = nil;
  property.mSelector = kAudioDevicePropertyStreamConfiguration;
  property.mScope = kAudioDevicePropertyScopeOutput;
  dataSize = 0;
  result = AudioObjectGetPropertyDataSize( id, &property, 0, NULL, &dataSize );
  if ( result != noErr || dataSize == 0 ) {
    errorStream_ << "RtApiCore::probeDeviceInfo: system error (" << getErrorCode( result ) << ") getting output stream configuration info for device (" << info.name << ").";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  // Allocate the AudioBufferList.
  bufferList = (AudioBufferList *) malloc( dataSize );
  if ( bufferList == NULL ) {
    errorText_ = "RtApiCore::probeDeviceInfo: memory error allocating output AudioBufferList.";
    error( RTAUDIO_WARNING );
    return false;
  }

  result = AudioObjectGetPropertyData( id, &property, 0, NULL, &dataSize, bufferList );
  if ( result != noErr || dataSize == 0 ) {
    free( bufferList );
    errorStream_ << "RtApiCore::probeDeviceInfo: system error (" << getErrorCode( result ) << ") getting output stream configuration for device (" << info.name << ").";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  // Get output channel information.
  unsigned int i, nStreams = bufferList->mNumberBuffers;
  for ( i=0; i<nStreams; i++ )
    info.outputChannels += bufferList->mBuffers[i].mNumberChannels;
  free( bufferList );

  // Get the input stream "configuration".
  property.mScope = kAudioDevicePropertyScopeInput;
  result = AudioObjectGetPropertyDataSize( id, &property, 0, NULL, &dataSize );
  if ( result != noErr || dataSize == 0 ) {
    errorStream_ << "RtApiCore::probeDeviceInfo: system error (" << getErrorCode( result ) << ") getting input stream configuration info for device (" << info.name << ").";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  // Allocate the AudioBufferList.
  bufferList = (AudioBufferList *) malloc( dataSize );
  if ( bufferList == NULL ) {
    errorText_ = "RtApiCore::probeDeviceInfo: memory error allocating input AudioBufferList.";
    error( RTAUDIO_WARNING );
    return false;
  }

  result = AudioObjectGetPropertyData( id, &property, 0, NULL, &dataSize, bufferList );
  if (result != noErr || dataSize == 0) {
    free( bufferList );
    errorStream_ << "RtApiCore::probeDeviceInfo: system error (" << getErrorCode( result ) << ") getting input stream configuration for device (" << info.name << ").";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  // Get input channel information.
  nStreams = bufferList->mNumberBuffers;
  for ( i=0; i<nStreams; i++ )
    info.inputChannels += bufferList->mBuffers[i].mNumberChannels;
  free( bufferList );

  // If device opens for both playback and capture, we determine the channels.
  if ( info.outputChannels > 0 && info.inputChannels > 0 )
    info.duplexChannels = (info.outputChannels > info.inputChannels) ? info.inputChannels : info.outputChannels;

  // Probe the device sample rates.
  bool isInput = false;
  if ( info.outputChannels == 0 ) isInput = true;

  // Determine the supported sample rates.
  property.mSelector = kAudioDevicePropertyAvailableNominalSampleRates;
  if ( isInput == false ) property.mScope = kAudioDevicePropertyScopeOutput;
  result = AudioObjectGetPropertyDataSize( id, &property, 0, NULL, &dataSize );
  if ( result != kAudioHardwareNoError || dataSize == 0 ) {
    errorStream_ << "RtApiCore::probeDeviceInfo: system error (" << getErrorCode( result ) << ") getting sample rate info.";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  UInt32 nRanges = dataSize / sizeof( AudioValueRange );
  AudioValueRange rangeList[ nRanges ];
  result = AudioObjectGetPropertyData( id, &property, 0, NULL, &dataSize, &rangeList );
  if ( result != kAudioHardwareNoError ) {
    errorStream_ << "RtApiCore::probeDeviceInfo: system error (" << getErrorCode( result ) << ") getting sample rates.";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  // The sample rate reporting mechanism is a bit of a mystery.  It
  // seems that it can either return individual rates or a range of
  // rates.  I assume that if the min / max range values are the same,
  // then that represents a single supported rate and if the min / max
  // range values are different, the device supports an arbitrary
  // range of values (though there might be multiple ranges, so we'll
  // use the most conservative range).
  Float64 minimumRate = 1.0, maximumRate = 10000000000.0;
  bool haveValueRange = false;
  info.sampleRates.clear();
  for ( UInt32 i=0; i<nRanges; i++ ) {
    if ( rangeList[i].mMinimum == rangeList[i].mMaximum ) {
      unsigned int tmpSr = (unsigned int) rangeList[i].mMinimum;
      info.sampleRates.push_back( tmpSr );

      if ( !info.preferredSampleRate || ( tmpSr <= 48000 && tmpSr > info.preferredSampleRate ) )
        info.preferredSampleRate = tmpSr;

    } else {
      haveValueRange = true;
      if ( rangeList[i].mMinimum > minimumRate ) minimumRate = rangeList[i].mMinimum;
      if ( rangeList[i].mMaximum < maximumRate ) maximumRate = rangeList[i].mMaximum;
    }
  }

  if ( haveValueRange ) {
    for ( unsigned int k=0; k<MAX_SAMPLE_RATES; k++ ) {
      if ( SAMPLE_RATES[k] >= (unsigned int) minimumRate && SAMPLE_RATES[k] <= (unsigned int) maximumRate ) {
        info.sampleRates.push_back( SAMPLE_RATES[k] );

        if ( !info.preferredSampleRate || ( SAMPLE_RATES[k] <= 48000 && SAMPLE_RATES[k] > info.preferredSampleRate ) )
          info.preferredSampleRate = SAMPLE_RATES[k];
      }
    }
  }

  // Sort and remove any redundant values
  std::sort( info.sampleRates.begin(), info.sampleRates.end() );
  info.sampleRates.erase( unique( info.sampleRates.begin(), info.sampleRates.end() ), info.sampleRates.end() );

  if ( info.sampleRates.size() == 0 ) {
    errorStream_ << "RtApiCore::probeDeviceInfo: No supported sample rates found for device (" << info.name << ").";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  // Probe the currently configured sample rate
  Float64 nominalRate;
  dataSize = sizeof( Float64 );
  property.mSelector = kAudioDevicePropertyNominalSampleRate;
  result = AudioObjectGetPropertyData( id, &property, 0, NULL, &dataSize, &nominalRate );
  if ( result == noErr ) info.currentSampleRate = (unsigned int) nominalRate;
    
  // CoreAudio always uses 32-bit floating point data for PCM streams.
  // Thus, any other "physical" formats supported by the device are of
  // no interest to the client.
  info.nativeFormats = RTAUDIO_FLOAT32;

  return true;
}

static OSStatus callbackHandler( AudioDeviceID inDevice,
                                 const AudioTimeStamp* /*inNow*/,
                                 const AudioBufferList* inInputData,
                                 const AudioTimeStamp* /*inInputTime*/,
                                 AudioBufferList* outOutputData,
                                 const AudioTimeStamp* /*inOutputTime*/,
                                 void* infoPointer )
{
  CallbackInfo *info = (CallbackInfo *) infoPointer;
  if(info == NULL || info->object == NULL)
    return kAudioHardwareUnspecifiedError;

  RtApiCore *object = (RtApiCore *) info->object;
  if ( object->callbackEvent( inDevice, inInputData, outOutputData ) == false )
    return kAudioHardwareUnspecifiedError;
  else
    return kAudioHardwareNoError;
}

static OSStatus xrunListener( AudioObjectID /*inDevice*/,
                              UInt32 nAddresses,
                              const AudioObjectPropertyAddress properties[],
                              void* handlePointer )
{
  CoreHandle *handle = (CoreHandle *) handlePointer;
  for ( UInt32 i=0; i<nAddresses; i++ ) {
    if ( properties[i].mSelector == kAudioDeviceProcessorOverload ) {
      if ( properties[i].mScope == kAudioDevicePropertyScopeInput )
        handle->xrun[1] = true;
      else
        handle->xrun[0] = true;
    }
  }

  return kAudioHardwareNoError;
}

bool RtApiCore :: probeDeviceOpen( unsigned int deviceId, StreamMode mode, unsigned int channels,
                                   unsigned int firstChannel, unsigned int sampleRate,
                                   RtAudioFormat format, unsigned int *bufferSize,
                                   RtAudio::StreamOptions *options )
{
  AudioDeviceID id = 0;
  for ( unsigned int m=0; m<deviceList_.size(); m++ ) {
    if ( deviceList_[m].ID == deviceId ) {
      id = deviceIds_[m];
      break;
    }
  }

  if ( id == 0 ) {
    errorText_ = "RtApiCore::probeDeviceOpen: the device ID was not found!";
    return FAILURE;
  }

  AudioObjectPropertyAddress property = { kAudioHardwarePropertyDevices,
                                          kAudioDevicePropertyScopeOutput,
                                          KAUDIOOBJECTPROPERTYELEMENT };

  // Setup for stream mode.
  if ( mode == INPUT ) {
    property.mScope = kAudioDevicePropertyScopeInput;
  }

  // Get the stream "configuration".
  AudioBufferList	*bufferList = nil;
  UInt32 dataSize = 0;
  property.mSelector = kAudioDevicePropertyStreamConfiguration;
  OSStatus result = AudioObjectGetPropertyDataSize( id, &property, 0, NULL, &dataSize );
  if ( result != noErr || dataSize == 0 ) {
    errorStream_ << "RtApiCore::probeDeviceOpen: system error (" << getErrorCode( result ) << ") getting stream configuration info for device (" << deviceId << ").";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Allocate the AudioBufferList.
  bufferList = (AudioBufferList *) malloc( dataSize );
  if ( bufferList == NULL ) {
    errorText_ = "RtApiCore::probeDeviceOpen: memory error allocating AudioBufferList.";
    return FAILURE;
  }

  result = AudioObjectGetPropertyData( id, &property, 0, NULL, &dataSize, bufferList );
  if (result != noErr || dataSize == 0) {
    free( bufferList );
    errorStream_ << "RtApiCore::probeDeviceOpen: system error (" << getErrorCode( result ) << ") getting stream configuration for device (" << deviceId << ").";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Search for one or more streams that contain the desired number of
  // channels. CoreAudio devices can have an arbitrary number of
  // streams and each stream can have an arbitrary number of channels.
  // For each stream, a single buffer of interleaved samples is
  // provided.  RtAudio prefers the use of one stream of interleaved
  // data or multiple consecutive single-channel streams.  However, we
  // now support multiple consecutive multi-channel streams of
  // interleaved data as well.
  UInt32 iStream, offsetCounter = firstChannel;
  UInt32 nStreams = bufferList->mNumberBuffers;
  bool monoMode = false;
  bool foundStream = false;

  // First check that the device supports the requested number of
  // channels.
  UInt32 deviceChannels = 0;
  for ( iStream=0; iStream<nStreams; iStream++ )
    deviceChannels += bufferList->mBuffers[iStream].mNumberChannels;

  if ( deviceChannels < ( channels + firstChannel ) ) {
    free( bufferList );
    errorStream_ << "RtApiCore::probeDeviceOpen: the device (" << deviceId << ") does not support the requested channel count.";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Look for a single stream meeting our needs.
  UInt32 firstStream = 0, streamCount = 1, streamChannels = 0, channelOffset = 0;
  for ( iStream=0; iStream<nStreams; iStream++ ) {
    streamChannels = bufferList->mBuffers[iStream].mNumberChannels;
    if ( streamChannels >= channels + offsetCounter ) {
      firstStream = iStream;
      channelOffset = offsetCounter;
      foundStream = true;
      break;
    }
    if ( streamChannels > offsetCounter ) break;
    offsetCounter -= streamChannels;
  }

  // If we didn't find a single stream above, then we should be able
  // to meet the channel specification with multiple streams.
  if ( foundStream == false ) {
    monoMode = true;
    offsetCounter = firstChannel;
    for ( iStream=0; iStream<nStreams; iStream++ ) {
      streamChannels = bufferList->mBuffers[iStream].mNumberChannels;
      if ( streamChannels > offsetCounter ) break;
      offsetCounter -= streamChannels;
    }

    firstStream = iStream;
    channelOffset = offsetCounter;
    Int32 channelCounter = channels + offsetCounter - streamChannels;

    if ( streamChannels > 1 ) monoMode = false;
    while ( channelCounter > 0 ) {
      streamChannels = bufferList->mBuffers[++iStream].mNumberChannels;
      if ( streamChannels > 1 ) monoMode = false;
      channelCounter -= streamChannels;
      streamCount++;
    }
  }

  free( bufferList );

  // Determine the buffer size.
  AudioValueRange	bufferRange;
  dataSize = sizeof( AudioValueRange );
  property.mSelector = kAudioDevicePropertyBufferFrameSizeRange;
  result = AudioObjectGetPropertyData( id, &property, 0, NULL, &dataSize, &bufferRange );

  if ( result != noErr ) {
    errorStream_ << "RtApiCore::probeDeviceOpen: system error (" << getErrorCode( result ) << ") getting buffer size range for device (" << deviceId << ").";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  if ( bufferRange.mMinimum > *bufferSize ) *bufferSize = (unsigned int) bufferRange.mMinimum;
  else if ( bufferRange.mMaximum < *bufferSize ) *bufferSize = (unsigned int) bufferRange.mMaximum;
  if ( options && options->flags & RTAUDIO_MINIMIZE_LATENCY ) *bufferSize = (unsigned int) bufferRange.mMinimum;

  // Set the buffer size.  For multiple streams, I'm assuming we only
  // need to make this setting for the master channel.
  UInt32 theSize = (UInt32) *bufferSize;
  dataSize = sizeof( UInt32 );
  property.mSelector = kAudioDevicePropertyBufferFrameSize;
  result = AudioObjectSetPropertyData( id, &property, 0, NULL, dataSize, &theSize );

  if ( result != noErr ) {
    errorStream_ << "RtApiCore::probeDeviceOpen: system error (" << getErrorCode( result ) << ") setting the buffer size for device (" << deviceId << ").";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // If attempting to setup a duplex stream, the bufferSize parameter
  // MUST be the same in both directions!
  *bufferSize = theSize;
  if ( stream_.mode == OUTPUT && mode == INPUT && *bufferSize != stream_.bufferSize ) {
    errorStream_ << "RtApiCore::probeDeviceOpen: system error setting buffer size for duplex stream on device (" << deviceId << ").";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  stream_.bufferSize = *bufferSize;
  stream_.nBuffers = 1;

  // Try to set "hog" mode ... it's not clear to me this is working.
  if ( options && options->flags & RTAUDIO_HOG_DEVICE ) {
    pid_t hog_pid;
    dataSize = sizeof( hog_pid );
    property.mSelector = kAudioDevicePropertyHogMode;
    result = AudioObjectGetPropertyData( id, &property, 0, NULL, &dataSize, &hog_pid );
    if ( result != noErr ) {
      errorStream_ << "RtApiCore::probeDeviceOpen: system error (" << getErrorCode( result ) << ") getting 'hog' state!";
      errorText_ = errorStream_.str();
      return FAILURE;
    }

    if ( hog_pid != getpid() ) {
      hog_pid = getpid();
      result = AudioObjectSetPropertyData( id, &property, 0, NULL, dataSize, &hog_pid );
      if ( result != noErr ) {
        errorStream_ << "RtApiCore::probeDeviceOpen: system error (" << getErrorCode( result ) << ") setting 'hog' state!";
        errorText_ = errorStream_.str();
        return FAILURE;
      }
    }
  }

  // Check and if necessary, change the sample rate for the device.
  Float64 nominalRate;
  dataSize = sizeof( Float64 );
  property.mSelector = kAudioDevicePropertyNominalSampleRate;
  result = AudioObjectGetPropertyData( id, &property, 0, NULL, &dataSize, &nominalRate );
  if ( result != noErr ) {
    errorStream_ << "RtApiCore::probeDeviceOpen: system error (" << getErrorCode( result ) << ") getting current sample rate.";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Only try to change the sample rate if off by more than 1 Hz.
  if ( fabs( nominalRate - (double)sampleRate ) > 1.0 ) {

    nominalRate = (Float64) sampleRate;
    result = AudioObjectSetPropertyData( id, &property, 0, NULL, dataSize, &nominalRate );
    if ( result != noErr ) {
      errorStream_ << "RtApiCore::probeDeviceOpen: system error (" << getErrorCode( result ) << ") setting sample rate for device (" << deviceId << ").";
      errorText_ = errorStream_.str();
      return FAILURE;
    }

    // Now wait until the reported nominal rate is what we just set.
    UInt32 microCounter = 0;
    Float64 reportedRate = 0.0;
    while ( reportedRate != nominalRate ) {
      microCounter += 5000;
      if ( microCounter > 2000000 ) break;
      usleep( 5000 );
      result = AudioObjectGetPropertyData( id, &property, 0, NULL, &dataSize, &reportedRate );
    }

    if ( microCounter > 2000000 ) {
      errorStream_ << "RtApiCore::probeDeviceOpen: timeout waiting for sample rate update for device (" << deviceId << ").";
      errorText_ = errorStream_.str();
      return FAILURE;
    }
  }

  // Now set the stream format for all streams.  Also, check the
  // physical format of the device and change that if necessary.
  AudioStreamBasicDescription	description;
  dataSize = sizeof( AudioStreamBasicDescription );
  property.mSelector = kAudioStreamPropertyVirtualFormat;
  result = AudioObjectGetPropertyData( id, &property, 0, NULL, &dataSize, &description );
  if ( result != noErr ) {
    errorStream_ << "RtApiCore::probeDeviceOpen: system error (" << getErrorCode( result ) << ") getting stream format for device (" << deviceId << ").";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Set the sample rate and data format id.  However, only make the
  // change if the sample rate is not within 1.0 of the desired
  // rate and the format is not linear pcm.
  bool updateFormat = false;
  if ( fabs( description.mSampleRate - (Float64)sampleRate ) > 1.0 ) {
    description.mSampleRate = (Float64) sampleRate;
    updateFormat = true;
  }

  if ( description.mFormatID != kAudioFormatLinearPCM ) {
    description.mFormatID = kAudioFormatLinearPCM;
    updateFormat = true;
  }

  if ( updateFormat ) {
    result = AudioObjectSetPropertyData( id, &property, 0, NULL, dataSize, &description );
    if ( result != noErr ) {
      errorStream_ << "RtApiCore::probeDeviceOpen: system error (" << getErrorCode( result ) << ") setting sample rate or data format for device (" << deviceId << ").";
      errorText_ = errorStream_.str();
      return FAILURE;
    }
  }

  // Now check the physical format.
  property.mSelector = kAudioStreamPropertyPhysicalFormat;
  result = AudioObjectGetPropertyData( id, &property, 0, NULL,  &dataSize, &description );
  if ( result != noErr ) {
    errorStream_ << "RtApiCore::probeDeviceOpen: system error (" << getErrorCode( result ) << ") getting stream physical format for device (" << deviceId << ").";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  //std::cout << "Current physical stream format:" << std::endl;
  //std::cout << "   mBitsPerChan = " << description.mBitsPerChannel << std::endl;
  //std::cout << "   aligned high = " << (description.mFormatFlags & kAudioFormatFlagIsAlignedHigh) << ", isPacked = " << (description.mFormatFlags & kAudioFormatFlagIsPacked) << std::endl;
  //std::cout << "   bytesPerFrame = " << description.mBytesPerFrame << std::endl;
  //std::cout << "   sample rate = " << description.mSampleRate << std::endl;

  if ( description.mFormatID != kAudioFormatLinearPCM || description.mBitsPerChannel < 16 ) {
    description.mFormatID = kAudioFormatLinearPCM;
    //description.mSampleRate = (Float64) sampleRate;
    AudioStreamBasicDescription	testDescription = description;
    UInt32 formatFlags;

    // We'll try higher bit rates first and then work our way down.
    std::vector< std::pair<UInt32, UInt32>  > physicalFormats;
    formatFlags = (description.mFormatFlags | kLinearPCMFormatFlagIsFloat) & ~kLinearPCMFormatFlagIsSignedInteger;
    physicalFormats.push_back( std::pair<Float32, UInt32>( 32, formatFlags ) );
    formatFlags = (description.mFormatFlags | kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked) & ~kLinearPCMFormatFlagIsFloat;
    physicalFormats.push_back( std::pair<Float32, UInt32>( 32, formatFlags ) );
    physicalFormats.push_back( std::pair<Float32, UInt32>( 24, formatFlags ) );   // 24-bit packed
    formatFlags &= ~( kAudioFormatFlagIsPacked | kAudioFormatFlagIsAlignedHigh );
    physicalFormats.push_back( std::pair<Float32, UInt32>( 24.2, formatFlags ) ); // 24-bit in 4 bytes, aligned low
    formatFlags |= kAudioFormatFlagIsAlignedHigh;
    physicalFormats.push_back( std::pair<Float32, UInt32>( 24.4, formatFlags ) ); // 24-bit in 4 bytes, aligned high
    formatFlags = (description.mFormatFlags | kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked) & ~kLinearPCMFormatFlagIsFloat;
    physicalFormats.push_back( std::pair<Float32, UInt32>( 16, formatFlags ) );
    physicalFormats.push_back( std::pair<Float32, UInt32>( 8, formatFlags ) );

    bool setPhysicalFormat = false;
    for( unsigned int i=0; i<physicalFormats.size(); i++ ) {
      testDescription = description;
      testDescription.mBitsPerChannel = (UInt32) physicalFormats[i].first;
      testDescription.mFormatFlags = physicalFormats[i].second;
      if ( (24 == (UInt32)physicalFormats[i].first) && ~( physicalFormats[i].second & kAudioFormatFlagIsPacked ) )
        testDescription.mBytesPerFrame =  4 * testDescription.mChannelsPerFrame;
      else
        testDescription.mBytesPerFrame =  testDescription.mBitsPerChannel/8 * testDescription.mChannelsPerFrame;
      testDescription.mBytesPerPacket = testDescription.mBytesPerFrame * testDescription.mFramesPerPacket;
      result = AudioObjectSetPropertyData( id, &property, 0, NULL, dataSize, &testDescription );
      if ( result == noErr ) {
        setPhysicalFormat = true;
        //std::cout << "Updated physical stream format:" << std::endl;
        //std::cout << "   mBitsPerChan = " << testDescription.mBitsPerChannel << std::endl;
        //std::cout << "   aligned high = " << (testDescription.mFormatFlags & kAudioFormatFlagIsAlignedHigh) << ", isPacked = " << (testDescription.mFormatFlags & kAudioFormatFlagIsPacked) << std::endl;
        //std::cout << "   bytesPerFrame = " << testDescription.mBytesPerFrame << std::endl;
        //std::cout << "   sample rate = " << testDescription.mSampleRate << std::endl;
        break;
      }
    }

    if ( !setPhysicalFormat ) {
      errorStream_ << "RtApiCore::probeDeviceOpen: system error (" << getErrorCode( result ) << ") setting physical data format for device (" << deviceId << ").";
      errorText_ = errorStream_.str();
      return FAILURE;
    }
  } // done setting virtual/physical formats.

  // Get the stream / device latency.
  UInt32 latency;
  dataSize = sizeof( UInt32 );
  property.mSelector = kAudioDevicePropertyLatency;
  if ( AudioObjectHasProperty( id, &property ) == true ) {
    result = AudioObjectGetPropertyData( id, &property, 0, NULL, &dataSize, &latency );
    if ( result == kAudioHardwareNoError ) stream_.latency[ mode ] = latency;
    else {
      errorStream_ << "RtApiCore::probeDeviceOpen: system error (" << getErrorCode( result ) << ") getting device latency for device (" << deviceId << ").";
      errorText_ = errorStream_.str();
      error( RTAUDIO_WARNING );
    }
  }

  // Byte-swapping: According to AudioHardware.h, the stream data will
  // always be presented in native-endian format, so we should never
  // need to byte swap.
  stream_.doByteSwap[mode] = false;

  // From the CoreAudio documentation, PCM data must be supplied as
  // 32-bit floats.
  stream_.userFormat = format;
  stream_.deviceFormat[mode] = RTAUDIO_FLOAT32;

  if ( streamCount == 1 )
    stream_.nDeviceChannels[mode] = description.mChannelsPerFrame;
  else // multiple streams
    stream_.nDeviceChannels[mode] = channels;
  stream_.nUserChannels[mode] = channels;
  stream_.channelOffset[mode] = channelOffset;  // offset within a CoreAudio stream
  if ( options && options->flags & RTAUDIO_NONINTERLEAVED ) stream_.userInterleaved = false;
  else stream_.userInterleaved = true;
  stream_.deviceInterleaved[mode] = true;
  if ( monoMode == true ) stream_.deviceInterleaved[mode] = false;

  // Set flags for buffer conversion.
  stream_.doConvertBuffer[mode] = false;
  if ( stream_.userFormat != stream_.deviceFormat[mode] )
    stream_.doConvertBuffer[mode] = true;
  if ( stream_.nUserChannels[mode] < stream_.nDeviceChannels[mode] )
    stream_.doConvertBuffer[mode] = true;
  if ( streamCount == 1 ) {
    if ( stream_.nUserChannels[mode] > 1 &&
         stream_.userInterleaved != stream_.deviceInterleaved[mode] )
      stream_.doConvertBuffer[mode] = true;
  }
  else if ( monoMode && stream_.userInterleaved )
    stream_.doConvertBuffer[mode] = true;

  // Allocate our CoreHandle structure for the stream.
  CoreHandle *handle = 0;
  if ( stream_.apiHandle == 0 ) {
    try {
      handle = new CoreHandle;
    }
    catch ( std::bad_alloc& ) {
      errorText_ = "RtApiCore::probeDeviceOpen: error allocating CoreHandle memory.";
      goto error;
    }

    if ( pthread_cond_init( &handle->condition, NULL ) ) {
      errorText_ = "RtApiCore::probeDeviceOpen: error initializing pthread condition variable.";
      goto error;
    }
    stream_.apiHandle = (void *) handle;
  }
  else
    handle = (CoreHandle *) stream_.apiHandle;
  handle->iStream[mode] = firstStream;
  handle->nStreams[mode] = streamCount;
  handle->id[mode] = id;

  // Allocate necessary internal buffers.
  unsigned long bufferBytes;
  bufferBytes = stream_.nUserChannels[mode] * *bufferSize * formatBytes( stream_.userFormat );
  stream_.userBuffer[mode] = (char *) calloc( bufferBytes, 1 );
  if ( stream_.userBuffer[mode] == NULL ) {
    errorText_ = "RtApiCore::probeDeviceOpen: error allocating user buffer memory.";
    goto error;
  }

  // If possible, we will make use of the CoreAudio stream buffers as
  // "device buffers".  However, we can't do this if using multiple
  // streams.
  if ( stream_.doConvertBuffer[mode] && handle->nStreams[mode] > 1 ) {

    bool makeBuffer = true;
    bufferBytes = stream_.nDeviceChannels[mode] * formatBytes( stream_.deviceFormat[mode] );
    if ( mode == INPUT ) {
      if ( stream_.mode == OUTPUT && stream_.deviceBuffer ) {
        unsigned long bytesOut = stream_.nDeviceChannels[0] * formatBytes( stream_.deviceFormat[0] );
        if ( bufferBytes <= bytesOut ) makeBuffer = false;
      }
    }

    if ( makeBuffer ) {
      bufferBytes *= *bufferSize;
      if ( stream_.deviceBuffer ) free( stream_.deviceBuffer );
      stream_.deviceBuffer = (char *) calloc( bufferBytes, 1 );
      if ( stream_.deviceBuffer == NULL ) {
        errorText_ = "RtApiCore::probeDeviceOpen: error allocating device buffer memory.";
        goto error;
      }
    }
  }

  stream_.sampleRate = sampleRate;
  stream_.deviceId[mode] = deviceId;
  stream_.state = STREAM_STOPPED;
  stream_.callbackInfo.object = (void *) this;

  // Setup the buffer conversion information structure.
  if ( stream_.doConvertBuffer[mode] ) {
    if ( streamCount > 1 ) setConvertInfo( mode, 0 );
    else setConvertInfo( mode, channelOffset );
  }

  if ( mode == INPUT && stream_.mode == OUTPUT && stream_.deviceId[0] == deviceId )
    // Only one callback procedure and property listener per device.
    stream_.mode = DUPLEX;
  else {
#if defined( MAC_OS_X_VERSION_10_5 ) && ( MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5 )
    result = AudioDeviceCreateIOProcID( id, callbackHandler, (void *) &stream_.callbackInfo, &handle->procId[mode] );
#else
    // deprecated in favor of AudioDeviceCreateIOProcID()
    result = AudioDeviceAddIOProc( id, callbackHandler, (void *) &stream_.callbackInfo );
#endif
    if ( result != noErr ) {
      errorStream_ << "RtApiCore::probeDeviceOpen: system error setting callback for device (" << deviceId << ").";
      errorText_ = errorStream_.str();
      goto error;
    }
    if ( stream_.mode == OUTPUT && mode == INPUT )
      stream_.mode = DUPLEX;
    else
      stream_.mode = mode;

    // Setup the device property listener for over/underload.
    property.mSelector = kAudioDeviceProcessorOverload;
    property.mScope = kAudioObjectPropertyScopeGlobal;
    result = AudioObjectAddPropertyListener( id, &property, xrunListener, (void *) handle );
    if ( result != noErr ) {
      errorStream_ << "RtApiCore::probeDeviceOpen: system error setting xrun listener for device (" << deviceId << ").";
      errorText_ = errorStream_.str();
      goto error;
    }
    handle->xrunListenerAdded[mode] = true;

    // Setup a listener to detect a possible device disconnect.
    property.mSelector = kAudioDevicePropertyDeviceIsAlive;
    result = AudioObjectAddPropertyListener( id , &property, streamDisconnectListener, (void *) &stream_.callbackInfo );
    if ( result != noErr ) {
      AudioObjectRemovePropertyListener( id, &property, xrunListener, (void *) handle );
      errorStream_ << "RtApiCore::probeDeviceOpen: system error setting disconnect listener for device (" << deviceId << ").";
      errorText_ = errorStream_.str();
      goto error;
    }
    handle->disconnectListenerAdded[mode] = true;
  }

  return SUCCESS;

 error:
  closeStream(); // this should safely clear out procedures, listeners and memory, even for duplex stream
  return FAILURE;
}

void RtApiCore :: closeStream( void )
{
  if ( stream_.state == STREAM_CLOSED ) {
    errorText_ = "RtApiCore::closeStream(): no open stream to close!";
    error( RTAUDIO_WARNING );
    return;
  }

  CoreHandle *handle = (CoreHandle *) stream_.apiHandle;
  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {
    if ( handle ) {
      AudioObjectPropertyAddress property = { kAudioHardwarePropertyDevices,
                                              kAudioObjectPropertyScopeGlobal,
                                              KAUDIOOBJECTPROPERTYELEMENT };
      if ( handle->xrunListenerAdded[0] ) {
        property.mSelector = kAudioDeviceProcessorOverload;
        if (AudioObjectRemovePropertyListener( handle->id[0], &property, xrunListener, (void *) handle ) != noErr) {
          errorText_ = "RtApiCore::closeStream(): error removing xrun property listener!";
          error( RTAUDIO_WARNING );
        }
      }
      if ( handle->disconnectListenerAdded[0] ) {
        property.mSelector = kAudioDevicePropertyDeviceIsAlive;
        if (AudioObjectRemovePropertyListener( handle->id[0], &property, streamDisconnectListener, (void *) &stream_.callbackInfo ) != noErr) {
          errorText_ = "RtApiCore::closeStream(): error removing disconnect property listener!";
          error( RTAUDIO_WARNING );
        }
      }

#if defined( MAC_OS_X_VERSION_10_5 ) && ( MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5 )
      if ( handle->procId[0] ) {
        if ( stream_.state == STREAM_RUNNING )
          AudioDeviceStop( handle->id[0], handle->procId[0] );
        AudioDeviceDestroyIOProcID( handle->id[0], handle->procId[0] );
      }
#else // deprecated behaviour
      if ( stream_.state == STREAM_RUNNING )
        AudioDeviceStop( handle->id[0], callbackHandler );
      AudioDeviceRemoveIOProc( handle->id[0], callbackHandler );
#endif
    }
  }

  if ( stream_.mode == INPUT || ( stream_.mode == DUPLEX && stream_.deviceId[0] != stream_.deviceId[1] ) ) {
    if ( handle ) {
      AudioObjectPropertyAddress property = { kAudioHardwarePropertyDevices,
        kAudioObjectPropertyScopeGlobal,
        KAUDIOOBJECTPROPERTYELEMENT };

      if ( handle->xrunListenerAdded[1] ) {
        property.mSelector = kAudioDeviceProcessorOverload;
        if (AudioObjectRemovePropertyListener( handle->id[1], &property, xrunListener, (void *) handle ) != noErr) {
          errorText_ = "RtApiCore::closeStream(): error removing xrun property listener!";
          error( RTAUDIO_WARNING );
        }
      }

      if ( handle->disconnectListenerAdded[0] ) {
        property.mSelector = kAudioDevicePropertyDeviceIsAlive;
        if (AudioObjectRemovePropertyListener( handle->id[1], &property, streamDisconnectListener, (void *) &stream_.callbackInfo ) != noErr) {
          errorText_ = "RtApiCore::closeStream(): error removing disconnect property listener!";
          error( RTAUDIO_WARNING );
        }
      }

#if defined( MAC_OS_X_VERSION_10_5 ) && ( MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5 )
      if ( handle->procId[1] ) {
        if ( stream_.state == STREAM_RUNNING )
          AudioDeviceStop( handle->id[1], handle->procId[1] );
        AudioDeviceDestroyIOProcID( handle->id[1], handle->procId[1] );
      }
#else // deprecated behaviour
      if ( stream_.state == STREAM_RUNNING )
        AudioDeviceStop( handle->id[1], callbackHandler );
      AudioDeviceRemoveIOProc( handle->id[1], callbackHandler );
#endif
    }
  }

  for ( int i=0; i<2; i++ ) {
    if ( stream_.userBuffer[i] ) {
      free( stream_.userBuffer[i] );
      stream_.userBuffer[i] = 0;
    }
  }

  if ( stream_.deviceBuffer ) {
    free( stream_.deviceBuffer );
    stream_.deviceBuffer = 0;
  }

  // Destroy pthread condition variable.
  pthread_cond_signal( &handle->condition ); // signal condition variable in case stopStream is blocked
  pthread_cond_destroy( &handle->condition );
  delete handle;
  stream_.apiHandle = 0;

  CallbackInfo *info = (CallbackInfo *) &stream_.callbackInfo;
  if ( info->deviceDisconnected ) {
    errorText_ = "RtApiCore: the stream device was disconnected (and closed)!";
    error( RTAUDIO_DEVICE_DISCONNECT );
  }
  
  clearStreamInfo();
}

RtAudioErrorType RtApiCore :: startStream( void )
{
  if ( stream_.state != STREAM_STOPPED ) {
    if ( stream_.state == STREAM_RUNNING )
      errorText_ = "RtApiCore::startStream(): the stream is already running!";
    else if ( stream_.state == STREAM_STOPPING || stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiCore::startStream(): the stream is stopping or closed!";
    return error( RTAUDIO_WARNING );
  }

  /*
  #if defined( HAVE_GETTIMEOFDAY )
  gettimeofday( &stream_.lastTickTimestamp, NULL );
  #endif
  */

  OSStatus result = noErr;
  CoreHandle *handle = (CoreHandle *) stream_.apiHandle;
  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {

#if defined( MAC_OS_X_VERSION_10_5 ) && ( MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5 )
    result = AudioDeviceStart( handle->id[0], handle->procId[0] );
#else // deprecated behaviour
    result = AudioDeviceStart( handle->id[0], callbackHandler );
#endif
    if ( result != noErr ) {
      errorStream_ << "RtApiCore::startStream: system error (" << getErrorCode( result ) << ") starting callback procedure on device (" << stream_.deviceId[0] << ").";
      errorText_ = errorStream_.str();
      goto unlock;
    }
  }

  if ( stream_.mode == INPUT ||
       ( stream_.mode == DUPLEX && stream_.deviceId[0] != stream_.deviceId[1] ) ) {

    // Clear user input buffer
    unsigned long bufferBytes;
    bufferBytes = stream_.nUserChannels[1] * stream_.bufferSize * formatBytes( stream_.userFormat );
    memset( stream_.userBuffer[1], 0, bufferBytes * sizeof(char) );

#if defined( MAC_OS_X_VERSION_10_5 ) && ( MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5 )
    result = AudioDeviceStart( handle->id[1], handle->procId[1] );
#else // deprecated behaviour
    result = AudioDeviceStart( handle->id[1], callbackHandler );
#endif
    if ( result != noErr ) {
      errorStream_ << "RtApiCore::startStream: system error starting input callback procedure on device (" << stream_.deviceId[1] << ").";
      errorText_ = errorStream_.str();
      goto unlock;
    }
  }

  handle->drainCounter = 0;
  handle->internalDrain = false;
  stream_.state = STREAM_RUNNING;

 unlock:
  if ( result == noErr ) return RTAUDIO_NO_ERROR;
  return error( RTAUDIO_SYSTEM_ERROR );
}

RtAudioErrorType RtApiCore :: stopStream( void )
{
  if ( stream_.state != STREAM_RUNNING && stream_.state != STREAM_STOPPING ) {
    if ( stream_.state == STREAM_STOPPED )
      errorText_ = "RtApiCore::stopStream(): the stream is already stopped!";
    else if ( stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiCore::stopStream(): the stream is closed!";
    return error( RTAUDIO_WARNING );
  }

  OSStatus result = noErr;
  CoreHandle *handle = (CoreHandle *) stream_.apiHandle;
  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {

    if ( handle->drainCounter == 0 ) {
      handle->drainCounter = 2;
      pthread_cond_wait( &handle->condition, &stream_.mutex ); // block until signaled
    }

#if defined( MAC_OS_X_VERSION_10_5 ) && ( MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5 )
    result = AudioDeviceStop( handle->id[0], handle->procId[0] );
#else // deprecated behaviour
    result = AudioDeviceStop( handle->id[0], callbackHandler );
#endif
    if ( result != noErr ) {
      errorStream_ << "RtApiCore::stopStream: system error (" << getErrorCode( result ) << ") stopping callback procedure on device (" << stream_.deviceId[0] << ").";
      errorText_ = errorStream_.str();
      goto unlock;
    }
  }

  if ( stream_.mode == INPUT || ( stream_.mode == DUPLEX && stream_.deviceId[0] != stream_.deviceId[1] ) ) {
#if defined( MAC_OS_X_VERSION_10_5 ) && ( MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_X_VERSION_10_5 )
    result = AudioDeviceStop( handle->id[1], handle->procId[1] );
#else  // deprecated behaviour
    result = AudioDeviceStop( handle->id[1], callbackHandler );
#endif
    if ( result != noErr ) {
      errorStream_ << "RtApiCore::stopStream: system error (" << getErrorCode( result ) << ") stopping input callback procedure on device (" << stream_.deviceId[1] << ").";
      errorText_ = errorStream_.str();
      goto unlock;
    }
  }

  stream_.state = STREAM_STOPPED;

 unlock:
  if ( result == noErr ) return RTAUDIO_NO_ERROR;
  return error( RTAUDIO_SYSTEM_ERROR );
}

RtAudioErrorType RtApiCore :: abortStream( void )
{
  if ( stream_.state != STREAM_RUNNING ) {
    if ( stream_.state == STREAM_STOPPED )
      errorText_ = "RtApiCore::abortStream(): the stream is already stopped!";
    else if ( stream_.state == STREAM_STOPPING || stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiCore::abortStream(): the stream is stopping or closed!";
    return error( RTAUDIO_WARNING );
  }

  CoreHandle *handle = (CoreHandle *) stream_.apiHandle;
  handle->drainCounter = 2;

  stream_.state = STREAM_STOPPING;
  return stopStream();
}

// This function will be called by a spawned thread when the user
// callback function signals that the stream should be stopped or
// aborted.  It is better to handle it this way because the
// callbackEvent() function probably should return before the
// AudioDeviceStop() function is called.
static void *coreStopStream( void *ptr )
{
  CallbackInfo *info = (CallbackInfo *) ptr;
  RtApiCore *object = (RtApiCore *) info->object;

  object->stopStream();
  pthread_exit( NULL );
}

bool RtApiCore :: callbackEvent( AudioDeviceID deviceId,
                                 const AudioBufferList *inBufferList,
                                 const AudioBufferList *outBufferList )
{
  if ( stream_.state == STREAM_STOPPED || stream_.state == STREAM_STOPPING ) return SUCCESS;
  if ( stream_.state == STREAM_CLOSED ) {
    errorText_ = "RtApiCore::callbackEvent(): the stream is closed ... this shouldn't happen!";
    error( RTAUDIO_WARNING );
    return FAILURE;
  }

  CallbackInfo *info = (CallbackInfo *) &stream_.callbackInfo;
  CoreHandle *handle = (CoreHandle *) stream_.apiHandle;

  // Check if we were draining the stream and signal is finished.
  if ( handle->drainCounter > 3 ) {
    ThreadHandle threadId;

    stream_.state = STREAM_STOPPING;
    if ( handle->internalDrain == true )
      pthread_create( &threadId, NULL, coreStopStream, info );
    else // external call to stopStream()
      pthread_cond_signal( &handle->condition );
    return SUCCESS;
  }

  AudioDeviceID outputDevice = handle->id[0];

  // Invoke user callback to get fresh output data UNLESS we are
  // draining stream or duplex mode AND the input/output devices are
  // different AND this function is called for the input device.
  if ( handle->drainCounter == 0 && ( stream_.mode != DUPLEX || deviceId == outputDevice ) ) {
    RtAudioCallback callback = (RtAudioCallback) info->callback;
    double streamTime = getStreamTime();
    RtAudioStreamStatus status = 0;
    if ( stream_.mode != INPUT && handle->xrun[0] == true ) {
      status |= RTAUDIO_OUTPUT_UNDERFLOW;
      handle->xrun[0] = false;
    }
    if ( stream_.mode != OUTPUT && handle->xrun[1] == true ) {
      status |= RTAUDIO_INPUT_OVERFLOW;
      handle->xrun[1] = false;
    }

    int cbReturnValue = callback( stream_.userBuffer[0], stream_.userBuffer[1],
                                  stream_.bufferSize, streamTime, status, info->userData );
    if ( cbReturnValue == 2 ) {
      abortStream();
      return SUCCESS;
    }
    else if ( cbReturnValue == 1 ) {
      handle->drainCounter = 1;
      handle->internalDrain = true;
    }
  }

  if ( stream_.mode == OUTPUT || ( stream_.mode == DUPLEX && deviceId == outputDevice ) ) {

    if ( handle->drainCounter > 1 ) { // write zeros to the output stream

      if ( handle->nStreams[0] == 1 ) {
        memset( outBufferList->mBuffers[handle->iStream[0]].mData,
                0,
                outBufferList->mBuffers[handle->iStream[0]].mDataByteSize );
      }
      else { // fill multiple streams with zeros
        for ( unsigned int i=0; i<handle->nStreams[0]; i++ ) {
          memset( outBufferList->mBuffers[handle->iStream[0]+i].mData,
                  0,
                  outBufferList->mBuffers[handle->iStream[0]+i].mDataByteSize );
        }
      }
    }
    else if ( handle->nStreams[0] == 1 ) {
      if ( stream_.doConvertBuffer[0] ) { // convert directly to CoreAudio stream buffer
        convertBuffer( (char *) outBufferList->mBuffers[handle->iStream[0]].mData,
                       stream_.userBuffer[0], stream_.convertInfo[0] );
      }
      else { // copy from user buffer
        memcpy( outBufferList->mBuffers[handle->iStream[0]].mData,
                stream_.userBuffer[0],
                outBufferList->mBuffers[handle->iStream[0]].mDataByteSize );
      }
    }
    else { // fill multiple streams
      Float32 *inBuffer = (Float32 *) stream_.userBuffer[0];
      if ( stream_.doConvertBuffer[0] ) {
        convertBuffer( stream_.deviceBuffer, stream_.userBuffer[0], stream_.convertInfo[0] );
        inBuffer = (Float32 *) stream_.deviceBuffer;
      }

      if ( stream_.deviceInterleaved[0] == false ) { // mono mode
        UInt32 bufferBytes = outBufferList->mBuffers[handle->iStream[0]].mDataByteSize;
        for ( unsigned int i=0; i<stream_.nUserChannels[0]; i++ ) {
          memcpy( outBufferList->mBuffers[handle->iStream[0]+i].mData,
                  (void *)&inBuffer[i*stream_.bufferSize], bufferBytes );
        }
      }
      else { // fill multiple multi-channel streams with interleaved data
        UInt32 streamChannels, channelsLeft, inJump, outJump, inOffset;
        Float32 *out, *in;

        bool inInterleaved = ( stream_.userInterleaved ) ? true : false;
        UInt32 inChannels = stream_.nUserChannels[0];
        if ( stream_.doConvertBuffer[0] ) {
          inInterleaved = true; // device buffer will always be interleaved for nStreams > 1 and not mono mode
          inChannels = stream_.nDeviceChannels[0];
        }

        if ( inInterleaved ) inOffset = 1;
        else inOffset = stream_.bufferSize;

        channelsLeft = inChannels;
        for ( unsigned int i=0; i<handle->nStreams[0]; i++ ) {
          in = inBuffer;
          out = (Float32 *) outBufferList->mBuffers[handle->iStream[0]+i].mData;
          streamChannels = outBufferList->mBuffers[handle->iStream[0]+i].mNumberChannels;

          outJump = 0;
          // Account for possible channel offset in first stream
          if ( i == 0 && stream_.channelOffset[0] > 0 ) {
            streamChannels -= stream_.channelOffset[0];
            outJump = stream_.channelOffset[0];
            out += outJump;
          }

          // Account for possible unfilled channels at end of the last stream
          if ( streamChannels > channelsLeft ) {
            outJump = streamChannels - channelsLeft;
            streamChannels = channelsLeft;
          }

          // Determine input buffer offsets and skips
          if ( inInterleaved ) {
            inJump = inChannels;
            in += inChannels - channelsLeft;
          }
          else {
            inJump = 1;
            in += (inChannels - channelsLeft) * inOffset;
          }

          for ( unsigned int i=0; i<stream_.bufferSize; i++ ) {
            for ( unsigned int j=0; j<streamChannels; j++ ) {
              *out++ = in[j*inOffset];
            }
            out += outJump;
            in += inJump;
          }
          channelsLeft -= streamChannels;
        }
      }
    }
  }

  // Don't bother draining input
  if ( handle->drainCounter ) {
    handle->drainCounter++;
    goto unlock;
  }

  AudioDeviceID inputDevice;
  inputDevice = handle->id[1];
  if ( stream_.mode == INPUT || ( stream_.mode == DUPLEX && deviceId == inputDevice ) ) {

    if ( handle->nStreams[1] == 1 ) {
      if ( stream_.doConvertBuffer[1] ) { // convert directly from CoreAudio stream buffer
        convertBuffer( stream_.userBuffer[1],
                       (char *) inBufferList->mBuffers[handle->iStream[1]].mData,
                       stream_.convertInfo[1] );
      }
      else { // copy to user buffer
        memcpy( stream_.userBuffer[1],
                inBufferList->mBuffers[handle->iStream[1]].mData,
                inBufferList->mBuffers[handle->iStream[1]].mDataByteSize );
      }
    }
    else { // read from multiple streams
      Float32 *outBuffer = (Float32 *) stream_.userBuffer[1];
      if ( stream_.doConvertBuffer[1] ) outBuffer = (Float32 *) stream_.deviceBuffer;

      if ( stream_.deviceInterleaved[1] == false ) { // mono mode
        UInt32 bufferBytes = inBufferList->mBuffers[handle->iStream[1]].mDataByteSize;
        for ( unsigned int i=0; i<stream_.nUserChannels[1]; i++ ) {
          memcpy( (void *)&outBuffer[i*stream_.bufferSize],
                  inBufferList->mBuffers[handle->iStream[1]+i].mData, bufferBytes );
        }
      }
      else { // read from multiple multi-channel streams
        UInt32 streamChannels, channelsLeft, inJump, outJump, outOffset;
        Float32 *out, *in;

        bool outInterleaved = ( stream_.userInterleaved ) ? true : false;
        UInt32 outChannels = stream_.nUserChannels[1];
        if ( stream_.doConvertBuffer[1] ) {
          outInterleaved = true; // device buffer will always be interleaved for nStreams > 1 and not mono mode
          outChannels = stream_.nDeviceChannels[1];
        }

        if ( outInterleaved ) outOffset = 1;
        else outOffset = stream_.bufferSize;

        channelsLeft = outChannels;
        for ( unsigned int i=0; i<handle->nStreams[1]; i++ ) {
          out = outBuffer;
          in = (Float32 *) inBufferList->mBuffers[handle->iStream[1]+i].mData;
          streamChannels = inBufferList->mBuffers[handle->iStream[1]+i].mNumberChannels;

          inJump = 0;
          // Account for possible channel offset in first stream
          if ( i == 0 && stream_.channelOffset[1] > 0 ) {
            streamChannels -= stream_.channelOffset[1];
            inJump = stream_.channelOffset[1];
            in += inJump;
          }

          // Account for possible unread channels at end of the last stream
          if ( streamChannels > channelsLeft ) {
            inJump = streamChannels - channelsLeft;
            streamChannels = channelsLeft;
          }

          // Determine output buffer offsets and skips
          if ( outInterleaved ) {
            outJump = outChannels;
            out += outChannels - channelsLeft;
          }
          else {
            outJump = 1;
            out += (outChannels - channelsLeft) * outOffset;
          }

          for ( unsigned int i=0; i<stream_.bufferSize; i++ ) {
            for ( unsigned int j=0; j<streamChannels; j++ ) {
              out[j*outOffset] = *in++;
            }
            out += outJump;
            in += inJump;
          }
          channelsLeft -= streamChannels;
        }
      }
      
      if ( stream_.doConvertBuffer[1] ) { // convert from our internal "device" buffer
        convertBuffer( stream_.userBuffer[1],
                       stream_.deviceBuffer,
                       stream_.convertInfo[1] );
      }
    }
  }

 unlock:

  // Make sure to only tick duplex stream time once if using two devices
  if ( stream_.mode == DUPLEX ) {
    if ( handle->id[0] == handle->id[1] ) // same device, only one callback
      RtApi::tickStreamTime();
    else if ( deviceId == handle->id[0] )
      RtApi::tickStreamTime(); // two devices, only tick on the output callback
  } else
    RtApi::tickStreamTime(); // input or output stream only
  
  return SUCCESS;
}

const char* RtApiCore :: getErrorCode( OSStatus code )
{
  switch( code ) {

  case kAudioHardwareNotRunningError:
    return "kAudioHardwareNotRunningError";

  case kAudioHardwareUnspecifiedError:
    return "kAudioHardwareUnspecifiedError";

  case kAudioHardwareUnknownPropertyError:
    return "kAudioHardwareUnknownPropertyError";

  case kAudioHardwareBadPropertySizeError:
    return "kAudioHardwareBadPropertySizeError";

  case kAudioHardwareIllegalOperationError:
    return "kAudioHardwareIllegalOperationError";

  case kAudioHardwareBadObjectError:
    return "kAudioHardwareBadObjectError";

  case kAudioHardwareBadDeviceError:
    return "kAudioHardwareBadDeviceError";

  case kAudioHardwareBadStreamError:
    return "kAudioHardwareBadStreamError";

  case kAudioHardwareUnsupportedOperationError:
    return "kAudioHardwareUnsupportedOperationError";

  case kAudioDeviceUnsupportedFormatError:
    return "kAudioDeviceUnsupportedFormatError";

  case kAudioDevicePermissionsError:
    return "kAudioDevicePermissionsError";

  default:
    return "CoreAudio unknown error";
  }
}

  //******************** End of __MACOSX_CORE__ *********************//
#endif

#if defined(__UNIX_JACK__)

// JACK is a low-latency audio server, originally written for the
// GNU/Linux operating system and now also ported to OS-X and
// Windows. It can connect a number of different applications to an
// audio device, as well as allowing them to share audio between
// themselves.
//
// When using JACK with RtAudio, "devices" refer to JACK clients that
// have ports connected to the server, while ports correspond to device
// channels.  The JACK server is typically started  in a terminal as
// follows:
//
// .jackd -d alsa -d hw:0
//
// or through an interface program such as qjackctl.  Many of the
// parameters normally set for a stream are fixed by the JACK server
// and can be specified when the JACK server is started.  In
// particular,
//
// .jackd -d alsa -d hw:0 -r 44100 -p 512 -n 4
//
// specifies a sample rate of 44100 Hz, a buffer size of 512 sample
// frames, and number of buffers = 4.  Once the server is running, it
// is not possible to override these values.  If the values are not
// specified in the command-line, the JACK server uses default values.
//
// The JACK server does not have to be running when an instance of
// RtApiJack is created, though the function getDeviceCount() will
// report 0 devices found until JACK has been started.  When no
// devices are available (i.e., the JACK server is not running), a
// stream cannot be opened.

#include <unistd.h>
#include <cstdio>

// A structure to hold various information related to the Jack API
// implementation.
struct JackHandle {
  jack_client_t *client;
  jack_port_t **ports[2];
  std::string deviceName[2];
  bool xrun[2];
  pthread_cond_t condition;
  int drainCounter;       // Tracks callback counts when draining
  bool internalDrain;     // Indicates if stop is initiated from callback or not.

  JackHandle()
    :client(0), drainCounter(0), internalDrain(false) { ports[0] = 0; ports[1] = 0; xrun[0] = false; xrun[1] = false; }
};

std::string escapeJackPortRegex(std::string &str)
{
  const std::string need_escaping = "()[]{}*+?$^.|\\";
  std::string escaped_string;
  for (auto c : str)
  {
    if (need_escaping.find(c) !=  std::string::npos)
      escaped_string.push_back('\\');

    escaped_string.push_back(c);
  }
  return escaped_string;
}

#if !defined(__RTAUDIO_DEBUG__)
static void jackSilentError( const char * ) {};
#endif

RtApiJack :: RtApiJack()
    :shouldAutoconnect_(true) {
  // Nothing to do here.
#if !defined(__RTAUDIO_DEBUG__)
  // Turn off Jack's internal error reporting.
  jack_set_error_function( &jackSilentError );
#endif
}

RtApiJack :: ~RtApiJack()
{
  if ( stream_.state != STREAM_CLOSED ) closeStream();
}

void RtApiJack :: probeDevices( void )
{
  // See list of required functionality in RtApi::probeDevices().

  // See if we can become a jack client.
  jack_options_t options = (jack_options_t) ( JackNoStartServer ); //JackNullOption;
  jack_status_t *status = NULL;
  jack_client_t *client = jack_client_open( "RtApiJackProbe", options, status );
  if ( client == 0 ) {
    deviceList_.clear(); // in case the server is shutdown after a previous successful probe
    errorText_ = "RtApiJack::probeDevices: Jack server not found or connection error!";
    //error( RTAUDIO_SYSTEM_ERROR );
    error( RTAUDIO_WARNING );
    return;
  }

  const char **ports;
  std::string port, previousPort;
  unsigned int nChannels = 0, nDevices = 0;
  std::vector<std::string> portNames;
  ports = jack_get_ports( client, NULL, JACK_DEFAULT_AUDIO_TYPE, 0 );
  if ( ports ) {
    // Parse the port names up to the first colon (:).
    size_t iColon = 0;
    do {
      port = (char *) ports[ nChannels ];
      iColon = port.find(":");
      if ( iColon != std::string::npos ) {
        port = port.substr( 0, iColon );
        if ( port != previousPort ) {
          portNames.push_back( port );
          nDevices++;
          previousPort = port;
        }
      }
    } while ( ports[++nChannels] );
    free( ports );
  }

  // Fill or update the deviceList_.
  unsigned int m, n;
  for ( n=0; n<nDevices; n++ ) {
    for ( m=0; m<deviceList_.size(); m++ ) {
      if ( deviceList_[m].name == portNames[n] )
        break; // We already have this device.
    }
    if ( m == deviceList_.size() ) { // new device
      RtAudio::DeviceInfo info;
      info.name = portNames[n];
      if ( probeDeviceInfo( info, client ) == false ) continue; // ignore if probe fails
      info.ID = currentDeviceId_++;  // arbitrary internal device ID
      deviceList_.push_back( info );
      // A callback can be registered in Jack to be notified about client
      // (dis)connections. However, this can only be done with an open client,
      // so unless we want to keep a special client open all the time, this
      // would only report (dis)connections when a stream is open. I'm not
      // going to bother for the moment.
    }
  }

  // Remove any devices left in the list that are no longer available.
  for ( std::vector<RtAudio::DeviceInfo>::iterator it=deviceList_.begin(); it!=deviceList_.end(); ) {
    for ( m=0; m<portNames.size(); m++ ) {
      if ( (*it).name == portNames[m] ) {
        ++it;
        break;
      }
    }
    if ( m == portNames.size() ) // not found so remove it from our list
      it = deviceList_.erase( it );
  }

  jack_client_close( client );

  if ( nDevices == 0 ) {
    deviceList_.clear();
    return;
  }
  
  // Jack doesn't provide default devices so call the getDefault
  // functions, which will set the first available input and output
  // devices as the defaults.
  getDefaultInputDevice();
  getDefaultOutputDevice();
}

bool RtApiJack :: probeDeviceInfo( RtAudio::DeviceInfo& info, jack_client_t *client )
{
  // Get the current jack server sample rate.
  info.sampleRates.clear();

  info.preferredSampleRate = jack_get_sample_rate( client );
  info.sampleRates.push_back( info.preferredSampleRate );

  // Count the available ports containing the client name as device
  // channels.  Jack "input ports" equal RtAudio output channels.
  unsigned int nChannels = 0;
  const char **ports = jack_get_ports( client, escapeJackPortRegex(info.name).c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput );
  if ( ports ) {
    while ( ports[ nChannels ] ) nChannels++;
    free( ports );
    info.outputChannels = nChannels;
  }

  // Jack "output ports" equal RtAudio input channels.
  nChannels = 0;
  ports = jack_get_ports( client, escapeJackPortRegex(info.name).c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput );
  if ( ports ) {
    while ( ports[ nChannels ] ) nChannels++;
    free( ports );
    info.inputChannels = nChannels;
  }

  if ( info.outputChannels == 0 && info.inputChannels == 0 ) {
    jack_client_close(client);
    errorText_ = "RtApiJack::getDeviceInfo: error determining Jack input/output channels!";
    error( RTAUDIO_WARNING );
    return false;
  }

  // If device opens for both playback and capture, we determine the channels.
  if ( info.outputChannels > 0 && info.inputChannels > 0 )
    info.duplexChannels = (info.outputChannels > info.inputChannels) ? info.inputChannels : info.outputChannels;

  // Jack always uses 32-bit floats.
  info.nativeFormats = RTAUDIO_FLOAT32;

  return true;
}

static int jackCallbackHandler( jack_nframes_t nframes, void *infoPointer )
{
  CallbackInfo *info = (CallbackInfo *) infoPointer;

  RtApiJack *object = (RtApiJack *) info->object;
  if ( object->callbackEvent( (unsigned long) nframes ) == false ) return 1;

  return 0;
}

// This function will be called by a spawned thread when the Jack
// server signals that it is shutting down.  It is necessary to handle
// it this way because the jackShutdown() function must return before
// the jack_deactivate() function (in closeStream()) will return.
static void *jackCloseStream( void *ptr )
{
  CallbackInfo *info = (CallbackInfo *) ptr;
  RtApiJack *object = (RtApiJack *) info->object;

  info->deviceDisconnected = true;
  object->closeStream();
  pthread_exit( NULL );
}

/*
// Could be used to catch client connections but requires open client.
static void jackClientChange( const char *name, int registered, void *infoPointer )
{
  std::cout << "in jackClientChange, name = " << name << ", registered = " << registered << std::endl;
}
*/

static void jackShutdown( void *infoPointer )
{
  CallbackInfo *info = (CallbackInfo *) infoPointer;
  RtApiJack *object = (RtApiJack *) info->object;

  // Check current stream state.  If stopped, then we'll assume this
  // was called as a result of a call to RtApiJack::stopStream (the
  // deactivation of a client handle causes this function to be called).
  // If not, we'll assume the Jack server is shutting down or some
  // other problem occurred and we should close the stream.
  if ( object->isStreamRunning() == false ) return;

  ThreadHandle threadId;
  pthread_create( &threadId, NULL, jackCloseStream, info );
}

static int jackXrun( void *infoPointer )
{
  JackHandle *handle = *((JackHandle **) infoPointer);

  if ( handle->ports[0] ) handle->xrun[0] = true;
  if ( handle->ports[1] ) handle->xrun[1] = true;

  return 0;
}

bool RtApiJack :: probeDeviceOpen( unsigned int deviceId, StreamMode mode, unsigned int channels,
                                   unsigned int firstChannel, unsigned int sampleRate,
                                   RtAudioFormat format, unsigned int *bufferSize,
                                   RtAudio::StreamOptions *options )
{
  JackHandle *handle = (JackHandle *) stream_.apiHandle;

  // Look for jack server and try to become a client (only do once per stream).
  jack_client_t *client = 0;
  if ( mode == OUTPUT || ( mode == INPUT && stream_.mode != OUTPUT ) ) {
    jack_options_t jackoptions = (jack_options_t) ( JackNoStartServer ); //JackNullOption;
    jack_status_t *status = NULL;
    if ( options && !options->streamName.empty() )
      client = jack_client_open( options->streamName.c_str(), jackoptions, status );
    else
      client = jack_client_open( "RtApiJack", jackoptions, status );
    if ( client == 0 ) {
      errorText_ = "RtApiJack::probeDeviceOpen: Jack server not found or connection error!";
      error( RTAUDIO_WARNING );
      return FAILURE;
    }
  }
  else {
    // The handle must have been created on an earlier pass.
    client = handle->client;
  }

  std::string deviceName;
  for ( unsigned int m=0; m<deviceList_.size(); m++ ) {
    if ( deviceList_[m].ID == deviceId ) {
      deviceName = deviceList_[m].name;
      break;
    }
  }

  if ( deviceName.empty() ) {
    errorText_ = "RtApiJack::probeDeviceOpen: device ID is invalid!";
    return FAILURE;
  }

  unsigned long flag = JackPortIsInput;
  if ( mode == INPUT ) flag = JackPortIsOutput;

  const char **ports;
  if ( ! (options && (options->flags & RTAUDIO_JACK_DONT_CONNECT)) ) {
    // Count the available ports containing the client name as device
    // channels.  Jack "input ports" equal RtAudio output channels.
    unsigned int nChannels = 0;
    ports = jack_get_ports( client, escapeJackPortRegex(deviceName).c_str(), JACK_DEFAULT_AUDIO_TYPE, flag );
    if ( ports ) {
      while ( ports[ nChannels ] ) nChannels++;
      free( ports );
    }
    // Compare the jack ports for specified client to the requested number of channels.
    if ( nChannels < (channels + firstChannel) ) {
      errorStream_ << "RtApiJack::probeDeviceOpen: requested number of channels (" << channels << ") + offset (" << firstChannel << ") not found for specified device (" << deviceName << ").";
      errorText_ = errorStream_.str();
      return FAILURE;
    }
  }

  // Check the jack server sample rate.
  unsigned int jackRate = jack_get_sample_rate( client );
  if ( sampleRate != jackRate ) {
    jack_client_close( client );
    errorStream_ << "RtApiJack::probeDeviceOpen: the requested sample rate (" << sampleRate << ") is different than the JACK server rate (" << jackRate << ").";
    errorText_ = errorStream_.str();
    return FAILURE;
  }
  stream_.sampleRate = jackRate;

  // Get the latency of the JACK port.
  ports = jack_get_ports( client, escapeJackPortRegex(deviceName).c_str(), JACK_DEFAULT_AUDIO_TYPE, flag );
  if ( ports[ firstChannel ] ) {
    // Added by Ge Wang
    jack_latency_callback_mode_t cbmode = (mode == INPUT ? JackCaptureLatency : JackPlaybackLatency);
    // the range (usually the min and max are equal)
    jack_latency_range_t latrange; latrange.min = latrange.max = 0;
    // get the latency range
    jack_port_get_latency_range( jack_port_by_name( client, ports[firstChannel] ), cbmode, &latrange );
    // be optimistic, use the min!
    stream_.latency[mode] = latrange.min;
    //stream_.latency[mode] = jack_port_get_latency( jack_port_by_name( client, ports[ firstChannel ] ) );
  }
  free( ports );

  // The jack server always uses 32-bit floating-point data.
  stream_.deviceFormat[mode] = RTAUDIO_FLOAT32;
  stream_.userFormat = format;

  if ( options && options->flags & RTAUDIO_NONINTERLEAVED ) stream_.userInterleaved = false;
  else stream_.userInterleaved = true;

  // Jack always uses non-interleaved buffers.
  stream_.deviceInterleaved[mode] = false;

  // Jack always provides host byte-ordered data.
  stream_.doByteSwap[mode] = false;

  // Get the buffer size.  The buffer size and number of buffers
  // (periods) is set when the jack server is started.
  stream_.bufferSize = (int) jack_get_buffer_size( client );
  *bufferSize = stream_.bufferSize;

  stream_.nDeviceChannels[mode] = channels;
  stream_.nUserChannels[mode] = channels;

  // Set flags for buffer conversion.
  stream_.doConvertBuffer[mode] = false;
  if ( stream_.userFormat != stream_.deviceFormat[mode] )
    stream_.doConvertBuffer[mode] = true;
  if ( stream_.userInterleaved != stream_.deviceInterleaved[mode] &&
       stream_.nUserChannels[mode] > 1 )
    stream_.doConvertBuffer[mode] = true;

  // Allocate our JackHandle structure for the stream.
  if ( handle == 0 ) {
    try {
      handle = new JackHandle;
    }
    catch ( std::bad_alloc& ) {
      errorText_ = "RtApiJack::probeDeviceOpen: error allocating JackHandle memory.";
      goto error;
    }

    if ( pthread_cond_init(&handle->condition, NULL) ) {
      errorText_ = "RtApiJack::probeDeviceOpen: error initializing pthread condition variable.";
      goto error;
    }
    stream_.apiHandle = (void *) handle;
    handle->client = client;
  }
  handle->deviceName[mode] = deviceName;

  // Allocate necessary internal buffers.
  unsigned long bufferBytes;
  bufferBytes = stream_.nUserChannels[mode] * *bufferSize * formatBytes( stream_.userFormat );
  stream_.userBuffer[mode] = (char *) calloc( bufferBytes, 1 );
  if ( stream_.userBuffer[mode] == NULL ) {
    errorText_ = "RtApiJack::probeDeviceOpen: error allocating user buffer memory.";
    goto error;
  }

  if ( stream_.doConvertBuffer[mode] ) {

    bool makeBuffer = true;
    if ( mode == OUTPUT )
      bufferBytes = stream_.nDeviceChannels[0] * formatBytes( stream_.deviceFormat[0] );
    else { // mode == INPUT
      bufferBytes = stream_.nDeviceChannels[1] * formatBytes( stream_.deviceFormat[1] );
      if ( stream_.mode == OUTPUT && stream_.deviceBuffer ) {
        unsigned long bytesOut = stream_.nDeviceChannels[0] * formatBytes(stream_.deviceFormat[0]);
        if ( bufferBytes < bytesOut ) makeBuffer = false;
      }
    }

    if ( makeBuffer ) {
      bufferBytes *= *bufferSize;
      if ( stream_.deviceBuffer ) free( stream_.deviceBuffer );
      stream_.deviceBuffer = (char *) calloc( bufferBytes, 1 );
      if ( stream_.deviceBuffer == NULL ) {
        errorText_ = "RtApiJack::probeDeviceOpen: error allocating device buffer memory.";
        goto error;
      }
    }
  }

  // Allocate memory for the Jack ports (channels) identifiers.
  handle->ports[mode] = (jack_port_t **) malloc ( sizeof (jack_port_t *) * channels );
  if ( handle->ports[mode] == NULL )  {
    errorText_ = "RtApiJack::probeDeviceOpen: error allocating port memory.";
    goto error;
  }

  stream_.channelOffset[mode] = firstChannel;
  stream_.state = STREAM_STOPPED;
  stream_.callbackInfo.object = (void *) this;

  if ( stream_.mode == OUTPUT && mode == INPUT )
    // We had already set up the stream for output.
    stream_.mode = DUPLEX;
  else {
    stream_.mode = mode;
    jack_set_process_callback( handle->client, jackCallbackHandler, (void *) &stream_.callbackInfo );
    jack_set_xrun_callback( handle->client, jackXrun, (void *) &stream_.apiHandle );
    jack_on_shutdown( handle->client, jackShutdown, (void *) &stream_.callbackInfo );
    //jack_set_client_registration_callback( handle->client, jackClientChange, (void *) &stream_.callbackInfo );
  }

  // Register our ports.
  char label[64];
  if ( mode == OUTPUT ) {
    for ( unsigned int i=0; i<stream_.nUserChannels[0]; i++ ) {
      snprintf( label, 64, "outport %d", i );
      handle->ports[0][i] = jack_port_register( handle->client, (const char *)label,
                                                JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput, 0 );
    }
  }
  else {
    for ( unsigned int i=0; i<stream_.nUserChannels[1]; i++ ) {
      snprintf( label, 64, "inport %d", i );
      handle->ports[1][i] = jack_port_register( handle->client, (const char *)label,
                                                JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput, 0 );
    }
  }

  // Setup the buffer conversion information structure.  We don't use
  // buffers to do channel offsets, so we override that parameter
  // here.
  if ( stream_.doConvertBuffer[mode] ) setConvertInfo( mode, 0 );

  if ( options && options->flags & RTAUDIO_JACK_DONT_CONNECT ) shouldAutoconnect_ = false;

  return SUCCESS;

 error:
  if ( handle ) {
    pthread_cond_destroy( &handle->condition );
    jack_client_close( handle->client );

    if ( handle->ports[0] ) free( handle->ports[0] );
    if ( handle->ports[1] ) free( handle->ports[1] );

    delete handle;
    stream_.apiHandle = 0;
  }

  for ( int i=0; i<2; i++ ) {
    if ( stream_.userBuffer[i] ) {
      free( stream_.userBuffer[i] );
      stream_.userBuffer[i] = 0;
    }
  }

  if ( stream_.deviceBuffer ) {
    free( stream_.deviceBuffer );
    stream_.deviceBuffer = 0;
  }

  return FAILURE;
}

void RtApiJack :: closeStream( void )
{
  if ( stream_.state == STREAM_CLOSED ) {
    errorText_ = "RtApiJack::closeStream(): no open stream to close!";
    error( RTAUDIO_WARNING );
    return;
  }

  JackHandle *handle = (JackHandle *) stream_.apiHandle;
  if ( handle ) {
    if ( stream_.state == STREAM_RUNNING )
      jack_deactivate( handle->client );

    if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {
      for ( unsigned int i=0; i<stream_.nUserChannels[0]; i++ )
        jack_port_unregister( handle->client, handle->ports[0][i] );
    }
    if ( stream_.mode == INPUT || stream_.mode == DUPLEX ) {
      for ( unsigned int i=0; i<stream_.nUserChannels[1]; i++ )
        jack_port_unregister( handle->client, handle->ports[1][i] );
    }
    jack_client_close( handle->client );
    
    if ( handle->ports[0] ) free( handle->ports[0] );
    if ( handle->ports[1] ) free( handle->ports[1] );
    pthread_cond_destroy( &handle->condition );
    delete handle;
    stream_.apiHandle = 0;
  }

  CallbackInfo *info = (CallbackInfo *) &stream_.callbackInfo;
  if ( info->deviceDisconnected ) {
    errorText_ = "RtApiJack: the Jack server is shutting down this client ... stream stopped and closed!";
    error( RTAUDIO_DEVICE_DISCONNECT );
  }

  for ( int i=0; i<2; i++ ) {
    if ( stream_.userBuffer[i] ) {
      free( stream_.userBuffer[i] );
      stream_.userBuffer[i] = 0;
    }
  }

  if ( stream_.deviceBuffer ) {
    free( stream_.deviceBuffer );
    stream_.deviceBuffer = 0;
  }

  clearStreamInfo();
}

RtAudioErrorType RtApiJack :: startStream( void )
{
  if ( stream_.state != STREAM_STOPPED ) {
    if ( stream_.state == STREAM_RUNNING )
      errorText_ = "RtApiJack::startStream(): the stream is already running!";
    else if ( stream_.state == STREAM_STOPPING || stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiJack::startStream(): the stream is stopping or closed!";
    return error( RTAUDIO_WARNING );
  }

  /*
  #if defined( HAVE_GETTIMEOFDAY )
  gettimeofday( &stream_.lastTickTimestamp, NULL );
  #endif
  */

  JackHandle *handle = (JackHandle *) stream_.apiHandle;
  int result = jack_activate( handle->client );
  if ( result ) {
    errorText_ = "RtApiJack::startStream(): unable to activate JACK client!";
    goto unlock;
  }

  const char **ports;

  // Get the list of available ports.
  if ( shouldAutoconnect_ && (stream_.mode == OUTPUT || stream_.mode == DUPLEX) ) {
    ports = jack_get_ports( handle->client, escapeJackPortRegex(handle->deviceName[0]).c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
    if ( ports == NULL) {
      errorText_ = "RtApiJack::startStream(): error determining available JACK input ports!";
      goto unlock;
    }

    // Now make the port connections.  Since RtAudio wasn't designed to
    // allow the user to select particular channels of a device, we'll
    // just open the first "nChannels" ports with offset.
    for ( unsigned int i=0; i<stream_.nUserChannels[0]; i++ ) {
      result = 1;
      if ( ports[ stream_.channelOffset[0] + i ] )
        result = jack_connect( handle->client, jack_port_name( handle->ports[0][i] ), ports[ stream_.channelOffset[0] + i ] );
      if ( result ) {
        free( ports );
        errorText_ = "RtApiJack::startStream(): error connecting output ports!";
        goto unlock;
      }
    }
    free(ports);
  }

  if ( shouldAutoconnect_ && (stream_.mode == INPUT || stream_.mode == DUPLEX) ) {
    ports = jack_get_ports( handle->client, escapeJackPortRegex(handle->deviceName[1]).c_str(), JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput );
    if ( ports == NULL) {
      errorText_ = "RtApiJack::startStream(): error determining available JACK output ports!";
      goto unlock;
    }

    // Now make the port connections.  See note above.
    for ( unsigned int i=0; i<stream_.nUserChannels[1]; i++ ) {
      result = 1;
      if ( ports[ stream_.channelOffset[1] + i ] )
        result = jack_connect( handle->client, ports[ stream_.channelOffset[1] + i ], jack_port_name( handle->ports[1][i] ) );
      if ( result ) {
        free( ports );
        errorText_ = "RtApiJack::startStream(): error connecting input ports!";
        goto unlock;
      }
    }
    free(ports);
  }

  handle->drainCounter = 0;
  handle->internalDrain = false;
  stream_.state = STREAM_RUNNING;

 unlock:
  if ( result == 0 ) return RTAUDIO_NO_ERROR;
  return error( RTAUDIO_SYSTEM_ERROR );
}

RtAudioErrorType  RtApiJack :: stopStream( void )
{
  if ( stream_.state != STREAM_RUNNING && stream_.state != STREAM_STOPPING ) {
    if ( stream_.state == STREAM_STOPPED )
      errorText_ = "RtApiJack::stopStream(): the stream is already stopped!";
    else if ( stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiJack::stopStream(): the stream is closed!";
    return error( RTAUDIO_WARNING );
  }

  JackHandle *handle = (JackHandle *) stream_.apiHandle;
  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {

    if ( handle->drainCounter == 0 ) {
      handle->drainCounter = 2;
      pthread_cond_wait( &handle->condition, &stream_.mutex ); // block until signaled
    }
  }

  jack_deactivate( handle->client );
  stream_.state = STREAM_STOPPED;
  return RTAUDIO_NO_ERROR;
}

RtAudioErrorType RtApiJack :: abortStream( void )
{
  if ( stream_.state != STREAM_RUNNING ) {
    if ( stream_.state == STREAM_STOPPED )
      errorText_ = "RtApiJack::abortStream(): the stream is already stopped!";
    else if ( stream_.state == STREAM_STOPPING || stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiJack::abortStream(): the stream is stopping or closed!";
    return error( RTAUDIO_WARNING );
  }

  JackHandle *handle = (JackHandle *) stream_.apiHandle;
  handle->drainCounter = 2;

  return stopStream();
}

// This function will be called by a spawned thread when the user
// callback function signals that the stream should be stopped or
// aborted.  It is necessary to handle it this way because the
// callbackEvent() function must return before the jack_deactivate()
// function will return.
static void *jackStopStream( void *ptr )
{
  CallbackInfo *info = (CallbackInfo *) ptr;
  RtApiJack *object = (RtApiJack *) info->object;

  object->stopStream();
  pthread_exit( NULL );
}

bool RtApiJack :: callbackEvent( unsigned long nframes )
{
  if ( stream_.state == STREAM_STOPPED || stream_.state == STREAM_STOPPING ) return SUCCESS;
  if ( stream_.state == STREAM_CLOSED ) {
    errorText_ = "RtApiJack::callbackEvent(): the stream is closed ... this shouldn't happen!";
    error( RTAUDIO_WARNING );
    return FAILURE;
  }
  if ( stream_.bufferSize != nframes ) {
    errorText_ = "RtApiJack::callbackEvent(): the JACK buffer size has changed ... cannot process!";
    error( RTAUDIO_WARNING );
    return FAILURE;
  }

  CallbackInfo *info = (CallbackInfo *) &stream_.callbackInfo;
  JackHandle *handle = (JackHandle *) stream_.apiHandle;

  // Check if we were draining the stream and signal is finished.
  if ( handle->drainCounter > 3 ) {
    ThreadHandle threadId;

    stream_.state = STREAM_STOPPING;
    if ( handle->internalDrain == true )
      pthread_create( &threadId, NULL, jackStopStream, info );
    else // external call to stopStream()
      pthread_cond_signal( &handle->condition );
    return SUCCESS;
  }

  // Invoke user callback first, to get fresh output data.
  if ( handle->drainCounter == 0 ) {
    RtAudioCallback callback = (RtAudioCallback) info->callback;
    double streamTime = getStreamTime();
    RtAudioStreamStatus status = 0;
    if ( stream_.mode != INPUT && handle->xrun[0] == true ) {
      status |= RTAUDIO_OUTPUT_UNDERFLOW;
      handle->xrun[0] = false;
    }
    if ( stream_.mode != OUTPUT && handle->xrun[1] == true ) {
      status |= RTAUDIO_INPUT_OVERFLOW;
      handle->xrun[1] = false;
    }
    int cbReturnValue = callback( stream_.userBuffer[0], stream_.userBuffer[1],
                                  stream_.bufferSize, streamTime, status, info->userData );
    if ( cbReturnValue == 2 ) {
      stream_.state = STREAM_STOPPING;
      handle->drainCounter = 2;
      ThreadHandle id;
      pthread_create( &id, NULL, jackStopStream, info );
      return SUCCESS;
    }
    else if ( cbReturnValue == 1 ) {
      handle->drainCounter = 1;
      handle->internalDrain = true;
    }
  }

  jack_default_audio_sample_t *jackbuffer;
  unsigned long bufferBytes = nframes * sizeof( jack_default_audio_sample_t );
  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {

    if ( handle->drainCounter > 1 ) { // write zeros to the output stream

      for ( unsigned int i=0; i<stream_.nDeviceChannels[0]; i++ ) {
        jackbuffer = (jack_default_audio_sample_t *) jack_port_get_buffer( handle->ports[0][i], (jack_nframes_t) nframes );
        memset( jackbuffer, 0, bufferBytes );
      }

    }
    else if ( stream_.doConvertBuffer[0] ) {

      convertBuffer( stream_.deviceBuffer, stream_.userBuffer[0], stream_.convertInfo[0] );

      for ( unsigned int i=0; i<stream_.nDeviceChannels[0]; i++ ) {
        jackbuffer = (jack_default_audio_sample_t *) jack_port_get_buffer( handle->ports[0][i], (jack_nframes_t) nframes );
        memcpy( jackbuffer, &stream_.deviceBuffer[i*bufferBytes], bufferBytes );
      }
    }
    else { // no buffer conversion
      for ( unsigned int i=0; i<stream_.nUserChannels[0]; i++ ) {
        jackbuffer = (jack_default_audio_sample_t *) jack_port_get_buffer( handle->ports[0][i], (jack_nframes_t) nframes );
        memcpy( jackbuffer, &stream_.userBuffer[0][i*bufferBytes], bufferBytes );
      }
    }
  }

  // Don't bother draining input
  if ( handle->drainCounter ) {
    handle->drainCounter++;
    goto unlock;
  }

  if ( stream_.mode == INPUT || stream_.mode == DUPLEX ) {

    if ( stream_.doConvertBuffer[1] ) {
      for ( unsigned int i=0; i<stream_.nDeviceChannels[1]; i++ ) {
        jackbuffer = (jack_default_audio_sample_t *) jack_port_get_buffer( handle->ports[1][i], (jack_nframes_t) nframes );
        memcpy( &stream_.deviceBuffer[i*bufferBytes], jackbuffer, bufferBytes );
      }
      convertBuffer( stream_.userBuffer[1], stream_.deviceBuffer, stream_.convertInfo[1] );
    }
    else { // no buffer conversion
      for ( unsigned int i=0; i<stream_.nUserChannels[1]; i++ ) {
        jackbuffer = (jack_default_audio_sample_t *) jack_port_get_buffer( handle->ports[1][i], (jack_nframes_t) nframes );
        memcpy( &stream_.userBuffer[1][i*bufferBytes], jackbuffer, bufferBytes );
      }
    }
  }

 unlock:
  RtApi::tickStreamTime();
  return SUCCESS;
}
  //******************** End of __UNIX_JACK__ *********************//
#endif

#if defined(__WINDOWS_ASIO__) // ASIO API on Windows

// The ASIO API is designed around a callback scheme, so this
// implementation is similar to that used for OS-X CoreAudio and unix
// Jack.  The primary constraint with ASIO is that it only allows
// access to a single driver at a time.  Thus, it is not possible to
// have more than one simultaneous RtAudio stream.
//
// This implementation also requires a number of external ASIO files
// and a few global variables.  The ASIO callback scheme does not
// allow for the passing of user data, so we must create a global
// pointer to our callbackInfo structure.
//
// On unix systems, we make use of a pthread condition variable.
// Since there is no equivalent in Windows, I hacked something based
// on information found in
// http://www.cs.wustl.edu/~schmidt/win32-cv-1.html.

#include "asiosys.h"
#include "asio.h"
#include "iasiothiscallresolver.h"
#include "asiodrivers.h"
#include <cmath>

static AsioDrivers drivers;
static ASIOCallbacks asioCallbacks;
static ASIODriverInfo driverInfo;
static CallbackInfo *asioCallbackInfo;
static bool asioXRun;
static bool streamOpen = false; // Tracks whether any instance of RtAudio has a stream open

struct AsioHandle {
  int drainCounter;       // Tracks callback counts when draining
  bool internalDrain;     // Indicates if stop is initiated from callback or not.
  ASIOBufferInfo *bufferInfos;
  HANDLE condition;

  AsioHandle()
    :drainCounter(0), internalDrain(false), bufferInfos(0) {}
};

// Function declarations (definitions at end of section)
static const char* getAsioErrorString( ASIOError result );
static void sampleRateChanged( ASIOSampleRate sRate );
static long asioMessages( long selector, long value, void* message, double* opt );

RtApiAsio :: RtApiAsio()
{
  // ASIO cannot run on a multi-threaded apartment. You can call
  // CoInitialize beforehand, but it must be for apartment threading
  // (in which case, CoInitilialize will return S_FALSE here).
  coInitialized_ = false;
  HRESULT hr = CoInitialize( NULL ); 
  if ( FAILED(hr) ) {
    errorText_ = "RtApiAsio::ASIO requires a single-threaded apartment. Call CoInitializeEx(0,COINIT_APARTMENTTHREADED)";
    error( RTAUDIO_WARNING );
  }
  coInitialized_ = true;

  // Check whether another RtAudio instance has an ASIO stream open.
  if ( streamOpen ) {
    errorText_ = "RtApiAsio(): Another RtAudio ASIO stream is open, functionality may be limited.";
    error( RTAUDIO_WARNING );
  }
  else
    drivers.removeCurrentDriver();

  driverInfo.asioVersion = 2;

  // See note in DirectSound implementation about GetDesktopWindow().
  driverInfo.sysRef = GetForegroundWindow();
}

RtApiAsio :: ~RtApiAsio()
{
  if ( stream_.state != STREAM_CLOSED ) closeStream();
  if ( coInitialized_ ) CoUninitialize();
}

void RtApiAsio :: probeDevices( void )
{
  // See list of required functionality in RtApi::probeDevices().

  if ( streamOpen ) {
    errorText_ = "RtApiAsio::probeDevices: Another RtAudio ASIO stream is open, cannot probe devices.";
    error( RTAUDIO_WARNING );
    return;
  }
    
  unsigned int nDevices = drivers.asioGetNumDev();
  if ( nDevices == 0 ) {
    deviceList_.clear();
    return;
  }

  char tmp[32];
  std::vector< std::string > driverNames;
  unsigned int n, m;
  for ( n=0; n<nDevices; n++ ) {
    ASIOError result = drivers.asioGetDriverName( (int) n, tmp, 32 );
    if ( result != ASE_OK ) {
      errorStream_ << "RtApiAsio::probeDevices: unable to get driver name (" << getAsioErrorString( result ) << ").";
      errorText_ = errorStream_.str();
      error( RTAUDIO_WARNING );
      continue;
    }
    driverNames.push_back( tmp );
    for ( m=0; m<deviceList_.size(); m++ ) {
      if ( deviceList_[m].name == driverNames.back() )
        break; // We already have this device.
    }
    if ( m == deviceList_.size() ) { // new device
      RtAudio::DeviceInfo info;
      info.name = driverNames.back();
      if ( probeDeviceInfo( info ) == false ) continue; // ignore if probe fails
      info.ID = currentDeviceId_++;  // arbitrary internal device ID
      deviceList_.push_back( info );
    }
  }

  // Remove any devices left in the list that are no longer available.
  for ( std::vector<RtAudio::DeviceInfo>::iterator it=deviceList_.begin(); it!=deviceList_.end(); ) {
    for ( m=0; m<driverNames.size(); m++ ) {
      if ( (*it).name == driverNames[m] ) {
        ++it;
        break;
      }
    }
    if ( m == driverNames.size() ) // not found so remove it from our list
      it = deviceList_.erase( it );
  }

  // Asio doesn't provide default devices so call the getDefault
  // functions, which will set the first available input and output
  // devices as the defaults. Don't call getDefaultXXXDevice if
  // deviceList is empty.
  if(deviceList_.size() > 0)
  {
    getDefaultInputDevice();
    getDefaultOutputDevice();
  }
}

bool RtApiAsio :: probeDeviceInfo( RtAudio::DeviceInfo &info )
{
  if ( !drivers.loadDriver( const_cast<char *>(info.name.c_str()) ) ) {
    errorStream_ << "RtApiAsio::probeDeviceInfo: unable to load driver (" << info.name << ").";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  ASIOError result = ASIOInit( &driverInfo );
  if ( result != ASE_OK ) {
    drivers.removeCurrentDriver();
    errorStream_ << "RtApiAsio::probeDeviceInfo: error (" << getAsioErrorString( result ) << ") initializing driver (" << info.name << ").";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  // Determine the device channel information.
  long inputChannels, outputChannels;
  result = ASIOGetChannels( &inputChannels, &outputChannels );
  if ( result != ASE_OK ) {
    ASIOExit();
    drivers.removeCurrentDriver();
    errorStream_ << "RtApiAsio::probeDeviceInfo: error (" << getAsioErrorString( result ) << ") getting channel count (" << info.name << ").";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  info.outputChannels = outputChannels;
  info.inputChannels = inputChannels;
  if ( info.outputChannels > 0 && info.inputChannels > 0 )
    info.duplexChannels = (info.outputChannels > info.inputChannels) ? info.inputChannels : info.outputChannels;

  // Determine the supported sample rates.
  info.sampleRates.clear();
  for ( unsigned int i=0; i<MAX_SAMPLE_RATES; i++ ) {
    result = ASIOCanSampleRate( (ASIOSampleRate) SAMPLE_RATES[i] );
    if ( result == ASE_OK ) {
      info.sampleRates.push_back( SAMPLE_RATES[i] );

      if ( !info.preferredSampleRate || ( SAMPLE_RATES[i] <= 48000 && SAMPLE_RATES[i] > info.preferredSampleRate ) )
        info.preferredSampleRate = SAMPLE_RATES[i];
    }
  }

  // Determine supported data types ... just check first channel and assume rest are the same.
  ASIOChannelInfo channelInfo;
  channelInfo.channel = 0;
  channelInfo.isInput = true;
  if ( info.inputChannels <= 0 ) channelInfo.isInput = false;
  result = ASIOGetChannelInfo( &channelInfo );
  if ( result != ASE_OK ) {
    ASIOExit();
    drivers.removeCurrentDriver();
    errorStream_ << "RtApiAsio::probeDeviceInfo: error (" << getAsioErrorString( result ) << ") getting driver channel info (" << info.name << ").";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  info.nativeFormats = 0;
  if ( channelInfo.type == ASIOSTInt16MSB || channelInfo.type == ASIOSTInt16LSB )
    info.nativeFormats |= RTAUDIO_SINT16;
  else if ( channelInfo.type == ASIOSTInt32MSB || channelInfo.type == ASIOSTInt32LSB )
    info.nativeFormats |= RTAUDIO_SINT32;
  else if ( channelInfo.type == ASIOSTFloat32MSB || channelInfo.type == ASIOSTFloat32LSB )
    info.nativeFormats |= RTAUDIO_FLOAT32;
  else if ( channelInfo.type == ASIOSTFloat64MSB || channelInfo.type == ASIOSTFloat64LSB )
    info.nativeFormats |= RTAUDIO_FLOAT64;
  else if ( channelInfo.type == ASIOSTInt24MSB || channelInfo.type == ASIOSTInt24LSB )
    info.nativeFormats |= RTAUDIO_SINT24;

  ASIOExit();
  drivers.removeCurrentDriver();
  return true;
}

static void bufferSwitch( long index, ASIOBool /*processNow*/ )
{
  RtApiAsio *object = (RtApiAsio *) asioCallbackInfo->object;
  object->callbackEvent( index );
}

bool RtApiAsio :: probeDeviceOpen( unsigned int deviceId, StreamMode mode, unsigned int channels,
                                   unsigned int firstChannel, unsigned int sampleRate,
                                   RtAudioFormat format, unsigned int *bufferSize,
                                   RtAudio::StreamOptions *options )
{
  bool isDuplexInput = mode == INPUT && stream_.mode == OUTPUT;

  // For ASIO, a duplex stream MUST use the same driver.
  if ( isDuplexInput && stream_.deviceId[0] != deviceId ) {
    errorText_ = "RtApiAsio::probeDeviceOpen: an ASIO duplex stream must use the same device for input and output!";
    return FAILURE;
  }

  std::string driverName;
  for ( unsigned int m=0; m<deviceList_.size(); m++ ) {
    if ( deviceList_[m].ID == deviceId ) {
      driverName = deviceList_[m].name;
      break;
    }
  }

  if ( driverName.empty() ) {
    errorText_ = "RtApiAsio::probeDeviceOpen: device ID is invalid!";
    return FAILURE;
  }

  // Only load the driver once for duplex stream.
  ASIOError result;
  if ( !isDuplexInput ) {
    if ( streamOpen ) {
      errorText_ = "RtApiAsio::probeDeviceOpen: Another RtAudio ASIO stream is open, cannot open more than one at a time.";
      return FAILURE;
    }
    if ( !drivers.loadDriver( const_cast<char *>(driverName.c_str()) ) ) {
      errorStream_ << "RtApiAsio::probeDeviceOpen: unable to load driver (" << driverName << ").";
      errorText_ = errorStream_.str();
      return FAILURE;
    }

    result = ASIOInit( &driverInfo );
    if ( result != ASE_OK ) {
      drivers.removeCurrentDriver();
      errorStream_ << "RtApiAsio::probeDeviceOpen: error (" << getAsioErrorString( result ) << ") initializing driver (" << driverName << ").";
      errorText_ = errorStream_.str();
      return FAILURE;
    }
  }

  bool buffersAllocated = false;
  AsioHandle *handle = (AsioHandle *) stream_.apiHandle;
  unsigned int nChannels;

  // Check the device channel count.
  long inputChannels, outputChannels;
  result = ASIOGetChannels( &inputChannels, &outputChannels );
  if ( result != ASE_OK ) {
    errorStream_ << "RtApiAsio::probeDeviceOpen: error (" << getAsioErrorString( result ) << ") getting channel count (" << driverName << ").";
    errorText_ = errorStream_.str();
    goto error;
  }

  if ( ( mode == OUTPUT && (channels+firstChannel) > (unsigned int) outputChannels) ||
       ( mode == INPUT && (channels+firstChannel) > (unsigned int) inputChannels) ) {
    errorStream_ << "RtApiAsio::probeDeviceOpen: driver (" << driverName << ") does not support requested channel count (" << channels << ") + offset (" << firstChannel << ").";
    errorText_ = errorStream_.str();
    goto error;
  }
  stream_.nDeviceChannels[mode] = channels;
  stream_.nUserChannels[mode] = channels;
  stream_.channelOffset[mode] = firstChannel;

  // Verify the sample rate is supported.
  result = ASIOCanSampleRate( (ASIOSampleRate) sampleRate );
  if ( result != ASE_OK ) {
    errorStream_ << "RtApiAsio::probeDeviceOpen: driver (" << driverName << ") does not support requested sample rate (" << sampleRate << ").";
    errorText_ = errorStream_.str();
    goto error;
  }

  // Get the current sample rate
  ASIOSampleRate currentRate;
  result = ASIOGetSampleRate( &currentRate );
  if ( result != ASE_OK ) {
    errorStream_ << "RtApiAsio::probeDeviceOpen: driver (" << driverName << ") error getting sample rate.";
    errorText_ = errorStream_.str();
    goto error;
  }

  // Set the sample rate only if necessary
  if ( currentRate != sampleRate ) {
    result = ASIOSetSampleRate( (ASIOSampleRate) sampleRate );
    if ( result != ASE_OK ) {
      errorStream_ << "RtApiAsio::probeDeviceOpen: driver (" << driverName << ") error setting sample rate (" << sampleRate << ").";
      errorText_ = errorStream_.str();
      goto error;
    }
  }

  // Determine the driver data type.
  ASIOChannelInfo channelInfo;
  channelInfo.channel = 0;
  if ( mode == OUTPUT ) channelInfo.isInput = false;
  else channelInfo.isInput = true;
  result = ASIOGetChannelInfo( &channelInfo );
  if ( result != ASE_OK ) {
    errorStream_ << "RtApiAsio::probeDeviceOpen: driver (" << driverName << ") error (" << getAsioErrorString( result ) << ") getting data format.";
    errorText_ = errorStream_.str();
    goto error;
  }

  // Assuming WINDOWS host is always little-endian.
  stream_.doByteSwap[mode] = false;
  stream_.userFormat = format;
  stream_.deviceFormat[mode] = 0;
  if ( channelInfo.type == ASIOSTInt16MSB || channelInfo.type == ASIOSTInt16LSB ) {
    stream_.deviceFormat[mode] = RTAUDIO_SINT16;
    if ( channelInfo.type == ASIOSTInt16MSB ) stream_.doByteSwap[mode] = true;
  }
  else if ( channelInfo.type == ASIOSTInt32MSB || channelInfo.type == ASIOSTInt32LSB ) {
    stream_.deviceFormat[mode] = RTAUDIO_SINT32;
    if ( channelInfo.type == ASIOSTInt32MSB ) stream_.doByteSwap[mode] = true;
  }
  else if ( channelInfo.type == ASIOSTFloat32MSB || channelInfo.type == ASIOSTFloat32LSB ) {
    stream_.deviceFormat[mode] = RTAUDIO_FLOAT32;
    if ( channelInfo.type == ASIOSTFloat32MSB ) stream_.doByteSwap[mode] = true;
  }
  else if ( channelInfo.type == ASIOSTFloat64MSB || channelInfo.type == ASIOSTFloat64LSB ) {
    stream_.deviceFormat[mode] = RTAUDIO_FLOAT64;
    if ( channelInfo.type == ASIOSTFloat64MSB ) stream_.doByteSwap[mode] = true;
  }
  else if ( channelInfo.type == ASIOSTInt24MSB || channelInfo.type == ASIOSTInt24LSB ) {
    stream_.deviceFormat[mode] = RTAUDIO_SINT24;
    if ( channelInfo.type == ASIOSTInt24MSB ) stream_.doByteSwap[mode] = true;
  }

  if ( stream_.deviceFormat[mode] == 0 ) {
    errorStream_ << "RtApiAsio::probeDeviceOpen: driver (" << driverName << ") data format not supported by RtAudio.";
    errorText_ = errorStream_.str();
    goto error;
  }

  // Set the buffer size.  For a duplex stream, this will end up
  // setting the buffer size based on the input constraints, which
  // should be ok.
  long minSize, maxSize, preferSize, granularity;
  result = ASIOGetBufferSize( &minSize, &maxSize, &preferSize, &granularity );
  if ( result != ASE_OK ) {
    errorStream_ << "RtApiAsio::probeDeviceOpen: driver (" << driverName << ") error (" << getAsioErrorString( result ) << ") getting buffer size.";
    errorText_ = errorStream_.str();
    goto error;
  }

  if ( isDuplexInput ) {
    // When this is the duplex input (output was opened before), then we have to use the same
    // buffersize as the output, because it might use the preferred buffer size, which most
    // likely wasn't passed as input to this. The buffer sizes have to be identically anyway,
    // So instead of throwing an error, make them equal. The caller uses the reference
    // to the "bufferSize" param as usual to set up processing buffers.

    *bufferSize = stream_.bufferSize;

  } else {
    if ( *bufferSize == 0 ) *bufferSize = preferSize;
    else if ( *bufferSize < (unsigned int) minSize ) *bufferSize = (unsigned int) minSize;
    else if ( *bufferSize > (unsigned int) maxSize ) *bufferSize = (unsigned int) maxSize;
    else if ( granularity == -1 ) {
      // Make sure bufferSize is a power of two.
      int log2_of_min_size = 0;
      int log2_of_max_size = 0;

      for ( unsigned int i = 0; i < sizeof(long) * 8; i++ ) {
        if ( minSize & ((long)1 << i) ) log2_of_min_size = i;
        if ( maxSize & ((long)1 << i) ) log2_of_max_size = i;
      }

      long min_delta = std::abs( (long)*bufferSize - ((long)1 << log2_of_min_size) );
      int min_delta_num = log2_of_min_size;

      for (int i = log2_of_min_size + 1; i <= log2_of_max_size; i++) {
        long current_delta = std::abs( (long)*bufferSize - ((long)1 << i) );
        if (current_delta < min_delta) {
          min_delta = current_delta;
          min_delta_num = i;
        }
      }

      *bufferSize = ( (unsigned int)1 << min_delta_num );
      if ( *bufferSize < (unsigned int) minSize ) *bufferSize = (unsigned int) minSize;
      else if ( *bufferSize > (unsigned int) maxSize ) *bufferSize = (unsigned int) maxSize;
    }
    else if ( granularity != 0 ) {
      // Set to an even multiple of granularity, rounding up.
      *bufferSize = (*bufferSize + granularity-1) / granularity * granularity;
    }
  }

  /*
  // we don't use it anymore, see above!
  // Just left it here for the case...
  if ( isDuplexInput && stream_.bufferSize != *bufferSize ) {
    errorText_ = "RtApiAsio::probeDeviceOpen: input/output buffersize discrepancy!";
    goto error;
  }
  */

  stream_.bufferSize = *bufferSize;
  stream_.nBuffers = 2;

  if ( options && options->flags & RTAUDIO_NONINTERLEAVED ) stream_.userInterleaved = false;
  else stream_.userInterleaved = true;

  // ASIO always uses non-interleaved buffers.
  stream_.deviceInterleaved[mode] = false;

  // Allocate, if necessary, our AsioHandle structure for the stream.
  if ( handle == 0 ) {
    try {
      handle = new AsioHandle;
    }
    catch ( std::bad_alloc& ) {
      errorText_ = "RtApiAsio::probeDeviceOpen: error allocating AsioHandle memory.";
      goto error;
    }
    handle->bufferInfos = 0;

    // Create a manual-reset event.
    handle->condition = CreateEvent( NULL,   // no security
                                     TRUE,   // manual-reset
                                     FALSE,  // non-signaled initially
                                     NULL ); // unnamed
    stream_.apiHandle = (void *) handle;
  }

  // Create the ASIO internal buffers.  Since RtAudio sets up input
  // and output separately, we'll have to dispose of previously
  // created output buffers for a duplex stream.
  if ( mode == INPUT && stream_.mode == OUTPUT ) {
    ASIODisposeBuffers();
    if ( handle->bufferInfos ) free( handle->bufferInfos );
  }

  // Allocate, initialize, and save the bufferInfos in our stream callbackInfo structure.
  unsigned int i;
  nChannels = stream_.nDeviceChannels[0] + stream_.nDeviceChannels[1];
  handle->bufferInfos = (ASIOBufferInfo *) malloc( nChannels * sizeof(ASIOBufferInfo) );
  if ( handle->bufferInfos == NULL ) {
    errorStream_ << "RtApiAsio::probeDeviceOpen: error allocating bufferInfo memory for driver (" << driverName << ").";
    errorText_ = errorStream_.str();
    goto error;
  }

  ASIOBufferInfo *infos;
  infos = handle->bufferInfos;
  for ( i=0; i<stream_.nDeviceChannels[0]; i++, infos++ ) {
    infos->isInput = ASIOFalse;
    infos->channelNum = i + stream_.channelOffset[0];
    infos->buffers[0] = infos->buffers[1] = 0;
  }
  for ( i=0; i<stream_.nDeviceChannels[1]; i++, infos++ ) {
    infos->isInput = ASIOTrue;
    infos->channelNum = i + stream_.channelOffset[1];
    infos->buffers[0] = infos->buffers[1] = 0;
  }

  // prepare for callbacks
  stream_.sampleRate = sampleRate;
  stream_.deviceId[mode] = deviceId;
  stream_.mode = isDuplexInput ? DUPLEX : mode;

  // store this class instance before registering callbacks, that are going to use it
  asioCallbackInfo = &stream_.callbackInfo;
  stream_.callbackInfo.object = (void *) this;

  // Set up the ASIO callback structure and create the ASIO data buffers.
  asioCallbacks.bufferSwitch = &bufferSwitch;
  asioCallbacks.sampleRateDidChange = &sampleRateChanged;
  asioCallbacks.asioMessage = &asioMessages;
  asioCallbacks.bufferSwitchTimeInfo = NULL;
  result = ASIOCreateBuffers( handle->bufferInfos, nChannels, stream_.bufferSize, &asioCallbacks );
  if ( result != ASE_OK ) {
    // Standard method failed. This can happen with strict/misbehaving drivers that return valid buffer size ranges
    // but only accept the preferred buffer size as parameter for ASIOCreateBuffers (e.g. Creative's ASIO driver).
    // In that case, let's be naïve and try that instead.
    *bufferSize = preferSize;
    stream_.bufferSize = *bufferSize;
    result = ASIOCreateBuffers( handle->bufferInfos, nChannels, stream_.bufferSize, &asioCallbacks );
  }

  if ( result != ASE_OK ) {
    errorStream_ << "RtApiAsio::probeDeviceOpen: driver (" << driverName << ") error (" << getAsioErrorString( result ) << ") creating buffers.";
    errorText_ = errorStream_.str();
    goto error;
  }
  buffersAllocated = true;  
  stream_.state = STREAM_STOPPED;

  // Set flags for buffer conversion.
  stream_.doConvertBuffer[mode] = false;
  if ( stream_.userFormat != stream_.deviceFormat[mode] )
    stream_.doConvertBuffer[mode] = true;
  if ( stream_.userInterleaved != stream_.deviceInterleaved[mode] &&
       stream_.nUserChannels[mode] > 1 )
    stream_.doConvertBuffer[mode] = true;

  // Allocate necessary internal buffers
  unsigned long bufferBytes;
  bufferBytes = stream_.nUserChannels[mode] * *bufferSize * formatBytes( stream_.userFormat );
  stream_.userBuffer[mode] = (char *) calloc( bufferBytes, 1 );
  if ( stream_.userBuffer[mode] == NULL ) {
    errorText_ = "RtApiAsio::probeDeviceOpen: error allocating user buffer memory.";
    goto error;
  }

  if ( stream_.doConvertBuffer[mode] ) {

    bool makeBuffer = true;
    bufferBytes = stream_.nDeviceChannels[mode] * formatBytes( stream_.deviceFormat[mode] );
    if ( isDuplexInput && stream_.deviceBuffer ) {
      unsigned long bytesOut = stream_.nDeviceChannels[0] * formatBytes( stream_.deviceFormat[0] );
      if ( bufferBytes <= bytesOut ) makeBuffer = false;
    }

    if ( makeBuffer ) {
      bufferBytes *= *bufferSize;
      if ( stream_.deviceBuffer ) free( stream_.deviceBuffer );
      stream_.deviceBuffer = (char *) calloc( bufferBytes, 1 );
      if ( stream_.deviceBuffer == NULL ) {
        errorText_ = "RtApiAsio::probeDeviceOpen: error allocating device buffer memory.";
        goto error;
      }
    }
  }

  // Determine device latencies
  long inputLatency, outputLatency;
  result = ASIOGetLatencies( &inputLatency, &outputLatency );
  if ( result != ASE_OK ) {
    errorStream_ << "RtApiAsio::probeDeviceOpen: driver (" << driverName << ") error (" << getAsioErrorString( result ) << ") getting latency.";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING); // warn but don't fail
  }
  else {
    stream_.latency[0] = outputLatency;
    stream_.latency[1] = inputLatency;
  }

  // Setup the buffer conversion information structure.  We don't use
  // buffers to do channel offsets, so we override that parameter
  // here.
  if ( stream_.doConvertBuffer[mode] ) setConvertInfo( mode, 0 );

  streamOpen = true;
  return SUCCESS;

 error:
  if ( !isDuplexInput ) {
    // the cleanup for error in the duplex input, is done by RtApi::openStream
    // So we clean up for single channel only

    if ( buffersAllocated )
      ASIODisposeBuffers();

    ASIOExit();
    drivers.removeCurrentDriver();

    if ( handle ) {
      CloseHandle( handle->condition );
      if ( handle->bufferInfos )
        free( handle->bufferInfos );

      delete handle;
      stream_.apiHandle = 0;
    }


    if ( stream_.userBuffer[mode] ) {
      free( stream_.userBuffer[mode] );
      stream_.userBuffer[mode] = 0;
    }

    if ( stream_.deviceBuffer ) {
      free( stream_.deviceBuffer );
      stream_.deviceBuffer = 0;
    }
  }

  return FAILURE;
}

void RtApiAsio :: closeStream()
{
  if ( stream_.state == STREAM_CLOSED ) {
    errorText_ = "RtApiAsio::closeStream(): no open stream to close!";
    error( RTAUDIO_WARNING );
    return;
  }

  if ( stream_.state == STREAM_RUNNING ) {
    stream_.state = STREAM_STOPPED;
    ASIOStop();
  }

  stream_.state = STREAM_CLOSED;
  CallbackInfo *info = (CallbackInfo *) &stream_.callbackInfo;
  if ( info->deviceDisconnected ) {
    // This could be either a disconnect or a sample rate change.
    errorText_ = "RtApiAsio: the streaming device was disconnected or the sample rate changed, closing stream!";
    error( RTAUDIO_DEVICE_DISCONNECT );
  }
  
  ASIODisposeBuffers();
  ASIOExit();
  drivers.removeCurrentDriver();

  AsioHandle *handle = (AsioHandle *) stream_.apiHandle;
  if ( handle ) {
    CloseHandle( handle->condition );
    if ( handle->bufferInfos )
      free( handle->bufferInfos );
    delete handle;
    stream_.apiHandle = 0;
  }

  for ( int i=0; i<2; i++ ) {
    if ( stream_.userBuffer[i] ) {
      free( stream_.userBuffer[i] );
      stream_.userBuffer[i] = 0;
    }
  }

  if ( stream_.deviceBuffer ) {
    free( stream_.deviceBuffer );
    stream_.deviceBuffer = 0;
  }
  
  clearStreamInfo();
  streamOpen = false;
  //stream_.mode = UNINITIALIZED;
  //stream_.state = STREAM_CLOSED;
}

RtAudioErrorType RtApiAsio :: startStream()
{
    if ( stream_.state != STREAM_STOPPED ) {
    if ( stream_.state == STREAM_RUNNING )
      errorText_ = "RtApiAsio::startStream(): the stream is already running!";
    else if ( stream_.state == STREAM_STOPPING || stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiAsio::startStream(): the stream is stopping or closed!";
    return error( RTAUDIO_WARNING );
  }

  /*
  #if defined( HAVE_GETTIMEOFDAY )
  gettimeofday( &stream_.lastTickTimestamp, NULL );
  #endif
  */

  AsioHandle *handle = (AsioHandle *) stream_.apiHandle;
  ASIOError result = ASIOStart();
  if ( result != ASE_OK ) {
    errorStream_ << "RtApiAsio::startStream: error (" << getAsioErrorString( result ) << ") starting device.";
    errorText_ = errorStream_.str();
    goto unlock;
  }

  handle->drainCounter = 0;
  handle->internalDrain = false;
  ResetEvent( handle->condition );
  stream_.state = STREAM_RUNNING;
  asioXRun = false;

 unlock:
  if ( result == ASE_OK ) return RTAUDIO_NO_ERROR;
  return error( RTAUDIO_SYSTEM_ERROR );
}

RtAudioErrorType RtApiAsio :: stopStream()
{
  if ( stream_.state != STREAM_RUNNING && stream_.state != STREAM_STOPPING ) {
    if ( stream_.state == STREAM_STOPPED )
      errorText_ = "RtApiAsio::stopStream(): the stream is already stopped!";
    else if ( stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiAsio::stopStream(): the stream is closed!";
    return error( RTAUDIO_WARNING );
  }

  AsioHandle *handle = (AsioHandle *) stream_.apiHandle;
  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {
    if ( handle->drainCounter == 0 ) {
      handle->drainCounter = 2;
      WaitForSingleObject( handle->condition, INFINITE );  // block until signaled
    }
  }

  stream_.state = STREAM_STOPPED;

  ASIOError result = ASIOStop();
  if ( result != ASE_OK ) {
    errorStream_ << "RtApiAsio::stopStream: error (" << getAsioErrorString( result ) << ") stopping device.";
    errorText_ = errorStream_.str();
  }

  if ( result == ASE_OK ) return RTAUDIO_NO_ERROR;
  return error( RTAUDIO_SYSTEM_ERROR );
}

RtAudioErrorType RtApiAsio :: abortStream()
{
  if ( stream_.state != STREAM_RUNNING ) {
    if ( stream_.state == STREAM_STOPPED )
      errorText_ = "RtApiAsio::abortStream(): the stream is already stopped!";
    else if ( stream_.state == STREAM_STOPPING || stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiAsio::abortStream(): the stream is stopping or closed!";
    return error( RTAUDIO_WARNING );
  }

  // The following lines were commented-out because some behavior was
  // noted where the device buffers need to be zeroed to avoid
  // continuing sound, even when the device buffers are completely
  // disposed.  So now, calling abort is the same as calling stop.
  // AsioHandle *handle = (AsioHandle *) stream_.apiHandle;
  // handle->drainCounter = 2;
  stopStream();
  return RTAUDIO_NO_ERROR;
}

// This function will be called by a spawned thread when: 1. The user
// callback function signals that the stream should be stopped or
// aborted; or 2. When a signal is received indicating that the device
// sample rate has changed or it has been disconnected.  It is
// necessary to handle it this way because the callbackEvent() or
// signaling function must return before the ASIOStop() function will
// return (or the driver can be removed).
static unsigned __stdcall asioStopStream( void *ptr )
{
  CallbackInfo *info = (CallbackInfo *) ptr;
  RtApiAsio *object = (RtApiAsio *) info->object;

  if ( info->deviceDisconnected == false )
    object->stopStream(); // drain the stream
  else
    object->closeStream(); // disconnect or sample rate change ... close the stream

  _endthreadex( 0 );
  return 0;
}

bool RtApiAsio :: callbackEvent( long bufferIndex )
{
  if ( stream_.state == STREAM_STOPPED || stream_.state == STREAM_STOPPING ) return SUCCESS;
  if ( stream_.state == STREAM_CLOSED ) {
    errorText_ = "RtApiAsio::callbackEvent(): the stream is closed ... this shouldn't happen!";
    error( RTAUDIO_WARNING );
    return FAILURE;
  }

  CallbackInfo *info = (CallbackInfo *) &stream_.callbackInfo;
  AsioHandle *handle = (AsioHandle *) stream_.apiHandle;

  // Check if we were draining the stream and signal if finished.
  if ( handle->drainCounter > 3 ) {

    stream_.state = STREAM_STOPPING;
    if ( handle->internalDrain == false )
      SetEvent( handle->condition );
    else { // spawn a thread to stop the stream
      unsigned threadId;
      stream_.callbackInfo.thread = _beginthreadex( NULL, 0, &asioStopStream,
                                                    &stream_.callbackInfo, 0, &threadId );
    }
    return SUCCESS;
  }

  // Invoke user callback to get fresh output data UNLESS we are
  // draining stream.
  if ( handle->drainCounter == 0 ) {
    RtAudioCallback callback = (RtAudioCallback) info->callback;
    double streamTime = getStreamTime();
    RtAudioStreamStatus status = 0;
    if ( stream_.mode != INPUT && asioXRun == true ) {
      status |= RTAUDIO_OUTPUT_UNDERFLOW;
      asioXRun = false;
    }
    if ( stream_.mode != OUTPUT && asioXRun == true ) {
      status |= RTAUDIO_INPUT_OVERFLOW;
      asioXRun = false;
    }
    int cbReturnValue = callback( stream_.userBuffer[0], stream_.userBuffer[1],
                                     stream_.bufferSize, streamTime, status, info->userData );
    if ( cbReturnValue == 2 ) {
      stream_.state = STREAM_STOPPING;
      handle->drainCounter = 2;
      unsigned threadId;
      stream_.callbackInfo.thread = _beginthreadex( NULL, 0, &asioStopStream,
                                                    &stream_.callbackInfo, 0, &threadId );
      return SUCCESS;
    }
    else if ( cbReturnValue == 1 ) {
      handle->drainCounter = 1;
      handle->internalDrain = true;
    }
  }

  unsigned int nChannels, bufferBytes, i, j;
  nChannels = stream_.nDeviceChannels[0] + stream_.nDeviceChannels[1];
  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {

    bufferBytes = stream_.bufferSize * formatBytes( stream_.deviceFormat[0] );

    if ( handle->drainCounter > 1 ) { // write zeros to the output stream

      for ( i=0, j=0; i<nChannels; i++ ) {
        if ( handle->bufferInfos[i].isInput != ASIOTrue )
          memset( handle->bufferInfos[i].buffers[bufferIndex], 0, bufferBytes );
      }

    }
    else if ( stream_.doConvertBuffer[0] ) {

      convertBuffer( stream_.deviceBuffer, stream_.userBuffer[0], stream_.convertInfo[0] );
      if ( stream_.doByteSwap[0] )
        byteSwapBuffer( stream_.deviceBuffer,
                        stream_.bufferSize * stream_.nDeviceChannels[0],
                        stream_.deviceFormat[0] );

      for ( i=0, j=0; i<nChannels; i++ ) {
        if ( handle->bufferInfos[i].isInput != ASIOTrue )
          memcpy( handle->bufferInfos[i].buffers[bufferIndex],
                  &stream_.deviceBuffer[j++*bufferBytes], bufferBytes );
      }

    }
    else {

      if ( stream_.doByteSwap[0] )
        byteSwapBuffer( stream_.userBuffer[0],
                        stream_.bufferSize * stream_.nUserChannels[0],
                        stream_.userFormat );

      for ( i=0, j=0; i<nChannels; i++ ) {
        if ( handle->bufferInfos[i].isInput != ASIOTrue )
          memcpy( handle->bufferInfos[i].buffers[bufferIndex],
                  &stream_.userBuffer[0][bufferBytes*j++], bufferBytes );
      }

    }
  }

  // Don't bother draining input
  if ( handle->drainCounter ) {
    handle->drainCounter++;
    goto unlock;
  }

  if ( stream_.mode == INPUT || stream_.mode == DUPLEX ) {

    bufferBytes = stream_.bufferSize * formatBytes(stream_.deviceFormat[1]);

    if (stream_.doConvertBuffer[1]) {

      // Always interleave ASIO input data.
      for ( i=0, j=0; i<nChannels; i++ ) {
        if ( handle->bufferInfos[i].isInput == ASIOTrue )
          memcpy( &stream_.deviceBuffer[j++*bufferBytes],
                  handle->bufferInfos[i].buffers[bufferIndex],
                  bufferBytes );
      }

      if ( stream_.doByteSwap[1] )
        byteSwapBuffer( stream_.deviceBuffer,
                        stream_.bufferSize * stream_.nDeviceChannels[1],
                        stream_.deviceFormat[1] );
      convertBuffer( stream_.userBuffer[1], stream_.deviceBuffer, stream_.convertInfo[1] );

    }
    else {
      for ( i=0, j=0; i<nChannels; i++ ) {
        if ( handle->bufferInfos[i].isInput == ASIOTrue ) {
          memcpy( &stream_.userBuffer[1][bufferBytes*j++],
                  handle->bufferInfos[i].buffers[bufferIndex],
                  bufferBytes );
        }
      }

      if ( stream_.doByteSwap[1] )
        byteSwapBuffer( stream_.userBuffer[1],
                        stream_.bufferSize * stream_.nUserChannels[1],
                        stream_.userFormat );
    }
  }

 unlock:
  // The following call was suggested by Malte Clasen.  While the API
  // documentation indicates it should not be required, some device
  // drivers apparently do not function correctly without it.
  ASIOOutputReady();

  RtApi::tickStreamTime();
  return SUCCESS;
}

static void sampleRateChanged( ASIOSampleRate sRate )
{
  // The ASIO documentation says that this usually only happens during
  // external sync.  Audio processing is not stopped by the driver,
  // actual sample rate might not have even changed, maybe only the
  // sample rate status of an AES/EBU or S/PDIF digital input at the
  // audio device.

  RtApi *object = (RtApi *) asioCallbackInfo->object;
  if ( object->getStreamSampleRate() != sRate ) {
    asioCallbackInfo->deviceDisconnected = true; // flag for either rate change or disconnect
    unsigned threadId;
    asioCallbackInfo->thread = _beginthreadex( NULL, 0, &asioStopStream,
                                               asioCallbackInfo, 0, &threadId );
  }
}

static long asioMessages( long selector, long value, void* /*message*/, double* /*opt*/ )
{
  long ret = 0;
  
  switch( selector ) {
  case kAsioSelectorSupported:
    if ( value == kAsioResetRequest
         || value == kAsioEngineVersion
         || value == kAsioResyncRequest
         || value == kAsioLatenciesChanged
         // The following three were added for ASIO 2.0, you don't
         // necessarily have to support them.
         || value == kAsioSupportsTimeInfo
         || value == kAsioSupportsTimeCode
         || value == kAsioSupportsInputMonitor)
      ret = 1L;
    break;
  case kAsioResetRequest:
    // This message is received when a device is disconnected (and
    // perhaps when the sample rate changes). It indicates that the
    // driver should be reset, which is accomplished by calling
    // ASIOStop(), ASIODisposeBuffers() and removing the driver. But
    // since this message comes from the driver, we need to let this
    // function return before attempting to close the stream and
    // remove the driver. Thus, we invoke a thread to initiate the
    // stream closing.
    asioCallbackInfo->deviceDisconnected = true; // flag for either rate change or disconnect
    unsigned threadId;
    asioCallbackInfo->thread = _beginthreadex( NULL, 0, &asioStopStream,
                                               asioCallbackInfo, 0, &threadId );
    //std::cerr << "\nRtApiAsio: driver reset requested!!!" << std::endl;
    ret = 1L;
    break;
  case kAsioResyncRequest:
    // This informs the application that the driver encountered some
    // non-fatal data loss.  It is used for synchronization purposes
    // of different media.  Added mainly to work around the Win16Mutex
    // problems in Windows 95/98 with the Windows Multimedia system,
    // which could lose data because the Mutex was held too long by
    // another thread.  However a driver can issue it in other
    // situations, too.
    // std::cerr << "\nRtApiAsio: driver resync requested!!!" << std::endl;
    asioXRun = true;
    ret = 1L;
    break;
  case kAsioLatenciesChanged:
    // This will inform the host application that the drivers were
    // latencies changed.  Beware, it this does not mean that the
    // buffer sizes have changed!  You might need to update internal
    // delay data.
    std::cerr << "\nRtApiAsio: driver latency may have changed!!!" << std::endl;
    ret = 1L;
    break;
  case kAsioEngineVersion:
    // Return the supported ASIO version of the host application.  If
    // a host application does not implement this selector, ASIO 1.0
    // is assumed by the driver.
    ret = 2L;
    break;
  case kAsioSupportsTimeInfo:
    // Informs the driver whether the
    // asioCallbacks.bufferSwitchTimeInfo() callback is supported.
    // For compatibility with ASIO 1.0 drivers the host application
    // should always support the "old" bufferSwitch method, too.
    ret = 0;
    break;
  case kAsioSupportsTimeCode:
    // Informs the driver whether application is interested in time
    // code info.  If an application does not need to know about time
    // code, the driver has less work to do.
    ret = 0;
    break;
  }
  return ret;
}

static const char* getAsioErrorString( ASIOError result )
{
  struct Messages 
  {
    ASIOError value;
    const char*message;
  };

  static const Messages m[] = 
    {
      {   ASE_NotPresent,    "Hardware input or output is not present or available." },
      {   ASE_HWMalfunction,  "Hardware is malfunctioning." },
      {   ASE_InvalidParameter, "Invalid input parameter." },
      {   ASE_InvalidMode,      "Invalid mode." },
      {   ASE_SPNotAdvancing,     "Sample position not advancing." },
      {   ASE_NoClock,            "Sample clock or rate cannot be determined or is not present." },
      {   ASE_NoMemory,           "Not enough memory to complete the request." }
    };

  for ( unsigned int i = 0; i < sizeof(m)/sizeof(m[0]); ++i )
    if ( m[i].value == result ) return m[i].message;

  return "Unknown error.";
}

//******************** End of __WINDOWS_ASIO__ *********************//
#endif


#if defined(__WINDOWS_WASAPI__) // Windows WASAPI API

// Authored by Marcus Tomlinson <themarcustomlinson@gmail.com>, April 2014
// Updates for new device selection scheme by Gary Scavone, January 2022
// - Introduces support for the Windows WASAPI API
// - Aims to deliver bit streams to and from hardware at the lowest possible latency, via the absolute minimum buffer sizes required
// - Provides flexible stream configuration to an otherwise strict and inflexible WASAPI interface
// - Includes automatic internal conversion of sample rate and buffer size between hardware and the user

#ifndef INITGUID
  #define INITGUID
#endif

#include <mfapi.h>
#include <mferror.h>
#include <mfplay.h>
#include <mftransform.h>
#include <wmcodecdsp.h>

#include <audioclient.h>
#include <avrt.h>
#include <mmdeviceapi.h>
#include <functiondiscoverykeys_devpkey.h>

#ifndef MF_E_TRANSFORM_NEED_MORE_INPUT
  #define MF_E_TRANSFORM_NEED_MORE_INPUT _HRESULT_TYPEDEF_(0xc00d6d72)
#endif

#ifndef MFSTARTUP_NOSOCKET
  #define MFSTARTUP_NOSOCKET 0x1
#endif

#ifdef _MSC_VER
  #pragma comment( lib, "ksuser" )
  #pragma comment( lib, "mfplat.lib" )
  #pragma comment( lib, "mfuuid.lib" )
  #pragma comment( lib, "wmcodecdspuuid" )
#endif

//=============================================================================

#define SAFE_RELEASE( objectPtr )\
if ( objectPtr )\
{\
  objectPtr->Release();\
  objectPtr = NULL;\
}

typedef HANDLE ( __stdcall *TAvSetMmThreadCharacteristicsPtr )( LPCWSTR TaskName, LPDWORD TaskIndex );

#ifndef __IAudioClient3_INTERFACE_DEFINED__
MIDL_INTERFACE( "00000000-0000-0000-0000-000000000000" ) IAudioClient3
{
  virtual HRESULT GetSharedModeEnginePeriod( WAVEFORMATEX*, UINT32*, UINT32*, UINT32*, UINT32* ) = 0;
  virtual HRESULT InitializeSharedAudioStream( DWORD, UINT32, WAVEFORMATEX*, LPCGUID ) = 0;
  virtual HRESULT Release() = 0;
};
#ifdef __CRT_UUID_DECL
__CRT_UUID_DECL( IAudioClient3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 )
#endif
#endif

//-----------------------------------------------------------------------------

// WASAPI dictates stream sample rate, format, channel count, and in some cases, buffer size.
// Therefore we must perform all necessary conversions to user buffers in order to satisfy these
// requirements. WasapiBuffer ring buffers are used between HwIn->UserIn and UserOut->HwOut to
// provide intermediate storage for read / write synchronization.
class WasapiBuffer
{
public:
  WasapiBuffer()
    : buffer_( NULL ),
      bufferSize_( 0 ),
      inIndex_( 0 ),
      outIndex_( 0 ) {}

  ~WasapiBuffer() {
    free( buffer_ );
  }

  // sets the length of the internal ring buffer
  void setBufferSize( unsigned int bufferSize, unsigned int formatBytes ) {
    free( buffer_ );

    buffer_ = ( char* ) calloc( bufferSize, formatBytes );

    bufferSize_ = bufferSize;
    inIndex_ = 0;
    outIndex_ = 0;
  }

  // attempt to push a buffer into the ring buffer at the current "in" index
  bool pushBuffer( char* buffer, unsigned int bufferSize, RtAudioFormat format )
  {
    if ( !buffer ||                 // incoming buffer is NULL
         bufferSize == 0 ||         // incoming buffer has no data
         bufferSize > bufferSize_ ) // incoming buffer too large
    {
      return false;
    }

    unsigned int relOutIndex = outIndex_;
    unsigned int inIndexEnd = inIndex_ + bufferSize;
    if ( relOutIndex < inIndex_ && inIndexEnd >= bufferSize_ ) {
      relOutIndex += bufferSize_;
    }

    // the "IN" index CAN BEGIN at the "OUT" index
    // the "IN" index CANNOT END at the "OUT" index
    if ( inIndex_ < relOutIndex && inIndexEnd >= relOutIndex ) {
      return false; // not enough space between "in" index and "out" index
    }

    // copy buffer from external to internal
    int fromZeroSize = inIndex_ + bufferSize - bufferSize_;
    fromZeroSize = fromZeroSize < 0 ? 0 : fromZeroSize;
    int fromInSize = bufferSize - fromZeroSize;

    switch( format )
      {
      case RTAUDIO_SINT8:
        memcpy( &( ( char* ) buffer_ )[inIndex_], buffer, fromInSize * sizeof( char ) );
        memcpy( buffer_, &( ( char* ) buffer )[fromInSize], fromZeroSize * sizeof( char ) );
        break;
      case RTAUDIO_SINT16:
        memcpy( &( ( short* ) buffer_ )[inIndex_], buffer, fromInSize * sizeof( short ) );
        memcpy( buffer_, &( ( short* ) buffer )[fromInSize], fromZeroSize * sizeof( short ) );
        break;
      case RTAUDIO_SINT24:
        memcpy( &( ( S24* ) buffer_ )[inIndex_], buffer, fromInSize * sizeof( S24 ) );
        memcpy( buffer_, &( ( S24* ) buffer )[fromInSize], fromZeroSize * sizeof( S24 ) );
        break;
      case RTAUDIO_SINT32:
        memcpy( &( ( int* ) buffer_ )[inIndex_], buffer, fromInSize * sizeof( int ) );
        memcpy( buffer_, &( ( int* ) buffer )[fromInSize], fromZeroSize * sizeof( int ) );
        break;
      case RTAUDIO_FLOAT32:
        memcpy( &( ( float* ) buffer_ )[inIndex_], buffer, fromInSize * sizeof( float ) );
        memcpy( buffer_, &( ( float* ) buffer )[fromInSize], fromZeroSize * sizeof( float ) );
        break;
      case RTAUDIO_FLOAT64:
        memcpy( &( ( double* ) buffer_ )[inIndex_], buffer, fromInSize * sizeof( double ) );
        memcpy( buffer_, &( ( double* ) buffer )[fromInSize], fromZeroSize * sizeof( double ) );
        break;
    }

    // update "in" index
    inIndex_ += bufferSize;
    inIndex_ %= bufferSize_;

    return true;
  }

  // attempt to pull a buffer from the ring buffer from the current "out" index
  bool pullBuffer( char* buffer, unsigned int bufferSize, RtAudioFormat format )
  {
    if ( !buffer ||                 // incoming buffer is NULL
         bufferSize == 0 ||         // incoming buffer has no data
         bufferSize > bufferSize_ ) // incoming buffer too large
    {
      return false;
    }

    unsigned int relInIndex = inIndex_;
    unsigned int outIndexEnd = outIndex_ + bufferSize;
    if ( relInIndex < outIndex_ && outIndexEnd >= bufferSize_ ) {
      relInIndex += bufferSize_;
    }

    // the "OUT" index CANNOT BEGIN at the "IN" index
    // the "OUT" index CAN END at the "IN" index
    if ( outIndex_ <= relInIndex && outIndexEnd > relInIndex ) {
      return false; // not enough space between "out" index and "in" index
    }

    // copy buffer from internal to external
    int fromZeroSize = outIndex_ + bufferSize - bufferSize_;
    fromZeroSize = fromZeroSize < 0 ? 0 : fromZeroSize;
    int fromOutSize = bufferSize - fromZeroSize;

    switch( format )
    {
      case RTAUDIO_SINT8:
        memcpy( buffer, &( ( char* ) buffer_ )[outIndex_], fromOutSize * sizeof( char ) );
        memcpy( &( ( char* ) buffer )[fromOutSize], buffer_, fromZeroSize * sizeof( char ) );
        break;
      case RTAUDIO_SINT16:
        memcpy( buffer, &( ( short* ) buffer_ )[outIndex_], fromOutSize * sizeof( short ) );
        memcpy( &( ( short* ) buffer )[fromOutSize], buffer_, fromZeroSize * sizeof( short ) );
        break;
      case RTAUDIO_SINT24:
        memcpy( buffer, &( ( S24* ) buffer_ )[outIndex_], fromOutSize * sizeof( S24 ) );
        memcpy( &( ( S24* ) buffer )[fromOutSize], buffer_, fromZeroSize * sizeof( S24 ) );
        break;
      case RTAUDIO_SINT32:
        memcpy( buffer, &( ( int* ) buffer_ )[outIndex_], fromOutSize * sizeof( int ) );
        memcpy( &( ( int* ) buffer )[fromOutSize], buffer_, fromZeroSize * sizeof( int ) );
        break;
      case RTAUDIO_FLOAT32:
        memcpy( buffer, &( ( float* ) buffer_ )[outIndex_], fromOutSize * sizeof( float ) );
        memcpy( &( ( float* ) buffer )[fromOutSize], buffer_, fromZeroSize * sizeof( float ) );
        break;
      case RTAUDIO_FLOAT64:
        memcpy( buffer, &( ( double* ) buffer_ )[outIndex_], fromOutSize * sizeof( double ) );
        memcpy( &( ( double* ) buffer )[fromOutSize], buffer_, fromZeroSize * sizeof( double ) );
        break;
    }

    // update "out" index
    outIndex_ += bufferSize;
    outIndex_ %= bufferSize_;

    return true;
  }

private:
  char* buffer_;
  unsigned int bufferSize_;
  unsigned int inIndex_;
  unsigned int outIndex_;
};

//-----------------------------------------------------------------------------

// In order to satisfy WASAPI's buffer requirements, we need a means of converting sample rate
// between HW and the user. The WasapiResampler class is used to perform this conversion between
// HwIn->UserIn and UserOut->HwOut during the stream callback loop.
class WasapiResampler
{
public:
  WasapiResampler( bool isFloat, unsigned int bitsPerSample, unsigned int channelCount,
                   unsigned int inSampleRate, unsigned int outSampleRate )
    : _bytesPerSample( bitsPerSample / 8 )
    , _channelCount( channelCount )
    , _sampleRatio( ( float ) outSampleRate / inSampleRate )
    , _transformUnk( NULL )
    , _transform( NULL )
    , _mediaType( NULL )
    , _inputMediaType( NULL )
    , _outputMediaType( NULL )

    #ifdef __IWMResamplerProps_FWD_DEFINED__
      , _resamplerProps( NULL )
    #endif
  {
    // 1. Initialization

    MFStartup( MF_VERSION, MFSTARTUP_NOSOCKET );

    // 2. Create Resampler Transform Object

    CoCreateInstance( CLSID_CResamplerMediaObject, NULL, CLSCTX_INPROC_SERVER,
                      IID_IUnknown, ( void** ) &_transformUnk );

    _transformUnk->QueryInterface( IID_PPV_ARGS( &_transform ) );

    #ifdef __IWMResamplerProps_FWD_DEFINED__
      _transformUnk->QueryInterface( IID_PPV_ARGS( &_resamplerProps ) );
      _resamplerProps->SetHalfFilterLength( 60 ); // best conversion quality
    #endif

    // 3. Specify input / output format

    MFCreateMediaType( &_mediaType );
    _mediaType->SetGUID( MF_MT_MAJOR_TYPE, MFMediaType_Audio );
    _mediaType->SetGUID( MF_MT_SUBTYPE, isFloat ? MFAudioFormat_Float : MFAudioFormat_PCM );
    _mediaType->SetUINT32( MF_MT_AUDIO_NUM_CHANNELS, channelCount );
    _mediaType->SetUINT32( MF_MT_AUDIO_SAMPLES_PER_SECOND, inSampleRate );
    _mediaType->SetUINT32( MF_MT_AUDIO_BLOCK_ALIGNMENT, _bytesPerSample * channelCount );
    _mediaType->SetUINT32( MF_MT_AUDIO_AVG_BYTES_PER_SECOND, _bytesPerSample * channelCount * inSampleRate );
    _mediaType->SetUINT32( MF_MT_AUDIO_BITS_PER_SAMPLE, bitsPerSample );
    _mediaType->SetUINT32( MF_MT_ALL_SAMPLES_INDEPENDENT, TRUE );

    MFCreateMediaType( &_inputMediaType );
    _mediaType->CopyAllItems( _inputMediaType );

    _transform->SetInputType( 0, _inputMediaType, 0 );

    MFCreateMediaType( &_outputMediaType );
    _mediaType->CopyAllItems( _outputMediaType );

    _outputMediaType->SetUINT32( MF_MT_AUDIO_SAMPLES_PER_SECOND, outSampleRate );
    _outputMediaType->SetUINT32( MF_MT_AUDIO_AVG_BYTES_PER_SECOND, _bytesPerSample * channelCount * outSampleRate );

    _transform->SetOutputType( 0, _outputMediaType, 0 );

    // 4. Send stream start messages to Resampler

    _transform->ProcessMessage( MFT_MESSAGE_COMMAND_FLUSH, 0 );
    _transform->ProcessMessage( MFT_MESSAGE_NOTIFY_BEGIN_STREAMING, 0 );
    _transform->ProcessMessage( MFT_MESSAGE_NOTIFY_START_OF_STREAM, 0 );
  }

  ~WasapiResampler()
  {
    // 8. Send stream stop messages to Resampler

    _transform->ProcessMessage( MFT_MESSAGE_NOTIFY_END_OF_STREAM, 0 );
    _transform->ProcessMessage( MFT_MESSAGE_NOTIFY_END_STREAMING, 0 );

    // 9. Cleanup

    MFShutdown();

    SAFE_RELEASE( _transformUnk );
    SAFE_RELEASE( _transform );
    SAFE_RELEASE( _mediaType );
    SAFE_RELEASE( _inputMediaType );
    SAFE_RELEASE( _outputMediaType );

    #ifdef __IWMResamplerProps_FWD_DEFINED__
      SAFE_RELEASE( _resamplerProps );
    #endif
  }

  void Convert( char* outBuffer, const char* inBuffer, unsigned int inSampleCount, unsigned int& outSampleCount, int maxOutSampleCount = -1 )
  {
    unsigned int inputBufferSize = _bytesPerSample * _channelCount * inSampleCount;
    if ( _sampleRatio == 1 )
    {
      // no sample rate conversion required
      memcpy( outBuffer, inBuffer, inputBufferSize );
      outSampleCount = inSampleCount;
      return;
    }

    unsigned int outputBufferSize = 0;
    if ( maxOutSampleCount != -1 )
    {
      outputBufferSize = _bytesPerSample * _channelCount * maxOutSampleCount;
    }
    else
    {
      outputBufferSize = ( unsigned int ) ceilf( inputBufferSize * _sampleRatio ) + ( _bytesPerSample * _channelCount );
    }

    IMFMediaBuffer* rInBuffer;
    IMFSample* rInSample;
    BYTE* rInByteBuffer = NULL;

    // 5. Create Sample object from input data

    MFCreateMemoryBuffer( inputBufferSize, &rInBuffer );

    rInBuffer->Lock( &rInByteBuffer, NULL, NULL );
    memcpy( rInByteBuffer, inBuffer, inputBufferSize );
    rInBuffer->Unlock();
    rInByteBuffer = NULL;

    rInBuffer->SetCurrentLength( inputBufferSize );

    MFCreateSample( &rInSample );
    rInSample->AddBuffer( rInBuffer );

    // 6. Pass input data to Resampler

    _transform->ProcessInput( 0, rInSample, 0 );

    SAFE_RELEASE( rInBuffer );
    SAFE_RELEASE( rInSample );

    // 7. Perform sample rate conversion

    IMFMediaBuffer* rOutBuffer = NULL;
    BYTE* rOutByteBuffer = NULL;

    MFT_OUTPUT_DATA_BUFFER rOutDataBuffer;
    DWORD rStatus;
    DWORD rBytes = outputBufferSize; // maximum bytes accepted per ProcessOutput

    // 7.1 Create Sample object for output data

    memset( &rOutDataBuffer, 0, sizeof rOutDataBuffer );
    MFCreateSample( &( rOutDataBuffer.pSample ) );
    MFCreateMemoryBuffer( rBytes, &rOutBuffer );
    rOutDataBuffer.pSample->AddBuffer( rOutBuffer );
    rOutDataBuffer.dwStreamID = 0;
    rOutDataBuffer.dwStatus = 0;
    rOutDataBuffer.pEvents = NULL;

    // 7.2 Get output data from Resampler

    if ( _transform->ProcessOutput( 0, 1, &rOutDataBuffer, &rStatus ) == MF_E_TRANSFORM_NEED_MORE_INPUT )
    {
      outSampleCount = 0;
      SAFE_RELEASE( rOutBuffer );
      SAFE_RELEASE( rOutDataBuffer.pSample );
      return;
    }

    // 7.3 Write output data to outBuffer

    SAFE_RELEASE( rOutBuffer );
    rOutDataBuffer.pSample->ConvertToContiguousBuffer( &rOutBuffer );
    rOutBuffer->GetCurrentLength( &rBytes );

    rOutBuffer->Lock( &rOutByteBuffer, NULL, NULL );
    memcpy( outBuffer, rOutByteBuffer, rBytes );
    rOutBuffer->Unlock();
    rOutByteBuffer = NULL;

    outSampleCount = rBytes / _bytesPerSample / _channelCount;
    SAFE_RELEASE( rOutBuffer );
    SAFE_RELEASE( rOutDataBuffer.pSample );
  }

private:
  unsigned int _bytesPerSample;
  unsigned int _channelCount;
  float _sampleRatio;

  IUnknown* _transformUnk;
  IMFTransform* _transform;
  IMFMediaType* _mediaType;
  IMFMediaType* _inputMediaType;
  IMFMediaType* _outputMediaType;

  #ifdef __IWMResamplerProps_FWD_DEFINED__
    IWMResamplerProps* _resamplerProps;
  #endif
};

//-----------------------------------------------------------------------------

// A structure to hold various information related to the WASAPI implementation.
struct WasapiHandle
{
  IAudioClient* captureAudioClient;
  IAudioClient* renderAudioClient;
  IAudioCaptureClient* captureClient;
  IAudioRenderClient* renderClient;
  HANDLE captureEvent;
  HANDLE renderEvent;

  WasapiHandle()
  : captureAudioClient( NULL ),
    renderAudioClient( NULL ),
    captureClient( NULL ),
    renderClient( NULL ),
    captureEvent( NULL ),
    renderEvent( NULL ) {}
};

//-----------------------------------------------------------------------------

RtApiWasapi::RtApiWasapi()
  : coInitialized_( false ), deviceEnumerator_( NULL )
{
  // WASAPI can run either apartment or multi-threaded
  HRESULT hr = CoInitialize( NULL );
  if ( !FAILED( hr ) )
    coInitialized_ = true;

  // Instantiate device enumerator
  hr = CoCreateInstance( __uuidof( MMDeviceEnumerator ), NULL,
                         CLSCTX_ALL, __uuidof( IMMDeviceEnumerator ),
                         ( void** ) &deviceEnumerator_ );

  // If this runs on an old Windows, it will fail. Ignore and proceed.
  if ( FAILED( hr ) )
    deviceEnumerator_ = NULL;
}

//-----------------------------------------------------------------------------

RtApiWasapi::~RtApiWasapi()
{
  MUTEX_LOCK( &stream_.mutex );
  if ( stream_.state != STREAM_CLOSED )
  {
    MUTEX_UNLOCK( &stream_.mutex );
    closeStream();
    MUTEX_LOCK( &stream_.mutex );
  }

  SAFE_RELEASE( deviceEnumerator_ );

  // If this object previously called CoInitialize()
  if ( coInitialized_ )
    CoUninitialize();
  MUTEX_UNLOCK( &stream_.mutex );
}

//-----------------------------------------------------------------------------

unsigned int RtApiWasapi::getDefaultInputDevice( void )
{
  IMMDevice* devicePtr = NULL;
  LPWSTR defaultId = NULL;
  std::string id;
  
  if ( !deviceEnumerator_ ) return 0; // invalid ID
  errorText_.clear();

  // Get the default capture device Id.
  HRESULT hr = deviceEnumerator_->GetDefaultAudioEndpoint( eCapture, eConsole, &devicePtr );
  if ( FAILED( hr ) ) {
    errorText_ = "RtApiWasapi::getDefaultInputDevice: Unable to retrieve default capture device handle.";
    goto Release;
  }

  hr = devicePtr->GetId( &defaultId );
  if ( FAILED( hr ) ) {
    errorText_ = "RtApiWasapi::getDefaultInputDevice: Unable to get default capture device Id.";
    goto Release;
  }
  id = convertCharPointerToStdString( defaultId );

 Release:
  SAFE_RELEASE( devicePtr );
  CoTaskMemFree( defaultId );

  if ( !errorText_.empty() ) {
    error( RTAUDIO_DRIVER_ERROR );
    return 0;
  }

  for ( unsigned int m=0; m<deviceIds_.size(); m++ ) {
    if ( deviceIds_[m].first == id ) {
      if ( deviceList_[m].isDefaultInput == false ) {
        deviceList_[m].isDefaultInput = true;
        for ( unsigned int j=m+1; j<deviceIds_.size(); j++ ) {
          // make sure any remaining devices are not listed as the default
          deviceList_[j].isDefaultInput = false;
        }
      }
      return deviceList_[m].ID;
    }
    deviceList_[m].isDefaultInput = false;
  }

  // If not found above, then do system probe of devices and try again.
  probeDevices();
  for ( unsigned int m=0; m<deviceIds_.size(); m++ ) {
    if ( deviceIds_[m].first == id ) return deviceList_[m].ID;
  }

  return 0;
}

//-----------------------------------------------------------------------------

unsigned int RtApiWasapi::getDefaultOutputDevice( void )
{
  IMMDevice* devicePtr = NULL;
  LPWSTR defaultId = NULL;
  std::string id;
  
  if ( !deviceEnumerator_ ) return 0; // invalid ID
  errorText_.clear();

  // Get the default render device Id.
  HRESULT hr = deviceEnumerator_->GetDefaultAudioEndpoint( eRender, eConsole, &devicePtr );
  if ( FAILED( hr ) ) {
    errorText_ = "RtApiWasapi::getDefaultOutputDevice: Unable to retrieve default render device handle.";
    goto Release;
  }

  hr = devicePtr->GetId( &defaultId );
  if ( FAILED( hr ) ) {
    errorText_ = "RtApiWasapi::getDefaultOutputDevice: Unable to get default render device Id.";
    goto Release;
  }
  id = convertCharPointerToStdString( defaultId );

 Release:
  SAFE_RELEASE( devicePtr );
  CoTaskMemFree( defaultId );

  if ( !errorText_.empty() ) {
    error( RTAUDIO_DRIVER_ERROR );
    return 0;
  }

  for ( unsigned int m=0; m<deviceIds_.size(); m++ ) {
    if ( deviceIds_[m].first == id ) {
      if ( deviceList_[m].isDefaultOutput == false ) {
        deviceList_[m].isDefaultOutput = true;
        for ( unsigned int j=m+1; j<deviceIds_.size(); j++ ) {
          // make sure any remaining devices are not listed as the default
          deviceList_[j].isDefaultOutput = false;
        }
      }
      return deviceList_[m].ID;
    }
    deviceList_[m].isDefaultOutput = false;
  }

  // If not found above, then do system probe of devices and try again.
  probeDevices();
  for ( unsigned int m=0; m<deviceIds_.size(); m++ ) {
    if ( deviceIds_[m].first == id ) return deviceList_[m].ID;
  }

  return 0;
}

//-----------------------------------------------------------------------------

void RtApiWasapi::probeDevices( void )
{
  unsigned int captureDeviceCount = 0;
  unsigned int renderDeviceCount = 0;
  
  IMMDeviceCollection* captureDevices = NULL;
  IMMDeviceCollection* renderDevices = NULL;
  IMMDevice* devicePtr = NULL;

  LPWSTR defaultCaptureId = NULL;
  LPWSTR defaultRenderId = NULL;
  std::string defaultCaptureString;
  std::string defaultRenderString;

  unsigned int nDevices;
  bool isCaptureDevice = false;
  std::vector< std::pair< std::string, bool> > ids;
  LPWSTR deviceId = NULL;

  if ( !deviceEnumerator_ ) return;
  errorText_.clear();
  
  // Count capture devices
  HRESULT hr = deviceEnumerator_->EnumAudioEndpoints( eCapture, DEVICE_STATE_ACTIVE, &captureDevices );
  if ( FAILED( hr ) ) {
    errorText_ = "RtApiWasapi::probeDevices: Unable to retrieve capture device collection.";
    goto Exit;
  }

  hr = captureDevices->GetCount( &captureDeviceCount );
  if ( FAILED( hr ) ) {
    errorText_ = "RtApiWasapi::probeDevices: Unable to retrieve capture device count.";
    goto Exit;
  }

  // Count render devices
  hr = deviceEnumerator_->EnumAudioEndpoints( eRender, DEVICE_STATE_ACTIVE, &renderDevices );
  if ( FAILED( hr ) ) {
    errorText_ = "RtApiWasapi::probeDevices: Unable to retrieve render device collection.";
    goto Exit;
  }

  hr = renderDevices->GetCount( &renderDeviceCount );
  if ( FAILED( hr ) ) {
    errorText_ = "RtApiWasapi::probeDevices: Unable to retrieve render device count.";
    goto Exit;
  }

  nDevices = captureDeviceCount + renderDeviceCount;
  if ( nDevices == 0 ) {
    errorText_ = "RtApiWasapi::probeDevices: No devices found.";
    goto Exit;
  }

  // Get the default capture device Id.
  hr = deviceEnumerator_->GetDefaultAudioEndpoint( eCapture, eConsole, &devicePtr );
  if ( SUCCEEDED( hr) ) {
    hr = devicePtr->GetId( &defaultCaptureId );
    if ( FAILED( hr ) ) {
      errorText_ = "RtApiWasapi::probeDevices: Unable to get default capture device Id.";
      goto Exit;
    }
    defaultCaptureString = convertCharPointerToStdString( defaultCaptureId );
  }

  // Get the default render device Id.
  SAFE_RELEASE( devicePtr );
  hr = deviceEnumerator_->GetDefaultAudioEndpoint( eRender, eConsole, &devicePtr );
  if ( SUCCEEDED( hr) ) {
    hr = devicePtr->GetId( &defaultRenderId );
    if ( FAILED( hr ) ) {
      errorText_ = "RtApiWasapi::probeDevices: Unable to get default render device Id.";
      goto Exit;
    }
    defaultRenderString = convertCharPointerToStdString( defaultRenderId );
  }
  
  // Collect device IDs with mode.
  for ( unsigned int n=0; n<nDevices; n++ ) {
    SAFE_RELEASE( devicePtr );
    if ( n < renderDeviceCount ) {
      hr = renderDevices->Item( n, &devicePtr );
      if ( FAILED( hr ) ) {
        errorText_ = "RtApiWasapi::probeDevices: Unable to retrieve render device handle.";
        error( RTAUDIO_WARNING );
        continue;
      }
    }
    else {
      hr = captureDevices->Item( n - renderDeviceCount, &devicePtr );
      if ( FAILED( hr ) ) {
        errorText_ = "RtApiWasapi::probeDevices: Unable to retrieve capture device handle.";
        error( RTAUDIO_WARNING );
        continue;
      }
      isCaptureDevice = true;
    }

    hr = devicePtr->GetId( &deviceId );
    if ( FAILED( hr ) ) {
      errorText_ = "RtApiWasapi::probeDevices: Unable to get device Id.";
      error( RTAUDIO_WARNING );
      continue;
    }

    ids.push_back( std::pair< std::string, bool>(convertCharPointerToStdString(deviceId), isCaptureDevice) );
    CoTaskMemFree( deviceId );
  }

  // Fill or update the deviceList_ and also save a corresponding list of Ids.
  for ( unsigned int n=0; n<ids.size(); n++ ) {
    if ( std::find( deviceIds_.begin(), deviceIds_.end(), ids[n] ) != deviceIds_.end() ) {
      continue; // We already have this device.
    }
    else { // There is a new device to probe.
      RtAudio::DeviceInfo info;
      std::wstring temp = std::wstring(ids[n].first.begin(), ids[n].first.end());
      if ( probeDeviceInfo( info, (LPWSTR) temp.c_str(), ids[n].second ) == false ) continue; // ignore if probe fails
      deviceIds_.push_back( ids[n] );
      info.ID = currentDeviceId_++;  // arbitrary internal device ID
      deviceList_.push_back( info );
    }
  }

  // Remove any devices left in the list that are no longer available.
  unsigned int m;
  for ( std::vector< std::pair< std::string, bool> >::iterator it=deviceIds_.begin(); it!=deviceIds_.end(); ) {
    for ( m=0; m<ids.size(); m++ ) {
      if ( ids[m] == *it ) {
        ++it;
        break;
      }
    }
    if ( m == ids.size() ) { // not found so remove it from our two lists
      it = deviceIds_.erase(it);
      deviceList_.erase( deviceList_.begin() + distance(deviceIds_.begin(), it ) );
    }
  }

  // Set the default device flags in deviceList_.
  for ( m=0; m<deviceList_.size(); m++ ) {
    if ( deviceIds_[m].first == defaultRenderString )
      deviceList_[m].isDefaultOutput = true;
    else
      deviceList_[m].isDefaultOutput = false;
    if ( deviceIds_[m].first == defaultCaptureString )
      deviceList_[m].isDefaultInput = true;
    else
      deviceList_[m].isDefaultInput = false;
  }

 Exit:
  // Release all references
  SAFE_RELEASE( captureDevices );
  SAFE_RELEASE( renderDevices );
  SAFE_RELEASE( devicePtr );

  CoTaskMemFree( defaultCaptureId );
  CoTaskMemFree( defaultRenderId );

  if ( !errorText_.empty() ) {
    deviceList_.clear();
    deviceIds_.clear();
    error( RTAUDIO_DRIVER_ERROR );
  }
  return;
}

//-----------------------------------------------------------------------------

bool RtApiWasapi::probeDeviceInfo( RtAudio::DeviceInfo &info, LPWSTR deviceId, bool isCaptureDevice )
{
  PROPVARIANT deviceNameProp;
  IMMDevice* devicePtr = NULL;
  IAudioClient* audioClient = NULL;
  IPropertyStore* devicePropStore = NULL;

  WAVEFORMATEX* deviceFormat = NULL;
  WAVEFORMATEX* closestMatchFormat = NULL;

  errorText_.clear();
  RtAudioErrorType errorType = RTAUDIO_DRIVER_ERROR;

  // Get the device pointer from the device Id
  HRESULT hr = deviceEnumerator_->GetDevice( deviceId, &devicePtr );
  if ( FAILED( hr ) ) {
    errorText_ = "RtApiWasapi::probeDeviceInfo: Unable to retrieve device handle.";
    goto Exit;
  }

  // Get device name
  hr = devicePtr->OpenPropertyStore( STGM_READ, &devicePropStore );
  if ( FAILED( hr ) ) {
    errorText_ = "RtApiWasapi::probeDeviceInfo: Unable to open device property store.";
    goto Exit;
  }

  PropVariantInit( &deviceNameProp );

  hr = devicePropStore->GetValue( PKEY_Device_FriendlyName, &deviceNameProp );
  if ( FAILED( hr ) || deviceNameProp.pwszVal == nullptr ) {
    errorText_ = "RtApiWasapi::probeDeviceInfo: Unable to retrieve device property: PKEY_Device_FriendlyName.";
    goto Exit;
  }

  info.name = convertCharPointerToStdString( deviceNameProp.pwszVal );

  // Get audio client
  hr = devicePtr->Activate( __uuidof( IAudioClient ), CLSCTX_ALL, NULL, ( void** ) &audioClient );
  if ( FAILED( hr ) ) {
    errorText_ = "RtApiWasapi::probeDeviceInfo: Unable to retrieve device audio client.";
    goto Exit;
  }

  hr = audioClient->GetMixFormat( &deviceFormat );
  if ( FAILED( hr ) ) {
    errorText_ = "RtApiWasapi::probeDeviceInfo: Unable to retrieve device mix format.";
    goto Exit;
  }

  // Set channel count
  if ( isCaptureDevice ) {
    info.inputChannels = deviceFormat->nChannels;
    info.outputChannels = 0;
    info.duplexChannels = 0;
  }
  else {
    info.inputChannels = 0;
    info.outputChannels = deviceFormat->nChannels;
    info.duplexChannels = 0;
  }

  // Set sample rates
  info.sampleRates.clear();

  // Allow support for all sample rates as we have a built-in sample rate converter.
  for ( unsigned int i = 0; i < MAX_SAMPLE_RATES; i++ ) {
    info.sampleRates.push_back( SAMPLE_RATES[i] );
  }
  info.preferredSampleRate = deviceFormat->nSamplesPerSec;

  // Set native formats
  info.nativeFormats = 0;

  if ( deviceFormat->wFormatTag == WAVE_FORMAT_IEEE_FLOAT ||
       ( deviceFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
         ( ( WAVEFORMATEXTENSIBLE* ) deviceFormat )->SubFormat == KSDATAFORMAT_SUBTYPE_IEEE_FLOAT ) )
  {
    if ( deviceFormat->wBitsPerSample == 32 ) {
      info.nativeFormats |= RTAUDIO_FLOAT32;
    }
    else if ( deviceFormat->wBitsPerSample == 64 ) {
      info.nativeFormats |= RTAUDIO_FLOAT64;
    }
  }
  else if ( deviceFormat->wFormatTag == WAVE_FORMAT_PCM ||
            ( deviceFormat->wFormatTag == WAVE_FORMAT_EXTENSIBLE &&
              ( ( WAVEFORMATEXTENSIBLE* ) deviceFormat )->SubFormat == KSDATAFORMAT_SUBTYPE_PCM ) )
  {
    if ( deviceFormat->wBitsPerSample == 8 ) {
      info.nativeFormats |= RTAUDIO_SINT8;
    }
    else if ( deviceFormat->wBitsPerSample == 16 ) {
      info.nativeFormats |= RTAUDIO_SINT16;
    }
    else if ( deviceFormat->wBitsPerSample == 24 ) {
      info.nativeFormats |= RTAUDIO_SINT24;
    }
    else if ( deviceFormat->wBitsPerSample == 32 ) {
      info.nativeFormats |= RTAUDIO_SINT32;
    }
  }

 Exit:
  // Release all references
  PropVariantClear( &deviceNameProp );

  SAFE_RELEASE( devicePtr );
  SAFE_RELEASE( audioClient );
  SAFE_RELEASE( devicePropStore );

  CoTaskMemFree( deviceFormat );
  CoTaskMemFree( closestMatchFormat );

  if ( !errorText_.empty() ) {
    error( errorType );
    return false;
  }
  return true;
}

void RtApiWasapi::closeStream( void )
{
  MUTEX_LOCK( &stream_.mutex );
  if ( stream_.state == STREAM_CLOSED ) {
    errorText_ = "RtApiWasapi::closeStream: No open stream to close.";
    error( RTAUDIO_WARNING );
    MUTEX_UNLOCK( &stream_.mutex );
    return;
  }

  if ( stream_.state != STREAM_STOPPED )
  {
    MUTEX_UNLOCK( &stream_.mutex );
    stopStream();
    MUTEX_LOCK( &stream_.mutex );
  }

  // clean up stream memory
  SAFE_RELEASE(((WasapiHandle*)stream_.apiHandle)->captureClient)
  SAFE_RELEASE(((WasapiHandle*)stream_.apiHandle)->renderClient)

  SAFE_RELEASE( ( ( WasapiHandle* ) stream_.apiHandle )->captureAudioClient )
  SAFE_RELEASE( ( ( WasapiHandle* ) stream_.apiHandle )->renderAudioClient )

  if ( ( ( WasapiHandle* ) stream_.apiHandle )->captureEvent )
    CloseHandle( ( ( WasapiHandle* ) stream_.apiHandle )->captureEvent );

  if ( ( ( WasapiHandle* ) stream_.apiHandle )->renderEvent )
    CloseHandle( ( ( WasapiHandle* ) stream_.apiHandle )->renderEvent );

  delete ( WasapiHandle* ) stream_.apiHandle;
  stream_.apiHandle = NULL;

  for ( int i = 0; i < 2; i++ ) {
    if ( stream_.userBuffer[i] ) {
      free( stream_.userBuffer[i] );
      stream_.userBuffer[i] = 0;
    }
  }

  if ( stream_.deviceBuffer ) {
    free( stream_.deviceBuffer );
    stream_.deviceBuffer = 0;
  }

  clearStreamInfo();
  MUTEX_UNLOCK( &stream_.mutex );
}

//-----------------------------------------------------------------------------

RtAudioErrorType RtApiWasapi::startStream( void )
{
  MUTEX_LOCK( &stream_.mutex );
  if ( stream_.state != STREAM_STOPPED ) {
    if ( stream_.state == STREAM_RUNNING )
      errorText_ = "RtApiWasapi::startStream(): the stream is already running!";
    else if ( stream_.state == STREAM_STOPPING || stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiWasapi::startStream(): the stream is stopping or closed!";
    MUTEX_UNLOCK( &stream_.mutex );
    return error( RTAUDIO_WARNING );
  }

  /*
  #if defined( HAVE_GETTIMEOFDAY )
  gettimeofday( &stream_.lastTickTimestamp, NULL );
  #endif
  */
  
  // update stream state
  stream_.state = STREAM_RUNNING;

  // create WASAPI stream thread
  stream_.callbackInfo.thread = ( ThreadHandle ) CreateThread( NULL, 0, runWasapiThread, this, CREATE_SUSPENDED, NULL );

  if ( !stream_.callbackInfo.thread ) {
    errorText_ = "RtApiWasapi::startStream: Unable to instantiate callback thread.";
    MUTEX_UNLOCK( &stream_.mutex );
    return error( RTAUDIO_THREAD_ERROR );
  }
  else {
    SetThreadPriority( ( void* ) stream_.callbackInfo.thread, stream_.callbackInfo.priority );
    ResumeThread( ( void* ) stream_.callbackInfo.thread );
  }
  MUTEX_UNLOCK( &stream_.mutex );
  return RTAUDIO_NO_ERROR;
}

//-----------------------------------------------------------------------------

RtAudioErrorType RtApiWasapi::stopStream( void )
{
  MUTEX_LOCK( &stream_.mutex );
  if ( stream_.state != STREAM_RUNNING && stream_.state != STREAM_STOPPING ) {
    if ( stream_.state == STREAM_STOPPED )
      errorText_ = "RtApiWasapi::stopStream(): the stream is already stopped!";
    else if ( stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiWasapi::stopStream(): the stream is closed!";
    MUTEX_UNLOCK( &stream_.mutex );
    return error( RTAUDIO_WARNING );
  }

  // inform stream thread by setting stream state to STREAM_STOPPING
  stream_.state = STREAM_STOPPING;

  WaitForSingleObject( ( void* ) stream_.callbackInfo.thread, INFINITE );

  // close thread handle
  if ( stream_.callbackInfo.thread && !CloseHandle( ( void* ) stream_.callbackInfo.thread ) ) {
    errorText_ = "RtApiWasapi::stopStream: Unable to close callback thread.";
    MUTEX_UNLOCK( &stream_.mutex );
    return error( RTAUDIO_THREAD_ERROR );
  }

  stream_.callbackInfo.thread = (ThreadHandle) NULL;
  MUTEX_UNLOCK( &stream_.mutex );
  return RTAUDIO_NO_ERROR;
}

//-----------------------------------------------------------------------------

RtAudioErrorType RtApiWasapi::abortStream( void )
{
  MUTEX_LOCK( &stream_.mutex );
  if ( stream_.state != STREAM_RUNNING ) {
    if ( stream_.state == STREAM_STOPPED )
      errorText_ = "RtApiWasapi::abortStream(): the stream is already stopped!";
    else if ( stream_.state == STREAM_STOPPING || stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiWasapi::abortStream(): the stream is stopping or closed!";
    MUTEX_UNLOCK( &stream_.mutex );
    return error( RTAUDIO_WARNING );
  }
    
  // inform stream thread by setting stream state to STREAM_STOPPING
  stream_.state = STREAM_STOPPING;

  WaitForSingleObject( ( void* ) stream_.callbackInfo.thread, INFINITE );

  // close thread handle
  if ( stream_.callbackInfo.thread && !CloseHandle( ( void* ) stream_.callbackInfo.thread ) ) {
    errorText_ = "RtApiWasapi::abortStream: Unable to close callback thread.";
    MUTEX_UNLOCK( &stream_.mutex );
    return error( RTAUDIO_THREAD_ERROR );
  }

  stream_.callbackInfo.thread = (ThreadHandle) NULL;
  MUTEX_UNLOCK( &stream_.mutex );
  return RTAUDIO_NO_ERROR;
}

//-----------------------------------------------------------------------------

bool RtApiWasapi::probeDeviceOpen( unsigned int deviceId, StreamMode mode, unsigned int channels,
                                   unsigned int firstChannel, unsigned int sampleRate,
                                   RtAudioFormat format, unsigned int* bufferSize,
                                   RtAudio::StreamOptions* options )
{
  MUTEX_LOCK( &stream_.mutex );
  bool methodResult = FAILURE;
  IMMDevice* devicePtr = NULL;
  WAVEFORMATEX* deviceFormat = NULL;
  unsigned int bufferBytes;
  stream_.state = STREAM_STOPPED;
  bool isInput = false;
  std::string id;

  unsigned int deviceIdx;
  for ( deviceIdx=0; deviceIdx<deviceList_.size(); deviceIdx++ ) {
    if ( deviceList_[deviceIdx].ID == deviceId ) {
      id = deviceIds_[deviceIdx].first;
      if ( deviceIds_[deviceIdx].second ) isInput = true;
      break;
    }
  }

  errorText_.clear();
  RtAudioErrorType errorType = RTAUDIO_INVALID_USE;
  if ( id.empty() ) {
    errorText_ = "RtApiWasapi::probeDeviceOpen: the device ID was not found!";
    MUTEX_UNLOCK( &stream_.mutex );
    return FAILURE;
  }

  if ( isInput && mode != INPUT ) {
    errorText_ = "RtApiWasapi::probeDeviceOpen: deviceId specified does not support output mode.";
    MUTEX_UNLOCK( &stream_.mutex );
    return FAILURE;
  }

  // Get the device pointer from the device Id
  errorType = RTAUDIO_DRIVER_ERROR;
  std::wstring temp = std::wstring(id.begin(), id.end());
  HRESULT hr = deviceEnumerator_->GetDevice( (LPWSTR)temp.c_str(), &devicePtr );
  if ( FAILED( hr ) ) {
    errorText_ = "RtApiWasapi::probeDeviceOpen: Unable to retrieve device handle.";
    MUTEX_UNLOCK( &stream_.mutex );
    return FAILURE;
  }
  
  // Create API handle if not already created.
  if ( !stream_.apiHandle )
    stream_.apiHandle = ( void* ) new WasapiHandle();

  if ( isInput ) {
    IAudioClient*& captureAudioClient = ( ( WasapiHandle* ) stream_.apiHandle )->captureAudioClient;

    hr = devicePtr->Activate( __uuidof( IAudioClient ), CLSCTX_ALL,
                              NULL, ( void** ) &captureAudioClient );
    if ( FAILED( hr ) ) {
      errorText_ = "RtApiWasapi::probeDeviceOpen: Unable to retrieve capture device audio client.";
      goto Exit;
    }

    hr = captureAudioClient->GetMixFormat( &deviceFormat );
    if ( FAILED( hr ) ) {
      errorText_ = "RtApiWasapi::probeDeviceOpen: Unable to retrieve capture device mix format.";
      goto Exit;
    }

    stream_.nDeviceChannels[mode] = deviceFormat->nChannels;
    captureAudioClient->GetStreamLatency( ( long long* ) &stream_.latency[mode] );
  }

  // If an output device and is configured for loopback (input mode)
  if ( isInput == false && mode == INPUT ) {
    // If renderAudioClient is not initialised, initialise it now
    IAudioClient*& renderAudioClient = ( ( WasapiHandle* ) stream_.apiHandle )->renderAudioClient;
    if ( !renderAudioClient ) {
      MUTEX_UNLOCK( &stream_.mutex );
      probeDeviceOpen( deviceId, OUTPUT, channels, firstChannel, sampleRate, format, bufferSize, options );
      MUTEX_LOCK( &stream_.mutex );
    }

    // Retrieve captureAudioClient from our stream handle.
    IAudioClient*& captureAudioClient = ( ( WasapiHandle* ) stream_.apiHandle )->captureAudioClient;

    hr = devicePtr->Activate( __uuidof( IAudioClient ), CLSCTX_ALL,
                              NULL, ( void** ) &captureAudioClient );
    if ( FAILED( hr ) ) {
      errorText_ = "RtApiWasapi::probeDeviceOpen: Unable to retrieve render device audio client.";
      goto Exit;
    }

    hr = captureAudioClient->GetMixFormat( &deviceFormat );
    if ( FAILED( hr ) ) {
      errorText_ = "RtApiWasapi::probeDeviceOpen: Unable to retrieve render device mix format.";
      goto Exit;
    }

    stream_.nDeviceChannels[mode] = deviceFormat->nChannels;
    captureAudioClient->GetStreamLatency( ( long long* ) &stream_.latency[mode] );
  }

  // If output device and is configured for output.
  if ( isInput == false && mode == OUTPUT ) {
    // If renderAudioClient is already initialised, don't initialise it again
    IAudioClient*& renderAudioClient = ( ( WasapiHandle* ) stream_.apiHandle )->renderAudioClient;
    if ( renderAudioClient ) {
      methodResult = SUCCESS;
      goto Exit;
    }

    hr = devicePtr->Activate( __uuidof( IAudioClient ), CLSCTX_ALL,
                              NULL, ( void** ) &renderAudioClient );
    if ( FAILED( hr ) ) {
      errorText_ = "RtApiWasapi::probeDeviceOpen: Unable to retrieve render device audio client.";
      goto Exit;
    }

    hr = renderAudioClient->GetMixFormat( &deviceFormat );
    if ( FAILED( hr ) ) {
      errorText_ = "RtApiWasapi::probeDeviceOpen: Unable to retrieve render device mix format.";
      goto Exit;
    }

    stream_.nDeviceChannels[mode] = deviceFormat->nChannels;
    renderAudioClient->GetStreamLatency( ( long long* ) &stream_.latency[mode] );
  }

  // Fill stream data
  if ( ( stream_.mode == OUTPUT && mode == INPUT ) ||
       ( stream_.mode == INPUT && mode == OUTPUT ) ) {
    stream_.mode = DUPLEX;
  }
  else {
    stream_.mode = mode;
  }

  stream_.deviceId[mode] = deviceId;
  stream_.doByteSwap[mode] = false;
  stream_.sampleRate = sampleRate;
  stream_.bufferSize = *bufferSize;
  stream_.nBuffers = 1;
  stream_.nUserChannels[mode] = channels;
  stream_.channelOffset[mode] = firstChannel;
  stream_.userFormat = format;
  stream_.deviceFormat[mode] = deviceList_[deviceIdx].nativeFormats;

  if ( options && options->flags & RTAUDIO_NONINTERLEAVED )
    stream_.userInterleaved = false;
  else
    stream_.userInterleaved = true;
  stream_.deviceInterleaved[mode] = true;

  // Set flags for buffer conversion.
  stream_.doConvertBuffer[mode] = false;
  if ( stream_.userFormat != stream_.deviceFormat[mode] ||
       stream_.nUserChannels[0] != stream_.nDeviceChannels[0] ||
       stream_.nUserChannels[1] != stream_.nDeviceChannels[1] )
    stream_.doConvertBuffer[mode] = true;
  else if ( stream_.userInterleaved != stream_.deviceInterleaved[mode] &&
            stream_.nUserChannels[mode] > 1 )
    stream_.doConvertBuffer[mode] = true;

  if ( stream_.doConvertBuffer[mode] )
    setConvertInfo( mode, firstChannel );

  // Allocate necessary internal buffers
  bufferBytes = stream_.nUserChannels[mode] * stream_.bufferSize * formatBytes( stream_.userFormat );

  stream_.userBuffer[mode] = ( char* ) calloc( bufferBytes, 1 );
  if ( !stream_.userBuffer[mode] ) {
    errorType = RTAUDIO_MEMORY_ERROR;
    errorText_ = "RtApiWasapi::probeDeviceOpen: Error allocating user buffer memory.";
    goto Exit;
  }

  if ( options && options->flags & RTAUDIO_SCHEDULE_REALTIME )
    stream_.callbackInfo.priority = 15;
  else
    stream_.callbackInfo.priority = 0;

  ///! TODO: RTAUDIO_MINIMIZE_LATENCY // Provide stream buffers directly to callback
  ///! TODO: RTAUDIO_HOG_DEVICE       // Exclusive mode

  methodResult = SUCCESS;

 Exit:
  //clean up
  SAFE_RELEASE( devicePtr );
  CoTaskMemFree( deviceFormat );

  // if method failed, close the stream
  if ( methodResult == FAILURE )
  {
    MUTEX_UNLOCK( &stream_.mutex );
    closeStream();
    MUTEX_LOCK( &stream_.mutex );
  }

  if ( !errorText_.empty() )
    error( errorType );

  MUTEX_UNLOCK( &stream_.mutex );
  return methodResult;
}

//=============================================================================

DWORD WINAPI RtApiWasapi::runWasapiThread( void* wasapiPtr )
{
  if ( wasapiPtr )
    ( ( RtApiWasapi* ) wasapiPtr )->wasapiThread();

  return 0;
}

DWORD WINAPI RtApiWasapi::stopWasapiThread( void* wasapiPtr )
{
  if ( wasapiPtr )
    ( ( RtApiWasapi* ) wasapiPtr )->stopStream();

  return 0;
}

DWORD WINAPI RtApiWasapi::abortWasapiThread( void* wasapiPtr )
{
  if ( wasapiPtr )
    ( ( RtApiWasapi* ) wasapiPtr )->abortStream();

  return 0;
}

//-----------------------------------------------------------------------------

void RtApiWasapi::wasapiThread()
{
  // as this is a new thread, we must CoInitialize it
  CoInitialize( NULL );

  HRESULT hr;

  IAudioClient* captureAudioClient = ( ( WasapiHandle* ) stream_.apiHandle )->captureAudioClient;
  IAudioClient* renderAudioClient = ( ( WasapiHandle* ) stream_.apiHandle )->renderAudioClient;
  IAudioCaptureClient* captureClient = ( ( WasapiHandle* ) stream_.apiHandle )->captureClient;
  IAudioRenderClient* renderClient = ( ( WasapiHandle* ) stream_.apiHandle )->renderClient;
  HANDLE captureEvent = ( ( WasapiHandle* ) stream_.apiHandle )->captureEvent;
  HANDLE renderEvent = ( ( WasapiHandle* ) stream_.apiHandle )->renderEvent;

  WAVEFORMATEX* captureFormat = NULL;
  WAVEFORMATEX* renderFormat = NULL;
  float captureSrRatio = 0.0f;
  float renderSrRatio = 0.0f;
  WasapiBuffer captureBuffer;
  WasapiBuffer renderBuffer;
  WasapiResampler* captureResampler = NULL;
  WasapiResampler* renderResampler = NULL;

  // declare local stream variables
  RtAudioCallback callback = ( RtAudioCallback ) stream_.callbackInfo.callback;
  BYTE* streamBuffer = NULL;
  DWORD captureFlags = 0;
  unsigned int bufferFrameCount = 0;
  unsigned int numFramesPadding = 0;
  unsigned int convBufferSize = 0;
  bool loopbackEnabled = stream_.deviceId[INPUT] == stream_.deviceId[OUTPUT];
  bool callbackPushed = true;
  bool callbackPulled = false;
  bool callbackStopped = false;
  int callbackResult = 0;

  // convBuffer is used to store converted buffers between WASAPI and the user
  char* convBuffer = NULL;
  unsigned int convBuffSize = 0;
  unsigned int deviceBuffSize = 0;

  std::string errorText;
  RtAudioErrorType errorType = RTAUDIO_DRIVER_ERROR;

  // Attempt to assign "Pro Audio" characteristic to thread
  HMODULE AvrtDll = LoadLibraryW( L"AVRT.dll" );
  if ( AvrtDll ) {
    DWORD taskIndex = 0;
    TAvSetMmThreadCharacteristicsPtr AvSetMmThreadCharacteristicsPtr =
      ( TAvSetMmThreadCharacteristicsPtr ) (void(*)()) GetProcAddress( AvrtDll, "AvSetMmThreadCharacteristicsW" );
    AvSetMmThreadCharacteristicsPtr( L"Pro Audio", &taskIndex );
    FreeLibrary( AvrtDll );
  }

  // start capture stream if applicable
  if ( captureAudioClient ) {
    hr = captureAudioClient->GetMixFormat( &captureFormat );
    if ( FAILED( hr ) ) {
      errorText = "RtApiWasapi::wasapiThread: Unable to retrieve device mix format.";
      goto Exit;
    }

    // init captureResampler
    captureResampler = new WasapiResampler( stream_.deviceFormat[INPUT] == RTAUDIO_FLOAT32 || stream_.deviceFormat[INPUT] == RTAUDIO_FLOAT64,
                                            formatBytes( stream_.deviceFormat[INPUT] ) * 8, stream_.nDeviceChannels[INPUT],
                                            captureFormat->nSamplesPerSec, stream_.sampleRate );

    captureSrRatio = ( ( float ) captureFormat->nSamplesPerSec / stream_.sampleRate );

    if ( !captureClient ) {
      IAudioClient3* captureAudioClient3 = nullptr;
      captureAudioClient->QueryInterface( __uuidof( IAudioClient3 ), ( void** ) &captureAudioClient3 );
      if ( captureAudioClient3 && !loopbackEnabled )
      {
        UINT32 Ignore;
        UINT32 MinPeriodInFrames;
        hr = captureAudioClient3->GetSharedModeEnginePeriod( captureFormat,
                                                             &Ignore,
                                                             &Ignore,
                                                             &MinPeriodInFrames,
                                                             &Ignore );
        if ( FAILED( hr ) ) {
          errorText = "RtApiWasapi::wasapiThread: Unable to initialize capture audio client.";
          goto Exit;
        }
        
        hr = captureAudioClient3->InitializeSharedAudioStream( AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                                               MinPeriodInFrames,
                                                               captureFormat,
                                                               NULL );
        SAFE_RELEASE(captureAudioClient3);
      }
      else
      {
        hr = captureAudioClient->Initialize( AUDCLNT_SHAREMODE_SHARED,
                                             loopbackEnabled ? AUDCLNT_STREAMFLAGS_LOOPBACK : AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                             0,
                                             0,
                                             captureFormat,
                                             NULL );
      }

      if ( FAILED( hr ) ) {
        errorText = "RtApiWasapi::wasapiThread: Unable to initialize capture audio client.";
        goto Exit;
      }

      hr = captureAudioClient->GetService( __uuidof( IAudioCaptureClient ),
                                           ( void** ) &captureClient );
      if ( FAILED( hr ) ) {
        errorText = "RtApiWasapi::wasapiThread: Unable to retrieve capture client handle.";
        goto Exit;
      }

      // don't configure captureEvent if in loopback mode
      if ( !loopbackEnabled )
      {
        // configure captureEvent to trigger on every available capture buffer
        captureEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
        if ( !captureEvent ) {
          errorType = RTAUDIO_SYSTEM_ERROR;
          errorText = "RtApiWasapi::wasapiThread: Unable to create capture event.";
          goto Exit;
        }

        hr = captureAudioClient->SetEventHandle( captureEvent );
        if ( FAILED( hr ) ) {
          errorText = "RtApiWasapi::wasapiThread: Unable to set capture event handle.";
          goto Exit;
        }

        ( ( WasapiHandle* ) stream_.apiHandle )->captureEvent = captureEvent;
      }

      ( ( WasapiHandle* ) stream_.apiHandle )->captureClient = captureClient;

      // reset the capture stream
      hr = captureAudioClient->Reset();
      if ( FAILED( hr ) ) {
        errorText = "RtApiWasapi::wasapiThread: Unable to reset capture stream.";
        goto Exit;
      }

      // start the capture stream
      hr = captureAudioClient->Start();
      if ( FAILED( hr ) ) {
        errorText = "RtApiWasapi::wasapiThread: Unable to start capture stream.";
        goto Exit;
      }
    }

    unsigned int inBufferSize = 0;
    hr = captureAudioClient->GetBufferSize( &inBufferSize );
    if ( FAILED( hr ) ) {
      errorText = "RtApiWasapi::wasapiThread: Unable to get capture buffer size.";
      goto Exit;
    }

    // scale outBufferSize according to stream->user sample rate ratio
    unsigned int outBufferSize = ( unsigned int ) ceilf( stream_.bufferSize * captureSrRatio ) * stream_.nDeviceChannels[INPUT];
    inBufferSize *= stream_.nDeviceChannels[INPUT];

    // set captureBuffer size
    captureBuffer.setBufferSize( inBufferSize + outBufferSize, formatBytes( stream_.deviceFormat[INPUT] ) );
  }

  // start render stream if applicable
  if ( renderAudioClient ) {
    hr = renderAudioClient->GetMixFormat( &renderFormat );
    if ( FAILED( hr ) ) {
      errorText = "RtApiWasapi::wasapiThread: Unable to retrieve device mix format.";
      goto Exit;
    }

    // init renderResampler
    renderResampler = new WasapiResampler( stream_.deviceFormat[OUTPUT] == RTAUDIO_FLOAT32 || stream_.deviceFormat[OUTPUT] == RTAUDIO_FLOAT64,
                                           formatBytes( stream_.deviceFormat[OUTPUT] ) * 8, stream_.nDeviceChannels[OUTPUT],
                                           stream_.sampleRate, renderFormat->nSamplesPerSec );

    renderSrRatio = ( ( float ) renderFormat->nSamplesPerSec / stream_.sampleRate );

    if ( !renderClient ) {
      IAudioClient3* renderAudioClient3 = nullptr;
      renderAudioClient->QueryInterface( __uuidof( IAudioClient3 ), ( void** ) &renderAudioClient3 );
      if ( renderAudioClient3 )
      {
        UINT32 Ignore;
        UINT32 MinPeriodInFrames;
        hr = renderAudioClient3->GetSharedModeEnginePeriod( renderFormat,
                                                            &Ignore,
                                                            &Ignore,
                                                            &MinPeriodInFrames,
                                                            &Ignore );
        if ( FAILED( hr ) ) {
          errorText = "RtApiWasapi::wasapiThread: Unable to initialize render audio client.";
          goto Exit;
        }
        
        hr = renderAudioClient3->InitializeSharedAudioStream( AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                                              MinPeriodInFrames,
                                                              renderFormat,
                                                              NULL );
        SAFE_RELEASE(renderAudioClient3);
      }
      else
      {
        hr = renderAudioClient->Initialize( AUDCLNT_SHAREMODE_SHARED,
                                            AUDCLNT_STREAMFLAGS_EVENTCALLBACK,
                                            0,
                                            0,
                                            renderFormat,
                                            NULL );
      }

      if ( FAILED( hr ) ) {
        errorText = "RtApiWasapi::wasapiThread: Unable to initialize render audio client.";
        goto Exit;
      }

      hr = renderAudioClient->GetService( __uuidof( IAudioRenderClient ),
                                          ( void** ) &renderClient );
      if ( FAILED( hr ) ) {
        errorText = "RtApiWasapi::wasapiThread: Unable to retrieve render client handle.";
        goto Exit;
      }

      // configure renderEvent to trigger on every available render buffer
      renderEvent = CreateEvent( NULL, FALSE, FALSE, NULL );
      if ( !renderEvent ) {
        errorType = RTAUDIO_SYSTEM_ERROR;
        errorText = "RtApiWasapi::wasapiThread: Unable to create render event.";
        goto Exit;
      }

      hr = renderAudioClient->SetEventHandle( renderEvent );
      if ( FAILED( hr ) ) {
        errorText = "RtApiWasapi::wasapiThread: Unable to set render event handle.";
        goto Exit;
      }

      ( ( WasapiHandle* ) stream_.apiHandle )->renderClient = renderClient;
      ( ( WasapiHandle* ) stream_.apiHandle )->renderEvent = renderEvent;

      // reset the render stream
      hr = renderAudioClient->Reset();
      if ( FAILED( hr ) ) {
        errorText = "RtApiWasapi::wasapiThread: Unable to reset render stream.";
        goto Exit;
      }

      // start the render stream
      hr = renderAudioClient->Start();
      if ( FAILED( hr ) ) {
        errorText = "RtApiWasapi::wasapiThread: Unable to start render stream.";
        goto Exit;
      }
    }

    unsigned int outBufferSize = 0;
    hr = renderAudioClient->GetBufferSize( &outBufferSize );
    if ( FAILED( hr ) ) {
      errorText = "RtApiWasapi::wasapiThread: Unable to get render buffer size.";
      goto Exit;
    }

    // scale inBufferSize according to user->stream sample rate ratio
    unsigned int inBufferSize = ( unsigned int ) ceilf( stream_.bufferSize * renderSrRatio ) * stream_.nDeviceChannels[OUTPUT];
    outBufferSize *= stream_.nDeviceChannels[OUTPUT];

    // set renderBuffer size
    renderBuffer.setBufferSize( inBufferSize + outBufferSize, formatBytes( stream_.deviceFormat[OUTPUT] ) );
  }

  // malloc buffer memory
  if ( stream_.mode == INPUT )
  {
    using namespace std; // for ceilf
    convBuffSize = ( unsigned int ) ( ceilf( stream_.bufferSize * captureSrRatio ) ) * stream_.nDeviceChannels[INPUT] * formatBytes( stream_.deviceFormat[INPUT] );
    deviceBuffSize = stream_.bufferSize * stream_.nDeviceChannels[INPUT] * formatBytes( stream_.deviceFormat[INPUT] );
  }
  else if ( stream_.mode == OUTPUT )
  {
    convBuffSize = ( unsigned int ) ( ceilf( stream_.bufferSize * renderSrRatio ) ) * stream_.nDeviceChannels[OUTPUT] * formatBytes( stream_.deviceFormat[OUTPUT] );
    deviceBuffSize = stream_.bufferSize * stream_.nDeviceChannels[OUTPUT] * formatBytes( stream_.deviceFormat[OUTPUT] );
  }
  else if ( stream_.mode == DUPLEX )
  {
    convBuffSize = std::max( ( unsigned int ) ( ceilf( stream_.bufferSize * captureSrRatio ) ) * stream_.nDeviceChannels[INPUT] * formatBytes( stream_.deviceFormat[INPUT] ),
                             ( unsigned int ) ( ceilf( stream_.bufferSize * renderSrRatio ) ) * stream_.nDeviceChannels[OUTPUT] * formatBytes( stream_.deviceFormat[OUTPUT] ) );
    deviceBuffSize = std::max( stream_.bufferSize * stream_.nDeviceChannels[INPUT] * formatBytes( stream_.deviceFormat[INPUT] ),
                               stream_.bufferSize * stream_.nDeviceChannels[OUTPUT] * formatBytes( stream_.deviceFormat[OUTPUT] ) );
  }

  convBuffSize *= 2; // allow overflow for *SrRatio remainders
  convBuffer = ( char* ) calloc( convBuffSize, 1 );
  stream_.deviceBuffer = ( char* ) calloc( deviceBuffSize, 1 );
  if ( !convBuffer || !stream_.deviceBuffer ) {
    errorType = RTAUDIO_MEMORY_ERROR;
    errorText = "RtApiWasapi::wasapiThread: Error allocating device buffer memory.";
    goto Exit;
  }

  // stream process loop
  while ( stream_.state != STREAM_STOPPING ) {
    if ( !callbackPulled ) {
      // Callback Input
      // ==============
      // 1. Pull callback buffer from inputBuffer
      // 2. If 1. was successful: Convert callback buffer to user sample rate and channel count
      //                          Convert callback buffer to user format

      if ( captureAudioClient )
      {
        int samplesToPull = ( unsigned int ) floorf( stream_.bufferSize * captureSrRatio );

        convBufferSize = 0;
        while ( convBufferSize < stream_.bufferSize )
        {
          // Pull callback buffer from inputBuffer
          callbackPulled = captureBuffer.pullBuffer( convBuffer,
                                                     samplesToPull * stream_.nDeviceChannels[INPUT],
                                                     stream_.deviceFormat[INPUT] );

          if ( !callbackPulled )
          {
            break;
          }

          // Convert callback buffer to user sample rate
          unsigned int deviceBufferOffset = convBufferSize * stream_.nDeviceChannels[INPUT] * formatBytes( stream_.deviceFormat[INPUT] );
          unsigned int convSamples = 0;

          captureResampler->Convert( stream_.deviceBuffer + deviceBufferOffset,
                                     convBuffer,
                                     samplesToPull,
                                     convSamples,
                                     convBufferSize == 0 ? -1 : stream_.bufferSize - convBufferSize );

          convBufferSize += convSamples;
          samplesToPull = 1; // now pull one sample at a time until we have stream_.bufferSize samples
        }

        if ( callbackPulled )
        {
          if ( stream_.doConvertBuffer[INPUT] ) {
            // Convert callback buffer to user format
            convertBuffer( stream_.userBuffer[INPUT],
                           stream_.deviceBuffer,
                           stream_.convertInfo[INPUT] );
          }
          else {
            // no further conversion, simple copy deviceBuffer to userBuffer
            memcpy( stream_.userBuffer[INPUT],
                    stream_.deviceBuffer,
                    stream_.bufferSize * stream_.nUserChannels[INPUT] * formatBytes( stream_.userFormat ) );
          }
        }
      }
      else {
        // if there is no capture stream, set callbackPulled flag
        callbackPulled = true;
      }

      // Execute Callback
      // ================
      // 1. Execute user callback method
      // 2. Handle return value from callback

      // if callback has not requested the stream to stop
      if ( callbackPulled && !callbackStopped ) {
        // Execute user callback method
        callbackResult = callback( stream_.userBuffer[OUTPUT],
                                   stream_.userBuffer[INPUT],
                                   stream_.bufferSize,
                                   getStreamTime(),
                                   captureFlags & AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY ? RTAUDIO_INPUT_OVERFLOW : 0,
                                   stream_.callbackInfo.userData );

        // tick stream time
        RtApi::tickStreamTime();

        // Handle return value from callback
        if ( callbackResult == 1 ) {
          // instantiate a thread to stop this thread
          HANDLE threadHandle = CreateThread( NULL, 0, stopWasapiThread, this, 0, NULL );
          if ( !threadHandle ) {
            errorType = RTAUDIO_THREAD_ERROR;
            errorText = "RtApiWasapi::wasapiThread: Unable to instantiate stream stop thread.";
            goto Exit;
          }
          else if ( !CloseHandle( threadHandle ) ) {
            errorType = RTAUDIO_THREAD_ERROR;
            errorText = "RtApiWasapi::wasapiThread: Unable to close stream stop thread handle.";
            goto Exit;
          }

          callbackStopped = true;
        }
        else if ( callbackResult == 2 ) {
          // instantiate a thread to stop this thread
          HANDLE threadHandle = CreateThread( NULL, 0, abortWasapiThread, this, 0, NULL );
          if ( !threadHandle ) {
            errorType = RTAUDIO_THREAD_ERROR;
            errorText = "RtApiWasapi::wasapiThread: Unable to instantiate stream abort thread.";
            goto Exit;
          }
          else if ( !CloseHandle( threadHandle ) ) {
            errorType = RTAUDIO_THREAD_ERROR;
            errorText = "RtApiWasapi::wasapiThread: Unable to close stream abort thread handle.";
            goto Exit;
          }

          callbackStopped = true;
        }
      }
    }

    // Callback Output
    // ===============
    // 1. Convert callback buffer to stream format
    // 2. Convert callback buffer to stream sample rate and channel count
    // 3. Push callback buffer into outputBuffer

    if ( renderAudioClient && callbackPulled )
    {
      // if the last call to renderBuffer.PushBuffer() was successful
      if ( callbackPushed || convBufferSize == 0 )
      {
        if ( stream_.doConvertBuffer[OUTPUT] )
        {
          // Convert callback buffer to stream format
          convertBuffer( stream_.deviceBuffer,
                         stream_.userBuffer[OUTPUT],
                         stream_.convertInfo[OUTPUT] );

        }
        else {
          // no further conversion, simple copy userBuffer to deviceBuffer
          memcpy( stream_.deviceBuffer,
                  stream_.userBuffer[OUTPUT],
                  stream_.bufferSize * stream_.nUserChannels[OUTPUT] * formatBytes( stream_.userFormat ) );
        }

        // Convert callback buffer to stream sample rate
        renderResampler->Convert( convBuffer,
                                  stream_.deviceBuffer,
                                  stream_.bufferSize,
                                  convBufferSize );
      }

      // Push callback buffer into outputBuffer
      callbackPushed = renderBuffer.pushBuffer( convBuffer,
                                                convBufferSize * stream_.nDeviceChannels[OUTPUT],
                                                stream_.deviceFormat[OUTPUT] );
    }
    else {
      // if there is no render stream, set callbackPushed flag
      callbackPushed = true;
    }

    // Stream Capture
    // ==============
    // 1. Get capture buffer from stream
    // 2. Push capture buffer into inputBuffer
    // 3. If 2. was successful: Release capture buffer

    if ( captureAudioClient ) {
      // if the callback input buffer was not pulled from captureBuffer, wait for next capture event
      if ( !callbackPulled ) {
        WaitForSingleObject( loopbackEnabled ? renderEvent : captureEvent, INFINITE );
      }

      // Get capture buffer from stream
      hr = captureClient->GetBuffer( &streamBuffer,
                                     &bufferFrameCount,
                                     &captureFlags, NULL, NULL );
      if ( FAILED( hr ) ) {
        errorText = "RtApiWasapi::wasapiThread: Unable to retrieve capture buffer.";
        goto Exit;
      }

      if ( bufferFrameCount != 0 ) {
        // Push capture buffer into inputBuffer
        if ( captureBuffer.pushBuffer( ( char* ) streamBuffer,
                                       bufferFrameCount * stream_.nDeviceChannels[INPUT],
                                       stream_.deviceFormat[INPUT] ) )
        {
          // Release capture buffer
          hr = captureClient->ReleaseBuffer( bufferFrameCount );
          if ( FAILED( hr ) ) {
            errorText = "RtApiWasapi::wasapiThread: Unable to release capture buffer.";
            goto Exit;
          }
        }
        else
        {
          // Inform WASAPI that capture was unsuccessful
          hr = captureClient->ReleaseBuffer( 0 );
          if ( FAILED( hr ) ) {
            errorText = "RtApiWasapi::wasapiThread: Unable to release capture buffer.";
            goto Exit;
          }
        }
      }
      else
      {
        // Inform WASAPI that capture was unsuccessful
        hr = captureClient->ReleaseBuffer( 0 );
        if ( FAILED( hr ) ) {
          errorText = "RtApiWasapi::wasapiThread: Unable to release capture buffer.";
          goto Exit;
        }
      }
    }

    // Stream Render
    // =============
    // 1. Get render buffer from stream
    // 2. Pull next buffer from outputBuffer
    // 3. If 2. was successful: Fill render buffer with next buffer
    //                          Release render buffer

    if ( renderAudioClient ) {
      // if the callback output buffer was not pushed to renderBuffer, wait for next render event
      if ( callbackPulled && !callbackPushed ) {
        WaitForSingleObject( renderEvent, INFINITE );
      }

      // Get render buffer from stream
      hr = renderAudioClient->GetBufferSize( &bufferFrameCount );
      if ( FAILED( hr ) ) {
        errorText = "RtApiWasapi::wasapiThread: Unable to retrieve render buffer size.";
        goto Exit;
      }

      hr = renderAudioClient->GetCurrentPadding( &numFramesPadding );
      if ( FAILED( hr ) ) {
        errorText = "RtApiWasapi::wasapiThread: Unable to retrieve render buffer padding.";
        goto Exit;
      }

      bufferFrameCount -= numFramesPadding;

      if ( bufferFrameCount != 0 ) {
        hr = renderClient->GetBuffer( bufferFrameCount, &streamBuffer );
        if ( FAILED( hr ) ) {
          errorText = "RtApiWasapi::wasapiThread: Unable to retrieve render buffer.";
          goto Exit;
        }

        // Pull next buffer from outputBuffer
        // Fill render buffer with next buffer
        if ( renderBuffer.pullBuffer( ( char* ) streamBuffer,
                                      bufferFrameCount * stream_.nDeviceChannels[OUTPUT],
                                      stream_.deviceFormat[OUTPUT] ) )
        {
          // Release render buffer
          hr = renderClient->ReleaseBuffer( bufferFrameCount, 0 );
          if ( FAILED( hr ) ) {
            errorText = "RtApiWasapi::wasapiThread: Unable to release render buffer.";
            goto Exit;
          }
        }
        else
        {
          // Inform WASAPI that render was unsuccessful
          hr = renderClient->ReleaseBuffer( 0, 0 );
          if ( FAILED( hr ) ) {
            errorText = "RtApiWasapi::wasapiThread: Unable to release render buffer.";
            goto Exit;
          }
        }
      }
      else
      {
        // Inform WASAPI that render was unsuccessful
        hr = renderClient->ReleaseBuffer( 0, 0 );
        if ( FAILED( hr ) ) {
          errorText = "RtApiWasapi::wasapiThread: Unable to release render buffer.";
          goto Exit;
        }
      }
    }

    // if the callback buffer was pushed renderBuffer reset callbackPulled flag
    if ( callbackPushed ) {
      // unsetting the callbackPulled flag lets the stream know that
      // the audio device is ready for another callback output buffer.
      callbackPulled = false;
    }

  }

Exit:
  // clean up
  CoTaskMemFree( captureFormat );
  CoTaskMemFree( renderFormat );

  free ( convBuffer );
  delete renderResampler;
  delete captureResampler;

  CoUninitialize();

  if ( !errorText.empty() )
  {
    errorText_ = errorText;
    error( errorType );
  }

  // update stream state
  stream_.state = STREAM_STOPPED;
}

//******************** End of __WINDOWS_WASAPI__ *********************//
#endif


#if defined(__WINDOWS_DS__) // Windows DirectSound API

// Modified by Robin Davies, October 2005
// - Improvements to DirectX pointer chasing. 
// - Bug fix for non-power-of-two Asio granularity used by Edirol PCR-A30.
// - Auto-call CoInitialize for DSOUND and ASIO platforms.
// Various revisions for RtAudio 4.0 by Gary Scavone, April 2007
// Changed device query structure for RtAudio 4.0.7, January 2010

#include <windows.h>
#include <process.h>
#include <mmsystem.h>
#include <mmreg.h>
#include <dsound.h>
#include <assert.h>
#include <algorithm>

#if defined(__MINGW32__)
  // missing from latest mingw winapi
#define WAVE_FORMAT_96M08 0x00010000 /* 96 kHz, Mono, 8-bit */
#define WAVE_FORMAT_96S08 0x00020000 /* 96 kHz, Stereo, 8-bit */
#define WAVE_FORMAT_96M16 0x00040000 /* 96 kHz, Mono, 16-bit */
#define WAVE_FORMAT_96S16 0x00080000 /* 96 kHz, Stereo, 16-bit */
#endif

#define MINIMUM_DEVICE_BUFFER_SIZE 32768

#ifdef _MSC_VER // if Microsoft Visual C++
#pragma comment( lib, "winmm.lib" ) // then, auto-link winmm.lib. Otherwise, it has to be added manually.
#endif

static inline DWORD dsPointerBetween( DWORD pointer, DWORD laterPointer, DWORD earlierPointer, DWORD bufferSize )
{
  if ( pointer > bufferSize ) pointer -= bufferSize;
  if ( laterPointer < earlierPointer ) laterPointer += bufferSize;
  if ( pointer < earlierPointer ) pointer += bufferSize;
  return pointer >= earlierPointer && pointer < laterPointer;
}

// A structure to hold various information related to the DirectSound
// API implementation.
struct DsHandle {
  unsigned int drainCounter; // Tracks callback counts when draining
  bool internalDrain;        // Indicates if stop is initiated from callback or not.
  void *id[2];
  void *buffer[2];
  bool xrun[2];
  UINT bufferPointer[2];  
  DWORD dsBufferSize[2];
  DWORD dsPointerLeadTime[2]; // the number of bytes ahead of the safe pointer to lead by.
  HANDLE condition;

  DsHandle()
    :drainCounter(0), internalDrain(false) { id[0] = 0; id[1] = 0; buffer[0] = 0; buffer[1] = 0; xrun[0] = false; xrun[1] = false; bufferPointer[0] = 0; bufferPointer[1] = 0; }
};

// Declarations for utility functions, callbacks, and structures
// specific to the DirectSound implementation.
static BOOL CALLBACK deviceQueryCallback( LPGUID lpguid,
                                          LPCTSTR description,
                                          LPCTSTR module,
                                          LPVOID lpContext );

static const char* getErrorString( int code );

static unsigned __stdcall callbackHandler( void *ptr );

struct DsDevice {
  LPGUID id;
  bool isInput;
  std::string name;
  std::string epID; // endpoint ID

  DsDevice()
    : isInput(false) {}
};

struct DsProbeData {
  bool isInput;
  std::vector<struct DsDevice>* dsDevices;
};

RtApiDs :: RtApiDs()
{
  // Dsound will run both-threaded. If CoInitialize fails, then just
  // accept whatever the mainline chose for a threading model.
  coInitialized_ = false;
  HRESULT hr = CoInitialize( NULL );
  if ( !FAILED( hr ) ) coInitialized_ = true;
}

RtApiDs :: ~RtApiDs()
{
  if ( stream_.state != STREAM_CLOSED ) closeStream();
  if ( coInitialized_ ) CoUninitialize(); // balanced call.
}

void RtApiDs :: probeDevices( void )
{
  // See list of required functionality in RtApi::probeDevices().

  // Query DirectSound devices.
  struct DsProbeData probeInfo;
  probeInfo.isInput = false;
  std::vector< struct DsDevice > devices;
  probeInfo.dsDevices = &devices;
  HRESULT result = DirectSoundEnumerate( (LPDSENUMCALLBACK) deviceQueryCallback, &probeInfo );
  if ( FAILED( result ) ) {
    errorStream_ << "RtApiDs::probeDevices: error (" << getErrorString( result ) << ") enumerating output devices!";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
  }

  // Query DirectSoundCapture devices.
  probeInfo.isInput = true;
  result = DirectSoundCaptureEnumerate( (LPDSENUMCALLBACK) deviceQueryCallback, &probeInfo );
  if ( FAILED( result ) ) {
    errorStream_ << "RtApiDs::probeDevices: error (" << getErrorString( result ) << ") enumerating input devices!";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
  }

  // Now fill or update our deviceList_ vector.
  unsigned int m, n;
  for ( n=0; n<devices.size(); n++ ) {
    for ( m=0; m<dsDevices_.size(); m++ ) {
      if ( ( dsDevices_[m].epID == devices[n].epID ) && ( devices[n].isInput == dsDevices_[m].isInput ) ) {
        dsDevices_[m].id = devices[n].id; // Update the ID, since it seems to change when devices are added/removed
        break; // We already have this device.
      }
    }
    if ( m == dsDevices_.size() ) { // new device
      RtAudio::DeviceInfo info;
      if ( probeDeviceInfo( info, devices[n] ) == false ) continue; // ignore if probe fails
      info.ID = currentDeviceId_++;  // arbitrary internal device ID
      deviceList_.push_back( info );
      dsDevices_.push_back( devices[n] );
    }
  }

  // Remove any devices left in the list that are no longer available.
  for ( std::vector< struct DsDevice >::iterator it=dsDevices_.begin(); it!=dsDevices_.end(); ) {
    for ( n=0; n<devices.size(); n++ ) {
      if ( (*it).epID == devices[n].epID ) {
        ++it;
        break;
      }
    }
    if ( n == devices.size() ) { // not found so remove it from our list
      it = dsDevices_.erase( it );
      deviceList_.erase( deviceList_.begin() + distance(dsDevices_.begin(), it ) );
    }
  }

  // Determine the default devices
  for ( n=0; n<dsDevices_.size(); n++ ) {
    if ( dsDevices_[n].id == NULL ) { // default device
      if ( dsDevices_[n].isInput )
        deviceList_[n].isDefaultInput = true;
      else
        deviceList_[n].isDefaultOutput = true;
    }
    else if ( dsDevices_[n].isInput )
      deviceList_[n].isDefaultInput = false;
    else
      deviceList_[n].isDefaultOutput = false;
  }
}

bool RtApiDs :: probeDeviceInfo( RtAudio::DeviceInfo &info, DsDevice &dsDevice )
{
  // Devices will either be input or output devices but not both.
  HRESULT result;
  if ( dsDevice.isInput ) goto probeInput;

  LPDIRECTSOUND output;
  DSCAPS outCaps;
  result = DirectSoundCreate( dsDevice.id, &output, NULL );
  if ( FAILED( result ) ) {
    errorStream_ << "RtApiDs::probeDeviceInfo: error (" << getErrorString( result ) << ") opening output device (" << dsDevice.name << ")!";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    goto probeInput;
  }

  outCaps.dwSize = sizeof( outCaps );
  result = output->GetCaps( &outCaps );
  if ( FAILED( result ) ) {
    output->Release();
    errorStream_ << "RtApiDs::probeDeviceInfo: error (" << getErrorString( result ) << ") getting capabilities!";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    goto probeInput;
  }

  // Get output channel information.
  info.outputChannels = ( outCaps.dwFlags & DSCAPS_PRIMARYSTEREO ) ? 2 : 1;

  // Get sample rate information.
  info.sampleRates.clear();
  for ( unsigned int k=0; k<MAX_SAMPLE_RATES; k++ ) {
    if ( SAMPLE_RATES[k] >= (unsigned int) outCaps.dwMinSecondarySampleRate &&
         SAMPLE_RATES[k] <= (unsigned int) outCaps.dwMaxSecondarySampleRate ) {
      info.sampleRates.push_back( SAMPLE_RATES[k] );

      if ( !info.preferredSampleRate || ( SAMPLE_RATES[k] <= 48000 && SAMPLE_RATES[k] > info.preferredSampleRate ) )
        info.preferredSampleRate = SAMPLE_RATES[k];
    }
  }

  // Get format information.
  if ( outCaps.dwFlags & DSCAPS_PRIMARY16BIT ) info.nativeFormats |= RTAUDIO_SINT16;
  if ( outCaps.dwFlags & DSCAPS_PRIMARY8BIT ) info.nativeFormats |= RTAUDIO_SINT8;

  output->Release();

  info.name = dsDevice.name;
  return true;

 probeInput:

  LPDIRECTSOUNDCAPTURE input;
  result = DirectSoundCaptureCreate( dsDevice.id, &input, NULL );
  if ( FAILED( result ) ) {
    errorStream_ << "RtApiDs::probeDeviceInfo: error (" << getErrorString( result ) << ") opening input device (" << dsDevice.name << ")!";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  DSCCAPS inCaps;
  inCaps.dwSize = sizeof( inCaps );
  result = input->GetCaps( &inCaps );
  if ( FAILED( result ) ) {
    input->Release();
    errorStream_ << "RtApiDs::probeDeviceInfo: error (" << getErrorString( result ) << ") getting object capabilities (" << dsDevice.name << ")!";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  // Get input channel information.
  info.inputChannels = inCaps.dwChannels;

  // Get sample rate and format information.
  std::vector<unsigned int> rates;
  if ( inCaps.dwChannels >= 2 ) {
    if ( inCaps.dwFormats & WAVE_FORMAT_1S16 ) info.nativeFormats |= RTAUDIO_SINT16;
    if ( inCaps.dwFormats & WAVE_FORMAT_2S16 ) info.nativeFormats |= RTAUDIO_SINT16;
    if ( inCaps.dwFormats & WAVE_FORMAT_4S16 ) info.nativeFormats |= RTAUDIO_SINT16;
    if ( inCaps.dwFormats & WAVE_FORMAT_96S16 ) info.nativeFormats |= RTAUDIO_SINT16;
    if ( inCaps.dwFormats & WAVE_FORMAT_1S08 ) info.nativeFormats |= RTAUDIO_SINT8;
    if ( inCaps.dwFormats & WAVE_FORMAT_2S08 ) info.nativeFormats |= RTAUDIO_SINT8;
    if ( inCaps.dwFormats & WAVE_FORMAT_4S08 ) info.nativeFormats |= RTAUDIO_SINT8;
    if ( inCaps.dwFormats & WAVE_FORMAT_96S08 ) info.nativeFormats |= RTAUDIO_SINT8;

    if ( info.nativeFormats & RTAUDIO_SINT16 ) {
      if ( inCaps.dwFormats & WAVE_FORMAT_1S16 ) rates.push_back( 11025 );
      if ( inCaps.dwFormats & WAVE_FORMAT_2S16 ) rates.push_back( 22050 );
      if ( inCaps.dwFormats & WAVE_FORMAT_4S16 ) rates.push_back( 44100 );
      if ( inCaps.dwFormats & WAVE_FORMAT_96S16 ) rates.push_back( 96000 );
    }
    else if ( info.nativeFormats & RTAUDIO_SINT8 ) {
      if ( inCaps.dwFormats & WAVE_FORMAT_1S08 ) rates.push_back( 11025 );
      if ( inCaps.dwFormats & WAVE_FORMAT_2S08 ) rates.push_back( 22050 );
      if ( inCaps.dwFormats & WAVE_FORMAT_4S08 ) rates.push_back( 44100 );
      if ( inCaps.dwFormats & WAVE_FORMAT_96S08 ) rates.push_back( 96000 );
    }
  }
  else if ( inCaps.dwChannels == 1 ) {
    if ( inCaps.dwFormats & WAVE_FORMAT_1M16 ) info.nativeFormats |= RTAUDIO_SINT16;
    if ( inCaps.dwFormats & WAVE_FORMAT_2M16 ) info.nativeFormats |= RTAUDIO_SINT16;
    if ( inCaps.dwFormats & WAVE_FORMAT_4M16 ) info.nativeFormats |= RTAUDIO_SINT16;
    if ( inCaps.dwFormats & WAVE_FORMAT_96M16 ) info.nativeFormats |= RTAUDIO_SINT16;
    if ( inCaps.dwFormats & WAVE_FORMAT_1M08 ) info.nativeFormats |= RTAUDIO_SINT8;
    if ( inCaps.dwFormats & WAVE_FORMAT_2M08 ) info.nativeFormats |= RTAUDIO_SINT8;
    if ( inCaps.dwFormats & WAVE_FORMAT_4M08 ) info.nativeFormats |= RTAUDIO_SINT8;
    if ( inCaps.dwFormats & WAVE_FORMAT_96M08 ) info.nativeFormats |= RTAUDIO_SINT8;

    if ( info.nativeFormats & RTAUDIO_SINT16 ) {
      if ( inCaps.dwFormats & WAVE_FORMAT_1M16 ) rates.push_back( 11025 );
      if ( inCaps.dwFormats & WAVE_FORMAT_2M16 ) rates.push_back( 22050 );
      if ( inCaps.dwFormats & WAVE_FORMAT_4M16 ) rates.push_back( 44100 );
      if ( inCaps.dwFormats & WAVE_FORMAT_96M16 ) rates.push_back( 96000 );
    }
    else if ( info.nativeFormats & RTAUDIO_SINT8 ) {
      if ( inCaps.dwFormats & WAVE_FORMAT_1M08 ) rates.push_back( 11025 );
      if ( inCaps.dwFormats & WAVE_FORMAT_2M08 ) rates.push_back( 22050 );
      if ( inCaps.dwFormats & WAVE_FORMAT_4M08 ) rates.push_back( 44100 );
      if ( inCaps.dwFormats & WAVE_FORMAT_96M08 ) rates.push_back( 96000 );
    }
  }
  else info.inputChannels = 0; // technically, this would be an error

  input->Release();

  if ( info.inputChannels == 0 ) return false;

  // Copy the supported rates to the info structure but avoid duplication.
  bool found;
  for ( unsigned int i=0; i<rates.size(); i++ ) {
    found = false;
    for ( unsigned int j=0; j<info.sampleRates.size(); j++ ) {
      if ( rates[i] == info.sampleRates[j] ) {
        found = true;
        break;
      }
    }
    if ( found == false ) info.sampleRates.push_back( rates[i] );
  }
  std::sort( info.sampleRates.begin(), info.sampleRates.end() );
  for ( unsigned int i=0; i<info.sampleRates.size(); i++ ) {
    if ( !info.preferredSampleRate || ( info.sampleRates[i] <= 48000 && info.sampleRates[i] > info.preferredSampleRate ) )
      info.preferredSampleRate = info.sampleRates[i];
  }

  // Copy name and return.
  info.name = dsDevice.name;
  return true;
}

bool RtApiDs :: probeDeviceOpen( unsigned int deviceId, StreamMode mode, unsigned int channels,
                                 unsigned int firstChannel, unsigned int sampleRate,
                                 RtAudioFormat format, unsigned int *bufferSize,
                                 RtAudio::StreamOptions *options )
{
  if ( channels + firstChannel > 2 ) {
    errorText_ = "RtApiDs::probeDeviceOpen: DirectSound does not support more than 2 channels per device.";
    return FAILURE;
  }

  size_t nDevices = dsDevices_.size();
  if ( nDevices == 0 ) {
    // This should not happen because a check is made before this function is called.
    errorText_ = "RtApiDs::probeDeviceOpen: no devices found!";
    return FAILURE;
  }

  int deviceIdx = -1;
  for ( unsigned int m=0; m<deviceList_.size(); m++ ) {
    if ( deviceList_[m].ID == deviceId ) {
      deviceIdx = m;
      break;
    }
  }

  if ( deviceIdx < 0 ) {
    errorText_ = "RtApiDs::probeDeviceOpen: device ID is invalid!";
    return FAILURE;
  }

  if ( mode == OUTPUT ) {
    if ( dsDevices_[ deviceIdx ].isInput ) {
      errorStream_ << "RtApiDs::probeDeviceOpen: device (" << deviceIdx << ") does not support output!";
      errorText_ = errorStream_.str();
      return FAILURE;
    }
  }
  else { // mode == INPUT
    if ( dsDevices_[ deviceIdx ].isInput == false ) {
      errorStream_ << "RtApiDs::probeDeviceOpen: device (" << deviceIdx << ") does not support input!";
      errorText_ = errorStream_.str();
      return FAILURE;
    }
  }

  // According to a note in PortAudio, using GetDesktopWindow()
  // instead of GetForegroundWindow() is supposed to avoid problems
  // that occur when the application's window is not the foreground
  // window.  Also, if the application window closes before the
  // DirectSound buffer, DirectSound can crash.  In the past, I had
  // problems when using GetDesktopWindow() but it seems fine now
  // (January 2010).  I'll leave it commented here.
  // HWND hWnd = GetForegroundWindow();
  HWND hWnd = GetDesktopWindow();

  // Check the numberOfBuffers parameter and limit the lowest value to
  // two.  This is a judgement call and a value of two is probably too
  // low for capture, but it should work for playback.
  int nBuffers = 0;
  if ( options ) nBuffers = options->numberOfBuffers;
  if ( options && options->flags & RTAUDIO_MINIMIZE_LATENCY ) nBuffers = 2;
  if ( nBuffers < 2 ) nBuffers = 3;

  // Check the lower range of the user-specified buffer size and set
  // (arbitrarily) to a lower bound of 32.
  if ( *bufferSize < 32 ) *bufferSize = 32;

  // Create the wave format structure.  The data format setting will
  // be determined later.
  WAVEFORMATEX waveFormat;
  ZeroMemory( &waveFormat, sizeof(WAVEFORMATEX) );
  waveFormat.wFormatTag = WAVE_FORMAT_PCM;
  waveFormat.nChannels = channels + firstChannel;
  waveFormat.nSamplesPerSec = (unsigned long) sampleRate;

  // Determine the device buffer size. By default, we'll use the value
  // defined above (32K), but we will grow it to make allowances for
  // very large software buffer sizes.
  DWORD dsBufferSize = MINIMUM_DEVICE_BUFFER_SIZE;
  DWORD dsPointerLeadTime = 0;

  void *ohandle = 0, *bhandle = 0;
  HRESULT result;
  if ( mode == OUTPUT ) {

    LPDIRECTSOUND output;
    result = DirectSoundCreate( dsDevices_[ deviceIdx ].id, &output, NULL );
    if ( FAILED( result ) ) {
      errorStream_ << "RtApiDs::probeDeviceOpen: error (" << getErrorString( result ) << ") opening output device (" << dsDevices_[ deviceIdx ].name << ")!";
      errorText_ = errorStream_.str();
      return FAILURE;
    }

    DSCAPS outCaps;
    outCaps.dwSize = sizeof( outCaps );
    result = output->GetCaps( &outCaps );
    if ( FAILED( result ) ) {
      output->Release();
      errorStream_ << "RtApiDs::probeDeviceOpen: error (" << getErrorString( result ) << ") getting capabilities (" << dsDevices_[ deviceIdx ].name << ")!";
      errorText_ = errorStream_.str();
      return FAILURE;
    }

    // Check channel information.
    if ( channels + firstChannel == 2 && !( outCaps.dwFlags & DSCAPS_PRIMARYSTEREO ) ) {
      errorStream_ << "RtApiDs::probeDeviceInfo: the output device (" << dsDevices_[ deviceIdx ].name << ") does not support stereo playback.";
      errorText_ = errorStream_.str();
      return FAILURE;
    }

    // Check format information.  Use 16-bit format unless not
    // supported or user requests 8-bit.
    if ( outCaps.dwFlags & DSCAPS_PRIMARY16BIT &&
         !( format == RTAUDIO_SINT8 && outCaps.dwFlags & DSCAPS_PRIMARY8BIT ) ) {
      waveFormat.wBitsPerSample = 16;
      stream_.deviceFormat[mode] = RTAUDIO_SINT16;
    }
    else {
      waveFormat.wBitsPerSample = 8;
      stream_.deviceFormat[mode] = RTAUDIO_SINT8;
    }
    stream_.userFormat = format;

    // Update wave format structure and buffer information.
    waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    dsPointerLeadTime = nBuffers * (*bufferSize) * (waveFormat.wBitsPerSample / 8) * channels;

    // If the user wants an even bigger buffer, increase the device buffer size accordingly.
    while ( dsPointerLeadTime * 2U > dsBufferSize )
      dsBufferSize *= 2;

    // Set cooperative level to DSSCL_EXCLUSIVE ... sound stops when window focus changes.
    // result = output->SetCooperativeLevel( hWnd, DSSCL_EXCLUSIVE );
    // Set cooperative level to DSSCL_PRIORITY ... sound remains when window focus changes.
    result = output->SetCooperativeLevel( hWnd, DSSCL_PRIORITY );
    if ( FAILED( result ) ) {
      output->Release();
      errorStream_ << "RtApiDs::probeDeviceOpen: error (" << getErrorString( result ) << ") setting cooperative level (" << dsDevices_[ deviceIdx ].name << ")!";
      errorText_ = errorStream_.str();
      return FAILURE;
    }

    // Even though we will write to the secondary buffer, we need to
    // access the primary buffer to set the correct output format
    // (since the default is 8-bit, 22 kHz!).  Setup the DS primary
    // buffer description.
    DSBUFFERDESC bufferDescription;
    ZeroMemory( &bufferDescription, sizeof( DSBUFFERDESC ) );
    bufferDescription.dwSize = sizeof( DSBUFFERDESC );
    bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

    // Obtain the primary buffer
    LPDIRECTSOUNDBUFFER buffer;
    result = output->CreateSoundBuffer( &bufferDescription, &buffer, NULL );
    if ( FAILED( result ) ) {
      output->Release();
      errorStream_ << "RtApiDs::probeDeviceOpen: error (" << getErrorString( result ) << ") accessing primary buffer (" << dsDevices_[ deviceIdx ].name << ")!";
      errorText_ = errorStream_.str();
      return FAILURE;
    }

    // Set the primary DS buffer sound format.
    result = buffer->SetFormat( &waveFormat );
    if ( FAILED( result ) ) {
      output->Release();
      errorStream_ << "RtApiDs::probeDeviceOpen: error (" << getErrorString( result ) << ") setting primary buffer format (" << dsDevices_[ deviceIdx ].name << ")!";
      errorText_ = errorStream_.str();
      return FAILURE;
    }

    // Setup the secondary DS buffer description.
    ZeroMemory( &bufferDescription, sizeof( DSBUFFERDESC ) );
    bufferDescription.dwSize = sizeof( DSBUFFERDESC );
    bufferDescription.dwFlags = ( DSBCAPS_STICKYFOCUS |
                                  DSBCAPS_GLOBALFOCUS |
                                  DSBCAPS_GETCURRENTPOSITION2 |
                                  DSBCAPS_LOCHARDWARE );  // Force hardware mixing
    bufferDescription.dwBufferBytes = dsBufferSize;
    bufferDescription.lpwfxFormat = &waveFormat;

    // Try to create the secondary DS buffer.  If that doesn't work,
    // try to use software mixing.  Otherwise, there's a problem.
    result = output->CreateSoundBuffer( &bufferDescription, &buffer, NULL );
    if ( FAILED( result ) ) {
      bufferDescription.dwFlags = ( DSBCAPS_STICKYFOCUS |
                                    DSBCAPS_GLOBALFOCUS |
                                    DSBCAPS_GETCURRENTPOSITION2 |
                                    DSBCAPS_LOCSOFTWARE );  // Force software mixing
      result = output->CreateSoundBuffer( &bufferDescription, &buffer, NULL );
      if ( FAILED( result ) ) {
        output->Release();
        errorStream_ << "RtApiDs::probeDeviceOpen: error (" << getErrorString( result ) << ") creating secondary buffer (" << dsDevices_[ deviceIdx ].name << ")!";
        errorText_ = errorStream_.str();
        return FAILURE;
      }
    }

    // Get the buffer size ... might be different from what we specified.
    DSBCAPS dsbcaps;
    dsbcaps.dwSize = sizeof( DSBCAPS );
    result = buffer->GetCaps( &dsbcaps );
    if ( FAILED( result ) ) {
      output->Release();
      buffer->Release();
      errorStream_ << "RtApiDs::probeDeviceOpen: error (" << getErrorString( result ) << ") getting buffer settings (" << dsDevices_[ deviceIdx ].name << ")!";
      errorText_ = errorStream_.str();
      return FAILURE;
    }

    dsBufferSize = dsbcaps.dwBufferBytes;

    // Lock the DS buffer
    LPVOID audioPtr;
    DWORD dataLen;
    result = buffer->Lock( 0, dsBufferSize, &audioPtr, &dataLen, NULL, NULL, 0 );
    if ( FAILED( result ) ) {
      output->Release();
      buffer->Release();
      errorStream_ << "RtApiDs::probeDeviceOpen: error (" << getErrorString( result ) << ") locking buffer (" << dsDevices_[ deviceIdx ].name << ")!";
      errorText_ = errorStream_.str();
      return FAILURE;
    }

    // Zero the DS buffer
    ZeroMemory( audioPtr, dataLen );

    // Unlock the DS buffer
    result = buffer->Unlock( audioPtr, dataLen, NULL, 0 );
    if ( FAILED( result ) ) {
      output->Release();
      buffer->Release();
      errorStream_ << "RtApiDs::probeDeviceOpen: error (" << getErrorString( result ) << ") unlocking buffer (" << dsDevices_[ deviceIdx ].name << ")!";
      errorText_ = errorStream_.str();
      return FAILURE;
    }

    ohandle = (void *) output;
    bhandle = (void *) buffer;
  }

  if ( mode == INPUT ) {

    LPDIRECTSOUNDCAPTURE input;
    result = DirectSoundCaptureCreate( dsDevices_[ deviceIdx ].id, &input, NULL );
    if ( FAILED( result ) ) {
      errorStream_ << "RtApiDs::probeDeviceOpen: error (" << getErrorString( result ) << ") opening input device (" << dsDevices_[ deviceIdx ].name << ")!";
      errorText_ = errorStream_.str();
      return FAILURE;
    }

    DSCCAPS inCaps;
    inCaps.dwSize = sizeof( inCaps );
    result = input->GetCaps( &inCaps );
    if ( FAILED( result ) ) {
      input->Release();
      errorStream_ << "RtApiDs::probeDeviceOpen: error (" << getErrorString( result ) << ") getting input capabilities (" << dsDevices_[ deviceIdx ].name << ")!";
      errorText_ = errorStream_.str();
      return FAILURE;
    }

    // Check channel information.
    if ( inCaps.dwChannels < channels + firstChannel ) {
      errorText_ = "RtApiDs::probeDeviceInfo: the input device does not support requested input channels.";
      return FAILURE;
    }

    // Check format information.  Use 16-bit format unless user
    // requests 8-bit.
    DWORD deviceFormats;
    if ( channels + firstChannel == 2 ) {
      deviceFormats = WAVE_FORMAT_1S08 | WAVE_FORMAT_2S08 | WAVE_FORMAT_4S08 | WAVE_FORMAT_96S08;
      if ( format == RTAUDIO_SINT8 && inCaps.dwFormats & deviceFormats ) {
        waveFormat.wBitsPerSample = 8;
        stream_.deviceFormat[mode] = RTAUDIO_SINT8;
      }
      else { // assume 16-bit is supported
        waveFormat.wBitsPerSample = 16;
        stream_.deviceFormat[mode] = RTAUDIO_SINT16;
      }
    }
    else { // channel == 1
      deviceFormats = WAVE_FORMAT_1M08 | WAVE_FORMAT_2M08 | WAVE_FORMAT_4M08 | WAVE_FORMAT_96M08;
      if ( format == RTAUDIO_SINT8 && inCaps.dwFormats & deviceFormats ) {
        waveFormat.wBitsPerSample = 8;
        stream_.deviceFormat[mode] = RTAUDIO_SINT8;
      }
      else { // assume 16-bit is supported
        waveFormat.wBitsPerSample = 16;
        stream_.deviceFormat[mode] = RTAUDIO_SINT16;
      }
    }
    stream_.userFormat = format;

    // Update wave format structure and buffer information.
    waveFormat.nBlockAlign = waveFormat.nChannels * waveFormat.wBitsPerSample / 8;
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
    dsPointerLeadTime = nBuffers * (*bufferSize) * (waveFormat.wBitsPerSample / 8) * channels;

    // If the user wants an even bigger buffer, increase the device buffer size accordingly.
    while ( dsPointerLeadTime * 2U > dsBufferSize )
      dsBufferSize *= 2;

    // Setup the secondary DS buffer description.
    DSCBUFFERDESC bufferDescription;
    ZeroMemory( &bufferDescription, sizeof( DSCBUFFERDESC ) );
    bufferDescription.dwSize = sizeof( DSCBUFFERDESC );
    bufferDescription.dwFlags = 0;
    bufferDescription.dwReserved = 0;
    bufferDescription.dwBufferBytes = dsBufferSize;
    bufferDescription.lpwfxFormat = &waveFormat;

    // Create the capture buffer.
    LPDIRECTSOUNDCAPTUREBUFFER buffer;
    result = input->CreateCaptureBuffer( &bufferDescription, &buffer, NULL );
    if ( FAILED( result ) ) {
      input->Release();
      errorStream_ << "RtApiDs::probeDeviceOpen: error (" << getErrorString( result ) << ") creating input buffer (" << dsDevices_[ deviceIdx ].name << ")!";
      errorText_ = errorStream_.str();
      return FAILURE;
    }

    // Get the buffer size ... might be different from what we specified.
    DSCBCAPS dscbcaps;
    dscbcaps.dwSize = sizeof( DSCBCAPS );
    result = buffer->GetCaps( &dscbcaps );
    if ( FAILED( result ) ) {
      input->Release();
      buffer->Release();
      errorStream_ << "RtApiDs::probeDeviceOpen: error (" << getErrorString( result ) << ") getting buffer settings (" << dsDevices_[ deviceIdx ].name << ")!";
      errorText_ = errorStream_.str();
      return FAILURE;
    }

    dsBufferSize = dscbcaps.dwBufferBytes;

    // NOTE: We could have a problem here if this is a duplex stream
    // and the play and capture hardware buffer sizes are different
    // (I'm actually not sure if that is a problem or not).
    // Currently, we are not verifying that.

    // Lock the capture buffer
    LPVOID audioPtr;
    DWORD dataLen;
    result = buffer->Lock( 0, dsBufferSize, &audioPtr, &dataLen, NULL, NULL, 0 );
    if ( FAILED( result ) ) {
      input->Release();
      buffer->Release();
      errorStream_ << "RtApiDs::probeDeviceOpen: error (" << getErrorString( result ) << ") locking input buffer (" << dsDevices_[ deviceIdx ].name << ")!";
      errorText_ = errorStream_.str();
      return FAILURE;
    }

    // Zero the buffer
    ZeroMemory( audioPtr, dataLen );

    // Unlock the buffer
    result = buffer->Unlock( audioPtr, dataLen, NULL, 0 );
    if ( FAILED( result ) ) {
      input->Release();
      buffer->Release();
      errorStream_ << "RtApiDs::probeDeviceOpen: error (" << getErrorString( result ) << ") unlocking input buffer (" << dsDevices_[ deviceIdx ].name << ")!";
      errorText_ = errorStream_.str();
      return FAILURE;
    }

    ohandle = (void *) input;
    bhandle = (void *) buffer;
  }

  // Set various stream parameters
  DsHandle *handle = 0;
  stream_.nDeviceChannels[mode] = channels + firstChannel;
  stream_.nUserChannels[mode] = channels;
  stream_.bufferSize = *bufferSize;
  stream_.channelOffset[mode] = firstChannel;
  stream_.deviceInterleaved[mode] = true;
  if ( options && options->flags & RTAUDIO_NONINTERLEAVED ) stream_.userInterleaved = false;
  else stream_.userInterleaved = true;

  // Set flag for buffer conversion
  stream_.doConvertBuffer[mode] = false;
  if (stream_.nUserChannels[mode] != stream_.nDeviceChannels[mode])
    stream_.doConvertBuffer[mode] = true;
  if (stream_.userFormat != stream_.deviceFormat[mode])
    stream_.doConvertBuffer[mode] = true;
  if ( stream_.userInterleaved != stream_.deviceInterleaved[mode] &&
       stream_.nUserChannels[mode] > 1 )
    stream_.doConvertBuffer[mode] = true;

  // Allocate necessary internal buffers
  long bufferBytes = stream_.nUserChannels[mode] * *bufferSize * formatBytes( stream_.userFormat );
  stream_.userBuffer[mode] = (char *) calloc( bufferBytes, 1 );
  if ( stream_.userBuffer[mode] == NULL ) {
    errorText_ = "RtApiDs::probeDeviceOpen: error allocating user buffer memory.";
    goto error;
  }

  if ( stream_.doConvertBuffer[mode] ) {

    bool makeBuffer = true;
    bufferBytes = stream_.nDeviceChannels[mode] * formatBytes( stream_.deviceFormat[mode] );
    if ( mode == INPUT ) {
      if ( stream_.mode == OUTPUT && stream_.deviceBuffer ) {
        unsigned long bytesOut = stream_.nDeviceChannels[0] * formatBytes( stream_.deviceFormat[0] );
        if ( bufferBytes <= (long) bytesOut ) makeBuffer = false;
      }
    }

    if ( makeBuffer ) {
      bufferBytes *= *bufferSize;
      if ( stream_.deviceBuffer ) free( stream_.deviceBuffer );
      stream_.deviceBuffer = (char *) calloc( bufferBytes, 1 );
      if ( stream_.deviceBuffer == NULL ) {
        errorText_ = "RtApiDs::probeDeviceOpen: error allocating device buffer memory.";
        goto error;
      }
    }
  }

  // Allocate our DsHandle structures for the stream.
  if ( stream_.apiHandle == 0 ) {
    try {
      handle = new DsHandle;
    }
    catch ( std::bad_alloc& ) {
      errorText_ = "RtApiDs::probeDeviceOpen: error allocating AsioHandle memory.";
      goto error;
    }

    // Create a manual-reset event.
    handle->condition = CreateEvent( NULL,   // no security
                                     TRUE,   // manual-reset
                                     FALSE,  // non-signaled initially
                                     NULL ); // unnamed
    stream_.apiHandle = (void *) handle;
  }
  else
    handle = (DsHandle *) stream_.apiHandle;
  handle->id[mode] = ohandle;
  handle->buffer[mode] = bhandle;
  handle->dsBufferSize[mode] = dsBufferSize;
  handle->dsPointerLeadTime[mode] = dsPointerLeadTime;

  stream_.deviceId[mode] = deviceIdx;
  stream_.state = STREAM_STOPPED;
  if ( stream_.mode == OUTPUT && mode == INPUT )
    // We had already set up an output stream.
    stream_.mode = DUPLEX;
  else
    stream_.mode = mode;
  stream_.nBuffers = nBuffers;
  stream_.sampleRate = sampleRate;

  // Setup the buffer conversion information structure.
  if ( stream_.doConvertBuffer[mode] ) setConvertInfo( mode, firstChannel );

  // Setup the callback thread.
  if ( stream_.callbackInfo.isRunning == false ) {
    unsigned threadId;
    stream_.callbackInfo.isRunning = true;
    stream_.callbackInfo.object = (void *) this;
    stream_.callbackInfo.thread = _beginthreadex( NULL, 0, &callbackHandler,
                                                  &stream_.callbackInfo, 0, &threadId );
    if ( stream_.callbackInfo.thread == 0 ) {
      errorText_ = "RtApiDs::probeDeviceOpen: error creating callback thread!";
      goto error;
    }

    // Boost DS thread priority
    SetThreadPriority( (HANDLE) stream_.callbackInfo.thread, THREAD_PRIORITY_HIGHEST );
  }
  return SUCCESS;

 error:
  if ( handle ) {
    if ( handle->buffer[0] ) { // the object pointer can be NULL and valid
      LPDIRECTSOUND object = (LPDIRECTSOUND) handle->id[0];
      LPDIRECTSOUNDBUFFER buffer = (LPDIRECTSOUNDBUFFER) handle->buffer[0];
      if ( buffer ) buffer->Release();
      object->Release();
    }
    if ( handle->buffer[1] ) {
      LPDIRECTSOUNDCAPTURE object = (LPDIRECTSOUNDCAPTURE) handle->id[1];
      LPDIRECTSOUNDCAPTUREBUFFER buffer = (LPDIRECTSOUNDCAPTUREBUFFER) handle->buffer[1];
      if ( buffer ) buffer->Release();
      object->Release();
    }
    CloseHandle( handle->condition );
    delete handle;
    stream_.apiHandle = 0;
  }

  for ( int i=0; i<2; i++ ) {
    if ( stream_.userBuffer[i] ) {
      free( stream_.userBuffer[i] );
      stream_.userBuffer[i] = 0;
    }
  }

  if ( stream_.deviceBuffer ) {
    free( stream_.deviceBuffer );
    stream_.deviceBuffer = 0;
  }

  stream_.state = STREAM_CLOSED;
  return FAILURE;
}

void RtApiDs :: closeStream()
{
  if ( stream_.state == STREAM_CLOSED ) {
    errorText_ = "RtApiDs::closeStream(): no open stream to close!";
    error( RTAUDIO_WARNING );
    return;
  }

  // Stop the callback thread.
  stream_.callbackInfo.isRunning = false;
  WaitForSingleObject( (HANDLE) stream_.callbackInfo.thread, INFINITE );
  CloseHandle( (HANDLE) stream_.callbackInfo.thread );

  DsHandle *handle = (DsHandle *) stream_.apiHandle;
  if ( handle ) {
    if ( handle->buffer[0] ) { // the object pointer can be NULL and valid
      LPDIRECTSOUND object = (LPDIRECTSOUND) handle->id[0];
      LPDIRECTSOUNDBUFFER buffer = (LPDIRECTSOUNDBUFFER) handle->buffer[0];
      if ( buffer ) {
        buffer->Stop();
        buffer->Release();
      }
      object->Release();
    }
    if ( handle->buffer[1] ) {
      LPDIRECTSOUNDCAPTURE object = (LPDIRECTSOUNDCAPTURE) handle->id[1];
      LPDIRECTSOUNDCAPTUREBUFFER buffer = (LPDIRECTSOUNDCAPTUREBUFFER) handle->buffer[1];
      if ( buffer ) {
        buffer->Stop();
        buffer->Release();
      }
      object->Release();
    }
    CloseHandle( handle->condition );
    delete handle;
    stream_.apiHandle = 0;
  }

  for ( int i=0; i<2; i++ ) {
    if ( stream_.userBuffer[i] ) {
      free( stream_.userBuffer[i] );
      stream_.userBuffer[i] = 0;
    }
  }

  if ( stream_.deviceBuffer ) {
    free( stream_.deviceBuffer );
    stream_.deviceBuffer = 0;
  }

  clearStreamInfo();
  //stream_.mode = UNINITIALIZED;
  //stream_.state = STREAM_CLOSED;
}

RtAudioErrorType RtApiDs :: startStream()
{
  if ( stream_.state != STREAM_STOPPED ) {
    if ( stream_.state == STREAM_RUNNING )
      errorText_ = "RtApiDs::startStream(): the stream is already running!";
    else if ( stream_.state == STREAM_STOPPING || stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiDs::startStream(): the stream is stopping or closed!";
    return error( RTAUDIO_WARNING );
  }

  /*
  #if defined( HAVE_GETTIMEOFDAY )
  gettimeofday( &stream_.lastTickTimestamp, NULL );
  #endif
  */
  
  DsHandle *handle = (DsHandle *) stream_.apiHandle;

  // Increase scheduler frequency on lesser windows (a side-effect of
  // increasing timer accuracy).  On greater windows (Win2K or later),
  // this is already in effect.
  timeBeginPeriod( 1 ); 

  buffersRolling = false;
  duplexPrerollBytes = 0;

  if ( stream_.mode == DUPLEX ) {
    // 0.5 seconds of silence in DUPLEX mode while the devices spin up and synchronize.
    duplexPrerollBytes = (int) ( 0.5 * stream_.sampleRate * formatBytes( stream_.deviceFormat[1] ) * stream_.nDeviceChannels[1] );
  }

  HRESULT result = 0;
  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {

    LPDIRECTSOUNDBUFFER buffer = (LPDIRECTSOUNDBUFFER) handle->buffer[0];
    result = buffer->Play( 0, 0, DSBPLAY_LOOPING );
    if ( FAILED( result ) ) {
      errorStream_ << "RtApiDs::startStream: error (" << getErrorString( result ) << ") starting output buffer!";
      errorText_ = errorStream_.str();
      goto unlock;
    }
  }

  if ( stream_.mode == INPUT || stream_.mode == DUPLEX ) {

    LPDIRECTSOUNDCAPTUREBUFFER buffer = (LPDIRECTSOUNDCAPTUREBUFFER) handle->buffer[1];
    result = buffer->Start( DSCBSTART_LOOPING );
    if ( FAILED( result ) ) {
      errorStream_ << "RtApiDs::startStream: error (" << getErrorString( result ) << ") starting input buffer!";
      errorText_ = errorStream_.str();
      goto unlock;
    }
  }

  handle->drainCounter = 0;
  handle->internalDrain = false;
  ResetEvent( handle->condition );
  stream_.state = STREAM_RUNNING;

 unlock:
  if ( FAILED( result ) ) error( RTAUDIO_SYSTEM_ERROR );
  return RTAUDIO_NO_ERROR;
}

RtAudioErrorType RtApiDs :: stopStream()
{
  if ( stream_.state != STREAM_RUNNING && stream_.state != STREAM_STOPPING ) {
    if ( stream_.state == STREAM_STOPPED )
      errorText_ = "RtApiDs::stopStream(): the stream is already stopped!";
    else if ( stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiDs::stopStream(): the stream is closed!";
    return error( RTAUDIO_WARNING );
  }

  HRESULT result = 0;
  LPVOID audioPtr;
  DWORD dataLen;
  DsHandle *handle = (DsHandle *) stream_.apiHandle;
  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {
    if ( handle->drainCounter == 0 ) {
      handle->drainCounter = 2;
      WaitForSingleObject( handle->condition, INFINITE );  // block until signaled
    }

    stream_.state = STREAM_STOPPED;

    MUTEX_LOCK( &stream_.mutex );

    // Stop the buffer and clear memory
    LPDIRECTSOUNDBUFFER buffer = (LPDIRECTSOUNDBUFFER) handle->buffer[0];
    result = buffer->Stop();
    if ( FAILED( result ) ) {
      errorStream_ << "RtApiDs::stopStream: error (" << getErrorString( result ) << ") stopping output buffer!";
      errorText_ = errorStream_.str();
      goto unlock;
    }

    // Lock the buffer and clear it so that if we start to play again,
    // we won't have old data playing.
    result = buffer->Lock( 0, handle->dsBufferSize[0], &audioPtr, &dataLen, NULL, NULL, 0 );
    if ( FAILED( result ) ) {
      errorStream_ << "RtApiDs::stopStream: error (" << getErrorString( result ) << ") locking output buffer!";
      errorText_ = errorStream_.str();
      goto unlock;
    }

    // Zero the DS buffer
    ZeroMemory( audioPtr, dataLen );

    // Unlock the DS buffer
    result = buffer->Unlock( audioPtr, dataLen, NULL, 0 );
    if ( FAILED( result ) ) {
      errorStream_ << "RtApiDs::stopStream: error (" << getErrorString( result ) << ") unlocking output buffer!";
      errorText_ = errorStream_.str();
      goto unlock;
    }

    // If we start playing again, we must begin at beginning of buffer.
    handle->bufferPointer[0] = 0;
  }

  if ( stream_.mode == INPUT || stream_.mode == DUPLEX ) {
    LPDIRECTSOUNDCAPTUREBUFFER buffer = (LPDIRECTSOUNDCAPTUREBUFFER) handle->buffer[1];
    audioPtr = NULL;
    dataLen = 0;

    stream_.state = STREAM_STOPPED;

    if ( stream_.mode != DUPLEX )
      MUTEX_LOCK( &stream_.mutex );

    result = buffer->Stop();
    if ( FAILED( result ) ) {
      errorStream_ << "RtApiDs::stopStream: error (" << getErrorString( result ) << ") stopping input buffer!";
      errorText_ = errorStream_.str();
      goto unlock;
    }

    // Lock the buffer and clear it so that if we start to play again,
    // we won't have old data playing.
    result = buffer->Lock( 0, handle->dsBufferSize[1], &audioPtr, &dataLen, NULL, NULL, 0 );
    if ( FAILED( result ) ) {
      errorStream_ << "RtApiDs::stopStream: error (" << getErrorString( result ) << ") locking input buffer!";
      errorText_ = errorStream_.str();
      goto unlock;
    }

    // Zero the DS buffer
    ZeroMemory( audioPtr, dataLen );

    // Unlock the DS buffer
    result = buffer->Unlock( audioPtr, dataLen, NULL, 0 );
    if ( FAILED( result ) ) {
      errorStream_ << "RtApiDs::stopStream: error (" << getErrorString( result ) << ") unlocking input buffer!";
      errorText_ = errorStream_.str();
      goto unlock;
    }

    // If we start recording again, we must begin at beginning of buffer.
    handle->bufferPointer[1] = 0;
  }

 unlock:
  timeEndPeriod( 1 ); // revert to normal scheduler frequency on lesser windows.
  MUTEX_UNLOCK( &stream_.mutex );

  if ( FAILED( result ) ) error( RTAUDIO_SYSTEM_ERROR );
  return RTAUDIO_NO_ERROR;
}

RtAudioErrorType RtApiDs :: abortStream()
{
  if ( stream_.state != STREAM_RUNNING ) {
    if ( stream_.state == STREAM_STOPPED )
      errorText_ = "RtApiDs::abortStream(): the stream is already stopped!";
    else if ( stream_.state == STREAM_STOPPING || stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiDs::abortStream(): the stream is stopping or closed!";
    return error( RTAUDIO_WARNING );
  }

  DsHandle *handle = (DsHandle *) stream_.apiHandle;
  handle->drainCounter = 2;

  return stopStream();
}

void RtApiDs :: callbackEvent()
{
  if ( stream_.state == STREAM_STOPPED || stream_.state == STREAM_STOPPING ) {
    Sleep( 50 ); // sleep 50 milliseconds
    return;
  }

  if ( stream_.state == STREAM_CLOSED ) {
    errorText_ = "RtApiDs::callbackEvent(): the stream is closed ... this shouldn't happen!";
    error( RTAUDIO_WARNING );
    return;
  }

  CallbackInfo *info = (CallbackInfo *) &stream_.callbackInfo;
  DsHandle *handle = (DsHandle *) stream_.apiHandle;

  // Check if we were draining the stream and signal is finished.
  if ( handle->drainCounter > stream_.nBuffers + 2 ) {

    stream_.state = STREAM_STOPPING;
    if ( handle->internalDrain == false )
      SetEvent( handle->condition );
    else
      stopStream();
    return;
  }

  // Invoke user callback to get fresh output data UNLESS we are
  // draining stream.
  if ( handle->drainCounter == 0 ) {
    RtAudioCallback callback = (RtAudioCallback) info->callback;
    double streamTime = getStreamTime();
    RtAudioStreamStatus status = 0;
    if ( stream_.mode != INPUT && handle->xrun[0] == true ) {
      status |= RTAUDIO_OUTPUT_UNDERFLOW;
      handle->xrun[0] = false;
    }
    if ( stream_.mode != OUTPUT && handle->xrun[1] == true ) {
      status |= RTAUDIO_INPUT_OVERFLOW;
      handle->xrun[1] = false;
    }
    int cbReturnValue = callback( stream_.userBuffer[0], stream_.userBuffer[1],
                                  stream_.bufferSize, streamTime, status, info->userData );
    if ( cbReturnValue == 2 ) {
      stream_.state = STREAM_STOPPING;
      handle->drainCounter = 2;
      abortStream();
      return;
    }
    else if ( cbReturnValue == 1 ) {
      handle->drainCounter = 1;
      handle->internalDrain = true;
    }
  }

  HRESULT result;
  DWORD currentWritePointer, safeWritePointer;
  DWORD currentReadPointer, safeReadPointer;
  UINT nextWritePointer;

  LPVOID buffer1 = NULL;
  LPVOID buffer2 = NULL;
  DWORD bufferSize1 = 0;
  DWORD bufferSize2 = 0;

  char *buffer;
  long bufferBytes;

  MUTEX_LOCK( &stream_.mutex );
  if ( stream_.state == STREAM_STOPPED ) {
    MUTEX_UNLOCK( &stream_.mutex );
    return;
  }

  if ( buffersRolling == false ) {
    if ( stream_.mode == DUPLEX ) {
      //assert( handle->dsBufferSize[0] == handle->dsBufferSize[1] );

      // It takes a while for the devices to get rolling. As a result,
      // there's no guarantee that the capture and write device pointers
      // will move in lockstep.  Wait here for both devices to start
      // rolling, and then set our buffer pointers accordingly.
      // e.g. Crystal Drivers: the capture buffer starts up 5700 to 9600
      // bytes later than the write buffer.

      // Stub: a serious risk of having a pre-emptive scheduling round
      // take place between the two GetCurrentPosition calls... but I'm
      // really not sure how to solve the problem.  Temporarily boost to
      // Realtime priority, maybe; but I'm not sure what priority the
      // DirectSound service threads run at. We *should* be roughly
      // within a ms or so of correct.

      LPDIRECTSOUNDBUFFER dsWriteBuffer = (LPDIRECTSOUNDBUFFER) handle->buffer[0];
      LPDIRECTSOUNDCAPTUREBUFFER dsCaptureBuffer = (LPDIRECTSOUNDCAPTUREBUFFER) handle->buffer[1];

      DWORD startSafeWritePointer, startSafeReadPointer;

      result = dsWriteBuffer->GetCurrentPosition( NULL, &startSafeWritePointer );
      if ( FAILED( result ) ) {
        errorStream_ << "RtApiDs::callbackEvent: error (" << getErrorString( result ) << ") getting current write position!";
        errorText_ = errorStream_.str();
        MUTEX_UNLOCK( &stream_.mutex );
        error( RTAUDIO_SYSTEM_ERROR );
        return;
      }
      result = dsCaptureBuffer->GetCurrentPosition( NULL, &startSafeReadPointer );
      if ( FAILED( result ) ) {
        errorStream_ << "RtApiDs::callbackEvent: error (" << getErrorString( result ) << ") getting current read position!";
        errorText_ = errorStream_.str();
        MUTEX_UNLOCK( &stream_.mutex );
        error( RTAUDIO_SYSTEM_ERROR );
        return;
      }
      while ( true ) {
        result = dsWriteBuffer->GetCurrentPosition( NULL, &safeWritePointer );
        if ( FAILED( result ) ) {
          errorStream_ << "RtApiDs::callbackEvent: error (" << getErrorString( result ) << ") getting current write position!";
          errorText_ = errorStream_.str();
          MUTEX_UNLOCK( &stream_.mutex );
          error( RTAUDIO_SYSTEM_ERROR );
          return;
        }
        result = dsCaptureBuffer->GetCurrentPosition( NULL, &safeReadPointer );
        if ( FAILED( result ) ) {
          errorStream_ << "RtApiDs::callbackEvent: error (" << getErrorString( result ) << ") getting current read position!";
          errorText_ = errorStream_.str();
          MUTEX_UNLOCK( &stream_.mutex );
          error( RTAUDIO_SYSTEM_ERROR );
          return;
        }
        if ( safeWritePointer != startSafeWritePointer && safeReadPointer != startSafeReadPointer ) break;
        Sleep( 1 );
      }

      //assert( handle->dsBufferSize[0] == handle->dsBufferSize[1] );

      handle->bufferPointer[0] = safeWritePointer + handle->dsPointerLeadTime[0];
      if ( handle->bufferPointer[0] >= handle->dsBufferSize[0] ) handle->bufferPointer[0] -= handle->dsBufferSize[0];
      handle->bufferPointer[1] = safeReadPointer;
    }
    else if ( stream_.mode == OUTPUT ) {

      // Set the proper nextWritePosition after initial startup.
      LPDIRECTSOUNDBUFFER dsWriteBuffer = (LPDIRECTSOUNDBUFFER) handle->buffer[0];
      result = dsWriteBuffer->GetCurrentPosition( &currentWritePointer, &safeWritePointer );
      if ( FAILED( result ) ) {
        errorStream_ << "RtApiDs::callbackEvent: error (" << getErrorString( result ) << ") getting current write position!";
        errorText_ = errorStream_.str();
        MUTEX_UNLOCK( &stream_.mutex );
        error( RTAUDIO_SYSTEM_ERROR );
        return;
      }
      handle->bufferPointer[0] = safeWritePointer + handle->dsPointerLeadTime[0];
      if ( handle->bufferPointer[0] >= handle->dsBufferSize[0] ) handle->bufferPointer[0] -= handle->dsBufferSize[0];
    }

    buffersRolling = true;
  }

  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {
    
    LPDIRECTSOUNDBUFFER dsBuffer = (LPDIRECTSOUNDBUFFER) handle->buffer[0];

    if ( handle->drainCounter > 1 ) { // write zeros to the output stream
      bufferBytes = stream_.bufferSize * stream_.nUserChannels[0];
      bufferBytes *= formatBytes( stream_.userFormat );
      memset( stream_.userBuffer[0], 0, bufferBytes );
    }

    // Setup parameters and do buffer conversion if necessary.
    if ( stream_.doConvertBuffer[0] ) {
      buffer = stream_.deviceBuffer;
      convertBuffer( buffer, stream_.userBuffer[0], stream_.convertInfo[0] );
      bufferBytes = stream_.bufferSize * stream_.nDeviceChannels[0];
      bufferBytes *= formatBytes( stream_.deviceFormat[0] );
    }
    else {
      buffer = stream_.userBuffer[0];
      bufferBytes = stream_.bufferSize * stream_.nUserChannels[0];
      bufferBytes *= formatBytes( stream_.userFormat );
    }

    // No byte swapping necessary in DirectSound implementation.

    // Ahhh ... windoze.  16-bit data is signed but 8-bit data is
    // unsigned.  So, we need to convert our signed 8-bit data here to
    // unsigned.
    if ( stream_.deviceFormat[0] == RTAUDIO_SINT8 )
      for ( int i=0; i<bufferBytes; i++ ) buffer[i] = (unsigned char) ( buffer[i] + 128 );

    DWORD dsBufferSize = handle->dsBufferSize[0];
    nextWritePointer = handle->bufferPointer[0];

    DWORD endWrite, leadPointer;
    while ( true ) {
      // Find out where the read and "safe write" pointers are.
      result = dsBuffer->GetCurrentPosition( &currentWritePointer, &safeWritePointer );
      if ( FAILED( result ) ) {
        errorStream_ << "RtApiDs::callbackEvent: error (" << getErrorString( result ) << ") getting current write position!";
        errorText_ = errorStream_.str();
        MUTEX_UNLOCK( &stream_.mutex );
        error( RTAUDIO_SYSTEM_ERROR );
        return;
      }

      // We will copy our output buffer into the region between
      // safeWritePointer and leadPointer.  If leadPointer is not
      // beyond the next endWrite position, wait until it is.
      leadPointer = safeWritePointer + handle->dsPointerLeadTime[0];
      //std::cout << "safeWritePointer = " << safeWritePointer << ", leadPointer = " << leadPointer << ", nextWritePointer = " << nextWritePointer << std::endl;
      if ( leadPointer > dsBufferSize ) leadPointer -= dsBufferSize;
      if ( leadPointer < nextWritePointer ) leadPointer += dsBufferSize; // unwrap offset
      endWrite = nextWritePointer + bufferBytes;

      // Check whether the entire write region is behind the play pointer.
      if ( leadPointer >= endWrite ) break;

      // If we are here, then we must wait until the leadPointer advances
      // beyond the end of our next write region. We use the
      // Sleep() function to suspend operation until that happens.
      double millis = ( endWrite - leadPointer ) * 1000.0;
      millis /= ( formatBytes( stream_.deviceFormat[0]) * stream_.nDeviceChannels[0] * stream_.sampleRate);
      if ( millis < 1.0 ) millis = 1.0;
      Sleep( (DWORD) millis );
    }

    if ( dsPointerBetween( nextWritePointer, safeWritePointer, currentWritePointer, dsBufferSize )
         || dsPointerBetween( endWrite, safeWritePointer, currentWritePointer, dsBufferSize ) ) { 
      // We've strayed into the forbidden zone ... resync the read pointer.
      handle->xrun[0] = true;
      nextWritePointer = safeWritePointer + handle->dsPointerLeadTime[0] - bufferBytes;
      if ( nextWritePointer >= dsBufferSize ) nextWritePointer -= dsBufferSize;
      handle->bufferPointer[0] = nextWritePointer;
      endWrite = nextWritePointer + bufferBytes;
    }

    // Lock free space in the buffer
    result = dsBuffer->Lock( nextWritePointer, bufferBytes, &buffer1,
                             &bufferSize1, &buffer2, &bufferSize2, 0 );
    if ( FAILED( result ) ) {
      errorStream_ << "RtApiDs::callbackEvent: error (" << getErrorString( result ) << ") locking buffer during playback!";
      errorText_ = errorStream_.str();
      MUTEX_UNLOCK( &stream_.mutex );
      error( RTAUDIO_SYSTEM_ERROR );
      return;
    }

    // Copy our buffer into the DS buffer
    CopyMemory( buffer1, buffer, bufferSize1 );
    if ( buffer2 != NULL ) CopyMemory( buffer2, buffer+bufferSize1, bufferSize2 );

    // Update our buffer offset and unlock sound buffer
    dsBuffer->Unlock( buffer1, bufferSize1, buffer2, bufferSize2 );
    if ( FAILED( result ) ) {
      errorStream_ << "RtApiDs::callbackEvent: error (" << getErrorString( result ) << ") unlocking buffer during playback!";
      errorText_ = errorStream_.str();
      MUTEX_UNLOCK( &stream_.mutex );
      error( RTAUDIO_SYSTEM_ERROR );
      return;
    }
    nextWritePointer = ( nextWritePointer + bufferSize1 + bufferSize2 ) % dsBufferSize;
    handle->bufferPointer[0] = nextWritePointer;
  }

  // Don't bother draining input
  if ( handle->drainCounter ) {
    handle->drainCounter++;
    goto unlock;
  }

  if ( stream_.mode == INPUT || stream_.mode == DUPLEX ) {

    // Setup parameters.
    if ( stream_.doConvertBuffer[1] ) {
      buffer = stream_.deviceBuffer;
      bufferBytes = stream_.bufferSize * stream_.nDeviceChannels[1];
      bufferBytes *= formatBytes( stream_.deviceFormat[1] );
    }
    else {
      buffer = stream_.userBuffer[1];
      bufferBytes = stream_.bufferSize * stream_.nUserChannels[1];
      bufferBytes *= formatBytes( stream_.userFormat );
    }

    LPDIRECTSOUNDCAPTUREBUFFER dsBuffer = (LPDIRECTSOUNDCAPTUREBUFFER) handle->buffer[1];
    long nextReadPointer = handle->bufferPointer[1];
    DWORD dsBufferSize = handle->dsBufferSize[1];

    // Find out where the write and "safe read" pointers are.
    result = dsBuffer->GetCurrentPosition( &currentReadPointer, &safeReadPointer );
    if ( FAILED( result ) ) {
      errorStream_ << "RtApiDs::callbackEvent: error (" << getErrorString( result ) << ") getting current read position!";
      errorText_ = errorStream_.str();
      MUTEX_UNLOCK( &stream_.mutex );
      error( RTAUDIO_SYSTEM_ERROR );
      return;
    }

    if ( safeReadPointer < (DWORD)nextReadPointer ) safeReadPointer += dsBufferSize; // unwrap offset
    DWORD endRead = nextReadPointer + bufferBytes;

    // Handling depends on whether we are INPUT or DUPLEX. 
    // If we're in INPUT mode then waiting is a good thing. If we're in DUPLEX mode,
    // then a wait here will drag the write pointers into the forbidden zone.
    // 
    // In DUPLEX mode, rather than wait, we will back off the read pointer until 
    // it's in a safe position. This causes dropouts, but it seems to be the only 
    // practical way to sync up the read and write pointers reliably, given the 
    // the very complex relationship between phase and increment of the read and write 
    // pointers.
    //
    // In order to minimize audible dropouts in DUPLEX mode, we will
    // provide a pre-roll period of 0.5 seconds in which we return
    // zeros from the read buffer while the pointers sync up.

    if ( stream_.mode == DUPLEX ) {
      if ( safeReadPointer < endRead ) {
        if ( duplexPrerollBytes <= 0 ) {
          // Pre-roll time over. Be more aggressive.
          int adjustment = endRead-safeReadPointer;

          handle->xrun[1] = true;
          // Two cases:
          //   - large adjustments: we've probably run out of CPU cycles, so just resync exactly,
          //     and perform fine adjustments later.
          //   - small adjustments: back off by twice as much.
          if ( adjustment >= 2*bufferBytes )
            nextReadPointer = safeReadPointer-2*bufferBytes;
          else
            nextReadPointer = safeReadPointer-bufferBytes-adjustment;

          if ( nextReadPointer < 0 ) nextReadPointer += dsBufferSize;

        }
        else {
          // In pre=roll time. Just do it.
          nextReadPointer = safeReadPointer - bufferBytes;
          while ( nextReadPointer < 0 ) nextReadPointer += dsBufferSize;
        }
        endRead = nextReadPointer + bufferBytes;
      }
    }
    else { // mode == INPUT
      while ( safeReadPointer < endRead && stream_.callbackInfo.isRunning ) {
        // See comments for playback.
        double millis = (endRead - safeReadPointer) * 1000.0;
        millis /= ( formatBytes(stream_.deviceFormat[1]) * stream_.nDeviceChannels[1] * stream_.sampleRate);
        if ( millis < 1.0 ) millis = 1.0;
        Sleep( (DWORD) millis );

        // Wake up and find out where we are now.
        result = dsBuffer->GetCurrentPosition( &currentReadPointer, &safeReadPointer );
        if ( FAILED( result ) ) {
          errorStream_ << "RtApiDs::callbackEvent: error (" << getErrorString( result ) << ") getting current read position!";
          errorText_ = errorStream_.str();
          MUTEX_UNLOCK( &stream_.mutex );
          error( RTAUDIO_SYSTEM_ERROR );
          return;
        }
      
        if ( safeReadPointer < (DWORD)nextReadPointer ) safeReadPointer += dsBufferSize; // unwrap offset
      }
    }

    // Lock free space in the buffer
    result = dsBuffer->Lock( nextReadPointer, bufferBytes, &buffer1,
                             &bufferSize1, &buffer2, &bufferSize2, 0 );
    if ( FAILED( result ) ) {
      errorStream_ << "RtApiDs::callbackEvent: error (" << getErrorString( result ) << ") locking capture buffer!";
      errorText_ = errorStream_.str();
      MUTEX_UNLOCK( &stream_.mutex );
      error( RTAUDIO_SYSTEM_ERROR );
      return;
    }

    if ( duplexPrerollBytes <= 0 ) {
      // Copy our buffer into the DS buffer
      CopyMemory( buffer, buffer1, bufferSize1 );
      if ( buffer2 != NULL ) CopyMemory( buffer+bufferSize1, buffer2, bufferSize2 );
    }
    else {
      memset( buffer, 0, bufferSize1 );
      if ( buffer2 != NULL ) memset( buffer + bufferSize1, 0, bufferSize2 );
      duplexPrerollBytes -= bufferSize1 + bufferSize2;
    }

    // Update our buffer offset and unlock sound buffer
    nextReadPointer = ( nextReadPointer + bufferSize1 + bufferSize2 ) % dsBufferSize;
    dsBuffer->Unlock( buffer1, bufferSize1, buffer2, bufferSize2 );
    if ( FAILED( result ) ) {
      errorStream_ << "RtApiDs::callbackEvent: error (" << getErrorString( result ) << ") unlocking capture buffer!";
      errorText_ = errorStream_.str();
      MUTEX_UNLOCK( &stream_.mutex );
      error( RTAUDIO_SYSTEM_ERROR );
      return;
    }
    handle->bufferPointer[1] = nextReadPointer;

    // No byte swapping necessary in DirectSound implementation.

    // If necessary, convert 8-bit data from unsigned to signed.
    if ( stream_.deviceFormat[1] == RTAUDIO_SINT8 )
      for ( int j=0; j<bufferBytes; j++ ) buffer[j] = (signed char) ( buffer[j] - 128 );

    // Do buffer conversion if necessary.
    if ( stream_.doConvertBuffer[1] )
      convertBuffer( stream_.userBuffer[1], stream_.deviceBuffer, stream_.convertInfo[1] );
  }

 unlock:
  MUTEX_UNLOCK( &stream_.mutex );
  RtApi::tickStreamTime();
}

// Definitions for utility functions and callbacks
// specific to the DirectSound implementation.

static unsigned __stdcall callbackHandler( void *ptr )
{
  CallbackInfo *info = (CallbackInfo *) ptr;
  RtApiDs *object = (RtApiDs *) info->object;
  bool* isRunning = &info->isRunning;

  while ( *isRunning == true ) {
    object->callbackEvent();
  }

  _endthreadex( 0 );
  return 0;
}

static BOOL CALLBACK deviceQueryCallback( LPGUID lpguid,
                                          LPCTSTR description,
                                          LPCTSTR lpctstr,
                                          LPVOID lpContext )
{
  struct DsProbeData& probeInfo = *(struct DsProbeData*) lpContext;
  std::vector<struct DsDevice>& dsDevices = *probeInfo.dsDevices;

  HRESULT hr;
  bool validDevice = false;
  if ( probeInfo.isInput == true ) {
    DSCCAPS caps;
    LPDIRECTSOUNDCAPTURE object;

    hr = DirectSoundCaptureCreate(  lpguid, &object,   NULL );
    if ( hr != DS_OK ) return TRUE;

    caps.dwSize = sizeof(caps);
    hr = object->GetCaps( &caps );
    if ( hr == DS_OK ) {
      if ( caps.dwChannels > 0 && caps.dwFormats > 0 )
        validDevice = true;
    }
    object->Release();
  }
  else {
    DSCAPS caps;
    LPDIRECTSOUND object;
    hr = DirectSoundCreate(  lpguid, &object,   NULL );
    if ( hr != DS_OK ) return TRUE;

    caps.dwSize = sizeof(caps);
    hr = object->GetCaps( &caps );
    if ( hr == DS_OK ) {
      if ( caps.dwFlags & DSCAPS_PRIMARYMONO || caps.dwFlags & DSCAPS_PRIMARYSTEREO )
        validDevice = true;
    }
    object->Release();
  }

  if ( validDevice ) {
    // If good device, then save its name and guid.
    DsDevice device;
    device.name = convertCharPointerToStdString( description );
    device.epID = convertCharPointerToStdString(lpctstr);
    device.id = lpguid;
    device.isInput = probeInfo.isInput;
    dsDevices.push_back( device );
  }

  return TRUE;
}

static const char* getErrorString( int code )
{
  switch ( code ) {

  case DSERR_ALLOCATED:
    return "Already allocated";

  case DSERR_CONTROLUNAVAIL:
    return "Control unavailable";

  case DSERR_INVALIDPARAM:
    return "Invalid parameter";

  case DSERR_INVALIDCALL:
    return "Invalid call";

  case DSERR_GENERIC:
    return "Generic error";

  case DSERR_PRIOLEVELNEEDED:
    return "Priority level needed";

  case DSERR_OUTOFMEMORY:
    return "Out of memory";

  case DSERR_BADFORMAT:
    return "The sample rate or the channel format is not supported";

  case DSERR_UNSUPPORTED:
    return "Not supported";

  case DSERR_NODRIVER:
    return "No driver";

  case DSERR_ALREADYINITIALIZED:
    return "Already initialized";

  case DSERR_NOAGGREGATION:
    return "No aggregation";

  case DSERR_BUFFERLOST:
    return "Buffer lost";

  case DSERR_OTHERAPPHASPRIO:
    return "Another application already has priority";

  case DSERR_UNINITIALIZED:
    return "Uninitialized";

  default:
    return "DirectSound unknown error";
  }
}
//******************** End of __WINDOWS_DS__ *********************//
#endif


#if defined(__LINUX_ALSA__)

#include <alsa/asoundlib.h>
#include <unistd.h>

  // A structure to hold various information related to the ALSA API
  // implementation.
struct AlsaHandle {
  snd_pcm_t *handles[2];
  bool synchronized;
  bool xrun[2];
  pthread_cond_t runnable_cv;
  bool runnable;

  AlsaHandle()
#if _cplusplus >= 201103L
    :handles{nullptr, nullptr}, synchronized(false), runnable(false) { xrun[0] = false; xrun[1] = false; }
#else 
    : synchronized(false), runnable(false) { handles[0] = NULL; handles[1] = NULL; xrun[0] = false; xrun[1] = false; }
#endif
};

static void *alsaCallbackHandler( void * ptr );

RtApiAlsa :: RtApiAlsa()
{
  // Nothing to do here.
}

RtApiAlsa :: ~RtApiAlsa()
{
  if ( stream_.state != STREAM_CLOSED ) closeStream();
}

void RtApiAlsa :: probeDevices( void )
{
  // See list of required functionality in RtApi::probeDevices().
  
  int result, device, card;
  char name[128];
  snd_ctl_t *handle = 0;
  snd_ctl_card_info_t *ctlinfo;
  snd_pcm_info_t *pcminfo;
  snd_ctl_card_info_alloca(&ctlinfo);
  snd_pcm_info_alloca(&pcminfo);
  // First element isthe device hw ID, second is the device "pretty name"
  std::vector<std::pair<std::string, std::string>> deviceID_prettyName;
  snd_pcm_stream_t stream;
  std::string defaultDeviceName;

  // Add the default interface if available.
  result = snd_ctl_open( &handle, "default", 0 );
  if (result == 0) {
    deviceID_prettyName.push_back({"default", "Default ALSA Device"});
    defaultDeviceName = deviceID_prettyName[0].second;
    snd_ctl_close( handle );
  }

  // Add the Pulse interface if available.
  result = snd_ctl_open( &handle, "pulse", 0 );
  if (result == 0) {
    deviceID_prettyName.push_back({"pulse",  "PulseAudio Sound Server"});
    snd_ctl_close( handle );
  }
  
  // Count cards and devices and get ascii identifiers.
  card = -1;
  snd_card_next( &card );
  while ( card >= 0 ) {
    sprintf( name, "hw:%d", card );
    result = snd_ctl_open( &handle, name, 0 );
    if ( result < 0 ) {
      handle = 0;
      errorStream_ << "RtApiAlsa::probeDevices: control open, card = " << card << ", " << snd_strerror( result ) << ".";
      errorText_ = errorStream_.str();
      error( RTAUDIO_WARNING );
      goto nextcard;
    }
    result = snd_ctl_card_info( handle, ctlinfo );
    if ( result < 0 ) {
      errorStream_ << "RtApiAlsa::probeDevices: control info, card = " << card << ", " << snd_strerror( result ) << ".";
      errorText_ = errorStream_.str();
      error( RTAUDIO_WARNING );
      goto nextcard;
    }
    device = -1;
    while( 1 ) {
      result = snd_ctl_pcm_next_device( handle, &device );
      if ( result < 0 ) {
        errorStream_ << "RtApiAlsa::probeDevices: control next device, card = " << card << ", " << snd_strerror( result ) << ".";
        errorText_ = errorStream_.str();
        error( RTAUDIO_WARNING );
        break;
      }
      if ( device < 0 )
        break;

      snd_pcm_info_set_device( pcminfo, device );
      snd_pcm_info_set_subdevice( pcminfo, 0 );
      stream = SND_PCM_STREAM_PLAYBACK;
      snd_pcm_info_set_stream( pcminfo, stream );
      result = snd_ctl_pcm_info( handle, pcminfo );
      if ( result < 0 ) {
        if ( result == -ENOENT ) { // try as input stream
          stream = SND_PCM_STREAM_CAPTURE;
          snd_pcm_info_set_stream( pcminfo, stream );
          result = snd_ctl_pcm_info( handle, pcminfo );
          if ( result < 0 ) {
            errorStream_ << "RtApiAlsa::probeDevices: control pcm info, card = " << card << ", device = " << device << ", " << snd_strerror( result ) << ".";
            errorText_ = errorStream_.str();
            error( RTAUDIO_WARNING );
            continue;
          }
        }
        else continue;
      }
      sprintf( name, "hw:%s,%d", snd_ctl_card_info_get_id(ctlinfo), device );
      std::string id(name);
      sprintf( name, "%s (%s)", snd_ctl_card_info_get_name(ctlinfo), snd_pcm_info_get_id(pcminfo) );
      std::string prettyName(name);
      deviceID_prettyName.push_back( {id, prettyName} );
      if ( card == 0 && device == 0 && defaultDeviceName.empty() )
        defaultDeviceName = name;
    }
  nextcard:
    if ( handle )
      snd_ctl_close( handle );
    snd_card_next( &card );
  }

  if ( deviceID_prettyName.size() == 0 ) {
    deviceList_.clear();
    deviceIdPairs_.clear();
    return;
  }

  // Clean removed devices
  for ( auto it = deviceIdPairs_.begin(); it != deviceIdPairs_.end(); ) {
    bool found = false;
    for ( auto& d: deviceID_prettyName ) {
      if ( d.first == (*it).first ) {
        found = true;
        break;
      }
    }

    if ( found )
      ++it;
    else
      it = deviceIdPairs_.erase(it);
  }

  // Fill or update the deviceList_ and also save a corresponding list of Ids.
  for ( auto& d : deviceID_prettyName ) {
    bool found = false;
    for ( auto& dID : deviceIdPairs_ ) {
      if ( d.first == dID.first ) {
        found = true;
        break; // We already have this device.
      }
    }

    if ( found )
      continue;

    // new device
    RtAudio::DeviceInfo info;
    info.name = d.second;
    if ( probeDeviceInfo( info, d.first ) == false ) continue; // ignore if probe fails
    info.ID = currentDeviceId_++;  // arbitrary internal device ID
    if ( info.name == defaultDeviceName ) {
      if ( info.outputChannels > 0 ) info.isDefaultOutput = true;
      if ( info.inputChannels > 0 ) info.isDefaultInput = true;
    }
    deviceList_.push_back( info );
    deviceIdPairs_.push_back({d.first, info.ID});
    // I don't see that ALSA provides property listeners to know if
    // devices are removed or added.
  }

  // Remove any devices left in the list that are no longer available.
  for ( std::vector<RtAudio::DeviceInfo>::iterator it=deviceList_.begin(); it!=deviceList_.end(); )
  {
    auto itID = deviceIdPairs_.begin();
    while ( itID != deviceIdPairs_.end() ) {
      if ( (*it).ID == (*itID).second ) {
        break;
      }
      ++itID;
    }

    if ( itID == deviceIdPairs_.end() ) {
      // not found so remove it from our list
      it = deviceList_.erase( it );
    }
    else
      ++it;
  }
}

bool RtApiAlsa :: probeDeviceInfo( RtAudio::DeviceInfo& info, std::string name )
{
  int result, openMode = SND_PCM_ASYNC;
  snd_pcm_stream_t stream;
  snd_pcm_t *phandle;
  snd_pcm_hw_params_t *params;
  snd_pcm_hw_params_alloca( &params );

  // First try for playback
  stream = SND_PCM_STREAM_PLAYBACK;
  result = snd_pcm_open( &phandle, name.c_str(), stream, openMode | SND_PCM_NONBLOCK );
  if ( result < 0 ) {
    if ( result == -16 ) return false; // device busy ... can't probe or use
    if ( result != -2 ) { // device doesn't support playback
      errorStream_ << "RtApiAlsa::probeDeviceInfo: snd_pcm_open (playback) error for device (" << name << "), " << snd_strerror( result ) << ".";
      errorText_ = errorStream_.str();
      error( RTAUDIO_WARNING );
    }
    goto captureProbe;
  }

  // The device is open ... fill the parameter structure.
  result = snd_pcm_hw_params_any( phandle, params );
  if ( result < 0 ) {
    snd_pcm_close( phandle );
    errorStream_ << "RtApiAlsa::probeDeviceInfo: snd_pcm_hw_params error for device (" << name << "), " << snd_strerror( result ) << ".";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    goto captureProbe;
  }

  // Get output channel information.
  unsigned int value;
  result = snd_pcm_hw_params_get_channels_max( params, &value );
  if ( result < 0 ) {
    snd_pcm_close( phandle );
    errorStream_ << "RtApiAlsa::probeDeviceInfo: error getting device (" << name << ") output channels, " << snd_strerror( result ) << ".";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    goto captureProbe;
  }
  info.outputChannels = value;
  snd_pcm_close( phandle );

 captureProbe:
  stream = SND_PCM_STREAM_CAPTURE;
  result = snd_pcm_open( &phandle, name.c_str(), stream, openMode | SND_PCM_NONBLOCK);
  if ( result < 0 && result ) {
    if ( result != -2 && result != -16 ) { // device busy or doesn't support capture
      errorStream_ << "RtApiAlsa::probeDeviceInfo: snd_pcm_open (capture) error for device (" << name << "), " << snd_strerror( result ) << ".";
      errorText_ = errorStream_.str();
      error( RTAUDIO_WARNING );
    }
    if ( info.outputChannels == 0 ) return false;
    goto probeParameters;
  }

  // The device is open ... fill the parameter structure.
  result = snd_pcm_hw_params_any( phandle, params );
  if ( result < 0 ) {
    snd_pcm_close( phandle );
    errorStream_ << "RtApiAlsa::probeDeviceInfo: snd_pcm_hw_params error for device (" << name << "), " << snd_strerror( result ) << ".";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    if ( info.outputChannels == 0 ) return false;
    goto probeParameters;
  }

  result = snd_pcm_hw_params_get_channels_max( params, &value );
  if ( result < 0 ) {
    snd_pcm_close( phandle );
    errorStream_ << "RtApiAlsa::probeDeviceInfo: error getting device (" << name << ") input channels, " << snd_strerror( result ) << ".";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    if ( info.outputChannels == 0 ) return false;
    goto probeParameters;
  }
  info.inputChannels = value;
  snd_pcm_close( phandle );

  // If device opens for both playback and capture, we determine the channels.
  if ( info.outputChannels > 0 && info.inputChannels > 0 )
    info.duplexChannels = (info.outputChannels > info.inputChannels) ? info.inputChannels : info.outputChannels;

 probeParameters:
  // At this point, we just need to figure out the supported data
  // formats and sample rates.  We'll proceed by opening the device in
  // the direction with the maximum number of channels, or playback if
  // they are equal.  This might limit our sample rate options, but so
  // be it.

  if ( info.outputChannels >= info.inputChannels )
    stream = SND_PCM_STREAM_PLAYBACK;
  else
    stream = SND_PCM_STREAM_CAPTURE;

  result = snd_pcm_open( &phandle, name.c_str(), stream, openMode | SND_PCM_NONBLOCK);
  if ( result < 0 ) {
    errorStream_ << "RtApiAlsa::probeDeviceInfo: snd_pcm_open error for device (" << name << "), " << snd_strerror( result ) << ".";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  // The device is open ... fill the parameter structure.
  result = snd_pcm_hw_params_any( phandle, params );
  if ( result < 0 ) {
    snd_pcm_close( phandle );
    errorStream_ << "RtApiAlsa::probeDeviceInfo: snd_pcm_hw_params error for device (" << name << "), " << snd_strerror( result ) << ".";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  // Test our discrete set of sample rate values.
  info.sampleRates.clear();
  for ( unsigned int i=0; i<MAX_SAMPLE_RATES; i++ ) {
    if ( snd_pcm_hw_params_test_rate( phandle, params, SAMPLE_RATES[i], 0 ) == 0 ) {
      info.sampleRates.push_back( SAMPLE_RATES[i] );

      if ( !info.preferredSampleRate || ( SAMPLE_RATES[i] <= 48000 && SAMPLE_RATES[i] > info.preferredSampleRate ) )
        info.preferredSampleRate = SAMPLE_RATES[i];
    }
  }
  if ( info.sampleRates.size() == 0 ) {
    snd_pcm_close( phandle );
    errorStream_ << "RtApiAlsa::probeDeviceInfo: no supported sample rates found for device (" << name << ").";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  // Probe the supported data formats ... we don't care about endian-ness just yet
  snd_pcm_format_t format;
  info.nativeFormats = 0;
  format = SND_PCM_FORMAT_S8;
  if ( snd_pcm_hw_params_test_format( phandle, params, format ) == 0 )
    info.nativeFormats |= RTAUDIO_SINT8;
  format = SND_PCM_FORMAT_S16;
  if ( snd_pcm_hw_params_test_format( phandle, params, format ) == 0 )
    info.nativeFormats |= RTAUDIO_SINT16;
  format = SND_PCM_FORMAT_S24;
  if ( snd_pcm_hw_params_test_format( phandle, params, format ) == 0 )
    info.nativeFormats |= RTAUDIO_SINT24;
  format = SND_PCM_FORMAT_S32;
  if ( snd_pcm_hw_params_test_format( phandle, params, format ) == 0 )
    info.nativeFormats |= RTAUDIO_SINT32;
  format = SND_PCM_FORMAT_FLOAT;
  if ( snd_pcm_hw_params_test_format( phandle, params, format ) == 0 )
    info.nativeFormats |= RTAUDIO_FLOAT32;
  format = SND_PCM_FORMAT_FLOAT64;
  if ( snd_pcm_hw_params_test_format( phandle, params, format ) == 0 )
    info.nativeFormats |= RTAUDIO_FLOAT64;

  // Check that we have at least one supported format
  if ( info.nativeFormats == 0 ) {
    snd_pcm_close( phandle );
    errorStream_ << "RtApiAlsa::probeDeviceInfo: pcm device (" << name << ") data format not supported by RtAudio.";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  // Close the device and return
  snd_pcm_close( phandle );
  return true;
}

bool RtApiAlsa :: probeDeviceOpen( unsigned int deviceId, StreamMode mode, unsigned int channels,
                                   unsigned int firstChannel, unsigned int sampleRate,
                                   RtAudioFormat format, unsigned int *bufferSize,
                                   RtAudio::StreamOptions *options )

{
#if defined(__RTAUDIO_DEBUG__)
  struct SndOutputTdealloc {
    SndOutputTdealloc() : _out(NULL) { snd_output_stdio_attach(&_out, stderr, 0); }
    ~SndOutputTdealloc() { snd_output_close(_out); }
    operator snd_output_t*() { return _out; }
    snd_output_t *_out;
  } out;
#endif

  std::string name;
  for ( auto& id : deviceIdPairs_) {
    if ( id.second == deviceId ) {
      name = id.first;
      break;
    }
  }

  snd_pcm_stream_t stream;
  if ( mode == OUTPUT )
    stream = SND_PCM_STREAM_PLAYBACK;
  else
    stream = SND_PCM_STREAM_CAPTURE;

  snd_pcm_t *phandle;
  int openMode = SND_PCM_ASYNC;
  int result = snd_pcm_open( &phandle, name.c_str(), stream, openMode );
  if ( result < 0 ) {
    if ( mode == OUTPUT )
      errorStream_ << "RtApiAlsa::probeDeviceOpen: pcm device (" << name << ") won't open for output.";
    else
      errorStream_ << "RtApiAlsa::probeDeviceOpen: pcm device (" << name << ") won't open for input.";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Fill the parameter structure.
  snd_pcm_hw_params_t *hw_params;
  snd_pcm_hw_params_alloca( &hw_params );
  result = snd_pcm_hw_params_any( phandle, hw_params );
  if ( result < 0 ) {
    snd_pcm_close( phandle );
    errorStream_ << "RtApiAlsa::probeDeviceOpen: error getting pcm device (" << name << ") parameters, " << snd_strerror( result ) << ".";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

#if defined(__RTAUDIO_DEBUG__)
  fprintf( stderr, "\nRtApiAlsa: dump hardware params just after device open:\n\n" );
  snd_pcm_hw_params_dump( hw_params, out );
#endif

  // Set access ... check user preference.
  if ( options && options->flags & RTAUDIO_NONINTERLEAVED ) {
    stream_.userInterleaved = false;
    result = snd_pcm_hw_params_set_access( phandle, hw_params, SND_PCM_ACCESS_RW_NONINTERLEAVED );
    if ( result < 0 ) {
      result = snd_pcm_hw_params_set_access( phandle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED );
      stream_.deviceInterleaved[mode] =  true;
    }
    else
      stream_.deviceInterleaved[mode] = false;
  }
  else {
    stream_.userInterleaved = true;
    result = snd_pcm_hw_params_set_access( phandle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED );
    if ( result < 0 ) {
      result = snd_pcm_hw_params_set_access( phandle, hw_params, SND_PCM_ACCESS_RW_NONINTERLEAVED );
      stream_.deviceInterleaved[mode] =  false;
    }
    else
      stream_.deviceInterleaved[mode] =  true;
  }

  if ( result < 0 ) {
    snd_pcm_close( phandle );
    errorStream_ << "RtApiAlsa::probeDeviceOpen: error setting pcm device (" << name << ") access, " << snd_strerror( result ) << ".";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Determine how to set the device format.
  stream_.userFormat = format;
  snd_pcm_format_t deviceFormat = SND_PCM_FORMAT_UNKNOWN;

  if ( format == RTAUDIO_SINT8 )
    deviceFormat = SND_PCM_FORMAT_S8;
  else if ( format == RTAUDIO_SINT16 )
    deviceFormat = SND_PCM_FORMAT_S16;
  else if ( format == RTAUDIO_SINT24 )
    deviceFormat = SND_PCM_FORMAT_S24;
  else if ( format == RTAUDIO_SINT32 )
    deviceFormat = SND_PCM_FORMAT_S32;
  else if ( format == RTAUDIO_FLOAT32 )
    deviceFormat = SND_PCM_FORMAT_FLOAT;
  else if ( format == RTAUDIO_FLOAT64 )
    deviceFormat = SND_PCM_FORMAT_FLOAT64;

  if ( snd_pcm_hw_params_test_format(phandle, hw_params, deviceFormat) == 0) {
    stream_.deviceFormat[mode] = format;
    goto setFormat;
  }

  // The user requested format is not natively supported by the device.
  deviceFormat = SND_PCM_FORMAT_FLOAT64;
  if ( snd_pcm_hw_params_test_format( phandle, hw_params, deviceFormat ) == 0 ) {
    stream_.deviceFormat[mode] = RTAUDIO_FLOAT64;
    goto setFormat;
  }

  deviceFormat = SND_PCM_FORMAT_FLOAT;
  if ( snd_pcm_hw_params_test_format(phandle, hw_params, deviceFormat ) == 0 ) {
    stream_.deviceFormat[mode] = RTAUDIO_FLOAT32;
    goto setFormat;
  }

  deviceFormat = SND_PCM_FORMAT_S32;
  if ( snd_pcm_hw_params_test_format(phandle, hw_params, deviceFormat ) == 0 ) {
    stream_.deviceFormat[mode] = RTAUDIO_SINT32;
    goto setFormat;
  }

  deviceFormat = SND_PCM_FORMAT_S24;
  if ( snd_pcm_hw_params_test_format(phandle, hw_params, deviceFormat ) == 0 ) {
    stream_.deviceFormat[mode] = RTAUDIO_SINT24;
    goto setFormat;
  }

  deviceFormat = SND_PCM_FORMAT_S16;
  if ( snd_pcm_hw_params_test_format(phandle, hw_params, deviceFormat ) == 0 ) {
    stream_.deviceFormat[mode] = RTAUDIO_SINT16;
    goto setFormat;
  }

  deviceFormat = SND_PCM_FORMAT_S8;
  if ( snd_pcm_hw_params_test_format(phandle, hw_params, deviceFormat ) == 0 ) {
    stream_.deviceFormat[mode] = RTAUDIO_SINT8;
    goto setFormat;
  }

  // If we get here, no supported format was found.
  snd_pcm_close( phandle );
  errorStream_ << "RtApiAlsa::probeDeviceOpen: pcm device (" << name << ") data format not supported by RtAudio.";
  errorText_ = errorStream_.str();
  return FAILURE;

 setFormat:
  result = snd_pcm_hw_params_set_format( phandle, hw_params, deviceFormat );
  if ( result < 0 ) {
    snd_pcm_close( phandle );
    errorStream_ << "RtApiAlsa::probeDeviceOpen: error setting pcm device (" << name << ") data format, " << snd_strerror( result ) << ".";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Determine whether byte-swaping is necessary.
  stream_.doByteSwap[mode] = false;
  if ( deviceFormat != SND_PCM_FORMAT_S8 ) {
    result = snd_pcm_format_cpu_endian( deviceFormat );
    if ( result == 0 )
      stream_.doByteSwap[mode] = true;
    else if (result < 0) {
      snd_pcm_close( phandle );
      errorStream_ << "RtApiAlsa::probeDeviceOpen: error getting pcm device (" << name << ") endian-ness, " << snd_strerror( result ) << ".";
      errorText_ = errorStream_.str();
      return FAILURE;
    }
  }

  // Set the sample rate.
  result = snd_pcm_hw_params_set_rate_near( phandle, hw_params, (unsigned int*) &sampleRate, 0 );
  if ( result < 0 ) {
    snd_pcm_close( phandle );
    errorStream_ << "RtApiAlsa::probeDeviceOpen: error setting sample rate on device (" << name << "), " << snd_strerror( result ) << ".";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Determine the number of channels for this device.  We support a possible
  // minimum device channel number > than the value requested by the user.
  stream_.nUserChannels[mode] = channels;
  unsigned int value;
  result = snd_pcm_hw_params_get_channels_max( hw_params, &value );
  unsigned int deviceChannels = value;
  if ( result < 0 || deviceChannels < channels + firstChannel ) {
    snd_pcm_close( phandle );
    errorStream_ << "RtApiAlsa::probeDeviceOpen: requested channel parameters not supported by device (" << name << "), " << snd_strerror( result ) << ".";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  result = snd_pcm_hw_params_get_channels_min( hw_params, &value );
  if ( result < 0 ) {
    snd_pcm_close( phandle );
    errorStream_ << "RtApiAlsa::probeDeviceOpen: error getting minimum channels for device (" << name << "), " << snd_strerror( result ) << ".";
    errorText_ = errorStream_.str();
    return FAILURE;
  }
  deviceChannels = value;
  if ( deviceChannels < channels + firstChannel ) deviceChannels = channels + firstChannel;
  stream_.nDeviceChannels[mode] = deviceChannels;

  // Set the device channels.
  result = snd_pcm_hw_params_set_channels( phandle, hw_params, deviceChannels );
  if ( result < 0 ) {
    snd_pcm_close( phandle );
    errorStream_ << "RtApiAlsa::probeDeviceOpen: error setting channels for device (" << name << "), " << snd_strerror( result ) << ".";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Set the buffer (or period) size.
  int dir = 0;
  snd_pcm_uframes_t periodSize = *bufferSize;
  result = snd_pcm_hw_params_set_period_size_near( phandle, hw_params, &periodSize, &dir );
  if ( result < 0 ) {
    snd_pcm_close( phandle );
    errorStream_ << "RtApiAlsa::probeDeviceOpen: error setting period size for device (" << name << "), " << snd_strerror( result ) << ".";
    errorText_ = errorStream_.str();
    return FAILURE;
  }
  *bufferSize = periodSize;

  // Set the buffer number, which in ALSA is referred to as the "period".
  unsigned int periods = 0;
  if ( options && options->flags & RTAUDIO_MINIMIZE_LATENCY ) periods = 2;
  if ( options && options->numberOfBuffers > 0 ) periods = options->numberOfBuffers;
  if ( periods < 2 ) periods = 4; // a fairly safe default value
  result = snd_pcm_hw_params_set_periods_near( phandle, hw_params, &periods, &dir );
  if ( result < 0 ) {
    snd_pcm_close( phandle );
    errorStream_ << "RtApiAlsa::probeDeviceOpen: error setting periods for device (" << name << "), " << snd_strerror( result ) << ".";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // If attempting to setup a duplex stream, the bufferSize parameter
  // MUST be the same in both directions!
  if ( stream_.mode == OUTPUT && mode == INPUT && *bufferSize != stream_.bufferSize ) {
    snd_pcm_close( phandle );
    errorStream_ << "RtApiAlsa::probeDeviceOpen: system error setting buffer size for duplex stream on device (" << name << ").";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  stream_.bufferSize = *bufferSize;

  // Install the hardware configuration
  result = snd_pcm_hw_params( phandle, hw_params );
  if ( result < 0 ) {
    snd_pcm_close( phandle );
    errorStream_ << "RtApiAlsa::probeDeviceOpen: error installing hardware configuration on device (" << name << "), " << snd_strerror( result ) << ".";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

#if defined(__RTAUDIO_DEBUG__)
  fprintf(stderr, "\nRtApiAlsa: dump hardware params after installation:\n\n");
  snd_pcm_hw_params_dump( hw_params, out );
#endif

  // Set the software configuration to fill buffers with zeros and prevent device stopping on xruns.
  snd_pcm_sw_params_t *sw_params = NULL;
  snd_pcm_sw_params_alloca( &sw_params );
  snd_pcm_sw_params_current( phandle, sw_params );
  snd_pcm_sw_params_set_start_threshold( phandle, sw_params, *bufferSize );
  snd_pcm_sw_params_set_stop_threshold( phandle, sw_params, ULONG_MAX );
  snd_pcm_sw_params_set_silence_threshold( phandle, sw_params, 0 );

  // The following two settings were suggested by Theo Veenker
  //snd_pcm_sw_params_set_avail_min( phandle, sw_params, *bufferSize );
  //snd_pcm_sw_params_set_xfer_align( phandle, sw_params, 1 );

  // here are two options for a fix
  //snd_pcm_sw_params_set_silence_size( phandle, sw_params, ULONG_MAX );
  snd_pcm_uframes_t val;
  snd_pcm_sw_params_get_boundary( sw_params, &val );
  snd_pcm_sw_params_set_silence_size( phandle, sw_params, val );

  result = snd_pcm_sw_params( phandle, sw_params );
  if ( result < 0 ) {
    snd_pcm_close( phandle );
    errorStream_ << "RtApiAlsa::probeDeviceOpen: error installing software configuration on device (" << name << "), " << snd_strerror( result ) << ".";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

#if defined(__RTAUDIO_DEBUG__)
  fprintf(stderr, "\nRtApiAlsa: dump software params after installation:\n\n");
  snd_pcm_sw_params_dump( sw_params, out );
#endif

  // Set flags for buffer conversion
  stream_.doConvertBuffer[mode] = false;
  if ( stream_.userFormat != stream_.deviceFormat[mode] )
    stream_.doConvertBuffer[mode] = true;
  if ( stream_.nUserChannels[mode] < stream_.nDeviceChannels[mode] )
    stream_.doConvertBuffer[mode] = true;
  if ( stream_.userInterleaved != stream_.deviceInterleaved[mode] &&
       stream_.nUserChannels[mode] > 1 )
    stream_.doConvertBuffer[mode] = true;

  // Allocate the ApiHandle if necessary and then save.
  AlsaHandle *apiInfo = 0;
  if ( stream_.apiHandle == 0 ) {
    try {
      apiInfo = (AlsaHandle *) new AlsaHandle;
    }
    catch ( std::bad_alloc& ) {
      errorText_ = "RtApiAlsa::probeDeviceOpen: error allocating AlsaHandle memory.";
      goto error;
    }

    if ( pthread_cond_init( &apiInfo->runnable_cv, NULL ) ) {
      errorText_ = "RtApiAlsa::probeDeviceOpen: error initializing pthread condition variable.";
      goto error;
    }

    stream_.apiHandle = (void *) apiInfo;
    apiInfo->handles[0] = 0;
    apiInfo->handles[1] = 0;
  }
  else {
    apiInfo = (AlsaHandle *) stream_.apiHandle;
  }
  apiInfo->handles[mode] = phandle;
  phandle = 0;

  // Allocate necessary internal buffers.
  unsigned long bufferBytes;
  bufferBytes = stream_.nUserChannels[mode] * *bufferSize * formatBytes( stream_.userFormat );
  stream_.userBuffer[mode] = (char *) calloc( bufferBytes, 1 );
  if ( stream_.userBuffer[mode] == NULL ) {
    errorText_ = "RtApiAlsa::probeDeviceOpen: error allocating user buffer memory.";
    goto error;
  }

  if ( stream_.doConvertBuffer[mode] ) {

    bool makeBuffer = true;
    bufferBytes = stream_.nDeviceChannels[mode] * formatBytes( stream_.deviceFormat[mode] );
    if ( mode == INPUT ) {
      if ( stream_.mode == OUTPUT && stream_.deviceBuffer ) {
        unsigned long bytesOut = stream_.nDeviceChannels[0] * formatBytes( stream_.deviceFormat[0] );
        if ( bufferBytes <= bytesOut ) makeBuffer = false;
      }
    }

    if ( makeBuffer ) {
      bufferBytes *= *bufferSize;
      if ( stream_.deviceBuffer ) free( stream_.deviceBuffer );
      stream_.deviceBuffer = (char *) calloc( bufferBytes, 1 );
      if ( stream_.deviceBuffer == NULL ) {
        errorText_ = "RtApiAlsa::probeDeviceOpen: error allocating device buffer memory.";
        goto error;
      }
    }
  }

  stream_.sampleRate = sampleRate;
  stream_.nBuffers = periods;
  stream_.deviceId[mode] = deviceId;
  stream_.state = STREAM_STOPPED;

  // Setup the buffer conversion information structure.
  if ( stream_.doConvertBuffer[mode] ) setConvertInfo( mode, firstChannel );

  // Setup thread if necessary.
  if ( stream_.mode == OUTPUT && mode == INPUT ) {
    // We had already set up an output stream.
    stream_.mode = DUPLEX;
    // Link the streams if possible.
    apiInfo->synchronized = false;
    if ( snd_pcm_link( apiInfo->handles[0], apiInfo->handles[1] ) == 0 )
      apiInfo->synchronized = true;
    else {
      errorText_ = "RtApiAlsa::probeDeviceOpen: unable to synchronize input and output devices.";
      error( RTAUDIO_WARNING );
    }
  }
  else {
    stream_.mode = mode;

    // Setup callback thread.
    stream_.callbackInfo.object = (void *) this;

    // Set the thread attributes for joinable and realtime scheduling
    // priority (optional).  The higher priority will only take affect
    // if the program is run as root or suid. Note, under Linux
    // processes with CAP_SYS_NICE privilege, a user can change
    // scheduling policy and priority (thus need not be root). See
    // POSIX "capabilities".
    pthread_attr_t attr;
    pthread_attr_init( &attr );
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE );
#ifdef SCHED_RR // Undefined with some OSes (e.g. NetBSD 1.6.x with GNU Pthread)
    if ( options && options->flags & RTAUDIO_SCHEDULE_REALTIME ) {
      stream_.callbackInfo.doRealtime = true;
      struct sched_param param;
      int priority = options->priority;
      int min = sched_get_priority_min( SCHED_RR );
      int max = sched_get_priority_max( SCHED_RR );
      if ( priority < min ) priority = min;
      else if ( priority > max ) priority = max;
      param.sched_priority = priority;

      // Set the policy BEFORE the priority. Otherwise it fails.
      pthread_attr_setschedpolicy(&attr, SCHED_RR);
      pthread_attr_setscope (&attr, PTHREAD_SCOPE_SYSTEM);
      // This is definitely required. Otherwise it fails.
      pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
      pthread_attr_setschedparam(&attr, &param);
    }
    else
      pthread_attr_setschedpolicy( &attr, SCHED_OTHER );
#else
    pthread_attr_setschedpolicy( &attr, SCHED_OTHER );
#endif

    stream_.callbackInfo.isRunning = true;
    result = pthread_create( &stream_.callbackInfo.thread, &attr, alsaCallbackHandler, &stream_.callbackInfo );
    pthread_attr_destroy( &attr );
    if ( result ) {
      // Failed. Try instead with default attributes.
      result = pthread_create( &stream_.callbackInfo.thread, NULL, alsaCallbackHandler, &stream_.callbackInfo );
      if ( result ) {
        stream_.callbackInfo.isRunning = false;
        errorText_ = "RtApiAlsa::error creating callback thread!";
        goto error;
      }
    }
  }

  return SUCCESS;

 error:
  if ( apiInfo ) {
    pthread_cond_destroy( &apiInfo->runnable_cv );
    if ( apiInfo->handles[0] ) snd_pcm_close( apiInfo->handles[0] );
    if ( apiInfo->handles[1] ) snd_pcm_close( apiInfo->handles[1] );
    delete apiInfo;
    stream_.apiHandle = 0;
  }

  if ( phandle) snd_pcm_close( phandle );

  for ( int i=0; i<2; i++ ) {
    if ( stream_.userBuffer[i] ) {
      free( stream_.userBuffer[i] );
      stream_.userBuffer[i] = 0;
    }
  }

  if ( stream_.deviceBuffer ) {
    free( stream_.deviceBuffer );
    stream_.deviceBuffer = 0;
  }

  stream_.state = STREAM_CLOSED;
  return FAILURE;
}

void RtApiAlsa :: closeStream()
{
  if ( stream_.state == STREAM_CLOSED ) {
    errorText_ = "RtApiAlsa::closeStream(): no open stream to close!";
    error( RTAUDIO_WARNING );
    return;
  }

  AlsaHandle *apiInfo = (AlsaHandle *) stream_.apiHandle;
  stream_.callbackInfo.isRunning = false;
  MUTEX_LOCK( &stream_.mutex );
  if ( stream_.state == STREAM_STOPPED ) {
    apiInfo->runnable = true;
    pthread_cond_signal( &apiInfo->runnable_cv );
  }
  MUTEX_UNLOCK( &stream_.mutex );
  pthread_join( stream_.callbackInfo.thread, NULL );

  if ( stream_.state == STREAM_RUNNING ) {
    stream_.state = STREAM_STOPPED;
    if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX )
      snd_pcm_drop( apiInfo->handles[0] );
    if ( stream_.mode == INPUT || stream_.mode == DUPLEX )
      snd_pcm_drop( apiInfo->handles[1] );
  }

  if ( apiInfo ) {
    pthread_cond_destroy( &apiInfo->runnable_cv );
    if ( apiInfo->handles[0] ) snd_pcm_close( apiInfo->handles[0] );
    if ( apiInfo->handles[1] ) snd_pcm_close( apiInfo->handles[1] );
    delete apiInfo;
    stream_.apiHandle = 0;
  }

  for ( int i=0; i<2; i++ ) {
    if ( stream_.userBuffer[i] ) {
      free( stream_.userBuffer[i] );
      stream_.userBuffer[i] = 0;
    }
  }

  if ( stream_.deviceBuffer ) {
    free( stream_.deviceBuffer );
    stream_.deviceBuffer = 0;
  }

  clearStreamInfo();
}

RtAudioErrorType RtApiAlsa :: startStream()
{
  // This method calls snd_pcm_prepare if the device isn't already in that state.

  if ( stream_.state != STREAM_STOPPED ) {
    if ( stream_.state == STREAM_RUNNING )
      errorText_ = "RtApiAlsa::startStream(): the stream is already running!";
    else if ( stream_.state == STREAM_STOPPING || stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiAlsa::startStream(): the stream is stopping or closed!";
    return error( RTAUDIO_WARNING );
  }

  MUTEX_LOCK( &stream_.mutex );

  /*
  #if defined( HAVE_GETTIMEOFDAY )
  gettimeofday( &stream_.lastTickTimestamp, NULL );
  #endif
  */
  
  int result = 0;
  snd_pcm_state_t state;
  AlsaHandle *apiInfo = (AlsaHandle *) stream_.apiHandle;
  snd_pcm_t **handle = (snd_pcm_t **) apiInfo->handles;
  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {
    state = snd_pcm_state( handle[0] );
    if ( state != SND_PCM_STATE_PREPARED ) {
      result = snd_pcm_prepare( handle[0] );
      if ( result < 0 ) {
        errorStream_ << "RtApiAlsa::startStream: error preparing output pcm device, " << snd_strerror( result ) << ".";
        errorText_ = errorStream_.str();
        goto unlock;
      }
    }
  }

  if ( ( stream_.mode == INPUT || stream_.mode == DUPLEX ) && !apiInfo->synchronized ) {
    result = snd_pcm_drop(handle[1]); // fix to remove stale data received since device has been open
    state = snd_pcm_state( handle[1] );
    if ( state != SND_PCM_STATE_PREPARED ) {
      result = snd_pcm_prepare( handle[1] );
      if ( result < 0 ) {
        errorStream_ << "RtApiAlsa::startStream: error preparing input pcm device, " << snd_strerror( result ) << ".";
        errorText_ = errorStream_.str();
        goto unlock;
      }
    }
  }

  stream_.state = STREAM_RUNNING;

 unlock:
  apiInfo->runnable = true;
  pthread_cond_signal( &apiInfo->runnable_cv );
  MUTEX_UNLOCK( &stream_.mutex );

  if ( result < 0 ) return error( RTAUDIO_SYSTEM_ERROR );
  return RTAUDIO_NO_ERROR;
}

RtAudioErrorType RtApiAlsa :: stopStream()
{
  if ( stream_.state != STREAM_RUNNING && stream_.state != STREAM_STOPPING ) {
    if ( stream_.state == STREAM_STOPPED )
      errorText_ = "RtApiAlsa::stopStream(): the stream is already stopped!";
    else if ( stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiAlsa::stopStream(): the stream is closed!";
    return error( RTAUDIO_WARNING );
  }

  stream_.state = STREAM_STOPPED;
  MUTEX_LOCK( &stream_.mutex );

  int result = 0;
  AlsaHandle *apiInfo = (AlsaHandle *) stream_.apiHandle;
  snd_pcm_t **handle = (snd_pcm_t **) apiInfo->handles;
  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {
    if ( apiInfo->synchronized ) 
      result = snd_pcm_drop( handle[0] );
    else
      result = snd_pcm_drain( handle[0] );
    if ( result < 0 ) {
      errorStream_ << "RtApiAlsa::stopStream: error draining output pcm device, " << snd_strerror( result ) << ".";
      errorText_ = errorStream_.str();
      goto unlock;
    }
  }

  if ( ( stream_.mode == INPUT || stream_.mode == DUPLEX ) && !apiInfo->synchronized ) {
    result = snd_pcm_drop( handle[1] );
    if ( result < 0 ) {
      errorStream_ << "RtApiAlsa::stopStream: error stopping input pcm device, " << snd_strerror( result ) << ".";
      errorText_ = errorStream_.str();
      goto unlock;
    }
  }

 unlock:
  apiInfo->runnable = false; // fixes high CPU usage when stopped
  MUTEX_UNLOCK( &stream_.mutex );

  if ( result < 0 ) return error( RTAUDIO_SYSTEM_ERROR );
  return RTAUDIO_NO_ERROR;
}

RtAudioErrorType RtApiAlsa :: abortStream()
{
  if ( stream_.state != STREAM_RUNNING ) {
    if ( stream_.state == STREAM_STOPPED )
      errorText_ = "RtApiAlsa::abortStream(): the stream is already stopped!";
    else if ( stream_.state == STREAM_STOPPING || stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiAlsa::abortStream(): the stream is stopping or closed!";
    return error( RTAUDIO_WARNING );
  }

  stream_.state = STREAM_STOPPED;
  MUTEX_LOCK( &stream_.mutex );

  int result = 0;
  AlsaHandle *apiInfo = (AlsaHandle *) stream_.apiHandle;
  snd_pcm_t **handle = (snd_pcm_t **) apiInfo->handles;
  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {
    result = snd_pcm_drop( handle[0] );
    if ( result < 0 ) {
      errorStream_ << "RtApiAlsa::abortStream: error aborting output pcm device, " << snd_strerror( result ) << ".";
      errorText_ = errorStream_.str();
      goto unlock;
    }
  }

  if ( ( stream_.mode == INPUT || stream_.mode == DUPLEX ) && !apiInfo->synchronized ) {
    result = snd_pcm_drop( handle[1] );
    if ( result < 0 ) {
      errorStream_ << "RtApiAlsa::abortStream: error aborting input pcm device, " << snd_strerror( result ) << ".";
      errorText_ = errorStream_.str();
      goto unlock;
    }
  }

 unlock:
  apiInfo->runnable = false; // fixes high CPU usage when stopped
  MUTEX_UNLOCK( &stream_.mutex );

  if ( result < 0 ) return error( RTAUDIO_SYSTEM_ERROR );
  return RTAUDIO_NO_ERROR;
}

void RtApiAlsa :: callbackEvent()
{
  AlsaHandle *apiInfo = (AlsaHandle *) stream_.apiHandle;
  if ( stream_.state == STREAM_STOPPED ) {
    MUTEX_LOCK( &stream_.mutex );
    while ( !apiInfo->runnable )
      pthread_cond_wait( &apiInfo->runnable_cv, &stream_.mutex );

    if ( stream_.state != STREAM_RUNNING ) {
      MUTEX_UNLOCK( &stream_.mutex );
      return;
    }
    MUTEX_UNLOCK( &stream_.mutex );
  }

  if ( stream_.state == STREAM_CLOSED ) {
    errorText_ = "RtApiAlsa::callbackEvent(): the stream is closed ... this shouldn't happen!";
    error( RTAUDIO_WARNING );
    return;
  }

  int doStopStream = 0;
  RtAudioCallback callback = (RtAudioCallback) stream_.callbackInfo.callback;
  double streamTime = getStreamTime();
  RtAudioStreamStatus status = 0;
  if ( stream_.mode != INPUT && apiInfo->xrun[0] == true ) {
    status |= RTAUDIO_OUTPUT_UNDERFLOW;
    apiInfo->xrun[0] = false;
  }
  if ( stream_.mode != OUTPUT && apiInfo->xrun[1] == true ) {
    status |= RTAUDIO_INPUT_OVERFLOW;
    apiInfo->xrun[1] = false;
  }
  doStopStream = callback( stream_.userBuffer[0], stream_.userBuffer[1],
                           stream_.bufferSize, streamTime, status, stream_.callbackInfo.userData );

  if ( doStopStream == 2 ) {
    abortStream();
    return;
  }

  MUTEX_LOCK( &stream_.mutex );

  // The state might change while waiting on a mutex.
  if ( stream_.state == STREAM_STOPPED ) goto unlock;

  int result;
  char *buffer;
  int channels;
  snd_pcm_t **handle;
  snd_pcm_sframes_t frames;
  RtAudioFormat format;
  handle = (snd_pcm_t **) apiInfo->handles;

  if ( stream_.mode == INPUT || stream_.mode == DUPLEX ) {

    // Setup parameters.
    if ( stream_.doConvertBuffer[1] ) {
      buffer = stream_.deviceBuffer;
      channels = stream_.nDeviceChannels[1];
      format = stream_.deviceFormat[1];
    }
    else {
      buffer = stream_.userBuffer[1];
      channels = stream_.nUserChannels[1];
      format = stream_.userFormat;
    }

    // Read samples from device in interleaved/non-interleaved format.
    if ( stream_.deviceInterleaved[1] )
      result = snd_pcm_readi( handle[1], buffer, stream_.bufferSize );
    else {
      void *bufs[channels];
      size_t offset = stream_.bufferSize * formatBytes( format );
      for ( int i=0; i<channels; i++ )
        bufs[i] = (void *) (buffer + (i * offset));
      result = snd_pcm_readn( handle[1], bufs, stream_.bufferSize );
    }

    if ( result < (int) stream_.bufferSize ) {
      // Either an error or overrun occurred.
      if ( result == -EPIPE ) {
        snd_pcm_state_t state = snd_pcm_state( handle[1] );
        if ( state == SND_PCM_STATE_XRUN ) {
          apiInfo->xrun[1] = true;
          result = snd_pcm_prepare( handle[1] );
          if ( result < 0 ) {
            errorStream_ << "RtApiAlsa::callbackEvent: error preparing device after overrun, " << snd_strerror( result ) << ".";
            errorText_ = errorStream_.str();
          }
        }
        else {
          errorStream_ << "RtApiAlsa::callbackEvent: error, current state is " << snd_pcm_state_name( state ) << ", " << snd_strerror( result ) << ".";
          errorText_ = errorStream_.str();
        }
      }
      else {
        errorStream_ << "RtApiAlsa::callbackEvent: audio read error, " << snd_strerror( result ) << ".";
        errorText_ = errorStream_.str();
      }
      error( RTAUDIO_WARNING );
      goto tryOutput;
    }

    // Do byte swapping if necessary.
    if ( stream_.doByteSwap[1] )
      byteSwapBuffer( buffer, stream_.bufferSize * channels, format );

    // Do buffer conversion if necessary.
    if ( stream_.doConvertBuffer[1] )
      convertBuffer( stream_.userBuffer[1], stream_.deviceBuffer, stream_.convertInfo[1] );

    // Check stream latency
    result = snd_pcm_delay( handle[1], &frames );
    if ( result == 0 && frames > 0 ) stream_.latency[1] = frames;
  }

 tryOutput:

  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {

    // Setup parameters and do buffer conversion if necessary.
    if ( stream_.doConvertBuffer[0] ) {
      buffer = stream_.deviceBuffer;
      convertBuffer( buffer, stream_.userBuffer[0], stream_.convertInfo[0] );
      channels = stream_.nDeviceChannels[0];
      format = stream_.deviceFormat[0];
    }
    else {
      buffer = stream_.userBuffer[0];
      channels = stream_.nUserChannels[0];
      format = stream_.userFormat;
    }

    // Do byte swapping if necessary.
    if ( stream_.doByteSwap[0] )
      byteSwapBuffer(buffer, stream_.bufferSize * channels, format);

    // Write samples to device in interleaved/non-interleaved format.
    if ( stream_.deviceInterleaved[0] )
      result = snd_pcm_writei( handle[0], buffer, stream_.bufferSize );
    else {
      void *bufs[channels];
      size_t offset = stream_.bufferSize * formatBytes( format );
      for ( int i=0; i<channels; i++ )
        bufs[i] = (void *) (buffer + (i * offset));
      result = snd_pcm_writen( handle[0], bufs, stream_.bufferSize );
    }

    if ( result < (int) stream_.bufferSize ) {
      // Either an error or underrun occurred.
      if ( result == -EPIPE ) {
        snd_pcm_state_t state = snd_pcm_state( handle[0] );
        if ( state == SND_PCM_STATE_XRUN ) {
          apiInfo->xrun[0] = true;
          result = snd_pcm_prepare( handle[0] );
          if ( result < 0 ) {
            errorStream_ << "RtApiAlsa::callbackEvent: error preparing device after underrun, " << snd_strerror( result ) << ".";
            errorText_ = errorStream_.str();
          }
          else
            errorText_ =  "RtApiAlsa::callbackEvent: audio write error, underrun.";
        }
        else {
          errorStream_ << "RtApiAlsa::callbackEvent: error, current state is " << snd_pcm_state_name( state ) << ", " << snd_strerror( result ) << ".";
          errorText_ = errorStream_.str();
        }
      }
      else {
        errorStream_ << "RtApiAlsa::callbackEvent: audio write error, " << snd_strerror( result ) << ".";
        errorText_ = errorStream_.str();
      }
      error( RTAUDIO_WARNING );
      goto unlock;
    }

    // Check stream latency
    result = snd_pcm_delay( handle[0], &frames );
    if ( result == 0 && frames > 0 ) stream_.latency[0] = frames;
  }

 unlock:
  MUTEX_UNLOCK( &stream_.mutex );

  RtApi::tickStreamTime();
  if ( doStopStream == 1 ) this->stopStream();
}

static void *alsaCallbackHandler( void *ptr )
{
  CallbackInfo *info = (CallbackInfo *) ptr;
  RtApiAlsa *object = (RtApiAlsa *) info->object;
  bool *isRunning = &info->isRunning;

#ifdef SCHED_RR // Undefined with some OSes (e.g. NetBSD 1.6.x with GNU Pthread)
  if ( info->doRealtime ) {
    std::cerr << "RtAudio alsa: " << 
             (sched_getscheduler(0) == SCHED_RR ? "" : "_NOT_ ") << 
             "running realtime scheduling" << std::endl;
  }
#endif

  while ( *isRunning == true ) {
    pthread_testcancel();
    object->callbackEvent();
  }

  pthread_exit( NULL );
}

//******************** End of __LINUX_ALSA__ *********************//
#endif

#if defined(__LINUX_PULSE__)

// Code written by Peter Meerwald, pmeerw@pmeerw.net and Tristan Matthews.
// Updated by Gary Scavone, 2021.

#include <pulse/error.h>
#include <pulse/simple.h>
#include <cstdio>

// A structure needed to pass variables for device probing.
struct PaDeviceProbeInfo {
  pa_mainloop_api *paMainLoopApi;
  std::string defaultSinkName;
  std::string defaultSourceName;
  int defaultRate;
  unsigned int *currentDeviceId;
  std::vector< std::string > deviceNames;
  std::vector< RtApiPulse::PaDeviceInfo > *paDeviceList;
  std::vector< RtAudio::DeviceInfo > *rtDeviceList;
};

static const unsigned int SUPPORTED_SAMPLERATES[] = { 8000, 16000, 22050, 32000,
                                                      44100, 48000, 96000, 192000, 0};

struct rtaudio_pa_format_mapping_t {
  RtAudioFormat rtaudio_format;
  pa_sample_format_t pa_format;
};

static const rtaudio_pa_format_mapping_t supported_sampleformats[] = {
  {RTAUDIO_SINT16, PA_SAMPLE_S16LE},
  {RTAUDIO_SINT24, PA_SAMPLE_S24LE},
  {RTAUDIO_SINT32, PA_SAMPLE_S32LE},
  {RTAUDIO_FLOAT32, PA_SAMPLE_FLOAT32LE},
  {0, PA_SAMPLE_INVALID}};

struct PulseAudioHandle {
  pa_simple *s_play;
  pa_simple *s_rec;
  pthread_t thread;
  pthread_cond_t runnable_cv;
  bool runnable;
  PulseAudioHandle() : s_play(0), s_rec(0), runnable(false) { }
};

// The following 3 functions are called by the device probing
// system. This first one gets overall system information.
static void rt_pa_set_server_info( pa_context *context, const pa_server_info *info, void *userdata )
{
  (void)context;
  pa_sample_spec ss;

  PaDeviceProbeInfo *paProbeInfo = static_cast<PaDeviceProbeInfo *>( userdata );
  if (!info) {
    paProbeInfo->paMainLoopApi->quit( paProbeInfo->paMainLoopApi, 1 );
    return;
  }

  ss = info->sample_spec;
  paProbeInfo->defaultRate = ss.rate;
  paProbeInfo->defaultSinkName = info->default_sink_name;
  paProbeInfo->defaultSourceName = info->default_source_name;
}

// Used to get output device information.
static void rt_pa_set_sink_info( pa_context * /*c*/, const pa_sink_info *i,
                                 int eol, void *userdata )
{
  if ( eol ) return;

  PaDeviceProbeInfo *paProbeInfo = static_cast<PaDeviceProbeInfo *>( userdata );
  std::string name = pa_proplist_gets( i->proplist, "device.description" );
  paProbeInfo->deviceNames.push_back( name );
  for ( size_t n=0; n<paProbeInfo->rtDeviceList->size(); n++ )
    if ( paProbeInfo->rtDeviceList->at(n).name == name ) return; // we've already probed this one
  
  RtAudio::DeviceInfo info;
  info.name = name;
  info.outputChannels = i->sample_spec.channels;
  info.preferredSampleRate = i->sample_spec.rate;
  info.isDefaultOutput = ( paProbeInfo->defaultSinkName == i->name );
  for ( const unsigned int *sr = SUPPORTED_SAMPLERATES; *sr; ++sr )
    info.sampleRates.push_back( *sr );
  for ( const rtaudio_pa_format_mapping_t *fm = supported_sampleformats; fm->rtaudio_format; ++fm )
    info.nativeFormats |= fm->rtaudio_format;
  info.ID = *(paProbeInfo->currentDeviceId);
  *(paProbeInfo->currentDeviceId) = info.ID + 1;
  paProbeInfo->rtDeviceList->push_back( info );

  RtApiPulse::PaDeviceInfo painfo;
  painfo.sinkName = i->name;
  paProbeInfo->paDeviceList->push_back( painfo );
}

// Used to get input device information.
static void rt_pa_set_source_info_and_quit( pa_context * /*c*/, const pa_source_info *i,
                                            int eol, void *userdata )
{
  PaDeviceProbeInfo *paProbeInfo = static_cast<PaDeviceProbeInfo *>( userdata );
  if ( eol ) {
    paProbeInfo->paMainLoopApi->quit( paProbeInfo->paMainLoopApi, 0 );
    return;
  }

  std::string name = pa_proplist_gets( i->proplist, "device.description" );
  paProbeInfo->deviceNames.push_back( name );
  for ( size_t n=0; n<paProbeInfo->rtDeviceList->size(); n++ ) {
    if ( paProbeInfo->rtDeviceList->at(n).name == name ) {
      // Check if we've already probed this as an output.
      if ( !paProbeInfo->paDeviceList->at(n).sinkName.empty() ) {
        // This must be a duplex device. Update the device info.
        paProbeInfo->paDeviceList->at(n).sourceName = i->name;
        paProbeInfo->rtDeviceList->at(n).inputChannels = i->sample_spec.channels;
        paProbeInfo->rtDeviceList->at(n).isDefaultInput = ( paProbeInfo->defaultSourceName == i->name );
        paProbeInfo->rtDeviceList->at(n).duplexChannels = 
          (paProbeInfo->rtDeviceList->at(n).inputChannels < paProbeInfo->rtDeviceList->at(n).outputChannels)
          ? paProbeInfo->rtDeviceList->at(n).inputChannels : paProbeInfo->rtDeviceList->at(n).outputChannels;
      }
      return; // we already have this
    }
  }

  RtAudio::DeviceInfo info;
  info.name = name;
  info.inputChannels = i->sample_spec.channels;
  info.preferredSampleRate = i->sample_spec.rate;
  info.isDefaultInput = ( paProbeInfo->defaultSourceName == i->name );
  for ( const unsigned int *sr = SUPPORTED_SAMPLERATES; *sr; ++sr )
    info.sampleRates.push_back( *sr );
  for ( const rtaudio_pa_format_mapping_t *fm = supported_sampleformats; fm->rtaudio_format; ++fm )
    info.nativeFormats |= fm->rtaudio_format;
  info.ID = *(paProbeInfo->currentDeviceId);
  *(paProbeInfo->currentDeviceId) = info.ID + 1;
  paProbeInfo->rtDeviceList->push_back( info );

  RtApiPulse::PaDeviceInfo painfo;
  painfo.sourceName = i->name;
  paProbeInfo->paDeviceList->push_back( painfo );
}

// This is the initial function that is called when the callback is
// set. This one then calls the functions above.
static void rt_pa_context_state_callback( pa_context *context, void *userdata )
{
  PaDeviceProbeInfo *paProbeInfo = static_cast<PaDeviceProbeInfo *>( userdata );
  auto state = pa_context_get_state(context);
  switch (state) {
    case PA_CONTEXT_CONNECTING:
    case PA_CONTEXT_AUTHORIZING:
    case PA_CONTEXT_SETTING_NAME:
      break;

    case PA_CONTEXT_READY:
      pa_context_get_server_info( context, rt_pa_set_server_info, userdata ); // server info
      pa_context_get_sink_info_list( context, rt_pa_set_sink_info, userdata ); // output info ... needs to be before input
      pa_context_get_source_info_list( context, rt_pa_set_source_info_and_quit, userdata ); // input info
      break;

    case PA_CONTEXT_TERMINATED:
      paProbeInfo->paMainLoopApi->quit( paProbeInfo->paMainLoopApi, 0 );
      break;

    case PA_CONTEXT_FAILED:
    default:
      paProbeInfo->paMainLoopApi->quit( paProbeInfo->paMainLoopApi, 1 );
  }
}

RtApiPulse::~RtApiPulse()
{
  if ( stream_.state != STREAM_CLOSED )
    closeStream();
}

void RtApiPulse :: probeDevices( void )
{
  // See list of required functionality in RtApi::probeDevices().
  
  pa_mainloop *ml = NULL;
  pa_context *context = NULL;
  char *server = NULL;
  int ret = 1;
  PaDeviceProbeInfo paProbeInfo;
  if (!(ml = pa_mainloop_new())) {
    errorStream_ << "RtApiPulse::probeDevices: pa_mainloop_new() failed.";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    goto quit;
  }

  paProbeInfo.paMainLoopApi = pa_mainloop_get_api( ml );
  paProbeInfo.currentDeviceId = &currentDeviceId_;
  paProbeInfo.paDeviceList = &paDeviceList_;
  paProbeInfo.rtDeviceList = &deviceList_;

  if (!(context = pa_context_new_with_proplist( paProbeInfo.paMainLoopApi, NULL, NULL ))) {
    errorStream_ << "RtApiPulse::probeDevices: pa_context_new() failed.";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    goto quit;
  }

  pa_context_set_state_callback( context, rt_pa_context_state_callback, &paProbeInfo );
  
  if (pa_context_connect( context, server, PA_CONTEXT_NOFLAGS, NULL ) < 0) {
    errorStream_ << "RtApiPulse::probeDevices: pa_context_connect() failed: "
      << pa_strerror(pa_context_errno(context));
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    goto quit;
  }

  if (pa_mainloop_run( ml, &ret ) < 0) {
    errorStream_ << "RtApiPulse::probeDevices: pa_mainloop_run() failed.";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    goto quit;
  }

  if (ret != 0) {
    errorStream_ << "RtApiPulse::probeDevices: could not get server info.";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    goto quit;
  }

  // Check for devices that have been unplugged.
  unsigned int m;
  for ( std::vector<RtAudio::DeviceInfo>::iterator it=deviceList_.begin(); it!=deviceList_.end(); ) {
    for ( m=0; m<paProbeInfo.deviceNames.size(); m++ ) {
      if ( (*it).name == paProbeInfo.deviceNames[m] ) {
        ++it;
        break;
      }
    }
    if ( m == paProbeInfo.deviceNames.size() ) { // not found so remove it from our list
      it = deviceList_.erase( it );
      paDeviceList_.erase( paDeviceList_.begin() + distance(deviceList_.begin(), it ) );
    }
  }
  
quit:
  if (context)
    pa_context_unref(context);

  if (ml)
    pa_mainloop_free(ml);

  pa_xfree(server);
}

static void *pulseaudio_callback( void * user )
{
  CallbackInfo *cbi = static_cast<CallbackInfo *>( user );
  RtApiPulse *context = static_cast<RtApiPulse *>( cbi->object );
  volatile bool *isRunning = &cbi->isRunning;
  
#ifdef SCHED_RR // Undefined with some OSes (e.g. NetBSD 1.6.x with GNU Pthread)
  if (cbi->doRealtime) {
    std::cerr << "RtAudio pulse: " << 
             (sched_getscheduler(0) == SCHED_RR ? "" : "_NOT_ ") << 
             "running realtime scheduling" << std::endl;
  }
#endif
  
  while ( *isRunning ) {
    pthread_testcancel();
    context->callbackEvent();
  }

  pthread_exit( NULL );
}

bool RtApiPulse::probeDeviceOpen( unsigned int deviceId, StreamMode mode,
                                  unsigned int channels, unsigned int firstChannel,
                                  unsigned int sampleRate, RtAudioFormat format,
                                  unsigned int *bufferSize, RtAudio::StreamOptions *options )
{
  PulseAudioHandle *pah = 0;
  unsigned long bufferBytes = 0;
  pa_sample_spec ss;

  int deviceIdx = -1;
  for ( unsigned int m=0; m<deviceList_.size(); m++ ) {
    if ( deviceList_[m].ID == deviceId ) {
      deviceIdx = m;
      break;
    }
  }

  if ( deviceIdx < 0 ) return false;

  if ( firstChannel != 0 ) {
    errorText_ = "PulseAudio does not support channel offset mapping.";
    return false;
  }

  // These may be NULL for default devices but we already have the names.
  const char *dev_input = NULL;
  const char *dev_output = NULL;
  if ( !paDeviceList_[deviceIdx].sourceName.empty() )
    dev_input = paDeviceList_[deviceIdx].sourceName.c_str();
  if ( !paDeviceList_[deviceIdx].sinkName.empty() )
    dev_output = paDeviceList_[deviceIdx].sinkName.c_str();

  if ( mode==INPUT && deviceList_[deviceIdx].inputChannels < channels ) {
    errorText_ = "PulseAudio device does not support requested input channel count.";
    return false;
  }
  if ( mode==OUTPUT && deviceList_[deviceIdx].outputChannels < channels ) {
    errorText_ = "PulseAudio device does not support requested output channel count.";
    return false;
  }

  ss.channels = channels;

  bool sr_found = false;
  for ( const unsigned int *sr = SUPPORTED_SAMPLERATES; *sr; ++sr ) {
    if ( sampleRate == *sr ) {
      sr_found = true;
      stream_.sampleRate = sampleRate;
      ss.rate = sampleRate;
      break;
    }
  }
  if ( !sr_found ) {
    stream_.sampleRate = sampleRate;
    ss.rate = sampleRate;
  }

  bool sf_found = 0;
  for ( const rtaudio_pa_format_mapping_t *sf = supported_sampleformats;
        sf->rtaudio_format && sf->pa_format != PA_SAMPLE_INVALID; ++sf ) {
    if ( format == sf->rtaudio_format ) {
      sf_found = true;
      stream_.userFormat = sf->rtaudio_format;
      stream_.deviceFormat[mode] = stream_.userFormat;
      ss.format = sf->pa_format;
      break;
    }
  }
  if ( !sf_found ) { // Use internal data format conversion.
    stream_.userFormat = format;
    stream_.deviceFormat[mode] = RTAUDIO_FLOAT32;
    ss.format = PA_SAMPLE_FLOAT32LE;
  }

  // Set other stream parameters.
  if ( options && options->flags & RTAUDIO_NONINTERLEAVED ) stream_.userInterleaved = false;
  else stream_.userInterleaved = true;
  stream_.deviceInterleaved[mode] = true;
  stream_.nBuffers = options ? options->numberOfBuffers : 1;
  stream_.doByteSwap[mode] = false;
  stream_.nUserChannels[mode] = channels;
  stream_.nDeviceChannels[mode] = channels + firstChannel;
  stream_.channelOffset[mode] = 0;
  std::string streamName = "RtAudio";

  // Set flags for buffer conversion.
  stream_.doConvertBuffer[mode] = false;
  if ( stream_.userFormat != stream_.deviceFormat[mode] )
    stream_.doConvertBuffer[mode] = true;
  if ( stream_.nUserChannels[mode] < stream_.nDeviceChannels[mode] )
    stream_.doConvertBuffer[mode] = true;
  if ( stream_.userInterleaved != stream_.deviceInterleaved[mode] )
    stream_.doConvertBuffer[mode] = true;

  // Allocate necessary internal buffers.
  bufferBytes = stream_.nUserChannels[mode] * *bufferSize * formatBytes( stream_.userFormat );
  stream_.userBuffer[mode] = (char *) calloc( bufferBytes, 1 );
  if ( stream_.userBuffer[mode] == NULL ) {
    errorText_ = "RtApiPulse::probeDeviceOpen: error allocating user buffer memory.";
    goto error;
  }
  stream_.bufferSize = *bufferSize;

  if ( stream_.doConvertBuffer[mode] ) {

    bool makeBuffer = true;
    bufferBytes = stream_.nDeviceChannels[mode] * formatBytes( stream_.deviceFormat[mode] );
    if ( mode == INPUT ) {
      if ( stream_.mode == OUTPUT && stream_.deviceBuffer ) {
        unsigned long bytesOut = stream_.nDeviceChannels[0] * formatBytes( stream_.deviceFormat[0] );
        if ( bufferBytes <= bytesOut ) makeBuffer = false;
      }
    }

    if ( makeBuffer ) {
      bufferBytes *= *bufferSize;
      if ( stream_.deviceBuffer ) free( stream_.deviceBuffer );
      stream_.deviceBuffer = (char *) calloc( bufferBytes, 1 );
      if ( stream_.deviceBuffer == NULL ) {
        errorText_ = "RtApiPulse::probeDeviceOpen: error allocating device buffer memory.";
        goto error;
      }
    }
  }

  stream_.deviceId[mode] = deviceIdx;

  // Setup the buffer conversion information structure.
  if ( stream_.doConvertBuffer[mode] ) setConvertInfo( mode, firstChannel );

  if ( !stream_.apiHandle ) {
    PulseAudioHandle *pah = new PulseAudioHandle;
    if ( !pah ) {
      errorText_ = "RtApiPulse::probeDeviceOpen: error allocating memory for handle.";
      goto error;
    }

    stream_.apiHandle = pah;
    if ( pthread_cond_init( &pah->runnable_cv, NULL ) != 0 ) {
      errorText_ = "RtApiPulse::probeDeviceOpen: error creating condition variable.";
      goto error;
    }
  }
  pah = static_cast<PulseAudioHandle *>( stream_.apiHandle );

  int error;
  if ( options && !options->streamName.empty() ) streamName = options->streamName;
  switch ( mode ) {
    pa_buffer_attr buffer_attr;
  case INPUT:
    buffer_attr.fragsize = bufferBytes;
    buffer_attr.maxlength = -1;

    pah->s_rec = pa_simple_new( NULL, streamName.c_str(), PA_STREAM_RECORD,
                                dev_input, "Record", &ss, NULL, &buffer_attr, &error );
    if ( !pah->s_rec ) {
      errorText_ = "RtApiPulse::probeDeviceOpen: error connecting input to PulseAudio server.";
      goto error;
    }
    break;
  case OUTPUT: {
    pa_buffer_attr * attr_ptr;

    if ( options && options->numberOfBuffers > 0 ) {
      // pa_buffer_attr::fragsize is recording-only.
      // Hopefully PortAudio won't access uninitialized fields.
      buffer_attr.maxlength = bufferBytes * options->numberOfBuffers;
      buffer_attr.minreq = -1;
      buffer_attr.prebuf = -1;
      buffer_attr.tlength = -1;
      attr_ptr = &buffer_attr;
    } else {
      attr_ptr = nullptr;
    }

    pah->s_play = pa_simple_new( NULL, streamName.c_str(), PA_STREAM_PLAYBACK,
                                 dev_output, "Playback", &ss, NULL, attr_ptr, &error );
    if ( !pah->s_play ) {
      errorText_ = "RtApiPulse::probeDeviceOpen: error connecting output to PulseAudio server.";
      goto error;
    }
    break;
  }
  case DUPLEX:
    /* Note: We could add DUPLEX by synchronizing multiple streams,
       but it would mean moving from Simple API to Asynchronous API:
       https://freedesktop.org/software/pulseaudio/doxygen/streams.html#sync_streams */
    errorText_ = "RtApiPulse::probeDeviceOpen: duplex not supported for PulseAudio.";
    goto error;
  default:
    goto error;
  }

  if ( stream_.mode == UNINITIALIZED )
    stream_.mode = mode;
  else if ( stream_.mode == mode )
    goto error;
  else
    stream_.mode = DUPLEX;

  if ( !stream_.callbackInfo.isRunning ) {
    stream_.callbackInfo.object = this;
    
    stream_.state = STREAM_STOPPED;
    // Set the thread attributes for joinable and realtime scheduling
    // priority (optional).  The higher priority will only take affect
    // if the program is run as root or suid. Note, under Linux
    // processes with CAP_SYS_NICE privilege, a user can change
    // scheduling policy and priority (thus need not be root). See
    // POSIX "capabilities".
    pthread_attr_t attr;
    pthread_attr_init( &attr );
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE );
#ifdef SCHED_RR // Undefined with some OSes (e.g. NetBSD 1.6.x with GNU Pthread)
    if ( options && options->flags & RTAUDIO_SCHEDULE_REALTIME ) {
      stream_.callbackInfo.doRealtime = true;
      struct sched_param param;
      int priority = options->priority;
      int min = sched_get_priority_min( SCHED_RR );
      int max = sched_get_priority_max( SCHED_RR );
      if ( priority < min ) priority = min;
      else if ( priority > max ) priority = max;
      param.sched_priority = priority;
      
      // Set the policy BEFORE the priority. Otherwise it fails.
      pthread_attr_setschedpolicy(&attr, SCHED_RR);
      pthread_attr_setscope (&attr, PTHREAD_SCOPE_SYSTEM);
      // This is definitely required. Otherwise it fails.
      pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
      pthread_attr_setschedparam(&attr, &param);
    }
    else
      pthread_attr_setschedpolicy( &attr, SCHED_OTHER );
#else
    pthread_attr_setschedpolicy( &attr, SCHED_OTHER );
#endif

    stream_.callbackInfo.isRunning = true;
    int result = pthread_create( &pah->thread, &attr, pulseaudio_callback, (void *)&stream_.callbackInfo);
    pthread_attr_destroy(&attr);
    if(result != 0) {
      // Failed. Try instead with default attributes.
      result = pthread_create( &pah->thread, NULL, pulseaudio_callback, (void *)&stream_.callbackInfo);
      if(result != 0) {
        stream_.callbackInfo.isRunning = false;
        errorText_ = "RtApiPulse::probeDeviceOpen: error creating thread.";
        goto error;
      }
    }
  }

  return SUCCESS;
 
 error:
  if ( pah && stream_.callbackInfo.isRunning ) {
    pthread_cond_destroy( &pah->runnable_cv );
    delete pah;
    stream_.apiHandle = 0;
  }

  for ( int i=0; i<2; i++ ) {
    if ( stream_.userBuffer[i] ) {
      free( stream_.userBuffer[i] );
      stream_.userBuffer[i] = 0;
    }
  }

  if ( stream_.deviceBuffer ) {
    free( stream_.deviceBuffer );
    stream_.deviceBuffer = 0;
  }

  stream_.state = STREAM_CLOSED;
  return FAILURE;
}

void RtApiPulse::closeStream( void )
{
  PulseAudioHandle *pah = static_cast<PulseAudioHandle *>( stream_.apiHandle );

  stream_.callbackInfo.isRunning = false;
  if ( pah ) {
    MUTEX_LOCK( &stream_.mutex );
    if ( stream_.state == STREAM_STOPPED ) {
      pah->runnable = true;
      pthread_cond_signal( &pah->runnable_cv );
    }
    MUTEX_UNLOCK( &stream_.mutex );

    pthread_join( pah->thread, 0 );
    if ( pah->s_play ) {
      pa_simple_flush( pah->s_play, NULL );
      pa_simple_free( pah->s_play );
    }
    if ( pah->s_rec )
      pa_simple_free( pah->s_rec );

    pthread_cond_destroy( &pah->runnable_cv );
    delete pah;
    stream_.apiHandle = 0;
  }

  if ( stream_.userBuffer[0] ) {
    free( stream_.userBuffer[0] );
    stream_.userBuffer[0] = 0;
  }
  if ( stream_.userBuffer[1] ) {
    free( stream_.userBuffer[1] );
    stream_.userBuffer[1] = 0;
  }

  clearStreamInfo();
}

void RtApiPulse::callbackEvent( void )
{
  PulseAudioHandle *pah = static_cast<PulseAudioHandle *>( stream_.apiHandle );

  if ( stream_.state == STREAM_STOPPED ) {
    MUTEX_LOCK( &stream_.mutex );
    while ( !pah->runnable )
      pthread_cond_wait( &pah->runnable_cv, &stream_.mutex );

    if ( stream_.state != STREAM_RUNNING ) {
      MUTEX_UNLOCK( &stream_.mutex );
      return;
    }
    MUTEX_UNLOCK( &stream_.mutex );
  }

  if ( stream_.state == STREAM_CLOSED ) {
    errorText_ = "RtApiPulse::callbackEvent(): the stream is closed ... "
      "this shouldn't happen!";
    error( RTAUDIO_WARNING );
    return;
  }

  RtAudioCallback callback = (RtAudioCallback) stream_.callbackInfo.callback;
  double streamTime = getStreamTime();
  RtAudioStreamStatus status = 0;
  int doStopStream = callback( stream_.userBuffer[OUTPUT], stream_.userBuffer[INPUT],
                               stream_.bufferSize, streamTime, status,
                               stream_.callbackInfo.userData );

  if ( doStopStream == 2 ) {
    abortStream();
    return;
  }

  MUTEX_LOCK( &stream_.mutex );
  void *pulse_in = stream_.doConvertBuffer[INPUT] ? stream_.deviceBuffer : stream_.userBuffer[INPUT];
  void *pulse_out = stream_.doConvertBuffer[OUTPUT] ? stream_.deviceBuffer : stream_.userBuffer[OUTPUT];

  if ( stream_.state != STREAM_RUNNING )
    goto unlock;

  int pa_error;
  size_t bytes;
  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {
    if ( stream_.doConvertBuffer[OUTPUT] ) {
        convertBuffer( stream_.deviceBuffer,
                       stream_.userBuffer[OUTPUT],
                       stream_.convertInfo[OUTPUT] );
        bytes = stream_.nDeviceChannels[OUTPUT] * stream_.bufferSize *
                formatBytes( stream_.deviceFormat[OUTPUT] );
    } else
        bytes = stream_.nUserChannels[OUTPUT] * stream_.bufferSize *
                formatBytes( stream_.userFormat );

    if ( pa_simple_write( pah->s_play, pulse_out, bytes, &pa_error ) < 0 ) {
      errorStream_ << "RtApiPulse::callbackEvent: audio write error, " <<
        pa_strerror( pa_error ) << ".";
      errorText_ = errorStream_.str();
      error( RTAUDIO_WARNING );
    }
  }

  if ( stream_.mode == INPUT || stream_.mode == DUPLEX) {
    if ( stream_.doConvertBuffer[INPUT] )
      bytes = stream_.nDeviceChannels[INPUT] * stream_.bufferSize *
        formatBytes( stream_.deviceFormat[INPUT] );
    else
      bytes = stream_.nUserChannels[INPUT] * stream_.bufferSize *
        formatBytes( stream_.userFormat );
            
    if ( pa_simple_read( pah->s_rec, pulse_in, bytes, &pa_error ) < 0 ) {
      errorStream_ << "RtApiPulse::callbackEvent: audio read error, " <<
        pa_strerror( pa_error ) << ".";
      errorText_ = errorStream_.str();
      error( RTAUDIO_WARNING );
    }
    if ( stream_.doConvertBuffer[INPUT] ) {
      convertBuffer( stream_.userBuffer[INPUT],
                     stream_.deviceBuffer,
                     stream_.convertInfo[INPUT] );
    }
  }

 unlock:
  MUTEX_UNLOCK( &stream_.mutex );
  RtApi::tickStreamTime();

  if ( doStopStream == 1 )
    stopStream();
}

RtAudioErrorType RtApiPulse::startStream( void )
{
  if ( stream_.state != STREAM_STOPPED ) {
    if ( stream_.state == STREAM_RUNNING )
      errorText_ = "RtApiPulse::startStream(): the stream is already running!";
    else if ( stream_.state == STREAM_STOPPING || stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiPulse::startStream(): the stream is stopping or closed!";
    return error( RTAUDIO_WARNING );
  }
  
  PulseAudioHandle *pah = static_cast<PulseAudioHandle *>( stream_.apiHandle );

  MUTEX_LOCK( &stream_.mutex );

  /*
  #if defined( HAVE_GETTIMEOFDAY )
  gettimeofday( &stream_.lastTickTimestamp, NULL );
  #endif
  */
  
  stream_.state = STREAM_RUNNING;

  pah->runnable = true;
  pthread_cond_signal( &pah->runnable_cv );
  MUTEX_UNLOCK( &stream_.mutex );
  return RTAUDIO_NO_ERROR;
}

RtAudioErrorType RtApiPulse::stopStream( void )
{
  if ( stream_.state != STREAM_RUNNING && stream_.state != STREAM_STOPPING ) {
    if ( stream_.state == STREAM_STOPPED )
      errorText_ = "RtApiPulse::stopStream(): the stream is already stopped!";
    else if ( stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiPulse::stopStream(): the stream is closed!";
    return error( RTAUDIO_WARNING );
  }
    
  PulseAudioHandle *pah = static_cast<PulseAudioHandle *>( stream_.apiHandle );

  stream_.state = STREAM_STOPPED;
  MUTEX_LOCK( &stream_.mutex );

  if ( pah ) {
    pah->runnable = false;
    if ( pah->s_play ) {
      int pa_error;
      if ( pa_simple_drain( pah->s_play, &pa_error ) < 0 ) {
        errorStream_ << "RtApiPulse::stopStream: error draining output device, " <<
          pa_strerror( pa_error ) << ".";
        errorText_ = errorStream_.str();
        MUTEX_UNLOCK( &stream_.mutex );
        return error( RTAUDIO_SYSTEM_ERROR );
      }
    }
  }

  stream_.state = STREAM_STOPPED;
  MUTEX_UNLOCK( &stream_.mutex );
  return RTAUDIO_NO_ERROR;
}

RtAudioErrorType RtApiPulse::abortStream( void )
{
  if ( stream_.state != STREAM_RUNNING ) {
    if ( stream_.state == STREAM_STOPPED )
      errorText_ = "RtApiPulse::abortStream(): the stream is already stopped!";
    else if ( stream_.state == STREAM_STOPPING || stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiPulse::abortStream(): the stream is stopping or closed!";
    return error( RTAUDIO_WARNING );
  }
  
  PulseAudioHandle *pah = static_cast<PulseAudioHandle*>( stream_.apiHandle );

  stream_.state = STREAM_STOPPED;
  MUTEX_LOCK( &stream_.mutex );

  if ( pah ) {
    pah->runnable = false;
    if ( pah->s_play ) {
      int pa_error;
      if ( pa_simple_flush( pah->s_play, &pa_error ) < 0 ) {
        errorStream_ << "RtApiPulse::abortStream: error flushing output device, " <<
          pa_strerror( pa_error ) << ".";
        errorText_ = errorStream_.str();
        MUTEX_UNLOCK( &stream_.mutex );
        return error( RTAUDIO_SYSTEM_ERROR );
      }
    }
  }

  stream_.state = STREAM_STOPPED;
  MUTEX_UNLOCK( &stream_.mutex );
  return RTAUDIO_NO_ERROR;
}

//******************** End of __LINUX_PULSE__ *********************//
#endif

#if defined(__LINUX_OSS__)

#include <unistd.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <math.h>

static void *ossCallbackHandler(void * ptr);

// A structure to hold various information related to the OSS API
// implementation.
struct OssHandle {
  int id[2];    // device ids
  bool xrun[2];
  bool triggered;
  pthread_cond_t runnable;

  OssHandle()
    :triggered(false) { id[0] = 0; id[1] = 0; xrun[0] = false; xrun[1] = false; }
};

RtApiOss :: RtApiOss()
{
  // Nothing to do here.
}

RtApiOss :: ~RtApiOss()
{
  if ( stream_.state != STREAM_CLOSED ) closeStream();
}

void RtApiOss :: probeDevices( void )
{
  // See list of required functionality in RtApi::probeDevices().
  
  int mixerfd = open( "/dev/mixer", O_RDWR, 0 );
  if ( mixerfd == -1 ) {
    errorText_ = "RtApiOss::probeDevices: error opening '/dev/mixer'.";
    error( RTAUDIO_SYSTEM_ERROR );
    return;
  }

  oss_sysinfo sysinfo;
  if ( ioctl( mixerfd, SNDCTL_SYSINFO, &sysinfo ) == -1 ) {
    close( mixerfd );
    errorText_ = "RtApiOss::probeDevices: error getting sysinfo, OSS version >= 4.0 is required.";
    error( RTAUDIO_SYSTEM_ERROR );
    return;
  }

  unsigned int nDevices = sysinfo.numaudios;
  if ( nDevices == 0 ) {
    close( mixerfd );
    deviceList_.clear();
    return;
  }

  oss_audioinfo ainfo;
  unsigned int m, n;
  std::vector<std::string> deviceNames;
  for ( n=0; n<nDevices; n++ ) {
    ainfo.dev = n;
    if ( ioctl( mixerfd, SNDCTL_AUDIOINFO, &ainfo ) == -1 ) continue;
    deviceNames.push_back( ainfo.name );
    for ( m=0; m<deviceList_.size(); m++ ) {
      if ( deviceList_[m].name == deviceNames.back() )
        break; // We already have this device.
    }
    if ( m == deviceList_.size() ) { // new device
      RtAudio::DeviceInfo info;
      if ( probeDeviceInfo( info, ainfo ) == false ) continue; // ignore if probe fails
      info.ID = currentDeviceId_++;  // arbitrary internal device ID
      deviceList_.push_back( info );
    }
  }
  close( mixerfd );

  // Remove any devices left in the list that are no longer available.
  for ( std::vector<RtAudio::DeviceInfo>::iterator it=deviceList_.begin(); it!=deviceList_.end(); ) {
    for ( m=0; m<deviceNames.size(); m++ ) {
      if ( (*it).name == deviceNames[m] ) {
        ++it;
        break;
      }
    }
    if ( m == deviceNames.size() ) // not found so remove it from our list
      it = deviceList_.erase( it );
  }

  // I don't think the OSS API supports default devices. Our parent
  // class versions of the getDefault functions will return the first
  // one found.
}

bool RtApiOss :: probeDeviceInfo( RtAudio::DeviceInfo &info, oss_audioinfo &ainfo )
{
  // Probe channels
  if ( ainfo.caps & PCM_CAP_OUTPUT ) info.outputChannels = ainfo.max_channels;
  if ( ainfo.caps & PCM_CAP_INPUT ) info.inputChannels = ainfo.max_channels;
  if ( ainfo.caps & PCM_CAP_DUPLEX ) {
    if ( info.outputChannels > 0 && info.inputChannels > 0 && ainfo.caps & PCM_CAP_DUPLEX )
      info.duplexChannels = (info.outputChannels > info.inputChannels) ? info.inputChannels : info.outputChannels;
  }

  // Probe data formats ... do for input
  unsigned long mask = ainfo.iformats;
  if ( mask & AFMT_S16_LE || mask & AFMT_S16_BE )
    info.nativeFormats |= RTAUDIO_SINT16;
  if ( mask & AFMT_S8 )
    info.nativeFormats |= RTAUDIO_SINT8;
  if ( mask & AFMT_S32_LE || mask & AFMT_S32_BE )
    info.nativeFormats |= RTAUDIO_SINT32;
#ifdef AFMT_FLOAT
  if ( mask & AFMT_FLOAT )
    info.nativeFormats |= RTAUDIO_FLOAT32;
#endif
  if ( mask & AFMT_S24_LE || mask & AFMT_S24_BE )
    info.nativeFormats |= RTAUDIO_SINT24;

  // Check that we have at least one supported format
  if ( info.nativeFormats == 0 ) {
    errorStream_ << "RtApiOss::probeDeviceInfo: device (" << ainfo.name << ") data format not supported by RtAudio.";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  // Probe the supported sample rates.
  info.sampleRates.clear();
  if ( ainfo.nrates ) {
    for ( unsigned int i=0; i<ainfo.nrates; i++ ) {
      for ( unsigned int k=0; k<MAX_SAMPLE_RATES; k++ ) {
        if ( ainfo.rates[i] == SAMPLE_RATES[k] ) {
          info.sampleRates.push_back( SAMPLE_RATES[k] );

          if ( !info.preferredSampleRate || ( SAMPLE_RATES[k] <= 48000 && SAMPLE_RATES[k] > info.preferredSampleRate ) )
            info.preferredSampleRate = SAMPLE_RATES[k];

          break;
        }
      }
    }
  }
  else {
    // Check min and max rate values;
    for ( unsigned int k=0; k<MAX_SAMPLE_RATES; k++ ) {
      if ( ainfo.min_rate <= (int) SAMPLE_RATES[k] && ainfo.max_rate >= (int) SAMPLE_RATES[k] ) {
        info.sampleRates.push_back( SAMPLE_RATES[k] );

        if ( !info.preferredSampleRate || ( SAMPLE_RATES[k] <= 48000 && SAMPLE_RATES[k] > info.preferredSampleRate ) )
          info.preferredSampleRate = SAMPLE_RATES[k];
      }
    }
  }

  if ( info.sampleRates.size() == 0 ) {
    errorStream_ << "RtApiOss::probeDeviceInfo: no supported sample rates found for device (" << ainfo.name << ").";
    errorText_ = errorStream_.str();
    error( RTAUDIO_WARNING );
    return false;
  }

  return true;
}


bool RtApiOss :: probeDeviceOpen( unsigned int deviceId, StreamMode mode, unsigned int channels,
                                  unsigned int firstChannel, unsigned int sampleRate,
                                  RtAudioFormat format, unsigned int *bufferSize,
                                  RtAudio::StreamOptions *options )
{
  int mixerfd = open( "/dev/mixer", O_RDWR, 0 );
  if ( mixerfd == -1 ) {
    errorText_ = "RtApiOss::probeDeviceOpen: error opening '/dev/mixer'.";
    return FAILURE;
  }

  oss_sysinfo sysinfo;
  int result = ioctl( mixerfd, SNDCTL_SYSINFO, &sysinfo );
  if ( result == -1 ) {
    close( mixerfd );
    errorText_ = "RtApiOss::probeDeviceOpen: error getting sysinfo, OSS version >= 4.0 is required.";
    return FAILURE;
  }

  unsigned int nDevices = sysinfo.numaudios;
  if ( nDevices == 0 ) {
    // This should not happen because a check is made before this function is called.
    close( mixerfd );
    errorText_ = "RtApiOss::probeDeviceOpen: no devices found!";
    return FAILURE;
  }

  std::string deviceName;
  unsigned int m, device;
  for ( m=0; m<deviceList_.size(); m++ ) {
    if ( deviceList_[m].ID == deviceId ) {
      deviceName = deviceList_[m].name;
      break;
    }
  }

  if ( deviceName.empty() ) {
    errorText_ = "RtApiOss::probeDeviceOpen: device ID is invalid!";
    return FAILURE;
  }

  oss_audioinfo ainfo;
  for ( device=0; device<nDevices; device++ ) {
    ainfo.dev = device;
    result = ioctl( mixerfd, SNDCTL_AUDIOINFO, &ainfo );
    if ( result == -1 ) continue;
    if ( deviceName == std::string( ainfo.name ) ) break;
  }

  close( mixerfd );
  if ( device == nDevices ) {
    errorStream_ << "RtApiOss::probeDeviceOpen: device (" << ainfo.name << ") not found.";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Check if device supports input or output
  if ( ( mode == OUTPUT && !( ainfo.caps & PCM_CAP_OUTPUT ) ) ||
       ( mode == INPUT && !( ainfo.caps & PCM_CAP_INPUT ) ) ) {
    if ( mode == OUTPUT )
      errorStream_ << "RtApiOss::probeDeviceOpen: device (" << ainfo.name << ") does not support output.";
    else
      errorStream_ << "RtApiOss::probeDeviceOpen: device (" << ainfo.name << ") does not support input.";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  int flags = 0;
  OssHandle *handle = (OssHandle *) stream_.apiHandle;
  if ( mode == OUTPUT )
    flags |= O_WRONLY;
  else { // mode == INPUT
    if (stream_.mode == OUTPUT && stream_.deviceId[0] == device) {
      // We just set the same device for playback ... close and reopen for duplex (OSS only).
      close( handle->id[0] );
      handle->id[0] = 0;
      if ( !( ainfo.caps & PCM_CAP_DUPLEX ) ) {
        errorStream_ << "RtApiOss::probeDeviceOpen: device (" << ainfo.name << ") does not support duplex mode.";
        errorText_ = errorStream_.str();
        return FAILURE;
      }
      // Check that the number previously set channels is the same.
      if ( stream_.nUserChannels[0] != channels ) {
        errorStream_ << "RtApiOss::probeDeviceOpen: input/output channels must be equal for OSS duplex device (" << ainfo.name << ").";
        errorText_ = errorStream_.str();
        return FAILURE;
      }
      flags |= O_RDWR;
    }
    else
      flags |= O_RDONLY;
  }

  // Set exclusive access if specified.
  if ( options && options->flags & RTAUDIO_HOG_DEVICE ) flags |= O_EXCL;

  // Try to open the device.
  int fd;
  fd = open( ainfo.devnode, flags, 0 );
  if ( fd == -1 ) {
    if ( errno == EBUSY )
      errorStream_ << "RtApiOss::probeDeviceOpen: device (" << ainfo.name << ") is busy.";
    else
      errorStream_ << "RtApiOss::probeDeviceOpen: error opening device (" << ainfo.name << ").";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // For duplex operation, specifically set this mode (this doesn't seem to work).
  /*
    if ( flags | O_RDWR ) {
    result = ioctl( fd, SNDCTL_DSP_SETDUPLEX, NULL );
    if ( result == -1) {
    errorStream_ << "RtApiOss::probeDeviceOpen: error setting duplex mode for device (" << ainfo.name << ").";
    errorText_ = errorStream_.str();
    return FAILURE;
    }
    }
  */

  // Check the device channel support.
  stream_.nUserChannels[mode] = channels;
  if ( ainfo.max_channels < (int)(channels + firstChannel) ) {
    close( fd );
    errorStream_ << "RtApiOss::probeDeviceOpen: the device (" << ainfo.name << ") does not support requested channel parameters.";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Set the number of channels.
  int deviceChannels = channels + firstChannel;
  result = ioctl( fd, SNDCTL_DSP_CHANNELS, &deviceChannels );
  if ( result == -1 || deviceChannels < (int)(channels + firstChannel) ) {
    close( fd );
    errorStream_ << "RtApiOss::probeDeviceOpen: error setting channel parameters on device (" << ainfo.name << ").";
    errorText_ = errorStream_.str();
    return FAILURE;
  }
  stream_.nDeviceChannels[mode] = deviceChannels;

  // Get the data format mask
  int mask;
  result = ioctl( fd, SNDCTL_DSP_GETFMTS, &mask );
  if ( result == -1 ) {
    close( fd );
    errorStream_ << "RtApiOss::probeDeviceOpen: error getting device (" << ainfo.name << ") data formats.";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Determine how to set the device format.
  stream_.userFormat = format;
  int deviceFormat = -1;
  stream_.doByteSwap[mode] = false;
  if ( format == RTAUDIO_SINT8 ) {
    if ( mask & AFMT_S8 ) {
      deviceFormat = AFMT_S8;
      stream_.deviceFormat[mode] = RTAUDIO_SINT8;
    }
  }
  else if ( format == RTAUDIO_SINT16 ) {
    if ( mask & AFMT_S16_NE ) {
      deviceFormat = AFMT_S16_NE;
      stream_.deviceFormat[mode] = RTAUDIO_SINT16;
    }
    else if ( mask & AFMT_S16_OE ) {
      deviceFormat = AFMT_S16_OE;
      stream_.deviceFormat[mode] = RTAUDIO_SINT16;
      stream_.doByteSwap[mode] = true;
    }
  }
  else if ( format == RTAUDIO_SINT24 ) {
    if ( mask & AFMT_S24_NE ) {
      deviceFormat = AFMT_S24_NE;
      stream_.deviceFormat[mode] = RTAUDIO_SINT24;
    }
    else if ( mask & AFMT_S24_OE ) {
      deviceFormat = AFMT_S24_OE;
      stream_.deviceFormat[mode] = RTAUDIO_SINT24;
      stream_.doByteSwap[mode] = true;
    }
  }
  else if ( format == RTAUDIO_SINT32 ) {
    if ( mask & AFMT_S32_NE ) {
      deviceFormat = AFMT_S32_NE;
      stream_.deviceFormat[mode] = RTAUDIO_SINT32;
    }
    else if ( mask & AFMT_S32_OE ) {
      deviceFormat = AFMT_S32_OE;
      stream_.deviceFormat[mode] = RTAUDIO_SINT32;
      stream_.doByteSwap[mode] = true;
    }
  }

  if ( deviceFormat == -1 ) {
    // The user requested format is not natively supported by the device.
    if ( mask & AFMT_S16_NE ) {
      deviceFormat = AFMT_S16_NE;
      stream_.deviceFormat[mode] = RTAUDIO_SINT16;
    }
    else if ( mask & AFMT_S32_NE ) {
      deviceFormat = AFMT_S32_NE;
      stream_.deviceFormat[mode] = RTAUDIO_SINT32;
    }
    else if ( mask & AFMT_S24_NE ) {
      deviceFormat = AFMT_S24_NE;
      stream_.deviceFormat[mode] = RTAUDIO_SINT24;
    }
    else if ( mask & AFMT_S16_OE ) {
      deviceFormat = AFMT_S16_OE;
      stream_.deviceFormat[mode] = RTAUDIO_SINT16;
      stream_.doByteSwap[mode] = true;
    }
    else if ( mask & AFMT_S32_OE ) {
      deviceFormat = AFMT_S32_OE;
      stream_.deviceFormat[mode] = RTAUDIO_SINT32;
      stream_.doByteSwap[mode] = true;
    }
    else if ( mask & AFMT_S24_OE ) {
      deviceFormat = AFMT_S24_OE;
      stream_.deviceFormat[mode] = RTAUDIO_SINT24;
      stream_.doByteSwap[mode] = true;
    }
    else if ( mask & AFMT_S8) {
      deviceFormat = AFMT_S8;
      stream_.deviceFormat[mode] = RTAUDIO_SINT8;
    }
  }

  if ( stream_.deviceFormat[mode] == 0 ) {
    // This really shouldn't happen ...
    close( fd );
    errorStream_ << "RtApiOss::probeDeviceOpen: device (" << ainfo.name << ") data format not supported by RtAudio.";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Set the data format.
  int temp = deviceFormat;
  result = ioctl( fd, SNDCTL_DSP_SETFMT, &deviceFormat );
  if ( result == -1 || deviceFormat != temp ) {
    close( fd );
    errorStream_ << "RtApiOss::probeDeviceOpen: error setting data format on device (" << ainfo.name << ").";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Attempt to set the buffer size.  According to OSS, the minimum
  // number of buffers is two.  The supposed minimum buffer size is 16
  // bytes, so that will be our lower bound.  The argument to this
  // call is in the form 0xMMMMSSSS (hex), where the buffer size (in
  // bytes) is given as 2^SSSS and the number of buffers as 2^MMMM.
  // We'll check the actual value used near the end of the setup
  // procedure.
  int ossBufferBytes = *bufferSize * formatBytes( stream_.deviceFormat[mode] ) * deviceChannels;
  if ( ossBufferBytes < 16 ) ossBufferBytes = 16;
  int buffers = 0;
  if ( options ) buffers = options->numberOfBuffers;
  if ( options && options->flags & RTAUDIO_MINIMIZE_LATENCY ) buffers = 2;
  if ( buffers < 2 ) buffers = 3;
  temp = ((int) buffers << 16) + (int)( log10( (double)ossBufferBytes ) / log10( 2.0 ) );
  result = ioctl( fd, SNDCTL_DSP_SETFRAGMENT, &temp );
  if ( result == -1 ) {
    close( fd );
    errorStream_ << "RtApiOss::probeDeviceOpen: error setting buffer size on device (" << ainfo.name << ").";
    errorText_ = errorStream_.str();
    return FAILURE;
  }
  stream_.nBuffers = buffers;

  // Save buffer size (in sample frames).
  *bufferSize = ossBufferBytes / ( formatBytes(stream_.deviceFormat[mode]) * deviceChannels );
  stream_.bufferSize = *bufferSize;

  // Set the sample rate.
  int srate = sampleRate;
  result = ioctl( fd, SNDCTL_DSP_SPEED, &srate );
  if ( result == -1 ) {
    close( fd );
    errorStream_ << "RtApiOss::probeDeviceOpen: error setting sample rate (" << sampleRate << ") on device (" << ainfo.name << ").";
    errorText_ = errorStream_.str();
    return FAILURE;
  }

  // Verify the sample rate setup worked.
  if ( abs( srate - (int)sampleRate ) > 100 ) {
    close( fd );
    errorStream_ << "RtApiOss::probeDeviceOpen: device (" << ainfo.name << ") does not support sample rate (" << sampleRate << ").";
    errorText_ = errorStream_.str();
    return FAILURE;
  }
  stream_.sampleRate = sampleRate;

  if ( mode == INPUT && stream_.mode == OUTPUT && stream_.deviceId[0] == device) {
    // We're doing duplex setup here.
    stream_.deviceFormat[0] = stream_.deviceFormat[1];
    stream_.nDeviceChannels[0] = deviceChannels;
  }

  // Set interleaving parameters.
  stream_.userInterleaved = true;
  stream_.deviceInterleaved[mode] =  true;
  if ( options && options->flags & RTAUDIO_NONINTERLEAVED )
    stream_.userInterleaved = false;

  // Set flags for buffer conversion
  stream_.doConvertBuffer[mode] = false;
  if ( stream_.userFormat != stream_.deviceFormat[mode] )
    stream_.doConvertBuffer[mode] = true;
  if ( stream_.nUserChannels[mode] < stream_.nDeviceChannels[mode] )
    stream_.doConvertBuffer[mode] = true;
  if ( stream_.userInterleaved != stream_.deviceInterleaved[mode] &&
       stream_.nUserChannels[mode] > 1 )
    stream_.doConvertBuffer[mode] = true;

  // Allocate the stream handles if necessary and then save.
  if ( stream_.apiHandle == 0 ) {
    try {
      handle = new OssHandle;
    }
    catch ( std::bad_alloc& ) {
      errorText_ = "RtApiOss::probeDeviceOpen: error allocating OssHandle memory.";
      goto error;
    }

    if ( pthread_cond_init( &handle->runnable, NULL ) ) {
      errorText_ = "RtApiOss::probeDeviceOpen: error initializing pthread condition variable.";
      goto error;
    }

    stream_.apiHandle = (void *) handle;
  }
  else {
    handle = (OssHandle *) stream_.apiHandle;
  }
  handle->id[mode] = fd;

  // Allocate necessary internal buffers.
  unsigned long bufferBytes;
  bufferBytes = stream_.nUserChannels[mode] * *bufferSize * formatBytes( stream_.userFormat );
  stream_.userBuffer[mode] = (char *) calloc( bufferBytes, 1 );
  if ( stream_.userBuffer[mode] == NULL ) {
    errorText_ = "RtApiOss::probeDeviceOpen: error allocating user buffer memory.";
    goto error;
  }

  if ( stream_.doConvertBuffer[mode] ) {

    bool makeBuffer = true;
    bufferBytes = stream_.nDeviceChannels[mode] * formatBytes( stream_.deviceFormat[mode] );
    if ( mode == INPUT ) {
      if ( stream_.mode == OUTPUT && stream_.deviceBuffer ) {
        unsigned long bytesOut = stream_.nDeviceChannels[0] * formatBytes( stream_.deviceFormat[0] );
        if ( bufferBytes <= bytesOut ) makeBuffer = false;
      }
    }

    if ( makeBuffer ) {
      bufferBytes *= *bufferSize;
      if ( stream_.deviceBuffer ) free( stream_.deviceBuffer );
      stream_.deviceBuffer = (char *) calloc( bufferBytes, 1 );
      if ( stream_.deviceBuffer == NULL ) {
        errorText_ = "RtApiOss::probeDeviceOpen: error allocating device buffer memory.";
        goto error;
      }
    }
  }

  stream_.deviceId[mode] = device;
  stream_.state = STREAM_STOPPED;

  // Setup the buffer conversion information structure.
  if ( stream_.doConvertBuffer[mode] ) setConvertInfo( mode, firstChannel );

  // Setup thread if necessary.
  if ( stream_.mode == OUTPUT && mode == INPUT ) {
    // We had already set up an output stream.
    stream_.mode = DUPLEX;
    if ( stream_.deviceId[0] == device ) handle->id[0] = fd;
  }
  else {
    stream_.mode = mode;

    // Setup callback thread.
    stream_.callbackInfo.object = (void *) this;

    // Set the thread attributes for joinable and realtime scheduling
    // priority.  The higher priority will only take affect if the
    // program is run as root or suid.
    pthread_attr_t attr;
    pthread_attr_init( &attr );
    pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE );
#ifdef SCHED_RR // Undefined with some OSes (e.g. NetBSD 1.6.x with GNU Pthread)
    if ( options && options->flags & RTAUDIO_SCHEDULE_REALTIME ) {
      stream_.callbackInfo.doRealtime = true;
      struct sched_param param;
      int priority = options->priority;
      int min = sched_get_priority_min( SCHED_RR );
      int max = sched_get_priority_max( SCHED_RR );
      if ( priority < min ) priority = min;
      else if ( priority > max ) priority = max;
      param.sched_priority = priority;
      
      // Set the policy BEFORE the priority. Otherwise it fails.
      pthread_attr_setschedpolicy(&attr, SCHED_RR);
      pthread_attr_setscope (&attr, PTHREAD_SCOPE_SYSTEM);
      // This is definitely required. Otherwise it fails.
      pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
      pthread_attr_setschedparam(&attr, &param);
    }
    else
      pthread_attr_setschedpolicy( &attr, SCHED_OTHER );
#else
    pthread_attr_setschedpolicy( &attr, SCHED_OTHER );
#endif

    stream_.callbackInfo.isRunning = true;
    result = pthread_create( &stream_.callbackInfo.thread, &attr, ossCallbackHandler, &stream_.callbackInfo );
    pthread_attr_destroy( &attr );
    if ( result ) {
      // Failed. Try instead with default attributes.
      result = pthread_create( &stream_.callbackInfo.thread, NULL, ossCallbackHandler, &stream_.callbackInfo );
      if ( result ) {
        stream_.callbackInfo.isRunning = false;
        errorText_ = "RtApiOss::error creating callback thread!";
        goto error;
      }
    }
  }

  return SUCCESS;

 error:
  if ( handle ) {
    pthread_cond_destroy( &handle->runnable );
    if ( handle->id[0] ) close( handle->id[0] );
    if ( handle->id[1] ) close( handle->id[1] );
    delete handle;
    stream_.apiHandle = 0;
  }

  for ( int i=0; i<2; i++ ) {
    if ( stream_.userBuffer[i] ) {
      free( stream_.userBuffer[i] );
      stream_.userBuffer[i] = 0;
    }
  }

  if ( stream_.deviceBuffer ) {
    free( stream_.deviceBuffer );
    stream_.deviceBuffer = 0;
  }

  stream_.state = STREAM_CLOSED;
  return FAILURE;
}

void RtApiOss :: closeStream()
{
  if ( stream_.state == STREAM_CLOSED ) {
    errorText_ = "RtApiOss::closeStream(): no open stream to close!";
    error( RTAUDIO_WARNING );
    return;
  }

  OssHandle *handle = (OssHandle *) stream_.apiHandle;
  stream_.callbackInfo.isRunning = false;
  MUTEX_LOCK( &stream_.mutex );
  if ( stream_.state == STREAM_STOPPED )
    pthread_cond_signal( &handle->runnable );
  MUTEX_UNLOCK( &stream_.mutex );
  pthread_join( stream_.callbackInfo.thread, NULL );

  if ( stream_.state == STREAM_RUNNING ) {
    if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX )
      ioctl( handle->id[0], SNDCTL_DSP_HALT, 0 );
    else
      ioctl( handle->id[1], SNDCTL_DSP_HALT, 0 );
    stream_.state = STREAM_STOPPED;
  }

  if ( handle ) {
    pthread_cond_destroy( &handle->runnable );
    if ( handle->id[0] ) close( handle->id[0] );
    if ( handle->id[1] ) close( handle->id[1] );
    delete handle;
    stream_.apiHandle = 0;
  }

  for ( int i=0; i<2; i++ ) {
    if ( stream_.userBuffer[i] ) {
      free( stream_.userBuffer[i] );
      stream_.userBuffer[i] = 0;
    }
  }

  if ( stream_.deviceBuffer ) {
    free( stream_.deviceBuffer );
    stream_.deviceBuffer = 0;
  }

  clearStreamInfo();
  //stream_.mode = UNINITIALIZED;
  //stream_.state = STREAM_CLOSED;
}

RtAudioErrorType RtApiOss :: startStream()
{
  if ( stream_.state != STREAM_STOPPED ) {
    if ( stream_.state == STREAM_RUNNING )
      errorText_ = "RtApiOss::startStream(): the stream is already running!";
    else if ( stream_.state == STREAM_STOPPING || stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiOss::startStream(): the stream is stopping or closed!";
    return error( RTAUDIO_WARNING );
  }

  MUTEX_LOCK( &stream_.mutex );

  /*
  #if defined( HAVE_GETTIMEOFDAY )
  gettimeofday( &stream_.lastTickTimestamp, NULL );
  #endif
  */

  stream_.state = STREAM_RUNNING;

  // No need to do anything else here ... OSS automatically starts
  // when fed samples.

  MUTEX_UNLOCK( &stream_.mutex );

  OssHandle *handle = (OssHandle *) stream_.apiHandle;
  pthread_cond_signal( &handle->runnable );
  return RTAUDIO_NO_ERROR;
}

RtAudioErrorType RtApiOss :: stopStream()
{
  if ( stream_.state != STREAM_RUNNING && stream_.state != STREAM_STOPPING ) {
    if ( stream_.state == STREAM_STOPPED )
      errorText_ = "RtApiOss::stopStream(): the stream is already stopped!";
    else if ( stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiOss::stopStream(): the stream is closed!";
    return error( RTAUDIO_WARNING );
  }

  MUTEX_LOCK( &stream_.mutex );

  // The state might change while waiting on a mutex.
  if ( stream_.state == STREAM_STOPPED ) {
    MUTEX_UNLOCK( &stream_.mutex );
    return RTAUDIO_NO_ERROR;
  }

  int result = 0;
  OssHandle *handle = (OssHandle *) stream_.apiHandle;
  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {

    // Flush the output with zeros a few times.
    char *buffer;
    int samples;
    RtAudioFormat format;

    if ( stream_.doConvertBuffer[0] ) {
      buffer = stream_.deviceBuffer;
      samples = stream_.bufferSize * stream_.nDeviceChannels[0];
      format = stream_.deviceFormat[0];
    }
    else {
      buffer = stream_.userBuffer[0];
      samples = stream_.bufferSize * stream_.nUserChannels[0];
      format = stream_.userFormat;
    }

    memset( buffer, 0, samples * formatBytes(format) );
    for ( unsigned int i=0; i<stream_.nBuffers+1; i++ ) {
      result = write( handle->id[0], buffer, samples * formatBytes(format) );
      if ( result == -1 ) {
        errorText_ = "RtApiOss::stopStream: audio write error.";
        error( RTAUDIO_WARNING );
      }
    }

    result = ioctl( handle->id[0], SNDCTL_DSP_HALT, 0 );
    if ( result == -1 ) {
      errorStream_ << "RtApiOss::stopStream: system error stopping callback procedure on device (" << stream_.deviceId[0] << ").";
      errorText_ = errorStream_.str();
      goto unlock;
    }
    handle->triggered = false;
  }

  if ( stream_.mode == INPUT || ( stream_.mode == DUPLEX && handle->id[0] != handle->id[1] ) ) {
    result = ioctl( handle->id[1], SNDCTL_DSP_HALT, 0 );
    if ( result == -1 ) {
      errorStream_ << "RtApiOss::stopStream: system error stopping input callback procedure on device (" << stream_.deviceId[0] << ").";
      errorText_ = errorStream_.str();
      goto unlock;
    }
  }

 unlock:
  stream_.state = STREAM_STOPPED;
  MUTEX_UNLOCK( &stream_.mutex );

  if ( result != -1 ) return RTAUDIO_NO_ERROR;
  return error( RTAUDIO_SYSTEM_ERROR );
}

RtAudioErrorType RtApiOss :: abortStream()
{
  if ( stream_.state != STREAM_RUNNING ) {
    if ( stream_.state == STREAM_STOPPED )
      errorText_ = "RtApiOss::abortStream(): the stream is already stopped!";
    else if ( stream_.state == STREAM_STOPPING || stream_.state == STREAM_CLOSED )
      errorText_ = "RtApiOss::abortStream(): the stream is stopping or closed!";
    return error( RTAUDIO_WARNING );
  }

  MUTEX_LOCK( &stream_.mutex );

  // The state might change while waiting on a mutex.
  if ( stream_.state == STREAM_STOPPED ) {
    MUTEX_UNLOCK( &stream_.mutex );
    return RTAUDIO_NO_ERROR;
  }

  int result = 0;
  OssHandle *handle = (OssHandle *) stream_.apiHandle;
  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {
    result = ioctl( handle->id[0], SNDCTL_DSP_HALT, 0 );
    if ( result == -1 ) {
      errorStream_ << "RtApiOss::abortStream: system error stopping callback procedure on device (" << stream_.deviceId[0] << ").";
      errorText_ = errorStream_.str();
      goto unlock;
    }
    handle->triggered = false;
  }

  if ( stream_.mode == INPUT || ( stream_.mode == DUPLEX && handle->id[0] != handle->id[1] ) ) {
    result = ioctl( handle->id[1], SNDCTL_DSP_HALT, 0 );
    if ( result == -1 ) {
      errorStream_ << "RtApiOss::abortStream: system error stopping input callback procedure on device (" << stream_.deviceId[0] << ").";
      errorText_ = errorStream_.str();
      goto unlock;
    }
  }

 unlock:
  stream_.state = STREAM_STOPPED;
  MUTEX_UNLOCK( &stream_.mutex );

  if ( result != -1 ) return RTAUDIO_SYSTEM_ERROR;
  return error( RTAUDIO_SYSTEM_ERROR );
}

void RtApiOss :: callbackEvent()
{
  OssHandle *handle = (OssHandle *) stream_.apiHandle;
  if ( stream_.state == STREAM_STOPPED ) {
    MUTEX_LOCK( &stream_.mutex );
    pthread_cond_wait( &handle->runnable, &stream_.mutex );
    if ( stream_.state != STREAM_RUNNING ) {
      MUTEX_UNLOCK( &stream_.mutex );
      return;
    }
    MUTEX_UNLOCK( &stream_.mutex );
  }

  if ( stream_.state == STREAM_CLOSED ) {
    errorText_ = "RtApiOss::callbackEvent(): the stream is closed ... this shouldn't happen!";
    error( RTAUDIO_WARNING );
    return;
  }

  // Invoke user callback to get fresh output data.
  int doStopStream = 0;
  RtAudioCallback callback = (RtAudioCallback) stream_.callbackInfo.callback;
  double streamTime = getStreamTime();
  RtAudioStreamStatus status = 0;
  if ( stream_.mode != INPUT && handle->xrun[0] == true ) {
    status |= RTAUDIO_OUTPUT_UNDERFLOW;
    handle->xrun[0] = false;
  }
  if ( stream_.mode != OUTPUT && handle->xrun[1] == true ) {
    status |= RTAUDIO_INPUT_OVERFLOW;
    handle->xrun[1] = false;
  }
  doStopStream = callback( stream_.userBuffer[0], stream_.userBuffer[1],
                           stream_.bufferSize, streamTime, status, stream_.callbackInfo.userData );
  if ( doStopStream == 2 ) {
    this->abortStream();
    return;
  }

  MUTEX_LOCK( &stream_.mutex );

  // The state might change while waiting on a mutex.
  if ( stream_.state == STREAM_STOPPED ) goto unlock;

  int result;
  char *buffer;
  int samples;
  RtAudioFormat format;

  if ( stream_.mode == OUTPUT || stream_.mode == DUPLEX ) {

    // Setup parameters and do buffer conversion if necessary.
    if ( stream_.doConvertBuffer[0] ) {
      buffer = stream_.deviceBuffer;
      convertBuffer( buffer, stream_.userBuffer[0], stream_.convertInfo[0] );
      samples = stream_.bufferSize * stream_.nDeviceChannels[0];
      format = stream_.deviceFormat[0];
    }
    else {
      buffer = stream_.userBuffer[0];
      samples = stream_.bufferSize * stream_.nUserChannels[0];
      format = stream_.userFormat;
    }

    // Do byte swapping if necessary.
    if ( stream_.doByteSwap[0] )
      byteSwapBuffer( buffer, samples, format );

    if ( stream_.mode == DUPLEX && handle->triggered == false ) {
      int trig = 0;
      ioctl( handle->id[0], SNDCTL_DSP_SETTRIGGER, &trig );
      result = write( handle->id[0], buffer, samples * formatBytes(format) );
      trig = PCM_ENABLE_INPUT|PCM_ENABLE_OUTPUT;
      ioctl( handle->id[0], SNDCTL_DSP_SETTRIGGER, &trig );
      handle->triggered = true;
    }
    else
      // Write samples to device.
      result = write( handle->id[0], buffer, samples * formatBytes(format) );

    if ( result == -1 ) {
      // We'll assume this is an underrun, though there isn't a
      // specific means for determining that.
      handle->xrun[0] = true;
      errorText_ = "RtApiOss::callbackEvent: audio write error.";
      error( RTAUDIO_WARNING );
      // Continue on to input section.
    }
  }

  if ( stream_.mode == INPUT || stream_.mode == DUPLEX ) {

    // Setup parameters.
    if ( stream_.doConvertBuffer[1] ) {
      buffer = stream_.deviceBuffer;
      samples = stream_.bufferSize * stream_.nDeviceChannels[1];
      format = stream_.deviceFormat[1];
    }
    else {
      buffer = stream_.userBuffer[1];
      samples = stream_.bufferSize * stream_.nUserChannels[1];
      format = stream_.userFormat;
    }

    // Read samples from device.
    result = read( handle->id[1], buffer, samples * formatBytes(format) );

    if ( result == -1 ) {
      // We'll assume this is an overrun, though there isn't a
      // specific means for determining that.
      handle->xrun[1] = true;
      errorText_ = "RtApiOss::callbackEvent: audio read error.";
      error( RTAUDIO_WARNING );
      goto unlock;
    }

    // Do byte swapping if necessary.
    if ( stream_.doByteSwap[1] )
      byteSwapBuffer( buffer, samples, format );

    // Do buffer conversion if necessary.
    if ( stream_.doConvertBuffer[1] )
      convertBuffer( stream_.userBuffer[1], stream_.deviceBuffer, stream_.convertInfo[1] );
  }

 unlock:
  MUTEX_UNLOCK( &stream_.mutex );

  RtApi::tickStreamTime();
  if ( doStopStream == 1 ) this->stopStream();
}

static void *ossCallbackHandler( void *ptr )
{
  CallbackInfo *info = (CallbackInfo *) ptr;
  RtApiOss *object = (RtApiOss *) info->object;
  bool *isRunning = &info->isRunning;

#ifdef SCHED_RR // Undefined with some OSes (e.g. NetBSD 1.6.x with GNU Pthread)
  if (info->doRealtime) {
    std::cerr << "RtAudio oss: " << 
             (sched_getscheduler(0) == SCHED_RR ? "" : "_NOT_ ") << 
             "running realtime scheduling" << std::endl;
  }
#endif

  while ( *isRunning == true ) {
    pthread_testcancel();
    object->callbackEvent();
  }

  pthread_exit( NULL );
}

//******************** End of __LINUX_OSS__ *********************//
#endif


// *************************************************** //
//
// Protected common (OS-independent) RtAudio methods.
//
// *************************************************** //

// This method can be modified to control the behavior of error
// message printing.
RtAudioErrorType RtApi :: error( RtAudioErrorType type )
{
  errorStream_.str(""); // clear the ostringstream to avoid repeated messages

  // Don't output warnings if showWarnings_ is false
  if ( type == RTAUDIO_WARNING && showWarnings_ == false ) return type;
  
  if ( errorCallback_ ) {
    //const std::string errorMessage = errorText_;
    //errorCallback_( type, errorMessage );
    errorCallback_( type, errorText_ );
  }
  else
    std::cerr << '\n' << errorText_ << "\n\n";
  return type;
}

/*
void RtApi :: verifyStream()
{
  if ( stream_.state == STREAM_CLOSED ) {
    errorText_ = "RtApi:: a stream is not open!";
    error( RtAudioError::INVALID_USE );
  }
}
*/

void RtApi :: clearStreamInfo()
{
  stream_.mode = UNINITIALIZED;
  stream_.state = STREAM_CLOSED;
  stream_.sampleRate = 0;
  stream_.bufferSize = 0;
  stream_.nBuffers = 0;
  stream_.userFormat = 0;
  stream_.userInterleaved = true;
  stream_.streamTime = 0.0;
  stream_.apiHandle = 0;
  stream_.deviceBuffer = 0;
  stream_.callbackInfo.callback = 0;
  stream_.callbackInfo.userData = 0;
  stream_.callbackInfo.isRunning = false;
  stream_.callbackInfo.deviceDisconnected = false;
  for ( int i=0; i<2; i++ ) {
    stream_.deviceId[i] = 11111;
    stream_.doConvertBuffer[i] = false;
    stream_.deviceInterleaved[i] = true;
    stream_.doByteSwap[i] = false;
    stream_.nUserChannels[i] = 0;
    stream_.nDeviceChannels[i] = 0;
    stream_.channelOffset[i] = 0;
    stream_.deviceFormat[i] = 0;
    stream_.latency[i] = 0;
    stream_.userBuffer[i] = 0;
    stream_.convertInfo[i].channels = 0;
    stream_.convertInfo[i].inJump = 0;
    stream_.convertInfo[i].outJump = 0;
    stream_.convertInfo[i].inFormat = 0;
    stream_.convertInfo[i].outFormat = 0;
    stream_.convertInfo[i].inOffset.clear();
    stream_.convertInfo[i].outOffset.clear();
  }
}

unsigned int RtApi :: formatBytes( RtAudioFormat format )
{
  if ( format == RTAUDIO_SINT16 )
    return 2;
  else if ( format == RTAUDIO_SINT32 || format == RTAUDIO_FLOAT32 )
    return 4;
  else if ( format == RTAUDIO_FLOAT64 )
    return 8;
  else if ( format == RTAUDIO_SINT24 )
    return 3;
  else if ( format == RTAUDIO_SINT8 )
    return 1;

  errorText_ = "RtApi::formatBytes: undefined format.";
  error( RTAUDIO_WARNING );

  return 0;
}

void RtApi :: setConvertInfo( StreamMode mode, unsigned int firstChannel )
{
  if ( mode == INPUT ) { // convert device to user buffer
    stream_.convertInfo[mode].inJump = stream_.nDeviceChannels[1];
    stream_.convertInfo[mode].outJump = stream_.nUserChannels[1];
    stream_.convertInfo[mode].inFormat = stream_.deviceFormat[1];
    stream_.convertInfo[mode].outFormat = stream_.userFormat;
  }
  else { // convert user to device buffer
    stream_.convertInfo[mode].inJump = stream_.nUserChannels[0];
    stream_.convertInfo[mode].outJump = stream_.nDeviceChannels[0];
    stream_.convertInfo[mode].inFormat = stream_.userFormat;
    stream_.convertInfo[mode].outFormat = stream_.deviceFormat[0];
  }

  if ( stream_.convertInfo[mode].inJump < stream_.convertInfo[mode].outJump )
    stream_.convertInfo[mode].channels = stream_.convertInfo[mode].inJump;
  else
    stream_.convertInfo[mode].channels = stream_.convertInfo[mode].outJump;

  // Set up the interleave/deinterleave offsets.
  if ( stream_.deviceInterleaved[mode] != stream_.userInterleaved ) {
    if ( ( mode == OUTPUT && stream_.deviceInterleaved[mode] ) ||
         ( mode == INPUT && stream_.userInterleaved ) ) {
      for ( int k=0; k<stream_.convertInfo[mode].channels; k++ ) {
        stream_.convertInfo[mode].inOffset.push_back( k * stream_.bufferSize );
        stream_.convertInfo[mode].outOffset.push_back( k );
        stream_.convertInfo[mode].inJump = 1;
      }
    }
    else {
      for ( int k=0; k<stream_.convertInfo[mode].channels; k++ ) {
        stream_.convertInfo[mode].inOffset.push_back( k );
        stream_.convertInfo[mode].outOffset.push_back( k * stream_.bufferSize );
        stream_.convertInfo[mode].outJump = 1;
      }
    }
  }
  else { // no (de)interleaving
    if ( stream_.userInterleaved ) {
      for ( int k=0; k<stream_.convertInfo[mode].channels; k++ ) {
        stream_.convertInfo[mode].inOffset.push_back( k );
        stream_.convertInfo[mode].outOffset.push_back( k );
      }
    }
    else {
      for ( int k=0; k<stream_.convertInfo[mode].channels; k++ ) {
        stream_.convertInfo[mode].inOffset.push_back( k * stream_.bufferSize );
        stream_.convertInfo[mode].outOffset.push_back( k * stream_.bufferSize );
        stream_.convertInfo[mode].inJump = 1;
        stream_.convertInfo[mode].outJump = 1;
      }
    }
  }

  // Add channel offset.
  if ( firstChannel > 0 ) {
    if ( stream_.deviceInterleaved[mode] ) {
      if ( mode == OUTPUT ) {
        for ( int k=0; k<stream_.convertInfo[mode].channels; k++ )
          stream_.convertInfo[mode].outOffset[k] += firstChannel;
      }
      else {
        for ( int k=0; k<stream_.convertInfo[mode].channels; k++ )
          stream_.convertInfo[mode].inOffset[k] += firstChannel;
      }
    }
    else {
      if ( mode == OUTPUT ) {
        for ( int k=0; k<stream_.convertInfo[mode].channels; k++ )
          stream_.convertInfo[mode].outOffset[k] += ( firstChannel * stream_.bufferSize );
      }
      else {
        for ( int k=0; k<stream_.convertInfo[mode].channels; k++ )
          stream_.convertInfo[mode].inOffset[k] += ( firstChannel  * stream_.bufferSize );
      }
    }
  }
}

void RtApi :: convertBuffer( char *outBuffer, char *inBuffer, ConvertInfo &info )
{
  // This function does format conversion, input/output channel compensation, and
  // data interleaving/deinterleaving.  24-bit integers are assumed to occupy
  // the lower three bytes of a 32-bit integer.

  // Clear our duplex device output buffer if there are more device outputs than user outputs
  if ( outBuffer == stream_.deviceBuffer && stream_.mode == DUPLEX && info.outJump > info.inJump )
    memset( outBuffer, 0, stream_.bufferSize * info.outJump * formatBytes( info.outFormat ) );

  int j;
  if (info.outFormat == RTAUDIO_FLOAT64) {
    Float64 *out = (Float64 *)outBuffer;

    if (info.inFormat == RTAUDIO_SINT8) {
      signed char *in = (signed char *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Float64) in[info.inOffset[j]] / 128.0;
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_SINT16) {
      Int16 *in = (Int16 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Float64) in[info.inOffset[j]] / 32768.0;
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_SINT24) {
      Int24 *in = (Int24 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Float64) in[info.inOffset[j]].asInt() / 8388608.0;
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_SINT32) {
      Int32 *in = (Int32 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Float64) in[info.inOffset[j]] / 2147483648.0;
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_FLOAT32) {
      Float32 *in = (Float32 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Float64) in[info.inOffset[j]];
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_FLOAT64) {
      // Channel compensation and/or (de)interleaving only.
      Float64 *in = (Float64 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = in[info.inOffset[j]];
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
  }
  else if (info.outFormat == RTAUDIO_FLOAT32) {
    Float32 *out = (Float32 *)outBuffer;

    if (info.inFormat == RTAUDIO_SINT8) {
      signed char *in = (signed char *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Float32) in[info.inOffset[j]] / 128.f;
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_SINT16) {
      Int16 *in = (Int16 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Float32) in[info.inOffset[j]] / 32768.f;
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_SINT24) {
      Int24 *in = (Int24 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Float32) in[info.inOffset[j]].asInt() / 8388608.f;
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_SINT32) {
      Int32 *in = (Int32 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Float32) in[info.inOffset[j]] / 2147483648.f;
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_FLOAT32) {
      // Channel compensation and/or (de)interleaving only.
      Float32 *in = (Float32 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = in[info.inOffset[j]];
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_FLOAT64) {
      Float64 *in = (Float64 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Float32) in[info.inOffset[j]];
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
  }
  else if (info.outFormat == RTAUDIO_SINT32) {
    Int32 *out = (Int32 *)outBuffer;
    if (info.inFormat == RTAUDIO_SINT8) {
      signed char *in = (signed char *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Int32) in[info.inOffset[j]];
          out[info.outOffset[j]] <<= 24;
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_SINT16) {
      Int16 *in = (Int16 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Int32) in[info.inOffset[j]];
          out[info.outOffset[j]] <<= 16;
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_SINT24) {
      Int24 *in = (Int24 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Int32) in[info.inOffset[j]].asInt();
          out[info.outOffset[j]] <<= 8;
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_SINT32) {
      // Channel compensation and/or (de)interleaving only.
      Int32 *in = (Int32 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = in[info.inOffset[j]];
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_FLOAT32) {
      Float32 *in = (Float32 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          // Use llround() which returns `long long` which is guaranteed to be at least 64 bits.
          out[info.outOffset[j]] = (Int32) std::max(std::min(std::llround(in[info.inOffset[j]] * 2147483648.f), 2147483647LL), -2147483648LL);
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_FLOAT64) {
      Float64 *in = (Float64 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Int32) std::max(std::min(std::llround(in[info.inOffset[j]] * 2147483648.0), 2147483647LL), -2147483648LL);
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
  }
  else if (info.outFormat == RTAUDIO_SINT24) {
    Int24 *out = (Int24 *)outBuffer;
    if (info.inFormat == RTAUDIO_SINT8) {
      signed char *in = (signed char *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Int32) (in[info.inOffset[j]] << 16);
          //out[info.outOffset[j]] <<= 16;
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_SINT16) {
      Int16 *in = (Int16 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Int32) (in[info.inOffset[j]] << 8);
          //out[info.outOffset[j]] <<= 8;
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_SINT24) {
      // Channel compensation and/or (de)interleaving only.
      Int24 *in = (Int24 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = in[info.inOffset[j]];
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_SINT32) {
      Int32 *in = (Int32 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Int32) (in[info.inOffset[j]] >> 8);
          //out[info.outOffset[j]] >>= 8;
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_FLOAT32) {
      Float32 *in = (Float32 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Int32) std::max(std::min(std::llround(in[info.inOffset[j]] * 8388608.f), 8388607LL), -8388608LL);
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_FLOAT64) {
      Float64 *in = (Float64 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Int32) std::max(std::min(std::llround(in[info.inOffset[j]] * 8388608.0), 8388607LL), -8388608LL);
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
  }
  else if (info.outFormat == RTAUDIO_SINT16) {
    Int16 *out = (Int16 *)outBuffer;
    if (info.inFormat == RTAUDIO_SINT8) {
      signed char *in = (signed char *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Int16) in[info.inOffset[j]];
          out[info.outOffset[j]] <<= 8;
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_SINT16) {
      // Channel compensation and/or (de)interleaving only.
      Int16 *in = (Int16 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = in[info.inOffset[j]];
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_SINT24) {
      Int24 *in = (Int24 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Int16) (in[info.inOffset[j]].asInt() >> 8);
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_SINT32) {
      Int32 *in = (Int32 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Int16) ((in[info.inOffset[j]] >> 16) & 0x0000ffff);
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_FLOAT32) {
      Float32 *in = (Float32 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Int16) std::max(std::min(std::llround(in[info.inOffset[j]] * 32768.f), 32767LL), -32768LL);
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_FLOAT64) {
      Float64 *in = (Float64 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (Int16) std::max(std::min(std::llround(in[info.inOffset[j]] * 32768.0), 32767LL), -32768LL);
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
  }
  else if (info.outFormat == RTAUDIO_SINT8) {
    signed char *out = (signed char *)outBuffer;
    if (info.inFormat == RTAUDIO_SINT8) {
      // Channel compensation and/or (de)interleaving only.
      signed char *in = (signed char *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = in[info.inOffset[j]];
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    if (info.inFormat == RTAUDIO_SINT16) {
      Int16 *in = (Int16 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (signed char) ((in[info.inOffset[j]] >> 8) & 0x00ff);
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_SINT24) {
      Int24 *in = (Int24 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (signed char) (in[info.inOffset[j]].asInt() >> 16);
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_SINT32) {
      Int32 *in = (Int32 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (signed char) ((in[info.inOffset[j]] >> 24) & 0x000000ff);
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_FLOAT32) {
      Float32 *in = (Float32 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (signed char) std::max(std::min(std::llround(in[info.inOffset[j]] * 128.f), 127LL), -128LL);
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
    else if (info.inFormat == RTAUDIO_FLOAT64) {
      Float64 *in = (Float64 *)inBuffer;
      for (unsigned int i=0; i<stream_.bufferSize; i++) {
        for (j=0; j<info.channels; j++) {
          out[info.outOffset[j]] = (signed char) std::max(std::min(std::llround(in[info.inOffset[j]] * 128.0), 127LL), -128LL);
        }
        in += info.inJump;
        out += info.outJump;
      }
    }
  }
}

//static inline uint16_t bswap_16(uint16_t x) { return (x>>8) | (x<<8); }
//static inline uint32_t bswap_32(uint32_t x) { return (bswap_16(x&0xffff)<<16) | (bswap_16(x>>16)); }
//static inline uint64_t bswap_64(uint64_t x) { return (((unsigned long long)bswap_32(x&0xffffffffull))<<32) | (bswap_32(x>>32)); }

void RtApi :: byteSwapBuffer( char *buffer, unsigned int samples, RtAudioFormat format )
{
  char val;
  char *ptr;

  ptr = buffer;
  if ( format == RTAUDIO_SINT16 ) {
    for ( unsigned int i=0; i<samples; i++ ) {
      // Swap 1st and 2nd bytes.
      val = *(ptr);
      *(ptr) = *(ptr+1);
      *(ptr+1) = val;

      // Increment 2 bytes.
      ptr += 2;
    }
  }
  else if ( format == RTAUDIO_SINT32 ||
            format == RTAUDIO_FLOAT32 ) {
    for ( unsigned int i=0; i<samples; i++ ) {
      // Swap 1st and 4th bytes.
      val = *(ptr);
      *(ptr) = *(ptr+3);
      *(ptr+3) = val;

      // Swap 2nd and 3rd bytes.
      ptr += 1;
      val = *(ptr);
      *(ptr) = *(ptr+1);
      *(ptr+1) = val;

      // Increment 3 more bytes.
      ptr += 3;
    }
  }
  else if ( format == RTAUDIO_SINT24 ) {
    for ( unsigned int i=0; i<samples; i++ ) {
      // Swap 1st and 3rd bytes.
      val = *(ptr);
      *(ptr) = *(ptr+2);
      *(ptr+2) = val;

      // Increment 2 more bytes.
      ptr += 2;
    }
  }
  else if ( format == RTAUDIO_FLOAT64 ) {
    for ( unsigned int i=0; i<samples; i++ ) {
      // Swap 1st and 8th bytes
      val = *(ptr);
      *(ptr) = *(ptr+7);
      *(ptr+7) = val;

      // Swap 2nd and 7th bytes
      ptr += 1;
      val = *(ptr);
      *(ptr) = *(ptr+5);
      *(ptr+5) = val;

      // Swap 3rd and 6th bytes
      ptr += 1;
      val = *(ptr);
      *(ptr) = *(ptr+3);
      *(ptr+3) = val;

      // Swap 4th and 5th bytes
      ptr += 1;
      val = *(ptr);
      *(ptr) = *(ptr+1);
      *(ptr+1) = val;

      // Increment 5 more bytes.
      ptr += 5;
    }
  }
}

  // Indentation settings for Vim and Emacs
  //
  // Local Variables:
  // c-basic-offset: 2
  // indent-tabs-mode: nil
  // End:
  //
  // vim: et sts=2 sw=2

