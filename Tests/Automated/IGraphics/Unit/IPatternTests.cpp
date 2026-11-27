/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for IColorStop, IPattern (solid colors and gradients) and IShadow.

#include <cmath>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "IGraphicsStructs.h"
#include "UnitTestHelpers.h"

using namespace iplug;
using namespace igraphics;
using namespace iplug_test;
using Catch::Approx;

namespace
{
const double kMargin = 1e-5;

struct TestPoint
{
  double x, y;
};

/** Maps a point in drawing coordinates in to the pattern's unit space */
TestPoint ToPatternSpace(const IPattern& pattern, double x, double y)
{
  TestPoint p {0., 0.};
  pattern.mTransform.TransformPoint(p.x, p.y, x, y);
  return p;
}

bool IsIdentity(const IMatrix& m)
{
  return m.mXX == 1. && m.mYX == 0. && m.mXY == 0. && m.mYY == 1. && m.mTX == 0. && m.mTY == 0.;
}

/** For a linear gradient the position along the gradient (0 at the start, 1 at the end) is the transformed y */
double LinearPosition(const IPattern& pattern, double x, double y)
{
  return ToPatternSpace(pattern, x, y).y;
}

/** For a radial gradient the position along the gradient is the distance from the origin after transformation */
double RadialPosition(const IPattern& pattern, double x, double y)
{
  const TestPoint p = ToPatternSpace(pattern, x, y);
  return std::sqrt(p.x * p.x + p.y * p.y);
}
} // namespace

TEST_CASE("IColorStop: default construction", "[unit][ipattern]")
{
  IColorStop stop;
  REQUIRE(stop.mOffset == 0.f);
  REQUIRE(SameColor(stop.mColor, IColor()));
}

TEST_CASE("IColorStop: construction from a color and an offset", "[unit][ipattern]")
{
  IColorStop stop {COLOR_RED, 0.25f};
  REQUIRE(stop.mOffset == 0.25f);
  REQUIRE(SameColor(stop.mColor, COLOR_RED));

  REQUIRE(IColorStop(COLOR_BLUE, 0.f).mOffset == 0.f);
  REQUIRE(IColorStop(COLOR_BLUE, 1.f).mOffset == 1.f);
}

TEST_CASE("IPattern: construction from a type has no stops", "[unit][ipattern]")
{
  for (EPatternType type : {EPatternType::Solid, EPatternType::Linear, EPatternType::Radial, EPatternType::Sweep})
  {
    IPattern pattern {type};
    REQUIRE(pattern.mType == type);
    REQUIRE(pattern.mExtend == EPatternExtend::Pad);
    REQUIRE(pattern.NStops() == 0);
    REQUIRE(IsIdentity(pattern.mTransform));
  }
}

TEST_CASE("IPattern: construction from a color is a solid pattern", "[unit][ipattern]")
{
  IPattern pattern {COLOR_ORANGE};
  REQUIRE(pattern.mType == EPatternType::Solid);
  REQUIRE(pattern.mExtend == EPatternExtend::Pad);
  REQUIRE(pattern.NStops() == 1);
  REQUIRE(SameColor(pattern.GetStop(0).mColor, COLOR_ORANGE));
  REQUIRE(pattern.GetStop(0).mOffset == 0.f);
  REQUIRE(IsIdentity(pattern.mTransform));

  // the alpha is kept
  IPattern translucent {IColor(10, 20, 30, 40)};
  REQUIRE(SameColor(translucent.GetStop(0).mColor, IColor(10, 20, 30, 40)));

  // an IColor converts implicitly
  IPattern implicit = COLOR_GREEN;
  REQUIRE(implicit.mType == EPatternType::Solid);
  REQUIRE(SameColor(implicit.GetStop(0).mColor, COLOR_GREEN));
}

TEST_CASE("IPattern: AddStop, NStops and GetStop", "[unit][ipattern]")
{
  IPattern pattern {EPatternType::Linear};

  pattern.AddStop(COLOR_RED, 0.f);
  REQUIRE(pattern.NStops() == 1);
  pattern.AddStop(COLOR_GREEN, 0.25f);
  pattern.AddStop(COLOR_BLUE, 1.f);
  REQUIRE(pattern.NStops() == 3);

  // stops are kept in the order they were added
  REQUIRE(SameColor(pattern.GetStop(0).mColor, COLOR_RED));
  REQUIRE(pattern.GetStop(0).mOffset == 0.f);
  REQUIRE(SameColor(pattern.GetStop(1).mColor, COLOR_GREEN));
  REQUIRE(pattern.GetStop(1).mOffset == 0.25f);
  REQUIRE(SameColor(pattern.GetStop(2).mColor, COLOR_BLUE));
  REQUIRE(pattern.GetStop(2).mOffset == 1.f);
}

