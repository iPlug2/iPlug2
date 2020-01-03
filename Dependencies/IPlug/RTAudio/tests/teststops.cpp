/******************************************/
/*
  teststop.cpp
  by Gary P. Scavone, 2011

  This program starts and stops an RtAudio
  stream many times in succession and in
  different ways to to test its functionality.
*/
/******************************************/

#include "RtAudio.h"
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cstdio>

#define PULSE_RATE 0.01  // seconds
#define RUNTIME    0.4   // seconds
#define PAUSETIME  0.1   // seconds
#define REPETITIONS 10

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
  std::cout << "\nuseage: teststops N fs <iDevice> <oDevice> <iChannelOffset> <oChannelOffset>\n";
  std::cout << "    where N = number of channels,\n";
  std::cout << "    fs = the sample rate,\n";
  std::cout << "    iDevice = optional input device to use (default = 0),\n";
  std::cout << "    oDevice = optional output device to use (default = 0),\n";
  std::cout << "    iChannelOffset = an optional input channel offset (default = 0),\n";
  std::cout << "    and oChannelOffset = optional output channel offset (default = 0).\n\n";
  exit( 0 );
}

struct MyData {
  unsigned int channels;
  unsigned int pulseCount;
  unsigned int frameCounter;
  unsigned int nFrames;
  unsigned int returnValue;
};

// Interleaved buffers
int pulse( void *outputBuffer, void * /*inputBuffer*/, unsigned int nBufferFrames,
           double /*streamTime*/, RtAudioStreamStatus status, void *mydata )
{
  // Write out a pulse signal and ignore the input buffer.
  unsigned int i, j;
  float sample;
  float *buffer = (float *) outputBuffer;
  MyData *data = (MyData *) mydata;

  if ( status ) std::cout << "Stream over/underflow detected!" << std::endl;

  for ( i=0; i<nBufferFrames; i++ ) {
    if ( data->frameCounter % data->pulseCount == 0 ) sample = 0.9f;
    else sample = 0.0;
    for ( j=0; j<data->channels; j++ )
      *buffer++ = sample;

    data->frameCounter++;
  }

  if ( data->frameCounter >= data->nFrames )
    return data->returnValue;
  else
    return 0;
}

