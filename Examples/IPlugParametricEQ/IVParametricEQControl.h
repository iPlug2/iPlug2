/*
 ==============================================================================

 IVParametricEQControl - Spectrum analyzer with parametric EQ curve overlay

 ==============================================================================
*/

#pragma once

#include "IControl.h"
#include "ISender.h"
#include "IPlugStructs.h"
#include "Extras/SVF.h"
#include <complex>
#include <cmath>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Parametric EQ control with spectrum analyzer and draggable band handles
 * Combines spectrum visualization with EQ frequency response curve
 */
template <int MAXNC = 2, int MAX_FFT_SIZE = 4096, int NUM_BANDS = 5>
class IVParametricEQControl : public IControl
                            , public IVectorBase
{
public:
  enum MsgTags
  {
    kMsgTagSampleRate = 100,
    kMsgTagFFTSize,
    kMsgTagOverlap,
    kMsgTagWindowType,
    kMsgTagBandParams
  };

  static constexpr int kNumPoints = 512; // Points for EQ curve
  using TDataPacket = std::array<float, MAX_FFT_SIZE>;

  // EQ band data for UI
  struct BandInfo
  {
    double freq = 1000.0;
    double gain = 0.0;
    double Q = 1.0;
    int type = SVF<>::kBell;
    bool enabled = true;
  };

  IVParametricEQControl(const IRECT& bounds, const char* label = "", const IVStyle& style = DEFAULT_STYLE,
                        int freqParamIdxStart = 0)
  : IControl(bounds)
  , IVectorBase(style)
  , mFreqParamIdxStart(freqParamIdxStart)
  {
    AttachIControl(this, label);
    SetFFTSize(2048);
    SetFreqRange(20.0f, 20000.0f);
    SetAmpRange(-90.0f, 6.0f);

    // Default band colors
    mBandColors[0] = IColor(255, 200, 100, 100); // Red-ish for low
    mBandColors[1] = IColor(255, 200, 180, 100); // Yellow-ish for low-mid
    mBandColors[2] = IColor(255, 100, 200, 100); // Green for mid
    mBandColors[3] = IColor(255, 100, 180, 200); // Cyan for high-mid
    mBandColors[4] = IColor(255, 150, 100, 200); // Purple for high

    // Default band frequencies
    mBands[0] = {80.0, 0.0, 0.7, SVF<>::kLowPassShelf, true};
    mBands[1] = {250.0, 0.0, 1.0, SVF<>::kBell, true};
    mBands[2] = {1000.0, 0.0, 1.0, SVF<>::kBell, true};
    mBands[3] = {4000.0, 0.0, 1.0, SVF<>::kBell, true};
    mBands[4] = {12000.0, 0.0, 0.7, SVF<>::kHighPassShelf, true};
  }

  void OnInit() override
  {
    // Read initial parameter values
    for (int b = 0; b < NUM_BANDS; b++)
    {
      int baseIdx = mFreqParamIdxStart + b * 5;
      if (GetDelegate())
      {
        mBands[b].freq = GetDelegate()->GetParam(baseIdx)->Value();
        mBands[b].gain = GetDelegate()->GetParam(baseIdx + 1)->Value();
        mBands[b].Q = GetDelegate()->GetParam(baseIdx + 2)->Value();
        mBands[b].type = static_cast<int>(GetDelegate()->GetParam(baseIdx + 3)->Value());
        mBands[b].enabled = GetDelegate()->GetParam(baseIdx + 4)->Value() > 0.5;
      }
    }
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    mWidgetBounds.Constrain(x, y);
    mDragging = false;
    mDragBand = -1;

    // Check if clicking on a band handle
    for (int b = 0; b < NUM_BANDS; b++)
    {
      if (!mBands[b].enabled)
        continue;

      float bx, by;
      GetBandHandlePos(b, bx, by);

      if (std::abs(x - bx) < mHandleRadius + 5 && std::abs(y - by) < mHandleRadius + 5)
      {
        if (mod.R) // Right click - show filter type menu
        {
          ShowFilterTypeMenu(b, x, y);
          return;
        }
        mDragging = true;
        mDragBand = b;
        return;
      }
    }

    // If no handle clicked, show options menu
    if (mod.R)
    {
      ShowOptionsMenu(x, y);
    }
  }

  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    if (!mDragging || mDragBand < 0)
      return;

    mWidgetBounds.Constrain(x, y);

    // Calculate new frequency from X position
    float xNorm = (x - mWidgetBounds.L) / mWidgetBounds.W();
    double freq = XNormToFreq(xNorm);
    freq = Clip(freq, 20.0, 20000.0);

    // Calculate new gain from Y position
    float yNorm = 1.0f - (y - mWidgetBounds.T) / mWidgetBounds.H();
    double gain = mAmpLo + yNorm * (mAmpHi - mAmpLo);
    gain = Clip(gain, -24.0, 24.0);

    // Update band parameters
    mBands[mDragBand].freq = freq;
    mBands[mDragBand].gain = gain;

    // Send parameter changes
    int baseIdx = mFreqParamIdxStart + mDragBand * 5;
    GetDelegate()->SetParameterValueFromUI(baseIdx, freq);
    GetDelegate()->SetParameterValueFromUI(baseIdx + 1, gain);

    SetDirty(true);
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    mDragging = false;
    mDragBand = -1;
  }

  void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override
  {
    // Find which band handle we're near
    for (int b = 0; b < NUM_BANDS; b++)
    {
      if (!mBands[b].enabled)
        continue;

      float bx, by;
      GetBandHandlePos(b, bx, by);

      if (std::abs(x - bx) < mHandleRadius + 10 && std::abs(y - by) < mHandleRadius + 10)
      {
        // Adjust Q with mouse wheel
        double newQ = mBands[b].Q * (1.0 + d * 0.1);
        newQ = Clip(newQ, 0.1, 20.0);
        mBands[b].Q = newQ;

        int baseIdx = mFreqParamIdxStart + b * 5 + 2;
        GetDelegate()->SetParameterValueFromUI(baseIdx, newQ);
        SetDirty(true);
        return;
      }
    }
  }

  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    mWidgetBounds.Constrain(x, y);
    float xNorm = (x - mWidgetBounds.L) / mWidgetBounds.W();
    mCursorFreq = XNormToFreq(xNorm);

    float yNorm = 1.0f - (y - mWidgetBounds.T) / mWidgetBounds.H();
    mCursorAmp = mAmpLo + yNorm * (mAmpHi - mAmpLo);

    // Check if hovering over a band handle
    mHoverBand = -1;
    for (int b = 0; b < NUM_BANDS; b++)
    {
      if (!mBands[b].enabled)
        continue;

      float bx, by;
      GetBandHandlePos(b, bx, by);

      if (std::abs(x - bx) < mHandleRadius + 5 && std::abs(y - by) < mHandleRadius + 5)
      {
        mHoverBand = b;
        break;
      }
    }
  }

  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
  {
    if (!pSelectedMenu)
      return;

    const char* title = pSelectedMenu->GetRootTitle();

    if (strcmp(title, "Filter Type") == 0 && mMenuBand >= 0)
    {
      int type = pSelectedMenu->GetChosenItemIdx();
      if (type >= 0)
      {
        // Map menu index to SVF mode
        static const int typeMap[] = {
          SVF<>::kLowPass,
          SVF<>::kHighPass,
          SVF<>::kBandPass,
          SVF<>::kNotch,
          SVF<>::kPeak,
          SVF<>::kBell,
          SVF<>::kLowPassShelf,
          SVF<>::kHighPassShelf
        };
        mBands[mMenuBand].type = typeMap[type];

        int baseIdx = mFreqParamIdxStart + mMenuBand * 5 + 3;
        GetDelegate()->SetParameterValueFromUI(baseIdx, static_cast<double>(typeMap[type]));
        SetDirty(true);
      }
    }
    else if (strcmp(title, "FFT Size") == 0)
    {
      int fftSize = atoi(pSelectedMenu->GetChosenItem()->GetText());
      GetDelegate()->SendArbitraryMsgFromUI(kMsgTagFFTSize, kNoTag, sizeof(int), &fftSize);
      SetFFTSize(fftSize);
    }
    else if (strcmp(title, "Band Enable") == 0 && mMenuBand >= 0)
    {
      bool enable = pSelectedMenu->GetChosenItemIdx() == 0;
      mBands[mMenuBand].enabled = enable;

      int baseIdx = mFreqParamIdxStart + mMenuBand * 5 + 4;
      GetDelegate()->SetParameterValueFromUI(baseIdx, enable ? 1.0 : 0.0);
      SetDirty(true);
    }

    mMenuBand = -1;
  }

  void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
    SetDirty(false);
  }

  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    IByteStream stream(pData, dataSize);

    if (!IsDisabled() && msgTag == ISender<>::kUpdateMessage)
    {
      ISenderData<MAXNC, TDataPacket> d;
      stream.Get(&d, 0);

      for (auto c = d.chanOffset; c < (d.chanOffset + d.nChans); c++)
      {
        CalculateYPoints(c, d.vals[c]);
      }
    }
    else if (msgTag == kMsgTagSampleRate)
    {
      double sr;
      stream.Get(&sr, 0);
      SetSampleRate(sr);
    }
    else if (msgTag == kMsgTagFFTSize)
    {
      int fftSize;
      stream.Get(&fftSize, 0);
      SetFFTSize(fftSize);
    }
    else if (msgTag == kMsgTagBandParams)
    {
      // Receive all band parameters
      for (int b = 0; b < NUM_BANDS; b++)
      {
        int baseIdx = mFreqParamIdxStart + b * 5;
        mBands[b].freq = GetDelegate()->GetParam(baseIdx)->Value();
        mBands[b].gain = GetDelegate()->GetParam(baseIdx + 1)->Value();
        mBands[b].Q = GetDelegate()->GetParam(baseIdx + 2)->Value();
        mBands[b].type = static_cast<int>(GetDelegate()->GetParam(baseIdx + 3)->Value());
        mBands[b].enabled = GetDelegate()->GetParam(baseIdx + 4)->Value() > 0.5;
      }
      SetDirty(true);
    }
  }

  void OnParamChangeForControl(int paramIdx)
  {
    // Determine which band and which parameter changed
    int relIdx = paramIdx - mFreqParamIdxStart;
    if (relIdx < 0 || relIdx >= NUM_BANDS * 5)
      return;

    int band = relIdx / 5;
    int param = relIdx % 5;

    if (GetDelegate())
    {
      double val = GetDelegate()->GetParam(paramIdx)->Value();
      switch (param)
      {
        case 0: mBands[band].freq = val; break;
        case 1: mBands[band].gain = val; break;
        case 2: mBands[band].Q = val; break;
        case 3: mBands[band].type = static_cast<int>(val); break;
        case 4: mBands[band].enabled = val > 0.5; break;
      }
    }
    SetDirty(false);
  }

  void Draw(IGraphics& g) override
  {
    DrawBackground(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);

    if (mStyle.drawFrame)
      g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);
  }

