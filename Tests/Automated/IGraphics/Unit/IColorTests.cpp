/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for IColor, its conversions and the COLOR_ constants.

#include <algorithm>
#include <cstdlib>
#include <cstring>

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
struct HSLA
{
  float h, s, l, a;
};

HSLA GetHSLA(const IColor& color)
{
  HSLA hsla {-1.f, -1.f, -1.f, -1.f};
  color.GetHSLA(hsla.h, hsla.s, hsla.l, hsla.a);
  return hsla;
}

IColor RoundTripHSLA(const IColor& color)
{
  const HSLA hsla = GetHSLA(color);
  return IColor::FromHSLA(hsla.h, hsla.s, hsla.l, hsla.a);
}

std::string ColorCodeStr(const IColor& color, bool withAlpha)
{
  WDL_String str;
  color.ToColorCodeStr(str, withAlpha);
  return std::string(str.Get());
}

const float kTolerance = 1e-4f;
} // namespace

TEST_CASE("IColor: default construction is opaque black", "[unit][icolor]")
{
  IColor c;
  REQUIRE(c.A == 255);
  REQUIRE(c.R == 0);
  REQUIRE(c.G == 0);
  REQUIRE(c.B == 0);
}

TEST_CASE("IColor: construction takes alpha first", "[unit][icolor]")
{
  IColor c {1, 2, 3, 4};
  REQUIRE(c.A == 1);
  REQUIRE(c.R == 2);
  REQUIRE(c.G == 3);
  REQUIRE(c.B == 4);

  IColor alphaOnly {128};
  REQUIRE(alphaOnly.A == 128);
  REQUIRE(alphaOnly.R == 0);
  REQUIRE(alphaOnly.G == 0);
  REQUIRE(alphaOnly.B == 0);

  IColor partial {10, 20, 30};
  REQUIRE(partial.A == 10);
  REQUIRE(partial.R == 20);
  REQUIRE(partial.G == 30);
  REQUIRE(partial.B == 0);
}

TEST_CASE("IColor: equality operators", "[unit][icolor]")
{
  IColor a {1, 2, 3, 4};
  IColor same {1, 2, 3, 4};

  REQUIRE((a == same));
  REQUIRE_FALSE((a != same));
  REQUIRE((a == a));

  IColor differentA {9, 2, 3, 4};
  IColor differentR {1, 9, 3, 4};
  IColor differentG {1, 2, 9, 4};
  IColor differentB {1, 2, 3, 9};

  for (const IColor& other : {differentA, differentR, differentG, differentB})
  {
    REQUIRE_FALSE((a == other));
    REQUIRE((a != other));
  }
}

TEST_CASE("IColor: Set", "[unit][icolor]")
{
  IColor c {1, 2, 3, 4};
  c.Set(10, 20, 30, 40);
  REQUIRE(SameColor(c, IColor(10, 20, 30, 40)));

  // the defaults are the same as the constructor's
  c.Set();
  REQUIRE(SameColor(c, IColor()));
  c.Set(77);
  REQUIRE(SameColor(c, IColor(77, 0, 0, 0)));
}

TEST_CASE("IColor: Empty is true only when all parts are zero", "[unit][icolor]")
{
  REQUIRE(IColor(0, 0, 0, 0).Empty());
  REQUIRE(COLOR_TRANSPARENT.Empty());

  REQUIRE_FALSE(IColor().Empty());
  REQUIRE_FALSE(IColor(1, 0, 0, 0).Empty());
  REQUIRE_FALSE(IColor(0, 1, 0, 0).Empty());
  REQUIRE_FALSE(IColor(0, 0, 1, 0).Empty());
  REQUIRE_FALSE(IColor(0, 0, 0, 1).Empty());
  REQUIRE_FALSE(COLOR_BLACK.Empty());
}

TEST_CASE("IColor: Clamp limits every part to 0-255", "[unit][icolor]")
{
  IColor c {300, -1, 256, -1000};
  c.Clamp();
  REQUIRE(SameColor(c, IColor(255, 0, 255, 0)));

  IColor inRange {0, 255, 128, 1};
  inRange.Clamp();
  REQUIRE(SameColor(inRange, IColor(0, 255, 128, 1)));

  IColor mixed {-5, 100, 1000, 200};
  mixed.Clamp();
  REQUIRE(SameColor(mixed, IColor(0, 100, 255, 200)));
}

TEST_CASE("IColor: Randomise gives parts in range and sets the alpha", "[unit][icolor]")
{
  std::srand(99);

  IColor c;
  bool allInRange = true;
  bool anyDifferent = false;
  IColor first;
  first.Randomise();

  for (int i = 0; i < 500; i++)
  {
    c.Randomise();
    allInRange = allInRange && c.A == 255 && c.R >= 0 && c.R <= 255 && c.G >= 0 && c.G <= 255 && c.B >= 0 && c.B <= 255;
    anyDifferent = anyDifferent || !SameColor(c, first);
  }

  REQUIRE(allInRange);
  REQUIRE(anyDifferent);

  c.Randomise(42);
  REQUIRE(c.A == 42);
  c.Randomise(0);
  REQUIRE(c.A == 0);
}

