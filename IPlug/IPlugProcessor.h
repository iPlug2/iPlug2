#pragma once

#include <cstring>
#include <cstdint>

#include "ptrlist.h"

#include "IPlugPlatform.h"
#include "IPlugConstants.h"
#include "IPlugStructs.h"
#include "IPlugUtilities.h"
#include "NChanDelay.h"

/**
 * @file
 * @copydoc IPlugProcessor
 * @defgroup APIClasses IPlug::APIClasses
*/

struct IPlugConfig;

/** The base class for IPlug Audio Processing. No UI framework code included. */
class IPlugProcessor
{
public:
  IPlugProcessor(IPlugConfig config, EAPI plugAPI);
  virtual ~IPlugProcessor();

  virtual void OnReset() { TRACE; }

  /** Default passthrough.
   * Inputs and outputs are two-dimensional arrays [nChannel][nSample]
   * @param inputs 2D array containing the input buffer for all channels. To access a specific channel:
   * @code
   * double* inLeft  = inputs[0];
   * double* inRight = inputs[1];
   * @endcode
   * The result is a double array, indexed from 0 to nFrames which can be accessed in this manner:
   * @code
   * for (int i = 0; i < nFrames; ++i) {
   *    double sampleL = inLeft[i];
   *    double sampleR = inRight[i];
   * }
   * @endcode
   * For multi-channel plugins, use subsequent index numbers
   * @param outputs 2D output array. The output of your DSP goes here
   * @param nFrames Number of samples per array (buffer size)
  */
  virtual void ProcessBlock(double** inputs, double** outputs, int nFrames);

  // This is an idle call from the audio processing thread
  // Only active if USE_IDLE_CALLS is defined
  virtual void OnIdle() {}

  // Not usually needed ... Reset is called on activate regardless of whether this is implemented.
  // Also different hosts have different interpretations of "activate".
  virtual void OnActivate(bool active) { TRACE; }

  virtual void ProcessMidiMsg(IMidiMsg& msg);
  virtual void ProcessSysEx(ISysEx& msg) {}


  // ----------------------------------------
  // Useful stuff for your plugin class or an outsider to call,
  // most of which is implemented by the API class.

  /** @brief Get sample rate (in Hz)
   * @return Sample rate (in Hz). Defaults to 44100.0 */
  double GetSampleRate() const { return mSampleRate; }
  int GetBlockSize() const { return mBlockSize; }
  /** @return Plugin latency (in samples)
   * @todo Please check this and remove this note once done */
  int GetLatency() const { return mLatency; }
  /** @return \c True if the plugin is currently bypassed */
  bool GetBypassed() const { return mBypassed; }

  // In ProcessBlock you are always guaranteed to get valid pointers
  // to all the channels the plugin requested.  If the host hasn't connected all the pins,
  // the unconnected channels will be full of zeros.
  
  int NChannelIO() const { return mIOConfigs.GetSize(); }
  void GetChannelIO(int optionIdx, int& numInputs, int& numOutputs);
  int NInChannels() const { return mInChannels.GetSize(); }
  int NOutChannels() const { return mOutChannels.GetSize(); }
  bool IsInChannelConnected(int chIdx) const;
  bool IsOutChannelConnected(int chIdx) const;
  int NInChansConnected() const;
  int NOutChansConnected() const;

  virtual bool IsRenderingOffline() { return false; };
  
  
  /**
   Called by api class prior to a block render
   */
  virtual void GetTimeInfo() {};
  virtual int GetSamplePos() { return mTimeInfo.mSamplePos; }  // Samples since start of project.
  virtual double GetTempo() { return mTimeInfo.mTempo; }
  double GetSamplesPerBeat();
  virtual void GetTimeSig(int& numerator, int& denominator) { numerator = mTimeInfo.mNumerator; denominator = mTimeInfo.mDenominator; }

  int GetTailSize() { return mTailSize; }


  /**
   Used to determine the maximum number of input or output buses based on the channel io config string

   @param direction Return input or output bus count
   @return The maximum bus count across all channel i/o configs
   */
  int MaxNBuses(ERoute direction)
  {
    if(direction == kInput)
      return mMaxNInBuses;
    else
      return mMaxNOutBuses;
  }
  
  /**
   For a given input or output bus what is the maximum possible number of channels

   @param direction Return input or output bus count
   @param busIdx The index of the bus to look up
   @return return The maximum number of channels on that bus
   */
  int MaxNChannelsForBus(ERoute direction, int busIdx);

  /**
   Check if we have any wildcard characters in the channel io configs

   @param direction Return input or output bus count
   @return /true if the bus has a wildcard, meaning it should work on any number of channels
   */
  bool HasWildcardBus(ERoute direction) { return mIOConfigs.Get(0)->ContainsWildcard(direction); } // only supports a single IO Config
  
