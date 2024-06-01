/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

#include <cstring>
#include <cstdint>
#include <ctime>
#include <cmath>
#include <cstdio>
#include <cassert>
#include <limits>
#include <memory>
#include <vector>

#include "ptrlist.h"

#include "IPlugPlatform.h"
#include "IPlugConstants.h"
#include "IPlugStructs.h"
#include "IPlugUtilities.h"
#include "NChanDelay.h"

/**
 * @file
 * @copydoc IPlugProcessor
*/

BEGIN_IPLUG_NAMESPACE

struct Config;

/** The base class for IPlug Audio Processing. It knows nothing about presets or parameters or user interface.  */
class IPlugProcessor
{
public:
    
  enum TailSize
  {
      kTailNone = 0,
      kTailInfinite = std::numeric_limits<int>::max()
  };
    
  /** IPlugProcessor constructor
   * @param config \todo
   * @param plugAPI \todo */
  IPlugProcessor(const Config& config, EAPI plugAPI);
  virtual ~IPlugProcessor();

  IPlugProcessor(const IPlugProcessor&) = delete;
  IPlugProcessor& operator=(const IPlugProcessor&) = delete;
  
#pragma mark - Methods you implement in your plug-in class - you do not call these methods

  /** Override in your plug-in class to process audio
   * In ProcessBlock you are always guaranteed to get valid pointers to all the channels the plugin requested
   * (the maximum possible input channel count and the maximum possible output channel count including multiple buses).
   * If the host hasn't connected all the pins, the unconnected channels will be full of zeros.
   * THIS METHOD IS CALLED BY THE HIGH PRIORITY AUDIO THREAD - You should be careful not to do any unbounded, blocking operations such as file I/O which could cause audio dropouts
   * @param inputs Two-dimensional array containing the non-interleaved input buffers of audio samples for all channels
   * @param outputs Two-dimensional array for audio output (non-interleaved).
   * @param nFrames The block size for this block: number of samples per channel.*/
  virtual void ProcessBlock(sample** inputs, sample** outputs, int nFrames);

  /** Override this method to handle incoming MIDI messages. The method is called prior to ProcessBlock().
   * You can use IMidiQueue in combination with this method in order to queue the message and process at the appropriate time in ProcessBlock()
   * THIS METHOD IS CALLED BY THE HIGH PRIORITY AUDIO THREAD - You should be careful not to do any unbounded, blocking operations such as file I/O which could cause audio dropouts
   * @param msg The incoming midi message (includes a timestamp to indicate the offset in the forthcoming block of audio to be processed in ProcessBlock()) */
  virtual void ProcessMidiMsg(const IMidiMsg& msg);

  /** Override this method to handle incoming MIDI System Exclusive (SysEx) messages. The method is called prior to ProcessBlock().
   * THIS METHOD IS CALLED BY THE HIGH PRIORITY AUDIO THREAD - You should be careful not to do any unbounded, blocking operations such as file I/O which could cause audio dropouts */
  virtual void ProcessSysEx(const ISysEx& msg) {}

  /** Override this method in your plug-in class to do something prior to playback etc. (e.g.clear buffers, update internal DSP with the latest sample rate) */
  virtual void OnReset() { TRACE }

  /** Override OnActivate() which should be called by the API class when a plug-in is "switched on" by the host on a track when the channel count is known.
   * This may not work reliably because different hosts have different interpretations of "activate".
   * Unlike OnReset() which called when the transport is reset or the sample rate changes OnActivate() is a good place to handle change of I/O connections.
   * @param active \c true if the host has activated the plug-in */
  virtual void OnActivate(bool active) { TRACE }

#pragma mark - Methods you can call - some of which have custom implementations in the API classes, some implemented in IPlugProcessor.cpp

  /** Send a single MIDI message // TODO: info about what thread should this be called on or not called on!
   * @param msg The IMidiMsg to send
   * @return \c true if successful */
  virtual bool SendMidiMsg(const IMidiMsg& msg) = 0;

  /** Send a collection of MIDI messages // TODO: info about what thread should this be called on or not called on!
   * @param msgs The IMidiMsg to send
   * @return \c true if successful */
  virtual bool SendMidiMsgs(WDL_TypedBuf<IMidiMsg>& msgs);

