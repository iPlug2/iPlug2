/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once
#include "IPlugConstants.h"
#include "IGraphicsConstants.h"

#include <cmath>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

// these are macros to shorten the instantiation of IControls
// for a paramater ID MyParam, define constants named MyParam_X, MyParam_Y, MyParam_W, MyParam_H to specify the Control's IRect
// then when instantiating a Control you can just call MakeIRect(MyParam) to specify the IRect
#define MakeIRect(a) IRECT(a##_X, a##_Y, a##_X + a##_W, a##_Y + a##_H)
#define MakeIRectHOffset(a, xoffs) IRECT(a##_X + xoffs, a##_Y, a##_X + a##_W + xoffs, a##_Y + a##_H)
#define MakeIRectVOffset(a, yoffs) IRECT(a##_X, a##_Y + yoffs, a##_X + a##_W, a##_Y + a##_H + yoffs)
#define MakeIRectHVOffset(a, xoffs, yoffs) IRECT(a##_X + xoffs, a##_Y + yoffs, a##_X + a##_W + xoffs, a##_Y + a##_H + yoffs)

static double GetTimestamp()
{
  static auto start = std::chrono::steady_clock::now();
  return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

template <typename T>
inline T DegToRad(T degrees)
{
  return static_cast<T>(iplug::PI) * (degrees / static_cast<T>(180.0));
}

/** Calculate evenly distributed points on a radial line. NOTE: will crash if the nPoints and data array do not match size.
 * @param angleDegrees The angle to draw at in degrees clockwise where 0 is up
 * @param cx centre point x coordinate
 * @param cy centre point y coordinate
 * @param rMin minima of the radial line (distance from cx,cy)
 * @param rMax maxima of the radial line (distance from cx,cy)
 * @param nPoints Number of points between rMin and rMax to obtain
 * @param data Multidimensional array for nPoints pairs of float coordinates for the points */
static inline void RadialPoints(float angleDegrees, float cx, float cy, float rMin, float rMax, int nPoints, float data[][2])
{
  const float angleRadians = DegToRad(angleDegrees - 90.f);
  const float sinV = std::sin(angleRadians);
  const float cosV = std::cos(angleRadians);
  
  for(auto i = 0; i < nPoints; i++)
  {
    const float r = rMin+(rMax-rMin) * (float) i / float (nPoints-1);
    data[i][0] = (cx + r * cosV);
    data[i][1] = (cy + r * sinV);
  }
}

// Return the intersection of line(p0, p1) with line(p2, p3) as a fraction of the distance along (p2, p3).
static float GetLineCrossing(Vec2 p0, Vec2 p1, Vec2 p2, Vec2 p3)
{
  auto b = p2 - p0;
  auto d = p1 - p0;
  auto e = p3 - p2;
  float m = d.x * e.y - d.y * e.x;
  
  float epsilon = 1e-8f;
  if (std::abs(m) < epsilon)
    return NAN;
  return -(d.x * b.y - d.y * b.x) / m;
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

