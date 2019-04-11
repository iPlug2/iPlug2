#include "IPlugControls.h"
#include "IPlug_include_in_plug_src.h"
#include "IControls.h"
#include "IPlugPaths.h"

class FileBrowser : public IDirBrowseControlBase
{
private:
  WDL_String mLabel;
  IBitmap mBitmap;
public:
  FileBrowser(IRECT bounds)
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
    
    GetUI()->CreatePopupMenu(mMainMenu, x, y, this);
  }
  
  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu) override
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

IPlugControls::IPlugControls(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  GetParam(kGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");
  GetParam(kMode)->InitEnum("Mode", 0, 4, "", IParam::kFlagsNone, "", "one", "two", "three", "four");

#if IPLUG_EDITOR // All UI methods and member variables should be within an IPLUG_EDITOR guard, should you want distributed UI
  mMakeGraphicsFunc = [&]() {
    return MakeGraphics(*this, PLUG_WIDTH, PLUG_HEIGHT, PLUG_FPS, 1.);
  };
  
  mLayoutFunc = [&](IGraphics* pGraphics) {
    if(pGraphics->NControls())
    {
      //Could handle new layout here
      return;
    }
    
//    pGraphics->EnableLiveEdit(true);
    pGraphics->HandleMouseOver(true);
    pGraphics->AttachCornerResizer(kUIResizerScale, true);
    pGraphics->AttachPanelBackground(COLOR_GRAY);
    pGraphics->EnableTooltips(true);
//    pGraphics->AttachTextEntryControl();
    
    IRECT b = pGraphics->GetBounds().GetPadded(-5);
    
    pGraphics->LoadFont("Roboto-Regular", ROBOTTO_FN);
    const IBitmap bitmap1 = pGraphics->LoadBitmap(PNGKNOB_FN, 60);
    const IBitmap bitmap2 = pGraphics->LoadBitmap(PNGKNOBROTATE_FN);
    const ISVG vectorknob = pGraphics->LoadSVG(SVGKNOBROTATE_FN);
    
    const IText bigLabel {24, COLOR_WHITE, "Roboto-Regular", IText::kAlignNear, IText::kVAlignTop, 0};
    const IText buttonLabels {14, COLOR_BLACK, "Roboto-Regular", IText::kAlignCenter, IText::kVAlignMiddle, 0};

    
    const int nRows = 5;
    const int nCols = 5;
    
    pGraphics->AttachControl(new ITextControl(b.GetGridCell(0, nRows, 1), "Bitmap Controls", bigLabel));
    pGraphics->AttachControl(new IBKnobControl(b.GetGridCell(0, nRows, nCols).GetPadded(-5.), bitmap1, kGain));
    pGraphics->AttachControl(new IBKnobRotaterControl(b.GetGridCell(1, nRows, nCols).GetPadded(-5.), bitmap2, kGain));
    pGraphics->AttachControl(new IBSwitchControl(b.GetGridCell(2, nRows, nCols), bitmap1));
    pGraphics->AttachControl(new IBButtonControl(b.GetGridCell(3, nRows, nCols), bitmap1));

    pGraphics->AttachControl(new ITextControl(b.GetGridCell(1, nRows, 1), "Vector Controls", bigLabel));
    pGraphics->AttachControl(new IVKnobControl(b.GetGridCell(5, nRows, nCols).GetCentredInside(100.), kGain));
    pGraphics->AttachControl(new IVSliderControl(b.GetGridCell(6, nRows, nCols).GetGridCell(0, 1, 3)));
    pGraphics->AttachControl(new IVSliderControl(b.GetGridCell(6, nRows, nCols).GetGridCell(3, 3, 2), kNoParameter, DEFAULT_SPEC, kHorizontal));
    pGraphics->AttachControl(new IVSwitchControl(b.GetGridCell(7, nRows, nCols).GetCentredInside(50.), kMode, [](IControl* pCaller)
    {
      SplashClickActionFunc(pCaller);
      dynamic_cast<IVectorBase*>(pCaller)->SetRoundness(pCaller->GetValue());
    }));
    
    //    pGraphics->AttachControl(new IVMeterControl<2>(*this, nextCell()), kControlTagMeter);
    //    pGraphics->AttachControl(new IVScopeControl<>(*this, nextCell()), kControlTagScope);
    pGraphics->AttachControl(new ISVGKnob(b.GetGridCell(8, nRows, nCols).GetCentredInside(100), vectorknob, kGain));
    
    auto button1action = [](IControl* pCaller) {
      SplashClickActionFunc(pCaller);
      int result = pCaller->GetUI()->ShowMessageBox("Message", "Title in Bold", kMB_YESNO);
      WDL_String str;
      str.SetFormatted(32, "%s pressed", kMessageResultStrs[result]);
      dynamic_cast<ITextControl*>(pCaller->GetUI()->GetControlWithTag(kCtrlTagDialogResult))->SetStr(str.Get());
    };
    
    auto button2action = [](IControl* pCaller) {
      SplashClickActionFunc(pCaller);
      WDL_String file, path;
      pCaller->GetUI()->PromptForFile(file, path);
      dynamic_cast<ITextControl*>(pCaller->GetUI()->GetControlWithTag(kCtrlTagDialogResult))->SetStr(file.Get());
    };
    
    auto button3action = [](IControl* pCaller) {
      SplashClickActionFunc(pCaller);
      WDL_String dir;
      pCaller->GetUI()->PromptForDirectory(dir);
      dynamic_cast<ITextControl*>(pCaller->GetUI()->GetControlWithTag(kCtrlTagDialogResult))->SetStr(dir.Get());
    };
    
    pGraphics->AttachControl(new IVButtonControl(b.GetGridCell(9, nRows, nCols).GetGridCell(0, 4, 1), button1action, "Trigger Message Box", buttonLabels));
    pGraphics->AttachControl(new IVButtonControl(b.GetGridCell(9, nRows, nCols).GetGridCell(1, 4, 1), button2action, "Trigger open file dialog", buttonLabels));
    pGraphics->AttachControl(new IVButtonControl(b.GetGridCell(9, nRows, nCols).GetGridCell(2, 4, 1), button3action, "Trigger open directory dialog", buttonLabels));
    pGraphics->AttachControl(new ITextControl(b.GetGridCell(9, nRows, nCols).GetGridCell(3, 4, 1), "Dialog result shown here...", DEFAULT_TEXT, COLOR_RED), kCtrlTagDialogResult);

    pGraphics->AttachControl(new ITextControl(b.GetGridCell(2, nRows, 1), "Text Controls", bigLabel));
    pGraphics->AttachControl(new ICaptionControl(b.GetGridCell(10, nRows, nCols).GetMidVPadded(20.), kGain, IText(50), false));

    pGraphics->AttachControl(new ITextControl(b.GetGridCell(3, nRows, 1), "Misc Controls", bigLabel));
    pGraphics->AttachControl(new FileBrowser(b.GetGridCell(15, nRows, nCols).Union(b.GetGridCell(16, nRows, nCols)).GetPadded(-25)));
    pGraphics->AttachControl(new IVKeyboardControl(b.GetGridCell(17, nRows, nCols).Union(b.GetGridCell(18, nRows, nCols)).GetPadded(-25), 36, 72));
//    pGraphics->AttachControl(new IColorPickerControl(b.GetGridCell(12, nRows, nCols).GetCentredInside(150.)));


  };
#endif
}

#if IPLUG_DSP
void IPlugControls::ProcessBlock(sample** inputs, sample** outputs, int nFrames)
{
  const double gain = GetParam(kGain)->Value() / 100.;
  const int nChans = NOutChansConnected();
  
  for (int s = 0; s < nFrames; s++) {
    for (int c = 0; c < nChans; c++) {
      outputs[c][s] = 0.;
    }
  }
}
#endif
