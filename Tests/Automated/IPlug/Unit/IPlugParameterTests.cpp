/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

// Unit tests for IParam (IPlugParameter.h/.cpp): initialisation, shapes, constraints, display and copying.

#include <cmath>
#include <string>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/generators/catch_generators.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include "IPlugParameter.h"

using namespace iplug;
using Catch::Approx;
using Catch::Matchers::WithinAbs;
using Catch::Matchers::ContainsSubstring;
using Catch::Matchers::StartsWith;
using Catch::Matchers::EndsWith;

namespace
{
std::string Display(const IParam& p, bool withDisplayText = true)
{
  WDL_String str;
  p.GetDisplay(str, withDisplayText);
  return str.Get();
}

std::string DisplayFor(const IParam& p, double value, bool normalized = false, bool withDisplayText = true)
{
  WDL_String str;
  p.GetDisplay(value, normalized, str, withDisplayText);
  return str.Get();
}

std::string DisplayWithLabel(const IParam& p, bool withDisplayText = true)
{
  WDL_String str;
  p.GetDisplayWithLabel(str, withDisplayText);
  return str.Get();
}

std::string Str(const char* str)
{
  return str ? std::string(str) : std::string("<null>");
}

std::string JSON(const IParam& p, int idx)
{
  WDL_String str;
  p.GetJSON(str, idx);
  return str.Get();
}
} // namespace

TEST_CASE("IParam: default constructed state", "[unit][param]")
{
  IParam p;

  REQUIRE(p.Type() == IParam::kTypeNone);
  REQUIRE(p.Unit() == IParam::kUnitCustom);
  REQUIRE(p.Value() == 0.0);
  REQUIRE(p.GetMin() == 0.0);
  REQUIRE(p.GetMax() == 1.0);
  REQUIRE(p.GetRange() == 1.0);
  REQUIRE(p.GetStep() == 1.0);
  REQUIRE(p.GetDefault() == 0.0);
  REQUIRE(p.GetDisplayPrecision() == 0);
  REQUIRE(p.GetFlags() == IParam::kFlagsNone);
  REQUIRE(Str(p.GetName()).empty());
  REQUIRE(Str(p.GetLabel()).empty());
  REQUIRE(Str(p.GetGroup()).empty());
  REQUIRE(p.NDisplayTexts() == 0);
  REQUIRE(p.GetShapeID() == IParam::kShapeLinear);
  REQUIRE(p.GetShapeValue() == 0.0);
  REQUIRE(p.DisplayType() == IParam::kDisplayLinear);
  REQUIRE(p.GetCanAutomate());
  REQUIRE_FALSE(p.GetStepped());
  REQUIRE_FALSE(p.GetNegateDisplay());
  REQUIRE_FALSE(p.GetSignDisplay());
  REQUIRE_FALSE(p.GetMeta());
  REQUIRE_FALSE(p.Bool());
  REQUIRE(p.Int() == 0);
}

TEST_CASE("IParam: InitDouble stores its arguments", "[unit][param]")
{
  IParam p;
  p.InitDouble("Cutoff", 2.5, -10., 30., 0.5, "units", IParam::kFlagMeta, "Filter");

  REQUIRE(p.Type() == IParam::kTypeDouble);
  REQUIRE(Str(p.GetName()) == "Cutoff");
  REQUIRE(Str(p.GetLabel()) == "units");
  REQUIRE(Str(p.GetGroup()) == "Filter");
  REQUIRE(p.GetMin() == -10.);
  REQUIRE(p.GetMax() == 30.);
  REQUIRE(p.GetRange() == 40.);
  REQUIRE(p.GetStep() == 0.5);
  REQUIRE(p.GetDefault() == 2.5);
  REQUIRE(p.Value() == 2.5);
  REQUIRE(p.GetFlags() == IParam::kFlagMeta);
  REQUIRE(p.Unit() == IParam::kUnitCustom);
  REQUIRE(Str(p.GetCustomUnit()) == "units");
  REQUIRE(p.GetShapeID() == IParam::kShapeLinear);
  REQUIRE(p.DisplayType() == IParam::kDisplayLinear);

  double lo = 0., hi = 0.;
  p.GetBounds(lo, hi);
  REQUIRE(lo == -10.);
  REQUIRE(hi == 30.);

  SECTION("optional arguments have neutral defaults")
  {
    IParam q;
    q.InitDouble("Plain", 0.25, 0., 1., 0.01);
    REQUIRE(Str(q.GetLabel()).empty());
    REQUIRE(Str(q.GetGroup()).empty());
    REQUIRE(q.GetFlags() == IParam::kFlagsNone);
    REQUIRE(q.Unit() == IParam::kUnitCustom);
    REQUIRE(q.GetCanAutomate());
    REQUIRE_FALSE(q.GetStepped());
  }

  SECTION("explicit unit is stored and hides the custom unit string")
  {
    IParam q;
    q.InitDouble("Pan", 0., -100., 100., 1., "%", 0, "", IParam::ShapeLinear(), IParam::kUnitPan);
    REQUIRE(q.Unit() == IParam::kUnitPan);
    REQUIRE(q.GetCustomUnit() == nullptr);
    REQUIRE(Str(q.GetLabel()) == "%");
  }
}

TEST_CASE("IParam: InitDouble derives the display precision from the step", "[unit][param]")
{
  struct Case { double step; int precision; };
  auto c = GENERATE(Case{1., 0}, Case{10., 0}, Case{0.5, 1}, Case{0.1, 1}, Case{0.25, 2},
                    Case{0.01, 2}, Case{0.125, 3}, Case{0.0625, 4}, Case{0.05, 2}, Case{0.2, 1},
                    Case{0.001, 3}, Case{0.0001, 4}, Case{0.00001, 5}, Case{0.000001, 6});
  CAPTURE(c.step);

  IParam p;
  p.InitDouble("P", 0., 0., 100., c.step);
  REQUIRE(p.GetDisplayPrecision() == c.precision);
  REQUIRE(p.GetStep() == c.step);
}

TEST_CASE("IParam: display precision never exceeds MAX_PARAM_DISPLAY_PRECISION", "[unit][param]")
{
  auto step = GENERATE(0.0000001, 0.123456789, 1. / 3.);
  CAPTURE(step);

  IParam p;
  p.InitDouble("P", 0., 0., 1., step);
  REQUIRE(p.GetDisplayPrecision() == MAX_PARAM_DISPLAY_PRECISION);
}

TEST_CASE("IParam: InitDouble clamps values to the range", "[unit][param]")
{
  IParam p;
  p.InitDouble("P", 5., -10., 10., 0.1);

  SECTION("values inside the range are kept exactly (not stepped)")
  {
    auto v = GENERATE(-10., -9.999, -0.123456, 0., 3.14159, 9.87654321, 10.);
    p.Set(v);
    REQUIRE(p.Value() == v);
    REQUIRE(p.Constrain(v) == v);
  }

  SECTION("values outside the range are clamped")
  {
    p.Set(10.0001);
    REQUIRE(p.Value() == 10.);
    p.Set(1e12);
    REQUIRE(p.Value() == 10.);
    p.Set(-10.0001);
    REQUIRE(p.Value() == -10.);
    p.Set(-1e12);
    REQUIRE(p.Value() == -10.);
    REQUIRE(p.Constrain(11.) == 10.);
    REQUIRE(p.Constrain(-11.) == -10.);
  }

  SECTION("an out of range default initialises the value at the nearest bound")
  {
    IParam hi;
    hi.InitDouble("Hi", 50., 0., 10., 0.1);
    REQUIRE(hi.Value() == 10.);

    IParam lo;
    lo.InitDouble("Lo", -50., 0., 10., 0.1);
    REQUIRE(lo.Value() == 0.);
  }
}

// BUG: IPlugParameter.cpp:151-160. The comment in InitDouble() says "apply stepping and constraints to the
// default value (and store the result)" but mDefault is stored unconstrained, so GetDefault() can report a
// value outside [min, max] and SetToDefault() puts the parameter out of range.
TEST_CASE("IParam: an out of range default is constrained", "[.][bug]")
{
  IParam p;
  p.InitDouble("P", 50., 0., 10., 0.1);
  REQUIRE(p.GetDefault() == 10.);
  p.Set(5.);
  p.SetToDefault();
  REQUIRE(p.Value() == 10.);

  IParam i;
  i.InitInt("I", 3, 0, 2);
  REQUIRE(i.GetDefault() == 2.);
  i.SetToDefault();
  REQUIRE(i.Value() == 2.);
}

