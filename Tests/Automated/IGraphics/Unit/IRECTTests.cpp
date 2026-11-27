/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for IRECT, the rectangle type that IGraphics layout is built on.

#include <cstdlib>
#include <vector>

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
// L 10, T 20, R 110, B 70: W 100, H 50. Every value used with it below is exactly representable.
const IRECT kRect {10.f, 20.f, 110.f, 70.f};

/** Checks that the cells cover parent exactly: the areas sum to the parent's, no two cells overlap, all are inside and the union is the parent */
void CheckTiles(const IRECT& parent, const std::vector<IRECT>& cells)
{
  float area = 0.f;
  IRECT bounds;

  for (size_t i = 0; i < cells.size(); i++)
  {
    INFO("cell " << i);
    CHECK(cells[i].L >= parent.L - 1e-3f);
    CHECK(cells[i].T >= parent.T - 1e-3f);
    CHECK(cells[i].R <= parent.R + 1e-3f);
    CHECK(cells[i].B <= parent.B + 1e-3f);
    CHECK(cells[i].W() > 0.f);
    CHECK(cells[i].H() > 0.f);

    area += cells[i].Area();
    bounds = bounds.Union(cells[i]);

    for (size_t j = i + 1; j < cells.size(); j++)
    {
      INFO("against cell " << j);
      // allow for slivers caused by float rounding at shared edges
      CHECK(OverlapArea(cells[i], cells[j]) <= 1e-4f * parent.Area());
    }
  }

  CHECK(area == Approx(parent.Area()).epsilon(1e-4));
  CHECK(NearRect(bounds, parent));
}
} // namespace

TEST_CASE("IRECT: default construction is empty", "[unit][irect]")
{
  IRECT r;
  REQUIRE(r.L == 0.f);
  REQUIRE(r.T == 0.f);
  REQUIRE(r.R == 0.f);
  REQUIRE(r.B == 0.f);
  REQUIRE(r.Empty());
  REQUIRE(r.W() == 0.f);
  REQUIRE(r.H() == 0.f);
  REQUIRE(r.Area() == 0.f);
}

TEST_CASE("IRECT: construction from edges", "[unit][irect]")
{
  IRECT r {1.f, 2.f, 3.f, 5.f};
  REQUIRE(r.L == 1.f);
  REQUIRE(r.T == 2.f);
  REQUIRE(r.R == 3.f);
  REQUIRE(r.B == 5.f);
  REQUIRE_FALSE(r.Empty());
}

TEST_CASE("IRECT: construction from a position and a bitmap uses the frame size", "[unit][irect]")
{
  APIBitmap apiBitmap {0, 100, 60, 1.f, 1.f};

  SECTION("single frame")
  {
    IBitmap bitmap {&apiBitmap, 1, false};
    REQUIRE(IRECT(5.f, 7.f, bitmap) == IRECT(5.f, 7.f, 105.f, 67.f));
  }

  SECTION("vertically stacked frames")
  {
    IBitmap bitmap {&apiBitmap, 4, false};
    REQUIRE(IRECT(5.f, 7.f, bitmap) == IRECT(5.f, 7.f, 105.f, 22.f));
  }

  SECTION("horizontally stacked frames")
  {
    IBitmap bitmap {&apiBitmap, 4, true};
    REQUIRE(IRECT(5.f, 7.f, bitmap) == IRECT(5.f, 7.f, 30.f, 67.f));
  }

  SECTION("a high DPI bitmap is measured in points, not pixels")
  {
    APIBitmap apiBitmap2x {0, 200, 120, 2.f, 1.f};
    IBitmap bitmap {&apiBitmap2x, 1, false};
    REQUIRE(IRECT(0.f, 0.f, bitmap) == IRECT(0.f, 0.f, 100.f, 60.f));
  }
}

TEST_CASE("IRECT: MakeXYWH", "[unit][irect]")
{
  const IRECT r = IRECT::MakeXYWH(10.f, 20.f, 100.f, 50.f);
  REQUIRE(r == kRect);
  REQUIRE(r.W() == 100.f);
  REQUIRE(r.H() == 50.f);

  REQUIRE(IRECT::MakeXYWH(-5.f, -6.f, 2.f, 3.f) == IRECT(-5.f, -6.f, -3.f, -3.f));
  REQUIRE(IRECT::MakeXYWH(4.f, 5.f, 0.f, 0.f) == IRECT(4.f, 5.f, 4.f, 5.f));
}

TEST_CASE("IRECT: MakeMidXYWH", "[unit][irect]")
{
  const IRECT r = IRECT::MakeMidXYWH(60.f, 45.f, 100.f, 50.f);
  REQUIRE(r == kRect);
  REQUIRE(r.MW() == 60.f);
  REQUIRE(r.MH() == 45.f);

  const IRECT odd = IRECT::MakeMidXYWH(0.f, 0.f, 5.f, 3.f);
  REQUIRE(odd == IRECT(-2.5f, -1.5f, 2.5f, 1.5f));
}

TEST_CASE("IRECT: W, H, MW, MH and Area", "[unit][irect]")
{
  REQUIRE(kRect.W() == 100.f);
  REQUIRE(kRect.H() == 50.f);
  REQUIRE(kRect.MW() == 60.f);
  REQUIRE(kRect.MH() == 45.f);
  REQUIRE(kRect.Area() == 5000.f);

  const IRECT negative {-30.f, -20.f, -10.f, -5.f};
  REQUIRE(negative.W() == 20.f);
  REQUIRE(negative.H() == 15.f);
  REQUIRE(negative.MW() == -20.f);
  REQUIRE(negative.MH() == -12.5f);
  REQUIRE(negative.Area() == 300.f);

  const IRECT fractional {0.25f, 0.5f, 0.75f, 2.f};
  REQUIRE(fractional.W() == 0.5f);
  REQUIRE(fractional.H() == 1.5f);
  REQUIRE(fractional.MW() == 0.5f);
  REQUIRE(fractional.MH() == 1.25f);
  REQUIRE(fractional.Area() == 0.75f);
}

TEST_CASE("IRECT: Empty is true only when every edge is zero", "[unit][irect]")
{
  REQUIRE(IRECT().Empty());
  REQUIRE(IRECT(0.f, 0.f, 0.f, 0.f).Empty());
  REQUIRE_FALSE(IRECT(1.f, 0.f, 0.f, 0.f).Empty());
  REQUIRE_FALSE(IRECT(0.f, 1.f, 0.f, 0.f).Empty());
  REQUIRE_FALSE(IRECT(0.f, 0.f, 1.f, 0.f).Empty());
  REQUIRE_FALSE(IRECT(0.f, 0.f, 0.f, 1.f).Empty());

  // A zero-sized rect away from the origin has a position, so is not "empty"
  REQUIRE_FALSE(IRECT(5.f, 5.f, 5.f, 5.f).Empty());
}

TEST_CASE("IRECT: Clear", "[unit][irect]")
{
  IRECT r = kRect;
  r.Clear();
  REQUIRE(r.Empty());
  REQUIRE(r == IRECT());
}

TEST_CASE("IRECT: equality operators", "[unit][irect]")
{
  REQUIRE(kRect == IRECT(10.f, 20.f, 110.f, 70.f));
  REQUIRE_FALSE(kRect != IRECT(10.f, 20.f, 110.f, 70.f));

  REQUIRE(kRect != IRECT(11.f, 20.f, 110.f, 70.f));
  REQUIRE(kRect != IRECT(10.f, 21.f, 110.f, 70.f));
  REQUIRE(kRect != IRECT(10.f, 20.f, 111.f, 70.f));
  REQUIRE(kRect != IRECT(10.f, 20.f, 110.f, 71.f));
  REQUIRE_FALSE(kRect == IRECT(10.f, 20.f, 110.f, 71.f));

  REQUIRE(IRECT() == IRECT(0.f, 0.f, 0.f, 0.f));
}

TEST_CASE("IRECT: Union", "[unit][irect]")
{
  const IRECT a {0.f, 0.f, 10.f, 10.f};
  const IRECT b {5.f, 5.f, 20.f, 30.f};
  const IRECT c {-40.f, 50.f, -30.f, 60.f};

  SECTION("overlapping")
  {
    REQUIRE(a.Union(b) == IRECT(0.f, 0.f, 20.f, 30.f));
  }

  SECTION("disjoint")
  {
    REQUIRE(a.Union(c) == IRECT(-40.f, 0.f, 10.f, 60.f));
  }

  SECTION("is commutative")
  {
    REQUIRE(a.Union(b) == b.Union(a));
    REQUIRE(a.Union(c) == c.Union(a));
    REQUIRE(b.Union(c) == c.Union(b));
  }

  SECTION("is associative")
  {
    REQUIRE(a.Union(b).Union(c) == a.Union(b.Union(c)));
  }

  SECTION("with itself")
  {
    REQUIRE(a.Union(a) == a);
  }

  SECTION("containing rect wins")
  {
    const IRECT inner {2.f, 2.f, 4.f, 4.f};
    REQUIRE(a.Union(inner) == a);
    REQUIRE(inner.Union(a) == a);
  }

  SECTION("an empty rect is ignored rather than extending the result to the origin")
  {
    REQUIRE(b.Union(IRECT()) == b);
    REQUIRE(IRECT().Union(b) == b);
    REQUIRE(c.Union(IRECT()) == c);
    REQUIRE(IRECT().Union(IRECT()).Empty());
  }

  SECTION("the result contains both inputs")
  {
    const IRECT u = b.Union(c);
    REQUIRE(u.Contains(b));
    REQUIRE(u.Contains(c));
  }
}

