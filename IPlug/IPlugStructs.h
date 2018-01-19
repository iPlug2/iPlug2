#pragma once

#include <algorithm>
#include "wdlstring.h"

#include "IPlugConstants.h"
#include "IPlugByteChunk.h"
#include "IPlugOSDetect.h"
#include "IPlugMidi.h" // <- Midi related structs in here

/** Helper struct to set compile time options to an API class constructor  */
struct IPlugConfig
{
  int nParams;
  int nPresets;
  const char* channelIOStr;
  const char* effectName;
  const char* productName;
  const char* mfrName;
  int vendorVersion;
  int uniqueID;
  int mfrID;
  int latency;
  bool plugDoesMidi;
  bool plugDoesChunks;
  bool plugIsInstrument;
  int plugScChans;
  
  IPlugConfig(int nParams,
              int nPresets,
              const char* channelIOStr,
              const char* effectName,
              const char* productName,
              const char* mfrName,
              int vendorVersion,
              int uniqueID,
              int mfrID,
              int latency,
              bool plugDoesMidi,
              bool plugDoesChunks,
              bool plugIsInstrument,
              int plugScChans)
              
  : nParams(nParams)
  , nPresets(nPresets)
  , channelIOStr(channelIOStr)
  , effectName(effectName)
  , productName(productName)
  , mfrName(mfrName)
  , vendorVersion(vendorVersion)
  , uniqueID(uniqueID)
  , mfrID(mfrID)
  , latency(latency)
  , plugDoesMidi(plugDoesMidi)
  , plugDoesChunks(plugDoesChunks)
  , plugIsInstrument(plugIsInstrument)
  , plugScChans(plugScChans)
  {};
};

/** Used to store channel i/o count together */
struct ChannelIO
{
  int mIn, mOut;
  ChannelIO(int nIn, int nOut) : mIn(nIn), mOut(nOut) {}
};

/** Encapsulates information about the host transport state */
struct ITimeInfo
{
  double mTempo = DEFAULT_TEMPO;
  double mSamplePos = -1.0;
  double mPPQPos = -1.0;
  double mLastBar = -1.0;
  double mCycleStart = -1.0;
  double mCycleEnd = -1.0;

  int mNumerator = 4;
  int mDenominator = 4;

  bool mTransportIsRunning = false;
  bool mTransportLoopEnabled = false;

  ITimeInfo()
  {}
};

/** A struct used for specifying baked-in factory presets */
struct IPreset
{
  bool mInitialized = false;
  char mName[MAX_PRESET_NAME_LEN];

  ByteChunk mChunk;

  IPreset()
  {
    sprintf(mName, "%s", UNUSED_PRESET_NAME);
  }
};
