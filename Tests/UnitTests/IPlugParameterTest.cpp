/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>

#include "IPlugParameter.h"

using namespace iplug;
using Catch::Approx;

TEST_CASE("IParam initialization", "[IParam]")
{
  SECTION("Default constructor creates uninitialized param")
  {
    IParam param;
    REQUIRE(param.Type() == IParam::kTypeNone);
  }

  SECTION("InitBool creates boolean parameter")
  {
    IParam param;
    param.InitBool("Bypass", false, "", 0, "", "Off", "On");

    REQUIRE(param.Type() == IParam::kTypeBool);
    REQUIRE(std::string(param.GetName()) == "Bypass");
    REQUIRE(param.GetMin() == 0.0);
    REQUIRE(param.GetMax() == 1.0);
    REQUIRE(param.Bool() == false);
    REQUIRE(param.GetDefault() == 0.0);

    // Check display texts
    REQUIRE(std::string(param.GetDisplayText(0)) == "Off");
    REQUIRE(std::string(param.GetDisplayText(1)) == "On");
  }

  SECTION("InitInt creates integer parameter")
  {
    IParam param;
    param.InitInt("Octave", 0, -2, 2, "oct");

    REQUIRE(param.Type() == IParam::kTypeInt);
    REQUIRE(param.GetMin() == -2.0);
    REQUIRE(param.GetMax() == 2.0);
    REQUIRE(param.GetStep() == 1.0);
    REQUIRE(param.Int() == 0);
    REQUIRE(param.GetStepped() == true);
  }

  SECTION("InitEnum creates enum parameter")
  {
    IParam param;
    param.InitEnum("Waveform", 0, {"Sine", "Triangle", "Saw", "Square"});

    REQUIRE(param.Type() == IParam::kTypeEnum);
    REQUIRE(param.GetMin() == 0.0);
    REQUIRE(param.GetMax() == 3.0);
    REQUIRE(param.NDisplayTexts() == 4);
    REQUIRE(std::string(param.GetDisplayText(0)) == "Sine");
    REQUIRE(std::string(param.GetDisplayText(2)) == "Saw");
  }

  SECTION("InitDouble creates double parameter")
  {
    IParam param;
    param.InitDouble("Gain", 0.0, -70.0, 12.0, 0.1, "dB");

    REQUIRE(param.Type() == IParam::kTypeDouble);
    REQUIRE(param.GetMin() == -70.0);
    REQUIRE(param.GetMax() == 12.0);
    REQUIRE(param.GetStep() == Approx(0.1));
    REQUIRE(param.Value() == 0.0);
    REQUIRE(std::string(param.GetLabel()) == "dB");
  }

  SECTION("InitFrequency uses exponential shape")
  {
    IParam param;
    param.InitFrequency("Cutoff", 1000.0, 20.0, 20000.0, 0.1);

    REQUIRE(param.Type() == IParam::kTypeDouble);
    REQUIRE(param.Unit() == IParam::kUnitFrequency);
    REQUIRE(param.DisplayType() == IParam::kDisplayLog);
  }

  SECTION("InitGain creates dB parameter")
  {
    IParam param;
    param.InitGain("Volume", 0.0, -70.0, 12.0, 0.5);

    REQUIRE(param.Unit() == IParam::kUnitDB);
    REQUIRE(std::string(param.GetLabel()) == "dB");
  }

  SECTION("InitPercentage creates percentage parameter")
  {
    IParam param;
    param.InitPercentage("Mix", 50.0, 0.0, 100.0);

    REQUIRE(param.Unit() == IParam::kUnitPercentage);
    REQUIRE(std::string(param.GetLabel()) == "%");
  }
}

