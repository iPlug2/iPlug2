// Copyright 2019-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef EXAMPLES_RECTS_H
#define EXAMPLES_RECTS_H

#include <math.h>
#include <stddef.h>

typedef float vec2[2];

typedef struct {
  float pos[2];
  float size[2];
  float fillColor[4];
} Rect;

static const vec2 rectVertices[] = {
  {0.0f, 0.0f}, // TL
  {1.0f, 0.0f}, // TR
  {0.0f, 1.0f}, // BL
  {1.0f, 1.0f}  // BR
};

static const unsigned rectIndices[4] = {0, 1, 2, 3};

/// Make a new rectangle with the given index (each is slightly different)
static inline Rect
makeRect(const size_t index, const float frameWidth)
{
  static const float alpha   = 0.3f;
  const float        minSize = frameWidth / 64.0f;
  const float        maxSize = frameWidth / 6.0f;
  const float        s       = (sinf((float)index) / 2.0f + 0.5f);
  const float        c       = (cosf((float)index) / 2.0f + 0.5f);

  const Rect rect = {
    {0.0f, 0.0f}, // Position is set later during expose
    {minSize + s * maxSize, minSize + c * maxSize},
    {0.0f, s / 2.0f + 0.25f, c / 2.0f + 0.25f, alpha},
  };

  return rect;
}

/// Move `rect` with the given index around in an arbitrary way that looks cool
static inline void
moveRect(Rect* const  rect,
         const size_t index,
         const size_t numRects,
         const float  frameWidth,
         const float  frameHeight,
         const double time)
{
  const float normal    = (float)index / (float)numRects;
  const float offset[2] = {normal * 128.0f, normal * 128.0f};

  rect->pos[0] = (frameWidth - rect->size[0] + offset[0]) *
                 (sinf((float)time * rect->size[0] / 64.0f + normal) + 1.0f) /
                 2.0f;
  rect->pos[1] = (frameHeight - rect->size[1] + offset[1]) *
                 (cosf((float)time * rect->size[1] / 64.0f + normal) + 1.0f) /
                 2.0f;
}

#endif // EXAMPLES_RECTS_H
