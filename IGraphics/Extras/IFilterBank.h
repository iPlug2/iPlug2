/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @ingroup Controls
 * @copydoc IFilterBank
 */

#include "IControl.h"
#include "IGraphicsConstants.h"

#include <vector>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Use the following idea, originally used for mel scale:
    http://practicalcryptography.com/miscellaneous/machine-learning/guide-mel-frequency-cepstral-coefficients-mfccs/
    ... but generalize it to any scale type
    It can also be used to smoothly decimate or scale up data */

// NOTES: when using for example a log scale, there is aliasing on the lower values (stairs effect). This is because the triangle filters are smaller than 1 bin. And also because several successive triangles match exactly the same bin. So this makes pure horizontal series of values (plateau). To avoid this, we grow the triangles that are too small on the left and on the right. It works to fix aliasing, because we use normalization based on triangles areas in floating format. So we have a good continuity.
#define FIX_ALIASING_LOW_FREQS 1
#define FIX_ALIASING_MIN_TRIANGLE_WIDTH 2.0

template<typename T>
class IFilterBank
{
public:
  IFilterBank(EFrequencyScale targetScaleType)
  : mTargetScaleType(targetScaleType) {}
  virtual ~IFilterBank() {}
  
  /** Convert from Hz to target scale using filters */
  void HzToTargetScale(std::vector<T>* output,
                       const std::vector<T>& input,
                       double sampleRate, T freqLo, T freqHi,
                       int numFilters)
  {
    if ((input.size() != mHzToTargetFilterBank.mDataSize) ||
        (sampleRate != mHzToTargetFilterBank.mSampleRate) ||
        (numFilters != mHzToTargetFilterBank.mNumFilters) ||
        (freqLo != mHzToTargetFilterBank.mFreqLo) ||
        (freqHi != mHzToTargetFilterBank.mFreqHi))
    {
      CreateFilterBankHzToTarget(&mHzToTargetFilterBank,
                                 input.size(),
                                 sampleRate, freqLo, freqHi,
                                 numFilters);
    }
    
    ApplyFilterBank(output, input, mHzToTargetFilterBank);
  }

  /** Convert from target scale to Hz filters */
  void TargetScaleToHz(std::vector<T>* output,
                       const std::vector<T>& input,
                       double sampleRate, T freqLo, T freqHi,
                       int numFilters)
  {
    if ((input.size() != mTargetToHzFilterBank.mDataSize) ||
        (sampleRate != mTargetToHzFilterBank.mSampleRate) ||
        (numFilters != mTargetToHzFilterBank.mNumFilters) ||
        (freqLo != mHzToTargetFilterBank.mFreqLo) ||
        (freqHi != mHzToTargetFilterBank.mFreqHi))
    {
      CreateFilterBankTargetToHz(&mTargetToHzFilterBank,
                                 input.size(),
                                 sampleRate, freqLo, freqHi,
                                 numFilters);
    }
    
    ApplyFilterBank(output, input, mTargetToHzFilterBank);
  }
    
protected:
  T ComputeTriangleAreaBetween(T txmin, T txmid, T txmax, T x0, T x1)
  {
    if ((x0 > txmax) || (x1 < txmin))
      return T(0.0);
    
    std::array<T, 5> x = {txmin, txmid, txmax, x0, x1};
    
    std::sort(x.begin(), x.end());
    
    T points[5][2];
    for (int i = 0; i < 5; i++)
    {
      points[i][0] = x[i];
      points[i][1] = ComputeTriangleY(txmin, txmid, txmax, x[i]);
    }
    
    T area = 0.0;
    for (int i = 0; i < 4; i++)
    {
      // Suppress the cases which are out of [x0, x1] bounds
      if ((points[i][0] >= x1) ||
          (points[i + 1][0] <= x0))
        continue;
      
      T y0 = points[i][1];
      T y1 = points[i + 1][1];
      if (y0 > y1)
      {
        T tmp = y0;
        y0 = y1;
        y1 = tmp;
      }
      
      T a = (points[i + 1][0] - points[i][0])*(y0 + (y1 - y0)*0.5);
      
      area += a;
    }
    
    return area;
  }
  
