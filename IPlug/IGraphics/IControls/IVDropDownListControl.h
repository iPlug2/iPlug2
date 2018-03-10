#pragma once
#include "IControl.h"

/** A vector drop down list.
Put this control on top of a draw stack
so that the expanded list is fully visible
and doesn't close when mouse is over another control */

class IVDropDownListControl : public IControl,
                              public IVectorBase
{
 typedef WDL_PtrList<WDL_String> strBuf;

 static const IColor DEFAULT_BG_COLOR;
 static const IColor DEFAULT_FR_COLOR;
 static const IColor DEFAULT_TXT_COLOR;
 static const IColor DEFAULT_HL_COLOR;

 // map to IVectorBase colors
 enum EVBColor
 {
   lTxt = kFG,
   lBG = kBG,
   lHL = kHL,
   lFR = kFR
 };

public:

  IVDropDownListControl(IDelegate& dlg, IRECT bounds, int paramIdx)
  : IControl(dlg, bounds, paramIdx)
  , IVectorBase(&DEFAULT_BG_COLOR, &DEFAULT_TXT_COLOR, &DEFAULT_FR_COLOR, &DEFAULT_HL_COLOR)
  {
    mInitRect = bounds;
    mText.mFGColor = DEFAULT_TXT_COLOR;
    FillNamesFromParamDisplayTexts();
  }
  
  IVDropDownListControl(IDelegate& dlg, IRECT bounds, int numStates, const char* names...)
  : IControl(dlg, bounds, kNoParameter)
  , IVectorBase(&DEFAULT_BG_COLOR, &DEFAULT_TXT_COLOR, &DEFAULT_FR_COLOR, &DEFAULT_HL_COLOR)
  {
    mInitRect = bounds;
    mText.mFGColor = DEFAULT_TXT_COLOR;
    if (numStates)
    {
      va_list args;
      va_start(args, names);
      SetNames(numStates, names, args);
      va_end(args);
    }
    else
      FillNamesFromParamDisplayTexts();
  }

 ~IVDropDownListControl()
 {
   mValNames.Empty(true);
 }

 void Draw(IGraphics& g) override
 {
   auto initR = GetInitRect();
   auto shadowColor = IColor(60, 0, 0, 0);

   auto textR = GetRectToAlignTextIn(initR);

   if (!mExpanded)
   {
     if (mDrawShadows && !mEmboss)
       DrawOuterShadowForRect(initR, shadowColor, g);

     if (mBlink)
     {
       mBlink = false;
       g.FillRect(GetColor(lHL), initR);
       SetDirty(false);
     }
     else
       g.FillRect(GetColor(lBG), initR);

     if (mDrawBorders)
       g.DrawRect(GetColor(lFR), initR);
     g.DrawText(mText, NameForVal(StateFromNormalized()), textR);
     ShrinkRects(); // shrink here to clean the expanded area
   }

   else
   {
     auto panelR = GetExpandedRect();
     if (mDrawShadows && !mEmboss)
       DrawOuterShadowForRect(panelR, shadowColor, g);
     g.FillRect(GetColor(lBG), panelR);
     if (mDrawShadows && mEmboss)
       DrawInnerShadowForRect(panelR, shadowColor, g);
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
           DrawOuterShadowForRect(vR, shadowColor, g);
         g.FillRect(GetColor(lHL), vR);
       }

       if (mDrawBorders)
         g.DrawRect(GetColor(lFR), vR);
       g.DrawText(mText, NameForVal(v), tR);
       ++sy;
     }

     if (mDrawBorders)
     {
       if (!mDrawShadows) // panelRect == mRECT
       {
         --panelR.R; // fix for strange graphics behavior
         --panelR.B; // mRECT right and bottom are not drawn in expanded state (on Win)
       }
       g.DrawRect(GetColor(lFR), panelR);
     }
   }

#ifdef _DEBUG
   //g.DrawRect(COLOR_ORANGE, mInitRect);
   //g.DrawRect(COLOR_BLUE, mRECT);
   //g.DrawRect(COLOR_GREEN, mTargetRECT); // if padded will not be drawn correctly
