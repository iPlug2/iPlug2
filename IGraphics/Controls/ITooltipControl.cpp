#include "ITooltipControl.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

const IColor TOOLTIP_BG = IColor::FromColorCodeStr("#292f34");
const IColor TOOLTIP_FG = IColor::FromColorCodeStr("#f7f7f7");
const IVStyle ITooltipControl::DEFAULT_STYLE = IVStyle()
  .WithColor(kBG, TOOLTIP_BG)
  .WithColor(kFG, TOOLTIP_FG)
  .WithLabelText(IText(16.f, EAlign::Center, TOOLTIP_FG))
  .WithRoundness(16.f);

ITooltipControl::ITooltipControl(const IRECT& bounds, const IVStyle& style)
: IControl(bounds, SplashClickActionFunc)
, IVectorBase(style)
{
  AttachIControl(this, "");
  mControlIdx = -1;
}

void ITooltipControl::Draw(IGraphics& g)
{
  float round = mStyle.roundness;
  if (mNeedsResize)
  {
    RecalcArea();
    mNeedsResize = false;
  }

  g.FillRoundRect(mStyle.colorSpec.GetColor(kBG), mRECT, round);
  g.DrawRoundRect(mStyle.colorSpec.GetColor(kFG), mRECT, round);
  g.DrawText(mStyle.labelText, mText.Get(), mRECT.GetPadded(-round * 2));
}

void ITooltipControl::SetForControl(int idx)
{
  mControlIdx = idx;
  if (idx == -1)
  {
    Hide(true);
  }
  else
  {
    auto pCtrl = GetUI()->GetControl(idx);
    mTargetArea = pCtrl->GetRECT();
    SetText(pCtrl->GetTooltip());
    RecalcArea();
    mNeedsResize = false;
  }
}

int ITooltipControl::GetControlIdx() const
{
  return mControlIdx;
}

void ITooltipControl::SetText(const char* text)
{
  if (text)
    mText.Set(text);
  else
    mText.Set("");
  mNeedsResize = true;
}

const WDL_String& ITooltipControl::GetText() const
{
  return mText;
}

void ITooltipControl::RecalcArea()
{
  if (mText.GetLength() == 0)
  {
    SetRECT(IRECT(0, 0, 0, 0));
    return;
  }

  IGraphics& g = *GetUI();

  float round = mStyle.roundness;
  IRECT uiArea = GetUI()->GetBounds();
  IRECT bounds { 0, 0, 2, 2 };
  g.MeasureText(mStyle.labelText, mText.Get(), bounds);
  // Increase width and height by round * 4 (2 * round on each side)
  IVec2 size { bounds.W() + (round * 4), bounds.H() + (round * 4) };
  // Move the bounds to be within the uiArea but close to target
  bounds = mTargetArea.GetCentredInside(bounds.GetFromTLHC(size.x, size.y));

  // Shift bounds to ensure the tooltip is inside uiArea
  float yOff = (mTargetArea.H() / 2.f) + (round * 8);
  float xOff = 0.f;
  float diff;
  // Shift up
  diff = (bounds.B + yOff) - uiArea.B;
  if (diff >= 0.f)
  {
    yOff -= diff + 1.f;
  }
  // Shift left
  diff = (bounds.R + xOff) - uiArea.R;
  if (diff >= 0.f)
  {
    xOff -= diff + 1.f;
  }
  // Shift right
  diff = uiArea.L - (bounds.L + xOff);
  if (diff >= 0.f)
  {
    xOff += diff + 1.f;
  }
  
  bounds.Translate(xOff, yOff);
  SetRECT(bounds);
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
