#pragma once

/**
 * @file
 * @brief Easing functions, based on Warren Moore's AHEasing library https://github.com/warrenm/AHEasing
 * See here for visualizations: http://easings.net/
 */


#include <math.h>

// line y = x
template<class TYPE>
TYPE IEaseLinear(TYPE p, TYPE c)
{
  return p;
}

// line y = x
template<class TYPE>
TYPE IEasePowCurve(TYPE p, TYPE c)
{
  return std::pow(p, 1.0 / c);
}

// parabola y = x^2
template<class TYPE>
TYPE IEaseQuadraticIn(TYPE p, TYPE c)
{
  return p * p;
}

// parabola y = -x^2 + 2x
template<class TYPE>
TYPE IEaseQuadraticOut(TYPE p, TYPE c)
{
  return -(p * (p - 2));
}

// piecewise quadratic
// y = (1/2)((2x)^2)             ; [0, 0.5)
// y = -(1/2)((2x-1)*(2x-3) - 1) ; [0.5, 1]
template<class TYPE>
TYPE IEaseQuadraticInOut(TYPE p, TYPE c)
{
  if(p < 0.5)
  {
    return 2 * p * p;
  }
  else
  {
    return (-2 * p * p) + (4 * p) - 1;
  }
}

// cubic y = x^3
template<class TYPE>
TYPE IEaseCubicIn(TYPE p, TYPE c)
{
  return p * p * p;
}

// cubic y = (x - 1)^3 + 1
template<class TYPE>
TYPE IEaseCubicOut(TYPE p, TYPE c)
{
  TYPE f = (p - 1);
  return f * f * f + 1;
}

// piecewise cubic
// y = (1/2)((2x)^3)       ; [0, 0.5)
// y = (1/2)((2x-2)^3 + 2) ; [0.5, 1]
template<class TYPE>
TYPE IEaseCubicInOut(TYPE p, TYPE c)
{
  if(p < 0.5)
  {
    return 4 * p * p * p;
  }
  else
  {
    TYPE f = ((2 * p) - 2);
    return 0.5 * f * f * f + 1;
  }
}

// quartic x^4
template<class TYPE>
TYPE IEaseQuarticIn(TYPE p, TYPE c)
{
  return p * p * p * p;
}

// quartic y = 1 - (x - 1)^4
template<class TYPE>
TYPE IEaseQuarticOut(TYPE p, TYPE c)
{
  TYPE f = (p - 1);
  return f * f * f * (1 - p) + 1;
}

// piecewise quartic
// y = (1/2)((2x)^4)        ; [0, 0.5)
// y = -(1/2)((2x-2)^4 - 2) ; [0.5, 1]
template<class TYPE>
TYPE IEaseQuarticInOut(TYPE p, TYPE c)
{
  if(p < 0.5)
  {
    return 8 * p * p * p * p;
  }
  else
  {
    TYPE f = (p - 1);
    return -8 * f * f * f * f + 1;
  }
}

// quintic y = x^5
template<class TYPE>
TYPE IEaseQuinticIn(TYPE p, TYPE c)
{
  return p * p * p * p * p;
}

// quintic y = (x - 1)^5 + 1
template<class TYPE>
TYPE IEaseQuinticOut(TYPE p, TYPE c)
{
  TYPE f = (p - 1);
  return f * f * f * f * f + 1;
}

// piecewise quintic
// y = (1/2)((2x)^5)       ; [0, 0.5)
// y = (1/2)((2x-2)^5 + 2) ; [0.5, 1]
template<class TYPE>
TYPE IEaseQuinticInOut(TYPE p, TYPE c)
{
  if(p < 0.5)
  {
    return 16 * p * p * p * p * p;
  }
  else
  {
    TYPE f = ((2 * p) - 2);
    return  0.5 * f * f * f * f * f + 1;
  }
}

// Modeled after quarter-cycle of sine wave
template<class TYPE>
TYPE IEaseSineIn(TYPE p, TYPE c)
{
  return std::sin((p - 1) * M_PI_2) + 1;
}

// Modeled after quarter-cycle of sine wave (different phase)
template<class TYPE>
TYPE IEaseSineOut(TYPE p, TYPE c)
{
  return std::sin(p * M_PI_2);
}

// Modeled after half sine wave
template<class TYPE>
TYPE IEaseSineInOut(TYPE p, TYPE c)
{
  return 0.5 * (1 - std::cos(p * M_PI));
}

// Modeled after shifted quadrant IV of unit circle
template<class TYPE>
TYPE IEaseCircularIn(TYPE p, TYPE c)
{
  return 1 - std::sqrt(1 - (p * p));
}

// Modeled after shifted quadrant II of unit circle
template<class TYPE>
TYPE IEaseCircularOut(TYPE p, TYPE c)
{
  return std::sqrt((2 - p) * p);
}