  T ComputeTriangleY(T txmin, T txmid, T txmax, T x)
  {
    if (x <= txmin)
      return (T)0.0;
    if (x >= txmax)
      return (T)0.0;
    
    if (x <= txmid)
    {
      T t = (x - txmin)/(txmid - txmin);
      
      return t;
    }
    else // x >= txmid
    {
      T t = 1.0 - (x - txmid)/(txmax - txmid);
      
      return t;
    }
  }
  
  T ScaleValue(T val, T minFreq, T maxFreq)
  {
    switch (mTargetScaleType)
    {
      case EFrequencyScale::Linear:
      {
        T minTargetValue = minFreq;
        T maxTargetValue = maxFreq;
        val = (val - minTargetValue)/(maxTargetValue - minTargetValue);
      }
      break;
      case EFrequencyScale::Log:
      {
        T minTargetValue = std::log(minFreq);
        T maxTargetValue = std::log(maxFreq); 
        val = std::log(val);

        val = (val - minTargetValue)/(maxTargetValue - minTargetValue);
      }
      break;    
      case EFrequencyScale::Mel:
      {
        T minTargetValue = HzToMel(minFreq);
        T maxTargetValue = HzToMel(maxFreq);
        val = HzToMel(val);

        val = (val - minTargetValue)/(maxTargetValue - minTargetValue);
      }
      break;
      default:
        break;
    }
    
    return val;
  }
  
  T ScaleValueInv(T val, T minFreq, T maxFreq)
  {
    switch (mTargetScaleType)
    {
      case EFrequencyScale::Linear:
      {
        T minTargetValue = minFreq;
        T maxTargetValue = maxFreq;
        val = val*(maxTargetValue - minTargetValue) + minTargetValue;
      }
      break;
      case EFrequencyScale::Log:
      {
        T minTargetValue = std::log(minFreq);
        T maxTargetValue = std::log(maxFreq);
        val = val*(maxTargetValue - minTargetValue) + minTargetValue;
        
        val = std::exp(val);
      }
      break;    
      case EFrequencyScale::Mel:
      {
        T minTargetValue = HzToMel(minFreq);
        T maxTargetValue = HzToMel(maxFreq);
        val = val*(maxTargetValue - minTargetValue) + minTargetValue;
        
        val = MelToHz(val);
      }
      break;
      default:
        break;
    }
    
    return val;
  }
    
  class IFilterBankObj
  {
  public:
    IFilterBankObj() {}
    IFilterBankObj(int dataSize, double sampleRate,
                   T freqHi, T freqLo, int numFilters)
    : mDataSize(dataSize)
    , mSampleRate(sampleRate)
    , mFreqLo(freqLo)
    , mFreqHi(freqHi)
    , mNumFilters(numFilters)
    {  
      mFilters.resize(mNumFilters);
      for (int i = 0; i < mFilters.size(); i++)
      {
        mFilters[i].mData.Resize(dataSize);
        memset(mFilters[i].mData.data(), 0,
               mFilters[i].mData.size()*sizeof(T));
        
        mFilters[i].mBounds[0] = -1;
        mFilters[i].mBounds[1] = -1;
      }
    }
    virtual ~IFilterBankObj() {}
      
  protected:
    friend class IFilterBank;
    
    double mSampleRate = 44100.0;
    T mFreqLo = 20.0;
    T mFreqHi = 22050.0;
    int mDataSize = 0;
    int mNumFilters = 0;
      
    struct IFilter
    {
      std::vector<T> mData;
      int mBounds[2];
    };
    
    std::vector<IFilter> mFilters;
  };
  
