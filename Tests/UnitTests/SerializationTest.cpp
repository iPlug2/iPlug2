/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <cstring>
#include <vector>

#include "IPlugStructs.h"
#include "IPlugParameter.h"

using namespace iplug;
using Catch::Approx;

// ============================================================================
// IByteChunk Tests
// ============================================================================

TEST_CASE("IByteChunk basic operations", "[Serialization][IByteChunk]")
{
  SECTION("Default construction creates empty chunk")
  {
    IByteChunk chunk;
    REQUIRE(chunk.Size() == 0);
  }

  SECTION("PutBytes and GetBytes")
  {
    IByteChunk chunk;
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};

    chunk.PutBytes(data, 4);
    REQUIRE(chunk.Size() == 4);

    uint8_t result[4];
    int endPos = chunk.GetBytes(result, 4, 0);

    REQUIRE(endPos == 4);
    REQUIRE(memcmp(data, result, 4) == 0);
  }

  SECTION("Put and Get for primitive types")
  {
    IByteChunk chunk;

    int intVal = 42;
    double doubleVal = 3.14159;
    float floatVal = 2.5f;
    bool boolVal = true;

    chunk.Put(&intVal);
    chunk.Put(&doubleVal);
    chunk.Put(&floatVal);
    chunk.Put(&boolVal);

    int pos = 0;
    int intResult;
    double doubleResult;
    float floatResult;
    bool boolResult;

    pos = chunk.Get(&intResult, pos);
    REQUIRE(intResult == 42);

    pos = chunk.Get(&doubleResult, pos);
    REQUIRE(doubleResult == Approx(3.14159));

    pos = chunk.Get(&floatResult, pos);
    REQUIRE(floatResult == Approx(2.5f));

    pos = chunk.Get(&boolResult, pos);
    REQUIRE(boolResult == true);
  }

  SECTION("PutStr and GetStr")
  {
    IByteChunk chunk;

    chunk.PutStr("Hello, World!");

    WDL_String result;
    int endPos = chunk.GetStr(result, 0);

    REQUIRE(endPos > 0);
    REQUIRE(std::string(result.Get()) == "Hello, World!");
  }

  SECTION("Empty string handling")
  {
    IByteChunk chunk;
    chunk.PutStr("");

    WDL_String result;
    int endPos = chunk.GetStr(result, 0);

    REQUIRE(endPos > 0);
    REQUIRE(std::string(result.Get()) == "");
  }

  SECTION("Multiple strings")
  {
    IByteChunk chunk;

    chunk.PutStr("First");
    chunk.PutStr("Second");
    chunk.PutStr("Third");

    WDL_String str1, str2, str3;
    int pos = 0;
    pos = chunk.GetStr(str1, pos);
    pos = chunk.GetStr(str2, pos);
    pos = chunk.GetStr(str3, pos);

    REQUIRE(std::string(str1.Get()) == "First");
    REQUIRE(std::string(str2.Get()) == "Second");
    REQUIRE(std::string(str3.Get()) == "Third");
  }
}

TEST_CASE("IByteChunk Clear and Resize", "[Serialization][IByteChunk]")
{
  SECTION("Clear empties chunk")
  {
    IByteChunk chunk;
    int val = 123;
    chunk.Put(&val);
    REQUIRE(chunk.Size() > 0);

    chunk.Clear();
    REQUIRE(chunk.Size() == 0);
  }

  SECTION("Resize grows chunk")
  {
    IByteChunk chunk;
    int oldSize = chunk.Resize(100);

    REQUIRE(oldSize == 0);
    REQUIRE(chunk.Size() == 100);
  }

  SECTION("Resize shrinks chunk")
  {
    IByteChunk chunk;
    chunk.Resize(100);
    chunk.Resize(50);

    REQUIRE(chunk.Size() == 50);
  }

  SECTION("Resize zero-fills new space")
  {
    IByteChunk chunk;
    chunk.Resize(10);

    uint8_t* data = chunk.GetData();
    for (int i = 0; i < 10; i++)
    {
      REQUIRE(data[i] == 0);
    }
  }
}