TEST_CASE("IParam: a zero or negative range is widened by one step", "[unit][param]")
{
  IParam p;
  p.InitDouble("P", 5., 5., 5., 0.5);
  REQUIRE(p.GetMin() == 5.);
  REQUIRE(p.GetMax() == 5.5);
  REQUIRE(p.GetRange() > 0.);

  IParam q;
  q.InitDouble("Q", 0., 10., -10., 1.);
  REQUIRE(q.GetMin() == 10.);
  REQUIRE(q.GetMax() == 11.);
  REQUIRE(q.Value() == 10.);
}

TEST_CASE("IParam: kFlagStepped quantises to the step size", "[unit][param]")
{
  IParam p;
  p.InitDouble("P", 0., -1., 1., 0.25, "", IParam::kFlagStepped);
  REQUIRE(p.GetStepped());

  struct Case { double in; double out; };
  auto c = GENERATE(Case{0., 0.}, Case{0.1, 0.}, Case{0.13, 0.25}, Case{0.3, 0.25}, Case{0.38, 0.5},
                    Case{-0.3, -0.25}, Case{-0.38, -0.5}, Case{0.99, 1.}, Case{1.2, 1.}, Case{-7., -1.},
                    Case{0.75, 0.75});
  CAPTURE(c.in);

  REQUIRE(p.Constrain(c.in) == Approx(c.out));
  p.Set(c.in);
  REQUIRE(p.Value() == Approx(c.out));
}

TEST_CASE("IParam: linear normalisation", "[unit][param]")
{
  IParam p;
  p.InitDouble("P", 0., -20., 60., 0.01);

  SECTION("known points")
  {
    REQUIRE(p.ToNormalized(-20.) == 0.);
    REQUIRE(p.ToNormalized(60.) == 1.);
    REQUIRE(p.ToNormalized(20.) == Approx(0.5));
    REQUIRE(p.ToNormalized(0.) == Approx(0.25));
    REQUIRE(p.FromNormalized(0.) == -20.);
    REQUIRE(p.FromNormalized(1.) == 60.);
    REQUIRE(p.FromNormalized(0.5) == Approx(20.));
    REQUIRE(p.FromNormalized(0.75) == Approx(40.));
  }

  SECTION("out of range input is clamped")
  {
    REQUIRE(p.ToNormalized(-21.) == 0.);
    REQUIRE(p.ToNormalized(1e9) == 1.);
    REQUIRE(p.FromNormalized(-0.5) == -20.);
    REQUIRE(p.FromNormalized(1.5) == 60.);
    REQUIRE(p.ConstrainNormalized(-3.) == 0.);
    REQUIRE(p.ConstrainNormalized(3.) == 1.);
  }

  SECTION("round trips")
  {
    auto n = GENERATE(0., 0.001, 0.1, 0.25, 1. / 3., 0.5, 0.75, 0.999, 1.);
    CAPTURE(n);
    REQUIRE(p.ToNormalized(p.FromNormalized(n)) == Approx(n).margin(1e-12));
    REQUIRE(p.ConstrainNormalized(n) == Approx(n).margin(1e-12));

    const double v = p.FromNormalized(n);
    REQUIRE(p.FromNormalized(p.ToNormalized(v)) == Approx(v).margin(1e-9));
  }

  SECTION("SetNormalized / GetNormalized")
  {
    p.SetNormalized(0.25);
    REQUIRE(p.Value() == Approx(0.));
    REQUIRE(p.GetNormalized() == Approx(0.25));
    p.SetNormalized(2.);
    REQUIRE(p.Value() == 60.);
    REQUIRE(p.GetNormalized() == 1.);
    p.SetNormalized(-2.);
    REQUIRE(p.Value() == -20.);
    REQUIRE(p.GetNormalized() == 0.);
    p.Set(40.);
    REQUIRE(p.GetNormalized() == Approx(0.75));
  }

  SECTION("GetDefault can be normalised")
  {
    REQUIRE(p.GetDefault() == 0.);
    REQUIRE(p.GetDefault(false) == 0.);
    REQUIRE(p.GetDefault(true) == Approx(0.25));
  }
}

TEST_CASE("IParam: ShapePowCurve", "[unit][param]")
{
  SECTION("shape id, value and known points")
  {
    IParam p;
    p.InitDouble("P", 0., 0., 100., 0.01, "", 0, "", IParam::ShapePowCurve(2.));
    REQUIRE(p.GetShapeID() == IParam::kShapePowCurve);
    REQUIRE(p.GetShapeValue() == 2.);
    REQUIRE(p.FromNormalized(0.) == 0.);
    REQUIRE(p.FromNormalized(1.) == 100.);
    REQUIRE(p.FromNormalized(0.5) == Approx(25.));
    REQUIRE(p.FromNormalized(0.1) == Approx(1.));
    REQUIRE(p.ToNormalized(25.) == Approx(0.5));
    REQUIRE(p.ToNormalized(1.) == Approx(0.1));
    REQUIRE(p.ToNormalized(0.) == 0.);
    REQUIRE(p.ToNormalized(100.) == 1.);
    REQUIRE(p.GetDefault(true) == 0.);
  }

  SECTION("a shape below one bends the other way and respects an offset range")
  {
    IParam p;
    p.InitDouble("P", -50., -50., 50., 0.01, "", 0, "", IParam::ShapePowCurve(0.5));
    REQUIRE(p.GetShapeValue() == 0.5);
    REQUIRE(p.FromNormalized(0.25) == Approx(0.));
    REQUIRE(p.ToNormalized(0.) == Approx(0.25));
    REQUIRE(p.FromNormalized(0.) == -50.);
    REQUIRE(p.FromNormalized(1.) == 50.);
  }

  SECTION("a shape of one is linear")
  {
    IParam p;
    p.InitDouble("P", 0., 0., 10., 0.01, "", 0, "", IParam::ShapePowCurve(1.));
    auto n = GENERATE(0., 0.2, 0.5, 0.9, 1.);
    REQUIRE(p.FromNormalized(n) == Approx(n * 10.));
  }

  SECTION("round trips and clamping")
  {
    auto shape = GENERATE(0.25, 0.5, 1., 2., 3., 5.);
    auto n = GENERATE(0., 0.01, 0.3, 0.5, 0.8, 1.);
    CAPTURE(shape, n);

    IParam p;
    p.InitDouble("P", 1., -3., 7., 0.001, "", 0, "", IParam::ShapePowCurve(shape));
    const double v = p.FromNormalized(n);
    REQUIRE(v >= -3.);
    REQUIRE(v <= 7.);
    REQUIRE(p.ToNormalized(v) == Approx(n).margin(1e-9));
    REQUIRE(p.ToNormalized(-100.) == 0.);
    REQUIRE(p.ToNormalized(100.) == 1.);
    REQUIRE(p.FromNormalized(2.) == 7.);
  }

  SECTION("is monotonic")
  {
    auto shape = GENERATE(0.3, 2., 4.);
    IParam p;
    p.InitDouble("P", 0., 0., 1., 0.001, "", 0, "", IParam::ShapePowCurve(shape));
    double prev = p.FromNormalized(0.);
    for (int i = 1; i <= 20; i++)
    {
      const double v = p.FromNormalized(i / 20.);
      REQUIRE(v > prev);
      prev = v;
    }
  }

  SECTION("display type follows the curve")
  {
    struct Case { double shape; IParam::EDisplayType type; };
    auto c = GENERATE(Case{3., IParam::kDisplayCubeRoot}, Case{2., IParam::kDisplaySquareRoot},
                      Case{1., IParam::kDisplayLinear}, Case{0.5, IParam::kDisplaySquared},
                      Case{1. / 3., IParam::kDisplayCubed});
    CAPTURE(c.shape);

    IParam p;
    p.InitDouble("P", 0., 0., 1., 0.01, "", 0, "", IParam::ShapePowCurve(c.shape));
    REQUIRE(p.DisplayType() == c.type);
    REQUIRE(IParam::ShapePowCurve(c.shape).GetDisplayType() == c.type);
  }
}