  /** Send a single MIDI System Exclusive (SysEx) message // TODO: info about what thread should this be called on or not called on!
   * @param msg The ISysEx to send
   * @return \c true if successful */
  virtual bool SendSysEx(const ISysEx& msg) { return false; }

  /** @return Sample rate (in Hz) */
  double GetSampleRate() const { return mSampleRate; }

  /** @return Maximum block size in samples, actual blocksize may vary each ProcessBlock() */
  int GetBlockSize() const { return mBlockSize; }

  /** @return Plugin latency (in samples) */
  int GetLatency() const { return mLatency; }

  /** @return The tail size in samples (useful for reverberation plug-ins, that may need to decay after the transport stops or an audio item ends) */
  int GetTailSize() const { return mTailSize; }

  /** @return \c true if the plugin has an infinite tail */
  bool GetTailIsInfinite() const { return GetTailSize() == kTailInfinite; }
    
  /** @return \c true if the plugin is currently bypassed */
  bool GetBypassed() const { return mBypassed; }

  /** @return \c true if the plugin is currently rendering off-line */
  bool GetRenderingOffline() const { return mRenderingOffline; };

#pragma mark -
  /** @return The number of samples elapsed since start of project timeline. */
  double GetSamplePos() const { return mTimeInfo.mSamplePos; }

  /** @return The tempo in beats per minute */
  double GetTempo() const { return mTimeInfo.mTempo; }

  /** @return The number of beats elapsed since start of project timeline. */
  double GetPPQPos() const { return mTimeInfo.mPPQPos; }

  /** @return \c true if the transport is running */
  bool GetTransportIsRunning() const { return mTimeInfo.mTransportIsRunning; }
  
  /** @return The number of samples in a beat */
  double GetSamplesPerBeat() const;

  /** @param numerator The upper part of the current time signature e.g "6" in the time signature 6/8
   *  @param denominator The lower part of the current time signature e.g "8" in the time signature 6/8 */
  void GetTimeSig(int& numerator, int& denominator) const { numerator = mTimeInfo.mNumerator; denominator = mTimeInfo.mDenominator; }

#pragma mark -
  
  /** Get the name for a particular bus
   * @param direction Input or output bus
   * @param busIdx The index of the bus
   * @param nBuses The total number of buses for this direction
   * @param str String to fill with the bus name */
  virtual void GetBusName(ERoute direction, int busIdx, int nBuses, WDL_String& str) const;
  
  /** @return The number of channel I/O configs derived from the channel io string*/
  int NIOConfigs() const { return mIOConfigs.GetSize(); }

  /** @return const Pointer to an IOConfig at idx. Can return nullptr if idx is invalid */
  const IOConfig* GetIOConfig(int idx) const { return mIOConfigs.Get(idx); }

  /** @return Index of IOConfig that matches input and output bus vectors. Can return -1 if not found */
  int GetIOConfigWithChanCounts(std::vector<int>& inputBuses, std::vector<int>& outputBuses);
  
  /** Used to determine the maximum number of input or output buses based on what was specified in the channel I/O config string
   * @param direction Return input or output bus count
   * @param pConfigIdxWithTheMostBuses Optional ptr to report the index of the config with the max bus count, if multiple configs have the same bus count, this should report the one with the higher channel count
   * @return The maximum bus count across all channel I/O configs */
  int MaxNBuses(ERoute direction, int* pConfigIdxWithTheMostBuses = nullptr) const;

  /** For a given input or output bus what is the maximum possible number of channels. This method is not Realtime safe.
   * @param direction Return input or output bus count
   * @param busIdx The index of the bus to look up
   * @return return The maximum number of channels on that bus */
  int MaxNChannelsForBus(ERoute direction, int busIdx) const;

  /** Check if we have any wildcard characters in the channel I/O configs
   * @param direction Return input or output bus count
   * @return \c true if the bus has a wildcard, meaning it should work on any number of channels */
  bool HasWildcardBus(ERoute direction) const { return mIOConfigs.Get(0)->ContainsWildcard(direction); } // \todo only supports a single I/O config

