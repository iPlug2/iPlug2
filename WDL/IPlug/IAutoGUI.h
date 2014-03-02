#ifndef __IAUTOGUI__
#define __IAUTOGUI__

#include "IControl.h"
#include "wdlstring.h"

#define SLIDER_HANDLE_WIDTH 5

struct AGTab 
{
  IRECT mRECT;
  WDL_TypedBuf<int> mParamsToMux;
  WDL_String mLabel;
  
  AGTab(IRECT rect, const char* pLabel)
  {
    mRECT = rect;
    mLabel.Set(pLabel);
  }
  
};

class AGPanelTabs : public IControl
{
private:
  WDL_PtrList<AGTab> mTabs;
  IColor mBGColor, mFGColor, mOnColor;
  int mActive;
  
public:
  
  AGPanelTabs(IPlugBase *pPlug, IRECT tabsRect, IText *pText, const IColor *pBGColor, const IColor *pFGColor, const IColor *pOnColor)
  : IControl(pPlug, tabsRect, -1) 
  , mBGColor(*pBGColor)
  , mFGColor(*pFGColor)
  , mOnColor(*pOnColor)
  , mActive(0)
  {
    mDblAsSingleClick = true;
    mText = *pText;
    mText.mAlign = IText::kAlignCenter;
  }
  
  ~AGPanelTabs()
  {
    mTabs.Empty(true);
  }
  
  void AddTab(AGTab* tab)
  {
    mTabs.Add(tab);
  }
  
  void OnMouseWheel(int x, int y, IMouseMod* pMod) {}
  
  void OnMouseDown(int x, int y, IMouseMod* pMod) 
  {
    int i, n = mTabs.GetSize();
    int hit = -1;
    
    for (i = 0; i < n; ++i) 
    {
      if (mTabs.Get(i)->mRECT.Contains(x, y)) 
      {
        hit = i;
        mValue = (double) i / (double) (n - 1);
        
        for (int t = 0; t < n; t++) 
        {
          if (t == i) 
          {
            for (int p = 0; p < mTabs.Get(t)->mParamsToMux.GetSize(); p++) 
            {
              mPlug->GetGUI()->HideControl(mTabs.Get(t)->mParamsToMux.Get()[p], false);
            }
          }
          else 
          {
            for (int p = 0; p < mTabs.Get(t)->mParamsToMux.GetSize(); p++) 
            {
              mPlug->GetGUI()->HideControl(mTabs.Get(t)->mParamsToMux.Get()[p], true);
            }
          }
          
        }
        
        break;
      }
    }
    
    if (hit != -1) 
    {
      mActive = hit;
    }
    
    SetDirty();
  }
  
  bool Draw(IGraphics* pGraphics)
  {
    for (int t = 0; t < mTabs.GetSize(); t++) 
    {
      if (t == mActive) {
        pGraphics->FillIRect(&mOnColor, &mTabs.Get(t)->mRECT);
      }
      pGraphics->DrawRect(&mFGColor, &mTabs.Get(t)->mRECT);
      pGraphics->DrawIText(&mText, mTabs.Get(t)->mLabel.Get(), &mTabs.Get(t)->mRECT);
    }
    
    return true;
  }
};

class AGHSliderControl: public IControl
{
public:
  AGHSliderControl(IPlugBase *pPlug,
                   IRECT pR,
                   int paramIdx,
                   IText *pText,
                   const IColor *pBGColor, 
                   const IColor *pFGColor,
                   int paramNameWidth,
                   int paramValWidth)
  : IControl(pPlug, pR, paramIdx)
  , mBGColor(*pBGColor)
  , mFGColor(*pFGColor)
  {
    mText = *pText;
    mText.mAlign = IText::kAlignNear;
    mDisablePrompt = false;
    
    mParamNameRECT =  IRECT(mRECT.L, mRECT.T, mRECT.L + paramNameWidth, mRECT.B);
    mParamValueRECT = IRECT(mRECT.R - paramValWidth, mRECT.T, mRECT.R, mRECT.B);
    mSliderRECT =     IRECT(mParamNameRECT.R + 2, mRECT.T, mParamValueRECT.L - 2, mRECT.B);
    mTextEntryRect =  mParamValueRECT;//IRECT(mParamValueRECT.L+3, mParamValueRECT.T+3, mParamValueRECT.R, mParamValueRECT.B-3);
    mBlend = IChannelBlend::kBlendNone;
    
    mParamNameStr.Set(mPlug->GetParam(mParamIdx)->GetNameForHost());
  }
  
