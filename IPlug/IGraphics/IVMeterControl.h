#pragma once

#include "IControl.h"

/*
Vector meter control by Eugene Yakshin, 2018
Eugene.Yakshin [at] ya [dot] ru
*/

class IVMeterControl : public IControl
  , public IVectorBase {
  public:
  static const IColor DEFAULT_BG_COLOR;
  static const IColor DEFAULT_RAW_COLOR;
  static const IColor DEFAULT_RMS_COLOR;
  static const IColor DEFAULT_PK_COLOR;
  static const IColor DEFAULT_FR_COLOR;

  // map to IVectorBase colors
  enum EVKColor {
    mBg = kBG,
    mRaw = kFG,
    mPeak = kHL,
    mFr = kFR,
    mRms = kX1
    };

  IVMeterControl(IDelegate& dlg, IRECT rect, int numChannels, const char* chanNames, ...);

  ~IVMeterControl() {
    for (auto c = 0; c != NumChannels(); ++c) {
      delete(RMSBufPtr(c));
      delete(ChanNamePtr(c));
      delete(MarksPtr(c));
      delete(MarkLabelsPtr(c));
      }
    }

// todo add thread safe channel add/del.

  void SetSampleRate(double sr) {
    if (mSampleRate == sr || sr <= 0.0) return;
    mSampleRate = sr;
    for (int ch = 0; ch != NumChannels(); ++ch) {
      RMSBufPtr(ch)->Resize(int(0.001 * RMSWindowMs(ch) * mSampleRate));
      ZeroRMSBuf(ch);
      }
    }
  void ResetRMS(int chId = -1) {
    if (chId < 0)
      for (auto ch = 0; ch != NumChannels(); ++ch)
        ZeroRMSBuf(ch);
    else
      ZeroRMSBuf(chId);
    SetDirty();
    }
  void Reset(int chId = -1) {
    // if you hack up too much
    auto R = [&] (int ch) {
      *RawValuePtr(ch) = 0.0;

      *MaxPeakPtr(ch) = 0.0;
      *HoldingAPeakPtr(ch) = false;
      *PeakSampHeldPtr(ch) = 0;
      *OverBlinkPtr(ch) = 0.0;

      *MemPeakPtr(ch) = 0.0;
      *MemExpPtr(ch) = 1.0;

      ZeroRMSBuf(ch);
      *RMSSumPtr(ch) = 0.0;
      };

    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        R(ch);
    else
      R(chId);
    SetDirty();
    }

  // use for raw data
  void ProcessChanValue(double value, int chId = -1) {
    value = abs(value);

    auto Set = [this] (int i, double v) {
      if (v > RawValue(i))
        *RawValuePtr(i) = v; // it makes sense to show max value that was gained between the Draw() calls.
                             // in Draw then the value is zeroed right after reading.
                             // it's not 100% thread safe, but in this case it doesn't matter.

      if (DrawRMS(i)) {
        auto s = v * v;
        auto pos = RMSBufPos(i);
        auto oldS = RMSBufVal(i, pos);
        *RMSBufValPtr(i, pos) = s;
        *RMSSumPtr(i) += s - oldS;
        ++pos;
        if (pos >= RMSBufLen(i)) pos = 0;
        *RMSBufPosPtr(i) = pos;
        }

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
  void ProcessBus(sample** ins, int blockSize, int numChans = -1, int sourceStartId = 0, int destStartId = 0) {
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

  auto GetRMS(int chId) {
    auto rms = RMSSum(chId) / RMSBufLen(chId);
    if (AESFix(chId))
      rms *= 2.0;
    rms = sqrt(rms);
    return rms;
    }
  auto GetMaxPeak(int chId) {
    return MaxPeak(chId);
    }

  void SetUnitsDB(bool db, int chId = -1, bool scaleFollowsUnits = true) {
    auto initLM = GetLeftMarginAbs();  // here and further: such margins recalculations and compensation
    auto initRM = GetRightMarginAbs(); // are necessary for correctd drawing.
                                       // hence it must appear in any setter that may affect the margins:
                                       // font sizes, units, ranges, preision, etc.

    if (chId < 0) {
      for (int ch = 0; ch != NumChannels(); ++ch)
        *UnitsDBPtr(ch) = db;
      if (scaleFollowsUnits)
        for (int ch = 0; ch != NumChannels(); ++ch)
          *ScaleLogPtr(ch) = db;
      for (int ch = 0; ch != NumChannels(); ++ch)
        RecalcMaxLabelLen(ch);
      }
    else {
      *UnitsDBPtr(chId) = db;
      if (scaleFollowsUnits)
        *ScaleLogPtr(chId) = db;
      RecalcMaxLabelLen(chId);
      }

    UpdateMargins(initLM, initRM);

    SetDirty();
    }
  void SetScaleLog(bool logScale, int chId = -1) {
    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *ScaleLogPtr(ch) = logScale;
    else
      *ScaleLogPtr(chId) = logScale;

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
      if (UnitsDB(i)) {
        *MaxDisplayValPtr(i) = DBToAmp(max);
        *MinDisplayValPtr(i) = DBToAmp(min);
        }
      else {
        *MinDisplayValPtr(i) = abs(min);
        *MaxDisplayValPtr(i) = abs(max);
        }
      };

    auto initLM = GetLeftMarginAbs();
    auto initRM = GetRightMarginAbs();

    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch) {
        Set(ch, min, max);
        RecalcMaxLabelLen(ch);
        }
    else {
      Set(chId, min, max);
      RecalcMaxLabelLen(chId);
      }

    UpdateMargins(initLM, initRM);

    SetDirty();
    }
  void SetNumDisplayPrecision(int precision, int chId = -1) {
    auto initLM = GetLeftMarginAbs();
    auto initRM = GetRightMarginAbs();

    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch) {
        *NumDisplPrecisionPtr(ch) = precision;
        RecalcMaxLabelLen(ch);
        }
    else {
      *NumDisplPrecisionPtr(chId) = precision;
      RecalcMaxLabelLen(chId);
      }

    UpdateMargins(initLM, initRM);

    SetDirty();
    }

  void SetOverdriveThreshold(double thresh, int chId = -1) {
    auto Set = [this] (int i, double t) {
      if (UnitsDB(i))
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
  void SetDrawMaxPeak(bool draw, int chId = -1) {
    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *DrawMaxPeakPtr(ch) = draw;
    else
      *DrawMaxPeakPtr(chId) = draw;
    SetDirty();
    }
  void SetPeakRectHeight(double h) {
    mPeakRectHeight = (float) h;
    SetDirty();
    }

  void SetDrawMemRect(bool draw, int chId = -1) {
    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *DrawMemRectPtr(ch) = draw;
    else
      *DrawMemRectPtr(chId) = draw;
    SetDirty();
    }

  void SetDrawRMS(bool draw, int chId = -1) {
    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch) {
        if (draw != DrawRMS(ch))
          ZeroRMSBuf(ch);
        *DrawRMSPtr(ch) = draw;
        }
    else {
      if (draw != DrawRMS(chId))
        ZeroRMSBuf(chId);
      *DrawRMSPtr(chId) = draw;
      }
    SetDirty();
    }
  void SetRMSWindowMs(double ms, int chId = -1) {
    if (ms <= 0.0) return;
    if (0.001 * ms * mSampleRate < 10.0) // window at least 10 samples long
      ms = 10000.0 / mSampleRate;

    auto s = int(0.001 * ms * mSampleRate);
    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch) {
        RMSBufPtr(ch)->Resize(s);
        // todo check Resize() implementation, maybe zero only the new allocations
        ZeroRMSBuf(ch);
        if (RMSBufPos(ch) >= s)
          *RMSBufPosPtr(ch) = 0;
        }
    else {
      RMSBufPtr(chId)->Resize(s);
      ZeroRMSBuf(chId);
      if (RMSBufPos(chId) >= s)
        *RMSBufPosPtr(chId) = 0;
      }
    }
  void SetAesRmsFix(bool plusThreeDB, int chId = -1) {
    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *AESFixPtr(ch) = plusThreeDB;
    else
      *AESFixPtr(chId) = plusThreeDB;
    }

  void SetDrawChName(bool draw, int chId = -1) {
    auto initH = mChNameMaxH * mText.mSize;

    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *DrawChanNamePtr(ch) = draw;
    else
      *DrawChanNamePtr(chId) = draw;

    RecalcMaxChNameH(initH);

    SetDirty();
    }
  void SetChanName(int chId, const char* name) {
    auto initH = mChNameMaxH * mText.mSize;
    if (chId < NumChannels())
      ChanNamePtr(chId)->Set(name);
    RecalcMaxChNameH(initH);
    SetDirty();
    }
  void SetChanNames(const char* names, ...) {
    auto initH = mChNameMaxH * mText.mSize;
    va_list args;
    va_start(args, names);
    SetChanNames(names, args);
    va_end(args);

    RecalcMaxChNameH(initH);
    SetDirty();
    }
  void SetChNameHOffset(int chId, double offset) {
    *ChanNameHOffsetPtr(chId) = (float) offset;
    SetDirty();
    }

  void SetDrawLevelMarks(bool draw, int chId = -1) {
    auto initLM = GetLeftMarginAbs();
    auto initRM = GetRightMarginAbs();

    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *DrawMarksPtr(ch) = draw;
    else
      *DrawMarksPtr(chId) = draw;

    UpdateMargins(initLM, initRM);

    SetDirty();
    }
  void SetLevelMarks(const char* marks, int chId = -1) {
    // provide values separated by spaces.
    // integers are ok,
    // omitting zeroes is ok,
    // scientific notation is ok.
    // if a meter's units are dB, values should be in dB too.
    // if you want the numerical value of the mark to be drawn
    // put 's' right after the value.
    // example for dB scale: "3.0 2 1. .s -1e0 -2 -3 -6s"
    // yields marks on 3, 2, 1, 0, -1, -2, -3 and -6 dB levels.
    // only 0 and -6 marks will show a number next to a mark.

    WDL_TypedBuf<double> markVals;
    WDL_TypedBuf<bool> markLabels;

    while (*marks != '\0') {
      double v = strtod(marks, NULL);
      if ((chId < 0 && UnitsDB(0)) || (chId > -1 && UnitsDB(chId)))
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
      // ^^^ the !(...) part is a simple typo filter. also allows for more spaces for readability
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

    auto initLM = GetLeftMarginAbs();
    auto initRM = GetRightMarginAbs();

    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch) {
        Set(ch);
        RecalcMaxLabelLen(ch);
        }
    else {
      Set(chId);
      RecalcMaxLabelLen(chId);
      }

#ifdef _DEBUG
    WDL_String wr("written marks: ");
    for (int v = 0; v != MarksPtr(0)->GetSize(); ++v)
      wr.AppendFormatted(8, "%3.2f, ", *(MarksPtr(0)->Get() + v));
    DBGMSG(wr.Get());
#endif

    UpdateMargins(initLM, initRM);

    SetDirty();
    }
  void SetMarkWidthRatio(float r, int chId) {
    // relative to meter width.
    // marks with no numeric values will be 0.5 * r * meterWidth

    auto initLM = GetLeftMarginAbs();
    auto initRM = GetRightMarginAbs();

    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *MarkWidthRPtr(ch) = r;
    else
      *MarkWidthRPtr(chId) = r;

    UpdateMargins(initLM, initRM);

    SetDirty();
    }
  void SetMarksAlignment(char alignment, int chId = -1, bool shortenMarksForSides = true) {
    if (alignment != 'l' && alignment != 'r')
      alignment = 'c';

    auto initLM = GetLeftMarginAbs();
    auto initRM = GetRightMarginAbs();

    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *MarksAlignPtr(ch) = alignment;
    else
      *MarksAlignPtr(chId) = alignment;

    if (shortenMarksForSides && alignment != 'c')
      if (chId < 0)
        for (int ch = 0; ch != NumChannels(); ++ch)
          *MarkWidthRPtr(ch) *= 0.3f;
      else
        *MarkWidthRPtr(chId) *= 0.3f;

    UpdateMargins(initLM, initRM);

    SetDirty();
    }
  void SetMarksHOffset(float offset, int chId = -1) {
    // offset is mainly used for drawing one set of marks between the adjacent meters.
    // setting high offset values outside the meter area doesn't guarantee correct drawing.
    if (chId < 0)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *MarksOffsetPtr(ch) = offset;
    else
      *MarksOffsetPtr(chId) = offset;

  //todo try to calc mARGINS TOO

    SetDirty();
    }

  void SetText(IText& txt) {
    SetTexts(txt, txt);
    }
  void SetTexts(IText chNameTxt, IText marksTxt) {
    // note that the color of marks' lines is mMarkText.mTextEntryBGColor,
    // whereas the color of marks' values is mMarkText.mFGColor,
    // so you can make them different.
    auto initLM = GetLeftMarginAbs();
    auto initRM = GetRightMarginAbs();
    auto initH = mChNameMaxH * mText.mSize;

    mText = chNameTxt;
    mMarkText = marksTxt;

    UpdateMargins(initLM, initRM);
    RecalcMaxChNameH(initH);
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
  void SetDistToTheNextMeter(double d, int chId = -1, bool compactLabels = true, bool glueRects = true) {
    if (d < 0.0) d = 30.0;
    glueRects = glueRects && (d == 0.0);
    d = round(d);

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
      if (compactLabels) {
        // only put marks in the middle, don't touch chan names
        for (int ch = 0; ch != NumChannels() - 1; ++ch) {
          *MarksAlignPtr(ch) = 'c';
          *MarksOffsetPtr(ch) = 0.5f * (MeterWidth(ch) + DistToTheNextM(ch));
          }
        *DrawMarksPtr(NumChannels()) = false;
        }
      }
    else {
      if (glueRects && chId != NumChannels() - 1) {
        auto stretch = 0.5f * DistToTheNextM(chId);
        *MeterWidthPtr(chId) += stretch;
        *MeterWidthPtr(chId) = trunc(MeterWidth(chId));
        *MeterWidthPtr(chId + 1) += stretch;
        *MeterWidthPtr(chId + 1) = trunc(MeterWidth(chId + 1));
        }

      *DistToTheNextMPtr(chId) = (float) d;

      if (compactLabels && chId < NumChannels() - 1) {
        // put 1st marks and chanName in the center, hide 2nd ones
        *DrawMarksPtr(chId) = true;
        *DrawMarksPtr(chId + 1) = false;
        *MarksAlignPtr(chId) = 'c';
        auto o = 0.5f * (MeterWidth(chId) + DistToTheNextM(chId));
        *MarksOffsetPtr(chId) = o;
        if (ChanNamePtr(chId)->GetLength()) {
          *DrawChanNamePtr(chId) = true;
          *DrawChanNamePtr(chId + 1) = false;
          *ChanNameHOffsetPtr(chId) = o;
          }
        else if (ChanNamePtr(chId + 1)->GetLength()) {
          *DrawChanNamePtr(chId) = false;
          *DrawChanNamePtr(chId + 1) = true;
          auto o = -0.5f * (MeterWidth(chId + 1) + DistToTheNextM(chId));
          *ChanNameHOffsetPtr(chId + 1) = o;
          }
        }
      }

    mRECT = mTargetRECT = GetControlRectFromChannelsData();

#ifdef _DEBUG
    WDL_String s("meters widths: ");
    for (int m = 0; m != NumChannels(); ++m)
    s.AppendFormatted(64, "%d, ", (int) MeterWidth(m));
    DBGMSG(s.Get());
#endif

    SetDirty();
    }

  void Draw(IGraphics& graphics)  override;
  void SetDirty(bool pushParamToDelegate = false) override {
    mDirty = true;
    }
  void OnResize() {
    auto initR = GetControlRectFromChannelsData();

    auto chNH = mChNameMaxH * mText.mSize;
    auto labW = 0.f;
    // labW represents nonscalable left and right margins
    // left margin is especially important because it affects the meter rects offset
    if (DrawMarks(0) && MarksAlign(0) == 'l')
      labW += MaxLabelLen(0);
    if (NumChannels() > 1) {
      int last = NumChannels() - 1;
      if (DrawMarks(last) && MarksAlign(last) == 'r')
        labW += MaxLabelLen(last);
      }

    labW *= mMarkText.mSize * 0.44f;

    auto minH = 30.f + chNH;
    auto minW = 4.f * NumChannels() + labW;

    if (mRECT.H() < minH) mRECT.B = mRECT.T + minH;
    if (mRECT.W() < minW) mRECT.R = mRECT.L + minW;
    mTargetRECT = mRECT;

    auto userFlexArea = mRECT.W() - labW;
    auto initFlexArea = initR.W() - labW;

    auto wr = userFlexArea / initFlexArea;

      // jerky resizing makes problems sometimes
    if (wr < 0.125f) {
      mTargetRECT = mRECT = initR;
      SetDirty();
      return;
      }
    else if (wr > 8.f) wr = 8.f;

#ifdef _DEBUG
    if (wr != 1.f) {
      WDL_String rs("\n OnResize r: ");
      rs.AppendFormatted(32, "%4.6f, ", wr);
      DBGMSG(rs.Get());
      }
#endif

    Scale(wr);
    mMeterHeight = mRECT.H() - chNH;

#ifdef _DEBUG
    // check the leftovers, shouldn't be big
    auto nR = GetControlRectFromChannelsData();
    auto ex = mRECT.W() - nR.W();

    WDL_String ls("\n OnResize left: ");
    ls.AppendFormatted(32, "%4.6f, ", ex);
    DBGMSG(ls.Get());
#endif

    SetDirty();
    }



  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override {
    if (mod.C)
      Reset();
    else
      for (int ch = 0; ch != NumChannels(); ++ch) {
        *HoldingAPeakPtr(ch) = false;
        *MaxPeakPtr(ch) = RawValue(ch);
        }
    SetDirty();
    }
  void OnMouseDown(float x, float y, const IMouseMod& mod) override {
    for (int ch = 0; ch != NumChannels(); ++ch)
      if (GetMeterRect(ch, true).Contains(x, y)) {
        *HoldingAPeakPtr(ch) = false;
        *MaxPeakPtr(ch) = RawValue(ch);
        break;
        }
    SetDirty();
    }

  protected:

  double mSampleRate = DEFAULT_SAMPLE_RATE;

  struct ChannelSpecificData {
  // all signal related vals are stored as abs vals and interpreted when needed
    double rawValue;
    double minDisplayVal = 0.001; // -60dB
    double maxDisplayVal = DBToAmp(3.0);
    bool unitsDB = true;
    bool scaleLog = true;

    double maxPeak = 0.0;
    double memPeak = 0.0; // max remembered value for memRect
    double memExp = 1.0; // decay exponent for memPeak
    size_t peakSampHeld = 0;
    double dropMs = 2000.0;
    double overThresh = 1.0;
    double overBlink = 0.0; // overdrive rect intensity
    bool holdingAPeak = false;

    bool drawRMS = true;
    bool AESFix = true;
    // AES17 requires X dB RMS for a pure sine peaking at X dB.
    // AES RMS is just a reference;
    // But scientific RMS of sine is X/sqrt(2) dB.
    double RMSWindowMs = 300.0;
    WDL_TypedBuf<double>* RMSBuf = nullptr;
    size_t RMSBufPos = 0; // position in the buffer
    double RMSSum = 0.0;

    float meterWidth = 20.f;
    float distToNextM = 30.f; // use for gluing chans into groups like ins and outs

    bool drawMemRect = true;
    bool drawPeakRect = true;
    bool drawMaxPeak = true;

    bool drawMarks = true;
    float markWidthR = 0.6f;
    WDL_TypedBuf<double>* marks = nullptr;
    WDL_TypedBuf<bool>* markLabels = nullptr;
    char marksAlign = 'c'; // 'l'eft, 'c'enter, 'r'ight
    float marksOffset = 0.f; // used for common marks for adjacent meters. using ouside the meters area is not perfect with OnResize()
    float maxLabelLen = 0.f;

    int numDisplPrecision = 2;
    bool drawChanName = true;
    float chanNameHOffset = 0.f; // e.g. one "IN L/R" label for glued meters
    WDL_String* chanName = nullptr;
    };

  WDL_TypedBuf<ChannelSpecificData> mChanData;

  int NumChannels() { return mChanData.GetSize(); }

  float mMeterHeight = 150.f; // including the peak rect
  float mPeakRectHeight = 15.f;
  bool mHoldPeaks = true; // useful in audio debugging

  float mChNameMaxH = 0.f;

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

  void ZeroRMSBuf(int ch) {
    for (int i = 0; i != RMSBufLen(ch); ++i)
      *RMSBufValPtr(ch, i) = 0.0;
    }

  float GetVCoordFromValInMeterRect(int chId, double v, IRECT meterR) {
    auto t = meterR.B;
    if (v > MaxDisplayVal(chId))
      t = meterR.T;
    else if (v > MinDisplayVal(chId)) {
      auto r = 0.0;
      if (ScaleLog(chId)) {
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
    auto dx = 0.f;
    dx += GetLeftMarginAbs();
    // identical narrow meters with noninteger sizes often look different and sometimes ugly, so use round().
    for (int m = 0; m != i; ++m) {
      dx += round(MeterWidth(m));
      dx += round(DistToTheNextM(m));
      }
    auto mr = mRECT;
    mr.L += dx;
    mr.R = mr.L + round(MeterWidth(i));
    mr.B = mr.T + mMeterHeight;
    if (DrawPeakRect(i) && !withODRect)
      mr.T += mPeakRectHeight;
    return mr;
    }
  IRECT GetControlRectFromChannelsData(bool keepPosition = true) {
    float w = 0.0;

    w += GetLeftMarginAbs() + GetRightMarginAbs();

    bool showChName = false;
    for (int m = 0; m != NumChannels(); ++m) {
      w += MeterWidth(m);
      if (m < NumChannels() - 1)
        w += DistToTheNextM(m);
      if (DrawChanName(m))
        showChName = true;
      }

    auto r = IRECT(0.0, 0.0, w, mMeterHeight);
    if (showChName) // simple scheme, don't care about the chName width
      r.B += mChNameMaxH * mText.mSize;
    if (keepPosition)
      r = ShiftRectBy(r, mRECT.L, mRECT.T);
    return r;
    }

  float GetLeftMarginAbs() {
    auto m = 0.f;
    if (DrawMarks(0) && MarksAlign(0) == 'l') {
      m += MaxLabelLen(0) * mMarkText.mSize * 0.44f + 2.f;
      m += MeterWidth(0) * MarkWidthR(0) + 2.f; // 2 is the same pad as used in DrawMarks()
      }
    return m;
    }
  float GetRightMarginAbs() {
    auto m = 0.f;
    auto n = NumChannels() - 1;
    if (DrawMarks(n) && MarksAlign(n) == 'r') {
      m += MaxLabelLen(n) * mMarkText.mSize * 0.44f + 2.f;
      m += MeterWidth(n) * MarkWidthR(n) + 2.f; // 2 is the same pad as used in DrawMarks()
      }
    return m;
    }
  void UpdateMargins(float oldL, float oldR) {
    auto newLM = GetLeftMarginAbs();
    auto newRM = GetRightMarginAbs();
    mRECT.L -= (newLM - oldL);
    mRECT.R += (newRM - oldR);
    mTargetRECT = mRECT;
    }

  void Scale(float r) {
    for (auto ch = 0; ch != NumChannels(); ++ch) {
      *MeterWidthPtr(ch) *= r;
      *DistToTheNextMPtr(ch) *= r;
      if (MeterWidth(ch) < 1.f)*MeterWidthPtr(ch) = 1.f;
      if (DistToTheNextM(ch) < 0.f) *DistToTheNextMPtr(ch) = 0.f;
      *ChanNameHOffsetPtr(ch) *= r;
      *MarksOffsetPtr(ch) *= r;
      }
    }

  void SetChanNames(const char* names, va_list args) {
    if (NumChannels() < 1) return;
    ChanNamePtr(0)->Set(names);
    for (int c = 1; c < NumChannels(); ++c)
      ChanNamePtr(c)->Set(va_arg(args, const char*));
    }
  void RecalcMaxChNameH(float oldAbsH) {
    mChNameMaxH = 0.f;
    float w, h;
    for (int ch = 0; ch != NumChannels(); ++ch)
      if (DrawChanName(ch)) {
        BasicTextMeasure(ChanNamePtr(ch)->Get(), h, w);
        if (w != 0.0 && h > mChNameMaxH) mChNameMaxH = h;
        }

    auto finalAbsH = mChNameMaxH * mText.mSize;
    mRECT.B += (finalAbsH - oldAbsH);
    mTargetRECT = mRECT;

    }

  void DrawMarks(IGraphics& graphics) {
    // todo some dots from labels appear on the right
    auto shadowColor = IColor(60, 0, 0, 0);
    for (auto ch = 0; ch != NumChannels(); ++ch)
      if (DrawMarks(ch)) {
        // compute common for all marks vals
        auto o = MarksOffset(ch);
        auto mR = GetMeterRect(ch);
        auto labelDx = o;
        auto longLen = MarkWidthR(ch) * mR.W(); // long mark
        auto lx1 = 0.f;
        auto lx2 = longLen;
        auto mx1 = 0.f; // short mark coords
        auto mx2 = 0.7f * longLen;
        auto pad = 2.f;

        auto mTxt = mMarkText;
        auto maxLabelW = MaxLabelLen(ch) * mMarkText.mSize * 0.44f; // 0.44 - approx symbol width
        auto lastLabBot = 0.f; // hide overlapping labels
        auto maxLabOverlap = 0.4f * mMarkText.mSize;

        auto align = MarksAlign(ch);
        if (align == 'l') {
          o += mR.L - pad;
          labelDx = o - longLen - pad;
          mTxt.mAlign = IText::kAlignFar;
          mx1 = o - mx2;
          mx2 = mx1 + mx2;
          lx2 = mx2;
          lx1 = lx2 - longLen;
          }
        else if (align == 'r') {
          o += mR.R + pad + 1.f;
          mTxt.mAlign = IText::kAlignNear;
          labelDx = o + longLen + pad;
          mx1 = o;
          mx2 = mx1 + mx2;
          lx1 = mx1;
          lx2 = lx1 + longLen;
          }
        else // center
          {
          auto hr = 0.5f * mR.W();
          o += mR.L + hr;
          mTxt.mAlign = IText::kAlignCenter;
          mx1 = o - 0.5f * mx2;
          mx2 = mx1 + mx2;
          lx1 = o - hr;
          lx2 = o + hr;
          labelDx = 0.5f * (lx1 + lx2);
          }

        auto RectForHorLine = [&] (float y, bool shortM = true) {
          auto r = mR;
          r.T = y;
          r.B = y + 1.f;
          if (shortM) {
            r.L = mx1;
            r.R = mx2;
            }
          else {
            r.L = lx1;
            r.R = lx2;
            }
          return r;
          };

        auto DrawLine = [&] (IRECT lr) {
          if (mDrawShadows) {
            auto sr = ShiftRectBy(lr, 2.f, 1.f);
            graphics.FillRect(shadowColor, sr);
            }
          graphics.FillRect(mMarkText.mTextEntryBGColor, lr);
          };

        auto DrawMark = [&] (float h) {
          auto r = RectForHorLine(h);
          DrawLine(r);
          };

        auto DrawMarkWithLabel = [&] (float h, double v) {
          auto tr = mR;
          tr.T = h - mMarkText.mSize / 2;
          tr.B = tr.T + mMarkText.mSize;
          auto r = RectForHorLine(h, false);
          tr.L = labelDx;
          tr.R = labelDx + 1.f;
          //graphics.DrawRect(COLOR_RED, tr);
          // first form a string
          WDL_String l;
          l.SetFormatted(8, PrecisionString(ch).Get(), v);
          RemoveTrailingZeroes(&l);

          bool drawLabel = false;
          if (lastLabBot - maxLabOverlap < tr.T) {
            lastLabBot = tr.B;
            drawLabel = true;
            }

          // then draw level lines
          if (align != 'c')
            DrawLine(r);
          else { // draw shorter level lines around the label or one long line
            if (drawLabel) {
              auto sl = 0.2f * longLen; // short lines length
              auto ld = 0.5f * maxLabelW + 2.f * pad; // distance from center to the line start
              float lw, lh;
              BasicTextMeasure(l.Get(), lh, lw);
              auto addLen = 0.25f * (maxLabelW - lw) - pad;

              auto d = mR.W() - maxLabelW - 4.f * pad; // how much horizontal space left for lines
              // NB maxLen is used ^^^ for consistency across all marks of the current meter
              if (d > 0.f) {
                // have space to draw lines
                // line on the left
                auto rl = r;
                rl.R = labelDx - ld;
                rl.L = rl.R - sl;
                rl.R += addLen;
                // line on the right
                auto rr = r;
                rr.L = labelDx + ld;
                rr.R = rr.L + sl;
                rr.L -= addLen;
                if (mDrawBorders) rl.L += 1.f;
                // if they stick out of the mR.W(), shorten
                auto ex = rr.R - (labelDx + 0.5f * mR.W());
                if (ex > 0.f) {
                  rl.L += ex;
                  rr.R -= ex;
                  }
                DrawLine(rl);
                DrawLine(rr);
                }
              else {
                // no space, at least draw short marks
                r.L = r.R - pad;
                DrawLine(r);
                if (!UnitsDB(ch)) { //dB often have minus signs
                  r.L = mR.L + labelDx + 1.f;
                  r.R = r.L + pad;
                  DrawLine(r);
                  }
                }
              }
            else // skipped label, drawing only line
              {
              r.L = labelDx - 0.5f * longLen;
              r.R = labelDx + 0.5f * longLen;
              DrawLine(r);
              }
            }

          if (drawLabel) {
          // finally draw the string
            if (mDrawShadows) {
              auto tt = mTxt;
              tt.mFGColor = shadowColor;
              auto sr = ShiftRectBy(tr, 1.f, 1.f);
              graphics.DrawTextA(tt, l.Get(), sr);
              }
            graphics.DrawTextA(mTxt, l.Get(), tr);
            }
          };

        for (int m = 0; m != MarksPtr(ch)->GetSize(); ++m) {
          auto v = Mark(ch, m);
          if (v > MinDisplayVal(ch) && v < MaxDisplayVal(ch)) {
            // don't draw marks that are outside of the display range
            auto h = GetVCoordFromValInMeterRect(ch, v, mR);
            h = trunc(h); // at least on LICE nonintegers look bad
            if (h < mR.B && h > mR.T) {
              // don't draw right on T or B
              if (UnitsDB(ch)) v = AmpToDB(v);
              if (MarkLabel(ch, m))
                DrawMarkWithLabel(h, v);
              else
                DrawMark(h);
              }
            }
          }
        }
    }
  void RecalcMaxLabelLen(int ch) {
    // this alg is almost the same as the one used in marks drawing
    auto maxL = 0.f;
    auto mR = GetMeterRect(ch);
    for (int m = 0; m != MarksPtr(ch)->GetSize(); ++m) {
      auto len = 0.f;
      auto th = 0.f;
      if (MarkLabel(ch, m)) {
        auto v = Mark(ch, m);
        if (v > MinDisplayVal(ch) && v < MaxDisplayVal(ch)) {
          auto h = GetVCoordFromValInMeterRect(ch, v, mR);
          h = trunc(h);
          if (h < mR.B && h > mR.T) {
            if (UnitsDB(ch)) v = AmpToDB(v);
            WDL_String l;
            l.SetFormatted(8, PrecisionString(ch).Get(), v);
            RemoveTrailingZeroes(&l);
            BasicTextMeasure(l.Get(), th, len);
            }
          }
        }
      if (len > maxL)
        maxL = len;
      }
    *MaxLabelLenPtr(ch) = maxL;
    }

  WDL_String PrecisionString(int ch) {
    WDL_String s;
    s.Set("%4."); // 4 for cases like -160 dB
    s.AppendFormatted(2, "%d", NumDisplPrecision(ch));
    s.Append("f");
    return s;
    }
  void RemoveTrailingZeroes(WDL_String* s, int minDecimalsToKeep = 0) {
    char* start = s->Get();
    char* p = start;

    while (*p != '.' && *p != '\0') // find a dot
      ++p;

    if (*p != '\0') // reverse search the 1st nonzero digit
      {
      auto dotPos = (int)(p - start);
      int delPos = s->GetLength() - 1;
      while (delPos != dotPos) {
        char* c = start + delPos;
        if (isdigit(c[0]) && c[0] != '0')
          break;
        else
          --delPos;
        }
      ++delPos;

      if (minDecimalsToKeep == 0) {
        if (delPos == dotPos + 1) // if all decimals are 0, we don't need the dot
          --delPos;
        }
      else if (delPos < dotPos + minDecimalsToKeep + 1)
        delPos = dotPos + minDecimalsToKeep + 1;

      if (delPos < s->GetLength())
        s->SetLen(delPos);
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

  bool* UnitsDBPtr          (int i) { return &(Ch(i)->unitsDB); }
  bool UnitsDB              (int i) { return *UnitsDBPtr(i); }
  bool* ScaleLogPtr         (int i) { return &(Ch(i)->scaleLog); }
  bool ScaleLog             (int i) { return *ScaleLogPtr(i); }

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
  bool* DrawMaxPeakPtr      (int i) { return &(Ch(i)->drawMaxPeak); }
  bool DrawMaxPeak          (int i) { return *DrawMaxPeakPtr(i); }
  double* MaxPeakPtr        (int i) { return &(Ch(i)->maxPeak); }
  double MaxPeak            (int i) { return *MaxPeakPtr(i); }

  bool* DrawMemRectPtr      (int i) { return &(Ch(i)->drawMemRect); }
  bool DrawMemRect          (int i) { return *DrawMemRectPtr(i); }
  double* MemPeakPtr        (int i) { return &(Ch(i)->memPeak); }
  double MemPeak            (int i) { return *MemPeakPtr(i); }
  double* MemExpPtr         (int i) { return &(Ch(i)->memExp); }
  double MemExp             (int i) { return *MemExpPtr(i); }

  WDL_TypedBuf<double>*  RMSBufPtr  (int i) { return *RMSBufPP(i); }
  WDL_TypedBuf<double>** RMSBufPP   (int i) { return &(Ch(i)->RMSBuf); }
  double* RMSBufValPtr (int ch, int i) { return RMSBufPtr(ch)->Get() + i; }
  double RMSBufVal     (int ch, int i) { return *RMSBufValPtr(ch, i); }
  size_t RMSBufLen          (int i) { return RMSBufPtr(i)->GetSize(); }
  size_t* RMSBufPosPtr      (int i) { return &(Ch(i)->RMSBufPos); }
  size_t RMSBufPos          (int i) { return *RMSBufPosPtr(i); }
  double* RMSWindowMsPtr    (int i) { return &(Ch(i)->RMSWindowMs); }
  double RMSWindowMs        (int i) { return *RMSWindowMsPtr(i); }
  double* RMSSumPtr         (int i) { return &(Ch(i)->RMSSum); }
  double RMSSum             (int i) { return *RMSSumPtr(i); }
  bool* DrawRMSPtr          (int i) { return &(Ch(i)->drawRMS); }
  bool DrawRMS              (int i) { return *DrawRMSPtr(i); }
  bool* AESFixPtr           (int i) { return &(Ch(i)->AESFix); }
  bool AESFix               (int i) { return *AESFixPtr(i); }

  float* MeterWidthPtr      (int i) { return &(Ch(i)->meterWidth); }
  float MeterWidth          (int i) { return *MeterWidthPtr(i); }
  float* DistToTheNextMPtr  (int i) { return &(Ch(i)->distToNextM); }
  float DistToTheNextM      (int i) { return *DistToTheNextMPtr(i); }

  bool* DrawMarksPtr        (int i) { return &(Ch(i)->drawMarks); }
  bool DrawMarks            (int i) { return *DrawMarksPtr(i); }
  float* MarkWidthRPtr      (int i) { return &(Ch(i)->markWidthR); }
  float MarkWidthR          (int i) { return *MarkWidthRPtr(i); }
  char* MarksAlignPtr       (int i) { return &(Ch(i)->marksAlign); }
  char MarksAlign           (int i) { return *MarksAlignPtr(i); }
  float* MarksOffsetPtr     (int i) { return &(Ch(i)->marksOffset); }
  float MarksOffset         (int i) { return *MarksOffsetPtr(i); }
  float* MaxLabelLenPtr     (int i) { return &(Ch(i)->maxLabelLen); }
  float MaxLabelLen         (int i) { return *MaxLabelLenPtr(i); }

  WDL_TypedBuf<double>*  MarksPtr    (int i) { return *MarksPP(i); }
  WDL_TypedBuf<double>** MarksPP     (int i) { return &(Ch(i)->marks); }
  double Mark        (int ch, int i) { return *(MarksPtr(ch)->Get() + i); }

  WDL_TypedBuf<bool>*  MarkLabelsPtr (int i) { return *MarkLabelsPP(i); }
  WDL_TypedBuf<bool>** MarkLabelsPP  (int i) { return &(Ch(i)->markLabels); }
  bool MarkLabel     (int ch, int i) { return *(MarkLabelsPtr(ch)->Get() + i); }

  int* NumDisplPrecisionPtr (int i) { return &(Ch(i)->numDisplPrecision); }
  int NumDisplPrecision     (int i) { return *NumDisplPrecisionPtr(i); }

  bool* DrawChanNamePtr     (int i) { return &(Ch(i)->drawChanName); }
  bool DrawChanName         (int i) { return *DrawChanNamePtr(i); }
  float* ChanNameHOffsetPtr (int i) { return &(Ch(i)->chanNameHOffset); }
  float ChanNameHOffset     (int i) { return *ChanNameHOffsetPtr(i); }
  WDL_String* ChanNamePtr   (int i) { return *ChanNamePP(i); }
  WDL_String** ChanNamePP   (int i) { return &(Ch(i)->chanName); }
  };