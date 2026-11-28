/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for the value semantics of IBitmap and ISVG (loading images is covered by the integration tests).

#include <string>

#include <catch2/catch_test_macros.hpp>

#include "IGraphicsStructs.h"

using namespace iplug;
using namespace igraphics;

TEST_CASE("IBitmap: default construction is invalid", "[unit][ibitmap]")
{
  IBitmap bitmap;
  REQUIRE_FALSE(bitmap.IsValid());
  REQUIRE(bitmap.GetAPIBitmap() == nullptr);
  REQUIRE(bitmap.W() == 0);
  REQUIRE(bitmap.H() == 0);
  REQUIRE(bitmap.N() == 0);
  REQUIRE_FALSE(bitmap.GetFramesAreHorizontal());
  REQUIRE(std::string(bitmap.GetResourceName().Get()).empty());
}

// a bitmap with no frames must not divide by its number of frames
TEST_CASE("IBitmap: a default constructed bitmap has a frame size of zero", "[unit][ibitmap]")
{
  IBitmap bitmap;
  CHECK(bitmap.FW() == 0);
  CHECK(bitmap.FH() == 0);
  CHECK(IRECT(10.f, 20.f, bitmap) == IRECT(10.f, 20.f, 10.f, 20.f));
}

TEST_CASE("IBitmap: construction from an APIBitmap", "[unit][ibitmap]")
{
  APIBitmap apiBitmap {0, 120, 60, 1.f, 1.f};
  IBitmap bitmap {&apiBitmap, 1, false, "knob.png"};

  REQUIRE(bitmap.IsValid());
  REQUIRE(bitmap.GetAPIBitmap() == &apiBitmap);
  REQUIRE(bitmap.W() == 120);
  REQUIRE(bitmap.H() == 60);
  REQUIRE(bitmap.N() == 1);
  REQUIRE(bitmap.FW() == 120);
  REQUIRE(bitmap.FH() == 60);
  REQUIRE_FALSE(bitmap.GetFramesAreHorizontal());
  REQUIRE(std::string(bitmap.GetResourceName().Get()) == "knob.png");
  REQUIRE(bitmap.GetScale() == 1.f);
  REQUIRE(bitmap.GetDrawScale() == 1.f);
}

TEST_CASE("IBitmap: the resource name defaults to empty", "[unit][ibitmap]")
{
  APIBitmap apiBitmap {0, 120, 60, 1.f, 1.f};
  IBitmap bitmap {&apiBitmap, 1, false};
  REQUIRE(std::string(bitmap.GetResourceName().Get()).empty());
}

TEST_CASE("IBitmap: frame size of multi-frame bitmaps", "[unit][ibitmap]")
{
  APIBitmap apiBitmap {0, 120, 60, 1.f, 1.f};

  SECTION("vertically stacked frames divide the height")
  {
    IBitmap bitmap {&apiBitmap, 4, false};
    REQUIRE(bitmap.N() == 4);
    REQUIRE_FALSE(bitmap.GetFramesAreHorizontal());
    REQUIRE(bitmap.W() == 120);
    REQUIRE(bitmap.H() == 60);
    REQUIRE(bitmap.FW() == 120);
    REQUIRE(bitmap.FH() == 15);
  }

  SECTION("horizontally stacked frames divide the width")
  {
    IBitmap bitmap {&apiBitmap, 4, true};
    REQUIRE(bitmap.N() == 4);
    REQUIRE(bitmap.GetFramesAreHorizontal());
    REQUIRE(bitmap.W() == 120);
    REQUIRE(bitmap.H() == 60);
    REQUIRE(bitmap.FW() == 30);
    REQUIRE(bitmap.FH() == 60);
  }

  SECTION("the frames exactly cover the bitmap")
  {
    for (int n : {1, 2, 3, 5, 6, 10, 12, 60})
    {
      INFO("n " << n);
      IBitmap vertical {&apiBitmap, n, false};
      REQUIRE(vertical.FH() * n == vertical.H());
      REQUIRE(vertical.FW() == vertical.W());

      IBitmap horizontal {&apiBitmap, n, true};
      REQUIRE(horizontal.FW() * n == horizontal.W());
      REQUIRE(horizontal.FH() == horizontal.H());
    }
  }
}

