#pragma once

#include "IControl.h"

/*
Vector meter control by Eugene Yakshin
*/
class IVMeterControl : public IControl
  , public IVectorBase {
  public:
  static const IColor DEFAULT_BG_COLOR;
  static const IColor DEFAULT_RAW_COLOR;
  static const IColor DEFAULT_PK_COLOR;
  static const IColor DEFAULT_FR_COLOR;
  static const IColor DEFAULT_TXT_COLOR;

  // map to IVectorBase colors
  enum EVKColor {
    mBg = kBG,
    mM = kFG,
    mPk = kHL,
    mFr = kFR,
    mTxt = kX1
    };

  IVMeterControl(IDelegate& dlg, IRECT rect, int numChannels, const char* chanNames, ...)
    : IControl(dlg, rect, kNoParameter)
    , IVectorBase(&DEFAULT_BG_COLOR, &DEFAULT_RAW_COLOR, &DEFAULT_TXT_COLOR, &DEFAULT_PK_COLOR) {

    ChannelSpecificData d;
    for (auto c = 0; c != numChannels; ++c)
      mChanData.Add(d);

    va_list args;
    va_start(args, chanNames);
    SetChanNames(chanNames, args);
    va_end(args);

    mTargetRECT = GetControlRectFromChannelsData(false);
    if (rect.Empty())
      mRECT = mTargetRECT;
    else {
      auto wr = rect.W() / mTargetRECT.W();
      for (auto m = 0; m != NumChannels(); ++m) {
        // todo account for labels and stuff
        // should be ~the same as in OnResize()

        *MeterWidthPtr(m) *= wr;
        *DistToTheNextMPtr(m) *= wr;
        }
      mMeterHeight = rect.H();
      mRECT = mTargetRECT = rect;
      }

    };

  ~IVMeterControl() {
    for (auto c = 0; c != NumChannels(); ++c)
      delete(ChanNamePtr(c));
    }

// todo add thread safe channel add/del.
// idea: add channelSpecific bool member "deleted" and process at the end of Draw()

  void SetSampleRate(double sr) { mSampleRate = sr; }

  void Draw(IGraphics& graphics)  override;

  void SetDirty(bool pushParamToDelegate = false) override {
    mDirty = true;
    }

  void OnResize() {
    // todo
    SetDirty();
    }

  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override {
    for (int ch = 0; ch != NumChannels(); ++ch)
      *HoldingAPeakPtr(ch) = false;
    }
  void OnMouseDown(float x, float y, const IMouseMod& mod) override {
    // todo : doesnt react during peakhold time
    for (int ch = 0; ch != NumChannels(); ++ch)
      if (GetMeterRect(ch, true).Contains(x, y)) {
        *HoldingAPeakPtr(ch) = false;
        break;
        }
    }

  void SetChanName(int chId, const char* name) {
    if (chId < NumChannels())
      ChanNamePtr(chId)->Set(name);
    }

  void SetChanNames(const char* names, ...) {
    va_list args;
    va_start(args, names);
    SetChanNames(names, args);
    va_end(args);
    SetDirty(false);
    }

  void SetOverdriveThreshold(double thresh, int chId = -1) {
    auto Set = [this] (int i, double t) {
      if (DisplayDB(i))
        t = DBToAmp(i);
      else if (t < 0.0)
        t *= -1.0;
      *OverdriveThreshPtr(i) = t;
      };

    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        Set(ch, thresh);
    else
      Set(chId, thresh);

    }

  // you can use it for raw data, but don't forget to adjust the sample rate for your use case
  void UpdateChannelValue(double value, int chId = -1) {
    value = abs(value);

    auto Set = [this] (int i, double v) {
      if (v > RawValue(i))
        *RawValuePtr(i) = v; // it makes sense to show max value that was gained between the Draw() calls.
                             // in Draw then the value is zeroed right after reading.
                             // it's not 100% thread safe, but in this case it doesn't matter.

      if (v >= GetPeakFromMemExp(i)) {
        *MemPeakPtr(i) = v;
        *MemExpPtr(i) = 1.0;
        *PeakSampHeldPtr(i) = 0;
        }

      if (MemPeak(i) >= OverdriveThresh(i)) {
        *ODBlinkPtr(i) = 1.0;
        if (mHoldPeaks)
          *HoldingAPeakPtr(i) = true;
        }
      };

    if (chId > -1) // this case first by use case frequency
      Set(chId, value);
    else
      for (int ch = 0; ch != NumChannels(); ++ch)
        Set(ch, value);

    SetDirty();
    }
  void ProcessChan(sample* in, int blockSize, int chId = -1) {
    if (chId > -1) // this case first by use case frequency
      for (int s = 0; s != blockSize; ++s, ++in)
        UpdateChannelValue((double) *in, chId);
    else
      // useful when your meter shows several different representations of the same signal,
      // or simply when you have only one channel.
      for (int ch = 0; ch != NumChannels(); ++ch)
        for (int s = 0; s != blockSize; ++s, ++in)
          UpdateChannelValue((double) *in, ch);
    }
  // handy for processing a block of either all ins or all outs
  void ProcessBus(sample** ins, int blockSize, int numChans = -1, int sourceStartId = 0, int destStartId = 0)
    {
    if (numChans < 0) numChans = NumChannels();

    for (int ch = 0; ch != numChans; ++ch) {
      auto in = ins[sourceStartId + ch];
      ProcessChan(in, blockSize, destStartId + ch);
      }
    }
  // handy if meter measures both inputs and outputs:
  void ProcessInsOuts(sample** ins, sample** outs, int blockSize, int numIns = -1, int numOuts = -1) {
    if (numIns < 0) numIns = NumChannels() / 2;
    if (numOuts < 0) numOuts = numIns;

    for (int ch = 0; ch != numIns; ++ch) {
      auto in = ins[ch];
      ProcessChan(in, blockSize, ch);
      }
    for (int ch = 0; ch != numOuts; ++ch) {
      auto out = outs[ch];
      ProcessChan(out, blockSize, ch + numIns);
      }
    }

  void SetPeakDropTimeMs(double ms, int chId = -1) {
    ms = abs(ms);

    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *DropMsPtr(ch) = ms;
    else
        *DropMsPtr(chId) = ms;
    }
  void SetHoldPeaks(bool hold) { mHoldPeaks = hold; }
  void SetPeakRectHeight(double h) { mODRectHeight = (float) h; }
  void SetMinMaxDisplayValues(double min, double max, int chId = -1) {
    if (min > max) {
      auto b = max;
      max = min;
      min = b;
      }
    else if (min == max)
      max += 0.1;

    auto Set = [this] (int i, double min, double max) {
      if (DisplayDB(i)) {
        *MaxDisplayValPtr(i) = DBToAmp(max);
        *MinDisplayValPtr(i) = DBToAmp(min);
        }
      else {
        *MinDisplayValPtr(i) = abs(min);
        *MaxDisplayValPtr(i) = abs(max);
        }
      };

    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        Set(ch, min, max);
    else
      Set(chId, min, max);

    SetDirty();
    }
  void SetDisplayInDB(bool db, int chId = -1) {
     if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *DisplayDBPtr(ch) = db;
    else
     *DisplayDBPtr(chId) = db;

    SetDirty();
    }

  void SetShowOverdriveRect(bool show, int chId = -1) {
    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch) {
        *ShowODRectPtr(ch) = show;
        *HoldingAPeakPtr(ch) = false;
        if (!show) *ODBlinkPtr(ch) = 0.0;
        }
    else {
      *ShowODRectPtr(chId) = show;
      *HoldingAPeakPtr(chId) = false;
      if (!show) *ODBlinkPtr(chId) = 0.0;
      }
    SetDirty();
    }
  void SetShowMemRect(bool show, int chId = -1) {
    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *ShowMemRectPtr(ch) = show;
    else
      *ShowMemRectPtr(chId) = show;
    SetDirty();
    }

  void SetMeterWidth(double w, int chId = -1) {
    if (w < 0.0) w = 20.0;
    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *MeterWidthPtr(ch) = (float)w;
    else
      *MeterWidthPtr(chId) = (float)w;

    mRECT = mTargetRECT = GetControlRectFromChannelsData();

    SetDirty();
    }
  void SetDistToTheNextMeter(double d, int chId = -1, bool glueRects = true) {
    if (d < 0.0) d = 30.0;
    glueRects = glueRects && (d == 0.0);

    if (chId < 0) {
      if (glueRects) {
        float w = 0.0;
        for (int ch = 0; ch != NumChannels() - 1; ++ch)
          w += DistToTheNextM(ch);
        w /= NumChannels();
        for (int ch = 0; ch != NumChannels(); ++ch)
          *MeterWidthPtr(ch) += w;
        }
      for (int ch = 0; ch != NumChannels(); ++ch)
        *DistToTheNextMPtr(ch) = (float)d;
      }
    else {
      if (glueRects && d == 0.0 && chId != NumChannels() - 1) {
        auto stretch = 0.5f * DistToTheNextM(chId);
        *MeterWidthPtr(chId) += stretch;
        *MeterWidthPtr(chId + 1) += stretch;
        }
      *DistToTheNextMPtr(chId) = (float)d;
      }

    mRECT = mTargetRECT = GetControlRectFromChannelsData();

    SetDirty();
    }

  private:

  double mSampleRate = DEFAULT_SAMPLE_RATE;

  struct ChannelSpecificData
    {
  // all signal related vals are stored as abs vals and interpreted when needed
    double rawValue;
    double minDisplayVal = 0.001; // -60dB
    double maxDisplayVal = DBToAmp(3.0);
    bool displayDB = true;

    double memPeak = 0.0; // max remembered value for memRect
    double memExp = 1.0; // decay exponent for memPeak
    size_t peakSampHeld = 0;
    double dropMs = 2000.0;
    double overdriveThresh = 1.0;
    double oDBlink = 0.0; // overdrive rect intensity
    bool holdingAPeak = false;

    float meterWidth = 20.0;
    float distToNextM = 30.0; // use for gluing chans into groups like ins and outs

    bool showMemRect = true;
    bool showODRect = true;
    bool showLabels = true; // todo
    bool showChanName = true; // todo

    WDL_String* chanName = nullptr;
    };

  WDL_TypedBuf<ChannelSpecificData> mChanData;

  int NumChannels() { return mChanData.GetSize(); }

  float mMeterHeight = 150.0;
  float mODRectHeight = 10.0;
  bool mHoldPeaks = true; // useful in audio debugging

  bool mDrawShadows = true;
  float mShadowOffset = 3.0;
  bool mDrawBorders = true;

  double GetExpForDrop(double ms, double fps) {
    return pow(0.01, 1000.0 / (ms * fps)); // musicdsp.org "envelope follower" discussion
    }
  double GetInvExpForDrop(double ms, double fps) {
    return pow(0.01, 1000.0 / (-ms * 2.0 * fps)); // 2.0 is empirical adjustment, nicer visually
    }
  double GetPeakFromMemExp(int chId) {
    return MemPeak(chId) - (MemExp(chId) - 1.0);
    }
  IRECT GetMeterRect(int i, bool withODRect = false) {
    // todo account for labels
    float dx = 0.0;
    for (int m = 0; m != i; ++m) {
      dx += MeterWidth(m);
      dx += DistToTheNextM(m);
      }
    auto mr = mRECT;
    mr.L += dx;
    mr.R = mr.L + MeterWidth(i);
    if (ShowODRect(i) && !withODRect)
      mr.T += mODRectHeight;
    return mr;
    }
  IRECT GetControlRectFromChannelsData(bool keepPosition = true) {
    // todo account for labels
    float w = 0.0;
    for (int m = 0; m != NumChannels(); ++m) {
      w += MeterWidth(m);
      if (m < NumChannels() - 1)
        w += DistToTheNextM(m);
      }
    auto r = IRECT(0.0, 0.0, w, mMeterHeight);
    if (keepPosition) {
      r.L += mRECT.L;
      r.R += mRECT.L;
      r.T += mRECT.T;
      r.B += mRECT.T;
      }
    return r;
    }
  float GetRTopFromValInMeterRect(int chId, double v, IRECT meterR) {
    auto t = meterR.B;
    if (v > MaxDisplayVal(chId))
      t = meterR.T;
    else if (v > MinDisplayVal(chId)) {
      auto r = 0.0;
      if (DisplayDB(chId)) {
        auto mindB = AmpToDB(MinDisplayVal(chId));
        auto maxdB = AmpToDB(MaxDisplayVal(chId));
        auto vdB = AmpToDB(v);
        r = (vdB - mindB) / (maxdB - mindB);
        }
      else {
        r = (v - MinDisplayVal(chId)) / (MaxDisplayVal(chId) - MinDisplayVal(chId));
        }
      t -= (float) r * meterR.H();
      }
    return t;
    }
  IColor LinearBlendColors(IColor cA, IColor cB, double mix) {
    IColor cM;
    cM.A = (int) ((1.0 - mix) * cA.A + mix * cB.A);
    cM.R = (int) ((1.0 - mix) * cA.R + mix * cB.R);
    cM.G = (int) ((1.0 - mix) * cA.G + mix * cB.G);
    cM.B = (int) ((1.0 - mix) * cA.B + mix * cB.B);
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

  void DrawInnerShadowForRect(IRECT r, IColor shadowColor, IGraphics& graphics) {
    auto& o = mShadowOffset;
    auto slr = r;
    slr.R = slr.L + o;
    auto str = r;
    str.L += o;
    str.B = str.T + o;
    graphics.FillRect(shadowColor, slr);
    graphics.FillRect(shadowColor, str);
    }

  void SetChanNames(const char* names, va_list args) {
    if (NumChannels() < 1) return;
    *(ChanNamePP(0)) = new WDL_String(names);
    for (int c = 1; c < NumChannels(); ++c)
      *(ChanNamePP(c)) = new WDL_String(va_arg(args, const char*));
    }

  // getters
  auto Ch(int i) { return mChanData.Get() + i; }

  double* RawValuePtr       (int i) { return &(Ch(i)->rawValue); }
  double RawValue           (int i) { return *RawValuePtr(i); }
  double* MinDisplayValPtr  (int i) { return &(Ch(i)->minDisplayVal); }
  double MinDisplayVal      (int i) { return *MinDisplayValPtr(i); }
  double* MaxDisplayValPtr  (int i) { return &(Ch(i)->maxDisplayVal); }
  double MaxDisplayVal      (int i) { return *MaxDisplayValPtr(i); }

  double* OverdriveThreshPtr(int i) { return &(Ch(i)->overdriveThresh); }
  double OverdriveThresh    (int i) { return *OverdriveThreshPtr(i); }

  bool* DisplayDBPtr        (int i) { return &(Ch(i)->displayDB); }
  bool DisplayDB            (int i) { return *DisplayDBPtr(i); }

  double* DropMsPtr         (int i) { return &(Ch(i)->dropMs); }
  double DropMs             (int i) { return *DropMsPtr(i); }

  bool* HoldingAPeakPtr     (int i) { return &(Ch(i)->holdingAPeak); }
  bool HoldingAPeak         (int i) { return *HoldingAPeakPtr(i); }
  size_t* PeakSampHeldPtr   (int i) { return &(Ch(i)->peakSampHeld); }
  size_t PeakSampHeld       (int i) { return *PeakSampHeldPtr(i); }

  double* ODBlinkPtr        (int i) { return &(Ch(i)->oDBlink); }
  double ODBlink            (int i) { return *ODBlinkPtr(i); }

  double* MemPeakPtr        (int i) { return &(Ch(i)->memPeak); }
  double MemPeak            (int i) { return *MemPeakPtr(i); }
  double* MemExpPtr         (int i) { return &(Ch(i)->memExp); }
  double MemExp             (int i) { return *MemExpPtr(i); }

  float* MeterWidthPtr      (int i) { return &(Ch(i)->meterWidth); }
  float MeterWidth          (int i) { return *MeterWidthPtr(i); }
  float* DistToTheNextMPtr  (int i) { return &(Ch(i)->distToNextM); }
  float DistToTheNextM      (int i) { return *DistToTheNextMPtr(i); }

  bool* ShowMemRectPtr      (int i) { return &(Ch(i)->showMemRect); }
  bool ShowMemRect          (int i) { return *ShowMemRectPtr(i); }
  bool* ShowODRectPtr       (int i) { return &(Ch(i)->showODRect); }
  bool ShowODRect           (int i) { return *ShowODRectPtr(i); }
  bool* ShowLabelsPtr       (int i) { return &(Ch(i)->showLabels); }
  bool ShowLabels           (int i) { return *ShowLabelsPtr(i); }
  bool* ShowChanNamePtr     (int i) { return &(Ch(i)->showChanName); }
  bool ShowChanName         (int i) { return *ShowChanNamePtr(i); }

  WDL_String* ChanNamePtr   (int i) { return *ChanNamePP(i); }
  WDL_String** ChanNamePP   (int i) { return &(Ch(i)->chanName); }

  };