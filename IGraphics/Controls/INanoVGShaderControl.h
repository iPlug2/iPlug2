/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#ifndef IGRAPHICS_NANOVG
#error This IControl only works with the NanoVG graphics backend
#endif

/**
 * @file
 * @ingroup Controls
 * @copydoc INanoVGShaderControl
 */

#include "IPlugPlatform.h"
#include "IGraphics_select.h"

#include "IControl.h"

#include "IGraphicsNanoVG.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/* Base class for NanoVG controls that draw to a framebuffer with OpenGL or Metal */
class INanoVGShaderControl : public IControl
{
public:
  INanoVGShaderControl(const IRECT& bounds)
  : IControl(bounds)
  {
    SetVertexShaderStr(
#if defined IGRAPHICS_GL
    R"(
      attribute vec4 apos;
      attribute vec4 acolor;
      varying vec4 color;
      void main() {
        color = acolor;
        gl_Position = apos;
      }
    )"
#elif defined IGRAPHICS_METAL
    R"(using namespace metal;
    
    #include <simd/simd.h>

    typedef enum VertexInputIndex
    {
      VertexInputIndexVertices = 0,
      VertexInputIndexAspectRatio = 1,
    } VertexInputIndex;

    typedef struct
    {
      vector_float2 position;
      vector_float4 color;
    } SimpleVertex;

    struct VertexOut {
      float4 position [[position]];
      float4 color;
    };

    vertex VertexOut vertex_main(const uint vertexID [[ vertex_id ]],
                   const device SimpleVertex *vertices [[ buffer(VertexInputIndexVertices) ]])
    {
      VertexOut out;
      
      out.position = vector_float4(0.0, 0.0, 0.0, 1.0);
      out.position.xy = vertices[vertexID].position.xy;
      
      out.color = vertices[vertexID].color;
      
      return out;
    })"
#endif
    );
    
    SetFragmentShaderStr(
#if defined IGRAPHICS_GL
    R"(
      varying vec4 color;
      void main() {
        gl_FragColor = color;
      }
    )"
#elif defined IGRAPHICS_METAL
   R"(using namespace metal;

   struct VertexOut {
     float4 position [[position]];
     float4 color;
   };

   fragment float4 fragment_main(VertexOut vert [[stage_in]]) {
       return vert.color;
   })"
#endif
   );
  }
  
  virtual ~INanoVGShaderControl()
  {
    if (mFBO)
      nvgDeleteFramebuffer(mFBO);
  }
  
  void OnAttached() override;
  
  void Draw(IGraphics& g) override;
  
  void OnResize() override
  {
    mInvalidateFBO = true;
  }
  
  void OnRescale() override
  {
    mInvalidateFBO = true;
  }
  
  void SetVertexShaderStr(const char* str, const char* glesPrecision = "lowp", const char* gl3version = "330 core")
  {
    mVertexShaderStr.Set("");
    
#if defined IGRAPHICS_GL3
    mVertexShaderStr.AppendFormatted(128, "#version %s\n", gl3version);
#endif
    
#if defined IGRAPHICS_GLES2 || defined IGRAPHICS_GLES3
    if (strlen(glesPrecision) > 0)
      mVertexShaderStr.AppendFormatted(128, "precision %s float;\n", glesPrecision);
#endif
    mVertexShaderStr.Append(str);
  }
  
  void SetFragmentShaderStr(const char* str, const char* glesPrecision = "lowp", const char* gl3version = "330 core")
  {
    mFragmentShaderStr.Set("");
    
#if defined IGRAPHICS_GL3
    mFragmentShaderStr.AppendFormatted(128, "#version %s\n", gl3version);
#endif
    
#if defined IGRAPHICS_GLES2 || defined IGRAPHICS_GLES3
    if (strlen(glesPrecision) > 0)
      mFragmentShaderStr.AppendFormatted(128, "precision %s float;\n", glesPrecision);
#endif
    mFragmentShaderStr.Append(str);
  }
  
#ifdef IGRAPHICS_GL
  int GetProgram() const { return mProgram; }
#endif
  
private:
  virtual void PreDraw(IGraphics& g) { /* NO-OP */ }
  virtual void PostDraw(IGraphics& g) { /* NO-OP */ }
  virtual void DrawToFBO(int w, int h) = 0;
  
  WDL_String mVertexShaderStr;
  WDL_String mFragmentShaderStr;

#if defined IGRAPHICS_GL
  GLuint mProgram;
  int mInitialFBO = 0;
#elif defined IGRAPHICS_METAL
  void* mRenderPassDescriptor = nullptr;
  void* mRenderPipeline = nullptr;
#endif

private:
  NVGframebuffer* mFBO = nullptr;
  bool mInvalidateFBO = true;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