TEST_CASE("IParam: ShapeExp", "[unit][param]")
{
  SECTION("maps the normalised range geometrically")
  {
    IParam p;
    p.InitDouble("Freq", 1000., 20., 20000., 0.01, "Hz", 0, "", IParam::ShapeExp());
    REQUIRE(p.GetShapeID() == IParam::kShapeExponential);
    REQUIRE(p.GetShapeValue() == 0.);
    REQUIRE(p.DisplayType() == IParam::kDisplayLog);
    REQUIRE(p.FromNormalized(0.) == Approx(20.));
    REQUIRE(p.FromNormalized(1.) == Approx(20000.));
    REQUIRE(p.FromNormalized(0.5) == Approx(std::sqrt(20. * 20000.)));
    REQUIRE(p.FromNormalized(1. / 3.) == Approx(200.));
    REQUIRE(p.FromNormalized(2. / 3.) == Approx(2000.));
    REQUIRE(p.ToNormalized(20.) == Approx(0.).margin(1e-12));
    REQUIRE(p.ToNormalized(20000.) == Approx(1.));
    REQUIRE(p.ToNormalized(200.) == Approx(1. / 3.));
    REQUIRE(p.ToNormalized(2000.) == Approx(2. / 3.));
    REQUIRE(p.GetDefault(true) == Approx(std::log(50.) / std::log(1000.)));
  }

  SECTION("round trips and clamping")
  {
    IParam p;
    p.InitDouble("Freq", 1000., 20., 20000., 0.01, "Hz", 0, "", IParam::ShapeExp());
    auto n = GENERATE(0., 0.05, 0.25, 0.5, 0.77, 1.);
    CAPTURE(n);
    const double v = p.FromNormalized(n);
    REQUIRE(v >= 20.);
    REQUIRE(v <= 20000.);
    REQUIRE(p.ToNormalized(v) == Approx(n).margin(1e-9));
    REQUIRE(p.ToNormalized(1.) == 0.);
    REQUIRE(p.ToNormalized(1e9) == 1.);
    REQUIRE(p.FromNormalized(-1.) == 20.);
    REQUIRE(p.FromNormalized(2.) == 20000.);
  }

  SECTION("a minimum of zero still gives a usable, monotonic mapping")
  {
    IParam p;
    p.InitDouble("P", 1., 0., 100., 0.01, "", 0, "", IParam::ShapeExp());
    REQUIRE(p.FromNormalized(0.) >= 0.);
    REQUIRE(p.FromNormalized(0.) < 1e-6);
    REQUIRE(p.FromNormalized(1.) == Approx(100.));
    REQUIRE(p.ToNormalized(100.) == Approx(1.));
    REQUIRE(p.ToNormalized(0.) == 0.);

    double prev = p.FromNormalized(0.);
    for (int i = 1; i <= 10; i++)
    {
      const double v = p.FromNormalized(i / 10.);
      REQUIRE(v > prev);
      REQUIRE(p.ToNormalized(v) == Approx(i / 10.));
      prev = v;
    }
  }

  SECTION("shapes are cloned into the parameter")
  {
    IParam::ShapeExp shape;
    IParam p;
    p.InitDouble("P", 10., 10., 1000., 0.1, "", 0, "", shape);
    // the caller's shape is left uninitialised, the parameter's own copy is initialised from its range
    REQUIRE(shape.mAdd == 1.0);
    REQUIRE(shape.mMul == 1.0);
    REQUIRE(p.FromNormalized(0.5) == Approx(100.));
  }
}

TEST_CASE("IParam: InitFrequency", "[unit][param]")
{
  IParam p;

  SECTION("defaults")
  {
    p.InitFrequency("Freq");
    REQUIRE(p.Type() == IParam::kTypeDouble);
    REQUIRE(Str(p.GetName()) == "Freq");
    REQUIRE(Str(p.GetLabel()) == "Hz");
    REQUIRE(p.Unit() == IParam::kUnitFrequency);
    REQUIRE(p.GetCustomUnit() == nullptr);
    REQUIRE(p.GetShapeID() == IParam::kShapeExponential);
    REQUIRE(p.DisplayType() == IParam::kDisplayLog);
    REQUIRE(p.Value() == 1000.);
    REQUIRE(p.GetDefault() == 1000.);
    REQUIRE(p.GetMin() == 0.1);
    REQUIRE(p.GetMax() == 10000.);
    REQUIRE(p.GetStep() == 0.1);
    REQUIRE(p.GetDisplayPrecision() == 1);
    REQUIRE(Display(p) == "1000.0");
    REQUIRE(DisplayWithLabel(p) == "1000.0 Hz");
    REQUIRE(p.GetNormalized() == Approx(0.8));
  }

  SECTION("custom range, flags and group")
  {
    p.InitFrequency("LPF", 440., 20., 20000., 1., IParam::kFlagCannotAutomate, "Filter");
    REQUIRE(p.Value() == 440.);
    REQUIRE(p.GetMin() == 20.);
    REQUIRE(p.GetMax() == 20000.);
    REQUIRE(p.GetDisplayPrecision() == 0);
    REQUIRE_FALSE(p.GetCanAutomate());
    REQUIRE(Str(p.GetGroup()) == "Filter");
    REQUIRE(DisplayWithLabel(p) == "440 Hz");
    REQUIRE(p.FromNormalized(0.5) == Approx(std::sqrt(20. * 20000.)));
  }
}

TEST_CASE("IParam: InitInt", "[unit][param]")
{
  IParam p;
  p.InitInt("Voices", 4, 1, 16, "voices", IParam::kFlagMeta, "Global");

  SECTION("stores its arguments and is always stepped")
  {
    REQUIRE(p.Type() == IParam::kTypeInt);
    REQUIRE(Str(p.GetName()) == "Voices");
    REQUIRE(Str(p.GetLabel()) == "voices");
    REQUIRE(Str(p.GetGroup()) == "Global");
    REQUIRE(p.GetMin() == 1.);
    REQUIRE(p.GetMax() == 16.);
    REQUIRE(p.GetStep() == 1.);
    REQUIRE(p.GetDefault() == 4.);
    REQUIRE(p.Value() == 4.);
    REQUIRE(p.Int() == 4);
    REQUIRE(p.GetStepped());
    REQUIRE(p.GetMeta());
    REQUIRE(p.GetFlags() == (IParam::kFlagMeta | IParam::kFlagStepped));
    REQUIRE(p.GetDisplayPrecision() == 0);
    REQUIRE(p.NDisplayTexts() == 0);
    REQUIRE(p.GetShapeID() == IParam::kShapeLinear);
  }

  SECTION("rounds to the nearest integer and clamps")
  {
    struct Case { double in; int out; };
    auto c = GENERATE(Case{3.4, 3}, Case{3.5, 4}, Case{3.6, 4}, Case{1.2, 1}, Case{0.2, 1}, Case{-5., 1},
                      Case{15.5, 16}, Case{16.4, 16}, Case{99., 16}, Case{8., 8});
    CAPTURE(c.in);
    p.Set(c.in);
    REQUIRE(p.Value() == c.out);
    REQUIRE(p.Int() == c.out);
  }

  SECTION("displays without decimals")
  {
    p.Set(7.);
    REQUIRE(Display(p) == "7");
    REQUIRE(DisplayWithLabel(p) == "7 voices");
    REQUIRE(DisplayFor(p, 12.) == "12");
  }

  SECTION("normalised values snap to integers")
  {
    REQUIRE(p.FromNormalized(0.) == 1.);
    REQUIRE(p.FromNormalized(1.) == 16.);
    REQUIRE(p.FromNormalized(0.5) == Approx(9.)); // 8.5 rounds half away from zero
    REQUIRE(p.FromNormalized(0.21) == 4.);        // 4.15
    REQUIRE(p.ToNormalized(4.) == Approx(0.2));
    REQUIRE(p.ToNormalized(4.3) == Approx(0.2));
    REQUIRE(p.ConstrainNormalized(0.21) == Approx(0.2));
    p.SetNormalized(0.39); // 6.85
    REQUIRE(p.Int() == 7);
  }

  SECTION("negative ranges")
  {
    IParam n;
    n.InitInt("Transpose", 0, -24, 24);
    REQUIRE(n.GetMin() == -24.);
    REQUIRE(n.GetMax() == 24.);
    REQUIRE(n.GetNormalized() == Approx(0.5));
    n.Set(-2.5);
    REQUIRE(n.Int() == -3);
    n.Set(-2.4);
    REQUIRE(n.Int() == -2);
    n.Set(-100.);
    REQUIRE(n.Int() == -24);
    REQUIRE(Display(n) == "-24");
    REQUIRE(n.FromNormalized(0.25) == -12.);
  }
}

