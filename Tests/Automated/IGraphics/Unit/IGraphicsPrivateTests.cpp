/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for IGraphicsPrivate.h: APIBitmap, IFontInfo/IFontData (with synthetic font data), StaticStorage and IVec2.

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "IGraphicsStructs.h"
#include "IGraphicsPrivate.h"

using namespace iplug;
using namespace igraphics;
using Catch::Approx;

namespace
{
using Bytes = std::vector<unsigned char>;

void PutU16(Bytes& bytes, size_t pos, uint32_t value)
{
  bytes[pos] = static_cast<unsigned char>((value >> 8) & 0xFF);
  bytes[pos + 1] = static_cast<unsigned char>(value & 0xFF);
}

void PutU32(Bytes& bytes, size_t pos, uint32_t value)
{
  PutU16(bytes, pos, (value >> 16) & 0xFFFF);
  PutU16(bytes, pos + 2, value & 0xFFFF);
}

void PutTag(Bytes& bytes, size_t pos, const char* tag)
{
  for (int i = 0; i < 4; i++)
    bytes[pos + static_cast<size_t>(i)] = static_cast<unsigned char>(tag[i]);
}

/** One entry in a font's 'name' table */
struct NameRecord
{
  int platformID;
  int encodingID;
  int languageID;
  int nameID;
  Bytes text; // already encoded for the platform
};

Bytes MacText(const std::string& str)
{
  return Bytes(str.begin(), str.end());
}

/** Encodes code points as big-endian UTF-16, as used by the Windows platform names */
Bytes WindowsText(const std::vector<uint32_t>& codePoints)
{
  Bytes bytes;

  auto put = [&bytes](uint32_t unit) {
    bytes.push_back(static_cast<unsigned char>((unit >> 8) & 0xFF));
    bytes.push_back(static_cast<unsigned char>(unit & 0xFF));
  };

  for (uint32_t codePoint : codePoints)
  {
    if (codePoint >= 0x10000)
    {
      const uint32_t v = codePoint - 0x10000;
      put(0xD800 + (v >> 10));
      put(0xDC00 + (v & 0x3FF));
    }
    else
      put(codePoint);
  }

  return bytes;
}

Bytes WindowsText(const std::string& ascii)
{
  return WindowsText(std::vector<uint32_t>(ascii.begin(), ascii.end()));
}

NameRecord MacName(int nameID, const std::string& str) { return {1, 0, 0, nameID, MacText(str)}; }
NameRecord WindowsName(int nameID, const std::string& str) { return {3, 1, 0x409, nameID, WindowsText(str)}; }

const int kFamilyNameID = 1;
const int kStyleNameID = 2;

/** Describes a minimal synthetic font: just the tables and fields that IFontInfo reads */
struct FontSpec
{
  bool openTypeCFF = false; // 'OTTO' rather than the TrueType version tag
  bool hasHead = true;
  bool hasHhea = true;
  bool hasName = true;
  int unitsPerEM = 1000;
  int macStyle = 0;
  int ascender = 800;
  int descender = -200;
  int lineGap = 90;
  std::vector<NameRecord> names = {WindowsName(kFamilyNameID, "Test Family"), WindowsName(kStyleNameID, "Regular")};
};

/** Builds the bytes of a font.
 *  @param fileOffset Where the font will be placed in the file (non-zero inside a collection): table offsets are relative to the start of the file */
Bytes MakeFont(const FontSpec& spec, size_t fileOffset = 0)
{
  const size_t kHeadSize = 54, kHheaSize = 36;
  const int nTables = (spec.hasHead ? 1 : 0) + (spec.hasHhea ? 1 : 0) + (spec.hasName ? 1 : 0);

  Bytes font(12 + 16 * static_cast<size_t>(nTables), 0);

  if (spec.openTypeCFF)
    PutTag(font, 0, "OTTO");
  else
    PutU32(font, 0, 0x00010000);

  PutU16(font, 4, static_cast<uint32_t>(nTables));

  size_t recordPos = 12;

  auto addTable = [&](const char* tag, const Bytes& table) {
    PutTag(font, recordPos, tag);
    PutU32(font, recordPos + 8, static_cast<uint32_t>(fileOffset + font.size()));
    PutU32(font, recordPos + 12, static_cast<uint32_t>(table.size()));
    recordPos += 16;
    font.insert(font.end(), table.begin(), table.end());
  };

  if (spec.hasHead)
  {
    Bytes head(kHeadSize, 0);
    PutU32(head, 0, 0x00010000);
    PutU32(head, 12, 0x5F0F3CF5); // magic number
    PutU16(head, 18, static_cast<uint32_t>(spec.unitsPerEM));
    PutU16(head, 44, static_cast<uint32_t>(spec.macStyle));
    addTable("head", head);
  }

  if (spec.hasHhea)
  {
    Bytes hhea(kHheaSize, 0);
    PutU32(hhea, 0, 0x00010000);
    PutU16(hhea, 4, static_cast<uint32_t>(spec.ascender) & 0xFFFF);
    PutU16(hhea, 6, static_cast<uint32_t>(spec.descender) & 0xFFFF);
    PutU16(hhea, 8, static_cast<uint32_t>(spec.lineGap) & 0xFFFF);
    addTable("hhea", hhea);
  }

  if (spec.hasName)
  {
    const size_t nRecords = spec.names.size();
    const size_t stringStart = 6 + 12 * nRecords;
    Bytes name(stringStart, 0);
    PutU16(name, 2, static_cast<uint32_t>(nRecords));
    PutU16(name, 4, static_cast<uint32_t>(stringStart));

    for (size_t i = 0; i < nRecords; i++)
    {
      const NameRecord& record = spec.names[i];
      const size_t pos = 6 + 12 * i;
      PutU16(name, pos + 0, static_cast<uint32_t>(record.platformID));
      PutU16(name, pos + 2, static_cast<uint32_t>(record.encodingID));
      PutU16(name, pos + 4, static_cast<uint32_t>(record.languageID));
      PutU16(name, pos + 6, static_cast<uint32_t>(record.nameID));
      PutU16(name, pos + 8, static_cast<uint32_t>(record.text.size()));
      PutU16(name, pos + 10, static_cast<uint32_t>(name.size() - stringStart));
      name.insert(name.end(), record.text.begin(), record.text.end());
    }

    addTable("name", name);
  }

  return font;
}

uint32_t Size(const Bytes& bytes)
{
  return static_cast<uint32_t>(bytes.size());
}

/** A trivial type for StaticStorage that counts how many instances are alive */
struct Tracked
{
  static int sNumAlive;
  int mValue;

