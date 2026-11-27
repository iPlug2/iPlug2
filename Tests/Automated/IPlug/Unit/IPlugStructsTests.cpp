/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

// Unit tests for the helper structs in IPlugStructs.h: IByteChunk, IByteStream, IByteChunkReader, IOConfig etc.

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>

#include "IPlugStructs.h"

using namespace iplug;
using Catch::Approx;

namespace
{
struct PODStruct
{
  int32_t a;
  double b;
  float c;
  uint8_t d[4];
};

const int kIntSize = static_cast<int>(sizeof(int));

/** Puts a POD value into a chunk and reads it back from the given position */
template <typename T>
T RoundTrip(const T& value)
{
  IByteChunk chunk;
  chunk.Put(&value);
  T result {};
  chunk.Get(&result, 0);
  return result;
}

std::string GetStrAt(const IByteChunk& chunk, int pos, int* pNextPos = nullptr)
{
  WDL_String str("previous contents");
  const int next = chunk.GetStr(str, pos);
  if (pNextPos)
    *pNextPos = next;
  return str.Get();
}
} // namespace

TEST_CASE("ParamTuple: construction", "[unit][structs]")
{
  ParamTuple def;
  REQUIRE(def.idx == kNoParameter);
  REQUIRE(def.value == 0.);

  ParamTuple p {3, 0.75};
  REQUIRE(p.idx == 3);
  REQUIRE(p.value == 0.75);

  ParamTuple idxOnly {5};
  REQUIRE(idxOnly.idx == 5);
  REQUIRE(idxOnly.value == 0.);
}

TEST_CASE("SysExData: construction", "[unit][structs]")
{
  SECTION("default is empty and zeroed")
  {
    SysExData d;
    REQUIRE(d.mOffset == 0);
    REQUIRE(d.mSize == 0);
    bool allZero = true;
    for (int i = 0; i < MAX_SYSEX_SIZE; i++)
      allZero = allZero && (d.mData[i] == 0);
    REQUIRE(allZero);
  }

  SECTION("copies the payload")
  {
    const uint8_t msg[6] = {0xF0, 0x7D, 0x01, 0x02, 0x03, 0xF7};
    SysExData d {17, 6, msg};
    REQUIRE(d.mOffset == 17);
    REQUIRE(d.mSize == 6);
    REQUIRE(memcmp(d.mData, msg, 6) == 0);
  }

  SECTION("a size without data gives a zeroed buffer")
  {
    SysExData d {4, 8};
    REQUIRE(d.mOffset == 4);
    REQUIRE(d.mSize == 8);
    for (int i = 0; i < 8; i++)
      REQUIRE(d.mData[i] == 0);
  }

  SECTION("the largest payload that fits")
  {
    std::vector<uint8_t> msg(MAX_SYSEX_SIZE - 1);
    for (size_t i = 0; i < msg.size(); i++)
      msg[i] = static_cast<uint8_t>(i & 0x7F);
    SysExData d {0, static_cast<int>(msg.size()), msg.data()};
    REQUIRE(d.mSize == MAX_SYSEX_SIZE - 1);
    REQUIRE(memcmp(d.mData, msg.data(), msg.size()) == 0);
  }
}

TEST_CASE("IByteChunk: starts empty", "[unit][structs][chunk]")
{
  IByteChunk chunk;
  REQUIRE(chunk.Size() == 0);

  int value = 42;
  REQUIRE(chunk.Get(&value, 0) == -1);
  REQUIRE(value == 42);

  WDL_String str("untouched");
  REQUIRE(chunk.GetStr(str, 0) == -1);
  REQUIRE(std::string(str.Get()) == "untouched");
}

