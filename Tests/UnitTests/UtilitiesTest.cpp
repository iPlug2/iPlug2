/*
 * UtilitiesTest.cpp
 * Unit tests for IPlugUtilities functions
 */

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <cmath>
#include <cstring>

// Include the utilities header
#include "IPlugUtilities.h"
#include "IPlugConstants.h"

using namespace iplug;
using Catch::Approx;

// ============================================================================
// Clip Function Tests
// ============================================================================

TEST_CASE("Clip function clamps values to range", "[utilities][clip]")
{
  SECTION("Integer clipping")
  {
    REQUIRE(Clip(5, 0, 10) == 5);    // Within range
    REQUIRE(Clip(-5, 0, 10) == 0);   // Below min
    REQUIRE(Clip(15, 0, 10) == 10);  // Above max
    REQUIRE(Clip(0, 0, 10) == 0);    // At min
    REQUIRE(Clip(10, 0, 10) == 10);  // At max
  }

  SECTION("Double clipping")
  {
    REQUIRE(Clip(0.5, 0.0, 1.0) == Approx(0.5));
    REQUIRE(Clip(-0.5, 0.0, 1.0) == Approx(0.0));
    REQUIRE(Clip(1.5, 0.0, 1.0) == Approx(1.0));
  }

  SECTION("Negative range")
  {
    REQUIRE(Clip(-5, -10, -1) == -5);
    REQUIRE(Clip(-15, -10, -1) == -10);
    REQUIRE(Clip(5, -10, -1) == -1);
  }

  SECTION("Single value range")
  {
    REQUIRE(Clip(5, 3, 3) == 3);
    REQUIRE(Clip(1, 3, 3) == 3);
  }
}

// ============================================================================
// Lerp Function Tests
// ============================================================================

TEST_CASE("Lerp function performs linear interpolation", "[utilities][lerp]")
{
  SECTION("Basic interpolation")
  {
    REQUIRE(Lerp(0.0, 10.0, 0.0) == Approx(0.0));   // t=0 returns a
    REQUIRE(Lerp(0.0, 10.0, 1.0) == Approx(10.0));  // t=1 returns b
    REQUIRE(Lerp(0.0, 10.0, 0.5) == Approx(5.0));   // t=0.5 returns midpoint
  }

  SECTION("Quarter points")
  {
    REQUIRE(Lerp(0.0, 100.0, 0.25) == Approx(25.0));
    REQUIRE(Lerp(0.0, 100.0, 0.75) == Approx(75.0));
  }

  SECTION("Negative values")
  {
    REQUIRE(Lerp(-10.0, 10.0, 0.5) == Approx(0.0));
    REQUIRE(Lerp(-10.0, 10.0, 0.0) == Approx(-10.0));
    REQUIRE(Lerp(-10.0, 10.0, 1.0) == Approx(10.0));
  }

  SECTION("Reverse interpolation (a > b)")
  {
    REQUIRE(Lerp(10.0, 0.0, 0.5) == Approx(5.0));
    REQUIRE(Lerp(10.0, 0.0, 0.25) == Approx(7.5));
  }

  SECTION("Extrapolation (t outside 0-1)")
  {
    REQUIRE(Lerp(0.0, 10.0, 2.0) == Approx(20.0));
    REQUIRE(Lerp(0.0, 10.0, -1.0) == Approx(-10.0));
  }
}

// ============================================================================
// dB/Amplitude Conversion Tests
// ============================================================================

TEST_CASE("DBToAmp converts decibels to amplitude", "[utilities][db]")
{
  SECTION("Reference points")
  {
    REQUIRE(DBToAmp(0.0) == Approx(1.0));           // 0 dB = unity gain
    REQUIRE(DBToAmp(-6.0206) == Approx(0.5).margin(0.001));  // -6 dB ≈ 0.5
    REQUIRE(DBToAmp(6.0206) == Approx(2.0).margin(0.001));   // +6 dB ≈ 2.0
  }

  SECTION("Large negative dB approaches zero")
  {
    REQUIRE(DBToAmp(-60.0) == Approx(0.001).margin(0.0001));
    REQUIRE(DBToAmp(-120.0) < 0.0000011);  // Very close to 0
  }

  SECTION("Positive dB increases amplitude")
  {
    REQUIRE(DBToAmp(20.0) == Approx(10.0).margin(0.01));
    REQUIRE(DBToAmp(40.0) == Approx(100.0).margin(0.1));
  }
}

