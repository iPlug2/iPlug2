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
 * @copydoc IAxis
 */

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** IAxisLabel is used to store one axis label */
struct IAxisLabel
{
  float mValue;
  WDL_String mStr;
};
  
/** IAxis displays configurable axis */
class IAxis
{
 public:
  IAxis(const IRECT& bounds,
        IText text,
        float lo = 0.0, float hi = 1.0,
        EDirection dir = EDirection::Horizontal,
        EAlign align = EAlign::Near)
  : mRECT(bounds)
  , mText(text)
  , mLo(lo)
  , mHi(hi)
  , mLogLo(std::log(lo))
  , mLogHi(std::log(hi))
  , mMelLo(HzToMel(lo))
  , mMelHi(HzToMel(hi))
  , mDBLo(AmpToDB(lo))
  , mDBHi(AmpToDB(hi))
  , mDirection(dir)
  , mAlign(align)
  {
    SetDirection(mDirection);
    mText.mVAlign = EVAlign::Middle;
  }
  
  virtual ~IAxis() {}
  
  void SetDirection(EDirection direction)
  {
    mDirection = direction;

    if (mDirection == EDirection::Vertical)
      mText.mAlign = EAlign::Far;
    else
      mText.mAlign = EAlign::Center;
  }
  
  void SetScale(EFrequencyScale scale)
  {
    mScale = scale;
    GenerateLabels();
  }

  void SetRange(float lo, float hi)
  {
    mLo = lo;
    mHi = hi;

    mLogLo = std::log(lo);
    mLogHi = std::log(hi);

    mMelLo = HzToMel(lo);
    mMelHi = HzToMel(hi);

    mDBLo = AmpToDB(lo);
    mDBHi = AmpToDB(hi);
    
    GenerateLabels();
  }

  void SetRangeLo(float lo)
  {
    mLo = lo;
    mLogLo = std::log(lo);
    mMelLo = HzToMel(lo);
    mDBLo = AmpToDB(lo);
    
    GenerateLabels();
  }
  
  void SetRangeHi(float hi)
  {
    mHi = hi;
    mLogHi = std::log(hi);
    mMelHi = HzToMel(hi);
    mDBHi = AmpToDB(hi);
    
    GenerateLabels();
  }
  
  void SetText(const IText& txt)
  {
    mText = txt;
    SetDirection(mDirection);
  }

  void Draw(IGraphics& g, IRECT& clipBounds)
  {
    g.PathClipRegion(clipBounds);
    
    for (int i = 0; i < mLabels.size(); i++)
    {
      const IAxisLabel& label = mLabels[i];
      
      IRECT textDims;
      g.MeasureText(mText, label.mStr.Get(), textDims);

      float t = CalcNorm(label.mValue);
      float x, y;
      if (mDirection == EDirection::Horizontal)
      {
        x = mRECT.L + mPadding + (t * (mRECT.W() - mPadding));
        y = mRECT.T + mRECT.H() / 2;
        if (mAlign == EAlign::Near)
          y = mRECT.T + textDims.H();
        else if (mAlign == EAlign::Far)
          y = mRECT.B - textDims.H();
      }
      else // Vertical
      {
        y = mRECT.B - (t * (mRECT.H() - mPadding));

        if (mAlign == EAlign::Near)
          x = mRECT.L + mPadding;
        else if (mAlign == EAlign::Far)
          x = mRECT.R - mPadding;
        else
          x = mRECT.MW();
      }

      g.DrawText(mText, label.mStr.Get(), x, y);
    }
  }
  
 protected:
  float CalcNorm(float y) const
  {
    switch(mScale)
    {
      case EFrequencyScale::Linear:
        y = (y - mLo)/(mHi - mLo);
      break;
      case EFrequencyScale::Log:
        y = (std::log(y) - mLogLo)/(mLogHi - mLogLo);
      break;
      case EFrequencyScale::Mel:
        y = (HzToMel(y) - mMelLo)/(mMelHi - mMelLo);
      break;
      case EAmplitudeScale::Decibel:
        y = (AmpToDB(y) - mDBLo)/(mDBHi - mDBLo);
      break;
      default:
        break;
    }

    return y;
  }

  virtual void GenerateLabels() {};
  
  IRECT mRECT;
  IText mText;

  float mLo;
  float mHi;

  float mLogLo;
  float mLogHi;

  float mMelLo;
  float mMelHi;

  float mDBLo;
  float mDBHi;
  
  EDirection mDirection;
  EAlign mAlign;
  
  float mPadding = 10.0f;

  std::vector<IAxisLabel> mLabels;
  const int mMaxNumLabels = 10;
};

