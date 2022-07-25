/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
 */

#pragma once

/**
 * @file
 * @copydoc ITransport
 * @brief Manage real time and time based on incoming data
 */

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** ITransport is a utility class which manages real elapsed time and theoric time based on amount of incoming data received */
class ITransport
{
public:
  using clock = std::chrono::high_resolution_clock;
  
  void Reset()
  {
    mStartTime = clock::now();
    mStartTimeDiff = mStartTime;
  }
  
  void SetSampleRate(double sampleRate)
  {
    mSampleRate = sampleRate;
  }

  void SetOverlap(int overlap)
  {
    mOverlap = overlap;
  }

  /** Compute time duration in ms for a given data size */
  double ComputeDataDuration(int dataSize) const
  {
    // NOTE: this is strange that we don't need overlap here
    return (1000.0 * dataSize) / mSampleRate;
  }

  void OnNewData(int dataSize)
  {
    // NOTE: this is strange to take overlap here
    dataSize *= mOverlap;
    
    if (mFirstDataReceived)
    {
      mNumSamplesProcessed += dataSize;
    }
    else
    {
      mFirstDataReceived = true;
      Reset();
    }
  }

  double GetElapsedTime() const
  {
    if (!mFirstDataReceived)
      return 0.0;

    auto now = clock::now();
    return (now - mStartTime).count();
  }

  /** Compute time difference in milliseconds between the real elapsed 
      time and the elapsed time computed from incomming data */
  double ComputeTimeDifference(double maxTimeDiff)
  {
    if (!mFirstDataReceived)
      return 0.0;

    auto now = clock::now();
    double realDuration = (now - mStartTimeDiff).count();
    double dataDuration = (1000.0 * mNumSamplesProcessed)/(mSampleRate*mOverlap);

    // Reset offset if too much drift
    double diff = realDuration - dataDuration;
    
    if (std::fabs(diff) > maxTimeDiff)
    {
      diff = 0.0;

      // Reset
      mStartTimeDiff = now;
      mNumSamplesProcessed = 0;
    }
    
    return diff;
  }

 private:
  double mSampleRate = 44100.0;
  int mOverlap = 2;

  bool mFirstDataReceived = false;
  TimePoint mStartTime;
  TimePoint mStartTimeDiff;
  long long mNumSamplesProcessed = 0;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
