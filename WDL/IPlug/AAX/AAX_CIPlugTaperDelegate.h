#ifndef	AAX_CIPlugTAPERDELEGATE_H
#define AAX_CIPlugTAPERDELEGATE_H

#include "IParam.h"

#include "AAX_ITaperDelegate.h"
#include "AAX.h"

#include <cmath>

template <typename T>
class AAX_CIPlugTaperDelegate : public AAX_ITaperDelegate<T>
{
public: 
	AAX_CIPlugTaperDelegate(T minValue=0, T maxValue=1, double shape = 1.);
	
	//Virtual AAX_ITaperDelegate Overrides
	AAX_CIPlugTaperDelegate<T>*	Clone() const;
	T		GetMinimumValue()	const						{ return mMinValue; }
	T		GetMaximumValue()	const						{ return mMaxValue; }
	T		ConstrainRealValue(T value)	const;
	T		NormalizedToReal(double normalizedValue) const;
	double	RealToNormalized(T realValue) const;

private:
	T	mMinValue;
	T	mMaxValue;
	double mShape;
};

template <typename T>
AAX_CIPlugTaperDelegate<T>::AAX_CIPlugTaperDelegate(T minValue, T maxValue, double shape)  :  AAX_ITaperDelegate<T>(),
	mMinValue(minValue),
	mMaxValue(maxValue),
	mShape(shape)
{

}

template <typename T>
AAX_CIPlugTaperDelegate<T>*		AAX_CIPlugTaperDelegate<T>::Clone() const
{
	return new AAX_CIPlugTaperDelegate(*this);
}

template <typename T>
T		AAX_CIPlugTaperDelegate<T>::ConstrainRealValue(T value)	const
{
	if (value > mMaxValue)
		return mMaxValue;
	if (value < mMinValue)
		return mMinValue;
	return value;		
}

template <typename T>
T		AAX_CIPlugTaperDelegate<T>::NormalizedToReal(double normalizedValue) const
{  
  double doubleRealValue = FromNormalizedParam(normalizedValue, mMinValue, mMaxValue, mShape);
  
	T realValue = (T)doubleRealValue;
	
	return ConstrainRealValue(realValue);
}

template <typename T>
double	AAX_CIPlugTaperDelegate<T>::RealToNormalized(T realValue) const
{
	realValue = ConstrainRealValue(realValue);
  
  return ToNormalizedParam(realValue, mMinValue, mMaxValue, mShape);
}

#endif //AAX_CIPlugTAPERDELEGATE_H
