/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#include "IGraphicsJSON.h"

#ifndef IGRAPHICS_NO_JSON

#include "IControls.h"
#include "IVTabbedPagesControl.h"
#include <fstream>
#include <sstream>
#include <cmath>
#include <sys/stat.h>

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

IGraphicsJSON::IGraphicsJSON(IGraphics* pGraphics, IGEditorDelegate* pDelegate)
: mGraphics(pGraphics)
, mDelegate(pDelegate)
{
  // Add default style
  mStyles["default"] = DEFAULT_STYLE;
}

bool IGraphicsJSON::Load(const char* jsonPath)
{
  mLoadedPath = jsonPath;

  std::ifstream file(jsonPath);
  if (!file.is_open())
  {
    DBGMSG("IGraphicsJSON: Failed to open file: %s\n", jsonPath);
    return false;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  return LoadFromString(buffer.str().c_str());
}

bool IGraphicsJSON::LoadFromString(const char* jsonStr)
{
  try
  {
    mRootSpec = json::parse(jsonStr);
    return ParseJSON(mRootSpec);
  }
  catch (const json::parse_error& e)
  {
    DBGMSG("IGraphicsJSON: Parse error: %s\n", e.what());
    return false;
  }
}

bool IGraphicsJSON::ParseJSON(const json& j)
{
  // Parse styles first
  if (j.contains("styles"))
  {
    ParseStyles(j["styles"]);
  }

  // Parse fonts
  if (j.contains("fonts"))
  {
    ParseFonts(j["fonts"]);
  }

  // Parse controls
  if (j.contains("controls"))
  {
    ParseControls(j["controls"], -1);
  }

  return true;
}

bool IGraphicsJSON::ParseStyles(const json& styles)
{
  for (auto& [name, styleDef] : styles.items())
  {
    mStyles[name] = ParseStyle(styleDef);
  }
  return true;
}

bool IGraphicsJSON::ParseFonts(const json& fonts)
{
  // Fonts are loaded by the plugin typically, but we can note them here
  // for future resource loading support
  return true;
}

void IGraphicsJSON::ParseControls(const json& controls, int parentIdx)
{
  for (const auto& def : controls)
  {
    IControl* pControl = CreateControl(def, parentIdx);

    if (pControl)
    {
      // Determine tag
      int tag = kNoTag;
      if (def.contains("id"))
      {
        std::string id = def["id"];
        tag = ResolveTag(id);
        mIdToControlIdx[id] = mGraphics->NControls();
      }

      // Determine group
      const char* group = "";
      if (def.contains("group"))
      {
        group = def["group"].get<std::string>().c_str();
      }

      mGraphics->AttachControl(pControl, tag, group);

      // Parse children if present
      if (def.contains("children"))
      {
        // Check if this control is a container (IContainerBase)
        if (auto* pContainer = pControl->As<IContainerBase>())
        {
          // Use proper container child creation
          CreateContainerChildren(pContainer, def["children"]);
        }
        else
        {
          // Fallback for non-container controls: use parent index for layout only
          int thisIdx = mGraphics->NControls() - 1;
          ParseControls(def["children"], thisIdx);
        }
      }
    }
  }
}

void IGraphicsJSON::CreateContainerChildren(IContainerBase* pContainer, const json& children)
{
  for (const auto& childDef : children)
  {
    IControl* pChild = CreateControl(childDef, -1);

    if (pChild)
    {
      // Determine tag
      int tag = kNoTag;
      if (childDef.contains("id"))
      {
        std::string id = childDef["id"];
        tag = ResolveTag(id);
      }

      // Determine group
      std::string groupStr;
      const char* group = "";
      if (childDef.contains("group"))
      {
        groupStr = childDef["group"].get<std::string>();
        group = groupStr.c_str();
      }

      // Add child to container using proper IContainerBase API
      pContainer->AddChildControl(pChild, tag, group);

      // Update ID map with actual control index
      if (childDef.contains("id"))
      {
        std::string id = childDef["id"];
        mIdToControlIdx[id] = mGraphics->NControls() - 1;
      }

      // Store layout for this child with container pointer
      LayoutDef layout;
      layout.containerPtr = pContainer;
      if (childDef.contains("bounds"))
      {
        layout.spec = childDef["bounds"];
        if (layout.spec.is_array())
          layout.type = LayoutDef::Type::Absolute;
        else
          layout.type = LayoutDef::Type::Relative;
      }
      mLayouts.push_back(layout);

      // Recurse for nested containers
      if (childDef.contains("children"))
      {
        if (auto* pChildContainer = pChild->As<IContainerBase>())
        {
          CreateContainerChildren(pChildContainer, childDef["children"]);
        }
        else
        {
          // Non-container with children: use parent index
          int thisIdx = mGraphics->NControls() - 1;
          ParseControls(childDef["children"], thisIdx);
        }
      }
    }
  }
}

IControl* IGraphicsJSON::CreateControl(const json& def, int parentIdx)
{
  std::string type = def.value("type", "");
  if (type.empty())
  {
    DBGMSG("IGraphicsJSON: Control missing 'type'\n");
    return nullptr;
  }

  // Initial bounds - will be recalculated in OnResize
  IRECT bounds(0, 0, 100, 100);

  // Store layout definition
  LayoutDef layout;
  layout.parentIdx = parentIdx;
  if (def.contains("bounds"))
  {
    layout.spec = def["bounds"];
    if (layout.spec.is_array())
      layout.type = LayoutDef::Type::Absolute;
    else
      layout.type = LayoutDef::Type::Relative;
  }
  mLayouts.push_back(layout);

  // Get param
  int paramIdx = kNoParameter;
  if (def.contains("param"))
  {
    paramIdx = ResolveParam(def["param"]);
  }

  // Get style
  IVStyle style = DEFAULT_STYLE;
  if (def.contains("style"))
  {
    if (def["style"].is_string())
      style = GetStyle(def["style"]);
    else
      style = ParseStyle(def["style"]);
  }

  // Apply style overrides on top of base style
  if (def.contains("styleOverrides"))
  {
    ApplyStyleOverrides(style, def["styleOverrides"]);
  }

  // Get label
  std::string label = def.value("label", "");

  // Create control based on type
  IControl* pControl = nullptr;

  //--- Vector Controls ---
  if (type == "IVKnobControl")
  {
    pControl = new IVKnobControl(bounds, paramIdx, label.c_str(), style);
  }
  else if (type == "IVSliderControl")
  {
    EDirection dir = EDirection::Vertical;
    if (def.contains("direction") && def["direction"] == "horizontal")
      dir = EDirection::Horizontal;
    pControl = new IVSliderControl(bounds, paramIdx, label.c_str(), style, false, dir);
  }
  else if (type == "IVButtonControl")
  {
    IActionFunction actionFunc = SplashClickActionFunc;
    if (def.contains("action"))
      actionFunc = GetAction(def["action"]);
    pControl = new IVButtonControl(bounds, actionFunc, label.c_str(), style);
  }
  else if (type == "IVToggleControl")
  {
    std::string offText = def.value("offText", "OFF");
    std::string onText = def.value("onText", "ON");
    pControl = new IVToggleControl(bounds, paramIdx, label.c_str(), style, offText.c_str(), onText.c_str());
  }
  else if (type == "IVSwitchControl")
  {
    int numStates = def.value("numStates", 2);
    pControl = new IVSwitchControl(bounds, paramIdx, label.c_str(), style, numStates);
  }
  else if (type == "IVTabSwitchControl")
  {
    std::vector<const char*> labels;
    std::vector<std::string> labelStorage;
    if (def.contains("labels"))
    {
      for (const auto& l : def["labels"])
      {
        labelStorage.push_back(l.get<std::string>());
      }
      for (const auto& s : labelStorage)
      {
        labels.push_back(s.c_str());
      }
    }
    EDirection dir = EDirection::Horizontal;
    if (def.contains("direction") && def["direction"] == "vertical")
      dir = EDirection::Vertical;
    pControl = new IVTabSwitchControl(bounds, paramIdx, labels, label.c_str(), style, EVShape::Rectangle, dir);
  }
  else if (type == "IVGroupControl")
  {
    float labelOffset = def.value("labelOffset", 10.f);
    pControl = new IVGroupControl(bounds, label.c_str(), labelOffset, style);
  }
  else if (type == "IVPanelControl")
  {
    pControl = new IVPanelControl(bounds, label.c_str(), style);
  }
  else if (type == "IVTabbedPagesControl")
  {
    float tabBarHeight = def.value("tabBarHeight", 20.f);
    float tabBarFrac = def.value("tabBarFrac", 0.5f);
    EAlign tabsAlign = EAlign::Near;
    if (def.contains("tabsAlign"))
    {
      std::string align = def["tabsAlign"];
      if (align == "far") tabsAlign = EAlign::Far;
      else if (align == "center") tabsAlign = EAlign::Center;
    }

    PageMap pages;
    if (def.contains("pages"))
    {
      for (auto& [pageName, pageDef] : def["pages"].items())
      {
        double padding = pageDef.value("padding", IVTabPage::kDefaultPadding);
        json childrenDef = pageDef.value("children", json::array());

        // Store page name for lifetime
        mPageNameStorage.push_back(pageName);
        const char* storedName = mPageNameStorage.back().c_str();

        // Create IVTabPage with attach function that creates children
        auto* pPage = new IVTabPage(
          [this, childrenDef](IVTabPage* pParent, const IRECT& bounds) {
            CreateContainerChildren(pParent, childrenDef);
          },
          IVTabPage::DefaultResizeFunc,
          style,
          padding
        );

        pages[storedName] = pPage;
      }
    }

    pControl = new IVTabbedPagesControl(bounds, pages, label.c_str(), style,
                                         tabBarHeight, tabBarFrac, tabsAlign);
  }
  else if (type == "IVLabelControl")
  {
    pControl = new IVLabelControl(bounds, label.c_str(), style);
  }
  //--- Basic Controls ---
  else if (type == "ITextControl")
  {
    std::string text = def.value("text", "");
    IText textStyle = DEFAULT_TEXT;

    if (def.contains("fontSize"))
      textStyle.mSize = def["fontSize"].get<float>();

    if (def.contains("align"))
    {
      std::string align = def["align"];
      if (align == "left") textStyle.mAlign = EAlign::Near;
      else if (align == "center") textStyle.mAlign = EAlign::Center;
      else if (align == "right") textStyle.mAlign = EAlign::Far;
    }

    if (def.contains("color"))
      textStyle.mFGColor = ParseColor(def["color"]);

    IColor bgColor = COLOR_TRANSPARENT;
    if (def.contains("bgColor"))
      bgColor = ParseColor(def["bgColor"]);

    pControl = new ITextControl(bounds, text.c_str(), textStyle, bgColor);
  }
  else if (type == "IPanelControl")
  {
    IColor color = COLOR_GRAY;
    if (def.contains("color"))
      color = ParseColor(def["color"]);
    bool drawFrame = def.value("drawFrame", false);
    pControl = new IPanelControl(bounds, color, drawFrame);
  }
  //--- Bitmap Controls ---
  else if (type == "IBKnobControl")
  {
    std::string bitmapName = def.value("bitmap", "");
    IBitmap bitmap = GetBitmap(bitmapName);
    if (bitmap.IsValid())
    {
      EDirection dir = EDirection::Vertical;
      if (def.contains("direction") && def["direction"] == "horizontal")
        dir = EDirection::Horizontal;
      pControl = new IBKnobControl(bounds, bitmap, paramIdx, dir);
    }
  }
  else if (type == "IBSliderControl")
  {
    std::string trackBmp = def.value("trackBitmap", "");
    std::string handleBmp = def.value("handleBitmap", "");
    EDirection dir = EDirection::Vertical;
    if (def.contains("direction") && def["direction"] == "horizontal")
      dir = EDirection::Horizontal;

    IBitmap track = GetBitmap(trackBmp);
    IBitmap handle = GetBitmap(handleBmp);

    if (track.IsValid())
    {
      pControl = new IBSliderControl(bounds, track, handle, paramIdx, dir);
    }
  }
  else if (type == "IBButtonControl")
  {
    std::string bitmapName = def.value("bitmap", "");
    IBitmap bitmap = GetBitmap(bitmapName);
    if (bitmap.IsValid())
    {
      IActionFunction actionFunc = SplashClickActionFunc;
      if (def.contains("action"))
        actionFunc = GetAction(def["action"]);
      pControl = new IBButtonControl(bounds, bitmap, actionFunc);
    }
  }
  else if (type == "IBSwitchControl")
  {
    std::string bitmapName = def.value("bitmap", "");
    IBitmap bitmap = GetBitmap(bitmapName);
    if (bitmap.IsValid())
    {
      pControl = new IBSwitchControl(bounds, bitmap, paramIdx);
    }
  }
  //--- SVG Controls ---
  else if (type == "ISVGKnobControl")
  {
    std::string svgName = def.value("svg", "");
    ISVG svg = GetSVG(svgName);
    if (svg.IsValid())
    {
      pControl = new ISVGKnobControl(bounds, svg, paramIdx);
    }
  }
  else if (type == "ISVGSliderControl")
  {
    std::string trackSvg = def.value("trackSVG", "");
    std::string handleSvg = def.value("handleSVG", "");
    EDirection dir = EDirection::Vertical;
    if (def.contains("direction") && def["direction"] == "horizontal")
      dir = EDirection::Horizontal;

    ISVG track = GetSVG(trackSvg);
    ISVG handle = GetSVG(handleSvg);

    if (track.IsValid() && handle.IsValid())
    {
      pControl = new ISVGSliderControl(bounds, track, handle, paramIdx, dir);
    }
  }
  else if (type == "ISVGButtonControl")
  {
    std::string offSvg = def.value("offSVG", "");
    std::string onSvg = def.value("onSVG", "");
    ISVG off = GetSVG(offSvg);
    ISVG on = GetSVG(onSvg);

    if (off.IsValid() && on.IsValid())
    {
      IActionFunction actionFunc = SplashClickActionFunc;
      if (def.contains("action"))
        actionFunc = GetAction(def["action"]);
      pControl = new ISVGButtonControl(bounds, actionFunc, off, on);
    }
  }
  else if (type == "ISVGToggleControl")
  {
    std::string offSvg = def.value("offSVG", "");
    std::string onSvg = def.value("onSVG", "");
    ISVG off = GetSVG(offSvg);
    ISVG on = GetSVG(onSvg);

    if (off.IsValid() && on.IsValid())
    {
      pControl = new ISVGToggleControl(bounds, paramIdx, off, on);
    }
  }
  else if (type == "ISVGSwitchControl")
  {
    std::vector<ISVG> svgList;
    if (def.contains("svgs"))
    {
      for (const auto& svgName : def["svgs"])
      {
        svgList.push_back(GetSVG(svgName.get<std::string>()));
      }
    }
    if (svgList.size() >= 2)
    {
      // ISVGSwitchControl requires an initializer_list, so we need to handle this differently
      // For 2 states, use the first two SVGs
      pControl = new ISVGSwitchControl(bounds, {svgList[0], svgList[1]}, paramIdx);
    }
  }
  else
  {
    DBGMSG("IGraphicsJSON: Unknown control type: %s\n", type.c_str());
  }

  return pControl;
}

void IGraphicsJSON::OnResize(const IRECT& newBounds)
{
  int nControls = mGraphics->NControls();
  int nLayouts = static_cast<int>(mLayouts.size());

  for (int i = 0; i < nLayouts && i < nControls; i++)
  {
    const auto& layout = mLayouts[i];

    // Get parent bounds
    IRECT parentBounds;
    if (layout.containerPtr)
    {
      // For container children, use the container's bounds
      parentBounds = layout.containerPtr->GetRECT();
    }
    else if (layout.parentIdx < 0)
    {
      parentBounds = newBounds;
    }
    else if (layout.parentIdx < nControls)
    {
      parentBounds = mGraphics->GetControl(layout.parentIdx)->GetRECT();
    }
    else
    {
      parentBounds = newBounds;
    }

    IRECT bounds = EvaluateBounds(i, parentBounds);
    mGraphics->GetControl(i)->SetTargetAndDrawRECTs(bounds);
  }
}

IRECT IGraphicsJSON::EvaluateBounds(int ctrlIdx, const IRECT& parent)
{
  if (ctrlIdx < 0 || ctrlIdx >= static_cast<int>(mLayouts.size()))
    return parent;

  const json& b = mLayouts[ctrlIdx].spec;

  if (b.is_null())
    return parent;

  // Simple array: [x, y, w, h] absolute pixels
  if (b.is_array() && b.size() >= 4)
  {
    float x = b[0].get<float>();
    float y = b[1].get<float>();
    float w = b[2].get<float>();
    float h = b[3].get<float>();
    return IRECT(parent.L + x, parent.T + y, parent.L + x + w, parent.T + y + h);
  }

  // Object with expressions
  IRECT result = parent;

  // Start from parent if specified
  if (b.contains("from") && b["from"] == "parent")
  {
    result = parent;
  }

  // Apply IRECT operations (order matters)
  if (b.contains("pad"))
  {
    result = result.GetPadded(b["pad"].get<float>());
  }

  if (b.contains("fracV"))
  {
    auto fv = b["fracV"];
    float frac = fv[0].get<float>();
    bool fromTop = fv[1].get<bool>();
    result = result.FracRectVertical(frac, fromTop);
  }

  if (b.contains("fracH"))
  {
    auto fh = b["fracH"];
    float frac = fh[0].get<float>();
    bool fromLeft = fh[1].get<bool>();
    result = result.FracRectHorizontal(frac, fromLeft);
  }

  if (b.contains("reduceFromRight"))
  {
    result = result.ReduceFromRight(b["reduceFromRight"].get<float>());
  }

  if (b.contains("reduceFromLeft"))
  {
    result = result.ReduceFromLeft(b["reduceFromLeft"].get<float>());
  }

  if (b.contains("reduceFromTop"))
  {
    result = result.ReduceFromTop(b["reduceFromTop"].get<float>());
  }

  if (b.contains("reduceFromBottom"))
  {
    result = result.ReduceFromBottom(b["reduceFromBottom"].get<float>());
  }

  if (b.contains("centredInside"))
  {
    auto ci = b["centredInside"];
    float w = ci[0].get<float>();
    float h = ci[1].get<float>();
    result = result.GetCentredInside(w, h);
  }

  if (b.contains("midVPadded"))
  {
    float padding = b["midVPadded"].get<float>();
    result = result.GetMidVPadded(padding);
  }

  if (b.contains("midHPadded"))
  {
    float padding = b["midHPadded"].get<float>();
    result = result.GetMidHPadded(padding);
  }

  // Apply shifts after shape-defining operations
  if (b.contains("vShift"))
  {
    float shift = EvaluateExpr(b["vShift"], parent, 'y');
    result = result.GetVShifted(shift);
  }

  if (b.contains("hShift"))
  {
    float shift = EvaluateExpr(b["hShift"], parent, 'x');
    result = result.GetHShifted(shift);
  }

  // Expression-based: "x": "50%", "w": "parent.w - 20"
  if (b.contains("x") || b.contains("y") || b.contains("w") || b.contains("h"))
  {
    float x = b.contains("x") ? EvaluateExpr(b["x"], parent, 'x') : parent.L;
    float y = b.contains("y") ? EvaluateExpr(b["y"], parent, 'y') : parent.T;
    float w = b.contains("w") ? EvaluateExpr(b["w"], parent, 'w') : parent.W();
    float h = b.contains("h") ? EvaluateExpr(b["h"], parent, 'h') : parent.H();

    // x/y are offsets from parent origin
    result = IRECT(parent.L + x, parent.T + y, parent.L + x + w, parent.T + y + h);
  }

  return result;
}

float IGraphicsJSON::EvaluateExpr(const json& val, const IRECT& parent, char dimension)
{
  if (val.is_number())
    return val.get<float>();

  if (!val.is_string())
    return 0.f;

  std::string expr = val.get<std::string>();

  // Percentage: "50%" = 50% of parent dimension
  if (!expr.empty() && expr.back() == '%')
  {
    float pct = std::stof(expr.substr(0, expr.size() - 1)) / 100.0f;
    switch (dimension)
    {
      case 'x':
      case 'w': return pct * parent.W();
      case 'y':
      case 'h': return pct * parent.H();
      default: return pct * parent.W();
    }
  }

  // Simple parent references: "parent.w", "parent.h", "parent.w - 20"
  if (expr.find("parent.w") != std::string::npos)
  {
    std::string modified = expr;
    size_t pos = modified.find("parent.w");
    modified.replace(pos, 8, std::to_string(parent.W()));

    // Try to evaluate simple arithmetic
    // For now, just handle "parent.w - N" and "parent.w * N"
    if (modified.find('-') != std::string::npos)
    {
      size_t dashPos = modified.find('-');
      float left = std::stof(modified.substr(0, dashPos));
      float right = std::stof(modified.substr(dashPos + 1));
      return left - right;
    }
    if (modified.find('*') != std::string::npos)
    {
      size_t mulPos = modified.find('*');
      float left = std::stof(modified.substr(0, mulPos));
      float right = std::stof(modified.substr(mulPos + 1));
      return left * right;
    }
    return std::stof(modified);
  }

  if (expr.find("parent.h") != std::string::npos)
  {
    std::string modified = expr;
    size_t pos = modified.find("parent.h");
    modified.replace(pos, 8, std::to_string(parent.H()));

    if (modified.find('-') != std::string::npos)
    {
      size_t dashPos = modified.find('-');
      float left = std::stof(modified.substr(0, dashPos));
      float right = std::stof(modified.substr(dashPos + 1));
      return left - right;
    }
    if (modified.find('*') != std::string::npos)
    {
      size_t mulPos = modified.find('*');
      float left = std::stof(modified.substr(0, mulPos));
      float right = std::stof(modified.substr(mulPos + 1));
      return left * right;
    }
    return std::stof(modified);
  }

  // Fallback: parse as number
  try
  {
    return std::stof(expr);
  }
  catch (...)
  {
    return 0.f;
  }
}

IVStyle IGraphicsJSON::ParseStyle(const json& j)
{
  IVStyle style = DEFAULT_STYLE;

  if (j.contains("showLabel")) style.showLabel = j["showLabel"].get<bool>();
  if (j.contains("showValue")) style.showValue = j["showValue"].get<bool>();
  if (j.contains("drawFrame")) style.drawFrame = j["drawFrame"].get<bool>();
  if (j.contains("drawShadows")) style.drawShadows = j["drawShadows"].get<bool>();
  if (j.contains("emboss")) style.emboss = j["emboss"].get<bool>();
  if (j.contains("roundness")) style.roundness = j["roundness"].get<float>();
  if (j.contains("frameThickness")) style.frameThickness = j["frameThickness"].get<float>();
  if (j.contains("shadowOffset")) style.shadowOffset = j["shadowOffset"].get<float>();
  if (j.contains("widgetFrac")) style.widgetFrac = j["widgetFrac"].get<float>();
  if (j.contains("angle")) style.angle = j["angle"].get<float>();

  // Colors
  if (j.contains("colorBG")) style.colorSpec.mColors[kBG] = ParseColor(j["colorBG"]);
  if (j.contains("colorFG")) style.colorSpec.mColors[kFG] = ParseColor(j["colorFG"]);
  if (j.contains("colorPR")) style.colorSpec.mColors[kPR] = ParseColor(j["colorPR"]);
  if (j.contains("colorFR")) style.colorSpec.mColors[kFR] = ParseColor(j["colorFR"]);
  if (j.contains("colorHL")) style.colorSpec.mColors[kHL] = ParseColor(j["colorHL"]);
  if (j.contains("colorSH")) style.colorSpec.mColors[kSH] = ParseColor(j["colorSH"]);
  if (j.contains("colorX1")) style.colorSpec.mColors[kX1] = ParseColor(j["colorX1"]);
  if (j.contains("colorX2")) style.colorSpec.mColors[kX2] = ParseColor(j["colorX2"]);
  if (j.contains("colorX3")) style.colorSpec.mColors[kX3] = ParseColor(j["colorX3"]);

  // Label text styling
  if (j.contains("labelText"))
  {
    style.labelText = ParseText(j["labelText"]);
  }

  // Value text styling
  if (j.contains("valueText"))
  {
    style.valueText = ParseText(j["valueText"]);
  }

  return style;
}

IVStyle IGraphicsJSON::GetStyle(const std::string& name)
{
  auto it = mStyles.find(name);
  if (it != mStyles.end())
    return it->second;
  return DEFAULT_STYLE;
}

void IGraphicsJSON::ApplyStyleOverrides(IVStyle& style, const json& j)
{
  if (j.contains("showLabel")) style.showLabel = j["showLabel"].get<bool>();
  if (j.contains("showValue")) style.showValue = j["showValue"].get<bool>();
  if (j.contains("drawFrame")) style.drawFrame = j["drawFrame"].get<bool>();
  if (j.contains("drawShadows")) style.drawShadows = j["drawShadows"].get<bool>();
  if (j.contains("emboss")) style.emboss = j["emboss"].get<bool>();
  if (j.contains("roundness")) style.roundness = j["roundness"].get<float>();
  if (j.contains("frameThickness")) style.frameThickness = j["frameThickness"].get<float>();
  if (j.contains("shadowOffset")) style.shadowOffset = j["shadowOffset"].get<float>();
  if (j.contains("widgetFrac")) style.widgetFrac = j["widgetFrac"].get<float>();
  if (j.contains("angle")) style.angle = j["angle"].get<float>();

  // Colors
  if (j.contains("colorBG")) style.colorSpec.mColors[kBG] = ParseColor(j["colorBG"]);
  if (j.contains("colorFG")) style.colorSpec.mColors[kFG] = ParseColor(j["colorFG"]);
  if (j.contains("colorPR")) style.colorSpec.mColors[kPR] = ParseColor(j["colorPR"]);
  if (j.contains("colorFR")) style.colorSpec.mColors[kFR] = ParseColor(j["colorFR"]);
  if (j.contains("colorHL")) style.colorSpec.mColors[kHL] = ParseColor(j["colorHL"]);
  if (j.contains("colorSH")) style.colorSpec.mColors[kSH] = ParseColor(j["colorSH"]);
  if (j.contains("colorX1")) style.colorSpec.mColors[kX1] = ParseColor(j["colorX1"]);
  if (j.contains("colorX2")) style.colorSpec.mColors[kX2] = ParseColor(j["colorX2"]);
  if (j.contains("colorX3")) style.colorSpec.mColors[kX3] = ParseColor(j["colorX3"]);

  // Label text styling
  if (j.contains("labelText"))
  {
    style.labelText = ParseText(j["labelText"]);
  }

  // Value text styling
  if (j.contains("valueText"))
  {
    style.valueText = ParseText(j["valueText"]);
  }
}

IBitmap IGraphicsJSON::GetBitmap(const std::string& name)
{
  auto it = mBitmapMap.find(name);
  if (it != mBitmapMap.end())
    return it->second;
  DBGMSG("IGraphicsJSON: Unknown bitmap: %s\n", name.c_str());
  return IBitmap();
}

ISVG IGraphicsJSON::GetSVG(const std::string& name)
{
  auto it = mSVGMap.find(name);
  if (it != mSVGMap.end())
    return it->second;
  DBGMSG("IGraphicsJSON: Unknown SVG: %s\n", name.c_str());
  return ISVG(nullptr);
}

IActionFunction IGraphicsJSON::GetAction(const std::string& name)
{
  auto it = mActionMap.find(name);
  if (it != mActionMap.end())
  {
    auto& userAction = it->second;
    return [userAction](IControl* pControl) { userAction(pControl); };
  }
  return SplashClickActionFunc;
}

IColor IGraphicsJSON::ParseColor(const json& c)
{
  // String format: "#RRGGBB" or "#RRGGBBAA"
  if (c.is_string())
  {
    std::string s = c.get<std::string>();
    if (s.empty() || s[0] != '#')
      return COLOR_WHITE;

    s = s.substr(1); // Remove #

    unsigned int hex = 0;
    std::stringstream ss;
    ss << std::hex << s;
    ss >> hex;

    if (s.length() == 6)
    {
      // #RRGGBB
      int r = (hex >> 16) & 0xFF;
      int g = (hex >> 8) & 0xFF;
      int b = hex & 0xFF;
      return IColor(255, r, g, b);
    }
    else if (s.length() == 8)
    {
      // #RRGGBBAA
      int r = (hex >> 24) & 0xFF;
      int g = (hex >> 16) & 0xFF;
      int b = (hex >> 8) & 0xFF;
      int a = hex & 0xFF;
      return IColor(a, r, g, b);
    }
  }
  // Array format: [r, g, b] or [r, g, b, a]
  else if (c.is_array())
  {
    int r = c[0].get<int>();
    int g = c[1].get<int>();
    int b = c[2].get<int>();
    int a = c.size() > 3 ? c[3].get<int>() : 255;
    return IColor(a, r, g, b);
  }

  return COLOR_WHITE;
}

IText IGraphicsJSON::ParseText(const json& t)
{
  IText text = DEFAULT_TEXT;

  if (t.contains("size")) text.mSize = t["size"].get<float>();
  if (t.contains("color")) text.mFGColor = ParseColor(t["color"]);

  if (t.contains("align"))
  {
    std::string align = t["align"];
    if (align == "left") text.mAlign = EAlign::Near;
    else if (align == "center") text.mAlign = EAlign::Center;
    else if (align == "right") text.mAlign = EAlign::Far;
  }

  if (t.contains("valign"))
  {
    std::string valign = t["valign"];
    if (valign == "top") text.mVAlign = EVAlign::Top;
    else if (valign == "middle") text.mVAlign = EVAlign::Middle;
    else if (valign == "bottom") text.mVAlign = EVAlign::Bottom;
  }

  return text;
}

int IGraphicsJSON::ResolveParam(const json& param)
{
  if (param.is_number())
    return param.get<int>();

  if (param.is_string())
  {
    std::string name = param.get<std::string>();
    auto it = mParamMap.find(name);
    if (it != mParamMap.end())
      return it->second;

    DBGMSG("IGraphicsJSON: Unknown param: %s\n", name.c_str());
  }

  return kNoParameter;
}

int IGraphicsJSON::ResolveTag(const std::string& id)
{
  auto it = mTagMap.find(id);
  if (it != mTagMap.end())
    return it->second;

  // Auto-generate tag from hash if not mapped
  // Simple hash to avoid collisions
  int hash = 0;
  for (char c : id)
  {
    hash = hash * 31 + c;
  }
  return hash & 0x7FFFFFFF; // Keep positive
}

IControl* IGraphicsJSON::GetControlById(const char* id)
{
  auto it = mIdToControlIdx.find(id);
  if (it != mIdToControlIdx.end())
  {
    int idx = it->second;
    if (idx >= 0 && idx < mGraphics->NControls())
      return mGraphics->GetControl(idx);
  }
  return nullptr;
}

void IGraphicsJSON::EnableHotReload(bool enable)
{
#ifndef NDEBUG
  mHotReloadEnabled = enable;
  if (enable && !mLoadedPath.empty())
  {
    struct stat st;
    if (stat(mLoadedPath.c_str(), &st) == 0)
    {
      mLastModified = st.st_mtime;
    }
  }
#endif
}

void IGraphicsJSON::CheckForChanges()
{
#ifndef NDEBUG
  if (!mHotReloadEnabled || mLoadedPath.empty())
    return;

  struct stat st;
  if (stat(mLoadedPath.c_str(), &st) != 0)
    return;

  if (st.st_mtime > mLastModified)
  {
    mLastModified = st.st_mtime;

    // Clear existing state
    mLayouts.clear();
    mIdToControlIdx.clear();
    mPageNameStorage.clear();

    // Remove all controls except background
    mGraphics->RemoveAllControls();

    // Reload
    Load(mLoadedPath.c_str());

    // Trigger resize to recompute bounds
    OnResize(mGraphics->GetBounds());

    DBGMSG("IGraphicsJSON: Hot-reloaded %s\n", mLoadedPath.c_str());
  }
#endif
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

#endif // IGRAPHICS_NO_JSON
