/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "IPlugCLI.h"

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

void IPlugCLI::QueueMidiMsg(const IMidiMsg& msg)
{
  mMidiQueue.Add(msg);
}

bool IPlugCLI::SendMidiMsg(const IMidiMsg& msg)
{
  mMidiOutQueue.Add(msg);
  return true;
}
