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
    float angle = mAngle1 + (float) mValue * (mAngle2 - mAngle1);
    graphics.FillArc(COLOR_BLUE, mRECT.MW(), mRECT.MH(), mRECT.W() * 0.44f, mAngle1, angle);
    graphics.DrawArc(COLOR_BLACK, mRECT.MW(), mRECT.MH(), mRECT.W() * 0.44f, mAngle1, angle);
    graphics.DrawRadialLine(COLOR_BLACK, mRECT.MW(), mRECT.MH(), angle, 0.f, mRECT.W() * 0.49f);
    graphics.FillCircle(COLOR_WHITE, mRECT.MW(), mRECT.MH(), mRECT.W() * 0.1f);
    graphics.DrawCircle(COLOR_BLACK, mRECT.MW(), mRECT.MH(), mRECT.W() * 0.1f);
    
    angle = DegToRad(angle);
    
    float x1 = mRECT.MW() + cosf(angle - 0.3f) * mRECT.W() * 0.3f;
    float y1 = mRECT.MH() + sinf(angle - 0.3f) * mRECT.W() * 0.3f;
    float x2 = mRECT.MW() + cosf(angle + 0.3f) * mRECT.W() * 0.3f;
    float y2 = mRECT.MH() + sinf(angle + 0.3f) * mRECT.W() * 0.3f;
    float x3 = mRECT.MW() + cosf(angle) * mRECT.W() * 0.44f;
    float y3 = mRECT.MH() + sinf(angle) * mRECT.W() * 0.44f;
    
    graphics.FillTriangle(COLOR_WHITE, x1, y1, x2, y2, x3, y3);
    graphics.DrawTriangle(COLOR_BLACK, x1, y1, x2, y2, x3, y3);
  }
  
private:
  float mAngle1;
  float mAngle2;
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
    int npoints = 3 + (int) roundf((float) mValue * 29.f);
    float angle = (-0.75f * (float) PI) + (float) mValue * (1.5f * (float) PI);
    float incr = (2.f * (float) PI) / npoints;
    float cr = (float) mValue * (mRECT.W() / 2.f);
    
    graphics.FillRoundRect(COLOR_GRAY, mRECT.GetPadded(-2.f), cr);
    graphics.DrawRoundRect(COLOR_BLACK, mRECT.GetPadded(-2.f), cr);
    
    for (int i = 0; i < npoints; i++)
    {
      xarray[i] = mRECT.MW() + sinf(angle + (float) i * incr) * mRECT.W() * 0.45f;
      yarray[i] = mRECT.MH() + cosf(angle + (float) i * incr) * mRECT.W() * 0.45f;
    }
    
    graphics.FillConvexPolygon(COLOR_ORANGE, xarray, yarray, npoints);
    graphics.DrawConvexPolygon(COLOR_BLACK, xarray, yarray, npoints);
    
  }
};
