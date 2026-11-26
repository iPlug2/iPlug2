/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for IMatrix, the 2D affine transformation matrix.

#include <cmath>

#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "IGraphicsStructs.h"

using namespace iplug;
using namespace igraphics;
using Catch::Approx;

namespace
{
const double kMargin = 1e-9;
// Translate/Scale/Rotate/Skew take floats, so results involving them are only accurate to float precision
const double kFloatMargin = 1e-5;

struct Point
{
  double x, y;
};

Point Apply(const IMatrix& m, double x, double y)
{
  Point p {0., 0.};
  m.TransformPoint(p.x, p.y, x, y);
  return p;
}

void CheckMatrix(const IMatrix& m, double xx, double yx, double xy, double yy, double tx, double ty, double margin = kMargin)
{
  CHECK(m.mXX == Approx(xx).margin(margin));
  CHECK(m.mYX == Approx(yx).margin(margin));
  CHECK(m.mXY == Approx(xy).margin(margin));
  CHECK(m.mYY == Approx(yy).margin(margin));
  CHECK(m.mTX == Approx(tx).margin(margin));
  CHECK(m.mTY == Approx(ty).margin(margin));
}

void CheckIdentity(const IMatrix& m, double margin = kMargin)
{
  CheckMatrix(m, 1., 0., 0., 1., 0., 0., margin);
}

void CheckSame(const IMatrix& a, const IMatrix& b, double margin = kMargin)
{
  CheckMatrix(a, b.mXX, b.mYX, b.mXY, b.mYY, b.mTX, b.mTY, margin);
}
} // namespace

TEST_CASE("IMatrix: default construction is the identity", "[unit][imatrix]")
{
  IMatrix m;
  REQUIRE(m.mXX == 1.);
  REQUIRE(m.mYX == 0.);
  REQUIRE(m.mXY == 0.);
  REQUIRE(m.mYY == 1.);
  REQUIRE(m.mTX == 0.);
  REQUIRE(m.mTY == 0.);

  for (double x : {-3., 0., 1.5, 100.})
  {
    for (double y : {-7., 0., 2.25, 50.})
    {
      const Point p = Apply(m, x, y);
      REQUIRE(p.x == x);
      REQUIRE(p.y == y);
    }
  }
}

TEST_CASE("IMatrix: construction from components", "[unit][imatrix]")
{
  IMatrix m {1., 2., 3., 4., 5., 6.};
  REQUIRE(m.mXX == 1.);
  REQUIRE(m.mYX == 2.);
  REQUIRE(m.mXY == 3.);
  REQUIRE(m.mYY == 4.);
  REQUIRE(m.mTX == 5.);
  REQUIRE(m.mTY == 6.);
}

TEST_CASE("IMatrix: TransformPoint", "[unit][imatrix]")
{
  // x' = x * xx + y * xy + tx, y' = x * yx + y * yy + ty
  const IMatrix m {1., 2., 3., 4., 5., 6.};

  SECTION("with separate input and output")
  {
    double x = 0., y = 0.;
    m.TransformPoint(x, y, 10., 100.);
    REQUIRE(x == 10. * 1. + 100. * 3. + 5.);
    REQUIRE(y == 10. * 2. + 100. * 4. + 6.);
  }

  SECTION("in place")
  {
    double x = 10., y = 100.;
    m.TransformPoint(x, y);
    REQUIRE(x == 315.);
    REQUIRE(y == 426.);
  }

  SECTION("the origin maps to the translation")
  {
    const Point p = Apply(m, 0., 0.);
    REQUIRE(p.x == 5.);
    REQUIRE(p.y == 6.);
  }

  SECTION("the unit vectors map to the columns")
  {
    const Point px = Apply(m, 1., 0.);
    REQUIRE(px.x == 1. + 5.);
    REQUIRE(px.y == 2. + 6.);
    const Point py = Apply(m, 0., 1.);
    REQUIRE(py.x == 3. + 5.);
    REQUIRE(py.y == 4. + 6.);
  }
}

TEST_CASE("IMatrix: Translate", "[unit][imatrix]")
{
  IMatrix m;
  IMatrix& result = m.Translate(10.f, -20.f);
  REQUIRE(&result == &m);
  CheckMatrix(m, 1., 0., 0., 1., 10., -20.);

  const Point p = Apply(m, 1., 2.);
  REQUIRE(p.x == Approx(11.));
  REQUIRE(p.y == Approx(-18.));

  // translations accumulate
  m.Translate(5.f, 5.f);
  CheckMatrix(m, 1., 0., 0., 1., 15., -15.);
}

