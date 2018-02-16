#pragma once

#include "IControl.h"

/*
Vector meter control by Eugene Yakshin
*/
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
    if (rect.Empty()) {
      mRECT = GetUI()->GetBounds();
      mRECT.L = mRECT.R - 20.0f;
      mTargetRECT = mRECT;
      }

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

  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override {
    mHoldingAPeak = false;
    }
  void SetOverdriveThreshold(double thresh) { mOverdriveThresh = thresh; }

  void ProcessBlock(sample* in, int blockSize) {
    for (int s = 0; s < blockSize; ++s, ++in)
      SetValueFromDelegate((double)*in);
    }
  void SetValueFromDelegate(double value) override {
    if (value < 0.0) value *= -1.0;
    if (value == mValue) return;

    mValue = value;

    if (value >= GetPeakFromMemExp()) {
      mMemPeak = value;
      mMemExp = 1.0;
      }

    if (mMemPeak >= mOverdriveThresh) {
      mODBlink = 1.0;
      if (mHoldPeaks)
        mHoldingAPeak = true;
      }

    SetDirty();
    }
  void SetValueFromUserInput(double value) override {
    SetValueFromUserInput(value);
    }
  void SetPeakDropTimeMs(double ms) {
    if (ms < 0.0) ms *= -1.0;
    mDropMs = ms;
    }
  void SetHoldPeaks(bool hold) { mHoldPeaks = hold; }
  void SetPeakRectHeight(double h) { mODRectHeight = (float) h; }
  void SetMinMaxDisplayValues(double min, double max) {
    // assume user vals units are consistent with the display mode
    if (min > max) {
      auto b = max;
      max = min;
      min = b;
      }
    else if (min == max)
      max += 0.1;

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

  bool mHoldPeaks = true; // useful in audio debugging
  bool mHoldingAPeak = false;
  double mOverdriveThresh = 1.0;
  double mODBlink = 0.0;
  double mMemPeak = 0.0;
  double mMemExp = 1.0;
  double mDropMs = 2000.0;

  float mMinDisplayVal = 0.0f; // display vals are stored as normalized vals,
  float mMaxDisplayVal = 1.0f; //  but in Set..() and Draw() are interpreted depending on display mode

  bool mShowMemRect = true; // todo
  bool mShowODRect = true; // todo
  float mODRectHeight = 10.0;

  bool mShowLabels = true; // todo

  double GetExpForDrop(double ms, double fps) {
    return pow(0.01, 1000.0 / (ms * fps)); // musicdsp.org "envelope follower" discussion
    }
  double GetInvExpForDrop(double ms, double fps) {
    return pow(0.01, 1000.0 / (-ms * 2.0 * fps)); // 2. is empirical adjustment, nicer visually
    }
  double GetPeakFromMemExp() {
    return mMemPeak - (mMemExp - 1.0);
    }
  IRECT GetMeterRect() {
    auto mr = mRECT;
    if (mShowODRect)
      mr.T += mODRectHeight;
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