TEST_CASE("IParam: InitBool", "[unit][param]")
{
  IParam p;

  SECTION("defaults")
  {
    p.InitBool("Bypass", false);
    REQUIRE(p.Type() == IParam::kTypeBool);
    REQUIRE(Str(p.GetName()) == "Bypass");
    REQUIRE(p.GetMin() == 0.);
    REQUIRE(p.GetMax() == 1.);
    REQUIRE(p.GetStep() == 1.);
    REQUIRE(p.GetDefault() == 0.);
    REQUIRE(p.GetStepped());
    REQUIRE_FALSE(p.Bool());
    REQUIRE(p.Int() == 0);
    REQUIRE(p.NDisplayTexts() == 2);
    REQUIRE(Display(p) == "off");
    REQUIRE(Str(p.GetDisplayText(0.)) == "off");
    REQUIRE(Str(p.GetDisplayText(1.)) == "on");
  }

  SECTION("default true")
  {
    p.InitBool("Enable", true);
    REQUIRE(p.Bool());
    REQUIRE(p.Value() == 1.);
    REQUIRE(p.GetDefault() == 1.);
    REQUIRE(p.GetDefault(true) == 1.);
    REQUIRE(Display(p) == "on");
  }

  SECTION("only ever holds 0 or 1")
  {
    p.InitBool("B", false);
    struct Case { double in; bool out; };
    auto c = GENERATE(Case{0., false}, Case{0.49, false}, Case{0.5, true}, Case{0.51, true}, Case{1., true},
                      Case{7., true}, Case{-3., false});
    CAPTURE(c.in);
    p.Set(c.in);
    REQUIRE(p.Bool() == c.out);
    REQUIRE(p.Value() == (c.out ? 1. : 0.));
    p.SetNormalized(c.in);
    REQUIRE(p.Bool() == c.out);
    REQUIRE(p.GetNormalized() == (c.out ? 1. : 0.));
  }

  SECTION("custom on/off text, flags and group")
  {
    p.InitBool("Mode", true, "", IParam::kFlagCannotAutomate, "Switches", "Mono", "Stereo");
    REQUIRE(Display(p) == "Stereo");
    p.Set(0.);
    REQUIRE(Display(p) == "Mono");
    REQUIRE(DisplayFor(p, 1.) == "Stereo");
    REQUIRE(DisplayFor(p, 0.8, true) == "Stereo");
    REQUIRE(DisplayFor(p, 0.2, true) == "Mono");
    REQUIRE_FALSE(p.GetCanAutomate());
    REQUIRE(p.GetStepped());
    REQUIRE(Str(p.GetGroup()) == "Switches");
  }

  SECTION("text conversion")
  {
    p.InitBool("B", false);
    REQUIRE(p.StringToValue("on") == 1.);
    REQUIRE(p.StringToValue("off") == 0.);
    p.SetString("on");
    REQUIRE(p.Bool());
    p.SetString("off");
    REQUIRE_FALSE(p.Bool());
    REQUIRE(Display(p, false) == "0");
    p.Set(1.);
    REQUIRE(Display(p, false) == "1");
  }

  SECTION("the label is not shown next to the on/off text")
  {
    p.InitBool("B", true, "label");
    REQUIRE(Str(p.GetLabel()).empty());
    REQUIRE(DisplayWithLabel(p) == "on");
    p.Set(0.);
    REQUIRE(DisplayWithLabel(p) == "off");
  }
}

TEST_CASE("IParam: InitEnum with a variadic list", "[unit][param]")
{
  IParam p;
  p.InitEnum("Wave", 1, 4, "", IParam::kFlagsNone, "Osc", "Sine", "Saw", "Square", "Noise");

  SECTION("stores its arguments")
  {
    REQUIRE(p.Type() == IParam::kTypeEnum);
    REQUIRE(Str(p.GetName()) == "Wave");
    REQUIRE(Str(p.GetGroup()) == "Osc");
    REQUIRE(p.GetMin() == 0.);
    REQUIRE(p.GetMax() == 3.);
    REQUIRE(p.GetStep() == 1.);
    REQUIRE(p.GetDefault() == 1.);
    REQUIRE(p.Int() == 1);
    REQUIRE(p.GetStepped());
    REQUIRE(p.NDisplayTexts() == 4);
  }

  SECTION("display texts")
  {
    const char* expected[4] = {"Sine", "Saw", "Square", "Noise"};

    for (int i = 0; i < 4; i++)
    {
      CAPTURE(i);
      double value = -1.;
      REQUIRE(Str(p.GetDisplayTextAtIdx(i, &value)) == expected[i]);
      REQUIRE(value == i);
      REQUIRE(Str(p.GetDisplayTextAtIdx(i)) == expected[i]);
      REQUIRE(Str(p.GetDisplayText(i)) == expected[i]);
      REQUIRE(DisplayFor(p, i) == expected[i]);
      p.Set(i);
      REQUIRE(Display(p) == expected[i]);
      REQUIRE(DisplayWithLabel(p) == expected[i]);

      double mapped = -1.;
      REQUIRE(p.MapDisplayText(expected[i], &mapped));
      REQUIRE(mapped == i);
      REQUIRE(p.StringToValue(expected[i]) == i);
    }

    REQUIRE(Str(p.GetDisplayText(4.)).empty());
    REQUIRE(Str(p.GetDisplayText(-1.)).empty());
    REQUIRE(Str(p.GetDisplayText(0.5)).empty());
  }

  SECTION("unknown text does not map")
  {
    double mapped = 123.;
    REQUIRE_FALSE(p.MapDisplayText("Triangle", &mapped));
    REQUIRE(mapped == 123.);
    REQUIRE_FALSE(p.MapDisplayText("sine", &mapped)); // case sensitive
    REQUIRE_FALSE(p.MapDisplayText("", &mapped));

    const double v = p.StringToValue("Triangle");
    REQUIRE(v >= p.GetMin());
    REQUIRE(v <= p.GetMax());
  }

  SECTION("values are rounded to the nearest item before display")
  {
    REQUIRE(DisplayFor(p, 1.4) == "Saw");
    REQUIRE(DisplayFor(p, 1.6) == "Square");
    REQUIRE(DisplayFor(p, 99.) == "Noise");
    REQUIRE(DisplayFor(p, -99.) == "Sine");
    REQUIRE(DisplayFor(p, 0., true) == "Sine");
    REQUIRE(DisplayFor(p, 1. / 3., true) == "Saw");
    REQUIRE(DisplayFor(p, 0.7, true) == "Square");
    REQUIRE(DisplayFor(p, 1., true) == "Noise");
  }

  SECTION("without display text the index is shown")
  {
    p.Set(2.);
    REQUIRE(Display(p, false) == "2");
    REQUIRE(DisplayFor(p, 3., false, false) == "3");
  }

  SECTION("normalised mapping")
  {
    REQUIRE(p.FromNormalized(0.) == 0.);
    REQUIRE(p.FromNormalized(0.3) == 1.);
    REQUIRE(p.FromNormalized(0.6) == 2.);
    REQUIRE(p.FromNormalized(1.) == 3.);
    REQUIRE(p.ToNormalized(2.) == Approx(2. / 3.));
    p.SetString("Noise");
    REQUIRE(p.Int() == 3);
    REQUIRE(p.GetNormalized() == 1.);
  }
}

TEST_CASE("IParam: InitEnum without list items", "[unit][param]")
{
  IParam p;
  p.InitEnum("Slot", 2, 5, "slot");
  REQUIRE(p.Type() == IParam::kTypeEnum);
  REQUIRE(p.GetMax() == 4.);
  REQUIRE(p.NDisplayTexts() == 0);
  REQUIRE(p.Int() == 2);
  REQUIRE(Display(p) == "2");
  REQUIRE(Str(p.GetLabel()) == "slot");
  REQUIRE(DisplayWithLabel(p) == "2 slot");

  p.SetDisplayText(2, "Two");
  REQUIRE(p.NDisplayTexts() == 1);
  REQUIRE(Display(p) == "Two");
  REQUIRE(DisplayWithLabel(p) == "Two");
  p.Set(3.);
  REQUIRE(DisplayWithLabel(p) == "3 slot");
}

TEST_CASE("IParam: InitEnum with an initializer list", "[unit][param]")
{
  IParam p;
  p.InitEnum("Type", 2, {"LP", "HP", "BP"}, IParam::kFlagCannotAutomate, "Filter");

  REQUIRE(p.Type() == IParam::kTypeEnum);
  REQUIRE(Str(p.GetName()) == "Type");
  REQUIRE(Str(p.GetGroup()) == "Filter");
  REQUIRE(Str(p.GetLabel()).empty());
  REQUIRE(p.GetMin() == 0.);
  REQUIRE(p.GetMax() == 2.);
  REQUIRE(p.GetDefault() == 2.);
  REQUIRE(p.GetStepped());
  REQUIRE_FALSE(p.GetCanAutomate());
  REQUIRE(p.NDisplayTexts() == 3);
  REQUIRE(Display(p) == "BP");
  REQUIRE(DisplayFor(p, 0.) == "LP");
  REQUIRE(DisplayFor(p, 1.) == "HP");
  REQUIRE(p.StringToValue("HP") == 1.);

  SECTION("the value is clamped to the list")
  {
    p.Set(7.);
    REQUIRE(p.Int() == 2);
    p.Set(-7.);
    REQUIRE(p.Int() == 0);
  }

  SECTION("default flags and group")
  {
    IParam q;
    q.InitEnum("Q", 0, {"A", "B"});
    REQUIRE(q.GetFlags() == IParam::kFlagStepped);
    REQUIRE(Str(q.GetGroup()).empty());
    REQUIRE(q.GetMax() == 1.);
    REQUIRE(Display(q) == "A");
  }

  SECTION("a single item list")
  {
    IParam q;
    q.InitEnum("Q", 0, {"Only"});
    REQUIRE(q.NDisplayTexts() == 1);
    REQUIRE(q.GetMin() == 0.);
    REQUIRE(q.Int() == 0);
    REQUIRE(Display(q) == "Only");
  }
}

