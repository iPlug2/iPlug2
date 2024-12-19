/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "INanoVGShaderControl.h"

using namespace iplug;
using namespace igraphics;

#if defined IGRAPHICS_GL

static GLuint CompileGLShader(GLenum shaderType, const char* src, WDL_String& err)
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
};

static GLuint CreateProgram(GLuint vertexShader, GLuint fragmentShader)
{
  GLuint program = glCreateProgram();
  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glBindAttribLocation(program, 0, "apos");
  glBindAttribLocation(program, 1, "atexcoord");
  glLinkProgram(program);
  return program;
};

void INanoVGShaderControl::OnAttached()
{
  WDL_String err;
  GLuint vs = CompileGLShader(GL_VERTEX_SHADER, mVertexShaderStr.Get(), err);

  if (err.GetLength())
  {
    DBGMSG("Error compiling vertex shader: %s\n", err.Get());
    return;
  }

  GLuint fs = CompileGLShader(GL_FRAGMENT_SHADER, mFragmentShaderStr.Get(), err);

  if (err.GetLength())
  {
    DBGMSG("Error compiling fragment shader: %s\n", err.Get());
    return;
  }

  mProgram = CreateProgram(vs, fs);
}

void INanoVGShaderControl::Draw(IGraphics& g)
{
  NVGcontext* vg = reinterpret_cast<NVGcontext*>(g.GetDrawContext());
  int w = static_cast<int>(mRECT.W() * g.GetDrawScale());
  int h = static_cast<int>(mRECT.H() * g.GetDrawScale());
  
  if (mInvalidateFBO)
  {
    if (mFBO)
      nvgDeleteFramebuffer(mFBO);
    
    mFBO = nvgCreateFramebuffer(vg, w, h, 0);
    
    mInvalidateFBO = false;
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

#endif