TEST_CASE("IPattern: holds up to 16 stops", "[unit][ipattern]")
{
  IPattern pattern {EPatternType::Radial};

  for (int i = 0; i < 16; i++)
    pattern.AddStop(IColor(255, i, i * 2, i * 3), static_cast<float>(i) / 15.f);

  REQUIRE(pattern.NStops() == 16);

  for (int i = 0; i < 16; i++)
  {
    INFO("stop " << i);
    REQUIRE(SameColor(pattern.GetStop(i).mColor, IColor(255, i, i * 2, i * 3)));
    REQUIRE(pattern.GetStop(i).mOffset == Approx(static_cast<float>(i) / 15.f));

    if (i > 0)
      REQUIRE(pattern.GetStop(i).mOffset > pattern.GetStop(i - 1).mOffset);
  }
}

TEST_CASE("IPattern: SetTransform", "[unit][ipattern]")
{
  IPattern pattern {EPatternType::Linear};

  SECTION("from components")
  {
    pattern.SetTransform(1.f, 2.f, 3.f, 4.f, 5.f, 6.f);
    REQUIRE(pattern.mTransform.mXX == 1.);
    REQUIRE(pattern.mTransform.mYX == 2.);
    REQUIRE(pattern.mTransform.mXY == 3.);
    REQUIRE(pattern.mTransform.mYY == 4.);
    REQUIRE(pattern.mTransform.mTX == 5.);
    REQUIRE(pattern.mTransform.mTY == 6.);
  }

  SECTION("from a matrix")
  {
    pattern.SetTransform(IMatrix(6., 5., 4., 3., 2., 1.));
    REQUIRE(pattern.mTransform.mXX == 6.);
    REQUIRE(pattern.mTransform.mYX == 5.);
    REQUIRE(pattern.mTransform.mXY == 4.);
    REQUIRE(pattern.mTransform.mYY == 3.);
    REQUIRE(pattern.mTransform.mTX == 2.);
    REQUIRE(pattern.mTransform.mTY == 1.);
  }

  SECTION("replaces the previous transform and leaves the stops alone")
  {
    pattern.AddStop(COLOR_RED, 0.5f);
    pattern.SetTransform(1.f, 2.f, 3.f, 4.f, 5.f, 6.f);
    pattern.SetTransform(IMatrix());
    REQUIRE(IsIdentity(pattern.mTransform));
    REQUIRE(pattern.NStops() == 1);
    REQUIRE(SameColor(pattern.GetStop(0).mColor, COLOR_RED));
  }
}