TEST_CASE("IByteChunk: Put and Get of POD types", "[unit][structs][chunk]")
{
  SECTION("individual types round trip")
  {
    REQUIRE(RoundTrip<int>(-123456) == -123456);
    REQUIRE(RoundTrip<int>(0) == 0);
    REQUIRE(RoundTrip<uint8_t>(0xAB) == 0xAB);
    REQUIRE(RoundTrip<int8_t>(-5) == -5);
    REQUIRE(RoundTrip<int16_t>(-32768) == -32768);
    REQUIRE(RoundTrip<uint16_t>(65535) == 65535);
    REQUIRE(RoundTrip<uint32_t>(0xDEADBEEFu) == 0xDEADBEEFu);
    REQUIRE(RoundTrip<int64_t>(INT64_C(-9007199254740993)) == INT64_C(-9007199254740993));
    REQUIRE(RoundTrip<uint64_t>(UINT64_C(0xFEDCBA9876543210)) == UINT64_C(0xFEDCBA9876543210));
    REQUIRE(RoundTrip<float>(1.5f) == 1.5f);
    REQUIRE(RoundTrip<double>(-0.1) == -0.1);
    REQUIRE(RoundTrip<double>(1e300) == 1e300);
    REQUIRE(RoundTrip<bool>(true) == true);
    REQUIRE(RoundTrip<bool>(false) == false);
    REQUIRE(RoundTrip<char>('x') == 'x');
  }

  SECTION("Put returns the new size and Get the next position")
  {
    IByteChunk chunk;
    const int32_t i = 7;
    const double d = 2.5;
    const float f = -1.25f;
    const uint8_t b = 200;

    REQUIRE(chunk.Put(&i) == 4);
    REQUIRE(chunk.Put(&d) == 12);
    REQUIRE(chunk.Put(&f) == 16);
    REQUIRE(chunk.Put(&b) == 17);
    REQUIRE(chunk.Size() == 17);

    int32_t i2 = 0;
    double d2 = 0.;
    float f2 = 0.f;
    uint8_t b2 = 0;

    int pos = chunk.Get(&i2, 0);
    REQUIRE(pos == 4);
    pos = chunk.Get(&d2, pos);
    REQUIRE(pos == 12);
    pos = chunk.Get(&f2, pos);
    REQUIRE(pos == 16);
    pos = chunk.Get(&b2, pos);
    REQUIRE(pos == 17);

    REQUIRE(i2 == i);
    REQUIRE(d2 == d);
    REQUIRE(f2 == f);
    REQUIRE(b2 == b);

    // values can be read from any position, in any order
    REQUIRE(chunk.Get(&f2, 12) == 16);
    REQUIRE(f2 == f);
    REQUIRE(chunk.Get(&i2, 0) == 4);
    REQUIRE(i2 == i);
  }

  SECTION("structs")
  {
    PODStruct in {};
    in.a = -99;
    in.b = 1234.5678;
    in.c = 0.5f;
    in.d[0] = 1; in.d[1] = 2; in.d[2] = 3; in.d[3] = 4;

    IByteChunk chunk;
    REQUIRE(chunk.Put(&in) == static_cast<int>(sizeof(PODStruct)));

    PODStruct out {};
    REQUIRE(chunk.Get(&out, 0) == static_cast<int>(sizeof(PODStruct)));
    REQUIRE(out.a == in.a);
    REQUIRE(out.b == in.b);
    REQUIRE(out.c == in.c);
    REQUIRE(memcmp(out.d, in.d, 4) == 0);
  }

  SECTION("many values")
  {
    IByteChunk chunk;
    for (int i = 0; i < 1000; i++)
    {
      const double v = i * 0.5;
      chunk.Put(&v);
    }
    REQUIRE(chunk.Size() == 8000);

    int pos = 0;
    bool allEqual = true;
    for (int i = 0; i < 1000; i++)
    {
      double v = -1.;
      pos = chunk.Get(&v, pos);
      allEqual = allEqual && (v == i * 0.5);
    }
    REQUIRE(allEqual);
    REQUIRE(pos == 8000);
  }
}

TEST_CASE("IByteChunk: Get fails with -1 outside the chunk", "[unit][structs][chunk]")
{
  IByteChunk chunk;
  const int32_t in = 0x01020304;
  chunk.Put(&in);
  chunk.Put(&in);
  REQUIRE(chunk.Size() == 8);

  int32_t out = 77;

  SECTION("at the end")
  {
    REQUIRE(chunk.Get(&out, 8) == -1);
  }
  SECTION("past the end")
  {
    REQUIRE(chunk.Get(&out, 100) == -1);
  }
  SECTION("straddling the end")
  {
    auto pos = GENERATE(5, 6, 7);
    REQUIRE(chunk.Get(&out, pos) == -1);
  }
  SECTION("negative position")
  {
    auto pos = GENERATE(-1, -4, -100);
    REQUIRE(chunk.Get(&out, pos) == -1);
  }
  SECTION("type larger than the chunk")
  {
    PODStruct big {};
    big.a = 5;
    REQUIRE(chunk.Get(&big, 0) == -1);
    REQUIRE(big.a == 5);
  }

  // the destination is left alone on failure
  REQUIRE(out == 77);

  // the last valid position still works
  REQUIRE(chunk.Get(&out, 4) == 8);
  REQUIRE(out == in);
}

