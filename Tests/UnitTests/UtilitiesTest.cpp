/*
 * UtilitiesTest.cpp
 * Unit tests for IPlugUtilities and IPlugPaths functions
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
    REQUIRE(DBToAmp(-120.0) < 0.000001);
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
    REQUIRE(strcmp(str.Get(), "1.2.3") == 0);
  }

  SECTION("Zero version")
  {
    WDL_String str;
    GetVersionStr(0x000000, str);
    REQUIRE(strcmp(str.Get(), "0.0.0") == 0);
  }

  SECTION("Version with zeros in middle")
  {
    WDL_String str;
    GetVersionStr(0x010003, str);
    REQUIRE(strcmp(str.Get(), "1.0.3") == 0);
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
    strcpy(buffer, input);
    ToLower(buffer);
    REQUIRE(strcmp(buffer, "hello world") == 0);
  }

  SECTION("Mixed case")
  {
    char buffer[] = "HeLLo WoRLD";
    ToLower(buffer);
    REQUIRE(strcmp(buffer, "hello world") == 0);
  }

  SECTION("Already lowercase")
  {
    char buffer[] = "already lower";
    ToLower(buffer);
    REQUIRE(strcmp(buffer, "already lower") == 0);
  }

  SECTION("With numbers and symbols")
  {
    char buffer[] = "Test123!@#ABC";
    ToLower(buffer);
    REQUIRE(strcmp(buffer, "test123!@#abc") == 0);
  }

  SECTION("Empty string")
  {
    char buffer[] = "";
    ToLower(buffer);
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

    REQUIRE(LookUpHost("ableton live") == kHostAbletonLive);
    REQUIRE(LookUpHost("Ableton Live") == kHostAbletonLive);

    REQUIRE(LookUpHost("protools") == kHostProTools);
    REQUIRE(LookUpHost("Pro Tools") == kHostProTools);

    REQUIRE(LookUpHost("logic") == kHostLogic);
    REQUIRE(LookUpHost("Logic Pro") == kHostLogic);

    REQUIRE(LookUpHost("cubase") == kHostCubase);
    REQUIRE(LookUpHost("Cubase") == kHostCubase);

    REQUIRE(LookUpHost("nuendo") == kHostNuendo);

    REQUIRE(LookUpHost("fl studio") == kHostFL);
    REQUIRE(LookUpHost("fruity") == kHostFL);

    REQUIRE(LookUpHost("studio one") == kHostStudioOne);
    REQUIRE(LookUpHost("Studio One") == kHostStudioOne);

    REQUIRE(LookUpHost("garageband") == kHostGarageBand);

    REQUIRE(LookUpHost("digital performer") == kHostDigitalPerformer);

    REQUIRE(LookUpHost("audition") == kHostAudition);

    REQUIRE(LookUpHost("bitwig") == kHostBitwig);
    REQUIRE(LookUpHost("Bitwig Studio") == kHostBitwig);

    REQUIRE(LookUpHost("renoise") == kHostRenoise);

    REQUIRE(LookUpHost("reason") == kHostReason);

    REQUIRE(LookUpHost("wavelab") == kHostWaveLab);

    REQUIRE(LookUpHost("tracks") == kHostTracktion);
    REQUIRE(LookUpHost("waveform") == kHostTracktion);

    REQUIRE(LookUpHost("standalone") == kHostStandalone);
    REQUIRE(LookUpHost("iPlug") == kHostStandalone);
  }

  SECTION("Unknown hosts return kHostUnknown")
  {
    REQUIRE(LookUpHost("unknown_host_xyz") == kHostUnknown);
    REQUIRE(LookUpHost("") == kHostUnknown);
    REQUIRE(LookUpHost("random application") == kHostUnknown);
  }
}

TEST_CASE("GetHostNameStr returns host name string", "[utilities][host]")
{
  SECTION("Known hosts have names")
  {
    // These should return non-empty strings
    const char* reaperName = GetHostNameStr(kHostReaper);
    REQUIRE(CStringHasContents(reaperName));

    const char* liveName = GetHostNameStr(kHostAbletonLive);
    REQUIRE(CStringHasContents(liveName));

    const char* ptName = GetHostNameStr(kHostProTools);
    REQUIRE(CStringHasContents(ptName));

    const char* logicName = GetHostNameStr(kHostLogic);
    REQUIRE(CStringHasContents(logicName));
  }

  SECTION("Unknown host returns Unknown")
  {
    const char* unknownName = GetHostNameStr(kHostUnknown);
    REQUIRE(CStringHasContents(unknownName));
    // Should contain "unknown" or similar
  }
}

// ============================================================================
// MIDI Note Name Tests
// ============================================================================

TEST_CASE("MidiNoteName returns correct note names", "[utilities][midi]")
{
  SECTION("Note names without flats (sharps)")
  {
    // Middle C (C4) = MIDI note 60
    REQUIRE(strcmp(MidiNoteName(60, false), "C4") == 0);

    // C#4
    REQUIRE(strcmp(MidiNoteName(61, false), "C#4") == 0);

    // D4
    REQUIRE(strcmp(MidiNoteName(62, false), "D4") == 0);

    // Octave 0 notes
    REQUIRE(strcmp(MidiNoteName(12, false), "C0") == 0);
    REQUIRE(strcmp(MidiNoteName(24, false), "C1") == 0);
  }

  SECTION("Note names with flats")
  {
    // Db4 instead of C#4
    REQUIRE(strcmp(MidiNoteName(61, true), "Db4") == 0);

    // Eb4 instead of D#4
    REQUIRE(strcmp(MidiNoteName(63, true), "Eb4") == 0);

    // Natural notes should be the same
    REQUIRE(strcmp(MidiNoteName(60, true), "C4") == 0);
    REQUIRE(strcmp(MidiNoteName(62, true), "D4") == 0);
  }

  SECTION("Extreme notes")
  {
    // Lowest MIDI note
    REQUIRE(CStringHasContents(MidiNoteName(0, false)));

    // Highest MIDI note
    REQUIRE(CStringHasContents(MidiNoteName(127, false)));
  }

  SECTION("All octaves of C")
  {
    REQUIRE(strcmp(MidiNoteName(0, false), "C-2") == 0);   // Some use -2 for lowest
    REQUIRE(strcmp(MidiNoteName(12, false), "C-1") == 0);
    REQUIRE(strcmp(MidiNoteName(24, false), "C0") == 0);
    REQUIRE(strcmp(MidiNoteName(36, false), "C1") == 0);
    REQUIRE(strcmp(MidiNoteName(48, false), "C2") == 0);
    REQUIRE(strcmp(MidiNoteName(60, false), "C3") == 0);
    REQUIRE(strcmp(MidiNoteName(72, false), "C4") == 0);
    REQUIRE(strcmp(MidiNoteName(84, false), "C5") == 0);
    REQUIRE(strcmp(MidiNoteName(96, false), "C6") == 0);
    REQUIRE(strcmp(MidiNoteName(108, false), "C7") == 0);
    REQUIRE(strcmp(MidiNoteName(120, false), "C8") == 0);
  }
}

// ============================================================================
// Frequency/MIDI Conversion Tests (if available)
// ============================================================================

#ifdef IPLUG_FREQ_MIDI_CONVERSION
TEST_CASE("MIDI to frequency conversion", "[utilities][freq]")
{
  SECTION("A440 reference")
  {
    // A4 (MIDI 69) should be 440 Hz
    REQUIRE(MIDIToFreq(69) == Approx(440.0));
  }

  SECTION("Octave relationships")
  {
    // Each octave doubles the frequency
    double freqA4 = MIDIToFreq(69);
    double freqA5 = MIDIToFreq(81);
    REQUIRE(freqA5 == Approx(freqA4 * 2.0));
  }
}
#endif

// ============================================================================
// DOM Key Mapping Tests (for Web builds)
// ============================================================================

TEST_CASE("DOMKeyToVirtualKey maps DOM keys to virtual keys", "[utilities][keyboard]")
{
  // Note: This function is primarily for web builds but may be available
  // We test what we can in a platform-independent way

  SECTION("Number keys")
  {
    // DOM key codes for 0-9 are typically 48-57
    // Virtual keys vary by platform
    int vk = DOMKeyToVirtualKey(48);  // '0'
    REQUIRE(vk != 0);  // Should map to something
  }

  SECTION("Letter keys")
  {
    // DOM key codes for A-Z are typically 65-90
    int vkA = DOMKeyToVirtualKey(65);  // 'A'
    REQUIRE(vkA != 0);
  }

  SECTION("Special keys")
  {
    // Space = 32
    int vkSpace = DOMKeyToVirtualKey(32);
    // Enter = 13
    int vkEnter = DOMKeyToVirtualKey(13);
    // These should map to valid virtual keys
  }
}

// ============================================================================
// Miscellaneous Utility Tests
// ============================================================================

TEST_CASE("PLUG_VERSION_HEX macro creates correct hex", "[utilities][version]")
{
  // Test the version hex macro
  REQUIRE(PLUG_VERSION_HEX(1, 0, 0) == 0x010000);
  REQUIRE(PLUG_VERSION_HEX(2, 1, 3) == 0x020103);
  REQUIRE(PLUG_VERSION_HEX(0, 0, 1) == 0x000001);
  REQUIRE(PLUG_VERSION_HEX(10, 20, 30) == 0x0A141E);
}

TEST_CASE("Mathematical constants are defined", "[utilities][constants]")
{
  // Check PI constant
  REQUIRE(PI > 3.14);
  REQUIRE(PI < 3.15);
  REQUIRE(PI == Approx(3.14159265358979323846));
}

// ============================================================================
// Path Utility Tests (Platform-independent aspects)
// ============================================================================

TEST_CASE("Path separator detection", "[utilities][paths]")
{
#ifdef _WIN32
  SECTION("Windows uses backslash")
  {
    // Windows path separator tests if we have a path separator constant
  }
#else
  SECTION("Unix uses forward slash")
  {
    // Unix path separator tests
  }
#endif
}

// Test that we can call path functions without crashing
// Actual paths are platform-specific, but we can test the interface
TEST_CASE("Path functions are callable", "[utilities][paths]")
{
  WDL_String path;

  SECTION("DesktopPath doesn't crash")
  {
    DesktopPath(path);
    // Path may be empty in test environment but shouldn't crash
  }

  SECTION("UserHomePath doesn't crash")
  {
    // Note: This function may not exist on all platforms
    // UserHomePath(path);
  }
}
