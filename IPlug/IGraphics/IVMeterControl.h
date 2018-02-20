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

  // map to IVectorBase colors
  enum EVKColor {
    mBg = kBG,
    mRaw = kFG,
    mPeak = kHL,
    mFr = kFR,
    };

  IVMeterControl(IDelegate& dlg, IRECT rect, int numChannels, const char* chanNames, ...)
    : IControl(dlg, rect, kNoParameter)
    , IVectorBase(&DEFAULT_BG_COLOR, &DEFAULT_RAW_COLOR, &DEFAULT_FR_COLOR, &DEFAULT_PK_COLOR) {

    ChannelSpecificData d;
    for (auto ch = 0; ch != numChannels; ++ch) {
      mChanData.Add(d);
      *(ChanNamePP(ch)) = new WDL_String;
      *(MarksPP(ch)) = new WDL_TypedBuf<double>;
      *(MarkLabelsPP(ch)) = new WDL_TypedBuf<bool>;
      }

    SetLevelMarks("3 0s -3 -6s -9 -12s -18 -24s -30 -36 -42 -48s -54s -60");

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
        *MeterWidthPtr(m) = trunc(MeterWidth(m)); // identical narrow meters with noninteger sizes often look different
        *DistToTheNextMPtr(m) *= wr;
        *DistToTheNextMPtr(m) = trunc(DistToTheNextM(m));
        }
      mMeterHeight = rect.H() - mChNameMaxH * mText.mSize;
      mRECT = mTargetRECT = rect;
      }

    };

  ~IVMeterControl() {
    for (auto c = 0; c != NumChannels(); ++c) {
      delete(ChanNamePtr(c));
      delete(MarksPtr(c));
      delete(MarkLabelsPtr(c));
      }
    }

