#pragma once

//#include "IPlugAAX.h"

using namespace iplug;
using namespace igraphics;

#ifdef WIN32
#define round(x) floor(x + 0.5)
#endif


//#define MAXSLIDERS 512

class MultiSliderControlV: public IControl
{
public:
  MultiSliderControlV(const IRECT& bounds,
                      int paramIdx,
                      int numSliders,
                      int handleWidth,
                      const IColor& bgcolor,
                      const IColor& fgcolor,
                      const IColor& hlcolor)
  : IControl(bounds, paramIdx)
  {
    mParamIdx = paramIdx;
    mBgColor = bgcolor;
    mFgColor = fgcolor;
    mHlColor = hlcolor;
    
    mNumSliders = numSliders;
    mHighlighted = -1;
    mGrain = 0.001;
    mSliderThatChanged = -1;
    
    float sliderWidth = floor((float) mRECT.W() / (float) mNumSliders);
    
    mSteps = new double[mNumSliders]();
    mSliderBounds = new IRECT*[mNumSliders];
    
    for(int i=0; i<mNumSliders; i++)
    {
      int lpos = (i * sliderWidth);
      mSteps[i] = 0.;
      
      mSliderBounds[i] = new IRECT(mRECT.L + lpos , mRECT.T, mRECT.L + lpos + sliderWidth, mRECT.B);
    }
    
    mHandleWidth = handleWidth;
  }
  
  ~MultiSliderControlV()
  {
    for (int i = 0; i < mNumSliders; i++)
    {
      if (mSliderBounds[i]) delete[] mSliderBounds[i];
    }
    if (mSliderBounds) delete[] mSliderBounds;

    if (mSteps) delete[] mSteps;
  }
  
  void Draw(IGraphics& g)
  {
    g.FillRect(mBgColor, mRECT);
    
    for(int i=0; i<mNumSliders; i++)
    {
      float yPos = mSteps[i] * mRECT.H();
      int top = mRECT.B - yPos;
      int bottom = mRECT.B;
      
      IColor * color = &mFgColor;
      if(i == mHighlighted) color = &mHlColor;
      
      IRECT srect = IRECT(mSliderBounds[i]->L, top, mSliderBounds[i]->R-1, bottom);
      g.FillRect(*color, srect);
    }
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod)
  {
    SnapToMouse(x, y);
  }
  
  /*void OnMouseUp(float x, float y, const IMouseMod& mod)
  {
    dynamic_cast<IPluginBase*>(GetDelegate())->ModifyCurrentPreset();
    dynamic_cast<IPlugAAX*>(GetDelegate())->DirtyPTCompareState();
  }*/
  
  void OnMouseDrag(float x, float y, float dX, float dY, const IMouseMod& mod)
  {
    SnapToMouse(x, y);
  }
  
  void SnapToMouse(float x, float y)
  {
    x = Clip(x, mRECT.L, mSliderBounds[mNumSliders-1]->R-1);
    y = Clip(y, mRECT.T, mRECT.B-1);
    
    float yValue =  (float) (y-mRECT.T) / (float) mRECT.H();
    
    yValue = round( yValue / mGrain ) * mGrain;
    yValue = Clip(yValue, 0.f, 1.f);
    
    int sliderTest = mNumSliders-1;
    bool foundIntersection = false;
    
    while (!foundIntersection)
    {
      foundIntersection = mSliderBounds[sliderTest]->Contains(x, y);
      
      if (!foundIntersection && sliderTest !=0 ) sliderTest--;
    }
    
    if (foundIntersection)
    {
      mSteps[sliderTest] = 1. - yValue;
      mSliderThatChanged = sliderTest;
      dynamic_cast<IPluginBase*>(GetDelegate())->OnParamChange(mParamIdx);
    }
    else
    {
      mSliderThatChanged = -1;
    }
    
    SetDirty();
  }
  
  void GetLatestChange(double* data)
  {
    data[mSliderThatChanged] = mSteps[mSliderThatChanged];
  }
  
  void GetState(double* data)
  {
    memcpy( data, mSteps, mNumSliders * sizeof(double));
  }
  
  void SetState(double* data)
  {
    memcpy(mSteps, data, mNumSliders * sizeof(double));
    
    SetDirty();
  }
  
