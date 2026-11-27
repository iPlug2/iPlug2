/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for the easing functions (IPlug/Extras/Easing.h)

// Easing.h uses M_PI, which MSVC's <math.h> only provides if this is defined before its first
// inclusion (Easing.h defines it itself, but too late if anything else has already pulled in <math.h>)
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cmath>
#include <string>
#include <vector>

#include "Easing.h"

using namespace iplug;
using Catch::Approx;

namespace
{
using EasingFunc = double (*)(double);

struct Easing
{
  const char* name;
  EasingFunc func;
  bool monotonic; // never goes backwards, and so stays within [0, 1]
};

// Elastic and back easings overshoot by design, bounce easings go back and forth within [0, 1]
const std::vector<Easing>& AllEasings()
{
  static const std::vector<Easing> easings = {
    {"Linear", EaseLinear<double>, true},
    {"QuadraticIn", EaseQuadraticIn<double>, true},
    {"QuadraticOut", EaseQuadraticOut<double>, true},
    {"QuadraticInOut", EaseQuadraticInOut<double>, true},
    {"CubicIn", EaseCubicIn<double>, true},
    {"CubicOut", EaseCubicOut<double>, true},
    {"CubicInOut", EaseCubicInOut<double>, true},
    {"QuarticIn", EaseQuarticIn<double>, true},
    {"QuarticOut", EaseQuarticOut<double>, true},
    {"QuarticInOut", EaseQuarticInOut<double>, true},
    {"QuinticIn", EaseQuinticIn<double>, true},
    {"QuinticOut", EaseQuinticOut<double>, true},
    {"QuinticInOut", EaseQuinticInOut<double>, true},
    {"SineIn", EaseSineIn<double>, true},
    {"SineOut", EaseSineOut<double>, true},
    {"SineInOut", EaseSineInOut<double>, true},
    {"CircularIn", EaseCircularIn<double>, true},
    {"CircularOut", EaseCircularOut<double>, true},
    {"CircularInOut", EaseCircularInOut<double>, true},
    {"ExponentialIn", EaseExponentialIn<double>, true},
    {"ExponentialOut", EaseExponentialOut<double>, true},
    {"ExponentialInOut", EaseExponentialInOut<double>, true},
    {"ElasticIn", EaseElasticIn<double>, false},
    {"ElasticOut", EaseElasticOut<double>, false},
    {"ElasticInOut", EaseElasticInOut<double>, false},
    {"BackIn", EaseBackIn<double>, false},
    {"BackOut", EaseBackOut<double>, false},
    {"BackInOut", EaseBackInOut<double>, false},
    {"BounceIn", EaseBounceIn<double>, false},
    {"BounceOut", EaseBounceOut<double>, false},
    {"BounceInOut", EaseBounceInOut<double>, false},
  };

  return easings;
}

struct EasingFamily
{
  const char* name;
  EasingFunc in, out, inOut;
};

const std::vector<EasingFamily>& AllFamilies()
{
  static const std::vector<EasingFamily> families = {
    {"Quadratic", EaseQuadraticIn<double>, EaseQuadraticOut<double>, EaseQuadraticInOut<double>},
    {"Cubic", EaseCubicIn<double>, EaseCubicOut<double>, EaseCubicInOut<double>},
    {"Quartic", EaseQuarticIn<double>, EaseQuarticOut<double>, EaseQuarticInOut<double>},
    {"Quintic", EaseQuinticIn<double>, EaseQuinticOut<double>, EaseQuinticInOut<double>},
    {"Sine", EaseSineIn<double>, EaseSineOut<double>, EaseSineInOut<double>},
    {"Circular", EaseCircularIn<double>, EaseCircularOut<double>, EaseCircularInOut<double>},
    {"Exponential", EaseExponentialIn<double>, EaseExponentialOut<double>, EaseExponentialInOut<double>},
    {"Elastic", EaseElasticIn<double>, EaseElasticOut<double>, EaseElasticInOut<double>},
    {"Back", EaseBackIn<double>, EaseBackOut<double>, EaseBackInOut<double>},
    {"Bounce", EaseBounceIn<double>, EaseBounceOut<double>, EaseBounceInOut<double>},
  };

  return families;
}

constexpr int kSteps = 1000;

double X(int step)
{
  return static_cast<double>(step) / static_cast<double>(kSteps);
}
} // namespace

