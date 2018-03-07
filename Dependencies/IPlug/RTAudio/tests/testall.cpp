/******************************************/
/*
  testall.cpp
  by Gary P. Scavone, 2007-2008

  This program will make a variety of calls
  to extensively test RtAudio functionality.
*/
/******************************************/

#include "RtAudio.h"
#include <iostream>
#include <cstdlib>
#include <cstring>

#define BASE_RATE 0.005
#define TIME   1.0

void usage( void ) {
  // Error function in case of incorrect command-line
  // argument specifications
  std::cout << "\nuseage: testall N fs <iDevice> <oDevice> <iChannelOffset> <oChannelOffset>\n";
  std::cout << "    where N = number of channels,\n";
  std::cout << "    fs = the sample rate,\n";
  std::cout << "    iDevice = optional input device to use (default = 0),\n";
  std::cout << "    oDevice = optional output device to use (default = 0),\n";
  std::cout << "    iChannelOffset = an optional input channel offset (default = 0),\n";
  std::cout << "    and oChannelOffset = optional output channel offset (default = 0).\n\n";
  exit( 0 );
}

unsigned int channels;

// Interleaved buffers
int sawi( void *outputBuffer, void * /*inputBuffer*/, unsigned int nBufferFrames,
          double /*streamTime*/, RtAudioStreamStatus status, void *data )
{
  unsigned int i, j;
  extern unsigned int channels;
  double *buffer = (double *) outputBuffer;
  double *lastValues = (double *) data;

  if ( status )
    std::cout << "Stream underflow detected!" << std::endl;

  for ( i=0; i<nBufferFrames; i++ ) {
    for ( j=0; j<channels; j++ ) {
      *buffer++ = (double) lastValues[j];
      lastValues[j] += BASE_RATE * (j+1+(j*0.1));
      if ( lastValues[j] >= 1.0 ) lastValues[j] -= 2.0;
    }
  }

  return 0;
}

// Non-interleaved buffers
int sawni( void *outputBuffer, void * /*inputBuffer*/, unsigned int nBufferFrames,
           double /*streamTime*/, RtAudioStreamStatus status, void *data )
{
  unsigned int i, j;
  extern unsigned int channels;
  double *buffer = (double *) outputBuffer;
  double *lastValues = (double *) data;

  if ( status )
    std::cout << "Stream underflow detected!" << std::endl;

  double increment;
  for ( j=0; j<channels; j++ ) {
    increment = BASE_RATE * (j+1+(j*0.1));
    for ( i=0; i<nBufferFrames; i++ ) {
      *buffer++ = (double) lastValues[j];
      lastValues[j] += increment;
      if ( lastValues[j] >= 1.0 ) lastValues[j] -= 2.0;
    }
  }

  return 0;
}

int inout( void *outputBuffer, void *inputBuffer, unsigned int /*nBufferFrames*/,
           double /*streamTime*/, RtAudioStreamStatus status, void *data )
{
  // Since the number of input and output channels is equal, we can do
  // a simple buffer copy operation here.
  if ( status ) std::cout << "Stream over/underflow detected." << std::endl;

  unsigned int *bytes = (unsigned int *) data;
  memcpy( outputBuffer, inputBuffer, *bytes );
  return 0;
}

