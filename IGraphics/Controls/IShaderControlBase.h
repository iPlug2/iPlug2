/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @brief Base class for shader controls providing backend-agnostic interface
 * @copydoc IShaderControlBase
 */

#include "IControl.h"
#include "ISender.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Standard uniform indices for shader controls.
 * Backend implementations should map these to their shader uniform system. */
enum class EShaderUniform
{
  Time = 0,       ///< Elapsed time in seconds
  Width,          ///< Viewport width in pixels
  Height,         ///< Viewport height in pixels
  MouseX,         ///< Mouse X position (relative to control)
  MouseY,         ///< Mouse Y position (relative to control)
  MouseL,         ///< Left mouse button state (0 or 1)
  MouseR,         ///< Right mouse button state (0 or 1)
  NumStandardUniforms
};

/** Maximum audio samples that can be sent to shader */
static constexpr int kShaderMaxAudioSamples = 512;

/** Data structure for sending audio to shader controls */
struct ShaderAudioData
{
  float left[kShaderMaxAudioSamples] = {0};
  float right[kShaderMaxAudioSamples] = {0};
  int numSamples = 0;
};

/** Abstract base class for shader-based controls.
 *
 * This class provides a backend-agnostic interface for creating controls that
 * render using GPU shaders. Concrete implementations exist for each IGraphics
 * backend:
 * - ISkiaShaderControl: Skia backend using SkSL (Skia Shading Language)
 * - INanoVGGLShaderControl: NanoVG+OpenGL using GLSL
 * - INanoVGMTLShaderControl: NanoVG+Metal using MSL
 *
 * ## Shader Language Differences
 *
 * Each backend uses its native shader language. The standard uniforms are:
 *
 * ### SkSL (Skia)
 * ```glsl
 * uniform float uTime;
 * uniform float2 uResolution;
 * uniform float2 uMouse;
 * uniform float2 uMouseButtons;
 * // Entry point: half4 main(float2 fragCoord)
 * ```
 *
 * ### GLSL (NanoVG+GL)
 * ```glsl
 * uniform float uTime;
 * uniform vec2 uResolution;
 * uniform vec2 uMouse;
 * uniform vec2 uMouseButtons;
 * // Entry point: void main() { gl_FragColor = ...; }
 * ```
 *
 * ### MSL (NanoVG+Metal)
 * ```metal
 * struct Uniforms {
 *   float time;
 *   float2 resolution;
 *   float2 mouse;
 *   float2 mouseButtons;
 * };
 * // Entry point: fragment half4 fragmentShader(...)
 * ```
 *
 * @ingroup IControls
 */
class IShaderControlBase : public IControl
{
public:
  /** Constructor
   * @param bounds The control's rectangular area
   * @param animate If true, continuously redraws for animation (default: false) */
  IShaderControlBase(const IRECT& bounds, bool animate = false)
  : IControl(bounds)
  , mAnimate(animate)
  {
    mIgnoreMouse = false;
  }

  virtual ~IShaderControlBase() = default;