TEST_CASE("IPattern: CreateLinearGradient from two points", "[unit][ipattern]")
{
  SECTION("type and defaults")
  {
    const IPattern pattern = IPattern::CreateLinearGradient(0.f, 0.f, 100.f, 0.f);
    REQUIRE(pattern.mType == EPatternType::Linear);
    REQUIRE(pattern.mExtend == EPatternExtend::Pad);
    REQUIRE(pattern.NStops() == 0);
  }

  SECTION("stops are added in order")
  {
    const IPattern pattern = IPattern::CreateLinearGradient(0.f, 0.f, 100.f, 0.f, {{COLOR_RED, 0.f}, {COLOR_GREEN, 0.5f}, {COLOR_BLUE, 1.f}});
    REQUIRE(pattern.NStops() == 3);
    REQUIRE(SameColor(pattern.GetStop(0).mColor, COLOR_RED));
    REQUIRE(pattern.GetStop(0).mOffset == 0.f);
    REQUIRE(SameColor(pattern.GetStop(1).mColor, COLOR_GREEN));
    REQUIRE(pattern.GetStop(1).mOffset == 0.5f);
    REQUIRE(SameColor(pattern.GetStop(2).mColor, COLOR_BLUE));
    REQUIRE(pattern.GetStop(2).mOffset == 1.f);
  }

  struct GradientLine
  {
    float x1, y1, x2, y2;
  };

  const GradientLine lines[] = {
    {0.f, 0.f, 100.f, 0.f},     // left to right
    {0.f, 0.f, 0.f, 100.f},     // top to bottom
    {100.f, 0.f, 0.f, 0.f},     // right to left
    {0.f, 100.f, 0.f, 0.f},     // bottom to top
    {10.f, 20.f, 110.f, 70.f},  // diagonal, away from the origin
    {-30.f, 40.f, 25.f, -15.f}, // diagonal, crossing the axes
  };

  SECTION("the start maps to position 0 and the end to position 1")
  {
    for (const GradientLine& line : lines)
    {
      INFO("(" << line.x1 << ", " << line.y1 << ") -> (" << line.x2 << ", " << line.y2 << ")");
      const IPattern pattern = IPattern::CreateLinearGradient(line.x1, line.y1, line.x2, line.y2);

      CHECK(LinearPosition(pattern, line.x1, line.y1) == Approx(0.).margin(kMargin));
      CHECK(LinearPosition(pattern, line.x2, line.y2) == Approx(1.).margin(kMargin));

      // the mid point is half way, and positions carry on beyond the ends
      CHECK(LinearPosition(pattern, 0.5 * (line.x1 + line.x2), 0.5 * (line.y1 + line.y2)) == Approx(0.5).margin(kMargin));
      CHECK(LinearPosition(pattern, 2. * line.x2 - line.x1, 2. * line.y2 - line.y1) == Approx(2.).margin(kMargin));
      CHECK(LinearPosition(pattern, 2. * line.x1 - line.x2, 2. * line.y1 - line.y2) == Approx(-1.).margin(kMargin));
    }
  }

  SECTION("moving perpendicular to the line doesn't change the position")
  {
    for (const GradientLine& line : lines)
    {
      INFO("(" << line.x1 << ", " << line.y1 << ") -> (" << line.x2 << ", " << line.y2 << ")");
      const IPattern pattern = IPattern::CreateLinearGradient(line.x1, line.y1, line.x2, line.y2);

      const double perpX = -(line.y2 - line.y1);
      const double perpY = line.x2 - line.x1;

      CHECK(LinearPosition(pattern, line.x1 + perpX, line.y1 + perpY) == Approx(0.).margin(kMargin));
      CHECK(LinearPosition(pattern, line.x2 - 3. * perpX, line.y2 - 3. * perpY) == Approx(1.).margin(kMargin));
    }
  }
}

TEST_CASE("IPattern: CreateLinearGradient across a rectangle", "[unit][ipattern]")
{
  const IRECT bounds {10.f, 20.f, 110.f, 70.f};

  SECTION("horizontal runs from the left edge to the right edge")
  {
    const IPattern pattern = IPattern::CreateLinearGradient(bounds, EDirection::Horizontal, {{COLOR_BLACK, 0.f}, {COLOR_WHITE, 1.f}});
    REQUIRE(pattern.mType == EPatternType::Linear);
    REQUIRE(pattern.NStops() == 2);
    REQUIRE(SameColor(pattern.GetStop(0).mColor, COLOR_BLACK));
    REQUIRE(SameColor(pattern.GetStop(1).mColor, COLOR_WHITE));

    for (float y : {bounds.T, bounds.MH(), bounds.B})
    {
      CHECK(LinearPosition(pattern, bounds.L, y) == Approx(0.).margin(kMargin));
      CHECK(LinearPosition(pattern, bounds.MW(), y) == Approx(0.5).margin(kMargin));
      CHECK(LinearPosition(pattern, bounds.R, y) == Approx(1.).margin(kMargin));
    }
  }

  SECTION("vertical runs from the top edge to the bottom edge")
  {
    const IPattern pattern = IPattern::CreateLinearGradient(bounds, EDirection::Vertical, {{COLOR_BLACK, 0.f}, {COLOR_WHITE, 1.f}});
    REQUIRE(pattern.mType == EPatternType::Linear);
    REQUIRE(pattern.NStops() == 2);

    for (float x : {bounds.L, bounds.MW(), bounds.R})
    {
      CHECK(LinearPosition(pattern, x, bounds.T) == Approx(0.).margin(kMargin));
      CHECK(LinearPosition(pattern, x, bounds.MH()) == Approx(0.5).margin(kMargin));
      CHECK(LinearPosition(pattern, x, bounds.B) == Approx(1.).margin(kMargin));
    }
  }

  SECTION("no stops by default")
  {
    REQUIRE(IPattern::CreateLinearGradient(bounds, EDirection::Horizontal).NStops() == 0);
    REQUIRE(IPattern::CreateLinearGradient(bounds, EDirection::Vertical).NStops() == 0);
  }
}

