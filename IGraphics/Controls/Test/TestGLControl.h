/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc TestGLControl
 */

#include "IControl.h"
#include "IGraphics_select.h"

#ifdef IGRAPHICS_NANOVG
 /** Control to test Drawing in 3D in supporting backends
  *   @ingroup TestControls */
class TestGLControl : public IControl
{
public:
  TestGLControl(const IRECT& rect)
    : IControl(rect, kNoParameter)
  {
    SetTooltip("TestGLControl");

//    SetActionFunction([&](IControl* pCaller)
//    {
//      SetAnimation([&](IControl* pCaller)
//      {
//        auto progress = pCaller->GetAnimationProgress();
//
//        mXRotation += progress * 5.;
//        mYRotation += progress * 10.;
//
//        pCaller->SetDirty(false);
//
//        if (progress > 1.) {
//          pCaller->OnEndAnimation();
//          return;
//        }
//
//      }, 1000);
//    });
  }
  
  ~TestGLControl()
  {
    if (mFBO)
      nvgDeleteFramebuffer(mFBO);
  }

  void Draw(IGraphics& g) override
  {
    NVGcontext* vg = static_cast<NVGcontext*>(g.GetDrawContext());
    int w = static_cast<int>(mRECT.W() * g.GetDrawScale());
    int h = static_cast<int>(mRECT.H() * g.GetDrawScale());
    
    if(invalidateFBO)
    {
      if (mFBO)
        nvgDeleteFramebuffer(mFBO);
      
      mFBO = nvgCreateFramebuffer(vg, w, h, 0);
      
      invalidateFBO = false;
    }
    
    g.DrawDottedRect(COLOR_BLACK, mRECT);
    g.FillRect(mMouseIsOver ? COLOR_TRANSLUCENT : COLOR_TRANSPARENT, mRECT);

#ifdef IGRAPHICS_GL
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

    DrawGL();
    glViewport(vp[0], vp[1], vp[2], vp[3]);
    
    nvgEndFrame(vg);
    glBindFramebuffer(GL_FRAMEBUFFER, mInitialFBO);
    nvgBeginFrame(vg, static_cast<float>(g.WindowWidth()), static_cast<float>(g.WindowHeight()), static_cast<float>(g.GetScreenScale()));

    APIBitmap apibmp {mFBO->image, w, h, 1, 1.};
    IBitmap bmp {&apibmp, 1, false};
    
    g.DrawFittedBitmap(bmp, mRECT);
#else
    g.DrawText(mText, "UNSUPPORTED", mRECT);
#endif
  }
  
  void OnResize() override
  {
    invalidateFBO = true;
  }
  
  void OnRescale() override
  {
    invalidateFBO = true;
  }

#ifdef IGRAPHICS_GL
  void DrawGL()
  {
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
    "gl_FragColor = color*color2;"
    "}";
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fs_str);
    
    GLuint program = createProgram(vs, fs);
    glUseProgram(program);
    
    static const float posAndColor[] = {
      //     x,     y, r, g, b
      -0.6f, -0.6f, 1, 0, 0,
      0.6f, -0.6f, 0, 1, 0,
      0.f,   0.6f, 0, 0, 1,
    };
    
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(posAndColor), posAndColor, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 20, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 20, (void*)8);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    
    float color2[4] = { 0.0f, 1.f, 0.0f, 1.0f };
    glUniform4fv(glGetUniformLocation(program, "color2"), 1, color2);
    glDrawArrays(GL_TRIANGLES, 0, 3);
  }
#endif

private:
  NVGframebuffer* mFBO = nullptr;
  int mInitialFBO = 0;
  bool invalidateFBO = true;
};

#else
class TestGLControl : public IControl
{
public:
  TestGLControl(const IRECT& rect)
  : IControl(rect)
  {
    SetTooltip("TestGLControl");
  }
  
  void Draw(IGraphics& g) override
  {
    g.DrawText(mText, "UNSUPPORTED", mRECT);
  }
};
#endif
