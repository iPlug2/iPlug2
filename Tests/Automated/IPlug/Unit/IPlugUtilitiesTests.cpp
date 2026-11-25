/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

// Unit tests for the free functions in IPlugUtilities.h and the constants in IPlugConstants.h.

#include <cmath>
#include <cstdint>
#include <limits>
#include <string>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/generators/catch_generators_range.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include "IPlugUtilities.h"

using namespace iplug;
using Catch::Approx;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::WithinRel;

namespace
{
std::string NoteName(double pitch, bool cents = false, bool middleCisC4 = false)
{
  WDL_String str;
  MidiNoteName(pitch, str, cents, middleCisC4);
  return str.Get();
}

std::string HostName(EHost host)
{
  WDL_String str("previous");
  GetHostNameStr(host, str);
  return str.Get();
}

std::string VersionStr(int version)
{
  WDL_String str;
  GetVersionStr(version, str);
  return str.Get();
}

std::string Lower(const char* str)
{
  char buf[128];
  ToLower(buf, str);
  return buf;
}
} // namespace

TEST_CASE("Clip: limits a value to a range", "[unit][utilities]")
{
  SECTION("doubles")
  {
    REQUIRE(Clip(0.5, 0., 1.) == 0.5);
    REQUIRE(Clip(-0.5, 0., 1.) == 0.);
    REQUIRE(Clip(1.5, 0., 1.) == 1.);
    REQUIRE(Clip(0., 0., 1.) == 0.);
    REQUIRE(Clip(1., 0., 1.) == 1.);
    REQUIRE(Clip(-7., -5., -3.) == -5.);
    REQUIRE(Clip(-1., -5., -3.) == -3.);
    REQUIRE(Clip(-4., -5., -3.) == -4.);
    REQUIRE(Clip(std::numeric_limits<double>::infinity(), 0., 1.) == 1.);
    REQUIRE(Clip(-std::numeric_limits<double>::infinity(), 0., 1.) == 0.);
  }

  SECTION("integers")
  {
    REQUIRE(Clip(5, 0, 10) == 5);
    REQUIRE(Clip(-1, 0, 10) == 0);
    REQUIRE(Clip(11, 0, 10) == 10);
    REQUIRE(Clip(0, 0, 10) == 0);
    REQUIRE(Clip(10, 0, 10) == 10);
    REQUIRE(Clip(3, 3, 3) == 3);
    REQUIRE(Clip(99, 3, 3) == 3);
    REQUIRE(Clip(-99, 3, 3) == 3);
  }

  SECTION("floats")
  {
    REQUIRE(Clip(0.25f, 0.f, 1.f) == 0.25f);
    REQUIRE(Clip(2.f, 0.f, 1.f) == 1.f);
    REQUIRE(Clip(-2.f, 0.f, 1.f) == 0.f);
  }
}

TEST_CASE("Lerp: linear interpolation", "[unit][utilities]")
{
  REQUIRE(Lerp(0., 10., 0.) == 0.);
  REQUIRE(Lerp(0., 10., 1.) == 10.);
  REQUIRE(Lerp(0., 10., 0.5) == 5.);
  REQUIRE(Lerp(0., 10., 0.25) == 2.5);
  REQUIRE(Lerp(-4., 4., 0.5) == 0.);
  REQUIRE(Lerp(10., 0., 0.25) == 7.5);
  REQUIRE(Lerp(3., 3., 0.7) == 3.);
  REQUIRE(Lerp(2.f, 4.f, 0.5f) == 3.f);

  // the factor is not clamped, so it extrapolates
  REQUIRE(Lerp(0., 10., 2.) == 20.);
  REQUIRE(Lerp(0., 10., -1.) == -10.);

  const double f = GENERATE(0., 0.1, 0.33, 0.5, 0.9, 1.);
  REQUIRE(Lerp(-20., 60., f) == Approx(-20. + 80. * f));
}