TEST_CASE("IPattern: CreateRadialGradient", "[unit][ipattern]")
{
  SECTION("type, defaults and stops")
  {
    REQUIRE(IPattern::CreateRadialGradient(0.f, 0.f, 10.f).mType == EPatternType::Radial);
    REQUIRE(IPattern::CreateRadialGradient(0.f, 0.f, 10.f).mExtend == EPatternExtend::Pad);
    REQUIRE(IPattern::CreateRadialGradient(0.f, 0.f, 10.f).NStops() == 0);

    const IPattern pattern = IPattern::CreateRadialGradient(0.f, 0.f, 10.f, {{COLOR_WHITE, 0.f}, {COLOR_RED, 0.75f}, {COLOR_BLACK, 1.f}});
    REQUIRE(pattern.NStops() == 3);
    REQUIRE(SameColor(pattern.GetStop(0).mColor, COLOR_WHITE));
    REQUIRE(SameColor(pattern.GetStop(1).mColor, COLOR_RED));
    REQUIRE(pattern.GetStop(1).mOffset == 0.75f);
    REQUIRE(SameColor(pattern.GetStop(2).mColor, COLOR_BLACK));
  }

  SECTION("the centre maps to position 0 and the radius to position 1 in every direction")
  {
    const float cx = 60.f, cy = 45.f, r = 25.f;
    const IPattern pattern = IPattern::CreateRadialGradient(cx, cy, r);

    CHECK(RadialPosition(pattern, cx, cy) == Approx(0.).margin(kMargin));

    for (int i = 0; i < 12; i++)
    {
      const double angle = 2. * PI * i / 12.;
      INFO("angle " << angle);
      CHECK(RadialPosition(pattern, cx + r * std::cos(angle), cy + r * std::sin(angle)) == Approx(1.).margin(kMargin));
      CHECK(RadialPosition(pattern, cx + 0.5 * r * std::cos(angle), cy + 0.5 * r * std::sin(angle)) == Approx(0.5).margin(kMargin));
      CHECK(RadialPosition(pattern, cx + 3. * r * std::cos(angle), cy + 3. * r * std::sin(angle)) == Approx(3.).margin(kMargin));
    }
  }

  SECTION("centred on the origin with a unit radius is the identity")
  {
    REQUIRE(IsIdentity(IPattern::CreateRadialGradient(0.f, 0.f, 1.f).mTransform));
  }
}

TEST_CASE("IPattern: CreateSweepGradient", "[unit][ipattern]")
{
  SECTION("type, defaults and stops")
  {
    REQUIRE(IPattern::CreateSweepGradient(0.f, 0.f).mType == EPatternType::Sweep);
    REQUIRE(IPattern::CreateSweepGradient(0.f, 0.f).mExtend == EPatternExtend::Pad);
    REQUIRE(IPattern::CreateSweepGradient(0.f, 0.f).NStops() == 0);

    const IPattern pattern = IPattern::CreateSweepGradient(0.f, 0.f, {{COLOR_RED, 0.f}, {COLOR_GREEN, 0.5f}, {COLOR_BLUE, 1.f}});
    REQUIRE(pattern.NStops() == 3);
    REQUIRE(SameColor(pattern.GetStop(0).mColor, COLOR_RED));
    REQUIRE(pattern.GetStop(0).mOffset == Approx(0.f));
    REQUIRE(SameColor(pattern.GetStop(1).mColor, COLOR_GREEN));
    REQUIRE(pattern.GetStop(1).mOffset == Approx(0.5f));
    REQUIRE(SameColor(pattern.GetStop(2).mColor, COLOR_BLUE));
    REQUIRE(pattern.GetStop(2).mOffset == Approx(1.f));
  }

  SECTION("the centre maps to the origin")
  {
    const IPattern pattern = IPattern::CreateSweepGradient(60.f, 45.f);
    const TestPoint centre = ToPatternSpace(pattern, 60., 45.);
    CHECK(centre.x == Approx(0.).margin(kMargin));
    CHECK(centre.y == Approx(0.).margin(kMargin));

    // distances from the centre are not scaled
    const TestPoint p = ToPatternSpace(pattern, 63., 49.);
    CHECK(std::sqrt(p.x * p.x + p.y * p.y) == Approx(5.).margin(kMargin));
  }

  SECTION("a partial sweep squeezes the stop offsets in to the fraction of the circle that is covered")
  {
    const IPattern pattern = IPattern::CreateSweepGradient(0.f, 0.f, {{COLOR_RED, 0.f}, {COLOR_GREEN, 0.5f}, {COLOR_BLUE, 1.f}}, 0.f, 180.f);
    REQUIRE(pattern.NStops() == 3);
    REQUIRE(pattern.GetStop(0).mOffset == Approx(0.f));
    REQUIRE(pattern.GetStop(1).mOffset == Approx(0.25f));
    REQUIRE(pattern.GetStop(2).mOffset == Approx(0.5f));

    const IPattern quarter = IPattern::CreateSweepGradient(0.f, 0.f, {{COLOR_RED, 0.f}, {COLOR_BLUE, 1.f}}, 90.f, 180.f);
    REQUIRE(quarter.GetStop(0).mOffset == Approx(0.f));
    REQUIRE(quarter.GetStop(1).mOffset == Approx(0.25f));
  }
}

