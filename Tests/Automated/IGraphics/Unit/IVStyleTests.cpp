/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Unit tests for IVColorSpec and IVStyle, which theme the IVControls, and the EVColor helpers.

#include <cstring>
#include <set>
#include <string>

#include <catch2/catch_test_macros.hpp>

#include "IGraphicsStructs.h"
#include "UnitTestHelpers.h"

using namespace iplug;
using namespace igraphics;
using namespace iplug_test;

namespace
{
/** Identifies each member of IVStyle, so tests can say which ones a builder is expected to change */
enum EStyleField
{
  kHideCursor = 1 << 0,
  kShowLabel = 1 << 1,
  kShowValue = 1 << 2,
  kDrawFrame = 1 << 3,
  kDrawShadows = 1 << 4,
  kEmboss = 1 << 5,
  kRoundness = 1 << 6,
  kFrameThickness = 1 << 7,
  kShadowOffset = 1 << 8,
  kWidgetFrac = 1 << 9,
  kAngle = 1 << 10,
  kColorSpec = 1 << 11,
  kLabelText = 1 << 12,
  kValueText = 1 << 13,
  kLabelOrientation = 1 << 14
};

bool SameText(const IText& a, const IText& b)
{
  return std::strcmp(a.mFont, b.mFont) == 0 && a.mSize == b.mSize && SameColor(a.mFGColor, b.mFGColor)
      && SameColor(a.mTextEntryBGColor, b.mTextEntryBGColor) && SameColor(a.mTextEntryFGColor, b.mTextEntryFGColor)
      && a.mAngle == b.mAngle && a.mAlign == b.mAlign && a.mVAlign == b.mVAlign;
}

bool SameColorSpec(const IVColorSpec& a, const IVColorSpec& b)
{
  for (int i = 0; i < kNumVColors; i++)
  {
    if (!SameColor(a.mColors[i], b.mColors[i]))
      return false;
  }

  return true;
}

/** @return A mask of the EStyleFields that differ between a and b */
int DifferingFields(const IVStyle& a, const IVStyle& b)
{
  int fields = 0;

  if (a.hideCursor != b.hideCursor) fields |= kHideCursor;
  if (a.showLabel != b.showLabel) fields |= kShowLabel;
  if (a.showValue != b.showValue) fields |= kShowValue;
  if (a.drawFrame != b.drawFrame) fields |= kDrawFrame;
  if (a.drawShadows != b.drawShadows) fields |= kDrawShadows;
  if (a.emboss != b.emboss) fields |= kEmboss;
  if (a.roundness != b.roundness) fields |= kRoundness;
  if (a.frameThickness != b.frameThickness) fields |= kFrameThickness;
  if (a.shadowOffset != b.shadowOffset) fields |= kShadowOffset;
  if (a.widgetFrac != b.widgetFrac) fields |= kWidgetFrac;
  if (a.angle != b.angle) fields |= kAngle;
  if (!SameColorSpec(a.colorSpec, b.colorSpec)) fields |= kColorSpec;
  if (!SameText(a.labelText, b.labelText)) fields |= kLabelText;
  if (!SameText(a.valueText, b.valueText)) fields |= kValueText;
  if (a.labelOrientation != b.labelOrientation) fields |= kLabelOrientation;

  return fields;
}

const IVColorSpec kCustomColors {COLOR_RED, COLOR_ORANGE, COLOR_YELLOW, COLOR_GREEN, COLOR_BLUE, COLOR_INDIGO, COLOR_VIOLET, COLOR_WHITE, COLOR_BLACK};
const IText kCustomLabelText {30.f, COLOR_RED, "Label-Font", EAlign::Near, EVAlign::Bottom, 10.f};
const IText kCustomValueText {31.f, COLOR_BLUE, "Value-Font", EAlign::Far, EVAlign::Top, 20.f};

/** An IVStyle where every member differs from the defaults */
IVStyle MakeNonDefaultStyle()
{
  IVStyle style;
  style.hideCursor = !DEFAULT_HIDE_CURSOR;
  style.showLabel = !DEFAULT_SHOW_LABEL;
  style.showValue = !DEFAULT_SHOW_VALUE;
  style.drawFrame = !DEFAULT_DRAW_FRAME;
  style.drawShadows = !DEFAULT_DRAW_SHADOWS;
  style.emboss = !DEFAULT_EMBOSS;
  style.roundness = 0.5f;
  style.frameThickness = 3.f;
  style.shadowOffset = 7.f;
  style.widgetFrac = 0.75f;
  style.angle = 45.f;
  style.colorSpec = kCustomColors;
  style.labelText = kCustomLabelText;
  style.valueText = kCustomValueText;
  style.labelOrientation = EOrientation::South;
  return style;
}

const int kAllFields = (1 << 15) - 1;
} // namespace

