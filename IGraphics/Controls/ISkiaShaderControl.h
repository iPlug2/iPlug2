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
#include <unordered_map>
#include <string>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Global cache for compiled Skia shaders.
 * Multiple ISkiaShaderControl instances with the same shader source
 * will share the compiled SkRuntimeEffect, avoiding redundant compilation. */
class SkiaShaderCache
{
public:
  static SkiaShaderCache& Get()
  {
    static SkiaShaderCache instance;
    return instance;
  }

  /** Get or compile a shader effect.
   * @param shaderStr The SkSL shader source code
   * @param error Output: error message if compilation fails
   * @return Shared pointer to compiled effect, or nullptr on error */
  sk_sp<SkRuntimeEffect> GetOrCompile(const char* shaderStr, WDL_String& error)
  {
    std::string key(shaderStr);

    auto it = mCache.find(key);
    if (it != mCache.end())
    {
      DBGMSG("SkiaShaderCache: Reusing cached shader (hash=%zu)\n", std::hash<std::string>{}(key));
      return it->second;
    }

    DBGMSG("SkiaShaderCache: Compiling new shader (hash=%zu)\n", std::hash<std::string>{}(key));
    auto [effect, errorText] = SkRuntimeEffect::MakeForShader(SkString(shaderStr));

    if (!effect)
    {
      error.Set(errorText.c_str());
      return nullptr;
    }

    mCache[key] = effect;
    return effect;
  }

  /** Clear all cached shaders */
  void Clear() { mCache.clear(); }

private:
  SkiaShaderCache() = default;
  std::unordered_map<std::string, sk_sp<SkRuntimeEffect>> mCache;
};

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
   * @param shaderStr SkSL shader source (nullptr for no shader)
   * @param animate If true, continuously animates */
  ISkiaShaderControl(const IRECT& bounds, const char* shaderStr = nullptr, bool animate = false)
  : IShaderControlBase(bounds, animate)
  {
    if (shaderStr)
    {
      WDL_String err;
      if (!SetShaderStr(shaderStr, err))
      {
        DBGMSG("ISkiaShaderControl: Shader compile error: %s\n", err.Get());
      }
    }
  }

  void Draw(IGraphics& g) override
  {
    // No shader loaded - show placeholder
    if (mShaderStr.isEmpty())
    {
      g.FillRect(COLOR_DARK_GRAY, mRECT);
      g.DrawText(IText(14, COLOR_GRAY), "No shader", mRECT);
      return;
    }

    // Shader failed to compile
    if (!mRTEffect)
    {
      g.FillRect(COLOR_DARK_GRAY, mRECT);
      g.DrawText(IText(14, COLOR_RED), "Shader error", mRECT);
      return;
    }

    if (mAnimate || IsDirty())
      UpdateTime();

    DrawShader(g, GetShaderBounds());
  }

  bool SetShaderStr(const char* shaderStr, WDL_String& error) override
  {
    mShaderStr = SkString(shaderStr);

    // Use cache to avoid recompiling identical shaders
    mRTEffect = SkiaShaderCache::Get().GetOrCompile(shaderStr, error);

    if (!mRTEffect)
      return false;

    RebuildShader();
    return true;
  }

  bool IsShaderValid() const override
  {
    return mRTEffect != nullptr;
  }

protected:
  void OnShaderResize(int w, int h) override
  {
    // Shader references mUniformData directly, no rebuild needed
  }

private:
  /** Uniform data structure matching the expected SkSL layout.
   * The shader references this memory directly via MakeWithoutCopy,
   * so uniform updates are automatic without per-frame allocations. */
  struct alignas(4) UniformData
  {
    float time = 0.f;
    float resolution[2] = {0.f, 0.f};
    float mouse[2] = {0.f, 0.f};
    float mouseButtons[2] = {0.f, 0.f};
  };

  /** Rebuild shader instance - only needed when shader source changes */
  void RebuildShader()
  {
    if (!mRTEffect)
      return;

    // Use MakeWithoutCopy so shader reads directly from mUniformData
    // This avoids allocations on every frame - just update mUniformData
    auto uniformData = SkData::MakeWithoutCopy(&mUniformData, sizeof(mUniformData));
    auto shader = mRTEffect->makeShader(std::move(uniformData), nullptr, 0);
    mPaint.setShader(std::move(shader));
  }

  /** Sync base class uniforms to our packed struct before drawing */
  void SyncUniforms()
  {
    mUniformData.time = mUniforms[static_cast<int>(EShaderUniform::Time)];
    mUniformData.resolution[0] = mUniforms[static_cast<int>(EShaderUniform::Width)];
    mUniformData.resolution[1] = mUniforms[static_cast<int>(EShaderUniform::Height)];
    mUniformData.mouse[0] = mUniforms[static_cast<int>(EShaderUniform::MouseX)];
    mUniformData.mouse[1] = mUniforms[static_cast<int>(EShaderUniform::MouseY)];
    mUniformData.mouseButtons[0] = mUniforms[static_cast<int>(EShaderUniform::MouseL)];
    mUniformData.mouseButtons[1] = mUniforms[static_cast<int>(EShaderUniform::MouseR)];
  }

  /** Draw the shader to the given bounds */
  void DrawShader(IGraphics& g, const IRECT& r)
  {
    SyncUniforms();

    SkCanvas* canvas = static_cast<SkCanvas*>(g.GetDrawContext());
    canvas->save();
    canvas->translate(r.L, r.T);
    canvas->drawRect({0, 0, r.W(), r.H()}, mPaint);
    canvas->restore();
  }

  SkPaint mPaint;
  SkString mShaderStr;
  sk_sp<SkRuntimeEffect> mRTEffect;
  UniformData mUniformData;  // Persistent buffer - shader reads directly from here
  bool mAnimate = false;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