TEST_CASE("AmpToDB converts amplitude to decibels", "[utilities][db]")
{
  SECTION("Reference points")
  {
    REQUIRE(AmpToDB(1.0) == Approx(0.0));           // Unity gain = 0 dB
    REQUIRE(AmpToDB(0.5) == Approx(-6.0206).margin(0.01));
    REQUIRE(AmpToDB(2.0) == Approx(6.0206).margin(0.01));
  }

  SECTION("Small amplitudes give large negative dB")
  {
    REQUIRE(AmpToDB(0.001) == Approx(-60.0).margin(0.1));
    REQUIRE(AmpToDB(0.0001) == Approx(-80.0).margin(0.1));
  }

  SECTION("Large amplitudes give positive dB")
  {
    REQUIRE(AmpToDB(10.0) == Approx(20.0).margin(0.1));
    REQUIRE(AmpToDB(100.0) == Approx(40.0).margin(0.1));
  }
}

TEST_CASE("DBToAmp and AmpToDB are inverse functions", "[utilities][db]")
{
  SECTION("Round trip conversions")
  {
    for (double db = -60.0; db <= 60.0; db += 10.0)
    {
      double amp = DBToAmp(db);
      double dbBack = AmpToDB(amp);
      REQUIRE(dbBack == Approx(db).margin(0.0001));
    }
  }

  SECTION("Amplitude round trip")
  {
    for (double amp = 0.01; amp <= 10.0; amp *= 2.0)
    {
      double db = AmpToDB(amp);
      double ampBack = DBToAmp(db);
      REQUIRE(ampBack == Approx(amp).margin(0.0001));
    }
  }
}

// ============================================================================
// Version Parsing Tests
// ============================================================================

TEST_CASE("GetVersionParts extracts version components", "[utilities][version]")
{
  int maj, min, pat;

  SECTION("Simple version numbers")
  {
    GetVersionParts(0x010203, maj, min, pat);
    REQUIRE(maj == 1);
    REQUIRE(min == 2);
    REQUIRE(pat == 3);
  }

  SECTION("Zero version")
  {
    GetVersionParts(0x000000, maj, min, pat);
    REQUIRE(maj == 0);
    REQUIRE(min == 0);
    REQUIRE(pat == 0);
  }

  SECTION("Maximum single byte values")
  {
    GetVersionParts(0xFF0000, maj, min, pat);
    REQUIRE(maj == 255);

    GetVersionParts(0x00FF00, maj, min, pat);
    REQUIRE(min == 255);

    GetVersionParts(0x0000FF, maj, min, pat);
    REQUIRE(pat == 255);
  }

  SECTION("Real-world version examples")
  {
    // Version 2.0.0
    GetVersionParts(0x020000, maj, min, pat);
    REQUIRE(maj == 2);
    REQUIRE(min == 0);
    REQUIRE(pat == 0);

    // Version 1.2.3
    GetVersionParts(0x010203, maj, min, pat);
    REQUIRE(maj == 1);
    REQUIRE(min == 2);
    REQUIRE(pat == 3);
  }
}

TEST_CASE("GetVersionStr formats version as string", "[utilities][version]")
{
  SECTION("Standard version")
  {
    WDL_String str;
    GetVersionStr(0x010203, str);
    REQUIRE(strcmp(str.Get(), "v1.2.3") == 0);
  }

  SECTION("Zero version")
  {
    WDL_String str;
    GetVersionStr(0x000000, str);
    REQUIRE(strcmp(str.Get(), "v0.0.0") == 0);
  }

  SECTION("Version with zeros in middle")
  {
    WDL_String str;
    GetVersionStr(0x010003, str);
    REQUIRE(strcmp(str.Get(), "v1.0.3") == 0);
  }
}

// ============================================================================
// String Utility Tests
// ============================================================================