TEST_CASE("IByteChunk: PutBytes and GetBytes", "[unit][structs][chunk]")
{
  IByteChunk chunk;
  const uint8_t in[5] = {10, 20, 30, 40, 50};

  REQUIRE(chunk.PutBytes(in, 5) == 5);
  REQUIRE(chunk.PutBytes(in, 2) == 7);
  REQUIRE(chunk.PutBytes(in, 0) == 7);
  REQUIRE(chunk.Size() == 7);

  const uint8_t expected[7] = {10, 20, 30, 40, 50, 10, 20};
  REQUIRE(memcmp(chunk.GetData(), expected, 7) == 0);

  uint8_t out[7] = {0};

  SECTION("whole chunk")
  {
    REQUIRE(chunk.GetBytes(out, 7, 0) == 7);
    REQUIRE(memcmp(out, expected, 7) == 0);
  }

  SECTION("a slice from the middle")
  {
    REQUIRE(chunk.GetBytes(out, 3, 2) == 5);
    REQUIRE(out[0] == 30);
    REQUIRE(out[1] == 40);
    REQUIRE(out[2] == 50);
    REQUIRE(out[3] == 0);
  }

  SECTION("zero bytes can be read anywhere up to the end")
  {
    REQUIRE(chunk.GetBytes(out, 0, 0) == 0);
    REQUIRE(chunk.GetBytes(out, 0, 7) == 7);
  }

  SECTION("bounds failures return -1 and copy nothing")
  {
    REQUIRE(chunk.GetBytes(out, 8, 0) == -1);
    REQUIRE(chunk.GetBytes(out, 1, 7) == -1);
    REQUIRE(chunk.GetBytes(out, 3, 5) == -1);
    REQUIRE(chunk.GetBytes(out, 1, -1) == -1);
    REQUIRE(chunk.GetBytes(out, 7, -7) == -1);
    REQUIRE(chunk.GetBytes(out, 1, 1000) == -1);
    for (int i = 0; i < 7; i++)
      REQUIRE(out[i] == 0);
  }
}

TEST_CASE("IByteChunk: PutStr and GetStr", "[unit][structs][chunk]")
{
  SECTION("a string is stored as an int length followed by the characters")
  {
    IByteChunk chunk;
    REQUIRE(chunk.PutStr("hello") == kIntSize + 5);
    REQUIRE(chunk.Size() == kIntSize + 5);

    int len = 0;
    REQUIRE(chunk.Get(&len, 0) == kIntSize);
    REQUIRE(len == 5);
    REQUIRE(memcmp(chunk.GetData() + kIntSize, "hello", 5) == 0);

    int next = 0;
    REQUIRE(GetStrAt(chunk, 0, &next) == "hello");
    REQUIRE(next == kIntSize + 5);
  }

  SECTION("the empty string")
  {
    IByteChunk chunk;
    REQUIRE(chunk.PutStr("") == kIntSize);
    int next = 0;
    REQUIRE(GetStrAt(chunk, 0, &next) == "");
    REQUIRE(next == kIntSize);
  }

  SECTION("several strings and values interleaved")
  {
    IByteChunk chunk;
    const double value = 0.125;
    chunk.PutStr("first");
    chunk.Put(&value);
    chunk.PutStr("");
    chunk.PutStr("the third string, which is a bit longer");

    WDL_String str;
    int pos = chunk.GetStr(str, 0);
    REQUIRE(std::string(str.Get()) == "first");
    double readValue = 0.;
    pos = chunk.Get(&readValue, pos);
    REQUIRE(readValue == value);
    pos = chunk.GetStr(str, pos);
    REQUIRE(std::string(str.Get()) == "");
    pos = chunk.GetStr(str, pos);
    REQUIRE(std::string(str.Get()) == "the third string, which is a bit longer");
    REQUIRE(pos == chunk.Size());
    REQUIRE(chunk.GetStr(str, pos) == -1);
  }

  SECTION("strings from a WDL_String, including non-ASCII bytes")
  {
    WDL_String in("Pr\xC3\xA9set \xE2\x82\xAC 1");
    IByteChunk chunk;
    chunk.PutStr(in.Get());
    REQUIRE(chunk.Size() == kIntSize + in.GetLength());
    REQUIRE(GetStrAt(chunk, 0) == std::string(in.Get()));
  }

  SECTION("a long string")
  {
    const std::string in(5000, 'z');
    IByteChunk chunk;
    chunk.PutStr(in.c_str());
    int next = 0;
    REQUIRE(GetStrAt(chunk, 0, &next) == in);
    REQUIRE(next == kIntSize + 5000);
  }

  SECTION("a start position outside the chunk fails")
  {
    IByteChunk chunk;
    chunk.PutStr("abc");
    WDL_String str("untouched");
    REQUIRE(chunk.GetStr(str, chunk.Size()) == -1);
    REQUIRE(chunk.GetStr(str, chunk.Size() - 1) == -1);
    REQUIRE(chunk.GetStr(str, -1) == -1);
    REQUIRE(chunk.GetStr(str, 1000) == -1);
    REQUIRE(std::string(str.Get()) == "untouched");
  }

  SECTION("a truncated string is never copied")
  {
    IByteChunk chunk;
    chunk.PutStr("abcdefgh");
    chunk.Resize(chunk.Size() - 3);
    WDL_String str("untouched");
    chunk.GetStr(str, 0);
    REQUIRE(std::string(str.Get()) == "untouched");
  }
}