TEST_CASE("IColor: GetRandomColor", "[unit][icolor]")
{
  std::srand(7);

  bool allInRange = true;
  bool allOpaque = true;
  bool anyTranslucent = false;

  for (int i = 0; i < 500; i++)
  {
    const IColor opaque = IColor::GetRandomColor();
    allOpaque = allOpaque && opaque.A == 255;
    allInRange = allInRange && opaque.R >= 0 && opaque.R <= 255 && opaque.G >= 0 && opaque.G <= 255 && opaque.B >= 0 && opaque.B <= 255;

    const IColor explicitOpaque = IColor::GetRandomColor(false);
    allOpaque = allOpaque && explicitOpaque.A == 255;

    const IColor withAlpha = IColor::GetRandomColor(true);
    allInRange = allInRange && withAlpha.A >= 0 && withAlpha.A <= 255 && withAlpha.R >= 0 && withAlpha.R <= 255
              && withAlpha.G >= 0 && withAlpha.G <= 255 && withAlpha.B >= 0 && withAlpha.B <= 255;
    anyTranslucent = anyTranslucent || withAlpha.A != 255;
  }

  REQUIRE(allInRange);
  REQUIRE(allOpaque);
  REQUIRE(anyTranslucent);
}

TEST_CASE("IColor: SetOpacity and WithOpacity", "[unit][icolor]")
{
  IColor c {255, 10, 20, 30};

  c.SetOpacity(0.f);
  REQUIRE(SameColor(c, IColor(0, 10, 20, 30)));
  c.SetOpacity(1.f);
  REQUIRE(SameColor(c, IColor(255, 10, 20, 30)));
  c.SetOpacity(0.2f);
  REQUIRE(SameColor(c, IColor(51, 10, 20, 30)));

  SECTION("out of range values are clipped")
  {
    c.SetOpacity(2.f);
    REQUIRE(c.A == 255);
    c.SetOpacity(-1.f);
    REQUIRE(c.A == 0);
  }

  SECTION("WithOpacity returns a copy and leaves the original alone")
  {
    const IColor original {255, 10, 20, 30};
    REQUIRE(SameColor(original.WithOpacity(0.f), IColor(0, 10, 20, 30)));
    REQUIRE(SameColor(original.WithOpacity(1.f), original));
    REQUIRE(SameColor(original.WithOpacity(0.2f), IColor(51, 10, 20, 30)));
    REQUIRE(SameColor(original.WithOpacity(5.f), original));
    REQUIRE(SameColor(original.WithOpacity(-5.f), IColor(0, 10, 20, 30)));
    REQUIRE(SameColor(original, IColor(255, 10, 20, 30)));
  }

  SECTION("the alpha is proportional to the opacity")
  {
    for (int i = 0; i <= 20; i++)
    {
      const float opacity = static_cast<float>(i) / 20.f;
      const IColor result = COLOR_RED.WithOpacity(opacity);
      INFO("opacity " << opacity);
      CHECK(result.A == Approx(opacity * 255.f).margin(1.));
      CHECK(result.R == 255);
    }
  }
}

TEST_CASE("IColor: Contrast and WithContrast", "[unit][icolor]")
{
  SECTION("positive values lighten each of R, G and B by the same amount")
  {
    IColor c {200, 10, 20, 30};
    c.Contrast(0.2f);
    REQUIRE(SameColor(c, IColor(200, 61, 71, 81)));
  }

  SECTION("negative values darken")
  {
    IColor c {200, 100, 110, 120};
    c.Contrast(-0.2f);
    REQUIRE(SameColor(c, IColor(200, 49, 59, 69)));
  }

  SECTION("results are clipped to 0-255")
  {
    IColor c {200, 10, 128, 250};
    c.Contrast(0.6f);
    REQUIRE(SameColor(c, IColor(200, 163, 255, 255)));

    IColor d {200, 10, 128, 250};
    d.Contrast(-0.6f);
    REQUIRE(SameColor(d, IColor(200, 0, 0, 97)));

    REQUIRE(SameColor(COLOR_GRAY.WithContrast(1.f), COLOR_WHITE));
    REQUIRE(SameColor(COLOR_GRAY.WithContrast(-1.f), COLOR_BLACK));
  }

  SECTION("zero does nothing")
  {
    REQUIRE(SameColor(COLOR_ORANGE.WithContrast(0.f), COLOR_ORANGE));
  }

  SECTION("WithContrast returns a copy and leaves the original and the alpha alone")
  {
    const IColor original {123, 10, 20, 30};
    REQUIRE(SameColor(original.WithContrast(0.2f), IColor(123, 61, 71, 81)));
    REQUIRE(SameColor(original, IColor(123, 10, 20, 30)));
  }
}