  void SetHighlight(int i)
  {
    mHighlighted = i;
    
    SetDirty();
  }
  
private:
  IColor mBgColor, mFgColor, mHlColor;
  int mNumSliders;
  int mHandleWidth;
  int mSliderThatChanged;
  double *mSteps;
  double mGrain;
  IRECT** mSliderBounds;
  int mHighlighted;
  int mParamIdx;
};

/****************************************************************************************************************/

/*For making presets during development- Creates a Rectangle with a triangle on it that opens a menu
 to load or save presets and banks. Also, there are options to move forward and back through presets and
 to dump preset source code to a text file. pParameterNames is a pointer to an array holding the parameter names. */

class IPresetFileMenuDev : public IPanelControl
{
private:
  WDL_String mPreviousPath;
  IColor mRectColor;
  IColor mTriColor;
  const char** mParamNames;
  
public:
  IPresetFileMenuDev(const IRECT &bounds, const char** pParameterNames, const IColor &RectColor = COLOR_WHITE, const IColor &TriColor = COLOR_GRAY)
  : IPanelControl(bounds, COLOR_BLUE)
  , mParamNames(pParameterNames)
  , mRectColor(RectColor)
  , mTriColor(TriColor)
  {
    mIgnoreMouse = false;
  }
  
  ~IPresetFileMenuDev() {}
  
  void Draw(IGraphics& g) override
  {
    g.FillRect(mRectColor, mRECT);
    
    int ax = mRECT.R - 8;
    int ay = mRECT.T + 4;
    int bx = ax + 4;
    int by = ay;
    int cx = ax + 2;
    int cy = ay + 2;
    
    g.FillTriangle(mTriColor, ax, ay, bx, by, cx, cy);
    
  }

  void ResetFactoryBank()
  {
    int numpresets = dynamic_cast<IPluginBase*>(GetDelegate())->NPresets();
    for (int i = 0; i < numpresets; i++)
    {
      IPreset* pPreset = dynamic_cast<IPluginBase*>(GetDelegate())->GetPreset(i);
      pPreset->mChunk.Clear();
      pPreset->mInitialized =false;
    }
    dynamic_cast<IPluginBase*>(GetDelegate())->CreatePresets();
    dynamic_cast<IPluginBase*>(GetDelegate())->RestorePreset(0);
    dynamic_cast<IPluginBase*>(GetDelegate())->InformHostOfPresetChange();
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (mod.L || mod.R)
    {
      doPopupMenu();
    }
    SetDirty(false);
  }
  
