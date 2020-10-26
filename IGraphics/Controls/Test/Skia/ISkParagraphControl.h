/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#ifndef IGRAPHICS_SKIA
#error This IControl only works with the Skia graphics backend
#endif

/**
 * @file
 * @copydoc ISkParagraphControl
 */

#include "IControl.h"
#include "IGraphicsSkia.h"
#include "include/core/SkCanvas.h"
#include "include/core/SkColorFilter.h"
#include "include/core/SkColorPriv.h"
#include "include/core/SkFontMgr.h"
#include "include/core/SkGraphics.h"
#include "include/core/SkPath.h"
#include "include/core/SkRegion.h"
#include "include/core/SkShader.h"
#include "include/core/SkStream.h"
#include "include/core/SkTextBlob.h"
#include "include/core/SkTime.h"
#include "include/core/SkTypeface.h"
#include "include/effects/SkGradientShader.h"
#include "include/utils/SkRandom.h"
#include "modules/skparagraph/include/Paragraph.h"
#include "modules/skparagraph/include/TypefaceFontProvider.h"
#include "modules/skparagraph/src/ParagraphBuilderImpl.h"
#include "modules/skparagraph/src/ParagraphImpl.h"
#include "modules/skparagraph/src/TextLine.h"
#include "modules/skparagraph/utils/TestFontCollection.h"

#ifdef OS_WIN
  #pragma comment(lib, "skparagraph.lib")
  #pragma comment(lib, "skshaper.lib")
#endif

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

using namespace skia::textlayout;


/** A control that demonstrates how to draw rich text using SkParagraph
 * @ingroup IControls */
class ISkParagraphControl : public IControl
{
public:
  ISkParagraphControl(const IRECT& bounds)
  : IControl(bounds)
  {
  }

  void Draw(IGraphics& g) override
  {
    g.FillRect(COLOR_GREEN, mRECT);
    SkCanvas* canvas = (SkCanvas*) g.GetDrawContext();
    DoDrawContent(canvas);
  }

  void DoDrawContent(SkCanvas* canvas)
  {
    float w = mRECT.W();
//    float h = mRECT.H();
    
    const std::vector<
      std::tuple<std::string, bool, bool, int, SkColor, SkColor, bool, TextDecorationStyle>>
      gParagraph = { {"monospace", true, false, 14, SK_ColorWHITE, SK_ColorRED, true,
                     TextDecorationStyle::kDashed},
                    {"Assyrian", false, false, 20, SK_ColorWHITE, SK_ColorBLUE, false,
                     TextDecorationStyle::kDotted},
                    {"serif", true, true, 10, SK_ColorWHITE, SK_ColorRED, true,
                     TextDecorationStyle::kDouble},
                    {"Arial", false, true, 16, SK_ColorGRAY, SK_ColorGREEN, true,
                     TextDecorationStyle::kSolid},
                    {"sans-serif", false, false, 8, SK_ColorWHITE, SK_ColorRED, false,
                     TextDecorationStyle::kWavy} };
    SkAutoCanvasRestore acr(canvas, true);

//    canvas->clipRect(SkRect::MakeWH(w, h));
    canvas->drawColor(SK_ColorWHITE);

    SkScalar margin = 20;

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SK_ColorWHITE);

    SkPaint blue;
    blue.setColor(SK_ColorBLUE);

    TextStyle defaultStyle;
    defaultStyle.setBackgroundColor(blue);
    defaultStyle.setForegroundColor(paint);
    ParagraphStyle paraStyle;

    auto fontCollection = sk_make_sp<FontCollection>();
    fontCollection->setDefaultFontManager(SkFontMgr::RefDefault());
    for (auto i = 1; i < 5; ++i) {
      defaultStyle.setFontSize(24 * i);
      paraStyle.setTextStyle(defaultStyle);
      ParagraphBuilderImpl builder(paraStyle, fontCollection);
      std::string name = "Paragraph: " + std::to_string(24 * i);
      builder.addText(name.c_str(), name.length());
      for (auto para : gParagraph) {
        TextStyle style;
        style.setFontFamilies({ SkString(std::get<0>(para).c_str()) });
        SkFontStyle fontStyle(std::get<1>(para) ? SkFontStyle::Weight::kBold_Weight
          : SkFontStyle::Weight::kNormal_Weight,
          SkFontStyle::Width::kNormal_Width,
          std::get<2>(para) ? SkFontStyle::Slant::kItalic_Slant
          : SkFontStyle::Slant::kUpright_Slant);
        style.setFontStyle(fontStyle);
        style.setFontSize(std::get<3>(para) * i);
        SkPaint background;
        background.setColor(std::get<4>(para));
        style.setBackgroundColor(background);
        SkPaint foreground;
        foreground.setColor(std::get<5>(para));
        foreground.setAntiAlias(true);
        style.setForegroundColor(foreground);
        if (std::get<6>(para)) {
          style.addShadow(TextShadow(SK_ColorBLACK, SkPoint::Make(5, 5), 2));
        }

        auto decoration = (i % 4);
        if (decoration == 3) {
          decoration = 4;
        }

        bool test = (TextDecoration)decoration != TextDecoration::kNoDecoration;
        std::string deco = std::to_string((int)decoration);
        if (test) {
          style.setDecoration((TextDecoration)decoration);
          style.setDecorationStyle(std::get<7>(para));
          style.setDecorationColor(std::get<5>(para));
        }
        builder.pushStyle(style);
        std::string name = " " + std::get<0>(para) + " " +
          (std::get<1>(para) ? ", bold" : "") +
          (std::get<2>(para) ? ", italic" : "") + " " +
          std::to_string(std::get<3>(para) * i) +
          (std::get<4>(para) != SK_ColorRED ? ", background" : "") +
          (std::get<5>(para) != SK_ColorWHITE ? ", foreground" : "") +
          (std::get<6>(para) ? ", shadow" : "") +
          (test ? ", decorations " + deco : "") + ";";
        builder.addText(name.c_str(), name.length());
        builder.pop();
      }

      auto paragraph = builder.Build();
      paragraph->layout(w - margin * 2);
      paragraph->paint(canvas, mRECT.L + margin, mRECT.T + margin);

      canvas->translate(0, paragraph->getHeight());
    }
  }

//  void DoDrawContent(SkCanvas* canvas)
//  {
//    const std::u16string text = u"The quick brown fox \U0001f98a ate a zesty ham burger fons \U0001f354."
//      "The \U0001f469\u200D\U0001f469\u200D\U0001f467\u200D\U0001f467 laughed.";
//    canvas->drawColor(SK_ColorWHITE);
//
//    auto fontCollection = getFontCollection();
//    fontCollection->setDefaultFontManager(SkFontMgr::RefDefault());
//    fontCollection->enableFontFallback();
//
//    ParagraphStyle paragraph_style;
//    paragraph_style.setMaxLines(7);
//    paragraph_style.setEllipsis(u"\u2026");
//    ParagraphBuilderImpl builder(paragraph_style, fontCollection);
//    TextStyle text_style;
//    text_style.setColor(SK_ColorBLACK);
//    text_style.setFontFamilies({ SkString("Roboto"), SkString("Noto Color Emoji") });
//    text_style.setFontSize(60);
//    builder.pushStyle(text_style);
//    builder.addText(text);
//    auto paragraph = builder.Build();
//    assert(paragraph);
//    paragraph->layout(mRECT.W());
//    paragraph->paint(canvas, mRECT.L, mRECT.T);
//  }
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
