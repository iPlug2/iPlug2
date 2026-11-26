/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Tests saving and restoring plug-in state: parameters, custom chunk data, and truncated or corrupt chunks.

#include <cstdint>
#include <limits>
#include <string>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "AudioBuffers.h"
#include "TestPlugins.h"

using namespace iplug;
using Catch::Approx;

namespace
{

constexpr int kDoubleSize = static_cast<int>(sizeof(double));

/** @return A copy of the first nBytes of a chunk */
IByteChunk Truncated(const IByteChunk& chunk, int nBytes)
{
  IByteChunk result;

  if (nBytes > 0) // IByteChunk::PutBytes() calls memcpy() on a null pointer when asked to add nothing to an empty chunk
    result.PutBytes(chunk.GetData(), nBytes);

  return result;
}

/** A small deterministic pseudo random byte generator, so the "garbage" is the same on every platform */
class ByteGenerator
{
public:
  explicit ByteGenerator(uint32_t seed) : mState(seed) {}

  uint8_t Next()
  {
    mState = mState * 1664525u + 1013904223u;
    return static_cast<uint8_t>(mState >> 24);
  }

private:
  uint32_t mState;
};

void SetCustomState(CustomStatePlug& plug)
{
  plug.GetParam(CustomStatePlug::kParamA)->Set(7.5);
  plug.GetParam(CustomStatePlug::kParamB)->Set(0.125);
  plug.GetParam(CustomStatePlug::kParamC)->Set(9.);
  plug.mText = "a string of custom state";
  plug.mCounter = -123456;
  plug.mTable[0] = 1.5;
  plug.mTable[1] = -2.5;
  plug.mTable[2] = 1e-9;
  plug.mTable[3] = 1e9;
}

} // namespace

TEST_CASE("State: SerializeParams() writes each parameter value as a double", "[integration][state]")
{
  GainPlug plug;
  plug.GetParam(GainPlug::kGain)->Set(42.5);
  plug.GetParam(GainPlug::kMute)->Set(1.);
  plug.GetParam(GainPlug::kMode)->Set(2.);

  IByteChunk chunk;
  REQUIRE(plug.SerializeParams(chunk));
  REQUIRE(chunk.Size() == 3 * kDoubleSize);

  double values[3] = {};
  int pos = 0;

  for (double& value : values)
    pos = chunk.Get(&value, pos);

  CHECK(pos == chunk.Size());
  CHECK(values[0] == 42.5);
  CHECK(values[1] == 1.);
  CHECK(values[2] == 2.);

  SECTION("it appends to a chunk which already has data in it")
  {
    REQUIRE(plug.SerializeParams(chunk));
    CHECK(chunk.Size() == 6 * kDoubleSize);

    double value = 0.;
    chunk.Get(&value, 3 * kDoubleSize);
    CHECK(value == 42.5);
  }

  SECTION("the default SerializeState() is the same thing")
  {
    IByteChunk state;
    REQUIRE(plug.SerializeState(state));
    CHECK(state.IsEqual(chunk));
  }
}

TEST_CASE("State: UnserializeParams() round trip", "[integration][state]")
{
  GainPlug source, dest;
  source.GetParam(GainPlug::kGain)->Set(42.5);
  source.GetParam(GainPlug::kMute)->Set(1.);
  source.GetParam(GainPlug::kMode)->Set(2.);

  IByteChunk chunk;
  REQUIRE(source.SerializeParams(chunk));

  const int pos = dest.UnserializeParams(chunk, 0);

  CHECK(pos == chunk.Size());
  CHECK(dest.GetParam(GainPlug::kGain)->Value() == 42.5);
  CHECK(dest.GetParam(GainPlug::kMute)->Bool());
  CHECK(dest.GetParam(GainPlug::kMode)->Int() == 2);

  SECTION("DSP and UI are told about every parameter, as a preset recall")
  {
    REQUIRE(dest.mParamChanges.size() == 3);
    REQUIRE(dest.mParamChangesUI.size() == 3);

    for (int i = 0; i < 3; i++)
    {
      CHECK(dest.mParamChanges[static_cast<size_t>(i)].mIdx == i);
      CHECK(dest.mParamChanges[static_cast<size_t>(i)].mSource == kPresetRecall);
      CHECK(dest.mParamChangesUI[static_cast<size_t>(i)].mIdx == i);
      CHECK(dest.mParamChangesUI[static_cast<size_t>(i)].mSource == kPresetRecall);
    }

    CHECK(dest.mParamChanges[0].mValue == 42.5);
  }

  SECTION("the host is not sent parameter changes")
  {
    CHECK(dest.mParamEvents.empty());
  }
}

