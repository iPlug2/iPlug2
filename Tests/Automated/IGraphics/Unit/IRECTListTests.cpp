/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for IRECTList, the list of rectangles used for dirty region handling and grid layouts.

#include <cstdint>
#include <initializer_list>
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
void AddAll(IRECTList& list, const std::vector<IRECT>& rects)
{
  for (const IRECT& r : rects)
    list.Add(r);
}

std::vector<IRECT> ToVector(const IRECTList& list)
{
  std::vector<IRECT> rects;

  for (int i = 0; i < list.Size(); i++)
    rects.push_back(list.Get(i));

  return rects;
}

bool Covers(const std::vector<IRECT>& rects, float x, float y)
{
  for (const IRECT& r : rects)
  {
    if (x >= r.L && x < r.R && y >= r.T && y < r.B)
      return true;
  }

  return false;
}

/** Optimizes the rects (which must have integer coordinates within 0-kExtent) and checks that exactly the same
 *  pixels are covered afterwards and that no two of the resulting rects overlap
 *  @return The number of rects after optimization */
int CheckOptimize(const std::vector<IRECT>& input)
{
  const int kExtent = 48;

  IRECTList list;
  AddAll(list, input);
  list.Optimize();

  const std::vector<IRECT> output = ToVector(list);

  int nCoverageDifferences = 0;

  for (int y = -1; y <= kExtent; y++)
  {
    for (int x = -1; x <= kExtent; x++)
    {
      const float px = static_cast<float>(x) + 0.5f;
      const float py = static_cast<float>(y) + 0.5f;

      if (Covers(input, px, py) != Covers(output, px, py))
        nCoverageDifferences++;
    }
  }

  CHECK(nCoverageDifferences == 0);

  float totalOverlap = 0.f;

  for (size_t i = 0; i < output.size(); i++)
  {
    CHECK(output[i].W() > 0.f);
    CHECK(output[i].H() > 0.f);

    for (size_t j = i + 1; j < output.size(); j++)
      totalOverlap += OverlapArea(output[i], output[j]);
  }

  CHECK(totalOverlap == 0.f);

  return list.Size();
}

/** A minimal deterministic generator, so that the pseudo-random cases are the same on every platform */
struct TestRandom
{
  uint32_t mState;

  explicit TestRandom(uint32_t seed) : mState(seed) {}

  /** @return A value in the range 0 to n-1 */
  int Next(int n)
  {
    mState = mState * 1664525u + 1013904223u;
    return static_cast<int>((mState >> 8) % static_cast<uint32_t>(n));
  }
};
} // namespace

TEST_CASE("IRECTList: starts empty", "[unit][irectlist]")
{
  IRECTList list;
  REQUIRE(list.Size() == 0);
  REQUIRE(list.Find(0.f, 0.f) == -1);
}

TEST_CASE("IRECTList: Add, Get and Size", "[unit][irectlist]")
{
  IRECTList list;
  list.Add(IRECT(0.f, 0.f, 10.f, 10.f));
  REQUIRE(list.Size() == 1);
  list.Add(IRECT(20.f, 20.f, 30.f, 30.f));
  list.Add(IRECT(1.f, 2.f, 3.f, 4.f));
  REQUIRE(list.Size() == 3);

  // insertion order is kept
  REQUIRE(list.Get(0) == IRECT(0.f, 0.f, 10.f, 10.f));
  REQUIRE(list.Get(1) == IRECT(20.f, 20.f, 30.f, 30.f));
  REQUIRE(list.Get(2) == IRECT(1.f, 2.f, 3.f, 4.f));

  // duplicates and empty rects are allowed
  list.Add(IRECT(1.f, 2.f, 3.f, 4.f));
  list.Add(IRECT());
  REQUIRE(list.Size() == 5);
  REQUIRE(list.Get(3) == list.Get(2));
  REQUIRE(list.Get(4).Empty());
}