  /** @param direction Whether you want to test inputs or outputs
   * @return Total number of input or output channel buffers (not necessarily connected) */
  int MaxNChannels(ERoute direction) const { return mChannelData[direction].GetSize(); }

  /** @param direction Whether you want to test inputs or outputs
    * @param chIdx channel index
    * @return \c true if the host has connected this channel*/
  bool IsChannelConnected(ERoute direction, int chIdx) const { return (chIdx < mChannelData[direction].GetSize() && mChannelData[direction].Get(chIdx)->mConnected); }

  /** @param direction Whether you want to test inputs or outputs
   * @return The number of channels connected for input/output. WARNING: this assumes consecutive channel connections */
  int NChannelsConnected(ERoute direction) const;

  /** Convenience method to find out how many input channels are connected
   * @return The number of channels connected for input. WARNING: this assumes consecutive channel connections */
  inline int NInChansConnected() const { return NChannelsConnected(ERoute::kInput); }

  /** Convenience method to find out how many output channels are connected
   * @return The number of channels connected for output. WARNING: this assumes consecutive channel connections */
  inline int NOutChansConnected() const { return NChannelsConnected(ERoute::kOutput); }

  /** Check if a certain configuration of input channels and output channels is allowed based on the channel I/O configs
   * @param NInputChans Number of inputs to test, if set to -1 = check NOutputChans only
   * @param NOutputChans Number of outputs to test, if set to -1 = check NInputChans only
   * @return \c true if the configurations is valid */
  bool LegalIO(int NInputChans, int NOutputChans) const; //TODO: this should be updated

  /** @return \c true if this plug-in has a side-chain input, which may not necessarily be active in the current I/O config */
  bool HasSidechainInput() const { return MaxNBuses(ERoute::kInput) > 1; }

  /** This is called by IPlugVST in order to limit a plug-in to stereo I/O for certain picky hosts \todo may no longer be relevant*/
  void LimitToStereoIO();//TODO: this should be updated

  /** @return \c true if the plug-in was configured as an instrument at compile time */
  bool IsInstrument() const { return mPlugType == EIPlugPluginType::kInstrument; }

  /** @return \c true if the plug-in was configured as an MFX at compile time */
  bool IsMidiEffect() const { return mPlugType == EIPlugPluginType::kMIDIEffect; }
  
  /** @return int The 4Char identifier for the type of audiounit plugin, e.g. 'aufx' for an effect audiounit */
  int GetAUPluginType() const;

  /** @return \c true if the plug-in was configured to receive midi at compile time */
  bool DoesMIDIIn() const { return mDoesMIDIIn; }

  /** @return \c true if the plug-in was configured to receive midi at compile time */
  bool DoesMIDIOut() const { return mDoesMIDIOut; }
  
  /** @return \c true if the plug-in was configured to support midi polyphonic expression at compile time */
  bool DoesMPE() const { return mDoesMPE; }

  /**  This allows you to label input/output channels in supporting VST2 hosts.
   * * For example a 4 channel plug-in that deals with FuMa BFormat first order ambisonic material, might label these channels
   "W", "X", "Y", "Z", rather than the default "input 1", "input 2", "input 3", "input 4"
   * @param idx The index of the channel that you wish to label
   * @param formatStr printf style format string to compose label for the channel - where %i will be the channel index
   * @param zeroBased If \c true the index in the format string will be zero based */
  void SetChannelLabel(ERoute direction, int idx, const char* formatStr, bool zeroBased = false);

  /** Call this if the latency of your plug-in changes after initialization (perhaps from OnReset() )
   * This may not be supported by the host. The method is virtual because it's overridden in API classes.
   @param latency Latency in samples */
  virtual void SetLatency(int latency);

  /** Call this method if you need to update the tail size at runtime, for example if the decay time of your reverb effect changes
   * Use kTailInfinite for an infinite tail
   * You may also use kTailNone for no tail (but this is default in any case)
   * @param tailSize the new tailsize in samples*/
  virtual void SetTailSize(int tailSize) { mTailSize = tailSize; }