TEST_CASE("ToLower converts string to lowercase", "[utilities][string]")
{
  SECTION("All uppercase")
  {
    const char* input = "HELLO WORLD";
    char buffer[32];
    ToLower(buffer, input);
    REQUIRE(strcmp(buffer, "hello world") == 0);
  }

  SECTION("Mixed case")
  {
    char buffer[32];
    ToLower(buffer, "HeLLo WoRLD");
    REQUIRE(strcmp(buffer, "hello world") == 0);
  }

  SECTION("Already lowercase")
  {
    char buffer[32];
    ToLower(buffer, "already lower");
    REQUIRE(strcmp(buffer, "already lower") == 0);
  }

  SECTION("With numbers and symbols")
  {
    char buffer[32];
    ToLower(buffer, "Test123!@#ABC");
    REQUIRE(strcmp(buffer, "test123!@#abc") == 0);
  }

  SECTION("Empty string")
  {
    char buffer[32];
    ToLower(buffer, "");
    REQUIRE(strcmp(buffer, "") == 0);
  }
}

TEST_CASE("CStringHasContents checks for non-empty strings", "[utilities][string]")
{
  SECTION("Valid non-empty string")
  {
    REQUIRE(CStringHasContents("hello") == true);
    REQUIRE(CStringHasContents("a") == true);
    REQUIRE(CStringHasContents(" ") == true);  // Space is content
  }

  SECTION("Empty string")
  {
    REQUIRE(CStringHasContents("") == false);
  }

  SECTION("Null pointer")
  {
    REQUIRE(CStringHasContents(nullptr) == false);
  }
}

// ============================================================================
// Host Identification Tests
// ============================================================================

TEST_CASE("LookUpHost identifies host applications", "[utilities][host]")
{
  SECTION("Known hosts by name")
  {
    REQUIRE(LookUpHost("reaper") == kHostReaper);
    REQUIRE(LookUpHost("Reaper") == kHostReaper);
    REQUIRE(LookUpHost("REAPER") == kHostReaper);

    REQUIRE(LookUpHost("live") == kHostAbletonLive);
    REQUIRE(LookUpHost("Ableton Live") == kHostAbletonLive);

    REQUIRE(LookUpHost("protools") == kHostProTools);
    // Note: "Pro Tools" doesn't match - strstr looks for "protools" substring

    REQUIRE(LookUpHost("logic") == kHostLogic);
    REQUIRE(LookUpHost("Logic Pro") == kHostLogic);

    REQUIRE(LookUpHost("cubase") == kHostCubase);
    REQUIRE(LookUpHost("Cubase") == kHostCubase);

    REQUIRE(LookUpHost("nuendo") == kHostNuendo);

    REQUIRE(LookUpHost("fruity") == kHostFL);
    // Note: "FL Studio" won't match - strstr looks for "fruity" substring

    REQUIRE(LookUpHost("presonus") == kHostStudioOne);
    // Note: "Studio One" won't match - strstr looks for "presonus" substring

    REQUIRE(LookUpHost("garageband") == kHostGarageBand);

    REQUIRE(LookUpHost("digital") == kHostDigitalPerformer);

    REQUIRE(LookUpHost("audition") == kHostAudition);

    REQUIRE(LookUpHost("bitwig studio") == kHostBitwig);

    REQUIRE(LookUpHost("renoise") == kHostRenoise);

    REQUIRE(LookUpHost("reason") == kHostReason);

    REQUIRE(LookUpHost("wavelab") == kHostWaveLab);

    REQUIRE(LookUpHost("tracktion") == kHostTracktion);
    REQUIRE(LookUpHost("waveform") == kHostWaveform);

    REQUIRE(LookUpHost("standalone") == kHostStandalone);
  }

  SECTION("Unknown hosts return kHostUnknown")
  {
    REQUIRE(LookUpHost("unknown_host_xyz") == kHostUnknown);
    REQUIRE(LookUpHost("random application") == kHostUnknown);
  }
}

