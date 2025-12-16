/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for more info.

 ==============================================================================
*/

#include "ISettingsDialog.h"

#include <algorithm>
#include <cmath>

#ifdef OS_WIN
  #include <windows.h>
#else
  #include "IPlugSWELL.h"
#endif

BEGIN_IPLUG_NAMESPACE

LevelMeter::LevelMeter()
{
  mPeakLevel.store(-96.f, std::memory_order_relaxed);
}

void LevelMeter::SetLevel(float peakdB)
{
  mPeakLevel.store(std::clamp(peakdB, -96.f, 6.f), std::memory_order_relaxed);
}

float LevelMeter::GetPeakLevel() const
{
  return mPeakLevel.load(std::memory_order_relaxed);
}

void LevelMeter::ResetPeak()
{
  mPeakLevel.store(-96.f, std::memory_order_relaxed);
  mPeakHold = -96.f;
}

void LevelMeter::SetColors(COLORREF background, COLORREF green, COLORREF yellow, COLORREF red)
{
  mColorBackground = background;
  mColorGreen = green;
  mColorYellow = yellow;
  mColorRed = red;
}

void LevelMeter::SetThresholds(float yellowdB, float reddB)
{
  mYellowThreshold = yellowdB;
  mRedThreshold = reddB;
}

void LevelMeter::Paint(HDC hdc, const RECT& bounds, bool horizontal)
{
  // Constants
  constexpr float kMinDb = -60.f;
  constexpr float kMaxDb = 0.f;
  constexpr float kDbRange = kMaxDb - kMinDb;

  // Get current level
  float levelDb = GetPeakLevel();
  levelDb = std::clamp(levelDb, kMinDb, kMaxDb);

  // Calculate meter fill proportion (0.0 to 1.0)
  float proportion = (levelDb - kMinDb) / kDbRange;

  int width = bounds.right - bounds.left;
  int height = bounds.bottom - bounds.top;

  // Fill background
  HBRUSH bgBrush = CreateSolidBrush(mColorBackground);
  FillRect(hdc, &bounds, bgBrush);
  DeleteObject(bgBrush);

  if (proportion > 0.f)
  {
    // Determine color based on level
    COLORREF meterColor;
    if (levelDb >= mRedThreshold)
      meterColor = mColorRed;
    else if (levelDb >= mYellowThreshold)
      meterColor = mColorYellow;
    else
      meterColor = mColorGreen;

    // Calculate fill rectangle
    RECT fillRect;
    if (horizontal)
    {
      int fillWidth = static_cast<int>(width * proportion);
      fillRect = { bounds.left, bounds.top, bounds.left + fillWidth, bounds.bottom };
    }
    else
    {
      // Vertical meter fills from bottom to top
      int fillHeight = static_cast<int>(height * proportion);
      fillRect = { bounds.left, bounds.bottom - fillHeight, bounds.right, bounds.bottom };
    }

    HBRUSH fillBrush = CreateSolidBrush(meterColor);
    FillRect(hdc, &fillRect, fillBrush);
    DeleteObject(fillBrush);

    // Draw graduated sections for better visual feedback
    // Yellow zone indicator
    float yellowProp = (mYellowThreshold - kMinDb) / kDbRange;
    // Red zone indicator
    float redProp = (mRedThreshold - kMinDb) / kDbRange;

    // Draw threshold lines
    HPEN linePen = CreatePen(PS_SOLID, 1, RGB(80, 80, 80));
    HPEN oldPen = (HPEN)SelectObject(hdc, linePen);

    if (!horizontal)
    {
      int yellowY = bounds.bottom - static_cast<int>(height * yellowProp);
      int redY = bounds.bottom - static_cast<int>(height * redProp);

      MoveToEx(hdc, bounds.left, yellowY, NULL);
      LineTo(hdc, bounds.right, yellowY);

      MoveToEx(hdc, bounds.left, redY, NULL);
      LineTo(hdc, bounds.right, redY);
    }
    else
    {
      int yellowX = bounds.left + static_cast<int>(width * yellowProp);
      int redX = bounds.left + static_cast<int>(width * redProp);

      MoveToEx(hdc, yellowX, bounds.top, NULL);
      LineTo(hdc, yellowX, bounds.bottom);

      MoveToEx(hdc, redX, bounds.top, NULL);
      LineTo(hdc, redX, bounds.bottom);
    }

    SelectObject(hdc, oldPen);
    DeleteObject(linePen);
  }

  // Draw border
  HBRUSH borderBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
  HPEN borderPen = CreatePen(PS_SOLID, 1, RGB(100, 100, 100));
  HPEN oldPen = (HPEN)SelectObject(hdc, borderPen);
  HBRUSH oldBrush = (HBRUSH)SelectObject(hdc, borderBrush);

  Rectangle(hdc, bounds.left, bounds.top, bounds.right, bounds.bottom);

  SelectObject(hdc, oldPen);
  SelectObject(hdc, oldBrush);
  DeleteObject(borderPen);
}

END_IPLUG_NAMESPACE
