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

#if !defined(NDEBUG) || defined(IPLUG_LIVE_EDIT)

#include "IControl.h"
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

#if defined(IPLUG_LIVE_EDIT_CLASS_NAME)
#include <cstring>
#include <typeinfo>
#if defined(__GNUG__) && !defined(_WIN32)
#include <cxxabi.h>
#include <cstdlib>
#endif
#endif

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
  , mMouseOversEnabled(mouseOversEnabled)
  , mGridSize(10)
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

      if (!mod.S)
        mSelectedControls.Empty();
      
      mSelectedControls.Add(pControl);

      if (mod.A)
      {
        GetUI()->AttachControl(new PlaceHolder(mMouseDownRECT));
        mClickedOnControl = GetUI()->NControls() - 1;
        mMouseClickedOnResizeHandle = false;
        EmitControlAdded(GetUI()->GetControl(mClickedOnControl));
      }
      else if (mod.R)
      {
        mClickedOnControl = c;
        GetUI()->CreatePopupMenu(*this, mRightClickOnControlMenu, x, y);
      }
      else
      {
        mClickedOnControl = c;
        
        if (GetHandleRect(mMouseDownRECT).Contains(x, y))
        {
          mMouseClickedOnResizeHandle = true;
        }
      }
    }
    else if (mod.R)
    {
      GetUI()->CreatePopupMenu(*this, mRightClickOutsideControlMenu, x, y);
    }
    else
    {
      mSelectedControls.Empty();
      mDragRegion.L = mDragRegion.R = x;
      mDragRegion.T = mDragRegion.B = y;
    }

    EmitSelectionChanged();
  }
  
  void OnMouseUp(float x, float y, const IMouseMod& mod) override
  {
    // Web popup menus are async, so keep the clicked control index alive
    // until OnPopupMenuSelection receives either a selection or a cancel.
    const bool waitingForControlPopup = mod.R && mClickedOnControl > 0;

    if (mClickedOnControl > 0)
    {
      IControl* pControl = GetUI()->GetControl(mClickedOnControl);
      IRECT r = pControl->GetRECT();

      if (mMouseClickedOnResizeHandle)
      {
        float w = r.R - r.L;
        float h = r.B - r.T;

        if (w < 0.f || h < 0.f)
        {
          pControl->SetRECT(mMouseDownRECT);
          pControl->SetTargetRECT(mMouseDownTargetRECT);
          r = pControl->GetRECT();
        }
      }

      if (r.L != mMouseDownRECT.L || r.T != mMouseDownRECT.T ||
          r.R != mMouseDownRECT.R || r.B != mMouseDownRECT.B)
      {
        EmitControlChanged(pControl, mMouseDownRECT);
      }
    }

    if (!waitingForControlPopup)
      mClickedOnControl = -1;

    mMouseClickedOnResizeHandle = false;
    GetUI()->SetAllControlsDirty();

    EmitSelectionChanged();

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
      
      if (h.Contains(x, y))
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
    
    if (mClickedOnControl > 0)
    {
      IControl* pControl = GetUI()->GetControl(mClickedOnControl);
      
      if (mMouseClickedOnResizeHandle)
      {
        IRECT r = pControl->GetRECT();
        r.R = SnapToGrid(mMouseDownRECT.R + (x - mouseDownX));
        r.B = SnapToGrid(mMouseDownRECT.B + (y - mouseDownY));
        
        if (r.R < mMouseDownRECT.L +mGridSize) r.R = mMouseDownRECT.L+mGridSize;
        if (r.B < mMouseDownRECT.T +mGridSize) r.B = mMouseDownRECT.T+mGridSize;
          
        GetUI()->SetControlSize(pControl, r.W(), r.H());
      }
      else
      {
        const float x1 = SnapToGrid(mMouseDownRECT.L + (x - mouseDownX));
        const float y1 = SnapToGrid(mMouseDownRECT.T + (y - mouseDownY));
          
        GetUI()->SetControlPosition(pControl, x1, y1);
      }
    }
    else
    {
      float mouseDownX, mouseDownY;
      GetUI()->GetMouseDownPoint(mouseDownX, mouseDownY);
      mDragRegion.L = x < mouseDownX ? x : mouseDownX;
      mDragRegion.R = x < mouseDownX ? mouseDownX : x;
      mDragRegion.T = y < mouseDownY ? y : mouseDownY;
      mDragRegion.B = y < mouseDownY ? mouseDownY : y;
      
      GetUI()->ForStandardControlsFunc([&](IControl* pControl) {
                                         if (mDragRegion.Contains(pControl->GetRECT())) {
                                           if (mSelectedControls.FindR(pControl) == -1)
                                             mSelectedControls.Add(pControl);
                                         }
                                         else {
                                           int idx = mSelectedControls.FindR(pControl);
                                           if (idx > -1)
                                             mSelectedControls.Delete(idx);
                                         }
                                       });
    }
  }
  
  bool OnKeyDown(float x, float y, const IKeyPress& key) override
  {
    GetUI()->ReleaseMouseCapture();
    
    if (key.VK == kVK_BACK || key.VK == kVK_DELETE)
    {
      if (mSelectedControls.GetSize())
      {
        std::vector<IControl*> snapshot;
        snapshot.reserve(mSelectedControls.GetSize());
        for (int i = 0; i < mSelectedControls.GetSize(); i++)
          snapshot.push_back(mSelectedControls.Get(i));
        EmitControlsDeleted(snapshot.data(), static_cast<int>(snapshot.size()));
        for (int i = 0; i < mSelectedControls.GetSize(); i++)
        {
          IControl* pControl = mSelectedControls.Get(i);
          GetUI()->RemoveControl(pControl);
        }
        
        mSelectedControls.Empty();
        GetUI()->SetAllControlsDirty();

        EmitSelectionChanged();

        return true;
      }
    }
    
    return false;
  }
  
  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
  {
    if (!pSelectedMenu)
    {
      mClickedOnControl = -1;
      return;
    }

    if (pSelectedMenu && pSelectedMenu == &mRightClickOutsideControlMenu)
    {
      auto idx = pSelectedMenu->GetChosenItemIdx();
      float x, y;
      GetUI()->GetMouseDownPoint(x, y);
      IRECT b = IRECT(x, y, x + 100.f, y + 100.f);

      switch(idx)
      {
        case 0:
        {
          GetUI()->AttachControl(new PlaceHolder(b));
          EmitControlAdded(GetUI()->GetControl(GetUI()->NControls() - 1));
          break;
        }
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
        {
          IControl* pToDelete = mClickedOnControl > 0 ? GetUI()->GetControl(mClickedOnControl) : nullptr;
          IControl* controls[1] = { pToDelete };
          if (pToDelete)
            EmitControlsDeleted(controls, 1);
          mSelectedControls.Empty();
          if (mClickedOnControl > 0)
            GetUI()->RemoveControl(mClickedOnControl);
          EmitSelectionChanged();
          break;
        }
        default:
          break;
      }

      mClickedOnControl = -1;
    }
  }
  
  void Draw(IGraphics& g) override
  {
    IBlend b {EBlend::Add, 0.25f};
    g.DrawGrid(mGridColor, g.GetBounds(), mGridSize, mGridSize, &b);
    
    for (int i = 1; i < g.NControls(); i++)
    {
      IControl* pControl = g.GetControl(i);
      IRECT cr = pControl->GetRECT();
      
      if (!pControl->GetParent()) // don't allow reszing sub controls
      {
        if (pControl->IsHidden())
          g.DrawDottedRect(COLOR_RED, cr);
        else if (pControl->IsDisabled())
          g.DrawDottedRect(COLOR_GREEN, cr);
        else
          g.DrawDottedRect(COLOR_BLUE, cr);
        
        IRECT h = GetHandleRect(cr);
        g.FillTriangle(mRectColor, h.L, h.B, h.R, h.B, h.R, h.T);
        g.DrawTriangle(COLOR_BLACK, h.L, h.B, h.R, h.B, h.R, h.T);
      }
    }
    
    for (int i = 0; i< mSelectedControls.GetSize(); i++)
    {
      g.DrawDottedRect(COLOR_WHITE, mSelectedControls.Get(i)->GetRECT());
    }
    
    if (!mDragRegion.Empty())
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

  static void AppendJsonString(std::string& out, const char* str)
  {
    out.push_back('"');

    if (str)
    {
      for (const char* p = str; *p; ++p)
      {
        unsigned char c = static_cast<unsigned char>(*p);

        switch (c)
        {
          case '\\': out += "\\\\"; break;
          case '"': out += "\\\""; break;
          case '\b': out += "\\b"; break;
          case '\f': out += "\\f"; break;
          case '\n': out += "\\n"; break;
          case '\r': out += "\\r"; break;
          case '\t': out += "\\t"; break;
          default:
          {
            if (c < 0x20)
            {
              char esc[8];
              std::snprintf(esc, sizeof(esc), "\\u%04x", c);
              out += esc;
            }
            else
            {
              out.push_back(static_cast<char>(c));
            }
            break;
          }
        }
      }
    }

    out.push_back('"');
  }

#if defined(IPLUG_LIVE_EDIT_CLASS_NAME)
  static std::string DemangledClassName(IControl* pControl)
  {
    if (!pControl)
      return "";

    const char* mangled = typeid(*pControl).name();
#if defined(__GNUG__) && !defined(_WIN32)
    int status = 0;
    char* demangled = abi::__cxa_demangle(mangled, nullptr, nullptr, &status);
    const char* name = (status == 0 && demangled) ? demangled : (mangled ? mangled : "");
#else
    const char* name = mangled ? mangled : "";
#endif
    static const char prefix[] = "iplug::igraphics::";
    const std::size_t prefixLen = sizeof(prefix) - 1;
    const char* compact = std::strncmp(name, prefix, prefixLen) == 0 ? name + prefixLen : name;
    std::string result(compact);

#if defined(__GNUG__) && !defined(_WIN32)
    if (demangled)
      std::free(demangled);
#endif

    return result;
  }
#endif

  void AppendControlDescriptor(std::string& out, IControl* pControl)
  {
    if (!pControl)
    {
      out += "null";
      return;
    }

    const IRECT r = pControl->GetRECT();
    out += "{\"idx\":";
    out += std::to_string(GetUI()->GetControlIdx(pControl));
#if defined(IPLUG_LIVE_EDIT_CLASS_NAME)
    out += ",\"className\":";
    AppendJsonString(out, DemangledClassName(pControl).c_str());
#endif
    out += ",\"tag\":";
    out += std::to_string(pControl->GetTag());
    out += ",\"paramIdx\":";
    out += std::to_string(pControl->GetParamIdx());
    out += ",\"l\":";
    out += std::to_string(static_cast<int>(std::round(r.L)));
    out += ",\"t\":";
    out += std::to_string(static_cast<int>(std::round(r.T)));
    out += ",\"r\":";
    out += std::to_string(static_cast<int>(std::round(r.R)));
    out += ",\"b\":";
    out += std::to_string(static_cast<int>(std::round(r.B)));
    out += "}";
  }

  static void AppendRect(std::string& out, const IRECT& r)
  {
    out += "{\"l\":";
    out += std::to_string(static_cast<int>(std::round(r.L)));
    out += ",\"t\":";
    out += std::to_string(static_cast<int>(std::round(r.T)));
    out += ",\"r\":";
    out += std::to_string(static_cast<int>(std::round(r.R)));
    out += ",\"b\":";
    out += std::to_string(static_cast<int>(std::round(r.B)));
    out += "}";
  }

  void EmitControlChanged(IControl* pControl, const IRECT& previousRECT)
  {
    if (!GetUI()->HasLiveEditEventFunc())
      return;

    std::string json = "{\"type\":\"iplug:live-edit:control-changed\",\"control\":";
    AppendControlDescriptor(json, pControl);
    json += ",\"prev\":";
    AppendRect(json, previousRECT);
    json += "}";
    GetUI()->EmitLiveEditEvent(json.c_str());
  }

  void EmitControlAdded(IControl* pControl)
  {
    if (!GetUI()->HasLiveEditEventFunc())
      return;

    std::string json = "{\"type\":\"iplug:live-edit:control-added\",\"control\":";
    AppendControlDescriptor(json, pControl);
    json += "}";
    GetUI()->EmitLiveEditEvent(json.c_str());
  }

  void EmitControlsDeleted(IControl* const* controls, int count)
  {
    if (!GetUI()->HasLiveEditEventFunc())
      return;

    if (count <= 0)
      return;

    std::string json = "{\"type\":\"iplug:live-edit:controls-deleted\",\"deleted\":[";

    for (int i = 0; i < count; i++)
    {
      if (i > 0)
        json += ",";

      AppendControlDescriptor(json, controls[i]);
    }

    json += "]}";
    GetUI()->EmitLiveEditEvent(json.c_str());
  }

  void EmitSelectionChanged()
  {
    if (!GetUI()->HasLiveEditEventFunc())
      return;

    std::string json = "{\"type\":\"iplug:live-edit:selection-changed\",\"selection\":[";

    for (int i = 0; i < mSelectedControls.GetSize(); i++)
    {
      if (i > 0)
        json += ",";

      AppendControlDescriptor(json, mSelectedControls.Get(i));
    }

    json += "]}";
    GetUI()->EmitLiveEditEvent(json.c_str());
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

#endif // !NDEBUG || IPLUG_LIVE_EDIT