// BUG: IPlugStructs.h:94-105 (IByteGetter::GetStr). The doc comment says -1 is returned "if the copy would
// have copied more data than in the src buffer", but when the length prefix is readable and the characters
// run past the end of the buffer the string is (correctly) not copied yet the would-be end position, which
// is beyond the end of the data, is returned instead of -1. Callers cannot detect the truncated string.
// Affects IByteChunk::GetStr, IByteStream::GetStr and IByteChunkReader::GetStr.
TEST_CASE("IByteChunk: GetStr reports a truncated string with -1", "[.][bug]")
{
  IByteChunk chunk;
  chunk.PutStr("abcdefgh");
  chunk.Resize(chunk.Size() - 3);

  WDL_String str;
  REQUIRE(chunk.GetStr(str, 0) == -1);

  IByteStream stream(chunk.GetData(), chunk.Size());
  REQUIRE(stream.GetStr(str, 0) == -1);

  // a corrupt (negative) length must not move the read position backwards either
  IByteChunk corrupt;
  const int negativeLen = -100;
  corrupt.Put(&negativeLen);
  REQUIRE(corrupt.GetStr(str, 0) == -1);
}

TEST_CASE("IByteChunk: PutChunk", "[unit][structs][chunk]")
{
  IByteChunk a, b, empty;
  const int32_t one = 1, two = 2, three = 3;
  a.Put(&one);
  b.Put(&two);
  b.Put(&three);

  REQUIRE(a.PutChunk(&b) == 12);
  REQUIRE(a.Size() == 12);
  REQUIRE(b.Size() == 8);

  int32_t v[3] = {0, 0, 0};
  int pos = 0;
  for (int i = 0; i < 3; i++)
    pos = a.Get(&v[i], pos);
  REQUIRE(v[0] == 1);
  REQUIRE(v[1] == 2);
  REQUIRE(v[2] == 3);

  REQUIRE(empty.PutChunk(&b) == 8);
  REQUIRE(empty.IsEqual(b));
}

// BUG: IPlugStructs.h:151 and IPlugStructs.h:259. A chunk that has never held data has a null data pointer,
// which PutBytes() (via PutChunk() of an empty chunk) and IsEqual() (comparing two empty chunks) pass to
// memcpy()/memcmp() with a size of zero. That is undefined behaviour before C23/C++26 and is reported by
// UBSan ("null pointer passed as argument ... declared to never be null"), although the results are as
// expected on the platforms tested. Kept out of the normal run so that sanitizer builds stay clean.
TEST_CASE("IByteChunk: empty chunks can be appended and compared", "[.][bug]")
{
  IByteChunk a, b, empty;
  REQUIRE(a.IsEqual(b));
  REQUIRE(a.IsEqual(a));

  const int32_t x = 5;
  a.Put(&x);
  REQUIRE(a.PutChunk(&empty) == 4);
  REQUIRE(a.Size() == 4);

  a.Clear();
  REQUIRE(a.IsEqual(b));
}

TEST_CASE("IByteChunk: Clear and Resize", "[unit][structs][chunk]")
{
  IByteChunk chunk;
  const uint8_t in[4] = {1, 2, 3, 4};
  chunk.PutBytes(in, 4);

  SECTION("Clear empties the chunk and it can be reused")
  {
    chunk.Clear();
    REQUIRE(chunk.Size() == 0);
    uint8_t out = 9;
    REQUIRE(chunk.Get(&out, 0) == -1);
    REQUIRE(chunk.PutBytes(in, 2) == 2);
    REQUIRE(chunk.Get(&out, 1) == 2);
    REQUIRE(out == 2);
  }

  SECTION("growing returns the old size, keeps the contents and zero fills")
  {
    REQUIRE(chunk.Resize(10) == 4);
    REQUIRE(chunk.Size() == 10);
    const uint8_t expected[10] = {1, 2, 3, 4, 0, 0, 0, 0, 0, 0};
    REQUIRE(memcmp(chunk.GetData(), expected, 10) == 0);
  }

  SECTION("shrinking keeps the leading bytes")
  {
    REQUIRE(chunk.Resize(2) == 4);
    REQUIRE(chunk.Size() == 2);
    REQUIRE(chunk.GetData()[0] == 1);
    REQUIRE(chunk.GetData()[1] == 2);
    uint8_t out = 0;
    REQUIRE(chunk.Get(&out, 2) == -1);
  }

  SECTION("regrowing after shrinking zero fills the new bytes")
  {
    chunk.Resize(1);
    REQUIRE(chunk.Resize(4) == 1);
    const uint8_t expected[4] = {1, 0, 0, 0};
    REQUIRE(memcmp(chunk.GetData(), expected, 4) == 0);
  }

  SECTION("resizing to the same size or to zero")
  {
    REQUIRE(chunk.Resize(4) == 4);
    REQUIRE(chunk.Size() == 4);
    REQUIRE(chunk.Resize(0) == 4);
    REQUIRE(chunk.Size() == 0);
    REQUIRE(chunk.Resize(3) == 0);
    for (int i = 0; i < 3; i++)
      REQUIRE(chunk.GetData()[i] == 0);
  }

  SECTION("data can be written through GetData")
  {
    chunk.GetData()[3] = 99;
    uint8_t out = 0;
    chunk.Get(&out, 3);
    REQUIRE(out == 99);
    const IByteChunk& constChunk = chunk;
    REQUIRE(constChunk.GetData() == chunk.GetData());
    REQUIRE(constChunk.GetData()[3] == 99);
  }
}

