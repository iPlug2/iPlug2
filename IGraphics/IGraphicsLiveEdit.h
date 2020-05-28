/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc IGraphicsLiveEdit
 */

#ifndef NDEBUG

#include "IControl.h"
#include <fstream>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** A control to enable live modification of control layout in an IGraphics context in debug builds
 * This is based on the work of Youlean, who first included it in iPlug-Youlean
 * The lives outside the main IGraphics control stack and it can be added with IGraphics::EnableLiveEdit().
 * It should not be used in the main control stack.
 * @ingroup SpecialControls */
class IGraphicsLiveEdit : public IControl
{
public:
  IGraphicsLiveEdit(bool mouseOversEnabled)
  : IControl(IRECT())
  , mGridSize(10)
  , mMouseOversEnabled(mouseOversEnabled) 
  {
    mTargetRECT = mRECT;
  }
  
  ~IGraphicsLiveEdit()
  {
    GetUI()->EnableMouseOver(mMouseOversEnabled); // Set it back to what it was
  }
  
  void OnInit() override
  {
    GetUI()->EnableMouseOver(true);
  }

  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    int c = GetUI()->GetMouseControlIdx(x, y, true);
    
    if (c > 0)
    {
      IControl* pControl = GetUI()->GetControl(c);
      mMouseDownRECT = pControl->GetRECT();
      mMouseDownTargetRECT = pControl->GetTargetRECT();

      if(!mod.S)
        mSelectedControls.Empty();
      
      mSelectedControls.Add(pControl);

      if(mod.A)
      {
        GetUI()->AttachControl(new PlaceHolder(mMouseDownRECT));
        mClickedOnControl = GetUI()->NControls() - 1;
        mMouseClickedOnResizeHandle = false;
      }
      else if (mod.R)
      {
        mClickedOnControl = c;
        GetUI()->CreatePopupMenu(*this, mRightClickOnControlMenu, x, y);
      }
      else
      {
        mClickedOnControl = c;
        
        if(GetHandleRect(mMouseDownRECT).Contains(x, y))
        {
          mMouseClickedOnResizeHandle = true;
        }
      }
    }
    else if(mod.R)
    {
      GetUI()->CreatePopupMenu(*this, mRightClickOutsideControlMenu, x, y);
    }
    else
    {
      mSelectedControls.Empty();
      mDragRegion.L = mDragRegion.R = x;
      mDragRegion.T = mDragRegion.B = y;
    }
  }
  
  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    if(mMouseClickedOnResizeHandle)
    {
      IControl* pControl = GetUI()->GetControl(mClickedOnControl);
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
    GetUI()->SetAllControlsDirty();
    
    mDragRegion = IRECT();
  }
  
  void OnMouseDblClick(float x, float y, const IMouseMod& mod) override
  {
  }
  
  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    int c = GetUI()->GetMouseControlIdx(x, y, true);
    if (c > 0)
    {
      IRECT cr = GetUI()->GetControl(c)->GetRECT();
      IRECT h = GetHandleRect(cr);
      
      if(h.Contains(x, y))
      {
        GetUI()->SetMouseCursor(ECursor::SIZENWSE);
        return;
      }
      else
        GetUI()->SetMouseCursor(ECursor::HAND);
    }
    else
      GetUI()->SetMouseCursor(ECursor::ARROW);
  }
  
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod) override
  {
    float mouseDownX, mouseDownY;
    GetUI()->GetMouseDownPoint(mouseDownX, mouseDownY);
    
    if(mClickedOnControl > 0)
    {
      IControl* pControl = GetUI()->GetControl(mClickedOnControl);
      
      if(mMouseClickedOnResizeHandle)
      {
        IRECT r = pControl->GetRECT();
        r.R = SnapToGrid(mMouseDownRECT.R + (x - mouseDownX));
        r.B = SnapToGrid(mMouseDownRECT.B + (y - mouseDownY));
        
        if(r.R < mMouseDownRECT.L +mGridSize) r.R = mMouseDownRECT.L+mGridSize;
        if(r.B < mMouseDownRECT.T +mGridSize) r.B = mMouseDownRECT.T+mGridSize;
          
        pControl->SetSize(r.W(), r.H());
      }
      else
      {
        const float x1 = SnapToGrid(mMouseDownRECT.L + (x - mouseDownX));
        const float y1 = SnapToGrid(mMouseDownRECT.T + (y - mouseDownY));
          
        pControl->SetPosition(x1, y1);
      }

      GetUI()->SetAllControlsDirty();
    }
    else
    {
      float mouseDownX, mouseDownY;
      GetUI()->GetMouseDownPoint(mouseDownX, mouseDownY);
      mDragRegion.L = x < mouseDownX ? x : mouseDownX;
      mDragRegion.R = x < mouseDownX ? mouseDownX : x;
      mDragRegion.T = y < mouseDownY ? y : mouseDownY;
      mDragRegion.B = y < mouseDownY ? mouseDownY : y;
      
      GetUI()->ForStandardControlsFunc([&](IControl& c) {
                                         if(mDragRegion.Contains(c.GetRECT())) {
                                           if(mSelectedControls.FindR(&c) == -1)
                                             mSelectedControls.Add(&c);
                                         }
                                         else {
                                           int idx = mSelectedControls.FindR(&c);
                                           if(idx > -1)
                                             mSelectedControls.Delete(idx);
                                         }
                                       });
    }
  }
  
  bool OnKeyDown(float x, float y, const IKeyPress& key) override
  {
    GetUI()->ReleaseMouseCapture();
    
    if(key.VK == kVK_BACK || key.VK == kVK_DELETE)
    {
      if(mSelectedControls.GetSize())
      {
        for(int i = 0; i < mSelectedControls.GetSize(); i++)
        {
          IControl* pControl = mSelectedControls.Get(i);
          GetUI()->RemoveControl(pControl);
        }
        
        mSelectedControls.Empty();
        GetUI()->SetAllControlsDirty();
        
        return true;
      }
    }
    
    return false;
  }
  
  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
  {
    if(pSelectedMenu && pSelectedMenu == &mRightClickOutsideControlMenu)
    {
      auto idx = pSelectedMenu->GetChosenItemIdx();
      float x, y;
      GetUI()->GetMouseDownPoint(x, y);
      IRECT b = IRECT(x, y, x + 100.f, y + 100.f);

      switch(idx)
      {
        case 0:
          GetUI()->AttachControl(new PlaceHolder(b));
          break;
        default:
          break;
      }
    }

    if (pSelectedMenu && pSelectedMenu == &mRightClickOnControlMenu)
    {
      auto idx = pSelectedMenu->GetChosenItemIdx();

      switch (idx)
      {
        case 0:
          mSelectedControls.Empty();
          GetUI()->RemoveControl(mClickedOnControl);
          mClickedOnControl = -1;
          break;
        default:
          break;
      }

    }
  }
  
  void Draw(IGraphics& g) override
  {
    IBlend b {EBlend::Add, 0.25f};
    g.DrawGrid(mGridColor, g.GetBounds(), mGridSize, mGridSize, &b);
    
    for(int i = 1; i < g.NControls(); i++)
    {
      IControl* pControl = g.GetControl(i);
      IRECT cr = pControl->GetRECT();

      if(pControl->IsHidden())
        g.DrawDottedRect(COLOR_RED, cr);
      else if(pControl->IsDisabled())
        g.DrawDottedRect(COLOR_GREEN, cr);
      else
        g.DrawDottedRect(COLOR_BLUE, cr);
      
      IRECT h = GetHandleRect(cr);
      g.FillTriangle(mRectColor, h.L, h.B, h.R, h.B, h.R, h.T);
      g.DrawTriangle(COLOR_BLACK, h.L, h.B, h.R, h.B, h.R, h.T);
    }
    
    for(int i = 0; i< mSelectedControls.GetSize(); i++)
    {
      g.DrawDottedRect(COLOR_WHITE, mSelectedControls.Get(i)->GetRECT());
    }
    
    if(!mDragRegion.Empty())
    {
      g.DrawDottedRect(COLOR_RED, mDragRegion);
    }
  }
  
  void OnResize() override
  {
    mSelectedControls.Empty();
    mRECT = GetUI()->GetBounds();
    SetTargetRECT(mRECT);
  }
  
  bool IsDirty() override { return true; }

  inline IRECT GetHandleRect(const IRECT& r)
  {
    return IRECT(r.R - RESIZE_HANDLE_SIZE, r.B - RESIZE_HANDLE_SIZE, r.R, r.B);
  }

  inline float SnapToGrid(float input)
  {
    if (mGridSize > 1)
      return (float) std::round(input / (float) mGridSize) * mGridSize;
    else
      return input;
  }

private:
  IPopupMenu mRightClickOutsideControlMenu {"Outside Control", {"Add Place Holder"}};
  IPopupMenu mRightClickOnControlMenu{ "On Control", {"Delete Control"} };

  bool mMouseOversEnabled = false;
  bool mMouseClickedOnResizeHandle = false;
  bool mMouseIsDragging = false;
  WDL_String mErrorMessage;
  WDL_PtrList<IControl> mSelectedControls;

  IColor mGridColor = COLOR_WHITE;
  IColor mRectColor = COLOR_WHITE;
  static const int RESIZE_HANDLE_SIZE = 10;

  IRECT mMouseDownRECT;
  IRECT mMouseDownTargetRECT;
  IRECT mDragRegion;

  float mGridSize = 10;
  int mClickedOnControl = -1;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

#endif // !NDEBUG