TEST_CASE("IRECT: Intersect", "[unit][irect]")
{
  const IRECT a {0.f, 0.f, 10.f, 10.f};
  const IRECT b {5.f, 5.f, 20.f, 30.f};
  const IRECT c {40.f, 50.f, 60.f, 70.f};

  SECTION("overlapping")
  {
    REQUIRE(a.Intersect(b) == IRECT(5.f, 5.f, 10.f, 10.f));
    REQUIRE(b.Intersect(a) == IRECT(5.f, 5.f, 10.f, 10.f));
  }

  SECTION("disjoint rects give an empty rect")
  {
    REQUIRE(a.Intersect(c).Empty());
    REQUIRE(c.Intersect(a).Empty());
  }

  SECTION("with itself")
  {
    REQUIRE(a.Intersect(a) == a);
  }

  SECTION("contained rect wins")
  {
    const IRECT inner {2.f, 3.f, 4.f, 5.f};
    REQUIRE(a.Intersect(inner) == inner);
    REQUIRE(inner.Intersect(a) == inner);
  }

  SECTION("with an empty rect")
  {
    REQUIRE(a.Intersect(IRECT()).Empty());
    REQUIRE(IRECT().Intersect(a).Empty());
  }

  SECTION("the result is contained by both inputs")
  {
    const IRECT i = a.Intersect(b);
    REQUIRE(a.Contains(i));
    REQUIRE(b.Contains(i));
  }

  SECTION("a cross shaped overlap")
  {
    const IRECT wide {0.f, 40.f, 100.f, 60.f};
    const IRECT tall {40.f, 0.f, 60.f, 100.f};
    REQUIRE(wide.Intersect(tall) == IRECT(40.f, 40.f, 60.f, 60.f));
    REQUIRE(tall.Intersect(wide) == IRECT(40.f, 40.f, 60.f, 60.f));
  }
}

TEST_CASE("IRECT: Intersects", "[unit][irect]")
{
  const IRECT a {0.f, 0.f, 10.f, 10.f};

  SECTION("overlapping rects")
  {
    const IRECT b {5.f, 5.f, 20.f, 30.f};
    REQUIRE(a.Intersects(b));
    REQUIRE(b.Intersects(a));
  }

  SECTION("a rect intersects itself")
  {
    REQUIRE(a.Intersects(a));
  }

  SECTION("contained rects")
  {
    const IRECT inner {2.f, 2.f, 4.f, 4.f};
    REQUIRE(a.Intersects(inner));
    REQUIRE(inner.Intersects(a));
  }

  SECTION("rects separated on each side")
  {
    const IRECT right {11.f, 0.f, 20.f, 10.f};
    const IRECT left {-20.f, 0.f, -1.f, 10.f};
    const IRECT below {0.f, 11.f, 10.f, 20.f};
    const IRECT above {0.f, -20.f, 10.f, -1.f};
    const IRECT diagonal {11.f, 11.f, 20.f, 20.f};

    for (const IRECT& other : {right, left, below, above, diagonal})
    {
      INFO(Catch::StringMaker<IRECT>::convert(other));
      REQUIRE_FALSE(a.Intersects(other));
      REQUIRE_FALSE(other.Intersects(a));
    }
  }

  SECTION("an empty rect intersects nothing")
  {
    REQUIRE_FALSE(a.Intersects(IRECT()));
    REQUIRE_FALSE(IRECT().Intersects(a));
    REQUIRE_FALSE(IRECT().Intersects(IRECT()));
  }

  SECTION("overlap on one axis only is not an intersection")
  {
    const IRECT sameRowFarRight {50.f, 2.f, 60.f, 8.f};
    const IRECT sameColumnFarBelow {2.f, 50.f, 8.f, 60.f};
    REQUIRE_FALSE(a.Intersects(sameRowFarRight));
    REQUIRE_FALSE(sameRowFarRight.Intersects(a));
    REQUIRE_FALSE(a.Intersects(sameColumnFarBelow));
    REQUIRE_FALSE(sameColumnFarBelow.Intersects(a));
  }
}

// BUG: IGraphicsStructs.h:868 IRECT::Intersects() tests "R >= rhs.L && L < rhs.R" (and the same vertically), so
// it is not symmetric: for two rects that only share an edge, the left/upper one reports an intersection and
// the right/lower one does not. The doc comment says it returns true if the rects share any common pixels,
// which rects that merely touch do not (Contains(x, y) treats the right and bottom edges as exclusive).
TEST_CASE("IRECT: Intersects is symmetric for rects that share an edge", "[.][bug]")
{
  const IRECT a {0.f, 0.f, 10.f, 10.f};
  const IRECT right {10.f, 0.f, 20.f, 10.f};
  const IRECT below {0.f, 10.f, 10.f, 20.f};

  CHECK(a.Intersects(right) == right.Intersects(a));
  CHECK(a.Intersects(below) == below.Intersects(a));
  CHECK_FALSE(a.Intersects(right));
  CHECK_FALSE(a.Intersects(below));
  CHECK(a.Intersect(right).Empty());
  CHECK(a.Intersect(below).Empty());
}

TEST_CASE("IRECT: Contains another rect", "[unit][irect]")
{
  const IRECT a {0.f, 0.f, 10.f, 10.f};

  REQUIRE(a.Contains(IRECT(2.f, 2.f, 8.f, 8.f)));
  REQUIRE(a.Contains(a));
  REQUIRE(a.Contains(IRECT(0.f, 0.f, 10.f, 5.f)));
  REQUIRE(a.Contains(IRECT(5.f, 5.f, 10.f, 10.f)));

  REQUIRE_FALSE(a.Contains(IRECT(-1.f, 2.f, 8.f, 8.f)));
  REQUIRE_FALSE(a.Contains(IRECT(2.f, -1.f, 8.f, 8.f)));
  REQUIRE_FALSE(a.Contains(IRECT(2.f, 2.f, 11.f, 8.f)));
  REQUIRE_FALSE(a.Contains(IRECT(2.f, 2.f, 8.f, 11.f)));
  REQUIRE_FALSE(a.Contains(IRECT(20.f, 20.f, 30.f, 30.f)));

  // the smaller rect never contains the larger
  REQUIRE_FALSE(IRECT(2.f, 2.f, 8.f, 8.f).Contains(a));

  // empty rects neither contain nor are contained
  REQUIRE_FALSE(a.Contains(IRECT()));
  REQUIRE_FALSE(IRECT().Contains(a));
  REQUIRE_FALSE(IRECT().Contains(IRECT()));
}

TEST_CASE("IRECT: Contains a point treats the right and bottom edges as outside", "[unit][irect]")
{
  REQUIRE(kRect.Contains(60.f, 45.f));
  REQUIRE(kRect.Contains(10.f, 20.f));
  REQUIRE(kRect.Contains(109.5f, 69.5f));
  REQUIRE(kRect.Contains(10.f, 69.5f));
  REQUIRE(kRect.Contains(109.5f, 20.f));

  REQUIRE_FALSE(kRect.Contains(110.f, 45.f));
  REQUIRE_FALSE(kRect.Contains(60.f, 70.f));
  REQUIRE_FALSE(kRect.Contains(110.f, 70.f));
  REQUIRE_FALSE(kRect.Contains(9.5f, 45.f));
  REQUIRE_FALSE(kRect.Contains(60.f, 19.5f));
  REQUIRE_FALSE(kRect.Contains(200.f, 45.f));
  REQUIRE_FALSE(kRect.Contains(60.f, 200.f));
  REQUIRE_FALSE(kRect.Contains(-60.f, -45.f));

  REQUIRE_FALSE(IRECT().Contains(0.f, 0.f));
}

TEST_CASE("IRECT: ContainsEdge includes the right and bottom edges", "[unit][irect]")
{
  REQUIRE(kRect.ContainsEdge(60.f, 45.f));
  REQUIRE(kRect.ContainsEdge(10.f, 20.f));
  REQUIRE(kRect.ContainsEdge(110.f, 45.f));
  REQUIRE(kRect.ContainsEdge(60.f, 70.f));
  REQUIRE(kRect.ContainsEdge(110.f, 70.f));

  REQUIRE_FALSE(kRect.ContainsEdge(110.5f, 45.f));
  REQUIRE_FALSE(kRect.ContainsEdge(60.f, 70.5f));
  REQUIRE_FALSE(kRect.ContainsEdge(9.5f, 45.f));
  REQUIRE_FALSE(kRect.ContainsEdge(60.f, 19.5f));

  REQUIRE_FALSE(IRECT().ContainsEdge(0.f, 0.f));
}

TEST_CASE("IRECT: Constrain moves a point to the nearest position in the rect", "[unit][irect]")
{
  auto constrained = [](float x, float y) {
    kRect.Constrain(x, y);
    return IVec2(x, y);
  };

  SECTION("a point inside is not moved")
  {
    const IVec2 p = constrained(60.f, 45.f);
    REQUIRE(p.x == 60.f);
    REQUIRE(p.y == 45.f);
  }

  SECTION("points on the edges are not moved")
  {
    IVec2 p = constrained(10.f, 20.f);
    REQUIRE(p.x == 10.f);
    REQUIRE(p.y == 20.f);
    p = constrained(110.f, 70.f);
    REQUIRE(p.x == 110.f);
    REQUIRE(p.y == 70.f);
  }

  SECTION("outside on one axis")
  {
    IVec2 p = constrained(0.f, 45.f);
    REQUIRE(p.x == 10.f);
    REQUIRE(p.y == 45.f);
    p = constrained(500.f, 45.f);
    REQUIRE(p.x == 110.f);
    REQUIRE(p.y == 45.f);
    p = constrained(60.f, -3.f);
    REQUIRE(p.x == 60.f);
    REQUIRE(p.y == 20.f);
    p = constrained(60.f, 71.f);
    REQUIRE(p.x == 60.f);
    REQUIRE(p.y == 70.f);
  }

  SECTION("outside on both axes goes to the corner")
  {
    IVec2 p = constrained(-100.f, -100.f);
    REQUIRE(p.x == 10.f);
    REQUIRE(p.y == 20.f);
    p = constrained(1000.f, 1000.f);
    REQUIRE(p.x == 110.f);
    REQUIRE(p.y == 70.f);
    p = constrained(-100.f, 1000.f);
    REQUIRE(p.x == 10.f);
    REQUIRE(p.y == 70.f);
  }

  SECTION("the result is always on or inside the rect")
  {
    for (float x = -50.f; x <= 200.f; x += 12.5f)
    {
      for (float y = -50.f; y <= 200.f; y += 12.5f)
      {
        const IVec2 p = constrained(x, y);
        REQUIRE(kRect.ContainsEdge(p.x, p.y));
      }
    }
  }
}

TEST_CASE("IRECT: Inset offsets a rect by this rect's top left", "[unit][irect]")
{
  REQUIRE(kRect.Inset(IRECT(1.f, 2.f, 30.f, 40.f)) == IRECT(11.f, 22.f, 40.f, 60.f));
  REQUIRE(kRect.Inset(IRECT()) == IRECT(10.f, 20.f, 10.f, 20.f));
  REQUIRE(IRECT().Inset(kRect) == kRect);

  // the size of the argument is kept
  const IRECT local {3.f, 4.f, 13.f, 9.f};
  REQUIRE(kRect.Inset(local).W() == local.W());
  REQUIRE(kRect.Inset(local).H() == local.H());
}