  void CreateFilterBankHzToTarget(IFilterBankObj* filterBank,
                                  size_t dataSize,
                                  double sampleRate, T freqLo, T freqHi,
                                  int numFilters)
  {
    // Clear previous
    filterBank->mFilters.resize(0);

    // Init
    filterBank->mDataSize = dataSize;
    filterBank->mSampleRate = sampleRate;
    filterBank->mNumFilters = numFilters;
    filterBank->mFreqLo = freqLo;
    filterBank->mFreqHi = freqHi;
    
    filterBank->mFilters.resize(filterBank->mNumFilters);
    
    for (int i = 0; i < filterBank->mFilters.size(); i++)
    {
      filterBank->mFilters[i].mData.resize(dataSize);
      memset(filterBank->mFilters[i].mData.data(),
             0, filterBank->mFilters[i].mData.size()*sizeof(T));
      
      filterBank->mFilters[i].mBounds[0] = -1;
      filterBank->mFilters[i].mBounds[1] = -1;
    }
      
    // Create filters
    T lowFreqTarget = ScaleValue(freqLo, freqLo, freqHi);
    T highFreqTarget = ScaleValue(freqHi, freqLo, freqHi);
    
    // Compute equally spaced target values
    std::vector<T> targetPoints;
    targetPoints.resize(numFilters + 2);
    for (int i = 0; i < targetPoints.size(); i++)
    {
      // Compute target value
      T t = ((T)i)/(targetPoints.size() - 1);
      T val = lowFreqTarget + t*(highFreqTarget - lowFreqTarget);
      
      targetPoints.data()[i] = val;
    }
    
    // Compute target points
    std::vector<T> hzPoints;
    hzPoints.resize(targetPoints.size());
    for (int i = 0; i < hzPoints.size(); i++)
    {
      // Compute hz value
      T val = targetPoints.data()[i];
      val = ScaleValueInv(val, freqLo, freqHi);
      
      hzPoints.data()[i] = val;
    }
    
    // Compute bin points
    std::vector<T> bin;
    bin.resize(hzPoints.size());
    
    T hzPerBinInv = (dataSize + 1)/(freqHi - freqLo);
    for (int i = 0; i < bin.size(); i++)
    {
      // Compute hz value
      T val = hzPoints.data()[i];
      
      // For the new solution that fills holes, do not round or trunk
      val = (val + freqLo)*hzPerBinInv;
      
      bin.data()[i] = val;
    }
    
    // For each filter
    for (int m = 1; m < numFilters + 1; m++)
    {
      T fmin = bin.data()[m - 1]; // left
      T fmid = bin.data()[m];     // center
      T fmax = bin.data()[m + 1]; // right
      
#if FIX_ALIASING_LOW_FREQS
      FixSmallTriangles(&fmin, &fmax, dataSize);
#endif
      
      filterBank->mFilters[m - 1].mBounds[0] = std::floor(fmin);
      filterBank->mFilters[m - 1].mBounds[1] = std::ceil(fmax);
      
      // Check upper bound
      if (filterBank->mFilters[m - 1].mBounds[1] > dataSize - 1)
        filterBank->mFilters[m - 1].mBounds[1] = dataSize - 1;
      
      for (int i = filterBank->mFilters[m - 1].mBounds[0];
           i <= filterBank->mFilters[m - 1].mBounds[1]; i++)
      {
        // Trapezoid
        T x0 = i;
        if (fmin > x0)
          x0 = fmin;
        
        T x1 = i + 1;
        if (fmax < x1)
          x1 = fmax;
        
        T tarea = ComputeTriangleAreaBetween(fmin, fmid, fmax, x0, x1);
        
        // Normalize
        tarea /= (fmid - fmin)*0.5 + (fmax - fmid)*0.5;
        
        filterBank->mFilters[m - 1].mData.data()[i] += tarea;
      }
    }
  }
  