TEST_CASE("State: UnserializeParams() from a position within a chunk", "[integration][state]")
{
  GainPlug source, dest;
  source.GetParam(GainPlug::kGain)->Set(150.);

  IByteChunk chunk;
  const int header = 0x12345678;
  chunk.Put(&header);
  chunk.PutStr("some other data");
  const int startPos = chunk.Size();
  REQUIRE(source.SerializeParams(chunk));
  const int endPos = chunk.Size();
  const int footer = 0x0BADF00D;
  chunk.Put(&footer);

  CHECK(dest.UnserializeParams(chunk, startPos) == endPos);
  CHECK(dest.GetParam(GainPlug::kGain)->Value() == 150.);
}

TEST_CASE("State: restored values are constrained to the parameter's range", "[integration][state]")
{
  GainPlug plug;

  IByteChunk chunk;
  const double values[3] = {1e9, -5., 7.4};

  for (const double& value : values)
    chunk.Put(&value);

  CHECK(plug.UnserializeParams(chunk, 0) == chunk.Size());
  CHECK(plug.GetParam(GainPlug::kGain)->Value() == 200.);
  CHECK(plug.GetParam(GainPlug::kMute)->Value() == 0.);
  CHECK(plug.GetParam(GainPlug::kMode)->Value() == 2.);

  SECTION("infinities too")
  {
    IByteChunk infChunk;
    const double infValues[3] = {-std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity()};

    for (const double& value : infValues)
      infChunk.Put(&value);

    CHECK(plug.UnserializeParams(infChunk, 0) == infChunk.Size());
    CHECK(plug.GetParam(GainPlug::kGain)->Value() == 0.);
    CHECK(plug.GetParam(GainPlug::kMute)->Value() == 1.);
    CHECK(plug.GetParam(GainPlug::kMode)->Value() == 2.);
  }
}

