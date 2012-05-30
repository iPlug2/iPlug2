#ifndef	AAX_CIPlugTAPERDELEGATE_H
#define AAX_CIPlugTAPERDELEGATE_H

#include "IParam.h"

#include "AAX_ITaperDelegate.h"
#include "AAX.h"	//for types

#include <cmath>	//for floor()

template <typename T, int32_t RealPrecision=1000>
class AAX_CIPlugTaperDelegate : public AAX_ITaperDelegate<T>
{
public: 
	AAX_CIPlugTaperDelegate(T minValue=0, T maxValue=1, double shape = 1.);
	
	//Virtual AAX_ITaperDelegate Overrides
	AAX_CIPlugTaperDelegate<T, RealPrecision>*	Clone() const;
	T		GetMinimumValue()	const						{ return mMinValue; }
	T		GetMaximumValue()	const						{ return mMaxValue; }
	T		ConstrainRealValue(T value)	const;
	T		NormalizedToReal(double normalizedValue) const;
	double	RealToNormalized(T realValue) const;
	
protected:
	T	Round(double iValue) const;

private:
	T	mMinValue;
	T	mMaxValue;
	double mShape;
};

template <typename T, int32_t RealPrecision>
T	AAX_CIPlugTaperDelegate<T, RealPrecision>::Round(double iValue) const
{
	if (RealPrecision > 0)
		return static_cast<T>(floor(iValue * RealPrecision + 0.5) / RealPrecision);
	else
		return static_cast<T>(iValue);
}

template <typename T, int32_t RealPrecision>
AAX_CIPlugTaperDelegate<T, RealPrecision>::AAX_CIPlugTaperDelegate(T minValue, T maxValue, double shape)  :  AAX_ITaperDelegate<T>(),
	mMinValue(minValue),
	mMaxValue(maxValue),
	mShape(shape)
{

}

template <typename T, int32_t RealPrecision>
AAX_CIPlugTaperDelegate<T, RealPrecision>*		AAX_CIPlugTaperDelegate<T, RealPrecision>::Clone() const
{
	return new AAX_CIPlugTaperDelegate(*this);
}

template <typename T, int32_t RealPrecision>
T		AAX_CIPlugTaperDelegate<T, RealPrecision>::ConstrainRealValue(T value)	const
{
	if (RealPrecision)
		value = Round(value);		//reduce the precision to get proper rounding behavior with integers.

	if (value > mMaxValue)
		return mMaxValue;
	if (value < mMinValue)
		return mMinValue;
	return value;		
}

template <typename T, int32_t RealPrecision>
T		AAX_CIPlugTaperDelegate<T, RealPrecision>::NormalizedToReal(double normalizedValue) const
{
  //double doubleRealValue = normalizedValue * (double(mMaxValue) - double(mMinValue)) + double(mMinValue);
  
  double doubleRealValue = FromNormalizedParam(normalizedValue, mMinValue, mMaxValue, mShape);
  
	T realValue = (T)doubleRealValue;
	
	if (RealPrecision)
		realValue = Round(doubleRealValue);		//reduce the precision to get proper rounding behavior with integers.
  
	return ConstrainRealValue(realValue);
}

template <typename T, int32_t RealPrecision>
double	AAX_CIPlugTaperDelegate<T, RealPrecision>::RealToNormalized(T realValue) const
{
	realValue = ConstrainRealValue(realValue);
//	double normalizedValue = (double(realValue) - double(mMinValue)) / (double(mMaxValue) - double(mMinValue));
  
  return ToNormalizedParam(realValue, mMinValue, mMaxValue, mShape);
}

#endif AAX_CIPlugTAPERDELEGATE_H
