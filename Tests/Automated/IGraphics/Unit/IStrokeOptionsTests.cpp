/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for IFillOptions and IStrokeOptions (including IStrokeOptions::DashOptions).

#include <catch2/catch_test_macros.hpp>

#include "IGraphicsStructs.h"

using namespace iplug;
using namespace igraphics;

TEST_CASE("IFillOptions: defaults", "[unit][ifilloptions]")
{
  IFillOptions options;
  REQUIRE(options.mFillRule == EFillRule::Winding);
  REQUIRE_FALSE(options.mPreserve);
}

TEST_CASE("IFillOptions: construction", "[unit][ifilloptions]")
{
  IFillOptions preserve {true};
  REQUIRE(preserve.mPreserve);
  REQUIRE(preserve.mFillRule == EFillRule::Winding);

  IFillOptions evenOdd {false, EFillRule::EvenOdd};
  REQUIRE_FALSE(evenOdd.mPreserve);
  REQUIRE(evenOdd.mFillRule == EFillRule::EvenOdd);

  IFillOptions both {true, EFillRule::EvenOdd};
  REQUIRE(both.mPreserve);
  REQUIRE(both.mFillRule == EFillRule::EvenOdd);
}

TEST_CASE("IStrokeOptions: defaults", "[unit][istrokeoptions]")
{
  IStrokeOptions options;
  REQUIRE(options.mMiterLimit == 10.f);
  REQUIRE_FALSE(options.mPreserve);
  REQUIRE(options.mCapOption == ELineCap::Butt);
  REQUIRE(options.mJoinOption == ELineJoin::Miter);

  // not dashed
  REQUIRE(options.mDash.GetCount() == 0);
  REQUIRE(options.mDash.GetOffset() == 0.f);
  REQUIRE(options.mDash.GetArray() != nullptr);
}

TEST_CASE("IStrokeOptions: members can be set and copied", "[unit][istrokeoptions]")
{
  float dashes[2] = {4.f, 2.f};

  IStrokeOptions options;
  options.mMiterLimit = 2.f;
  options.mPreserve = true;
  options.mCapOption = ELineCap::Round;
  options.mJoinOption = ELineJoin::Bevel;
  options.mDash.SetDash(dashes, 1.f, 2);

  const IStrokeOptions copy = options;
  REQUIRE(copy.mMiterLimit == 2.f);
  REQUIRE(copy.mPreserve);
  REQUIRE(copy.mCapOption == ELineCap::Round);
  REQUIRE(copy.mJoinOption == ELineJoin::Bevel);
  REQUIRE(copy.mDash.GetCount() == 2);
  REQUIRE(copy.mDash.GetOffset() == 1.f);
  REQUIRE(copy.mDash.GetArray()[0] == 4.f);
  REQUIRE(copy.mDash.GetArray()[1] == 2.f);

  // the copy has its own dash array
  REQUIRE(copy.mDash.GetArray() != options.mDash.GetArray());
}

TEST_CASE("IStrokeOptions::DashOptions: default construction is empty", "[unit][istrokeoptions]")
{
  IStrokeOptions::DashOptions dash;
  REQUIRE(dash.GetCount() == 0);
  REQUIRE(dash.GetOffset() == 0.f);

  // the unused entries are zeroed
  for (int i = 0; i < 8; i++)
    REQUIRE(dash.GetArray()[i] == 0.f);
}

TEST_CASE("IStrokeOptions::DashOptions: construction from an array", "[unit][istrokeoptions]")
{
  float dashes[3] = {5.f, 2.5f, 1.f};
  IStrokeOptions::DashOptions dash {dashes, 0.5f, 3};

  REQUIRE(dash.GetCount() == 3);
  REQUIRE(dash.GetOffset() == 0.5f);
  REQUIRE(dash.GetArray()[0] == 5.f);
  REQUIRE(dash.GetArray()[1] == 2.5f);
  REQUIRE(dash.GetArray()[2] == 1.f);

  // the values are copied
  REQUIRE(dash.GetArray() != dashes);
  dashes[0] = 99.f;
  REQUIRE(dash.GetArray()[0] == 5.f);
}

TEST_CASE("IStrokeOptions::DashOptions: SetDash", "[unit][istrokeoptions]")
{
  IStrokeOptions::DashOptions dash;

  SECTION("a single value")
  {
    float dashes[1] = {3.f};
    dash.SetDash(dashes, 0.f, 1);
    REQUIRE(dash.GetCount() == 1);
    REQUIRE(dash.GetOffset() == 0.f);
    REQUIRE(dash.GetArray()[0] == 3.f);
  }

  SECTION("the maximum of 8 values")
  {
    float dashes[8] = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f, 7.f, 8.f};
    dash.SetDash(dashes, 12.f, 8);
    REQUIRE(dash.GetCount() == 8);
    REQUIRE(dash.GetOffset() == 12.f);

    for (int i = 0; i < 8; i++)
      REQUIRE(dash.GetArray()[i] == static_cast<float>(i + 1));
  }

  SECTION("only count values are read from the array")
  {
    float dashes[4] = {1.f, 2.f, 3.f, 4.f};
    dash.SetDash(dashes, 0.f, 2);
    REQUIRE(dash.GetCount() == 2);
    REQUIRE(dash.GetArray()[0] == 1.f);
    REQUIRE(dash.GetArray()[1] == 2.f);
    REQUIRE(dash.GetArray()[2] == 0.f);
  }

  SECTION("replacing a longer pattern with a shorter one")
  {
    float longDashes[4] = {1.f, 2.f, 3.f, 4.f};
    float shortDashes[2] = {9.f, 8.f};
    dash.SetDash(longDashes, 1.f, 4);
    dash.SetDash(shortDashes, 2.f, 2);
    REQUIRE(dash.GetCount() == 2);
    REQUIRE(dash.GetOffset() == 2.f);
    REQUIRE(dash.GetArray()[0] == 9.f);
    REQUIRE(dash.GetArray()[1] == 8.f);
  }

  SECTION("a count of zero removes the dashes")
  {
    float dashes[2] = {9.f, 8.f};
    dash.SetDash(dashes, 1.f, 2);
    dash.SetDash(nullptr, 0.f, 0);
    REQUIRE(dash.GetCount() == 0);
    REQUIRE(dash.GetOffset() == 0.f);
  }

  SECTION("a negative offset is kept")
  {
    float dashes[2] = {9.f, 8.f};
    dash.SetDash(dashes, -3.5f, 2);
    REQUIRE(dash.GetOffset() == -3.5f);
  }
}
