/*
 ==============================================================================
 
 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.
 
 See LICENSE.txt for  more info.
 
 ==============================================================================
 */

#pragma once

#include "colormap/colormap.h"
#include "IGraphicsStructs.h"
#include <map>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class IColorMapList
{
  using ColorMapPtr = std::shared_ptr<colormap::Colormap const>;
public:
  IColorMapList(const std::initializer_list<std::string>& names, const std::string& initial)
  {
    using namespace colormap;

    if (names.size())
    {
      for (auto name : names)
      {
        for (ColorMapPtr const& c : ColormapList::getAll())
        {
          auto fullName = c->getCategory() + "_" + c->getTitle();

          if (fullName == name)
          {
            mSelectedColorMaps.insert(std::make_pair(name, c));
            break;
          }
        }
      }
    }
    else
    {
      for (ColorMapPtr const& c : ColormapList::getAll())
      {
        auto fullName = c->getCategory() + "_" + c->getTitle();
        mSelectedColorMaps.insert(std::make_pair(fullName, c));
      }
    }
    
    mCurrentColorMap = mSelectedColorMaps[initial];
  }

  IPopupMenu* CreatePopupMenu(bool withSubMenus = false)
  {
    auto* pColorMapMenu = new IPopupMenu("ColorMap");
    IPopupMenu* pToAddToMenu = pColorMapMenu;
    std::string lastCategory = "";
    int tagIdx = 0;
    
    for (auto& element : mSelectedColorMaps)
    {
      auto& c = element.second;
      
      if (withSubMenus && (c->getCategory() != lastCategory))
      {
        pToAddToMenu = new IPopupMenu();
        pColorMapMenu->AddItem(c->getCategory().c_str(), pToAddToMenu);
        lastCategory = c->getCategory();
      }
      auto fullName = c->getCategory() + "_" + c->getTitle();

      pToAddToMenu->AddItem(new IPopupMenu::Item(fullName.c_str(),
                                                 mCurrentColorMap == c ? IPopupMenu::Item::kChecked
                                                 : IPopupMenu::Item::kNoFlags, tagIdx++));
    }

    return pColorMapMenu;
  }

  IColor GetColor(float value) const
  {
    auto color = mCurrentColorMap->getColor(value);
    return IColor(255, color.r * 255, color.g * 255, color.b * 255);
  }

  void SetColorMap(const char* name)
  {
    mCurrentColorMap = mSelectedColorMaps[std::string(name)];
  }

  void AppendToShader(WDL_String& shader)
  {
    shader.Append(mCurrentColorMap->getSource().c_str());
  }
  
  std::vector<std::string> GetNames() const
  {
    std::vector<std::string> names;
    
    for (auto& element : mSelectedColorMaps)
    {
      auto& c = element.second;
      auto fullName = c->getCategory() + "_" + c->getTitle();
      names.push_back(fullName);
    }
    
    return names;
  }
  
private:
  ColorMapPtr mCurrentColorMap;
  std::map<std::string, ColorMapPtr> mSelectedColorMaps;
};

END_IPLUG_NAMESPACE
END_IGRAPHICS_NAMESPACE
