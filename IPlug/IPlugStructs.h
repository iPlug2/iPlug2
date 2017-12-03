#pragma once

#include <algorithm>
#include "wdlstring.h"

#include "IPlugConstants.h"
#include "IPlugByteChunk.h"
#include "IPlugOSDetect.h"

struct ChannelIO
{
  int mIn, mOut;
  ChannelIO(int nIn, int nOut) : mIn(nIn), mOut(nOut) {}
};


struct IMidiMsg
{
  int mOffset;
  BYTE mStatus, mData1, mData2;

  enum EStatusMsg
  {
    kNone = 0,
    kNoteOff = 8,
    kNoteOn = 9,
    kPolyAftertouch = 10,
    kControlChange = 11,
    kProgramChange = 12,
    kChannelAftertouch = 13,
    kPitchWheel = 14
  };

  enum EControlChangeMsg
  {
    kModWheel = 1,
    kBreathController = 2,
    kUndefined003 = 3,
    kFootController = 4,
    kPortamentoTime = 5,
    kChannelVolume = 7,
    kBalance = 8,
    kUndefined009 = 9,
    kPan = 10,
    kExpressionController = 11,
    kEffectControl1 = 12,
    kEffectControl2 = 13,
    kUndefined014 = 14,
    kUndefined015 = 15,
    kGeneralPurposeController1 = 16,
    kGeneralPurposeController2 = 17,
    kGeneralPurposeController3 = 18,
    kGeneralPurposeController4 = 19,
    kUndefined020 = 20,
    kUndefined021 = 21,
    kUndefined022 = 22,
    kUndefined023 = 23,
    kUndefined024 = 24,
    kUndefined025 = 25,
    kUndefined026 = 26,
    kUndefined027 = 27,
    kUndefined028 = 28,
    kUndefined029 = 29,
    kUndefined030 = 30,
    kUndefined031 = 31,
    kSustainOnOff = 64,
    kPortamentoOnOff = 65,
    kSustenutoOnOff = 66,
    kSoftPedalOnOff = 67,
    kLegatoOnOff = 68,
    kHold2OnOff = 69,
    kSoundVariation = 70,
    kResonance = 71,
    kReleaseTime = 72,
    kAttackTime = 73,
    kCutoffFrequency = 74,
    kDecayTime = 75,
    kVibratoRate = 76,
    kVibratoDepth = 77,
    kVibratoDelay = 78,
    kSoundControllerUndefined = 79,
    kUndefined085 = 85,
    kUndefined086 = 86,
    kUndefined087 = 87,
    kUndefined088 = 88,
    kUndefined089 = 89,
    kUndefined090 = 90,
    kTremoloDepth = 92,
    kChorusDepth = 93,
    kPhaserDepth = 95,
    kUndefined102 = 102,
    kUndefined103 = 103,
    kUndefined104 = 104,
    kUndefined105 = 105,
    kUndefined106 = 106,
    kUndefined107 = 107,
    kUndefined108 = 108,
    kUndefined109 = 109,
    kUndefined110 = 110,
    kUndefined111 = 111,
    kUndefined112 = 112,
    kUndefined113 = 113,
    kUndefined114 = 114,
    kUndefined115 = 115,
    kUndefined116 = 116,
    kUndefined117 = 117,
    kUndefined118 = 118,
    kUndefined119 = 119,
    kAllNotesOff = 123
  };

  IMidiMsg(int offs = 0, BYTE s = 0, BYTE d1 = 0, BYTE d2 = 0) : mOffset(offs), mStatus(s), mData1(d1), mData2(d2) {}

  void MakeNoteOnMsg(int noteNumber, int velocity, int offset, int channel=0);
  void MakeNoteOffMsg(int noteNumber, int offset, int channel=0);
  void MakePitchWheelMsg(double value, int channel=0);  // Value in [-1, 1], converts to [0, 16384) where 8192 = no pitch change.
  void MakeControlChangeMsg(EControlChangeMsg idx, double value, int channel=0);           //  Value in [0, 1].
  int Channel() const; // returns [0, 15] for midi channels 1 ... 16

  EStatusMsg StatusMsg() const;
  int NoteNumber() const;     // Returns [0, 127), -1 if NA.
  int Velocity() const;       // Returns [0, 127), -1 if NA.
  int PolyAfterTouch() const;       // Returns [0, 127), -1 if NA.
  int ChannelAfterTouch() const;       // Returns [0, 127), -1 if NA.
  int Program() const;        // Returns [0, 127), -1 if NA.
  double PitchWheel() const;  // Returns [-1.0, 1.0], zero if NA.
  EControlChangeMsg ControlChangeIdx() const;
  double ControlChange(EControlChangeMsg idx) const;      // return [0, 1], -1 if NA.
  static bool ControlChangeOnOff(double msgValue) { return (msgValue >= 0.5); }  // true = on.
  void Clear();
  void LogMsg();
};

struct ITimeInfo
{
  double mTempo;
  double mSamplePos;
  double mPPQPos;
  double mLastBar;
  double mCycleStart;
  double mCycleEnd;

  int mNumerator;
  int mDenominator;

  bool mTransportIsRunning;
  bool mTransportLoopEnabled;

  ITimeInfo()
  {
    mSamplePos = mSamplePos = mTempo = mPPQPos = mLastBar = mCycleStart = mCycleEnd = -1.0;
    mTempo = 120.;
    mNumerator = mDenominator = 4;
    mTransportIsRunning = mTransportLoopEnabled = false;
  }
};

struct ISysEx
{
  int mOffset, mSize;
  const BYTE* mData;

  ISysEx(int offs = 0, const BYTE* pData = NULL, int size = 0) : mOffset(offs), mData(pData), mSize(size) {}

  void Clear();
  void LogMsg();
};

struct IPreset
{
  bool mInitialized;
  char mName[MAX_PRESET_NAME_LEN];

  ByteChunk mChunk;

  IPreset(int idx)
    : mInitialized(false)
  {
    sprintf(mName, "%s", UNUSED_PRESET_NAME);
  }
};
