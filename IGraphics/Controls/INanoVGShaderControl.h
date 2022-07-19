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
#if defined IGRAPHICS_GL2
    R"(
      attribute vec4 apos;
      attribute vec4 acolor;
      varying vec4 color;
      void main() {
        color = acolor;
        gl_Position = apos;
      }
    )"
#elif defined IGRAPHICS_GL3
#elif defined IGRAPHICS_METAL
#endif
    );
    
    // "precision lowp float;"
    SetFragmentShaderStr(
#if defined IGRAPHICS_GL2
    R"(
      varying vec4 color;
      void main() {
        gl_FragColor = color;
      }
    )"
#elif defined IGRAPHICS_GL3
#elif defined IGRAPHICS_GL3
#endif
   );
  }
  
  virtual ~INanoVGShaderControl()
  {
    if (mFBO)
      nvgDeleteFramebuffer(mFBO);
  }
  
  void OnAttached() override
  {
    WDL_String err;
    GLuint vs = CompileShader(GL_VERTEX_SHADER, mVertexShaderStr.Get(), err);
    
    if (err.GetLength())
    {
      DBGMSG("Error compiling vertex shader: %s\n", err.Get());
      return;
    }

    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, mFragmentShaderStr.Get(), err);
    
    if (err.GetLength())
    {
      DBGMSG("Error compiling fragment shader: %s\n", err.Get());
      return;
    }

    mProgram = CreateProgram(vs, fs);
  }
  
  void Draw(IGraphics& g) override
  {
    NVGcontext* vg = reinterpret_cast<NVGcontext*>(g.GetDrawContext());
    int w = static_cast<int>(mRECT.W() * g.GetDrawScale());
    int h = static_cast<int>(mRECT.H() * g.GetDrawScale());
    
    if (invalidateFBO)
    {
      if (mFBO)
        nvgDeleteFramebuffer(mFBO);
      
      mFBO = nvgCreateFramebuffer(vg, w, h, 0);
      
      invalidateFBO = false;
    }

    PreDraw(g);
    
    nvgEndFrame(vg);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mInitialFBO);

    nvgBindFramebuffer(mFBO);
    nvgBeginFrame(vg, static_cast<float>(w),
                      static_cast<float>(h),
                      static_cast<float>(g.GetScreenScale()));
    
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);

    glViewport(0, 0, w, h);
    glScissor(0, 0, w, h);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glUseProgram(mProgram);
    
    DrawToFBO(w, h);
    
    glViewport(vp[0], vp[1], vp[2], vp[3]);
    
    nvgEndFrame(vg);
    glBindFramebuffer(GL_FRAMEBUFFER, mInitialFBO);
    nvgBeginFrame(vg, static_cast<float>(g.WindowWidth()),
                      static_cast<float>(g.WindowHeight()),
                      static_cast<float>(g.GetScreenScale()));

    APIBitmap apibmp {mFBO->image, w, h, 1, 1.};
    IBitmap bmp {&apibmp, 1, false};
    
    g.DrawFittedBitmap(bmp, mRECT);
    
    PostDraw(g);
  }
  
  void OnResize() override
  {
    invalidateFBO = true;
  }
  
  void OnRescale() override
  {
    invalidateFBO = true;
  }
  
  void SetVertexShaderStr(const char* str)
  {
    mVertexShaderStr.Set(str);
  }
  
  void SetFragmentShaderStr(const char* str)
  {
    mFragmentShaderStr.Set(str);
  }
  
private:
  virtual void PreDraw(IGraphics& g) { /* NO-OP */ }
  virtual void PostDraw(IGraphics& g) { /* NO-OP */ }
  virtual void DrawToFBO(int w, int h) = 0;

  GLuint CompileShader(GLenum shaderType, const char* src, WDL_String& err)
  {
    GLuint shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint isCompiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
    
    if (!isCompiled)
    {
      GLint maxLength = 0;
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
      err.SetLen(maxLength);
      glGetShaderInfoLog(shader, maxLength, &maxLength, err.Get());
      return GLuint(0);
    }

    return shader;
  }

  GLuint CreateProgram(GLuint vertexShader, GLuint fragmentShader)
  {
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glBindAttribLocation(program, 0, "apos");
    //glBindAttribLocation(program, 1, "acolor");
    glBindAttribLocation(program, 1, "atexcoord"); // tmp
    glLinkProgram(program);
    return program;
  };
  
  WDL_String mVertexShaderStr;
  WDL_String mFragmentShaderStr;
protected: // tmp
  GLuint mProgram;
private:
  NVGframebuffer* mFBO = nullptr;
  int mInitialFBO = 0;
  bool invalidateFBO = true;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