TEST_CASE("IRECTList: holds many rects", "[unit][irectlist]")
{
  IRECTList list;

  for (int i = 0; i < 1000; i++)
    list.Add(IRECT::MakeXYWH(static_cast<float>(i), static_cast<float>(i * 2), 1.f, 2.f));

  REQUIRE(list.Size() == 1000);

  bool allMatch = true;

  for (int i = 0; i < 1000; i++)
    allMatch = allMatch && list.Get(i) == IRECT::MakeXYWH(static_cast<float>(i), static_cast<float>(i * 2), 1.f, 2.f);

  REQUIRE(allMatch);
}

TEST_CASE("IRECTList: Set replaces a rect", "[unit][irectlist]")
{
  IRECTList list;
  AddAll(list, {IRECT(0.f, 0.f, 10.f, 10.f), IRECT(20.f, 20.f, 30.f, 30.f), IRECT(1.f, 2.f, 3.f, 4.f)});

  list.Set(1, IRECT(5.f, 6.f, 7.f, 8.f));
  REQUIRE(list.Size() == 3);
  REQUIRE(list.Get(0) == IRECT(0.f, 0.f, 10.f, 10.f));
  REQUIRE(list.Get(1) == IRECT(5.f, 6.f, 7.f, 8.f));
  REQUIRE(list.Get(2) == IRECT(1.f, 2.f, 3.f, 4.f));

  list.Set(0, IRECT());
  list.Set(2, IRECT(9.f, 9.f, 9.f, 9.f));
  REQUIRE(list.Get(0).Empty());
  REQUIRE(list.Get(2) == IRECT(9.f, 9.f, 9.f, 9.f));
}

TEST_CASE("IRECTList: Clear", "[unit][irectlist]")
{
  IRECTList list;
  AddAll(list, {IRECT(0.f, 0.f, 10.f, 10.f), IRECT(20.f, 20.f, 30.f, 30.f)});
  list.Clear();
  REQUIRE(list.Size() == 0);
  REQUIRE(list.Find(5.f, 5.f) == -1);

  // can be reused
  list.Add(IRECT(1.f, 2.f, 3.f, 4.f));
  REQUIRE(list.Size() == 1);
  REQUIRE(list.Get(0) == IRECT(1.f, 2.f, 3.f, 4.f));
}

TEST_CASE("IRECTList: Bounds is the union of all the rects", "[unit][irectlist]")
{
  IRECTList list;
  list.Add(IRECT(10.f, 20.f, 30.f, 40.f));
  REQUIRE(list.Bounds() == IRECT(10.f, 20.f, 30.f, 40.f));

  list.Add(IRECT(-5.f, 25.f, 15.f, 60.f));
  REQUIRE(list.Bounds() == IRECT(-5.f, 20.f, 30.f, 60.f));

  list.Add(IRECT(100.f, 0.f, 110.f, 5.f));
  REQUIRE(list.Bounds() == IRECT(-5.f, 0.f, 110.f, 60.f));

  // contained rects don't change it
  list.Add(IRECT(0.f, 30.f, 5.f, 35.f));
  REQUIRE(list.Bounds() == IRECT(-5.f, 0.f, 110.f, 60.f));

  // every rect is inside the bounds
  const IRECT bounds = list.Bounds();
  for (int i = 0; i < list.Size(); i++)
    REQUIRE(bounds.Contains(list.Get(i)));
}

TEST_CASE("IRECTList: Bounds ignores empty rects", "[unit][irectlist]")
{
  IRECTList list;
  AddAll(list, {IRECT(), IRECT(10.f, 20.f, 30.f, 40.f), IRECT(), IRECT(50.f, 60.f, 70.f, 80.f)});
  REQUIRE(list.Bounds() == IRECT(10.f, 20.f, 70.f, 80.f));
}

TEST_CASE("IRECTList: PixelAlign aligns every rect outwards", "[unit][irectlist]")
{
  IRECTList list;
  AddAll(list, {IRECT(0.3f, 0.7f, 9.2f, 9.9f), IRECT(10.f, 20.f, 30.f, 40.f), IRECT(-1.5f, -2.5f, 1.5f, 2.5f)});
  list.PixelAlign();

  REQUIRE(list.Size() == 3);
  REQUIRE(list.Get(0) == IRECT(0.f, 0.f, 10.f, 10.f));
  REQUIRE(list.Get(1) == IRECT(10.f, 20.f, 30.f, 40.f));
  REQUIRE(list.Get(2) == IRECT(-2.f, -3.f, 2.f, 3.f));
}