TEST_CASE("CStringHasContents", "[unit][utilities]")
{
  REQUIRE_FALSE(CStringHasContents(nullptr));
  REQUIRE_FALSE(CStringHasContents(""));
  REQUIRE(CStringHasContents("a"));
  REQUIRE(CStringHasContents(" "));
  REQUIRE(CStringHasContents("hello world"));

  const char embedded[3] = {'\0', 'a', '\0'};
  REQUIRE_FALSE(CStringHasContents(embedded));
}

TEST_CASE("MAKE_STR: stringifies after macro expansion", "[unit][utilities]")
{
#define IPLUG_TEST_UTIL_VALUE 42
  REQUIRE(std::string(MAKE_STR(IPLUG_TEST_UTIL_VALUE)) == "42");
  REQUIRE(std::string(MAKE_QUOTE(IPLUG_TEST_UTIL_VALUE)) == "IPLUG_TEST_UTIL_VALUE");
#undef IPLUG_TEST_UTIL_VALUE
}

TEST_CASE("DBToAmp and AmpToDB", "[unit][utilities]")
{
  SECTION("known values")
  {
    REQUIRE(DBToAmp(0.) == 1.);
    REQUIRE(DBToAmp(20.) == Approx(10.));
    REQUIRE(DBToAmp(-20.) == Approx(0.1));
    REQUIRE(DBToAmp(40.) == Approx(100.));
    REQUIRE(DBToAmp(-120.) == Approx(1e-6));
    REQUIRE(DBToAmp(6.) == Approx(1.9952623));
    REQUIRE(DBToAmp(-6.) == Approx(0.5011872));

    REQUIRE(AmpToDB(1.) == 0.);
    REQUIRE(AmpToDB(10.) == Approx(20.));
    REQUIRE(AmpToDB(0.1) == Approx(-20.));
    REQUIRE(AmpToDB(2.) == Approx(6.0206));
    REQUIRE(AmpToDB(0.5) == Approx(-6.0206));
    REQUIRE(AmpToDB(1e-6) == Approx(-120.));
  }

  SECTION("agree with the textbook formulas")
  {
    const double dB = GENERATE(-144., -96., -60., -12., -3., -0.1, 0.1, 3., 12., 24., 48.);
    CAPTURE(dB);
    REQUIRE_THAT(DBToAmp(dB), WithinRel(std::pow(10., dB / 20.), 1e-9));
    const double amp = std::pow(10., dB / 20.);
    REQUIRE_THAT(AmpToDB(amp), WithinAbs(20. * std::log10(amp), 1e-9));
  }

  SECTION("are inverses of each other")
  {
    const double dB = GENERATE(-100., -24., -6., -1.5, 0., 0.5, 6., 18.);
    CAPTURE(dB);
    REQUIRE_THAT(AmpToDB(DBToAmp(dB)), WithinAbs(dB, 1e-9));
    const double amp = DBToAmp(dB);
    REQUIRE_THAT(DBToAmp(AmpToDB(amp)), WithinRel(amp, 1e-9));
  }

  SECTION("AmpToDB uses the magnitude, so polarity does not matter")
  {
    REQUIRE(AmpToDB(-1.) == 0.);
    REQUIRE(AmpToDB(-0.5) == AmpToDB(0.5));
    REQUIRE(AmpToDB(-10.) == Approx(20.));
  }

  SECTION("silence is minus infinity dB")
  {
    const double dB = AmpToDB(0.);
    REQUIRE(std::isinf(dB));
    REQUIRE(dB < 0.);
    REQUIRE(DBToAmp(dB) == 0.);
  }

  SECTION("DBToAmp is monotonic and always positive")
  {
    double prev = 0.;
    for (int dB = -200; dB <= 60; dB += 10)
    {
      const double amp = DBToAmp(dB);
      REQUIRE(amp > prev);
      prev = amp;
    }
  }

  SECTION("the conversion constants are reciprocal")
  {
    REQUIRE_THAT(AMP_DB * IAMP_DB, WithinAbs(1., 1e-12));
    REQUIRE_THAT(AMP_DB, WithinRel(20. / std::log(10.), 1e-12));
    REQUIRE_THAT(IAMP_DB, WithinRel(std::log(10.) / 20., 1e-12));
  }
}