int main( int argc, char *argv[] )
{
  unsigned int bufferFrames, fs, oDevice = 0, iDevice = 0, iOffset = 0, oOffset = 0;
  unsigned int runtime, pausetime;
  char input;

  // minimal command-line checking
  if (argc < 3 || argc > 7 ) usage();

  RtAudio *adc = new RtAudio();
  if ( adc->getDeviceCount() < 1 ) {
    std::cout << "\nNo audio devices found!\n";
    exit( 1 );
  }

  MyData mydata;
  mydata.channels = (unsigned int) atoi( argv[1] );
  fs = (unsigned int) atoi( argv[2] );
  if ( argc > 3 )
    iDevice = (unsigned int) atoi( argv[3] );
  if ( argc > 4 )
    oDevice = (unsigned int) atoi(argv[4]);
  if ( argc > 5 )
    iOffset = (unsigned int) atoi(argv[5]);
  if ( argc > 6 )
    oOffset = (unsigned int) atoi(argv[6]);

  // Let RtAudio print messages to stderr.
  adc->showWarnings( true );

  runtime = static_cast<unsigned int>(RUNTIME * 1000);
  pausetime = static_cast<unsigned int>(PAUSETIME * 1000);

  // Set our stream parameters for a duplex stream.
  bufferFrames = 512;
  RtAudio::StreamParameters oParams, iParams;
  oParams.deviceId = oDevice;
  oParams.nChannels = mydata.channels;
  oParams.firstChannel = oOffset;

  iParams.deviceId = iDevice;
  iParams.nChannels = mydata.channels;
  iParams.firstChannel = iOffset;

  if ( iDevice == 0 )
    iParams.deviceId = adc->getDefaultInputDevice();
  if ( oDevice == 0 )
    oParams.deviceId = adc->getDefaultOutputDevice();

  // First, test external stopStream() calls.
  mydata.pulseCount = static_cast<unsigned int>(PULSE_RATE * fs);
  mydata.nFrames = 50 * fs;
  mydata.returnValue = 0;
  try {
    adc->openStream( &oParams, &iParams, RTAUDIO_SINT32, fs, &bufferFrames, &pulse, (void *)&mydata );

    std::cout << "Press <enter> to start test.\n";
    std::cin.get( input );

    for (int i=0; i<REPETITIONS; i++ ) {
      mydata.frameCounter = 0;
      adc->startStream();
      std::cout << "Stream started ... ";
      SLEEP( runtime );
      adc->stopStream();
      std::cout << "stream externally stopped.\n";
      SLEEP( pausetime );
    }
  }
  catch ( RtAudioError& e ) {
    e.printMessage();
    goto cleanup;
  }

  adc->closeStream();

  // Next, test internal stopStream() calls.
  mydata.nFrames = (unsigned int) (RUNTIME * fs);
  mydata.returnValue = 1;
  try {
    adc->openStream( &oParams, &iParams, RTAUDIO_SINT32, fs, &bufferFrames, &pulse, (void *)&mydata );

    std::cin.clear();
    fflush(stdin);
    std::cout << "\nPress <enter> to continue test.\n";
    std::cin.get( input );

    for (int i=0; i<REPETITIONS; i++ ) {
      mydata.frameCounter = 0;
      adc->startStream();
      std::cout << "Stream started ... ";
      while ( adc->isStreamRunning() ) SLEEP( 5 );
      std::cout << "stream stopped via callback return value = 1.\n";
      SLEEP( pausetime );
    }
  }
  catch ( RtAudioError& e ) {
    e.printMessage();
    goto cleanup;
  }

  adc->closeStream();

  // Test internal abortStream() calls.
  mydata.returnValue = 2;
  try {
    adc->openStream( &oParams, &iParams, RTAUDIO_SINT32, fs, &bufferFrames, &pulse, (void *)&mydata );
    std::cin.clear();
    fflush(stdin);
    std::cout << "\nPress <enter> to continue test.\n";
    std::cin.get( input );

    for (int i=0; i<REPETITIONS; i++ ) {
      mydata.frameCounter = 0;
      adc->startStream();
      std::cout << "Stream started ... ";
      while ( adc->isStreamRunning() ) SLEEP( 5 );
      std::cout << "stream aborted via callback return value = 2.\n";
      SLEEP( pausetime );
    }
  }
  catch ( RtAudioError& e ) {
    e.printMessage();
    goto cleanup;
  }

  adc->closeStream();

  // Test consecutive stream re-opening.
  mydata.returnValue = 0;
  mydata.nFrames = 50 * fs;
  try {

    std::cin.clear();
    fflush(stdin);
    std::cout << "\nPress <enter> to continue test.\n";
    std::cin.get( input );

    for (int i=0; i<REPETITIONS; i++ ) {
      adc->openStream( &oParams, &iParams, RTAUDIO_SINT32, fs, &bufferFrames, &pulse, (void *)&mydata );
      mydata.frameCounter = 0;
      adc->startStream();
      std::cout << "New stream started ... ";
      SLEEP( runtime );
      adc->stopStream();
      adc->closeStream();
      std::cout << "stream stopped externally and closed.\n";
      SLEEP( pausetime );
    }
  }
  catch ( RtAudioError& e ) {
    e.printMessage();
    goto cleanup;
  }

  delete adc;
  adc = 0;

  // Test consecutive RtAudio creating and deletion.
  try {

    std::cin.clear();
    fflush(stdin);
    std::cout << "\nPress <enter> to continue test.\n";
    std::cin.get( input );

    for (int i=0; i<REPETITIONS; i++ ) {
      adc = new RtAudio();      
      adc->openStream( &oParams, &iParams, RTAUDIO_SINT32, fs, &bufferFrames, &pulse, (void *)&mydata );
      mydata.frameCounter = 0;
      adc->startStream();
      std::cout << "New instance and stream started ... ";
      SLEEP( runtime );
      adc->stopStream();
      adc->closeStream();
      delete adc;
      adc = 0;
      std::cout << "stream stopped and instance deleted.\n";
      SLEEP( pausetime );
    }
  }
  catch ( RtAudioError& e ) {
    e.printMessage();
    goto cleanup;
  }

 cleanup:
  if ( adc && adc->isStreamOpen() ) adc->closeStream();
  if ( adc ) delete adc;

  return 0;
}
