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
#include "include/core/SkTypeface.h"
#include "include/effects/SkGradientShader.h"
#include "modules/skparagraph/include/DartTypes.h"
#include "modules/skparagraph/include/FontCollection.h"
#include "modules/skparagraph/include/Paragraph.h"
#include "modules/skparagraph/include/ParagraphBuilder.h"
#include "modules/skparagraph/include/ParagraphStyle.h"
#include "modules/skunicode/include/SkUnicode_icu.h"

#if defined OS_MAC || defined OS_IOS
  #include "include/ports/SkFontMgr_mac_ct.h"
#endif

#ifdef OS_WIN
  #include "include/ports/SkTypeface_win.h"
  #pragma comment(lib, "skparagraph.lib")
#endif

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

using namespace skia::textlayout;

static sk_sp<SkFontMgr> fontmgr_factory() {
#if defined OS_MAC || OS_IOS
  return SkFontMgr_New_CoreText(nullptr);
#elif defined OS_WIN
  return SkFontMgr_New_DirectWrite();
#else
  #error "Not supported"
#endif
}

bool g_factory_called = false;

sk_sp<SkFontMgr> SkFontMgr_RefDefault() {
  static std::once_flag flag;
  static sk_sp<SkFontMgr> mgr;
  std::call_once(flag, [] {
    mgr = fontmgr_factory();
    g_factory_called = true;
  });
  return mgr;
}

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