TEST_CASE("State: round trip between two instances", "[integration][state]")
{
  GainPlug source, dest;

  source.HostSetParameterNormalized(GainPlug::kGain, 0.125);
  source.HostSetParameterNormalized(GainPlug::kMute, 1.);
  source.HostSetParameterNormalized(GainPlug::kMode, 0.5);

  dest.HostSetParameterNormalized(GainPlug::kGain, 0.9);
  dest.HostSetParameterNormalized(GainPlug::kMode, 1.);
  dest.HostIdle();
  dest.ClearAll();

  REQUIRE(dest.GetParam(GainPlug::kGain)->Value() != source.GetParam(GainPlug::kGain)->Value());
  REQUIRE(dest.GetParam(GainPlug::kMute)->Value() != source.GetParam(GainPlug::kMute)->Value());
  REQUIRE(dest.GetParam(GainPlug::kMode)->Value() != source.GetParam(GainPlug::kMode)->Value());

  IByteChunk state;
  REQUIRE(source.HostSaveState(state));
  CHECK(dest.HostLoadState(state) == state.Size());

  for (int i = 0; i < GainPlug::kNumParams; i++)
    CHECK(dest.GetParam(i)->Value() == source.GetParam(i)->Value());

  SECTION("saving the restored instance gives an identical chunk")
  {
    IByteChunk resaved;
    REQUIRE(dest.HostSaveState(resaved));
    CHECK(resaved.IsEqual(state));
  }

  SECTION("OnRestoreState() is called once, and sends the new values to the UI")
  {
    CHECK(dest.mNumRestoreStateCalls == 1);

    REQUIRE(dest.mParamValuesToUI.size() == 3);

    for (int i = 0; i < 3; i++)
    {
      const auto& toUI = dest.mParamValuesToUI[static_cast<size_t>(i)];
      CHECK(toUI.mIdx == i);
      CHECK(toUI.mNormalized);
      CHECK(toUI.mValue == Approx(source.GetParam(i)->GetNormalized()));
    }
  }

  SECTION("the restored instance sounds the same")
  {
    source.HostSetParameterNormalized(GainPlug::kMute, 0.);
    state.Clear();
    REQUIRE(source.HostSaveState(state));
    REQUIRE(dest.HostLoadState(state) > 0);

    source.HostActivate(48000., 16);
    dest.HostActivate(48000., 16);

    AudioBuffers<double> in {2, 16, 0.5}, outSource {2, 16}, outDest {2, 16};
    source.HostProcess(in.Get(), outSource.Get(), 16);
    dest.HostProcess(in.Get(), outDest.Get(), 16);

    CHECK(outSource[0][0] == Approx(0.125));
    CHECK(outDest[0][0] == outSource[0][0]);
    CHECK(outDest[1][15] == outSource[1][15]);
  }

  SECTION("the source is not modified by saving")
  {
    CHECK(source.GetParam(GainPlug::kGain)->Value() == Approx(25.));
    CHECK(source.mNumRestoreStateCalls == 0);
  }
}

TEST_CASE("State: custom state after the parameters", "[integration][state]")
{
  CustomStatePlug source, dest;
  SetCustomState(source);

  IByteChunk state;
  REQUIRE(source.HostSaveState(state));

  // Params, then a length-prefixed string, an int and a table of doubles
  const int expectedSize = 3 * kDoubleSize + static_cast<int>(sizeof(int)) + static_cast<int>(source.mText.size()) + static_cast<int>(sizeof(int)) + CustomStatePlug::kTableSize * kDoubleSize;
  CHECK(state.Size() == expectedSize);

  SECTION("everything is restored in another instance")
  {
    CHECK(dest.HostLoadState(state) == state.Size());

    CHECK(dest.GetParam(CustomStatePlug::kParamA)->Value() == 7.5);
    CHECK(dest.GetParam(CustomStatePlug::kParamB)->Value() == 0.125);
    CHECK(dest.GetParam(CustomStatePlug::kParamC)->Int() == 9);
    CHECK(dest.mText == "a string of custom state");
    CHECK(dest.mCounter == -123456);

    for (int i = 0; i < CustomStatePlug::kTableSize; i++)
      CHECK(dest.mTable[i] == source.mTable[i]);

    CHECK(dest.mNumRestoreStateCalls == 1);

    IByteChunk resaved;
    REQUIRE(dest.HostSaveState(resaved));
    CHECK(resaved.IsEqual(state));
  }

  SECTION("an empty string survives the round trip")
  {
    source.mText.clear();
    IByteChunk emptyTextState;
    REQUIRE(source.HostSaveState(emptyTextState));

    dest.mText = "not empty";
    CHECK(dest.HostLoadState(emptyTextState) == emptyTextState.Size());
    CHECK(dest.mText.empty());
    CHECK(dest.mCounter == -123456);
  }

  SECTION("state can be restored from the middle of a larger chunk")
  {
    IByteChunk container;
    const int64_t header = 0x1122334455667788;
    container.Put(&header);
    const int startPos = container.Size();
    container.PutChunk(&state);
    const int endPos = container.Size();
    container.Put(&header);

    CHECK(dest.UnserializeState(container, startPos) == endPos);
    CHECK(dest.mText == source.mText);
    CHECK(dest.mTable[3] == 1e9);
  }

  SECTION("trailing data after the state is ignored")
  {
    IByteChunk longer;
    longer.PutChunk(&state);
    longer.PutStr("extra data from a newer version");

    CHECK(dest.HostLoadState(longer) == state.Size());
    CHECK(dest.mText == source.mText);
  }
}