// piecewise circular function
// y = (1/2)(1 - std::sqrt(1 - 4x^2))           ; [0, 0.5)
// y = (1/2)(std::sqrt(-(2x - 3)*(2x - 1)) + 1) ; [0.5, 1]
template<class TYPE>
TYPE IEaseCircularInOut(TYPE p, TYPE c)
{
  if(p < 0.5)
  {
    return 0.5 * (1 - std::sqrt(1 - 4 * (p * p)));
  }
  else
  {
    return 0.5 * (std::sqrt(-((2 * p) - 3) * ((2 * p) - 1)) + 1);
  }
}

// exponential function y = 2^(10(x - 1))
template<class TYPE>
TYPE IEaseExponentialIn(TYPE p, TYPE c)
{
  return (p == 0.0) ? p : std::pow(2, 10 * (p - 1));
}

// exponential function y = -2^(-10x) + 1
template<class TYPE>
TYPE IEaseExponentialOut(TYPE p, TYPE c)
{
  return (p == 1.0) ? p : 1 - std::pow(2, -10 * p);
}

// piecewise exponential
// y = (1/2)2^(10(2x - 1))         ; [0,0.5)
// y = -(1/2)*2^(-10(2x - 1))) + 1 ; [0.5,1]
template<class TYPE>
TYPE IEaseExponentialInOut(TYPE p, TYPE c)
{
  if(p == 0.0 || p == 1.0) return p;

  if(p < 0.5)
  {
    return 0.5 * std::pow(2, (20 * p) - 10);
  }
  else
  {
    return -0.5 * std::pow(2, (-20 * p) + 10) + 1;
  }
}

// damped sine wave y = std::sin(13pi/2*x)*std::pow(2, 10 * (x - 1))
template<class TYPE>
TYPE IEaseElasticIn(TYPE p, TYPE c)
{
  return std::sin(13 * M_PI_2 * p) * std::pow(2, 10 * (p - 1));
}

// damped sine wave y = std::sin(-13pi/2*(x + 1))*std::pow(2, -10x) + 1
template<class TYPE>
TYPE IEaseElasticOut(TYPE p, TYPE c)
{
  return std::sin(-13 * M_PI_2 * (p + 1)) * std::pow(2, -10 * p) + 1;
}

// piecewise exponentially-damped sine wave:
// y = (1/2)*std::sin(13pi/2*(2*x))*std::pow(2, 10 * ((2*x) - 1))      ; [0,0.5)
// y = (1/2)*(std::sin(-13pi/2*((2x-1)+1))*std::pow(2,-10(2*x-1)) + 2) ; [0.5, 1]
template<class TYPE>
TYPE IEaseElasticInOut(TYPE p, TYPE c)
{
  if(p < 0.5)
  {
    return 0.5 * std::sin(13 * M_PI_2 * (2 * p)) * std::pow(2, 10 * ((2 * p) - 1));
  }
  else
  {
    return 0.5 * (std::sin(-13 * M_PI_2 * ((2 * p - 1) + 1)) * std::pow(2, -10 * (2 * p - 1)) + 2);
  }
}

// overshooting cubic y = x^3-x*std::sin(x*pi)
template<class TYPE>
TYPE IEaseBackIn(TYPE p, TYPE c)
{
  return p * p * p - p * std::sin(p * M_PI);
}

// Modeled after overshooting cubic y = 1-((1-x)^3-(1-x)*std::sin((1-x)*pi))
template<class TYPE>
TYPE IEaseBackOut(TYPE p, TYPE c)
{
  TYPE f = (1 - p);
  return 1 - (f * f * f - f * std::sin(f * M_PI));
}

// piecewise overshooting cubic function:
// y = (1/2)*((2x)^3-(2x)*std::sin(2*x*pi))           ; [0, 0.5)
// y = (1/2)*(1-((1-x)^3-(1-x)*std::sin((1-x)*pi))+1) ; [0.5, 1]
template<class TYPE>
TYPE IEaseBackInOut(TYPE p, TYPE c)
{
  if(p < 0.5)
  {
    TYPE f = 2 * p;
    return 0.5 * (f * f * f - f * std::sin(f * M_PI));
  }
  else
  {
    TYPE f = (1 - (2*p - 1));
    return 0.5 * (1 - (f * f * f - f * std::sin(f * M_PI))) + 0.5;
  }
}

template<class TYPE>
TYPE IEaseBounceOut(TYPE p, TYPE c)
{
  if(p < 4/11.0)
  {
    return (121 * p * p)/16.0;
  }
  else if(p < 8/11.0)
  {
    return (363/40.0 * p * p) - (99/10.0 * p) + 17/5.0;
  }
  else if(p < 9/10.0)
  {
    return (4356/361.0 * p * p) - (35442/1805.0 * p) + 16061/1805.0;
  }
  else
  {
    return (54/5.0 * p * p) - (513/25.0 * p) + 268/25.0;
  }
}

template<class TYPE>
TYPE IEaseBounceIn(TYPE p, TYPE c)
{
  return 1 - IEaseBounceOut(1 - p, c);
}

template<class TYPE>
TYPE IEaseBounceInOut(TYPE p, TYPE c)
{
  if(p < 0.5)
  {
    return 0.5 * IEaseBounceIn(p*2, c);
  }
  else
  {
    return 0.5 * IEaseBounceOut(p * 2 - 1, c) + 0.5;
  }
}
