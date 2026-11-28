/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for IGraphicsConstants.h: the invariants that other code relies on (string tables, win32 values, limits).

#include <cstring>
#include <set>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "IGraphicsStructs.h"
#include "IGraphicsConstants.h"

using namespace iplug;
using namespace igraphics;

TEST_CASE("IGraphicsConstants: numeric defaults are sensible", "[unit][igraphicsconstants]")
{
  REQUIRE(DEFAULT_FPS > 0);
  REQUIRE(IDLE_TICKS > 0);
  REQUIRE(DEFAULT_ANIMATION_DURATION > 0);
  REQUIRE(MAX_IMG_SCALE >= 1);
  REQUIRE(DEFAULT_TEXT_ENTRY_LEN > 0);
  REQUIRE(DEFAULT_GEARING > 0.);
  REQUIRE(DEFAULT_TEXT_SIZE > 0.f);

  // the default draw scale of 1 is inside the allowed range
  REQUIRE(DEFAULT_MIN_DRAW_SCALE > 0.);
  REQUIRE(DEFAULT_MIN_DRAW_SCALE <= 1.);
  REQUIRE(DEFAULT_MAX_DRAW_SCALE >= 1.);
  REQUIRE(DEFAULT_MIN_DRAW_SCALE < DEFAULT_MAX_DRAW_SCALE);

  REQUIRE(GRAYED_ALPHA > 0.f);
  REQUIRE(GRAYED_ALPHA < 1.f);
}

TEST_CASE("IGraphicsConstants: the default font name fits in an IText", "[unit][igraphicsconstants]")
{
  REQUIRE(DEFAULT_FONT != nullptr);
  REQUIRE(std::strlen(DEFAULT_FONT) > 0);
  REQUIRE(std::strlen(DEFAULT_FONT) < static_cast<size_t>(FONT_LEN));
  REQUIRE(sizeof(IText().mFont) == static_cast<size_t>(FONT_LEN));
}

TEST_CASE("IGraphicsConstants: the default blend is source-over", "[unit][igraphicsconstants]")
{
  REQUIRE(EBlend::Default == EBlend::SrcOver);

  // all the blend modes are distinct
  std::set<int> values;
  for (EBlend blend : {EBlend::SrcOver, EBlend::SrcIn, EBlend::SrcOut, EBlend::SrcAtop, EBlend::DstOver, EBlend::DstIn, EBlend::DstOut, EBlend::DstAtop, EBlend::Add, EBlend::XOR})
    values.insert(static_cast<int>(blend));

  REQUIRE(values.size() == 10);
}

TEST_CASE("IGraphicsConstants: alignment names match the enums", "[unit][igraphicsconstants]")
{
  REQUIRE(std::string(kEAlignStrs[static_cast<int>(EAlign::Near)]) == "Near");
  REQUIRE(std::string(kEAlignStrs[static_cast<int>(EAlign::Center)]) == "Center");
  REQUIRE(std::string(kEAlignStrs[static_cast<int>(EAlign::Far)]) == "Far");

  REQUIRE(std::string(kEVAlignStrs[static_cast<int>(EVAlign::Top)]) == "Top");
  REQUIRE(std::string(kEVAlignStrs[static_cast<int>(EVAlign::Middle)]) == "Middle");
  REQUIRE(std::string(kEVAlignStrs[static_cast<int>(EVAlign::Bottom)]) == "Bottom");
}

TEST_CASE("IGraphicsConstants: gesture names match the enum", "[unit][igraphicsconstants]")
{
  REQUIRE(std::string(kGestureTypeStrs[static_cast<int>(EGestureType::Unknown)]) == "Unknown");
  REQUIRE(std::string(kGestureTypeStrs[static_cast<int>(EGestureType::DoubleTap)]) == "DoubleTap");
  REQUIRE(std::string(kGestureTypeStrs[static_cast<int>(EGestureType::TripleTap)]) == "TripleTap");
  REQUIRE(std::string(kGestureTypeStrs[static_cast<int>(EGestureType::LongPress1)]) == "LongPress1");
  REQUIRE(std::string(kGestureTypeStrs[static_cast<int>(EGestureType::LongPress2)]) == "LongPress2");
  REQUIRE(std::string(kGestureTypeStrs[static_cast<int>(EGestureType::SwipeLeft)]) == "SwipeLeft");
  REQUIRE(std::string(kGestureTypeStrs[static_cast<int>(EGestureType::SwipeRight)]) == "SwipeRight");
  REQUIRE(std::string(kGestureTypeStrs[static_cast<int>(EGestureType::SwipeUp)]) == "SwipeUp");
  REQUIRE(std::string(kGestureTypeStrs[static_cast<int>(EGestureType::SwipeDown)]) == "SwipeDown");
  REQUIRE(std::string(kGestureTypeStrs[static_cast<int>(EGestureType::Pinch)]) == "Pinch");
  REQUIRE(std::string(kGestureTypeStrs[static_cast<int>(EGestureType::Rotate)]) == "Rotate");
  REQUIRE(std::string(kGestureTypeStrs[static_cast<int>(EGestureType::Pan)]) == "Pan");

  // Pan is the last gesture type, and the table has an entry for each
  REQUIRE(sizeof(kGestureTypeStrs) / sizeof(kGestureTypeStrs[0]) == static_cast<size_t>(EGestureType::Pan) + 1);
}

TEST_CASE("IGraphicsConstants: message box types match the win32 values", "[unit][igraphicsconstants]")
{
  // MB_OK, MB_OKCANCEL, MB_YESNOCANCEL, MB_YESNO, MB_RETRYCANCEL
  REQUIRE(kMB_OK == 0);
  REQUIRE(kMB_OKCANCEL == 1);
  REQUIRE(kMB_YESNOCANCEL == 3);
  REQUIRE(kMB_YESNO == 4);
  REQUIRE(kMB_RETRYCANCEL == 5);
}

TEST_CASE("IGraphicsConstants: message box results match the win32 values and their names", "[unit][igraphicsconstants]")
{
  // IDOK, IDCANCEL, IDABORT, IDRETRY, IDIGNORE, IDYES, IDNO
  REQUIRE(kNoResult == 0);
  REQUIRE(kOK == 1);
  REQUIRE(kCANCEL == 2);
  REQUIRE(kABORT == 3);
  REQUIRE(kRETRY == 4);
  REQUIRE(kIGNORE == 5);
  REQUIRE(kYES == 6);
  REQUIRE(kNO == 7);

  REQUIRE(std::string(kMessageResultStrs[kNoResult]).empty());
  REQUIRE(std::string(kMessageResultStrs[kOK]) == "OK");
  REQUIRE(std::string(kMessageResultStrs[kCANCEL]) == "CANCEL");
  REQUIRE(std::string(kMessageResultStrs[kABORT]) == "ABORT");
  REQUIRE(std::string(kMessageResultStrs[kRETRY]) == "RETRY");
  REQUIRE(std::string(kMessageResultStrs[kIGNORE]) == "IGNORE");
  REQUIRE(std::string(kMessageResultStrs[kYES]) == "YES");
  REQUIRE(std::string(kMessageResultStrs[kNO]) == "NO");
}
