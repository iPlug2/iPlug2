#include "IControl.h"

class SVGKnob : public IKnobControlBase
{
private:
  double mPhase = 0.;
  ISVG mSVG;
  
public:
  SVGKnob(IPlugBaseGraphics& plug, float x, float y, ISVG& svg, int param = kNoParameter)
  : IKnobControlBase(plug, IRECT(x, y, x + svg.W(), y + svg.H()), param)
  , mSVG(svg)
  {
  }
  
  void Draw(IGraphics& g) override
  {
    g.DrawRotatedSVG(mSVG, mRECT.MW(), mRECT.MH(), mRECT.W(), mRECT.H(), mValue * 360.);
  }
  
//  bool IsDirty() override
//  {
//    mValue += 0.01;
//    
////    if(mAnimationFunc)
////    {
////      mAnimationFunc(this, 0., 0.);
////      return true;
////    }
//    if(mValue >= 1.)
//      mValue -= 1.;
//    return true;
//  }
  
  void SetSVG(ISVG& svg)
  {
    mSVG = svg;
    SetDirty();
  }
};

class FileMenu : public IDirBrowseControlBase
{
public:
  FileMenu(IPlugBaseGraphics& plug, IRECT rect, IActionFunction actionFunc,
           const char* path)
  : IDirBrowseControlBase(plug, rect, ".svg")
  {
    mLabel.Set("Select an svg");
    AddPath(path, "VCVRackComponents"); // label not needed as it's only 1 folder
    SetUpMenu();
    mActionFunc = actionFunc;
  }
  
  void Draw(IGraphics& graphics)
  {
    graphics.FillRect(COLOR_WHITE, mRECT);
    graphics.DrawText(mText, mLabel.Get(), mRECT);
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod)
  {
    IPopupMenu* menu = GetGUI()->CreateIPopupMenu(mMainMenu, mRECT);
    
    if(menu)
    {
      IPopupMenu::Item* item = menu->GetItem(menu->GetChosenItemIdx());
      mSelectedIndex = item->GetTag();
      mSelectedMenu = menu; // TODO: what if this is a submenu do we end up with pointer to an invalid object?
      mLabel.Set(item->GetText());
      
      mActionFunc(this);
    }
    
    //Redraw(); // TODO:  seems to need this
    SetDirty();
  }
  
private:
  WDL_String mLabel;
};

class IArcControl : public IKnobControlBase
{
public:
  IArcControl(IPlugBaseGraphics& plug, IRECT rect, int param, float angle1 = -135.f, float angle2 = 135.f)
  : IKnobControlBase(plug, rect, param), mAngle1(angle1), mAngle2(angle2)
  {
    
  }
  
  void Draw(IGraphics& graphics)
  {
    graphics.FillRect(COLOR_GRAY, mRECT.GetPadded(-2));
    graphics.DrawRect(COLOR_BLACK, mRECT.GetPadded(-2));
    double angle = mAngle1 + mValue * (mAngle2 - mAngle1);
    graphics.FillArc(COLOR_BLUE, mRECT.MW(), mRECT.MH(), mRECT.W() * 0.44, mAngle1, angle);
    graphics.DrawArc(COLOR_BLACK, mRECT.MW(), mRECT.MH(), mRECT.W() * 0.44, mAngle1, angle);
    graphics.DrawRadialLine(COLOR_BLACK, mRECT.MW(), mRECT.MH(), angle, 0., mRECT.W() * 0.49);
    graphics.FillCircle(COLOR_WHITE, mRECT.MW(), mRECT.MH(), mRECT.W() * 0.1);
    graphics.DrawCircle(COLOR_BLACK, mRECT.MW(), mRECT.MH(), mRECT.W() * 0.1);
    
    angle = DegToRad(angle);
    
    float x1 = mRECT.MW() + cos(angle - 0.3) * mRECT.W() * 0.3;
    float y1 = mRECT.MH() + sin(angle - 0.3) * mRECT.W() * 0.3;
    float x2 = mRECT.MW() + cos(angle + 0.3) * mRECT.W() * 0.3;
    float y2 = mRECT.MH() + sin(angle + 0.3) * mRECT.W() * 0.3;
    float x3 = mRECT.MW() + cos(angle) * mRECT.W() * 0.44;
    float y3 = mRECT.MH() + sin(angle) * mRECT.W() * 0.44;
    
    graphics.FillTriangle(COLOR_WHITE, x1, y1, x2, y2, x3, y3);
    graphics.DrawTriangle(COLOR_BLACK, x1, y1, x2, y2, x3, y3);
  }
  
private:
  double mAngle1;
  double mAngle2;
};

class IPolyControl : public IKnobControlBase
{
public:
  IPolyControl(IPlugBaseGraphics& plug, IRECT rect, int param)
  : IKnobControlBase(plug, rect, param)
  {
  }
  
private:
  void Draw(IGraphics& graphics)
  {
    float xarray[32];
    float yarray[32];
    int npoints = 3 + round(mValue * 29);
    double angle = (-0.75 * PI) + mValue * (1.5 * PI);
    double incr = (2 * PI) / npoints;
    double cr = mValue * (mRECT.W() / 2.0);
    
    graphics.FillRoundRect(COLOR_GRAY, mRECT.GetPadded(-2), cr);
    graphics.DrawRoundRect(COLOR_BLACK, mRECT.GetPadded(-2), cr);
    
    for (int i = 0; i < npoints; i++)
    {
      xarray[i] = mRECT.MW() + sin(angle + i * incr) * mRECT.W() * 0.45;
      yarray[i] = mRECT.MH() + cos(angle + i * incr) * mRECT.W() * 0.45;
    }
    
    graphics.FillConvexPolygon(COLOR_ORANGE, xarray, yarray, npoints);
    graphics.DrawConvexPolygon(COLOR_BLACK, xarray, yarray, npoints);
    
  }
};