TEST_CASE("Version helpers", "[unit][utilities]")
{
  SECTION("GetVersionParts unpacks 0xMMMMmmpp")
  {
    struct Case { int version; int maj; int min; int pat; };
    auto c = GENERATE(Case{0x00010203, 1, 2, 3}, Case{0x00000000, 0, 0, 0}, Case{0x00010000, 1, 0, 0},
                      Case{0x00000100, 0, 1, 0}, Case{0x00000001, 0, 0, 1}, Case{0x000A0B0C, 10, 11, 12},
                      Case{0x0001FFFF, 1, 255, 255}, Case{0x7FFF0000, 32767, 0, 0}, Case{0x01234567, 0x123, 0x45, 0x67});
    CAPTURE(c.version);

    int maj = -1, min = -1, pat = -1;
    GetVersionParts(c.version, maj, min, pat);
    REQUIRE(maj == c.maj);
    REQUIRE(min == c.min);
    REQUIRE(pat == c.pat);
  }

  SECTION("GetDecimalVersion")
  {
    REQUIRE(GetDecimalVersion(0x00010203) == 10203);
    REQUIRE(GetDecimalVersion(0x00000000) == 0);
    REQUIRE(GetDecimalVersion(0x00010000) == 10000);
    REQUIRE(GetDecimalVersion(0x00000100) == 100);
    REQUIRE(GetDecimalVersion(0x00000001) == 1);
    REQUIRE(GetDecimalVersion(0x000C2D3F) == 124563);
    REQUIRE(GetDecimalVersion(0x00630909) == 990909);
  }

  SECTION("GetVersionStr")
  {
    REQUIRE(VersionStr(0x00010203) == "v1.2.3");
    REQUIRE(VersionStr(0x00000000) == "v0.0.0");
    REQUIRE(VersionStr(0x000A0000) == "v10.0.0");
    REQUIRE(VersionStr(0x0001FFFF) == "v1.255.255");
    REQUIRE(VersionStr(0x7FFFFFFF) == "v32767.255.255");

    // the previous contents of the string are replaced
    WDL_String str("something else entirely");
    GetVersionStr(0x00020100, str);
    REQUIRE(std::string(str.Get()) == "v2.1.0");
  }

  SECTION("the IPlug version constant unpacks sensibly")
  {
    int maj = 0, min = 0, pat = 0;
    GetVersionParts(IPLUG_VERSION, maj, min, pat);
    REQUIRE(maj == 1);
    REQUIRE(min == 0);
    REQUIRE(pat == 0);
    REQUIRE(VersionStr(IPLUG_VERSION) == "v1.0.0");
    REQUIRE(GetDecimalVersion(IPLUG_VERSION) == 10000);
  }
}

TEST_CASE("CastCopy: converts between sample types", "[unit][utilities]")
{
  SECTION("float to double")
  {
    float src[5] = {0.f, 0.5f, -0.25f, 1.f, -1.f};
    double dst[5] = {9., 9., 9., 9., 9.};
    CastCopy(dst, src, 5);
    for (int i = 0; i < 5; i++)
      REQUIRE(dst[i] == static_cast<double>(src[i]));
  }

  SECTION("double to float")
  {
    double src[4] = {0.1, -0.2, 1e-3, 123456.789};
    float dst[4] = {9.f, 9.f, 9.f, 9.f};
    CastCopy(dst, src, 4);
    for (int i = 0; i < 4; i++)
      REQUIRE(dst[i] == static_cast<float>(src[i]));
  }

  SECTION("only n elements are written")
  {
    float src[4] = {1.f, 2.f, 3.f, 4.f};
    double dst[4] = {-1., -1., -1., -1.};
    CastCopy(dst, src, 2);
    REQUIRE(dst[0] == 1.);
    REQUIRE(dst[1] == 2.);
    REQUIRE(dst[2] == -1.);
    REQUIRE(dst[3] == -1.);

    CastCopy(dst, src, 0);
    REQUIRE(dst[0] == 1.);
  }

  SECTION("integer conversions truncate like a cast")
  {
    double src[3] = {1.9, -1.9, 100.5};
    int dst[3] = {0, 0, 0};
    CastCopy(dst, src, 3);
    REQUIRE(dst[0] == 1);
    REQUIRE(dst[1] == -1);
    REQUIRE(dst[2] == 100);
  }
}

