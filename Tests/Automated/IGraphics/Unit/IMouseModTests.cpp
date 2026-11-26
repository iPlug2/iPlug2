/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for the input event structs: IMouseMod, IMouseInfo, IGestureInfo and IKeyPress.

#include <string>

#include <catch2/catch_test_macros.hpp>

#include "IGraphicsStructs.h"

using namespace iplug;
using namespace igraphics;

TEST_CASE("IMouseMod: default construction has nothing pressed", "[unit][imousemod]")
{
  IMouseMod mod;
  REQUIRE_FALSE(mod.L);
  REQUIRE_FALSE(mod.R);
  REQUIRE_FALSE(mod.S);
  REQUIRE_FALSE(mod.C);
  REQUIRE_FALSE(mod.A);
  REQUIRE(mod.touchID == 0);
  REQUIRE(mod.touchRadius == 0.f);
  REQUIRE_FALSE(mod.IsTouch());
}

TEST_CASE("IMouseMod: construction sets each modifier independently", "[unit][imousemod]")
{
  SECTION("left button")
  {
    IMouseMod mod {true};
    REQUIRE(mod.L);
    REQUIRE_FALSE(mod.R);
    REQUIRE_FALSE(mod.S);
    REQUIRE_FALSE(mod.C);
    REQUIRE_FALSE(mod.A);
  }

  SECTION("right button")
  {
    IMouseMod mod {false, true};
    REQUIRE_FALSE(mod.L);
    REQUIRE(mod.R);
    REQUIRE_FALSE(mod.S);
    REQUIRE_FALSE(mod.C);
    REQUIRE_FALSE(mod.A);
  }

  SECTION("shift")
  {
    IMouseMod mod {false, false, true};
    REQUIRE_FALSE(mod.L);
    REQUIRE_FALSE(mod.R);
    REQUIRE(mod.S);
    REQUIRE_FALSE(mod.C);
    REQUIRE_FALSE(mod.A);
  }

  SECTION("control")
  {
    IMouseMod mod {false, false, false, true};
    REQUIRE_FALSE(mod.L);
    REQUIRE_FALSE(mod.R);
    REQUIRE_FALSE(mod.S);
    REQUIRE(mod.C);
    REQUIRE_FALSE(mod.A);
  }

  SECTION("alt")
  {
    IMouseMod mod {false, false, false, false, true};
    REQUIRE_FALSE(mod.L);
    REQUIRE_FALSE(mod.R);
    REQUIRE_FALSE(mod.S);
    REQUIRE_FALSE(mod.C);
    REQUIRE(mod.A);
  }

  SECTION("everything")
  {
    IMouseMod mod {true, true, true, true, true};
    REQUIRE(mod.L);
    REQUIRE(mod.R);
    REQUIRE(mod.S);
    REQUIRE(mod.C);
    REQUIRE(mod.A);
    REQUIRE_FALSE(mod.IsTouch());
  }
}

TEST_CASE("IMouseMod: touches", "[unit][imousemod]")
{
  IMouseMod touch {true, false, false, false, false, static_cast<ITouchID>(42)};
  REQUIRE(touch.touchID == 42);
  REQUIRE(touch.IsTouch());
  REQUIRE(touch.L);
  REQUIRE(touch.touchRadius == 0.f);

  touch.touchRadius = 12.5f;
  REQUIRE(touch.touchRadius == 12.5f);

  // a touch ID of zero means a mouse event
  IMouseMod mouse {true, false, false, false, false, static_cast<ITouchID>(0)};
  REQUIRE_FALSE(mouse.IsTouch());

  mouse.touchID = 1;
  REQUIRE(mouse.IsTouch());
}

TEST_CASE("IMouseMod: is copyable", "[unit][imousemod]")
{
  IMouseMod original {true, false, true, false, true, static_cast<ITouchID>(7)};
  original.touchRadius = 3.f;

  const IMouseMod copy = original;
  REQUIRE(copy.L);
  REQUIRE_FALSE(copy.R);
  REQUIRE(copy.S);
  REQUIRE_FALSE(copy.C);
  REQUIRE(copy.A);
  REQUIRE(copy.touchID == 7);
  REQUIRE(copy.touchRadius == 3.f);
}

TEST_CASE("IMouseInfo: default construction", "[unit][imouseinfo]")
{
  IMouseInfo info;
  REQUIRE(info.x == 0.f);
  REQUIRE(info.y == 0.f);
  REQUIRE(info.dX == 0.f);
  REQUIRE(info.dY == 0.f);
  REQUIRE_FALSE(info.ms.L);
  REQUIRE_FALSE(info.ms.R);
  REQUIRE_FALSE(info.ms.S);
  REQUIRE_FALSE(info.ms.C);
  REQUIRE_FALSE(info.ms.A);
  REQUIRE_FALSE(info.ms.IsTouch());
}

