#include "SkiaExperiments.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"

#include "modules/skparagraph/include/TypefaceFontProvider.h"
#include "modules/skparagraph/src/ParagraphBuilderImpl.h"
#include "modules/skparagraph/src/ParagraphImpl.h"
#include "modules/skparagraph/utils/TestFontCollection.h"


//#include "src/core/SkBlurMask.h"
//#include "src/effects/SkEmbossMaskFilter.h"
//#include "src/utils/SkUTF.h"

//SkEmbossMaskFilter::Light   fLight;

using namespace skia::textlayout;

//void onDrawContent(SkCanvas* canvas, float x, float y, float w) {
//    SkPaint paint;
//
//    paint.setAntiAlias(true);
//    paint.setStyle(SkPaint::kStroke_Style);
//    paint.setStrokeWidth(SkIntToScalar(20));
//    paint.setMaskFilter(SkEmbossMaskFilter::Make(SkBlurMask::ConvertRadiusToSigma(4), fLight));
//    paint.setShader(SkShaders::Color(SK_ColorBLUE));
//    paint.setDither(true);
//
//    canvas->drawCircle(SkIntToScalar(100), SkIntToScalar(100), SkIntToScalar(40), paint);
//}

void onDrawContent(SkCanvas* canvas, float x, float y, float w) {
//    canvas->drawColor(SK_ColorWHITE);
    auto multiplier = 5.67;
    const char* text = "English 字典 字典 \n😀😃😄 😀😃😄";

    auto fontCollection = sk_make_sp<FontCollection>();
    fontCollection->setDefaultFontManager(SkFontMgr::RefDefault());
    fontCollection->enableFontFallback();

    ParagraphStyle paragraph_style;
    paragraph_style.turnHintingOff();
    ParagraphBuilderImpl builder(paragraph_style, fontCollection);

    TextStyle text_style;
    text_style.setFontFamilies({SkString("Roboto"),
                                SkString("Apple Color Emoji"),
                                SkString("Noto Serif CJK JP")});
    text_style.setFontSize(10 * multiplier);
    text_style.setLetterSpacing(0);
    text_style.setWordSpacing(0);
    text_style.setColor(SK_ColorBLUE);
    text_style.setHeight(1);
    text_style.addShadow(TextShadow(SK_ColorBLACK, SkPoint::Make(5, 5), 2));

    builder.pushStyle(text_style);
    builder.addText(text, strlen(text));
    builder.pop();

    auto paragraph = builder.Build();
    paragraph->layout(w);

    paragraph->paint(canvas, x, y);
}

