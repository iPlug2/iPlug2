#ifndef __IPLUGGUIRESIZE__
#define __IPLUGGUIRESIZE__

#include "IPlug_include_in_plug_hdr.h"

class IGUIResizeButton : public IControl
{
private:
  int mResizeWidth;
  int mResizeHeight;
  WDL_String mStr;

public:
  IGUIResizeButton(IPlugBase* pPlug, IRECT pR, const char* label, int w, int h)
    :	IControl(pPlug, pR)
  {
    mResizeWidth = w;
    mResizeHeight = h;
    mStr.Set(label);
    mText.mColor = COLOR_WHITE;
    mText.mSize = 24;
  }

  ~IGUIResizeButton() {}

  bool Draw(IGraphics* pGraphics)
  {
    pGraphics->FillIRect(&COLOR_BLACK, &mRECT, &mBlend);
    char* cStr = mStr.Get();
    return pGraphics->DrawIText(&mText, cStr, &mRECT);
  }

  void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    mPlug->GetGUI()->Resize(mResizeWidth, mResizeHeight);
  }

  //bool IsDirty()
  //{
  //  return true;
  //}
};

class IPlugGUIResize : public IPlug
{
public:

  IPlugGUIResize(IPlugInstanceInfo instanceInfo);
  ~IPlugGUIResize();

  void Reset();
  void OnParamChange(int paramIdx);
  void OnWindowResize();

  void CreateControls(IGraphics* pGraphics, int size);

  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:
};

#endif