TEST_CASE("IByteChunk PutChunk", "[Serialization][IByteChunk]")
{
  SECTION("Append another chunk")
  {
    IByteChunk chunk1, chunk2;

    int val1 = 100;
    int val2 = 200;

    chunk1.Put(&val1);
    chunk2.Put(&val2);

    chunk1.PutChunk(&chunk2);

    int result1, result2;
    int pos = 0;
    pos = chunk1.Get(&result1, pos);
    pos = chunk1.Get(&result2, pos);

    REQUIRE(result1 == 100);
    REQUIRE(result2 == 200);
  }
}

TEST_CASE("IByteChunk IsEqual", "[Serialization][IByteChunk]")
{
  SECTION("Equal chunks")
  {
    IByteChunk chunk1, chunk2;

    int val = 42;
    chunk1.Put(&val);
    chunk2.Put(&val);

    REQUIRE(chunk1.IsEqual(chunk2));
  }

  SECTION("Different size chunks are not equal")
  {
    IByteChunk chunk1, chunk2;

    int val = 42;
    chunk1.Put(&val);

    REQUIRE_FALSE(chunk1.IsEqual(chunk2));
  }

  SECTION("Same size but different content")
  {
    IByteChunk chunk1, chunk2;

    int val1 = 42;
    int val2 = 43;
    chunk1.Put(&val1);
    chunk2.Put(&val2);

    REQUIRE_FALSE(chunk1.IsEqual(chunk2));
  }
}

TEST_CASE("IByteChunk version handling", "[Serialization][IByteChunk]")
{
  SECTION("InitChunkWithIPlugVer and GetIPlugVerFromChunk")
  {
    IByteChunk chunk;
    IByteChunk::InitChunkWithIPlugVer(chunk);

    REQUIRE(chunk.Size() > 0);

    int pos = 0;
    int version = IByteChunk::GetIPlugVerFromChunk(chunk, pos);

    REQUIRE(version == IPLUG_VERSION);
    REQUIRE(pos > 0);
  }

  SECTION("GetIPlugVerFromChunk returns 0 for invalid chunk")
  {
    IByteChunk chunk;
    int garbage = 0x12345678;
    chunk.Put(&garbage);

    int pos = 0;
    int version = IByteChunk::GetIPlugVerFromChunk(chunk, pos);

    REQUIRE(version == 0);
  }
}

TEST_CASE("IByteChunk boundary conditions", "[Serialization][IByteChunk]")
{
  SECTION("GetBytes beyond chunk size returns -1")
  {
    IByteChunk chunk;
    int val = 42;
    chunk.Put(&val);

    uint8_t buf[100];
    int result = chunk.GetBytes(buf, 100, 0);

    REQUIRE(result == -1);
  }

  SECTION("GetBytes with negative start returns -1")
  {
    IByteChunk chunk;
    int val = 42;
    chunk.Put(&val);

    uint8_t buf[4];
    int result = chunk.GetBytes(buf, 4, -1);

    REQUIRE(result == -1);
  }
}

// ============================================================================
// IByteStream Tests
// ============================================================================

TEST_CASE("IByteStream basic operations", "[Serialization][IByteStream]")
{
  SECTION("Construction from raw data")
  {
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04};
    IByteStream stream(data, 4);

    REQUIRE(stream.Size() == 4);
  }

  SECTION("GetBytes from stream")
  {
    int originalVal = 12345;
    IByteStream stream(&originalVal, sizeof(int));

    int result;
    int endPos = stream.Get(&result, 0);

    REQUIRE(endPos == sizeof(int));
    REQUIRE(result == 12345);
  }

  SECTION("GetStr from stream")
  {
    // Create a chunk with a string, then read via stream
    IByteChunk chunk;
    chunk.PutStr("Test String");

    IByteStream stream(chunk.GetData(), chunk.Size());

    WDL_String result;
    int endPos = stream.GetStr(result, 0);

    REQUIRE(endPos > 0);
    REQUIRE(std::string(result.Get()) == "Test String");
  }

  SECTION("IsEqual comparison")
  {
    uint8_t data1[] = {0x01, 0x02, 0x03};
    uint8_t data2[] = {0x01, 0x02, 0x03};

    IByteStream stream1(data1, 3);
    IByteStream stream2(data2, 3);

    REQUIRE(stream1.IsEqual(stream2));
  }
}

// ============================================================================
// IByteChunkReader Tests
// ============================================================================