  void OnInit() override
  {
    mStartTime = std::chrono::high_resolution_clock::now();
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    UpdateMouseState(x, y, mod.L, mod.R);
    SetDirty(true);
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    UpdateMouseState(x, y, false, false);
    SetDirty(false);
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    UpdateMouseState(x, y, mod.L, mod.R);
    SetDirty(false);
  }

  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    UpdateMouseState(x, y, mod.L, mod.R);
    if (mAnimate)
      SetDirty(false);
  }

  void OnResize() override
  {
    IRECT shaderBounds = GetShaderBounds();
    mUniforms[static_cast<int>(EShaderUniform::Width)] = shaderBounds.W();
    mUniforms[static_cast<int>(EShaderUniform::Height)] = shaderBounds.H();
    OnShaderResize(static_cast<int>(shaderBounds.W()), static_cast<int>(shaderBounds.H()));
    SetDirty(false);
  }

  bool IsDirty() override
  {
    if (mAnimate)
    {
      UpdateTime();
      return true;
    }
    return IControl::IsDirty();
  }

  /** Set the shader source code.
   * @param shaderStr The shader source in the backend's native language
   * @param error Output: error message if compilation fails
   * @return true if shader compiled successfully */
  virtual bool SetShaderStr(const char* shaderStr, WDL_String& error) = 0;

  /** Check if the shader is valid and ready to render */
  virtual bool IsShaderValid() const = 0;

  /** Enable or disable animation mode.
   * When enabled, the control continuously redraws and updates the time uniform.
   * @param animate true to enable animation */
  void SetAnimate(bool animate)
  {
    mAnimate = animate;
    if (animate)
      SetDirty(false);
  }

  /** Check if animation is enabled */
  bool GetAnimate() const { return mAnimate; }

  /** Set a custom uniform value.
   * @param index Uniform index (use values >= NumStandardUniforms for custom)
   * @param value The uniform value */
  void SetUniform(int index, float value)
  {
    if (index >= 0 && index < kMaxUniforms)
      mUniforms[index] = value;
  }

  /** Get a uniform value.
   * @param index Uniform index
   * @return The uniform value, or 0 if index is invalid */
  float GetUniform(int index) const
  {
    if (index >= 0 && index < kMaxUniforms)
      return mUniforms[index];
    return 0.f;
  }

  /** Get current time in seconds since control initialization */
  float GetTime() const
  {
    return mUniforms[static_cast<int>(EShaderUniform::Time)];
  }

  /** Reset the time counter to zero */
  void ResetTime()
  {
    mStartTime = std::chrono::high_resolution_clock::now();
    mUniforms[static_cast<int>(EShaderUniform::Time)] = 0.f;
  }

  /** Update audio data for visualization.
   * Call this from your plugin's ProcessBlock to send audio to the shader.
   * @param data Audio sample data */
  void SetAudioData(const ShaderAudioData& data)
  {
    mAudioData = data;
    mHasAudioData = true;
  }

  /** Clear audio data */
  void ClearAudioData()
  {
    mHasAudioData = false;
  }

  /** Check if audio data is available */
  bool HasAudioData() const { return mHasAudioData; }

  /** Get the current audio data */
  const ShaderAudioData& GetAudioData() const { return mAudioData; }

protected:
  /** Maximum number of uniforms (standard + custom) */
  static constexpr int kMaxUniforms = 32;

  /** Get the bounds where the shader should be rendered.
   * Override to render shader in a sub-region of the control.
   * @return The shader rendering bounds */
  virtual IRECT GetShaderBounds() const { return mRECT; }

  /** Called when shader bounds change.
   * Override to recreate FBOs or other size-dependent resources.
   * @param w New width in pixels
   * @param h New height in pixels */
  virtual void OnShaderResize(int w, int h) {}

  /** Update time uniform from system clock */
  void UpdateTime()
  {
    auto now = std::chrono::high_resolution_clock::now();
    std::chrono::duration<float> elapsed = now - mStartTime;
    mUniforms[static_cast<int>(EShaderUniform::Time)] = elapsed.count();
  }

  /** Update mouse state uniforms */
  void UpdateMouseState(float x, float y, bool left, bool right)
  {
    IRECT bounds = GetShaderBounds();
    mUniforms[static_cast<int>(EShaderUniform::MouseX)] = Clip(x - bounds.L, 0.f, bounds.W());
    mUniforms[static_cast<int>(EShaderUniform::MouseY)] = Clip(y - bounds.T, 0.f, bounds.H());
    mUniforms[static_cast<int>(EShaderUniform::MouseL)] = left ? 1.f : 0.f;
    mUniforms[static_cast<int>(EShaderUniform::MouseR)] = right ? 1.f : 0.f;
  }

  /** Array of uniform values */
  float mUniforms[kMaxUniforms] = {0.f};

  /** Audio data for visualization */
  ShaderAudioData mAudioData;

  /** Whether audio data is available */
  bool mHasAudioData = false;

private:
  std::chrono::high_resolution_clock::time_point mStartTime;
  bool mAnimate = false;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
