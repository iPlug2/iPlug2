/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#define _USE_MATH_DEFINES
#include <math.h>
#include "IPlugCLI.h"
#include <random>

using namespace iplug;

IPlugCLI::IPlugCLI(const InstanceInfo& info, const Config& config)
: IPlugAPIBase(config, kAPICLI)
, IPlugProcessor(config, kAPICLI)
{
  Trace(TRACELOC, "%s%s", config.pluginName, config.channelIOStr);

  SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), true);
  SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), true);

  SetBlockSize(512);
  mBlockSize = 512;
}

void IPlugCLI::SetupProcessing(double sampleRate, int blockSize)
{
  SetSampleRate(sampleRate);
  SetBlockSize(blockSize);
  mBlockSize = blockSize;
  OnParamReset(kReset);
  OnReset();
}

void IPlugCLI::Process(double** inputs, double** outputs, int nFrames)
{
  SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), !IsInstrument());
  SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), true);
  AttachBuffers(ERoute::kInput, 0, NChannelsConnected(ERoute::kInput), inputs, nFrames);
  AttachBuffers(ERoute::kOutput, 0, NChannelsConnected(ERoute::kOutput), outputs, nFrames);

  // Process any queued MIDI messages
  while (!mMidiQueue.Empty())
  {
    IMidiMsg msg = mMidiQueue.Peek();
    if (msg.mOffset < nFrames)
    {
      ProcessMidiMsg(msg);
      mMidiQueue.Remove();
    }
    else
    {
      // Adjust offset for next block
      msg.mOffset -= nFrames;
      mMidiQueue.Remove();
      mMidiQueue.Add(msg);
      break;
    }
  }

  ProcessBuffers(0.0, nFrames);
}

void IPlugCLI::ProcessSilence(int nFrames, double** outputs)
{
  int nOutCh = MaxNChannels(ERoute::kOutput);
  int nInCh = MaxNChannels(ERoute::kInput);

  // Create silent input buffers
  std::vector<std::vector<double>> inputBuffers(nInCh);
  std::vector<double*> inputs(nInCh);
  for (int c = 0; c < nInCh; c++)
  {
    inputBuffers[c].resize(mBlockSize, 0.0);
    inputs[c] = inputBuffers[c].data();
  }

  int framesProcessed = 0;
  while (framesProcessed < nFrames)
  {
    int framesToProcess = std::min(mBlockSize, nFrames - framesProcessed);

    // Point outputs to correct offset
    std::vector<double*> outputPtrs(nOutCh);
    for (int c = 0; c < nOutCh; c++)
    {
      outputPtrs[c] = outputs[c] + framesProcessed;
    }

    Process(inputs.data(), outputPtrs.data(), framesToProcess);
    framesProcessed += framesToProcess;
  }
}

void IPlugCLI::ProcessImpulse(int responseLength, std::vector<double>& output)
{
  int nOutCh = MaxNChannels(ERoute::kOutput);
  int nInCh = MaxNChannels(ERoute::kInput);

  // Allocate output buffers
  std::vector<std::vector<double>> outputBuffers(nOutCh);
  std::vector<double*> outputs(nOutCh);
  for (int c = 0; c < nOutCh; c++)
  {
    outputBuffers[c].resize(responseLength, 0.0);
    outputs[c] = outputBuffers[c].data();
  }

  // Create input buffers - impulse in first block
  std::vector<std::vector<double>> inputBuffers(nInCh);
  std::vector<double*> inputs(nInCh);
  for (int c = 0; c < nInCh; c++)
  {
    inputBuffers[c].resize(mBlockSize, 0.0);
    inputs[c] = inputBuffers[c].data();
  }

  int framesProcessed = 0;
  bool impulseApplied = false;

  while (framesProcessed < responseLength)
  {
    int framesToProcess = std::min(mBlockSize, responseLength - framesProcessed);

    // Apply impulse at sample 0 of first block
    if (!impulseApplied && nInCh > 0)
    {
      inputBuffers[0][0] = 1.0;
      impulseApplied = true;
    }

    // Point outputs to correct offset
    std::vector<double*> outputPtrs(nOutCh);
    for (int c = 0; c < nOutCh; c++)
    {
      outputPtrs[c] = outputs[c] + framesProcessed;
    }

    Process(inputs.data(), outputPtrs.data(), framesToProcess);

    // Clear input after first block
    if (framesProcessed == 0 && nInCh > 0)
    {
      inputBuffers[0][0] = 0.0;
    }

    framesProcessed += framesToProcess;
  }

  // Copy first output channel to result
  output.resize(responseLength);
  if (nOutCh > 0)
  {
    std::copy(outputBuffers[0].begin(), outputBuffers[0].end(), output.begin());
  }
}