TEST_CASE("IByteChunkReader sequential reading", "[Serialization][IByteChunkReader]")
{
  SECTION("Sequential Get calls")
  {
    IByteChunk chunk;
    int val1 = 100, val2 = 200, val3 = 300;
    chunk.Put(&val1);
    chunk.Put(&val2);
    chunk.Put(&val3);

    IByteChunkReader reader(chunk);

    int result1, result2, result3;
    reader.Get(&result1);
    reader.Get(&result2);
    reader.Get(&result3);

    REQUIRE(result1 == 100);
    REQUIRE(result2 == 200);
    REQUIRE(result3 == 300);
  }

  SECTION("Tell returns current position")
  {
    IByteChunk chunk;
    int val = 42;
    chunk.Put(&val);

    IByteChunkReader reader(chunk);
    REQUIRE(reader.Tell() == 0);

    int result;
    reader.Get(&result);
    REQUIRE(reader.Tell() == sizeof(int));
  }

  SECTION("Seek sets position")
  {
    IByteChunk chunk;
    int val1 = 100, val2 = 200;
    chunk.Put(&val1);
    chunk.Put(&val2);

    IByteChunkReader reader(chunk);

    // Skip first int
    reader.Seek(sizeof(int));

    int result;
    reader.Get(&result);
    REQUIRE(result == 200);
  }

  SECTION("GetStr in reader")
  {
    IByteChunk chunk;
    chunk.PutStr("Hello");
    chunk.PutStr("World");

    IByteChunkReader reader(chunk);

    WDL_String str1, str2;
    reader.GetStr(str1);
    reader.GetStr(str2);

    REQUIRE(std::string(str1.Get()) == "Hello");
    REQUIRE(std::string(str2.Get()) == "World");
  }

  SECTION("Construction with start position")
  {
    IByteChunk chunk;
    int val1 = 100, val2 = 200;
    chunk.Put(&val1);
    chunk.Put(&val2);

    IByteChunkReader reader(chunk, sizeof(int)); // Start at second int

    int result;
    reader.Get(&result);
    REQUIRE(result == 200);
  }
}

// ============================================================================
// IPreset Tests
// ============================================================================

TEST_CASE("IPreset structure", "[Serialization][IPreset]")
{
  SECTION("Default construction")
  {
    IPreset preset;

    REQUIRE(preset.mInitialized == false);
    REQUIRE(std::string(preset.mName) == UNUSED_PRESET_NAME);
    REQUIRE(preset.mChunk.Size() == 0);
  }

  SECTION("Preset data storage")
  {
    IPreset preset;

    preset.mInitialized = true;
    strcpy(preset.mName, "Test Preset");

    double val = 0.75;
    preset.mChunk.Put(&val);

    REQUIRE(preset.mInitialized == true);
    REQUIRE(std::string(preset.mName) == "Test Preset");

    double result;
    preset.mChunk.Get(&result, 0);
    REQUIRE(result == Approx(0.75));
  }
}

// ============================================================================
// Serialization Roundtrip Tests
// ============================================================================

TEST_CASE("Serialization roundtrip with mixed types", "[Serialization]")
{
  SECTION("Complex data structure roundtrip")
  {
    IByteChunk chunk;

    // Write various types
    int intVal = -12345;
    double doubleVal = 3.14159265359;
    float floatVal = 2.71828f;
    bool boolVal = true;
    uint8_t bytes[] = {0xDE, 0xAD, 0xBE, 0xEF};

    chunk.Put(&intVal);
    chunk.Put(&doubleVal);
    chunk.Put(&floatVal);
    chunk.Put(&boolVal);
    chunk.PutBytes(bytes, 4);
    chunk.PutStr("Test string with spaces and numbers 123");

    // Read back
    IByteChunkReader reader(chunk);

    int intResult;
    double doubleResult;
    float floatResult;
    bool boolResult;
    uint8_t bytesResult[4];
    WDL_String strResult;

    reader.Get(&intResult);
    reader.Get(&doubleResult);
    reader.Get(&floatResult);
    reader.Get(&boolResult);
    reader.GetBytes(bytesResult, 4);
    reader.GetStr(strResult);

    REQUIRE(intResult == -12345);
    REQUIRE(doubleResult == Approx(3.14159265359));
    REQUIRE(floatResult == Approx(2.71828f));
    REQUIRE(boolResult == true);
    REQUIRE(memcmp(bytes, bytesResult, 4) == 0);
    REQUIRE(std::string(strResult.Get()) == "Test string with spaces and numbers 123");
  }
}

