/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

// Helpers shared by the IGraphics unit tests: comparisons and Catch2 string conversions for IRECT/IColor.

#pragma once

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

#include <catch2/catch_tostring.hpp>

#include "IGraphicsStructs.h"

namespace iplug_test
{
/** IColor::operator== is not const-qualified, so it can't be used on the COLOR_ constants (or inside Catch2 expressions) */
inline bool SameColor(const iplug::igraphics::IColor& a, const iplug::igraphics::IColor& b)
{
  return a.A == b.A && a.R == b.R && a.G == b.G && a.B == b.B;
}

/** @return true if every channel of a and b differs by no more than tolerance */
inline bool NearColor(const iplug::igraphics::IColor& a, const iplug::igraphics::IColor& b, int tolerance)
{
  auto within = [tolerance](int x, int y) { return (x > y ? x - y : y - x) <= tolerance; };
  return within(a.A, b.A) && within(a.R, b.R) && within(a.G, b.G) && within(a.B, b.B);
}

/** @return true if every edge of a and b differs by no more than tolerance */
inline bool NearRect(const iplug::igraphics::IRECT& a, const iplug::igraphics::IRECT& b, float tolerance = 1e-3f)
{
  return std::fabs(a.L - b.L) <= tolerance && std::fabs(a.T - b.T) <= tolerance
      && std::fabs(a.R - b.R) <= tolerance && std::fabs(a.B - b.B) <= tolerance;
}

/** The area shared by two rectangles, treating them as plain geometric regions (zero if they don't overlap) */
inline float OverlapArea(const iplug::igraphics::IRECT& a, const iplug::igraphics::IRECT& b)
{
  const float w = std::min(a.R, b.R) - std::max(a.L, b.L);
  const float h = std::min(a.B, b.B) - std::max(a.T, b.T);
  return (w > 0.f && h > 0.f) ? w * h : 0.f;
}
} // namespace iplug_test

namespace Catch
{
template <>
struct StringMaker<iplug::igraphics::IRECT>
{
  static std::string convert(const iplug::igraphics::IRECT& r)
  {
    std::ostringstream str;
    str << "IRECT(" << r.L << ", " << r.T << ", " << r.R << ", " << r.B << ")";
    return str.str();
  }
};

template <>
struct StringMaker<iplug::igraphics::IColor>
{
  static std::string convert(const iplug::igraphics::IColor& c)
  {
    std::ostringstream str;
    str << "IColor(A " << c.A << ", R " << c.R << ", G " << c.G << ", B " << c.B << ")";
    return str.str();
  }
};
} // namespace Catch
