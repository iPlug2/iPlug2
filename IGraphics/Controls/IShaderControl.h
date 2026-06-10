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
 * @copydoc IShaderControl
 */

#include "IControl.h"
#include "include/effects/SkRuntimeEffect.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** This control allows you to draw to the UI via a shader written using the Skia shading language, which is similar to GLSL */
class IShaderControl : public IControl
{
public:
  enum EUniform
  {
    kTime,
    kWidth,
    kHeight,
    kX,
    kY,
    kL,
    kR,
    kNumUniforms
  };
  
  IShaderControl(const IRECT& bounds, const char* shaderStr = nullptr)
  : IControl(bounds)
  {
    mText.mAlign = EAlign::Near;
    mText.mVAlign = EVAlign::Top;
        
//    mTimer = std::unique_ptr<Timer>(Timer::Create([&](Timer& t) {
//
//      SetDirty(false);
//    }, 20));
        
//    SetActionFunction([&](IControl* pCaller) {
//      SetAnimation([&](IControl* pCaller) {
//        float p = pCaller->GetAnimationProgress();
//        mUniforms[kTime] = p;
//
//        if (p > 1.) {
//          pCaller->OnEndAnimation();
//        }
//
//      }, 10000);
//    });

    WDL_String err;
    
    SetShaderStr(shaderStr ? shaderStr :
    R"(
      uniform float uTime;
      uniform float2 uDim;
      uniform float2 uMouse;
      uniform float2 uMouseBut;

      half4 main(float2 fragCoord) {
       float2 pos = uMouse.xy/uDim.xy;
       return half4(pos.x, pos.y, 1, 1);
      }
    )", err);
    
    if(err.GetLength())
      DBGMSG("%s\n", err.Get());
  }

  void Draw(IGraphics& g) override
  {
    if(mRTEffect)
      DrawShader(g, GetShaderBounds());
    
//    WDL_String str;
//    str.SetFormatted(32, "%i:%i", (int) mUniforms[kX], (int) mUniforms[kY]);
//    g.DrawText(mText, str.Get(), mRECT);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    IRECT shaderBounds = GetShaderBounds();

    mUniforms[kX] = Clip(0.f, x - shaderBounds.L, shaderBounds.W());
    mUniforms[kY] = Clip(0.f, y - shaderBounds.T, shaderBounds.H());
    mUniforms[kL] = (float) mod.L ? 1.f : 0.f;
    mUniforms[kR] = (float) mod.R ? 1.f : 0.f;
    SetDirty(true);
  }
  
  void OnMouseUp(float x, float y, const IMouseMod &mod) override
  {
    IRECT shaderBounds = GetShaderBounds();

    mUniforms[kX] = Clip(0.f, x - shaderBounds.L, shaderBounds.W());
    mUniforms[kY] = Clip(0.f, y - shaderBounds.T, shaderBounds.H());
    mUniforms[kL] = 0.f;
    mUniforms[kR] = 0.f;
    SetDirty(false);
  }
  
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod &mod) override
  {
    IRECT shaderBounds = GetShaderBounds();
    mUniforms[kX] = Clip(0.f, x - shaderBounds.L, shaderBounds.W());
    mUniforms[kY] = Clip(0.f, y - shaderBounds.T, shaderBounds.H());
    SetDirty(false);
  }
  
  void OnResize() override
  {
    mUniforms[kWidth] = GetShaderBounds().W();
    mUniforms[kHeight] = GetShaderBounds().H();
    SetDirty(false);
  }
  
  bool SetShaderStr(const char* str, WDL_String& error)
  {
    mShaderStr = SkString(str);
    
    auto [effect, errorText] = SkRuntimeEffect::MakeForShader(mShaderStr);
    
    if (!effect)
    {
      error.Set(errorText.c_str());
      return false;
    }

    mRTEffect = effect;
    
    auto inputs = SkData::MakeWithoutCopy(mUniforms.data(), mRTEffect->uniformSize());
    auto shader = mRTEffect->makeShader(std::move(inputs), nullptr, 0, nullptr);
    mPaint.setShader(std::move(shader));

    return true;
  }
  
private:
  /* Override this method to only draw the shader in a sub region of the control's mRECT */
  virtual IRECT GetShaderBounds() const
  {
    return mRECT;
  }

  void DrawShader(IGraphics& g, const IRECT& r)
  {
    SkCanvas* canvas = static_cast<SkCanvas*>(g.GetDrawContext());
    canvas->save();
    canvas->translate(r.L, r.T);
    canvas->drawRect({ 0, 0, r.W(), r.H() }, mPaint);
    canvas->restore();
  }

//  std::unique_ptr<Timer> mTimer;
  SkPaint mPaint;
  SkString mShaderStr;
  sk_sp<SkRuntimeEffect> mRTEffect;
  std::array<float, kNumUniforms> mUniforms {0.f};
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