  void CreateFilterBankTargetToHz(IFilterBankObj* filterBank,
                                  int dataSize,
                                  double sampleRate, T freqLo, T freqHi,
                                  int numFilters)
  {
    filterBank->mDataSize = dataSize;
    filterBank->mSampleRate = sampleRate;
    filterBank->mNumFilters = numFilters;
    filterBank->mFreqLo = freqLo;
    filterBank->mFreqHi = freqHi;
      
    filterBank->mFilters.resize(filterBank->mNumFilters);
  
    for (int i = 0; i < filterBank->mFilters.size(); i++)
    {
      filterBank->mFilters[i].mData.Resize(dataSize);
      memset(filterBank->mFilters[i].mData.data(),
             0, filterBank->mFilters[i].mData.size()*sizeof(T));
      
      filterBank->mFilters[i].mBounds[0] = -1;
      filterBank->mFilters[i].mBounds[1] = -1;
    }
    
    // Create filters
    T lowFreqHz = freqLo;
    T highFreqHz = freqHi;
    
    std::vector<T> hzPoints;
    hzPoints.Resize(numFilters + 2);
    for (int i = 0; i < hzPoints.size(); i++)
    {
      // Compute hz value
      T t = ((T)i)/(hzPoints.size() - 1);
      T val = lowFreqHz + t*(highFreqHz - lowFreqHz);
      
      hzPoints.data()[i] = val;
    }
    
    // Compute hz points
    std::vector<T> targetPoints;
    targetPoints.Resize(hzPoints.size());
    for (int i = 0; i < targetPoints.size(); i++)
    {
      // Compute hz value
      T val = hzPoints.data()[i];
      val = ScaleValue(val, freqLo, freqHi);
      
      targetPoints.data()[i] = val;
    }
    
    // Compute bin points
    std::vector<T> bin;
    bin.Resize(targetPoints.size());
    
    T maxTarget = ScaleValue(freqHi, freqLo, freqHi);
    T targetPerBinInv = (dataSize + 1)/maxTarget;
    for (int i = 0; i < bin.size(); i++)
    {
      // Compute target value
      T val = targetPoints.data()[i];
      
      // For the new solution that fills holes, do not round or trunk
      val = val*targetPerBinInv;
      
      bin.data()[i] = val;
    }
    
    // For each filter
    for (int m = 1; m < numFilters; m++)
    {
      T fmin = bin.data()[m - 1]; // left
      T fmid = bin.data()[m];     // center
      T fmax = bin.data()[m + 1]; // right
      
#if FIX_ALIASING_LOW_FREQS
      FixSmallTriangles(&fmin, &fmax, dataSize);
#endif
      
      filterBank->mFilters[m].mBounds[0] = std::floor(fmin);
      filterBank->mFilters[m].mBounds[1] = std::ceil(fmax);
      
      // Check upper bound
      if (filterBank->mFilters[m ].mBounds[1] > dataSize - 1)
        filterBank->mFilters[m].mBounds[1] = dataSize - 1;
      
      for (int i = filterBank->mFilters[m].mBounds[0];
           i <= filterBank->mFilters[m].mBounds[1]; i++)
      {
        // Trapezoid
        T x0 = i;
        if (fmin > x0)
          x0 = fmin;
        
        T x1 = i + 1;
        if (fmax < x1)
          x1 = fmax;
        
        T tarea = ComputeTriangleAreaBetween(fmin, fmid, fmax, x0, x1);
        
        // Normalize
        tarea /= (fmid - fmin)*0.5 + (fmax - fmid)*0.5;
        
        filterBank->mFilters[m].mData.data()[i] += tarea;
      }
    }
  }

  void ApplyFilterBank(std::vector<T>* output, const std::vector<T> &input, const IFilterBankObj& filterBank)
  {
    output->resize(filterBank.mNumFilters);
    memset(output->data(), 0, output->size()*sizeof(T));
    
    // For each filter
    for (int m = 0; m < filterBank.mNumFilters; m++)
    {
      const typename IFilterBankObj::IFilter& filter = filterBank.mFilters[m];
      
      const T* filterData = filter.mData.data();
      T* outputData = output->data();
      const T* inputData = input.data();
      
      // For each destination value
      for (int i = filter.mBounds[0]; i <= filter.mBounds[1]; i++)
      {
        if (i < 0)
          continue;
        if (i >= input.size())
          continue;
        
        // Apply the filter value
        outputData[m] += filterData[i]*inputData[i];
      }
    }
  }

  void FixSmallTriangles(T* fmin, T* fmax, int dataSize)
  {
    if (dataSize < FIX_ALIASING_MIN_TRIANGLE_WIDTH)
      return;
  
    if (*fmax - *fmin < FIX_ALIASING_MIN_TRIANGLE_WIDTH)
    {
      T diff = FIX_ALIASING_MIN_TRIANGLE_WIDTH - (*fmax - *fmin);
      *fmin -= diff*0.5;
      *fmax += diff*0.5;
      
      if (*fmin < 0.0)
      {
        *fmax += -*fmin;
        *fmin = 0.0;
      }
      
      if (*fmax > dataSize - 1)
      {
        *fmin -= *fmax - (dataSize - 1);
        *fmax = dataSize - 1;
      }
    }
  }
  
  IFilterBankObj mHzToTargetFilterBank;
  IFilterBankObj mTargetToHzFilterBank;

  EFrequencyScale mTargetScaleType;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
