/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests the plug-in lifecycle as driven by a host: construction, (re)activation, processing, idle and destruction.

#include <memory>
#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "AudioBuffers.h"
#include "TestPlugins.h"

using namespace iplug;

TEST_CASE("Lifecycle: nothing is called back during construction", "[integration][lifecycle]")
{
  GainPlug plug;

  CHECK(plug.mNumResets == 0);
  CHECK(plug.mActivations.empty());
  CHECK(plug.mParamChanges.empty());
  CHECK(plug.mParamChangesUI.empty());
  CHECK(plug.mNumIdleCalls == 0);
  CHECK(plug.mNumProcessCalls == 0);
  CHECK(plug.mParamEvents.empty());
}

TEST_CASE("Lifecycle: activation resets the plug-in with the host's sample rate and block size", "[integration][lifecycle]")
{
  GainPlug plug;
  plug.HostActivate(44100., 256);

  CHECK(plug.mActivations == std::vector<bool>{true});
  CHECK(plug.mNumResets == 1);
  CHECK(plug.mSampleRateAtReset == 44100.);
  CHECK(plug.mBlockSizeAtReset == 256);
  CHECK(plug.GetSampleRate() == 44100.);
  CHECK(plug.GetBlockSize() == 256);

  plug.HostDeactivate();
  CHECK(plug.mActivations == std::vector<bool>{true, false});
  CHECK(plug.mNumResets == 1);
}

TEST_CASE("Lifecycle: reactivating with a different sample rate and block size", "[integration][lifecycle]")
{
  GainPlug plug;

  struct Setup
  {
    double mSampleRate;
    int mBlockSize;
  };

  const Setup setups[] = {{44100., 64}, {96000., 2048}, {48000., 1}, {192000., 4096}, {22050., 333}};
  int nActivations = 0;

  for (const Setup& setup : setups)
  {
    INFO("sample rate " << setup.mSampleRate << ", block size " << setup.mBlockSize);

    plug.HostActivate(setup.mSampleRate, setup.mBlockSize);
    nActivations++;

    CHECK(plug.mNumResets == nActivations);
    CHECK(plug.mSampleRateAtReset == setup.mSampleRate);
    CHECK(plug.mBlockSizeAtReset == setup.mBlockSize);
    CHECK(plug.GetSampleRate() == setup.mSampleRate);
    CHECK(plug.GetBlockSize() == setup.mBlockSize);

    // The plug-in must be able to process a full block straight away
    AudioBuffers<float> in {2, setup.mBlockSize, 0.25f}, out {2, setup.mBlockSize};
    plug.HostProcess(in.Get(), out.Get(), setup.mBlockSize);

    CHECK(plug.mLastNFrames == setup.mBlockSize);
    CHECK(out[0][0] == 0.25f);
    CHECK(out[1][static_cast<size_t>(setup.mBlockSize - 1)] == 0.25f);

    plug.HostDeactivate();
  }

  CHECK(plug.mNumProcessCalls == nActivations);
}

TEST_CASE("Lifecycle: a full session", "[integration][lifecycle]")
{
  auto pPlug = std::make_unique<GainPlug>();
  GainPlug& plug = *pPlug;

  plug.SetHost("Reaper", 0x00070000);
  plug.HostActivate(48000., 64);
  plug.HostSetParameterNormalized(GainPlug::kGain, 0.25); // 50%

  AudioBuffers<float> in {2, 64, 1.f}, out {2, 64};

  for (int block = 0; block < 10; block++)
  {
    plug.HostProcess(in.Get(), out.Get(), 64);
    plug.HostIdle();
  }

  CHECK(plug.mNumProcessCalls == 10);
  CHECK(plug.mNumIdleCalls == 10);
  CHECK(out[0][0] == 0.5f);
  CHECK(out[1][63] == 0.5f);

  IByteChunk state;
  REQUIRE(plug.HostSaveState(state));
  CHECK(state.Size() > 0);

  plug.HostDeactivate();
  pPlug.reset();

  // A new instance picks up where the old one left off
  GainPlug restored;
  CHECK(restored.HostLoadState(state) == state.Size());
  restored.HostActivate(48000., 64);
  restored.HostProcess(in.Get(), out.Get(), 64);
  CHECK(out[0][0] == 0.5f);
}

TEST_CASE("Lifecycle: instances are independent", "[integration][lifecycle]")
{
  GainPlug a, b;

  a.HostActivate(44100., 32);
  b.HostActivate(96000., 128);
  a.HostSetParameterNormalized(GainPlug::kGain, 0.);

  CHECK(a.GetSampleRate() == 44100.);
  CHECK(b.GetSampleRate() == 96000.);
  CHECK(a.GetBlockSize() == 32);
  CHECK(b.GetBlockSize() == 128);
  CHECK(a.GetParam(GainPlug::kGain)->Value() == 0.);
  CHECK(b.GetParam(GainPlug::kGain)->Value() == 100.);
  CHECK(a.mParamChanges.size() == 1);
  CHECK(b.mParamChanges.empty());

  AudioBuffers<double> in {2, 32, 1.}, outA {2, 32, 9.}, outB {2, 32, 9.};
  a.HostProcess(in.Get(), outA.Get(), 32);
  b.HostProcess(in.Get(), outB.Get(), 32);

  CHECK(outA[0][0] == 0.);
  CHECK(outB[0][0] == 1.);
}

TEST_CASE("Lifecycle: the idle call runs without any processing having happened", "[integration][lifecycle]")
{
  GainPlug plug;

  plug.HostIdle();
  plug.HostIdle();

  CHECK(plug.mNumIdleCalls == 2);
  CHECK(plug.mParamChangesUI.empty());
  CHECK(plug.mMidiToUI.empty());
  CHECK(plug.mMessages.empty());
}

TEST_CASE("Lifecycle: a plug-in can be destroyed while active", "[integration][lifecycle]")
{
  auto pPlug = std::make_unique<SynthPlug>();
  pPlug->HostActivate(48000., 64);

  IMidiMsg msg;
  msg.MakeNoteOnMsg(60, 100, 0);
  pPlug->HostSendMidi(msg);

  AudioBuffers<float> in {0, 64}, out {2, 64};
  pPlug->HostProcess(in.Get(), out.Get(), 64);
  CHECK(out[0][63] > 0.f);

  pPlug.reset(); // no deactivate, with a held note and queued processor -> editor MIDI
  SUCCEED();
}