  bool Draw(IGraphics* pGraphics)
  {
    //pGraphics->RoundRect(&mFGColor, &mRECT, &mBlend, 2, true);

    pGraphics->DrawIText(&mText, mParamNameStr.Get(), &mParamNameRECT);
    
    // Draw Slider track
    pGraphics->DrawLine(&mFGColor, (float) mSliderRECT.L, (float) mSliderRECT.MH(), (float) mSliderRECT.R, (float) mSliderRECT.MH(), &mBlend, false);
    
    // Draw Slider handle
    int xPos = int(mValue * (mSliderRECT.W() - (SLIDER_HANDLE_WIDTH-1)));
  
    IRECT sliderHandleRect = IRECT(mSliderRECT.L + xPos, mRECT.T+4, mSliderRECT.L + xPos + SLIDER_HANDLE_WIDTH, mRECT.B-4);
    pGraphics->FillRoundRect(&mFGColor, &sliderHandleRect, &mBlend, 2, true);

    char cstr[32];    
    mPlug->GetParam(mParamIdx)->GetDisplayForHost(cstr);
    mParamValueStr.Set(cstr);
    mParamValueStr.Append(" ");
    mParamValueStr.Append(mPlug->GetParam(mParamIdx)->GetLabelForHost());
    pGraphics->DrawIText(&mText, mParamValueStr.Get(), &mParamValueRECT);

    return true;
  }
  
  void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    if (mParamValueRECT.Contains(x, y)) 
    {
      PromptUserInput(&mTextEntryRect);
    }
    else SnapToMouse(x, y);      
  }
  
  void OnMouseDrag(int x, int y, int dX, int dY, IMouseMod* pMod)
  {
    SnapToMouse(x, y);
  }
  
private:
  IColor mBGColor, mFGColor;
  int mHandleWidth;
  IRECT mSliderRECT;
  IRECT mParamValueRECT;  
  IRECT mParamNameRECT;
  IRECT mUnitRECT;
  IRECT mTextEntryRect;
  WDL_String mParamNameStr, mParamValueStr;
  
  void SnapToMouse(int x, int y)
  {
    if (mSliderRECT.Contains(x, mSliderRECT.T+3))
    {
      float xValue =  (float) (x-mSliderRECT.L -2) / (float) (mSliderRECT.W() - 4);
      mValue = BOUNDED(xValue, 0., 1.);
    }
    
    SetDirty(); 
  }
};

class AGKnobControl : public IKnobControl
{
public:
  AGKnobControl(IPlugBase* pPlug, 
                IRECT pR, 
                int paramIdx,
                IText *pText,
                const IColor *pBGColor, 
                const IColor *pFGColor,
                int textHeight)
  :   IKnobControl(pPlug, pR, paramIdx, kVertical, DEFAULT_GEARING)
  , mBGColor(*pBGColor)
  , mFGColor(*pFGColor)
  {
    mText = *pText;
    mText.mAlign = IText::kAlignCenter;

    mMinAngle = -0.75f * float(PI);
    mMaxAngle = 0.75f * float(PI);
    
    mDisablePrompt = false;
    
    mParamNameRECT =  IRECT(mRECT.L, mRECT.T, mRECT.R, mRECT.T + textHeight);
    mParamValueRECT = IRECT(mRECT.L, mRECT.B - textHeight, mRECT.R, mRECT.B);
    mKnobRECT =       IRECT(mRECT.L, mParamNameRECT.B, mRECT.R, mParamValueRECT.T);

    //mUnitRECT =       IRECT(mRECT.R - unitWidth, mRECT.T, mRECT.R, mRECT.B);
    mTextEntryRect =  IRECT(mParamValueRECT.L+2, mParamValueRECT.T+3, mParamValueRECT.R - 2, mParamValueRECT.B-3);
    
    mInnerRadius = 0.;
    mOuterRadius = 0.5f * (float) mKnobRECT.H();
    
    mBlend = IChannelBlend(IChannelBlend::kBlendNone);
    
    mParamNameStr.Set(mPlug->GetParam(mParamIdx)->GetNameForHost());
  }
  
