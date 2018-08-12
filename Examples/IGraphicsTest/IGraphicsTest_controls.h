#include "IControl.h"

class FileMenu : public IDirBrowseControlBase
{
public:
  FileMenu(IGEditorDelegate& dlg, IRECT rect, IActionFunction actionFunc, const IText& text,
           const char* extension)
  : IDirBrowseControlBase(dlg, rect, extension)
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

    SetDirty();
  }

private:
  WDL_String mLabel;
};

class IArcControl : public IKnobControlBase
{
public:
  IArcControl(IGEditorDelegate& dlg, IRECT rect, int paramIdx, float angle1 = -135.f, float angle2 = 135.f)
  : IKnobControlBase(dlg, rect, paramIdx)
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
  IPolyControl(IGEditorDelegate& dlg, IRECT rect, int paramIdx)
  : IKnobControlBase(dlg, rect, paramIdx)
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
  IGradientControl(IGEditorDelegate& dlg, IRECT rect, int paramIdx)
  : IKnobControlBase(dlg, rect, paramIdx)
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
      g.PathStart();
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

    if (std::rand() & 0x100)
      tmp = IPattern(mRECT.MW(), mRECT.MH(), mRECT.MH());
    else
      tmp = IPattern(mRECT.L, mRECT.MH(), mRECT.L + mRECT.W() * 0.5, mRECT.MH());

      tmp.mExtend = (std::rand() & 0x10) ? ((std::rand() & 0x1000) ? kExtendNone : kExtendPad) : ((std::rand() & 0x1000) ? kExtendRepeat : kExtendReflect);

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
  IMultiPathControl(IGEditorDelegate& dlg, IRECT rect, int paramIdx)
  : IKnobControlBase(dlg, rect, paramIdx), mShape(0)
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

class RandomTextControl : public IControl
{
public:
  RandomTextControl(IGEditorDelegate& dlg, IRECT bounds)
  : IControl(dlg, bounds, -1)
  {
    Randomise();
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    Randomise();
    SetDirty(false);
  }

  void Randomise()
  {
    int size = (std::rand() % 100) + 5;
    int style = (std::rand() % 3);
    int align = (std::rand() % 3);
    int type = (std::rand() % 2);
    mStringIndex = (std::rand() % 6);

    const char* types[] = { "Roboto-Regular", "Montserrat-LightItalic" };

    mText = IText(size, IColor::GetRandomColor(), types[type], (IText::EStyle) style, (IText::EAlign) align);
  }

  void Draw(IGraphics& g) override
  {
    const char* words[] = { "there", "are many" , "possible", "ways", "to display text", "here" };

    g.FillRect(COLOR_BLACK, mRECT);
    g.DrawText(mText, words[mStringIndex], mRECT);
  }

private:
  int mStringIndex;
};

class DrawingTest : public IControl
{
public:
  DrawingTest(IGEditorDelegate& plug, IRECT bounds)
  : IControl(plug, bounds, -1)
  {
  }
  
  void Draw(IGraphics& g) override
  {
    g.FillRect(COLOR_BLUE, mRECT);
//    g.FillRect(COLOR_RED, mRECT.GetRandomSubRect());
#ifdef IGRAPHICS_NANOVG

    NVGcontext* vg = (NVGcontext*) g.GetDrawContext();
    //
    const int h = 28;
    const int w = mRECT.W();
    const int x = mRECT.L;
    const int y = mRECT.T;
    const float pos = 0.5;
    //
    NVGpaint bg, knob;
    float cy = y+(int)(h*0.5f);
    float kr = (int)(h*0.25f);

    nvgSave(vg);
    //  nvgClearState(vg);

    //Slot
    bg = nvgBoxGradient(vg, x,cy-2+1, w,4, 2,2, nvgRGBA(0,0,0,32), nvgRGBA(0,0,0,128));
    nvgBeginPath(vg);
    nvgRoundedRect(vg, x,cy-2, w,4, 2);
    nvgFillPaint(vg, bg);
    nvgFill(vg);

    // Knob Shadow
    bg = nvgRadialGradient(vg, x+(int)(pos*w),cy+1, kr-3,kr+3, nvgRGBA(0,0,0,64), nvgRGBA(0,0,0,0));
    nvgBeginPath(vg);
    nvgRect(vg, x+(int)(pos*w)-kr-5,cy-kr-5,kr*2+5+5,kr*2+5+5+3);
    nvgCircle(vg, x+(int)(pos*w),cy, kr);
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillPaint(vg, bg);
    nvgFill(vg);

    // Knob
    knob = nvgLinearGradient(vg, x,cy-kr,x,cy+kr, nvgRGBA(255,255,255,16), nvgRGBA(0,0,0,16));
    nvgBeginPath(vg);
    nvgCircle(vg, x+(int)(pos*w),cy, kr-1);
    nvgFillColor(vg, nvgRGBA(255,255,255,255));
    nvgFill(vg);
    nvgFillPaint(vg, knob);
    nvgFill(vg);

    nvgBeginPath(vg);
    nvgCircle(vg, x+(int)(pos*w),cy, kr-0.5f);
    nvgStrokeColor(vg, nvgRGBA(0,0,0,92));
    nvgStroke(vg);

    nvgRestore(vg);
#endif
  }
};