TEST_CASE("IColor: GetRGBf and GetRGBAf", "[unit][icolor]")
{
  const IColor c {51, 255, 0, 102};

  float rgb[3] = {-1.f, -1.f, -1.f};
  c.GetRGBf(rgb);
  REQUIRE(rgb[0] == Approx(1.f));
  REQUIRE(rgb[1] == Approx(0.f));
  REQUIRE(rgb[2] == Approx(0.4f));

  float rgba[4] = {-1.f, -1.f, -1.f, -1.f};
  c.GetRGBAf(rgba);
  REQUIRE(rgba[0] == Approx(1.f));
  REQUIRE(rgba[1] == Approx(0.f));
  REQUIRE(rgba[2] == Approx(0.4f));
  REQUIRE(rgba[3] == Approx(0.2f));

  float white[4];
  COLOR_WHITE.GetRGBAf(white);
  for (float v : white)
    REQUIRE(v == 1.f);

  float transparent[4];
  COLOR_TRANSPARENT.GetRGBAf(transparent);
  for (float v : transparent)
    REQUIRE(v == 0.f);
}

TEST_CASE("IColor: FromRGBf and FromRGBAf", "[unit][icolor]")
{
  float rgb[3] = {1.f, 0.f, 0.5f};
  REQUIRE(NearColor(IColor::FromRGBf(rgb), IColor(255, 255, 0, 127), 1));
  REQUIRE(IColor::FromRGBf(rgb).A == 255);

  float rgba[4] = {0.f, 1.f, 0.5f, 0.25f};
  REQUIRE(NearColor(IColor::FromRGBAf(rgba), IColor(63, 0, 255, 127), 1));

  float white[4] = {1.f, 1.f, 1.f, 1.f};
  REQUIRE(SameColor(IColor::FromRGBf(white), COLOR_WHITE));
  REQUIRE(SameColor(IColor::FromRGBAf(white), COLOR_WHITE));

  float zeros[4] = {0.f, 0.f, 0.f, 0.f};
  REQUIRE(SameColor(IColor::FromRGBf(zeros), COLOR_BLACK));
  REQUIRE(SameColor(IColor::FromRGBAf(zeros), COLOR_TRANSPARENT));
}

TEST_CASE("IColor: converting to floats and back is within one step", "[unit][icolor]")
{
  bool allNear = true;

  for (int i = 0; i < 256; i++)
  {
    const IColor original {255 - i, i, (i * 7) % 256, (i * 13) % 256};

    float rgba[4];
    original.GetRGBAf(rgba);
    allNear = allNear && NearColor(IColor::FromRGBAf(rgba), original, 1);

    float rgb[3];
    original.GetRGBf(rgb);
    allNear = allNear && NearColor(IColor::FromRGBf(rgb), IColor(255, original.R, original.G, original.B), 1);
  }

  REQUIRE(allNear);
}

TEST_CASE("IColor: GetLuminosity is the mean of the largest and smallest of R, G and B", "[unit][icolor]")
{
  REQUIRE(COLOR_BLACK.GetLuminosity() == 0);
  REQUIRE(COLOR_WHITE.GetLuminosity() == 255);
  REQUIRE(COLOR_GRAY.GetLuminosity() == 127);
  REQUIRE(COLOR_RED.GetLuminosity() == 127);
  REQUIRE(COLOR_GREEN.GetLuminosity() == 127);
  REQUIRE(COLOR_BLUE.GetLuminosity() == 127);
  REQUIRE(COLOR_YELLOW.GetLuminosity() == 127);

  REQUIRE(IColor(255, 10, 20, 30).GetLuminosity() == 20);
  REQUIRE(IColor(255, 30, 20, 10).GetLuminosity() == 20);
  REQUIRE(IColor(255, 20, 30, 10).GetLuminosity() == 20);
  REQUIRE(IColor(255, 20, 10, 30).GetLuminosity() == 20);
  REQUIRE(IColor(255, 200, 100, 100).GetLuminosity() == 150);
  REQUIRE(IColor(255, 100, 100, 200).GetLuminosity() == 150);

  // alpha is ignored
  REQUIRE(IColor(0, 200, 100, 100).GetLuminosity() == 150);
}

TEST_CASE("IColor: GetHSLA for primary and secondary colors", "[unit][icolor]")
{
  struct Expected
  {
    IColor color;
    float h;
  };

  const Expected cases[] = {
    {COLOR_RED, 0.f},
    {COLOR_YELLOW, 1.f / 6.f},
    {COLOR_GREEN, 2.f / 6.f},
    {IColor(255, 0, 255, 255), 3.f / 6.f},
    {COLOR_BLUE, 4.f / 6.f},
    {IColor(255, 255, 0, 255), 5.f / 6.f},
  };

  for (const Expected& expected : cases)
  {
    INFO(Catch::StringMaker<IColor>::convert(expected.color));
    const HSLA hsla = GetHSLA(expected.color);
    CHECK(hsla.h == Approx(expected.h).margin(kTolerance));
    CHECK(hsla.s == Approx(1.f).margin(kTolerance));
    CHECK(hsla.l == Approx(0.5f).margin(kTolerance));
    CHECK(hsla.a == Approx(1.f).margin(kTolerance));
  }
}

TEST_CASE("IColor: GetHSLA for grays has no hue or saturation", "[unit][icolor]")
{
  for (int v : {0, 1, 64, 127, 128, 200, 254, 255})
  {
    INFO("gray level " << v);
    const HSLA hsla = GetHSLA(IColor(255, v, v, v));
    CHECK(hsla.h == 0.f);
    CHECK(hsla.s == 0.f);
    CHECK(hsla.l == Approx(static_cast<float>(v) / 255.f).margin(kTolerance));
    CHECK(hsla.a == Approx(1.f).margin(kTolerance));
  }
}