  explicit Tracked(int value) : mValue(value) { sNumAlive++; }
  ~Tracked() { sNumAlive--; }

  Tracked(const Tracked&) = delete;
  Tracked& operator=(const Tracked&) = delete;
};

int Tracked::sNumAlive = 0;

using TrackedStorage = StaticStorage<Tracked>;
} // namespace

// MARK: - APIBitmap

TEST_CASE("APIBitmap: default construction", "[unit][apibitmap]")
{
  APIBitmap bitmap;
  REQUIRE(bitmap.GetBitmap() == 0);
  REQUIRE(bitmap.GetWidth() == 0);
  REQUIRE(bitmap.GetHeight() == 0);
  REQUIRE(bitmap.GetScale() == 0.f);
  REQUIRE(bitmap.GetDrawScale() == 1.f);
}

TEST_CASE("APIBitmap: construction with values", "[unit][apibitmap]")
{
  APIBitmap bitmap {7, 200, 100, 2.f, 1.5f};
  REQUIRE(bitmap.GetBitmap() == 7);
  REQUIRE(bitmap.GetWidth() == 200);
  REQUIRE(bitmap.GetHeight() == 100);
  REQUIRE(bitmap.GetScale() == 2.f);
  REQUIRE(bitmap.GetDrawScale() == 1.5f);
}

TEST_CASE("APIBitmap: SetBitmap", "[unit][apibitmap]")
{
  APIBitmap bitmap;
  bitmap.SetBitmap(3, 64, 32, 1.f, 2.f);
  REQUIRE(bitmap.GetBitmap() == 3);
  REQUIRE(bitmap.GetWidth() == 64);
  REQUIRE(bitmap.GetHeight() == 32);
  REQUIRE(bitmap.GetScale() == 1.f);
  REQUIRE(bitmap.GetDrawScale() == 2.f);

  // replaces everything
  bitmap.SetBitmap(4, 10, 20, 3.f, 1.f);
  REQUIRE(bitmap.GetBitmap() == 4);
  REQUIRE(bitmap.GetWidth() == 10);
  REQUIRE(bitmap.GetHeight() == 20);
  REQUIRE(bitmap.GetScale() == 3.f);
  REQUIRE(bitmap.GetDrawScale() == 1.f);
}

// MARK: - IFontInfo

TEST_CASE("IFontInfo: null data is invalid", "[unit][ifontinfo]")
{
  IFontInfo info {nullptr, 0, 0};
  REQUIRE_FALSE(info.IsValid());
  REQUIRE(info.GetFamily().GetLength() == 0);
  REQUIRE(info.GetStyle().GetLength() == 0);
  REQUIRE(info.GetUnitsPerEM() == 0);
  REQUIRE(info.GetAscender() == 0);
  REQUIRE(info.GetDescender() == 0);
  REQUIRE(info.GetLineGap() == 0);
  REQUIRE(info.GetLineHeight() == 0);
  REQUIRE_FALSE(info.IsBold());
  REQUIRE_FALSE(info.IsItalic());
}

TEST_CASE("IFontInfo: data that isn't a font is invalid", "[unit][ifontinfo]")
{
  SECTION("zeros")
  {
    const Bytes data(64, 0);
    REQUIRE_FALSE(IFontInfo(data.data(), Size(data), 0).IsValid());
  }

  SECTION("text")
  {
    const std::string text = "This is not a font file, it is just some text that is long enough to look at.";
    REQUIRE_FALSE(IFontInfo(text.data(), static_cast<uint32_t>(text.size()), 0).IsValid());
  }

  SECTION("a PNG header")
  {
    const Bytes data = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A, 0, 0, 0, 0x0D, 'I', 'H', 'D', 'R', 0, 0, 0, 0, 0, 0, 0, 0};
    REQUIRE_FALSE(IFontInfo(data.data(), Size(data), 0).IsValid());
  }

  SECTION("a font header with no tables")
  {
    Bytes data(12, 0);
    PutU32(data, 0, 0x00010000);
    REQUIRE_FALSE(IFontInfo(data.data(), Size(data), 0).IsValid());
  }

  SECTION("invalid whatever face is asked for")
  {
    const Bytes data(64, 0xAA);
    REQUIRE_FALSE(IFontInfo(data.data(), Size(data), 0).IsValid());
    REQUIRE_FALSE(IFontInfo(data.data(), Size(data), 1).IsValid());
  }
}