TEST_CASE("EVColor: indices and names", "[unit][ivstyle]")
{
  REQUIRE(kNumVColors == 9);
  REQUIRE(kBG == 0);

  // the aliases for on/off states
  REQUIRE(kOFF == kFG);
  REQUIRE(kON == kPR);

  // every color has a distinct, non-empty name
  std::set<std::string> names;

  for (int i = 0; i < kNumVColors; i++)
  {
    REQUIRE(kVColorStrs[i] != nullptr);
    REQUIRE(std::strlen(kVColorStrs[i]) > 0);
    names.insert(kVColorStrs[i]);
  }

  REQUIRE(names.size() == static_cast<size_t>(kNumVColors));

  REQUIRE(std::string(kVColorStrs[kBG]) == "bg");
  REQUIRE(std::string(kVColorStrs[kFR]) == "frame");
  REQUIRE(std::string(kVColorStrs[kHL]) == "highlight");
  REQUIRE(std::string(kVColorStrs[kSH]) == "shadow");
  REQUIRE(std::string(kVColorStrs[kX1]) == "extra1");
  REQUIRE(std::string(kVColorStrs[kX2]) == "extra2");
  REQUIRE(std::string(kVColorStrs[kX3]) == "extra3");
}

TEST_CASE("IVColorSpec: GetDefaultColor", "[unit][ivstyle]")
{
  REQUIRE(SameColor(IVColorSpec::GetDefaultColor(kBG), DEFAULT_BGCOLOR));
  REQUIRE(SameColor(IVColorSpec::GetDefaultColor(kFG), DEFAULT_FGCOLOR));
  REQUIRE(SameColor(IVColorSpec::GetDefaultColor(kPR), DEFAULT_PRCOLOR));
  REQUIRE(SameColor(IVColorSpec::GetDefaultColor(kFR), DEFAULT_FRCOLOR));
  REQUIRE(SameColor(IVColorSpec::GetDefaultColor(kHL), DEFAULT_HLCOLOR));
  REQUIRE(SameColor(IVColorSpec::GetDefaultColor(kSH), DEFAULT_SHCOLOR));
  REQUIRE(SameColor(IVColorSpec::GetDefaultColor(kX1), DEFAULT_X1COLOR));
  REQUIRE(SameColor(IVColorSpec::GetDefaultColor(kX2), DEFAULT_X2COLOR));
  REQUIRE(SameColor(IVColorSpec::GetDefaultColor(kX3), DEFAULT_X3COLOR));

  REQUIRE(SameColor(IVColorSpec::GetDefaultColor(kOFF), DEFAULT_FGCOLOR));
  REQUIRE(SameColor(IVColorSpec::GetDefaultColor(kON), DEFAULT_PRCOLOR));

  // anything else is transparent
  REQUIRE(SameColor(IVColorSpec::GetDefaultColor(kNumVColors), COLOR_TRANSPARENT));
}