void IPlugCLI::ProcessSine(double frequency, int nFrames, std::vector<double>& output)
{
  int nOutCh = MaxNChannels(ERoute::kOutput);
  int nInCh = MaxNChannels(ERoute::kInput);

  // Allocate output buffers
  std::vector<std::vector<double>> outputBuffers(nOutCh);
  std::vector<double*> outputs(nOutCh);
  for (int c = 0; c < nOutCh; c++)
  {
    outputBuffers[c].resize(nFrames, 0.0);
    outputs[c] = outputBuffers[c].data();
  }

  // Create input buffers
  std::vector<std::vector<double>> inputBuffers(nInCh);
  std::vector<double*> inputs(nInCh);
  for (int c = 0; c < nInCh; c++)
  {
    inputBuffers[c].resize(mBlockSize, 0.0);
    inputs[c] = inputBuffers[c].data();
  }

  double phase = 0.0;
  double phaseInc = 2.0 * M_PI * frequency / GetSampleRate();
  int framesProcessed = 0;

  while (framesProcessed < nFrames)
  {
    int framesToProcess = std::min(mBlockSize, nFrames - framesProcessed);

    // Fill input buffer with sine wave
    if (nInCh > 0)
    {
      for (int i = 0; i < framesToProcess; i++)
      {
        inputBuffers[0][i] = std::sin(phase);
        phase += phaseInc;
      }
      // Keep phase in range to avoid precision loss
      while (phase > 2.0 * M_PI)
        phase -= 2.0 * M_PI;
    }

    // Point outputs to correct offset
    std::vector<double*> outputPtrs(nOutCh);
    for (int c = 0; c < nOutCh; c++)
    {
      outputPtrs[c] = outputs[c] + framesProcessed;
    }

    Process(inputs.data(), outputPtrs.data(), framesToProcess);
    framesProcessed += framesToProcess;
  }

  // Copy first output channel to result
  output.resize(nFrames);
  if (nOutCh > 0)
  {
    std::copy(outputBuffers[0].begin(), outputBuffers[0].end(), output.begin());
  }
}

void IPlugCLI::ProcessNoise(int nFrames, std::vector<double>& output)
{
  int nOutCh = MaxNChannels(ERoute::kOutput);
  int nInCh = MaxNChannels(ERoute::kInput);

  // Allocate output buffers
  std::vector<std::vector<double>> outputBuffers(nOutCh);
  std::vector<double*> outputs(nOutCh);
  for (int c = 0; c < nOutCh; c++)
  {
    outputBuffers[c].resize(nFrames, 0.0);
    outputs[c] = outputBuffers[c].data();
  }

  // Create input buffers
  std::vector<std::vector<double>> inputBuffers(nInCh);
  std::vector<double*> inputs(nInCh);
  for (int c = 0; c < nInCh; c++)
  {
    inputBuffers[c].resize(mBlockSize, 0.0);
    inputs[c] = inputBuffers[c].data();
  }

  // Set up random number generator
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dist(-1.0, 1.0);

  int framesProcessed = 0;

  while (framesProcessed < nFrames)
  {
    int framesToProcess = std::min(mBlockSize, nFrames - framesProcessed);

    // Fill input buffer with white noise
    if (nInCh > 0)
    {
      for (int i = 0; i < framesToProcess; i++)
      {
        inputBuffers[0][i] = dist(gen);
      }
    }

    // Point outputs to correct offset
    std::vector<double*> outputPtrs(nOutCh);
    for (int c = 0; c < nOutCh; c++)
    {
      outputPtrs[c] = outputs[c] + framesProcessed;
    }

    Process(inputs.data(), outputPtrs.data(), framesToProcess);
    framesProcessed += framesToProcess;
  }

  // Copy first output channel to result
  output.resize(nFrames);
  if (nOutCh > 0)
  {
    std::copy(outputBuffers[0].begin(), outputBuffers[0].end(), output.begin());
  }
}