TEST_CASE("IColor: GetHSLA for dark colors", "[unit][icolor]")
{
  SECTION("dark red")
  {
    const HSLA hsla = GetHSLA(IColor(255, 102, 0, 0));
    CHECK(hsla.h == Approx(0.f).margin(kTolerance));
    CHECK(hsla.s == Approx(1.f).margin(kTolerance));
    CHECK(hsla.l == Approx(0.2f).margin(kTolerance));
  }

  SECTION("muted dark green")
  {
    // max 0.4, min 0.2: l = 0.3, s = 0.2 / 0.6
    const HSLA hsla = GetHSLA(IColor(255, 51, 102, 51));
    CHECK(hsla.h == Approx(1.f / 3.f).margin(kTolerance));
    CHECK(hsla.s == Approx(1.f / 3.f).margin(kTolerance));
    CHECK(hsla.l == Approx(0.3f).margin(kTolerance));
  }

  SECTION("a hue between primaries")
  {
    // orange-ish: hue 30 degrees
    const HSLA hsla = GetHSLA(IColor(255, 204, 102, 0));
    CHECK(hsla.h == Approx(30.f / 360.f).margin(kTolerance));
    CHECK(hsla.s == Approx(1.f).margin(kTolerance));
    CHECK(hsla.l == Approx(0.4f).margin(kTolerance));
  }

  SECTION("a hue that wraps below red")
  {
    // max R, B > G: hue 330 degrees
    const HSLA hsla = GetHSLA(IColor(255, 204, 0, 102));
    CHECK(hsla.h == Approx(330.f / 360.f).margin(kTolerance));
    CHECK(hsla.s == Approx(1.f).margin(kTolerance));
    CHECK(hsla.l == Approx(0.4f).margin(kTolerance));
  }
}

TEST_CASE("IColor: GetHSLA alpha is independent of the color", "[unit][icolor]")
{
  CHECK(GetHSLA(IColor(0, 255, 0, 0)).a == 0.f);
  CHECK(GetHSLA(IColor(51, 255, 0, 0)).a == Approx(0.2f).margin(kTolerance));
  CHECK(GetHSLA(IColor(255, 255, 0, 0)).a == 1.f);
  CHECK(GetHSLA(IColor(51, 80, 80, 80)).a == Approx(0.2f).margin(kTolerance));

  // ...and the color is independent of the alpha
  const HSLA opaque = GetHSLA(IColor(255, 204, 102, 0));
  const HSLA translucent = GetHSLA(IColor(10, 204, 102, 0));
  CHECK(opaque.h == translucent.h);
  CHECK(opaque.s == translucent.s);
  CHECK(opaque.l == translucent.l);
}

TEST_CASE("IColor: GetHSLA outputs are normalized", "[unit][icolor]")
{
  bool allNormalized = true;

  for (int r = 0; r <= 255; r += 15)
  {
    for (int g = 0; g <= 255; g += 15)
    {
      for (int b = 0; b <= 255; b += 15)
      {
        const HSLA hsla = GetHSLA(IColor(r, r, g, b));
        allNormalized = allNormalized && hsla.h >= 0.f && hsla.h <= 1.f && hsla.s >= 0.f && hsla.s <= 1.f + kTolerance
                     && hsla.l >= 0.f && hsla.l <= 1.f && hsla.a >= 0.f && hsla.a <= 1.f;
      }
    }
  }

  REQUIRE(allNormalized);
}

TEST_CASE("IColor: GetHSLA saturation of light colors", "[unit][icolor]")
{
  SECTION("pink is a fully saturated light red")
  {
    const HSLA hsla = GetHSLA(IColor(255, 255, 153, 153));
    CHECK(hsla.h == Approx(0.f).margin(kTolerance));
    CHECK(hsla.s == Approx(1.f).margin(kTolerance));
    CHECK(hsla.l == Approx(0.8f).margin(kTolerance));
  }

  SECTION("a muted light blue")
  {
    // max 0.8, min 0.6: l = 0.7, s = 0.2 / (2 - 1.4)
    const HSLA hsla = GetHSLA(IColor(255, 153, 153, 204));
    CHECK(hsla.h == Approx(2.f / 3.f).margin(kTolerance));
    CHECK(hsla.s == Approx(1.f / 3.f).margin(kTolerance));
    CHECK(hsla.l == Approx(0.7f).margin(kTolerance));
  }

  SECTION("every color survives a round trip through HSLA")
  {
    int nDifferent = 0;

    for (int r = 0; r <= 255; r += 51)
    {
      for (int g = 0; g <= 255; g += 51)
      {
        for (int b = 0; b <= 255; b += 51)
        {
          const IColor original {255, r, g, b};

          if (!NearColor(RoundTripHSLA(original), original, 1))
            nDifferent++;
        }
      }
    }

    CHECK(nDifferent == 0);
  }
}

