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

/** All attached controls should be inside LIVE_EDIT_INIT and LIVE_EDIT_END
 * All controls should be wrapped with LIVE_EDIT_CONTROL_START and LIVE_EDIT_CONTROL_END
 * LIVE_EDIT_RECT should be used for the IRECT, LIVE_EDIT_COLOR for an IColor
 *
 * All macros should be placed on the new line. */

#define LIVE_EDIT_INIT(p) pGraphics->SetLiveEditSourcePath(p);
#define LIVE_EDIT_END

#define LIVE_EDIT_CONTROL_START
#define LIVE_EDIT_CONTROL_END
#define LIVE_EDIT_RECT(L, T, R, B) IRECT(L, T, R, B)
#define LIVE_EDIT_COLOR(A, R, G, B) IColor(A, R, G, B)
#define LIVE_EDIT_LABEL(STR) STR
#define LIVE_EDIT_PARAM(IDX) IDX
#define LIVE_EDIT_PROPS_START )->SetProperties({
#define LIVE_EDIT_PROPS_END });

constexpr int kLineSzMax = 2048;

constexpr std::initializer_list<const char*> kControlList
{
  "IVLabelControl",
  "IVButtonControl",
  "IVSwitchControl",
  "IVToggleControl",
  "IVSlideSwitchControl",
  "IVTabSwitchControl",
  "IVRadioButtonControl",
  "IVKnobControl",
  "IVSliderControl",
  "IVRangeSliderControl",
  "IVXYPadControl",
  "IVPlotControl",
  "IVGroupControl",
  "IVPanelControl",
  "IVColorSwatchControl",
//  "ISVGKnobControl",
//  "ISVGButtonControl",
//  "ISVGSwitchControl",
//  "ISVGSliderControl",
//  "IBButtonControl",
//  "IBSwitchControl",
//  "IBKnobControl",
//  "IBKnobRotaterControl",
//  "IBSliderControl",
//  "IBTextControl",
//  "IPanelControl",
//  "ILambdaControl",
//  "IBitmapControl",
//  "ISVGControl",
  "ITextControl",
  "IURLControl",
  "ITextToggleControl",
  "ICaptionControl",
  "IPlaceHolderControl"
};

class IGraphicsLiveEditSourceEditor
{
public:
  IGraphicsLiveEditSourceEditor(const char* liveEditSourcePath)
  {
    mLiveEditSourcePath = liveEditSourcePath;
   
    ReadSourceFile();
  }

  void UpdateControlRectSource(int controlIndex, const IRECT& r)
  {
    int sourceControlIndexStart = FindSourceIndex(controlIndex, "LIVE_EDIT_CONTROL_START");
    int sourceControlIndexEnd = FindSourceIndex(controlIndex, "LIVE_EDIT_CONTROL_END");

    if (controlIndex == -1 || sourceControlIndexStart == -1 || sourceControlIndexEnd == -1) return;

    WDL_String rectSrc;
    rectSrc.SetFormatted(128, "LIVE_EDIT_RECT(%0.1f, %0.1f, %0.1f, %0.1f)", r.L, r.T, r.R, r.B);
    
    for (int i = sourceControlIndexStart + 1; i < sourceControlIndexEnd; i++)
    {
      ReplaceSourceText(mSourceFile[i], "LIVE_EDIT_RECT", ")", rectSrc.Get());
    }

    WriteSourceFile();
  }
  
  void UpdateControlColorSource(int controlIndex, const IColor& c)
  {
    int sourceControlIndexStart = FindSourceIndex(controlIndex, "LIVE_EDIT_CONTROL_START");
    int sourceControlIndexEnd = FindSourceIndex(controlIndex, "LIVE_EDIT_CONTROL_END");

    if (controlIndex == -1 || sourceControlIndexStart == -1 || sourceControlIndexEnd == -1) return;

    WDL_String colorSrc;
    colorSrc.SetFormatted(128, "LIVE_EDIT_COLOR(%i, %i, %i, %i)", c.A, c.R, c.G, c.B);
    
    for (int i = sourceControlIndexStart + 1; i < sourceControlIndexEnd; i++)
    {
      ReplaceSourceText(mSourceFile[i], "LIVE_EDIT_COLOR", ")", colorSrc.Get());
    }

    WriteSourceFile();
  }
  
  void UpdateControlParamIdx(int controlIndex, int paramIdx)
  {
    int sourceControlIndexStart = FindSourceIndex(controlIndex, "LIVE_EDIT_CONTROL_START");
    int sourceControlIndexEnd = FindSourceIndex(controlIndex, "LIVE_EDIT_CONTROL_END");

    if (controlIndex == -1 || sourceControlIndexStart == -1 || sourceControlIndexEnd == -1) return;

    WDL_String paramSrc;
    paramSrc.SetFormatted(128, "LIVE_EDIT_PARAM(%i)", paramIdx);
    
    for (int i = sourceControlIndexStart + 1; i < sourceControlIndexEnd; i++)
    {
      ReplaceSourceText(mSourceFile[i], "LIVE_EDIT_PARAM", ")", paramSrc.Get());
    }

    WriteSourceFile();
  }
  
  void UpdateControlLabel(int controlIndex, const char* label)
  {
    int sourceControlIndexStart = FindSourceIndex(controlIndex, "LIVE_EDIT_CONTROL_START");
    int sourceControlIndexEnd = FindSourceIndex(controlIndex, "LIVE_EDIT_CONTROL_END");

    if (controlIndex == -1 || sourceControlIndexStart == -1 || sourceControlIndexEnd == -1) return;

    WDL_String labelSrc;
    labelSrc.SetFormatted(128, "LIVE_EDIT_LABEL(\"%s\")", label);
    
    for (int i = sourceControlIndexStart + 1; i < sourceControlIndexEnd; i++)
    {
      ReplaceSourceText(mSourceFile[i], "LIVE_EDIT_LABEL", ")", labelSrc.Get());
    }

    WriteSourceFile();
  }
  
