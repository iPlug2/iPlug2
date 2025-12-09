/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#if !defined(IGRAPHICS_NANOVG) || !defined(IGRAPHICS_METAL)
#error INanoVGMTLShaderControl requires NanoVG with Metal backend
#endif

/**
 * @file
 * @brief NanoVG Metal implementation of shader control
 * @copydoc INanoVGMTLShaderControl
 */

#include "IShaderControlBase.h"
#include "IGraphicsNanoVG.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Shader control implementation for NanoVG with Metal backend.
 *
 * Uses Metal shader programs written in MSL (Metal Shading Language).
 * Renders to a Metal texture via a render pipeline, then composites
 * the result back to the NanoVG canvas.
 *
 * ## MSL Shader Format
 *
 * Your Metal shader library should contain vertex and fragment functions.
 * The control will look for functions named "shaderVertexFunc" and
 * "shaderFragmentFunc" by default, or you can specify custom names.
 *
 * ```metal
 * #include <metal_stdlib>
 * using namespace metal;
 *
 * struct Uniforms {
 *   float time;
 *   float2 resolution;
 *   float2 mouse;
 *   float2 mouseButtons;
 * };
 *
 * struct VertexOut {
 *   float4 position [[position]];
 *   float2 texCoord;
 * };
 *
 * vertex VertexOut shaderVertexFunc(uint vertexID [[vertex_id]],
 *                                    constant float2* vertices [[buffer(0)]]) {
 *   VertexOut out;
 *   out.position = float4(vertices[vertexID], 0.0, 1.0);
 *   out.texCoord = vertices[vertexID] * 0.5 + 0.5;
 *   return out;
 * }
 *
 * fragment half4 shaderFragmentFunc(VertexOut in [[stage_in]],
 *                                    constant Uniforms& uniforms [[buffer(0)]]) {
 *   float2 uv = in.texCoord;
 *   half3 col = half3(uv.x, uv.y, 0.5 + 0.5 * sin(uniforms.time));
 *   return half4(col, 1.0);
 * }
 * ```
 *
 * ## Usage
 *
 * 1. Create a .metal file with your shaders
 * 2. Add it to your Xcode project
 * 3. Instantiate INanoVGMTLShaderControl with function names
 *
 * Note: Unlike GLSL shaders which are compiled at runtime from strings,
 * Metal shaders must be pre-compiled into your app bundle.
 *
 * @see IShaderControlBase for the common interface
 * @ingroup IControls
 */
class INanoVGMTLShaderControl : public IShaderControlBase
{
public:
  /** Constructor
   * @param bounds The control's rectangular area
   * @param vertexFuncName Metal vertex function name (nullptr for default)
   * @param fragmentFuncName Metal fragment function name (nullptr for default)
   * @param animate If true, continuously animates */
  INanoVGMTLShaderControl(const IRECT& bounds,
                          const char* vertexFuncName = nullptr,
                          const char* fragmentFuncName = nullptr,
                          bool animate = false);

  ~INanoVGMTLShaderControl();

  void Draw(IGraphics& g) override;

  /** Not supported for Metal - shaders must be pre-compiled.
   * This method exists for interface compatibility but always returns false.
   * Use SetShaderFunctions() instead. */
  bool SetShaderStr(const char* shaderStr, WDL_String& error) override;

  /** Set the Metal shader function names to use.
   * @param vertexFuncName Name of vertex function in default library
   * @param fragmentFuncName Name of fragment function in default library
   * @param error Output: error message if functions not found
   * @return true if functions were found */
  bool SetShaderFunctions(const char* vertexFuncName,
                          const char* fragmentFuncName,
                          WDL_String& error);

  bool IsShaderValid() const override;

  void OnResize() override;
  void OnRescale() override;

protected:
  void OnShaderResize(int w, int h) override;

private:
  void CleanupMTL();
  bool SetupPipeline(WDL_String& error);

  // Opaque pointers to Objective-C objects
  void* mDevice = nullptr;           // id<MTLDevice>
  void* mCommandQueue = nullptr;     // id<MTLCommandQueue>
  void* mRenderPipeline = nullptr;   // id<MTLRenderPipelineState>
  void* mRenderPassDesc = nullptr;   // MTLRenderPassDescriptor*
  void* mUniformBuffer = nullptr;    // id<MTLBuffer>

  NVGframebuffer* mFBO = nullptr;

  const char* mVertexFuncName = nullptr;
  const char* mFragmentFuncName = nullptr;

  bool mNeedsSetup = true;
  bool mInvalidateFBO = true;
  bool mAnimate = false;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