TEST_CASE("State: a truncated parameter chunk fails without touching the remaining parameters", "[integration][state][robustness]")
{
  GainPlug source;
  source.GetParam(GainPlug::kGain)->Set(11.);
  source.GetParam(GainPlug::kMute)->Set(1.);
  source.GetParam(GainPlug::kMode)->Set(2.);

  IByteChunk state;
  REQUIRE(source.HostSaveState(state));

  for (int nBytes = 0; nBytes < state.Size(); nBytes++)
  {
    INFO("truncated to " << nBytes << " bytes");

    GainPlug dest;
    const IByteChunk truncated = Truncated(state, nBytes);

    CHECK(dest.HostLoadState(truncated) < 0);

    // Parameters which were completely present are restored, the others keep the values they had
    const int nComplete = nBytes / kDoubleSize;
    CHECK(dest.GetParam(GainPlug::kGain)->Value() == (nComplete > 0 ? 11. : 100.));
    CHECK(dest.GetParam(GainPlug::kMute)->Value() == (nComplete > 1 ? 1. : 0.));
    CHECK(dest.GetParam(GainPlug::kMode)->Value() == 0.);
  }

  SECTION("an empty chunk")
  {
    GainPlug dest;
    IByteChunk empty;
    CHECK(dest.HostLoadState(empty) < 0);
    CHECK(dest.GetParam(GainPlug::kGain)->Value() == 100.);
  }

  SECTION("a start position beyond the end of the chunk")
  {
    GainPlug dest;
    CHECK(dest.UnserializeState(state, state.Size()) < 0);
    CHECK(dest.UnserializeState(state, state.Size() + 100) < 0);
    CHECK(dest.UnserializeState(state, state.Size() - 1) < 0);
    CHECK(dest.GetParam(GainPlug::kGain)->Value() == 100.);
  }

  SECTION("a negative start position")
  {
    GainPlug dest;
    CHECK(dest.UnserializeState(state, -1) < 0);
    CHECK(dest.UnserializeState(state, -kDoubleSize) < 0);
    CHECK(dest.GetParam(GainPlug::kGain)->Value() == 100.);
  }
}

TEST_CASE("State: truncated custom state fails at every length", "[integration][state][robustness]")
{
  CustomStatePlug source;
  SetCustomState(source);

  IByteChunk state;
  REQUIRE(source.HostSaveState(state));

  for (int nBytes = 0; nBytes < state.Size(); nBytes++)
  {
    INFO("truncated to " << nBytes << " bytes");

    CustomStatePlug dest;
    const IByteChunk truncated = Truncated(state, nBytes);

    CHECK(dest.HostLoadState(truncated) < 0);

    // The plug-in only commits its custom data if all of it could be read
    CHECK(dest.mText == "default");
    CHECK(dest.mCounter == 0);
    CHECK(dest.mTable[0] == 0.);
  }
}

