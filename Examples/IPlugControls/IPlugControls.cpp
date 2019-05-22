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

IPlugControls::IPlugControls(IPlugInstanceInfo instanceInfo)
: IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  GetParam(kGain)->InitDouble("Gain", 100., 0., 100.0, 0.01, "%");
  GetParam(kMode)->InitEnum("Mode", 0, 4, "", IParam::kFlagsNone, "", "one", "two", "three", "four");
  GetParam(kX)->InitDouble("X", 50., 0., 100.0, 0.01, "%");
  GetParam(kY)->InitDouble("Y", 50., 0., 100.0, 0.01, "%");

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
    
    pGraphics->LoadFont("Roboto-Regular", ROBOTO_FN);
    const IBitmap bitmap1 = pGraphics->LoadBitmap(PNGKNOB_FN, 60);
    const IBitmap bitmap2 = pGraphics->LoadBitmap(PNGKNOBROTATE_FN);
    const ISVG vectorknob = pGraphics->LoadSVG(SVGKNOBROTATE_FN);
    
    const IText bigLabel {24, COLOR_WHITE, "Roboto-Regular", IText::kAlignNear, IText::kVAlignTop, 0};
    const IText buttonLabels {14, COLOR_BLACK, "Roboto-Regular", IText::kAlignCenter, IText::kVAlignMiddle, 0};

    
    const int nRows = 5;
    const int nCols = 8;
    
    int cellIdx = 0;
    
    auto nextCell = [&](){
      return b.GetGridCell(cellIdx++, nRows, nCols).GetPadded(-5.);
    };
    
    
//    ITextControl* pLabel;
//    pGraphics->AttachControl(pLabel = new ITextControl(b.GetGridCell(0, nRows, 1), "Bitmap Controls", bigLabel));
//    pLabel->SetBoundsBasedOnTextDimensions();
    pGraphics->AttachControl(new IBKnobControl(nextCell().GetPadded(-5.), bitmap1, kGain));
    pGraphics->AttachControl(new IBKnobRotaterControl(nextCell().GetPadded(-5.), bitmap2, kGain));
    pGraphics->AttachControl(new IBSwitchControl(nextCell(), bitmap1));
    pGraphics->AttachControl(new IBButtonControl(nextCell(), bitmap1));

//    pGraphics->AttachControl(pLabel = new ITextControl(b.GetGridCell(1, nRows, 1), "Vector Controls", bigLabel));
//    pLabel->SetBoundsBasedOnTextDimensions();

    const IVStyle style {
      true, // Show label
      true, // Show value
      {
        DEFAULT_BGCOLOR, // Background
        DEFAULT_FGCOLOR, // Foreground
        DEFAULT_PRCOLOR, // Pressed
        DEFAULT_FRCOLOR, // Frame
        DEFAULT_HLCOLOR, // Highlight
        DEFAULT_SHCOLOR, // Shadow
        DEFAULT_X1COLOR, // Extra 1
        DEFAULT_X2COLOR, // Extra 2
        DEFAULT_X3COLOR  // Extra 3
      }, // Colors
      IText(12.f, IText::kAlignCenter) // Label text
    };
    
    auto button1action = [](IControl* pCaller) {
      SplashClickActionFunc(pCaller);
      int result = pCaller->GetUI()->ShowMessageBox("Message Title", "Message", kMB_YESNO, [](EMsgBoxResult result)
                                                    {
                                                      
                                                    });
      WDL_String str;
      str.SetFormatted(32, "%s pressed", kMessageResultStrs[result]);
//      dynamic_cast<ITextControl*>(pCaller->GetUI()->GetControlWithTag(kCtrlTagDialogResult))->SetStr(str.Get());
    };
    
    pGraphics->AttachControl(new IVKnobControl(nextCell().GetCentredInside(110.), kGain, "IVKnobControl", style, true));
    pGraphics->AttachControl(new IVSliderControl(nextCell().GetCentredInside(110.), kGain, "IVSliderControl", style, true));
    pGraphics->AttachControl(new IVSliderControl(nextCell().GetCentredInside(110.), kGain, "IVSliderControl", style, true, kHorizontal));
    pGraphics->AttachControl(new IVRangeSliderControl(nextCell().GetCentredInside(110.), kX, kY, "IVRangeSliderControl", style, kVertical, 2.f, 10.f, 50.f));
    
    pGraphics->AttachControl(new IVButtonControl(nextCell().FracRectVertical(0.5, true), button1action, "IVButtonControl", style, false));
    pGraphics->AttachControl(new IVButtonControl(nextCell().FracRectVertical(0.5), button1action, "IVButtonControl", style, true));
    pGraphics->AttachControl(new IVSwitchControl(nextCell().GetCentredInside(110.), kMode, "IVSwitchControl", style));
    pGraphics->AttachControl(new IVRadioButtonControl(nextCell().GetCentredInside(110.), [](IControl* pCaller)
    {
//      dynamic_cast<IVRadioButtonControl*>(pCaller)->SetShape(static_cast<IVShape>(1.f/pCaller->GetValue()));
      SplashClickActionFunc(pCaller);
    }, {"Circle", "Rectangle", "Triangle"}, "IVRadioButtonControl", style, kVShapeCircle, 5.f));

    pGraphics->AttachControl(new IVXYPadControl(nextCell(), {kX, kY}));