  void UpdateControlProperties(int controlIndex, IControl* pControl)
  {
    int sourceControlIndexStart = FindSourceIndex(controlIndex, "LIVE_EDIT_CONTROL_START");
    int sourceControlIndexEnd = FindSourceIndex(controlIndex, "LIVE_EDIT_CONTROL_END");

    if (controlIndex == -1 || sourceControlIndexStart == -1 || sourceControlIndexEnd == -1) return;
    
    auto findStartOfProps = [&](){
      for (int i = sourceControlIndexStart; i < sourceControlIndexEnd; i++) {
        size_t pos = mSourceFile[i].find("LIVE_EDIT_PROPS_START");
        if (pos != std::string::npos)
          return i;
      }
      return -1;
    };
    
    auto findEndOfProps = [&](){
      for (int i = sourceControlIndexEnd; i > sourceControlIndexStart; i--) {
        size_t pos = mSourceFile[i].find("LIVE_EDIT_PROPS_END");
        if (pos != std::string::npos)
          return i;
      }
      return -1;
    };
    
    auto startOfProps = findStartOfProps();
    auto endOfProps = findEndOfProps();

    if(startOfProps > -1 && startOfProps != endOfProps)
    {
      mSourceFile.erase(mSourceFile.begin() + startOfProps, mSourceFile.begin() + endOfProps + 1);
      
      std::vector<std::string> ctrlSrc;
      ctrlSrc.push_back("    LIVE_EDIT_PROPS_START");
      ctrlSrc.push_back("");
      WDL_String propertiesSrc;
      AddProperties(pControl, propertiesSrc);
      ctrlSrc.back().append(propertiesSrc.Get());
      ctrlSrc.back().append("    LIVE_EDIT_PROPS_END");

      mSourceFile.insert(mSourceFile.begin() + startOfProps, ctrlSrc.begin(), ctrlSrc.end());

      WriteSourceFile();
    }
  }
  
  void AddProperties(IControl* pControl, WDL_String& src)
  {
    IPropMap map = pControl->GetProperties();

    if(!map.empty()) {
      for (auto&& prop : map)
      {
        src.AppendFormatted(kLineSzMax, "    {\"%s\", ", prop.first.c_str());
        switch (prop.second.index())
        {
          case kColor:
          {
            IColor val = *pControl->GetProp<IColor>(prop.first);
            src.AppendFormatted(kLineSzMax, "IColor(%i,%i,%i,%i)", val.A, val.R, val.G, val.B);
            break;
          }
          case kBool:
          {
            bool val = *pControl->GetProp<bool>(prop.first);
            src.AppendFormatted(kLineSzMax, val ? "true" : "false");
            break;
          }
          case kFloat:
          {
            float val = *pControl->GetProp<float>(prop.first);
            src.AppendFormatted(kLineSzMax, "%0.2ff", val);
            break;
          }
          case kInt:
          {
            int val = *pControl->GetProp<int>(prop.first);
            src.AppendFormatted(kLineSzMax, "%i", val);
            break;
          }
          case kRect:
          {
            IRECT val = *pControl->GetProp<IRECT>(prop.first);
            src.AppendFormatted(kLineSzMax, "{%f,%f,%f,%f}", val.L, val.T, val.R, val.B);
            break;
          }
          case kColorSpec:
          {
            IVColorSpec val = *pControl->GetProp<IVColorSpec>(prop.first);
            src.AppendFormatted(kLineSzMax, "IVColorSpec{");
            for(int i=0; i <kNumVColors; i++)
            {
              auto& c = val.GetColor((EVColor) i);
              src.AppendFormatted(kLineSzMax, "{%i,%i,%i,%i}, ", c.A, c.R, c.G, c.B);
            }
            src.AppendFormatted(kLineSzMax, "}");
            break;
          }
          case kText:
          {
            IText val = *pControl->GetProp<IText>(prop.first);
            src.AppendFormatted(kLineSzMax, "IText(%0.2ff, IColor(%i,%i,%i,%i), \"%s\", ", val.mSize, val.mFGColor.A, val.mFGColor.R, val.mFGColor.G, val.mFGColor.B, val.mFont);
            switch (val.mAlign) {
              case EAlign::Near: src.Append("EAlign::Near, "); break;
              case EAlign::Center: src.Append("EAlign::Center, "); break;
              case EAlign::Far: src.Append("EAlign::Far, "); break;
              default:
                break;
            }
            switch (val.mVAlign) {
              case EVAlign::Top: src.Append("EVAlign::Top, "); break;
              case EVAlign::Middle: src.Append("EVAlign::Middle, "); break;
              case EVAlign::Bottom: src.Append("EVAlign::Bottom, "); break;
              default:
                break;
            }
            src.AppendFormatted(kLineSzMax, "%0.2ff, IColor(%i,%i,%i,%i), IColor(%i,%i,%i,%i)", val.mAngle, val.mTextEntryBGColor.A, val.mTextEntryBGColor.R, val.mTextEntryBGColor.G, val.mTextEntryBGColor.B, val.mTextEntryFGColor.A, val.mTextEntryFGColor.R, val.mTextEntryFGColor.G, val.mTextEntryFGColor.B);
            src.AppendFormatted(kLineSzMax, ")");
            break;
          }
          case kStr:
          {
            const char* val = *pControl->GetProp<const char*>(prop.first);
            src.AppendFormatted(kLineSzMax, "\"%s\"", val);
            break;
          }
          default:
          {
            break;
          }
        }
        src.AppendFormatted(kLineSzMax, "},\n");
      }
    }
  }

