/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for IText, the text style struct, and TextStyleString().

#include <cstring>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "IGraphicsStructs.h"
#include "UnitTestHelpers.h"

using namespace iplug;
using namespace igraphics;
using namespace iplug_test;

namespace
{
/** Identifies each member of IText, so tests can say which ones a builder is expected to change */
enum ETextField
{
  kFont = 1 << 0,
  kSize = 1 << 1,
  kFGColor = 1 << 2,
  kTEBGColor = 1 << 3,
  kTEFGColor = 1 << 4,
  kAngle = 1 << 5,
  kAlign = 1 << 6,
  kVAlign = 1 << 7
};

/** @return A mask of the ETextFields that differ between a and b */
int DifferingFields(const IText& a, const IText& b)
{
  int fields = 0;

  if (std::strcmp(a.mFont, b.mFont) != 0) fields |= kFont;
  if (a.mSize != b.mSize) fields |= kSize;
  if (!SameColor(a.mFGColor, b.mFGColor)) fields |= kFGColor;
  if (!SameColor(a.mTextEntryBGColor, b.mTextEntryBGColor)) fields |= kTEBGColor;
  if (!SameColor(a.mTextEntryFGColor, b.mTextEntryFGColor)) fields |= kTEFGColor;
  if (a.mAngle != b.mAngle) fields |= kAngle;
  if (a.mAlign != b.mAlign) fields |= kAlign;
  if (a.mVAlign != b.mVAlign) fields |= kVAlign;

  return fields;
}

/** An IText where every member differs from the defaults */
IText MakeNonDefaultText()
{
  return IText(33.f, COLOR_RED, "Custom-Font", EAlign::Far, EVAlign::Bottom, 45.f, COLOR_GREEN, COLOR_BLUE);
}
} // namespace

TEST_CASE("IText: default construction", "[unit][itext]")
{
  IText text;
  REQUIRE(text.mSize == DEFAULT_TEXT_SIZE);
  REQUIRE(SameColor(text.mFGColor, DEFAULT_TEXT_FGCOLOR));
  REQUIRE(std::string(text.mFont) == DEFAULT_FONT);
  REQUIRE(text.mAlign == EAlign::Center);
  REQUIRE(text.mVAlign == EVAlign::Middle);
  REQUIRE(text.mAngle == 0.f);
  REQUIRE(SameColor(text.mTextEntryBGColor, DEFAULT_TEXTENTRY_BGCOLOR));
  REQUIRE(SameColor(text.mTextEntryFGColor, DEFAULT_TEXTENTRY_FGCOLOR));

  REQUIRE(DifferingFields(text, DEFAULT_TEXT) == 0);
}

TEST_CASE("IText: full construction", "[unit][itext]")
{
  IText text {33.f, COLOR_RED, "Custom-Font", EAlign::Far, EVAlign::Bottom, 45.f, COLOR_GREEN, COLOR_BLUE};
  REQUIRE(text.mSize == 33.f);
  REQUIRE(SameColor(text.mFGColor, COLOR_RED));
  REQUIRE(std::string(text.mFont) == "Custom-Font");
  REQUIRE(text.mAlign == EAlign::Far);
  REQUIRE(text.mVAlign == EVAlign::Bottom);
  REQUIRE(text.mAngle == 45.f);
  REQUIRE(SameColor(text.mTextEntryBGColor, COLOR_GREEN));
  REQUIRE(SameColor(text.mTextEntryFGColor, COLOR_BLUE));

  // the helper used below really does change everything
  REQUIRE(DifferingFields(MakeNonDefaultText(), IText()) == (kFont | kSize | kFGColor | kTEBGColor | kTEFGColor | kAngle | kAlign | kVAlign));
}

