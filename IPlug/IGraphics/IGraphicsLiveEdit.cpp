#include "IGraphicsLiveEdit.h"

void IGraphicsLiveEdit::OnMouseDown(int x, int y, const IMouseMod& mod)
{
  int c = GetGUI()->GetMouseControlIdx(x, y, true);
  
  if (c > 0)
  {
    mMouseDownX = x;
    mMouseDownY = y;
    
    IControl* pControl = GetGUI()->GetControl(c);
    mMouseDownRECT = pControl->GetRECT();
    mMouseDownTargetRECT = pControl->GetTargetRECT();
    mClickedOnControl = c;

    if(GetHandleRect(mMouseDownRECT).Contains(x, y))
    {
      mMouseClickedOnResizeHandle = true;
    }
    else
    {
      if(mod.A)
      {
        mClickedOnControl = GetGUI()->AttachControl(new IPanelControl(mPlug, mMouseDownRECT, COLOR_BLACK));
      }
    }
  }
  else if(mod.R)
  {
    IPopupMenu menu;
    menu.AddItem("IBitmapControl");
    menu.AddItem("IKnobLineControl");

    GetGUI()->CreateIPopupMenu(menu, x, y);
  }
}

void IGraphicsLiveEdit::OnMouseUp(int x, int y, const IMouseMod& mod)
{
  if(mMouseClickedOnResizeHandle)
  {
    IControl* pControl = GetGUI()->GetControl(mClickedOnControl);
    IRECT r = pControl->GetRECT();
    int w = r.R - r.L;
    int h = r.B - r.T;

    if(w < 0 || h < 0)
    {
      pControl->SetRECT(mMouseDownRECT);
      pControl->SetTargetRECT(mMouseDownTargetRECT);
    }
  }
  mClickedOnControl = -1;
  mMouseClickedOnResizeHandle = false;
  GetGUI()->SetAllControlsDirty();
}

void IGraphicsLiveEdit::OnMouseOver(int x, int y, const IMouseMod& mod)
{
  int c = GetGUI()->GetMouseControlIdx(x, y, true);
  if (c > 0)
  {
    IRECT cr = GetGUI()->GetControl(c)->GetRECT();
    IRECT h = GetHandleRect(cr);
    
    if(h.Contains(x, y))
    {
      SetCursor(LoadCursor(NULL, IDC_SIZENWSE));
      return;
    }
    else
      SetCursor(LoadCursor(NULL, IDC_HAND));
  }
  else
    SetCursor(LoadCursor(NULL, IDC_ARROW));
}

void IGraphicsLiveEdit::OnMouseDrag(int x, int y, int dX, int dY, const IMouseMod& mod)
{
  if(mClickedOnControl > 0)
  {
    IControl* pControl = GetGUI()->GetControl(mClickedOnControl);
    IRECT r = pControl->GetRECT();
    
    if(mMouseClickedOnResizeHandle)
    {
      r.R = SnapToGrid(mMouseDownRECT.R + (x - mMouseDownX));
      r.B = SnapToGrid(mMouseDownRECT.B + (y - mMouseDownY));
    }
    else
    {
      r.L = SnapToGrid(mMouseDownRECT.L + (x - mMouseDownX));
      r.T = SnapToGrid(mMouseDownRECT.T + (y - mMouseDownY));
      r.R = r.L + mMouseDownRECT.W();
      r.B = r.T + mMouseDownRECT.H();
    }
    
    pControl->SetRECT(r);
    pControl->SetTargetRECT(r);
    GetGUI()->SetAllControlsDirty();
  }
}

void IGraphicsLiveEdit::Draw(IGraphics& graphics)
{
  graphics.DrawGrid(mGridColor, mRECT, mGridSize, mGridSize, &mBlend);
  
  for(int i = 1; i < graphics.GetNControls(); i++)
  {
    IRECT cr = graphics.GetControl(i)->GetRECT();
    
    graphics.DrawDottedRect(mRectColor, cr);
    
    IRECT h = GetHandleRect(cr);
    graphics.FillTriangle(mRectColor, h.L, h.B, h.R, h.B, h.R, h.T);
    graphics.DrawTriangle(COLOR_BLACK, h.L, h.B, h.R, h.B, h.R, h.T);
  }
}