  ~AGKnobControl() {}
  
  bool Draw(IGraphics* pGraphics)
  {
    pGraphics->RoundRect(&mFGColor, &mRECT, &mBlend, 2, true);

    // Draw Param Name
    pGraphics->DrawIText(&mText, mParamNameStr.Get(), &mParamNameRECT);

    // Draw Knob
    double v = mMinAngle + mValue * (mMaxAngle - mMinAngle);
    float sinV = (float) sin(v);
    float cosV = (float) cos(v);
    float cx = mKnobRECT.MW(), cy = mKnobRECT.MH();
    float x1 = cx + mInnerRadius * sinV, y1 = cy - mInnerRadius * cosV;
    float x2 = cx + (mOuterRadius) * sinV, y2 = cy - (mOuterRadius) * cosV;

    pGraphics->FillCircle(&mBGColor, (int) cx, (int) cy, mOuterRadius - 5, &mBlend, true);
    pGraphics->DrawArc(&mFGColor, cx, cy, mOuterRadius, mMinAngle, mMaxAngle, &mBlend, true);
    pGraphics->DrawArc(&mFGColor, cx, cy, mOuterRadius+1, mMinAngle, mMaxAngle, &mBlend, true);

    pGraphics->DrawLine(&mFGColor, x1, y1, x2, y2, &mBlend, true);
    
    if (fabs(x2-x1) > fabs(y2-y1))
    {
      ++y1;
      ++y2;
    }
    else
    {
      ++x1;
      ++x2;
    }
    
    // thicken line
    pGraphics->DrawLine(&mFGColor, x1, y1, x2, y2, &mBlend, true);
    
    char cstr[32];    
    mPlug->GetParam(mParamIdx)->GetDisplayForHost(cstr);
    mParamValueStr.Set(cstr);
    mParamValueStr.Append(" ");
    mParamValueStr.Append(mPlug->GetParam(mParamIdx)->GetLabelForHost());
    pGraphics->DrawIText(&mText, mParamValueStr.Get(), &mParamValueRECT);
    
    return true;
  }
  
  void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    if (mParamValueRECT.Contains(x, y)) 
    {
      PromptUserInput(&mTextEntryRect);
    }
  }
  
private:
  IColor mBGColor, mFGColor;
  float mMinAngle, mMaxAngle, mInnerRadius, mOuterRadius;
  
  IRECT mKnobRECT;
  IRECT mParamValueRECT;  
  IRECT mParamNameRECT;
  IRECT mUnitRECT;
  IRECT mTextEntryRect;
  
  WDL_String mParamNameStr, mParamValueStr;
};

class AGPresetSaveButtonControl : public IPanelControl
{
private:
  const char** mParamNameStrings;
  
public:
  AGPresetSaveButtonControl(IPlugBase *pPlug, IRECT pR, IText *pText, const char** pParamNameStrings)
  : IPanelControl(pPlug, pR, &COLOR_RED)
  , mParamNameStrings(pParamNameStrings)
  {
    mText = *pText;
    mText.mAlign = IText::kAlignCenter;
  }
  
  void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    WDL_String presetFilePath, desktopPath;
    
    mPlug->GetGUI()->DesktopPath(&desktopPath);
    mPlug->GetGUI()->PromptForFile(&presetFilePath, kFileSave, &desktopPath, "txt");
    
    if (strcmp(presetFilePath.Get(), "") != 0) {
      mPlug->DumpPresetSrcCode(presetFilePath.Get(), mParamNameStrings);
    }
  }
  
  bool Draw(IGraphics* pGraphics)
  {
    pGraphics->FillIRect(&mColor, &mRECT);
    pGraphics->DrawIText(&mText, "Dump preset", &mRECT);
    
    return true;
  }
};

#define WIDTH 48
#define HEIGHT 50
#define GAP 2