  void doPopupMenu()
  {
    IPopupMenu menu;
    
    menu.AddItem("Previous preset");
    menu.AddItem("Next preset");
    menu.AddSeparator();
    menu.AddItem("Save Program...");
    menu.AddItem("Save Bank...");
    menu.AddSeparator();
    menu.AddItem("Load Program...");
    menu.AddItem("Load Bank...");
    menu.AddSeparator();
    menu.AddItem("Dump MakePreset");
    menu.AddItem("Dump MakePresetFromNamedParams");
    menu.AddItem("Dump PresetBlob");
    menu.AddItem("Dump AllPresetsBlob");
    menu.AddItem("Dump BankBlob");
    menu.AddSeparator();
    menu.AddItem("Reset to Factory Default");
    
    if(GetUI() != nullptr)
    {
      GetUI()->CreatePopupMenu(*this, menu, mRECT);
      int itemChosen = menu.GetChosenItemIdx();
      WDL_String fileName;
      // Delete commented code if Pull Request #438 gets merged into master branch, otherwise uncomment and fix calls
      //int numpresets = dynamic_cast<IPluginBase*>(GetDelegate())->NPresets();
      int currpreset = dynamic_cast<IPluginBase*>(GetDelegate())->GetCurrentPresetIdx();
      //int prevpreset = (currpreset == 0) ? numpresets - 1 : currpreset - 1;
      //int nextpreset = (currpreset == numpresets - 1) ? 0 : currpreset + 1;
      
      switch (itemChosen)
      {
        case 0://Previous preset
          //dynamic_cast<IPluginBase*>(GetDelegate())->RestorePreset(prevpreset);
          //dynamic_cast<IPluginBase*>(GetDelegate())->InformHostOfProgramChange();
          //dynamic_cast<IPluginBase*>(GetDelegate())->DirtyParametersFromUI();
          dynamic_cast<IPluginBase*>(GetDelegate())->IncrementPreset(false);
          break;
        case 1://Next preset
          //dynamic_cast<IPluginBase*>(GetDelegate())->RestorePreset(nextpreset);
          //dynamic_cast<IPluginBase*>(GetDelegate())->InformHostOfProgramChange();
          //dynamic_cast<IPluginBase*>(GetDelegate())->DirtyParametersFromUI();
          dynamic_cast<IPluginBase*>(GetDelegate())->IncrementPreset(true);
          break;
        case 3: //Save Program
          fileName.Set(dynamic_cast<IPluginBase*>(GetDelegate())->GetPresetName(currpreset));
          GetUI()->PromptForFile(fileName, mPreviousPath, EFileAction::Save, "fxp");
          dynamic_cast<IPluginBase*>(GetDelegate())->SavePresetAsFXP(fileName.Get());
          break;
        case 4: //Save Bank
          fileName.Set("FactoryBank");
          GetUI()->PromptForFile(fileName, mPreviousPath, EFileAction::Save, "fxb");
          dynamic_cast<IPluginBase*>(GetDelegate())->SaveBankAsFXB(fileName.Get());
          break;
        case 6: //Load Preset
          GetUI()->PromptForFile(fileName, mPreviousPath, EFileAction::Open, "fxp");
          dynamic_cast<IPluginBase*>(GetDelegate())->LoadPresetFromFXP(fileName.Get());
          break;
        case 7: // Load Bank
          GetUI()->PromptForFile(fileName, mPreviousPath, EFileAction::Open, "fxb");
          dynamic_cast<IPluginBase*>(GetDelegate())->LoadBankFromFXB(fileName.Get());
          break;
        case 9: //Dumps/adds MakePreset to file
          fileName.Set("presets");
          GetUI()->PromptForFile(fileName, mPreviousPath, EFileAction::Save, "txt");
          dynamic_cast<IPluginBase*>(GetDelegate())->DumpMakePresetSrc(fileName.Get());
          break;
        case 10: //Dumps/adds MakePresetFromNamedParams to file
          fileName.Set("presets");
          GetUI()->PromptForFile(fileName, mPreviousPath, EFileAction::Save, "txt");
          dynamic_cast<IPluginBase*>(GetDelegate())->DumpMakePresetFromNamedParamsSrc(fileName.Get(), mParamNames);
          break;
        case 11: //Dumps/adds MakePresetFromBlob to file
          fileName.Set("blobpresets");
          GetUI()->PromptForFile(fileName, mPreviousPath, EFileAction::Save, "txt");
          dynamic_cast<IPluginBase*>(GetDelegate())->DumpPresetBlob(fileName.Get());
          break;
        case 12: // Dumps all Blob presets to file
          fileName.Set("blobBank");
          GetUI()->PromptForFile(fileName, mPreviousPath, EFileAction::Save, "txt");
          dynamic_cast<IPluginBase*>(GetDelegate())->DumpAllPresetsBlob(fileName.Get());
          break;
        case 13: // Dumps all Blob presets to file
          fileName.Set("blobBank");
          GetUI()->PromptForFile(fileName, mPreviousPath, EFileAction::Save, "txt");
          dynamic_cast<IPluginBase*>(GetDelegate())->DumpBankBlob(fileName.Get());
          break;
          case 15: //Reset factory presets
          ResetFactoryBank();
          break;
        default:
          break;
      }
    }
  }
  
};

/****************************************************************************************************************/

/*Creates a menu with submenus for selecting presets. Supports up to 128 presets
 in up to 8 submenus depending on the number of presets. pSubmenuNames is a pointer to an array of SubMenu names.*/

