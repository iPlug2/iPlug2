/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for IBlend, BlendWeight() and the BLEND_ constants.

#include <catch2/catch_test_macros.hpp>

#include "IGraphicsStructs.h"

using namespace iplug;
using namespace igraphics;

TEST_CASE("IBlend: default construction is source-over at full weight", "[unit][iblend]")
{
  IBlend blend;
  REQUIRE(blend.mMethod == EBlend::Default);
  REQUIRE(blend.mMethod == EBlend::SrcOver);
  REQUIRE(blend.mWeight == 1.f);
}

TEST_CASE("IBlend: construction with a method and weight", "[unit][iblend]")
{
  IBlend blend {EBlend::Add, 0.5f};
  REQUIRE(blend.mMethod == EBlend::Add);
  REQUIRE(blend.mWeight == 0.5f);

  // the weight defaults to 1
  REQUIRE(IBlend(EBlend::XOR).mMethod == EBlend::XOR);
  REQUIRE(IBlend(EBlend::XOR).mWeight == 1.f);

  for (EBlend method : {EBlend::SrcOver, EBlend::SrcIn, EBlend::SrcOut, EBlend::SrcAtop, EBlend::DstOver, EBlend::DstIn, EBlend::DstOut, EBlend::DstAtop, EBlend::Add, EBlend::XOR})
    REQUIRE(IBlend(method, 0.25f).mMethod == method);
}

TEST_CASE("IBlend: the weight is clipped to 0-1", "[unit][iblend]")
{
  REQUIRE(IBlend(EBlend::Default, 0.f).mWeight == 0.f);
  REQUIRE(IBlend(EBlend::Default, 1.f).mWeight == 1.f);
  REQUIRE(IBlend(EBlend::Default, 0.75f).mWeight == 0.75f);
  REQUIRE(IBlend(EBlend::Default, 1.5f).mWeight == 1.f);
  REQUIRE(IBlend(EBlend::Default, 100.f).mWeight == 1.f);
  REQUIRE(IBlend(EBlend::Default, -0.5f).mWeight == 0.f);
  REQUIRE(IBlend(EBlend::Default, -100.f).mWeight == 0.f);
}

TEST_CASE("IBlend: BlendWeight", "[unit][iblend]")
{
  // no blend means fully opaque
  REQUIRE(BlendWeight(nullptr) == 1.f);

  IBlend half {EBlend::Default, 0.5f};
  REQUIRE(BlendWeight(&half) == 0.5f);

  IBlend none {EBlend::Add, 0.f};
  REQUIRE(BlendWeight(&none) == 0.f);

  IBlend clipped {EBlend::Default, 3.f};
  REQUIRE(BlendWeight(&clipped) == 1.f);

  REQUIRE(BlendWeight(&BLEND_25) == 0.25f);
}

TEST_CASE("IBlend: BLEND_ constants", "[unit][iblend]")
{
  REQUIRE(BLEND_75.mWeight == 0.75f);
  REQUIRE(BLEND_50.mWeight == 0.5f);
  REQUIRE(BLEND_25.mWeight == 0.25f);
  REQUIRE(BLEND_10.mWeight == 0.1f);
  REQUIRE(BLEND_05.mWeight == 0.05f);
  REQUIRE(BLEND_01.mWeight == 0.01f);

  for (const IBlend& blend : {BLEND_75, BLEND_50, BLEND_25, BLEND_10, BLEND_05, BLEND_01})
    REQUIRE(blend.mMethod == EBlend::SrcOver);

  REQUIRE(BLEND_DST_IN.mMethod == EBlend::DstIn);
  REQUIRE(BLEND_DST_IN.mWeight == 1.f);
  REQUIRE(BLEND_DST_OVER.mMethod == EBlend::DstOver);
  REQUIRE(BLEND_DST_OVER.mWeight == 1.f);
}
