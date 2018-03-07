/** A vector drop down list.
Put this control on top of a draw stack
so that the expanded list is fully visible
and doesn't close when mouse is over another control */
class IVDropDownListControl : public IControl,
 public IVectorBase
{
 typedef WDL_PtrList<WDL_String> strBuf;

 static const IColor IVDropDownListControl::DEFAULT_BG_COLOR;
 static const IColor IVDropDownListControl::DEFAULT_FR_COLOR;
 static const IColor IVDropDownListControl::DEFAULT_TXT_COLOR;
 static const IColor IVDropDownListControl::DEFAULT_HL_COLOR;

 // map to IVectorBase colors
 enum EVBColor
 {
   lTxt = kFG,
   lBG = kBG,
   lHL = kHL,
   lFR = kFR
 };

public:

 IVDropDownListControl(IDelegate& dlg, IRECT rect, int param);
 IVDropDownListControl(IDelegate& dlg, IRECT rect, int param,
                int numStates, const char* names...);

 ~IVDropDownListControl()
 {
   mValNames.Empty(true);
 };

 void Draw(IGraphics& graphics) override;
 void OnMouseOver(float x, float y, const IMouseMod& mod) override;
 void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
 {
   OnMouseOver(x, y, mod);
 }
 void OnMouseDown(float x, float y, const IMouseMod& mod) override;
 void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override;
 void OnMouseDblClick(float x, float y, const IMouseMod& mod) override;
 void OnMouseOut() override;
 void OnMouseUp(float x, float y, const IMouseMod& mod) override
 {
   mBlink = false;
   SetDirty(false);
 }
 void OnResize() override;

 void SetDrawBorders(bool draw)
 {
   mDrawBorders = draw;
   SetDirty(false);
 }
 void SetDrawShadows(bool draw, bool keepButtonRect = true);
 void SetEmboss(bool emboss, bool keepButtonRect = true);
 void SetShadowOffset(float offset, bool keepButtonRect = true);
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
 void SetNames(int numStates, const char* names...);
 void FillNamesFromParamDisplayTexts();

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

 void SetNames(int numStates, const char* names, va_list args);
 auto NameForVal(int val)
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

 IRECT GetInitRect();
 IRECT GetExpandedRect();
 void ExpandRects();
 void ShrinkRects()
 {
   mTargetRECT = mRECT = mInitRect;
 }
 void UpdateRectsOnInitChange();

 void DrawInnerShadowForRect(IRECT r, IColor shadowColor, IGraphics& graphics);
 void DrawOuterShadowForRect(IRECT r, IColor shadowColor, IGraphics& graphics)
 {
   auto sr = ShiftRectBy(r, mShadowOffset, mShadowOffset);
   graphics.FillRect(shadowColor, sr);
 }
 IRECT GetRectToAlignTextIn(IRECT r);
 IRECT ShiftRectBy(IRECT r, float x, float y = 0.0)
 {
   return IRECT(r.L + x, r.T + y, r.R + x, r.B + y);
 }

 void DbgMsg(const char* msg, float val)
 {
#ifdef _DEBUG
   char str[32];
   int p = 0;
   while (*msg != '\0')
   {
     str[p] = *msg;
     ++msg;
     ++p;
   }
   sprintf(str + p, "%f", val);
   DBGMSG(str);
#endif
 }
};