TEST_CASE("ToLower", "[unit][utilities]")
{
  REQUIRE(Lower("REAPER") == "reaper");
  REQUIRE(Lower("Ableton Live") == "ableton live");
  REQUIRE(Lower("already lower") == "already lower");
  REQUIRE(Lower("MiXeD 123 !?_-") == "mixed 123 !?_-");
  REQUIRE(Lower("") == "");
  REQUIRE(Lower("Z") == "z");

  SECTION("the destination is terminated and nothing is written beyond it")
  {
    char buf[8] = {'#', '#', '#', '#', '#', '#', '#', '#'};
    ToLower(buf, "ABC");
    REQUIRE(buf[0] == 'a');
    REQUIRE(buf[1] == 'b');
    REQUIRE(buf[2] == 'c');
    REQUIRE(buf[3] == '\0');
    REQUIRE(buf[4] == '#');
  }

  SECTION("can convert in place")
  {
    char buf[16] = "In Place";
    ToLower(buf, buf);
    REQUIRE(std::string(buf) == "in place");
  }
}

TEST_CASE("LookUpHost: identifies hosts from their reported names", "[unit][utilities][host]")
{
  struct Case { const char* name; EHost host; };

  SECTION("typical host name strings")
  {
    auto c = GENERATE(Case{"REAPER", kHostReaper},
                      Case{"reaper", kHostReaper},
                      Case{"Reaper64.exe", kHostReaper},
                      Case{"ProTools", kHostProTools},
                      Case{"Cubase 12 Pro", kHostCubase},
                      Case{"Nuendo", kHostNuendo},
                      Case{"Cakewalk by BandLab", kHostSonar},
                      Case{"Vegas Pro", kHostVegas},
                      Case{"Fruity Wrapper", kHostFL},
                      Case{"Samplitude", kHostSamplitude},
                      Case{"Live", kHostAbletonLive},
                      Case{"Ableton Live 11 Suite", kHostAbletonLive},
                      Case{"Tracktion", kHostTracktion},
                      Case{"Melodyne", kHostMelodyneStudio},
                      Case{"AULab", kHostAULab},
                      Case{"Adobe Audition", kHostAudition},
                      Case{"Logic Pro X", kHostLogic},
                      Case{"GarageBand", kHostGarageBand},
                      Case{"Digital Performer", kHostDigitalPerformer},
                      Case{"AudioMulch", kHostAudioMulch},
                      Case{"PreSonus Studio One", kHostStudioOne},
                      Case{"VST3PluginTestHost", kHostVST3TestHost},
                      Case{"Ardour", kHostArdour},
                      Case{"Renoise", kHostRenoise},
                      Case{"OpenMPT", kHostOpenMPT},
                      Case{"WaveLab Pro", kHostWaveLab},
                      Case{"WaveLab Elements 11", kHostWaveLabElements},
                      Case{"TwistedWave", kHostTwistedWave},
                      Case{"Bitwig Studio", kHostBitwig},
                      Case{"Reason", kHostReason},
                      Case{"Waveform 12", kHostWaveform},
                      Case{"Audacity", kHostAudacity},
                      Case{"Acoustica", kHostAcoustica},
                      Case{"PluginDoctor", kHostPluginDoctor},
                      Case{"iZotope RX 10", kHostiZotopeRX},
                      Case{"SAVIHost", kHostSAVIHost},
                      Case{"Blue Cat's VST Host", kHostBlueCat},
                      Case{"Mixbus", kHostMixbus32C},
                      Case{"standalone", kHostStandalone},
                      Case{"Standalone App", kHostStandalone},
                      Case{"www", kHostWWW});
    CAPTURE(c.name);
    REQUIRE(LookUpHost(c.name) == c.host);
  }

  SECTION("unrecognised names")
  {
    const std::string name = GENERATE(as<std::string> {}, "", " ", "SomeUnknownDAW", "12345", "host");
    CAPTURE(name);
    REQUIRE(LookUpHost(name.c_str()) == kHostUnknown);
  }

  SECTION("kHostUnknown is zero and kHostUninit is distinct from it")
  {
    REQUIRE(kHostUnknown == 0);
    REQUIRE(kHostUninit == -1);
  }
}

