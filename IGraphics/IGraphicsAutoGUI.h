// #pragma once
//
// #include "IControl.h"
// #include "wdlstring.h"
//
// #define SLIDER_HANDLE_WIDTH 5
//
// struct AGTab
// {
//   IRECT mRECT;
//   WDL_TypedBuf<int> mParamsToMux;
//   WDL_String mLabel;
//
//   AGTab(IRECT bounds, const char* pLabel)
//   {
//     mRECT = bounds;
//     mLabel.Set(pLabel);
//   }
//
// };
//
// class AGPanelTabs : public IControl
// {
// private:
//   WDL_PtrList<AGTab> mTabs;
//   IColor mbgcolor, mfgcolor, mOnColor;
//   int mActive;
//
// public:
//
//   AGPanelTabs(IDelegate& dlg, IRECT tabsRect, IText& text, const IColor& bgcolor, const IColor& fgcolor, const IColor& onColor)
//   : IControl(dlg, tabsRect, kNoParameter)
//   , mbgcolor(bgcolor)
//   , mfgcolor(fgcolor)
//   , mOnColor(onColor)
//   , mActive(0)
//   {
//     mDblAsSingleClick = true;
//     mText = text;
//     mText.mAlign = IText::kAlignCenter;
//   }
//
//   ~AGPanelTabs()
//   {
//     mTabs.Empty(true);
//   }
//
//   void AddTab(AGTab* tab)
//   {
//     mTabs.Add(tab);
//   }
//
//   void OnMouseWheel(float x, float y, const IMouseMod& mod, float d) override {}
//
//   void OnMouseDown(float x, float y, const IMouseMod& mod) override
//   {
//     int i, n = mTabs.GetSize();
//     int hit = -1;
//
//     for (i = 0; i < n; ++i)
//     {
//       if (mTabs.Get(i)->mRECT.Contains(x, y))
//       {
//         hit = i;
//         mValue = (double) i / (double) (n - 1);
//
//         for (int t = 0; t < n; t++)
//         {
//           if (t == i)
//           {
//             for (int p = 0; p < mTabs.Get(t)->mParamsToMux.GetSize(); p++)
//             {
//               mDelegate.GetGUI()->HideControl(mTabs.Get(t)->mParamsToMux.Get()[p], false);
//             }
//           }
//           else
//           {
//             for (int p = 0; p < mTabs.Get(t)->mParamsToMux.GetSize(); p++)
//             {
//               mDelegate.GetGUI()->HideControl(mTabs.Get(t)->mParamsToMux.Get()[p], true);
//             }
//           }
//
//         }
//
//         break;
//       }
//     }
//
//     if (hit != -1)
//     {
//       mActive = hit;
//     }
//
//     SetDirty();
//   }
//
//   void Draw(IGraphics& g) override
//   {
//     for (int t = 0; t < mTabs.GetSize(); t++)
//     {
//       if (t == mActive) {
//         g.FillRect(mOnColor, mTabs.Get(t)->mRECT);
//       }
//       g.DrawRect(mfgcolor, mTabs.Get(t)->mRECT);
//       g.DrawText(mText, mTabs.Get(t)->mLabel.Get(), mTabs.Get(t)->mRECT);
//     }
//   }
// };
//
// class AGPresetSaveButtonControl : public IPanelControl
// {
// private:
//   const char** mParamNameStrings;
//
// public:
//   AGPresetSaveButtonControl(IDelegate& dlg, IRECT bounds, IText& text, const char** ppParamNameStrings)
//   : IPanelControl(dlg, bounds, COLOR_RED)
//   , mParamNameStrings(ppParamNameStrings)
//   {
//     mText = text;
//     mText.mAlign = IText::kAlignCenter;
//   }
//
//   void OnMouseDown(float x, float y, IMouseMod& mod)
//   {
//     WDL_String presetFilePath, desktopPath;
//
//     mDelegate.GetGUI()->DesktopPath(desktopPath);
//     mDelegate.GetGUI()->PromptForFile(presetFilePath, desktopPath, kFileSave, "txt");
//
//     if (strcmp(presetFilePath.Get(), "") != 0) {
//       mDelegate.DumpPresetSrcCode(presetFilePath.Get(), mParamNameStrings);
//     }
//   }
//
//   void Draw(IGraphics& g) override
//   {
//     g.FillRect(mColor, mRECT);
//     g.DrawText(mText, "Dump preset", mRECT);
//   }
// };
//
// #define WIDTH 48
// #define HEIGHT 50
// #define GAP 2
//
// void GenerateKnobGUI(IGraphics& g,
//                      IDelegate& dlg,
//                      IText& text,
//                      const IColor& bgcolor,
//                      const IColor& fgcolor,
//                      int minWidth,
//                      int minHeight)
// {
//   g.AttachPanelBackground(bgcolor);
//
//   const int w = g.Width();
//
//   // Calculate max bounds
//   WDL_String tmtext;
//   IRECT paramNameMaxBounds;
//   IRECT paramValueMaxBounds;
//
//   for(int p = 0; p < dlg.NParams(); p++)
//   {
//     IRECT thisParamNameMaxBounds;
//     tmtext.Set(dlg.GetParam(p)->GetNameForHost());
//     g.MeasureText(text, tmtext.Get(), thisParamNameMaxBounds);
//     paramNameMaxBounds = paramNameMaxBounds.Union(thisParamNameMaxBounds);
//
//     // hope that the display texts are longer than normal values for double params etc
//     // TODO: account for length of normal param values
//     for(int dt = 0; dt < dlg.GetParam(p)->GetNDisplayTexts(); dt++)
//     {
//       IRECT thisParamValueMaxBounds;
//       tmtext.Set(dlg.GetParam(p)->GetDisplayTextAtIdx(dt));
//       g.MeasureText(text, tmtext.Get(), thisParamValueMaxBounds);
//       paramValueMaxBounds = paramValueMaxBounds.Union(thisParamValueMaxBounds);
//     }
//   }
//
//   paramNameMaxBounds = paramNameMaxBounds.Union(paramValueMaxBounds);
//
//   int width = std::max(paramNameMaxBounds.W(), float(minWidth));
//
//   width = (width % 2 == 0) ? width : (width + 1); // make sure it's an even number, otherwise LICE draw errors
//
//   int height = std::max(paramNameMaxBounds.H(), float(minHeight));
//   int row = 0;
//   int column = 0;
//   int xoffs = 2;
//
//   for(int p = 0; p < dlg.NParams(); p++)
//   {
//     if ((((width + GAP) * column) + 2) + width >= w)
//     {
//       column = 0;
//       row++;
//       xoffs = 2;
//     }
//
//     xoffs = ((width + GAP) * column++) + 2;
//
//     int yoffs = ((height + GAP) * row) + 2;
//
//     IRECT paramRect = IRECT(xoffs, yoffs, xoffs+width, yoffs + height);
//
//     switch (dlg.GetParam(p)->Type())
//     {
//       case IParam::kTypeBool:
//         g.AttachControl(new AGKnobControl(dlg, paramRect, p, text, bgcolor, fgcolor, paramNameMaxBounds.H()));
//         break;
//       case IParam::kTypeInt:
//         g.AttachControl(new AGKnobControl(dlg, paramRect, p, text, bgcolor, fgcolor, paramNameMaxBounds.H()));
//         break;
//       case IParam::kTypeEnum:
//         g.AttachControl(new AGKnobControl(dlg, paramRect, p, text, bgcolor, fgcolor, paramNameMaxBounds.H()));
//         break;
//       case IParam::kTypeDouble:
//         g.AttachControl(new AGKnobControl(dlg, paramRect, p, text, bgcolor, fgcolor, paramNameMaxBounds.H()));
//         break;
//       default:
//         break;
//     }
//   }
// }
//
// void GenerateSliderGUI(IGraphics& g, IPlug& dlg, IText& text, const IColor& bgcolor, const IColor& fgcolor, int colWidth = 300, int tabs = 0, const char** pParamNameStrings = 0)
// {
//   g.AttachPanelBackground(bgcolor);
//
//   WDL_PtrList<const char> groupNames;
//   WDL_String thisGroup("");
//
//   // Calculate max bounds
//   WDL_String tmtext;
//   IRECT paramNameMaxBounds;
//   IRECT paramValueMaxBounds = IRECT(0, 0, 70, 10);  // the values here are a hack to make a minimum bounds
//
//   for(int p = 0; p < dlg.NParams(); p++)
//   {
//     IRECT thisParamNameMaxBounds;
//     tmtext.Set(dlg.GetParam(p)->GetNameForHost());
//     g.MeasureText(text, tmtext.Get(), thisParamNameMaxBounds);
//     paramNameMaxBounds = paramNameMaxBounds.Union(thisParamNameMaxBounds);
//
//     // hope that the display texts are longer than normal values for double params etc
//     // TODO: account for length of normal param values
//     for(int dt = 0; dt < dlg.GetParam(p)->GetNDisplayTexts(); dt++)
//     {
//       IRECT thisParamValueMaxBounds;
//       tmtext.Set(dlg.GetParam(p)->GetDisplayTextAtIdx(dt));
//       g.MeasureText(text, tmtext.Get(), thisParamValueMaxBounds);
//       paramValueMaxBounds = paramValueMaxBounds.Union(thisParamValueMaxBounds);
//     }
//
//     const char* label = dlg.GetParam(p)->GetParamGroupForHost();
//
//     if (strcmp(label, thisGroup.Get()) != 0)
//     {
//       groupNames.Add(label);
//       thisGroup.Set(label);
//     }
//   }
//
//   //printf("%i groups\n", groupNames.GetSize());
//
//   int yoffs = 2;
//   int row = 0;
//   int col = 0;
//
//   if (pParamNameStrings)
//   {
//     IRECT buttonsRect = IRECT(2, yoffs, colWidth-2, yoffs + paramNameMaxBounds.H());
//
//     g.AttachControl(new AGPresetSaveButtonControl(dlg, buttonsRect, text, pParamNameStrings));
//
//     yoffs += 20;
//   }
//
//   AGPanelTabs* pTabsControl = 0;
//   IRECT tabsRect = IRECT(2, yoffs, colWidth-2, yoffs + paramNameMaxBounds.H());
//
//   if (tabs)
//   {
//     pTabsControl = new AGPanelTabs(dlg, tabsRect, text, bgcolor, fgcolor, COLOR_RED);
//     g.AttachControl(pTabsControl);
//     yoffs += 20;
//   }
//
//   AGTab* pTab = 0;
//   thisGroup.Set("");
//   IRECT thisTabRect;
//   int groupIdx = 0;
//   char buf[32];
//
//   int paramStartYoffs = yoffs;
//
//   for(int p = 0; p < dlg.NParams(); p++)
//   {
//     if (tabs && groupNames.GetSize())
//     {
//       const char* label = dlg.GetParam(p)->GetParamGroupForHost();
//
//       if (strcmp(label, thisGroup.Get()) != 0)
//       {
//         thisTabRect = tabsRect.SubRectHorizontal(groupNames.GetSize(), groupIdx);
//         thisGroup.Set(label);
//         if (tabs == 1)
//         {
//           sprintf(buf, "%i", groupIdx+1);
//         }
//         else {
//           strcpy(buf, label);
//         }
//         pTab = new AGTab(thisTabRect, buf);
//         pTabsControl->AddTab(pTab);
//         groupIdx++;
//
//         col = 0;
//         yoffs = paramStartYoffs;
//       }
//
//       pTab->mParamsToMux.Add(p);
//     }
//
//     IRECT paramRect = IRECT(2 + (col * colWidth), yoffs, (col+1) * colWidth, yoffs + paramNameMaxBounds.H());
//
// //    switch (dlg.GetParam(p)->Type())
// //    {
// //      case IParam::kTypeBool:
// //        g.AttachControl(new AGHSliderControl(dlg, paramRect, p, text, bgcolor, fgcolor, paramNameMaxBounds.W(), paramValueMaxBounds.W()));
// //        break;
// //      case IParam::kTypeInt:
// //        g.AttachControl(new AGHSliderControl(dlg, paramRect, p, text, bgcolor, fgcolor, paramNameMaxBounds.W(), paramValueMaxBounds.W()));
// //        break;
// //      case IParam::kTypeEnum:
// //        g.AttachControl(new AGHSliderControl(dlg, paramRect, p, text, bgcolor, fgcolor, paramNameMaxBounds.W(), paramValueMaxBounds.W()));
// //        break;
// //      case IParam::kTypeDouble:
// //        g.AttachControl(new AGHSliderControl(dlg, paramRect, p, text, bgcolor, fgcolor, paramNameMaxBounds.W(), paramValueMaxBounds.W()));
// //        break;
// //      default:
// //        break;
// //    }
//
//     IControl* pControl = new AGHSliderControl(dlg, paramRect, p, text, bgcolor, fgcolor, paramNameMaxBounds.W(), paramValueMaxBounds.W());
//     g.AttachControl(pControl);
//
//     if (tabs && groupIdx != 1)
//     {
//       pControl->Hide(true);
//     }
//
//     if (yoffs + paramNameMaxBounds.H() >= g.Height() - 5)
//     {
//       col++;
//       yoffs = 2;
//       row = 0;
//     }
//     else
//     {
//       yoffs += paramNameMaxBounds.H();
//     }
//   }
// }