TEST_CASE("IFontInfo: a font needs head, hhea and name tables", "[unit][ifontinfo]")
{
  FontSpec spec;

  SECTION("all present")
  {
    const Bytes font = MakeFont(spec);
    REQUIRE(IFontInfo(font.data(), Size(font), 0).IsValid());
  }

  SECTION("no head")
  {
    spec.hasHead = false;
    const Bytes font = MakeFont(spec);
    REQUIRE_FALSE(IFontInfo(font.data(), Size(font), 0).IsValid());
  }

  SECTION("no hhea")
  {
    spec.hasHhea = false;
    const Bytes font = MakeFont(spec);
    REQUIRE_FALSE(IFontInfo(font.data(), Size(font), 0).IsValid());
  }

  SECTION("no name")
  {
    spec.hasName = false;
    const Bytes font = MakeFont(spec);
    REQUIRE_FALSE(IFontInfo(font.data(), Size(font), 0).IsValid());
  }
}

TEST_CASE("IFontInfo: reads the names and metrics of a TrueType font", "[unit][ifontinfo]")
{
  FontSpec spec;
  spec.unitsPerEM = 2048;
  spec.ascender = 1900;
  spec.descender = -500;
  spec.lineGap = 67;
  spec.names = {WindowsName(kFamilyNameID, "Synthetic Sans"), WindowsName(kStyleNameID, "Bold Italic")};

  const Bytes font = MakeFont(spec);
  IFontInfo info {font.data(), Size(font), 0};

  REQUIRE(info.IsValid());
  REQUIRE(std::string(info.GetFamily().Get()) == "Synthetic Sans");
  REQUIRE(std::string(info.GetStyle().Get()) == "Bold Italic");
  REQUIRE(info.GetUnitsPerEM() == 2048);
  REQUIRE(info.GetAscender() == 1900);
  REQUIRE(info.GetDescender() == -500);
  REQUIRE(info.GetLineGap() == 67);

  // derived metrics
  REQUIRE(info.GetLineHeight() == 1900 + 500 + 67);
  REQUIRE(info.GetHeightEMRatio() == Approx(2048. / 2400.));
}

TEST_CASE("IFontInfo: reads an OpenType (CFF) font", "[unit][ifontinfo]")
{
  FontSpec spec;
  spec.openTypeCFF = true;

  const Bytes font = MakeFont(spec);
  IFontInfo info {font.data(), Size(font), 0};

  REQUIRE(info.IsValid());
  REQUIRE(std::string(info.GetFamily().Get()) == "Test Family");
  REQUIRE(std::string(info.GetStyle().Get()) == "Regular");
  REQUIRE(info.GetUnitsPerEM() == 1000);
}

TEST_CASE("IFontInfo: a single font only has face 0", "[unit][ifontinfo]")
{
  const Bytes font = MakeFont(FontSpec());
  REQUIRE(IFontInfo(font.data(), Size(font), 0).IsValid());
  REQUIRE_FALSE(IFontInfo(font.data(), Size(font), 1).IsValid());
  REQUIRE_FALSE(IFontInfo(font.data(), Size(font), 7).IsValid());
}

TEST_CASE("IFontInfo: style flags come from the head table", "[unit][ifontinfo]")
{
  struct Flag
  {
    int bit;
    bool (IFontInfo::*getter)() const;
    const char* name;
  };

  const Flag flags[] = {
    {0, &IFontInfo::IsBold, "bold"},
    {1, &IFontInfo::IsItalic, "italic"},
    {2, &IFontInfo::IsUnderline, "underline"},
    {3, &IFontInfo::IsOutline, "outline"},
    {4, &IFontInfo::IsShadow, "shadow"},
    {5, &IFontInfo::IsCondensed, "condensed"},
    {6, &IFontInfo::IsExpanded, "expanded"},
  };

  SECTION("none set")
  {
    const Bytes font = MakeFont(FontSpec());
    IFontInfo info {font.data(), Size(font), 0};

    for (const Flag& flag : flags)
    {
      INFO(flag.name);
      REQUIRE_FALSE((info.*flag.getter)());
    }
  }

  SECTION("each flag alone")
  {
    for (const Flag& set : flags)
    {
      FontSpec spec;
      spec.macStyle = 1 << set.bit;
      const Bytes font = MakeFont(spec);
      IFontInfo info {font.data(), Size(font), 0};
      REQUIRE(info.IsValid());

      for (const Flag& flag : flags)
      {
        INFO(set.name << " set, checking " << flag.name);
        REQUIRE((info.*flag.getter)() == (flag.bit == set.bit));
      }
    }
  }

  SECTION("bold italic")
  {
    FontSpec spec;
    spec.macStyle = 3;
    const Bytes font = MakeFont(spec);
    IFontInfo info {font.data(), Size(font), 0};
    REQUIRE(info.IsBold());
    REQUIRE(info.IsItalic());
    REQUIRE_FALSE(info.IsUnderline());
    REQUIRE_FALSE(info.IsCondensed());
  }
}

TEST_CASE("IFontInfo: metrics are signed where they should be", "[unit][ifontinfo]")
{
  FontSpec spec;
  spec.unitsPerEM = 0xFFFF; // unsigned, so not -1
  spec.ascender = 32767;
  spec.descender = -32768;
  spec.lineGap = -10;

  const Bytes font = MakeFont(spec);
  IFontInfo info {font.data(), Size(font), 0};

  REQUIRE(info.IsValid());
  REQUIRE(info.GetUnitsPerEM() == 65535);
  REQUIRE(info.GetAscender() == 32767);
  REQUIRE(info.GetDescender() == -32768);
  REQUIRE(info.GetLineGap() == -10);
}