// todo add thread safe channel add/del.
// idea: add channelSpecific bool member "deleted" and process at the end of Draw()

  void SetSampleRate(double sr) { mSampleRate = sr; }

  // you can use it for raw data, but don't forget to adjust the sample rate for your use case
  void ProcessChanValue(double value, int chId = -1) {
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

      if (MemPeak(i) >= OverThresh(i)) {
        *OverBlinkPtr(i) = 1.0;
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
        ProcessChanValue((double) *in, chId);
    else
      // useful when your meter shows several different representations of the same signal,
      // or simply when you have only one channel.
      for (int ch = 0; ch != NumChannels(); ++ch)
        for (int s = 0; s != blockSize; ++s, ++in)
          ProcessChanValue((double) *in, ch);
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

  void SetDisplayInDB(bool db, int chId = -1) {
     if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *DisplayDBPtr(ch) = db;
    else
     *DisplayDBPtr(chId) = db;

    SetDirty();
    }
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

  void SetDrawChName(bool draw, int chId = -1) {
    // todo
    SetDirty();
    }
  void SetChanName(int chId, const char* name) {
    if (chId < NumChannels())
      ChanNamePtr(chId)->Set(name);
    SetDirty();
    }
  void SetChanNames(const char* names, ...) {
    va_list args;
    va_start(args, names);
    SetChanNames(names, args);
    va_end(args);
    SetDirty();
    }
  void SetChNameHOffset(int chId, double offset) {
    *ChanNameHOffsetPtr(chId) = (float) offset;
    // todo recalc if 1st or last
    SetDirty();
    }
  // todo: call from all labels and names setters
  // todo these two protected
  void RecalcLabelsMargins() {
    float w, h;
    // todo widths. don't forget level labels
    for (int ch = 0; ch != NumChannels(); ++ch) {
      BasicTextMeasure(ChanNamePtr(ch)->Get(), h, w);
      if (h > mChNameMaxH) mChNameMaxH = h;
      }
    UpdateLabelsMargins();
    }
  void UpdateLabelsMargins() {
    mRECT.B = mRECT.T + mMeterHeight + mChNameMaxH * mText.mSize;
    mTargetRECT = mRECT;
    }

  void SetDrawLevelMarks(bool draw, int chId = -1) {
    // todo dont forget aboul mrect recalcs
    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *DrawMarksPtr(ch) = draw;
    else
        *DrawMarksPtr(chId) = draw;
    }
  void SetLevelMarks(const char* marks, int chId = -1) {
    // provide values separated by spaces.
    // integers is ok,
    // omitting zeroes is ok,
    // scientific notation is ok.
    // if you set a meter to dB scale, values should be in dB too.
    // if you want the numerical value of the mark to be drawn,
    // put 's' right after the value.
    // example for dB scale: "3.0 2 1. .s -1e0 -2 -3 -6s"
    // yields marks on 3, 2, 1, 0, -1, -2, -3 and -6 dB levels.
    // only 0 and -6 marks will show a number next to a mark.

    WDL_TypedBuf<double> markVals;
    WDL_TypedBuf<bool> markLabels;

    while (*marks != '\0') {
      double v = strtod(marks, NULL);
      if ((chId < 0 && DisplayDB(0)) || (chId > -1 && DisplayDB(chId)))
        v = DBToAmp(v);

      while (*marks != ' ' && *marks != '\0')
        ++marks;

      bool l = false;
      if (*(marks - 1) == 's')
        l = true;

      markVals.Add(v);
      markLabels.Add(l);

      while (!(*marks == '-' || *marks == '+' || *marks == '.' || isdigit(*marks))
             && *marks != '\0')
      // ^^^ the !(...) part is a simple typo filter.
      // condition if(*marks != '\0') could be used instead
        ++marks;
      }

#ifdef _DEBUG
    WDL_String parsed("parsed marks: ");
    for (int v = 0; v != markVals.GetSize(); ++v)
      parsed.AppendFormatted(8, "%3.2f, ", *(markVals.Get() + v));
    DBGMSG(parsed.Get());
#endif

    auto Set = [&] (int ch) {
      auto n = markVals.GetSize();
      MarkLabelsPtr(ch)->Resize(0);
      MarksPtr(ch)->Resize(0);

      for (int i = 0; i != n; ++i) {
        MarksPtr(ch)->Add(*(markVals.Get() + i));
        MarkLabelsPtr(ch)->Add(*(markLabels.Get() + i));
        }
      };

    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        Set(ch);
    else
        Set(chId);

#ifdef _DEBUG
    WDL_String wr("written marks: ");
    for (int v = 0; v != MarksPtr(0)->GetSize(); ++v)
      wr.AppendFormatted(8, "%3.2f, ", *(MarksPtr(0)->Get() + v));
    DBGMSG(wr.Get());
#endif

    }
  void SetMarkWidthRatio(float r, int chId) {
    // relative to meter width
    // todo dont forget aboul mrect recalcs
     if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *MarkWidthRPtr(ch) = r;
    else
        *MarkWidthRPtr(chId) = r;

    }

  //todo dont forget about mRect recalc
  void SetText(IText& txt) {
    SetTexts(txt, txt);
    }
  void SetTexts(IText chNameTxt, IText marksTxt) {
    mText = chNameTxt;
    mMarkText = marksTxt;
    SetDirty();
    }

  void SetOverdriveThreshold(double thresh, int chId = -1) {
    auto Set = [this] (int i, double t) {
      if (DisplayDB(i))
        t = DBToAmp(i);
      else if (t < 0.0)
        t *= -1.0;
      *OverThreshPtr(i) = t;
      };

    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        Set(ch, thresh);
    else
      Set(chId, thresh);

    }
  void SetHoldPeaks(bool hold) { mHoldPeaks = hold; }
  void SetPeakDropTimeMs(double ms, int chId = -1) {
    ms = abs(ms);

    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *DropMsPtr(ch) = ms;
    else
        *DropMsPtr(chId) = ms;
    }
  void SetDrawPeakRect(bool draw, int chId = -1) {
    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch) {
        *DrawPeakRectPtr(ch) = draw;
        *HoldingAPeakPtr(ch) = false;
        if (!draw) *OverBlinkPtr(ch) = 0.0;
        }
    else {
      *DrawPeakRectPtr(chId) = draw;
      *HoldingAPeakPtr(chId) = false;
      if (!draw) *OverBlinkPtr(chId) = 0.0;
      }
    SetDirty();
    }
  // true peak is drawn in peak rect, so !DrawPeakRect == !DrawTruePeak
  void SetDrawTruePeak(bool draw, int chId = -1) {
    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *DrawTruePeakPtr(ch) = draw;
    else
      *DrawTruePeakPtr(chId) = draw;
    SetDirty();
    }
  void SetPeakRectHeight(double h) {
    mPeakRectHeight = (float) h;
    SetDirty();
    }

  void SetDrawMemRect(bool show, int chId = -1) {
    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *DrawMemRectPtr(ch) = show;
    else
      *DrawMemRectPtr(chId) = show;
    SetDirty();
    }

  void SetMeterWidth(double w, int chId = -1) {
    if (w < 0.0) w = 20.0;
    w = trunc(w); // identical narrow meters with noninteger sizes often look different
    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *MeterWidthPtr(ch) = (float) w;
    else
      *MeterWidthPtr(chId) = (float) w;

    mRECT = mTargetRECT = GetControlRectFromChannelsData();

    SetDirty();
    }
  void SetDistToTheNextMeter(double d, int chId = -1, bool glueRects = true) {
    // todo calc sum of truncated tales and distribute it
    // in a nice way to keep the mRECT.W when needed
    if (d < 0.0) d = 30.0;
    glueRects = glueRects && (d == 0.0);
    // identical narrow meters with noninteger sizes often look different
    d = trunc(d);

    if (chId < 0) {
      if (glueRects) {
        float w = 0.0;
        for (int ch = 0; ch != NumChannels() - 1; ++ch)
          w += DistToTheNextM(ch);
        w /= NumChannels();
        for (int ch = 0; ch != NumChannels(); ++ch) {
          *MeterWidthPtr(ch) += w;
          *MeterWidthPtr(ch) = trunc(MeterWidth(ch));
          }
        }
      for (int ch = 0; ch != NumChannels(); ++ch)
        *DistToTheNextMPtr(ch) = (float) d;
      }
    else {
      if (glueRects && chId != NumChannels() - 1) {
        auto stretch = 0.5f * DistToTheNextM(chId);
        *MeterWidthPtr(chId) += stretch;
        *MeterWidthPtr(chId) = trunc(MeterWidth(chId));
        *MeterWidthPtr(chId + 1) += stretch;
        *MeterWidthPtr(chId + 1) = trunc(MeterWidth(chId + 1));
        }
      *DistToTheNextMPtr(chId) = (float)d;
      }

    mRECT = mTargetRECT = GetControlRectFromChannelsData();

#ifdef _DEBUG
    WDL_String s("meters widths: ");
    for (int m = 0; m != NumChannels(); ++m)
      s.AppendFormatted(64, "%2.2f, ", MeterWidth(m));
    DBGMSG(s.Get());
#endif


    SetDirty();
    }

  void Draw(IGraphics& graphics)  override;
  void SetDirty(bool pushParamToDelegate = false) override {
    mDirty = true;
    }
  void OnResize() {
    // todo
    // dont forget to trunc the horisontal distances
    // scale channel name offsets accordingly
    SetDirty();
    }

  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override {
    for (int ch = 0; ch != NumChannels(); ++ch) {
      *HoldingAPeakPtr(ch) = false;
      *TruePickPtr(ch) = RawValue(ch);
      }
    SetDirty();
    }
  void OnMouseDown(float x, float y, const IMouseMod& mod) override {
    // todo : doesnt react during peakhold time
    for (int ch = 0; ch != NumChannels(); ++ch)
      if (GetMeterRect(ch, true).Contains(x, y)) {
        *HoldingAPeakPtr(ch) = false;
        *TruePickPtr(ch) = RawValue(ch);
        break;
        }
    SetDirty();
    }

  protected:

  double mSampleRate = DEFAULT_SAMPLE_RATE;

  struct ChannelSpecificData
    {
  // all signal related vals are stored as abs vals and interpreted when needed
    double rawValue;
    double minDisplayVal = 0.001; // -60dB
    double maxDisplayVal = DBToAmp(3.0);
    bool displayDB = true;

    double truePick = 0.0;
    double memPeak = 0.0; // max remembered value for memRect
    double memExp = 1.0; // decay exponent for memPeak
    size_t peakSampHeld = 0;
    double dropMs = 2000.0;
    double overThresh = 1.0;
    double overBlink = 0.0; // overdrive rect intensity
    bool holdingAPeak = false;

    float meterWidth = 20.f;
    float distToNextM = 30.f; // use for gluing chans into groups like ins and outs

    bool drawMemRect = true;
    bool drawPeakRect = true;
    bool drawTruePeak = true;

    bool drawMarks = true;
    float markWidthR = 0.6f;
    WDL_TypedBuf<double>* marks = nullptr;
    WDL_TypedBuf<bool>* markLabels = nullptr;

    bool drawChanName = true; // todo
    float chanNameHOffset = 0.f; // e.g. one "IN L/R" label for glued meters
    WDL_String* chanName = nullptr;
    };

  WDL_TypedBuf<ChannelSpecificData> mChanData;

  int NumChannels() { return mChanData.GetSize(); }

  float mMeterHeight = 150.f; // including the overdrive rect
  float mPeakRectHeight = 15.f;
  bool mHoldPeaks = true; // useful in audio debugging

  // todo:
  float mChNameMaxH = 1.f; // these three used for stretching mRECT to fit labels
  float mChNameLeftMargin = 0.f;
  float mChNameRightMargin = 0.f;

  IText mMarkText = mText;

  bool mDrawShadows = true;
  float mShadowOffset = 3.f;
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

  float GetVCoordFromValInMeterRect(int chId, double v, IRECT meterR) {
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
    mr.B = mr.T + mMeterHeight;
    if (DrawPeakRect(i) && !withODRect)
      mr.T += mPeakRectHeight;
    return mr;
    }
  IRECT GetControlRectFromChannelsData(bool keepPosition = true) {
    // todo account for labels
    float w = 0.0;
    bool showChName = false;
    for (int m = 0; m != NumChannels(); ++m) {
      w += MeterWidth(m);
      if (m < NumChannels() - 1)
        w += DistToTheNextM(m);
      if (DrawChanName(m))
        showChName = true;
      }
    auto r = IRECT(0.0, 0.0, w, mMeterHeight);
    if (showChName)
      r.B += mChNameMaxH * mText.mSize;
    if (keepPosition) {
      r.L += mRECT.L;
      r.R += mRECT.L;
      r.T += mRECT.T;
      r.B += mRECT.T;
      }
    return r;
    }

  void SetChanNames(const char* names, va_list args) {
    if (NumChannels() < 1) return;
    ChanNamePtr(0)->Set(names);
    for (int c = 1; c < NumChannels(); ++c)
      ChanNamePtr(c)->Set(va_arg(args, const char*));
    }

  void DrawMarks(IGraphics& graphics) {
    auto shadowColor = IColor(60, 0, 0, 0);
      // todo add alignment and offset
    for (auto ch = 0; ch != NumChannels(); ++ch) {
      if (DrawMarks(ch)) {
        auto mR = GetMeterRect(ch);
        for (int m = 0; m != MarksPtr(ch)->GetSize(); ++m) {
          auto v = Mark(ch, m);
          if (v > MinDisplayVal(ch) && v < MaxDisplayVal(ch)) {
            auto h = GetVCoordFromValInMeterRect(ch, v, mR);
            h = trunc(h); // NB at least on LICE nonintegers look bad
            if (DisplayDB(ch)) v = AmpToDB(v);
            if (MarkLabel(ch, m)) {
              auto tr = mR;
              tr.T = tr.B = h - mMarkText.mSize / 2;
              WDL_String l;
              // todo: make precision a member
              l.SetFormatted(8, "%2.1f", v);
              if (mDrawShadows) {
                auto tt = mMarkText;
                tt.mFGColor = shadowColor;
                auto sr = ShiftRectBy(tr, 1.0, 1.0);
                graphics.DrawTextA(tt, l.Get(), sr);
                }
              graphics.DrawTextA(mMarkText, l.Get(), tr);
              }
            else {
              float x2 = mR.L + 1.f + MarkWidthR(ch) * mR.W();
              if (mDrawShadows) {
                auto sr = mR;
                sr.T = h;
                sr.B = h + 1.f;
                sr.R = x2;
                sr = ShiftRectBy(sr, 1.0, 1.0);
                graphics.FillRect(shadowColor, sr);
                }
              graphics.DrawLine(mMarkText.mFGColor, mR.L + 1.f, h, x2, h);
              }
            }
          }
        }
      }
    }

  IColor LinearBlendColors(IColor cA, IColor cB, double mix) {
    IColor cM;
    cM.A = (int) ((1.0 - mix) * cA.A + mix * cB.A);
    cM.R = (int) ((1.0 - mix) * cA.R + mix * cB.R);
    cM.G = (int) ((1.0 - mix) * cA.G + mix * cB.G);
    cM.B = (int) ((1.0 - mix) * cA.B + mix * cB.B);
    return cM;
    }
  IRECT ShiftRectBy(IRECT r, float dx, float dy = 0.0) {
    auto sr = r;
    sr.L += dx;
    sr.R += dx;
    sr.T += dy;
    sr.B += dy;
    return sr;
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

  // getters
  auto Ch(int i) { return mChanData.Get() + i; }

  double* RawValuePtr       (int i) { return &(Ch(i)->rawValue); }
  double RawValue           (int i) { return *RawValuePtr(i); }
  double* MinDisplayValPtr  (int i) { return &(Ch(i)->minDisplayVal); }
  double MinDisplayVal      (int i) { return *MinDisplayValPtr(i); }
  double* MaxDisplayValPtr  (int i) { return &(Ch(i)->maxDisplayVal); }
  double MaxDisplayVal      (int i) { return *MaxDisplayValPtr(i); }

  double* OverThreshPtr     (int i) { return &(Ch(i)->overThresh); }
  double OverThresh         (int i) { return *OverThreshPtr(i); }

  bool* DisplayDBPtr        (int i) { return &(Ch(i)->displayDB); }
  bool DisplayDB            (int i) { return *DisplayDBPtr(i); }

  double* DropMsPtr         (int i) { return &(Ch(i)->dropMs); }
  double DropMs             (int i) { return *DropMsPtr(i); }

  bool* HoldingAPeakPtr     (int i) { return &(Ch(i)->holdingAPeak); }
  bool HoldingAPeak         (int i) { return *HoldingAPeakPtr(i); }
  size_t* PeakSampHeldPtr   (int i) { return &(Ch(i)->peakSampHeld); }
  size_t PeakSampHeld       (int i) { return *PeakSampHeldPtr(i); }

  double* OverBlinkPtr      (int i) { return &(Ch(i)->overBlink); }
  double OverBlink          (int i) { return *OverBlinkPtr(i); }

  bool* DrawPeakRectPtr     (int i) { return &(Ch(i)->drawPeakRect); }
  bool DrawPeakRect         (int i) { return *DrawPeakRectPtr(i); }
  bool* DrawTruePeakPtr     (int i) { return &(Ch(i)->drawTruePeak); }
  bool DrawTruePeak         (int i) { return *DrawTruePeakPtr(i); }
  double* TruePickPtr       (int i) { return &(Ch(i)->truePick); }
  double TruePeak           (int i) { return *TruePickPtr(i); }

  bool* DrawMemRectPtr      (int i) { return &(Ch(i)->drawMemRect); }
  bool DrawMemRect          (int i) { return *DrawMemRectPtr(i); }
  double* MemPeakPtr        (int i) { return &(Ch(i)->memPeak); }
  double MemPeak            (int i) { return *MemPeakPtr(i); }
  double* MemExpPtr         (int i) { return &(Ch(i)->memExp); }
  double MemExp             (int i) { return *MemExpPtr(i); }

  float* MeterWidthPtr      (int i) { return &(Ch(i)->meterWidth); }
  float MeterWidth          (int i) { return *MeterWidthPtr(i); }
  float* DistToTheNextMPtr  (int i) { return &(Ch(i)->distToNextM); }
  float DistToTheNextM      (int i) { return *DistToTheNextMPtr(i); }

  bool* DrawMarksPtr        (int i) { return &(Ch(i)->drawMarks); }
  bool DrawMarks            (int i) { return *DrawMarksPtr(i); }
  float* MarkWidthRPtr      (int i) { return &(Ch(i)->markWidthR); }
  float MarkWidthR          (int i) { return *MarkWidthRPtr(i); }

  double Mark         (int ch, int i) { return *(MarksPtr(ch)->Get() + i); }
  WDL_TypedBuf<double>*  MarksPtr     (int i) { return *MarksPP(i); }
  WDL_TypedBuf<double>** MarksPP      (int i) { return &(Ch(i)->marks); }

  bool MarkLabel      (int ch, int i) { return *(MarkLabelsPtr(ch)->Get() + i); }
  WDL_TypedBuf<bool>*  MarkLabelsPtr  (int i) { return *MarkLabelsPP(i); }
  WDL_TypedBuf<bool>** MarkLabelsPP   (int i) { return &(Ch(i)->markLabels); }

  bool* DrawChanNamePtr     (int i) { return &(Ch(i)->drawChanName); }
  bool DrawChanName         (int i) { return *DrawChanNamePtr(i); }
  float* ChanNameHOffsetPtr (int i) { return &(Ch(i)->chanNameHOffset); }
  float ChanNameHOffset     (int i) { return *ChanNameHOffsetPtr(i); }

  WDL_String* ChanNamePtr   (int i) { return *ChanNamePP(i); }
  WDL_String** ChanNamePP   (int i) { return &(Ch(i)->chanName); }

  };