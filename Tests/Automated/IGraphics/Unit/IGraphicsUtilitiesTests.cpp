/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for the free functions in IGraphicsUtilities.h: angle conversion, RadialPoints, GetLineCrossing.

#include <cmath>
#include <type_traits>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "IGraphicsStructs.h"
#include "IGraphicsUtilities.h"

using namespace iplug;
using namespace igraphics;
using Catch::Approx;

namespace
{
const float kMargin = 1e-4f;
}

TEST_CASE("IGraphicsUtilities: DegToRad", "[unit][igraphicsutilities]")
{
  SECTION("double")
  {
    REQUIRE(DegToRad(0.) == 0.);
    REQUIRE(DegToRad(180.) == Approx(PI));
    REQUIRE(DegToRad(360.) == Approx(2. * PI));
    REQUIRE(DegToRad(90.) == Approx(0.5 * PI));
    REQUIRE(DegToRad(45.) == Approx(0.25 * PI));
    REQUIRE(DegToRad(-90.) == Approx(-0.5 * PI));
    REQUIRE(DegToRad(720.) == Approx(4. * PI));
  }

  SECTION("float")
  {
    REQUIRE(DegToRad(0.f) == 0.f);
    REQUIRE(DegToRad(180.f) == Approx(static_cast<float>(PI)));
    REQUIRE(DegToRad(90.f) == Approx(static_cast<float>(0.5 * PI)));
    REQUIRE(DegToRad(-45.f) == Approx(static_cast<float>(-0.25 * PI)));
  }

  SECTION("the result has the type of the argument")
  {
    REQUIRE((std::is_same<decltype(DegToRad(1.f)), float>::value));
    REQUIRE((std::is_same<decltype(DegToRad(1.)), double>::value));
  }

  SECTION("is linear")
  {
    REQUIRE(DegToRad(30.) + DegToRad(60.) == Approx(DegToRad(90.)));
    REQUIRE(DegToRad(2. * 77.) == Approx(2. * DegToRad(77.)));
  }
}

TEST_CASE("IGraphicsUtilities: RadToDeg", "[unit][igraphicsutilities]")
{
  SECTION("double")
  {
    REQUIRE(RadToDeg(0.) == 0.);
    REQUIRE(RadToDeg(PI) == Approx(180.));
    REQUIRE(RadToDeg(2. * PI) == Approx(360.));
    REQUIRE(RadToDeg(0.5 * PI) == Approx(90.));
    REQUIRE(RadToDeg(-0.25 * PI) == Approx(-45.));
    REQUIRE(RadToDeg(1.) == Approx(57.29577951308232));
  }

  SECTION("float")
  {
    REQUIRE(RadToDeg(0.f) == 0.f);
    REQUIRE(RadToDeg(static_cast<float>(PI)) == Approx(180.f));
    REQUIRE(RadToDeg(static_cast<float>(0.5 * PI)) == Approx(90.f));
  }

  SECTION("the result has the type of the argument")
  {
    REQUIRE((std::is_same<decltype(RadToDeg(1.f)), float>::value));
    REQUIRE((std::is_same<decltype(RadToDeg(1.)), double>::value));
  }
}

TEST_CASE("IGraphicsUtilities: DegToRad and RadToDeg are inverses", "[unit][igraphicsutilities]")
{
  for (double degrees = -720.; degrees <= 720.; degrees += 22.5)
  {
    INFO("degrees " << degrees);
    CHECK(RadToDeg(DegToRad(degrees)) == Approx(degrees).margin(1e-9));
    CHECK(RadToDeg(DegToRad(static_cast<float>(degrees))) == Approx(static_cast<float>(degrees)).margin(1e-3));
  }

  for (double radians = -6.; radians <= 6.; radians += 0.25)
  {
    INFO("radians " << radians);
    CHECK(DegToRad(RadToDeg(radians)) == Approx(radians).margin(1e-12));
  }
}