  void AddControlToSource(IControl* pControl)
  {
    int numberOfEnds = FindNumberOf("LIVE_EDIT_CONTROL_END");
    int appendToSourceIndex = 0;

    if (numberOfEnds == 0)
      appendToSourceIndex = FindSourceIndex(0, "LIVE_EDIT_INIT");
    else
      appendToSourceIndex = FindSourceIndex(numberOfEnds - 1, "LIVE_EDIT_CONTROL_END");
    
    std::vector<std::string> ctrlSrc;
    WDL_String ctorStr;
    CreateSrcBasedOnClassName(pControl, ctorStr);
    
    ctrlSrc.push_back("");
    ctrlSrc.push_back("    LIVE_EDIT_CONTROL_START");
    ctrlSrc.push_back("    pGraphics->AttachControl(new ");
    ctrlSrc.back().append(ctorStr.Get());
    
    if (pControl->GetProperties().empty())
    {
      ctrlSrc.back().append(");");
    }
    else
    {
      ctrlSrc.push_back("    LIVE_EDIT_PROPS_START");
      ctrlSrc.push_back("");
      WDL_String propertiesSrc;
      AddProperties(pControl, propertiesSrc);
      ctrlSrc.back().append(propertiesSrc.Get());
      ctrlSrc.back().append("    LIVE_EDIT_PROPS_END");
    }

    ctrlSrc.push_back("    LIVE_EDIT_CONTROL_END");

    mSourceFile.insert(mSourceFile.begin() + appendToSourceIndex + 1, ctrlSrc.begin(), ctrlSrc.end());

    WriteSourceFile();
  }

  void RemoveControlFromSource(int controlIndexToRemove)
  {
    int sourceControlIndexStart = FindSourceIndex(controlIndexToRemove, "LIVE_EDIT_CONTROL_START");
    int sourceControlIndexEnd = FindSourceIndex(controlIndexToRemove, "LIVE_EDIT_CONTROL_END");

    if (sourceControlIndexStart == -1 || sourceControlIndexEnd == -1) return;

    mSourceFile.erase(mSourceFile.begin() + sourceControlIndexStart - 1, mSourceFile.begin() + sourceControlIndexEnd + 1);

    WriteSourceFile();
  }
  
  void WriteCleanSourceFile()
  {
    std::string data;
    
    auto findAndReplaceInLine =[](std::string& line, const std::string& findStr, const std::string& replaceStr){
      size_t pos = std::string::npos;
      
      pos = line.find(findStr);
      if(pos != std::string::npos)
         line.replace(pos, findStr.length(), replaceStr);
    };

    for (auto i = 0; i < mSourceFile.size(); i++)
    {
      std::string line = mSourceFile[i];

      findAndReplaceInLine(line, "LIVE_EDIT_RECT", "IRECT");
      findAndReplaceInLine(line, "LIVE_EDIT_COLOR", "IColor");
      findAndReplaceInLine(line, "LIVE_EDIT_LABEL", "");
      findAndReplaceInLine(line, "LIVE_EDIT_PARAM", "int");
      findAndReplaceInLine(line, "LIVE_EDIT_PROPS_START", ")->SetProperties({");
      findAndReplaceInLine(line, "LIVE_EDIT_PROPS_END", "});");

      if(line.find("LIVE_EDIT") == std::string::npos) // remove remaining LIVE_EDIT lines
      {
        data.append(line);
        data.append("\n");
      }
    }
    
    printf("%s\n", data.c_str());
  }
  
private:
  void ReplaceSourceText(std::string& textToBeReplaced, std::string start, std::string end, std::string replaceWith)
  {
    size_t startPos = textToBeReplaced.find(start);
    size_t endPos = textToBeReplaced.find(end, startPos);

    if (startPos == std::string::npos || endPos == std::string::npos) return;

    textToBeReplaced.replace(startPos, endPos - startPos + 1, replaceWith);
  }

  int FindNumberOf(const char* stringToFind)
  {
    int foundNumber = 0;

    for (auto i = 0; i < mSourceFile.size(); i++)
    {
      size_t pos = mSourceFile[i].find(stringToFind);

      if (pos != std::string::npos)
        if (!IsLineCommented(i, pos))
          foundNumber++;
    }

    return foundNumber;
  }

  int FindSourceIndex(int index, const char* stringToFind)
  {
    int foundNumber = 0;

    for (auto i = 0; i < mSourceFile.size(); i++)
    {
      size_t pos = mSourceFile[i].find(stringToFind);

      if (pos != std::string::npos)
        if (!IsLineCommented(i, pos))
          foundNumber++;

      if (foundNumber - 1 == index)
        return (int) i;
    }

    return -1;
  }

  bool IsLineCommented(size_t lineIndex, size_t endCharIndex)
  {
    size_t commentPosition = mSourceFile[lineIndex].find("//");

    if (commentPosition != std::string::npos && commentPosition < endCharIndex)
      return true;

    return false;
  }

  void ReadSourceFile()
  {
    mSourceFile.resize(0);

    std::string line;
    std::ifstream file(mLiveEditSourcePath);

    if (file.is_open())
    {
      while (getline(file, line))
      {
        mSourceFile.push_back(line);
      }
      file.close();
    }
  }

  void WriteSourceFile()
  {
    std::string data;

    for (auto i = 0; i < mSourceFile.size(); i++)
    {
      data.append(mSourceFile[i]);
      data.append("\n");
    }

    std::ofstream file(mLiveEditSourcePath);

    if (file.is_open())
    {
      file << data.c_str();
      file.close();
    }
    
    ReadSourceFile();
  }
  
