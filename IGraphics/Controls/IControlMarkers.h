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
 * @brief Controls for drawing markers alongside sliders, knobs etc
 */

#include "IControl.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

static void DrawRadialMarks(IGraphics& g, const IRECT& track, int numMajor, float a1 = -135.0f, float a2 = 135.0f, float majorThickness = 2.0f, float minorThickness = 1.0f, float markSize = 10.0f, float markRatio = 0.6f)
{
  auto radius = track.W() / 2.0f;
  auto a1r = DegToRad(a1);
  auto a2r = DegToRad(a2);
  
  auto div = (a2r-a1r) / float(numMajor-1);

  for (auto i = 0; i < numMajor; ++i)
  {
    auto from = radius;
    auto markWidth = majorThickness;

//    if (i % (numMajor / (numMajor-1)))
//    {
//      from -= (markSize * (1.0f-markRatio));
//      markWidth = minorThickness;
//    }

    auto angle = a1r - (iplug::PI / 2.0f) + (i * div);
    auto sinX = std::sin(angle);
    auto cosX = std::cos(angle);
    auto to = radius - markSize;
    
    from = radius;
    to = radius * 1.5f;

    g.DrawLine(COLOR_BLACK, track.MW() + from * cosX,
                            track.MH() + from * sinX,
                            track.MW() + to * cosX,
                            track.MH() + to * sinX, 0, markWidth);
  }
}


static void DrawLinearMarks(IGraphics& g, const IRECT& track, int numMajor, int numMinor, EDirection dir, const IColor& color = COLOR_BLACK, float majorThickness = 2.0f, float minorThickness = 1.0f, float majorAlpha = 0.5f, float minorAlpha = 0.25f, float markFrac = 0.15f, float distanceFromTrackCentre = 15.0f)
{
  const auto cellIncr = 1.0f / numMajor;
  const auto subMarkCellIncr = cellIncr / float(numMinor+1);

  const auto lineDirection = dir == EDirection::Horizontal ? EDirection::Vertical : EDirection::Horizontal;
  const auto wholeLineRect = dir == EDirection::Horizontal ? track.GetMidVPadded(distanceFromTrackCentre) : track.GetMidHPadded(distanceFromTrackCentre);
    
  const auto lineRect1 = wholeLineRect.FracRect(lineDirection, markFrac);
  const auto lineRect2 = wholeLineRect.FracRect(lineDirection, markFrac, true);

  for (int i = numMajor; i >= 0; i--)
  {
    auto majorPos = float(i) / float(numMajor);
    g.DrawLineAcross(color.WithOpacity(majorAlpha), lineRect1, lineDirection, majorPos, nullptr, majorThickness);
    g.DrawLineAcross(color.WithOpacity(majorAlpha), lineRect2, lineDirection, majorPos, nullptr, majorThickness);

    for (int j = 1; j <= numMinor; j++)
    {
      auto minorPos = (cellIncr * i) + (subMarkCellIncr * j);
      g.DrawLineAcross(color.WithOpacity(minorAlpha), lineRect1, lineDirection, minorPos, nullptr, minorThickness);
      g.DrawLineAcross(color.WithOpacity(minorAlpha), lineRect2, lineDirection, minorPos, nullptr, minorThickness);
    }
  }
}

static void DrawRadialLabels(IGraphics& g, const IRECT& track, const std::vector<std::string>& labels, float a1 = -135.0f, float a2 = 135.0f, const IText& text = DEFAULT_VALUE_TEXT)
{
  auto radius = track.W() / 2.0f;
  auto a1r = DegToRad(a1);
  auto a2r = DegToRad(a2);
  
  auto numMajor = labels.size();
  
  auto div = (a2r-a1r) / float(numMajor-1);
  
  for (auto i = 0; i < numMajor; ++i)
  {
    auto angle = a1r - (iplug::PI / 2.0f) + (i * div);
    auto sinX = std::sin(angle);
    auto cosX = std::cos(angle);

    auto x = track.MW() + (radius * 1.75) * cosX;
    auto y = track.MH() + (radius * 1.75) * sinX;

    g.DrawText(text, labels[i].c_str(), x, y);
  }
}