class IPresetMenu : public IControl
{
private:
  IPopupMenu mMainMenu;
  IPopupMenu *pSubMenu1, *pSubMenu2, *pSubMenu3, *pSubMenu4, *pSubMenu5, *pSubMenu6, *pSubMenu7, *pSubMenu8;
  WDL_String mDisp;
  IColor mColor;
  const char** pSubmenuNames;
  int mNumItems, mNumItemsInLastSubMenu, mNumSubMenus, mCurrPresetIdx;
  
public:
  IPresetMenu(const IRECT &bounds, const IText &Text, const char** pSubmenuNames, const IColor &BgColor = DEFAULT_BGCOLOR)
  : IControl(bounds, kNoParameter)
  , mColor(BgColor)
  , pSubmenuNames(pSubmenuNames)
  , mNumItems(0)
  , mNumItemsInLastSubMenu(0)
  , mNumSubMenus(0)
  , mCurrPresetIdx(-1)
  {
    mTextEntryLength = MAX_PRESET_NAME_LEN - 3;
    mText = Text;
    pSubMenu1 = nullptr;
    pSubMenu2 = nullptr;
    pSubMenu3 = nullptr;
    pSubMenu4 = nullptr;
    pSubMenu5 = nullptr;
    pSubMenu6 = nullptr;
    pSubMenu7 = nullptr;
    pSubMenu8 = nullptr;
  }
  
  ~IPresetMenu() {}
  
  void OnInit() override
  {
    mNumItems = dynamic_cast<IPluginBase*>(GetDelegate())->NPresets();
    mNumItemsInLastSubMenu = mNumItems % 16;//Assuming no more than 128 presets in 8 banks (16 presets max per submenu).
    mNumSubMenus = (mNumItemsInLastSubMenu > 0) ? (mNumItems / 16) + 1 : mNumItems / 16;
    mCurrPresetIdx = dynamic_cast<IPluginBase*>(GetDelegate())->GetCurrentPresetIdx();
  }
  
  void OpenMenu()
  {
    //create submenus
    if (mNumSubMenus > 0) pSubMenu1 = new IPopupMenu;
    if (mNumSubMenus > 1) pSubMenu2 = new IPopupMenu;
    if (mNumSubMenus > 2) pSubMenu3 = new IPopupMenu;
    if (mNumSubMenus > 3) pSubMenu4 = new IPopupMenu;
    if (mNumSubMenus > 4) pSubMenu5 = new IPopupMenu;
    if (mNumSubMenus > 5) pSubMenu6 = new IPopupMenu;
    if (mNumSubMenus > 6) pSubMenu7 = new IPopupMenu;
    if (mNumSubMenus == 8) pSubMenu8 = new IPopupMenu;
    //Add presets to submenus
    for (int i = 0; i< mNumItems; i++)
    {
      const char* str = dynamic_cast<IPluginBase*>(GetDelegate())->GetPresetName(i);
      if (i < 16)
      {
        if (i == mCurrPresetIdx)
          pSubMenu1->AddItem(str, -1, IPopupMenu::Item::kChecked);
        else
          pSubMenu1->AddItem(str, -1, IPopupMenu::Item::kNoFlags);
      }
      if (i >= 16 && i < 32)
      {
        if (i == mCurrPresetIdx)
          pSubMenu2->AddItem(str, -1, IPopupMenu::Item::kChecked);
        else
          pSubMenu2->AddItem(str, -1, IPopupMenu::Item::kNoFlags);
      }
      if (i >= 32 && i < 48)
      {
        if (i == mCurrPresetIdx)
          pSubMenu3->AddItem(str, -1, IPopupMenu::Item::kChecked);
        else
          pSubMenu3->AddItem(str, -1, IPopupMenu::Item::kNoFlags);
      }
      if (i >= 48 && i < 64)
      {
        if (i == mCurrPresetIdx)
          pSubMenu4->AddItem(str, -1, IPopupMenu::Item::kChecked);
        else
          pSubMenu4->AddItem(str, -1, IPopupMenu::Item::kNoFlags);
      }
      if (i >= 64 && i < 80)
      {
        if (i == mCurrPresetIdx)
          pSubMenu5->AddItem(str, -1, IPopupMenu::Item::kChecked);
        else
          pSubMenu5->AddItem(str, -1, IPopupMenu::Item::kNoFlags);
      }
      if (i >= 80 && i < 96)
      {
        if (i == mCurrPresetIdx)
          pSubMenu6->AddItem(str, -1, IPopupMenu::Item::kChecked);
        else
          pSubMenu6->AddItem(str, -1, IPopupMenu::Item::kNoFlags);
      }
      if (i >= 96 && i < 112)
      {
        if (i == mCurrPresetIdx)
          pSubMenu7->AddItem(str, -1, IPopupMenu::Item::kChecked);
        else
          pSubMenu7->AddItem(str, -1, IPopupMenu::Item::kNoFlags);
      }
      if (i >= 112 && i < 128)
      {
        if (i == mCurrPresetIdx)
          pSubMenu8->AddItem(str, -1, IPopupMenu::Item::kChecked);
        else
          pSubMenu8->AddItem(str, -1, IPopupMenu::Item::kNoFlags);
      }
    }
    //Add submenus to main menu
    if (mNumSubMenus > 0) mMainMenu.AddItem(pSubmenuNames[0], pSubMenu1);
    if (mNumSubMenus > 1) mMainMenu.AddItem(pSubmenuNames[1], pSubMenu2);
    if (mNumSubMenus > 2) mMainMenu.AddItem(pSubmenuNames[2], pSubMenu3);
    if (mNumSubMenus > 3) mMainMenu.AddItem(pSubmenuNames[3], pSubMenu4);
    if (mNumSubMenus > 4) mMainMenu.AddItem(pSubmenuNames[4], pSubMenu5);
    if (mNumSubMenus > 5) mMainMenu.AddItem(pSubmenuNames[5], pSubMenu6);
    if (mNumSubMenus > 6) mMainMenu.AddItem(pSubmenuNames[6], pSubMenu7);
    if (mNumSubMenus == 8) mMainMenu.AddItem(pSubmenuNames[7], pSubMenu8);
    
    GetUI()->CreatePopupMenu(*this, mMainMenu, mRECT);
  }
  