//void drawTest(SkCanvas* canvas, SkScalar w, SkScalar h, SkColor fg, SkColor bg) {
//    const std::vector<
//        std::tuple<std::string, bool, bool, int, SkColor, SkColor, bool, TextDecorationStyle>>
//        gParagraph = {{"monospace", true, false, 14, SK_ColorWHITE, SK_ColorRED, true,
//                       TextDecorationStyle::kDashed},
//                      {"Assyrian", false, false, 20, SK_ColorWHITE, SK_ColorBLUE, false,
//                       TextDecorationStyle::kDotted},
//                      {"serif", true, true, 10, SK_ColorWHITE, SK_ColorRED, true,
//                       TextDecorationStyle::kDouble},
//                      {"Arial", false, true, 16, SK_ColorGRAY, SK_ColorGREEN, true,
//                       TextDecorationStyle::kSolid},
//                      {"sans-serif", false, false, 8, SK_ColorWHITE, SK_ColorRED, false,
//                       TextDecorationStyle::kWavy}};
//    SkAutoCanvasRestore acr(canvas, true);
//
//    canvas->clipRect(SkRect::MakeWH(w, h));
//    canvas->drawColor(SK_ColorWHITE);
//
//    SkScalar margin = 20;
//
//    SkPaint paint;
//    paint.setAntiAlias(true);
//    paint.setColor(fg);
//
//    SkPaint blue;
//    blue.setColor(SK_ColorBLUE);
//
//    TextStyle defaultStyle;
//    defaultStyle.setBackgroundColor(blue);
//    defaultStyle.setForegroundColor(paint);
//    ParagraphStyle paraStyle;
//
//    auto fontCollection = sk_make_sp<FontCollection>();
//    fontCollection->setDefaultFontManager(SkFontMgr::RefDefault());
//    for (auto i = 1; i < 5; ++i) {
//        defaultStyle.setFontSize(24 * i);
//        paraStyle.setTextStyle(defaultStyle);
//        ParagraphBuilderImpl builder(paraStyle, fontCollection);
//        std::string name = "Paragraph: " + std::to_string(24 * i);
//        builder.addText(name.c_str(), name.length());
//        for (auto para : gParagraph) {
//            TextStyle style;
//            style.setFontFamilies({SkString(std::get<0>(para).c_str())});
//            SkFontStyle fontStyle(std::get<1>(para) ? SkFontStyle::Weight::kBold_Weight
//                                                    : SkFontStyle::Weight::kNormal_Weight,
//                                  SkFontStyle::Width::kNormal_Width,
//                                  std::get<2>(para) ? SkFontStyle::Slant::kItalic_Slant
//                                                    : SkFontStyle::Slant::kUpright_Slant);
//            style.setFontStyle(fontStyle);
//            style.setFontSize(std::get<3>(para) * i);
//            SkPaint background;
//            background.setColor(std::get<4>(para));
//            style.setBackgroundColor(background);
//            SkPaint foreground;
//            foreground.setColor(std::get<5>(para));
//            foreground.setAntiAlias(true);
//            style.setForegroundColor(foreground);
//            if (std::get<6>(para)) {
//                style.addShadow(TextShadow(SK_ColorBLACK, SkPoint::Make(5, 5), 2));
//            }
//
//            auto decoration = (i % 4);
//            if (decoration == 3) {
//                decoration = 4;
//            }
//
//            bool test = (TextDecoration)decoration != TextDecoration::kNoDecoration;
//            std::string deco = std::to_string((int)decoration);
//            if (test) {
//                style.setDecoration((TextDecoration)decoration);
//                style.setDecorationStyle(std::get<7>(para));
//                style.setDecorationColor(std::get<5>(para));
//            }
//            builder.pushStyle(style);
//            std::string name = " " + std::get<0>(para) + " " +
//                               (std::get<1>(para) ? ", bold" : "") +
//                               (std::get<2>(para) ? ", italic" : "") + " " +
//                               std::to_string(std::get<3>(para) * i) +
//                               (std::get<4>(para) != bg ? ", background" : "") +
//                               (std::get<5>(para) != fg ? ", foreground" : "") +
//                               (std::get<6>(para) ? ", shadow" : "") +
//                               (test ? ", decorations " + deco : "") + ";";
//            builder.addText(name.c_str(), name.length());
//            builder.pop();
//        }
//
//        auto paragraph = builder.Build();
//        paragraph->layout(w - margin * 2);
//        paragraph->paint(canvas, margin, margin);
//
//        canvas->translate(0, paragraph->getHeight());
//    }
//}

SkiaExperiments::SkiaExperiments(const InstanceInfo& info)
: Plugin(info, MakeConfig(kNumParams, kNumPrograms))
{
  GetParam(kGain)->InitDouble("Gain", 0., 0., 100.0, 0.01, "%");

#if IPLUG_EDITOR // All UI methods and member variables should be within an IPLUG_EDITOR guard, should you want distributed UI
  mMakeGraphicsFunc = [&]() {
    
//    fLight.fDirection[0] = SK_Scalar1;
//    fLight.fDirection[1] = SK_Scalar1;
//    fLight.fDirection[2] = SK_Scalar1;
//    fLight.fAmbient = 128;
//    fLight.fSpecular = 16*2;
    
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.);
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    pGraphics->AttachCornerResizer(EUIResizerMode::Scale, false);
    pGraphics->AttachPanelBackground(COLOR_ORANGE);
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IRECT b = pGraphics->GetBounds();
    
    pGraphics->AttachControl(new ILambdaControl(b,
                                                [](ILambdaControl*, IGraphics& g, IRECT& b) {
      
//        drawTest((SkCanvas*) g.GetDrawContext(), b.W(), b.H(), SK_ColorRED, SK_ColorWHITE);
        onDrawContent((SkCanvas*) g.GetDrawContext(), b.L, b.T, b.W());
      
      }));
  };
    
#endif
}

#if IPLUG_DSP
void SkiaExperiments::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = inputs[c][s] * gain;
    }
  }
}
#endif