TEST_CASE("IVColorSpec: default construction uses the default colors", "[unit][ivstyle]")
{
  IVColorSpec spec;

  for (int i = 0; i < kNumVColors; i++)
  {
    INFO("color " << kVColorStrs[i]);
    REQUIRE(SameColor(spec.mColors[i], IVColorSpec::GetDefaultColor(static_cast<EVColor>(i))));
    REQUIRE(SameColor(spec.GetColor(static_cast<EVColor>(i)), IVColorSpec::GetDefaultColor(static_cast<EVColor>(i))));
  }

  REQUIRE(SameColorSpec(spec, DEFAULT_COLOR_SPEC));
}

TEST_CASE("IVColorSpec: construction from a list of colors", "[unit][ivstyle]")
{
  SECTION("a full list")
  {
    REQUIRE(SameColor(kCustomColors.GetColor(kBG), COLOR_RED));
    REQUIRE(SameColor(kCustomColors.GetColor(kFG), COLOR_ORANGE));
    REQUIRE(SameColor(kCustomColors.GetColor(kPR), COLOR_YELLOW));
    REQUIRE(SameColor(kCustomColors.GetColor(kFR), COLOR_GREEN));
    REQUIRE(SameColor(kCustomColors.GetColor(kHL), COLOR_BLUE));
    REQUIRE(SameColor(kCustomColors.GetColor(kSH), COLOR_INDIGO));
    REQUIRE(SameColor(kCustomColors.GetColor(kX1), COLOR_VIOLET));
    REQUIRE(SameColor(kCustomColors.GetColor(kX2), COLOR_WHITE));
    REQUIRE(SameColor(kCustomColors.GetColor(kX3), COLOR_BLACK));
  }

  SECTION("a partial list uses the defaults for the rest")
  {
    const IVColorSpec spec {COLOR_RED, COLOR_ORANGE, COLOR_YELLOW};
    REQUIRE(SameColor(spec.GetColor(kBG), COLOR_RED));
    REQUIRE(SameColor(spec.GetColor(kFG), COLOR_ORANGE));
    REQUIRE(SameColor(spec.GetColor(kPR), COLOR_YELLOW));

    for (int i = kFR; i < kNumVColors; i++)
      REQUIRE(SameColor(spec.mColors[i], IVColorSpec::GetDefaultColor(static_cast<EVColor>(i))));
  }

  SECTION("a single color")
  {
    const IVColorSpec spec {COLOR_RED};
    REQUIRE(SameColor(spec.GetColor(kBG), COLOR_RED));

    for (int i = kFG; i < kNumVColors; i++)
      REQUIRE(SameColor(spec.mColors[i], IVColorSpec::GetDefaultColor(static_cast<EVColor>(i))));
  }

  SECTION("the list of defaults is the same as default construction")
  {
    const IVColorSpec spec {DEFAULT_BGCOLOR, DEFAULT_FGCOLOR, DEFAULT_PRCOLOR, DEFAULT_FRCOLOR, DEFAULT_HLCOLOR, DEFAULT_SHCOLOR, DEFAULT_X1COLOR, DEFAULT_X2COLOR, DEFAULT_X3COLOR};
    REQUIRE(SameColorSpec(spec, IVColorSpec()));
  }
}

TEST_CASE("IVColorSpec: GetColor and the on/off aliases", "[unit][ivstyle]")
{
  REQUIRE(SameColor(kCustomColors.GetColor(kOFF), kCustomColors.GetColor(kFG)));
  REQUIRE(SameColor(kCustomColors.GetColor(kON), kCustomColors.GetColor(kPR)));

  // GetColor refers to the stored color
  REQUIRE(&kCustomColors.GetColor(kHL) == &kCustomColors.mColors[kHL]);
}