TEST_CASE("IRECT: Mergeable is true when two rects exactly cover their union", "[unit][irect]")
{
  const IRECT a {0.f, 0.f, 10.f, 10.f};

  SECTION("side by side with matching heights")
  {
    const IRECT b {10.f, 0.f, 20.f, 10.f};
    REQUIRE(a.Mergeable(b));
    REQUIRE(b.Mergeable(a));
  }

  SECTION("stacked with matching widths")
  {
    const IRECT b {0.f, 10.f, 10.f, 30.f};
    REQUIRE(a.Mergeable(b));
    REQUIRE(b.Mergeable(a));
  }

  SECTION("overlapping along one axis with the other axis matching")
  {
    const IRECT b {5.f, 0.f, 20.f, 10.f};
    REQUIRE(a.Mergeable(b));
    REQUIRE(b.Mergeable(a));
    const IRECT c {0.f, 5.f, 10.f, 30.f};
    REQUIRE(a.Mergeable(c));
    REQUIRE(c.Mergeable(a));
  }

  SECTION("identical rects")
  {
    REQUIRE(a.Mergeable(a));
  }

  SECTION("anything merges with an empty rect")
  {
    REQUIRE(a.Mergeable(IRECT()));
    REQUIRE(IRECT().Mergeable(a));
  }

  SECTION("a gap between the rects")
  {
    const IRECT b {11.f, 0.f, 20.f, 10.f};
    REQUIRE_FALSE(a.Mergeable(b));
    REQUIRE_FALSE(b.Mergeable(a));
    const IRECT c {0.f, 11.f, 10.f, 20.f};
    REQUIRE_FALSE(a.Mergeable(c));
    REQUIRE_FALSE(c.Mergeable(a));
  }

  SECTION("differing extents on the shared axis")
  {
    const IRECT b {10.f, 0.f, 20.f, 9.f};
    REQUIRE_FALSE(a.Mergeable(b));
    REQUIRE_FALSE(b.Mergeable(a));
    const IRECT c {1.f, 10.f, 10.f, 20.f};
    REQUIRE_FALSE(a.Mergeable(c));
    REQUIRE_FALSE(c.Mergeable(a));
  }

  SECTION("corner overlap")
  {
    const IRECT b {5.f, 5.f, 15.f, 15.f};
    REQUIRE_FALSE(a.Mergeable(b));
    REQUIRE_FALSE(b.Mergeable(a));
  }

  SECTION("when mergeable, the union has exactly the combined area less the overlap")
  {
    const IRECT b {5.f, 0.f, 20.f, 10.f};
    REQUIRE(a.Mergeable(b));
    REQUIRE(a.Union(b).Area() == a.Area() + b.Area() - OverlapArea(a, b));
  }
}

TEST_CASE("IRECT: FracRectHorizontal", "[unit][irect]")
{
  REQUIRE(kRect.FracRectHorizontal(0.25f) == IRECT(10.f, 20.f, 35.f, 70.f));
  REQUIRE(kRect.FracRectHorizontal(0.25f, false) == IRECT(10.f, 20.f, 35.f, 70.f));
  REQUIRE(kRect.FracRectHorizontal(0.25f, true) == IRECT(85.f, 20.f, 110.f, 70.f));
  REQUIRE(kRect.FracRectHorizontal(1.f) == kRect);
  REQUIRE(kRect.FracRectHorizontal(1.f, true) == kRect);
  REQUIRE(kRect.FracRectHorizontal(0.f).W() == 0.f);
  REQUIRE(kRect.FracRectHorizontal(0.f).L == kRect.L);
  REQUIRE(kRect.FracRectHorizontal(0.f, true).R == kRect.R);

  // fractions over 1 expand beyond the rect
  REQUIRE(kRect.FracRectHorizontal(2.f) == IRECT(10.f, 20.f, 210.f, 70.f));
  REQUIRE(kRect.FracRectHorizontal(2.f, true) == IRECT(-90.f, 20.f, 110.f, 70.f));
}

TEST_CASE("IRECT: FracRectVertical", "[unit][irect]")
{
  // N.B. the default is to come from the bottom
  REQUIRE(kRect.FracRectVertical(0.5f) == IRECT(10.f, 45.f, 110.f, 70.f));
  REQUIRE(kRect.FracRectVertical(0.5f, false) == IRECT(10.f, 45.f, 110.f, 70.f));
  REQUIRE(kRect.FracRectVertical(0.5f, true) == IRECT(10.f, 20.f, 110.f, 45.f));
  REQUIRE(kRect.FracRectVertical(0.25f, true) == IRECT(10.f, 20.f, 110.f, 32.5f));
  REQUIRE(kRect.FracRectVertical(0.25f, false) == IRECT(10.f, 57.5f, 110.f, 70.f));
  REQUIRE(kRect.FracRectVertical(1.f) == kRect);
  REQUIRE(kRect.FracRectVertical(1.f, true) == kRect);
  REQUIRE(kRect.FracRectVertical(2.f, true) == IRECT(10.f, 20.f, 110.f, 120.f));
  REQUIRE(kRect.FracRectVertical(2.f, false) == IRECT(10.f, -30.f, 110.f, 70.f));
}

TEST_CASE("IRECT: FracRect dispatches on direction", "[unit][irect]")
{
  for (float frac : {0.f, 0.125f, 0.5f, 0.75f, 1.f})
  {
    for (bool fromTopOrRight : {false, true})
    {
      INFO("frac " << frac << " fromTopOrRight " << fromTopOrRight);
      REQUIRE(kRect.FracRect(EDirection::Vertical, frac, fromTopOrRight) == kRect.FracRectVertical(frac, fromTopOrRight));
      REQUIRE(kRect.FracRect(EDirection::Horizontal, frac, fromTopOrRight) == kRect.FracRectHorizontal(frac, fromTopOrRight));
    }
  }

  // defaults: from the bottom/left
  REQUIRE(kRect.FracRect(EDirection::Vertical, 0.5f) == IRECT(10.f, 45.f, 110.f, 70.f));
  REQUIRE(kRect.FracRect(EDirection::Horizontal, 0.5f) == IRECT(10.f, 20.f, 60.f, 70.f));
}

TEST_CASE("IRECT: FracRect and its complement partition the rect", "[unit][irect]")
{
  for (float frac : {0.125f, 0.25f, 0.5f, 0.75f})
  {
    INFO("frac " << frac);
    CheckTiles(kRect, {kRect.FracRectHorizontal(frac), kRect.FracRectHorizontal(1.f - frac, true)});
    CheckTiles(kRect, {kRect.FracRectVertical(frac, true), kRect.FracRectVertical(1.f - frac, false)});
  }
}

TEST_CASE("IRECT: SubRectVertical", "[unit][irect]")
{
  REQUIRE(kRect.SubRectVertical(2, 0) == IRECT(10.f, 20.f, 110.f, 45.f));
  REQUIRE(kRect.SubRectVertical(2, 1) == IRECT(10.f, 45.f, 110.f, 70.f));
  REQUIRE(kRect.SubRectVertical(5, 0) == IRECT(10.f, 20.f, 110.f, 30.f));
  REQUIRE(kRect.SubRectVertical(5, 3) == IRECT(10.f, 50.f, 110.f, 60.f));
  REQUIRE(kRect.SubRectVertical(5, 4) == IRECT(10.f, 60.f, 110.f, 70.f));
  REQUIRE(kRect.SubRectVertical(1, 0) == kRect);
}

TEST_CASE("IRECT: SubRectHorizontal", "[unit][irect]")
{
  REQUIRE(kRect.SubRectHorizontal(2, 0) == IRECT(10.f, 20.f, 60.f, 70.f));
  REQUIRE(kRect.SubRectHorizontal(2, 1) == IRECT(60.f, 20.f, 110.f, 70.f));
  REQUIRE(kRect.SubRectHorizontal(4, 0) == IRECT(10.f, 20.f, 35.f, 70.f));
  REQUIRE(kRect.SubRectHorizontal(4, 2) == IRECT(60.f, 20.f, 85.f, 70.f));
  REQUIRE(kRect.SubRectHorizontal(4, 3) == IRECT(85.f, 20.f, 110.f, 70.f));
  REQUIRE(kRect.SubRectHorizontal(1, 0) == kRect);
}

TEST_CASE("IRECT: SubRect dispatches on direction", "[unit][irect]")
{
  for (int n = 1; n <= 6; n++)
  {
    for (int i = 0; i < n; i++)
    {
      INFO("slice " << i << " of " << n);
      REQUIRE(kRect.SubRect(EDirection::Vertical, n, i) == kRect.SubRectVertical(n, i));
      REQUIRE(kRect.SubRect(EDirection::Horizontal, n, i) == kRect.SubRectHorizontal(n, i));
    }
  }
}

TEST_CASE("IRECT: SubRect slices are equal sized and tile the rect", "[unit][irect]")
{
  const IRECT parents[] = {kRect, IRECT(-33.f, -7.f, 14.f, 121.f), IRECT(0.5f, 0.25f, 300.75f, 17.5f)};

  for (const IRECT& parent : parents)
  {
    for (int n = 1; n <= 9; n++)
    {
      INFO(Catch::StringMaker<IRECT>::convert(parent) << " n " << n);
      std::vector<IRECT> hSlices, vSlices;

      for (int i = 0; i < n; i++)
      {
        hSlices.push_back(parent.SubRectHorizontal(n, i));
        vSlices.push_back(parent.SubRectVertical(n, i));

        CHECK(hSlices.back().W() == Approx(parent.W() / static_cast<float>(n)).margin(1e-3));
        CHECK(hSlices.back().T == parent.T);
        CHECK(hSlices.back().B == parent.B);
        CHECK(vSlices.back().H() == Approx(parent.H() / static_cast<float>(n)).margin(1e-3));
        CHECK(vSlices.back().L == parent.L);
        CHECK(vSlices.back().R == parent.R);

        if (i > 0)
        {
          // each slice starts where the previous one ended
          CHECK(hSlices[i].L == Approx(hSlices[i - 1].R).margin(1e-3));
          CHECK(vSlices[i].T == Approx(vSlices[i - 1].B).margin(1e-3));
        }
      }

      CheckTiles(parent, hSlices);
      CheckTiles(parent, vSlices);
    }
  }
}

