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
  TestGLControl(IRECT rect)
    : IControl(rect, kNoParameter)
  {
    SetTooltip("TestGLControl");

    SetActionFunction([&](IControl* pCaller)
    {
      SetAnimation([&](IControl* pCaller)
      {
        auto progress = pCaller->GetAnimationProgress();

        mXRotation += progress * 5.;
        mYRotation += progress * 10.;

        pCaller->SetDirty(false);

        if (progress > 1.) {
          pCaller->OnEndAnimation();
          return;
        }

      }, 1000);
    });
  }
  
  ~TestGLControl()
  {
    if (mFBO)
      nvgDeleteFramebuffer(mFBO);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    SetDirty(true);
  }

  void Draw(IGraphics& g) override
  {
    NVGcontext* vg = static_cast<NVGcontext*>(g.GetDrawContext());
    int w = static_cast<int>(mRECT.W() * g.GetDrawScale());
    int h = static_cast<int>(mRECT.H() * g.GetDrawScale());
    
    
    if(mFBO == nullptr)
      mFBO = nvgCreateFramebuffer(vg, w, h, 0);
    
    g.DrawDottedRect(COLOR_BLACK, mRECT);
    g.FillRect(mMouseIsOver ? COLOR_TRANSLUCENT : COLOR_TRANSPARENT, mRECT);

#ifdef IGRAPHICS_GL

    nvgEndFrame(vg);
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &mInitialFBO);

    nvgBindFramebuffer(mFBO);
    nvgBeginFrame(vg, w, h, g.GetScreenScale());
    GLint vp[4];
    glGetIntegerv(GL_VIEWPORT, vp);

    glViewport(0, 0, w, h);

    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);

    glScissor(0, 0, w, h);
    glClearColor(0.f, 0.f, 0.f, 0.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    DrawGL();

    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

    glViewport(vp[0], vp[1], vp[2], vp[3]);

    nvgEndFrame(vg);

    glBindFramebuffer(GL_FRAMEBUFFER, mInitialFBO);
    nvgBeginFrame(vg, g.WindowWidth(), g.WindowHeight(), g.GetScreenScale());

    APIBitmap apibmp {mFBO->image, w, h, 1, 1.};
    IBitmap bmp {&apibmp, 1, false};
    
    g.DrawFittedBitmap(bmp, mRECT);
#else
    g.DrawText(mText, "UNSUPPORTED", mRECT);
#endif
  }

#ifdef IGRAPHICS_GL
  //https://www.wikihow.com/Make-a-Cube-in-OpenGL
  void DrawGL()
  {
    // Reset transformations
    glLoadIdentity();

    // Rotate when user changes rotate_x and rotate_y
    glRotatef(mXRotation, 1.0, 0.0, 0.0);
    glRotatef(mYRotation, 0.0, 1.0, 0.0);

    // Yellow side - FRONT
    glBegin(GL_POLYGON);
    glColor3f(1.0, 1.0, 0.0); 
    glVertex3f(0.5, -0.5, -0.5);
    glVertex3f(0.5, 0.5, -0.5);
    glVertex3f(-0.5, 0.5, -0.5);
    glVertex3f(-0.5, -0.5, -0.5);
    glEnd();

    // White side - BACK
    glBegin(GL_POLYGON);
    glColor3f(1.0, 1.0, 1.0);
    glVertex3f(0.5, -0.5, 0.5);
    glVertex3f(0.5, 0.5, 0.5);
    glVertex3f(-0.5, 0.5, 0.5);
    glVertex3f(-0.5, -0.5, 0.5);
    glEnd();

    // Purple side - RIGHT
    glBegin(GL_POLYGON);
    glColor3f(1.0, 0.0, 1.0);
    glVertex3f(0.5, -0.5, -0.5);
    glVertex3f(0.5, 0.5, -0.5);
    glVertex3f(0.5, 0.5, 0.5);
    glVertex3f(0.5, -0.5, 0.5);
    glEnd();

    // Green side - LEFT
    glBegin(GL_POLYGON);
    glColor3f(0.0, 1.0, 0.0);
    glVertex3f(-0.5, -0.5, 0.5);
    glVertex3f(-0.5, 0.5, 0.5);
    glVertex3f(-0.5, 0.5, -0.5);
    glVertex3f(-0.5, -0.5, -0.5);
    glEnd();

    // Blue side - TOP
    glBegin(GL_POLYGON);
    glColor3f(0.0, 0.0, 1.0);
    glVertex3f(0.5, 0.5, 0.5);
    glVertex3f(0.5, 0.5, -0.5);
    glVertex3f(-0.5, 0.5, -0.5);
    glVertex3f(-0.5, 0.5, 0.5);
    glEnd();

    // Red side - BOTTOM
    glBegin(GL_POLYGON);
    glColor3f(1.0, 0.0, 0.0);
    glVertex3f(0.5, -0.5, -0.5);
    glVertex3f(0.5, -0.5, 0.5);
    glVertex3f(-0.5, -0.5, 0.5);
    glVertex3f(-0.5, -0.5, -0.5);
    glEnd();
  }
#endif

private:
  NVGframebuffer* mFBO = nullptr;
  int mInitialFBO = 0;
  double mYRotation = 0;
  double mXRotation = 0;
};

#endif