TEST_CASE("IParam: InitGain", "[unit][param]")
{
  IParam p;

  SECTION("defaults")
  {
    p.InitGain("Gain");
    REQUIRE(p.Type() == IParam::kTypeDouble);
    REQUIRE(Str(p.GetLabel()) == "dB");
    REQUIRE(p.Unit() == IParam::kUnitDB);
    REQUIRE(p.GetCustomUnit() == nullptr);
    REQUIRE(p.GetShapeID() == IParam::kShapeLinear);
    REQUIRE(p.Value() == 0.);
    REQUIRE(p.GetMin() == -70.);
    REQUIRE(p.GetMax() == 24.);
    REQUIRE(p.GetStep() == 0.5);
    REQUIRE(p.GetDisplayPrecision() == 1);
    REQUIRE_FALSE(p.GetStepped());
    REQUIRE(Display(p) == "0.0");
    REQUIRE(DisplayWithLabel(p) == "0.0 dB");
    REQUIRE(p.DBToAmp() == Approx(1.));
  }

  SECTION("DBToAmp tracks the value")
  {
    p.InitGain("Gain", -6., -60., 12., 0.1);
    REQUIRE(p.DBToAmp() == Approx(std::pow(10., -6. / 20.)));
    p.Set(12.);
    REQUIRE(p.DBToAmp() == Approx(std::pow(10., 12. / 20.)));
    p.Set(-20.);
    REQUIRE(p.DBToAmp() == Approx(0.1));
    REQUIRE(Display(p) == "-20.0");
    REQUIRE(DisplayWithLabel(p) == "-20.0 dB");
  }

  SECTION("a display text can stand in for a value, e.g. -inf")
  {
    p.InitGain("Gain", 0., -70., 12.);
    p.SetDisplayText(-70., "-inf");
    REQUIRE(Display(p) == "0.0");
    p.Set(-70.);
    REQUIRE(Display(p) == "-inf");
    REQUIRE(Display(p, false) == "-70.0");
    REQUIRE(DisplayWithLabel(p) == "-inf");
    REQUIRE(DisplayWithLabel(p, false) == "-70.0");
    REQUIRE(DisplayFor(p, 0., true) == "-inf");
    REQUIRE(p.StringToValue("-inf") == -70.);
    REQUIRE(p.StringToValue("-12.5") == -12.5);
    p.Set(-69.5);
    REQUIRE(DisplayWithLabel(p) == "-69.5 dB");
  }
}

TEST_CASE("IParam: InitPercentage, InitSeconds, InitMilliseconds and InitAngleDegrees", "[unit][param]")
{
  IParam p;

  SECTION("percentage defaults")
  {
    p.InitPercentage("Mix");
    REQUIRE(p.Type() == IParam::kTypeDouble);
    REQUIRE(Str(p.GetLabel()) == "%");
    REQUIRE(p.Unit() == IParam::kUnitPercentage);
    REQUIRE(p.Value() == 0.);
    REQUIRE(p.GetMin() == 0.);
    REQUIRE(p.GetMax() == 100.);
    REQUIRE(p.GetDisplayPrecision() == 0);
    REQUIRE(DisplayWithLabel(p) == "0 %");
  }

  SECTION("percentage is continuous but displayed as whole numbers")
  {
    p.InitPercentage("Mix", 50., -100., 100., IParam::kFlagMeta, "Out");
    REQUIRE_FALSE(p.GetStepped());
    REQUIRE(p.GetMeta());
    REQUIRE(Str(p.GetGroup()) == "Out");
    REQUIRE(p.GetMin() == -100.);
    REQUIRE(p.GetNormalized() == Approx(0.75));
    p.Set(33.3);
    REQUIRE(p.Value() == 33.3);
    REQUIRE(Display(p) == "33");
    p.Set(-33.7);
    REQUIRE(Display(p) == "-34");
  }

  SECTION("seconds")
  {
    p.InitSeconds("Attack");
    REQUIRE(Str(p.GetLabel()) == "Seconds");
    REQUIRE(p.Unit() == IParam::kUnitSeconds);
    REQUIRE(p.Value() == 1.);
    REQUIRE(p.GetMin() == 0.);
    REQUIRE(p.GetMax() == 10.);
    REQUIRE(p.GetStep() == 0.1);
    REQUIRE(p.GetDisplayPrecision() == 1);
    REQUIRE(DisplayWithLabel(p) == "1.0 Seconds");

    IParam q;
    q.InitSeconds("Release", 0.25, 0.01, 5., 0.01, IParam::kFlagCannotAutomate, "Env");
    REQUIRE(q.Value() == 0.25);
    REQUIRE(q.GetMin() == 0.01);
    REQUIRE(q.GetMax() == 5.);
    REQUIRE(q.GetDisplayPrecision() == 2);
    REQUIRE_FALSE(q.GetCanAutomate());
    REQUIRE(Str(q.GetGroup()) == "Env");
    REQUIRE(Display(q) == "0.25");
  }

  SECTION("milliseconds")
  {
    p.InitMilliseconds("Delay");
    REQUIRE(Str(p.GetLabel()) == "ms");
    REQUIRE(p.Unit() == IParam::kUnitMilliseconds);
    REQUIRE(p.Value() == 1.);
    REQUIRE(p.GetMin() == 0.);
    REQUIRE(p.GetMax() == 100.);
    REQUIRE(DisplayWithLabel(p) == "1 ms");

    IParam q;
    q.InitMilliseconds("Delay", 250., 1., 2000., IParam::kFlagMeta, "FX");
    REQUIRE(q.Value() == 250.);
    REQUIRE(q.GetMax() == 2000.);
    REQUIRE(q.GetMeta());
    REQUIRE(Str(q.GetGroup()) == "FX");
  }

  SECTION("angle")
  {
    p.InitAngleDegrees("Phase");
    REQUIRE(Str(p.GetLabel()) == "degrees");
    REQUIRE(p.Unit() == IParam::kUnitDegrees);
    REQUIRE(p.Value() == 0.);
    REQUIRE(p.GetMin() == 0.);
    REQUIRE(p.GetMax() == 360.);
    REQUIRE(DisplayWithLabel(p) == "0 degrees");

    IParam q;
    q.InitAngleDegrees("Phase", 90., -180., 180., 0, "LFO");
    REQUIRE(q.Value() == 90.);
    REQUIRE(q.GetNormalized() == Approx(0.75));
    REQUIRE(Str(q.GetGroup()) == "LFO");
    REQUIRE(DisplayWithLabel(q) == "90 degrees");
  }
}

TEST_CASE("IParam: InitPitch", "[unit][param]")
{
  IParam p;

  SECTION("defaults cover the MIDI note range with note names")
  {
    p.InitPitch("Pitch");
    REQUIRE(p.Type() == IParam::kTypeEnum);
    REQUIRE(p.GetStepped());
    REQUIRE(p.GetMin() == 0.);
    REQUIRE(p.GetMax() == 128.);
    REQUIRE(p.Int() == 60);
    REQUIRE(p.NDisplayTexts() == 129);
    REQUIRE(Display(p) == "C 3");
    p.Set(61.);
    REQUIRE(Display(p) == "C#3");
    p.Set(69.);
    REQUIRE(Display(p) == "A 3");
    p.Set(0.);
    REQUIRE(Display(p) == "C -2");
    p.Set(127.);
    REQUIRE(Display(p) == "G 8");
    REQUIRE(p.StringToValue("A 3") == 69.);
    REQUIRE(p.StringToValue("C 3") == 60.);
  }

  SECTION("middle C can be C4")
  {
    p.InitPitch("Pitch", 60, 0, 127, 0, "", true);
    REQUIRE(p.GetMax() == 127.);
    REQUIRE(p.NDisplayTexts() == 128);
    REQUIRE(Display(p) == "C 4");
    REQUIRE(DisplayFor(p, 69.) == "A 4");
    REQUIRE(DisplayFor(p, 0.) == "C -1");
  }

  SECTION("flags and group")
  {
    p.InitPitch("Root", 36, 0, 127, IParam::kFlagMeta, "Key");
    REQUIRE(p.GetMeta());
    REQUIRE(Str(p.GetGroup()) == "Key");
    REQUIRE(p.Int() == 36);
    REQUIRE(Display(p) == "C 1");
  }
}