void IPlugCLI::ProcessStep(int nFrames, std::vector<double>& output)
{
  int nOutCh = MaxNChannels(ERoute::kOutput);
  int nInCh = MaxNChannels(ERoute::kInput);

  // Allocate output buffers
  std::vector<std::vector<double>> outputBuffers(nOutCh);
  std::vector<double*> outputs(nOutCh);
  for (int c = 0; c < nOutCh; c++)
  {
    outputBuffers[c].resize(nFrames, 0.0);
    outputs[c] = outputBuffers[c].data();
  }

  // Create input buffers - filled with 1.0 (unit step starts immediately)
  std::vector<std::vector<double>> inputBuffers(nInCh);
  std::vector<double*> inputs(nInCh);
  for (int c = 0; c < nInCh; c++)
  {
    inputBuffers[c].resize(mBlockSize, 1.0);
    inputs[c] = inputBuffers[c].data();
  }

  int framesProcessed = 0;

  while (framesProcessed < nFrames)
  {
    int framesToProcess = std::min(mBlockSize, nFrames - framesProcessed);

    // Point outputs to correct offset
    std::vector<double*> outputPtrs(nOutCh);
    for (int c = 0; c < nOutCh; c++)
    {
      outputPtrs[c] = outputs[c] + framesProcessed;
    }

    Process(inputs.data(), outputPtrs.data(), framesToProcess);
    framesProcessed += framesToProcess;
  }

  // Copy first output channel to result
  output.resize(nFrames);
  if (nOutCh > 0)
  {
    std::copy(outputBuffers[0].begin(), outputBuffers[0].end(), output.begin());
  }
}

void IPlugCLI::ProcessChirp(double startFreq, double endFreq, int nFrames, std::vector<double>& output)
{
  int nOutCh = MaxNChannels(ERoute::kOutput);
  int nInCh = MaxNChannels(ERoute::kInput);

  // Allocate output buffers
  std::vector<std::vector<double>> outputBuffers(nOutCh);
  std::vector<double*> outputs(nOutCh);
  for (int c = 0; c < nOutCh; c++)
  {
    outputBuffers[c].resize(nFrames, 0.0);
    outputs[c] = outputBuffers[c].data();
  }

  // Create input buffers
  std::vector<std::vector<double>> inputBuffers(nInCh);
  std::vector<double*> inputs(nInCh);
  for (int c = 0; c < nInCh; c++)
  {
    inputBuffers[c].resize(mBlockSize, 0.0);
    inputs[c] = inputBuffers[c].data();
  }

  // Log sweep parameters
  double sr = GetSampleRate();
  double duration = nFrames / sr;
  double k = std::pow(endFreq / startFreq, 1.0 / duration);

  double phase = 0.0;
  int globalSample = 0;
  int framesProcessed = 0;

  while (framesProcessed < nFrames)
  {
    int framesToProcess = std::min(mBlockSize, nFrames - framesProcessed);

    // Fill input buffer with chirp
    if (nInCh > 0)
    {
      for (int i = 0; i < framesToProcess; i++)
      {
        double t = globalSample / sr;
        double instantFreq = startFreq * std::pow(k, t);
        double phaseInc = 2.0 * M_PI * instantFreq / sr;
        inputBuffers[0][i] = std::sin(phase);
        phase += phaseInc;
        globalSample++;
      }
      // Keep phase in range
      while (phase > 2.0 * M_PI)
        phase -= 2.0 * M_PI;
    }

    // Point outputs to correct offset
    std::vector<double*> outputPtrs(nOutCh);
    for (int c = 0; c < nOutCh; c++)
    {
      outputPtrs[c] = outputs[c] + framesProcessed;
    }

    Process(inputs.data(), outputPtrs.data(), framesToProcess);
    framesProcessed += framesToProcess;
  }

  // Copy first output channel to result
  output.resize(nFrames);
  if (nOutCh > 0)
  {
    std::copy(outputBuffers[0].begin(), outputBuffers[0].end(), output.begin());
  }
}

void IPlugCLI::QueueMidiMsg(const IMidiMsg& msg)
{
  mMidiQueue.Add(msg);
}

bool IPlugCLI::SendMidiMsg(const IMidiMsg& msg)
{
  mMidiOutQueue.Add(msg);
  return true;
}
