#ifndef _IPLUGSTRUCTS_
#define _IPLUGSTRUCTS_

#include "Containers.h"
#include "IPlugOSDetect.h"

#ifndef OS_IOS
#include "../swell/swell.h"
#include "../lice/lice_text.h"

enum EFileAction { kFileOpen, kFileSave };

struct IBitmap
{
  void* mData;
  int W, H, N;    // N = number of states (for multibitmaps).
  bool mFramesAreHorizontal;
  IBitmap(void* pData = 0, int w = 0, int h = 0, int n = 1, bool framesAreHorizontal = false)
    : mData(pData)
    , W(w)
    , H(h)
    , N(n)
    , mFramesAreHorizontal(framesAreHorizontal)
  {}

  inline int frameHeight() const { return H / N; }
};

struct IColor
{
  int A, R, G, B;
  IColor(int a = 255, int r = 0, int g = 0, int b = 0) : A(a), R(r), G(g), B(b) {}
  bool operator==(const IColor& rhs) { return (rhs.A == A && rhs.R == R && rhs.G == G && rhs.B == B); }
  bool operator!=(const IColor& rhs) { return !operator==(rhs); }
  bool Empty() const { return A == 0 && R == 0 && G == 0 && B == 0; }
  void Clamp() { A = IPMIN(A, 255); R = IPMIN(R, 255); G = IPMIN(G, 255); B = IPMIN(B, 255); }
};

const IColor COLOR_TRANSPARENT(0, 0, 0, 0);
const IColor COLOR_BLACK(255, 0, 0, 0);
const IColor COLOR_GRAY(255, 127, 127, 127);
const IColor COLOR_WHITE(255, 255, 255, 255);
const IColor COLOR_RED(255, 255, 0, 0);
const IColor COLOR_GREEN(255, 0, 255, 0);
const IColor COLOR_BLUE(255, 0, 0, 255);
const IColor COLOR_YELLOW(255, 255, 255, 0);
const IColor COLOR_ORANGE(255, 255, 127, 0);

struct IChannelBlend
{
  enum EBlendMethod
  {
    kBlendNone,   // Copy over whatever is already there, but look at src alpha.
    kBlendClobber,  // Copy completely over whatever is already there.
    kBlendAdd,
    kBlendColorDodge,
    // etc
  };
  EBlendMethod mMethod;
  float mWeight;

  IChannelBlend(EBlendMethod method = kBlendNone, float weight = 1.0f) : mMethod(method), mWeight(weight) {}
};

const IColor DEFAULT_TEXT_COLOR = COLOR_BLACK;
const IColor DEFAULT_TEXT_ENTRY_BGCOLOR = COLOR_WHITE;
const IColor DEFAULT_TEXT_ENTRY_FGCOLOR = COLOR_BLACK;

#ifdef OS_WIN
  const char* const DEFAULT_FONT = "Verdana";
  const int DEFAULT_TEXT_SIZE = 12;
#elif defined OS_OSX
  const char* const DEFAULT_FONT = "Monaco";
  const int DEFAULT_TEXT_SIZE = 10;
#endif

const int FONT_LEN = 32;

struct IText
{
  char mFont[FONT_LEN];
  int mSize;
  IColor mColor, mTextEntryBGColor, mTextEntryFGColor;
  enum EStyle { kStyleNormal, kStyleBold, kStyleItalic } mStyle;
  enum EAlign { kAlignNear, kAlignCenter, kAlignFar } mAlign;
  int mOrientation;   // Degrees ccwise from normal.
  enum EQuality { kQualityDefault, kQualityNonAntiAliased, kQualityAntiAliased, kQualityClearType } mQuality;
  LICE_IFont* mCached;

  IText(int size = DEFAULT_TEXT_SIZE,
        const IColor* pColor = 0,
        char* font = 0,
        EStyle style = kStyleNormal,
        EAlign align = kAlignCenter,
        int orientation = 0,
        EQuality quality = kQualityDefault,
        const IColor* pTEBGColor = 0,
        const IColor* pTEFGColor = 0)
    : mSize(size)
    , mColor(pColor ? *pColor : DEFAULT_TEXT_COLOR)
    , mStyle(style)
    , mAlign(align)
    , mOrientation(orientation)
    , mQuality(quality)
    , mCached(0)
    , mTextEntryBGColor(pTEBGColor ? *pTEBGColor : DEFAULT_TEXT_ENTRY_BGCOLOR)
    , mTextEntryFGColor(pTEFGColor ? *pTEFGColor : DEFAULT_TEXT_ENTRY_FGCOLOR)
  {
    strcpy(mFont, (font ? font : DEFAULT_FONT));
  }

  IText(const IColor* pColor)
    : mSize(DEFAULT_TEXT_SIZE)
    , mColor(*pColor)
    , mStyle(kStyleNormal)
    , mAlign(kAlignCenter)
    , mOrientation(0)
    , mQuality(kQualityDefault)
    , mCached(0)
    , mTextEntryBGColor(DEFAULT_TEXT_ENTRY_BGCOLOR)
    , mTextEntryFGColor(DEFAULT_TEXT_ENTRY_FGCOLOR)
  {
    strcpy(mFont, DEFAULT_FONT);
  }

};