void GenerateKnobGUI(IGraphics* pGraphics, 
                     IPlug* pPlug,
                     IText *pText,
                     const IColor *pBGColor, 
                     const IColor *pFGColor,
                     int minWidth,
                     int minHeight)
{
  pGraphics->AttachPanelBackground(pBGColor);
  
  const int w = pGraphics->Width();
  
  // Calculate max bounds
  WDL_String tmpText;
  IRECT paramNameMaxBounds;
  IRECT paramValueMaxBounds;
  
  for(int p = 0; p < pPlug->NParams(); p++)
  {
    IRECT thisParamNameMaxBounds;
    tmpText.Set(pPlug->GetParam(p)->GetNameForHost());
    pGraphics->MeasureIText(pText, tmpText.Get(), &thisParamNameMaxBounds);
    paramNameMaxBounds = paramNameMaxBounds.Union(&thisParamNameMaxBounds);
    
    // hope that the display texts are longer than normal values for double params etc
    // TODO: account for length of normal param values
    for(int dt = 0; dt < pPlug->GetParam(p)->GetNDisplayTexts(); dt++)
    {
      IRECT thisParamValueMaxBounds;
      tmpText.Set(pPlug->GetParam(p)->GetDisplayTextAtIdx(dt));
      pGraphics->MeasureIText(pText, tmpText.Get(), &thisParamValueMaxBounds);
      paramValueMaxBounds = paramValueMaxBounds.Union(&thisParamValueMaxBounds);
    }
  }

  paramNameMaxBounds = paramNameMaxBounds.Union(&paramValueMaxBounds);
  
  int width = IPMAX(paramNameMaxBounds.W(), minWidth);
  
  width = (width % 2 == 0) ? width : (width + 1); // make sure it's an even number, otherwise LICE draw errors
  
  int height = IPMAX(paramNameMaxBounds.H(), minHeight);
  int row = 0;
  int column = 0;
  int xoffs = 2;
  
  for(int p = 0; p < pPlug->NParams(); p++)
  {
    if ((((width + GAP) * column) + 2) + width >= w) 
    {
      column = 0;
      row++;
      xoffs = 2;
    }

    xoffs = ((width + GAP) * column++) + 2;

    int yoffs = ((height + GAP) * row) + 2;
    
    IRECT paramRect = IRECT(xoffs, yoffs, xoffs+width, yoffs + height);
    
    switch (pPlug->GetParam(p)->Type()) 
    {
      case IParam::kTypeBool:
        pGraphics->AttachControl(new AGKnobControl(pPlug, paramRect, p, pText, pBGColor, pFGColor, paramNameMaxBounds.H()));
        break;
      case IParam::kTypeInt:
        pGraphics->AttachControl(new AGKnobControl(pPlug, paramRect, p, pText, pBGColor, pFGColor, paramNameMaxBounds.H()));
        break;
      case IParam::kTypeEnum:
        pGraphics->AttachControl(new AGKnobControl(pPlug, paramRect, p, pText, pBGColor, pFGColor, paramNameMaxBounds.H()));
        break;
      case IParam::kTypeDouble:
        pGraphics->AttachControl(new AGKnobControl(pPlug, paramRect, p, pText, pBGColor, pFGColor, paramNameMaxBounds.H()));
        break;
      default:
        break;
    }
  }
}

