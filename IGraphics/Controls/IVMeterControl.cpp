#include "IVMeterControl.h"

const IColor IVMeterControl::DEFAULT_BG_COLOR = IColor(255, 70, 70, 70);
const IColor IVMeterControl::DEFAULT_RAW_COLOR = IColor(255, 200, 200, 200);
const IColor IVMeterControl::DEFAULT_RMS_COLOR = IColor(200, 70, 150, 80);
const IColor IVMeterControl::DEFAULT_PK_COLOR = IColor(255, 240, 60, 60);
const IColor IVMeterControl::DEFAULT_FR_COLOR = DEFAULT_BG_COLOR;


IVMeterControl::IVMeterControl(IDelegate& dlg, IRECT bounds, int numChannels, const char* chanNames, ...)
  : IControl(dlg, bounds, kNoParameter)
  , IVectorBase(&DEFAULT_BG_COLOR, &DEFAULT_RAW_COLOR, &DEFAULT_FR_COLOR, &DEFAULT_PK_COLOR, &DEFAULT_RMS_COLOR)
{
  AttachIControl(this);

  ChannelSpecificData d;
  for (auto ch = 0; ch != numChannels; ++ch)
  {
    mChanData.Add(d);
    *(RMSBufPP(ch)) = new WDL_TypedBuf<double>;
    *(ChanNamePP(ch)) = new WDL_String;
    *(MarksPP(ch)) = new WDL_TypedBuf<double>;
    *(MarkLabelsPP(ch)) = new WDL_TypedBuf<bool>;
  }

  SetRMSWindowMs(300.0);
  SetLevelMarks("3 0s -3 -6s -9 -12s -18s -24s -30s -36s -42s -48s -54s -60s");

  if(chanNames)
  {
    va_list args;
    va_start(args, chanNames);
    SetChanNames(chanNames, args);
    va_end(args);
  }
  
  RecalcMaxChNameH(0.f);

  if (bounds.Empty())
    mRECT = mTargetRECT = GetControlRectFromChannelsData(false);
  else
    OnResize();

};

IVMeterControl::~IVMeterControl()
{
  for (auto c = 0; c != NumChannels(); ++c)
  {
    delete(RMSBufPtr(c));
    delete(ChanNamePtr(c));
    delete(MarksPtr(c));
    delete(MarkLabelsPtr(c));
  }
}


void IVMeterControl::SetSampleRate(double sr)
{
  if (mSampleRate == sr || sr <= 0.0) return;
  mSampleRate = sr;
  for (int ch = 0; ch != NumChannels(); ++ch)
  {
    RMSBufPtr(ch)->Resize(int(0.001 * RMSWindowMs(ch) * mSampleRate));
    ZeroRMSBuf(ch);
  }
}