#endif

 }

 void OnMouseOver(float x, float y, const IMouseMod& mod) override
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
         //DBGMSG("%s %f" ,"mState ", mState);
         SetDirty(false);
       }
     }
   }
 }

 void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
 {
   OnMouseOver(x, y, mod);
 }

 void OnMouseDown(float x, float y, const IMouseMod& mod) override
 {
   if (!mExpanded)
     ExpandRects();
   else
   {
     mExpanded = false;
     mValue = NormalizedFromState();
     SetDirty();
   }
   //DBGMSG("%s %f" ,"mValue ", mValue);
 }

 void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override
 {
   int ns = mState;
   ns += (int)d;
   ns = BOUNDED(ns, 0, NumStates() - 1);
   if (ns != mState)
   {
     mState = ns;
     mValue = NormalizedFromState();
     //DBGMSG("%s %f" ,"mState ", mState);
     //DBGMSG("%s %f" ,"mValue ", mValue);
     if (!mExpanded)
       mBlink = true;
     SetDirty();
   }
 }

 void OnMouseDblClick(float x, float y, const IMouseMod& mod) override
 {
   mValue = mDefaultValue;
   int ns = StateFromNormalized();
   if (mState != ns)
   {
     mState = ns;
     mValue = NormalizedFromState();
     //DBGMSG("%s %f" ,"mState ", mState);
     //DBGMSG("%s %f" ,"mValue ", mValue);
     if (!mExpanded)
       mBlink = true;
     SetDirty();
   }
   mExpanded = false;
 }

 void OnMouseOut() override
 {
   mState = StateFromNormalized();
   mExpanded = false;
   mLastX = mLastY = -1.0;
   SetDirty(false);
   //DBGMSG("%s %f" ,"mState ", mState);
   //DBGMSG("%s %f" ,"mValue ", mValue);
 }

 void OnMouseUp(float x, float y, const IMouseMod& mod) override
 {
   mBlink = false;
   SetDirty(false);
 }

 void OnResize() override
 {
   mInitRect = mRECT;
   mExpanded = false;
   mLastX = mLastY = -1.0;
   mBlink = false;
   SetDirty(false);
 }

 void SetDrawBorders(bool draw)
 {
   mDrawBorders = draw;
   SetDirty(false);
 }

 void SetDrawShadows(bool draw, bool keepButtonRect = true)
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

 void SetEmboss(bool emboss, bool keepButtonRect = true)
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

 void SetShadowOffset(float offset, bool keepButtonRect = true)
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

 void SetRect(IRECT r)
 {
   mInitRect = r;
   UpdateRectsOnInitChange();
   mLastX = mLastY = -1.0;
   SetDirty(false);
 }

 void SetMaxListHeight(int numItems)
 {
   mColHeight = numItems;
 }

 void SetNames(int numStates, const char* names...)
 {
   mValNames.Empty(true);

   va_list args;
   va_start(args, names);
   SetNames(numStates, names, args);
   va_end(args);

   SetDirty(false);
 }

 void FillNamesFromParamDisplayTexts()
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

protected:
 IRECT mInitRect;
 strBuf mValNames;

 bool mExpanded = false;
 bool mBlink = false;
 bool mDrawBorders = true;
 bool mDrawShadows = true;
 bool mEmboss = false;
 float mShadowOffset = 3.0;

 float mLastX = -1.0; // to avoid lots of useless extra computations
 float mLastY = -1.0;
 int mState = -1;

 int mColHeight = 5; // how long the list can get before adding a new column

 void SetNames(int numStates, const char* names, va_list args)
 {
   if (numStates < 1) return;
   mValNames.Add(new WDL_String(names));
   for (int i = 1; i < numStates; ++i)
     mValNames.Add(new WDL_String(va_arg(args, const char*)));
 }

 const char* NameForVal(int val)
 {
   return (mValNames.Get(val))->Get();
 }

 int NumStates()
 {
   return mValNames.GetSize();
 }

 double NormalizedFromState()
 {
   if (NumStates() < 2)
     return 0.0;
   else
     return (double) mState / (NumStates() - 1);
 }

 int StateFromNormalized()
 {
   return (int) (mValue * (NumStates() - 1));
 }

 IRECT GetInitRect()
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

 IRECT GetExpandedRect()
 {
   auto er = mRECT;
   if (mDrawShadows && !mEmboss)
   {
     er.R -= mShadowOffset;
     er.B -= mShadowOffset;
   }
   return er;
 }

 void ExpandRects()
 {
   // expand from top left of init Rect
   auto ir = GetInitRect();
   auto& l = ir.L;
   auto& t = ir.T;
   // if num states > max list height, we need more columns
   float w = (float)NumStates() / mColHeight;
   if (w < 1.0) w = 1.0;
   else w += 0.5;
   w = std::round(w);
   w *= ir.W();
   float h = (float)NumStates();
   if (mColHeight < h)
     h = (float)mColHeight;
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

 void ShrinkRects()
 {
   mTargetRECT = mRECT = mInitRect;
 }

 void UpdateRectsOnInitChange()
 {
   if (!mExpanded)
     ShrinkRects();
   else
     ExpandRects();
 }

 void DrawInnerShadowForRect(IRECT r, IColor shadowColor, IGraphics& g)
 {
   auto& o = mShadowOffset;
   auto slr = r;
   slr.R = slr.L + o;
   auto str = r;
   str.L += o;
   str.B = str.T + o;
   g.FillRect(shadowColor, slr);
   g.FillRect(shadowColor, str);
 }

 void DrawOuterShadowForRect(IRECT r, IColor shadowColor, IGraphics& g)
 {
   auto sr = ShiftRectBy(r, mShadowOffset, mShadowOffset);
   g.FillRect(shadowColor, sr);
 }

 IRECT GetRectToAlignTextIn(IRECT r)
 {
   // this rect is not precise, it serves as a horizontal level
   auto tr = r;
   // assume all items are 1 line high
   tr.T += 0.5f * (tr.H() - mText.mSize) - 1.0f; // -1 looks better with small text
   tr.B = tr.T + 0.1f;
   return tr;
 }

 IRECT ShiftRectBy(IRECT r, float x, float y = 0.0)
 {
   return IRECT(r.L + x, r.T + y, r.R + x, r.B + y);
 }
};

const IColor IVDropDownListControl::DEFAULT_BG_COLOR = IColor(255, 200, 200, 200);
const IColor IVDropDownListControl::DEFAULT_FR_COLOR = IColor(255, 70, 70, 70);
const IColor IVDropDownListControl::DEFAULT_TXT_COLOR = DEFAULT_FR_COLOR;
const IColor IVDropDownListControl::DEFAULT_HL_COLOR = IColor(255, 240, 240, 240);