TEST_CASE("IColor: FromHSLA for primary and secondary colors", "[unit][icolor]")
{
  CHECK(NearColor(IColor::FromHSLA(0.f, 1.f, 0.5f), COLOR_RED, 1));
  CHECK(NearColor(IColor::FromHSLA(1.f / 6.f, 1.f, 0.5f), COLOR_YELLOW, 1));
  CHECK(NearColor(IColor::FromHSLA(2.f / 6.f, 1.f, 0.5f), COLOR_GREEN, 1));
  CHECK(NearColor(IColor::FromHSLA(3.f / 6.f, 1.f, 0.5f), IColor(255, 0, 255, 255), 1));
  CHECK(NearColor(IColor::FromHSLA(4.f / 6.f, 1.f, 0.5f), COLOR_BLUE, 1));
  CHECK(NearColor(IColor::FromHSLA(5.f / 6.f, 1.f, 0.5f), IColor(255, 255, 0, 255), 1));
}

TEST_CASE("IColor: FromHSLA lightness and saturation", "[unit][icolor]")
{
  SECTION("zero lightness is black and full lightness is white, whatever the hue and saturation")
  {
    for (float h : {0.f, 0.2f, 0.5f, 0.9f})
    {
      for (float s : {0.f, 0.5f, 1.f})
      {
        CHECK(SameColor(IColor::FromHSLA(h, s, 0.f), COLOR_BLACK));
        CHECK(SameColor(IColor::FromHSLA(h, s, 1.f), COLOR_WHITE));
      }
    }
  }

  SECTION("zero saturation is gray, whatever the hue")
  {
    for (float h : {0.f, 0.2f, 0.5f, 0.9f})
    {
      const IColor c = IColor::FromHSLA(h, 0.f, 0.4f);
      CHECK(NearColor(c, IColor(255, 102, 102, 102), 1));
      CHECK(c.R == c.G);
      CHECK(c.G == c.B);
    }
  }

  SECTION("dark, light and muted colors")
  {
    CHECK(NearColor(IColor::FromHSLA(0.f, 1.f, 0.2f), IColor(255, 102, 0, 0), 1));
    CHECK(NearColor(IColor::FromHSLA(0.f, 1.f, 0.8f), IColor(255, 255, 153, 153), 1));
    CHECK(NearColor(IColor::FromHSLA(1.f / 3.f, 1.f / 3.f, 0.3f), IColor(255, 51, 102, 51), 1));
    CHECK(NearColor(IColor::FromHSLA(2.f / 3.f, 1.f / 3.f, 0.7f), IColor(255, 153, 153, 204), 1));
    CHECK(NearColor(IColor::FromHSLA(30.f / 360.f, 1.f, 0.4f), IColor(255, 204, 102, 0), 1));
    CHECK(NearColor(IColor::FromHSLA(330.f / 360.f, 1.f, 0.4f), IColor(255, 204, 0, 102), 1));
  }

  SECTION("saturation and lightness are clipped to 0-1")
  {
    CHECK(SameColor(IColor::FromHSLA(0.25f, 2.f, 0.5f), IColor::FromHSLA(0.25f, 1.f, 0.5f)));
    CHECK(SameColor(IColor::FromHSLA(0.25f, -1.f, 0.5f), IColor::FromHSLA(0.25f, 0.f, 0.5f)));
    CHECK(SameColor(IColor::FromHSLA(0.25f, 0.5f, 3.f), COLOR_WHITE));
    CHECK(SameColor(IColor::FromHSLA(0.25f, 0.5f, -3.f), COLOR_BLACK));
  }
}

TEST_CASE("IColor: FromHSLA hue wraps around", "[unit][icolor]")
{
  CHECK(NearColor(IColor::FromHSLA(1.f, 1.f, 0.5f), COLOR_RED, 1));
  CHECK(NearColor(IColor::FromHSLA(2.f, 1.f, 0.5f), COLOR_RED, 1));
  CHECK(NearColor(IColor::FromHSLA(1.5f, 1.f, 0.5f), IColor::FromHSLA(0.5f, 1.f, 0.5f), 1));
  CHECK(NearColor(IColor::FromHSLA(-0.5f, 1.f, 0.5f), IColor::FromHSLA(0.5f, 1.f, 0.5f), 1));
  CHECK(NearColor(IColor::FromHSLA(-0.25f, 1.f, 0.5f), IColor::FromHSLA(0.75f, 1.f, 0.5f), 1));
  CHECK(NearColor(IColor::FromHSLA(3.25f, 0.5f, 0.25f), IColor::FromHSLA(0.25f, 0.5f, 0.25f), 1));
}

TEST_CASE("IColor: FromHSLA alpha", "[unit][icolor]")
{
  CHECK(IColor::FromHSLA(0.f, 1.f, 0.5f).A == 255);
  CHECK(IColor::FromHSLA(0.f, 1.f, 0.5f, 1.f).A == 255);
  CHECK(IColor::FromHSLA(0.f, 1.f, 0.5f, 0.f).A == 0);
  CHECK(IColor::FromHSLA(0.f, 1.f, 0.5f, 0.2f).A == 51);

  // the alpha doesn't affect the color
  const IColor opaque = IColor::FromHSLA(0.3f, 0.6f, 0.4f, 1.f);
  const IColor translucent = IColor::FromHSLA(0.3f, 0.6f, 0.4f, 0.2f);
  CHECK(opaque.R == translucent.R);
  CHECK(opaque.G == translucent.G);
  CHECK(opaque.B == translucent.B);
}