TEST_CASE("IParam value operations", "[IParam]")
{
  SECTION("Set and Get value")
  {
    IParam param;
    param.InitDouble("Test", 0.5, 0.0, 1.0, 0.01);

    param.Set(0.75);
    REQUIRE(param.Value() == Approx(0.75));

    param.Set(0.33);
    REQUIRE(param.Value() == Approx(0.33));
  }

  SECTION("Value is constrained to bounds")
  {
    IParam param;
    param.InitDouble("Test", 0.5, 0.0, 1.0, 0.01);

    param.Set(1.5);
    REQUIRE(param.Value() == Approx(1.0));

    param.Set(-0.5);
    REQUIRE(param.Value() == Approx(0.0));
  }

  SECTION("SetToDefault restores default value")
  {
    IParam param;
    param.InitDouble("Test", 0.5, 0.0, 1.0, 0.01);

    param.Set(0.9);
    REQUIRE(param.Value() == Approx(0.9));

    param.SetToDefault();
    REQUIRE(param.Value() == Approx(0.5));
  }

  SECTION("Bool conversion")
  {
    IParam param;
    param.InitBool("Toggle", false);

    REQUIRE(param.Bool() == false);

    param.Set(1.0);
    REQUIRE(param.Bool() == true);

    param.Set(0.4);
    REQUIRE(param.Bool() == false);

    param.Set(0.5);
    REQUIRE(param.Bool() == true);
  }

  SECTION("Int conversion")
  {
    IParam param;
    param.InitInt("Steps", 5, 0, 10);

    REQUIRE(param.Int() == 5);

    param.Set(7.8);
    REQUIRE(param.Int() == 8); // Stepped, rounds to nearest
  }

  SECTION("DBToAmp conversion")
  {
    IParam param;
    param.InitGain("Gain", 0.0, -70.0, 12.0, 0.1);

    param.Set(0.0);
    REQUIRE(param.DBToAmp() == Approx(1.0).margin(0.001));

    param.Set(-6.0);
    REQUIRE(param.DBToAmp() == Approx(0.5).margin(0.01));

    param.Set(6.0);
    REQUIRE(param.DBToAmp() == Approx(2.0).margin(0.01));
  }
}

TEST_CASE("IParam normalization with linear shape", "[IParam][Shape]")
{
  SECTION("ToNormalized and FromNormalized are inverse operations")
  {
    IParam param;
    param.InitDouble("Test", 50.0, 0.0, 100.0, 0.1);

    // Test various values
    for (double v : {0.0, 25.0, 50.0, 75.0, 100.0})
    {
      double normalized = param.ToNormalized(v);
      double restored = param.FromNormalized(normalized);
      REQUIRE(restored == Approx(v).margin(0.001));
    }
  }

  SECTION("Linear normalization maps correctly")
  {
    IParam param;
    param.InitDouble("Test", 0.0, -100.0, 100.0, 1.0);

    REQUIRE(param.ToNormalized(-100.0) == Approx(0.0));
    REQUIRE(param.ToNormalized(0.0) == Approx(0.5));
    REQUIRE(param.ToNormalized(100.0) == Approx(1.0));

    REQUIRE(param.FromNormalized(0.0) == Approx(-100.0));
    REQUIRE(param.FromNormalized(0.5) == Approx(0.0));
    REQUIRE(param.FromNormalized(1.0) == Approx(100.0));
  }

  SECTION("SetNormalized sets correct value")
  {
    IParam param;
    param.InitDouble("Test", 0.0, 0.0, 100.0, 0.1);

    param.SetNormalized(0.5);
    REQUIRE(param.Value() == Approx(50.0));
    REQUIRE(param.GetNormalized() == Approx(0.5));
  }
}