TEST_CASE("GetHostNameStr", "[unit][utilities][host]")
{
  SECTION("known values")
  {
    REQUIRE(HostName(kHostReaper) == "reaper");
    REQUIRE(HostName(kHostProTools) == "protools");
    REQUIRE(HostName(kHostAbletonLive) == "live");
    REQUIRE(HostName(kHostLogic) == "logic");
    REQUIRE(HostName(kHostStudioOne) == "presonus");
    REQUIRE(HostName(kHostBitwig) == "bitwig studio");
    REQUIRE(HostName(kHostStandalone) == "standalone");
    REQUIRE(HostName(kHostWWW) == "www");
  }

  SECTION("unknown hosts")
  {
    REQUIRE(HostName(kHostUnknown) == "Unknown");
    REQUIRE(HostName(kHostUninit) == "Unknown");
  }

  SECTION("every host has a name that LookUpHost maps back to it")
  {
    const int host = GENERATE(range(static_cast<int>(kHostReaper), static_cast<int>(kHostMixbus32C) + 1));
    const std::string name = HostName(static_cast<EHost>(host));
    CAPTURE(host, name);
    REQUIRE_FALSE(name.empty());
    REQUIRE(name != "Unknown");
    REQUIRE(static_cast<int>(LookUpHost(name.c_str())) == host);
  }
}

TEST_CASE("MidiNoteName", "[unit][utilities]")
{
  SECTION("middle C is C3 by default, or C4 on request")
  {
    REQUIRE(NoteName(60.) == "C 3");
    REQUIRE(NoteName(60., false, true) == "C 4");
    REQUIRE(NoteName(69.) == "A 3");
    REQUIRE(NoteName(69., false, true) == "A 4");
  }

  SECTION("the pitch classes of an octave")
  {
    const char* expected[12] = {"C 3", "C#3", "D 3", "D#3", "E 3", "F 3", "F#3", "G 3", "G#3", "A 3", "A#3", "B 3"};
    for (int i = 0; i < 12; i++)
    {
      CAPTURE(i);
      REQUIRE(NoteName(60. + i) == expected[i]);
    }
  }

  SECTION("the extremes of the MIDI range")
  {
    REQUIRE(NoteName(0.) == "C -2");
    REQUIRE(NoteName(0., false, true) == "C -1");
    REQUIRE(NoteName(11.) == "B -2");
    REQUIRE(NoteName(12.) == "C -1");
    REQUIRE(NoteName(24.) == "C 0");
    REQUIRE(NoteName(127.) == "G 8");
    REQUIRE(NoteName(127., false, true) == "G 9");
  }

  SECTION("octaves change at C")
  {
    const int octave = GENERATE(range(0, 10));
    WDL_String expected;
    expected.SetFormatted(32, "C %i", octave - 2);
    REQUIRE(NoteName(octave * 12.) == expected.Get());
    expected.SetFormatted(32, "B %i", octave - 2);
    REQUIRE(NoteName(octave * 12. + 11.) == expected.Get());
  }

  SECTION("fractional pitches round to the nearest note")
  {
    REQUIRE(NoteName(59.6) == "C 3");
    REQUIRE(NoteName(60.4) == "C 3");
    REQUIRE(NoteName(60.6) == "C#3");
    REQUIRE(NoteName(59.4) == "B 2");
  }

  SECTION("cents show the deviation from the nearest note")
  {
    REQUIRE(NoteName(60., true) == "C 3 0");
    REQUIRE(NoteName(60.25, true) == "C 3 25");
    REQUIRE(NoteName(59.75, true) == "C 3 -25");
    REQUIRE(NoteName(61.1, true) == "C#3 10");
    REQUIRE(NoteName(60.9, true) == "C#3 -10");
    REQUIRE(NoteName(69.25, true, true) == "A 4 25");
  }
}