TEST_CASE("Easing: every function starts at 0 and ends at 1", "[unit][extras][easing]")
{
  for (const auto& easing : AllEasings())
  {
    CAPTURE(easing.name);
    REQUIRE(easing.func(0.) == Approx(0.).margin(1e-9));
    REQUIRE(easing.func(1.) == Approx(1.).margin(1e-9));
  }
}

TEST_CASE("Easing: every function is finite over [0, 1]", "[unit][extras][easing]")
{
  for (const auto& easing : AllEasings())
  {
    bool finite = true;
    bool bounded = true;

    for (int i = 0; i <= kSteps; i++)
    {
      const double y = easing.func(X(i));

      if (!std::isfinite(y))
        finite = false;

      // the overshooting easings stay within half the range either side
      if (y < -0.5 || y > 1.5)
        bounded = false;
    }

    CAPTURE(easing.name);
    REQUIRE(finite);
    REQUIRE(bounded);
  }
}

TEST_CASE("Easing: non-overshooting functions are monotonic and stay within [0, 1]", "[unit][extras][easing]")
{
  for (const auto& easing : AllEasings())
  {
    if (!easing.monotonic)
      continue;

    bool monotonic = true;
    bool bounded = true;
    double prev = easing.func(0.);

    for (int i = 1; i <= kSteps; i++)
    {
      const double y = easing.func(X(i));

      if (y < prev - 1e-12)
        monotonic = false;
      if (y < -1e-12 || y > 1. + 1e-12)
        bounded = false;

      prev = y;
    }

    CAPTURE(easing.name);
    REQUIRE(monotonic);
    REQUIRE(bounded);
  }
}

TEST_CASE("Easing: bounce stays within [0, 1]", "[unit][extras][easing]")
{
  const EasingFunc bounces[] = {EaseBounceIn<double>, EaseBounceOut<double>, EaseBounceInOut<double>};

  for (EasingFunc func : bounces)
  {
    bool bounded = true;

    for (int i = 0; i <= kSteps; i++)
    {
      const double y = func(X(i));
      if (y < -1e-9 || y > 1. + 1e-9)
        bounded = false;
    }

    REQUIRE(bounded);
  }
}

TEST_CASE("Easing: elastic and back overshoot the range", "[unit][extras][easing]")
{
  const auto minMax = [](EasingFunc func, double& minValue, double& maxValue) {
    minValue = 1e9;
    maxValue = -1e9;

    for (int i = 0; i <= kSteps; i++)
    {
      const double y = func(X(i));
      minValue = std::fmin(minValue, y);
      maxValue = std::fmax(maxValue, y);
    }
  };

  double lo, hi;

  // "in" easings dip below zero on the way...
  minMax(EaseElasticIn<double>, lo, hi);
  REQUIRE(lo < -0.1);
  REQUIRE(hi <= 1. + 1e-9);

  minMax(EaseBackIn<double>, lo, hi);
  REQUIRE(lo < -0.05);
  REQUIRE(hi <= 1. + 1e-9);

  // ..."out" easings go past one...
  minMax(EaseElasticOut<double>, lo, hi);
  REQUIRE(hi > 1.1);
  REQUIRE(lo >= -1e-9);

  minMax(EaseBackOut<double>, lo, hi);
  REQUIRE(hi > 1.05);
  REQUIRE(lo >= -1e-9);

  // ...and "in out" easings do both
  minMax(EaseElasticInOut<double>, lo, hi);
  REQUIRE(lo < -0.05);
  REQUIRE(hi > 1.05);

  minMax(EaseBackInOut<double>, lo, hi);
  REQUIRE(lo < -0.02);
  REQUIRE(hi > 1.02);
}

TEST_CASE("Easing: out is the mirror image of in", "[unit][extras][easing]")
{
  for (const auto& family : AllFamilies())
  {
    double maxError = 0.;

    for (int i = 0; i <= kSteps; i++)
    {
      const double x = X(i);
      maxError = std::fmax(maxError, std::fabs(family.out(x) - (1. - family.in(1. - x))));
    }

    CAPTURE(family.name);
    REQUIRE(maxError < 1e-9);
  }
}