TEST_CASE("IParam normalization with PowCurve shape", "[IParam][Shape]")
{
  SECTION("PowCurve shape=2 (square root mapping)")
  {
    IParam param;
    param.InitDouble("Test", 0.0, 0.0, 100.0, 0.1, "", 0, "",
                     IParam::ShapePowCurve(2.0));

    // With shape=2, normalized 0.5 maps to 25 (0.5^2 * 100)
    REQUIRE(param.FromNormalized(0.5) == Approx(25.0).margin(0.1));

    // And 25 normalizes to 0.5
    REQUIRE(param.ToNormalized(25.0) == Approx(0.5).margin(0.01));

    // Endpoints still correct
    REQUIRE(param.FromNormalized(0.0) == Approx(0.0));
    REQUIRE(param.FromNormalized(1.0) == Approx(100.0));
  }

  SECTION("PowCurve shape=0.5 (square mapping)")
  {
    IParam param;
    param.InitDouble("Test", 0.0, 0.0, 100.0, 0.1, "", 0, "",
                     IParam::ShapePowCurve(0.5));

    // With shape=0.5, normalized 0.5 maps to sqrt(0.5)*100 ≈ 70.7
    REQUIRE(param.FromNormalized(0.5) == Approx(70.71).margin(0.1));
  }

  SECTION("Roundtrip with PowCurve")
  {
    IParam param;
    param.InitDouble("Test", 50.0, 0.0, 100.0, 0.1, "", 0, "",
                     IParam::ShapePowCurve(2.5));

    for (double v : {0.0, 10.0, 25.0, 50.0, 75.0, 100.0})
    {
      double normalized = param.ToNormalized(v);
      double restored = param.FromNormalized(normalized);
      REQUIRE(restored == Approx(v).margin(0.1));
    }
  }
}

TEST_CASE("IParam normalization with Exponential shape", "[IParam][Shape]")
{
  SECTION("Exponential shape for frequency")
  {
    IParam param;
    param.InitFrequency("Freq", 1000.0, 20.0, 20000.0, 0.1);

    // Endpoints
    REQUIRE(param.FromNormalized(0.0) == Approx(20.0).margin(0.1));
    REQUIRE(param.FromNormalized(1.0) == Approx(20000.0).margin(1.0));

    // Middle should be geometric mean: sqrt(20 * 20000) ≈ 632
    REQUIRE(param.FromNormalized(0.5) == Approx(632.0).margin(5.0));
  }

  SECTION("Roundtrip with Exponential shape")
  {
    IParam param;
    param.InitFrequency("Freq", 1000.0, 20.0, 20000.0, 0.1);

    for (double v : {20.0, 100.0, 500.0, 1000.0, 5000.0, 20000.0})
    {
      double normalized = param.ToNormalized(v);
      double restored = param.FromNormalized(normalized);
      REQUIRE(restored == Approx(v).margin(v * 0.01)); // 1% tolerance
    }
  }
}

TEST_CASE("IParam display text", "[IParam]")
{
  SECTION("Display text for specific values")
  {
    IParam param;
    param.InitDouble("Gain", -70.0, -70.0, 12.0, 0.1, "dB");
    param.SetDisplayText(-70.0, "-inf");

    REQUIRE(std::string(param.GetDisplayText(-70.0)) == "-inf");
    REQUIRE(std::string(param.GetDisplayText(0.0)) == ""); // No display text
  }

  SECTION("GetDisplay returns formatted value")
  {
    IParam param;
    param.InitDouble("Value", 0.0, 0.0, 100.0, 0.01);

    param.Set(42.55);
    WDL_String display;
    param.GetDisplay(display);

    REQUIRE(std::string(display.Get()) == "42.55");
  }

  SECTION("GetDisplayWithLabel includes unit")
  {
    IParam param;
    param.InitDouble("Freq", 1000.0, 20.0, 20000.0, 1.0, "Hz");

    param.Set(440.0);
    WDL_String display;
    param.GetDisplayWithLabel(display);

    REQUIRE(std::string(display.Get()) == "440 Hz");
  }

  SECTION("MapDisplayText returns value for text")
  {
    IParam param;
    param.InitEnum("Mode", 0, {"Off", "Low", "High"});

    double value;
    REQUIRE(param.MapDisplayText("Low", &value) == true);
    REQUIRE(value == 1.0);

    REQUIRE(param.MapDisplayText("Unknown", &value) == false);
  }
}