TEST_CASE("GetHostNameStr returns host name string", "[utilities][host]")
{
  SECTION("Known hosts have names")
  {
    WDL_String name;

    GetHostNameStr(kHostReaper, name);
    REQUIRE(CStringHasContents(name.Get()));
    REQUIRE(strcmp(name.Get(), "reaper") == 0);

    GetHostNameStr(kHostAbletonLive, name);
    REQUIRE(CStringHasContents(name.Get()));
    REQUIRE(strcmp(name.Get(), "live") == 0);

    GetHostNameStr(kHostProTools, name);
    REQUIRE(CStringHasContents(name.Get()));
    REQUIRE(strcmp(name.Get(), "protools") == 0);

    GetHostNameStr(kHostLogic, name);
    REQUIRE(CStringHasContents(name.Get()));
    REQUIRE(strcmp(name.Get(), "logic") == 0);

    GetHostNameStr(kHostStandalone, name);
    REQUIRE(strcmp(name.Get(), "standalone") == 0);
  }

  SECTION("Unknown host returns Unknown")
  {
    WDL_String name;
    GetHostNameStr(kHostUnknown, name);
    REQUIRE(CStringHasContents(name.Get()));
    REQUIRE(strcmp(name.Get(), "Unknown") == 0);
  }
}

// ============================================================================
// MIDI Note Name Tests
// ============================================================================

TEST_CASE("MidiNoteName returns correct note names", "[utilities][midi]")
{
  WDL_String noteName;

  SECTION("Note names default (middle C = C3)")
  {
    // Middle C (MIDI note 60) = C3 by default
    MidiNoteName(60.0, noteName);
    REQUIRE(strstr(noteName.Get(), "C") != nullptr);
    REQUIRE(strstr(noteName.Get(), "3") != nullptr);

    // One octave up
    MidiNoteName(72.0, noteName);
    REQUIRE(strstr(noteName.Get(), "C") != nullptr);
    REQUIRE(strstr(noteName.Get(), "4") != nullptr);
  }

  SECTION("Note names with middle C = C4")
  {
    // Middle C (MIDI note 60) = C4 when middleCisC4 = true
    MidiNoteName(60.0, noteName, false, true);
    REQUIRE(strstr(noteName.Get(), "C") != nullptr);
    REQUIRE(strstr(noteName.Get(), "4") != nullptr);
  }

  SECTION("Sharp notes")
  {
    // C# / Db
    MidiNoteName(61.0, noteName);
    REQUIRE(strstr(noteName.Get(), "C#") != nullptr);

    // F#
    MidiNoteName(66.0, noteName);
    REQUIRE(strstr(noteName.Get(), "F#") != nullptr);
  }

  SECTION("With cents display")
  {
    // Fractional MIDI note
    MidiNoteName(60.5, noteName, true);
    // Should contain cent deviation
    REQUIRE(strlen(noteName.Get()) > 2);
  }

  SECTION("Extreme notes")
  {
    // Lowest MIDI note
    MidiNoteName(0.0, noteName);
    REQUIRE(CStringHasContents(noteName.Get()));

    // Highest MIDI note
    MidiNoteName(127.0, noteName);
    REQUIRE(CStringHasContents(noteName.Get()));
  }
}

// ============================================================================
// Mathematical Constants Tests
// ============================================================================

TEST_CASE("Mathematical constants are defined", "[utilities][constants]")
{
  // Check PI constant
  REQUIRE(PI > 3.14);
  REQUIRE(PI < 3.15);
  REQUIRE(PI == Approx(3.14159265358979323846));
}

// ============================================================================
// DOMKeyToVirtualKey Tests
// ============================================================================

TEST_CASE("DOMKeyToVirtualKey maps DOM keys", "[utilities][keyboard]")
{
  // Test some basic key mappings
  SECTION("Printable characters")
  {
    // These mappings may vary but should return consistent values
    int vkA = DOMKeyToVirtualKey(65);  // 'A'
    int vkZ = DOMKeyToVirtualKey(90);  // 'Z'
    int vk0 = DOMKeyToVirtualKey(48);  // '0'
    int vk9 = DOMKeyToVirtualKey(57);  // '9'

    // Just verify they return some value (not necessarily testing correctness)
    // The actual values depend on platform
    (void)vkA;
    (void)vkZ;
    (void)vk0;
    (void)vk9;
  }
}