TEST_CASE("IGraphicsUtilities: RadialPoints angle is clockwise from up", "[unit][igraphicsutilities]")
{
  float data[2][2];

  SECTION("0 degrees is up")
  {
    RadialPoints(0.f, 100.f, 100.f, 10.f, 20.f, 2, data);
    CHECK(data[0][0] == Approx(100.f).margin(kMargin));
    CHECK(data[0][1] == Approx(90.f).margin(kMargin));
    CHECK(data[1][0] == Approx(100.f).margin(kMargin));
    CHECK(data[1][1] == Approx(80.f).margin(kMargin));
  }

  SECTION("90 degrees is right")
  {
    RadialPoints(90.f, 100.f, 100.f, 10.f, 20.f, 2, data);
    CHECK(data[0][0] == Approx(110.f).margin(kMargin));
    CHECK(data[0][1] == Approx(100.f).margin(kMargin));
    CHECK(data[1][0] == Approx(120.f).margin(kMargin));
    CHECK(data[1][1] == Approx(100.f).margin(kMargin));
  }

  SECTION("180 degrees is down")
  {
    RadialPoints(180.f, 100.f, 100.f, 10.f, 20.f, 2, data);
    CHECK(data[0][0] == Approx(100.f).margin(kMargin));
    CHECK(data[0][1] == Approx(110.f).margin(kMargin));
    CHECK(data[1][0] == Approx(100.f).margin(kMargin));
    CHECK(data[1][1] == Approx(120.f).margin(kMargin));
  }

  SECTION("270 degrees and -90 degrees are left")
  {
    RadialPoints(270.f, 100.f, 100.f, 10.f, 20.f, 2, data);
    CHECK(data[0][0] == Approx(90.f).margin(kMargin));
    CHECK(data[0][1] == Approx(100.f).margin(kMargin));
    CHECK(data[1][0] == Approx(80.f).margin(kMargin));
    CHECK(data[1][1] == Approx(100.f).margin(kMargin));

    RadialPoints(-90.f, 100.f, 100.f, 10.f, 20.f, 2, data);
    CHECK(data[0][0] == Approx(90.f).margin(kMargin));
    CHECK(data[0][1] == Approx(100.f).margin(kMargin));
  }

  SECTION("45 degrees is up and to the right")
  {
    RadialPoints(45.f, 0.f, 0.f, 0.f, 10.f, 2, data);
    CHECK(data[0][0] == Approx(0.f).margin(kMargin));
    CHECK(data[0][1] == Approx(0.f).margin(kMargin));
    CHECK(data[1][0] == Approx(10.f * std::sqrt(0.5f)).margin(kMargin));
    CHECK(data[1][1] == Approx(-10.f * std::sqrt(0.5f)).margin(kMargin));
  }
}

TEST_CASE("IGraphicsUtilities: RadialPoints are evenly spaced between the two radii", "[unit][igraphicsutilities]")
{
  const float cx = 50.f, cy = -20.f, rMin = 5.f, rMax = 45.f;
  const int nPoints = 9;
  float data[nPoints][2];

  for (float angle : {0.f, 33.f, 90.f, 135.f, 222.f, 359.f})
  {
    INFO("angle " << angle);
    RadialPoints(angle, cx, cy, rMin, rMax, nPoints, data);

    for (int i = 0; i < nPoints; i++)
    {
      INFO("point " << i);
      const float dx = data[i][0] - cx;
      const float dy = data[i][1] - cy;
      const float expectedRadius = rMin + (rMax - rMin) * static_cast<float>(i) / static_cast<float>(nPoints - 1);

      // the first point is at rMin, the last at rMax, with equal steps between
      CHECK(std::sqrt(dx * dx + dy * dy) == Approx(expectedRadius).margin(1e-3));

      // all the points are on the same line from the centre
      CHECK(dx == Approx(expectedRadius * std::sin(DegToRad(angle))).margin(1e-3));
      CHECK(dy == Approx(-expectedRadius * std::cos(DegToRad(angle))).margin(1e-3));
    }
  }
}

TEST_CASE("IGraphicsUtilities: RadialPoints with equal radii gives the same point", "[unit][igraphicsutilities]")
{
  float data[3][2];
  RadialPoints(90.f, 10.f, 10.f, 5.f, 5.f, 3, data);

  for (int i = 0; i < 3; i++)
  {
    CHECK(data[i][0] == Approx(15.f).margin(kMargin));
    CHECK(data[i][1] == Approx(10.f).margin(kMargin));
  }
}

// BUG: IGraphicsUtilities.h:65 RadialPoints() divides by (nPoints - 1), so asking for a single point is a
// 0 / 0 division and the point comes back as NaN rather than a point on the line (e.g. at rMin).
TEST_CASE("IGraphicsUtilities: RadialPoints with a single point", "[.][bug]")
{
  float data[1][2] = {{-1.f, -1.f}};
  RadialPoints(90.f, 100.f, 100.f, 10.f, 20.f, 1, data);
  CHECK(std::isfinite(data[0][0]));
  CHECK(std::isfinite(data[0][1]));
  CHECK(data[0][0] >= 110.f - kMargin);
  CHECK(data[0][0] <= 120.f + kMargin);
  CHECK(data[0][1] == Approx(100.f).margin(kMargin));
}