TEST_CASE("IFontInfo: names", "[unit][ifontinfo]")
{
  FontSpec spec;

  SECTION("Macintosh platform names are used when there are no Windows ones")
  {
    spec.names = {MacName(kFamilyNameID, "Mac Family"), MacName(kStyleNameID, "Oblique")};
    const Bytes font = MakeFont(spec);
    IFontInfo info {font.data(), Size(font), 0};
    REQUIRE(info.IsValid());
    REQUIRE(std::string(info.GetFamily().Get()) == "Mac Family");
    REQUIRE(std::string(info.GetStyle().Get()) == "Oblique");
  }

  SECTION("Windows platform names are preferred")
  {
    spec.names = {MacName(kFamilyNameID, "Mac Family"), MacName(kStyleNameID, "Mac Style"),
                  WindowsName(kFamilyNameID, "Windows Family"), WindowsName(kStyleNameID, "Windows Style")};
    const Bytes font = MakeFont(spec);
    IFontInfo info {font.data(), Size(font), 0};
    REQUIRE(std::string(info.GetFamily().Get()) == "Windows Family");
    REQUIRE(std::string(info.GetStyle().Get()) == "Windows Style");
  }

  SECTION("the platforms can be mixed")
  {
    spec.names = {WindowsName(kFamilyNameID, "Windows Family"), MacName(kStyleNameID, "Mac Style")};
    const Bytes font = MakeFont(spec);
    IFontInfo info {font.data(), Size(font), 0};
    REQUIRE(std::string(info.GetFamily().Get()) == "Windows Family");
    REQUIRE(std::string(info.GetStyle().Get()) == "Mac Style");
  }

  SECTION("other name IDs are ignored")
  {
    // 0: copyright, 3: unique ID, 4: full name, 6: PostScript name
    spec.names = {WindowsName(0, "Copyright"), WindowsName(kFamilyNameID, "Family"), WindowsName(kStyleNameID, "Style"),
                  WindowsName(3, "Unique"), WindowsName(4, "Family Style"), WindowsName(6, "Family-Style")};
    const Bytes font = MakeFont(spec);
    IFontInfo info {font.data(), Size(font), 0};
    REQUIRE(std::string(info.GetFamily().Get()) == "Family");
    REQUIRE(std::string(info.GetStyle().Get()) == "Style");
  }

  SECTION("names in other languages are ignored")
  {
    // 0x407 is German: only US English (0x409) names are used
    spec.names = {{3, 1, 0x407, kFamilyNameID, WindowsText("Familie")}, {3, 1, 0x407, kStyleNameID, WindowsText("Fett")},
                  WindowsName(kFamilyNameID, "Family"), WindowsName(kStyleNameID, "Bold")};
    const Bytes font = MakeFont(spec);
    IFontInfo info {font.data(), Size(font), 0};
    REQUIRE(std::string(info.GetFamily().Get()) == "Family");
    REQUIRE(std::string(info.GetStyle().Get()) == "Bold");
  }

  SECTION("a font with no usable names is still valid, with empty names")
  {
    spec.names = {WindowsName(4, "Full Name Only")};
    const Bytes font = MakeFont(spec);
    IFontInfo info {font.data(), Size(font), 0};
    REQUIRE(info.IsValid());
    REQUIRE(info.GetFamily().GetLength() == 0);
    REQUIRE(info.GetStyle().GetLength() == 0);
  }

  SECTION("an empty name table")
  {
    spec.names = {};
    const Bytes font = MakeFont(spec);
    IFontInfo info {font.data(), Size(font), 0};
    REQUIRE(info.IsValid());
    REQUIRE(info.GetFamily().GetLength() == 0);
  }
}

TEST_CASE("IFontInfo: Windows names are converted from UTF-16 to UTF-8", "[unit][ifontinfo]")
{
  FontSpec spec;

  // "Caf" + U+00E9 (2 bytes in UTF-8), U+20AC (3 bytes), U+1F600 (a surrogate pair in UTF-16, 4 bytes in UTF-8)
  spec.names = {{3, 1, 0x409, kFamilyNameID, WindowsText(std::vector<uint32_t> {'C', 'a', 'f', 0xE9})},
                {3, 1, 0x409, kStyleNameID, WindowsText(std::vector<uint32_t> {0x20AC, 0x1F600, '!'})}};

  const Bytes font = MakeFont(spec);
  IFontInfo info {font.data(), Size(font), 0};

  REQUIRE(info.IsValid());
  REQUIRE(std::string(info.GetFamily().Get()) == "Caf\xC3\xA9");
  REQUIRE(std::string(info.GetStyle().Get()) == "\xE2\x82\xAC\xF0\x9F\x98\x80!");
}

// reads are checked against the size of the data
TEST_CASE("IFontInfo: truncated data is invalid", "[unit][ifontinfo]")
{
  SECTION("a header that claims more tables than there is data for")
  {
    Bytes data(6, 0);
    PutU32(data, 0, 0x00010000);
    PutU16(data, 4, 5);
    CHECK_FALSE(IFontInfo(data.data(), Size(data), 0).IsValid());
  }

  SECTION("a font cut off part way through its tables")
  {
    Bytes font = MakeFont(FontSpec());
    font.resize(12 + 16 * 3 + 10);
    font.shrink_to_fit();
    CHECK_FALSE(IFontInfo(font.data(), Size(font), 0).IsValid());
  }

  SECTION("fewer than 4 bytes")
  {
    const Bytes data = {0, 1};
    CHECK_FALSE(IFontInfo(data.data(), Size(data), 0).IsValid());
  }
}