TEST_CASE("IMatrix: Scale", "[unit][imatrix]")
{
  IMatrix m;
  IMatrix& result = m.Scale(2.f, 3.f);
  REQUIRE(&result == &m);
  CheckMatrix(m, 2., 0., 0., 3., 0., 0.);

  const Point p = Apply(m, 10., 10.);
  REQUIRE(p.x == Approx(20.));
  REQUIRE(p.y == Approx(30.));

  // scales multiply
  m.Scale(0.5f, 2.f);
  CheckMatrix(m, 1., 0., 0., 6., 0., 0.);

  // a negative scale flips
  const Point flipped = Apply(IMatrix().Scale(-1.f, 1.f), 3., 4.);
  REQUIRE(flipped.x == Approx(-3.));
  REQUIRE(flipped.y == Approx(4.));
}

TEST_CASE("IMatrix: Rotate is clockwise in degrees, with y pointing down", "[unit][imatrix]")
{
  SECTION("90 degrees")
  {
    IMatrix m;
    IMatrix& result = m.Rotate(90.f);
    REQUIRE(&result == &m);
    CheckMatrix(m, 0., 1., -1., 0., 0., 0., kFloatMargin);

    // the x axis rotates on to the y axis (i.e. right -> down)
    Point p = Apply(m, 1., 0.);
    CHECK(p.x == Approx(0.).margin(kFloatMargin));
    CHECK(p.y == Approx(1.).margin(kFloatMargin));

    // down -> left
    p = Apply(m, 0., 1.);
    CHECK(p.x == Approx(-1.).margin(kFloatMargin));
    CHECK(p.y == Approx(0.).margin(kFloatMargin));
  }

  SECTION("180 degrees")
  {
    const Point p = Apply(IMatrix().Rotate(180.f), 3., 4.);
    CHECK(p.x == Approx(-3.).margin(kFloatMargin));
    CHECK(p.y == Approx(-4.).margin(kFloatMargin));
  }

  SECTION("45 degrees")
  {
    const Point p = Apply(IMatrix().Rotate(45.f), 1., 0.);
    CHECK(p.x == Approx(std::sqrt(0.5)).margin(kFloatMargin));
    CHECK(p.y == Approx(std::sqrt(0.5)).margin(kFloatMargin));
  }

  SECTION("0 and 360 degrees are the identity")
  {
    CheckIdentity(IMatrix().Rotate(0.f));
    CheckIdentity(IMatrix().Rotate(360.f), kFloatMargin);
  }

  SECTION("negative angles rotate the other way")
  {
    const Point p = Apply(IMatrix().Rotate(-90.f), 1., 0.);
    CHECK(p.x == Approx(0.).margin(kFloatMargin));
    CHECK(p.y == Approx(-1.).margin(kFloatMargin));
  }

  SECTION("rotations accumulate")
  {
    CheckSame(IMatrix().Rotate(30.f).Rotate(60.f), IMatrix().Rotate(90.f), kFloatMargin);
    CheckIdentity(IMatrix().Rotate(120.f).Rotate(120.f).Rotate(120.f), kFloatMargin);
  }

  SECTION("rotation preserves distance from the origin")
  {
    for (float angle : {10.f, 77.f, 135.f, 200.f, 359.f})
    {
      const Point p = Apply(IMatrix().Rotate(angle), 3., 4.);
      INFO("angle " << angle);
      CHECK(std::sqrt(p.x * p.x + p.y * p.y) == Approx(5.).margin(kFloatMargin));
    }
  }
}

TEST_CASE("IMatrix: Skew", "[unit][imatrix]")
{
  SECTION("in x: x moves in proportion to y")
  {
    IMatrix m;
    IMatrix& result = m.Skew(45.f, 0.f);
    REQUIRE(&result == &m);
    CheckMatrix(m, 1., 0., 1., 1., 0., 0., kFloatMargin);

    const Point p = Apply(m, 10., 5.);
    CHECK(p.x == Approx(15.).margin(1e-4));
    CHECK(p.y == Approx(5.).margin(1e-4));
  }

  SECTION("in y: y moves in proportion to x")
  {
    IMatrix m;
    m.Skew(0.f, 45.f);
    CheckMatrix(m, 1., 1., 0., 1., 0., 0., kFloatMargin);

    const Point p = Apply(m, 10., 5.);
    CHECK(p.x == Approx(10.).margin(1e-4));
    CHECK(p.y == Approx(15.).margin(1e-4));
  }

  SECTION("no skew is the identity")
  {
    CheckIdentity(IMatrix().Skew(0.f, 0.f));
  }

  SECTION("points on the axis are not moved")
  {
    const Point p = Apply(IMatrix().Skew(30.f, 0.f), 7., 0.);
    CHECK(p.x == Approx(7.));
    CHECK(p.y == Approx(0.).margin(kMargin));
  }
}