TEST_CASE("IVColorSpec: colors can be changed and reset", "[unit][ivstyle]")
{
  IVColorSpec spec;
  spec.mColors[kFG] = COLOR_RED;
  spec.mColors[kX3] = COLOR_ORANGE;
  REQUIRE(SameColor(spec.GetColor(kFG), COLOR_RED));
  REQUIRE(SameColor(spec.GetColor(kX3), COLOR_ORANGE));
  REQUIRE(SameColor(spec.GetColor(kBG), DEFAULT_BGCOLOR));
  REQUIRE_FALSE(SameColorSpec(spec, DEFAULT_COLOR_SPEC));

  spec.ResetColors();
  REQUIRE(SameColorSpec(spec, DEFAULT_COLOR_SPEC));

  IVColorSpec custom = kCustomColors;
  custom.ResetColors();
  REQUIRE(SameColorSpec(custom, DEFAULT_COLOR_SPEC));
}

TEST_CASE("IVStyle: default construction", "[unit][ivstyle]")
{
  IVStyle style;
  REQUIRE(style.hideCursor == DEFAULT_HIDE_CURSOR);
  REQUIRE(style.showLabel == DEFAULT_SHOW_LABEL);
  REQUIRE(style.showValue == DEFAULT_SHOW_VALUE);
  REQUIRE(style.drawFrame == DEFAULT_DRAW_FRAME);
  REQUIRE(style.drawShadows == DEFAULT_DRAW_SHADOWS);
  REQUIRE(style.emboss == DEFAULT_EMBOSS);
  REQUIRE(style.roundness == DEFAULT_ROUNDNESS);
  REQUIRE(style.frameThickness == DEFAULT_FRAME_THICKNESS);
  REQUIRE(style.shadowOffset == DEFAULT_SHADOW_OFFSET);
  REQUIRE(style.widgetFrac == DEFAULT_WIDGET_FRAC);
  REQUIRE(style.angle == DEFAULT_WIDGET_ANGLE);
  REQUIRE(SameColorSpec(style.colorSpec, DEFAULT_COLOR_SPEC));
  REQUIRE(SameText(style.labelText, DEFAULT_LABEL_TEXT));
  REQUIRE(SameText(style.valueText, DEFAULT_VALUE_TEXT));
  REQUIRE(style.labelOrientation == DEFAULT_LABEL_ORIENTATION);

  REQUIRE(DifferingFields(style, DEFAULT_STYLE) == 0);
}

TEST_CASE("IVStyle: the test helper changes every field", "[unit][ivstyle]")
{
  REQUIRE(DifferingFields(MakeNonDefaultStyle(), IVStyle()) == kAllFields);
}

TEST_CASE("IVStyle: construction with values", "[unit][ivstyle]")
{
  SECTION("showLabel and showValue")
  {
    REQUIRE(DifferingFields(IVStyle(false), IVStyle()) == kShowLabel);
    REQUIRE_FALSE(IVStyle(false).showLabel);
    REQUIRE(DifferingFields(IVStyle(true, false), IVStyle()) == kShowValue);
    REQUIRE_FALSE(IVStyle(true, false).showValue);
    REQUIRE(DifferingFields(IVStyle(false, false), IVStyle()) == (kShowLabel | kShowValue));
  }

  SECTION("colors and text")
  {
    const IVStyle style {true, true, kCustomColors, kCustomLabelText, kCustomValueText};
    REQUIRE(DifferingFields(style, IVStyle()) == (kColorSpec | kLabelText | kValueText));
    REQUIRE(SameColorSpec(style.colorSpec, kCustomColors));
    REQUIRE(SameText(style.labelText, kCustomLabelText));
    REQUIRE(SameText(style.valueText, kCustomValueText));
  }

  SECTION("all of the arguments other than the label orientation")
  {
    const IVStyle style {!DEFAULT_SHOW_LABEL, !DEFAULT_SHOW_VALUE, kCustomColors, kCustomLabelText, kCustomValueText,
                         !DEFAULT_HIDE_CURSOR, !DEFAULT_DRAW_FRAME, !DEFAULT_DRAW_SHADOWS, !DEFAULT_EMBOSS,
                         0.5f, 3.f, 7.f, 0.75f, 45.f};

    REQUIRE(DifferingFields(style, MakeNonDefaultStyle()) == kLabelOrientation);
    REQUIRE(style.roundness == 0.5f);
    REQUIRE(style.frameThickness == 3.f);
    REQUIRE(style.shadowOffset == 7.f);
    REQUIRE(style.widgetFrac == 0.75f);
    REQUIRE(style.angle == 45.f);
  }
}