TEST_CASE("UTF-8 / UTF-16 conversion", "[unit][utilities][utf]")
{
  SECTION("null and empty input")
  {
    REQUIRE(UTF8ToUTF16String(nullptr).empty());
    REQUIRE(UTF8ToUTF16String("").empty());
    REQUIRE(UTF16ToUTF8String(std::u16string()).empty());
  }

  SECTION("ASCII is unchanged")
  {
    const std::u16string u16 = UTF8ToUTF16String("Hello, iPlug 2!");
    REQUIRE(u16 == u"Hello, iPlug 2!");
    REQUIRE(UTF16ToUTF8String(u16) == "Hello, iPlug 2!");
  }

  SECTION("two byte sequences")
  {
    const std::u16string u16 = UTF8ToUTF16String("\xC3\xA9"); // U+00E9
    REQUIRE(u16.size() == 1);
    REQUIRE(u16[0] == 0x00E9);
    REQUIRE(UTF16ToUTF8String(u16) == "\xC3\xA9");
  }

  SECTION("three byte sequences")
  {
    const std::u16string u16 = UTF8ToUTF16String("\xE2\x82\xAC"); // U+20AC
    REQUIRE(u16.size() == 1);
    REQUIRE(u16[0] == 0x20AC);
    REQUIRE(UTF16ToUTF8String(u16) == "\xE2\x82\xAC");
  }

  SECTION("four byte sequences become surrogate pairs")
  {
    const std::u16string u16 = UTF8ToUTF16String("\xF0\x9F\x98\x80"); // U+1F600
    REQUIRE(u16.size() == 2);
    REQUIRE(u16[0] == 0xD83D);
    REQUIRE(u16[1] == 0xDE00);
    REQUIRE(UTF16ToUTF8String(u16) == "\xF0\x9F\x98\x80");

    const std::u16string first = UTF8ToUTF16String("\xF0\x90\x80\x80"); // U+10000
    REQUIRE(first.size() == 2);
    REQUIRE(first[0] == 0xD800);
    REQUIRE(first[1] == 0xDC00);
    REQUIRE(UTF16ToUTF8String(first) == "\xF0\x90\x80\x80");
  }

  SECTION("mixed strings round trip")
  {
    const std::string str = GENERATE(as<std::string> {}, "plain", "na\xC3\xAFve caf\xC3\xA9",
                                     "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E", "a\xF0\x9F\x8E\xB9z",
                                     "\xCE\xA9 = 1 \xE2\x84\xA6", "tab\tand\nnewline");
    REQUIRE(UTF16ToUTF8String(UTF8ToUTF16String(str.c_str())) == str);
  }

  SECTION("code unit counts")
  {
    REQUIRE(UTF8ToUTF16String("na\xC3\xAFve").size() == 5);
    REQUIRE(UTF8ToUTF16String("\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E").size() == 3);
    REQUIRE(UTF8ToUTF16String("a\xF0\x9F\x8E\xB9z").size() == 4);
  }

  SECTION("single code unit overload")
  {
    REQUIRE(UTF16ToUTF8String(u'A') == "A");
    REQUIRE(UTF16ToUTF8String(static_cast<char16_t>(0x00E9)) == "\xC3\xA9");
    REQUIRE(UTF16ToUTF8String(static_cast<char16_t>(0x20AC)) == "\xE2\x82\xAC");
  }

  SECTION("pointer range overload")
  {
    const std::u16string u16 = u"range test";
    REQUIRE(UTF16ToUTF8String(u16.data(), u16.data() + u16.size()) == "range test");
    REQUIRE(UTF16ToUTF8String(u16.data(), u16.data() + 5) == "range");
    REQUIRE(UTF16ToUTF8String(u16.data(), u16.data()).empty());

    const std::u16string pair = UTF8ToUTF16String("\xF0\x9F\x98\x80!");
    REQUIRE(UTF16ToUTF8String(pair.data(), pair.data() + pair.size()) == "\xF0\x9F\x98\x80!");
  }
}