TEST_CASE("State: garbage chunks do not crash or leave parameters out of range", "[integration][state][robustness]")
{
  SECTION("parameter-only state, random bytes of many sizes")
  {
    ByteGenerator generator {12345};

    for (int nBytes = 0; nBytes <= 64; nBytes++)
    {
      INFO("size " << nBytes);

      IByteChunk garbage;

      for (int i = 0; i < nBytes; i++)
      {
        const uint8_t byte = generator.Next();
        garbage.Put(&byte);
      }

      GainPlug plug;
      const int pos = plug.HostLoadState(garbage);

      if (nBytes < 3 * kDoubleSize)
        CHECK(pos < 0);
      else
        CHECK(pos == 3 * kDoubleSize);
    }
  }

  SECTION("parameter-only state, extreme finite values")
  {
    const double extremes[] = {std::numeric_limits<double>::max(), std::numeric_limits<double>::lowest(), std::numeric_limits<double>::min(),
                               std::numeric_limits<double>::denorm_min(), -0., 1e300, -1e300, 0.4999999, 0.5, 1.5};

    for (double a : extremes)
    {
      for (double b : extremes)
      {
        IByteChunk chunk;
        chunk.Put(&a);
        chunk.Put(&b);
        chunk.Put(&a);

        GainPlug plug;
        REQUIRE(plug.HostLoadState(chunk) == chunk.Size());

        for (int i = 0; i < GainPlug::kNumParams; i++)
        {
          const IParam* pParam = plug.GetParam(i);
          CHECK(pParam->Value() >= pParam->GetMin());
          CHECK(pParam->Value() <= pParam->GetMax());
        }

        // Still usable afterwards
        plug.HostActivate(48000., 8);
        AudioBuffers<float> in {2, 8, 0.5f}, out {2, 8};
        plug.HostProcess(in.Get(), out.Get(), 8);
        CHECK(out[0][0] >= 0.f);
        CHECK(out[0][0] <= 1.f);
      }
    }
  }

  SECTION("custom state, random bytes after valid parameters")
  {
    ByteGenerator generator {987654321};

    for (int repeat = 0; repeat < 200; repeat++)
    {
      INFO("repeat " << repeat);

      CustomStatePlug source, dest;
      IByteChunk chunk;
      REQUIRE(source.SerializeParams(chunk));

      const int nGarbageBytes = generator.Next() % 96;

      for (int i = 0; i < nGarbageBytes; i++)
      {
        // Keep the top byte of the string length small, so the length stays a plausible, if wrong, number
        const uint8_t byte = (i == 3) ? static_cast<uint8_t>(generator.Next() & 0x01) : generator.Next();
        chunk.Put(&byte);
      }

      const int pos = dest.HostLoadState(chunk);

      // Whatever happened, the result is either a failure or a position inside the chunk
      CHECK(pos <= chunk.Size());

      if (pos < 0)
      {
        CHECK(dest.mText == "default");
        CHECK(dest.mCounter == 0);
      }
    }
  }
}

// BUG: IByteGetter::GetStr() (IPlug/IPlugStructs.h) is documented to return -1 "if the copy would have copied more
// data than in the src buffer". When the string's length prefix can be read but the string itself runs past the end of
// the data, it leaves the output string untouched and returns the would-be end position (which is beyond the end of the
// chunk) rather than -1, so a caller checking for a negative position takes a truncated string for a success.
// (The same function also computes `strStartPos + len` with a length read from the chunk, which is a signed integer
// overflow for a corrupt, very large length.)
TEST_CASE("State: reading a string from a truncated chunk fails", "[.][bug]")
{
  IByteChunk chunk;
  chunk.PutStr("hello world");

  const IByteChunk truncated = Truncated(chunk, chunk.Size() - 3);

  WDL_String str("untouched");
  CHECK(truncated.GetStr(str, 0) == -1);
}

TEST_CASE("State: CompareState() compares incoming parameter values with the current ones", "[integration][state]")
{
  GainPlug plug;
  plug.GetParam(GainPlug::kGain)->Set(33.);
  plug.GetParam(GainPlug::kMode)->Set(1.);

  IByteChunk state;
  REQUIRE(plug.SerializeState(state));

  CHECK(plug.CompareState(state.GetData(), 0));

  SECTION("a changed parameter makes the states differ")
  {
    plug.GetParam(GainPlug::kGain)->Set(34.);
    CHECK_FALSE(plug.CompareState(state.GetData(), 0));

    plug.GetParam(GainPlug::kGain)->Set(33.);
    CHECK(plug.CompareState(state.GetData(), 0));
  }

  SECTION("the last parameter is compared too")
  {
    plug.GetParam(GainPlug::kMode)->Set(2.);
    CHECK_FALSE(plug.CompareState(state.GetData(), 0));
  }

  SECTION("differences below single precision are ignored")
  {
    plug.GetParam(GainPlug::kGain)->Set(33.0000001);
    CHECK(plug.CompareState(state.GetData(), 0));
  }
}