  void Draw(IGraphics& g) override
  {
    mCurrPresetIdx = dynamic_cast<IPluginBase*>(GetDelegate())->GetCurrentPresetIdx();
    mDisp.SetFormatted(32, "%02d: %s", mCurrPresetIdx + 1, dynamic_cast<IPluginBase*>(GetDelegate())->GetPresetName(mCurrPresetIdx));
    g.FillRect(mColor, mRECT);
    g.DrawText(mText, mDisp.Get(), mRECT);
    SetDirty(false);
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if (mod.R)
    {
      const char* pname = dynamic_cast<IPluginBase*>(GetDelegate())->GetPresetName(mCurrPresetIdx);
      GetUI()->CreateTextEntry(*this, mText, mRECT, pname, kNoValIdx);
    }
    else
    {
      OpenMenu();
    }
    
    SetDirty(false);
  }
  
  void OnPopupMenuSelection(IPopupMenu* pSelectedMenu, int valIdx) override
  {
    int itemChosen = -1;
    int submenu1idx = -1;
    int submenu2idx = -1;
    int submenu3idx = -1;
    int submenu4idx = -1;
    int submenu5idx = -1;
    int submenu6idx = -1;
    int submenu7idx = -1;
    int submenu8idx = -1;
    
    if (pSubMenu1 != nullptr)
    {
      submenu1idx = pSubMenu1->GetChosenItemIdx();
      if (submenu1idx > -1) itemChosen = submenu1idx;
    }
    if (pSubMenu2 != nullptr)
    {
      submenu2idx = pSubMenu2->GetChosenItemIdx();
      if (submenu2idx > -1) itemChosen = submenu2idx + 16;
    }
    if (pSubMenu3 != nullptr)
    {
      submenu3idx = pSubMenu3->GetChosenItemIdx();
      if (submenu3idx > -1) itemChosen = submenu3idx + 32;
    }
    if (pSubMenu4 != nullptr)
    {
      submenu4idx = pSubMenu4->GetChosenItemIdx();
      if (submenu4idx > -1) itemChosen = submenu4idx + 48;
    }
    if (pSubMenu5 != nullptr)
    {
      submenu5idx = pSubMenu5->GetChosenItemIdx();
      if (submenu5idx > -1) itemChosen = submenu5idx + 64;
    }
    if (pSubMenu6 != nullptr)
    {
      submenu6idx = pSubMenu6->GetChosenItemIdx();
      if (submenu6idx > -1) itemChosen = submenu6idx + 80;
    }
    if (pSubMenu7 != nullptr)
    {
      submenu7idx = pSubMenu7->GetChosenItemIdx();
      if (submenu7idx > -1) itemChosen = submenu7idx + 96;
    }
    if (pSubMenu8 != nullptr)
    {
      submenu8idx = pSubMenu8->GetChosenItemIdx();
      if (submenu8idx > -1) itemChosen = submenu8idx + 112;
    }
    
    if (itemChosen > -1)
    {
      dynamic_cast<IPluginBase*>(GetDelegate())->RestorePreset(itemChosen);
      dynamic_cast<IPluginBase*>(GetDelegate())->InformHostOfPresetChange();
      dynamic_cast<IPluginBase*>(GetDelegate())->DirtyParametersFromUI();
    }
    SetDirty(false);
    mMainMenu.RemoveEmptySubmenus();
    mMainMenu.Clear();
  }
  
