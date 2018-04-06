#include "IControl.h"

class FileMenu : public IDirBrowseControlBase
{
public:
  FileMenu(IPlugBaseGraphics& plug, IRECT rect, const IText& text,
           const char* extension)
  : IDirBrowseControlBase(plug, rect, extension)
  {
    mText = text;
    mLabel.SetFormatted(32, "%s \n%s File", "Select a", extension);
  }

  void SetPath(const char* path)
  {
    AddPath(path, "");
    SetUpMenu();
  }

  void Draw(IGraphics& g) override
  {
    g.FillRect(COLOR_BLUE, mRECT);
    g.DrawText(mText, mLabel.Get(), mRECT);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    IPopupMenu* menu = GetUI()->CreatePopupMenu(mMainMenu, mRECT);

    if(menu)
    {
      IPopupMenu::Item* item = menu->GetItem(menu->GetChosenItemIdx());
      mSelectedIndex = item->GetTag();
      mSelectedMenu = menu; // TODO: what if this is a submenu do we end up with pointer to an invalid object?
      mLabel.Set(item->GetText());
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
  IArcControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx, float angle1 = -135.f, float angle2 = 135.f)
  : IKnobControlBase(plug, rect, paramIdx)
  , mAngle1(angle1)
  , mAngle2(angle2)
  {
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    GetUI()->HideMouseCursor();
    IKnobControlBase::OnMouseDown(x, y, mod);
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    GetUI()->HideMouseCursor(false);
    IKnobControlBase::OnMouseUp(x, y, mod);
  }

  void Draw(IGraphics& g) override
  {
    g.FillRect(COLOR_WHITE, mRECT.GetPadded(-2));
    g.DrawRect(COLOR_BLACK, mRECT.GetPadded(-2));
    float angle = mAngle1 + (float) mValue * (mAngle2 - mAngle1);
    g.FillArc(COLOR_BLUE, mRECT.MW(), mRECT.MH(), mRECT.W() * 0.44f, mAngle1, angle);
    g.DrawArc(COLOR_BLACK, mRECT.MW(), mRECT.MH(), mRECT.W() * 0.44f, mAngle1, angle);
    g.DrawRadialLine(COLOR_BLACK, mRECT.MW(), mRECT.MH(), angle, 0.f, mRECT.W() * 0.49f);
    g.FillCircle(COLOR_WHITE, mRECT.MW(), mRECT.MH(), mRECT.W() * 0.1f);
    g.DrawCircle(COLOR_BLACK, mRECT.MW(), mRECT.MH(), mRECT.W() * 0.1f);

    angle = DegToRad(angle-90.f);

    float x1 = mRECT.MW() + cosf(angle - 0.3f) * mRECT.W() * 0.3f;
    float y1 = mRECT.MH() + sinf(angle - 0.3f) * mRECT.W() * 0.3f;
    float x2 = mRECT.MW() + cosf(angle + 0.3f) * mRECT.W() * 0.3f;
    float y2 = mRECT.MH() + sinf(angle + 0.3f) * mRECT.W() * 0.3f;
    float x3 = mRECT.MW() + cosf(angle) * mRECT.W() * 0.44f;
    float y3 = mRECT.MH() + sinf(angle) * mRECT.W() * 0.44f;

    g.FillTriangle(COLOR_WHITE, x1, y1, x2, y2, x3, y3);
    g.DrawTriangle(COLOR_BLACK, x1, y1, x2, y2, x3, y3);
  }

private:
  float mAngle1;
  float mAngle2;
};

class IPolyControl : public IKnobControlBase
{
public:
  IPolyControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdx)
  : IKnobControlBase(plug, rect, paramIdx)
  {
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    GetUI()->HideMouseCursor();
    IKnobControlBase::OnMouseDown(x, y, mod);
  }

  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    GetUI()->HideMouseCursor(false);
    IKnobControlBase::OnMouseUp(x, y, mod);
  }

private:
  void Draw(IGraphics& g) override
  {
    float xarray[32];
    float yarray[32];
    int npoints = 3 + (int) roundf((float) mValue * 29.f);
    float angle = (-0.75f * (float) PI) + (float) mValue * (1.5f * (float) PI);
    float incr = (2.f * (float) PI) / npoints;
    float cr = (float) mValue * (mRECT.W() / 2.f);

    g.FillRoundRect(COLOR_WHITE, mRECT.GetPadded(-2.f), cr);
    g.DrawRoundRect(COLOR_BLACK, mRECT.GetPadded(-2.f), cr);

    for (int i = 0; i < npoints; i++)
    {
      xarray[i] = mRECT.MW() + sinf(angle + (float) i * incr) * mRECT.W() * 0.45f;
      yarray[i] = mRECT.MH() + cosf(angle + (float) i * incr) * mRECT.W() * 0.45f;
    }

    g.FillConvexPolygon(COLOR_ORANGE, xarray, yarray, npoints);
    g.DrawConvexPolygon(COLOR_BLACK, xarray, yarray, npoints);
  }
};