TEST_CASE("IRECT: corner helpers", "[unit][irect]")
{
  REQUIRE(kRect.GetFromTLHC(30.f, 10.f) == IRECT(10.f, 20.f, 40.f, 30.f));
  REQUIRE(kRect.GetFromBLHC(30.f, 10.f) == IRECT(10.f, 60.f, 40.f, 70.f));
  REQUIRE(kRect.GetFromTRHC(30.f, 10.f) == IRECT(80.f, 20.f, 110.f, 30.f));
  REQUIRE(kRect.GetFromBRHC(30.f, 10.f) == IRECT(80.f, 60.f, 110.f, 70.f));

  // all four have the requested size
  for (const IRECT& r : {kRect.GetFromTLHC(30.f, 10.f), kRect.GetFromBLHC(30.f, 10.f), kRect.GetFromTRHC(30.f, 10.f), kRect.GetFromBRHC(30.f, 10.f)})
  {
    REQUIRE(r.W() == 30.f);
    REQUIRE(r.H() == 10.f);
    REQUIRE(kRect.Contains(r));
  }

  // full size gives the rect back
  REQUIRE(kRect.GetFromTLHC(100.f, 50.f) == kRect);
  REQUIRE(kRect.GetFromBLHC(100.f, 50.f) == kRect);
  REQUIRE(kRect.GetFromTRHC(100.f, 50.f) == kRect);
  REQUIRE(kRect.GetFromBRHC(100.f, 50.f) == kRect);

  // half sized corners tile the rect
  CheckTiles(kRect, {kRect.GetFromTLHC(50.f, 25.f), kRect.GetFromTRHC(50.f, 25.f), kRect.GetFromBLHC(50.f, 25.f), kRect.GetFromBRHC(50.f, 25.f)});
}

TEST_CASE("IRECT: GetFromTop, GetFromBottom, GetFromLeft and GetFromRight", "[unit][irect]")
{
  REQUIRE(kRect.GetFromTop(10.f) == IRECT(10.f, 20.f, 110.f, 30.f));
  REQUIRE(kRect.GetFromBottom(10.f) == IRECT(10.f, 60.f, 110.f, 70.f));
  REQUIRE(kRect.GetFromLeft(10.f) == IRECT(10.f, 20.f, 20.f, 70.f));
  REQUIRE(kRect.GetFromRight(10.f) == IRECT(100.f, 20.f, 110.f, 70.f));

  REQUIRE(kRect.GetFromTop(10.f).H() == 10.f);
  REQUIRE(kRect.GetFromBottom(10.f).H() == 10.f);
  REQUIRE(kRect.GetFromLeft(10.f).W() == 10.f);
  REQUIRE(kRect.GetFromRight(10.f).W() == 10.f);

  REQUIRE(kRect.GetFromTop(50.f) == kRect);
  REQUIRE(kRect.GetFromBottom(50.f) == kRect);
  REQUIRE(kRect.GetFromLeft(100.f) == kRect);
  REQUIRE(kRect.GetFromRight(100.f) == kRect);

  // amounts larger than the rect extend past the opposite edge
  REQUIRE(kRect.GetFromTop(80.f) == IRECT(10.f, 20.f, 110.f, 100.f));
  REQUIRE(kRect.GetFromRight(120.f) == IRECT(-10.f, 20.f, 110.f, 70.f));
}

TEST_CASE("IRECT: GetReducedFrom helpers", "[unit][irect]")
{
  REQUIRE(kRect.GetReducedFromTop(10.f) == IRECT(10.f, 30.f, 110.f, 70.f));
  REQUIRE(kRect.GetReducedFromBottom(10.f) == IRECT(10.f, 20.f, 110.f, 60.f));
  REQUIRE(kRect.GetReducedFromLeft(10.f) == IRECT(20.f, 20.f, 110.f, 70.f));
  REQUIRE(kRect.GetReducedFromRight(10.f) == IRECT(10.f, 20.f, 100.f, 70.f));

  REQUIRE(kRect.GetReducedFromTop(0.f) == kRect);
  REQUIRE(kRect.GetReducedFromBottom(0.f) == kRect);
  REQUIRE(kRect.GetReducedFromLeft(0.f) == kRect);
  REQUIRE(kRect.GetReducedFromRight(0.f) == kRect);

  // negative amounts grow the rect
  REQUIRE(kRect.GetReducedFromTop(-10.f) == IRECT(10.f, 10.f, 110.f, 70.f));
  REQUIRE(kRect.GetReducedFromRight(-10.f) == IRECT(10.f, 20.f, 120.f, 70.f));
}

TEST_CASE("IRECT: GetFrom and GetReducedFrom partition the rect", "[unit][irect]")
{
  for (float amount : {1.f, 12.5f, 25.f, 49.f})
  {
    INFO("amount " << amount);
    CheckTiles(kRect, {kRect.GetFromTop(amount), kRect.GetReducedFromTop(amount)});
    CheckTiles(kRect, {kRect.GetFromBottom(amount), kRect.GetReducedFromBottom(amount)});
    CheckTiles(kRect, {kRect.GetFromLeft(amount), kRect.GetReducedFromLeft(amount)});
    CheckTiles(kRect, {kRect.GetFromRight(amount), kRect.GetReducedFromRight(amount)});

    // the two parts meet exactly
    CHECK(kRect.GetFromTop(amount).B == kRect.GetReducedFromTop(amount).T);
    CHECK(kRect.GetFromBottom(amount).T == kRect.GetReducedFromBottom(amount).B);
    CHECK(kRect.GetFromLeft(amount).R == kRect.GetReducedFromLeft(amount).L);
    CHECK(kRect.GetFromRight(amount).L == kRect.GetReducedFromRight(amount).R);
  }
}

TEST_CASE("IRECT: ReduceFrom helpers shrink the rect and return the removed part", "[unit][irect]")
{
  SECTION("ReduceFromTop")
  {
    IRECT r = kRect;
    const IRECT removed = r.ReduceFromTop(10.f);
    REQUIRE(removed == kRect.GetFromTop(10.f));
    REQUIRE(r == kRect.GetReducedFromTop(10.f));
    CheckTiles(kRect, {removed, r});
  }

  SECTION("ReduceFromBottom")
  {
    IRECT r = kRect;
    const IRECT removed = r.ReduceFromBottom(10.f);
    REQUIRE(removed == kRect.GetFromBottom(10.f));
    REQUIRE(r == kRect.GetReducedFromBottom(10.f));
    CheckTiles(kRect, {removed, r});
  }

  SECTION("ReduceFromLeft")
  {
    IRECT r = kRect;
    const IRECT removed = r.ReduceFromLeft(10.f);
    REQUIRE(removed == kRect.GetFromLeft(10.f));
    REQUIRE(r == kRect.GetReducedFromLeft(10.f));
    CheckTiles(kRect, {removed, r});
  }

  SECTION("ReduceFromRight")
  {
    IRECT r = kRect;
    const IRECT removed = r.ReduceFromRight(10.f);
    REQUIRE(removed == kRect.GetFromRight(10.f));
    REQUIRE(r == kRect.GetReducedFromRight(10.f));
    CheckTiles(kRect, {removed, r});
  }

  SECTION("repeatedly carving up a rect, as when laying out a UI")
  {
    IRECT r = kRect;
    const IRECT header = r.ReduceFromTop(10.f);
    const IRECT footer = r.ReduceFromBottom(5.f);
    const IRECT sidebar = r.ReduceFromLeft(20.f);
    const IRECT meter = r.ReduceFromRight(8.f);

    REQUIRE(header == IRECT(10.f, 20.f, 110.f, 30.f));
    REQUIRE(footer == IRECT(10.f, 65.f, 110.f, 70.f));
    REQUIRE(sidebar == IRECT(10.f, 30.f, 30.f, 65.f));
    REQUIRE(meter == IRECT(102.f, 30.f, 110.f, 65.f));
    REQUIRE(r == IRECT(30.f, 30.f, 102.f, 65.f));
    CheckTiles(kRect, {header, footer, sidebar, meter, r});
  }
}

TEST_CASE("IRECT: GetGridCell by row and column", "[unit][irect]")
{
  // 2 rows of 25, 4 columns of 25
  REQUIRE(kRect.GetGridCell(0, 0, 2, 4) == IRECT(10.f, 20.f, 35.f, 45.f));
  REQUIRE(kRect.GetGridCell(0, 3, 2, 4) == IRECT(85.f, 20.f, 110.f, 45.f));
  REQUIRE(kRect.GetGridCell(1, 0, 2, 4) == IRECT(10.f, 45.f, 35.f, 70.f));
  REQUIRE(kRect.GetGridCell(1, 2, 2, 4) == IRECT(60.f, 45.f, 85.f, 70.f));
  REQUIRE(kRect.GetGridCell(1, 3, 2, 4) == IRECT(85.f, 45.f, 110.f, 70.f));
  REQUIRE(kRect.GetGridCell(0, 0, 1, 1) == kRect);

  // a single row or column is the same as a slice
  REQUIRE(kRect.GetGridCell(0, 2, 1, 5) == kRect.SubRectHorizontal(5, 2));
  REQUIRE(kRect.GetGridCell(2, 0, 5, 1) == kRect.SubRectVertical(5, 2));
}

TEST_CASE("IRECT: GetGridCell by index, counting along rows", "[unit][irect]")
{
  REQUIRE(kRect.GetGridCell(0, 2, 4) == IRECT(10.f, 20.f, 35.f, 45.f));
  REQUIRE(kRect.GetGridCell(1, 2, 4) == IRECT(35.f, 20.f, 60.f, 45.f));
  REQUIRE(kRect.GetGridCell(3, 2, 4) == IRECT(85.f, 20.f, 110.f, 45.f));
  REQUIRE(kRect.GetGridCell(4, 2, 4) == IRECT(10.f, 45.f, 35.f, 70.f));
  REQUIRE(kRect.GetGridCell(7, 2, 4) == IRECT(85.f, 45.f, 110.f, 70.f));
  REQUIRE(kRect.GetGridCell(4, 2, 4, EDirection::Horizontal) == IRECT(10.f, 45.f, 35.f, 70.f));

  for (int row = 0; row < 3; row++)
  {
    for (int col = 0; col < 5; col++)
    {
      INFO("row " << row << " col " << col);
      REQUIRE(kRect.GetGridCell(row * 5 + col, 3, 5) == kRect.GetGridCell(row, col, 3, 5));
      REQUIRE(kRect.GetGridCell(row * 5 + col, 3, 5, EDirection::Horizontal) == kRect.GetGridCell(row, col, 3, 5));
    }
  }
}