  void OnTextEntryCompletion(const char* txt, int valIdx) override
  {
    WDL_String safeName;
    safeName.Set(txt, MAX_PRESET_NAME_LEN);
    
    dynamic_cast<IPluginBase*>(GetDelegate())->ModifyCurrentPreset(safeName.Get());
    dynamic_cast<IPluginBase*>(GetDelegate())->InformHostOfPresetChange();
    dynamic_cast<IPluginBase*>(GetDelegate())->DirtyParametersFromUI();
    SetDirty(false);
  }
  
};

//***************************************************************************************

/** The IAboutControl displays information about your plugin or application such as name, version,
 * copyright info, plugin API trademark info, and text that you specify to describe it.
 * Optionally, you can add plugin API logo bitmaps that will be attached to the control according to
 * which plugin API you are currently building and you can add your own logo bitmap that can be
 * specified to function as a URL control. See functions for details.*/
class IAboutControl : public IControl
{
private:
  WDL_String mAPIString1, mAPIString2;
  IRECT mTargetArea, mMfrLogoRECT, mAPILogoRECT, mURLRECT;
  IBitmap mMfrLogoBitmap, mAPILogoBitmap;
  IText mLargeText, mSmallText, mSmallTextCentered;
  const IColor mBGColor;
  float mCornerRadius, mSmalltextsize, mLargetextsize, mBodyStart, mFooterStart;
  double mValue;
  const char** pBodyText;
  const char* pURL;
  int mNLines;
  bool mHasMfrLogo, mIsURL, mBlendIsReversed, mHasAPILogo, mIsPlugin;
  IBlend mBlend;
  
public:
  /** Creates an IAboutControl
   * @param x and y: x and y coordinates of the upper left corner of the control.
   * @param width and height: The width and height of the control.
   * @param cornerRadius: The rounding of the controls corners.
   * @param BGColor: The background color of the control.
   * @param targetArea: A rectangle that you specify to open the IAboutControl when clicked.
   * @param smallTextSize: The size of all text in the control except the plugin name.
   * @param largeTextSize: The text size of the plugin name.
   * @param TxtColor: The text color.
   * @param font: The font of the text within the control.
   * Note: To close the control, click anywhere in the IAboutControl except the manufacturer bitmap/rectangle
   * if it is designated as a URL in which case it will open the link that you specify. See SetHeader(), SetBody(), and SetFooter().
   * Usage example:
   * pAboutControl = new IAboutControl(0.f, 0.f, PLUG_WIDTH, PLUG_HEIGHT, 5.f, IColor(210, 0, 0, 0), IRECT(8, 50, 24, 66), 16.f, 24.f, COLOR_LIGHT_GRAY, "Roboto-Regular");
   * pGraphics->AttachControl(pAboutControl);
   * pAboutControl->SetHeader(yourlogo, PLUG_URL_STR, true);
   * pAboutControl->SetBody(pAboutTextBody, 6);
   * pAboutControl->SetFooter(vst2logo, vst3logo, aulogo, aaxlogo);*/
  IAboutControl(float x, float y, float width, float height, float cornerRadius, const IColor &BGColor, IRECT targetArea, float smallTextSize, float largeTextSize, const IColor &TxtColor, const char* font)
  : IControl(IRECT(x, y, x + width, y + height))
  ,mTargetArea(targetArea)
  ,mBGColor(BGColor)
  ,mCornerRadius(cornerRadius)
  ,mSmalltextsize(smallTextSize)
  ,mLargetextsize(largeTextSize)
  ,mBodyStart(0.f)
  ,mFooterStart(0.f)
  ,mValue(0.)
  ,pBodyText(nullptr)
  ,pURL(nullptr)
  ,mNLines(0)
  ,mHasMfrLogo(false)
  ,mIsURL(false)
  ,mBlendIsReversed(false)
  ,mHasAPILogo(false)
  ,mIsPlugin(false)
  {
    mLargeText = IText(mLargetextsize, TxtColor, font, EAlign::Center, EVAlign::Top);
    mSmallText = IText(mSmalltextsize, TxtColor, font, EAlign::Near, EVAlign::Top);
    mSmallTextCentered = IText(mSmalltextsize, TxtColor, font, EAlign::Center, EVAlign::Top);
  }
  