static void DrawLinearLabels(IGraphics& g, const IRECT& track, int numMajor, EDirection dir, const IText& text, float distanceFromTrackCentre = -25.0f)
{
  IRECT labelRect;
  
  if (dir == EDirection::Horizontal)
    labelRect = track.GetTranslated(-(track.W() / numMajor) / 2.0, distanceFromTrackCentre);
  else
    labelRect = track.GetTranslated(distanceFromTrackCentre, -(track.H() / numMajor) / 2.0);

  for (int i = 0; i <= numMajor; i++)
  {
    WDL_FastString str;
    str.SetFormatted(32, "%i", i);
    g.DrawText(text, str.Get(), labelRect.SubRect(dir, numMajor, i));
  }
}

class ILinearMarks : public IControl
{
public:
  ILinearMarks(const IRECT& bounds, const IColor& lineColor = COLOR_BLACK, EDirection dir = EDirection::Vertical, const IText& text = DEFAULT_VALUE_TEXT)
  : IControl(bounds)
  , mLineColor(lineColor)
  , mDirection(dir)
  {
  }

  void Draw(IGraphics& g) override
  {
    if (!g.CheckLayer(mLayer))
    {
      g.StartLayer(this, mRECT);
      
      DrawLinearMarks(g, mTrackBounds, mNumMarks, mNumMinorMarks, mDirection, mLineColor);
      DrawLinearLabels(g, mTrackBounds, mNumMarks, mDirection, mText);
      
      mLayer = g.EndLayer();
    }
    
    g.DrawLayer(mLayer);
  }

  void SetTrackBounds(const IRECT& r)
  {
    mTrackBounds = r;
    SetDirty(false);
  }
  
private:
  IRECT mTrackBounds;
  const int mNumMarks = 10;
  const int mNumMinorMarks = 1;
  const double mMajorMarksWidth = 2.0;
  const double mMinorMarksWidth = 1.0;
  const EDirection mDirection;
  IColor mLineColor;
  ILayerPtr mLayer;
};

class IRadialMarks : public IControl
{
public:
  IRadialMarks(const IRECT& bounds,
              const std::initializer_list<std::string>& labels, bool showMarkLabels,
              float a1 = -135.0f, float a2 = 135.0f, float markSize = 2.0f, float markRatio = 0.6f)
  : IControl(bounds)
  , mLabels(labels)
  , mAngle1(a1)
  , mAngle2(a2)
  , mMarkSize(markSize)
  , mMarkRatio(markRatio)
  , mShowMarkLabels(showMarkLabels)
  {
    assert(labels.size() >= 2);
    mText.mAlign = EAlign::Center;
    mText.mVAlign = EVAlign::Middle;
  }
  
  void Draw(IGraphics& g) override
  {
    if (!g.CheckLayer(mLayer))
    {
      g.StartLayer(this, mRECT);
      DrawRadialMarks(g, mTrackBounds, static_cast<int>(mLabels.size()), mAngle1, mAngle2, mMarkSize);
      
      if (mShowMarkLabels)
        DrawRadialLabels(g, mTrackBounds, mLabels, mAngle1, mAngle2, mText);
      
      mLayer = g.EndLayer();
    }
    g.DrawLayer(mLayer);
  }
    
  void SetTrackBounds(const IRECT& r)
  {
    mTrackBounds = r;
    SetDirty(false);
  }
  
private:
  bool mShowMarkLabels;
  IRECT mTrackBounds;
  std::vector<std::string> mLabels;
  const double mAngle1, mAngle2;
  const double mMarkSize;
  const double mMarkRatio;
  const double mMajorMarksWidth = 2.0;
  const double mMinorMarksWidth = 0.5;
  ILayerPtr mLayer;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
