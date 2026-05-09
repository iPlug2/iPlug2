/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#define _USE_MATH_DEFINES
#include <cmath>
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
}

void IPlugCLI::SetupProcessing(double sampleRate, int blockSize)
{
  SetSampleRate(sampleRate);
  SetBlockSize(blockSize);
  SetChannelConnections(ERoute::kInput, 0, MaxNChannels(ERoute::kInput), !IsInstrument());
  SetChannelConnections(ERoute::kOutput, 0, MaxNChannels(ERoute::kOutput), true);
  OnParamReset(kReset);
  OnReset();
}

void IPlugCLI::Process(double** inputs, double** outputs, int nFrames)
{
  AttachBuffers(ERoute::kInput, 0, NChannelsConnected(ERoute::kInput), inputs, nFrames);
  AttachBuffers(ERoute::kOutput, 0, NChannelsConnected(ERoute::kOutput), outputs, nFrames);

  while (!mMidiQueue.Empty())
  {
    IMidiMsg& msg = mMidiQueue.Peek();
    if (msg.mOffset < nFrames)
    {
      ProcessMidiMsg(msg);
      mMidiQueue.Remove();
    }
    else
      break;
  }

  mMidiQueue.Flush(nFrames);

  ProcessBuffers(0.0, nFrames);
}

void IPlugCLI::ProcessSilence(int nFrames, double** outputs)
{
  int nOutCh = MaxNChannels(ERoute::kOutput);
  int nInCh = MaxNChannels(ERoute::kInput);

  std::vector<std::vector<double>> inputBuffers(nInCh);
  std::vector<double*> inputs(nInCh);
  for (int c = 0; c < nInCh; c++)
  {
    inputBuffers[c].resize(GetBlockSize(), 0.0);
    inputs[c] = inputBuffers[c].data();
  }

  int framesProcessed = 0;
  while (framesProcessed < nFrames)
  {
    int framesToProcess = std::min(GetBlockSize(), nFrames - framesProcessed);

    std::vector<double*> outputPtrs(nOutCh);
    for (int c = 0; c < nOutCh; c++)
    {
      outputPtrs[c] = outputs[c] + framesProcessed;
    }

    Process(inputs.data(), outputPtrs.data(), framesToProcess);
    framesProcessed += framesToProcess;
  }
}

void IPlugCLI::ProcessImpulse(int responseLength, std::vector<std::vector<double>>& output)
{
  int nOutCh = MaxNChannels(ERoute::kOutput);
  int nInCh = MaxNChannels(ERoute::kInput);

  output.resize(nOutCh);
  std::vector<double*> outputs(nOutCh);
  for (int c = 0; c < nOutCh; c++)
  {
    output[c].resize(responseLength, 0.0);
    outputs[c] = output[c].data();
  }

  std::vector<std::vector<double>> inputBuffers(nInCh);
  std::vector<double*> inputs(nInCh);
  for (int c = 0; c < nInCh; c++)
  {
    inputBuffers[c].resize(GetBlockSize(), 0.0);
    inputs[c] = inputBuffers[c].data();
  }

  int framesProcessed = 0;
  bool impulseApplied = false;

  while (framesProcessed < responseLength)
  {
    int framesToProcess = std::min(GetBlockSize(), responseLength - framesProcessed);

    if (!impulseApplied && nInCh > 0)
    {
      inputBuffers[0][0] = 1.0;
      impulseApplied = true;
    }

    std::vector<double*> outputPtrs(nOutCh);
    for (int c = 0; c < nOutCh; c++)
      outputPtrs[c] = outputs[c] + framesProcessed;

    Process(inputs.data(), outputPtrs.data(), framesToProcess);

    if (framesProcessed == 0 && nInCh > 0)
      inputBuffers[0][0] = 0.0;

    framesProcessed += framesToProcess;
  }
}

void IPlugCLI::ProcessSine(double frequency, int nFrames, std::vector<std::vector<double>>& output)
{
  int nOutCh = MaxNChannels(ERoute::kOutput);
  int nInCh = MaxNChannels(ERoute::kInput);

  output.resize(nOutCh);
  std::vector<double*> outputs(nOutCh);
  for (int c = 0; c < nOutCh; c++)
  {
    output[c].resize(nFrames, 0.0);
    outputs[c] = output[c].data();
  }

  std::vector<std::vector<double>> inputBuffers(nInCh);
  std::vector<double*> inputs(nInCh);
  for (int c = 0; c < nInCh; c++)
  {
    inputBuffers[c].resize(GetBlockSize(), 0.0);
    inputs[c] = inputBuffers[c].data();
  }

  double phase = 0.0;
  double phaseInc = 2.0 * M_PI * frequency / GetSampleRate();
  int framesProcessed = 0;

  while (framesProcessed < nFrames)
  {
    int framesToProcess = std::min(GetBlockSize(), nFrames - framesProcessed);

    if (nInCh > 0)
    {
      for (int i = 0; i < framesToProcess; i++)
      {
        inputBuffers[0][i] = std::sin(phase);
        phase = std::fmod(phase + phaseInc, 2.0 * M_PI);
      }
    }

    std::vector<double*> outputPtrs(nOutCh);
    for (int c = 0; c < nOutCh; c++)
      outputPtrs[c] = outputs[c] + framesProcessed;

    Process(inputs.data(), outputPtrs.data(), framesToProcess);
    framesProcessed += framesToProcess;
  }
}