//  void DoDrawContent(SkCanvas* canvas)
//  {
//    float w = mRECT.W();
//    
//    const std::vector<
//      std::tuple<std::string, bool, bool, int, SkColor, SkColor, bool, TextDecorationStyle>>
//      gParagraph = { {"monospace", true, false, 14, SK_ColorWHITE, SK_ColorRED, true,
//                     TextDecorationStyle::kDashed},
//                    {"Assyrian", false, false, 20, SK_ColorWHITE, SK_ColorBLUE, false,
//                     TextDecorationStyle::kDotted},
//                    {"serif", true, true, 10, SK_ColorWHITE, SK_ColorRED, true,
//                     TextDecorationStyle::kDouble},
//                    {"Arial", false, true, 16, SK_ColorGRAY, SK_ColorGREEN, true,
//                     TextDecorationStyle::kSolid},
//                    {"sans-serif", false, false, 8, SK_ColorWHITE, SK_ColorRED, false,
//                     TextDecorationStyle::kWavy} };
//    SkAutoCanvasRestore acr(canvas, true);
//
//    canvas->drawColor(SK_ColorWHITE);
//
//    SkScalar margin = 20;
//
//    SkPaint paint;
//    paint.setAntiAlias(true);
//    paint.setColor(SK_ColorWHITE);
//
//    SkPaint blue;
//    blue.setColor(SK_ColorBLUE);
//
//    TextStyle defaultStyle;
//    defaultStyle.setBackgroundColor(blue);
//    defaultStyle.setForegroundColor(paint);
//    ParagraphStyle paraStyle;
//    sk_sp<SkUnicode> unicode = SkUnicodes::ICU::Make();
//
//    #ifdef OS_WIN
//      if (unicode == nullptr)
//      {
//        const char* errorText = "Failed to create SkUnicode - could not find icudtl.dat next to DLL/EXE";
//        DBGMSG("%s\n", errorText);
//        GetUI()->DrawText(mText, errorText, mRECT);
//        return;
//      }
//    #endif
//
//    auto fontCollection = sk_make_sp<FontCollection>();
//    fontCollection->enableFontFallback();
//    fontCollection->setDefaultFontManager(SkFontMgr_RefDefault());
//    for (auto i = 1; i < 5; ++i) {
//      defaultStyle.setFontSize(24 * i);
//      paraStyle.setTextStyle(defaultStyle);
//      std::unique_ptr<ParagraphBuilder> builder = ParagraphBuilder::make(paraStyle, fontCollection, unicode);
//      std::string name = "Paragraph: " + std::to_string(24 * i);
//      builder->addText(name.c_str(), name.length());
//      for (auto para : gParagraph) {
//        TextStyle style;
//        style.setFontFamilies({ SkString(std::get<0>(para).c_str()) });
//        SkFontStyle fontStyle(std::get<1>(para) ? SkFontStyle::Weight::kBold_Weight
//          : SkFontStyle::Weight::kNormal_Weight,
//          SkFontStyle::Width::kNormal_Width,
//          std::get<2>(para) ? SkFontStyle::Slant::kItalic_Slant
//          : SkFontStyle::Slant::kUpright_Slant);
//        style.setFontStyle(fontStyle);
//        style.setFontSize(std::get<3>(para) * i);
//        SkPaint background;
//        background.setColor(std::get<4>(para));
//        style.setBackgroundColor(background);
//        SkPaint foreground;
//        foreground.setColor(std::get<5>(para));
//        foreground.setAntiAlias(true);
//        style.setForegroundColor(foreground);
//        if (std::get<6>(para)) {
//          style.addShadow(TextShadow(SK_ColorBLACK, SkPoint::Make(5, 5), 2));
//        }
//
//        auto decoration = (i % 4);
//        if (decoration == 3) {
//          decoration = 4;
//        }
//
//        bool test = (TextDecoration)decoration != TextDecoration::kNoDecoration;
//        std::string deco = std::to_string((int)decoration);
//        if (test) {
//          style.setDecoration((TextDecoration)decoration);
//          style.setDecorationStyle(std::get<7>(para));
//          style.setDecorationColor(std::get<5>(para));
//        }
//        builder->pushStyle(style);
//        std::string name = " " + std::get<0>(para) + " " +
//          (std::get<1>(para) ? ", bold" : "") +
//          (std::get<2>(para) ? ", italic" : "") + " " +
//          std::to_string(std::get<3>(para) * i) +
//          (std::get<4>(para) != SK_ColorRED ? ", background" : "") +
//          (std::get<5>(para) != SK_ColorWHITE ? ", foreground" : "") +
//          (std::get<6>(para) ? ", shadow" : "") +
//          (test ? ", decorations " + deco : "") + ";";
//        builder->addText(name.c_str(), name.length());
//        builder->pop();
//      }
//
//      auto paragraph = builder->Build();
//      paragraph->layout(w - margin * 2);
//      paragraph->paint(canvas, mRECT.L + margin, mRECT.T + margin);
//
//      canvas->translate(0, paragraph->getHeight());
//    }
//  }

  void DoDrawContent(SkCanvas* canvas)
  {
    auto fontCollection = sk_make_sp<skia::textlayout::FontCollection>();
    fontCollection->setDefaultFontManager(SkFontMgr_RefDefault());

    canvas->clear(SK_ColorWHITE);

    SkPaint paint;
    paint.setAntiAlias(true);
    paint.setColor(SK_ColorBLACK);

    skia::textlayout::TextStyle style;
    style.setForegroundColor(paint);
    style.setFontFamilies({SkString("sans-serif")});
    style.setFontSize(30);
    skia::textlayout::ParagraphStyle paraStyle;
    paraStyle.setTextStyle(style);
    paraStyle.setTextAlign(skia::textlayout::TextAlign::kRight);

    sk_sp<SkUnicode> unicode = SkUnicodes::ICU::Make();

#ifdef OS_WIN
    if (unicode == nullptr)
    {
      const char* errorText = "Failed to create SkUnicode - could not find icudtl.dat next to DLL/EXE";
      DBGMSG("%s\n", errorText);
      GetUI()->DrawText(mText, errorText, mRECT);
      return;
    }
#endif

    using skia::textlayout::ParagraphBuilder;
    std::unique_ptr<ParagraphBuilder> builder =
        ParagraphBuilder::make(paraStyle, fontCollection, unicode);
    builder->addText(" Furthermore, العربية نص جميل. द क्विक ब्राउन फ़ॉक्स jumps over the lazy 🐕.");
    auto paragraph = builder->Build();
    paragraph->layout(mRECT.W() - 20);
    paragraph->paint(canvas, 10, 10);

  }
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