//    pGraphics->AttachControl(new IVMeterControl<2>(nextCell()), 0);
//    pGraphics->AttachControl(new IVScopeControl<>(nextCell()), 0);
    
//    pGraphics->AttachControl(new ISVGKnob(b.GetGridCell(8, nRows, nCols).GetCentredInside(100), vectorknob, kGain));
    

//
//    auto button2action = [](IControl* pCaller) {
//      SplashClickActionFunc(pCaller);
//      WDL_String file, path;
//      pCaller->GetUI()->PromptForFile(file, path);
//      dynamic_cast<ITextControl*>(pCaller->GetUI()->GetControlWithTag(kCtrlTagDialogResult))->SetStr(file.Get());
//    };
//
//    auto button3action = [](IControl* pCaller) {
//      SplashClickActionFunc(pCaller);
//      WDL_String dir;
//      pCaller->GetUI()->PromptForDirectory(dir);
//      dynamic_cast<ITextControl*>(pCaller->GetUI()->GetControlWithTag(kCtrlTagDialogResult))->SetStr(dir.Get());
//    };
//
//    pGraphics->AttachControl(new IVButtonControl(b.GetGridCell(9, nRows, nCols).GetGridCell(1, 4, 1), button2action, "Trigger open file dialog"));
//    pGraphics->AttachControl(new IVButtonControl(b.GetGridCell(9, nRows, nCols).GetGridCell(2, 4, 1), button3action, "Trigger open directory dialog"));
//    pGraphics->AttachControl(new ITextControl(b.GetGridCell(9, nRows, nCols).GetGridCell(3, 4, 1), "Dialog result shown here...", DEFAULT_TEXT, COLOR_RED), kCtrlTagDialogResult);

//    pGraphics->AttachControl(pLabel = new ITextControl(b.GetGridCell(2, nRows, 1), "Text Controls", bigLabel));
//    pLabel->SetBoundsBasedOnTextDimensions();
//
//    pGraphics->AttachControl(new ICaptionControl(b.GetGridCell(10, nRows, nCols).GetMidVPadded(20.), kGain, IText(50), false));
//
//    pGraphics->AttachControl(pLabel = new ITextControl(b.GetGridCell(3, nRows, 1), "Misc Controls", bigLabel));
//    pLabel->SetBoundsBasedOnTextDimensions();
//
//    pGraphics->AttachControl(new FileBrowser(b.GetGridCell(15, nRows, nCols).Union(b.GetGridCell(16, nRows, nCols)).GetPadded(-25)));
//    pGraphics->AttachControl(new IVKeyboardControl(b.GetGridCell(17, nRows, nCols).Union(b.GetGridCell(18, nRows, nCols)).GetPadded(-25), 36, 72));
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
