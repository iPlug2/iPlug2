IVDropDownListControl::IVDropDownListControl(IDelegate& dlg, IRECT rect, int param)
  : IControl(dlg, rect, param),
    IVectorBase(&DEFAULT_BG_COLOR, &DEFAULT_TXT_COLOR, &DEFAULT_FR_COLOR, &DEFAULT_HL_COLOR)
{
  mInitRect = rect;
  mText.mFGColor = DEFAULT_TXT_COLOR;
  FillNamesFromParamDisplayTexts();
}

IVDropDownListControl::IVDropDownListControl(IDelegate& dlg, IRECT rect, int param,
                               int numStates, const char* names...)
  : IControl(dlg, rect, param),
    IVectorBase(&DEFAULT_BG_COLOR, &DEFAULT_TXT_COLOR, &DEFAULT_FR_COLOR, &DEFAULT_HL_COLOR)
{
  mInitRect = rect;
  mText.mFGColor = DEFAULT_TXT_COLOR;
  if (numStates)
  {
    va_list args;
    va_start(args, names);
    SetNames(numStates, names, args);
    va_end (args);
  }
  else
    FillNamesFromParamDisplayTexts();
};

const IColor IVDropDownListControl::DEFAULT_BG_COLOR = IColor(255, 200, 200, 200);
const IColor IVDropDownListControl::DEFAULT_FR_COLOR = IColor(255, 70, 70, 70);
const IColor IVDropDownListControl::DEFAULT_TXT_COLOR = DEFAULT_FR_COLOR;
const IColor IVDropDownListControl::DEFAULT_HL_COLOR = IColor(255, 240, 240, 240);

void IVDropDownListControl::Draw(IGraphics& graphics)
{
  auto initR = GetInitRect();
  auto shadowColor = IColor(60, 0, 0, 0);

  auto textR = GetRectToAlignTextIn(initR);

  if (!mExpanded)
  {
    if (mDrawShadows && !mEmboss)
      DrawOuterShadowForRect(initR, shadowColor, graphics);

    if (mBlink)
    {
      mBlink = false;
      graphics.FillRect(GetColor(lHL), initR);
      SetDirty(false);
    }
    else
      graphics.FillRect(GetColor(lBG), initR);

    if (mDrawBorders)
      graphics.DrawRect(GetColor(lFR), initR);
    graphics.DrawTextA(mText, NameForVal(StateFromNormalized()), textR);
    ShrinkRects(); // shrink here to clean the expanded area
  }

  else
  {
    auto panelR = GetExpandedRect();
    if (mDrawShadows && !mEmboss)
      DrawOuterShadowForRect(panelR, shadowColor, graphics);
    graphics.FillRect(GetColor(lBG), panelR);
    if (mDrawShadows && mEmboss)
      DrawInnerShadowForRect(panelR, shadowColor, graphics);
    int sx = -1;
    int sy = 0;
    auto rw = initR.W();
    auto rh = initR.H();
    // now just shift the rects and draw them
    for (int v = 0; v < NumStates(); ++v)
    {
      if (v % mColHeight == 0.0)
      {
        ++sx;
        sy = 0;
      }
      IRECT vR = ShiftRectBy(initR, sx * rw, sy * rh);
      IRECT tR = ShiftRectBy(textR, sx * rw, sy * rh);
      if (v == mState)
      {
        if (mDrawShadows) // draw when emboss too, looks good
          DrawOuterShadowForRect(vR, shadowColor, graphics);
        graphics.FillRect(GetColor(lHL), vR);
      }

      if (mDrawBorders)
        graphics.DrawRect(GetColor(lFR), vR);
      graphics.DrawTextA(mText, NameForVal(v), tR);
      ++sy;
    }

    if (mDrawBorders)
    {
      if (!mDrawShadows)   // panelRect == mRECT
      {
        --panelR.R; // fix for strange graphics behavior
        --panelR.B; // mRECT right and bottom are not drawn in expanded state (on Win)
      }
      graphics.DrawRect(GetColor(lFR), panelR);
    }
  }

#ifdef _DEBUG
  //graphics.DrawRect(COLOR_ORANGE, mInitRect);
  //graphics.DrawRect(COLOR_BLUE, mRECT);
  //graphics.DrawRect(COLOR_GREEN, mTargetRECT); // if padded will not be drawn correctly
#endif

}