TEST_CASE("State: CompareState() takes its start position in bytes", "[integration][state]")
{
  GainPlug plug;
  plug.GetParam(GainPlug::kGain)->Set(33.);

  // A one double header, then the state
  IByteChunk chunk;
  const double header = 12345.;
  chunk.Put(&header);
  const int startPos = chunk.Size();
  REQUIRE(plug.SerializeState(chunk));

  CHECK(plug.CompareState(chunk.GetData(), startPos));
}

TEST_CASE("State: the IPlug version header helpers", "[integration][state]")
{
  GainPlug source, dest;
  source.GetParam(GainPlug::kGain)->Set(77.);

  SECTION("a chunk with a version header")
  {
    IByteChunk chunk;
    IByteChunk::InitChunkWithIPlugVer(chunk);
    const int headerSize = chunk.Size();
    CHECK(headerSize == 2 * static_cast<int>(sizeof(int)));
    REQUIRE(source.SerializeState(chunk));

    int pos = 0;
    CHECK(IByteChunk::GetIPlugVerFromChunk(chunk, pos) == IPLUG_VERSION);
    CHECK(pos == headerSize);

    CHECK(dest.UnserializeState(chunk, pos) == chunk.Size());
    CHECK(dest.GetParam(GainPlug::kGain)->Value() == 77.);
  }

  SECTION("InitChunkWithIPlugVer() discards what was in the chunk")
  {
    IByteChunk chunk;
    chunk.PutStr("old contents");
    IByteChunk::InitChunkWithIPlugVer(chunk);
    CHECK(chunk.Size() == 2 * static_cast<int>(sizeof(int)));
  }

  SECTION("a chunk without a version header: version 0 and the position is not moved")
  {
    IByteChunk chunk;
    REQUIRE(source.SerializeState(chunk));

    int pos = 0;
    CHECK(IByteChunk::GetIPlugVerFromChunk(chunk, pos) == 0);
    CHECK(pos == 0);

    CHECK(dest.UnserializeState(chunk, pos) == chunk.Size());
    CHECK(dest.GetParam(GainPlug::kGain)->Value() == 77.);
  }

  SECTION("an empty chunk")
  {
    IByteChunk chunk;
    int pos = 0;
    CHECK(IByteChunk::GetIPlugVerFromChunk(chunk, pos) == 0);
    CHECK(pos == 0);
  }
}

TEST_CASE("State: editor and VST3 controller state hooks do nothing by default", "[integration][state]")
{
  GainPlug plug;
  IByteChunk chunk;

  CHECK(plug.SerializeEditorState(chunk));
  CHECK(plug.SerializeVST3CtrlrState(chunk));
  CHECK(chunk.Size() == 0);

  chunk.PutStr("something");
  CHECK(plug.UnserializeEditorState(chunk, 3) == 3);
  CHECK(plug.UnserializeVST3CtrlrState(chunk, 5) == 5);
  CHECK(plug.mParamChanges.empty());
}

TEST_CASE("State: a plug-in without parameters has an empty state", "[integration][state]")
{
  RecordingPlug plug {InstanceInfo{}, TestConfig{}};

  IByteChunk state;
  CHECK(plug.HostSaveState(state));
  CHECK(state.Size() == 0);
  CHECK(plug.HostLoadState(state) == 0);
  CHECK(plug.mNumRestoreStateCalls == 1);
}