TEST_CASE("IVStyle: construction with a label orientation", "[unit][ivstyle]")
{
  const IVStyle style {DEFAULT_SHOW_LABEL, DEFAULT_SHOW_VALUE, DEFAULT_COLOR_SPEC, DEFAULT_LABEL_TEXT, DEFAULT_VALUE_TEXT,
                       DEFAULT_HIDE_CURSOR, DEFAULT_DRAW_FRAME, DEFAULT_DRAW_SHADOWS, DEFAULT_EMBOSS,
                       DEFAULT_ROUNDNESS, DEFAULT_FRAME_THICKNESS, DEFAULT_SHADOW_OFFSET, DEFAULT_WIDGET_FRAC, DEFAULT_WIDGET_ANGLE,
                       EOrientation::South};

  CHECK(style.labelOrientation == EOrientation::South);
  CHECK(DifferingFields(style, IVStyle()) == kLabelOrientation);
}

TEST_CASE("IVStyle: construction from a list of colors", "[unit][ivstyle]")
{
  const IVStyle full {COLOR_RED, COLOR_ORANGE, COLOR_YELLOW, COLOR_GREEN, COLOR_BLUE, COLOR_INDIGO, COLOR_VIOLET, COLOR_WHITE, COLOR_BLACK};
  REQUIRE(DifferingFields(full, IVStyle()) == kColorSpec);
  REQUIRE(SameColorSpec(full.colorSpec, kCustomColors));

  // a partial list uses the default colors for the rest
  const IVStyle partial {COLOR_RED, COLOR_ORANGE};
  REQUIRE(DifferingFields(partial, IVStyle()) == kColorSpec);
  REQUIRE(SameColor(partial.colorSpec.GetColor(kBG), COLOR_RED));
  REQUIRE(SameColor(partial.colorSpec.GetColor(kFG), COLOR_ORANGE));

  for (int i = kPR; i < kNumVColors; i++)
    REQUIRE(SameColor(partial.colorSpec.mColors[i], IVColorSpec::GetDefaultColor(static_cast<EVColor>(i))));
}