TEST_CASE("IRECT: GetGridCell by index, counting down columns", "[unit][irect]")
{
  REQUIRE(kRect.GetGridCell(0, 2, 4, EDirection::Vertical) == IRECT(10.f, 20.f, 35.f, 45.f));
  REQUIRE(kRect.GetGridCell(1, 2, 4, EDirection::Vertical) == IRECT(10.f, 45.f, 35.f, 70.f));
  REQUIRE(kRect.GetGridCell(2, 2, 4, EDirection::Vertical) == IRECT(35.f, 20.f, 60.f, 45.f));
  REQUIRE(kRect.GetGridCell(7, 2, 4, EDirection::Vertical) == IRECT(85.f, 45.f, 110.f, 70.f));

  for (int row = 0; row < 3; row++)
  {
    for (int col = 0; col < 5; col++)
    {
      INFO("row " << row << " col " << col);
      REQUIRE(NearRect(kRect.GetGridCell(col * 3 + row, 3, 5, EDirection::Vertical), kRect.GetGridCell(row, col, 3, 5)));
    }
  }
}

TEST_CASE("IRECT: GetGridCell can join sequential cells", "[unit][irect]")
{
  SECTION("along a row")
  {
    REQUIRE(kRect.GetGridCell(1, 2, 4, EDirection::Horizontal, 2) == IRECT(35.f, 20.f, 85.f, 45.f));
    REQUIRE(kRect.GetGridCell(0, 2, 4, EDirection::Horizontal, 4) == IRECT(10.f, 20.f, 110.f, 45.f));
    REQUIRE(kRect.GetGridCell(4, 2, 4, EDirection::Horizontal, 3) == IRECT(10.f, 45.f, 85.f, 70.f));
    REQUIRE(kRect.GetGridCell(5, 2, 4, EDirection::Horizontal, 1) == kRect.GetGridCell(5, 2, 4));
  }

  SECTION("joining stops at the end of the row")
  {
    REQUIRE(kRect.GetGridCell(3, 2, 4, EDirection::Horizontal, 3) == IRECT(85.f, 20.f, 110.f, 45.f));
    REQUIRE(kRect.GetGridCell(2, 2, 4, EDirection::Horizontal, 10) == IRECT(60.f, 20.f, 110.f, 45.f));
  }

  SECTION("down a column")
  {
    const IRECT tall {0.f, 0.f, 20.f, 100.f}; // 4 rows of 25, 2 columns of 10
    REQUIRE(tall.GetGridCell(1, 4, 2, EDirection::Vertical, 2) == IRECT(0.f, 25.f, 10.f, 75.f));
    REQUIRE(tall.GetGridCell(0, 4, 2, EDirection::Vertical, 4) == IRECT(0.f, 0.f, 10.f, 100.f));
    REQUIRE(tall.GetGridCell(4, 4, 2, EDirection::Vertical, 3) == IRECT(10.f, 0.f, 20.f, 75.f));
  }

  SECTION("joining stops at the end of the column")
  {
    const IRECT tall {0.f, 0.f, 20.f, 100.f};
    REQUIRE(tall.GetGridCell(3, 4, 2, EDirection::Vertical, 3) == IRECT(0.f, 75.f, 10.f, 100.f));
    REQUIRE(tall.GetGridCell(6, 4, 2, EDirection::Vertical, 10) == IRECT(10.f, 50.f, 20.f, 100.f));
  }
}

TEST_CASE("IRECT: GetGridCell tiles the parent", "[unit][irect]")
{
  const IRECT parents[] = {kRect, IRECT(0.f, 0.f, 120.f, 60.f), IRECT(-17.f, 3.f, 86.f, 44.f), IRECT(0.25f, 0.75f, 640.5f, 480.25f)};

  for (const IRECT& parent : parents)
  {
    for (int nRows = 1; nRows <= 5; nRows++)
    {
      for (int nCols = 1; nCols <= 7; nCols += 2)
      {
        INFO(Catch::StringMaker<IRECT>::convert(parent) << " " << nRows << " x " << nCols);

        std::vector<IRECT> byRowCol, byIndexH, byIndexV;

        for (int row = 0; row < nRows; row++)
        {
          for (int col = 0; col < nCols; col++)
            byRowCol.push_back(parent.GetGridCell(row, col, nRows, nCols));
        }

        for (int i = 0; i < nRows * nCols; i++)
        {
          byIndexH.push_back(parent.GetGridCell(i, nRows, nCols, EDirection::Horizontal));
          byIndexV.push_back(parent.GetGridCell(i, nRows, nCols, EDirection::Vertical));
        }

        CheckTiles(parent, byRowCol);
        CheckTiles(parent, byIndexH);
        CheckTiles(parent, byIndexV);

        // all cells are the same size
        for (const IRECT& cell : byRowCol)
        {
          CHECK(cell.W() == Approx(parent.W() / static_cast<float>(nCols)).margin(1e-3));
          CHECK(cell.H() == Approx(parent.H() / static_cast<float>(nRows)).margin(1e-3));
        }
      }
    }
  }
}

TEST_CASE("IRECT: IsPixelAligned", "[unit][irect]")
{
  REQUIRE(kRect.IsPixelAligned());
  REQUIRE(IRECT().IsPixelAligned());
  REQUIRE(IRECT(-3.f, -2.f, 5.f, 8.f).IsPixelAligned());

  REQUIRE_FALSE(IRECT(10.5f, 20.f, 110.f, 70.f).IsPixelAligned());
  REQUIRE_FALSE(IRECT(10.f, 20.5f, 110.f, 70.f).IsPixelAligned());
  REQUIRE_FALSE(IRECT(10.f, 20.f, 110.5f, 70.f).IsPixelAligned());
  REQUIRE_FALSE(IRECT(10.f, 20.f, 110.f, 70.5f).IsPixelAligned());
  REQUIRE_FALSE(IRECT(10.01f, 20.f, 110.f, 70.f).IsPixelAligned());

  // values within 1/1000th of an integer count as aligned
  REQUIRE(IRECT(10.0005f, 19.9995f, 110.0005f, 69.9995f).IsPixelAligned());
}

TEST_CASE("IRECT: IsPixelAligned at a scale", "[unit][irect]")
{
  const IRECT halves {0.5f, 1.5f, 10.5f, 20.5f};
  REQUIRE_FALSE(halves.IsPixelAligned(1.f));
  REQUIRE(halves.IsPixelAligned(2.f));
  REQUIRE(halves.IsPixelAligned(4.f));

  const IRECT quarters {0.25f, 1.75f, 10.25f, 20.75f};
  REQUIRE_FALSE(quarters.IsPixelAligned(1.f));
  REQUIRE_FALSE(quarters.IsPixelAligned(2.f));
  REQUIRE(quarters.IsPixelAligned(4.f));

  // an odd rect is not aligned when scaled down
  REQUIRE(IRECT(1.f, 1.f, 3.f, 3.f).IsPixelAligned(1.f));
  REQUIRE_FALSE(IRECT(1.f, 1.f, 3.f, 3.f).IsPixelAligned(0.5f));
  REQUIRE(IRECT(2.f, 2.f, 4.f, 4.f).IsPixelAligned(0.5f));

  // the rect itself is not modified
  REQUIRE(halves == IRECT(0.5f, 1.5f, 10.5f, 20.5f));
}

TEST_CASE("IRECT: PixelAlign moves all edges outwards", "[unit][irect]")
{
  IRECT r {10.3f, 20.7f, 109.2f, 69.9f};
  r.PixelAlign();
  REQUIRE(r == IRECT(10.f, 20.f, 110.f, 70.f));
  REQUIRE(r.IsPixelAligned());

  IRECT negative {-10.3f, -20.7f, -1.2f, -0.9f};
  negative.PixelAlign();
  REQUIRE(negative == IRECT(-11.f, -21.f, -1.f, 0.f));

  IRECT aligned = kRect;
  aligned.PixelAlign();
  REQUIRE(aligned == kRect);

  REQUIRE(IRECT(10.3f, 20.7f, 109.2f, 69.9f).GetPixelAligned() == IRECT(10.f, 20.f, 110.f, 70.f));
  REQUIRE(kRect.GetPixelAligned() == kRect);
}

TEST_CASE("IRECT: PixelAlign at a scale", "[unit][irect]")
{
  IRECT r {0.3f, 0.3f, 10.6f, 10.6f};
  r.PixelAlign(2.f);
  REQUIRE(r == IRECT(0.f, 0.f, 11.f, 11.f));

  // on a 2x display, half points are whole pixels
  IRECT halves {0.5f, 1.5f, 10.5f, 20.5f};
  halves.PixelAlign(2.f);
  REQUIRE(halves == IRECT(0.5f, 1.5f, 10.5f, 20.5f));

  IRECT quarters {0.25f, 0.25f, 10.25f, 10.25f};
  quarters.PixelAlign(2.f);
  REQUIRE(quarters == IRECT(0.f, 0.f, 10.5f, 10.5f));

  REQUIRE(IRECT(0.3f, 0.3f, 10.6f, 10.6f).GetPixelAligned(2.f) == IRECT(0.f, 0.f, 11.f, 11.f));
  REQUIRE(IRECT(0.3f, 0.3f, 10.6f, 10.6f).GetPixelAligned(1.f) == IRECT(0.f, 0.f, 11.f, 11.f));
}

TEST_CASE("IRECT: pixel aligned rects contain the original and are aligned at the scale used", "[unit][irect]")
{
  const IRECT rects[] = {IRECT(0.1f, 0.2f, 9.3f, 7.4f), IRECT(-5.55f, -3.21f, 4.44f, 2.22f), IRECT(100.01f, 200.99f, 300.5f, 400.5f)};

  for (const IRECT& r : rects)
  {
    for (float scale : {1.f, 1.5f, 2.f, 3.f})
    {
      INFO(Catch::StringMaker<IRECT>::convert(r) << " scale " << scale);
      const IRECT aligned = r.GetPixelAligned(scale);
      CHECK(aligned.IsPixelAligned(scale));
      CHECK(aligned.L <= r.L + 1e-4f);
      CHECK(aligned.T <= r.T + 1e-4f);
      CHECK(aligned.R >= r.R - 1e-4f);
      CHECK(aligned.B >= r.B - 1e-4f);
      // ...and it never grows by a whole pixel or more
      CHECK(aligned.W() - r.W() < 2.f / scale + 1e-3f);
      CHECK(aligned.H() - r.H() < 2.f / scale + 1e-3f);
    }
  }
}