void IPlugCLI::ProcessNoise(int nFrames, std::vector<std::vector<double>>& output)
{
  int nOutCh = MaxNChannels(ERoute::kOutput);
  int nInCh = MaxNChannels(ERoute::kInput);

  output.resize(nOutCh);
  std::vector<double*> outputs(nOutCh);
  for (int c = 0; c < nOutCh; c++)
  {
    output[c].resize(nFrames, 0.0);
    outputs[c] = output[c].data();
  }

  std::vector<std::vector<double>> inputBuffers(nInCh);
  std::vector<double*> inputs(nInCh);
  for (int c = 0; c < nInCh; c++)
  {
    inputBuffers[c].resize(GetBlockSize(), 0.0);
    inputs[c] = inputBuffers[c].data();
  }

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_real_distribution<> dist(-1.0, 1.0);

  int framesProcessed = 0;

  while (framesProcessed < nFrames)
  {
    int framesToProcess = std::min(GetBlockSize(), nFrames - framesProcessed);

    if (nInCh > 0)
    {
      for (int i = 0; i < framesToProcess; i++)
        inputBuffers[0][i] = dist(gen);
    }

    std::vector<double*> outputPtrs(nOutCh);
    for (int c = 0; c < nOutCh; c++)
      outputPtrs[c] = outputs[c] + framesProcessed;

    Process(inputs.data(), outputPtrs.data(), framesToProcess);
    framesProcessed += framesToProcess;
  }
}

void IPlugCLI::ProcessStep(int nFrames, std::vector<std::vector<double>>& output)
{
  int nOutCh = MaxNChannels(ERoute::kOutput);
  int nInCh = MaxNChannels(ERoute::kInput);

  output.resize(nOutCh);
  std::vector<double*> outputs(nOutCh);
  for (int c = 0; c < nOutCh; c++)
  {
    output[c].resize(nFrames, 0.0);
    outputs[c] = output[c].data();
  }

  std::vector<std::vector<double>> inputBuffers(nInCh);
  std::vector<double*> inputs(nInCh);
  for (int c = 0; c < nInCh; c++)
  {
    inputBuffers[c].resize(GetBlockSize(), 1.0);
    inputs[c] = inputBuffers[c].data();
  }

  int framesProcessed = 0;

  while (framesProcessed < nFrames)
  {
    int framesToProcess = std::min(GetBlockSize(), nFrames - framesProcessed);

    std::vector<double*> outputPtrs(nOutCh);
    for (int c = 0; c < nOutCh; c++)
      outputPtrs[c] = outputs[c] + framesProcessed;

    Process(inputs.data(), outputPtrs.data(), framesToProcess);
    framesProcessed += framesToProcess;
  }
}

void IPlugCLI::ProcessChirp(double startFreq, double endFreq, int nFrames, std::vector<std::vector<double>>& output)
{
  int nOutCh = MaxNChannels(ERoute::kOutput);
  int nInCh = MaxNChannels(ERoute::kInput);

  output.resize(nOutCh);
  std::vector<double*> outputs(nOutCh);
  for (int c = 0; c < nOutCh; c++)
  {
    output[c].resize(nFrames, 0.0);
    outputs[c] = output[c].data();
  }

  std::vector<std::vector<double>> inputBuffers(nInCh);
  std::vector<double*> inputs(nInCh);
  for (int c = 0; c < nInCh; c++)
  {
    inputBuffers[c].resize(GetBlockSize(), 0.0);
    inputs[c] = inputBuffers[c].data();
  }

  double sr = GetSampleRate();
  double duration = nFrames / sr;
  double k = std::pow(endFreq / startFreq, 1.0 / duration);

  double phase = 0.0;
  int globalSample = 0;
  int framesProcessed = 0;

  while (framesProcessed < nFrames)
  {
    int framesToProcess = std::min(GetBlockSize(), nFrames - framesProcessed);

    if (nInCh > 0)
    {
      for (int i = 0; i < framesToProcess; i++)
      {
        double t = globalSample / sr;
        double instantFreq = startFreq * std::pow(k, t);
        double phaseInc = 2.0 * M_PI * instantFreq / sr;
        inputBuffers[0][i] = std::sin(phase);
        phase = std::fmod(phase + phaseInc, 2.0 * M_PI);
        globalSample++;
      }
    }

    std::vector<double*> outputPtrs(nOutCh);
    for (int c = 0; c < nOutCh; c++)
      outputPtrs[c] = outputs[c] + framesProcessed;

    Process(inputs.data(), outputPtrs.data(), framesToProcess);
    framesProcessed += framesToProcess;
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
