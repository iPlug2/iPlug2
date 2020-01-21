/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/

#pragma once

/**
 * @file
 * @brief Easing functions, based on Warren Moore's AHEasing library https://github.com/warrenm/AHEasing
 * See here for visualizations: http://easings.net/
 */

#define _USE_MATH_DEFINES
#include <math.h>
#include "IPlugPlatform.h"

BEGIN_IPLUG_NAMESPACE

// line y = x ^ 1/c;
template<class T>
T EasePowCurve(T x, T c)
{
  return std::pow(x, 1.0 / c);
}

// line y = x
template<class T>
T EaseLinear(T x)
{
  return x;
}

// parabola y = x^2
template<class T>
T EaseQuadraticIn(T x)
{
  return x * x;
}

// parabola y = -x^2 + 2x
template<class T>
T EaseQuadraticOut(T x)
{
  return -(x * (x - 2));
}

// piecewise quadratic
// y = (1/2)((2x)^2)             ; [0, 0.5)
// y = -(1/2)((2x-1)*(2x-3) - 1) ; [0.5, 1]
template<class T>
T EaseQuadraticInOut(T x)
{
  if(x < 0.5)
  {
    return 2 * x * x;
  }
  else
  {
    return (-2 * x * x) + (4 * x) - 1;
  }
}

// cubic y = x^3
template<class T>
T EaseCubicIn(T x)
{
  return x * x * x;
}

// cubic y = (x - 1)^3 + 1
template<class T>
T EaseCubicOut(T x)
{
  T f = (x - 1);
  return f * f * f + 1;
}

// piecewise cubic
// y = (1/2)((2x)^3)       ; [0, 0.5)
// y = (1/2)((2x-2)^3 + 2) ; [0.5, 1]
template<class T>
T EaseCubicInOut(T x)
{
  if(x < 0.5)
  {
    return 4 * x * x * x;
  }
  else
  {
    T f = ((2 * x) - 2);
    return 0.5 * f * f * f + 1;
  }
}

// quartic x^4
template<class T>
T EaseQuarticIn(T x)
{
  return x * x * x * x;
}

// quartic y = 1 - (x - 1)^4
template<class T>
T EaseQuarticOut(T x)
{
  T f = (x - 1);
  return f * f * f * (1 - x) + 1;
}

// piecewise quartic
// y = (1/2)((2x)^4)        ; [0, 0.5)
// y = -(1/2)((2x-2)^4 - 2) ; [0.5, 1]
template<class T>
T EaseQuarticInOut(T x)
{
  if(x < 0.5)
  {
    return 8 * x * x * x * x;
  }
  else
  {
    T f = (x - 1);
    return -8 * f * f * f * f + 1;
  }
}

// quintic y = x^5
template<class T>
T EaseQuinticIn(T x)
{
  return x * x * x * x * x;
}

// quintic y = (x - 1)^5 + 1
template<class T>
T EaseQuinticOut(T x)
{
  T f = (x - 1);
  return f * f * f * f * f + 1;
}

// piecewise quintic
// y = (1/2)((2x)^5)       ; [0, 0.5)
// y = (1/2)((2x-2)^5 + 2) ; [0.5, 1]
template<class T>
T EaseQuinticInOut(T x)
{
  if(x < 0.5)
  {
    return 16 * x * x * x * x * x;
  }
  else
  {
    T f = ((2 * x) - 2);
    return  0.5 * f * f * f * f * f + 1;
  }
}

// Modeled after quarter-cycle of sine wave
template<class T>
T EaseSineIn(T x)
{
  return std::sin((x - 1) * M_PI_2) + 1;
}

// Modeled after quarter-cycle of sine wave (different phase)
template<class T>
T EaseSineOut(T x)
{
  return std::sin(x * M_PI_2);
}

// Modeled after half sine wave
template<class T>
T EaseSineInOut(T x)
{
  return 0.5 * (1 - std::cos(x * M_PI));
}

// Modeled after shifted quadrant IV of unit circle
template<class T>
T EaseCircularIn(T x)
{
  return 1 - std::sqrt(1 - (x * x));
}

// Modeled after shifted quadrant II of unit circle
template<class T>
T EaseCircularOut(T x)
{
  return std::sqrt((2 - x) * x);
}

// piecewise circular function
// y = (1/2)(1 - std::sqrt(1 - 4x^2))           ; [0, 0.5)
// y = (1/2)(std::sqrt(-(2x - 3)*(2x - 1)) + 1) ; [0.5, 1]
template<class T>
T EaseCircularInOut(T x)
{
  if(x < 0.5)
  {
    return 0.5 * (1 - std::sqrt(1 - 4 * (x * x)));
  }
  else
  {
    return 0.5 * (std::sqrt(-((2 * x) - 3) * ((2 * x) - 1)) + 1);
  }
}