TEST_CASE("IRECT: PixelSnap rounds each edge to the nearest pixel", "[unit][irect]")
{
  IRECT r {10.3f, 20.7f, 109.6f, 70.2f};
  r.PixelSnap();
  REQUIRE(r == IRECT(10.f, 21.f, 110.f, 70.f));
  REQUIRE(r.IsPixelAligned());

  IRECT negative {-10.3f, -20.7f, -1.2f, -0.9f};
  negative.PixelSnap();
  REQUIRE(negative == IRECT(-10.f, -21.f, -1.f, -1.f));

  IRECT aligned = kRect;
  aligned.PixelSnap();
  REQUIRE(aligned == kRect);

  REQUIRE(IRECT(10.3f, 20.7f, 109.6f, 70.2f).GetPixelSnapped() == IRECT(10.f, 21.f, 110.f, 70.f));

  // unlike PixelAlign, the result may be smaller than the original
  const IRECT shrunk = IRECT(0.4f, 0.4f, 9.6f, 9.4f).GetPixelSnapped();
  REQUIRE(shrunk == IRECT(0.f, 0.f, 10.f, 9.f));
}

TEST_CASE("IRECT: PixelSnap at a scale", "[unit][irect]")
{
  IRECT r {0.3f, 0.2f, 10.6f, 10.8f};
  r.PixelSnap(2.f);
  REQUIRE(r == IRECT(0.5f, 0.f, 10.5f, 11.f));

  REQUIRE(IRECT(0.3f, 0.2f, 10.6f, 10.8f).GetPixelSnapped(2.f) == IRECT(0.5f, 0.f, 10.5f, 11.f));
  REQUIRE(IRECT(0.3f, 0.2f, 10.6f, 10.8f).GetPixelSnapped(1.f) == IRECT(0.f, 0.f, 11.f, 11.f));

  for (float scale : {1.f, 1.5f, 2.f, 3.f})
  {
    INFO("scale " << scale);
    const IRECT original {3.14f, 2.72f, 31.4f, 27.2f};
    const IRECT snapped = original.GetPixelSnapped(scale);
    CHECK(snapped.IsPixelAligned(scale));
    // no edge moves by more than half a pixel
    CHECK(NearRect(snapped, original, 0.5f / scale + 1e-3f));
  }
}

TEST_CASE("IRECT: Pad", "[unit][irect]")
{
  SECTION("uniform")
  {
    IRECT r = kRect;
    r.Pad(5.f);
    REQUIRE(r == IRECT(5.f, 15.f, 115.f, 75.f));
    r.Pad(-5.f);
    REQUIRE(r == kRect);
  }

  SECTION("negative padding contracts")
  {
    IRECT r = kRect;
    r.Pad(-10.f);
    REQUIRE(r == IRECT(20.f, 30.f, 100.f, 60.f));
  }

  SECTION("per edge")
  {
    IRECT r = kRect;
    r.Pad(1.f, 2.f, 3.f, 4.f);
    REQUIRE(r == IRECT(9.f, 18.f, 113.f, 74.f));
    r.Pad(-1.f, -2.f, -3.f, -4.f);
    REQUIRE(r == kRect);
  }

  SECTION("HPad only changes the width")
  {
    IRECT r = kRect;
    r.HPad(5.f);
    REQUIRE(r == IRECT(5.f, 20.f, 115.f, 70.f));
  }

  SECTION("VPad only changes the height")
  {
    IRECT r = kRect;
    r.VPad(5.f);
    REQUIRE(r == IRECT(10.f, 15.f, 110.f, 75.f));
  }

  SECTION("MidHPad sets the width about the centre")
  {
    IRECT r = kRect;
    r.MidHPad(5.f);
    REQUIRE(r == IRECT(55.f, 20.f, 65.f, 70.f));
    REQUIRE(r.MW() == kRect.MW());
    REQUIRE(r.W() == 10.f);
  }

  SECTION("MidVPad sets the height about the centre")
  {
    IRECT r = kRect;
    r.MidVPad(5.f);
    REQUIRE(r == IRECT(10.f, 40.f, 110.f, 50.f));
    REQUIRE(r.MH() == kRect.MH());
    REQUIRE(r.H() == 10.f);
  }
}

TEST_CASE("IRECT: GetPadded", "[unit][irect]")
{
  REQUIRE(kRect.GetPadded(5.f) == IRECT(5.f, 15.f, 115.f, 75.f));
  REQUIRE(kRect.GetPadded(-5.f) == IRECT(15.f, 25.f, 105.f, 65.f));
  REQUIRE(kRect.GetPadded(0.f) == kRect);
  REQUIRE(kRect.GetPadded(5.f).GetPadded(-5.f) == kRect);

  REQUIRE(kRect.GetPadded(1.f, 2.f, 3.f, 4.f) == IRECT(9.f, 18.f, 113.f, 74.f));
  REQUIRE(kRect.GetPadded(-1.f, -2.f, -3.f, -4.f) == IRECT(11.f, 22.f, 107.f, 66.f));
  REQUIRE(kRect.GetPadded(0.f, 0.f, 0.f, 0.f) == kRect);
  REQUIRE(kRect.GetPadded(5.f, 5.f, 5.f, 5.f) == kRect.GetPadded(5.f));

  // the centre doesn't move with uniform padding, and the size changes by twice the padding
  REQUIRE(kRect.GetPadded(7.f).MW() == kRect.MW());
  REQUIRE(kRect.GetPadded(7.f).MH() == kRect.MH());
  REQUIRE(kRect.GetPadded(7.f).W() == kRect.W() + 14.f);
  REQUIRE(kRect.GetPadded(7.f).H() == kRect.H() + 14.f);

  // the non-mutating and mutating versions agree
  IRECT r = kRect;
  r.Pad(3.f);
  REQUIRE(r == kRect.GetPadded(3.f));
  r = kRect;
  r.Pad(1.f, 2.f, 3.f, 4.f);
  REQUIRE(r == kRect.GetPadded(1.f, 2.f, 3.f, 4.f));
}

TEST_CASE("IRECT: GetHPadded, GetVPadded, GetMidHPadded and GetMidVPadded", "[unit][irect]")
{
  REQUIRE(kRect.GetHPadded(5.f) == IRECT(5.f, 20.f, 115.f, 70.f));
  REQUIRE(kRect.GetHPadded(-5.f) == IRECT(15.f, 20.f, 105.f, 70.f));
  REQUIRE(kRect.GetVPadded(5.f) == IRECT(10.f, 15.f, 110.f, 75.f));
  REQUIRE(kRect.GetVPadded(-5.f) == IRECT(10.f, 25.f, 110.f, 65.f));
  REQUIRE(kRect.GetHPadded(5.f).GetVPadded(5.f) == kRect.GetPadded(5.f));

  REQUIRE(kRect.GetMidHPadded(5.f) == IRECT(55.f, 20.f, 65.f, 70.f));
  REQUIRE(kRect.GetMidHPadded(50.f) == kRect);
  REQUIRE(kRect.GetMidHPadded(100.f) == IRECT(-40.f, 20.f, 160.f, 70.f));
  REQUIRE(kRect.GetMidVPadded(5.f) == IRECT(10.f, 40.f, 110.f, 50.f));
  REQUIRE(kRect.GetMidVPadded(25.f) == kRect);

  REQUIRE(kRect.GetMidHPadded(8.f).W() == 16.f);
  REQUIRE(kRect.GetMidHPadded(8.f).MW() == kRect.MW());
  REQUIRE(kRect.GetMidVPadded(8.f).H() == 16.f);
  REQUIRE(kRect.GetMidVPadded(8.f).MH() == kRect.MH());

  // the non-mutating and mutating versions agree
  IRECT r = kRect;
  r.HPad(3.f);
  REQUIRE(r == kRect.GetHPadded(3.f));
  r = kRect;
  r.VPad(3.f);
  REQUIRE(r == kRect.GetVPadded(3.f));
  r = kRect;
  r.MidHPad(3.f);
  REQUIRE(r == kRect.GetMidHPadded(3.f));
  r = kRect;
  r.MidVPad(3.f);
  REQUIRE(r == kRect.GetMidVPadded(3.f));
}

TEST_CASE("IRECT: GetHSliced and GetVSliced", "[unit][irect]")
{
  REQUIRE(kRect.GetHSliced(30.f) == IRECT(10.f, 20.f, 40.f, 70.f));
  REQUIRE(kRect.GetHSliced(30.f, false) == IRECT(10.f, 20.f, 40.f, 70.f));
  REQUIRE(kRect.GetHSliced(30.f, true) == IRECT(80.f, 20.f, 110.f, 70.f));
  REQUIRE(kRect.GetHSliced(100.f) == kRect);
  REQUIRE(kRect.GetHSliced(100.f, true) == kRect);

  REQUIRE(kRect.GetVSliced(30.f) == IRECT(10.f, 20.f, 110.f, 50.f));
  REQUIRE(kRect.GetVSliced(30.f, false) == IRECT(10.f, 20.f, 110.f, 50.f));
  REQUIRE(kRect.GetVSliced(30.f, true) == IRECT(10.f, 40.f, 110.f, 70.f));
  REQUIRE(kRect.GetVSliced(50.f) == kRect);
  REQUIRE(kRect.GetVSliced(50.f, true) == kRect);

  // equivalent to the GetFrom helpers
  REQUIRE(kRect.GetHSliced(30.f) == kRect.GetFromLeft(30.f));
  REQUIRE(kRect.GetHSliced(30.f, true) == kRect.GetFromRight(30.f));
  REQUIRE(kRect.GetVSliced(30.f) == kRect.GetFromTop(30.f));
  REQUIRE(kRect.GetVSliced(30.f, true) == kRect.GetFromBottom(30.f));
}