TEST_CASE("IText: partial construction uses the defaults for the rest", "[unit][itext]")
{
  REQUIRE(DifferingFields(IText(20.f), IText()) == kSize);
  REQUIRE(IText(20.f).mSize == 20.f);

  IText sizeAndColor {20.f, COLOR_RED};
  REQUIRE(DifferingFields(sizeAndColor, IText()) == (kSize | kFGColor));
  REQUIRE(SameColor(sizeAndColor.mFGColor, COLOR_RED));

  IText withFont {20.f, COLOR_RED, "Custom-Font"};
  REQUIRE(DifferingFields(withFont, IText()) == (kSize | kFGColor | kFont));
  REQUIRE(std::string(withFont.mFont) == "Custom-Font");

  IText withAlign {20.f, COLOR_RED, "Custom-Font", EAlign::Near};
  REQUIRE(DifferingFields(withAlign, IText()) == (kSize | kFGColor | kFont | kAlign));
  REQUIRE(withAlign.mAlign == EAlign::Near);

  IText withVAlign {20.f, COLOR_RED, "Custom-Font", EAlign::Near, EVAlign::Top};
  REQUIRE(DifferingFields(withVAlign, IText()) == (kSize | kFGColor | kFont | kAlign | kVAlign));
  REQUIRE(withVAlign.mVAlign == EVAlign::Top);

  IText withAngle {20.f, COLOR_RED, "Custom-Font", EAlign::Near, EVAlign::Top, 90.f};
  REQUIRE(DifferingFields(withAngle, IText()) == (kSize | kFGColor | kFont | kAlign | kVAlign | kAngle));
  REQUIRE(withAngle.mAngle == 90.f);
}

TEST_CASE("IText: a null font name means the default font", "[unit][itext]")
{
  IText text {20.f, COLOR_RED, nullptr};
  REQUIRE(std::string(text.mFont) == DEFAULT_FONT);

  IText fromFontCtor {20.f, static_cast<const char*>(nullptr)};
  REQUIRE(std::string(fromFontCtor.mFont) == DEFAULT_FONT);
}

TEST_CASE("IText: construction from size and vertical alignment", "[unit][itext]")
{
  IText top {20.f, EVAlign::Top};
  REQUIRE(DifferingFields(top, IText()) == (kSize | kVAlign));
  REQUIRE(top.mSize == 20.f);
  REQUIRE(top.mVAlign == EVAlign::Top);

  IText bottomRed {20.f, EVAlign::Bottom, COLOR_RED};
  REQUIRE(DifferingFields(bottomRed, IText()) == (kSize | kVAlign | kFGColor));
  REQUIRE(bottomRed.mVAlign == EVAlign::Bottom);
  REQUIRE(SameColor(bottomRed.mFGColor, COLOR_RED));

  // the default vertical alignment, given explicitly
  REQUIRE(DifferingFields(IText(20.f, EVAlign::Middle), IText()) == kSize);
}

TEST_CASE("IText: construction from size and horizontal alignment", "[unit][itext]")
{
  IText nearAligned {20.f, EAlign::Near};
  REQUIRE(DifferingFields(nearAligned, IText()) == (kSize | kAlign));
  REQUIRE(nearAligned.mSize == 20.f);
  REQUIRE(nearAligned.mAlign == EAlign::Near);

  IText farRed {20.f, EAlign::Far, COLOR_RED};
  REQUIRE(DifferingFields(farRed, IText()) == (kSize | kAlign | kFGColor));
  REQUIRE(farRed.mAlign == EAlign::Far);
  REQUIRE(SameColor(farRed.mFGColor, COLOR_RED));

  // the default horizontal alignment, given explicitly
  REQUIRE(DifferingFields(IText(20.f, EAlign::Center), IText()) == kSize);
}

TEST_CASE("IText: construction from size and font", "[unit][itext]")
{
  IText text {20.f, "Custom-Font"};
  REQUIRE(DifferingFields(text, IText()) == (kSize | kFont));
  REQUIRE(text.mSize == 20.f);
  REQUIRE(std::string(text.mFont) == "Custom-Font");
}

TEST_CASE("IText: font names up to the maximum length are stored", "[unit][itext]")
{
  const std::string longName(FONT_LEN - 1, 'x');

  IText text {20.f, longName.c_str()};
  REQUIRE(std::string(text.mFont) == longName);
  REQUIRE(std::string(IText().WithFont(longName.c_str()).mFont) == longName);
  REQUIRE(std::string(IText(20.f, COLOR_BLACK, longName.c_str()).mFont) == longName);

  REQUIRE(std::strlen(DEFAULT_FONT) < static_cast<size_t>(FONT_LEN));
}