namespace
{
/** Checks that each With builder changes its own member of base, and nothing else */
void CheckBuilders(const IVStyle& base)
{
  {
    INFO("WithShowLabel");
    REQUIRE(DifferingFields(base.WithShowLabel(!base.showLabel), base) == kShowLabel);
    REQUIRE(DifferingFields(base.WithShowLabel(base.showLabel), base) == 0);
    REQUIRE(base.WithShowLabel().showLabel);
    REQUIRE_FALSE(base.WithShowLabel(false).showLabel);
  }

  {
    INFO("WithShowValue");
    REQUIRE(DifferingFields(base.WithShowValue(!base.showValue), base) == kShowValue);
    REQUIRE(DifferingFields(base.WithShowValue(base.showValue), base) == 0);
    REQUIRE(base.WithShowValue().showValue);
    REQUIRE_FALSE(base.WithShowValue(false).showValue);
  }

  {
    INFO("WithLabelText");
    const IText text {99.f, COLOR_ORANGE, "Other-Font"};
    const IVStyle style = base.WithLabelText(text);
    REQUIRE(DifferingFields(style, base) == kLabelText);
    REQUIRE(SameText(style.labelText, text));
  }

  {
    INFO("WithValueText");
    const IText text {99.f, COLOR_ORANGE, "Other-Font"};
    const IVStyle style = base.WithValueText(text);
    REQUIRE(DifferingFields(style, base) == kValueText);
    REQUIRE(SameText(style.valueText, text));
  }

  {
    INFO("WithHideCursor");
    REQUIRE(DifferingFields(base.WithHideCursor(!base.hideCursor), base) == kHideCursor);
    REQUIRE(DifferingFields(base.WithHideCursor(base.hideCursor), base) == 0);
    REQUIRE(base.WithHideCursor().hideCursor);
    REQUIRE_FALSE(base.WithHideCursor(false).hideCursor);
  }

  {
    INFO("WithColor");
    const IColor newColor {12, 34, 56, 78};

    for (int i = 0; i < kNumVColors; i++)
    {
      INFO("color " << kVColorStrs[i]);
      const IVStyle style = base.WithColor(static_cast<EVColor>(i), newColor);
      REQUIRE(DifferingFields(style, base) == kColorSpec);

      // only the one color changes
      for (int j = 0; j < kNumVColors; j++)
        REQUIRE(SameColor(style.colorSpec.mColors[j], i == j ? newColor : base.colorSpec.mColors[j]));
    }
  }

  {
    INFO("WithColors");
    const IVColorSpec spec {COLOR_WHITE, COLOR_BLACK, COLOR_WHITE, COLOR_BLACK, COLOR_WHITE, COLOR_BLACK, COLOR_WHITE, COLOR_BLACK, COLOR_WHITE};
    const IVStyle style = base.WithColors(spec);
    REQUIRE(DifferingFields(style, base) == kColorSpec);
    REQUIRE(SameColorSpec(style.colorSpec, spec));
  }

  {
    INFO("WithRoundness");
    const IVStyle style = base.WithRoundness(0.25f);
    REQUIRE(DifferingFields(style, base) == kRoundness);
    REQUIRE(style.roundness == 0.25f);

    // limited to 0-1
    REQUIRE(base.WithRoundness(0.f).roundness == 0.f);
    REQUIRE(base.WithRoundness(1.f).roundness == 1.f);
    REQUIRE(base.WithRoundness(5.f).roundness == 1.f);
    REQUIRE(base.WithRoundness(-5.f).roundness == 0.f);
  }

  {
    INFO("WithFrameThickness");
    const IVStyle style = base.WithFrameThickness(12.5f);
    REQUIRE(DifferingFields(style, base) == kFrameThickness);
    REQUIRE(style.frameThickness == 12.5f);
  }

  {
    INFO("WithShadowOffset");
    const IVStyle style = base.WithShadowOffset(12.5f);
    REQUIRE(DifferingFields(style, base) == kShadowOffset);
    REQUIRE(style.shadowOffset == 12.5f);
  }

  {
    INFO("WithDrawShadows");
    REQUIRE(DifferingFields(base.WithDrawShadows(!base.drawShadows), base) == kDrawShadows);
    REQUIRE(DifferingFields(base.WithDrawShadows(base.drawShadows), base) == 0);
    REQUIRE(base.WithDrawShadows().drawShadows);
    REQUIRE_FALSE(base.WithDrawShadows(false).drawShadows);
  }

  {
    INFO("WithDrawFrame");
    REQUIRE(DifferingFields(base.WithDrawFrame(!base.drawFrame), base) == kDrawFrame);
    REQUIRE(DifferingFields(base.WithDrawFrame(base.drawFrame), base) == 0);
    REQUIRE(base.WithDrawFrame().drawFrame);
    REQUIRE_FALSE(base.WithDrawFrame(false).drawFrame);
  }

  {
    INFO("WithWidgetFrac");
    const IVStyle style = base.WithWidgetFrac(0.25f);
    REQUIRE(DifferingFields(style, base) == kWidgetFrac);
    REQUIRE(style.widgetFrac == 0.25f);

    // limited to 0-1
    REQUIRE(base.WithWidgetFrac(0.f).widgetFrac == 0.f);
    REQUIRE(base.WithWidgetFrac(1.f).widgetFrac == 1.f);
    REQUIRE(base.WithWidgetFrac(5.f).widgetFrac == 1.f);
    REQUIRE(base.WithWidgetFrac(-5.f).widgetFrac == 0.f);
  }

  {
    INFO("WithAngle");
    const IVStyle style = base.WithAngle(270.f);
    REQUIRE(DifferingFields(style, base) == kAngle);
    REQUIRE(style.angle == 270.f);

    // limited to 0-360 degrees
    REQUIRE(base.WithAngle(0.f).angle == 0.f);
    REQUIRE(base.WithAngle(360.f).angle == 360.f);
    REQUIRE(base.WithAngle(1000.f).angle == 360.f);
    REQUIRE(base.WithAngle(-90.f).angle == 0.f);
  }

  {
    INFO("WithEmboss");
    REQUIRE(DifferingFields(base.WithEmboss(!base.emboss), base) == kEmboss);
    REQUIRE(DifferingFields(base.WithEmboss(base.emboss), base) == 0);
    REQUIRE(base.WithEmboss().emboss);
    REQUIRE_FALSE(base.WithEmboss(false).emboss);
  }

  {
    INFO("WithLabelOrientation");
    for (EOrientation orientation : {EOrientation::North, EOrientation::East, EOrientation::South, EOrientation::West})
    {
      const IVStyle style = base.WithLabelOrientation(orientation);
      REQUIRE(DifferingFields(style, base) == (orientation == base.labelOrientation ? 0 : static_cast<int>(kLabelOrientation)));
      REQUIRE(style.labelOrientation == orientation);
    }
  }
}
} // namespace