TEST_CASE("IMatrix: transforms are applied to points in the reverse of the order they are added", "[unit][imatrix]")
{
  // As with a canvas/NanoVG/Cairo context: the most recently added transform is the first to be applied to a point

  SECTION("translate then scale")
  {
    const Point p = Apply(IMatrix().Translate(10.f, 20.f).Scale(2.f, 2.f), 1., 1.);
    CHECK(p.x == Approx(12.));
    CHECK(p.y == Approx(22.));
  }

  SECTION("scale then translate")
  {
    const Point p = Apply(IMatrix().Scale(2.f, 2.f).Translate(10.f, 20.f), 1., 1.);
    CHECK(p.x == Approx(22.));
    CHECK(p.y == Approx(42.));
  }

  SECTION("rotating about a point")
  {
    // translate to the pivot, rotate, translate back
    const IMatrix m = IMatrix().Translate(50.f, 50.f).Rotate(90.f).Translate(-50.f, -50.f);

    Point p = Apply(m, 50., 50.);
    CHECK(p.x == Approx(50.).margin(1e-4));
    CHECK(p.y == Approx(50.).margin(1e-4));

    p = Apply(m, 60., 50.);
    CHECK(p.x == Approx(50.).margin(1e-4));
    CHECK(p.y == Approx(60.).margin(1e-4));
  }

  SECTION("the order matters")
  {
    const Point a = Apply(IMatrix().Rotate(90.f).Translate(10.f, 0.f), 0., 0.);
    const Point b = Apply(IMatrix().Translate(10.f, 0.f).Rotate(90.f), 0., 0.);
    CHECK(a.x == Approx(0.).margin(1e-4));
    CHECK(a.y == Approx(10.).margin(1e-4));
    CHECK(b.x == Approx(10.).margin(1e-4));
    CHECK(b.y == Approx(0.).margin(1e-4));
  }
}

TEST_CASE("IMatrix: Transform with another matrix", "[unit][imatrix]")
{
  const IMatrix a {1., 2., 3., 4., 5., 6.};
  const IMatrix b {-2., 0.5, 1.5, 3., -4., 7.};

  SECTION("returns a reference to the transformed matrix")
  {
    IMatrix m = a;
    IMatrix& result = m.Transform(b);
    REQUIRE(&result == &m);
  }

  SECTION("the result applies the argument first, then the original")
  {
    IMatrix combined = a;
    combined.Transform(b);

    for (double x : {-3., 0., 1.5, 100.})
    {
      for (double y : {-7., 0., 2.25, 50.})
      {
        const Point viaB = Apply(b, x, y);
        const Point expected = Apply(a, viaB.x, viaB.y);
        const Point actual = Apply(combined, x, y);
        INFO("x " << x << " y " << y);
        CHECK(actual.x == Approx(expected.x).margin(kMargin));
        CHECK(actual.y == Approx(expected.y).margin(kMargin));
      }
    }
  }

  SECTION("the identity is neutral on either side")
  {
    IMatrix m = a;
    m.Transform(IMatrix());
    CheckSame(m, a);

    IMatrix identity;
    identity.Transform(a);
    CheckSame(identity, a);
  }

  SECTION("is associative")
  {
    const IMatrix c {0.5, -1., 2., 0.25, 9., -3.};

    IMatrix left = a;
    left.Transform(b);
    left.Transform(c);

    IMatrix bc = b;
    bc.Transform(c);
    IMatrix right = a;
    right.Transform(bc);

    CheckSame(left, right);
  }

  SECTION("is not commutative")
  {
    IMatrix ab = a;
    ab.Transform(b);
    IMatrix ba = b;
    ba.Transform(a);

    const Point p = Apply(ab, 1., 1.);
    const Point q = Apply(ba, 1., 1.);
    CHECK((p.x != q.x || p.y != q.y));
  }
}

