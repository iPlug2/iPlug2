#pragma once

#include "IControl.h"

/*
Vector meter control by Eugene Yakshin, 2018
Eugene.Yakshin [at] ya [dot] ru
*/

class IVMeterControl : public IControl
                     , public IVectorBase
{
public:
  static const IColor DEFAULT_BG_COLOR;
  static const IColor DEFAULT_RAW_COLOR;
  static const IColor DEFAULT_RMS_COLOR;
  static const IColor DEFAULT_PK_COLOR;
  static const IColor DEFAULT_FR_COLOR;

  // map to IVectorBase colors
  enum EVKColor
  {
    mBg = kBG,
    mRaw = kFG,
    mPeak = kHL,
    mFr = kFR,
    mRms = kX1
  };

  IVMeterControl(IDelegate& dlg, IRECT bounds, int nChans, const char* chanNames = 0, ...);
  ~IVMeterControl();

// todo add thread safe channel add/del.
  void SetSampleRate(double sr);
  void Reset(int chId = -1);
  void ResetRMS(int chId = -1);

  // use for raw data
  void ProcessChanValue(double value, int chId = -1);
  void ProcessChan(sample* in, int blockSize, int chId = -1);
  // handy for processing a block of either all ins or all outs
  void ProcessBus(sample** ins, int blockSize, int numChans = -1, int sourceStartId = 0, int destStartId = 0);
  // handy if meter measures both inputs and outputs:
  void ProcessInsOuts(sample** ins, sample** outs, int blockSize, int numIns = -1, int numOuts = -1);

  double GetRMS(int chId);
  double GetMaxPeak(int chId);

  void SetUnitsDB(bool db, int chId = -1, bool scaleFollowsUnits = true);
  void SetScaleLog(bool logScale, int chId = -1);
  void SetMinMaxDisplayValues(double min, double max, int chId = -1);
  void SetNumDisplayPrecision(int precision, int chId = -1);

  void SetOverdriveThreshold(double thresh, int chId = -1);

  void SetHoldPeaks(bool hold)
  {
    mHoldPeaks = hold;
  }

  void SetPeakDropTimeMs(double ms, int chId = -1);
  void SetDrawPeakRect(bool draw, int chId = -1);
  void SetDrawMaxPeak(bool draw, int chId = -1);

  void SetPeakRectHeight(double h)
  {
    mPeakRectHeight = (float) h;
    SetDirty();
  }

  void SetDrawMemRect(bool draw, int chId = -1);

  void SetDrawRMS(bool draw, int chId = -1);
  void SetRMSWindowMs(double ms, int chId = -1);
  void SetAesRmsFix(bool plusThreeDB, int chId = -1);

  void SetDrawChName(bool draw, int chId = -1);
  void SetChanName(int chId, const char* name);
  void SetChanNames(const char* names, ...);

  void SetChNameHOffset(int chId, double offset)
  {
    *ChanNameHOffsetPtr(chId) = (float) offset;
    SetDirty();
  }

  void SetDrawMarks(bool draw, int chId = -1);
  void SetLevelMarks(const char* marks, int chId = -1);
  void SetMarkWidthRatio(float r, int chId);
  void SetMarksAlignment(char alignment, int chId = -1, bool shortenMarksForSides = true);
  void SetMarksHOffset(float offset, int chId = -1);

  void SetMeterWidth(double w, int chId = -1);
  void SetDistToTheNextMeter(double d, int chId = -1, bool compactLabels = true, bool glueRects = true);

  // note that the color of marks' lines is mMarkText.mTextEntryBGColor,
  // whereas the color of marks' values is mMarkText.mFGColor,
  // so you can make them different.
  void SetText(IText& txt)
  {
    SetTexts(txt, txt);
  }

  void SetTexts(IText chNameTxt, IText marksTxt);

  void SetDrawBorders(bool draw)
  {
    mDrawFrame = draw;
    SetDirty();
  }

  void SetDrawShadows(bool draw)
  {
    mDrawShadows = draw;
    SetDirty();
  }

  void Draw(IGraphics& g) override;

  //TODO wrong
  void SetDirty(bool triggerAction = false) override
  {
    mDirty = true;
  }

  void OnResize() override;

  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override;
  void OnMouseDown(float x, float y, const IMouseMod& mod) override;

protected:

  double mSampleRate = DEFAULT_SAMPLE_RATE;

  struct ChannelSpecificData
  {
    // all signal related vals are stored as abs vals and converted to dB if needed
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
    // But scientific RMS of a sine is X/sqrt(2) dB.
    // AES RMS is just a traditional inexact reference.
    double RMSWindowMs = 300.0;
    WDL_TypedBuf<double>* RMSBuf = nullptr;
    size_t RMSBufPos = 0; // position in the buffer
    double RMSSum = 0.0;

    float meterWidth = 20.f;
    float distToNextM = 1.f; // use for gluing chans into groups like ins and outs

    bool drawMemRect = true;
    bool drawPeakRect = true;
    bool drawMaxPeak = true;

    bool drawMarks = true;
    float markWidthR = 0.6f;
    WDL_TypedBuf<double>* marks = nullptr;
    WDL_TypedBuf<bool>* markLabels = nullptr;
    char marksAlign = 'c'; // 'l'eft, 'c'enter, 'r'ight
    float marksOffset = 0.f; // mainly used for common marks for adjacent meters
    float maxLabelLen = 0.f;

    int numDisplPrecision = 2;
    bool drawChanName = true;
    float chanNameHOffset = 0.f; // e.g. one "IN L/R" label for adjacent meters
    WDL_String* chanName = nullptr;
  };

  WDL_TypedBuf<ChannelSpecificData> mChanData;

  int NumChannels()
  {
    return mChanData.GetSize();
  }

  float mMeterHeight = 150.f; // including the peak rect
  float mPeakRectHeight = 15.f;
  bool mHoldPeaks = true; // useful in audio debugging

  float mChNameMaxH = 0.f;

  IText mMarkText = mText;

  bool mDrawShadows = true;
  float mShadowOffset = 3.f;
  bool mDrawFrame = true;

  double GetExpForDrop(double ms, double fps)
  {
    return pow(0.01, 1000.0 / (ms * fps)); // musicdsp.org "envelope follower" discussion
  }
  
  double GetInvExpForDrop(double ms, double fps)
  {
    return pow(0.01, 1000.0 / (-ms * 2.0 * fps)); // 2.0 is empirical adjustment, nicer visually
  }
  
  double GetPeakFromMemExp(int chId)
  {
    return MemPeak(chId) - (MemExp(chId) - 1.0);
  }

  void ZeroRMSBuf(int ch)
  {
    for (int i = 0; i != RMSBufLen(ch); ++i)
      *RMSBufValPtr(ch, i) = 0.0;
  }

  IRECT GetMeterRect(int i, bool withODRect = false);
  IRECT GetControlRectFromChannelsData(bool keepPosition = true);

  float GetVCoordFromValInMeterRect(int chId, double v, IRECT meterR);

  float GetLeftMarginAbs();
  float GetRightMarginAbs();
  void UpdateMargins(float oldL, float oldR);

  void SetChanNames(const char* names, va_list args);
  void RecalcMaxChNameH(float oldAbsH);

  void DrawMarks(IGraphics& g);
  void RecalcMaxLabelLen(int ch);

  WDL_String PrecisionString(int ch);
  void RemoveTrailingZeroes(WDL_String* s, int minDecimalsToKeep = 0);
  IColor LinearBlendColors(IColor cA, IColor cB, double mix);
  IRECT ShiftRectBy(IRECT r, float dx, float dy = 0.0);
  IRECT ShiftRectVerticallyAt(IRECT r, float y);
//  void BasicTextMeasure(const char* txt, float& numLines, float& maxLineWidth);
  void DrawInnerShadowForRect(IRECT r, IColor shadowColor, IGraphics& g);

  // getters
  ChannelSpecificData* Ch(int i) { return mChanData.Get() + i; }

  double* RawValuePtr(int i)                { return &(Ch(i)->rawValue); }
  double RawValue(int i)                    { return *RawValuePtr(i); }
  double* MinDisplayValPtr(int i)           { return &(Ch(i)->minDisplayVal); }
  double MinDisplayVal(int i)               { return *MinDisplayValPtr(i); }
  double* MaxDisplayValPtr(int i)           { return &(Ch(i)->maxDisplayVal); }
  double MaxDisplayVal(int i)               { return *MaxDisplayValPtr(i); }

  double* OverThreshPtr(int i)              { return &(Ch(i)->overThresh); }
  double OverThresh(int i)                  { return *OverThreshPtr(i); }

  bool* UnitsDBPtr(int i)                   { return &(Ch(i)->unitsDB); }
  bool UnitsDB(int i)                       { return *UnitsDBPtr(i); }
  bool* ScaleLogPtr(int i)                  { return &(Ch(i)->scaleLog); }
  bool ScaleLog(int i)                      { return *ScaleLogPtr(i); }

  double* DropMsPtr(int i)                  { return &(Ch(i)->dropMs); }
  double DropMs(int i)                      { return *DropMsPtr(i); }

  bool* HoldingAPeakPtr(int i)              { return &(Ch(i)->holdingAPeak); }
  bool HoldingAPeak(int i)                  { return *HoldingAPeakPtr(i); }
  size_t* PeakSampHeldPtr(int i)            { return &(Ch(i)->peakSampHeld); }
  size_t PeakSampHeld(int i)                { return *PeakSampHeldPtr(i); }

  double* OverBlinkPtr(int i)               { return &(Ch(i)->overBlink); }
  double OverBlink(int i)                   { return *OverBlinkPtr(i); }

  bool* DrawPeakRectPtr(int i)              { return &(Ch(i)->drawPeakRect); }
  bool DrawPeakRect(int i)                  { return *DrawPeakRectPtr(i); }
  bool* DrawMaxPeakPtr(int i)               { return &(Ch(i)->drawMaxPeak); }
  bool DrawMaxPeak(int i)                   { return *DrawMaxPeakPtr(i); }
  double* MaxPeakPtr(int i)                 { return &(Ch(i)->maxPeak); }
  double MaxPeak(int i)                     { return *MaxPeakPtr(i); }

  bool* DrawMemRectPtr(int i)               { return &(Ch(i)->drawMemRect); }
  bool DrawMemRect(int i)                   { return *DrawMemRectPtr(i); }
  double* MemPeakPtr(int i)                 { return &(Ch(i)->memPeak); }
  double MemPeak(int i)                     { return *MemPeakPtr(i); }
  double* MemExpPtr(int i)                  { return &(Ch(i)->memExp); }
  double MemExp(int i)                      { return *MemExpPtr(i); }

  WDL_TypedBuf<double>*  RMSBufPtr(int i)   { return *RMSBufPP(i); }
  WDL_TypedBuf<double>** RMSBufPP(int i)    { return &(Ch(i)->RMSBuf); }
  double* RMSBufValPtr(int ch, int i)       { return RMSBufPtr(ch)->Get() + i; }
  double RMSBufVal(int ch, int i)           { return *RMSBufValPtr(ch, i); }
  size_t RMSBufLen(int i)                   { return RMSBufPtr(i)->GetSize(); }
  size_t* RMSBufPosPtr(int i)               { return &(Ch(i)->RMSBufPos); }
  size_t RMSBufPos(int i)                   { return *RMSBufPosPtr(i); }
  double* RMSWindowMsPtr(int i)             { return &(Ch(i)->RMSWindowMs); }
  double RMSWindowMs(int i)                 { return *RMSWindowMsPtr(i); }
  double* RMSSumPtr(int i)                  { return &(Ch(i)->RMSSum); }
  double RMSSum(int i)                      { return *RMSSumPtr(i); }
  bool* DrawRMSPtr(int i)                   { return &(Ch(i)->drawRMS); }
  bool DrawRMS(int i)                       { return *DrawRMSPtr(i); }
  bool* AESFixPtr(int i)                    { return &(Ch(i)->AESFix); }
  bool AESFix(int i)                        { return *AESFixPtr(i); }

  float* MeterWidthPtr(int i)               { return &(Ch(i)->meterWidth); }
  float MeterWidth(int i)                   { return *MeterWidthPtr(i); }
  float* DistToTheNextMPtr(int i)           { return &(Ch(i)->distToNextM); }
  float DistToTheNextM(int i)               { return *DistToTheNextMPtr(i); }

  bool* DrawMarksPtr(int i)                 { return &(Ch(i)->drawMarks); }
  bool DrawMarks(int i)                     { return *DrawMarksPtr(i); }
  float* MarkWidthRPtr(int i)               { return &(Ch(i)->markWidthR); }
  float MarkWidthR(int i)                   { return *MarkWidthRPtr(i); }
  char* MarksAlignPtr(int i)                { return &(Ch(i)->marksAlign); }
  char MarksAlign(int i)                    { return *MarksAlignPtr(i); }
  float* MarksOffsetPtr(int i)              { return &(Ch(i)->marksOffset); }
  float MarksOffset(int i)                  { return *MarksOffsetPtr(i); }
  float* MaxLabelLenPtr(int i)              { return &(Ch(i)->maxLabelLen); }
  float MaxLabelLen(int i)                  { return *MaxLabelLenPtr(i); }

  WDL_TypedBuf<double>*  MarksPtr(int i)    { return *MarksPP(i); }
  WDL_TypedBuf<double>** MarksPP(int i)     { return &(Ch(i)->marks); }
  double Mark(int ch, int i)                { return *(MarksPtr(ch)->Get() + i); }

  WDL_TypedBuf<bool>*  MarkLabelsPtr(int i) { return *MarkLabelsPP(i); }
  WDL_TypedBuf<bool>** MarkLabelsPP(int i)  { return &(Ch(i)->markLabels); }
  bool MarkLabel(int ch, int i)             { return *(MarkLabelsPtr(ch)->Get() + i); }

  int* NumDisplPrecisionPtr(int i)          { return &(Ch(i)->numDisplPrecision); }
  int NumDisplPrecision(int i)              { return *NumDisplPrecisionPtr(i); }

  bool* DrawChanNamePtr(int i)              { return &(Ch(i)->drawChanName); }
  bool DrawChanName(int i)                  { return *DrawChanNamePtr(i); }
  float* ChanNameHOffsetPtr(int i)          { return &(Ch(i)->chanNameHOffset); }
  float ChanNameHOffset(int i)              { return *ChanNameHOffsetPtr(i); }
  WDL_String* ChanNamePtr(int i)            { return *ChanNamePP(i); }
  WDL_String** ChanNamePP(int i)            { return &(Ch(i)->chanName); }
};