// these are macros to shorten the instantiation of IControls
// for a paramater ID MyParam, define constants named MyParam_X, MyParam_Y, MyParam_W, MyParam_H to specify the Control's IRect
// then when instantiating a Control you can just call MakeIRect(MyParam) to specify the IRect
#define MakeIRect(a) IRECT(a##_X, a##_Y, a##_X + a##_W, a##_Y + a##_H)
#define MakeIRectHOffset(a, xoffs) IRECT(a##_X + xoffs, a##_Y, a##_X + a##_W + xoffs, a##_Y + a##_H)
#define MakeIRectVOffset(a, yoffs) IRECT(a##_X, a##_Y + yoffs, a##_X + a##_W, a##_Y + a##_H + yoffs)
#define MakeIRectHVOffset(a, xoffs, yoffs) IRECT(a##_X + xoffs, a##_Y + yoffs, a##_X + a##_W + xoffs, a##_Y + a##_H + yoffs)

struct IRECT
{
  int L, T, R, B;

  IRECT() { L = T = R = B = 0; }
  IRECT(int l, int t, int r, int b) : L(l), R(r), T(t), B(b) {}
  IRECT(int x, int y, IBitmap* pBitmap) : L(x), T(y), R(x + pBitmap->W), B(y + pBitmap->H / pBitmap->N) {}

  bool Empty() const
  {
    return (L == 0 && T == 0 && R == 0 && B == 0);
  }

  void Clear()
  {
    L = T = R = B = 0;
  }

  bool operator==(const IRECT& rhs) const
  {
    return (L == rhs.L && T == rhs.T && R == rhs.R && B == rhs.B);
  }

  bool operator!=(const IRECT& rhs) const
  {
    return !(*this == rhs);
  }

  inline int W() const { return R - L; }
  inline int H() const { return B - T; }
  inline float MW() const { return 0.5f * (float) (L + R); }
  inline float MH() const { return 0.5f * (float) (T + B); }

  inline IRECT Union(IRECT* pRHS)
  {
    if (Empty()) { return *pRHS; }
    if (pRHS->Empty()) { return *this; }
    return IRECT(IPMIN(L, pRHS->L), IPMIN(T, pRHS->T), IPMAX(R, pRHS->R), IPMAX(B, pRHS->B));
  }

  inline IRECT Intersect(IRECT* pRHS)
  {
    if (Intersects(pRHS))
    {
      return IRECT(IPMAX(L, pRHS->L), IPMAX(T, pRHS->T), IPMIN(R, pRHS->R), IPMIN(B, pRHS->B));
    }
    return IRECT();
  }

  inline bool Intersects(IRECT* pRHS)
  {
    return (!Empty() && !pRHS->Empty() && R >= pRHS->L && L < pRHS->R && B >= pRHS->T && T < pRHS->B);
  }

  inline bool Contains(IRECT* pRHS)
  {
    return (!Empty() && !pRHS->Empty() && pRHS->L >= L && pRHS->R <= R && pRHS->T >= T && pRHS->B <= B);
  }

  inline bool Contains(int x, int y)
  {
    return (!Empty() && x >= L && x < R && y >= T && y < B);
  }

  inline void Constrain(int* x, int* y)
  {
    if (*x < L)
    {
      *x = L;
    }
    else if (*x > R)
    {
      *x = R;
    }

    if (*y < T)
    {
      *y = T;
    }
    else if (*y > B)
    {
      *y = B;
    }
  }

  inline IRECT SubRectVertical(int numSlices, int sliceIdx)
  {
    int heightOfSubRect = (H() / numSlices);
    int t = heightOfSubRect * sliceIdx;

    return IRECT(L, T + t, R, T + t + heightOfSubRect);
  }

  inline IRECT SubRectHorizontal(int numSlices, int sliceIdx)
  {
    int widthOfSubRect = (W() / numSlices);
    int l = widthOfSubRect * sliceIdx;

    return IRECT(L + l, T, L + l + widthOfSubRect, B);
  }
  
  inline IRECT GetPadded(int padding)
  {
    return IRECT(L-padding, T-padding, R+padding, B+padding);
  }
  
  inline IRECT GetPadded(int padL, int padT, int padR, int padB)
  {
    return IRECT(L+padL, T+padT, R+padR, B+padB);
  }
  
  inline IRECT GetHPadded(int padding)
  {
    return IRECT(L-padding, T, R+padding, B);
  }

  inline IRECT GetVPadded(int padding)
  {
    return IRECT(L, T-padding, R, B+padding);
  }
  
  void Clank(IRECT* pRHS)
  {
    if (L < pRHS->L)
    {
      R = IPMIN(pRHS->R - 1, R + pRHS->L - L);
      L = pRHS->L;
    }
    if (T < pRHS->T)
    {
      B = IPMIN(pRHS->B - 1, B + pRHS->T - T);
      T = pRHS->T;
    }
    if (R >= pRHS->R)
    {
      L = IPMAX(pRHS->L, L - (R - pRHS->R + 1));
      R = pRHS->R - 1;
    }
    if (B >= pRHS->B)
    {
      T = IPMAX(pRHS->T, T - (B - pRHS->B + 1));
      B = pRHS->B - 1;
    }
  }
};

struct IMouseMod
{
  bool L, R, S, C, A;
  IMouseMod(bool l = false, bool r = false, bool s = false, bool c = false, bool a = false)
    : L(l), R(r), S(s), C(c), A(a) {}
};

#endif // !OS_IOS

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
  int Channel(); // returns [0, 15] for midi channels 1 ... 16

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

const int MAX_PRESET_NAME_LEN = 256;
#define UNUSED_PRESET_NAME "empty"

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

enum
{
  KEY_SPACE,
  KEY_UPARROW,
  KEY_DOWNARROW,
  KEY_LEFTARROW,
  KEY_RIGHTARROW,
  KEY_DIGIT_0,
  KEY_DIGIT_9=KEY_DIGIT_0+9,
  KEY_ALPHA_A,
  KEY_ALPHA_Z=KEY_ALPHA_A+25
};

#endif