TEST_CASE("IByteChunk: IsEqual", "[unit][structs][chunk]")
{
  IByteChunk a, b;

  const int32_t x = 5, y = 6;
  a.Put(&x);
  REQUIRE_FALSE(a.IsEqual(b));
  REQUIRE_FALSE(b.IsEqual(a));

  b.Put(&x);
  REQUIRE(a.IsEqual(b));
  REQUIRE(b.IsEqual(a));
  REQUIRE(a.IsEqual(a));

  a.Put(&x);
  b.Put(&y);
  REQUIRE(a.Size() == b.Size());
  REQUIRE_FALSE(a.IsEqual(b));

  b.Resize(4);
  REQUIRE_FALSE(a.IsEqual(b));
  a.Resize(4);
  REQUIRE(a.IsEqual(b));

  // an emptied chunk differs from one with data
  a.Clear();
  REQUIRE_FALSE(a.IsEqual(b));
  REQUIRE_FALSE(b.IsEqual(a));
}

TEST_CASE("IByteChunk: IPlug version helpers", "[unit][structs][chunk]")
{
  SECTION("InitChunkWithIPlugVer writes a magic number and the version, replacing the contents")
  {
    IByteChunk chunk;
    chunk.PutStr("stale data");
    IByteChunk::InitChunkWithIPlugVer(chunk);
    REQUIRE(chunk.Size() == 2 * kIntSize);

    int magic = 0, ver = 0;
    int pos = chunk.Get(&magic, 0);
    chunk.Get(&ver, pos);
    REQUIRE(magic == IPLUG_VERSION_MAGIC);
    REQUIRE(ver == IPLUG_VERSION);
  }

  SECTION("GetIPlugVerFromChunk reads it back and advances the position")
  {
    IByteChunk chunk;
    IByteChunk::InitChunkWithIPlugVer(chunk);
    const double payload = 3.5;
    chunk.Put(&payload);

    int pos = 0;
    REQUIRE(IByteChunk::GetIPlugVerFromChunk(chunk, pos) == IPLUG_VERSION);
    REQUIRE(pos == 2 * kIntSize);

    double readPayload = 0.;
    REQUIRE(chunk.Get(&readPayload, pos) == chunk.Size());
    REQUIRE(readPayload == payload);
  }

  SECTION("the version can be read from a non-zero position")
  {
    IByteChunk header, chunk;
    IByteChunk::InitChunkWithIPlugVer(header);
    const int32_t prefix = 1234;
    chunk.Put(&prefix);
    chunk.PutChunk(&header);

    int pos = 4;
    REQUIRE(IByteChunk::GetIPlugVerFromChunk(chunk, pos) == IPLUG_VERSION);
    REQUIRE(pos == 4 + 2 * kIntSize);
  }

  SECTION("a chunk without the magic number gives 0 and leaves the position alone")
  {
    IByteChunk chunk;
    const int notMagic = 0x12345678;
    const int ver = IPLUG_VERSION;
    chunk.Put(&notMagic);
    chunk.Put(&ver);

    int pos = 0;
    REQUIRE(IByteChunk::GetIPlugVerFromChunk(chunk, pos) == 0);
    REQUIRE(pos == 0);
  }

  SECTION("an empty or too short chunk gives 0")
  {
    IByteChunk chunk;
    int pos = 0;
    REQUIRE(IByteChunk::GetIPlugVerFromChunk(chunk, pos) == 0);
    REQUIRE(pos == 0);

    const uint8_t byte = 1;
    chunk.Put(&byte);
    REQUIRE(IByteChunk::GetIPlugVerFromChunk(chunk, pos) == 0);
    REQUIRE(pos == 0);
  }
}

