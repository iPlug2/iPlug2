class IXYPad : public IControl
{
public:
  IXYPad(IPlugBase *pPlug, IRECT pR, int handleRadius, int paramA, int paramB)
  : IControl(pPlug, pR)
  , mHandleRadius(handleRadius)
  , mHandleColor(COLOR_WHITE)
  {
    AddAuxParam(paramA);
    AddAuxParam(paramB);
  }
  
  bool Draw(IGraphics* pGraphics)
  {
    double xpos = GetAuxParam(0)->mValue * mRECT.W();
    double ypos = GetAuxParam(1)->mValue * mRECT.H();

    pGraphics->DrawLine(&mHandleColor, xpos+mRECT.L, mRECT.T, xpos+mRECT.L, mRECT.B, 0, false);
    pGraphics->DrawLine(&mHandleColor, mRECT.L, ypos+mRECT.T, mRECT.R, ypos+mRECT.T, 0, false);
    pGraphics->FillCircle(&mHandleColor, xpos+mRECT.L, ypos+mRECT.T, mHandleRadius, 0, true);
    
    return true;
  }
  
//  void OnMouseOver(int x, int y, IMouseMod* pMod)
//  {
//    return SnapToMouse(x, y);
//  }
//  
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
    GetAuxParam(0)->mValue = BOUNDED((double)x / (double)mRECT.W(), 0, 1);
    GetAuxParam(1)->mValue = BOUNDED((double)y / (double)mRECT.H(), 0, 1);
    
    SetDirty();
  }

  void SetDirty(bool pushParamToPlug = true)
  {
    mDirty = true;
    
    if (pushParamToPlug && mPlug)
    {
      SetAllAuxParamsFromGUI();
    }
  }
private:
  int mHandleRadius;
  IColor mHandleColor;
};