private:
  void DrawWidget(IGraphics& g) override
  {
    // Draw grid
    DrawGrids(g);

    // Draw spectrum (sum of channels)
    DrawSpectrum(g);

    // Draw EQ response curve
    DrawEQCurve(g);

    // Draw band handles
    DrawBandHandles(g);

    // Draw cursor info
    DrawCursorValues(g);
  }

  void DrawGrids(IGraphics& g)
  {
    const IColor gridColor = GetColor(kFG).WithOpacity(0.3f);

    // Frequency grid (logarithmic)
    const double freqLines[] = {20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000};
    for (double freq : freqLines)
    {
      if (freq < mFreqLo || freq > mFreqHi)
        continue;
      float x = mWidgetBounds.L + FreqToXNorm(freq) * mWidgetBounds.W();
      g.DrawLine(gridColor, x, mWidgetBounds.T, x, mWidgetBounds.B, 0, 1.f);

      // Label major frequencies
      if (freq == 100 || freq == 1000 || freq == 10000)
      {
        WDL_String label;
        if (freq >= 1000)
          label.SetFormatted(16, "%dk", static_cast<int>(freq / 1000));
        else
          label.SetFormatted(16, "%d", static_cast<int>(freq));

        IText txt = mStyle.valueText.WithSize(10).WithFGColor(gridColor.WithOpacity(0.7f));
        g.DrawText(txt, label.Get(), IRECT(x - 15, mWidgetBounds.B - 15, x + 15, mWidgetBounds.B));
      }
    }

    // Amplitude grid
    for (float db = -80.f; db <= 10.f; db += 10.f)
    {
      if (db < mAmpLo || db > mAmpHi)
        continue;
      float yNorm = (db - mAmpLo) / (mAmpHi - mAmpLo);
      float y = mWidgetBounds.B - yNorm * mWidgetBounds.H();
      g.DrawLine(db == 0 ? gridColor.WithOpacity(0.6f) : gridColor, mWidgetBounds.L, y, mWidgetBounds.R, y, 0, 1.f);

      // Label
      WDL_String label;
      label.SetFormatted(16, "%+.0f", db);
      IText txt = mStyle.valueText.WithSize(10).WithFGColor(gridColor.WithOpacity(0.7f));
      g.DrawText(txt, label.Get(), IRECT(mWidgetBounds.L + 2, y - 7, mWidgetBounds.L + 30, y + 7));
    }
  }

  void DrawSpectrum(IGraphics& g)
  {
    const int nBins = NumBins();
    if (nBins < 2)
      return;

    // Draw spectrum as filled path
    g.PathClear();
    float x0 = mWidgetBounds.L + mXPoints[0] * mWidgetBounds.W();
    float y0 = mWidgetBounds.B - mYPoints[0][0] * mWidgetBounds.H();
    g.PathMoveTo(x0, y0);

    for (int i = 1; i < nBins; i++)
    {
      float x = mWidgetBounds.L + mXPoints[i] * mWidgetBounds.W();
      float y = mWidgetBounds.B - mYPoints[0][i] * mWidgetBounds.H();
      g.PathLineTo(x, y);
    }

    // Close path to bottom
    g.PathLineTo(mWidgetBounds.R, mWidgetBounds.B);
    g.PathLineTo(mWidgetBounds.L, mWidgetBounds.B);
    g.PathClose();

    // Fill with gradient
    IColor specColor(180, 60, 120, 180);
    IPattern fill = IPattern::CreateLinearGradient(mWidgetBounds, EDirection::Vertical,
      {IColorStop(specColor.WithOpacity(0.5f), 0.f), IColorStop(specColor.WithOpacity(0.1f), 1.f)});
    g.PathFill(fill);

    // Stroke the curve
    g.PathClear();
    g.PathMoveTo(x0, y0);
    for (int i = 1; i < nBins; i++)
    {
      float x = mWidgetBounds.L + mXPoints[i] * mWidgetBounds.W();
      float y = mWidgetBounds.B - mYPoints[0][i] * mWidgetBounds.H();
      g.PathLineTo(x, y);
    }
    g.PathStroke(IPattern(specColor), 1.5f);
  }

  void DrawEQCurve(IGraphics& g)
  {
    // Calculate combined EQ response
    std::array<double, kNumPoints> response;
    for (int i = 0; i < kNumPoints; i++)
    {
      response[i] = 0.0; // dB, starts at 0
    }

    // Sum response from each enabled band
    for (int b = 0; b < NUM_BANDS; b++)
    {
      if (!mBands[b].enabled)
        continue;

      for (int i = 0; i < kNumPoints; i++)
      {
        double xNorm = static_cast<double>(i) / (kNumPoints - 1);
        double freq = XNormToFreq(xNorm);
        double bandResponse = CalculateFilterResponse(mBands[b], freq);
        response[i] += bandResponse;
      }
    }

    // Draw the combined curve
    g.PathClear();
    bool firstPoint = true;

    for (int i = 0; i < kNumPoints; i++)
    {
      double xNorm = static_cast<double>(i) / (kNumPoints - 1);
      float x = mWidgetBounds.L + static_cast<float>(xNorm) * mWidgetBounds.W();

      // Convert dB to y position
      double yNorm = (response[i] - mAmpLo) / (mAmpHi - mAmpLo);
      yNorm = Clip(yNorm, 0.0, 1.0);
      float y = mWidgetBounds.B - static_cast<float>(yNorm) * mWidgetBounds.H();

      if (firstPoint)
      {
        g.PathMoveTo(x, y);
        firstPoint = false;
      }
      else
      {
        g.PathLineTo(x, y);
      }
    }

    // Stroke with yellow/gold color for EQ curve
    IColor curveColor(255, 220, 200, 80);
    g.PathStroke(IPattern(curveColor), 2.5f);

    // Fill under the curve with semi-transparent color
    g.PathClear();
    firstPoint = true;
    float zeroY = mWidgetBounds.B - ((0.0 - mAmpLo) / (mAmpHi - mAmpLo)) * mWidgetBounds.H();

    for (int i = 0; i < kNumPoints; i++)
    {
      double xNorm = static_cast<double>(i) / (kNumPoints - 1);
      float x = mWidgetBounds.L + static_cast<float>(xNorm) * mWidgetBounds.W();
      double yNorm = (response[i] - mAmpLo) / (mAmpHi - mAmpLo);
      yNorm = Clip(yNorm, 0.0, 1.0);
      float y = mWidgetBounds.B - static_cast<float>(yNorm) * mWidgetBounds.H();

      if (firstPoint)
      {
        g.PathMoveTo(x, y);
        firstPoint = false;
      }
      else
      {
        g.PathLineTo(x, y);
      }
    }

    // Close to zero line
    g.PathLineTo(mWidgetBounds.R, zeroY);
    g.PathLineTo(mWidgetBounds.L, zeroY);
    g.PathClose();
    g.PathFill(IPattern(curveColor.WithOpacity(0.2f)));
  }

  void DrawBandHandles(IGraphics& g)
  {
    for (int b = 0; b < NUM_BANDS; b++)
    {
      if (!mBands[b].enabled)
        continue;

      float x, y;
      GetBandHandlePos(b, x, y);

      IColor handleColor = mBandColors[b];

      // Draw handle circle
      bool isHovered = (mHoverBand == b);
      bool isDragged = (mDragBand == b);

      float radius = mHandleRadius;
      if (isHovered || isDragged)
        radius += 2;

      // Outer ring
      g.DrawCircle(handleColor, x, y, radius, 0, 2.f);

      // Filled center
      g.FillCircle(handleColor.WithOpacity(isDragged ? 0.8f : 0.5f), x, y, radius - 2);

      // Draw band label
      WDL_String label;
      label.SetFormatted(8, "%d", b + 1);
      IText txt = mStyle.valueText.WithSize(10).WithFGColor(COLOR_WHITE);
      g.DrawText(txt, label.Get(), IRECT(x - 5, y - 5, x + 5, y + 5));

      // Draw frequency/gain tooltip when hovered
      if (isHovered || isDragged)
      {
        WDL_String tip;
        if (mBands[b].freq >= 1000)
          tip.SetFormatted(32, "%.1fkHz  %+.1fdB  Q:%.1f",
                          mBands[b].freq / 1000.0, mBands[b].gain, mBands[b].Q);
        else
          tip.SetFormatted(32, "%.0fHz  %+.1fdB  Q:%.1f",
                          mBands[b].freq, mBands[b].gain, mBands[b].Q);

        IText tipTxt = mStyle.valueText.WithSize(12).WithFGColor(COLOR_WHITE);
        IRECT tipRect(x - 60, y - 30, x + 60, y - 15);
        g.FillRect(COLOR_BLACK.WithOpacity(0.7f), tipRect);
        g.DrawText(tipTxt, tip.Get(), tipRect);
      }
    }
  }

  void DrawCursorValues(IGraphics& g)
  {
    if (mCursorFreq < 0)
      return;

    WDL_String label;
    if (mCursorFreq >= 1000)
      label.SetFormatted(32, "%.1fkHz", mCursorFreq / 1000.0);
    else
      label.SetFormatted(32, "%.0fHz", mCursorFreq);

    IText txt = mStyle.valueText.WithFGColor(COLOR_WHITE);
    g.DrawText(txt, label.Get(), mWidgetBounds.GetFromTRHC(80, 40).GetVShifted(5));

    label.SetFormatted(32, "%+.1fdB", mCursorAmp);
    g.DrawText(txt, label.Get(), mWidgetBounds.GetFromTRHC(80, 40).GetVShifted(20));
  }

  void GetBandHandlePos(int band, float& x, float& y) const
  {
    double xNorm = FreqToXNorm(mBands[band].freq);
    x = mWidgetBounds.L + static_cast<float>(xNorm) * mWidgetBounds.W();

    double yNorm = (mBands[band].gain - mAmpLo) / (mAmpHi - mAmpLo);
    y = mWidgetBounds.B - static_cast<float>(yNorm) * mWidgetBounds.H();
  }

  double CalculateFilterResponse(const BandInfo& band, double freq) const
  {
    // Calculate the frequency response magnitude in dB
    using cdouble = std::complex<double>;
    static const cdouble j(0, 1);

    // Bilinear transform: s = (2*fs) * (z-1)/(z+1)
    // At frequency f: z = e^(j*2*pi*f/fs)
    double w = 2.0 * PI * freq / mSampleRate;
    cdouble z = std::exp(j * w);

    // Pre-warp the center frequency
    double wc = 2.0 * mSampleRate * std::tan(PI * band.freq / mSampleRate);
    double Q = band.Q;
    double A = std::pow(10.0, band.gain / 40.0); // sqrt of linear gain

    cdouble H;

    // Calculate H(z) based on filter type using bilinear transform
    cdouble s = 2.0 * mSampleRate * (z - 1.0) / (z + 1.0);

    switch (band.type)
    {
      case SVF<>::kLowPass:
      {
        cdouble denom = s * s + (wc / Q) * s + wc * wc;
        H = (wc * wc) / denom;
        break;
      }
      case SVF<>::kHighPass:
      {
        cdouble denom = s * s + (wc / Q) * s + wc * wc;
        H = (s * s) / denom;
        break;
      }
      case SVF<>::kBandPass:
      {
        cdouble denom = s * s + (wc / Q) * s + wc * wc;
        H = ((wc / Q) * s) / denom;
        break;
      }
      case SVF<>::kNotch:
      {
        cdouble denom = s * s + (wc / Q) * s + wc * wc;
        H = (s * s + wc * wc) / denom;
        break;
      }
      case SVF<>::kPeak:
      {
        cdouble denom = s * s + (wc / Q) * s + wc * wc;
        H = (s * s - wc * wc) / denom;
        break;
      }
      case SVF<>::kBell:
      {
        // Peaking EQ: H(s) = (s^2 + s*(A*wc/Q) + wc^2) / (s^2 + s*(wc/(A*Q)) + wc^2)
        cdouble num = s * s + s * (A * wc / Q) + wc * wc;
        cdouble denom = s * s + s * (wc / (A * Q)) + wc * wc;
        H = num / denom;
        break;
      }
      case SVF<>::kLowPassShelf:
      {
        // Low shelf: boost/cut frequencies below wc
        double sqrtA = std::sqrt(A);
        cdouble num = A * (s * s + sqrtA * (wc / Q) * s + wc * wc);
        cdouble denom = s * s + sqrtA * (wc / Q) * s + A * wc * wc;
        H = num / denom;
        break;
      }
      case SVF<>::kHighPassShelf:
      {
        // High shelf: boost/cut frequencies above wc
        double sqrtA = std::sqrt(A);
        cdouble num = A * (A * s * s + sqrtA * (wc / Q) * s + wc * wc);
        cdouble denom = A * s * s + sqrtA * (wc / Q) * s + wc * wc;
        H = num / denom;
        break;
      }
      default:
        return 0.0;
    }

    double magnitude = std::abs(H);
    if (magnitude < 1e-10)
      magnitude = 1e-10;

    return 20.0 * std::log10(magnitude);
  }

  void ShowFilterTypeMenu(int band, float x, float y)
  {
    mMenuBand = band;
    IPopupMenu menu{"Filter Type", {"LowPass", "HighPass", "BandPass", "Notch", "Peak", "Bell", "LowShelf", "HighShelf"}};

    // Check current type
    static const int typeToIdx[] = {0, 1, 2, 3, 4, 5, 6, 7};
    for (int i = 0; i < 8; i++)
    {
      if (mBands[band].type == i)
        menu.CheckItem(i, true);
    }

    GetUI()->CreatePopupMenu(*this, menu, x, y);
  }

  void ShowOptionsMenu(float x, float y)
  {
    IPopupMenu menu{"Options"};
    auto* pFftMenu = menu.AddItem("FFT Size", new IPopupMenu("FFT Size", {"512", "1024", "2048", "4096"}))->GetSubmenu();

    pFftMenu->CheckItem(0, mFFTSize == 512);
    pFftMenu->CheckItem(1, mFFTSize == 1024);
    pFftMenu->CheckItem(2, mFFTSize == 2048);
    pFftMenu->CheckItem(3, mFFTSize == 4096);

    GetUI()->CreatePopupMenu(*this, menu, x, y);
  }

  double FreqToXNorm(double freq) const
  {
    double logLo = std::log10(mFreqLo);
    double logHi = std::log10(mFreqHi);
    double logFreq = std::log10(std::max(freq, 1.0));
    return (logFreq - logLo) / (logHi - logLo);
  }

  double XNormToFreq(double xNorm) const
  {
    double logLo = std::log10(mFreqLo);
    double logHi = std::log10(mFreqHi);
    return std::pow(10.0, logLo + xNorm * (logHi - logLo));
  }

  void SetFFTSize(int fftSize)
  {
    mFFTSize = fftSize;
    mXPoints.resize(NumBins());
    for (int c = 0; c < MAXNC; c++)
    {
      mYPoints[c].assign(NumBins(), 0.0f);
      mEnvelopeValues[c].assign(NumBins(), 0.0f);
    }
    CalculateXPoints();
    SetDirty(false);
  }

  void SetSampleRate(double sampleRate)
  {
    mSampleRate = sampleRate;
    SetDirty(false);
  }

  void SetFreqRange(float lo, float hi)
  {
    mFreqLo = lo;
    mFreqHi = hi;
    SetDirty(false);
  }

  void SetAmpRange(float lo, float hi)
  {
    mAmpLo = lo;
    mAmpHi = hi;
    SetDirty(false);
  }

  void CalculateXPoints()
  {
    const int nBins = NumBins();
    for (int i = 0; i < nBins; i++)
    {
      double freq = (static_cast<double>(i) / (nBins - 1)) * NyquistFreq();
      freq = std::max(freq, 1.0);
      mXPoints[i] = static_cast<float>(FreqToXNorm(freq));
    }
  }

  void CalculateYPoints(int ch, const TDataPacket& powerSpectrum)
  {
    const int nBins = NumBins();

    for (int i = 0; i < nBins; i++)
    {
      float amp = powerSpectrum[i];
      float dB = 20.f * std::log10(amp + 1e-10f);
      float yNorm = (dB - mAmpLo) / (mAmpHi - mAmpLo);
      yNorm = Clip(yNorm, 0.0f, 1.0f);

      // Smoothing
      float prev = mEnvelopeValues[ch][i];
      float smoothed;
      if (yNorm > prev)
        smoothed = mAttackCoeff * prev + (1.f - mAttackCoeff) * yNorm;
      else
        smoothed = mReleaseCoeff * prev + (1.f - mReleaseCoeff) * yNorm;

      mEnvelopeValues[ch][i] = smoothed;
      mYPoints[ch][i] = smoothed;
    }

    SetDirty(false);
  }

  int NumBins() const { return mFFTSize / 2; }
  double NyquistFreq() const { return mSampleRate * 0.5; }

private:
  int mFreqParamIdxStart = 0;
  BandInfo mBands[NUM_BANDS];
  IColor mBandColors[NUM_BANDS];

  double mSampleRate = 44100.0;
  int mFFTSize = 2048;
  float mFreqLo = 20.0f;
  float mFreqHi = 20000.0f;
  float mAmpLo = -90.0f;
  float mAmpHi = 6.0f;

  std::vector<float> mXPoints;
  std::array<std::vector<float>, MAXNC> mYPoints;
  std::array<std::vector<float>, MAXNC> mEnvelopeValues;
  float mAttackCoeff = 0.5f;
  float mReleaseCoeff = 0.95f;

  float mCursorFreq = -1.0f;
  float mCursorAmp = 0.0f;

  bool mDragging = false;
  int mDragBand = -1;
  int mHoverBand = -1;
  int mMenuBand = -1;
  float mHandleRadius = 10.0f;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
