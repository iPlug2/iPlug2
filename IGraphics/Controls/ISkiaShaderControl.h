/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#ifndef IGRAPHICS_SKIA
#error ISkiaShaderControl requires the Skia graphics backend
#endif

/**
 * @file
 * @brief Skia implementation of shader control using SkRuntimeEffect
 * @copydoc ISkiaShaderControl
 */

#include "IShaderControlBase.h"
#include "include/effects/SkRuntimeEffect.h"
#include "include/core/SkCanvas.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Default SkSL shader - gradient based on mouse position */
static const char* kDefaultSkSLShader = R"(
uniform float uTime;
uniform float2 uResolution;
uniform float2 uMouse;
uniform float2 uMouseButtons;

half4 main(float2 fragCoord) {
  float2 uv = fragCoord / uResolution;
  float2 mouse = uMouse / uResolution;

  // Simple gradient based on position and mouse
  half3 col = half3(uv.x, uv.y, 0.5 + 0.5 * sin(uTime));
  col = mix(col, half3(1.0, 0.5, 0.2), smoothstep(0.2, 0.0, length(uv - mouse)));

  return half4(col, 1.0);
}
)";

/** Shader control implementation for the Skia backend.
 *
 * Uses Skia's SkRuntimeEffect to compile and execute shaders written in
 * SkSL (Skia Shading Language), which is similar to GLSL.
 *
 * ## SkSL Shader Format
 *
 * ```glsl
 * // Standard uniforms (automatically provided)
 * uniform float uTime;           // Time in seconds
 * uniform float2 uResolution;    // Viewport size
 * uniform float2 uMouse;         // Mouse position in pixels
 * uniform float2 uMouseButtons;  // (left, right) button state
 *
 * // Entry point - return color for each pixel
 * half4 main(float2 fragCoord) {
 *   // fragCoord is pixel position (0,0 = top-left)
 *   return half4(r, g, b, a);
 * }
 * ```
 *
 * ## Example: Shadertoy-style shader
 *
 * ```glsl
 * uniform float uTime;
 * uniform float2 uResolution;
 *
 * half4 main(float2 fragCoord) {
 *   float2 uv = fragCoord / uResolution;
 *   half3 col = half3(0.5 + 0.5 * cos(uTime + uv.xyx + half3(0, 2, 4)));
 *   return half4(col, 1.0);
 * }
 * ```
 *
 * @see IShaderControlBase for the common interface
 * @ingroup IControls
 */
class ISkiaShaderControl : public IShaderControlBase
{
public:
  /** Constructor with optional shader string
   * @param bounds The control's rectangular area
   * @param shaderStr SkSL shader source (nullptr for default shader)
   * @param animate If true, continuously animates */
  ISkiaShaderControl(const IRECT& bounds, const char* shaderStr = nullptr, bool animate = false)
  : IShaderControlBase(bounds, animate)
  {
    WDL_String err;
    if (!SetShaderStr(shaderStr ? shaderStr : kDefaultSkSLShader, err))
    {
      DBGMSG("ISkiaShaderControl: Shader compile error: %s\n", err.Get());
    }
  }

  void Draw(IGraphics& g) override
  {
    if (mAnimate || IsDirty())
      UpdateTime();

    if (mRTEffect)
      DrawShader(g, GetShaderBounds());
  }

  bool SetShaderStr(const char* shaderStr, WDL_String& error) override
  {
    mShaderStr = SkString(shaderStr);

    auto [effect, errorText] = SkRuntimeEffect::MakeForShader(mShaderStr);

    if (!effect)
    {
      error.Set(errorText.c_str());
      mRTEffect = nullptr;
      return false;
    }

    mRTEffect = effect;
    UpdateShader();
    return true;
  }

  bool IsShaderValid() const override
  {
    return mRTEffect != nullptr;
  }

protected:
  void OnShaderResize(int w, int h) override
  {
    // Shader will use updated uniform values on next draw
  }

private:
  /** Rebuild the shader with current uniform values */
  void UpdateShader()
  {
    if (!mRTEffect)
      return;

    // Build uniform data matching our standard layout
    struct UniformData
    {
      float time;
      float resolution[2];
      float mouse[2];
      float mouseButtons[2];
    } data;

    data.time = mUniforms[static_cast<int>(EShaderUniform::Time)];
    data.resolution[0] = mUniforms[static_cast<int>(EShaderUniform::Width)];
    data.resolution[1] = mUniforms[static_cast<int>(EShaderUniform::Height)];
    data.mouse[0] = mUniforms[static_cast<int>(EShaderUniform::MouseX)];
    data.mouse[1] = mUniforms[static_cast<int>(EShaderUniform::MouseY)];
    data.mouseButtons[0] = mUniforms[static_cast<int>(EShaderUniform::MouseL)];
    data.mouseButtons[1] = mUniforms[static_cast<int>(EShaderUniform::MouseR)];

    auto uniformData = SkData::MakeWithCopy(&data, sizeof(data));
    auto shader = mRTEffect->makeShader(std::move(uniformData), nullptr, 0);
    mPaint.setShader(std::move(shader));
  }

  /** Draw the shader to the given bounds */
  void DrawShader(IGraphics& g, const IRECT& r)
  {
    UpdateShader();

    SkCanvas* canvas = static_cast<SkCanvas*>(g.GetDrawContext());
    canvas->save();
    canvas->translate(r.L, r.T);
    canvas->drawRect({0, 0, r.W(), r.H()}, mPaint);
    canvas->restore();
  }

  SkPaint mPaint;
  SkString mShaderStr;
  sk_sp<SkRuntimeEffect> mRTEffect;
  bool mAnimate = false;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