TEST_CASE("IText: With builders change only their own member", "[unit][itext]")
{
  const IText base = MakeNonDefaultText();

  SECTION("WithFGColor")
  {
    const IText text = base.WithFGColor(COLOR_ORANGE);
    REQUIRE(DifferingFields(text, base) == kFGColor);
    REQUIRE(SameColor(text.mFGColor, COLOR_ORANGE));
  }

  SECTION("WithTEColors")
  {
    const IText text = base.WithTEColors(COLOR_ORANGE, COLOR_VIOLET);
    REQUIRE(DifferingFields(text, base) == (kTEBGColor | kTEFGColor));
    REQUIRE(SameColor(text.mTextEntryBGColor, COLOR_ORANGE));
    REQUIRE(SameColor(text.mTextEntryFGColor, COLOR_VIOLET));
  }

  SECTION("WithAlign")
  {
    for (EAlign align : {EAlign::Near, EAlign::Center})
    {
      const IText text = base.WithAlign(align);
      REQUIRE(DifferingFields(text, base) == kAlign);
      REQUIRE(text.mAlign == align);
    }

    REQUIRE(DifferingFields(base.WithAlign(EAlign::Far), base) == 0);
  }

  SECTION("WithVAlign")
  {
    for (EVAlign valign : {EVAlign::Top, EVAlign::Middle})
    {
      const IText text = base.WithVAlign(valign);
      REQUIRE(DifferingFields(text, base) == kVAlign);
      REQUIRE(text.mVAlign == valign);
    }

    REQUIRE(DifferingFields(base.WithVAlign(EVAlign::Bottom), base) == 0);
  }

  SECTION("WithSize")
  {
    const IText text = base.WithSize(7.5f);
    REQUIRE(DifferingFields(text, base) == kSize);
    REQUIRE(text.mSize == 7.5f);
  }

  SECTION("WithAngle")
  {
    const IText text = base.WithAngle(-90.f);
    REQUIRE(DifferingFields(text, base) == kAngle);
    REQUIRE(text.mAngle == -90.f);
  }

  SECTION("WithFont")
  {
    const IText text = base.WithFont("Another-Font");
    REQUIRE(DifferingFields(text, base) == kFont);
    REQUIRE(std::string(text.mFont) == "Another-Font");

    // a shorter name replaces a longer one completely
    REQUIRE(std::string(text.WithFont("A").mFont) == "A");

    // null means the default font
    REQUIRE(std::string(base.WithFont(nullptr).mFont) == DEFAULT_FONT);
  }

  SECTION("the original is not modified")
  {
    (void) base.WithFGColor(COLOR_ORANGE).WithSize(1.f).WithFont("Another-Font").WithAngle(1.f);
    REQUIRE(DifferingFields(base, MakeNonDefaultText()) == 0);
  }
}

TEST_CASE("IText: With builders can be chained", "[unit][itext]")
{
  const IText text = IText().WithSize(33.f).WithFGColor(COLOR_RED).WithFont("Custom-Font").WithAlign(EAlign::Far).WithVAlign(EVAlign::Bottom).WithAngle(45.f).WithTEColors(COLOR_GREEN, COLOR_BLUE);
  REQUIRE(DifferingFields(text, MakeNonDefaultText()) == 0);
}

TEST_CASE("IText: is copyable", "[unit][itext]")
{
  const IText original = MakeNonDefaultText();
  IText copy = original;
  REQUIRE(DifferingFields(copy, original) == 0);

  IText assigned;
  assigned = original;
  REQUIRE(DifferingFields(assigned, original) == 0);

  // the font name is copied, not shared
  REQUIRE(static_cast<const void*>(copy.mFont) != static_cast<const void*>(original.mFont));
}

TEST_CASE("IText: default label and value text styles", "[unit][itext]")
{
  // labels sit at the top of their area and are a little larger than values, which sit at the bottom
  REQUIRE(DifferingFields(DEFAULT_LABEL_TEXT, IText()) == (kSize | kVAlign));
  REQUIRE(DEFAULT_LABEL_TEXT.mVAlign == EVAlign::Top);
  REQUIRE(DEFAULT_LABEL_TEXT.mSize > DEFAULT_TEXT_SIZE);

  REQUIRE(DifferingFields(DEFAULT_VALUE_TEXT, IText()) == kVAlign);
  REQUIRE(DEFAULT_VALUE_TEXT.mVAlign == EVAlign::Bottom);
  REQUIRE(DEFAULT_VALUE_TEXT.mSize == DEFAULT_TEXT_SIZE);
}

TEST_CASE("IText: TextStyleString", "[unit][itext]")
{
  REQUIRE(std::string(TextStyleString(ETextStyle::Normal)) == "Regular");
  REQUIRE(std::string(TextStyleString(ETextStyle::Bold)) == "Bold");
  REQUIRE(std::string(TextStyleString(ETextStyle::Italic)) == "Italic");
}