IRECT IVDropDownListControl::GetInitRect()
{
  auto ir = mInitRect;
  if (mDrawShadows && !mEmboss)
  {
    ir.R -= mShadowOffset;
    ir.B -= mShadowOffset;
  }
  if (mExpanded)
    ir = ShiftRectBy(ir, mRECT.L - ir.L, mRECT.T - ir.T); // if mRECT didn't fit and was shifted.
  // will be different for some other expand directions
  return ir;
}

IRECT IVDropDownListControl::GetExpandedRect()
{
  auto er = mRECT;
  if (mDrawShadows && !mEmboss)
  {
    er.R -= mShadowOffset;
    er.B -= mShadowOffset;
  }
  return er;
}

IRECT IVDropDownListControl::GetRectToAlignTextIn(IRECT r)
{
  // this rect is not precise, it serves as a horizontal level
  auto tr = r;
  // assume all items are 1 line high
  tr.T += 0.5f * (tr.H() - mText.mSize) - 1.0f; // -1 looks better with small text
  tr.B = tr.T + 0.1f;
  return tr;
}

void IVDropDownListControl::DrawInnerShadowForRect(IRECT r, IColor shadowColor, IGraphics& graphics)
{
  auto& o = mShadowOffset;
  auto slr = r;
  slr.R = slr.L + o;
  auto str = r;
  str.L += o;
  str.B = str.T + o;
  graphics.FillRect(shadowColor, slr);
  graphics.FillRect(shadowColor, str);
}

void IVDropDownListControl::SetDrawShadows(bool draw, bool keepButtonRect)
{
  if (draw == mDrawShadows) return;

  if (keepButtonRect && !mEmboss)
  {
    auto d = mShadowOffset;
    if (!draw) d *= -1.0;
    mInitRect.R += d;
    mInitRect.B += d;
    UpdateRectsOnInitChange();
  }

  mDrawShadows = draw;
  SetDirty(false);
}

void IVDropDownListControl::SetEmboss(bool emboss, bool keepButtonRect)
{
  if (emboss == mEmboss) return;

  if (keepButtonRect && mDrawShadows)
  {
    auto d = mShadowOffset;
    if (emboss) d *= -1.0;
    mInitRect.R += d;
    mInitRect.B += d;
    UpdateRectsOnInitChange();
  }

  mEmboss = emboss;
  SetDirty(false);
}

void IVDropDownListControl::SetShadowOffset(float offset, bool keepButtonRect)
{
  if (offset == mShadowOffset) return;

  auto oldOff = mShadowOffset;

  if (offset < 0.0)
    mShadowOffset = 0.0;
  else
    mShadowOffset = offset;

  if (keepButtonRect && mDrawShadows && !mEmboss)
  {
    auto d = offset - oldOff;
    mInitRect.R += d;
    mInitRect.B += d;
    UpdateRectsOnInitChange();
  }

  SetDirty(false);
}

void IVDropDownListControl::UpdateRectsOnInitChange()
{
  if (!mExpanded)
    ShrinkRects();
  else
    ExpandRects();
}

void IVDropDownListControl::OnResize()
{
  mInitRect = mRECT;
  mExpanded = false;
  mLastX = mLastY = -1.0;
  mBlink = false;
  SetDirty(false);
}

void IVDropDownListControl::OnMouseOver(float x, float y, const IMouseMod& mod)
{
  if (mLastX != x || mLastY != y)
  {
    mLastX = x;
    mLastY = y;
    auto panelR = GetExpandedRect();
    if (mExpanded && panelR.Contains(x, y))
    {
      auto rx = x - panelR.L;
      auto ry = y - panelR.T;

      auto initR = GetInitRect();
      int ix = (int)(rx / initR.W());
      int iy = (int)(ry / initR.H());

      int i = ix * mColHeight + iy;

      if (i >= NumStates())
        i = NumStates() - 1;
      if (i != mState)
      {
        mState = i;
        //DbgMsg("mState ", mState);
        SetDirty(false);
      }
    }
  }
}