// BUG (suspected): IPlugParameter.cpp:188-197. InitPitch() offsets the display texts by minVal (so the
// parameter is a zero based index into [minVal, maxVal]) but passes defaultVal on untouched, so with a
// non-zero minVal the default is not the requested pitch and is usually clamped to the top of the range.
TEST_CASE("IParam: InitPitch honours the default pitch when the range does not start at zero", "[.][bug]")
{
  IParam p;
  p.InitPitch("Pitch", 60, 48, 72);
  REQUIRE(p.NDisplayTexts() == 25);
  REQUIRE(Display(p) == "C 3");
  REQUIRE(DisplayFor(p, 0., true) == "C 2");
  REQUIRE(DisplayFor(p, 1., true) == "C 4");
}

TEST_CASE("IParam: numeric display formatting", "[unit][param]")
{
  IParam p;

  SECTION("uses the display precision")
  {
    p.InitDouble("P", 1.5, -10., 10., 0.01);
    REQUIRE(Display(p) == "1.50");
    p.SetDisplayPrecision(0);
    REQUIRE(p.GetDisplayPrecision() == 0);
    REQUIRE(Display(p) == "2");
    p.SetDisplayPrecision(1);
    REQUIRE(Display(p) == "1.5");
    p.SetDisplayPrecision(4);
    REQUIRE(p.GetDisplayPrecision() == 4);
    REQUIRE(Display(p) == "1.5000");
    p.Set(-0.125);
    REQUIRE(Display(p) == "-0.1250");
  }

  SECTION("zero precision rounds to the nearest integer")
  {
    p.InitDouble("P", 0., -10., 10., 1.);
    REQUIRE(DisplayFor(p, 2.4) == "2");
    REQUIRE(DisplayFor(p, 2.6) == "3");
    REQUIRE(DisplayFor(p, -2.4) == "-2");
    REQUIRE(DisplayFor(p, -2.6) == "-3");
    REQUIRE(DisplayFor(p, 0.) == "0");
    REQUIRE(DisplayFor(p, -0.2) == "0");
  }

  SECTION("an explicit value can be real or normalised")
  {
    p.InitDouble("P", 0., 0., 200., 0.1);
    REQUIRE(DisplayFor(p, 50.) == "50.0");
    REQUIRE(DisplayFor(p, 0.5, true) == "100.0");
    REQUIRE(DisplayFor(p, 0., true) == "0.0");
    REQUIRE(DisplayFor(p, 1., true) == "200.0");
    REQUIRE(DisplayFor(p, 7., true) == "200.0");
    // displaying another value does not change the parameter
    REQUIRE(p.Value() == 0.);
  }

  SECTION("kFlagNegateDisplay")
  {
    p.InitDouble("P", 5., -10., 10., 0.1, "", IParam::kFlagNegateDisplay);
    REQUIRE(p.GetNegateDisplay());
    REQUIRE(Display(p) == "-5.0");
    p.Set(-2.5);
    REQUIRE(Display(p) == "2.5");
    p.Set(0.);
    REQUIRE(Display(p) == "0.0");
    REQUIRE(p.Value() == 0.);
  }

  SECTION("kFlagSignDisplay")
  {
    p.InitDouble("P", 5., -10., 10., 0.1, "", IParam::kFlagSignDisplay);
    REQUIRE(p.GetSignDisplay());
    REQUIRE(Display(p) == "+5.0");
    p.Set(-2.5);
    REQUIRE(Display(p) == "-2.5");
    p.Set(0.);
    REQUIRE(Display(p) == "0.0");
    p.SetDisplayPrecision(2);
    p.Set(0.25);
    REQUIRE(Display(p) == "+0.25");
  }

  SECTION("negate and sign combined")
  {
    p.InitDouble("P", 5., -10., 10., 0.1, "", IParam::kFlagNegateDisplay | IParam::kFlagSignDisplay);
    REQUIRE(Display(p) == "-5.0");
    p.Set(-5.);
    REQUIRE(Display(p) == "+5.0");
  }
}

TEST_CASE("IParam: display functions", "[unit][param]")
{
  IParam p;
  int nCalls = 0;
  auto func = [&nCalls](double value, WDL_String& str) {
    nCalls++;
    str.SetFormatted(32, "<%d>", static_cast<int>(value));
  };

  SECTION("passed to InitDouble")
  {
    p.InitDouble("P", 3., 0., 10., 1., "x", 0, "", IParam::ShapeLinear(), IParam::kUnitCustom, func);
    REQUIRE(Display(p) == "<3>");
    REQUIRE(nCalls == 1);
    REQUIRE(DisplayFor(p, 7.) == "<7>");
    REQUIRE(DisplayFor(p, 0.5, true) == "<5>");
    REQUIRE(DisplayWithLabel(p) == "<3> x");
  }

  SECTION("set afterwards, and removed again")
  {
    p.InitDouble("P", 3., 0., 10., 1.);
    REQUIRE(Display(p) == "3");
    p.SetDisplayFunc(func);
    REQUIRE(Display(p) == "<3>");
    p.SetDisplayFunc(nullptr);
    REQUIRE(Display(p) == "3");
    REQUIRE(nCalls == 1);
  }

  SECTION("takes priority over display texts")
  {
    p.InitEnum("E", 1, {"A", "B"});
    p.SetDisplayFunc(func);
    REQUIRE(Display(p) == "<1>");
    REQUIRE(Display(p, false) == "<1>");
  }
}

TEST_CASE("IParam: display texts on numeric parameters", "[unit][param]")
{
  IParam p;
  p.InitDouble("P", 0., 0., 10., 0.5, "u");

  REQUIRE(p.NDisplayTexts() == 0);
  REQUIRE(Str(p.GetDisplayText(0.)).empty());

  p.SetDisplayText(0., "Min");
  p.SetDisplayText(10., "Max");
  p.SetDisplayText(2.5, "Quarter");

  REQUIRE(p.NDisplayTexts() == 3);
  REQUIRE(Str(p.GetDisplayText(0.)) == "Min");
  REQUIRE(Str(p.GetDisplayText(10.)) == "Max");
  REQUIRE(Str(p.GetDisplayText(2.5)) == "Quarter");
  REQUIRE(Str(p.GetDisplayText(5.)).empty());

  double value = -1.;
  REQUIRE(Str(p.GetDisplayTextAtIdx(0, &value)) == "Min");
  REQUIRE(value == 0.);
  REQUIRE(Str(p.GetDisplayTextAtIdx(1, &value)) == "Max");
  REQUIRE(value == 10.);
  REQUIRE(Str(p.GetDisplayTextAtIdx(2, &value)) == "Quarter");
  REQUIRE(value == 2.5);

  REQUIRE(Display(p) == "Min");
  REQUIRE(Display(p, false) == "0.0");
  REQUIRE(DisplayFor(p, 10.) == "Max");
  REQUIRE(DisplayFor(p, 2.5) == "Quarter");
  REQUIRE(DisplayFor(p, 2.5, false, false) == "2.5");
  REQUIRE(DisplayFor(p, 3.) == "3.0");
  REQUIRE(DisplayFor(p, 1., true) == "Max");

  SECTION("the label is hidden while a display text is showing")
  {
    REQUIRE(Str(p.GetLabel()).empty());
    REQUIRE(DisplayWithLabel(p) == "Min");
    p.Set(5.);
    REQUIRE(Str(p.GetLabel()) == "u");
    REQUIRE(DisplayWithLabel(p) == "5.0 u");
    p.Set(10.);
    REQUIRE(DisplayWithLabel(p) == "Max");
  }

  SECTION("MapDisplayText")
  {
    REQUIRE(p.MapDisplayText("Max", &value));
    REQUIRE(value == 10.);
    REQUIRE(p.MapDisplayText("Quarter", &value));
    REQUIRE(value == 2.5);
    value = 77.;
    REQUIRE_FALSE(p.MapDisplayText("Half", &value));
    REQUIRE(value == 77.);
  }

  SECTION("StringToValue prefers display texts, then parses numbers")
  {
    REQUIRE(p.StringToValue("Max") == 10.);
    REQUIRE(p.StringToValue("Min") == 0.);
    REQUIRE(p.StringToValue("Quarter") == 2.5);
    REQUIRE(p.StringToValue("7.5") == 7.5);
    REQUIRE(p.StringToValue("3.25") == 3.25);
  }
}