TEST_CASE("IRECTList: PixelAlign at a scale", "[unit][irectlist]")
{
  IRECTList list;
  AddAll(list, {IRECT(0.3f, 0.3f, 10.6f, 10.6f), IRECT(0.5f, 1.5f, 10.5f, 20.5f), IRECT(0.25f, 0.25f, 10.25f, 10.25f)});
  list.PixelAlign(2.f);

  REQUIRE(list.Size() == 3);
  REQUIRE(list.Get(0) == IRECT(0.f, 0.f, 11.f, 11.f));
  REQUIRE(list.Get(1) == IRECT(0.5f, 1.5f, 10.5f, 20.5f));
  REQUIRE(list.Get(2) == IRECT(0.f, 0.f, 10.5f, 10.5f));

  for (int i = 0; i < list.Size(); i++)
    REQUIRE(list.Get(i).IsPixelAligned(2.f));
}

TEST_CASE("IRECTList: Find returns the first rect containing a point", "[unit][irectlist]")
{
  IRECTList list;
  AddAll(list, {IRECT(0.f, 0.f, 10.f, 10.f), IRECT(20.f, 0.f, 30.f, 10.f), IRECT(5.f, 5.f, 25.f, 25.f)});

  REQUIRE(list.Find(2.f, 2.f) == 0);
  REQUIRE(list.Find(22.f, 2.f) == 1);
  REQUIRE(list.Find(15.f, 15.f) == 2);

  // where rects overlap, the earliest wins
  REQUIRE(list.Find(7.f, 7.f) == 0);
  REQUIRE(list.Find(22.f, 7.f) == 1);

  REQUIRE(list.Find(15.f, 2.f) == -1);
  REQUIRE(list.Find(-1.f, -1.f) == -1);
  REQUIRE(list.Find(100.f, 100.f) == -1);

  // right and bottom edges are exclusive, as with IRECT::Contains
  REQUIRE(list.Find(0.f, 0.f) == 0);
  REQUIRE(list.Find(30.f, 5.f) == -1);
  REQUIRE(list.Find(15.f, 25.f) == -1);
}

TEST_CASE("IRECTList: GetFracGrid lays cells out by row", "[unit][irectlist]")
{
  const IRECT bounds {0.f, 0.f, 100.f, 40.f};
  IRECTList list;

  REQUIRE(IRECTList::GetFracGrid(bounds, list, {0.25f, 0.75f}, {0.5f, 0.25f, 0.25f}));
  REQUIRE(list.Size() == 6);

  REQUIRE(list.Get(0) == IRECT(0.f, 0.f, 50.f, 10.f));
  REQUIRE(list.Get(1) == IRECT(50.f, 0.f, 75.f, 10.f));
  REQUIRE(list.Get(2) == IRECT(75.f, 0.f, 100.f, 10.f));
  REQUIRE(list.Get(3) == IRECT(0.f, 10.f, 50.f, 40.f));
  REQUIRE(list.Get(4) == IRECT(50.f, 10.f, 75.f, 40.f));
  REQUIRE(list.Get(5) == IRECT(75.f, 10.f, 100.f, 40.f));
  REQUIRE(list.Bounds() == bounds);
}

TEST_CASE("IRECTList: GetFracGrid lays cells out by column", "[unit][irectlist]")
{
  const IRECT bounds {0.f, 0.f, 100.f, 40.f};
  IRECTList list;

  REQUIRE(IRECTList::GetFracGrid(bounds, list, {0.25f, 0.75f}, {0.5f, 0.25f, 0.25f}, EDirection::Vertical));
  REQUIRE(list.Size() == 6);

  REQUIRE(list.Get(0) == IRECT(0.f, 0.f, 50.f, 10.f));
  REQUIRE(list.Get(1) == IRECT(0.f, 10.f, 50.f, 40.f));
  REQUIRE(list.Get(2) == IRECT(50.f, 0.f, 75.f, 10.f));
  REQUIRE(list.Get(3) == IRECT(50.f, 10.f, 75.f, 40.f));
  REQUIRE(list.Get(4) == IRECT(75.f, 0.f, 100.f, 10.f));
  REQUIRE(list.Get(5) == IRECT(75.f, 10.f, 100.f, 40.f));
  REQUIRE(list.Bounds() == bounds);
}