// BUG: IGraphicsPrivate.h:333 for a font collection ('ttcf') IFontInfo::FindFace() advances mData to the face's
// offset table, and the table offsets found there are then applied relative to that new position. In a TrueType
// collection the table offsets are relative to the start of the file, so every table of a face is looked for in
// the wrong place (the face's offset within the file too far along) and the names/metrics read are wrong.
TEST_CASE("IFontInfo: reads the faces of a TrueType collection", "[.][bug]")
{
  FontSpec regular;
  regular.names = {WindowsName(kFamilyNameID, "Collection"), WindowsName(kStyleNameID, "Regular")};
  regular.unitsPerEM = 1000;

  FontSpec bold;
  bold.names = {WindowsName(kFamilyNameID, "Collection"), WindowsName(kStyleNameID, "Bold")};
  bold.unitsPerEM = 2048;
  bold.macStyle = 1;

  // header: 'ttcf', version, number of fonts, then an offset for each font
  const size_t kHeaderSize = 12 + 4 * 2;
  Bytes collection(kHeaderSize, 0);
  PutTag(collection, 0, "ttcf");
  PutU32(collection, 4, 0x00010000);
  PutU32(collection, 8, 2);

  PutU32(collection, 12, static_cast<uint32_t>(collection.size()));
  const Bytes regularFont = MakeFont(regular, collection.size());
  collection.insert(collection.end(), regularFont.begin(), regularFont.end());

  PutU32(collection, 16, static_cast<uint32_t>(collection.size()));
  const Bytes boldFont = MakeFont(bold, collection.size());
  collection.insert(collection.end(), boldFont.begin(), boldFont.end());

  // padding so that the misplaced reads described above stay inside the buffer
  collection.resize(collection.size() + 4096, 0);

  IFontInfo face0 {collection.data(), Size(collection), 0};
  CHECK(face0.IsValid());
  CHECK(std::string(face0.GetFamily().Get()) == "Collection");
  CHECK(std::string(face0.GetStyle().Get()) == "Regular");
  CHECK(face0.GetUnitsPerEM() == 1000);
  CHECK_FALSE(face0.IsBold());

  IFontInfo face1 {collection.data(), Size(collection), 1};
  CHECK(face1.IsValid());
  CHECK(std::string(face1.GetStyle().Get()) == "Bold");
  CHECK(face1.GetUnitsPerEM() == 2048);
  CHECK(face1.IsBold());

  // there are only two faces
  CHECK_FALSE(IFontInfo(collection.data(), Size(collection), 2).IsValid());
}

TEST_CASE("IFontInfo: a face index beyond the end of a collection is invalid", "[unit][ifontinfo]")
{
  // a collection header declaring a single font
  Bytes collection(16, 0);
  PutTag(collection, 0, "ttcf");
  PutU32(collection, 4, 0x00010000);
  PutU32(collection, 8, 1);
  PutU32(collection, 12, 16);

  REQUIRE_FALSE(IFontInfo(collection.data(), Size(collection), 1).IsValid());
  REQUIRE_FALSE(IFontInfo(collection.data(), Size(collection), 5).IsValid());

  // an unknown collection version
  PutU32(collection, 4, 0x00030000);
  REQUIRE_FALSE(IFontInfo(collection.data(), Size(collection), 0).IsValid());
}

// MARK: - IFontData

TEST_CASE("IFontData: default construction is invalid and empty", "[unit][ifontdata]")
{
  IFontData data;
  REQUIRE_FALSE(data.IsValid());
  REQUIRE(data.GetSize() == 0);
  REQUIRE(data.GetFaceIdx() == -1);
}

TEST_CASE("IFontData: construction with a size allocates a buffer to be filled in", "[unit][ifontdata]")
{
  IFontData data {256};
  REQUIRE(data.GetSize() == 256);
  REQUIRE(data.Get() != nullptr);
  REQUIRE(data.GetFaceIdx() == -1);

  // not valid until the data has been filled in and a face chosen
  REQUIRE_FALSE(data.IsValid());

  // the whole buffer is writable
  std::memset(data.Get(), 0x5A, static_cast<size_t>(data.GetSize()));
  REQUIRE(data.Get()[0] == 0x5A);
  REQUIRE(data.Get()[255] == 0x5A);
}

TEST_CASE("IFontData: construction from font data takes a copy", "[unit][ifontdata]")
{
  FontSpec spec;
  spec.names = {WindowsName(kFamilyNameID, "Copied Family"), WindowsName(kStyleNameID, "Light")};
  spec.macStyle = 2;
  Bytes font = MakeFont(spec);

  IFontData data {font.data(), static_cast<int>(font.size()), 0};

  REQUIRE(data.IsValid());
  REQUIRE(data.GetFaceIdx() == 0);
  REQUIRE(data.GetSize() == static_cast<int>(font.size()));
  REQUIRE(data.Get() != font.data());
  REQUIRE(std::memcmp(data.Get(), font.data(), font.size()) == 0);

  // the font info is available
  REQUIRE(std::string(data.GetFamily().Get()) == "Copied Family");
  REQUIRE(std::string(data.GetStyle().Get()) == "Light");
  REQUIRE(data.IsItalic());
  REQUIRE_FALSE(data.IsBold());
  REQUIRE(data.GetUnitsPerEM() == 1000);
  REQUIRE(data.GetAscender() == 800);
  REQUIRE(data.GetDescender() == -200);

  // changing the original afterwards makes no difference
  const Bytes original = font;
  std::fill(font.begin(), font.end(), static_cast<unsigned char>(0));
  REQUIRE(std::memcmp(data.Get(), original.data(), original.size()) == 0);
  REQUIRE(data.IsValid());
  REQUIRE(std::string(data.GetFamily().Get()) == "Copied Family");
}

