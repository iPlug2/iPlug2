/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @ingroup Controls
 * @copydoc IVGoniometerControl
 */

#include "IControl.h"
#include "ISender.h"
#include "IPlugStructs.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Vectorial goniometer/vectorscope control for visualizing stereo audio
 *
 * A goniometer displays the stereo field by plotting L/R sample pairs as X/Y coordinates,
 * rotated 45 degrees so that:
 * - Vertical axis (M) = Mid = (L+R)/sqrt(2) - mono content
 * - Horizontal axis (S) = Side = (L-R)/sqrt(2) - stereo width
 *
 * Features:
 * - Configurable point size and decay
 * - Optional correlation meter showing phase coherence
 * - L/R/M/S axis labels
 * - Crosshair grid overlay
 *
 * Use with IGoniometerSender to receive stereo data from the audio thread.
 *
 * @ingroup IControls */
template <int MAXPOINTS = 512>
class IVGoniometerControl : public IControl
                          , public IVectorBase
{
public:
  /** Display mode for the goniometer */
  enum class EMode
  {
    Dots = 0,   ///< Draw individual dots
    Lines       ///< Draw connected lines between consecutive points
  };

  /** Constructs an IVGoniometerControl
   * @param bounds The rectangular area that the control occupies
   * @param label A CString to label the control
   * @param style The IVStyle for the control
   * @param showCorrelation If true, displays a correlation meter below the goniometer
   * @param mode Display mode (dots or lines) */
  IVGoniometerControl(const IRECT& bounds, const char* label = "", const IVStyle& style = DEFAULT_STYLE,
                      bool showCorrelation = true, EMode mode = EMode::Dots)
  : IControl(bounds)
  , IVectorBase(style)
  , mShowCorrelation(showCorrelation)
  , mMode(mode)
  {
    AttachIControl(this, label);
  }

  void Draw(IGraphics& g) override
  {
    DrawBackground(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);

    if (mStyle.drawFrame)
      g.DrawRect(GetColor(kFR), mWidgetBounds, &mBlend, mStyle.frameThickness);
  }

  void DrawWidget(IGraphics& g) override
  {
    IRECT gonioBounds = mWidgetBounds;
    IRECT correlationBounds;

    // Reserve space for correlation meter if enabled
    if (mShowCorrelation)
    {
      gonioBounds = mWidgetBounds.GetPadded(-mPadding).FracRectVertical(0.85f, true);
      correlationBounds = mWidgetBounds.GetPadded(-mPadding).FracRectVertical(0.12f, false);
    }
    else
    {
      gonioBounds = mWidgetBounds.GetPadded(-mPadding);
    }

    // Make goniometer area square, centered
    float size = std::min(gonioBounds.W(), gonioBounds.H());
    IRECT squareBounds = gonioBounds.GetCentredInside(size, size);

    DrawGoniometer(g, squareBounds);

    if (mShowCorrelation)
    {
      DrawCorrelationMeter(g, correlationBounds);
    }
  }

  void DrawGoniometer(IGraphics& g, const IRECT& bounds)
  {
    const float cx = bounds.MW();
    const float cy = bounds.MH();
    const float radius = bounds.W() * 0.5f * 0.9f; // 90% of half-width

    // Draw crosshairs/grid
    const IColor gridColor = GetColor(kSH);
    g.DrawLine(gridColor, cx - radius, cy, cx + radius, cy, &mBlend, 1.f); // Horizontal (S axis)
    g.DrawLine(gridColor, cx, cy - radius, cx, cy + radius, &mBlend, 1.f); // Vertical (M axis)

    // Draw diagonal lines for L and R axes
    const float diag = radius * 0.707f; // sqrt(2)/2
    g.DrawLine(gridColor, cx - diag, cy - diag, cx + diag, cy + diag, &mBlend, 0.5f); // L axis
    g.DrawLine(gridColor, cx + diag, cy - diag, cx - diag, cy + diag, &mBlend, 0.5f); // R axis

    // Draw axis labels
    const IText& labelText = mStyle.labelText.WithSize(10.f).WithAlign(EAlign::Center);
    const IColor textColor = GetColor(kFG);

    g.DrawText(IText(10.f, textColor, nullptr, EAlign::Center), "M", IRECT(cx - 10, cy - radius - 15, cx + 10, cy - radius));
    g.DrawText(IText(10.f, textColor, nullptr, EAlign::Center), "S", IRECT(cx + radius + 2, cy - 5, cx + radius + 15, cy + 5));
    g.DrawText(IText(10.f, textColor, nullptr, EAlign::Center), "L", IRECT(cx - diag - 15, cy - diag - 15, cx - diag, cy - diag));
    g.DrawText(IText(10.f, textColor, nullptr, EAlign::Center), "R", IRECT(cx + diag, cy - diag - 15, cx + diag + 15, cy - diag));

    // Draw sample points
    if (mData.nPoints > 0)
    {
      const IColor pointColor = GetColor(kFG);
      const float scale = radius * mScale;

      if (mMode == EMode::Dots)
      {
        for (int i = 0; i < mData.nPoints; i++)
        {
          float L = mData.points[i].first;
          float R = mData.points[i].second;

          // Convert L/R to M/S (rotated 45 degrees)
          // M = (L + R) / sqrt(2), S = (L - R) / sqrt(2)
          // But we simplify by just rotating the coordinate system
          float x = (L - R) * scale; // Side (horizontal)
          float y = -(L + R) * scale; // Mid (vertical, negated for screen coords)

          float px = cx + x;
          float py = cy + y;

          // Clamp to bounds
          px = std::max(bounds.L, std::min(bounds.R, px));
          py = std::max(bounds.T, std::min(bounds.B, py));

          g.FillCircle(pointColor, px, py, mPointSize, &mBlend);
        }
      }
      else // Lines mode
      {
        float prevX = 0, prevY = 0;
        bool first = true;

        for (int i = 0; i < mData.nPoints; i++)
        {
          float L = mData.points[i].first;
          float R = mData.points[i].second;

          float x = (L - R) * scale;
          float y = -(L + R) * scale;

          float px = cx + x;
          float py = cy + y;

          px = std::max(bounds.L, std::min(bounds.R, px));
          py = std::max(bounds.T, std::min(bounds.B, py));

          if (!first)
          {
            g.DrawLine(pointColor, prevX, prevY, px, py, &mBlend, mLineThickness);
          }

          prevX = px;
          prevY = py;
          first = false;
        }
      }
    }
  }

  void DrawCorrelationMeter(IGraphics& g, const IRECT& bounds)
  {
    // Draw correlation meter background
    g.FillRect(GetColor(kBG), bounds, &mBlend);

    // Draw meter frame
    g.DrawRect(GetColor(kFR), bounds, &mBlend, 1.f);

    // Draw center line (correlation = 0)
    const float cx = bounds.MW();
    g.DrawLine(GetColor(kSH), cx, bounds.T, cx, bounds.B, &mBlend, 1.f);

    // Draw tick marks and labels
    const IColor textColor = GetColor(kFG);
    g.DrawText(IText(8.f, textColor), "-1", IRECT(bounds.L, bounds.T - 12, bounds.L + 20, bounds.T));
    g.DrawText(IText(8.f, textColor, nullptr, EAlign::Center), "0", IRECT(cx - 10, bounds.T - 12, cx + 10, bounds.T));
    g.DrawText(IText(8.f, textColor, nullptr, EAlign::Far), "+1", IRECT(bounds.R - 20, bounds.T - 12, bounds.R, bounds.T));

    // Draw correlation indicator
    // Correlation ranges from -1 to +1, map to meter width
    const float meterPadding = 2.f;
    const float meterWidth = bounds.W() - 2 * meterPadding;

    // Smoothed correlation value
    float displayCorrelation = mSmoothedCorrelation;

    // Map correlation [-1, 1] to [0, meterWidth]
    float indicatorX = bounds.L + meterPadding + (displayCorrelation + 1.f) * 0.5f * meterWidth;

    // Color based on correlation: red (out of phase) -> yellow (uncorrelated) -> green (mono)
    IColor barColor;
    if (displayCorrelation < 0)
    {
      // Red to yellow gradient for negative correlation
      float t = displayCorrelation + 1.f; // 0 to 1
      barColor = IColor::LinearInterpolateBetween(IColor(255, 255, 0, 0), IColor(255, 255, 255, 0), t);
    }
    else
    {
      // Yellow to green gradient for positive correlation
      float t = displayCorrelation; // 0 to 1
      barColor = IColor::LinearInterpolateBetween(IColor(255, 255, 255, 0), IColor(255, 0, 255, 0), t);
    }

    // Draw bar from center to correlation position
    float barLeft = std::min(cx, indicatorX);
    float barRight = std::max(cx, indicatorX);
    IRECT barRect(barLeft, bounds.T + meterPadding, barRight, bounds.B - meterPadding);
    g.FillRect(barColor, barRect, &mBlend);

    // Draw indicator line
    g.DrawLine(GetColor(kFG), indicatorX, bounds.T + meterPadding, indicatorX, bounds.B - meterPadding, &mBlend, 2.f);
  }

  void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
    SetDirty(false);
  }

  void OnMsgFromDelegate(int msgTag, int dataSize, const void* pData) override
  {
    if (!IsDisabled() && msgTag == ISender<>::kUpdateMessage)
    {
      IByteStream stream(pData, dataSize);
      ISenderData<1, IGoniometerData<MAXPOINTS>> d;
      stream.Get(&d, 0);

      mData = d.vals[0];

      // Smooth the correlation value
      const float smoothing = 0.3f;
      mSmoothedCorrelation = mSmoothedCorrelation * (1.f - smoothing) + mData.correlation * smoothing;

      SetDirty(false);
    }
  }

  /** Set the display mode
   * @param mode Dots or Lines */
  void SetMode(EMode mode) { mMode = mode; }

  /** Get the current display mode */
  EMode GetMode() const { return mMode; }

  /** Set the point size for dot mode
   * @param size Point radius in pixels */
  void SetPointSize(float size) { mPointSize = size; }

  /** Set the line thickness for line mode
   * @param thickness Line thickness in pixels */
  void SetLineThickness(float thickness) { mLineThickness = thickness; }

  /** Set the display scale factor
   * @param scale Multiplier for sample values (higher = more spread) */
  void SetScale(float scale) { mScale = scale; }

  /** Enable or disable the correlation meter
   * @param show True to show correlation meter */
  void SetShowCorrelation(bool show)
  {
    mShowCorrelation = show;
    SetDirty(false);
  }

private:
  IGoniometerData<MAXPOINTS> mData;
  float mSmoothedCorrelation = 0.f;
  float mPadding = 5.f;
  float mPointSize = 1.5f;
  float mLineThickness = 1.f;
  float mScale = 0.707f; // 1/sqrt(2) for unity gain display
  bool mShowCorrelation = true;
  EMode mMode = EMode::Dots;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