void IVMeterControl::Reset(int chId)
{
  // if you hack up too much
  auto R = [&] (int ch)
  {
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

void IVMeterControl::ResetRMS(int chId)
{
  if (chId < 0)
    for (auto ch = 0; ch != NumChannels(); ++ch)
      ZeroRMSBuf(ch);
  else
    ZeroRMSBuf(chId);
  SetDirty();
}


void IVMeterControl::ProcessChanValue(double value, int chId)
{
  value = abs(value);

  auto Set = [this] (int i, double v)
  {
    if (v > RawValue(i))
      *RawValuePtr(i) = v; // it makes sense to show max value that was gained between the Draw() calls.
    // in Draw then the value is zeroed right after reading.
    // it's not 100% thread safe, but in this case it doesn't matter.

    if (DrawRMS(i))
    {
      auto s = v * v;
      auto pos = RMSBufPos(i);
      auto oldS = RMSBufVal(i, pos);
      *RMSBufValPtr(i, pos) = s;
      *RMSSumPtr(i) += s - oldS;
      ++pos;
      if (pos >= RMSBufLen(i)) pos = 0;
      *RMSBufPosPtr(i) = pos;
    }

    if (v >= GetPeakFromMemExp(i))
    {
      *MemPeakPtr(i) = v;
      *MemExpPtr(i) = 1.0;
      *PeakSampHeldPtr(i) = 0;
    }

    if (MemPeak(i) >= OverThresh(i))
    {
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

void IVMeterControl::ProcessChan(sample* in, int blockSize, int chId)
{
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

void IVMeterControl::ProcessBus(sample** ins, int blockSize, int numChans, int sourceStartId, int destStartId)
{
  if (numChans < 0) numChans = NumChannels();

  for (int ch = 0; ch != numChans; ++ch)
  {
    auto in = ins[sourceStartId + ch];
    ProcessChan(in, blockSize, destStartId + ch);
  }
}

void IVMeterControl::ProcessInsOuts(sample** ins, sample** outs, int blockSize, int numIns, int numOuts)
{
  if (numIns < 0) numIns = NumChannels() / 2;
  if (numOuts < 0) numOuts = numIns;

  for (int ch = 0; ch != numIns; ++ch)
  {
    auto in = ins[ch];
    ProcessChan(in, blockSize, ch);
  }
  for (int ch = 0; ch != numOuts; ++ch)
  {
    auto out = outs[ch];
    ProcessChan(out, blockSize, ch + numIns);
  }
}


double IVMeterControl::GetRMS(int chId)
{
  auto rms = RMSSum(chId) / RMSBufLen(chId);
  if (AESFix(chId))
    rms *= 2.0;
  rms = sqrt(rms);
  return rms;
}

double IVMeterControl::GetMaxPeak(int chId)
{
  return MaxPeak(chId);
}

void IVMeterControl::SetUnitsDB(bool db, int chId, bool scaleFollowsUnits)
{
  auto initLM = GetLeftMarginAbs();  // here and further: such margins recalculations and compensation
  auto initRM = GetRightMarginAbs(); // are necessary for correctd drawing.
  // hence it must appear in any setter that may affect the margins:
  // font sizes, units, ranges, preision, etc.

  if (chId < 0)
  {
    for (int ch = 0; ch != NumChannels(); ++ch)
      *UnitsDBPtr(ch) = db;
    if (scaleFollowsUnits)
      for (int ch = 0; ch != NumChannels(); ++ch)
        *ScaleLogPtr(ch) = db;
    for (int ch = 0; ch != NumChannels(); ++ch)
      RecalcMaxLabelLen(ch);
  }
  else
  {
    *UnitsDBPtr(chId) = db;
    if (scaleFollowsUnits)
      *ScaleLogPtr(chId) = db;
    RecalcMaxLabelLen(chId);
  }

  UpdateMargins(initLM, initRM);

  SetDirty();
}

void IVMeterControl::SetScaleLog(bool logScale, int chId)
{
  if (chId < 0)
    for (int ch = 0; ch != NumChannels(); ++ch)
      *ScaleLogPtr(ch) = logScale;
  else
    *ScaleLogPtr(chId) = logScale;

  SetDirty();
}

void IVMeterControl::SetMinMaxDisplayValues(double min, double max, int chId)
{
  if (min > max)
  {
    auto b = max;
    max = min;
    min = b;
  }
  else if (min == max)
    max += 0.1;

  auto Set = [this] (int i, double min, double max)
  {
    if (UnitsDB(i))
    {
      *MaxDisplayValPtr(i) = DBToAmp(max);
      *MinDisplayValPtr(i) = DBToAmp(min);
    }
    else
    {
      *MinDisplayValPtr(i) = abs(min);
      *MaxDisplayValPtr(i) = abs(max);
    }
  };

  auto initLM = GetLeftMarginAbs();
  auto initRM = GetRightMarginAbs();

  if (chId < 0)
    for (int ch = 0; ch != NumChannels(); ++ch)
    {
      Set(ch, min, max);
      RecalcMaxLabelLen(ch);
    }
  else
  {
    Set(chId, min, max);
    RecalcMaxLabelLen(chId);
  }

  UpdateMargins(initLM, initRM);

  SetDirty();
}

void IVMeterControl::SetNumDisplayPrecision(int precision, int chId)
{
  auto initLM = GetLeftMarginAbs();
  auto initRM = GetRightMarginAbs();

  if (chId < 0)
    for (int ch = 0; ch != NumChannels(); ++ch)
    {
      *NumDisplPrecisionPtr(ch) = precision;
      RecalcMaxLabelLen(ch);
    }
  else
  {
    *NumDisplPrecisionPtr(chId) = precision;
    RecalcMaxLabelLen(chId);
  }

  UpdateMargins(initLM, initRM);

  SetDirty();
}


void IVMeterControl::SetOverdriveThreshold(double thresh, int chId)
{
  auto Set = [this] (int i, double t)
  {
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

void IVMeterControl::SetPeakDropTimeMs(double ms, int chId)
{
  ms = abs(ms);

  if (chId < 0)
    for (int ch = 0; ch != NumChannels(); ++ch)
      *DropMsPtr(ch) = ms;
  else
    *DropMsPtr(chId) = ms;
}

void IVMeterControl::SetDrawPeakRect(bool draw, int chId)
{
  if (chId < 0)
    for (int ch = 0; ch != NumChannels(); ++ch)
    {
      *DrawPeakRectPtr(ch) = draw;
      *HoldingAPeakPtr(ch) = false;
      if (!draw) *OverBlinkPtr(ch) = 0.0;
    }
  else
  {
    *DrawPeakRectPtr(chId) = draw;
    *HoldingAPeakPtr(chId) = false;
    if (!draw) *OverBlinkPtr(chId) = 0.0;
  }
  SetDirty();
}

void IVMeterControl::SetDrawMaxPeak(bool draw, int chId)
{
  if (chId < 0)
    for (int ch = 0; ch != NumChannels(); ++ch)
      *DrawMaxPeakPtr(ch) = draw;
  else
    *DrawMaxPeakPtr(chId) = draw;
  SetDirty();
}

void IVMeterControl::SetDrawMemRect(bool draw, int chId)
{
  if (chId < 0)
    for (int ch = 0; ch != NumChannels(); ++ch)
      *DrawMemRectPtr(ch) = draw;
  else
    *DrawMemRectPtr(chId) = draw;
  SetDirty();
}


void IVMeterControl::SetDrawRMS(bool draw, int chId)
{
  if (chId < 0)
    for (int ch = 0; ch != NumChannels(); ++ch)
    {
      if (draw != DrawRMS(ch))
        ZeroRMSBuf(ch);
      *DrawRMSPtr(ch) = draw;
    }
  else
  {
    if (draw != DrawRMS(chId))
      ZeroRMSBuf(chId);
    *DrawRMSPtr(chId) = draw;
  }
  SetDirty();
}

void IVMeterControl::SetRMSWindowMs(double ms, int chId)
{
  if (ms <= 0.0) return;
  if (0.001 * ms * mSampleRate < 10.0) // window at least 10 samples long
    ms = 10000.0 / mSampleRate;

  auto s = int(0.001 * ms * mSampleRate);
  if (chId < 0)
    for (int ch = 0; ch != NumChannels(); ++ch)
    {
      RMSBufPtr(ch)->Resize(s);
      // todo check Resize() implementation, maybe zero only the new allocations
      ZeroRMSBuf(ch);
      if (RMSBufPos(ch) >= s)
        *RMSBufPosPtr(ch) = 0;
    }
  else
  {
    RMSBufPtr(chId)->Resize(s);
    ZeroRMSBuf(chId);
    if (RMSBufPos(chId) >= s)
      *RMSBufPosPtr(chId) = 0;
  }
}

void IVMeterControl::SetAesRmsFix(bool plusThreeDB, int chId)
{
  if (chId < 0)
    for (int ch = 0; ch != NumChannels(); ++ch)
      *AESFixPtr(ch) = plusThreeDB;
  else
    *AESFixPtr(chId) = plusThreeDB;
}


void IVMeterControl::SetDrawChName(bool draw, int chId)
{
  auto initH = mChNameMaxH * mText.mSize;

  if (chId < 0)
    for (int ch = 0; ch != NumChannels(); ++ch)
      *DrawChanNamePtr(ch) = draw;
  else
    *DrawChanNamePtr(chId) = draw;

  RecalcMaxChNameH(initH);

  SetDirty();
}

void IVMeterControl::SetChanName(int chId, const char* name)
{
  auto initH = mChNameMaxH * mText.mSize;
  if (chId < NumChannels())
    ChanNamePtr(chId)->Set(name);
  RecalcMaxChNameH(initH);
  SetDirty();
}

void IVMeterControl::SetChanNames(const char* names, ...)
{
  auto initH = mChNameMaxH * mText.mSize;
  va_list args;
  va_start(args, names);
  SetChanNames(names, args);
  va_end(args);

  RecalcMaxChNameH(initH);
  SetDirty();
}


void IVMeterControl::SetDrawMarks(bool draw, int chId)
{
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

void IVMeterControl::SetLevelMarks(const char* marks, int chId)
{
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

  while (*marks != '\0')
  {
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
  //DBGMSG(parsed.Get());
#endif

  auto Set = [&] (int ch)
  {
    auto n = markVals.GetSize();
    MarkLabelsPtr(ch)->Resize(0);
    MarksPtr(ch)->Resize(0);

    for (int i = 0; i != n; ++i)
    {
      MarksPtr(ch)->Add(*(markVals.Get() + i));
      MarkLabelsPtr(ch)->Add(*(markLabels.Get() + i));
    }
  };

  auto initLM = GetLeftMarginAbs();
  auto initRM = GetRightMarginAbs();

  if (chId < 0)
    for (int ch = 0; ch != NumChannels(); ++ch)
    {
      Set(ch);
      RecalcMaxLabelLen(ch);
    }
  else
  {
    Set(chId);
    RecalcMaxLabelLen(chId);
  }

#ifdef _DEBUG
  WDL_String wr("written marks: ");
  for (int v = 0; v != MarksPtr(0)->GetSize(); ++v)
    wr.AppendFormatted(8, "%3.2f, ", *(MarksPtr(0)->Get() + v));
  //DBGMSG(wr.Get());
#endif

  UpdateMargins(initLM, initRM);

  SetDirty();
}

void IVMeterControl::SetMarkWidthRatio(float r, int chId)
{
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

void IVMeterControl::SetMarksAlignment(char alignment, int chId, bool shortenMarksForSides)
{
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

void IVMeterControl::SetMarksHOffset(float offset, int chId)
{
// offset is mainly used for drawing one set of marks between the adjacent meters.
  auto initLM = GetLeftMarginAbs();
  auto initRM = GetRightMarginAbs();

  if (chId < 0)
    for (int ch = 0; ch != NumChannels(); ++ch)
      *MarksOffsetPtr(ch) = offset;
  else
    *MarksOffsetPtr(chId) = offset;

  UpdateMargins(initLM, initRM);

  SetDirty();
}


void IVMeterControl::SetMeterWidth(double w, int chId)
{
  if (w < 0.0) w = 20.0;

  if (chId < 0)
    for (int ch = 0; ch != NumChannels(); ++ch)
      *MeterWidthPtr(ch) = (float) w;
  else
    *MeterWidthPtr(chId) = (float) w;

  mRECT = mTargetRECT = GetControlRectFromChannelsData();

  SetDirty();
}

void IVMeterControl::SetDistToTheNextMeter(double d, int chId, bool compactLabels, bool glueRects)
{
  if (d < 0.0) d = 30.0;
  glueRects = glueRects && (d == 0.0);

  if (chId < 0)
  {
    if (glueRects)
    {
      float w = 0.0;
      for (int ch = 0; ch != NumChannels() - 1; ++ch)
        w += DistToTheNextM(ch);
      w /= NumChannels();
      for (int ch = 0; ch != NumChannels(); ++ch)
        *MeterWidthPtr(ch) += w;
    }
    for (int ch = 0; ch != NumChannels(); ++ch)
      *DistToTheNextMPtr(ch) = (float) d;
    if (compactLabels)
    {
      // only put marks in the middle, don't touch chan names
      for (int ch = 0; ch != NumChannels() - 1; ++ch)
      {
        *MarksAlignPtr(ch) = 'c';
        *MarksOffsetPtr(ch) = 0.5f * (MeterWidth(ch) + DistToTheNextM(ch));
      }
      *DrawMarksPtr(NumChannels()) = false;
    }
  }
  else
  {
    if (glueRects && chId != NumChannels() - 1)
    {
      auto stretch = 0.5f * DistToTheNextM(chId);
      *MeterWidthPtr(chId) += stretch;
      *MeterWidthPtr(chId + 1) += stretch;
    }

    *DistToTheNextMPtr(chId) = (float) d;

    if (compactLabels && chId < NumChannels() - 1)
    {
      // put 1st marks and chanName in the center, hide 2nd ones
      *DrawMarksPtr(chId) = true;
      *DrawMarksPtr(chId + 1) = false;
      *MarksAlignPtr(chId) = 'c';
      auto o = 0.5f * (MeterWidth(chId) + DistToTheNextM(chId));
      *MarksOffsetPtr(chId) = o;
      if (ChanNamePtr(chId)->GetLength())
      {
        *DrawChanNamePtr(chId) = true;
        *DrawChanNamePtr(chId + 1) = false;
        *ChanNameHOffsetPtr(chId) = o;
      }
      else if (ChanNamePtr(chId + 1)->GetLength())
      {
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
  //DBGMSG(s.Get());
#endif

  SetDirty();
}


void IVMeterControl::SetTexts(IText chNameTxt, IText marksTxt)
{
  auto initLM = GetLeftMarginAbs();
  auto initRM = GetRightMarginAbs();
  auto initH = mChNameMaxH * mText.mSize;

  mText = chNameTxt;
  mMarkText = marksTxt;

  UpdateMargins(initLM, initRM);
  RecalcMaxChNameH(initH);
  SetDirty();
}


void IVMeterControl::Draw(IGraphics& g)
{
  double fps = g.FPS();
  auto spf = 1.0 / fps;
  auto sampPerDraw = mSampleRate / fps;
  auto shadowColor = IColor(60, 0, 0, 0);

  for (auto ch = 0; ch != NumChannels(); ++ch)
  {
    auto v = RawValue(ch); // always >= 0.0
    *RawValuePtr(ch) = 0.0;

    auto meterRect = GetMeterRect(ch);

    // background and shadows
    auto bgRect = meterRect;
    if (DrawPeakRect(ch)) bgRect.T -= mPeakRectHeight;
    g.FillRect(GetColor(mBg), bgRect);
    if (mDrawShadows)
      DrawInnerShadowForRect(bgRect, shadowColor, g);

    // raw value rect
    auto rawR = meterRect;
    rawR.T = GetVCoordFromValInMeterRect(ch, v, meterRect);
    if (v >= MinDisplayVal(ch))
      g.FillRect(GetColor(mRaw), rawR);

    // memory rect
    // math
    auto p = GetPeakFromMemExp(ch);
    if (p < 0.0) p = 0.0;
    if (p < v || p == 0.0)
    {
      p = v;
      *MemPeakPtr(ch) = v;
      *MemExpPtr(ch) = 1.0;
      *PeakSampHeldPtr(ch) = 0;
    }
    else
    {
      if (PeakSampHeld(ch) >= 0.001 * DropMs(ch) * mSampleRate)
      {
        auto t = DropMs(ch);
        if (p > 0.0 && p < 1.0) t /= p; // low values should decay ~at the same rate
        *MemExpPtr(ch) *= GetInvExpForDrop(t, fps); // different decay character than a simple exp
      }
      else
        *PeakSampHeldPtr(ch) += (size_t) sampPerDraw;
    }
    // graphics
    if (DrawMemRect(ch) && p >= MinDisplayVal(ch) && DropMs(ch) > spf * 1000.0)
      // if drop time is shorter than the time between Draw() calls it's redundant
    {
      auto memR = meterRect;
      memR.T = GetVCoordFromValInMeterRect(ch, p, meterRect);
      memR.B = rawR.T;
      auto c = GetColor(mRaw);
      c.A /= 2;
      g.FillRect(c, memR);
      auto pc = LinearBlendColors(GetColor(mRaw), GetColor(mPeak), OverBlink(ch));
      if (p <= MaxDisplayVal(ch))
        g.DrawLine(pc, memR.L, memR.T, memR.R, memR.T);
    }

    // rms rect
    auto rms = 0.0;
    if (DrawRMS(ch))
    {
      rms = GetRMS(ch);
      if (rms > MinDisplayVal(ch))
      {
        auto rmsR = meterRect;
        rmsR.T = GetVCoordFromValInMeterRect(ch, rms, meterRect);
        g.FillRect(GetColor(mRms), rmsR);

      }
    }

    // peak rect
    if (v > MaxPeak(ch))
      *MaxPeakPtr(ch) = v;
    // graphics stuff
    IRECT pvtr;
    if (DrawPeakRect(ch))
    {
      auto pR = meterRect;
      pR.T = meterRect.T - mPeakRectHeight;
      pR.B = meterRect.T;
      auto pc = LinearBlendColors(COLOR_TRANSPARENT, GetColor(mPeak), OverBlink(ch));
      pvtr = pR;
      g.FillRect(pc, pR);
    }
    // math
    if (!HoldingAPeak(ch))
      *OverBlinkPtr(ch) *= GetExpForDrop(1000.0 + 2.5 * DropMs(ch), fps);

    if (mDrawFrame)
      g.DrawRect(GetColor(mFr), bgRect);

    if (DrawMaxPeak(ch)) // not to draw borders over the peak value
    {
      if (!DrawPeakRect(ch))
      {
        pvtr = meterRect;
        pvtr.B = pvtr.T + mMarkText.mSize;
      }

      WDL_String mps;
      auto v = MaxPeak(ch);
      if (UnitsDB(ch))
      {
        v = AmpToDB(v);
        if (v >= -300.0)
        {
          mps.SetFormatted(8, PrecisionString(ch).Get(), v);
          RemoveTrailingZeroes(&mps, 1);
        }
        else mps.Set("<-300");
      }
      else
      {
        mps.SetFormatted(8, PrecisionString(ch).Get(), v);
        RemoveTrailingZeroes(&mps, 1);
      }

      pvtr = ShiftRectBy(pvtr, 0.f, 1.0);
      if (mDrawShadows)
      {
        auto tt = mMarkText;
        tt.mFGColor = shadowColor;
        auto sr = ShiftRectBy(pvtr, 1.0, 1.0);
        g.DrawText(tt, mps.Get(), sr);
      }
      g.DrawText(mMarkText, mps.Get(), pvtr);
    }

    if (DrawChanName(ch)) // can be inside the loop because names are below the meters
    {
      auto cnr = meterRect;
      auto h = mText.mSize;
      cnr.B += h;
      cnr.T = cnr.B - h;
      cnr = ShiftRectBy(cnr, ChanNameHOffset(ch));
      g.DrawText(mText, ChanNamePtr(ch)->Get(), cnr);
    }


#ifdef _DEBUG

    auto trms = mText;
    trms.mFGColor = COLOR_BLACK;
    WDL_String ps;
    auto vt = rms;
    if (UnitsDB(ch)) vt = AmpToDB(vt);
    ps.SetFormatted(16, "rms\n%4.2f", vt);
    auto dtr = rawR;
    dtr.T = dtr.B - 2.0f * trms.mSize - 10.0f;
    g.DrawText(trms, ps.Get(), dtr);

    auto tl = GetVCoordFromValInMeterRect(ch, OverThresh(ch), meterRect);
    g.DrawLine(COLOR_ORANGE, meterRect.L, tl, meterRect.R + 0.3f * DistToTheNextM(ch), tl);
#endif
  }

  DrawMarks(g); // draw outside because meters can be drawn over the marks

#ifdef _DEBUG
  //auto txtfps = mText;
  // txtfps.mFGColor = COLOR_GREEN;
  // WDL_String fpss;
  // fpss.SetFormatted(8, "fps\n%d", (int) fps);
  // auto dtr = mRECT;
  // dtr.T = dtr.B - 2.0f * txtfps.mSize - 10.0f;
  // g.DrawText(txtfps, fpss.Get(), dtr);

  g.DrawRect(COLOR_BLUE, mRECT);
#endif

  SetDirty();
}

void IVMeterControl::OnResize()
{
  auto initR = GetControlRectFromChannelsData();

  auto chNH = mChNameMaxH * mText.mSize;
  auto labW = 0.f;
  // labW represents nonscalable left and right margins
  // left margin is especially important because it affects the meter rects offset
  if (DrawMarks(0) && MarksAlign(0) == 'l')
    labW += MaxLabelLen(0);
  if (NumChannels() > 1)
  {
    int last = NumChannels() - 1;
    if (DrawMarks(last) && MarksAlign(last) == 'r')
      labW += MaxLabelLen(last);
  }

  labW *= mMarkText.mSize * 0.5f; // NB: 0.5 is used instead of 0.44 to fit all labels
  // in mRECT and avoid artefacts.
  // it is critical to use the same width coeff
  // in GetLeftMarginAbs() and GetRightMarginAbs().

  auto minH = 30.f + chNH;
  auto minW = 4.f * NumChannels() + labW;

  if (mRECT.H() < minH) mRECT.B = mRECT.T + minH;
  if (mRECT.W() < minW) mRECT.R = mRECT.L + minW;
  mTargetRECT = mRECT;

  auto userFlexArea = mRECT.W() - labW;
  auto initFlexArea = initR.W() - labW;

  auto wr = userFlexArea / initFlexArea;

  // jerky resizing makes problems sometimes
  if (wr < 0.125f)
  {
    mTargetRECT = mRECT = initR;
    SetDirty();
    return;
  }
  else if (wr > 8.f) wr = 8.f;

#ifdef _DEBUG
  if (wr != 1.f)
  {
    WDL_String rs("\n OnResize r: ");
    rs.AppendFormatted(32, "%4.6f, ", wr);
    //DBGMSG(rs.Get());
  }
#endif

  for (auto ch = 0; ch != NumChannels(); ++ch)
  {
    *MeterWidthPtr(ch) *= wr;
    *DistToTheNextMPtr(ch) *= wr;
    if (MeterWidth(ch) < 1.f)*MeterWidthPtr(ch) = 1.f;
    if (DistToTheNextM(ch) < 0.f) *DistToTheNextMPtr(ch) = 0.f;
    *ChanNameHOffsetPtr(ch) *= wr;
    *MarksOffsetPtr(ch) *= wr;
  }
  mMeterHeight = mRECT.H() - chNH;

#ifdef _DEBUG
  // check the leftovers, shouldn't be big, no need to distribute
  auto nR = GetControlRectFromChannelsData();
  auto ex = mRECT.W() - nR.W();

  WDL_String ls("\n OnResize left: ");
  ls.AppendFormatted(32, "%4.6f, ", ex);
  //DBGMSG(ls.Get());
#endif

  SetDirty();
}

void IVMeterControl::OnMouseDblClick(float x, float y, const IMouseMod& mod)
{
  if (mod.C)
    Reset();
  else
    for (int ch = 0; ch != NumChannels(); ++ch)
    {
      *HoldingAPeakPtr(ch) = false;
      *MaxPeakPtr(ch) = RawValue(ch);
    }
  SetDirty();
}

void IVMeterControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  for (int ch = 0; ch != NumChannels(); ++ch)
    if (GetMeterRect(ch, true).Contains(x, y))
    {
      *HoldingAPeakPtr(ch) = false;
      *MaxPeakPtr(ch) = RawValue(ch);
      break;
    }
  SetDirty();
}


IRECT IVMeterControl::GetMeterRect(int i, bool withODRect)
{
  auto dx = 0.f;
  dx += GetLeftMarginAbs();
  // identical narrow meters with noninteger sizes often look different,
  // so use round() to "snap to grid".
  for (int m = 0; m != i; ++m)
  {
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

IRECT IVMeterControl::GetControlRectFromChannelsData(bool keepPosition)
{
  float w = 0.0;

  w += GetLeftMarginAbs() + GetRightMarginAbs();

  bool showChName = false;
  for (int m = 0; m != NumChannels(); ++m)
  {
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


float IVMeterControl::GetVCoordFromValInMeterRect(int chId, double v, IRECT meterR)
{
  auto t = meterR.B;
  if (v > MaxDisplayVal(chId))
    t = meterR.T;
  else if (v > MinDisplayVal(chId))
  {
    auto r = 0.0;
    if (ScaleLog(chId))
    {
      auto mindB = AmpToDB(MinDisplayVal(chId));
      auto maxdB = AmpToDB(MaxDisplayVal(chId));
      auto vdB = AmpToDB(v);
      r = (vdB - mindB) / (maxdB - mindB);
    }
    else
    {
      r = (v - MinDisplayVal(chId)) / (MaxDisplayVal(chId) - MinDisplayVal(chId));
    }
    t -= (float) r * meterR.H();
  }
  return t;
}


float IVMeterControl::GetLeftMarginAbs()
{
  auto m = 0.f;
  if (DrawMarks(0) && MarksAlign(0) == 'l')
  {
    m += MaxLabelLen(0) * mMarkText.mSize * 0.5f + 2.f; // 0.5, not 0.44. same as in OnResize()
    m += MeterWidth(0) * MarkWidthR(0) + 2.f; // 2 is the same pad as used in DrawMarks()
    m -= MarksOffset(0); // offset to the left is <0
    if (m < 0.f) m = 0.f;
  }
  return m;
}

float IVMeterControl::GetRightMarginAbs()
{
  auto m = 0.f;
  auto n = NumChannels() - 1;
  if (DrawMarks(n) && MarksAlign(n) == 'r')
  {
    m += MaxLabelLen(n) * mMarkText.mSize * 0.5f + 2.f; // 0.5, not 0.44. same as in OnResize()
    m += MeterWidth(n) * MarkWidthR(n) + 2.f; // 2 is the same pad as used in DrawMarks()
    m += MarksOffset(n);
    if (m < 0.f) m = 0.f;
  }
  return m;
}

void IVMeterControl::UpdateMargins(float oldL, float oldR)
{
  auto newLM = GetLeftMarginAbs();
  auto newRM = GetRightMarginAbs();
  mRECT.L -= (newLM - oldL);
  mRECT.R += (newRM - oldR);
  mTargetRECT = mRECT;
}


void IVMeterControl::SetChanNames(const char* names, va_list args)
{
  if (NumChannels() < 1) return;
  ChanNamePtr(0)->Set(names);
  for (int c = 1; c < NumChannels(); ++c)
    ChanNamePtr(c)->Set(va_arg(args, const char*));
}

void IVMeterControl::RecalcMaxChNameH(float oldAbsH)
{
//  mChNameMaxH = 0.f;
//  float w, h;
//  for (int ch = 0; ch != NumChannels(); ++ch)
//    if (DrawChanName(ch))
//    {
//      BasicTextMeasure(ChanNamePtr(ch)->Get(), h, w);
//      if (w != 0.0 && h > mChNameMaxH) mChNameMaxH = h;
//    }
//
//  auto finalAbsH = mChNameMaxH * mText.mSize;
//  mRECT.B += (finalAbsH - oldAbsH);
//  mTargetRECT = mRECT;
}


void IVMeterControl::DrawMarks(IGraphics& g)
{
  auto shadowColor = IColor(60, 0, 0, 0);
  for (auto ch = 0; ch != NumChannels(); ++ch)
    if (DrawMarks(ch))
    {
      // first compute all the common stuff for all marks of the meter
      auto o = MarksOffset(ch);
      auto mR = GetMeterRect(ch);

      auto labelDx = o; // mid line of labels
      auto longLen = MarkWidthR(ch) * mR.W(); // long mark
      auto minLL = 0.2f * longLen; // min length of lines to the sides of label
      auto lx1 = 0.f;
      auto lx2 = longLen;
      bool clampLabelLines = MarksOffset(ch) > 0.4f * mR.W(); // if true, marks are probably used as common for adjacent meters,
      // so it's fine if they are wider than meter's rect.W

      auto mx1 = 0.f; // short mark coords
      auto mx2 = 0.7f * longLen;
      auto pad = 2.f;

      auto mTxt = mMarkText;
      auto maxLabelW = MaxLabelLen(ch) * mMarkText.mSize * 0.44f; // 0.44 - approx symbol width
      auto lastLabBot = 0.f; // hide vertical label overlap
      auto maxLabOverlap = 0.4f * mMarkText.mSize;

      IRECT wideLabMark;
      IRECT lLabMark(-1.f, -1.f, -2.f, -2.f);
      IRECT rLabMark = lLabMark;
      IRECT LabTxtRect;

      auto align = MarksAlign(ch);
      if (align == 'l')
      {
        o += mR.L - pad;
        labelDx = o - longLen - pad;
        mTxt.mAlign = IText::kAlignFar;
        mx1 = o - mx2;
        mx2 = mx1 + mx2;
        lx2 = mx2;
        lx1 = lx2 - longLen;
      }
      else if (align == 'r')
      {
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
        if (DrawMaxPeak(ch))
        {
          if (!DrawPeakRect(ch) || mPeakRectHeight < mMarkText.mSize)
            lastLabBot = mR.T + mMarkText.mSize;
        }
        o += mR.L + 0.5f * mR.W();
        mTxt.mAlign = IText::kAlignCenter;
        mx1 = o - 0.5f * mx2;
        mx2 = mx1 + mx2;
        auto hl = 0.5f * longLen;
        lx1 = o - hl;
        lx2 = o + hl;
        labelDx = 0.5f * (lx1 + lx2);
      }

      auto& ltr = LabTxtRect;
      ltr.B = float(mMarkText.mSize);
      ltr.L = labelDx;
      ltr.R = labelDx + 1.f;

      auto RectForHorLine = [&] (float y, bool shortM = true)
      {
        auto r = mR;
        r.T = y;
        r.B = y + 1.f;
        if (shortM)
        {
          r.L = mx1;
          r.R = mx2;
        }
        else
        {
          r.L = lx1;
          r.R = lx2;
        }
        return r;
      };

      wideLabMark = RectForHorLine(0, false); // init mark for all labels in this meter

      // more common preparations for central alignment
      if (align == 'c')
      {
        // calculate the lines on the sides of labels and later only shift them vrtically
        auto labpad = 2.f * pad;
        auto d = mR.W() - maxLabelW - 2.f * labpad - 4.f; // how much horizontal space left for lines. -4 is empirical adjustment
        // NB maxLen is used ^^^ for consistency across all marks of the current meter
        auto& rl = lLabMark;
        auto& rr = rLabMark;
        if (d > 0.f)
        {
          // have space to draw lines
          if (longLen - maxLabelW > 10.f + 2.f * labpad)
          {
            // draw "line - label - line" with total longLen width
            auto l = 0.5f * (longLen - maxLabelW - 5.f * pad); // 5 is empirical nice val
            rl = wideLabMark;
            rl.R = rl.L + l;
            rr = wideLabMark;
            rr.L = rr.R - l;
          }
          else
          {
            // draw "line - label - line" that is possibly wider than longLen
            auto l = 0.5f * maxLabelW + labpad;
            rl = wideLabMark;
            rl.R = labelDx - l + 1.f;
            rl.L = rl.R - minLL;
            rr = wideLabMark;
            rr.L = labelDx + l;
            rr.R = rr.L + minLL - 1.f;
            if (clampLabelLines)
            {
              auto ex = rr.R - (labelDx + 0.5f * mR.W());
              if (ex > 0.f)
              {
                rl.L += ex;
                rr.R -= ex;
              }
            }
            if (mDrawFrame) rl.L += 1.f;
          }
        }
        else
        {
          // no space, draw very short marks
          rr = wideLabMark;
          rr.R = labelDx + 0.5f * mR.W();
          rr.L = rr.R - 2.f;
          if (!UnitsDB(ch)) // dB often have minus signs
          {
            rl = wideLabMark;
            rl.L = labelDx - +0.5f * mR.W() + 1.f;
            rl.R = rl.L + 2.f;
          }
        }
      }

      auto DrawLine = [&] (IRECT lr)
      {
        if (mDrawShadows)
        {
          auto sr = ShiftRectBy(lr, 2.f, 1.f);
          g.FillRect(shadowColor, sr);
        }
        g.FillRect(mMarkText.mTextEntryBGColor, lr);
      };

      auto DrawMark = [&] (float h, bool ignoreLabels = false)
      {
        auto r = RectForHorLine(h);
        if (ignoreLabels || (lastLabBot < r.B))
          DrawLine(r);
      };

      auto DrawMarkWithLabel = [&] (float h, double v)
      {
        wideLabMark = ShiftRectVerticallyAt(wideLabMark, h);
        lLabMark = ShiftRectVerticallyAt(lLabMark, h);
        rLabMark = ShiftRectVerticallyAt(rLabMark, h);

        auto tr = ShiftRectVerticallyAt(LabTxtRect, h - mMarkText.mSize / 2);
        //g.DrawRect(COLOR_RED, tr);

        // first form a string
        WDL_String ltxt;
        ltxt.SetFormatted(8, PrecisionString(ch).Get(), v);
        RemoveTrailingZeroes(&ltxt);

        bool drawLabel = false;
        if (lastLabBot - maxLabOverlap < tr.T)
        {
          lastLabBot = tr.B;
          drawLabel = true;
        }

        // then draw level lines
        if (align != 'c')
          DrawLine(wideLabMark);

        else   // draw shorter level lines around the label or one long line if labels overlap vertically
        {
          if (drawLabel)
          {
            DrawLine(lLabMark);
            DrawLine(rLabMark);
          }
          else if (lastLabBot < wideLabMark.B)  // skipped label, drawing only line
            DrawLine(wideLabMark);              // if it doesnt intersect prev. drawn label
        }

        if (drawLabel)
        {
          // finally draw the string
          if (mDrawShadows)
          {
            auto tt = mTxt;
            tt.mFGColor = shadowColor;
            auto sr = ShiftRectBy(tr, 1.f, 1.f);
            g.DrawText(tt, ltxt.Get(), sr);
          }
          g.DrawText(mTxt, ltxt.Get(), tr);
        }
      };

      for (int m = 0; m != MarksPtr(ch)->GetSize(); ++m)
      {
        auto v = Mark(ch, m);
        if (v > MinDisplayVal(ch) && v < MaxDisplayVal(ch))
        {
          // don't draw marks that are outside of the display range
          auto h = GetVCoordFromValInMeterRect(ch, v, mR);
          h = trunc(h); // at least on LICE nonintegers look bad
          if (h < mR.B && h > mR.T)
          {
            // don't draw right on T or B
            if (UnitsDB(ch)) v = AmpToDB(v);
            if (MarkLabel(ch, m))
              DrawMarkWithLabel(h, v);
            else
              DrawMark(h, align != 'c');
          }
        }
      }
    }
}

void IVMeterControl::RecalcMaxLabelLen(int ch)
{
// this alg is almost the same as the one used in marks drawing
  auto maxL = 0.f;
  auto mR = GetMeterRect(ch);
  for (int m = 0; m != MarksPtr(ch)->GetSize(); ++m)
  {
    auto len = 0.f;
    auto th = 0.f;
    if (MarkLabel(ch, m))
    {
      auto v = Mark(ch, m);
      if (v > MinDisplayVal(ch) && v < MaxDisplayVal(ch))
      {
        auto h = GetVCoordFromValInMeterRect(ch, v, mR);
        h = trunc(h);
        if (h < mR.B && h > mR.T)
        {
          if (UnitsDB(ch)) v = AmpToDB(v);
          WDL_String l;
          l.SetFormatted(8, PrecisionString(ch).Get(), v);
          RemoveTrailingZeroes(&l);
//          BasicTextMeasure(l.Get(), th, len);
        }
      }
    }
    if (len > maxL)
      maxL = len;
  }
  *MaxLabelLenPtr(ch) = maxL;
}


WDL_String IVMeterControl::PrecisionString(int ch)
{
  WDL_String s;
  s.Set("%4."); // 4 for cases like -160 dB
  s.AppendFormatted(2, "%d", NumDisplPrecision(ch));
  s.Append("f");
  return s;
}

void IVMeterControl::RemoveTrailingZeroes(WDL_String* s, int minDecimalsToKeep)
{
  char* start = s->Get();
  char* p = start;

  while (*p != '.' && *p != '\0') // find a dot
    ++p;

  if (*p != '\0') // reverse search the 1st nonzero digit
  {
    auto dotPos = (int) (p - start);
    int delPos = s->GetLength() - 1;
    while (delPos != dotPos)
    {
      char* c = start + delPos;
      if (isdigit(c[0]) && c[0] != '0')
        break;
      else
        --delPos;
    }
    ++delPos;

    if (minDecimalsToKeep == 0)
    {
      if (delPos == dotPos + 1) // if all decimals are 0, we don't need the dot
        --delPos;
    }
    else if (delPos < dotPos + minDecimalsToKeep + 1)
      delPos = dotPos + minDecimalsToKeep + 1;

    if (delPos < s->GetLength())
      s->SetLen(delPos);
  }
}

IColor IVMeterControl::LinearBlendColors(IColor cA, IColor cB, double mix)
{
  IColor cM;
  cM.A = (int) ((1.0 - mix) * cA.A + mix * cB.A);
  cM.R = (int) ((1.0 - mix) * cA.R + mix * cB.R);
  cM.G = (int) ((1.0 - mix) * cA.G + mix * cB.G);
  cM.B = (int) ((1.0 - mix) * cA.B + mix * cB.B);
  return cM;
}

IRECT IVMeterControl::ShiftRectBy(IRECT r, float dx, float dy)
{
  auto sr = r;
  sr.L += dx;
  sr.R += dx;
  sr.T += dy;
  sr.B += dy;
  return sr;
}

IRECT IVMeterControl::ShiftRectVerticallyAt(IRECT r, float y)
{
  auto dy = y - r.T;
  r.T += dy;
  r.B += dy;
  return r;
}

//void IVMeterControl::BasicTextMeasure(const char* txt, float& numLines, float& maxLineWidth)
//{
//  float w = 0.0;
//  maxLineWidth = 0.0;
//  numLines = 0.0;
//  while (true)
//  {
//    if (*txt == '\0' || *txt == '\n')
//    {
//      ++numLines;
//      if (w > maxLineWidth)
//        maxLineWidth = w;
//      if (*txt == '\0')
//        break;
//      w = 0.0;
//    }
//    else
//      ++w;
//    ++txt;
//  }
//}

void IVMeterControl::DrawInnerShadowForRect(IRECT r, IColor shadowColor, IGraphics& g)
{
  auto& o = mShadowOffset;
  auto slr = r;
  slr.R = slr.L + o;
  auto str = r;
  str.L += o;
  str.B = str.T + o;
  g.FillRect(shadowColor, slr);
  g.FillRect(shadowColor, str);
}