TEST_CASE("IVStyle: With builders change only their own member, starting from the defaults", "[unit][ivstyle]")
{
  CheckBuilders(IVStyle());
}

TEST_CASE("IVStyle: With builders change only their own member, starting from a customised style", "[unit][ivstyle]")
{
  CheckBuilders(MakeNonDefaultStyle());
}

TEST_CASE("IVStyle: With builders do not modify the original and can be chained", "[unit][ivstyle]")
{
  const IVStyle base;

  const IVStyle style = base.WithShowLabel(false).WithShowValue(false).WithLabelText(kCustomLabelText).WithValueText(kCustomValueText)
                            .WithHideCursor(false).WithColors(kCustomColors).WithRoundness(0.5f).WithFrameThickness(3.f)
                            .WithShadowOffset(7.f).WithDrawShadows(false).WithDrawFrame(false).WithWidgetFrac(0.75f)
                            .WithAngle(45.f).WithEmboss(true).WithLabelOrientation(EOrientation::South);

  REQUIRE(DifferingFields(base, IVStyle()) == 0);
  REQUIRE(DifferingFields(style, MakeNonDefaultStyle()) == 0);
  REQUIRE(DifferingFields(style, base) == kAllFields);
}

TEST_CASE("IVStyle: DEFAULT_STYLE builders, as used when creating controls", "[unit][ivstyle]")
{
  const IVStyle style = DEFAULT_STYLE.WithColor(kFG, COLOR_RED).WithColor(kPR, COLOR_BLUE).WithShowValue(false);
  REQUIRE(DifferingFields(style, DEFAULT_STYLE) == (kColorSpec | kShowValue));
  REQUIRE(SameColor(style.colorSpec.GetColor(kFG), COLOR_RED));
  REQUIRE(SameColor(style.colorSpec.GetColor(kPR), COLOR_BLUE));
  REQUIRE(SameColor(style.colorSpec.GetColor(kBG), DEFAULT_BGCOLOR));
  REQUIRE_FALSE(style.showValue);
}
