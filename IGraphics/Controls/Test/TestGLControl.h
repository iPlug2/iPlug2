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

 /** Control to test Drawing in 3D in supporting backends
  *   @ingroup TestControls */
class TestGLControl : public IControl
{
public:
  TestGLControl(IRECT rect)
    : IControl(rect, kNoParameter)
  {
    SetTooltip("TestGLControl");

    SetActionFunction([&](IControl* pCaller) {

      SetAnimation([&](IControl* pCaller)
      {
        auto progress = pCaller->GetAnimationProgress();

        mXRotation += 10;
        mYRotation += 20;

        pCaller->SetDirty(false);

        if (progress > 1.) {
          pCaller->OnEndAnimation();
          return;
        }

      }, 1000);
    });
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    SetDirty(true);
  }

  void Draw(IGraphics& g) override
  {
    g.DrawDottedRect(COLOR_BLACK, mRECT);

#ifdef IGRAPHICS_GL
    g.FillRect(mMouseIsOver ? COLOR_TRANSLUCENT : COLOR_TRANSPARENT, mRECT);

    if (!g.CheckLayer(mLayer))
    {
      g.StartLayer(mRECT);

      GLint vp[4];
      glGetIntegerv(GL_VIEWPORT, vp);

      glViewport(0, 0, mRECT.W() * g.GetDrawScale(), mRECT.H() * g.GetDrawScale());

      glEnable(GL_SCISSOR_TEST);
      glEnable(GL_CULL_FACE);
      glEnable(GL_DEPTH_TEST);

      glScissor(0, 0, mRECT.W() * g.GetDrawScale(), mRECT.H() * g.GetDrawScale());
      glClearColor(0.f, 0.f, 0.f, 0.f);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

      DrawGL();

      glDisable(GL_SCISSOR_TEST);
      glDisable(GL_CULL_FACE);
      glDisable(GL_DEPTH_TEST);

      glViewport(vp[0], vp[1], vp[2], vp[3]);

      mLayer = g.EndLayer();
    }

    g.DrawLayer(mLayer);
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

  void SetDirty(bool push) override
  {
    if(mLayer)
      mLayer->Invalidate();

    IControl::SetDirty(push);
  }

private:
  ILayerPtr mLayer;
  double mYRotation = 0;
  double mXRotation = 0;
};