int main( int argc, char *argv[] )
{
  unsigned int bufferFrames, fs, oDevice = 0, iDevice = 0, iOffset = 0, oOffset = 0;
  char input;

  // minimal command-line checking
  if (argc < 3 || argc > 7 ) usage();

  RtAudio dac;
  if ( dac.getDeviceCount() < 1 ) {
    std::cout << "\nNo audio devices found!\n";
    exit( 1 );
  }

  channels = (unsigned int) atoi( argv[1] );
  fs = (unsigned int) atoi( argv[2] );
  if ( argc > 3 )
    iDevice = (unsigned int) atoi( argv[3] );
  if ( argc > 4 )
    oDevice = (unsigned int) atoi(argv[4]);
  if ( argc > 5 )
    iOffset = (unsigned int) atoi(argv[5]);
  if ( argc > 6 )
    oOffset = (unsigned int) atoi(argv[6]);

  double *data = (double *) calloc( channels, sizeof( double ) );

  // Let RtAudio print messages to stderr.
  dac.showWarnings( true );

  // Set our stream parameters for output only.
  bufferFrames = 512;
  RtAudio::StreamParameters oParams, iParams;
  oParams.deviceId = oDevice;
  oParams.nChannels = channels;
  oParams.firstChannel = oOffset;

  if ( oDevice == 0 )
    oParams.deviceId = dac.getDefaultOutputDevice();

  RtAudio::StreamOptions options;
  options.flags = RTAUDIO_HOG_DEVICE;
  try {
    dac.openStream( &oParams, NULL, RTAUDIO_FLOAT64, fs, &bufferFrames, &sawi, (void *)data, &options );
    std::cout << "\nStream latency = " << dac.getStreamLatency() << std::endl;

    // Start the stream
    dac.startStream();
    std::cout << "\nPlaying ... press <enter> to stop.\n";
    std::cin.get( input );

    // Stop the stream
    dac.stopStream();

    // Restart again
    std::cout << "Press <enter> to restart.\n";
    std::cin.get( input );
    dac.startStream();

    // Test abort function
    std::cout << "Playing again ... press <enter> to abort.\n";
    std::cin.get( input );
    dac.abortStream();

    // Restart another time
    std::cout << "Press <enter> to restart again.\n";
    std::cin.get( input );
    dac.startStream();

    std::cout << "Playing again ... press <enter> to close the stream.\n";
    std::cin.get( input );
  }
  catch ( RtAudioError& e ) {
    e.printMessage();
    goto cleanup;
  }

  if ( dac.isStreamOpen() ) dac.closeStream();

  // Test non-interleaved functionality
  options.flags = RTAUDIO_NONINTERLEAVED;
  try {
    dac.openStream( &oParams, NULL, RTAUDIO_FLOAT64, fs, &bufferFrames, &sawni, (void *)data, &options );

    std::cout << "Press <enter> to start non-interleaved playback.\n";
    std::cin.get( input );

    // Start the stream
    dac.startStream();
    std::cout << "\nPlaying ... press <enter> to stop.\n";
    std::cin.get( input );
  }
  catch ( RtAudioError& e ) {
    e.printMessage();
    goto cleanup;
  }

  if ( dac.isStreamOpen() ) dac.closeStream();

  // Now open a duplex stream.
  unsigned int bufferBytes;
  iParams.deviceId = iDevice;
  iParams.nChannels = channels;
  iParams.firstChannel = iOffset;
  if ( iDevice == 0 )
    iParams.deviceId = dac.getDefaultInputDevice();
  options.flags = RTAUDIO_NONINTERLEAVED;
  try {
    dac.openStream( &oParams, &iParams, RTAUDIO_SINT32, fs, &bufferFrames, &inout, (void *)&bufferBytes, &options );

    bufferBytes = bufferFrames * channels * 4;

    std::cout << "Press <enter> to start duplex operation.\n";
    std::cin.get( input );

    // Start the stream
    dac.startStream();
    std::cout << "\nRunning ... press <enter> to stop.\n";
    std::cin.get( input );

    // Stop the stream
    dac.stopStream();
    std::cout << "\nStopped ... press <enter> to restart.\n";
    std::cin.get( input );

    // Restart the stream
    dac.startStream();
    std::cout << "\nRunning ... press <enter> to stop.\n";
    std::cin.get( input );
  }
  catch ( RtAudioError& e ) {
    e.printMessage();
  }

 cleanup:
  if ( dac.isStreamOpen() ) dac.closeStream();
  free( data );

  return 0;
}
