/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc ISkShaderControl
 */

#include "IControl.h"
#include "src/core/SkRuntimeEffect.h"
#include "SkGradientShader.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** A control that hosts an SkSL fragment shader
 * https://github.com/google/skia/blob/master/src/sksl/README
 * @ingroup IControls */
class ISkShaderControl : public IControl
{
public:
  ISkShaderControl(const IRECT& bounds)
  : IControl(bounds)
  {
    fSkSL =
        "uniform half4 gColor;\n"
        "void main(float x, float y, inout half4 color) {\n"
          "color = half4(1,0,0,1);"
        "}\n";
    
    Load(bounds.W(), bounds.H());
  }

  void Draw(IGraphics& g) override
  {
    g.FillRect(COLOR_GREEN, mRECT);
    
    if(fEffect)
      DoDraw((SkCanvas*) g.GetDrawContext());
  }
  
  void DoDraw(SkCanvas* canvas)
  {
    canvas->clear(SK_ColorWHITE);

    auto inputs = SkData::MakeWithoutCopy(fInputs.get(), fEffect->inputSize());
    auto shader = fEffect->makeShader(std::move(inputs), fChildren.data(), fChildren.count(), nullptr, false);

    SkPaint p;
    p.setShader(std::move(shader));
    canvas->drawRect({ mRECT.L, mRECT.T, mRECT.W(), mRECT.H() }, p);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
  }
  
  void Load(SkScalar winWidth, SkScalar winHeight)
  {
    SkPoint points[] = { { 0, 0 }, { winWidth, 0 } };
    SkColor colors[] = { SK_ColorRED, SK_ColorGREEN };

    sk_sp<SkShader> shader;

//    shader = SkGradientShader::MakeLinear(points, colors, nullptr, 2, SkTileMode::kClamp);
//    fShaders.push_back(std::make_pair("Linear Gradient", shader));
//
//    shader = SkGradientShader::MakeRadial({ 128, 128 }, 128, colors, nullptr, 2,
//                                          SkTileMode::kClamp);
//    fShaders.push_back(std::make_pair("Radial Gradient", shader));
//
//    shader = SkGradientShader::MakeSweep(128, 128, colors, nullptr, 2);
//    fShaders.push_back(std::make_pair("Sweep Gradient", shader));

    this->Rebuild();
  }
  
  bool Rebuild()
  {
    auto [effect, errorText] = SkRuntimeEffect::Make(fSkSL);
    if (!effect) {
      printf("%s\n", errorText.c_str());
      return false;
    }

    size_t oldSize = fEffect ? fEffect->inputSize() : 0;
    fInputs.realloc(effect->inputSize());
    if (effect->inputSize() > oldSize) {
        memset(fInputs.get() + oldSize, 0, effect->inputSize() - oldSize);
    }
    fChildren.resize_back(effect->childCount());
    for (auto& c : fChildren) {
        if (!c) {
            c = fShaders[0].second;
        }
    }

    fEffect = effect;
    return true;
  }
  
private:
  sk_sp<SkRuntimeEffect> fEffect;
  SkAutoTMalloc<char> fInputs;
  SkTArray<sk_sp<SkShader>> fChildren;
  SkString fSkSL;
  
  // Named shaders that can be selected as inputs
  SkTArray<std::pair<const char*, sk_sp<SkShader>>> fShaders;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