TEST_CASE("IColor: HSLA round trip for grays, dark colors and fully saturated colors", "[unit][icolor]")
{
  // N.B. light colors are excluded here because of the saturation bug in GetHSLA(), see the [bug] test above
  int nTested = 0;
  int nDifferent = 0;

  for (int a = 0; a <= 255; a += 85)
  {
    for (int r = 0; r <= 255; r += 17)
    {
      for (int g = 0; g <= 255; g += 17)
      {
        for (int b = 0; b <= 255; b += 17)
        {
          const int maxPart = std::max(r, std::max(g, b));
          const int minPart = std::min(r, std::min(g, b));
          const bool isGray = maxPart == minPart;
          const bool isDark = (maxPart + minPart) < 255;
          const bool isPure = minPart == 0;

          if (!(isGray || isDark || isPure))
            continue;

          const IColor original {a, r, g, b};
          nTested++;

          if (!NearColor(RoundTripHSLA(original), original, 1))
          {
            nDifferent++;
            UNSCOPED_INFO(Catch::StringMaker<IColor>::convert(original) << " -> " << Catch::StringMaker<IColor>::convert(RoundTripHSLA(original)));
          }
        }
      }
    }
  }

  CHECK(nTested > 1000);
  CHECK(nDifferent == 0);
}

TEST_CASE("IColor: FromColorCode", "[unit][icolor]")
{
  REQUIRE(SameColor(IColor::FromColorCode(0x55a6ff), IColor(255, 0x55, 0xa6, 0xff)));
  REQUIRE(SameColor(IColor::FromColorCode(0x55a6ff, 0x88), IColor(0x88, 0x55, 0xa6, 0xff)));
  REQUIRE(SameColor(IColor::FromColorCode(0x000000), COLOR_BLACK));
  REQUIRE(SameColor(IColor::FromColorCode(0xffffff), COLOR_WHITE));
  REQUIRE(SameColor(IColor::FromColorCode(0xff0000), COLOR_RED));
  REQUIRE(SameColor(IColor::FromColorCode(0x00ff00), COLOR_GREEN));
  REQUIRE(SameColor(IColor::FromColorCode(0x0000ff), COLOR_BLUE));
  REQUIRE(SameColor(IColor::FromColorCode(0x000000, 0), COLOR_TRANSPARENT));
}

TEST_CASE("IColor: ToColorCode", "[unit][icolor]")
{
  REQUIRE(IColor(255, 0x55, 0xa6, 0xff).ToColorCode() == 0x55a6ff);
  REQUIRE(COLOR_BLACK.ToColorCode() == 0x000000);
  REQUIRE(COLOR_WHITE.ToColorCode() == 0xffffff);
  REQUIRE(COLOR_RED.ToColorCode() == 0xff0000);
  REQUIRE(COLOR_GREEN.ToColorCode() == 0x00ff00);
  REQUIRE(COLOR_BLUE.ToColorCode() == 0x0000ff);

  // alpha is not included
  REQUIRE(IColor(0, 0x12, 0x34, 0x56).ToColorCode() == 0x123456);
  REQUIRE(IColor(128, 0x12, 0x34, 0x56).ToColorCode() == 0x123456);
}

TEST_CASE("IColor: color code round trip", "[unit][icolor]")
{
  bool allMatch = true;

  for (int r = 0; r <= 255; r += 17)
  {
    for (int g = 0; g <= 255; g += 17)
    {
      for (int b = 0; b <= 255; b += 17)
      {
        const IColor original {255, r, g, b};
        allMatch = allMatch && SameColor(IColor::FromColorCode(original.ToColorCode()), original);

        const IColor translucent {b, r, g, b};
        allMatch = allMatch && SameColor(IColor::FromColorCode(translucent.ToColorCode(), translucent.A), translucent);
      }
    }
  }

  REQUIRE(allMatch);

  for (int code : {0x000000, 0x000001, 0x000100, 0x010000, 0x123456, 0xabcdef, 0xffffff})
    REQUIRE(IColor::FromColorCode(code).ToColorCode() == code);
}

TEST_CASE("IColor: ToColorCodeStr", "[unit][icolor]")
{
  SECTION("with alpha (the default) the alpha comes last")
  {
    WDL_String str;
    IColor(0x88, 0x55, 0xa6, 0xff).ToColorCodeStr(str);
    REQUIRE(std::string(str.Get()) == "#55a6ff88");

    REQUIRE(ColorCodeStr(IColor(0x88, 0x55, 0xa6, 0xff), true) == "#55a6ff88");
    REQUIRE(ColorCodeStr(COLOR_WHITE, true) == "#ffffffff");
    REQUIRE(ColorCodeStr(COLOR_BLACK, true) == "#000000ff");
    REQUIRE(ColorCodeStr(COLOR_TRANSPARENT, true) == "#00000000");
    REQUIRE(ColorCodeStr(COLOR_RED, true) == "#ff0000ff");
  }

  SECTION("without alpha")
  {
    REQUIRE(ColorCodeStr(IColor(0x88, 0x55, 0xa6, 0xff), false) == "#55a6ff");
    REQUIRE(ColorCodeStr(COLOR_WHITE, false) == "#ffffff");
    REQUIRE(ColorCodeStr(COLOR_BLACK, false) == "#000000");
    REQUIRE(ColorCodeStr(COLOR_GREEN, false) == "#00ff00");
    REQUIRE(ColorCodeStr(COLOR_BLUE, false) == "#0000ff");
  }

  SECTION("small values are zero padded")
  {
    REQUIRE(ColorCodeStr(IColor(1, 2, 3, 4), true) == "#02030401");
    REQUIRE(ColorCodeStr(IColor(1, 2, 3, 4), false) == "#020304");
  }

  SECTION("replaces any existing contents of the string")
  {
    WDL_String str {"existing contents that are longer than a color code"};
    COLOR_RED.ToColorCodeStr(str, false);
    REQUIRE(std::string(str.Get()) == "#ff0000");
  }
}