TEST_CASE("IRECT: Clank keeps a rect within bounds", "[unit][irect]")
{
  const IRECT bounds {0.f, 0.f, 100.f, 100.f};

  SECTION("a rect inside the bounds is unchanged")
  {
    IRECT r {10.f, 10.f, 30.f, 30.f};
    r.Clank(bounds);
    REQUIRE(r == IRECT(10.f, 10.f, 30.f, 30.f));
  }

  SECTION("off the left/top is moved in, keeping its size")
  {
    IRECT r {-10.f, -5.f, 10.f, 15.f};
    r.Clank(bounds);
    REQUIRE(r == IRECT(0.f, 0.f, 20.f, 20.f));
  }

  SECTION("off the right/bottom is moved in, keeping its size")
  {
    // the right and bottom edges of the bounds are exclusive
    IRECT r {90.f, 95.f, 110.f, 115.f};
    r.Clank(bounds);
    REQUIRE(r == IRECT(79.f, 79.f, 99.f, 99.f));
    REQUIRE(r.W() == 20.f);
    REQUIRE(r.H() == 20.f);
  }

  SECTION("off one side only leaves the other axis alone")
  {
    IRECT r {-10.f, 40.f, 10.f, 60.f};
    r.Clank(bounds);
    REQUIRE(r == IRECT(0.f, 40.f, 20.f, 60.f));
  }

  SECTION("bounds that are not at the origin")
  {
    const IRECT offsetBounds {50.f, 50.f, 150.f, 150.f};
    IRECT r {0.f, 0.f, 20.f, 20.f};
    r.Clank(offsetBounds);
    REQUIRE(r == IRECT(50.f, 50.f, 70.f, 70.f));
  }

  SECTION("a rect larger than the bounds is limited to them")
  {
    IRECT r {-50.f, -50.f, 500.f, 500.f};
    r.Clank(bounds);
    REQUIRE(r.L == 0.f);
    REQUIRE(r.T == 0.f);
    REQUIRE(r.R == 99.f);
    REQUIRE(r.B == 99.f);
  }

  SECTION("the result is always within the bounds")
  {
    for (float x = -150.f; x <= 250.f; x += 37.5f)
    {
      for (float y = -150.f; y <= 250.f; y += 37.5f)
      {
        IRECT r = IRECT::MakeXYWH(x, y, 40.f, 25.f);
        r.Clank(bounds);
        INFO("x " << x << " y " << y);
        CHECK(bounds.Contains(r));
        CHECK(r.W() == 40.f);
        CHECK(r.H() == 25.f);
      }
    }
  }
}

TEST_CASE("IRECT: Scale multiplies every edge", "[unit][irect]")
{
  IRECT r = kRect;
  r.Scale(2.f);
  REQUIRE(r == IRECT(20.f, 40.f, 220.f, 140.f));
  r.Scale(0.5f);
  REQUIRE(r == kRect);
  r.Scale(1.f);
  REQUIRE(r == kRect);
  r.Scale(0.f);
  REQUIRE(r.Empty());

  REQUIRE(kRect.GetScaled(2.f) == IRECT(20.f, 40.f, 220.f, 140.f));
  REQUIRE(kRect.GetScaled(0.5f) == IRECT(5.f, 10.f, 55.f, 35.f));
  REQUIRE(kRect.GetScaled(1.f) == kRect);
  REQUIRE(kRect.GetScaled(3.f).W() == 300.f);
  REQUIRE(kRect.GetScaled(3.f).H() == 150.f);
  REQUIRE(kRect.GetScaled(3.f).Area() == 9.f * kRect.Area());
}

TEST_CASE("IRECT: ScaleAboutCentre keeps the centre point", "[unit][irect]")
{
  IRECT r = kRect;
  r.ScaleAboutCentre(2.f);
  REQUIRE(r == IRECT(-40.f, -5.f, 160.f, 95.f));
  r.ScaleAboutCentre(0.5f);
  REQUIRE(r == kRect);

  REQUIRE(kRect.GetScaledAboutCentre(2.f) == IRECT(-40.f, -5.f, 160.f, 95.f));
  REQUIRE(kRect.GetScaledAboutCentre(0.5f) == IRECT(35.f, 32.5f, 85.f, 57.5f));
  REQUIRE(kRect.GetScaledAboutCentre(1.f) == kRect);

  const IRECT collapsed = kRect.GetScaledAboutCentre(0.f);
  REQUIRE(collapsed == IRECT(60.f, 45.f, 60.f, 45.f));

  for (float scale : {0.1f, 0.5f, 0.9f, 1.25f, 3.f})
  {
    INFO("scale " << scale);
    const IRECT scaled = kRect.GetScaledAboutCentre(scale);
    CHECK(scaled.MW() == Approx(kRect.MW()));
    CHECK(scaled.MH() == Approx(kRect.MH()));
    CHECK(scaled.W() == Approx(kRect.W() * scale));
    CHECK(scaled.H() == Approx(kRect.H() * scale));
  }
}

TEST_CASE("IRECT: LinearInterpolateBetween", "[unit][irect]")
{
  const IRECT start {0.f, 10.f, 100.f, 110.f};
  const IRECT dest {50.f, 30.f, 200.f, 90.f};

  REQUIRE(IRECT::LinearInterpolateBetween(start, dest, 0.f) == start);
  REQUIRE(IRECT::LinearInterpolateBetween(start, dest, 1.f) == dest);
  REQUIRE(IRECT::LinearInterpolateBetween(start, dest, 0.5f) == IRECT(25.f, 20.f, 150.f, 100.f));
  REQUIRE(IRECT::LinearInterpolateBetween(start, dest, 0.25f) == IRECT(12.5f, 15.f, 125.f, 105.f));
  REQUIRE(IRECT::LinearInterpolateBetween(start, start, 0.5f) == start);

  // progress beyond 0-1 extrapolates
  REQUIRE(IRECT::LinearInterpolateBetween(start, dest, 2.f) == IRECT(100.f, 50.f, 300.f, 70.f));
  REQUIRE(IRECT::LinearInterpolateBetween(start, dest, -1.f) == IRECT(-50.f, -10.f, 0.f, 130.f));

  // interpolating in the opposite direction mirrors the progress
  REQUIRE(IRECT::LinearInterpolateBetween(start, dest, 0.25f) == IRECT::LinearInterpolateBetween(dest, start, 0.75f));
}

TEST_CASE("IRECT: GetRandomPoint is inside the rect", "[unit][irect]")
{
  std::srand(1234);
  const IRECT rects[] = {kRect, IRECT(-50.f, -60.f, -10.f, -20.f), IRECT(0.f, 0.f, 1.f, 1.f)};

  for (const IRECT& r : rects)
  {
    bool allInside = true;
    float minX = r.R, maxX = r.L, minY = r.B, maxY = r.T;

    for (int i = 0; i < 500; i++)
    {
      float x = -1000.f, y = -1000.f;
      r.GetRandomPoint(x, y);
      allInside = allInside && x >= r.L && x <= r.R && y >= r.T && y <= r.B;
      minX = std::min(minX, x);
      maxX = std::max(maxX, x);
      minY = std::min(minY, y);
      maxY = std::max(maxY, y);
    }

    INFO(Catch::StringMaker<IRECT>::convert(r));
    REQUIRE(allInside);
    // not all the same point
    REQUIRE(maxX > minX);
    REQUIRE(maxY > minY);
  }
}

TEST_CASE("IRECT: GetRandomSubRect is inside the rect", "[unit][irect]")
{
  std::srand(4321);

  for (int i = 0; i < 500; i++)
  {
    const IRECT sub = kRect.GetRandomSubRect();
    REQUIRE(sub.L >= kRect.L);
    REQUIRE(sub.T >= kRect.T);
    REQUIRE(sub.R <= kRect.R);
    REQUIRE(sub.B <= kRect.B);
    REQUIRE(sub.W() >= 0.f);
    REQUIRE(sub.H() >= 0.f);
  }
}

TEST_CASE("IRECT: Offset and GetOffset move each edge independently", "[unit][irect]")
{
  IRECT r = kRect;
  r.Offset(1.f, 2.f, 3.f, 4.f);
  REQUIRE(r == IRECT(11.f, 22.f, 113.f, 74.f));
  r.Offset(-1.f, -2.f, -3.f, -4.f);
  REQUIRE(r == kRect);

  REQUIRE(kRect.GetOffset(1.f, 2.f, 3.f, 4.f) == IRECT(11.f, 22.f, 113.f, 74.f));
  REQUIRE(kRect.GetOffset(-10.f, -20.f, -110.f, -70.f).Empty());
  REQUIRE(kRect.GetOffset(0.f, 0.f, 0.f, 0.f) == kRect);

  // offsetting all edges by the same x/y is a translation
  REQUIRE(kRect.GetOffset(5.f, 7.f, 5.f, 7.f) == kRect.GetTranslated(5.f, 7.f));
}

TEST_CASE("IRECT: Translate, GetTranslated, GetHShifted and GetVShifted", "[unit][irect]")
{
  IRECT r = kRect;
  r.Translate(5.f, -7.f);
  REQUIRE(r == IRECT(15.f, 13.f, 115.f, 63.f));
  REQUIRE(r.W() == kRect.W());
  REQUIRE(r.H() == kRect.H());
  r.Translate(-5.f, 7.f);
  REQUIRE(r == kRect);

  REQUIRE(kRect.GetTranslated(5.f, -7.f) == IRECT(15.f, 13.f, 115.f, 63.f));
  REQUIRE(kRect.GetTranslated(0.f, 0.f) == kRect);
  REQUIRE(kRect.GetTranslated(-10.f, -20.f) == IRECT(0.f, 0.f, 100.f, 50.f));

  REQUIRE(kRect.GetHShifted(5.f) == IRECT(15.f, 20.f, 115.f, 70.f));
  REQUIRE(kRect.GetHShifted(-5.f) == IRECT(5.f, 20.f, 105.f, 70.f));
  REQUIRE(kRect.GetVShifted(5.f) == IRECT(10.f, 25.f, 110.f, 75.f));
  REQUIRE(kRect.GetVShifted(-5.f) == IRECT(10.f, 15.f, 110.f, 65.f));
  REQUIRE(kRect.GetHShifted(3.f).GetVShifted(4.f) == kRect.GetTranslated(3.f, 4.f));
}