TEST_CASE("DOMKeyToVirtualKey", "[unit][utilities][keys]")
{
  SECTION("control keys")
  {
    struct Case { uint32_t dom; int vk; };
    auto c = GENERATE(Case{DOM_VK_BACK_SPACE, kVK_BACK}, Case{DOM_VK_TAB, kVK_TAB}, Case{DOM_VK_RETURN, kVK_RETURN},
                      Case{DOM_VK_SHIFT, kVK_SHIFT}, Case{DOM_VK_CONTROL, kVK_CONTROL}, Case{DOM_VK_ALT, kVK_MENU},
                      Case{DOM_VK_PAUSE, kVK_PAUSE}, Case{DOM_VK_CAPS_LOCK, kVK_CAPITAL}, Case{DOM_VK_ESCAPE, kVK_ESCAPE},
                      Case{DOM_VK_SPACE, kVK_SPACE}, Case{DOM_VK_PAGE_UP, kVK_PRIOR}, Case{DOM_VK_PAGE_DOWN, kVK_NEXT},
                      Case{DOM_VK_END, kVK_END}, Case{DOM_VK_HOME, kVK_HOME}, Case{DOM_VK_LEFT, kVK_LEFT},
                      Case{DOM_VK_UP, kVK_UP}, Case{DOM_VK_RIGHT, kVK_RIGHT}, Case{DOM_VK_DOWN, kVK_DOWN},
                      Case{DOM_VK_INSERT, kVK_INSERT}, Case{DOM_VK_DELETE, kVK_DELETE});
    CAPTURE(c.dom);
    REQUIRE(DOMKeyToVirtualKey(c.dom) == c.vk);
  }

  SECTION("letters")
  {
    for (uint32_t key = 'A'; key <= 'Z'; key++)
    {
      CAPTURE(key);
      REQUIRE(DOMKeyToVirtualKey(key) == static_cast<int>(kVK_A + (key - 'A')));
    }
    REQUIRE(DOMKeyToVirtualKey('A') == kVK_A);
    REQUIRE(DOMKeyToVirtualKey('Z') == kVK_Z);
  }

  SECTION("digits, on the main keyboard and the number pad")
  {
    for (uint32_t digit = 0; digit < 10; digit++)
    {
      CAPTURE(digit);
      REQUIRE(DOMKeyToVirtualKey('0' + digit) == static_cast<int>(kVK_0 + digit));
      REQUIRE(DOMKeyToVirtualKey(DOM_VK_NUMPAD0 + digit) == static_cast<int>(kVK_0 + digit));
    }
  }

  SECTION("function keys")
  {
    for (uint32_t f = 0; f < 12; f++)
    {
      CAPTURE(f);
      REQUIRE(DOMKeyToVirtualKey(DOM_VK_F1 + f) == static_cast<int>(kVK_F1 + f));
    }
  }

  SECTION("unmapped codes give zero")
  {
    const uint32_t key = GENERATE(0u, 1u, 0x0Cu, 0x3Au, 0x40u, 0x5Bu, 0x6Au, 0x7Cu, 0xFFu, 0x100u, 0xFFFFFFFFu);
    CAPTURE(key);
    REQUIRE(DOMKeyToVirtualKey(key) == 0);
  }
}