TEST_CASE("IByteStream: reads from memory it does not own", "[unit][structs][chunk]")
{
  IByteChunk chunk;
  const int32_t i = -42;
  const double d = 6.25;
  chunk.Put(&i);
  chunk.PutStr("stream");
  chunk.Put(&d);

  IByteStream stream(chunk.GetData(), chunk.Size());
  REQUIRE(stream.Size() == chunk.Size());
  REQUIRE(stream.GetData() == chunk.GetData());

  SECTION("Get, GetStr and GetBytes")
  {
    int32_t i2 = 0;
    double d2 = 0.;
    WDL_String str;
    int pos = stream.Get(&i2, 0);
    REQUIRE(pos == 4);
    pos = stream.GetStr(str, pos);
    REQUIRE(pos == 4 + kIntSize + 6);
    pos = stream.Get(&d2, pos);
    REQUIRE(pos == stream.Size());
    REQUIRE(i2 == i);
    REQUIRE(d2 == d);
    REQUIRE(std::string(str.Get()) == "stream");

    char raw[6] = {0};
    REQUIRE(stream.GetBytes(raw, 6, 4 + kIntSize) == 4 + kIntSize + 6);
    REQUIRE(memcmp(raw, "stream", 6) == 0);
  }

  SECTION("bounds failures return -1")
  {
    double d2 = 1.;
    WDL_String str("untouched");
    REQUIRE(stream.Get(&d2, stream.Size()) == -1);
    REQUIRE(stream.Get(&d2, stream.Size() - 7) == -1);
    REQUIRE(stream.Get(&d2, -1) == -1);
    REQUIRE(stream.GetBytes(&d2, 1, stream.Size()) == -1);
    REQUIRE(stream.GetStr(str, stream.Size()) == -1);
    REQUIRE(stream.GetStr(str, -3) == -1);
    REQUIRE(d2 == 1.);
    REQUIRE(std::string(str.Get()) == "untouched");
  }

  SECTION("IsEqual compares size and contents")
  {
    std::vector<uint8_t> copy(chunk.GetData(), chunk.GetData() + chunk.Size());
    IByteStream same(copy.data(), static_cast<int>(copy.size()));
    REQUIRE(stream.IsEqual(same));
    REQUIRE(same.IsEqual(stream));

    IByteStream shorter(copy.data(), static_cast<int>(copy.size()) - 1);
    REQUIRE_FALSE(stream.IsEqual(shorter));

    copy.back() ^= 0xFF;
    REQUIRE_FALSE(stream.IsEqual(same));
  }

  SECTION("an empty stream")
  {
    IByteStream empty(nullptr, 0);
    REQUIRE(empty.Size() == 0);
    int v = 3;
    REQUIRE(empty.Get(&v, 0) == -1);
    REQUIRE(v == 3);
  }
}

TEST_CASE("IByteChunkReader: keeps track of the read position", "[unit][structs][chunk]")
{
  IByteChunk chunk;
  const int32_t i = 11;
  const float f = 0.75f;
  const uint8_t raw[3] = {7, 8, 9};
  chunk.Put(&i);
  chunk.PutStr("reader");
  chunk.Put(&f);
  chunk.PutBytes(raw, 3);

  SECTION("sequential reads")
  {
    IByteChunkReader reader(chunk);
    REQUIRE(reader.Tell() == 0);

    int32_t i2 = 0;
    REQUIRE(reader.Get(&i2) == 4);
    REQUIRE(reader.Tell() == 4);
    REQUIRE(i2 == i);

    WDL_String str;
    REQUIRE(reader.GetStr(str) == 4 + kIntSize + 6);
    REQUIRE(reader.Tell() == 4 + kIntSize + 6);
    REQUIRE(std::string(str.Get()) == "reader");

    float f2 = 0.f;
    REQUIRE(reader.Get(&f2) == 8 + kIntSize + 6);
    REQUIRE(f2 == f);

    uint8_t raw2[3] = {0, 0, 0};
    REQUIRE(reader.GetBytes(raw2, 3) == chunk.Size());
    REQUIRE(memcmp(raw, raw2, 3) == 0);
    REQUIRE(reader.Tell() == chunk.Size());
  }

  SECTION("Seek and a start position")
  {
    IByteChunkReader reader(chunk, 4);
    REQUIRE(reader.Tell() == 4);
    WDL_String str;
    reader.GetStr(str);
    REQUIRE(std::string(str.Get()) == "reader");

    reader.Seek(0);
    REQUIRE(reader.Tell() == 0);
    int32_t i2 = 0;
    reader.Get(&i2);
    REQUIRE(i2 == i);

    reader.Seek(chunk.Size() - 3);
    uint8_t b = 0;
    reader.Get(&b);
    REQUIRE(b == 7);
    REQUIRE(reader.Tell() == chunk.Size() - 2);
  }

  SECTION("reading past the end reports -1")
  {
    IByteChunkReader reader(chunk, chunk.Size() - 2);
    int32_t i2 = 55;
    REQUIRE(reader.Get(&i2) == -1);
    REQUIRE(reader.Tell() == -1);
    REQUIRE(i2 == 55);

    // once failed, further reads keep failing until the reader is repositioned
    uint8_t b = 0;
    REQUIRE(reader.Get(&b) == -1);
    reader.Seek(chunk.Size() - 1);
    REQUIRE(reader.Get(&b) == chunk.Size());
    REQUIRE(b == 9);
  }

  SECTION("it sees data added to the chunk afterwards")
  {
    IByteChunkReader reader(chunk, chunk.Size());
    const int32_t extra = 321;
    chunk.Put(&extra);
    int32_t i2 = 0;
    REQUIRE(reader.Get(&i2) == chunk.Size());
    REQUIRE(i2 == extra);
  }
}