TEST_CASE("IGraphicsUtilities: GetLineCrossing gives the fraction along the second line", "[unit][igraphicsutilities]")
{
  SECTION("diagonals of a square cross in the middle")
  {
    CHECK(GetLineCrossing(IVec2(0.f, 0.f), IVec2(10.f, 10.f), IVec2(0.f, 10.f), IVec2(10.f, 0.f)) == Approx(0.5f));
    CHECK(GetLineCrossing(IVec2(0.f, 10.f), IVec2(10.f, 0.f), IVec2(0.f, 0.f), IVec2(10.f, 10.f)) == Approx(0.5f));
  }

  SECTION("a vertical line crossing a horizontal one")
  {
    // the horizontal line runs from x = 0 to x = 20, and is crossed at x = 5
    CHECK(GetLineCrossing(IVec2(5.f, 0.f), IVec2(5.f, 10.f), IVec2(0.f, 5.f), IVec2(20.f, 5.f)) == Approx(0.25f));
    CHECK(GetLineCrossing(IVec2(15.f, 0.f), IVec2(15.f, 10.f), IVec2(0.f, 5.f), IVec2(20.f, 5.f)) == Approx(0.75f));

    // reversing the second line reverses the fraction
    CHECK(GetLineCrossing(IVec2(5.f, 0.f), IVec2(5.f, 10.f), IVec2(20.f, 5.f), IVec2(0.f, 5.f)) == Approx(0.75f));

    // reversing the first line makes no difference
    CHECK(GetLineCrossing(IVec2(5.f, 10.f), IVec2(5.f, 0.f), IVec2(0.f, 5.f), IVec2(20.f, 5.f)) == Approx(0.25f));
  }

  SECTION("crossing at the ends of the second line")
  {
    CHECK(GetLineCrossing(IVec2(0.f, -5.f), IVec2(0.f, 5.f), IVec2(0.f, 0.f), IVec2(20.f, 0.f)) == Approx(0.f).margin(1e-6));
    CHECK(GetLineCrossing(IVec2(20.f, -5.f), IVec2(20.f, 5.f), IVec2(0.f, 0.f), IVec2(20.f, 0.f)) == Approx(1.f));
  }

  SECTION("the lines are infinite, so the crossing can be outside the second segment")
  {
    CHECK(GetLineCrossing(IVec2(30.f, -5.f), IVec2(30.f, 5.f), IVec2(0.f, 0.f), IVec2(20.f, 0.f)) == Approx(1.5f));
    CHECK(GetLineCrossing(IVec2(-10.f, -5.f), IVec2(-10.f, 5.f), IVec2(0.f, 0.f), IVec2(20.f, 0.f)) == Approx(-0.5f));

    // ...or beyond the points that define the first line
    CHECK(GetLineCrossing(IVec2(5.f, 100.f), IVec2(5.f, 110.f), IVec2(0.f, 0.f), IVec2(20.f, 0.f)) == Approx(0.25f));
  }

  SECTION("the crossing point lies on both lines")
  {
    const IVec2 p0 {1.f, 2.f}, p1 {7.f, 11.f}, p2 {-3.f, 8.f}, p3 {9.f, 1.f};
    const float t = GetLineCrossing(p0, p1, p2, p3);
    const float x = p2.x + t * (p3.x - p2.x);
    const float y = p2.y + t * (p3.y - p2.y);

    // (x, y) - p0 is parallel to p1 - p0
    const float cross = (x - p0.x) * (p1.y - p0.y) - (y - p0.y) * (p1.x - p0.x);
    CHECK(cross == Approx(0.f).margin(1e-3));
    CHECK(t > 0.f);
    CHECK(t < 1.f);
  }
}

TEST_CASE("IGraphicsUtilities: GetLineCrossing gives NaN for parallel lines", "[unit][igraphicsutilities]")
{
  CHECK(std::isnan(GetLineCrossing(IVec2(0.f, 0.f), IVec2(10.f, 0.f), IVec2(0.f, 5.f), IVec2(10.f, 5.f))));
  CHECK(std::isnan(GetLineCrossing(IVec2(0.f, 0.f), IVec2(10.f, 10.f), IVec2(5.f, 0.f), IVec2(15.f, 10.f))));

  // coincident lines
  CHECK(std::isnan(GetLineCrossing(IVec2(0.f, 0.f), IVec2(10.f, 10.f), IVec2(0.f, 0.f), IVec2(10.f, 10.f))));

  // a zero length line
  CHECK(std::isnan(GetLineCrossing(IVec2(3.f, 3.f), IVec2(3.f, 3.f), IVec2(0.f, 5.f), IVec2(10.f, 5.f))));
}

TEST_CASE("IGraphicsUtilities: GetTimestamp does not go backwards", "[unit][igraphicsutilities]")
{
  const double first = GetTimestamp();
  REQUIRE(first >= 0.);

  double previous = first;

  for (int i = 0; i < 100; i++)
  {
    const double now = GetTimestamp();
    REQUIRE(now >= previous);
    previous = now;
  }
}