  /** Adds your logo, optionally specify it as a URL link, and adjust mouse over behavior ( HandleMouseOver() must be
   * set to true in your plugin). If SetHeader() is not called, only the plugin name and version will be displayed at the top
   * of the IAboutControl.
   * @param bitmap: Your logo.
   * @param URL: a URL that you specify.
   * @param mouseoverBlendIsReversed: If false, the overlapping background color(at blendweight) will cover your logo when the mouse is over it
   * and will be transparent when the mouse is off of it. The opposite if set to true.
   * @param mouseoverBlendWeight: The amount of the mouse over blend color. 1.0f is fully blended in; 0.05f is almost transparent.*/
  void SetHeader(const IBitmap& bitmap, const char* URL = nullptr, bool mouseoverBlendIsReversed = false, float mouseoverBlendWeight = 0.2f)
  {
    if(bitmap.IsValid())
    {
      mMfrLogoBitmap = bitmap;
      float x = mRECT.MW() - (mMfrLogoBitmap.W() * 0.5);
      mMfrLogoRECT = IRECT(x, 10.f, mMfrLogoBitmap);
      mHasMfrLogo = true;
    }
    if(URL != nullptr)
    {
      pURL = URL;
      mIsURL = true;
      mURLRECT = IRECT(mMfrLogoRECT);
      mBlendIsReversed = mouseoverBlendIsReversed;
      mBlend = IBlend(EBlend::Default, mouseoverBlendWeight);
    }
  }
  
  /** Adds text that you specify in a const char* [] to the control.
   * @param bodytext: Text that you want to add to the control.
   * @param nLines: The number of lines of text that you are adding.
   * Example:
   *const char* pAboutTextBody[] =
   *{
   * " This is my really cool Plugin.",
   * "It takes sonic energy and transforms it",
   * "into a semi-sentient, sound enhancing",
   * "being that resides in the frequency domain",
   * "for an almost non-existant period of time before",
   * "collapsing upon itself and releasing...."
   *};*/
  void SetBody(const char** bodytext, int nLines)
  {
    pBodyText = bodytext;
    mNLines = nLines;
  }
  
  /** Adds the logo image that is associated with the plugins API. if called, it must be done so after
   * the control is attached. If no bitmap is specified for the current build, only the PLUG_COPYRIGHT_STR and
   * plugAPI  trademark info strings will be displayed. If SetFooter() is not called at all, only the PLUG_COPYRIGHT_STR will be shown.
   * @param vst2logo, vst3logo, aulogo, and aaxlogo: Bitmap images loaded into your plugin from your resources/img folder.*/
  void SetFooter(const IBitmap& vst2logo = IBitmap(), const IBitmap& vst3logo = IBitmap(), const IBitmap& aulogo = IBitmap(), const IBitmap& aaxlogo = IBitmap())
  {
    EAPI api = dynamic_cast<IPlugAPIBase*>(GetDelegate())->GetAPI();
    
    if(api <= kAPIAAX)
    {
      if(api == kAPIVST2 || api == kAPIVST3)
      {
        mAPIString1.Set("VST Plugin Technology by");
        mAPIString2.Set("Steinberg Media Technologies GmbH");
        api == kAPIVST2 ? mAPILogoBitmap = vst2logo : mAPILogoBitmap = vst3logo;
      }
      
      if(api == kAPIAU || api == kAPIAUv3)
      {
        mAPIString1.Set("The Audio Units logo is a");
        mAPIString2.Set("trademark of Apple Computer, Inc");
        mAPILogoBitmap = aulogo;
      }
      
      if(api == kAPIAAX)
      {
        mAPIString1.Set("AAX is a registered trademark");
        mAPIString2.Set("of Avid Technology, Inc");
        mAPILogoBitmap = aaxlogo;
      }
      
      if(mAPILogoBitmap.IsValid())
      {
        mHasAPILogo = true;
        float x = mRECT.MW() - (mAPILogoBitmap.W() * 0.5f);
        float y = mRECT.B - ((mSmalltextsize * 2.5f) + mAPILogoBitmap.H());
        mAPILogoRECT = IRECT(x, y, mAPILogoBitmap);
        mFooterStart = y - (mSmalltextsize * 1.5f);
      }
      else
      {
        mHasAPILogo = false;
        mFooterStart = mRECT.B - (mSmalltextsize * 3.5f);
      }
      mIsPlugin = true;
    }
    
    else
    {
      mHasAPILogo = false;
      mIsPlugin = false;
      mFooterStart = mRECT.B - (mSmalltextsize * 1.25f);
    }
  }
  