TEST_CASE("Config: constructor arguments map to the right members", "[unit][structs]")
{
  const char* ioStr = "1-1 2-2";
  Config c(11, 12, ioStr, "Plugin", "Product", "Mfr", 0x00010203, 13, 14, 15,
           true, false, true, false, kInstrument, true, 800, 600, true, 400, 1600, 300, 1200,
           "com.mfr.plugin", "group.com.mfr");

  REQUIRE(c.nParams == 11);
  REQUIRE(c.nPresets == 12);
  REQUIRE(c.channelIOStr == ioStr);
  REQUIRE(std::string(c.pluginName) == "Plugin");
  REQUIRE(std::string(c.productName) == "Product");
  REQUIRE(std::string(c.mfrName) == "Mfr");
  REQUIRE(c.vendorVersion == 0x00010203);
  REQUIRE(c.uniqueID == 13);
  REQUIRE(c.mfrID == 14);
  REQUIRE(c.latency == 15);
  REQUIRE(c.plugDoesMidiIn);
  REQUIRE_FALSE(c.plugDoesMidiOut);
  REQUIRE(c.plugDoesMPE);
  REQUIRE_FALSE(c.plugDoesChunks);
  REQUIRE(c.plugType == kInstrument);
  REQUIRE(c.plugHasUI);
  REQUIRE(c.plugWidth == 800);
  REQUIRE(c.plugHeight == 600);
  REQUIRE(c.plugHostResize);
  REQUIRE(c.plugMinWidth == 400);
  REQUIRE(c.plugMaxWidth == 1600);
  REQUIRE(c.plugMinHeight == 300);
  REQUIRE(c.plugMaxHeight == 1200);
  REQUIRE(std::string(c.bundleID) == "com.mfr.plugin");
  REQUIRE(std::string(c.appGroupID) == "group.com.mfr");
}

TEST_CASE("IChannelData: defaults", "[unit][structs]")
{
  IChannelData<> data;
  REQUIRE_FALSE(data.mConnected);
  REQUIRE(data.mData == nullptr);
  REQUIRE(data.mIncomingData == nullptr);
  REQUIRE(data.mScratchBuf.GetSize() == 0);
  REQUIRE(data.mLabel.GetLength() == 0);
}

TEST_CASE("IBusInfo: stores direction and channel count", "[unit][structs]")
{
  IBusInfo in(ERoute::kInput, 2);
  REQUIRE(in.GetDirection() == ERoute::kInput);
  REQUIRE(in.NChans() == 2);

  IBusInfo out(ERoute::kOutput);
  REQUIRE(out.GetDirection() == ERoute::kOutput);
  REQUIRE(out.NChans() == 0);
}