TEST_CASE("IFontData: data that isn't a font is kept but is invalid", "[unit][ifontdata]")
{
  const Bytes garbage(64, 0xAA);
  IFontData data {garbage.data(), static_cast<int>(garbage.size()), 0};

  REQUIRE_FALSE(data.IsValid());
  REQUIRE(data.GetSize() == 64);
  REQUIRE(data.GetFaceIdx() == 0);
  REQUIRE(std::memcmp(data.Get(), garbage.data(), garbage.size()) == 0);
}

TEST_CASE("IFontData: a face that doesn't exist is invalid", "[unit][ifontdata]")
{
  const Bytes font = MakeFont(FontSpec());
  IFontData data {font.data(), static_cast<int>(font.size()), 1};
  REQUIRE_FALSE(data.IsValid());
  REQUIRE(data.GetFaceIdx() == 1);
  REQUIRE(data.GetSize() == static_cast<int>(font.size()));
}

TEST_CASE("IFontData: SetFaceIdx parses data that was filled in after construction", "[unit][ifontdata]")
{
  FontSpec spec;
  spec.names = {WindowsName(kFamilyNameID, "Filled Family"), WindowsName(kStyleNameID, "Medium")};
  spec.unitsPerEM = 2048;
  const Bytes font = MakeFont(spec);

  IFontData data {static_cast<int>(font.size())};
  std::memcpy(data.Get(), font.data(), font.size());
  REQUIRE_FALSE(data.IsValid());

  data.SetFaceIdx(0);

  REQUIRE(data.IsValid());
  REQUIRE(data.GetFaceIdx() == 0);
  REQUIRE(data.GetSize() == static_cast<int>(font.size()));
  REQUIRE(std::memcmp(data.Get(), font.data(), font.size()) == 0);
  REQUIRE(std::string(data.GetFamily().Get()) == "Filled Family");
  REQUIRE(std::string(data.GetStyle().Get()) == "Medium");
  REQUIRE(data.GetUnitsPerEM() == 2048);

  SECTION("choosing a face that doesn't exist invalidates")
  {
    data.SetFaceIdx(3);
    REQUIRE_FALSE(data.IsValid());
    REQUIRE(data.GetFaceIdx() == 3);
    REQUIRE(data.GetSize() == static_cast<int>(font.size()));
  }

  SECTION("...and the face can be chosen again")
  {
    data.SetFaceIdx(3);
    data.SetFaceIdx(0);
    REQUIRE(data.IsValid());
    REQUIRE(std::string(data.GetFamily().Get()) == "Filled Family");
  }
}

TEST_CASE("IFontData: can be held in an IFontDataPtr", "[unit][ifontdata]")
{
  const Bytes font = MakeFont(FontSpec());

  IFontDataPtr pData {new IFontData(font.data(), static_cast<int>(font.size()), 0)};
  REQUIRE(pData->IsValid());

  IFontDataPtr pMoved = std::move(pData);
  REQUIRE(pData == nullptr);
  REQUIRE(pMoved->IsValid());
  REQUIRE(std::string(pMoved->GetFamily().Get()) == "Test Family");

  IFontDataPtr pEmpty {new IFontData()};
  REQUIRE_FALSE(pEmpty->IsValid());
}

// MARK: - StaticStorage

TEST_CASE("StaticStorage: starts empty", "[unit][staticstorage]")
{
  TrackedStorage storage;
  TrackedStorage::Accessor accessor {storage};
  REQUIRE(accessor.Find("anything") == nullptr);
  REQUIRE(accessor.Find("anything", 2.) == nullptr);
  REQUIRE(accessor.Find("") == nullptr);
}

TEST_CASE("StaticStorage: Add and Find", "[unit][staticstorage]")
{
  REQUIRE(Tracked::sNumAlive == 0);

  {
    TrackedStorage storage;
    TrackedStorage::Accessor accessor {storage};

    Tracked* pKnob = new Tracked(1);
    Tracked* pSlider = new Tracked(2);
    accessor.Add(pKnob, "knob.png");
    accessor.Add(pSlider, "slider.png");

    REQUIRE(accessor.Find("knob.png") == pKnob);
    REQUIRE(accessor.Find("slider.png") == pSlider);
    REQUIRE(accessor.Find("knob.png")->mValue == 1);
    REQUIRE(accessor.Find("slider.png")->mValue == 2);

    // found repeatedly
    REQUIRE(accessor.Find("knob.png") == pKnob);

    // the default scale is 1
    REQUIRE(accessor.Find("knob.png", 1.) == pKnob);

    SECTION("names must match exactly")
    {
      REQUIRE(accessor.Find("knob") == nullptr);
      REQUIRE(accessor.Find("knob.png ") == nullptr);
      REQUIRE(accessor.Find("Knob.png") == nullptr);
      REQUIRE(accessor.Find("knob.pngx") == nullptr);
      REQUIRE(accessor.Find("") == nullptr);
      REQUIRE(accessor.Find("button.png") == nullptr);
    }

    SECTION("the name is copied")
    {
      std::string name = "temporary.png";
      Tracked* pTemporary = new Tracked(3);
      accessor.Add(pTemporary, name.c_str());
      name = "changed.png";
      REQUIRE(accessor.Find("temporary.png") == pTemporary);
      REQUIRE(accessor.Find("changed.png") == nullptr);
    }

    REQUIRE(Tracked::sNumAlive >= 2);
  }

  // the storage owned the data
  REQUIRE(Tracked::sNumAlive == 0);
}