TEST_CASE("Parameter value serialization pattern", "[Serialization]")
{
  SECTION("Simulate parameter serialization/unserialization")
  {
    // Simulate what SerializeParams does
    std::vector<double> originalValues = {0.0, 0.25, 0.5, 0.75, 1.0, -10.0, 440.0};

    IByteChunk chunk;

    // Serialize (like SerializeParams)
    for (double v : originalValues)
    {
      chunk.Put(&v);
    }

    // Unserialize (like UnserializeParams)
    std::vector<double> restoredValues(originalValues.size());
    int pos = 0;
    for (size_t i = 0; i < restoredValues.size(); i++)
    {
      pos = chunk.Get(&restoredValues[i], pos);
      REQUIRE(pos > 0);
    }

    // Verify
    for (size_t i = 0; i < originalValues.size(); i++)
    {
      REQUIRE(restoredValues[i] == Approx(originalValues[i]));
    }
  }
}

TEST_CASE("IParam serialization", "[Serialization][IParam]")
{
  SECTION("Serialize and restore parameter value")
  {
    IParam param;
    param.InitDouble("Test", 0.5, 0.0, 1.0, 0.01);
    param.Set(0.75);

    // Serialize
    IByteChunk chunk;
    double val = param.Value();
    chunk.Put(&val);

    // Restore to new param
    IParam param2;
    param2.InitDouble("Test2", 0.0, 0.0, 1.0, 0.01);

    double restoredVal;
    chunk.Get(&restoredVal, 0);
    param2.Set(restoredVal);

    REQUIRE(param2.Value() == Approx(0.75));
  }

  SECTION("Serialize multiple parameter types")
  {
    IParam boolParam, intParam, enumParam, doubleParam;

    boolParam.InitBool("Bool", true);
    intParam.InitInt("Int", 5, 0, 10);
    enumParam.InitEnum("Enum", 2, {"A", "B", "C", "D"});
    doubleParam.InitDouble("Double", 0.333, 0.0, 1.0, 0.001);

    // Serialize
    IByteChunk chunk;
    double v1 = boolParam.Value();
    double v2 = intParam.Value();
    double v3 = enumParam.Value();
    double v4 = doubleParam.Value();

    chunk.Put(&v1);
    chunk.Put(&v2);
    chunk.Put(&v3);
    chunk.Put(&v4);

    // Create new params and restore
    IParam boolParam2, intParam2, enumParam2, doubleParam2;
    boolParam2.InitBool("Bool", false);
    intParam2.InitInt("Int", 0, 0, 10);
    enumParam2.InitEnum("Enum", 0, {"A", "B", "C", "D"});
    doubleParam2.InitDouble("Double", 0.0, 0.0, 1.0, 0.001);

    double r1, r2, r3, r4;
    int pos = 0;
    pos = chunk.Get(&r1, pos);
    pos = chunk.Get(&r2, pos);
    pos = chunk.Get(&r3, pos);
    pos = chunk.Get(&r4, pos);

    boolParam2.Set(r1);
    intParam2.Set(r2);
    enumParam2.Set(r3);
    doubleParam2.Set(r4);

    REQUIRE(boolParam2.Bool() == true);
    REQUIRE(intParam2.Int() == 5);
    REQUIRE(enumParam2.Int() == 2);
    REQUIRE(doubleParam2.Value() == Approx(0.333));
  }
}

// ============================================================================
// ParamTuple Tests
// ============================================================================

TEST_CASE("ParamTuple", "[Serialization][ParamTuple]")
{
  SECTION("Default construction")
  {
    ParamTuple tuple;
    REQUIRE(tuple.idx == kNoParameter);
    REQUIRE(tuple.value == 0.0);
  }

  SECTION("Parameterized construction")
  {
    ParamTuple tuple(5, 0.75);
    REQUIRE(tuple.idx == 5);
    REQUIRE(tuple.value == Approx(0.75));
  }
}

// ============================================================================
// SysExData Tests
// ============================================================================