TEST_CASE("IColor: FromColorCodeStr", "[unit][icolor]")
{
  REQUIRE(SameColor(IColor::FromColorCodeStr("#ff38a2"), IColor(255, 0xff, 0x38, 0xa2)));
  REQUIRE(SameColor(IColor::FromColorCodeStr("#000000"), COLOR_BLACK));
  REQUIRE(SameColor(IColor::FromColorCodeStr("#ffffff"), COLOR_WHITE));
  REQUIRE(SameColor(IColor::FromColorCodeStr("#ff0000"), COLOR_RED));
  REQUIRE(SameColor(IColor::FromColorCodeStr("#00ff00"), COLOR_GREEN));
  REQUIRE(SameColor(IColor::FromColorCodeStr("#0000ff"), COLOR_BLUE));
  REQUIRE(SameColor(IColor::FromColorCodeStr("#010203"), IColor(255, 1, 2, 3)));

  // upper case hex digits
  REQUIRE(SameColor(IColor::FromColorCodeStr("#FF38A2"), IColor(255, 0xff, 0x38, 0xa2)));
  REQUIRE(SameColor(IColor::FromColorCodeStr("#aBcDeF"), IColor(255, 0xab, 0xcd, 0xef)));
}

TEST_CASE("IColor: color code string round trip without alpha", "[unit][icolor]")
{
  bool allMatch = true;

  for (int r = 0; r <= 255; r += 17)
  {
    for (int g = 0; g <= 255; g += 17)
    {
      for (int b = 0; b <= 255; b += 17)
      {
        const IColor original {255, r, g, b};
        const std::string str = ColorCodeStr(original, false);
        allMatch = allMatch && str.size() == 7 && SameColor(IColor::FromColorCodeStr(str.c_str()), original);
      }
    }
  }

  REQUIRE(allMatch);
}

TEST_CASE("IColor: color code string round trip with alpha", "[unit][icolor]")
{
  CHECK(SameColor(IColor::FromColorCodeStr("#55a6ff88"), IColor(0x88, 0x55, 0xa6, 0xff)));
  CHECK(SameColor(IColor::FromColorCodeStr("#ff0000ff"), COLOR_RED));
  CHECK(SameColor(IColor::FromColorCodeStr("#00000000"), COLOR_TRANSPARENT));

  for (const IColor& original : {IColor(0x12, 0x34, 0x56, 0x78), COLOR_ORANGE, COLOR_TRANSLUCENT, COLOR_BLACK_DROP_SHADOW})
  {
    const std::string str = ColorCodeStr(original, true);
    INFO(str);
    CHECK(SameColor(IColor::FromColorCodeStr(str.c_str()), original));
  }
}

TEST_CASE("IColor: LinearInterpolateBetween", "[unit][icolor]")
{
  const IColor start {0, 0, 100, 250};
  const IColor dest {200, 100, 0, 50};

  REQUIRE(SameColor(IColor::LinearInterpolateBetween(start, dest, 0.f), start));
  REQUIRE(SameColor(IColor::LinearInterpolateBetween(start, dest, 1.f), dest));
  REQUIRE(SameColor(IColor::LinearInterpolateBetween(start, dest, 0.5f), IColor(100, 50, 50, 150)));
  REQUIRE(SameColor(IColor::LinearInterpolateBetween(start, dest, 0.25f), IColor(50, 25, 75, 200)));
  REQUIRE(SameColor(IColor::LinearInterpolateBetween(start, dest, 0.75f), IColor(150, 75, 25, 100)));

  REQUIRE(SameColor(IColor::LinearInterpolateBetween(start, start, 0.3f), start));
  REQUIRE(SameColor(IColor::LinearInterpolateBetween(COLOR_BLACK, COLOR_WHITE, 0.f), COLOR_BLACK));
  REQUIRE(SameColor(IColor::LinearInterpolateBetween(COLOR_BLACK, COLOR_WHITE, 1.f), COLOR_WHITE));
  REQUIRE(SameColor(IColor::LinearInterpolateBetween(COLOR_TRANSPARENT, COLOR_WHITE, 0.2f), IColor(51, 51, 51, 51)));

  SECTION("every part stays between the start and the destination")
  {
    for (int i = 0; i <= 100; i++)
    {
      const float progress = static_cast<float>(i) / 100.f;
      const IColor c = IColor::LinearInterpolateBetween(start, dest, progress);
      INFO("progress " << progress);
      CHECK(c.A >= 0);
      CHECK(c.A <= 200);
      CHECK(c.R >= 0);
      CHECK(c.R <= 100);
      CHECK(c.G >= 0);
      CHECK(c.G <= 100);
      CHECK(c.B >= 50);
      CHECK(c.B <= 250);
      CHECK(c.A == Approx(200.f * progress).margin(1.));
      CHECK(c.B == Approx(250.f - 200.f * progress).margin(1.));
    }
  }
}

