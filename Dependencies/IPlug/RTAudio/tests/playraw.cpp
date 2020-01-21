/******************************************/
/*
  playraw.cpp
  by Gary P. Scavone, 2007

  Play a specified raw file.  It is necessary
  that the file be of the same data format as
  defined below.
*/
/******************************************/

#include "RtAudio.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <stdio.h>

/*
typedef char  MY_TYPE;
#define FORMAT RTAUDIO_SINT8
#define SCALE  127.0
*/

typedef signed short  MY_TYPE;
#define FORMAT RTAUDIO_SINT16
#define SCALE  32767.0

/*
typedef S24 MY_TYPE;
#define FORMAT RTAUDIO_SINT24
#define SCALE  8388607.0

typedef signed int  MY_TYPE;
#define FORMAT RTAUDIO_SINT32
#define SCALE  2147483647.0

typedef float  MY_TYPE;
#define FORMAT RTAUDIO_FLOAT32
#define SCALE  1.0;

typedef double  MY_TYPE;
#define FORMAT RTAUDIO_FLOAT64
#define SCALE  1.0;
*/

// Platform-dependent sleep routines.
#if defined( WIN32 )
  #include <windows.h>
  #define SLEEP( milliseconds ) Sleep( (DWORD) milliseconds ) 
#else // Unix variants
  #include <unistd.h>
  #define SLEEP( milliseconds ) usleep( (unsigned long) (milliseconds * 1000.0) )
#endif

void usage( void ) {
  // Error function in case of incorrect command-line
  // argument specifications
  std::cout << "\nuseage: playraw N fs file <device> <channelOffset>\n";
  std::cout << "    where N = number of channels,\n";
  std::cout << "    fs = the sample rate, \n";
  std::cout << "    file = the raw file to play,\n";
  std::cout << "    device = optional device to use (default = 0),\n";
  std::cout << "    and channelOffset = an optional channel offset on the device (default = 0).\n\n";
  exit( 0 );
}

struct OutputData {
  FILE *fd;
  unsigned int channels;
};

// Interleaved buffers
int output( void *outputBuffer, void * /*inputBuffer*/, unsigned int nBufferFrames,
            double /*streamTime*/, RtAudioStreamStatus /*status*/, void *data )
{
  OutputData *oData = (OutputData*) data;

  // In general, it's not a good idea to do file input in the audio
  // callback function but I'm doing it here because I don't know the
  // length of the file we are reading.
  unsigned int count = fread( outputBuffer, oData->channels * sizeof( MY_TYPE ), nBufferFrames, oData->fd);
  if ( count < nBufferFrames ) {
    unsigned int bytes = (nBufferFrames - count) * oData->channels * sizeof( MY_TYPE );
    unsigned int startByte = count * oData->channels * sizeof( MY_TYPE );
    memset( (char *)(outputBuffer)+startByte, 0, bytes );
    return 1;
  }

  return 0;
}

int main( int argc, char *argv[] )
{
  unsigned int channels, fs, bufferFrames, device = 0, offset = 0;
  char *file;

  // minimal command-line checking
  if ( argc < 4 || argc > 6 ) usage();

  RtAudio dac;
  if ( dac.getDeviceCount() < 1 ) {
    std::cout << "\nNo audio devices found!\n";
    exit( 0 );
  }

  channels = (unsigned int) atoi( argv[1]) ;
  fs = (unsigned int) atoi( argv[2] );
  file = argv[3];
  if ( argc > 4 )
    device = (unsigned int) atoi( argv[4] );
  if ( argc > 5 )
    offset = (unsigned int) atoi( argv[5] );

  OutputData data;
  data.fd = fopen( file, "rb" );
  if ( !data.fd ) {
    std::cout << "Unable to find or open file!\n";
    exit( 1 );
  }

  // Set our stream parameters for output only.
  bufferFrames = 512;
  RtAudio::StreamParameters oParams;
  oParams.deviceId = device;
  oParams.nChannels = channels;
  oParams.firstChannel = offset;

  if ( device == 0 )
    oParams.deviceId = dac.getDefaultOutputDevice();

  data.channels = channels;
  try {
    dac.openStream( &oParams, NULL, FORMAT, fs, &bufferFrames, &output, (void *)&data );
    dac.startStream();
  }
  catch ( RtAudioError& e ) {
    std::cout << '\n' << e.getMessage() << '\n' << std::endl;
    goto cleanup;
  }

  std::cout << "\nPlaying raw file " << file << " (buffer frames = " << bufferFrames << ")." << std::endl;
  while ( 1 ) {
    SLEEP( 100 ); // wake every 100 ms to check if we're done
    if ( dac.isStreamRunning() == false ) break;
  }

 cleanup:
  fclose( data.fd );
  dac.closeStream();

  return 0;
}