  /** A static method to parse the config.h channel I/O string.
   * @param IOStr Space separated cstring list of I/O configurations for this plug-in in the format ninchans-noutchans.
   * A hypen character \c(-) deliminates input-output. Supports multiple buses, which are indicated using a period \c(.) character.
   * For instance plug-in that supports mono input and mono output with a mono side-chain input could have a channel io string of "1.1-1".
   * A drum synthesiser with four stereo output busses could be configured with a io string of "0-2.2.2.2";
   * @param channelIOList A list of pointers to ChannelIO structs, where we will store here
   * @param totalNInChans The total number of input channels across all buses will be stored here
   * @param totalNOutChans The total number of output channels across all buses will be stored here
   * @param totalNInBuses The total number of input buses across all channel I/O configs will be stored here
   * @param totalNOutBuses The total number of output buses across all channel I/O configs will be stored here
   * @return The number of space separated channel I/O configs that have been detected in IOStr */
  static int ParseChannelIOStr(const char* IOStr, WDL_PtrList<IOConfig>& channelIOList, int& totalNInChans, int& totalNOutChans, int& totalNInBuses, int& totalNOutBuses);

protected:
#pragma mark - Methods called by the API class - you do not call these methods in your plug-in class
  void SetChannelConnections(ERoute direction, int idx, int n, bool connected);
  void InitLatencyDelay();
  
  //The following methods are duplicated, in order to deal with either single or double precision processing,
  //depending on the value of arguments passed in
  void AttachBuffers(ERoute direction, int idx, int n, PLUG_SAMPLE_DST** ppData, int nFrames);
  void AttachBuffers(ERoute direction, int idx, int n, PLUG_SAMPLE_SRC** ppData, int nFrames);
  void PassThroughBuffers(PLUG_SAMPLE_SRC type, int nFrames);
  void PassThroughBuffers(PLUG_SAMPLE_DST type, int nFrames);
  void ProcessBuffers(PLUG_SAMPLE_SRC type, int nFrames);
  void ProcessBuffers(PLUG_SAMPLE_DST type, int nFrames);
  void ProcessBuffersAccumulating(int nFrames); // only for VST2 deprecated method single precision
  void ZeroScratchBuffers();
  void SetSampleRate(double sampleRate) { mSampleRate = sampleRate; }
  void SetBlockSize(int blockSize);
  void SetBypassed(bool bypassed) { mBypassed = bypassed; }
  void SetTimeInfo(const ITimeInfo& timeInfo) { mTimeInfo = timeInfo; }
  void SetRenderingOffline(bool renderingOffline) { mRenderingOffline = renderingOffline; }
  const WDL_String& GetChannelLabel(ERoute direction, int idx) { return mChannelData[direction].Get(idx)->mLabel; }

private:
  /** See EIPlugPluginTypes */
  EIPlugPluginType mPlugType;
  /** \c true if the plug-in accepts MIDI input */
  bool mDoesMIDIIn;
  /** \c true if the plug-in produces MIDI output */
  bool mDoesMIDIOut;
  /** \c true if the plug-in supports MIDI Polyphonic Expression */
  bool mDoesMPE;
  /** Plug-in latency (in samples) */
  int mLatency;
  /** Current sample rate (in Hz) */
  double mSampleRate = DEFAULT_SAMPLE_RATE;
  /** Current block size (in samples) */
  int mBlockSize = 0;
  /** Current tail size (in samples) */
  int mTailSize = 0;
  /** \c true if the plug-in is bypassed */
  bool mBypassed = false;
  /** \c true if the plug-in is rendering off-line*/
  bool mRenderingOffline = false;
  /** A list of IOConfig structures populated by ParseChannelIOStr in the IPlugProcessor constructor */
  WDL_PtrList<IOConfig> mIOConfigs;
  /* Manages pointers to the actual data for each channel */
  WDL_TypedBuf<sample*> mScratchData[2];
  /* A list of IChannelData structures corresponding to every input/output channel */
  WDL_PtrList<IChannelData<>> mChannelData[2];
  /** A multi-channel delay line used to delay the bypassed signal when a plug-in with latency is bypassed. */
  std::unique_ptr<NChanDelayLine<sample>> mLatencyDelay = nullptr;
protected: // protected because it needs to be access by the API classes, and don't want a setter/getter
  /** Contains detailed information about the transport state */
  ITimeInfo mTimeInfo;
};

END_IPLUG_NAMESPACE