TEST_CASE("IPlugConstants: values that other code depends on", "[unit][constants]")
{
  SECTION("maths and audio constants")
  {
    REQUIRE_THAT(PI, WithinAbs(std::acos(-1.), 1e-15));
    REQUIRE(DEFAULT_SAMPLE_RATE == 48000.);
    REQUIRE(DEFAULT_TEMPO == 120.);
  }

  SECTION("sentinel values")
  {
    REQUIRE(kNoParameter == -1);
    REQUIRE(kNoValIdx == -1);
    REQUIRE(kNoTag == -1);
  }

  SECTION("sample types are one float and one double")
  {
    REQUIRE(sizeof(PLUG_SAMPLE_DST) != sizeof(PLUG_SAMPLE_SRC));
    REQUIRE(sizeof(sample) == sizeof(PLUG_SAMPLE_DST));
    REQUIRE(sizeof(PLUG_SAMPLE_DST) + sizeof(PLUG_SAMPLE_SRC) == sizeof(float) + sizeof(double));
  }

  SECTION("routing directions index the two-element arrays")
  {
    REQUIRE(ERoute::kInput == 0);
    REQUIRE(ERoute::kOutput == 1);
    REQUIRE(std::string(RoutingDirStrs[ERoute::kInput]) == "Input");
    REQUIRE(std::string(RoutingDirStrs[ERoute::kOutput]) == "Output");
  }

  SECTION("every parameter source has a name")
  {
    REQUIRE(std::string(ParamSourceStrs[kReset]) == "Reset");
    REQUIRE(std::string(ParamSourceStrs[kHost]) == "Host");
    REQUIRE(std::string(ParamSourceStrs[kPresetRecall]) == "Preset");
    REQUIRE(std::string(ParamSourceStrs[kUI]) == "UI");
    REQUIRE(std::string(ParamSourceStrs[kDelegate]) == "Editor Delegate");
    REQUIRE(std::string(ParamSourceStrs[kRecompile]) == "Recompile");
    REQUIRE(std::string(ParamSourceStrs[kUnknown]) == "Unknown");
    REQUIRE(kNumParamSources == 7);
  }

  SECTION("plug-in types and APIs keep their numeric values")
  {
    REQUIRE(kEffect == 0);
    REQUIRE(kInstrument == 1);
    REQUIRE(kMIDIEffect == 2);
    REQUIRE(kGenerator == 3);

    REQUIRE(kAPIVST2 == 0);
    REQUIRE(kAPIVST3 == 1);
    REQUIRE(kAPIAU == 2);
    REQUIRE(kAPIAUv3 == 3);
    REQUIRE(kAPIAAX == 4);
    REQUIRE(kAPIAPP == 5);
    REQUIRE(kAPIWAM == 6);
    REQUIRE(kAPIWEB == 7);
    REQUIRE(kAPICLAP == 8);
  }

  SECTION("VST3 preset chunk identifiers")
  {
    REQUIRE(std::string(commonChunks[kHeader], 4) == "VST3");
    REQUIRE(std::string(commonChunks[kComponentState], 4) == "Comp");
    REQUIRE(std::string(commonChunks[kControllerState], 4) == "Cont");
    REQUIRE(std::string(commonChunks[kProgramData], 4) == "Prog");
    REQUIRE(std::string(commonChunks[kMetaInfo], 4) == "Info");
    REQUIRE(std::string(commonChunks[kChunkList], 4) == "List");
    REQUIRE(kNumPresetChunks == 6);
    REQUIRE(kFormatVersion == 1);
    REQUIRE(kClassIDSize == 32);
    REQUIRE(kHeaderSize == 48);
  }

  SECTION("virtual keys match the win32 values")
  {
    REQUIRE(kVK_BACK == 0x08);
    REQUIRE(kVK_RETURN == 0x0D);
    REQUIRE(kVK_ESCAPE == 0x1B);
    REQUIRE(kVK_SPACE == 0x20);
    REQUIRE(kVK_0 == '0');
    REQUIRE(kVK_9 == '9');
    REQUIRE(kVK_A == 'A');
    REQUIRE(kVK_Z == 'Z');
    REQUIRE(kVK_F1 == 0x70);
    REQUIRE(kVK_F24 == 0x87);
    REQUIRE(kFSHIFT == 0x04);
    REQUIRE(kFCONTROL == 0x08);
    REQUIRE(kFALT == 0x10);
  }
}