TEST_CASE("StaticStorage: data is stored per scale", "[unit][staticstorage]")
{
  TrackedStorage storage;
  TrackedStorage::Accessor accessor {storage};

  Tracked* p1x = new Tracked(1);
  Tracked* p2x = new Tracked(2);
  accessor.Add(p1x, "knob.png", 1.);
  accessor.Add(p2x, "knob.png", 2.);

  REQUIRE(accessor.Find("knob.png", 1.) == p1x);
  REQUIRE(accessor.Find("knob.png", 2.) == p2x);
  REQUIRE(accessor.Find("knob.png") == p1x);

  // scales that haven't been added
  REQUIRE(accessor.Find("knob.png", 3.) == nullptr);
  REQUIRE(accessor.Find("knob.png", 0.5) == nullptr);
  REQUIRE(accessor.Find("knob.png", 1.5) == nullptr);

  // fractional scales
  Tracked* pFractional = new Tracked(15);
  accessor.Add(pFractional, "knob.png", 1.5);
  REQUIRE(accessor.Find("knob.png", 1.5) == pFractional);
  REQUIRE(accessor.Find("knob.png", 1.) == p1x);
  REQUIRE(accessor.Find("knob.png", 2.) == p2x);

  // a scale added for one name doesn't exist for another
  accessor.Add(new Tracked(4), "slider.png", 2.);
  REQUIRE(accessor.Find("slider.png", 2.) != nullptr);
  REQUIRE(accessor.Find("slider.png", 1.) == nullptr);
  REQUIRE(accessor.Find("slider.png") == nullptr);
}

TEST_CASE("StaticStorage: Remove deletes the data", "[unit][staticstorage]")
{
  REQUIRE(Tracked::sNumAlive == 0);

  TrackedStorage storage;
  TrackedStorage::Accessor accessor {storage};

  Tracked* pA = new Tracked(1);
  Tracked* pB = new Tracked(2);
  Tracked* pC = new Tracked(3);
  accessor.Add(pA, "a");
  accessor.Add(pB, "b");
  accessor.Add(pC, "c");
  REQUIRE(Tracked::sNumAlive == 3);

  accessor.Remove(pB);
  REQUIRE(Tracked::sNumAlive == 2);
  REQUIRE(accessor.Find("b") == nullptr);
  REQUIRE(accessor.Find("a") == pA);
  REQUIRE(accessor.Find("c") == pC);

  SECTION("removing something that isn't stored does nothing")
  {
    Tracked notStored {99};
    accessor.Remove(&notStored);
    accessor.Remove(nullptr);
    REQUIRE(Tracked::sNumAlive == 3); // a, c and notStored
    REQUIRE(accessor.Find("a") == pA);
    REQUIRE(accessor.Find("c") == pC);
  }

  SECTION("the name can be reused")
  {
    Tracked* pNewB = new Tracked(22);
    accessor.Add(pNewB, "b");
    REQUIRE(accessor.Find("b") == pNewB);
    REQUIRE(accessor.Find("b")->mValue == 22);
  }

  SECTION("removing only affects the scale that the data was stored with")
  {
    Tracked* pA2x = new Tracked(11);
    accessor.Add(pA2x, "a", 2.);
    accessor.Remove(pA);
    REQUIRE(accessor.Find("a") == nullptr);
    REQUIRE(accessor.Find("a", 2.) == pA2x);
  }

  SECTION("everything can be removed")
  {
    accessor.Remove(pA);
    accessor.Remove(pC);
    REQUIRE(Tracked::sNumAlive == 0);
    REQUIRE(accessor.Find("a") == nullptr);
    REQUIRE(accessor.Find("c") == nullptr);
  }
}

TEST_CASE("StaticStorage: Clear deletes everything", "[unit][staticstorage]")
{
  REQUIRE(Tracked::sNumAlive == 0);

  TrackedStorage storage;
  TrackedStorage::Accessor accessor {storage};

  for (int i = 0; i < 20; i++)
    accessor.Add(new Tracked(i), std::to_string(i).c_str(), 1. + (i % 3));

  REQUIRE(Tracked::sNumAlive == 20);
  REQUIRE(accessor.Find("7", 2.) != nullptr);
  REQUIRE(accessor.Find("7", 2.)->mValue == 7);

  accessor.Clear();
  REQUIRE(Tracked::sNumAlive == 0);
  REQUIRE(accessor.Find("7", 2.) == nullptr);

  // clearing when empty is fine, and the storage can be used again
  accessor.Clear();
  Tracked* pNew = new Tracked(1);
  accessor.Add(pNew, "new");
  REQUIRE(accessor.Find("new") == pNew);
  REQUIRE(Tracked::sNumAlive == 1);
}