  void DrawHeader(IGraphics& g)
  {
    float nextline = 10.f;
    if(mHasMfrLogo)
    {
      g.DrawBitmap(mMfrLogoBitmap, mMfrLogoRECT);
      if(mIsURL)
      {
        if(mMouseIsOver)
        {
          mBlendIsReversed ? GetUI()->FillRect(COLOR_TRANSPARENT, mURLRECT) : GetUI()->FillRect(mBGColor, mURLRECT, &mBlend);
        }
        else
        {
          mBlendIsReversed ? GetUI()->FillRect(mBGColor, mURLRECT, &mBlend) : GetUI()->FillRect(COLOR_TRANSPARENT, mURLRECT);
        }
      }
      nextline = mMfrLogoRECT.B + 10.f;
    }
    g.DrawText(mLargeText, PLUG_NAME, mRECT.MW(), nextline);
    nextline += mLargetextsize;
    g.DrawText(mSmallTextCentered, "Version " PLUG_VERSION_STR, mRECT.MW(), nextline);
    mBodyStart = nextline + mLargetextsize;
  }
  
  void DrawBody(IGraphics& g)
  {
    for (int s = 0; s < mNLines; s++)
    {
      g.DrawText(mSmallText, pBodyText[s], 10.f, mBodyStart + mSmalltextsize * s);
    }
  }
  
  void DrawFooter(IGraphics& g)
  {
    float nextline = 0.f;
    g.DrawText(mSmallTextCentered, PLUG_COPYRIGHT_STR, mRECT.MW(), mFooterStart);
    if(mHasAPILogo)
    {
      g.DrawBitmap(mAPILogoBitmap, mAPILogoRECT);
      nextline = mAPILogoRECT.B + (mSmalltextsize * 0.25f);
    }
    else
    {
      nextline = mFooterStart + (mSmalltextsize * 1.5f);
    }
    if(mIsPlugin)
    {
      g.DrawText(mSmallTextCentered, mAPIString1.Get(), mRECT.MW(), nextline);
      g.DrawText(mSmallTextCentered, mAPIString2.Get(), mRECT.MW(), nextline + mSmalltextsize);
    }
  }
  
  void Draw(IGraphics& g) override
  {
    if (mValue < 0.5)
    {
      mTargetRECT = mTargetArea;
      SetDirty(false);
    }
    else
    {
      mTargetRECT = mRECT;
      g.FillRoundRect(mBGColor, mRECT, mCornerRadius);
      DrawHeader(g);
      DrawBody(g);
      DrawFooter(g);
      SetDirty(false);
    }
  }
  
  void OnMouseDown(float x, float y, const IMouseMod& mod) override
  {
    if(mIsURL && mMfrLogoRECT.Contains(x, y))
    {
      GetUI()->OpenURL(pURL);
      GetUI()->ReleaseMouseCapture();
    }
    else
    {
      mValue += 1.;
      if(mValue > 1.)
        mValue = 0.;
      SetValue(mValue);
    }
  }
  
  void OnMouseOver(float x, float y, const IMouseMod& mod) override
  {
    if(mIsURL && mMfrLogoRECT.Contains(x, y))
    {
      GetUI()->SetMouseCursor(ECursor::HAND);
      IControl::OnMouseOver(x, y, mod);
    }
    else
    {
      GetUI()->SetMouseCursor();
      IControl::OnMouseOut();
    }
  }
};
