/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

#include "IControl.h"
#if defined IGRAPHICS_NANOVG

/**
 * @file
 * @copydoc TestCustomShaderControl
 */

#include "IGraphicsNanoVG.h"

using namespace iplug;
using namespace igraphics;

/** Control to test IGraphicsNanoVG with a custom shader (OpenGL/Metal)
 *   @ingroup TestControls */
class TestCustomShaderControl : public IKnobControlBase
{
public:
  TestCustomShaderControl(const IRECT& bounds, int paramIdx)
  : IKnobControlBase(bounds, paramIdx)
  {
    SetTooltip("TestCustomShaderControl");
  }
  
#ifdef IGRAPHICS_GL
  ~TestCustomShaderControl()
  {
    if (mFBO)
      nvgDeleteFramebuffer(mFBO);
  }
  
  void Draw(IGraphics& g) override
  {
    NVGcontext* vg = static_cast<NVGcontext*>(g.GetDrawContext());
    int w = static_cast<int>(mRECT.W() * g.GetDrawScale());
    int h = static_cast<int>(mRECT.H() * g.GetDrawScale());
    
    if (invalidateFBO)
    {
      if (mFBO)
        nvgDeleteFramebuffer(mFBO);
      
      mFBO = nvgCreateFramebuffer(vg, w, h, 0);
      
      invalidateFBO = false;
    }
    
    g.DrawDottedRect(COLOR_BLACK, mRECT);
    g.FillRect(mMouseIsOver ? COLOR_TRANSLUCENT : COLOR_TRANSPARENT, mRECT);

    nvgEndFrame(vg);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mInitialFBO);

    nvgBindFramebuffer(mFBO);
    nvgBeginFrame(vg, static_cast<float>(w), static_cast<float>(h), static_cast<float>(g.GetScreenScale()));
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);

    glViewport(0, 0, w, h);

    glScissor(0, 0, w, h);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    // code from emscripten tests
    
    auto compileShader = [](GLenum shaderType, const char *src) {
      GLuint shader = glCreateShader(shaderType);
      glShaderSource(shader, 1, &src, NULL);
      glCompileShader(shader);
      
      GLint isCompiled = 0;
      glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
      if (!isCompiled)
      {
        GLint maxLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);
        char *buf = (char*)malloc(maxLength+1);
        glGetShaderInfoLog(shader, maxLength, &maxLength, buf);
        printf("%s\n", buf);
        free(buf);
        return GLuint(0);
      }
      
      return shader;
    };
    
    auto createProgram = [](GLuint vertexShader, GLuint fragmentShader) {
      GLuint program = glCreateProgram();
      glAttachShader(program, vertexShader);
      glAttachShader(program, fragmentShader);
      glBindAttribLocation(program, 0, "apos");
      glBindAttribLocation(program, 1, "acolor");
      glLinkProgram(program);
      return program;
    };
    
    printf("Supported GLSL version is %s.\n", (char *)glGetString(GL_SHADING_LANGUAGE_VERSION));

    static const char vs_str[] =
    "attribute vec4 apos;"
    "attribute vec4 acolor;"
    "varying vec4 color;"
    "void main() {"
    "color = acolor;"
    "gl_Position = apos;"
    "}";
    GLuint vs = compileShader(GL_VERTEX_SHADER, vs_str);
    
    static const char fs_str[] =
#ifdef OS_WEB
    "precision lowp float;"
#endif
    "varying vec4 color;"
    "uniform vec4 color2;"
    "void main() {"
    "gl_FragColor = color;"
    "}";
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fs_str);
    
    GLuint program = createProgram(vs, fs);
    glUseProgram(program);
    
    static const float posAndColor[] = {
    //     x,     y,    r,     g,  b
          -0.6f, -0.6f, 1.0, 0.0, 0.0,
           0.6f, -0.6f, 0.0, 1.0, 0.0,
           0.f,   0.6f, 0.0, 0.0, 1.0,
    };
    
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(posAndColor), posAndColor, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 20, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 20, (void*)8);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    
    glDrawArrays(GL_TRIANGLES, 0, 3);
    
    glViewport(vp[0], vp[1], vp[2], vp[3]);
    
    nvgEndFrame(vg);
    glBindFramebuffer(GL_FRAMEBUFFER, mInitialFBO);
    nvgBeginFrame(vg, static_cast<float>(g.WindowWidth()),
                      static_cast<float>(g.WindowHeight()),
                      static_cast<float>(g.GetScreenScale()));

    APIBitmap apibmp {mFBO->image, w, h, 1, 1.};
    IBitmap bmp {&apibmp, 1, false};
    
    g.DrawFittedBitmap(bmp, mRECT);
  }
  
  void OnResize() override
  {
    invalidateFBO = true;
  }
  
  void OnRescale() override
  {
    invalidateFBO = true;
  }
#elif defined IGRAPHICS_METAL
  ~TestCustomShaderControl();
  
  void CleanUp();
  
  void Draw(IGraphics& g) override;
#endif

private:
  NVGframebuffer* mFBO = nullptr;
  
#ifdef IGRAPHICS_METAL
  void* mRenderPassDescriptor = nullptr;
  void* mRenderPipeline = nullptr;
#else
  int mInitialFBO = 0;
#endif
  bool invalidateFBO = true;
};

#else
/** Control to test IGraphicsNanoVG with Metal Shaders */
class TestCustomShaderControl : public IControl
{
public:
  TestCustomShaderControl(const IRECT& bounds, int paramIdx)
  : IControl(bounds)
  {
    SetTooltip("TestCustomShaderControl");
  }
  
  void Draw(IGraphics& g) override
  {
    g.DrawText(mText, "UNSUPPORTED", mRECT);
  }
};
#endif