TEST_CASE("IRECTList: GetFracGrid with bounds that are not at the origin", "[unit][irectlist]")
{
  const IRECT bounds {10.f, 20.f, 110.f, 60.f};

  for (EDirection dir : {EDirection::Horizontal, EDirection::Vertical})
  {
    IRECTList list;
    REQUIRE(IRECTList::GetFracGrid(bounds, list, {0.5f, 0.25f, 0.25f}, {0.25f, 0.25f, 0.5f}, dir));
    REQUIRE(list.Size() == 9);
    REQUIRE(list.Get(0) == IRECT(10.f, 20.f, 35.f, 40.f));
    REQUIRE(list.Get(8) == IRECT(60.f, 50.f, 110.f, 60.f));
    REQUIRE(list.Bounds() == bounds);

    // the cells tile the bounds
    float area = 0.f;
    float overlap = 0.f;

    for (int i = 0; i < list.Size(); i++)
    {
      REQUIRE(bounds.Contains(list.Get(i)));
      area += list.Get(i).Area();

      for (int j = i + 1; j < list.Size(); j++)
        overlap += OverlapArea(list.Get(i), list.Get(j));
    }

    REQUIRE(area == bounds.Area());
    REQUIRE(overlap == 0.f);
  }
}

TEST_CASE("IRECTList: GetFracGrid with a single cell", "[unit][irectlist]")
{
  const IRECT bounds {10.f, 20.f, 110.f, 60.f};
  IRECTList list;
  REQUIRE(IRECTList::GetFracGrid(bounds, list, {1.f}, {1.f}));
  REQUIRE(list.Size() == 1);
  REQUIRE(list.Get(0) == bounds);
}

TEST_CASE("IRECTList: GetFracGrid fails if the fractions don't sum to one", "[unit][irectlist]")
{
  const IRECT bounds {0.f, 0.f, 100.f, 40.f};
  IRECTList list;

  REQUIRE_FALSE(IRECTList::GetFracGrid(bounds, list, {0.25f, 0.5f}, {0.5f, 0.5f}));
  REQUIRE(list.Size() == 0);
  REQUIRE_FALSE(IRECTList::GetFracGrid(bounds, list, {0.5f, 0.5f}, {0.5f, 0.75f}));
  REQUIRE(list.Size() == 0);
  REQUIRE_FALSE(IRECTList::GetFracGrid(bounds, list, {0.5f, 0.5f}, {0.5f, 0.75f}, EDirection::Vertical));
  REQUIRE(list.Size() == 0);
  REQUIRE_FALSE(IRECTList::GetFracGrid(bounds, list, {}, {1.f}));
  REQUIRE(list.Size() == 0);
}

// ten columns of 0.1f accumulate to 1.0000001f
TEST_CASE("IRECTList: GetFracGrid accepts fractions that sum to one within rounding error", "[unit][irectlist]")
{
  const IRECT bounds {0.f, 0.f, 100.f, 40.f};
  IRECTList list;

  CHECK(IRECTList::GetFracGrid(bounds, list, {1.f}, {0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f, 0.1f}));
  CHECK(list.Size() == 10);
}

TEST_CASE("IRECTList: Optimize leaves separate rects alone", "[unit][irectlist]")
{
  IRECTList list;
  AddAll(list, {IRECT(0.f, 0.f, 10.f, 10.f), IRECT(20.f, 20.f, 30.f, 30.f), IRECT(0.f, 30.f, 5.f, 40.f)});
  list.Optimize();

  REQUIRE(list.Size() == 3);
  REQUIRE(list.Get(0) == IRECT(0.f, 0.f, 10.f, 10.f));
  REQUIRE(list.Get(1) == IRECT(20.f, 20.f, 30.f, 30.f));
  REQUIRE(list.Get(2) == IRECT(0.f, 30.f, 5.f, 40.f));
}

