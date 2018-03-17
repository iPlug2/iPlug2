#pragma once

/**
 * @file
 * @brief Easing functions, based on Warren Moore's AHEasing library https://github.com/warrenm/AHEasing
 * See here for visualizations: http://easings.net/
 */


#include <math.h>

// line y = x
template<class T>
T IEaseLinear(T x)
{
  return x;
}

// line y = x ^ 1/c;
template<class T>
T IEasePowCurve(T x, T c)
{
  return std::pow(x, 1.0 / c);
}

// parabola y = x^2
template<class T>
T IEaseQuadraticIn(T x)
{
  return x * x;
}

// parabola y = -x^2 + 2x
template<class T>
T IEaseQuadraticOut(T x)
{
  return -(x * (x - 2));
}

// piecewise quadratic
// y = (1/2)((2x)^2)             ; [0, 0.5)
// y = -(1/2)((2x-1)*(2x-3) - 1) ; [0.5, 1]
template<class T>
T IEaseQuadraticInOut(T x)
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
T IEaseCubicIn(T x)
{
  return x * x * x;
}

// cubic y = (x - 1)^3 + 1
template<class T>
T IEaseCubicOut(T x)
{
  T f = (x - 1);
  return f * f * f + 1;
}

// piecewise cubic
// y = (1/2)((2x)^3)       ; [0, 0.5)
// y = (1/2)((2x-2)^3 + 2) ; [0.5, 1]
template<class T>
T IEaseCubicInOut(T x)
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
T IEaseQuarticIn(T x)
{
  return x * x * x * x;
}

// quartic y = 1 - (x - 1)^4
template<class T>
T IEaseQuarticOut(T x)
{
  T f = (x - 1);
  return f * f * f * (1 - x) + 1;
}

// piecewise quartic
// y = (1/2)((2x)^4)        ; [0, 0.5)
// y = -(1/2)((2x-2)^4 - 2) ; [0.5, 1]
template<class T>
T IEaseQuarticInOut(T x)
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
T IEaseQuinticIn(T x)
{
  return x * x * x * x * x;
}

// quintic y = (x - 1)^5 + 1
template<class T>
T IEaseQuinticOut(T x)
{
  T f = (x - 1);
  return f * f * f * f * f + 1;
}

// piecewise quintic
// y = (1/2)((2x)^5)       ; [0, 0.5)
// y = (1/2)((2x-2)^5 + 2) ; [0.5, 1]
template<class T>
T IEaseQuinticInOut(T x)
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
T IEaseSineIn(T x)
{
  return std::sin((x - 1) * M_PI_2) + 1;
}

// Modeled after quarter-cycle of sine wave (different phase)
template<class T>
T IEaseSineOut(T x)
{
  return std::sin(x * M_PI_2);
}

// Modeled after half sine wave
template<class T>
T IEaseSineInOut(T x)
{
  return 0.5 * (1 - std::cos(x * M_PI));
}

// Modeled after shifted quadrant IV of unit circle
template<class T>
T IEaseCircularIn(T x)
{
  return 1 - std::sqrt(1 - (x * x));
}

// Modeled after shifted quadrant II of unit circle
template<class T>
T IEaseCircularOut(T x)
{
  return std::sqrt((2 - x) * x);
}

// piecewise circular function
// y = (1/2)(1 - std::sqrt(1 - 4x^2))           ; [0, 0.5)
// y = (1/2)(std::sqrt(-(2x - 3)*(2x - 1)) + 1) ; [0.5, 1]
template<class T>
T IEaseCircularInOut(T x)
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
T IEaseExponentialIn(T x)
{
  return (x == 0.0) ? x : std::pow(2, 10 * (x - 1));
}

// exponential function y = -2^(-10x) + 1
template<class T>
T IEaseExponentialOut(T x)
{
  return (x == 1.0) ? x : 1 - std::pow(2, -10 * x);
}

// piecewise exponential
// y = (1/2)2^(10(2x - 1))         ; [0,0.5)
// y = -(1/2)*2^(-10(2x - 1))) + 1 ; [0.5,1]
template<class T>
T IEaseExponentialInOut(T x)
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
T IEaseElasticIn(T x)
{
  return std::sin(13 * M_PI_2 * x) * std::pow(2, 10 * (x - 1));
}

// damped sine wave y = std::sin(-13pi/2*(x + 1))*std::pow(2, -10x) + 1
template<class T>
T IEaseElasticOut(T x)
{
  return std::sin(-13 * M_PI_2 * (x + 1)) * std::pow(2, -10 * x) + 1;
}

// piecewise exponentially-damped sine wave:
// y = (1/2)*std::sin(13pi/2*(2*x))*std::pow(2, 10 * ((2*x) - 1))      ; [0,0.5)
// y = (1/2)*(std::sin(-13pi/2*((2x-1)+1))*std::pow(2,-10(2*x-1)) + 2) ; [0.5, 1]
template<class T>
T IEaseElasticInOut(T x)
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
T IEaseBackIn(T x)
{
  return x * x * x - x * std::sin(x * M_PI);
}

// Modeled after overshooting cubic y = 1-((1-x)^3-(1-x)*std::sin((1-x)*pi))
template<class T>
T IEaseBackOut(T x)
{
  T f = (1 - x);
  return 1 - (f * f * f - f * std::sin(f * M_PI));
}

// piecewise overshooting cubic function:
// y = (1/2)*((2x)^3-(2x)*std::sin(2*x*pi))           ; [0, 0.5)
// y = (1/2)*(1-((1-x)^3-(1-x)*std::sin((1-x)*pi))+1) ; [0.5, 1]
template<class T>
T IEaseBackInOut(T x)
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
T IEaseBounceOut(T x)
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
T IEaseBounceIn(T x)
{
  return 1 - IEaseBounceOut(1 - x);
}

template<class T>
T IEaseBounceInOut(T x)
{
  if(x < 0.5)
  {
    return 0.5 * IEaseBounceIn(x*2);
  }
  else
  {
    return 0.5 * IEaseBounceOut(x * 2 - 1) + 0.5;
  }
}