TEST_CASE("Easing: in-out is in followed by out, each at half scale", "[unit][extras][easing]")
{
  for (const auto& family : AllFamilies())
  {
    double maxError = 0.;

    for (int i = 0; i <= kSteps; i++)
    {
      const double x = X(i);
      const double expected = x < 0.5 ? 0.5 * family.in(2. * x) : 0.5 + 0.5 * family.out(2. * x - 1.);
      maxError = std::fmax(maxError, std::fabs(family.inOut(x) - expected));
    }

    CAPTURE(family.name);
    REQUIRE(maxError < 1e-9);
    REQUIRE(family.inOut(0.5) == Approx(0.5).margin(1e-9));
  }
}

TEST_CASE("Easing: in-out functions have point symmetry about the middle", "[unit][extras][easing]")
{
  for (const auto& family : AllFamilies())
  {
    double maxError = 0.;

    for (int i = 0; i <= kSteps; i++)
    {
      const double x = X(i);
      maxError = std::fmax(maxError, std::fabs(family.inOut(x) + family.inOut(1. - x) - 1.));
    }

    CAPTURE(family.name);
    REQUIRE(maxError < 1e-9);
  }

  REQUIRE(EaseLinear(0.5) == 0.5);
}

TEST_CASE("Easing: in functions start slowly and out functions finish slowly", "[unit][extras][easing]")
{
  for (const auto& family : AllFamilies())
  {
    // elastic, back and bounce move around too much for this to mean anything
    const std::string name = family.name;
    if (name == "Elastic" || name == "Back" || name == "Bounce")
      continue;

    CAPTURE(family.name);
    REQUIRE(family.in(0.25) < 0.25);
    REQUIRE(family.in(0.5) < 0.5);
    REQUIRE(family.out(0.5) > 0.5);
    REQUIRE(family.out(0.75) > 0.75);
    REQUIRE(family.inOut(0.25) < 0.25);
    REQUIRE(family.inOut(0.75) > 0.75);
  }
}

TEST_CASE("Easing: polynomial easings have the expected values", "[unit][extras][easing]")
{
  REQUIRE(EaseLinear(0.3) == Approx(0.3));

  REQUIRE(EaseQuadraticIn(0.5) == Approx(0.25));
  REQUIRE(EaseCubicIn(0.5) == Approx(0.125));
  REQUIRE(EaseQuarticIn(0.5) == Approx(0.0625));
  REQUIRE(EaseQuinticIn(0.5) == Approx(0.03125));

  REQUIRE(EaseQuadraticOut(0.5) == Approx(0.75));
  REQUIRE(EaseCubicOut(0.5) == Approx(0.875));
  REQUIRE(EaseQuarticOut(0.5) == Approx(0.9375));
  REQUIRE(EaseQuinticOut(0.5) == Approx(0.96875));

  REQUIRE(EaseQuadraticInOut(0.25) == Approx(0.125));
  REQUIRE(EaseCubicInOut(0.25) == Approx(0.0625));
  REQUIRE(EaseQuarticInOut(0.25) == Approx(0.03125));
  REQUIRE(EaseQuinticInOut(0.25) == Approx(0.015625));
}

TEST_CASE("Easing: sine, circular and exponential easings have the expected values", "[unit][extras][easing]")
{
  REQUIRE(EaseSineOut(1. / 3.) == Approx(0.5));
  REQUIRE(EaseSineIn(2. / 3.) == Approx(0.5));
  REQUIRE(EaseSineInOut(0.25) == Approx(0.5 - 0.25 * std::sqrt(2.)));

  // quarter circles
  REQUIRE(EaseCircularIn(0.6) == Approx(0.2));
  REQUIRE(EaseCircularIn(0.8) == Approx(0.4));
  REQUIRE(EaseCircularOut(0.4) == Approx(0.8));
  REQUIRE(EaseCircularOut(0.2) == Approx(0.6));

  // 2^(10(x-1)), i.e. doubling every tenth
  REQUIRE(EaseExponentialIn(0.9) == Approx(0.5));
  REQUIRE(EaseExponentialIn(0.8) == Approx(0.25));
  REQUIRE(EaseExponentialIn(0.5) == Approx(1. / 32.));
  REQUIRE(EaseExponentialOut(0.1) == Approx(0.5));
  REQUIRE(EaseExponentialOut(0.2) == Approx(0.75));
}