TEST_CASE("IRECTList: Optimize on an empty or single rect list", "[unit][irectlist]")
{
  IRECTList list;
  list.Optimize();
  REQUIRE(list.Size() == 0);

  list.Add(IRECT(1.f, 2.f, 3.f, 4.f));
  list.Optimize();
  REQUIRE(list.Size() == 1);
  REQUIRE(list.Get(0) == IRECT(1.f, 2.f, 3.f, 4.f));
}

TEST_CASE("IRECTList: Optimize removes contained rects", "[unit][irectlist]")
{
  SECTION("the container comes first")
  {
    IRECTList list;
    AddAll(list, {IRECT(0.f, 0.f, 40.f, 40.f), IRECT(10.f, 10.f, 20.f, 20.f), IRECT(25.f, 5.f, 35.f, 30.f)});
    list.Optimize();
    REQUIRE(list.Size() == 1);
    REQUIRE(list.Get(0) == IRECT(0.f, 0.f, 40.f, 40.f));
  }

  SECTION("the container comes last")
  {
    IRECTList list;
    AddAll(list, {IRECT(10.f, 10.f, 20.f, 20.f), IRECT(25.f, 5.f, 35.f, 30.f), IRECT(0.f, 0.f, 40.f, 40.f)});
    list.Optimize();
    REQUIRE(list.Size() == 1);
    REQUIRE(list.Get(0) == IRECT(0.f, 0.f, 40.f, 40.f));
  }

  SECTION("duplicates")
  {
    IRECTList list;
    AddAll(list, {IRECT(10.f, 10.f, 20.f, 20.f), IRECT(10.f, 10.f, 20.f, 20.f), IRECT(10.f, 10.f, 20.f, 20.f)});
    list.Optimize();
    REQUIRE(list.Size() == 1);
    REQUIRE(list.Get(0) == IRECT(10.f, 10.f, 20.f, 20.f));
  }
}

TEST_CASE("IRECTList: Optimize merges rects that exactly cover their union", "[unit][irectlist]")
{
  SECTION("side by side")
  {
    IRECTList list;
    AddAll(list, {IRECT(0.f, 0.f, 10.f, 10.f), IRECT(10.f, 0.f, 20.f, 10.f)});
    list.Optimize();
    REQUIRE(list.Size() == 1);
    REQUIRE(list.Get(0) == IRECT(0.f, 0.f, 20.f, 10.f));
  }

  SECTION("stacked")
  {
    IRECTList list;
    AddAll(list, {IRECT(0.f, 10.f, 10.f, 20.f), IRECT(0.f, 0.f, 10.f, 10.f)});
    list.Optimize();
    REQUIRE(list.Size() == 1);
    REQUIRE(list.Get(0) == IRECT(0.f, 0.f, 10.f, 20.f));
  }

  SECTION("overlapping with the same height")
  {
    IRECTList list;
    AddAll(list, {IRECT(0.f, 0.f, 15.f, 10.f), IRECT(10.f, 0.f, 30.f, 10.f)});
    list.Optimize();
    REQUIRE(list.Size() == 1);
    REQUIRE(list.Get(0) == IRECT(0.f, 0.f, 30.f, 10.f));
  }

  SECTION("a row of cells becomes a single rect")
  {
    IRECTList list;
    for (int i = 0; i < 8; i++)
      list.Add(IRECT::MakeXYWH(static_cast<float>(i * 5), 0.f, 5.f, 10.f));
    list.Optimize();
    REQUIRE(list.Size() == 1);
    REQUIRE(list.Get(0) == IRECT(0.f, 0.f, 40.f, 10.f));
  }

  SECTION("a grid of cells becomes a single rect")
  {
    const IRECT bounds {0.f, 0.f, 40.f, 30.f};
    IRECTList list;
    for (int i = 0; i < 12; i++)
      list.Add(bounds.GetGridCell(i, 3, 4));
    list.Optimize();
    REQUIRE(list.Size() == 1);
    REQUIRE(list.Get(0) == bounds);
  }
}