TEST_CASE("IParam: StringToValue and SetString", "[unit][param]")
{
  IParam p;

  SECTION("parses and clamps numbers")
  {
    p.InitDouble("P", 0., -10., 10., 0.01);
    REQUIRE(p.StringToValue("2.5") == 2.5);
    REQUIRE(p.StringToValue("-2.5") == -2.5);
    REQUIRE(p.StringToValue("  4") == 4.);
    REQUIRE(p.StringToValue("1e1") == 10.);
    REQUIRE(p.StringToValue("3.5 dB") == 3.5);
    REQUIRE(p.StringToValue("1000") == 10.);
    REQUIRE(p.StringToValue("-1000") == -10.);
    // StringToValue is const: the parameter is unchanged
    REQUIRE(p.Value() == 0.);
  }

  SECTION("text that is not a number is treated as zero, constrained to the range")
  {
    p.InitDouble("P", 5., -10., 10., 0.01);
    REQUIRE(p.StringToValue("abc") == 0.);
    REQUIRE(p.StringToValue("") == 0.);

    IParam q;
    q.InitDouble("Q", 5., 2., 10., 0.01);
    REQUIRE(q.StringToValue("abc") == 2.);
  }

  SECTION("stepped parameters round the parsed number")
  {
    p.InitInt("P", 0, 0, 10);
    REQUIRE(p.StringToValue("3.4") == 3.);
    REQUIRE(p.StringToValue("3.6") == 4.);
    REQUIRE(p.StringToValue("42") == 10.);
  }

  SECTION("kFlagNegateDisplay negates the parsed number")
  {
    p.InitDouble("P", 0., -10., 10., 0.1, "", IParam::kFlagNegateDisplay);
    REQUIRE(p.StringToValue("5") == -5.);
    REQUIRE(p.StringToValue("-2.5") == 2.5);
    p.Set(3.);
    // what is displayed parses back to the same value
    REQUIRE(p.StringToValue(Display(p).c_str()) == Approx(3.));
  }

  SECTION("display round trips")
  {
    p.InitDouble("P", 0., -100., 100., 0.01);
    auto v = GENERATE(-100., -12.34, -0.01, 0., 0.5, 99.99, 100.);
    CAPTURE(v);
    p.Set(v);
    REQUIRE(p.StringToValue(Display(p).c_str()) == Approx(v).margin(1e-9));
  }

  SECTION("SetString")
  {
    p.InitDouble("P", 0., -10., 10., 0.01);
    p.SetString("7.25");
    REQUIRE(p.Value() == 7.25);
    p.SetString("99");
    REQUIRE(p.Value() == 10.);

    IParam e;
    e.InitEnum("E", 0, {"One", "Two", "Three"});
    e.SetString("Three");
    REQUIRE(e.Int() == 2);
    e.SetString("Two");
    REQUIRE(e.Int() == 1);
  }
}

TEST_CASE("IParam: defaults", "[unit][param]")
{
  IParam p;
  p.InitDouble("P", 2., 0., 10., 0.1);

  p.Set(7.);
  REQUIRE(p.Value() == 7.);
  REQUIRE(p.GetDefault() == 2.);
  p.SetToDefault();
  REQUIRE(p.Value() == 2.);

  p.Set(9.);
  p.SetDefault(4.);
  REQUIRE(p.GetDefault() == 4.);
  REQUIRE(p.Value() == 4.);
  REQUIRE(p.GetDefault(true) == Approx(0.4));

  p.Set(1.);
  p.SetToDefault();
  REQUIRE(p.Value() == 4.);
}

TEST_CASE("IParam: value accessors", "[unit][param]")
{
  IParam p;
  p.InitDouble("P", 0., -10., 10., 0.01);

  SECTION("Bool is true from 0.5 upwards")
  {
    struct Case { double value; bool expected; };
    auto c = GENERATE(Case{-1., false}, Case{0., false}, Case{0.49, false}, Case{0.5, true}, Case{1., true}, Case{10., true});
    p.Set(c.value);
    REQUIRE(p.Bool() == c.expected);
  }

  SECTION("Int truncates towards zero")
  {
    struct Case { double value; int expected; };
    auto c = GENERATE(Case{0., 0}, Case{0.99, 0}, Case{1., 1}, Case{7.9, 7}, Case{-0.5, 0}, Case{-7.9, -7});
    p.Set(c.value);
    REQUIRE(p.Int() == c.expected);
  }

  SECTION("DBToAmp")
  {
    auto db = GENERATE(-10., -6., 0., 6., 10.);
    p.Set(db);
    REQUIRE(p.DBToAmp() == Approx(std::pow(10., db / 20.)));
  }
}

TEST_CASE("IParam: labels and custom units", "[unit][param]")
{
  IParam p;
  p.InitDouble("P", 1., 0., 10., 1., "Hz");
  REQUIRE(Str(p.GetLabel()) == "Hz");
  REQUIRE(Str(p.GetCustomUnit()) == "Hz");
  REQUIRE(DisplayWithLabel(p) == "1 Hz");

  p.SetLabel("kHz");
  REQUIRE(Str(p.GetLabel()) == "kHz");
  REQUIRE(Str(p.GetCustomUnit()) == "kHz");
  REQUIRE(DisplayWithLabel(p) == "1 kHz");

  p.SetLabel("");
  REQUIRE(Str(p.GetLabel()).empty());
  REQUIRE(DisplayWithLabel(p) == "1");

  SECTION("an empty label adds no trailing space")
  {
    IParam q;
    q.InitDouble("Q", 2.5, 0., 10., 0.1);
    REQUIRE(DisplayWithLabel(q) == "2.5");
  }

  SECTION("names, labels and groups can use the full buffer length")
  {
    const std::string name(MAX_PARAM_NAME_LEN - 1, 'n');
    const std::string label(MAX_PARAM_LABEL_LEN - 1, 'l');
    const std::string group(MAX_PARAM_GROUP_LEN - 1, 'g');
    IParam q;
    q.InitDouble(name.c_str(), 0., 0., 1., 0.1, label.c_str(), 0, group.c_str());
    REQUIRE(Str(q.GetName()) == name);
    REQUIRE(Str(q.GetLabel()) == label);
    REQUIRE(Str(q.GetGroup()) == group);
  }
}

TEST_CASE("IParam: flags", "[unit][param]")
{
  SECTION("each flag maps to its accessor")
  {
    auto flag = GENERATE(IParam::kFlagsNone, IParam::kFlagCannotAutomate, IParam::kFlagStepped,
                         IParam::kFlagNegateDisplay, IParam::kFlagSignDisplay, IParam::kFlagMeta);
    CAPTURE(flag);

    IParam p;
    p.InitDouble("P", 0., 0., 1., 0.1, "", flag);
    REQUIRE(p.GetFlags() == flag);
    REQUIRE(p.GetCanAutomate() == (flag != IParam::kFlagCannotAutomate));
    REQUIRE(p.GetStepped() == (flag == IParam::kFlagStepped));
    REQUIRE(p.GetNegateDisplay() == (flag == IParam::kFlagNegateDisplay));
    REQUIRE(p.GetSignDisplay() == (flag == IParam::kFlagSignDisplay));
    REQUIRE(p.GetMeta() == (flag == IParam::kFlagMeta));
  }

  SECTION("flags combine")
  {
    const int flags = IParam::kFlagCannotAutomate | IParam::kFlagMeta | IParam::kFlagSignDisplay;
    IParam p;
    p.InitDouble("P", 0., 0., 1., 0.1, "", flags);
    REQUIRE(p.GetFlags() == flags);
    REQUIRE_FALSE(p.GetCanAutomate());
    REQUIRE(p.GetMeta());
    REQUIRE(p.GetSignDisplay());
    REQUIRE_FALSE(p.GetStepped());
    REQUIRE_FALSE(p.GetNegateDisplay());
  }

  SECTION("the stepped types add kFlagStepped to the user's flags")
  {
    IParam b, e, i;
    b.InitBool("B", false, "", IParam::kFlagCannotAutomate);
    e.InitEnum("E", 0, 3, "", IParam::kFlagMeta);
    i.InitInt("I", 0, 0, 3, "", IParam::kFlagSignDisplay);
    REQUIRE(b.GetFlags() == (IParam::kFlagCannotAutomate | IParam::kFlagStepped));
    REQUIRE(e.GetFlags() == (IParam::kFlagMeta | IParam::kFlagStepped));
    REQUIRE(i.GetFlags() == (IParam::kFlagSignDisplay | IParam::kFlagStepped));
  }

  SECTION("the flag values are part of the public interface")
  {
    REQUIRE(IParam::kFlagCannotAutomate == 0x1);
    REQUIRE(IParam::kFlagStepped == 0x2);
    REQUIRE(IParam::kFlagNegateDisplay == 0x4);
    REQUIRE(IParam::kFlagSignDisplay == 0x8);
    REQUIRE(IParam::kFlagMeta == 0x10);
  }
}