void GenerateSliderGUI(IGraphics* pGraphics, 
                 IPlug* pPlug,
                 IText *pText,
                 const IColor *pBGColor, 
                 const IColor *pFGColor,
                 int colWidth = 300,
                 int tabs = 0, // 0 = off, 1 = numbers, 2 = group name
                 const char** pParamNameStrings = 0) 
{
  pGraphics->AttachPanelBackground(pBGColor);

  WDL_PtrList<const char> groupNames;
  WDL_String thisGroup("");
  
  // Calculate max bounds
  WDL_String tmpText;
  IRECT paramNameMaxBounds;
  IRECT paramValueMaxBounds = IRECT(0, 0, 70, 10);  // the values here are a hack to make a minimum bounds 
  
  for(int p = 0; p < pPlug->NParams(); p++)
  {
    IRECT thisParamNameMaxBounds;
    tmpText.Set(pPlug->GetParam(p)->GetNameForHost());
    pGraphics->MeasureIText(pText, tmpText.Get(), &thisParamNameMaxBounds);
    paramNameMaxBounds = paramNameMaxBounds.Union(&thisParamNameMaxBounds);
    
    // hope that the display texts are longer than normal values for double params etc
    // TODO: account for length of normal param values
    for(int dt = 0; dt < pPlug->GetParam(p)->GetNDisplayTexts(); dt++)
    {
      IRECT thisParamValueMaxBounds;
      tmpText.Set(pPlug->GetParam(p)->GetDisplayTextAtIdx(dt));
      pGraphics->MeasureIText(pText, tmpText.Get(), &thisParamValueMaxBounds);
      paramValueMaxBounds = paramValueMaxBounds.Union(&thisParamValueMaxBounds);
    }
    
    const char* label = pPlug->GetParam(p)->GetParamGroupForHost();
    
    if (strcmp(label, thisGroup.Get()) != 0) 
    {
      groupNames.Add(label);
      thisGroup.Set(label);
    }
  }
  
  //printf("%i groups\n", groupNames.GetSize());
  
  int yoffs = 2;
  int row = 0;
  int col = 0;
  
  if (pParamNameStrings) 
  {
    IRECT buttonsRect = IRECT(2, yoffs, colWidth-2, yoffs + paramNameMaxBounds.H());
    
    pGraphics->AttachControl(new AGPresetSaveButtonControl(pPlug, buttonsRect, pText, pParamNameStrings));
    
    yoffs += 20;
  }

  AGPanelTabs* pTabsControl = 0;
  IRECT tabsRect = IRECT(2, yoffs, colWidth-2, yoffs + paramNameMaxBounds.H());
  
  if (tabs) 
  {
    pTabsControl = new AGPanelTabs(pPlug, tabsRect, pText, pBGColor, pFGColor, &COLOR_RED);
    pGraphics->AttachControl(pTabsControl);
    yoffs += 20;
  }
  
  AGTab* pTab = 0;
  thisGroup.Set("");
  IRECT thisTabRect;
  int groupIdx = 0;
  char buf[32];
  
  int paramStartYoffs = yoffs;
  
  for(int p = 0; p < pPlug->NParams(); p++)
  {
    if (tabs && groupNames.GetSize()) 
    {
      const char* label = pPlug->GetParam(p)->GetParamGroupForHost();

      if (strcmp(label, thisGroup.Get()) != 0) 
      {
        thisTabRect = tabsRect.SubRectHorizontal(groupNames.GetSize(), groupIdx);
        thisGroup.Set(label);
        if (tabs == 1)
        {
          sprintf(buf, "%i", groupIdx+1);
        }
        else {
          strcpy(buf, label);
        }
        pTab = new AGTab(thisTabRect, buf);
        pTabsControl->AddTab(pTab);
        groupIdx++;
        
        col = 0;
        yoffs = paramStartYoffs;
      }
      
      pTab->mParamsToMux.Add(p);
    }

    IRECT paramRect = IRECT(2 + (col * colWidth), yoffs, (col+1) * colWidth, yoffs + paramNameMaxBounds.H());
    
//    switch (pPlug->GetParam(p)->Type()) 
//    {
//      case IParam::kTypeBool:
//        pGraphics->AttachControl(new AGHSliderControl(pPlug, paramRect, p, pText, pBGColor, pFGColor, paramNameMaxBounds.W(), paramValueMaxBounds.W()));
//        break;
//      case IParam::kTypeInt:
//        pGraphics->AttachControl(new AGHSliderControl(pPlug, paramRect, p, pText, pBGColor, pFGColor, paramNameMaxBounds.W(), paramValueMaxBounds.W()));
//        break;
//      case IParam::kTypeEnum:
//        pGraphics->AttachControl(new AGHSliderControl(pPlug, paramRect, p, pText, pBGColor, pFGColor, paramNameMaxBounds.W(), paramValueMaxBounds.W()));
//        break;
//      case IParam::kTypeDouble:
//        pGraphics->AttachControl(new AGHSliderControl(pPlug, paramRect, p, pText, pBGColor, pFGColor, paramNameMaxBounds.W(), paramValueMaxBounds.W()));
//        break;
//      default:
//        break;
//    }
    
    IControl* pControl = new AGHSliderControl(pPlug, paramRect, p, pText, pBGColor, pFGColor, paramNameMaxBounds.W(), paramValueMaxBounds.W());
    pGraphics->AttachControl(pControl);
    
    if (tabs && groupIdx != 1) 
    {
      pControl->Hide(true);
    }
    
    if (yoffs + paramNameMaxBounds.H() >= pGraphics->Height() - 5) 
    {
      col++;
      yoffs = 2;
      row = 0;
    }
    else 
    {
      yoffs += paramNameMaxBounds.H();
    }
  }
}

#endif //__IAUTOGUI__