TEST_CASE("IRECT: GetCentredInside with a size rect", "[unit][irect]")
{
  REQUIRE(kRect.GetCentredInside(IRECT(0.f, 0.f, 20.f, 10.f)) == IRECT(50.f, 40.f, 70.f, 50.f));

  // only the size of the argument matters, not its position
  REQUIRE(kRect.GetCentredInside(IRECT(500.f, 600.f, 520.f, 610.f)) == IRECT(50.f, 40.f, 70.f, 50.f));

  REQUIRE(kRect.GetCentredInside(kRect) == kRect);

  // a larger rect is still centred, extending outside
  REQUIRE(kRect.GetCentredInside(IRECT(0.f, 0.f, 200.f, 100.f)) == IRECT(-40.f, -5.f, 160.f, 95.f));
}

TEST_CASE("IRECT: GetCentredInside with a width and height", "[unit][irect]")
{
  REQUIRE(kRect.GetCentredInside(20.f, 10.f) == IRECT(50.f, 40.f, 70.f, 50.f));
  REQUIRE(kRect.GetCentredInside(100.f, 50.f) == kRect);

  SECTION("a height of zero (the default) gives a square")
  {
    REQUIRE(kRect.GetCentredInside(20.f) == IRECT(50.f, 35.f, 70.f, 55.f));
    REQUIRE(kRect.GetCentredInside(20.f, 0.f) == IRECT(50.f, 35.f, 70.f, 55.f));
    REQUIRE(kRect.GetCentredInside(20.f).W() == kRect.GetCentredInside(20.f).H());
  }

  SECTION("the width is at least 1")
  {
    REQUIRE(kRect.GetCentredInside(0.f, 10.f) == IRECT(59.5f, 40.f, 60.5f, 50.f));
    REQUIRE(kRect.GetCentredInside(-20.f, 10.f).W() == 1.f);
    REQUIRE(kRect.GetCentredInside(0.f) == IRECT(59.5f, 44.5f, 60.5f, 45.5f));
  }

  SECTION("the centre is kept")
  {
    for (float w : {1.f, 3.f, 10.f, 64.f, 250.f})
    {
      for (float h : {1.f, 7.f, 50.f, 99.f})
      {
        const IRECT r = kRect.GetCentredInside(w, h);
        INFO("w " << w << " h " << h);
        CHECK(r.MW() == Approx(kRect.MW()));
        CHECK(r.MH() == Approx(kRect.MH()));
        CHECK(r.W() == Approx(w));
        CHECK(r.H() == Approx(h));
      }
    }
  }
}

TEST_CASE("IRECT: GetCentredInside with a bitmap uses the frame size", "[unit][irect]")
{
  APIBitmap apiBitmap {0, 40, 20, 1.f, 1.f};

  REQUIRE(kRect.GetCentredInside(IBitmap(&apiBitmap, 1, false)) == IRECT(40.f, 35.f, 80.f, 55.f));
  REQUIRE(kRect.GetCentredInside(IBitmap(&apiBitmap, 2, false)) == IRECT(40.f, 40.f, 80.f, 50.f));
  REQUIRE(kRect.GetCentredInside(IBitmap(&apiBitmap, 2, true)) == IRECT(50.f, 35.f, 70.f, 55.f));
}

TEST_CASE("IRECT: VAlignTo and GetVAlignedTo", "[unit][irect]")
{
  const IRECT reference {0.f, 100.f, 500.f, 200.f};
  const IRECT r {10.f, 20.f, 110.f, 40.f}; // H 20

  REQUIRE(r.GetVAlignedTo(reference, EVAlign::Top) == IRECT(10.f, 100.f, 110.f, 120.f));
  REQUIRE(r.GetVAlignedTo(reference, EVAlign::Middle) == IRECT(10.f, 140.f, 110.f, 160.f));
  REQUIRE(r.GetVAlignedTo(reference, EVAlign::Bottom) == IRECT(10.f, 180.f, 110.f, 200.f));

  for (EVAlign align : {EVAlign::Top, EVAlign::Middle, EVAlign::Bottom})
  {
    const IRECT aligned = r.GetVAlignedTo(reference, align);
    // only the vertical position changes
    REQUIRE(aligned.L == r.L);
    REQUIRE(aligned.R == r.R);
    REQUIRE(aligned.H() == r.H());

    IRECT mutated = r;
    mutated.VAlignTo(reference, align);
    REQUIRE(mutated == aligned);
  }

  REQUIRE(r.GetVAlignedTo(reference, EVAlign::Middle).MH() == reference.MH());

  // taller than the reference: still aligned, overhanging
  const IRECT tall {0.f, 0.f, 10.f, 300.f};
  REQUIRE(tall.GetVAlignedTo(reference, EVAlign::Top) == IRECT(0.f, 100.f, 10.f, 400.f));
  REQUIRE(tall.GetVAlignedTo(reference, EVAlign::Bottom) == IRECT(0.f, -100.f, 10.f, 200.f));
  REQUIRE(tall.GetVAlignedTo(reference, EVAlign::Middle) == IRECT(0.f, 0.f, 10.f, 300.f));
}

TEST_CASE("IRECT: HAlignTo and GetHAlignedTo", "[unit][irect]")
{
  const IRECT reference {100.f, 0.f, 200.f, 500.f};
  const IRECT r {10.f, 20.f, 30.f, 70.f}; // W 20

  REQUIRE(r.GetHAlignedTo(reference, EAlign::Near) == IRECT(100.f, 20.f, 120.f, 70.f));
  REQUIRE(r.GetHAlignedTo(reference, EAlign::Center) == IRECT(140.f, 20.f, 160.f, 70.f));
  REQUIRE(r.GetHAlignedTo(reference, EAlign::Far) == IRECT(180.f, 20.f, 200.f, 70.f));

  for (EAlign align : {EAlign::Near, EAlign::Center, EAlign::Far})
  {
    const IRECT aligned = r.GetHAlignedTo(reference, align);
    // only the horizontal position changes
    REQUIRE(aligned.T == r.T);
    REQUIRE(aligned.B == r.B);
    REQUIRE(aligned.W() == r.W());

    IRECT mutated = r;
    mutated.HAlignTo(reference, align);
    REQUIRE(mutated == aligned);
  }

  REQUIRE(r.GetHAlignedTo(reference, EAlign::Center).MW() == reference.MW());

  // wider than the reference: still aligned, overhanging
  const IRECT wide {0.f, 0.f, 300.f, 10.f};
  REQUIRE(wide.GetHAlignedTo(reference, EAlign::Near) == IRECT(100.f, 0.f, 400.f, 10.f));
  REQUIRE(wide.GetHAlignedTo(reference, EAlign::Far) == IRECT(-100.f, 0.f, 200.f, 10.f));
  REQUIRE(wide.GetHAlignedTo(reference, EAlign::Center) == IRECT(0.f, 0.f, 300.f, 10.f));
}

TEST_CASE("IRECT: aligning on both axes", "[unit][irect]")
{
  const IRECT reference {0.f, 0.f, 200.f, 100.f};
  const IRECT r {7.f, 9.f, 27.f, 19.f}; // 20 x 10

  REQUIRE(r.GetHAlignedTo(reference, EAlign::Center).GetVAlignedTo(reference, EVAlign::Middle) == reference.GetCentredInside(20.f, 10.f));
  REQUIRE(r.GetHAlignedTo(reference, EAlign::Near).GetVAlignedTo(reference, EVAlign::Top) == reference.GetFromTLHC(20.f, 10.f));
  REQUIRE(r.GetHAlignedTo(reference, EAlign::Far).GetVAlignedTo(reference, EVAlign::Top) == reference.GetFromTRHC(20.f, 10.f));
  REQUIRE(r.GetHAlignedTo(reference, EAlign::Near).GetVAlignedTo(reference, EVAlign::Bottom) == reference.GetFromBLHC(20.f, 10.f));
  REQUIRE(r.GetHAlignedTo(reference, EAlign::Far).GetVAlignedTo(reference, EVAlign::Bottom) == reference.GetFromBRHC(20.f, 10.f));
}

TEST_CASE("IRECT: GetLengthOfShortestSide", "[unit][irect]")
{
  REQUIRE(kRect.GetLengthOfShortestSide() == 50.f);
  REQUIRE(IRECT(0.f, 0.f, 10.f, 30.f).GetLengthOfShortestSide() == 10.f);
  REQUIRE(IRECT(0.f, 0.f, 30.f, 10.f).GetLengthOfShortestSide() == 10.f);
  REQUIRE(IRECT(0.f, 0.f, 20.f, 20.f).GetLengthOfShortestSide() == 20.f);
  REQUIRE(IRECT().GetLengthOfShortestSide() == 0.f);
  REQUIRE(IRECT(-5.f, -5.f, 5.f, 0.f).GetLengthOfShortestSide() == 5.f);
}

TEST_CASE("IRECT: the non-mutating helpers leave the rect unchanged", "[unit][irect]")
{
  const IRECT r = kRect;

  (void) r.GetPadded(3.f);
  (void) r.GetScaled(2.f);
  (void) r.GetScaledAboutCentre(2.f);
  (void) r.GetTranslated(1.f, 1.f);
  (void) r.GetPixelAligned();
  (void) r.GetPixelSnapped(2.f);
  (void) r.GetCentredInside(4.f);
  (void) r.GetGridCell(1, 2, 2);
  (void) r.GetFromTop(5.f);
  (void) r.GetReducedFromLeft(5.f);
  (void) r.GetHAlignedTo(IRECT(0.f, 0.f, 1.f, 1.f), EAlign::Far);
  (void) r.GetVAlignedTo(IRECT(0.f, 0.f, 1.f, 1.f), EVAlign::Bottom);

  REQUIRE(r == kRect);
}

TEST_CASE("IRECT: MakeIRect macros build a rect from _X, _Y, _W, _H constants", "[unit][irect]")
{
  const float kKnob_X = 10.f;
  const float kKnob_Y = 20.f;
  const float kKnob_W = 100.f;
  const float kKnob_H = 50.f;

  REQUIRE(MakeIRect(kKnob) == kRect);
  REQUIRE(MakeIRectHOffset(kKnob, 5.f) == kRect.GetHShifted(5.f));
  REQUIRE(MakeIRectVOffset(kKnob, 5.f) == kRect.GetVShifted(5.f));
  REQUIRE(MakeIRectHVOffset(kKnob, 5.f, -7.f) == kRect.GetTranslated(5.f, -7.f));
}
