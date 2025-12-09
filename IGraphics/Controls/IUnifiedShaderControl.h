/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file IUnifiedShaderControl.h
 * @brief Unified shader control that automatically selects the correct backend
 *
 * Include this header to get the appropriate shader control for your configured
 * graphics backend. The control provides a common interface across all backends
 * while allowing backend-specific shader code.
 *
 * ## Quick Start
 *
 * ```cpp
 * #include "IUnifiedShaderControl.h"
 *
 * // In your UI setup:
 * pGraphics->AttachControl(new IUnifiedShaderControl(bounds, shaderCode, true));
 * ```
 *
 * ## Backend-Specific Shader Languages
 *
 * The shader code you provide must match your configured backend:
 *
 * ### Skia (IGRAPHICS_SKIA) - SkSL
 * ```glsl
 * uniform float uTime;
 * uniform float2 uResolution;
 * uniform float2 uMouse;
 * uniform float2 uMouseButtons;
 *
 * half4 main(float2 fragCoord) {
 *   float2 uv = fragCoord / uResolution;
 *   return half4(uv.x, uv.y, 0.5 + 0.5 * sin(uTime), 1.0);
 * }
 * ```
 *
 * ### NanoVG + OpenGL (IGRAPHICS_NANOVG + IGRAPHICS_GL) - GLSL
 * ```glsl
 * // GL2/GLES2:
 * uniform float uTime;
 * uniform vec2 uResolution;
 * uniform vec2 uMouse;
 * uniform vec2 uMouseButtons;
 *
 * void main() {
 *   vec2 uv = gl_FragCoord.xy / uResolution;
 *   gl_FragColor = vec4(uv.x, uv.y, 0.5 + 0.5 * sin(uTime), 1.0);
 * }
 *
 * // GL3/GLES3: Use `#version 330 core`, `in`/`out`, `FragColor`
 * ```
 *
 * ### NanoVG + Metal (IGRAPHICS_NANOVG + IGRAPHICS_METAL) - MSL
 * Metal shaders must be pre-compiled in your .metal files.
 * Use INanoVGMTLShaderControl directly and call SetShaderFunctions().
 *
 * ## Standard Uniforms
 *
 * All backends provide these uniforms (with backend-appropriate types):
 * - `uTime` - Elapsed time in seconds
 * - `uResolution` - Viewport size in pixels (float2/vec2)
 * - `uMouse` - Mouse position in pixels (float2/vec2)
 * - `uMouseButtons` - Left/right button state (float2/vec2, 0.0 or 1.0)
 *
 * ## Cross-Platform Shader Code
 *
 * For projects targeting multiple backends, use preprocessor conditionals:
 *
 * ```cpp
 * #if defined(IGRAPHICS_SKIA)
 *   const char* shader = R"(
 *     uniform float uTime;
 *     uniform float2 uResolution;
 *     half4 main(float2 fragCoord) {
 *       return half4(fragCoord / uResolution, 0.5, 1.0);
 *     }
 *   )";
 * #elif defined(IGRAPHICS_GL)
 *   const char* shader = R"(
 *     uniform float uTime;
 *     uniform vec2 uResolution;
 *     void main() {
 *       gl_FragColor = vec4(gl_FragCoord.xy / uResolution, 0.5, 1.0);
 *     }
 *   )";
 * #endif
 * ```
 */

#include "IShaderControlBase.h"

#if defined(IGRAPHICS_SKIA)
  #include "ISkiaShaderControl.h"
  /** Alias for the shader control matching the current graphics backend */
  namespace iplug { namespace igraphics {
    using IUnifiedShaderControl = ISkiaShaderControl;
  }}

#elif defined(IGRAPHICS_NANOVG)
  #if defined(IGRAPHICS_GL)
    #include "INanoVGShaderControl.h"
    namespace iplug { namespace igraphics {
      using IUnifiedShaderControl = INanoVGGLShaderControl;
    }}
  #elif defined(IGRAPHICS_METAL)
    #include "INanoVGMTLShaderControl.h"
    namespace iplug { namespace igraphics {
      using IUnifiedShaderControl = INanoVGMTLShaderControl;
    }}
  #else
    #error IUnifiedShaderControl requires IGRAPHICS_GL or IGRAPHICS_METAL with NanoVG
  #endif

#else
  #error IUnifiedShaderControl requires IGRAPHICS_SKIA or IGRAPHICS_NANOVG
#endif
