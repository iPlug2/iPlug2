#pragma once

#include "IPlug_include_in_plug_hdr.h"
#include "IControls.h"

const int kNumPrograms = 1;

enum EParams
{
  kParamGain = 0,
  kParamMode,
  kParamFreq1,
  kParamFreq2,
  kNumParams
};

enum ECtrlTags
{
  kCtrlTagDialogResult = 0,
  kCtrlTagVectorButton,
  kCtrlTagVectorSlider,
  kCtrlTagTabSwitch,
  kCtrlTagRadioButton,
  kCtrlTagScope,
  kCtrlTagMeter,
  kCtrlTags
};

using namespace iplug;
using namespace igraphics;

class IVCustomControl : public IControl
                      , public IVectorBase
{
public:
  IVCustomControl(const IRECT& bounds, const char* label, const IVStyle& style)
  : IControl(bounds)
  , IVectorBase(style)
  {
    AttachIControl(this, label);
  }
  
  void OnInit() override
  {
    mValueStr.Set("Test");
  }
  
  void Draw(IGraphics& g) override
  {
    DrawBackGround(g, mRECT);
    DrawWidget(g);
    DrawLabel(g);
    DrawValue(g, mMouseIsOver);
  }
  
  virtual void DrawWidget(IGraphics& g) override
  {
    g.FillRect(GetColor(kFG), mWidgetBounds);
  }
  
  void OnResize() override
  {
    SetTargetRECT(MakeRects(mRECT));
  }
};

class FileBrowser : public IDirBrowseControlBase
{
private:
  WDL_String mLabel;
  IBitmap mBitmap;
public:
  FileBrowser(const IRECT& bounds)
  : IDirBrowseControlBase(bounds, ".png")
  {
    WDL_String path;
    //    DesktopPath(path);
    path.Set(__FILE__);
    path.remove_filepart();
#ifdef OS_WIN
    path.Append("\\resources\\img\\");
#else
    path.Append("/resources/img/");
#endif
    AddPath(path.Get(), "");
    
    mLabel.Set("Click here to browse png files...");
  }
  
  void Draw(IGraphics& g) override
  {
    g.FillRect(COLOR_TRANSLUCENT, mRECT);
    
    IRECT labelRect = mRECT.GetFromBottom(mText.mSize);
    IRECT bmpRect = mRECT.GetReducedFromBottom(mText.mSize);
    
    if(mBitmap.GetAPIBitmap())
    {
      //if stacked frames, don't try and fit the whole bitmap to the bounds
      if(mBitmap.N())
        g.DrawBitmap(mBitmap, bmpRect, 1);
      else
        g.DrawFittedBitmap(mBitmap, bmpRect);
    }
    
    g.FillRect(COLOR_WHITE, labelRect);
    g.DrawText(mText, mLabel.Get(), labelRect);
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    SetUpMenu();
    
    GetUI()->CreatePopupMenu(*this, mMainMenu, x, y);
  }
  
  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
  {
    if(pSelectedMenu)
    {
      IPopupMenu::Item* pItem = pSelectedMenu->GetChosenItem();
      WDL_String* pStr = mFiles.Get(pItem->GetTag());
      mLabel.Set(pStr);
      mBitmap = GetUI()->LoadBitmap(pStr->Get());
      SetTooltip(pStr->Get());
      SetDirty(false);
    }
  }
};


class IPlugControls : public Plugin
{
public:
  IPlugControls(const InstanceInfo& info);

#if IPLUG_DSP // All DSP methods and member variables should be within an IPLUG_DSP guard, should you want distributed UI
  void ProcessBlock(sample** inputs, sample** outputs, int nFrames) override;
  void OnIdle() override;
private:
  IVScopeControl<2>::Sender mScopeSender { kCtrlTagScope };
  IVMeterControl<2>::Sender mMeterSender { kCtrlTagMeter };
#endif
};
