#ifndef __IPLUGMOUSETEST__
#define __IPLUGMOUSETEST__

#include "IPlug_include_in_plug_hdr.h"

class IXYPad : public IControl
{
public:
  IXYPad(IPlugBase *pPlug, IRECT pR, int handleRadius)
    : IControl(pPlug, pR)
  {
    mHandleRadius = handleRadius;
    mXPos = pR.MW();
    mYPos = pR.MH();
    mHandleColor = COLOR_WHITE;
  }

  bool Draw(IGraphics* pGraphics)
  {
    pGraphics->DrawLine(&mHandleColor, mXPos+mRECT.L, mRECT.T, mXPos+mRECT.L, mRECT.B, 0, false);
    pGraphics->DrawLine(&mHandleColor, mRECT.L, mYPos+mRECT.T, mRECT.R, mYPos+mRECT.T, 0, false);
    pGraphics->FillCircle(&mHandleColor, mXPos+mRECT.L, mYPos+mRECT.T, mHandleRadius, 0, true);

    return true;
  }

  void OnMouseOver(int x, int y, IMouseMod* pMod)
  {
    return SnapToMouse(x, y);
  }

  void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    mHandleColor = COLOR_BLACK;
    return SnapToMouse(x, y);
  }

  void OnMouseUp(int x, int y, IMouseMod* pMod)
  {
    mHandleColor = COLOR_WHITE;
  }

  void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
  {
    return SnapToMouse(x, y);
  }

  void SnapToMouse(int x, int y)
  {
    mXPos = x;
    mYPos = y;

    //SetDirty();
  }

  bool IsDirty() { return true;}

private:
  int mHandleRadius;
  IColor mHandleColor;
  int mXPos, mYPos;
};

class IPlugMouseTest : public IPlug
{
public:

  IPlugMouseTest(IPlugInstanceInfo instanceInfo);
  ~IPlugMouseTest();

  void Reset();
  void OnParamChange(int paramIdx);

  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:

  double mGain;
};

#endif
