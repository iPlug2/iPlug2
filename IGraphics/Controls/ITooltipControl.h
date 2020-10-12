#include "IControl.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class ITooltipControl : public IControl, public IVectorBase
{
public:
  static const IVStyle DEFAULT_STYLE;

  ITooltipControl(const IRECT& bounds, const IVStyle& style=DEFAULT_STYLE);

  virtual void Draw(IGraphics& g) override;

  /** Set this as the tooltip for the given control.
   * This tooltip will move to an appropriate position and set its
   * text to the tooltip text of the control. */
  void SetForControl(int idx);
  int GetControlIdx() const;
  void SetText(const char* text);
  const WDL_String& GetText() const;

private:
  void RecalcArea();

  int mControlIdx;
  IRECT mTargetArea;
  WDL_String mText;
  bool mNeedsResize;

};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