TEST_CASE("Easing: bounce out lands on the floor between bounces", "[unit][extras][easing]")
{
  // four parabolic arcs, each touching 1.0, at these points
  REQUIRE(EaseBounceOut(4. / 11.) == Approx(1.).margin(1e-9));
  REQUIRE(EaseBounceOut(8. / 11.) == Approx(1.).margin(1e-9));
  REQUIRE(EaseBounceOut(9. / 10.) == Approx(1.).margin(1e-9));
  REQUIRE(EaseBounceOut(1.) == Approx(1.).margin(1e-9));

  // and coming back off it in between
  REQUIRE(EaseBounceOut(6. / 11.) < 0.8);
  REQUIRE(EaseBounceOut(0.82) < 0.96);
  REQUIRE(EaseBounceOut(0.95) < 0.99);

  // continuous where the arcs join
  const double eps = 1e-7;
  for (double x : {4. / 11., 8. / 11., 9. / 10.})
    REQUIRE(EaseBounceOut(x - eps) == Approx(EaseBounceOut(x + eps)).margin(1e-5));
}

TEST_CASE("Easing: pow curve", "[unit][extras][easing]")
{
  // x^(1/c): c = 1 is linear, c > 1 bends up, c < 1 bends down
  for (double c : {0.25, 0.5, 1., 2., 4.})
  {
    CAPTURE(c);
    REQUIRE(EasePowCurve(0., c) == Approx(0.).margin(1e-12));
    REQUIRE(EasePowCurve(1., c) == Approx(1.));

    double prev = 0.;
    bool monotonic = true;
    for (int i = 1; i <= 100; i++)
    {
      const double y = EasePowCurve(static_cast<double>(i) / 100., c);
      if (y <= prev)
        monotonic = false;
      prev = y;
    }
    REQUIRE(monotonic);
  }

  REQUIRE(EasePowCurve(0.3, 1.) == Approx(0.3));
  REQUIRE(EasePowCurve(0.25, 2.) == Approx(0.5));
  REQUIRE(EasePowCurve(0.5, 0.5) == Approx(0.25));
  REQUIRE(EasePowCurve(0.5, 4.) > 0.5);
  REQUIRE(EasePowCurve(0.5, 0.25) < 0.5);
}

TEST_CASE("Easing: single precision agrees with double precision", "[unit][extras][easing]")
{
  using EasingFuncF = float (*)(float);

  struct Pair
  {
    const char* name;
    EasingFuncF single;
    EasingFunc dbl;
  };

  const std::vector<Pair> pairs = {
    {"Linear", EaseLinear<float>, EaseLinear<double>},
    {"QuadraticInOut", EaseQuadraticInOut<float>, EaseQuadraticInOut<double>},
    {"CubicOut", EaseCubicOut<float>, EaseCubicOut<double>},
    {"QuarticOut", EaseQuarticOut<float>, EaseQuarticOut<double>},
    {"QuinticInOut", EaseQuinticInOut<float>, EaseQuinticInOut<double>},
    {"SineInOut", EaseSineInOut<float>, EaseSineInOut<double>},
    {"CircularInOut", EaseCircularInOut<float>, EaseCircularInOut<double>},
    {"ExponentialInOut", EaseExponentialInOut<float>, EaseExponentialInOut<double>},
    {"ElasticOut", EaseElasticOut<float>, EaseElasticOut<double>},
    {"BackInOut", EaseBackInOut<float>, EaseBackInOut<double>},
    {"BounceInOut", EaseBounceInOut<float>, EaseBounceInOut<double>},
  };

  for (const auto& pair : pairs)
  {
    double maxError = 0.;

    // a step that is exact in both precisions, so both see the same x
    for (int i = 0; i <= 256; i++)
    {
      const float x = static_cast<float>(i) / 256.f;
      maxError = std::fmax(maxError, std::fabs(static_cast<double>(pair.single(x)) - pair.dbl(static_cast<double>(x))));
    }

    CAPTURE(pair.name);
    REQUIRE(maxError < 1e-4);
  }
}
