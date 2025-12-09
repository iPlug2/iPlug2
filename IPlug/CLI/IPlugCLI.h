/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#ifndef _IPLUGCLI_
#define _IPLUGCLI_

/**
 * @file
 * @copydoc IPlugCLI
 */

#include "IPlugPlatform.h"
#include "IPlugAPIBase.h"
#include "IPlugProcessor.h"

BEGIN_IPLUG_NAMESPACE

struct InstanceInfo
{
  int argc;
  char** argv;
};

/** Command-line interface base class for an IPlug plug-in.
 *  Enables offline audio processing without real-time I/O or graphics.
 *  @ingroup APIClasses */
class IPlugCLI : public IPlugAPIBase
               , public IPlugProcessor
{
public:
  IPlugCLI(const InstanceInfo& info, const Config& config);
  virtual ~IPlugCLI() = default;

  // IPlugAPIBase overrides - no-ops for CLI
  void BeginInformHostOfParamChange(int idx) override {}
  void InformHostOfParamChange(int idx, double normalizedValue) override {}
  void EndInformHostOfParamChange(int idx) override {}
  void InformHostOfPresetChange() override {}
  bool EditorResize(int viewWidth, int viewHeight) override { return false; }

  // IPlugProcessor overrides
  bool SendMidiMsg(const IMidiMsg& msg) override;
  bool SendSysEx(const ISysEx& msg) override { return false; }

  // CLI-specific methods

  /** Configure audio processing parameters
   * @param sampleRate Sample rate in Hz
   * @param blockSize Processing block size in samples */
  void SetupProcessing(double sampleRate, int blockSize);

  /** Process audio through the plugin
   * @param inputs Array of input channel pointers
   * @param outputs Array of output channel pointers
   * @param nFrames Number of frames to process */
  void Process(double** inputs, double** outputs, int nFrames);

  /** Process silence and capture output (for generators/synths)
   * @param nFrames Total frames to process
   * @param outputs Array of output channel pointers */
  void ProcessSilence(int nFrames, double** outputs);

  /** Process an impulse response
   * @param responseLength Number of output samples to capture
   * @param output Vector to store the impulse response */
  void ProcessImpulse(int responseLength, std::vector<double>& output);

  /** Process a sine wave input and capture output
   * @param frequency Sine wave frequency in Hz
   * @param nFrames Total frames to process
   * @param output Vector to store the output */
  void ProcessSine(double frequency, int nFrames, std::vector<double>& output);

  /** Process white noise input and capture output
   * @param nFrames Total frames to process
   * @param output Vector to store the output */
  void ProcessNoise(int nFrames, std::vector<double>& output);

  /** Process a unit step input and capture output
   * @param nFrames Total frames to process
   * @param output Vector to store the output */
  void ProcessStep(int nFrames, std::vector<double>& output);

  /** Process a log chirp (frequency sweep) and capture output
   * @param startFreq Starting frequency in Hz
   * @param endFreq Ending frequency in Hz
   * @param nFrames Total frames to process
   * @param output Vector to store the output */
  void ProcessChirp(double startFreq, double endFreq, int nFrames, std::vector<double>& output);

  /** Queue a MIDI message for processing
   * @param msg MIDI message to queue */
  void QueueMidiMsg(const IMidiMsg& msg);

  /** Get number of input channels */
  int NInChannels() const { return MaxNChannels(ERoute::kInput); }

  /** Get number of output channels */
  int NOutChannels() const { return MaxNChannels(ERoute::kOutput); }

private:
  IMidiQueue mMidiQueue;
  IMidiQueue mMidiOutQueue;
  int mBlockSize = 512;
};

IPlugCLI* MakePlug(const InstanceInfo& info);

END_IPLUG_NAMESPACE

#endif