TEST_CASE("IParam: GetJSON", "[unit][param]")
{
  SECTION("double parameter")
  {
    IParam p;
    p.InitDouble("Gain", 0.5, -1., 2., 0.01);
    const std::string json = JSON(p, 3);
    REQUIRE_THAT(json, StartsWith("{"));
    REQUIRE_THAT(json, EndsWith("}"));
    REQUIRE_THAT(json, ContainsSubstring("\"id\":3"));
    REQUIRE_THAT(json, ContainsSubstring("\"name\":\"Gain\""));
    REQUIRE_THAT(json, ContainsSubstring("\"type\":\"float\""));
    REQUIRE_THAT(json, ContainsSubstring("\"min\":-1.000000"));
    REQUIRE_THAT(json, ContainsSubstring("\"max\":2.000000"));
    REQUIRE_THAT(json, ContainsSubstring("\"default\":0.500000"));
    REQUIRE_THAT(json, ContainsSubstring("\"display_type\":0"));
    REQUIRE_THAT(json, ContainsSubstring("\"rate\":\"control\""));
  }

  SECTION("type names")
  {
    IParam b, i, e;
    b.InitBool("B", true);
    i.InitInt("I", 1, 0, 4);
    e.InitEnum("E", 0, {"x", "y"});
    REQUIRE_THAT(JSON(b, 0), ContainsSubstring("\"type\":\"bool\""));
    REQUIRE_THAT(JSON(b, 0), ContainsSubstring("\"default\":1.000000"));
    REQUIRE_THAT(JSON(i, 1), ContainsSubstring("\"type\":\"int\""));
    REQUIRE_THAT(JSON(i, 1), ContainsSubstring("\"max\":4.000000"));
    REQUIRE_THAT(JSON(e, 2), ContainsSubstring("\"type\":\"enum\""));
    REQUIRE_THAT(JSON(e, 2), ContainsSubstring("\"id\":2"));
  }

  SECTION("display type reflects the shape")
  {
    IParam p;
    p.InitFrequency("F");
    WDL_String expected;
    expected.SetFormatted(64, "\"display_type\":%i", static_cast<int>(IParam::kDisplayLog));
    REQUIRE_THAT(JSON(p, 0), ContainsSubstring(expected.Get()));
  }

  SECTION("appends to the string so that several parameters can be concatenated")
  {
    IParam p;
    p.InitDouble("A", 0., 0., 1., 0.1);
    WDL_String json("[");
    p.GetJSON(json, 0);
    json.Append(",");
    p.GetJSON(json, 1);
    json.Append("]");
    const std::string str = json.Get();
    REQUIRE_THAT(str, StartsWith("[{\"id\":0"));
    REQUIRE_THAT(str, ContainsSubstring("},{\"id\":1"));
    REQUIRE_THAT(str, EndsWith("}]"));
  }
}

TEST_CASE("IParam: Init from another parameter", "[unit][param]")
{
  SECTION("copies a double parameter")
  {
    IParam src;
    src.InitDouble("Cutoff", 3., 1., 9., 0.25, "Hz", IParam::kFlagMeta | IParam::kFlagSignDisplay, "Filter",
                   IParam::ShapePowCurve(3.), IParam::kUnitFrequency,
                   [](double value, WDL_String& str) { str.SetFormatted(32, "[%.1f]", value); });
    src.Set(7.);

    IParam dst;
    dst.Init(src);

    REQUIRE(dst.Type() == IParam::kTypeDouble);
    REQUIRE(Str(dst.GetName()) == "Cutoff");
    REQUIRE(Str(dst.GetLabel()) == "Hz");
    REQUIRE(Str(dst.GetGroup()) == "Filter");
    REQUIRE(dst.GetMin() == 1.);
    REQUIRE(dst.GetMax() == 9.);
    REQUIRE(dst.GetStep() == 0.25);
    REQUIRE(dst.GetDefault() == 3.);
    REQUIRE(dst.GetFlags() == src.GetFlags());
    REQUIRE(dst.Unit() == IParam::kUnitFrequency);
    REQUIRE(dst.GetShapeID() == IParam::kShapePowCurve);
    REQUIRE(dst.GetShapeValue() == 3.);
    REQUIRE(dst.DisplayType() == src.DisplayType());
    REQUIRE(dst.GetDisplayPrecision() == src.GetDisplayPrecision());
    // the copy starts at the default, not at the source's current value
    REQUIRE(dst.Value() == 3.);
    REQUIRE(Display(dst) == "[3.0]");
    REQUIRE(dst.FromNormalized(0.5) == Approx(src.FromNormalized(0.5)));

    // the two are independent afterwards
    dst.Set(5.);
    REQUIRE(src.Value() == 7.);
  }

  SECTION("copies type and display texts")
  {
    IParam src;
    src.InitEnum("Mode", 1, {"A", "B", "C"}, 0, "Modes");

    IParam dst;
    dst.Init(src);
    REQUIRE(dst.Type() == IParam::kTypeEnum);
    REQUIRE(dst.GetStepped());
    REQUIRE(dst.NDisplayTexts() == 3);
    REQUIRE(dst.Int() == 1);
    REQUIRE(Display(dst) == "B");
    REQUIRE(DisplayFor(dst, 2.) == "C");
    REQUIRE(dst.StringToValue("A") == 0.);

    IParam b;
    b.InitBool("Switch", true, "", 0, "", "no", "yes");
    IParam b2;
    b2.Init(b);
    REQUIRE(b2.Type() == IParam::kTypeBool);
    REQUIRE(b2.Bool());
    REQUIRE(Display(b2) == "yes");

    IParam i;
    i.InitInt("Count", 2, 0, 8);
    IParam i2;
    i2.Init(i);
    REQUIRE(i2.Type() == IParam::kTypeInt);
    REQUIRE(i2.Int() == 2);
  }

  SECTION("copies an exponential shape")
  {
    IParam src;
    src.InitFrequency("Freq L", 440., 20., 20000.);
    IParam dst;
    dst.Init(src);
    REQUIRE(dst.GetShapeID() == IParam::kShapeExponential);
    REQUIRE(dst.FromNormalized(0.5) == Approx(src.FromNormalized(0.5)));
    REQUIRE(dst.ToNormalized(440.) == Approx(src.ToNormalized(440.)));
  }

  SECTION("the group can be replaced")
  {
    IParam src;
    src.InitDouble("Level", 0., 0., 1., 0.1, "", 0, "Osc 1");
    IParam same, changed;
    same.Init(src);
    changed.Init(src, "", "", "Osc 2");
    REQUIRE(Str(same.GetGroup()) == "Osc 1");
    REQUIRE(Str(changed.GetGroup()) == "Osc 2");
    REQUIRE(Str(changed.GetName()) == "Level");
  }

  SECTION("a search string at the start of the name is replaced")
  {
    IParam src;
    src.InitDouble("Left Gain", 0., 0., 1., 0.1);
    IParam dst;
    dst.Init(src, "Left", "Right");
    REQUIRE(Str(dst.GetName()) == "Right Gain");
    REQUIRE(Str(src.GetName()) == "Left Gain");
  }

  SECTION("a search string that is not found leaves the name alone")
  {
    IParam src;
    src.InitDouble("Gain", 0., 0., 1., 0.1);
    IParam dst;
    dst.Init(src, "Pan", "Width");
    REQUIRE(Str(dst.GetName()) == "Gain");
  }
}

TEST_CASE("IParam: Init replaces a search string in the middle or at the end of the name", "[unit][param]")
{
  IParam src;
  src.InitDouble("Osc 1 Gain", 0., 0., 1., 0.1);

  IParam mid;
  mid.Init(src, "1", "2");
  REQUIRE(Str(mid.GetName()) == "Osc 2 Gain");

  IParam end;
  end.Init(src, "Gain", "Pan");
  REQUIRE(Str(end.GetName()) == "Osc 1 Pan");
}