TEST_CASE("IBitmap: size is in points, whatever the scale of the image", "[unit][ibitmap]")
{
  SECTION("@2x")
  {
    APIBitmap apiBitmap {0, 240, 120, 2.f, 1.f};
    IBitmap bitmap {&apiBitmap, 4, false};
    REQUIRE(bitmap.W() == 120);
    REQUIRE(bitmap.H() == 60);
    REQUIRE(bitmap.FW() == 120);
    REQUIRE(bitmap.FH() == 15);
    REQUIRE(bitmap.GetScale() == 2.f);
  }

  SECTION("@3x")
  {
    APIBitmap apiBitmap {0, 360, 180, 3.f, 1.f};
    IBitmap bitmap {&apiBitmap, 1, false};
    REQUIRE(bitmap.W() == 120);
    REQUIRE(bitmap.H() == 60);
    REQUIRE(bitmap.GetScale() == 3.f);
  }

  SECTION("the draw scale is reported but doesn't affect the size")
  {
    APIBitmap apiBitmap {0, 120, 60, 1.f, 1.5f};
    IBitmap bitmap {&apiBitmap, 1, false};
    REQUIRE(bitmap.W() == 120);
    REQUIRE(bitmap.H() == 60);
    REQUIRE(bitmap.GetDrawScale() == 1.5f);
  }
}

TEST_CASE("IBitmap: copies refer to the same APIBitmap", "[unit][ibitmap]")
{
  APIBitmap apiBitmap {0, 120, 60, 1.f, 1.f};
  const IBitmap original {&apiBitmap, 3, true, "strip.png"};

  IBitmap copy = original;
  REQUIRE(copy.IsValid());
  REQUIRE(copy.GetAPIBitmap() == &apiBitmap);
  REQUIRE(copy.W() == 120);
  REQUIRE(copy.H() == 60);
  REQUIRE(copy.N() == 3);
  REQUIRE(copy.FW() == 40);
  REQUIRE(copy.GetFramesAreHorizontal());
  REQUIRE(std::string(copy.GetResourceName().Get()) == "strip.png");

  IBitmap assigned;
  assigned = original;
  REQUIRE(assigned.IsValid());
  REQUIRE(assigned.GetAPIBitmap() == &apiBitmap);
  REQUIRE(assigned.N() == 3);
  REQUIRE(std::string(assigned.GetResourceName().Get()) == "strip.png");

  // assigning a default constructed bitmap invalidates
  // N.B. copying the empty resource name makes WDL_HeapBuf::CopyFrom() (WDL/heapbuf.h:233) call memcpy() with
  // null pointers and a size of zero, which UBSan reports (without failing the test)
  assigned = IBitmap();
  REQUIRE_FALSE(assigned.IsValid());
}

TEST_CASE("ISVG: validity", "[unit][isvg]")
{
  ISVG invalid {nullptr};
  REQUIRE_FALSE(invalid.IsValid());

  NSVGimage image {};
  ISVG valid {&image};
  REQUIRE(valid.IsValid());
}

TEST_CASE("ISVG: size comes from the image", "[unit][isvg]")
{
  NSVGimage image {};
  image.width = 24.f;
  image.height = 32.5f;
  image.shapes = nullptr;

  ISVG svg {&image};
  REQUIRE(svg.W() == 24.f);
  REQUIRE(svg.H() == 32.5f);

  // copies refer to the same image
  const ISVG copy = svg;
  REQUIRE(copy.IsValid());
  REQUIRE(copy.mImage == &image);
  REQUIRE(copy.W() == 24.f);
  REQUIRE(copy.H() == 32.5f);
}