TEST_CASE("IMouseInfo: holds a position, a delta and modifiers", "[unit][imouseinfo]")
{
  IMouseInfo info;
  info.x = 10.5f;
  info.y = 20.5f;
  info.dX = -1.f;
  info.dY = 2.f;
  info.ms = IMouseMod(true, false, true);

  const IMouseInfo copy = info;
  REQUIRE(copy.x == 10.5f);
  REQUIRE(copy.y == 20.5f);
  REQUIRE(copy.dX == -1.f);
  REQUIRE(copy.dY == 2.f);
  REQUIRE(copy.ms.L);
  REQUIRE_FALSE(copy.ms.R);
  REQUIRE(copy.ms.S);
}

TEST_CASE("IGestureInfo: default construction", "[unit][igestureinfo]")
{
  IGestureInfo info;
  REQUIRE(info.x == 0.f);
  REQUIRE(info.y == 0.f);
  REQUIRE(info.scale == 0.f);
  REQUIRE(info.velocity == 0.f);
  REQUIRE(info.angle == 0.f);
  REQUIRE(info.state == EGestureState::Unknown);
  REQUIRE(info.type == EGestureType::Unknown);
}

TEST_CASE("IGestureInfo: holds the details of a gesture", "[unit][igestureinfo]")
{
  IGestureInfo info;
  info.x = 1.f;
  info.y = 2.f;
  info.scale = 1.5f;
  info.velocity = 0.25f;
  info.angle = 90.f;
  info.state = EGestureState::InProcess;
  info.type = EGestureType::Pinch;

  const IGestureInfo copy = info;
  REQUIRE(copy.x == 1.f);
  REQUIRE(copy.y == 2.f);
  REQUIRE(copy.scale == 1.5f);
  REQUIRE(copy.velocity == 0.25f);
  REQUIRE(copy.angle == 90.f);
  REQUIRE(copy.state == EGestureState::InProcess);
  REQUIRE(copy.type == EGestureType::Pinch);
}

TEST_CASE("IKeyPress: construction", "[unit][ikeypress]")
{
  SECTION("a plain key")
  {
    IKeyPress key {"a", 65};
    REQUIRE(std::string(key.utf8) == "a");
    REQUIRE(key.VK == 65);
    REQUIRE_FALSE(key.S);
    REQUIRE_FALSE(key.C);
    REQUIRE_FALSE(key.A);
  }

  SECTION("modifiers are set independently")
  {
    IKeyPress shift {"A", 65, true};
    REQUIRE(shift.S);
    REQUIRE_FALSE(shift.C);
    REQUIRE_FALSE(shift.A);

    IKeyPress control {"a", 65, false, true};
    REQUIRE_FALSE(control.S);
    REQUIRE(control.C);
    REQUIRE_FALSE(control.A);

    IKeyPress alt {"a", 65, false, false, true};
    REQUIRE_FALSE(alt.S);
    REQUIRE_FALSE(alt.C);
    REQUIRE(alt.A);

    IKeyPress all {"a", 65, true, true, true};
    REQUIRE(all.S);
    REQUIRE(all.C);
    REQUIRE(all.A);
  }

  SECTION("a key with no character")
  {
    IKeyPress key {"", 0x25};
    REQUIRE(std::string(key.utf8).empty());
    REQUIRE(key.VK == 0x25);
  }

  SECTION("multi-byte UTF-8 characters")
  {
    // U+00E9 (2 bytes), U+20AC (3 bytes) and U+1F600 (4 bytes, the longest that fits)
    IKeyPress twoBytes {"\xC3\xA9", 0};
    REQUIRE(std::string(twoBytes.utf8) == "\xC3\xA9");

    IKeyPress threeBytes {"\xE2\x82\xAC", 0};
    REQUIRE(std::string(threeBytes.utf8) == "\xE2\x82\xAC");

    IKeyPress fourBytes {"\xF0\x9F\x98\x80", 0};
    REQUIRE(std::string(fourBytes.utf8) == "\xF0\x9F\x98\x80");
    REQUIRE(fourBytes.utf8[4] == '\0');
  }

  SECTION("is copyable")
  {
    const IKeyPress original {"z", 90, true, false, true};
    const IKeyPress copy = original;
    REQUIRE(std::string(copy.utf8) == "z");
    REQUIRE(copy.VK == 90);
    REQUIRE(copy.S);
    REQUIRE_FALSE(copy.C);
    REQUIRE(copy.A);
  }
}
