#pragma once
#ifndef NDEBUG

#include "IControl.h"

class IGraphicsLiveEdit : public IControl
{
public:
  IGraphicsLiveEdit(IPlugBaseGraphics& plug, const char* pathToSourceFile, int gridSize)
  : IControl(plug, IRECT(0, 0, 1, 1))
  , mPathToSourceFile(pathToSourceFile)
  , mGridSize(gridSize)
  {
    mTargetRECT = mRECT;
    mBlend.mWeight = 0.2f;
  }
  
  ~IGraphicsLiveEdit() {}

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
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
          WDL_String json;
          pControl->GetJSON(json, c);
          //printf("%s\n", json.Get());
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
  
  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    if(mMouseClickedOnResizeHandle)
    {
      IControl* pControl = GetGUI()->GetControl(mClickedOnControl);
      IRECT r = pControl->GetRECT();
      float w = r.R - r.L;
      float h = r.B - r.T;
      
      if(w < 0.f || h < 0.f)
      {
        pControl->SetRECT(mMouseDownRECT);
        pControl->SetTargetRECT(mMouseDownTargetRECT);
      }
    }
    mClickedOnControl = -1;
    mMouseClickedOnResizeHandle = false;
    GetGUI()->SetAllControlsDirty();
  }
  
  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override
  {
  }
  
  void OnMouseOver(float x, float y, const IMouseMod& mod) override
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
  
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    if(mClickedOnControl > 0)
    {
      IControl* pControl = GetGUI()->GetControl(mClickedOnControl);
      IRECT r = pControl->GetRECT();
      
      if(mMouseClickedOnResizeHandle)
      {
        r.R = SnapToGrid(mMouseDownRECT.R + (x - mMouseDownX));
        r.B = SnapToGrid(mMouseDownRECT.B + (y - mMouseDownY));
        
        if(r.R < mMouseDownRECT.L +mGridSize) r.R = mMouseDownRECT.L+mGridSize;
        if(r.B < mMouseDownRECT.T +mGridSize) r.B = mMouseDownRECT.T+mGridSize;
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
  
  void Draw(IGraphics& graphics) override
  {
    graphics.DrawGrid(mGridColor, graphics.GetDrawRect(), mGridSize, mGridSize, &mBlend);
    
    for(int i = 1; i < graphics.GetNControls(); i++)
    {
      IControl* pControl = graphics.GetControl(i);
      IRECT cr = pControl->GetRECT();
      
      
      if(pControl->IsHidden())
        graphics.DrawDottedRect(COLOR_RED, cr);
      else if(pControl->IsGrayed())
        graphics.DrawDottedRect(COLOR_GREEN, cr);
      else
        graphics.DrawDottedRect(COLOR_BLUE, cr);
      
      IRECT h = GetHandleRect(cr);
      graphics.FillTriangle(mRectColor, h.L, h.B, h.R, h.B, h.R, h.T);
      graphics.DrawTriangle(COLOR_BLACK, h.L, h.B, h.R, h.B, h.R, h.T);
    }
  }
  
  bool IsDirty() override { return true; }

  inline IRECT GetHandleRect(IRECT& r)
  {
    return IRECT(r.R - RESIZE_HANDLE_SIZE, r.B - RESIZE_HANDLE_SIZE, r.R, r.B);
  }

  inline float SnapToGrid(float input)
  {
    if (mGridSize > 1)
      return std::round((input / mGridSize) * (float) mGridSize);
    else
      return input;
  }

private:
  bool mEditModeActive = false;
  bool mLiveEditingEnabled = false;
  bool mMouseClickedOnResizeHandle = false;
  bool mMouseIsDragging = false;
  WDL_String mPathToSourceFile;
  WDL_String mErrorMessage;

  IColor mGridColor = COLOR_GRAY;
  IColor mRectColor = COLOR_WHITE;
  static const int RESIZE_HANDLE_SIZE = 10;

  IRECT mMouseDownRECT = IRECT(0, 0, 0, 0);
  IRECT mMouseDownTargetRECT = IRECT(0, 0, 0, 0);

  float mMouseDownX = 0;
  float mMouseDownY = 0;
  
  int mGridSize = 10;
  int mClickedOnControl = -1;
};

#endif // !NDEBUG