TEST_CASE("IColor: COLOR_ constants", "[unit][icolor]")
{
  REQUIRE(SameColor(COLOR_TRANSPARENT, IColor(0, 0, 0, 0)));
  REQUIRE(SameColor(COLOR_BLACK, IColor(255, 0, 0, 0)));
  REQUIRE(SameColor(COLOR_WHITE, IColor(255, 255, 255, 255)));
  REQUIRE(SameColor(COLOR_RED, IColor(255, 255, 0, 0)));
  REQUIRE(SameColor(COLOR_GREEN, IColor(255, 0, 255, 0)));
  REQUIRE(SameColor(COLOR_BLUE, IColor(255, 0, 0, 255)));
  REQUIRE(SameColor(COLOR_YELLOW, IColor(255, 255, 255, 0)));
  REQUIRE(SameColor(COLOR_ORANGE, IColor(255, 255, 127, 0)));
  REQUIRE(SameColor(COLOR_INDIGO, IColor(255, 75, 0, 130)));
  REQUIRE(SameColor(COLOR_VIOLET, IColor(255, 148, 0, 211)));

  // the grays have no hue, and are ordered from dark to light
  for (const IColor& gray : {COLOR_DARK_GRAY, COLOR_GRAY, COLOR_MID_GRAY, COLOR_LIGHT_GRAY})
  {
    REQUIRE(gray.A == 255);
    REQUIRE(gray.R == gray.G);
    REQUIRE(gray.G == gray.B);
  }

  REQUIRE(COLOR_BLACK.R < COLOR_DARK_GRAY.R);
  REQUIRE(COLOR_DARK_GRAY.R < COLOR_GRAY.R);
  REQUIRE(COLOR_GRAY.R < COLOR_MID_GRAY.R);
  REQUIRE(COLOR_MID_GRAY.R < COLOR_LIGHT_GRAY.R);
  REQUIRE(COLOR_LIGHT_GRAY.R < COLOR_WHITE.R);

  // the translucent constants are black with a partial alpha
  for (const IColor& translucent : {COLOR_TRANSLUCENT, COLOR_BLACK_DROP_SHADOW})
  {
    REQUIRE(translucent.A > 0);
    REQUIRE(translucent.A < 255);
    REQUIRE(translucent.R == 0);
    REQUIRE(translucent.G == 0);
    REQUIRE(translucent.B == 0);
  }
}

TEST_CASE("IColor: GetRainbow", "[unit][icolor]")
{
  REQUIRE(SameColor(GetRainbow(0), COLOR_RED));
  REQUIRE(SameColor(GetRainbow(1), COLOR_ORANGE));
  REQUIRE(SameColor(GetRainbow(2), COLOR_YELLOW));
  REQUIRE(SameColor(GetRainbow(3), COLOR_GREEN));
  REQUIRE(SameColor(GetRainbow(4), COLOR_BLUE));
  REQUIRE(SameColor(GetRainbow(5), COLOR_INDIGO));
  REQUIRE(SameColor(GetRainbow(6), COLOR_VIOLET));
}

TEST_CASE("IColor: default color constants", "[unit][icolor]")
{
  REQUIRE(SameColor(DEFAULT_GRAPHICS_BGCOLOR, COLOR_GRAY));
  REQUIRE(SameColor(DEFAULT_BGCOLOR, COLOR_TRANSPARENT));
  REQUIRE(SameColor(DEFAULT_FGCOLOR, COLOR_MID_GRAY));
  REQUIRE(SameColor(DEFAULT_PRCOLOR, COLOR_LIGHT_GRAY));
  REQUIRE(SameColor(DEFAULT_FRCOLOR, COLOR_DARK_GRAY));
  REQUIRE(SameColor(DEFAULT_HLCOLOR, COLOR_TRANSLUCENT));
  REQUIRE(SameColor(DEFAULT_X1COLOR, COLOR_BLACK));
  REQUIRE(SameColor(DEFAULT_X2COLOR, COLOR_GREEN));
  REQUIRE(SameColor(DEFAULT_X3COLOR, COLOR_BLUE));
  REQUIRE(SameColor(DEFAULT_TEXT_FGCOLOR, COLOR_BLACK));
  REQUIRE(SameColor(DEFAULT_TEXTENTRY_BGCOLOR, COLOR_WHITE));
  REQUIRE(SameColor(DEFAULT_TEXTENTRY_FGCOLOR, COLOR_BLACK));

  // the default shadow is translucent black
  REQUIRE(DEFAULT_SHCOLOR.A > 0);
  REQUIRE(DEFAULT_SHCOLOR.A < 255);
  REQUIRE(DEFAULT_SHCOLOR.ToColorCode() == 0);
}
