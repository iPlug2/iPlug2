#include "IControl.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class ITooltipControl : public IControl
{
public:
  ITooltipControl(const IColor& BGColor = COLOR_WHITE, const IText& text = DEFAULT_TEXT)
  : IControl(IRECT())
  , mBGColor(BGColor)
  {
    mIgnoreMouse = true;
    mText = text;
    Hide(true);
  }
  
  virtual ~ITooltipControl()
  {
  }

  virtual void Draw(IGraphics& g) override
  {
    IRECT innerRECT = mRECT.GetPadded(-10);
    g.DrawFastDropShadow(innerRECT, mRECT);
    g.FillRect(COLOR_WHITE, innerRECT);
    g.DrawText(mText, mDisplayStr.Get(), mRECT.GetPadded(-2));
  }

  /** Set this as the tooltip for the given control.
   * This tooltip will move to an appropriate position and set its
   * text to the tooltip text of the control. */
  void SetControl(IControl* pControl)
  {
    if (!pControl)
    {
      Hide(true);
    }
    else
    {
      mHoverControlBounds = pControl->GetRECT();
      mDisplayStr.Set(pControl->GetTooltip());
      
      if (mDisplayStr.GetLength() == 0)
      {
        SetRECT(IRECT());
        Hide(true);
        return;
      }
      else
      {
        SetRECT(CalculateBounds());
        Hide(false);
      }
    }
  }

private:
  virtual IRECT CalculateBounds()
  {
    IGraphics* pGraphics = GetUI();

    IRECT maxBounds = pGraphics->GetBounds();
    IRECT bounds { 0, 0, 2, 2 };
    pGraphics->MeasureText(mText, mDisplayStr.Get(), bounds);
    bounds.Pad(20.f);
    
    // Move the bounds to be within the maxBounds but close to target
    bounds = mHoverControlBounds.GetCentredInside(bounds.GetFromTLHC(bounds.W(), bounds.H()));

    // Shift bounds to ensure the tooltip is inside maxBounds
    float yOff = (mHoverControlBounds.H() / 2.f);
    float xOff = 0.f;
    float diff = 0.f;
    
    // Shift up
    diff = (bounds.B + yOff) - maxBounds.B;
    
    if (diff >= 0.f)
      yOff -= diff + 1.f;
    
    // Shift left
    diff = (bounds.R + xOff) - maxBounds.R;
    
    if (diff >= 0.f)
      xOff -= diff + 1.f;
    
    // Shift right
    diff = maxBounds.L - (bounds.L + xOff);
    
    if (diff >= 0.f)
      xOff += diff + 1.f;
    
    bounds.Translate(xOff, yOff);
    
    return bounds;
  }

private:
  IColor mBGColor;
  IRECT mHoverControlBounds;
  WDL_String mDisplayStr;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
