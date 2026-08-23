// Copyright 2012-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef EXAMPLES_CUBE_VIEW_H
#define EXAMPLES_CUBE_VIEW_H

#define GL_SILENCE_DEPRECATION 1

#include "demo_utils.h"

#include "pugl/gl.h"
#include "pugl/pugl.h"

#include <stdbool.h>

// clang-format off

static const float cubeStripVertices[] = {
  -1.0f,  1.0f,  1.0f, // Front top left
   1.0f,  1.0f,  1.0f, // Front top right
  -1.0f, -1.0f,  1.0f, // Front bottom left
   1.0f, -1.0f,  1.0f, // Front bottom right
   1.0f, -1.0f, -1.0f, // Back bottom right
   1.0f,  1.0f,  1.0f, // Front top right
   1.0f,  1.0f, -1.0f, // Back top right
  -1.0f,  1.0f,  1.0f, // Front top left
  -1.0f,  1.0f, -1.0f, // Back top left
  -1.0f, -1.0f,  1.0f, // Front bottom left
  -1.0f, -1.0f, -1.0f, // Back bottom left
   1.0f, -1.0f, -1.0f, // Back bottom right
  -1.0f,  1.0f, -1.0f, // Back top left
   1.0f,  1.0f, -1.0f, // Back top right
};

static const float cubeStripColorVertices[] = {
  0.25f, 0.75f, 0.75f, // Front top left
  0.75f, 0.75f, 0.75f, // Front top right
  0.25f, 0.25f, 0.75f, // Front bottom left
  0.75f, 0.25f, 0.75f, // Front bottom right
  0.75f, 0.25f, 0.25f, // Back bottom right
  0.75f, 0.75f, 0.75f, // Front top right
  0.75f, 0.75f, 0.25f, // Back top right
  0.25f, 0.75f, 0.75f, // Front top left
  0.25f, 0.75f, 0.25f, // Back top left
  0.25f, 0.25f, 0.75f, // Front bottom left
  0.25f, 0.25f, 0.25f, // Back bottom left
  0.75f, 0.25f, 0.25f, // Back bottom right
  0.25f, 0.75f, 0.25f, // Back top left
  0.75f, 0.75f, 0.25f, // Back top right
};

static const float cubeFrontLineLoop[] = {
  -1.0f,  1.0f,  1.0f, // Front top left
   1.0f,  1.0f,  1.0f, // Front top right
   1.0f, -1.0f,  1.0f, // Front bottom right
  -1.0f, -1.0f,  1.0f, // Front bottom left
};

static const float cubeFrontLineLoopColors[] = {
  0.25f, 0.75f, 0.75f, // Front top left
  0.75f, 0.75f, 0.75f, // Front top right
  0.75f, 0.25f, 0.75f, // Front bottom right
  0.25f, 0.25f, 0.75f, // Front bottom left
};

static const float cubeBackLineLoop[] = {
  -1.0f,  1.0f, -1.0f, // Back top left
   1.0f,  1.0f, -1.0f, // Back top right
   1.0f, -1.0f, -1.0f, // Back bottom right
  -1.0f, -1.0f, -1.0f, // Back bottom left
};

static const float cubeBackLineLoopColors[] = {
  0.25f, 0.75f, 0.25f, // Back top left
  0.75f, 0.75f, 0.25f, // Back top right
  0.75f, 0.25f, 0.25f, // Back bottom right
  0.25f, 0.25f, 0.25f, // Back bottom left
};

static const float cubeSideLines[] = {
  -1.0f,  1.0f,  1.0f, // Front top left
  -1.0f,  1.0f, -1.0f, // Back top left

  -1.0f, -1.0f,  1.0f, // Front bottom left
  -1.0f, -1.0f, -1.0f, // Back bottom left

   1.0f,  1.0f,  1.0f, // Front top right
   1.0f,  1.0f, -1.0f, // Back top right

   1.0f, -1.0f,  1.0f, // Front bottom right
   1.0f, -1.0f, -1.0f, // Back bottom right
};

static const float cubeSideLineColors[] = {
  0.25f, 0.75f, 0.75f, // Front top left
  0.25f, 0.75f, 0.25f, // Back top left

  0.25f, 0.25f, 0.75f, // Front bottom left
  0.25f, 0.25f, 0.25f, // Back bottom left

  0.75f, 0.75f, 0.75f, // Front top right
  0.75f, 0.75f, 0.25f, // Back top right

  0.75f, 0.25f, 0.75f, // Front bottom right
  0.75f, 0.25f, 0.25f, // Back bottom right
};

// clang-format on

static inline void
reshapeCube(const float width, const float height)
{
  const float aspect = width / height;

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  glFrontFace(GL_CW);

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glViewport(0, 0, (int)width, (int)height);

  float projection[16];
  perspective(projection, 1.8f, aspect, 1.0f, 100.0f);
  glLoadMatrixf(projection);
}

static inline void
displayCube(PuglView* const view,
            const float     distance,
            const float     xAngle,
            const float     yAngle,
            const bool      entered)
{
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslatef(0.0f, 0.0f, distance * -1.0f);
  glRotatef(xAngle, 0.0f, 1.0f, 0.0f);
  glRotatef(yAngle, 1.0f, 0.0f, 0.0f);

  if (entered) {
    glClearColor(0.13f, 0.14f, 0.14f, 1.0f);
  } else {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  }

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  if (puglHasFocus(view)) {
    // Draw cube surfaces
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, cubeStripVertices);
    glColorPointer(3, GL_FLOAT, 0, cubeStripColorVertices);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 14);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glColor3f(0.0f, 0.0f, 0.0f);
  } else {
    // Draw cube wireframe
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, cubeFrontLineLoop);
    glColorPointer(3, GL_FLOAT, 0, cubeFrontLineLoopColors);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
    glVertexPointer(3, GL_FLOAT, 0, cubeBackLineLoop);
    glColorPointer(3, GL_FLOAT, 0, cubeBackLineLoopColors);
    glDrawArrays(GL_LINE_LOOP, 0, 4);
    glVertexPointer(3, GL_FLOAT, 0, cubeSideLines);
    glColorPointer(3, GL_FLOAT, 0, cubeSideLineColors);
    glDrawArrays(GL_LINES, 0, 8);
    glDisableClientState(GL_VERTEX_ARRAY);
  }
}

#endif // EXAMPLES_CUBE_VIEW_H
