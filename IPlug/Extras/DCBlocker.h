/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers. 
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
*/
#pragma once

#include "denormal.h"
#include "IPlugConstants.h"

BEGIN_IPLUG_NAMESPACE

template<typename T, int NC = 1>
class DCBlocker
{
private:
  // one pole IIR
  // y[n] = x[n] + a[n] * y[n-1]
  class rpole {
    T y_m1;
  public:  
    inline T process(T x, T a) { 
      const T y = y_m1 = x + a * y_m1;
      return y;
    }
  };

  // one zero FIR
  // y[n] = x[n] - b[n] * x[n-1]
  class rzero {
    T x_m1;
  public:
    inline T process(T x, T b) {
      const T y = x - b * x_m1;
      x_m1 = x;
      return y;
    }
  };

  rpole mPole[NC];
  rzero mZero[NC];

public:

  void ProcessBlock(T** inputs, T** outputs, int nFrames)
  {
    for (auto s=0; s<nFrames; s++)
    {
      for (auto c=0; c<NC; c++)
      {
        const auto x = inputs[c][s];
        outputs[c][s] = mPole[c].process(mZero[c].process(x, T(1.0)), T(0.995));
      }
    }
  }

} WDL_FIXALIGN;

END_IPLUG_NAMESPACE