class IGradientControl : public IKnobControlBase
{
public:
  IGradientControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdxIdx)
  : IKnobControlBase(plug, rect, paramIdxIdx)
  {
    RandomiseGradient();
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    RandomiseGradient();
    SetDirty(false);
  }

  void Draw(IGraphics& g) override
  {
    if (g.HasPathSupport())
    {
      double cr = mValue * (mRECT.H() / 2.0);
      g.PathRoundRect(mRECT.GetPadded(-2), cr);
      IFillOptions fillOptions;
      IStrokeOptions strokeOptions;
      fillOptions.mPreserve = true;
      g.PathFill(mPattern, fillOptions);
      g.PathStroke(IColor(255, 0, 0, 0), 3, strokeOptions);
    }
    else
      g.DrawText(mText, "UNSUPPORTED", mRECT);
  }

  void RandomiseGradient()
  {
    //IPattern tmp(kLinearPattern);
    //tmp.SetTransform(1.0/mRECT.W(), 0, 0, 1.0/mRECT.W(), 1.0/mRECT.W()*-mRECT.L, 1.0/mRECT.W()*-mRECT.T);
    IPattern tmp(kSolidPattern);

    if (rand() & 0x100)
      tmp = IPattern(mRECT.MW(), mRECT.MH(), mRECT.MH());
    else
      tmp = IPattern(mRECT.L, mRECT.MH(), mRECT.L + mRECT.W() * 0.5, mRECT.MH());

    tmp.mExtend = (rand() & 0x10) ? ((rand() & 0x1000) ? kExtendNone : kExtendPad) : ((rand() & 0x1000) ? kExtendRepeat : kExtendReflect);

    tmp.AddStop(IColor::GetRandomColor(), 0.0);
    tmp.AddStop(IColor::GetRandomColor(), 0.1);
    tmp.AddStop(IColor::GetRandomColor(), 0.4);
    tmp.AddStop(IColor::GetRandomColor(), 0.6);
    tmp.AddStop(IColor::GetRandomColor(), 1.0);

    mPattern = tmp;
  }

private:
  IPattern mPattern = IPattern(kLinearPattern);
};

class IMultiPathControl : public IKnobControlBase
{
public:
  IMultiPathControl(IPlugBaseGraphics& plug, IRECT rect, int paramIdxIdx)
  : IKnobControlBase(plug, rect, paramIdxIdx), mShape(0)
  {
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (++mShape > 3)
      mShape = 0;
    SetDirty(false);
  }

  void Draw(IGraphics& g) override
  {
    g.DrawRoundRect(COLOR_BLACK, mRECT, 5.);

    if (g.HasPathSupport())
    {
      double r = mValue * (mRECT.H() / 2.0);
      if (mShape == 0)
      {
        g.PathCircle(mRECT.MW(), mRECT.MH(), r);
        g.PathCircle(mRECT.MW(), mRECT.MH(), r * 0.5);
      }
      else if (mShape == 1)
      {
        float pad1 = (mRECT.W() / 2.0) * (1.0 - mValue);
        float pad2 = (mRECT.H() / 2.0) * (1.0 - mValue);
        IRECT size1 = mRECT.GetPadded(pad1, pad2, -pad1, -pad2);
        pad1 = (size1.W() / 2.0) * (1.0 - mValue);
        pad2 = (size1.H() / 2.0) * (1.0 - mValue);
        IRECT size2 = size1.GetPadded(pad1, pad2, -pad1, -pad2);
        g.PathRect(size1);
        g.PathRect(size2);
      }
      else if (mShape == 2)
      {
        float pad1 = (mRECT.W() / 2.0) * (1.0 - mValue);
        float pad2 = (mRECT.H() / 2.0) * (1.0 - mValue);
        IRECT size1 = mRECT.GetPadded(pad1, pad2, -pad1, -pad2);
        pad1 = (size1.W() / 2.0) * (1.0 - mValue);
        pad2 = (size1.H() / 2.0) * (1.0 - mValue);
        IRECT size2 = size1.GetPadded(pad1, pad2, -pad1, -pad2);
        g.PathRoundRect(size1, size1.H() * 0.125);
        g.PathRoundRect(size2, size2.H() * 0.125);
      }
      else if (mShape == 3)
      {
        g.PathMoveTo(mRECT.L, mRECT.B);
        g.PathCurveTo(mRECT.L + mRECT.W() * 0.125, mRECT.T + mRECT.H() * 0.725, mRECT.L + mRECT.W() * 0.25, mRECT.T + mRECT.H() * 0.35, mRECT.MW(), mRECT.MH());
        g.PathLineTo(mRECT.MW(), mRECT.B);
        g.PathClose();
      }

      IFillOptions fillOptions;
      fillOptions.mFillRule = mValue > 0.5 ? kFillEvenOdd : kFillWinding;
      fillOptions.mPreserve = true;
      IStrokeOptions strokeOptions;
      float dashes[] = { 11, 4, 7 };
      strokeOptions.mDash.SetDash(dashes, 0.0, 2);
      g.PathFill(COLOR_BLACK, fillOptions);
      g.PathStroke(COLOR_WHITE, 1, strokeOptions);
    }
    else
      g.DrawText(mText, "UNSUPPORTED", mRECT);
  }

private:

  int mShape;
};