  void CreateSrcBasedOnClassName(IControl* pControl, WDL_String& str)
  {
    auto r = pControl->GetRECT();
    auto* vecBase = pControl->As<IVectorBase>();
    const char* label = vecBase ? vecBase->GetLabelStr() : "";
    int paramIdx = pControl->GetParamIdx();
    const char* cname = pControl->GetClassName();
    
    WDL_String rectStr;
    rectStr.SetFormatted(128, "LIVE_EDIT_RECT(%0.2f,%0.2f,%0.2f,%0.2f)", r.L, r.T, r.R, r.B);

    if     (strcmp(cname, "IVLabelControl")       == 0) str.SetFormatted(kLineSzMax, "IVLabelControl(%s, LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), label);
    else if(strcmp(cname, "IVButtonControl")      == 0) str.SetFormatted(kLineSzMax, "IVButtonControl(%s, SplashClickActionFunc, LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), label);
    else if(strcmp(cname, "IVSwitchControl")      == 0) str.SetFormatted(kLineSzMax, "IVSwitchControl(%s, SplashClickActionFunc, LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), label);
    else if(strcmp(cname, "IVToggleControl")      == 0) str.SetFormatted(kLineSzMax, "IVToggleControl(%s, LIVE_EDIT_PARAM(%i), LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), paramIdx, label);
    else if(strcmp(cname, "IVSlideSwitchControl") == 0) str.SetFormatted(kLineSzMax, "IVSlideSwitchControl(%s, LIVE_EDIT_PARAM(%i), LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), paramIdx, label);
    else if(strcmp(cname, "IVTabSwitchControl")   == 0) str.SetFormatted(kLineSzMax, "IVTabSwitchControl(%s, LIVE_EDIT_PARAM(%i), {\"One\", \"Two\", \"Three\")},LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), paramIdx, label);
    else if(strcmp(cname, "IVRadioButtonControl") == 0) str.SetFormatted(kLineSzMax, "IVRadioButtonControl(%s, LIVE_EDIT_PARAM(%i), {\"One\", \"Two\", \"Three\"})", rectStr.Get(), paramIdx);
    else if(strcmp(cname, "IVKnobControl")        == 0) str.SetFormatted(kLineSzMax, "IVKnobControl(%s, LIVE_EDIT_PARAM(%i), LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), paramIdx, label);
    else if(strcmp(cname, "IVSliderControl")      == 0) str.SetFormatted(kLineSzMax, "IVSliderControl(%s, LIVE_EDIT_PARAM(%i), LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), paramIdx, label);
    else if(strcmp(cname, "IVRangeSliderControl") == 0) str.SetFormatted(kLineSzMax, "IVRangeSliderControl(%s, {kNoParameter, kNoParameter}, LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), label);
    else if(strcmp(cname, "IVXYPadControl")       == 0) str.SetFormatted(kLineSzMax, "IVXYPadControl(%s, {kNoParameter, kNoParameter}, LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), label);
    else if(strcmp(cname, "IVPlotControl")        == 0) str.SetFormatted(kLineSzMax, "IVPlotControl(%s, LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), label);
    else if(strcmp(cname, "IVGroupControl")       == 0) str.SetFormatted(kLineSzMax, "IVGroupControl(%s, LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), label);
    else if(strcmp(cname, "IVPanelControl")       == 0) str.SetFormatted(kLineSzMax, "IVPanelControl(%s, LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), label);
    else if(strcmp(cname, "IVColorSwatchControl") == 0) str.SetFormatted(kLineSzMax, "IVColorSwatchControl(%s, LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), label);
    else if(strcmp(cname, "ISVGKnobControl")      == 0) str.SetFormatted(kLineSzMax, "ISVGKnobControl(%s, LIVE_EDIT_PARAM(%i), LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), paramIdx, label);
    else if(strcmp(cname, "ISVGButtonControl")    == 0) str.SetFormatted(kLineSzMax, "ISVGButtonControl(%s, LIVE_EDIT_PARAM(%i), LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), paramIdx, label);
    else if(strcmp(cname, "ISVGSwitchControl")    == 0) str.SetFormatted(kLineSzMax, "ISVGSwitchControl(%s, LIVE_EDIT_PARAM(%i), LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), paramIdx, label);
    else if(strcmp(cname, "ISVGSliderControl")    == 0) str.SetFormatted(kLineSzMax, "ISVGSliderControl(%s, LIVE_EDIT_PARAM(%i), LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), paramIdx, label);
    else if(strcmp(cname, "IBButtonControl")      == 0) str.SetFormatted(kLineSzMax, "IBButtonControl(%s, LIVE_EDIT_PARAM(%i), LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), paramIdx, label);
    else if(strcmp(cname, "IBSwitchControl")      == 0) str.SetFormatted(kLineSzMax, "IBSwitchControl(%s, LIVE_EDIT_PARAM(%i), LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), paramIdx, label);
    else if(strcmp(cname, "IBKnobControl")        == 0) str.SetFormatted(kLineSzMax, "IBKnobControl(%s, LIVE_EDIT_PARAM(%i), LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), paramIdx, label);
    else if(strcmp(cname, "IBKnobRotaterControl") == 0) str.SetFormatted(kLineSzMax, "IBKnobRotaterControl(%s, LIVE_EDIT_PARAM(%i), LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), paramIdx, label);
    else if(strcmp(cname, "IBSliderControl")      == 0) str.SetFormatted(kLineSzMax, "IBSliderControl(%s, LIVE_EDIT_PARAM(%i), LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), paramIdx, label);
    else if(strcmp(cname, "IBTextControl")        == 0) str.SetFormatted(kLineSzMax, "IBTextControl(%s, LIVE_EDIT_PARAM(%i), LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), paramIdx, label);
    else if(strcmp(cname, "IPanelControl")        == 0) str.SetFormatted(kLineSzMax, "IPanelControl(%s, LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), label);
    else if(strcmp(cname, "ILambdaControl")       == 0) str.SetFormatted(kLineSzMax, "ILambdaControl(%s, nullptr, LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), label);
    else if(strcmp(cname, "IBitmapControl")       == 0) str.SetFormatted(kLineSzMax, "IBitmapControl(%s, LIVE_EDIT_PARAM(%i), LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), paramIdx, label);
    else if(strcmp(cname, "ISVGControl")          == 0) str.SetFormatted(kLineSzMax, "ISVGControl(%s, LIVE_EDIT_PARAM(%i), LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), paramIdx, label);
    else if(strcmp(cname, "ITextControl")         == 0) str.SetFormatted(kLineSzMax, "ITextControl(%s, LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), label);
    else if(strcmp(cname, "IURLControl")          == 0) str.SetFormatted(kLineSzMax, "IURLControl(%s, \"URL\", \"https://iPlug2.github.io\",LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), label);
    else if(strcmp(cname, "ITextToggleControl")   == 0) str.SetFormatted(kLineSzMax, "ITextToggleControl(%s, LIVE_EDIT_PARAM(%i), \"OFF\", \"ON\",LIVE_EDIT_LABEL(\"%s\"))", rectStr.Get(), paramIdx, label);
    else if(strcmp(cname, "ICaptionControl")      == 0) str.SetFormatted(kLineSzMax, "ICaptionControl(%s, LIVE_EDIT_PARAM(%i))", rectStr.Get(), paramIdx);
    else                                                str.SetFormatted(kLineSzMax, "IPlaceHolderControl(%s)", rectStr.Get());
  }


  std::vector<std::string> mSourceFile;
  std::string mLiveEditSourcePath;
};

class IGraphicsLiveEdit : public IControl
{
public:
  IGraphicsLiveEdit(bool mouseOversEnabled, const char* liveEditSourcePath)
  : IControl(IRECT())
  , mSourceEditor(liveEditSourcePath)
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
    IGraphics* pGraphics = GetUI();
    int c = pGraphics->GetMouseControlIdx(x, y, true);
    
    if (c > 0)
    {
      IControl* pControl = pGraphics->GetControl(c);
      mMouseDownRECT = pControl->GetRECT();
      mMouseDownTargetRECT = pControl->GetTargetRECT();

      if(!mod.S)
        mSelectedControls.Empty();
      
      mSelectedControls.Add(pControl);

      if(mod.A)
      {
        IControl* pNewControl = nullptr;

        IVectorBase* pVectorBase = dynamic_cast<IVectorBase*>(pControl);
        
        pNewControl = CreateNewControlBasedOnClassName(pControl->GetClassName(), mMouseDownRECT, pVectorBase ? pVectorBase->GetLabelStr() : pControl->GetClassName(), pControl->GetParamIdx());
        
        if(pNewControl)
        {
          pNewControl->SetProperties(pControl->GetProperties());
        
          pGraphics->AttachControl(pNewControl);
          
          mSourceEditor.AddControlToSource(pNewControl);
        }
      
        mClickedOnControl = GetUI()->NControls() - 1;
        mMouseClickedOnResizeHandle = false;
      }
      else
      {
        mClickedOnControl = c;
        
        if(GetHandleRect(mMouseDownRECT).Contains(x, y))
        {
          mMouseClickedOnResizeHandle = true;
        }
        else if(mod.R)
        {
          mRightClickMenu.Clear();
          
          IPopupMenu* pStyleMenu = new IPopupMenu("Properties");
          
          for(auto& prop : pControl->GetProperties())
          {
            int flags = 0;
            
            IPopupMenu::Item* pItem = nullptr;
            
            switch (prop.second.index() ) {
              case kBool:
                if(*pControl->GetProp<bool>(prop.first))
                  flags |= IPopupMenu::Item::Flags::kChecked;
                pItem = new IPopupMenu::Item(prop.first.c_str(), flags);
                break;
              case kColorSpec:
              {
                WDL_String rootLabel;
                rootLabel.SetFormatted(32, "ColorSpec: %s", prop.first.c_str());
                IPopupMenu* pColorsMenu = new IPopupMenu(rootLabel.Get());
                for(int i=0; i<kNumVColors;i++) {
                  pColorsMenu->AddItem(kVColorStrs[i]);
                }
                pItem = new IPopupMenu::Item(prop.first.c_str(), pColorsMenu);
                break;
              }
              case kText:
              {
                IText currentVal = *pControl->GetProp<IText>(prop.first);
                WDL_String rootLabel;
                rootLabel.SetFormatted(32, "IText: %s", prop.first.c_str());
                IPopupMenu* pTextMenu = new IPopupMenu(rootLabel.Get());
                pTextMenu->AddItem("size");
                pTextMenu->AddItem("color");
                rootLabel.SetFormatted(32, "IText Align: %s", prop.first.c_str());
                IPopupMenu* pAlignMenu = new IPopupMenu(rootLabel.Get(), {kEAlignStrs[0], kEAlignStrs[1], kEAlignStrs[2]} );
                pTextMenu->AddItem("align", pAlignMenu);
                pAlignMenu->CheckItemAlone((int) currentVal.mAlign);
                rootLabel.SetFormatted(32, "IText VAlign: %s", prop.first.c_str());
                IPopupMenu* pVAlignMenu = new IPopupMenu(rootLabel.Get(), {kEVAlignStrs[0], kEVAlignStrs[1], kEVAlignStrs[2]} );
                pTextMenu->AddItem("valign", pVAlignMenu);
                pVAlignMenu->CheckItemAlone((int) currentVal.mVAlign);
                pTextMenu->AddItem("angle");
                pTextMenu->AddItem("text entry bg color");
                pTextMenu->AddItem("text entry fg color");
                pItem = new IPopupMenu::Item(prop.first.c_str(), pTextMenu);
                break;
              }
              case kFloat:
              {
                WDL_String rootLabel;
                rootLabel.SetFormatted(32, "Float: %s", prop.first.c_str());
                IPopupMenu* pFloatMenu = new IPopupMenu(rootLabel.Get(), {"1", "2","3","4","5","6","7","8","9","10","11","12","13","14","15","20","21","22","23","24"});
                pItem = new IPopupMenu::Item(prop.first.c_str(), pFloatMenu);
                break;
              }
              default:
                pItem = new IPopupMenu::Item(prop.first.c_str());
                break;
            }
              
            if(pItem)
              pStyleMenu->AddItem(pItem);
          }
          
          mRightClickMenu.AddItem("Properties", pStyleMenu);
          
          if (pControl->As<IVectorBase>()) // TODO: other text controls
            mRightClickMenu.AddItem("Edit Label");
          
          mRightClickMenu.AddSeparator();

          mRightClickMenu.AddItem("Add control", new IPopupMenu("Add control", kControlList));
          //          mRightClickMenu.AddItem("Replace with control");
          mRightClickMenu.AddItem("Delete Control");
          
          mRightClickMenu.AddSeparator();

          IPopupMenu* pParamMenu = new IPopupMenu("Params");
          IEditorDelegate* pDelegate = GetDelegate();
          
          pParamMenu->AddItem("kNoParameter (-1)");
          
          for(int i=0; i<pDelegate->NParams();i++)
          {
            pParamMenu->AddItem(pDelegate->GetParam(i)->GetName(), -1, pControl->GetParamIdx() == i ? IPopupMenu::Item::kChecked : 0);
          }
          
          mRightClickMenu.AddItem("Link to parameter", pParamMenu);
          
          GetUI()->CreatePopupMenu(*this, mRightClickMenu, x, y);
        }
      }
    }
    else if(mod.R) // right click on background
    {
      mClickedOnControl = 0;
      
      mRightClickMenu.Clear();
      
      IPopupMenu* pStyleMenu = new IPopupMenu("Properties");
      
      for(auto& prop : GetUI()->GetBackgroundControl()->GetProperties())
      {
        pStyleMenu->AddItem(prop.first.c_str());
      }
      
      mRightClickMenu.AddItem("Properties", pStyleMenu);
      mRightClickMenu.AddItem("Add control", new IPopupMenu("Add control", kControlList));
      mRightClickMenu.AddItem("Save clean source code ...");
      GetUI()->CreatePopupMenu(*this, mRightClickMenu, x, y);
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
    
    mMouseX = x;
    mMouseY = y;
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
          
        GetUI()->SetControlSize(mClickedOnControl, r.W(), r.H());
      }
      else
      {
        const float x1 = SnapToGrid(mMouseDownRECT.L + (x - mouseDownX));
        const float y1 = SnapToGrid(mMouseDownRECT.T + (y - mouseDownY));
          
        GetUI()->SetControlPosition(mClickedOnControl, x1, y1);
      }
      
      mSourceEditor.UpdateControlRectSource(GetUI()->GetControlIdx(pControl), pControl->GetRECT());
      
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
          mSourceEditor.RemoveControlFromSource(GetUI()->GetControlIdx(pControl));
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
    IGraphics* pGraphics = GetUI();
    
    if(pSelectedMenu)
    {
      IControl* pControl = pGraphics->GetControl(mClickedOnControl);

      if(pControl)
      {
        auto& props = pControl->GetProperties(); // TODO: does this copy?
        
        if(strcmp(pSelectedMenu->GetRootTitle(), "Add control") == 0)
        {
          float x, y;
          pGraphics->GetMouseDownPoint(x, y);
          IRECT b = IRECT(x, y, x + 100.f, y + 100.f);
          WDL_String label;
          const char* className = pSelectedMenu->GetChosenItem()->GetText();
          label.SetFormatted(128, "%s", className);
          IControl* pNewControl = CreateNewControlBasedOnClassName(className, b, label.Get());

          if(pNewControl)
          {
            pGraphics->AttachControl(pNewControl);
            mSourceEditor.AddControlToSource(pNewControl);
          }
          else
            pGraphics->ShowMessageBox("Not implemented yet!", "", EMsgBoxType::kMB_OK, nullptr);
        }
        else if(strcmp(pSelectedMenu->GetRootTitle(), "Replace control") == 0)
        {
          
        }
        else if(strcmp(pSelectedMenu->GetRootTitle(), "Params") == 0)
        {
          int paramIdx = pSelectedMenu->GetChosenItemIdx() - 1; // first element is kNoParameter
          pControl->SetParamIdx(paramIdx);
          mSourceEditor.UpdateControlParamIdx(pGraphics->GetControlIdx(pControl), paramIdx);
        }
        else if(strstr(pSelectedMenu->GetRootTitle(), "ColorSpec"))
        {
          WDL_String propName;
          propName.Set(pSelectedMenu->GetRootTitle());
          propName.DeleteSub(0, strlen("ColorSpec: "));
          
          auto currentSpec = *(pControl->GetProp<IVColorSpec>(propName.Get()));
          auto colorId = (EVColor) pSelectedMenu->GetChosenItemIdx();
          auto startColor = currentSpec.GetColor(colorId);
          auto colorName = kVColorStrs[colorId];
          
          pGraphics->PromptForColor(startColor, colorName,
                                  [this, pGraphics, pControl, colorName, currentSpec, colorId, propName](const IColor& color) {
                                    IVColorSpec newSpec;
                                    newSpec = currentSpec;
                                    newSpec.SetColor(colorId, color);
                                    pControl->SetProp(propName.Get(), newSpec, true);
                                    mSourceEditor.UpdateControlProperties(pGraphics->GetControlIdx(pControl), pControl);
                                  });
        }
        else if(strstr(pSelectedMenu->GetRootTitle(), "IText Align"))
        {
          WDL_String propName;
          propName.Set(pSelectedMenu->GetRootTitle());
          propName.DeleteSub(0, strlen("IText Align: "));
          auto text = *(pControl->GetProp<IText>(propName.Get()));
          text.mAlign = (EAlign) pSelectedMenu->GetChosenItemIdx();
          pControl->SetProp(propName.Get(), text, true);
        }
        else if(strstr(pSelectedMenu->GetRootTitle(), "IText VAlign"))
        {
          WDL_String propName;
          propName.Set(pSelectedMenu->GetRootTitle());
          propName.DeleteSub(0, strlen("IText VAlign: "));
          auto text = *(pControl->GetProp<IText>(propName.Get()));
          text.mVAlign = (EVAlign) pSelectedMenu->GetChosenItemIdx();
          pControl->SetProp(propName.Get(), text, true);
        }
        else if(strstr(pSelectedMenu->GetRootTitle(), "IText"))
        {
          WDL_String propName;
          propName.Set(pSelectedMenu->GetRootTitle());
          propName.DeleteSub(0, strlen("IText: "));
          auto currentText = *(pControl->GetProp<IText>(propName.Get()));
          
          switch (pSelectedMenu->GetChosenItemIdx())
          {
            case 1:
            {
              pGraphics->PromptForColor(currentText.mFGColor, "Text Color",
                                      [this, pGraphics, pControl, propName, currentText](const IColor& color) {
                                        IText newText = currentText;
                                        newText.mFGColor = color;
                                        pControl->SetProp(propName.Get(), newText, true);
                                        mSourceEditor.UpdateControlProperties(pGraphics->GetControlIdx(pControl), pControl);
                                      });
              break;
            }
            case 6:
            {
              pGraphics->PromptForColor(currentText.mTextEntryBGColor, "Text Entry BG Color",
                                      [this, pGraphics, pControl, propName, currentText](const IColor& color) {
                                        IText newText = currentText;
                                        newText.mTextEntryBGColor = color;
                                        pControl->SetProp(propName.Get(), newText, true);
                                        mSourceEditor.UpdateControlProperties(pGraphics->GetControlIdx(pControl), pControl);
                                      });
              break;
            }
            case 7:
            {
              pGraphics->PromptForColor(currentText.mTextEntryBGColor, "Text Entry FG Color",
                                      [this, pGraphics, pControl, propName, currentText](const IColor& color) { // AGGGGH!!!
                                        IText newText = currentText;
                                        newText.mTextEntryFGColor = color;
                                        pControl->SetProp(propName.Get(), newText, true);
                                        mSourceEditor.UpdateControlProperties(pGraphics->GetControlIdx(pControl), pControl);
                                      });

              break;
            }
            default:
              pGraphics->ShowMessageBox("Not Implemented!", "", kMB_OK);
              break;
          }
        }
        else if(strcmp(pSelectedMenu->GetRootTitle(), "Properties") == 0)
        {
          if(pSelectedMenu->GetChosenItemIdx() < props.size())
          {
            auto prop = *(props.find(pSelectedMenu->GetChosenItem()->GetText()));
            auto& propName = prop.first;
            
            switch (prop.second.index())
            {
              case kColor:
              {
                IColor startColor = *(pControl->GetProp<IColor>(propName));
                pGraphics->PromptForColor(startColor,
                                        propName.c_str(),
                                        [pControl, pGraphics, propName, this](const IColor& color) {
                                          pControl->SetProp(propName, color, true);
                                      mSourceEditor.UpdateControlProperties(pGraphics->GetControlIdx(pControl), pControl);
                                        });
                break;
              }
              case kBool:
              {
                bool checked = pSelectedMenu->GetChosenItem()->GetChecked();
                pControl->SetProp(propName, !checked, true);
                mSourceEditor.UpdateControlProperties(pGraphics->GetControlIdx(pControl), pControl);
                break;
              }
              default:
                break;
            }
          }
          
          mSourceEditor.UpdateControlProperties(pGraphics->GetControlIdx(pControl), pControl);
        }
        else
        {
          if(strcmp(pSelectedMenu->GetChosenItem()->GetText(), "Delete Control") == 0)
          {
            mSelectedControls.Empty();
            mSourceEditor.RemoveControlFromSource(pGraphics->GetControlIdx(pControl));
            pGraphics->RemoveControl(mClickedOnControl);
            mClickedOnControl = -1;
          }
          else if(strcmp(pSelectedMenu->GetChosenItem()->GetText(), "Edit Label") == 0)
          {
            pGraphics->CreateTextEntry(*this, mText, pControl->As<IVectorBase>()->GetLabelBounds());
            return; // don't invalidate mClickedOnControl
          }
          if(strcmp(pSelectedMenu->GetChosenItem()->GetText(), "Save clean source code ...") == 0)
          {
            mSourceEditor.WriteCleanSourceFile();
          }
        }
      }
    }
    
    mClickedOnControl = -1;
  }
  
  void OnTextEntryCompletion(const char *str, int valIdx) override
  {
    IControl* pControl = GetUI()->GetControl(mClickedOnControl);
    
    pControl->As<IVectorBase>()->SetLabelStr(str);
    
    mSourceEditor.UpdateControlLabel(GetUI()->GetControlIdx(pControl), str);
    
    mClickedOnControl = -1;
  }
  
  void Draw(IGraphics& g) override
  {
    IBlend b {EBlend::Default, 0.25f};
    g.DrawGrid(mGridColor, g.GetBounds(), mGridSize, mGridSize, &b);
    g.DrawText(mText, "LIVE EDIT MODE", g.GetBounds());
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
  
  void OnDrop(const char* str) override
  {
    IGraphics* pGraphics = GetUI();

    IBitmap bmp = pGraphics->LoadBitmap(str);
  
    IBitmapControl* pControl = new IBitmapControl(mMouseX, mMouseY, bmp);
    pGraphics->AttachControl(pControl);
    mSourceEditor.AddControlToSource(pControl);
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
  
  bool GetActive() const
  {
    return mClickedOnControl > 0;
  }

private:
  IControl* CreateNewControlBasedOnClassName(const char* cname, const IRECT& r, const char* label, int paramIdx = kNoParameter)
  {
    IControl* pNewControl = nullptr;
    if     (strcmp(cname, "IVLabelControl")       == 0) pNewControl = new IVLabelControl(r, label);
    else if(strcmp(cname, "IVButtonControl")      == 0) pNewControl = new IVButtonControl(r, SplashClickActionFunc, label);
    else if(strcmp(cname, "IVSwitchControl")      == 0) pNewControl = new IVSwitchControl(r, paramIdx, label);
    else if(strcmp(cname, "IVToggleControl")      == 0) pNewControl = new IVToggleControl(r, paramIdx, label);
    else if(strcmp(cname, "IVSlideSwitchControl") == 0) pNewControl = new IVSlideSwitchControl(r, paramIdx, label);
    else if(strcmp(cname, "IVTabSwitchControl")   == 0) pNewControl = new IVTabSwitchControl(r, paramIdx, {"One", "Two", "Three"}, label);
    else if(strcmp(cname, "IVRadioButtonControl") == 0) pNewControl = new IVRadioButtonControl(r, paramIdx, {"One", "Two", "Three"}, label);
    else if(strcmp(cname, "IVKnobControl")        == 0) pNewControl = new IVKnobControl(r, paramIdx, label);
    else if(strcmp(cname, "IVSliderControl")      == 0) pNewControl = new IVSliderControl(r, paramIdx, label);
    else if(strcmp(cname, "IVRangeSliderControl") == 0) pNewControl = new IVRangeSliderControl(r, {kNoParameter, kNoParameter}, label);
    else if(strcmp(cname, "IVXYPadControl")       == 0) pNewControl = new IVXYPadControl(r, {kNoParameter, kNoParameter}, label);
//    else if(strcmp(cname, "IVPlotControl")        == 0) pNewControl = new IVPlotControl(r);
    else if(strcmp(cname, "IVGroupControl")       == 0) pNewControl = new IVGroupControl(r);
    else if(strcmp(cname, "IVPanelControl")       == 0) pNewControl = new IVPanelControl(r);
    else if(strcmp(cname, "IVColorSwatchControl") == 0) pNewControl = new IVColorSwatchControl(r);
//    else if(strcmp(cname, "ISVGKnobControl")      == 0) pNewControl = new ISVGKnobControl(r);
//    else if(strcmp(cname, "ISVGButtonControl")    == 0) pNewControl = new ISVGButtonControl(r);
//    else if(strcmp(cname, "ISVGSwitchControl")    == 0) pNewControl = new ISVGSwitchControl(r);
//    else if(strcmp(cname, "ISVGSliderControl")    == 0) pNewControl = new ISVGSliderControl(r);
//    else if(strcmp(cname, "ISVGControl")          == 0) pNewControl = new ISVGControl(r);
//    else if(strcmp(cname, "IBButtonControl")      == 0) pNewControl = new IBButtonControl(r);
//    else if(strcmp(cname, "IBSwitchControl")      == 0) pNewControl = new IBSwitchControl(r);
//    else if(strcmp(cname, "IBKnobControl")        == 0) pNewControl = new IBKnobControl(r);
//    else if(strcmp(cname, "IBKnobRotaterControl") == 0) pNewControl = new IBKnobRotaterControl(r);
//    else if(strcmp(cname, "IBSliderControl")      == 0) pNewControl = new IBSliderControl(r);
//    else if(strcmp(cname, "IBTextControl")        == 0) pNewControl = new IBTextControl(r);
//    else if(strcmp(cname, "IBitmapControl")       == 0) pNewControl = new IBitmapControl(r);
    else if(strcmp(cname, "IPanelControl")        == 0) pNewControl = new IPanelControl(r);
    else if(strcmp(cname, "ILambdaControl")       == 0) pNewControl = new ILambdaControl(r, nullptr);
    else if(strcmp(cname, "ITextControl")         == 0) pNewControl = new ITextControl(r);
    else if(strcmp(cname, "IURLControl")          == 0) pNewControl = new IURLControl(r, label, "https://iplug2.github.io");
    else if(strcmp(cname, "ITextToggleControl")   == 0) pNewControl = new ITextToggleControl(r, paramIdx, "OFF", "ON");
    else if(strcmp(cname, "ICaptionControl")      == 0) pNewControl = new ICaptionControl(r, paramIdx);
    else if(strcmp(cname, "IPlaceHolderControl")  == 0) pNewControl = new IPlaceHolderControl(r);
    
    return pNewControl;
  }
  
  IPopupMenu mRightClickMenu {"On Control", {""}};

  bool mMouseOversEnabled;
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
  float mMouseX = 0.f;
  float mMouseY = 0.f;

  float mGridSize = 10;
  int mClickedOnControl = -1;

  IGraphicsLiveEditSourceEditor mSourceEditor;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

#endif // !NDEBUG