TEST_CASE("IParam StringToValue", "[IParam]")
{
  SECTION("Parse numeric string")
  {
    IParam param;
    param.InitDouble("Value", 0.0, -100.0, 100.0, 0.1);

    REQUIRE(param.StringToValue("42.5") == Approx(42.5));
    REQUIRE(param.StringToValue("-10") == Approx(-10.0));
  }

  SECTION("Parse with display text mapping")
  {
    IParam param;
    param.InitEnum("Wave", 0, {"Sine", "Saw", "Square"});

    REQUIRE(param.StringToValue("Saw") == 1.0);
    REQUIRE(param.StringToValue("Square") == 2.0);
  }

  SECTION("Value is constrained")
  {
    IParam param;
    param.InitDouble("Value", 0.0, 0.0, 100.0, 0.1);

    REQUIRE(param.StringToValue("150") == Approx(100.0));
    REQUIRE(param.StringToValue("-50") == Approx(0.0));
  }
}

TEST_CASE("IParam flags", "[IParam]")
{
  SECTION("kFlagCannotAutomate")
  {
    IParam param;
    param.InitDouble("Internal", 0.0, 0.0, 1.0, 0.01, "", IParam::kFlagCannotAutomate);

    REQUIRE(param.GetCanAutomate() == false);
  }

  SECTION("kFlagStepped")
  {
    IParam param;
    param.InitDouble("Steps", 0.0, 0.0, 10.0, 1.0, "", IParam::kFlagStepped);

    REQUIRE(param.GetStepped() == true);

    // Values should be stepped
    param.Set(3.7);
    REQUIRE(param.Value() == Approx(4.0));
  }

  SECTION("kFlagNegateDisplay")
  {
    IParam param;
    param.InitDouble("Pan", 0.0, -100.0, 100.0, 1.0, "", IParam::kFlagNegateDisplay);

    REQUIRE(param.GetNegateDisplay() == true);

    param.Set(50.0);
    WDL_String display;
    param.GetDisplay(display);
    REQUIRE(std::string(display.Get()) == "-50"); // Negated for display
  }

  SECTION("kFlagSignDisplay")
  {
    IParam param;
    param.InitDouble("Detune", 0.0, -100.0, 100.0, 0.1, "cents", IParam::kFlagSignDisplay);

    REQUIRE(param.GetSignDisplay() == true);

    param.Set(25.5);
    WDL_String display;
    param.GetDisplay(display);
    REQUIRE(std::string(display.Get()).find('+') != std::string::npos);
  }

  SECTION("kFlagMeta")
  {
    IParam param;
    param.InitDouble("Master", 0.0, 0.0, 1.0, 0.01, "", IParam::kFlagMeta);

    REQUIRE(param.GetMeta() == true);
  }
}

TEST_CASE("IParam groups", "[IParam]")
{
  SECTION("Parameter group is stored")
  {
    IParam param;
    param.InitDouble("Attack", 10.0, 0.1, 1000.0, 0.1, "ms", 0, "Envelope");

    REQUIRE(std::string(param.GetGroup()) == "Envelope");
  }
}

TEST_CASE("IParam copy initialization", "[IParam]")
{
  SECTION("Init from another parameter")
  {
    IParam source;
    source.InitDouble("Osc1 Freq", 440.0, 20.0, 20000.0, 0.1, "Hz", 0, "Oscillator",
                      IParam::ShapeExp());

    IParam dest;
    dest.Init(source, "Osc1", "Osc2", "Oscillator 2");

    REQUIRE(std::string(dest.GetName()) == "Osc2 Freq");
    REQUIRE(std::string(dest.GetGroup()) == "Oscillator 2");
    REQUIRE(dest.GetMin() == source.GetMin());
    REQUIRE(dest.GetMax() == source.GetMax());
    REQUIRE(dest.GetDefault() == source.GetDefault());
  }
}