TEST_CASE("StaticStorage: Retain and Release count the users of the storage", "[unit][staticstorage]")
{
  REQUIRE(Tracked::sNumAlive == 0);

  TrackedStorage storage;
  TrackedStorage::Accessor accessor {storage};

  SECTION("the data is cleared when the last user releases")
  {
    accessor.Retain();
    accessor.Add(new Tracked(1), "a");
    accessor.Add(new Tracked(2), "b");
    accessor.Release();
    REQUIRE(Tracked::sNumAlive == 0);
    REQUIRE(accessor.Find("a") == nullptr);
  }

  SECTION("the data is kept while there are other users")
  {
    // two plug-in instances sharing the storage
    accessor.Retain();
    Tracked* pA = new Tracked(1);
    accessor.Add(pA, "a");
    accessor.Retain();
    REQUIRE(accessor.Find("a") == pA);

    accessor.Release();
    REQUIRE(Tracked::sNumAlive == 1);
    REQUIRE(accessor.Find("a") == pA);

    accessor.Release();
    REQUIRE(Tracked::sNumAlive == 0);
    REQUIRE(accessor.Find("a") == nullptr);
  }

  SECTION("the storage can be retained again after being released")
  {
    accessor.Retain();
    accessor.Add(new Tracked(1), "a");
    accessor.Release();

    accessor.Retain();
    REQUIRE(accessor.Find("a") == nullptr);
    Tracked* pA = new Tracked(2);
    accessor.Add(pA, "a");
    REQUIRE(accessor.Find("a") == pA);
    accessor.Release();
    REQUIRE(Tracked::sNumAlive == 0);
  }

  SECTION("many users")
  {
    for (int i = 0; i < 10; i++)
      accessor.Retain();

    accessor.Add(new Tracked(1), "a");

    for (int i = 0; i < 9; i++)
    {
      accessor.Release();
      REQUIRE(Tracked::sNumAlive == 1);
    }

    accessor.Release();
    REQUIRE(Tracked::sNumAlive == 0);
  }
}

TEST_CASE("StaticStorage: separate accessors see the same data", "[unit][staticstorage]")
{
  REQUIRE(Tracked::sNumAlive == 0);

  {
    TrackedStorage storage;
    Tracked* pA = new Tracked(1);

    {
      TrackedStorage::Accessor accessor {storage};
      accessor.Retain();
      accessor.Add(pA, "a");
    }

    // the data outlives the accessor
    REQUIRE(Tracked::sNumAlive == 1);

    {
      TrackedStorage::Accessor accessor {storage};
      REQUIRE(accessor.Find("a") == pA);
    }

    {
      TrackedStorage::Accessor accessor {storage};
      accessor.Release();
    }

    REQUIRE(Tracked::sNumAlive == 0);
  }
}

TEST_CASE("StaticStorage: separate storages are independent", "[unit][staticstorage]")
{
  REQUIRE(Tracked::sNumAlive == 0);

  TrackedStorage storageA;
  TrackedStorage storageB;
  Tracked* pA = new Tracked(1);

  {
    TrackedStorage::Accessor accessor {storageA};
    accessor.Add(pA, "shared name");
  }

  {
    TrackedStorage::Accessor accessor {storageB};
    REQUIRE(accessor.Find("shared name") == nullptr);
    accessor.Clear();
  }

  {
    TrackedStorage::Accessor accessor {storageA};
    REQUIRE(accessor.Find("shared name") == pA);
  }

  REQUIRE(Tracked::sNumAlive == 1);
}

TEST_CASE("StaticStorage: holds many entries", "[unit][staticstorage]")
{
  TrackedStorage storage;
  TrackedStorage::Accessor accessor {storage};

  for (int i = 0; i < 300; i++)
    accessor.Add(new Tracked(i), ("resource_" + std::to_string(i)).c_str());

  bool allFound = true;

  for (int i = 0; i < 300; i++)
  {
    Tracked* pFound = accessor.Find(("resource_" + std::to_string(i)).c_str());
    allFound = allFound && pFound && pFound->mValue == i;
  }

  REQUIRE(allFound);
  REQUIRE(accessor.Find("resource_300") == nullptr);
}

// MARK: - IVec2

TEST_CASE("IVec2: construction", "[unit][ivec2]")
{
  IVec2 v {3.f, -4.f};
  REQUIRE(v.x == 3.f);
  REQUIRE(v.y == -4.f);

  IVec2 zero {};
  REQUIRE(zero.x == 0.f);
  REQUIRE(zero.y == 0.f);
}

TEST_CASE("IVec2: addition and subtraction", "[unit][ivec2]")
{
  IVec2 a {3.f, -4.f};
  IVec2 b {1.5f, 2.f};

  const IVec2 sum = a + b;
  REQUIRE(sum.x == 4.5f);
  REQUIRE(sum.y == -2.f);

  const IVec2 difference = a - b;
  REQUIRE(difference.x == 1.5f);
  REQUIRE(difference.y == -6.f);

  // the operands are not changed
  REQUIRE(a.x == 3.f);
  REQUIRE(a.y == -4.f);
  REQUIRE(b.x == 1.5f);
  REQUIRE(b.y == 2.f);

  // subtraction undoes addition
  IVec2 roundTrip = (a + b) - b;
  REQUIRE(roundTrip.x == a.x);
  REQUIRE(roundTrip.y == a.y);

  IVec2 self = a - a;
  REQUIRE(self.x == 0.f);
  REQUIRE(self.y == 0.f);
}