TEST_CASE("IPattern: is copyable", "[unit][ipattern]")
{
  const IPattern original = IPattern::CreateLinearGradient(0.f, 0.f, 100.f, 0.f, {{COLOR_RED, 0.f}, {COLOR_BLUE, 0.5f}});
  IPattern copy = original;
  copy.AddStop(COLOR_GREEN, 1.f);

  REQUIRE(original.NStops() == 2);
  REQUIRE(copy.NStops() == 3);
  REQUIRE(copy.mType == EPatternType::Linear);
  REQUIRE(SameColor(copy.GetStop(0).mColor, COLOR_RED));
  REQUIRE(SameColor(copy.GetStop(1).mColor, COLOR_BLUE));
  REQUIRE(copy.mTransform.mTY == original.mTransform.mTY);
}

TEST_CASE("IShadow: default construction", "[unit][ishadow]")
{
  IShadow shadow;
  REQUIRE(shadow.mPattern.mType == EPatternType::Solid);
  REQUIRE(shadow.mPattern.NStops() == 1);
  REQUIRE(SameColor(shadow.mPattern.GetStop(0).mColor, COLOR_BLACK));
  REQUIRE(shadow.mBlurSize == 0.f);
  REQUIRE(shadow.mXOffset == 0.f);
  REQUIRE(shadow.mYOffset == 0.f);
  REQUIRE(shadow.mOpacity == 1.f);
  REQUIRE(shadow.mDrawForeground);
}

TEST_CASE("IShadow: construction with values", "[unit][ishadow]")
{
  SECTION("with a solid color")
  {
    IShadow shadow {COLOR_RED, 5.f, 2.f, 3.f, 0.5f};
    REQUIRE(shadow.mPattern.mType == EPatternType::Solid);
    REQUIRE(SameColor(shadow.mPattern.GetStop(0).mColor, COLOR_RED));
    REQUIRE(shadow.mBlurSize == 5.f);
    REQUIRE(shadow.mXOffset == 2.f);
    REQUIRE(shadow.mYOffset == 3.f);
    REQUIRE(shadow.mOpacity == 0.5f);
    REQUIRE(shadow.mDrawForeground);
  }

  SECTION("shadow only")
  {
    IShadow shadow {COLOR_BLACK_DROP_SHADOW, 1.f, -2.f, -3.f, 0.25f, false};
    REQUIRE(SameColor(shadow.mPattern.GetStop(0).mColor, COLOR_BLACK_DROP_SHADOW));
    REQUIRE(shadow.mXOffset == -2.f);
    REQUIRE(shadow.mYOffset == -3.f);
    REQUIRE(shadow.mOpacity == 0.25f);
    REQUIRE_FALSE(shadow.mDrawForeground);
  }

  SECTION("with a gradient")
  {
    const IPattern gradient = IPattern::CreateLinearGradient(0.f, 0.f, 0.f, 10.f, {{COLOR_BLACK, 0.f}, {COLOR_TRANSPARENT, 1.f}});
    IShadow shadow {gradient, 4.f, 0.f, 6.f, 1.f, true};
    REQUIRE(shadow.mPattern.mType == EPatternType::Linear);
    REQUIRE(shadow.mPattern.NStops() == 2);
    REQUIRE(SameColor(shadow.mPattern.GetStop(1).mColor, COLOR_TRANSPARENT));
    REQUIRE(shadow.mBlurSize == 4.f);
    REQUIRE(shadow.mYOffset == 6.f);
  }
}