// exponential function y = 2^(10(x - 1))
template<class T>
T EaseExponentialIn(T x)
{
  return (x == 0.0) ? x : std::pow(2, 10 * (x - 1));
}

// exponential function y = -2^(-10x) + 1
template<class T>
T EaseExponentialOut(T x)
{
  return (x == 1.0) ? x : 1 - std::pow(2, -10 * x);
}

// piecewise exponential
// y = (1/2)2^(10(2x - 1))         ; [0,0.5)
// y = -(1/2)*2^(-10(2x - 1))) + 1 ; [0.5,1]
template<class T>
T EaseExponentialInOut(T x)
{
  if(x == 0.0 || x == 1.0) return x;

  if(x < 0.5)
  {
    return 0.5 * std::pow(2, (20 * x) - 10);
  }
  else
  {
    return -0.5 * std::pow(2, (-20 * x) + 10) + 1;
  }
}

// damped sine wave y = std::sin(13pi/2*x)*std::pow(2, 10 * (x - 1))
template<class T>
T EaseElasticIn(T x)
{
  return std::sin(13 * M_PI_2 * x) * std::pow(2, 10 * (x - 1));
}

// damped sine wave y = std::sin(-13pi/2*(x + 1))*std::pow(2, -10x) + 1
template<class T>
T EaseElasticOut(T x)
{
  return std::sin(-13 * M_PI_2 * (x + 1)) * std::pow(2, -10 * x) + 1;
}

// piecewise exponentially-damped sine wave:
// y = (1/2)*std::sin(13pi/2*(2*x))*std::pow(2, 10 * ((2*x) - 1))      ; [0,0.5)
// y = (1/2)*(std::sin(-13pi/2*((2x-1)+1))*std::pow(2,-10(2*x-1)) + 2) ; [0.5, 1]
template<class T>
T EaseElasticInOut(T x)
{
  if(x < 0.5)
  {
    return 0.5 * std::sin(13 * M_PI_2 * (2 * x)) * std::pow(2, 10 * ((2 * x) - 1));
  }
  else
  {
    return 0.5 * (std::sin(-13 * M_PI_2 * ((2 * x - 1) + 1)) * std::pow(2, -10 * (2 * x - 1)) + 2);
  }
}

// overshooting cubic y = x^3-x*std::sin(x*pi)
template<class T>
T EaseBackIn(T x)
{
  return x * x * x - x * std::sin(x * M_PI);
}

// Modeled after overshooting cubic y = 1-((1-x)^3-(1-x)*std::sin((1-x)*pi))
template<class T>
T EaseBackOut(T x)
{
  T f = (1 - x);
  return 1 - (f * f * f - f * std::sin(f * M_PI));
}

// piecewise overshooting cubic function:
// y = (1/2)*((2x)^3-(2x)*std::sin(2*x*pi))           ; [0, 0.5)
// y = (1/2)*(1-((1-x)^3-(1-x)*std::sin((1-x)*pi))+1) ; [0.5, 1]
template<class T>
T EaseBackInOut(T x)
{
  if(x < 0.5)
  {
    T f = 2 * x;
    return 0.5 * (f * f * f - f * std::sin(f * M_PI));
  }
  else
  {
    T f = (1 - (2*x - 1));
    return 0.5 * (1 - (f * f * f - f * std::sin(f * M_PI))) + 0.5;
  }
}

template<class T>
T EaseBounceOut(T x)
{
  if(x < 4/11.0)
  {
    return (121 * x * x)/16.0;
  }
  else if(x < 8/11.0)
  {
    return (363/40.0 * x * x) - (99/10.0 * x) + 17/5.0;
  }
  else if(x < 9/10.0)
  {
    return (4356/361.0 * x * x) - (35442/1805.0 * x) + 16061/1805.0;
  }
  else
  {
    return (54/5.0 * x * x) - (513/25.0 * x) + 268/25.0;
  }
}

template<class T>
T EaseBounceIn(T x)
{
  return 1 - EaseBounceOut(1 - x);
}

template<class T>
T EaseBounceInOut(T x)
{
  if(x < 0.5)
  {
    return 0.5 * EaseBounceIn(x*2);
  }
  else
  {
    return 0.5 * EaseBounceOut(x * 2 - 1) + 0.5;
  }
}

END_IPLUG_NAMESPACE
