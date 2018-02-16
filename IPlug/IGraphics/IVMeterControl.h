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

  IVMeterControl(IDelegate& dlg, IRECT rect, int paramIdx = kNoParameter,
                 bool displayIndB = false, double minVal = 0.0, double maxVal = 1.0)
    : IControl(dlg, rect)
    , IVectorBase(&DEFAULT_BG_COLOR, &DEFAULT_M_COLOR, &DEFAULT_TXT_COLOR, &DEFAULT_PK_COLOR)
    {
    // todo:
    //if (rect.Empty()) {
    //  mRECT = GetUI()->GetBounds();
    //  mRECT.L = mRECT.R - 20.0f;
    //  mTargetRECT = mRECT;
    //  }

    mDisplayDB = displayIndB;
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
  void OnMouseDown(float x, float y, const IMouseMod& mod) override {
    mHoldingAPeak = false;
    }

  void SetOverdriveThreshold(double thresh) {
    if (mDisplayDB)
      mOverdriveThresh = DBToAmp(thresh);
    else {
      if (thresh < 0.0) thresh *= -1.0;
      mOverdriveThresh = thresh;
      }
    }

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
    if (min > max) {
      auto b = max;
      max = min;
      min = b;
      }
    else if (min == max)
      max += 0.1;

    if (mDisplayDB) {
      mMaxDisplayVal = DBToAmp(max);
      mMinDisplayVal = DBToAmp(min);
      }
    else {
      if (min < 0.0) min *= -1.0;
      if (max < 0.0) max *= -1.0;
      mMinDisplayVal = min;
      mMaxDisplayVal = max;
      }
    SetDirty();
    }
  void SetDisplayInDB(bool db) {
    mDisplayDB = db;
    SetDirty();
  }

  void SetShowOverdriveRect(bool show) {
    mShowODRect = show;
    mHoldingAPeak = false;
    }
  void SetShowMemRect(bool show) {
    mShowMemRect = show;
    }

  private:
  bool mDisplayDB = false;

  bool mHoldPeaks = true; // useful in audio debugging
  bool mHoldingAPeak = false;
  double mOverdriveThresh = 1.0;
  double mODBlink = 0.0;
  double mMemPeak = 0.0;
  double mMemExp = 1.0;
  double mDropMs = 2000.0;

  double mMinDisplayVal = 0.0; // all signal vals are stored as normalized vals,
  double mMaxDisplayVal = 1.0; // but in Setters are interpreted depending on display mode

  bool mShowMemRect = true;
  bool mShowODRect = true;
  float mODRectHeight = 10.0;

  bool mShowLabels = true; // todo
  bool mDrawShadows = true; // todo

  double GetExpForDrop(double ms, double fps) {
    return pow(0.01, 1000.0 / (ms * fps)); // musicdsp.org "envelope follower" discussion
    }
  double GetInvExpForDrop(double ms, double fps) {
    return pow(0.01, 1000.0 / (-ms * 2.0 * fps)); // 2.0 is empirical adjustment, nicer visually
    }
  double GetPeakFromMemExp() {
    return mMemPeak - (mMemExp - 1.0);
    }
  IRECT GetMeterRect() {
    auto mr = mRECT;
    if (mShowODRect)
      mr.T += mODRectHeight;
    // todo labels
    return mr;
    }
  float GetRTopFromValInMeterRect(double v, IRECT meterR) {
    auto dynRange = mMaxDisplayVal - mMinDisplayVal;
    auto t = meterR.B;
    if (v > mMaxDisplayVal)
      t = meterR.T;
    else if (v > mMinDisplayVal) {
      auto r = 0.0;
      if (mDisplayDB) {
        auto mindB = AmpToDB(mMinDisplayVal);
        auto maxdB = AmpToDB(mMaxDisplayVal);
        auto vdB = AmpToDB(v);
        r = (vdB - mindB) / (maxdB - mindB);
        }
      else {
        r = (v - mMinDisplayVal) / (mMaxDisplayVal - mMinDisplayVal);
        }
      t -= (float) r * meterR.H();
      }
    return t;
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