TEST_CASE("IRECTList: Optimize covers the same area without overlaps", "[unit][irectlist]")
{
  SECTION("separate")
  {
    REQUIRE(CheckOptimize({IRECT(0.f, 0.f, 10.f, 10.f), IRECT(20.f, 20.f, 30.f, 30.f)}) == 2);
  }

  SECTION("contained")
  {
    REQUIRE(CheckOptimize({IRECT(0.f, 0.f, 40.f, 40.f), IRECT(10.f, 10.f, 20.f, 20.f)}) == 1);
  }

  SECTION("corner overlap")
  {
    const int n = CheckOptimize({IRECT(0.f, 0.f, 20.f, 20.f), IRECT(10.f, 10.f, 30.f, 30.f)});
    REQUIRE(n >= 2);
    REQUIRE(n <= 3);
  }

  SECTION("corner overlap, other diagonal")
  {
    const int n = CheckOptimize({IRECT(10.f, 0.f, 30.f, 20.f), IRECT(0.f, 10.f, 20.f, 30.f)});
    REQUIRE(n >= 2);
    REQUIRE(n <= 3);
  }

  SECTION("partial overlap along one edge")
  {
    const int n = CheckOptimize({IRECT(0.f, 0.f, 20.f, 20.f), IRECT(10.f, 5.f, 30.f, 15.f)});
    REQUIRE(n >= 2);
    REQUIRE(n <= 3);
  }

  SECTION("side by side with different heights")
  {
    REQUIRE(CheckOptimize({IRECT(0.f, 0.f, 10.f, 10.f), IRECT(10.f, 0.f, 20.f, 20.f)}) == 2);
  }

  SECTION("three in a staircase")
  {
    CheckOptimize({IRECT(0.f, 0.f, 20.f, 20.f), IRECT(10.f, 10.f, 30.f, 30.f), IRECT(20.f, 20.f, 40.f, 40.f)});
  }

  SECTION("several overlapping the same region")
  {
    CheckOptimize({IRECT(0.f, 0.f, 20.f, 20.f), IRECT(5.f, 5.f, 25.f, 25.f), IRECT(10.f, 10.f, 30.f, 30.f), IRECT(0.f, 15.f, 40.f, 20.f)});
  }
}

// BUG: IGraphicsStructs.h:1967-1993 IRECTList::Split() (used by Optimize() when two rects partly overlap and
// neither can simply be shrunk) assumes that the intersection is at a corner of the rect being split. When the
// intersection spans the rect's full width or height but is in the middle of it, as with the two bars of a
// cross, the part on the far side of the intersection is replaced by a zero-sized rect: for the cross below
// the bottom arm, IRECT(15, 25, 25, 40), is lost, so an area that was dirty is no longer in the list.
TEST_CASE("IRECTList: Optimize covers the same area without overlaps for a cross", "[.][bug]")
{
  CheckOptimize({IRECT(0.f, 15.f, 40.f, 25.f), IRECT(15.f, 0.f, 25.f, 40.f)});
}

// BUG: as above (IGraphicsStructs.h:1967-1993): around one in twenty of these lists of 2-6 overlapping rects
// loses part of its area, or is left with zero-sized rects, when optimized.
TEST_CASE("IRECTList: Optimize covers the same area without overlaps for pseudo-random lists", "[.][bug]")
{
  TestRandom random {20240917u};

  for (int iteration = 0; iteration < 200; iteration++)
  {
    const int nRects = 2 + random.Next(5);
    std::vector<IRECT> rects;

    for (int i = 0; i < nRects; i++)
    {
      // a coarse grid of positions makes shared edges and exact containment common
      const float l = static_cast<float>(random.Next(8) * 4);
      const float t = static_cast<float>(random.Next(8) * 4);
      const float w = static_cast<float>((1 + random.Next(4)) * 4);
      const float h = static_cast<float>((1 + random.Next(4)) * 4);
      rects.push_back(IRECT::MakeXYWH(l, t, w, h));
    }

    INFO("iteration " << iteration);
    for (const IRECT& r : rects)
      UNSCOPED_INFO(Catch::StringMaker<IRECT>::convert(r));

    const int n = CheckOptimize(rects);
    CHECK(n >= 1);
  }
}
