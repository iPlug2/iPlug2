#pragma once

#include "IControl.h"

class IVMeterControl : public IControl
  , public IVectorBase
  {
  public:
  static const IColor DEFAULT_BG_COLOR;
  static const IColor DEFAULT_M_COLOR;
  static const IColor DEFAULT_PK_COLOR;
  static const IColor DEFAULT_FR_COLOR;
  static const IColor DEFAULT_TXT_COLOR;

  // map to IVectorBase colors
  enum EVKColor
  {
    mBg = kBG,
    mM = kFG,
    mPk = kHL,
    mFr = kFR,
    mTxt = kX1
  };

  enum DisplayMode {
    PMDM_NORM,
    PMDM_DB
    };

  IVMeterControl(IDelegate& dlg, IRECT rect, int paramIdx = kNoParameter,
                 DisplayMode dm = PMDM_NORM, double minVal = 0.0, double maxVal = 1.0)
    : IControl(dlg, rect)
    , IVectorBase(&DEFAULT_BG_COLOR, &DEFAULT_M_COLOR, &DEFAULT_TXT_COLOR, &DEFAULT_PK_COLOR)
    {
    mDisplayMode = dm;
    SetMinMaxDisplayValues(minVal, maxVal);
    };

  ~IVMeterControl() {
    // todo del labels
    }

  void Draw(IGraphics& graphics)  override {
    float fps = (float) graphics.FPS();
    float v = (float) mValue;
    // todo watch out for 0 in db conversions
    graphics.FillRect(GetColor(mBg), mRECT);

    auto mr = mRECT;
    mr.T = mr.B - v * mr.H();
    graphics.FillRect(GetColor(mM), mr);

    auto p = mRECT.B - mPeak * mRECT.H();
    graphics.DrawLine(GetColor(mPk), mr.L, p, mr.R, p);
    mPeak *= 0.99;
    SetDirty();

    };

  void SetDirty(bool pushParamToDelegate = false) override {
    mDirty = true;
    }

  void OnResize() {
    // todo
    SetDirty();
    }

  void ProcessBlock(sample* in, int blockSize) {
    for (int s = 0; s < blockSize; ++s, ++in)
      SetValueFromDelegate((double)*in);
    }
  void SetValueFromDelegate(double value) override {
    if (value < 0.0) value *= -1.0;
    if (value == mValue) return;
    mValue = value;
    if (value > mPeak)
      mPeak = (float) value;
    SetDirty();
    }
  void SetValueFromUserInput(double value) override {
    SetValueFromUserInput(value);
    }
  void SetPeakDropSpeed(double s) { mPeakDropSpeed = (float) s; }
  void SetMinMaxDisplayValues(double min, double max) {
    // assume a user provides display vals that are consistent with the display mode
    switch (mDisplayMode) {
        default:
        case IVMeterControl::PMDM_NORM:
          if (min < 0.0) min *= -1.0;
          if (max < 0.0) max *= -1.0;
          mMinDisplayVal = (float) min;
          mMaxDisplayVal = (float) max;
          break;
        case IVMeterControl::PMDM_DB:
          // todo
          break;
      }
    SetDirty();
    }
  void SetDisplayMode(DisplayMode m) {
    mDisplayMode = m;
    SetDirty();
  }

  private:
  DisplayMode mDisplayMode = PMDM_NORM;

  float mPeak = 0.0;
  float mPeakDropSpeed = 0.5; // 0.0 = holds forever, 1.0 = falls immidiately

  float mMinDisplayVal = 0.0; // display vals are stored as normalized vals,
  float mMaxDisplayVal = 1.0; //  but in Set..() and Draw() are interpreted depending on display mode

  bool mShowPeakLine = true; // todo
  bool mShowPeakRect = true; // todo

  bool mShowLabels = true; // todo

  };