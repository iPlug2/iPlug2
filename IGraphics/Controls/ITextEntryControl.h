#pragma once

/** A control for drawing a text entry in the graphics context */
class ITextEntryControl : public IControl
{
public:
  ITextEntryControl(IGEditorDelegate& dlg)
  : IControl(dlg, IRECT())
  {
  }

  ~ITextEntryControl() {}

  void Draw(IGraphics& g) override
  {
  }

protected:
};