void IVDropDownListControl::OnMouseDown(float x, float y, const IMouseMod& mod)
{
  if (!mExpanded)
    ExpandRects();
  else
  {
    mExpanded = false;
    mValue = NormalizedFromState();
    SetDirty();
  }
  //DbgMsg("mValue ", mValue);
}

void IVDropDownListControl::OnMouseWheel(float x, float y, const IMouseMod& mod, float d)
{
  int ns = mState;
  ns += (int)d;
  ns = BOUNDED(ns, 0, NumStates() - 1);
  if (ns != mState)
  {
    mState = ns;
    mValue = NormalizedFromState();
    //DbgMsg("mState ", mState);
    //DbgMsg("mValue ", mValue);
    if (!mExpanded)
      mBlink = true;
    SetDirty();
  }
}

void IVDropDownListControl::OnMouseDblClick(float x, float y, const IMouseMod& mod)
{
  mValue = mDefaultValue;
  int ns = StateFromNormalized();
  if (mState != ns)
  {
    mState = ns;
    mValue = NormalizedFromState();
    //DbgMsg("mState ", mState);
    //DbgMsg("mValue ", mValue);
    if (!mExpanded)
      mBlink = true;
    SetDirty();
  }
  mExpanded = false;
}

void IVDropDownListControl::OnMouseOut()
{
  mState = StateFromNormalized();
  mExpanded = false;
  mLastX = mLastY = -1.0;
  SetDirty(false);
  //DbgMsg("mState ", mState);
  //DbgMsg("mValue ", mValue);
}

void IVDropDownListControl::ExpandRects()
{
  // expand from top left of init Rect
  auto ir = GetInitRect();
  auto& l = ir.L;
  auto& t = ir.T;
  // if num states > max list height, we need more columns
  float w = (float) NumStates() / mColHeight;
  if (w < 1.0) w = 1.0;
  else w += 0.5;
  w = std::round(w);
  w *= ir.W();
  float h = (float) NumStates();
  if (mColHeight < h)
    h = (float) mColHeight;
  h *= ir.H();

  // todo add expand directions. for now only down right
  auto& mR = mRECT;
  auto& mT = mTargetRECT;
  mR = IRECT(l, t, l + w, t + h);
  if (mDrawShadows && !mEmboss)
  {
    mR.R += mShadowOffset;
    mR.B += mShadowOffset;
  }
  // we don't want expansion to collapse right around the borders, that'd be very UI unfriendly
  mT = mR.GetPadded(20.0); // todo perhaps padding should depend on display dpi
  // expansion may get over the bounds. if so, shift it
  auto br = GetUI()->GetBounds();
  auto ex = mR.R - br.R;
  if (ex > 0.0)
  {
    mR = ShiftRectBy(mR, -ex);
    mT = ShiftRectBy(mT, -ex);
  }
  auto ey = mR.B - br.B;
  if (ey > 0.0)
  {
    mR = ShiftRectBy(mR, 0.0, -ey);
    mT = ShiftRectBy(mT, 0.0, -ey);
  }

  mExpanded = true;
  SetDirty(false);
}

void IVDropDownListControl::SetNames(int numStates, const char* names, va_list args)
{
  if (numStates < 1) return;
  mValNames.Add(new WDL_String(names));
  for (int i = 1; i < numStates; ++i)
    mValNames.Add(new WDL_String(va_arg(args, const char*)));
}

void IVDropDownListControl::SetNames(int numStates, const char* names...)
{
  mValNames.Empty(true);

  va_list args;
  va_start(args, names);
  SetNames(numStates, names, args);
  va_end(args);

  SetDirty(false);
}

void IVDropDownListControl::FillNamesFromParamDisplayTexts()
{
  mValNames.Empty(true);
  auto param = GetParam();
  if (param)
  {
    int n = param->NDisplayTexts();
    if (n > 0)
      for (int i = 0; i < n; ++i)
        mValNames.Add(new WDL_String(param->GetDisplayTextAtIdx(i)));
    else
      mValNames.Add(new WDL_String("no display texts"));
  }
  else
    mValNames.Add(new WDL_String("no param"));

  SetDirty(false);
}