TEST_CASE("IParam JSON serialization", "[IParam]")
{
  SECTION("GetJSON produces valid output")
  {
    IParam param;
    param.InitDouble("Gain", 0.0, -70.0, 12.0, 0.1, "dB");

    WDL_String json;
    param.GetJSON(json, 0);

    std::string jsonStr(json.Get());
    REQUIRE(jsonStr.find("\"id\":0") != std::string::npos);
    REQUIRE(jsonStr.find("\"name\":\"Gain\"") != std::string::npos);
    REQUIRE(jsonStr.find("\"type\":\"float\"") != std::string::npos);
    REQUIRE(jsonStr.find("\"min\":-70") != std::string::npos);
    REQUIRE(jsonStr.find("\"max\":12") != std::string::npos);
  }

  SECTION("JSON type for different param types")
  {
    IParam boolParam;
    boolParam.InitBool("Toggle", false);
    WDL_String boolJson;
    boolParam.GetJSON(boolJson, 0);
    REQUIRE(std::string(boolJson.Get()).find("\"type\":\"bool\"") != std::string::npos);

    IParam enumParam;
    enumParam.InitEnum("Mode", 0, {"A", "B"});
    WDL_String enumJson;
    enumParam.GetJSON(enumJson, 1);
    REQUIRE(std::string(enumJson.Get()).find("\"type\":\"enum\"") != std::string::npos);

    IParam intParam;
    intParam.InitInt("Count", 5, 0, 10);
    WDL_String intJson;
    intParam.GetJSON(intJson, 2);
    REQUIRE(std::string(intJson.Get()).find("\"type\":\"int\"") != std::string::npos);
  }
}

TEST_CASE("IParam shape identification", "[IParam][Shape]")
{
  SECTION("GetShapeID identifies linear shape")
  {
    IParam param;
    param.InitDouble("Linear", 0.0, 0.0, 1.0, 0.01);

    REQUIRE(param.GetShapeID() == IParam::kShapeLinear);
  }

  SECTION("GetShapeID identifies PowCurve shape")
  {
    IParam param;
    param.InitDouble("Curved", 0.0, 0.0, 1.0, 0.01, "", 0, "",
                     IParam::ShapePowCurve(2.0));

    REQUIRE(param.GetShapeID() == IParam::kShapePowCurve);
    REQUIRE(param.GetShapeValue() == Approx(2.0));
  }

  SECTION("GetShapeID identifies Exponential shape")
  {
    IParam param;
    param.InitFrequency("Freq", 1000.0, 20.0, 20000.0, 0.1);

    REQUIRE(param.GetShapeID() == IParam::kShapeExponential);
  }
}

TEST_CASE("IParam display precision", "[IParam]")
{
  SECTION("Precision is auto-calculated from step")
  {
    IParam param1;
    param1.InitDouble("P1", 0.0, 0.0, 1.0, 1.0);
    REQUIRE(param1.GetDisplayPrecision() == 0);

    IParam param2;
    param2.InitDouble("P2", 0.0, 0.0, 1.0, 0.1);
    REQUIRE(param2.GetDisplayPrecision() == 1);

    IParam param3;
    param3.InitDouble("P3", 0.0, 0.0, 1.0, 0.01);
    REQUIRE(param3.GetDisplayPrecision() == 2);

    IParam param4;
    param4.InitDouble("P4", 0.0, 0.0, 1.0, 0.001);
    REQUIRE(param4.GetDisplayPrecision() == 3);
  }

  SECTION("SetDisplayPrecision overrides auto-calculation")
  {
    IParam param;
    param.InitDouble("Test", 0.0, 0.0, 1.0, 0.1);
    REQUIRE(param.GetDisplayPrecision() == 1);

    param.SetDisplayPrecision(3);
    REQUIRE(param.GetDisplayPrecision() == 3);
  }
}

TEST_CASE("IParam custom display function", "[IParam]")
{
  SECTION("DisplayFunc overrides default display")
  {
    IParam param;
    param.InitDouble("Time", 1.0, 0.001, 10.0, 0.001, "s");

    // Custom function to show ms for small values, s for larger
    param.SetDisplayFunc([](double value, WDL_String& str) {
      if (value < 1.0)
        str.SetFormatted(32, "%.1f ms", value * 1000.0);
      else
        str.SetFormatted(32, "%.2f s", value);
    });

    param.Set(0.5);
    WDL_String display;
    param.GetDisplay(display);
    REQUIRE(std::string(display.Get()) == "500.0 ms");

    param.Set(2.5);
    param.GetDisplay(display);
    REQUIRE(std::string(display.Get()) == "2.50 s");
  }
}
