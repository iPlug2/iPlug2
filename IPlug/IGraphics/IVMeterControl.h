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

  void Draw(IGraphics& graphics)  override;

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
    if (mPeak >= 1.0f)
      mPeakRectBlink = 1.0;
    SetDirty();
    }
  void SetValueFromUserInput(double value) override {
    SetValueFromUserInput(value);
    }
  void SetPeakDropTimeMs(double s) {
    if (s < 0.0) s *= -1.0;
    mPeakDropMs = (float)s;
    }
  void SetPeakRectHeight(double h) { mPeakRectHeight = (float) h; }
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
  float mPeakRectBlink = 0.0;
  float mPeakDropMs = 0.5; // 0.0 = holds forever, 1.0 = falls immidiately

  float mMinDisplayVal = 0.0; // display vals are stored as normalized vals,
  float mMaxDisplayVal = 1.0; //  but in Set..() and Draw() are interpreted depending on display mode

  bool mShowPeakLine = true; // todo
  bool mShowPeakRect = true; // todo
  float mPeakRectHeight = 10.0;

  bool mShowLabels = true; // todo

  IRECT GetMeterRect() {
    auto mr = mRECT;
    if (mShowPeakRect)
      mr.T += mPeakRectHeight;
    // todo shadows, labels
    return mr;
    }
  IColor LinearBlendColors(IColor cA, IColor cB, double mix) {
    IColor cM;
    cM.A = (int)((1.0 - mix) * cA.A + mix * cB.A);
    cM.R = (int)((1.0 - mix) * cA.R + mix * cB.R);
    cM.G = (int)((1.0 - mix) * cA.G + mix * cB.G);
    cM.B = (int)((1.0 - mix) * cA.B + mix * cB.B);
    return cM;
    }
  void BasicTextMeasure(const char* txt, float& numLines, float& maxLineWidth) {
    // todo del from here if upstream has it on the moment of pull request
   float w = 0.0;
   maxLineWidth = 0.0;
   numLines = 0.0;
   while (true) {
     if (*txt == '\0' || *txt == '\n') {
       ++numLines;
       if (w > maxLineWidth)
         maxLineWidth = w;
       if (*txt == '\0')
         break;
       w = 0.0;
       }
     else
       ++w;
     ++txt;
     }
   }

  };