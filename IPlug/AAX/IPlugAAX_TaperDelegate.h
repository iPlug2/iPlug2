/*
 ==============================================================================
 
 This file is part of the iPlug 2 library
 
 Oli Larkin et al. 2018 - https://www.olilarkin.co.uk
 
 iPlug 2 is an open source library subject to commercial or open-source
 licensing.
 
 The code included in this file is provided under the terms of the WDL license
 - https://www.cockos.com/wdl/
 
 ==============================================================================
 */

#pragma once

#include <cmath>

#include "AAX_ITaperDelegate.h"
#include "AAX.h"

#include "IPlugParameter.h"

template <typename T>
class AAX_CIPlugTaperDelegate : public AAX_ITaperDelegate<T>
{
public: 
  AAX_CIPlugTaperDelegate(IParam& iParam);
  
  //Virtual AAX_ITaperDelegate Overrides
  AAX_CIPlugTaperDelegate<T>* Clone() const;
  T GetMinimumValue() const { return mParam.GetMin(); }
  T GetMaximumValue() const { return mParam.GetMax(); }
  T ConstrainRealValue(T value) const;
  T NormalizedToReal(double normalizedValue) const;
  double  RealToNormalized(T realValue) const;

private:
  IParam& mParam;
};

template <typename T>
AAX_CIPlugTaperDelegate<T>::AAX_CIPlugTaperDelegate(IParam& iParam):AAX_ITaperDelegate<T>(),
  mParam(iParam)
{
}

template <typename T>
AAX_CIPlugTaperDelegate<T>* AAX_CIPlugTaperDelegate<T>::Clone() const
{
  return new AAX_CIPlugTaperDelegate(*this);
}

template <typename T>
T AAX_CIPlugTaperDelegate<T>::ConstrainRealValue(T value) const
{
  return mParam.Constrain(value);
}

template <typename T>
T AAX_CIPlugTaperDelegate<T>::NormalizedToReal(double normalizedValue) const
{
  return mParam.FromNormalized(normalizedValue);
}

template <typename T>
double AAX_CIPlugTaperDelegate<T>::RealToNormalized(T realValue) const
{
  return mParam.ToNormalized(realValue);
}