TEST_CASE("IOConfig: bus bookkeeping", "[unit][structs]")
{
  IOConfig config;

  SECTION("starts with no buses")
  {
    auto dir = GENERATE(ERoute::kInput, ERoute::kOutput);
    REQUIRE(config.NBuses(dir) == 0);
    REQUIRE(config.GetTotalNChannels(dir) == 0);
    REQUIRE(config.NChansOnBusSAFE(dir, 0) == 0);
    REQUIRE_FALSE(config.ContainsWildcard(dir));
  }

  SECTION("inputs and outputs are tracked separately, in the order added")
  {
    config.AddBusInfo(ERoute::kInput, 2);
    config.AddBusInfo(ERoute::kInput, 1);
    config.AddBusInfo(ERoute::kOutput, 6);

    REQUIRE(config.NBuses(ERoute::kInput) == 2);
    REQUIRE(config.NBuses(ERoute::kOutput) == 1);
    REQUIRE(config.GetTotalNChannels(ERoute::kInput) == 3);
    REQUIRE(config.GetTotalNChannels(ERoute::kOutput) == 6);

    REQUIRE(config.GetBusInfo(ERoute::kInput, 0)->NChans() == 2);
    REQUIRE(config.GetBusInfo(ERoute::kInput, 1)->NChans() == 1);
    REQUIRE(config.GetBusInfo(ERoute::kOutput, 0)->NChans() == 6);
    REQUIRE(config.GetBusInfo(ERoute::kInput, 0)->GetDirection() == ERoute::kInput);
    REQUIRE(config.GetBusInfo(ERoute::kOutput, 0)->GetDirection() == ERoute::kOutput);

    REQUIRE(config.NChansOnBusSAFE(ERoute::kInput, 0) == 2);
    REQUIRE(config.NChansOnBusSAFE(ERoute::kInput, 1) == 1);
    REQUIRE(config.NChansOnBusSAFE(ERoute::kOutput, 0) == 6);
  }

  SECTION("NChansOnBusSAFE gives 0 for buses that do not exist")
  {
    config.AddBusInfo(ERoute::kInput, 2);
    REQUIRE(config.NChansOnBusSAFE(ERoute::kInput, 1) == 0);
    REQUIRE(config.NChansOnBusSAFE(ERoute::kInput, 100) == 0);
    REQUIRE(config.NChansOnBusSAFE(ERoute::kInput, -1) == 0);
    REQUIRE(config.NChansOnBusSAFE(ERoute::kOutput, 0) == 0);
  }

  SECTION("a bus with no channels still counts as a bus")
  {
    config.AddBusInfo(ERoute::kInput, 0);
    config.AddBusInfo(ERoute::kOutput, 2);
    REQUIRE(config.NBuses(ERoute::kInput) == 1);
    REQUIRE(config.GetTotalNChannels(ERoute::kInput) == 0);
    REQUIRE_FALSE(config.ContainsWildcard(ERoute::kInput));
  }

  SECTION("negative channel counts are wildcards")
  {
    config.AddBusInfo(ERoute::kInput, 2);
    config.AddBusInfo(ERoute::kOutput, 2);
    REQUIRE_FALSE(config.ContainsWildcard(ERoute::kInput));
    REQUIRE_FALSE(config.ContainsWildcard(ERoute::kOutput));

    config.AddBusInfo(ERoute::kInput, -1);
    REQUIRE(config.ContainsWildcard(ERoute::kInput));
    REQUIRE_FALSE(config.ContainsWildcard(ERoute::kOutput));
    REQUIRE(config.NChansOnBusSAFE(ERoute::kInput, 1) == -1);

    config.AddBusInfo(ERoute::kOutput, 1);
    config.AddBusInfo(ERoute::kOutput, -2);
    REQUIRE(config.ContainsWildcard(ERoute::kOutput));
  }
}

TEST_CASE("ITimeInfo: defaults describe a stopped transport with unknown positions", "[unit][structs]")
{
  ITimeInfo info;
  REQUIRE(info.mTempo == DEFAULT_TEMPO);
  REQUIRE(info.mTempo == 120.);
  REQUIRE(info.mSamplePos == -1.);
  REQUIRE(info.mPPQPos == -1.);
  REQUIRE(info.mLastBar == -1.);
  REQUIRE(info.mCycleStart == -1.);
  REQUIRE(info.mCycleEnd == -1.);
  REQUIRE(info.mNumerator == 4);
  REQUIRE(info.mDenominator == 4);
  REQUIRE_FALSE(info.mTransportIsRunning);
  REQUIRE_FALSE(info.mTransportLoopEnabled);
}

TEST_CASE("IPreset: defaults", "[unit][structs]")
{
  IPreset preset;
  REQUIRE_FALSE(preset.mInitialized);
  REQUIRE(std::string(preset.mName) == UNUSED_PRESET_NAME);
  REQUIRE(std::string(preset.mName) == "empty");
  REQUIRE(preset.mChunk.Size() == 0);
  REQUIRE(sizeof(preset.mName) == static_cast<size_t>(MAX_PRESET_NAME_LEN));
}

TEST_CASE("IKeyPress: construction", "[unit][structs]")
{
  IKeyPress plain("a", kVK_A);
  REQUIRE(plain.VK == kVK_A);
  REQUIRE(std::string(plain.utf8) == "a");
  REQUIRE_FALSE(plain.S);
  REQUIRE_FALSE(plain.C);
  REQUIRE_FALSE(plain.A);

  IKeyPress modified("", kVK_F1, true, false, true);
  REQUIRE(modified.VK == kVK_F1);
  REQUIRE(std::string(modified.utf8).empty());
  REQUIRE(modified.S);
  REQUIRE_FALSE(modified.C);
  REQUIRE(modified.A);

  // a four byte UTF-8 sequence is the longest that fits
  IKeyPress emoji("\xF0\x9F\x98\x80", kVK_NONE, false, true, false);
  REQUIRE(std::string(emoji.utf8) == "\xF0\x9F\x98\x80");
  REQUIRE(emoji.C);
}