TEST_CASE("IMatrix: Transform mapping one rectangle to another", "[unit][imatrix]")
{
  const IRECT before {10.f, 20.f, 110.f, 70.f};
  const IRECT after {0.f, 0.f, 200.f, 200.f};

  IMatrix m;
  IMatrix& result = m.Transform(before, after);
  REQUIRE(&result == &m);

  SECTION("corners and centre map across")
  {
    Point p = Apply(m, before.L, before.T);
    CHECK(p.x == Approx(after.L).margin(kMargin));
    CHECK(p.y == Approx(after.T).margin(kMargin));

    p = Apply(m, before.R, before.B);
    CHECK(p.x == Approx(after.R));
    CHECK(p.y == Approx(after.B));

    p = Apply(m, before.R, before.T);
    CHECK(p.x == Approx(after.R));
    CHECK(p.y == Approx(after.T).margin(kMargin));

    p = Apply(m, before.MW(), before.MH());
    CHECK(p.x == Approx(after.MW()));
    CHECK(p.y == Approx(after.MH()));
  }

  SECTION("there is no rotation or skew")
  {
    CHECK(m.mXX == Approx(2.));
    CHECK(m.mYY == Approx(4.));
    CHECK(m.mYX == 0.);
    CHECK(m.mXY == 0.);
  }

  SECTION("replaces the existing matrix rather than combining with it")
  {
    IMatrix existing {9., 8., 7., 6., 5., 4.};
    existing.Transform(before, after);
    CheckSame(existing, m);
  }

  SECTION("mapping a rect to itself is the identity")
  {
    IMatrix same;
    same.Transform(before, before);
    CheckIdentity(same);
  }

  SECTION("a translation only")
  {
    IMatrix translation;
    translation.Transform(before, before.GetTranslated(5.f, -7.f));
    CheckMatrix(translation, 1., 0., 0., 1., 5., -7.);
  }
}

TEST_CASE("IMatrix: Invert", "[unit][imatrix]")
{
  SECTION("returns a reference to the inverted matrix")
  {
    IMatrix m {2., 0., 0., 4., 0., 0.};
    IMatrix& result = m.Invert();
    REQUIRE(&result == &m);
  }

  SECTION("the identity is its own inverse")
  {
    CheckIdentity(IMatrix().Invert());
  }

  SECTION("translation")
  {
    CheckMatrix(IMatrix().Translate(10.f, -20.f).Invert(), 1., 0., 0., 1., -10., 20.);
  }

  SECTION("scale")
  {
    CheckMatrix(IMatrix().Scale(2.f, 4.f).Invert(), 0.5, 0., 0., 0.25, 0., 0.);
  }

  SECTION("rotation")
  {
    CheckSame(IMatrix().Rotate(30.f).Invert(), IMatrix().Rotate(-30.f), kFloatMargin);
  }

  SECTION("inverting twice gives the original")
  {
    const IMatrix original {1., 2., 3., 4., 5., 6.};
    IMatrix m = original;
    m.Invert().Invert();
    CheckSame(m, original);
  }

  SECTION("a matrix combined with its inverse is the identity, in either order")
  {
    const IMatrix matrices[] = {
      IMatrix(1., 2., 3., 4., 5., 6.),
      IMatrix(-2., 0.5, 1.5, 3., -4., 7.),
      IMatrix(0.01, 0., 0., 100., 1000., -1000.),
      IMatrix().Translate(3.f, 4.f).Rotate(33.f).Scale(2.f, 0.5f).Skew(10.f, -20.f),
      IMatrix().Scale(-1.f, 1.f).Rotate(270.f),
    };

    for (const IMatrix& original : matrices)
    {
      IMatrix inverse = original;
      inverse.Invert();

      IMatrix a = original;
      a.Transform(inverse);
      CheckIdentity(a, 1e-9);

      IMatrix b = inverse;
      b.Transform(original);
      CheckIdentity(b, 1e-9);
    }
  }

  SECTION("the inverse maps transformed points back")
  {
    const IMatrix m = IMatrix().Translate(30.f, -4.f).Rotate(33.f).Scale(2.f, 0.5f);
    IMatrix inverse = m;
    inverse.Invert();

    for (double x : {-3., 0., 1.5, 100.})
    {
      for (double y : {-7., 0., 2.25, 50.})
      {
        const Point transformed = Apply(m, x, y);
        const Point back = Apply(inverse, transformed.x, transformed.y);
        INFO("x " << x << " y " << y);
        CHECK(back.x == Approx(x).margin(1e-9));
        CHECK(back.y == Approx(y).margin(1e-9));
      }
    }
  }
}