TEST_CASE("SysExData", "[Serialization][SysExData]")
{
  SECTION("Default construction zeros data")
  {
    SysExData sysex;
    REQUIRE(sysex.mOffset == 0);
    REQUIRE(sysex.mSize == 0);

    // Data should be zeroed
    for (int i = 0; i < MAX_SYSEX_SIZE; i++)
    {
      REQUIRE(sysex.mData[i] == 0);
    }
  }

  SECTION("Construction with data")
  {
    uint8_t data[] = {0xF0, 0x41, 0x10, 0xF7};
    SysExData sysex(100, 4, data);

    REQUIRE(sysex.mOffset == 100);
    REQUIRE(sysex.mSize == 4);
    REQUIRE(memcmp(sysex.mData, data, 4) == 0);
  }
}

// ============================================================================
// IOConfig and IBusInfo Tests
// ============================================================================

TEST_CASE("IBusInfo", "[Serialization][IBusInfo]")
{
  SECTION("Input bus info")
  {
    IBusInfo bus(kInput, 2);

    REQUIRE(bus.GetDirection() == kInput);
    REQUIRE(bus.NChans() == 2);
  }

  SECTION("Output bus info")
  {
    IBusInfo bus(kOutput, 8);

    REQUIRE(bus.GetDirection() == kOutput);
    REQUIRE(bus.NChans() == 8);
  }
}

TEST_CASE("IOConfig", "[Serialization][IOConfig]")
{
  SECTION("Add and retrieve bus info")
  {
    IOConfig config;
    config.AddBusInfo(kInput, 2);
    config.AddBusInfo(kOutput, 2);

    REQUIRE(config.NBuses(kInput) == 1);
    REQUIRE(config.NBuses(kOutput) == 1);

    const IBusInfo* inputBus = config.GetBusInfo(kInput, 0);
    const IBusInfo* outputBus = config.GetBusInfo(kOutput, 0);

    REQUIRE(inputBus->NChans() == 2);
    REQUIRE(outputBus->NChans() == 2);
  }

  SECTION("GetTotalNChannels")
  {
    IOConfig config;
    config.AddBusInfo(kInput, 2);
    config.AddBusInfo(kInput, 4);
    config.AddBusInfo(kOutput, 8);

    REQUIRE(config.GetTotalNChannels(kInput) == 6);
    REQUIRE(config.GetTotalNChannels(kOutput) == 8);
  }

  SECTION("NChansOnBusSAFE with out of bounds")
  {
    IOConfig config;
    config.AddBusInfo(kInput, 2);

    REQUIRE(config.NChansOnBusSAFE(kInput, 0) == 2);
    REQUIRE(config.NChansOnBusSAFE(kInput, 1) == 0); // Out of bounds
    REQUIRE(config.NChansOnBusSAFE(kInput, -1) == 0); // Negative index
  }

  SECTION("ContainsWildcard")
  {
    IOConfig config;
    config.AddBusInfo(kInput, 2);
    REQUIRE(config.ContainsWildcard(kInput) == false);

    IOConfig configWithWildcard;
    configWithWildcard.AddBusInfo(kInput, -1); // Wildcard
    REQUIRE(configWithWildcard.ContainsWildcard(kInput) == true);
  }
}

// ============================================================================
// ITimeInfo Tests
// ============================================================================

TEST_CASE("ITimeInfo", "[Serialization][ITimeInfo]")
{
  SECTION("Default values")
  {
    ITimeInfo info;

    REQUIRE(info.mTempo == DEFAULT_TEMPO);
    REQUIRE(info.mSamplePos == -1.0);
    REQUIRE(info.mPPQPos == -1.0);
    REQUIRE(info.mNumerator == 4);
    REQUIRE(info.mDenominator == 4);
    REQUIRE(info.mTransportIsRunning == false);
    REQUIRE(info.mTransportLoopEnabled == false);
  }

  SECTION("Can modify values")
  {
    ITimeInfo info;
    info.mTempo = 140.0;
    info.mSamplePos = 44100.0;
    info.mPPQPos = 4.0;
    info.mNumerator = 3;
    info.mDenominator = 4;
    info.mTransportIsRunning = true;

    REQUIRE(info.mTempo == 140.0);
    REQUIRE(info.mSamplePos == 44100.0);
    REQUIRE(info.mNumerator == 3);
    REQUIRE(info.mTransportIsRunning == true);
  }
}