/** ITimeAxis displays time axis */
class ITimeAxis : public IAxis
{
public:
  ITimeAxis(const IRECT& bounds,
            IText& txt,
            float lo = 0.0, float hi = 1.0,
            EDirection dir = EDirection::Horizontal,
            EAlign align = EAlign::Far)
  : IAxis(bounds, txt, lo, hi, EFrequencyScale::Linear, dir, align) {}

  virtual ~ITimeAxis() {}

protected:
  void GenerateLabels() override
  {
    float step = (mHi - mLo)/mMaxNumLabels;
    float start = ((int)(mLo/step))*step;
    float i = start;

    mLabels.clear();

    while(i < mHi)
    {
      IAxisLabel label;
      label.mValue = i;

      label.mStr = SecondsToLabel(i);
      mLabels.push_back(label);

      i += step;
    }
  }

  WDL_String SecondsToLabel(float seconds)
  {
    WDL_String label;
    
    int iseconds = (int) seconds;
    int hours = seconds/3600.0;
    int minutes = (seconds - hours*3600.0)/60.0;
    int sec = (seconds - minutes*60.0);
    int millis = (seconds - sec)*1000.0;

    if (hours > 0)
    {
      // Keep 2 decimals
      millis = millis / 100;
      label.SetFormatted(64, "%d:%d:%d.%d",
                         hours, minutes, iseconds, millis);
    }
    else if (minutes > 0)
    {
      // Keep 2 decimals
      millis = millis / 100;
      label.SetFormatted(64, "%d:%d.%d", minutes, iseconds, millis);
    }
    else if (sec > 0)
    {
      // Keep 2 decimals
      millis = millis / 100;
      label.SetFormatted(64, "%d.%ds", sec, millis);
    }
    else
    {
      label.SetFormatted(64, "%dms", millis);
    }

    return label;
  }
};

/** ITimeAxis displays time axis */
class IFrequencyAxis : public IAxis
{
public:
  IFrequencyAxis(const IRECT& bounds,
                 IText& txt,
                 const std::vector<float>& freqs,
                 float lo = 20.0, float hi = 22050.0,
                 EFrequencyScale scale = EFrequencyScale::Log,
                 EDirection dir = EDirection::Vertical,
                 EAlign align = EAlign::Far)
  : IAxis(bounds, txt, lo, hi, scale, dir, align)
  {
    GenerateLabels(freqs);
  }

  virtual ~IFrequencyAxis() {}
  
protected:
  void GenerateLabels(const std::vector<float>& freqs)
  {
    mLabels.clear();

    for (float freq : freqs)
    {
      IAxisLabel label;
      label.mValue = freq;

      label.mStr = FreqToLabel(freq);
      mLabels.push_back(label);
    }
  }

  WDL_String FreqToLabel(float freq)
  {
    WDL_String label;
    
    if (freq < 1000.0)
    {
      label.SetFormatted(64, "%dHz", (int)freq);
    }
    else if (freq >= 1000.0)
    {
      label.SetFormatted(64, "%dKHz", (int)(freq*0.001));
    }
    
    return label;
  }
};

/** IAmpAxis displays amplitude axis */
class IAmpAxis : public IAxis
{
public:
  IAmpAxis(const IRECT& bounds,
           IText& txt,
           const std::vector<float>& ampsDB,
           float lo = 0.0, float hi = 1.0,
           EAmplitudeScale scale = EFrequencyScale::DB,
           EDirection dir = EDirection::Vertical,
           EAlign align = EAlign::Far)
  : IAxis(bounds, txt, lo, hi, scale, dir, align)
  {
    // Trick, because we pass DB as input
    mLo = DBToAmp(lo);
    mHi = DBToAmp(hi);

    mDBLo = lo;
    mDBHi = hi;
    
    std::vector<float> amps;
    amps.resize(ampsDB.size());
    if (scale == EFrequencyScale::DB)
    {
      int i = 0;
      for (float ampDB : ampsDB)
      {
        float amp = DBToAmp(ampDB);
        amps[i++] = amp;
      }
    }
    
    GenerateLabels(amps);
  }

  virtual ~IAmpAxis() {}
  
protected:
  void GenerateLabels(const std::vector<float>& amps)
  {
    mLabels.clear();

    for (float amp : amps)
    {
      IAxisLabel label;
      label.mValue = amp;

      amp = AmpToDB(amp);
      
      label.mStr = AmpToLabel(amp);
      mLabels.push_back(label);
    }
  }

  WDL_String AmpToLabel(float amp)
  {
    WDL_String label;
    
    if (mScale == EAmplitudeScale::Linear)
      label.SetFormatted(64, "%.5f", amp);
    else if (mScale == EAmplitudeScale::Decibel)
      label.SetFormatted(64, "%ddB", (int)amp);
    
    return label;
  }
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