  /**
   @return c/ true if this plug-in has a side-chain input, which may not necessarily be active in the current io config
   */
  bool HasSidechainInput() { return mMaxNInBuses > 1; }
  int NSidechainChannels() { return 1; } // TODO: this needs to be more flexible, based on channel i/o
  bool IsInstrument() { return mIsInstrument; }
  bool DoesMIDI() { return mDoesMIDI; }
  bool LegalIO(int NInputChans, int NOutputChans);    // -1 for either means check the other value only.


  // for labelling individual inputs/outputs (VST2)
  void SetInputLabel(int idx, const char* label);
  void SetOutputLabel(int idx, const char* label);

  const WDL_String& GetInputLabel(int idx) { return mInChannels.Get(idx)->mLabel; }
  const WDL_String& GetOutputLabel(int idx) { return mOutChannels.Get(idx)->mLabel; }

  void LimitToStereoIO();

  // If latency changes after initialization (often not supported by the host).
  virtual void SetLatency(int samples);

  // set to 0xffffffff for infinite tail (VST3), or 0 for none (default)
  // for VST2 setting to 1 means no tail, but it would be better i think to leave it at 0, the default
  void SetTailSize(unsigned int tailSizeSamples) { mTailSize = tailSizeSamples; }

  virtual bool SendMidiMsg(IMidiMsg& msg) = 0;
  bool SendMidiMsgs(WDL_TypedBuf<IMidiMsg>* pMsgs);
  virtual bool SendSysEx(ISysEx& msg) { return false; }


//  TODO:use RoutingDir to reduce the number of methods
  void SetInputChannelConnections(int idx, int n, bool connected);
  void SetOutputChannelConnections(int idx, int n, bool connected);
  void AttachInputBuffers(int idx, int n, double** ppData, int nFrames);
  void AttachInputBuffers(int idx, int n, float** ppData, int nFrames);
  void AttachOutputBuffers(int idx, int n, double** ppData);
  void AttachOutputBuffers(int idx, int n, float** ppData);
  
  
  void PassThroughBuffers(float sampleType, int nFrames);
  void PassThroughBuffers(double sampleType, int nFrames);
  void ProcessBuffers(float sampleType, int nFrames);
  void ProcessBuffers(double sampleType, int nFrames);
  void ProcessBuffersAccumulating(float sampleType, int nFrames);
  void ZeroScratchBuffers();

  void SetSampleRate(double sampleRate);
  virtual void SetBlockSize(int blockSize); // overridden in IPlugAU

  //TODO: almost everything below this line is going in the processor
  
  /** \c True if the plug-in is an instrument */
  bool mIsInstrument;
  /** \c True if the plug-in requires MIDI input */
  /** @todo Someone check this please */
  bool mDoesMIDI;
  /** Plug-in latency (in samples) */
  /** @todo Someone check this please */
  int mLatency;
  /** \c True if the plug-in is bypassed */
  bool mBypassed = false;
  
  double mSampleRate  = DEFAULT_SAMPLE_RATE;
  int mBlockSize = 0;
  int mTailSize = 0;
  
  int mMaxNInBuses, mMaxNOutBuses; // could be private
  NChanDelayLine<double>* mLatencyDelay = nullptr;
  WDL_TypedBuf<double*> mInData, mOutData;
  WDL_PtrList<IChannelData<>> mInChannels;
  WDL_PtrList<IChannelData<>> mOutChannels;
  WDL_PtrList<IOConfig> mIOConfigs;

  ITimeInfo mTimeInfo;
  
  /**
   Static method to parse the config.h channel i/o string.
   
   @param IOStr Space separated cstring list of i/o configurations for this plug-in in the format ninchans-noutchans. A hypen character \c(-) deliminates input-output. Supports multiple buses, which are indicated using a period \c(.) character. For instance plug-in that supports mono input and mono output with a mono side-chain input could have a channel io string of "1.1-1". A drum synthesiser with four stereo output busses could be configured with a io string of "0-2.2.2.2";
   @param channelIOList A list of pointers to ChannelIO structs, where we will store here
   @param totalNInChans The total number of input channels across all buses will be stored here
   @param totalNOutChans The total number of output channels across all buses will be stored here
   @param totalNInBuses The total number of input buses across all channel io configs will be stored here
   @param totalNOutBuses The total number of output buses across all channel io configs will be stored here
   @return The number of space separated channel io configs that have been detected in IOStr
   */
  static int ParseChannelIOStr(const char* IOStr, WDL_PtrList<IOConfig>& channelIOList, int& totalNInChans, int& totalNOutChans, int& totalNInBuses, int& totalNOutBuses);
};
