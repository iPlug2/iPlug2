/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#pragma once

/**
 * @file
 * @copydoc IGraphicsFaustUI
 */

#include <array>
#include <functional>
#include <memory>

#include "IControls.h"
#include "IVTabbedPagesControl.h"
#include "IVMeterControl.h"
#include "ILEDControl.h"

#include "IPlugFaust.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

/** Builds an IGraphics user interface from the UI description of an IPlugFaust DSP, like Faust's own generic GUIs.
 * Faust boxes become panels (hgroup/vgroup) or tabbed pages (tgroup), widgets become IVControls linked to the IPlug parameters created by
 * IPlugFaust::CreateIPlugParameters() and bargraphs become meters/LEDs/value displays updated via their control tags.
 * Widget metadata such as [style:knob], [style:menu{...}], [style:radio{...}], [style:led], [style:numerical], [unit:dB], [tooltip:...] and [hidden:1] is respected.
 *
 * Basic usage, in the plug-in's layout function:
 * \code{.cpp}
 * IGraphicsFaustUI faustUI(mFaust);
 * pGraphics->AttachControl(faustUI.CreateControl(pGraphics->GetBounds().GetPadded(-10.f)));
 * \endcode
 * The control keeps a copy of the UI description, so the IGraphicsFaustUI object doesn't need to outlive it. If the DSP is recompiled
 * (see FaustGen::SetOnCompileFunc), remove the control and create a new one.
 * The functions used to create the controls for each widget type and to size them can be replaced to customise the UI. */
class IGraphicsFaustUI
{
public:
  using Item = FaustUIItem;
  using EType = FaustUIItem::EType;

  /** Creates the control for a widget
   * @param item The widget's description
   * @param paramIdx The IPlug parameter linked to the widget, or kNoParameter
   * @param style The style to use
   * @return The new control, or nullptr to skip the widget */
  using CreateControlFunc = std::function<IControl*(const Item& item, int paramIdx, const IVStyle& style)>;

  /** Returns the bounds of a widget's control, given the cell that the layout allocated for it */
  using ControlBoundsFunc = std::function<IRECT(const Item& item, const IRECT& cell)>;

  /** A momentary button, for Faust's button() */
  class ButtonControl : public IVButtonControl
  {
  public:
    ButtonControl(const IRECT& bounds, int paramIdx, const char* label, const IVStyle& style)
    : IVButtonControl(bounds, nullptr, label, style, true, false)
    {
      SetParamIdx(paramIdx);
    }

    void OnMouseDown(float x, float y, const IMouseMod& mod) override
    {
      SetValue(1.);
      SetDirty(true);
    }

    void OnMouseUp(float x, float y, const IMouseMod& mod) override
    {
      SetValue(0.);
      SetDirty(true);
    }
  };

  /** Displays a bargraph's value as text, for [style:numerical] */
  class ValueDisplayControl : public IControl, public IVectorBase
  {
  public:
    ValueDisplayControl(const IRECT& bounds, const char* label, const IVStyle& style, double min, double max, const char* unit)
    : IControl(bounds)
    , IVectorBase(style)
    , mMin(min)
    , mMax(max)
    , mUnit(unit)
    {
      mIgnoreMouse = true;
      AttachIControl(this, label);
    }

    void Draw(IGraphics& g) override
    {
      DrawLabel(g);
      WDL_String str;
      str.SetFormatted(64, "%.2f %s", mMin + GetValue() * (mMax - mMin), mUnit.Get());
      g.DrawText(mStyle.valueText, str.Get(), mWidgetBounds, &mBlend);
    }

    void OnResize() override
    {
      SetTargetRECT(MakeRects(mRECT));
      SetDirty(false);
    }

  private:
    double mMin, mMax;
    WDL_String mUnit;
  };

  /** A labelled panel that lays out the controls for the items of a Faust box (hgroup/vgroup) */
  class BoxControl;

  /** @param faust The Faust DSP, which must have been initialized. Call IPlugFaust::CreateIPlugParameters() first to link the controls to parameters.
   * @param style The style for the controls */
  IGraphicsFaustUI(IPlugFaust& faust, const IVStyle& style = DEFAULT_STYLE)
  : mFaust(faust)
  , mConfig(std::make_shared<Config>())
  {
    mConfig->style = style;

    for (int t = 0; t < kNumTypes; t++)
      mConfig->createFuncs[t] = DefaultCreateControl;

    mConfig->boundsFunc = DefaultControlBounds;
  }

  /** Create a control containing the whole Faust UI. Attach it with IGraphics::AttachControl()
   * @param bounds The bounds of the UI
   * @return The control, or nullptr if the DSP has no UI */
  inline IControl* CreateControl(const IRECT& bounds);

  /** Replace the function that creates the controls for a widget type */
  void SetCreateControlFunc(EType type, CreateControlFunc func) { mConfig->createFuncs[static_cast<int>(type)] = func; }

  /** Replace the function that sizes the controls within their layout cells */
  void SetControlBoundsFunc(ControlBoundsFunc func) { mConfig->boundsFunc = func; }

  /** Set the padding between controls and around boxes */
  void SetPadding(float padding) { mConfig->padding = padding; }

  /** Set the height of the tab bar for tab boxes */
  void SetTabBarHeight(float height) { mConfig->tabBarHeight = height; }

  /** The default control for each widget type */
  static IControl* DefaultCreateControl(const Item& item, int paramIdx, const IVStyle& style)
  {
    const char* label = item.label.c_str();
    IControl* pControl = nullptr;

    switch (item.type)
    {
      case EType::Button:
        pControl = new ButtonControl(IRECT(), paramIdx, label, style);
        break;
      case EType::CheckButton:
        pControl = new IVToggleControl(IRECT(), paramIdx, label, style, "Off", "On");
        break;
      case EType::HSlider:
      case EType::VSlider:
      {
        const EDirection dir = item.type == EType::HSlider ? EDirection::Horizontal : EDirection::Vertical;

        if (item.menu)
          pControl = new IVMenuButtonControl(IRECT(), paramIdx, label, style);
        else if (item.radio)
          pControl = new IVRadioButtonControl(IRECT(), paramIdx, {}, label, style, EVShape::Ellipse, dir);
        else if (item.knob)
          pControl = new IVKnobControl(IRECT(), paramIdx, label, style, true);
        else
          pControl = new IVSliderControl(IRECT(), paramIdx, label, style, true, dir);
        break;
      }
      case EType::NumEntry:
        if (item.menu)
          pControl = new IVMenuButtonControl(IRECT(), paramIdx, label, style);
        else if (item.radio)
          pControl = new IVRadioButtonControl(IRECT(), paramIdx, {}, label, style);
        else if (item.knob)
          pControl = new IVKnobControl(IRECT(), paramIdx, label, style, true);
        else
          pControl = new IVNumberBoxControl(IRECT(), paramIdx, nullptr, label, style);
        break;
      case EType::HBargraph:
      case EType::VBargraph:
        if (item.led)
          pControl = new ILEDControl(IRECT());
        else if (item.numerical)
          pControl = new ValueDisplayControl(IRECT(), label, style, item.min, item.max, item.unit.c_str());
        else
          pControl = new IVMeterControl<1>(IRECT(), label, style, item.type == EType::HBargraph ? EDirection::Horizontal : EDirection::Vertical);
        break;
      default:
        break;
    }

    if (pControl && item.tooltip.size())
      pControl->SetTooltip(item.tooltip.c_str());

    return pControl;
  }

  /** Default control sizing: stops controls from stretching too much in large cells */
  static IRECT DefaultControlBounds(const Item& item, const IRECT& cell)
  {
    auto constrain = [&cell](float maxW, float maxH) {
      return cell.GetCentredInside(std::min(cell.W(), maxW), std::min(cell.H(), maxH));
    };

    switch (item.type)
    {
      case EType::Button:
      case EType::CheckButton:
        return constrain(150.f, 60.f);
      case EType::HSlider:
        if (item.knob) return constrain(100.f, 130.f);
        if (item.menu) return constrain(150.f, 60.f);
        return constrain(300.f, item.radio ? 100.f : 70.f);
      case EType::VSlider:
        if (item.knob) return constrain(100.f, 130.f);
        if (item.menu) return constrain(150.f, 60.f);
        return constrain(item.radio ? 150.f : 70.f, 300.f);
      case EType::NumEntry:
        if (item.knob) return constrain(100.f, 130.f);
        return constrain(150.f, item.radio ? 150.f : 60.f);
      case EType::HBargraph:
        if (item.led) return constrain(30.f, 30.f);
        return constrain(300.f, item.numerical ? 50.f : 60.f);
      case EType::VBargraph:
        if (item.led) return constrain(30.f, 30.f);
        return constrain(item.numerical ? 150.f : 50.f, 300.f);
      default:
        return cell;
    }
  }

private:
  static constexpr int kNumTypes = static_cast<int>(EType::Soundfile) + 1;

  struct Config
  {
    IVStyle style;
    std::array<CreateControlFunc, kNumTypes> createFuncs;
    ControlBoundsFunc boundsFunc;
    float padding = 5.f;
    float tabBarHeight = 25.f;
  };

  /** The description and config shared by all the controls of one UI, kept alive by them */
  struct Context
  {
    Item root;
    std::vector<int> plugParamIdx; // Faust parameter index -> IPlug parameter index
    std::shared_ptr<const Config> config;

    int GetPlugParamIdx(const Item& item) const
    {
      return (item.paramIdx > kNoParameter && item.paramIdx < static_cast<int>(plugParamIdx.size())) ? plugParamIdx[item.paramIdx] : kNoParameter;
    }
  };

  static bool IsVisible(const Item& item)
  {
    return !item.hidden && item.type != EType::Soundfile;
  }

  static inline IControl* CreateItemControl(const std::shared_ptr<const Context>& pContext, const Item& item, const IRECT& bounds = IRECT());

  IPlugFaust& mFaust;
  std::shared_ptr<Config> mConfig;
};

class IGraphicsFaustUI::BoxControl : public IContainerBase, public IVectorBase
{
public:
  BoxControl(const std::shared_ptr<const Context>& pContext, const Item& item, const IRECT& bounds)
  : IContainerBase(bounds)
  , IVectorBase(pContext->config->style)
  , mContext(pContext)
  , mItem(item)
  {
    mIgnoreMouse = true;
    AttachIControl(this, item.label.c_str());
  }

  /** Don't show the label, e.g. when the box is a page of a tab box, which already shows it */
  void HideLabel() { mShowLabel = false; }

  void Draw(IGraphics& g) override
  {
    if (!HasLabel())
      return;

    DrawLabel(g);

    if (mStyle.drawFrame)
      g.DrawRoundRect(GetColor(kFR), mWidgetBounds, GetRoundedCornerRadius(mWidgetBounds), &mBlend, mStyle.frameThickness);
  }

  void OnAttached() override
  {
    for (const auto& child : mItem.children)
    {
      if (!IsVisible(child))
        continue;

      if (IControl* pControl = CreateItemControl(mContext, child))
      {
        AddChildControl(pControl, child.ctrlTag);
        mChildItems.push_back(&child);
      }
    }

    OnResize();
  }

  void OnResize() override
  {
    if (!GetUI()) // not attached yet
      return;

    SetTargetRECT(HasLabel() ? MakeRects(mRECT) : mRECT);

    const int nChildren = NChildren();

    if (!nChildren)
      return;

    const float padding = mContext->config->padding;
    const IRECT area = (HasLabel() ? mWidgetBounds : mRECT).GetPadded(-padding);
    const EDirection dir = mItem.type == EType::HBox ? EDirection::Horizontal : EDirection::Vertical;

    // Each child gets space in proportion to the number of widgets it lays out in this direction
    float totalWeight = 0.f;

    for (const Item* pChildItem : mChildItems)
      totalWeight += Weight(*pChildItem, dir);

    float start = 0.f;

    for (int i = 0; i < nChildren; i++)
    {
      const Item& childItem = *mChildItems[i];
      const float end = start + Weight(childItem, dir) / totalWeight;
      const IRECT cell = (dir == EDirection::Horizontal ? area.FracRectHorizontal(end - start).GetHShifted(area.W() * start)
                                                         : area.FracRectVertical(end - start, true).GetVShifted(area.H() * start)).GetPadded(-padding);
      GetChild(i)->SetTargetAndDrawRECTs(childItem.IsGroup() ? cell : mContext->config->boundsFunc(childItem, cell));
      start = end;
    }
  }

  /** The relative amount of space an item needs in a direction: widgets count as 1, boxes sum their children in their own direction and take the maximum across it */
  static float Weight(const Item& item, EDirection dir)
  {
    if (!item.IsGroup())
      return 1.f;

    const bool sum = (item.type == EType::HBox && dir == EDirection::Horizontal) || (item.type == EType::VBox && dir == EDirection::Vertical);
    float weight = 0.f;

    for (const auto& child : item.children)
    {
      if (!IsVisible(child))
        continue;

      const float childWeight = Weight(child, dir);
      weight = sum ? weight + childWeight : std::max(weight, childWeight);
    }

    return std::max(weight, 1.f);
  }

private:
  bool HasLabel() const { return mShowLabel && !mItem.label.empty(); }

  std::shared_ptr<const Context> mContext;
  const Item& mItem;
  bool mShowLabel = true;
  std::vector<const Item*> mChildItems;
};

IControl* IGraphicsFaustUI::CreateItemControl(const std::shared_ptr<const Context>& pContext, const Item& item, const IRECT& bounds)
{
  const Config& config = *pContext->config;

  switch (item.type)
  {
    case EType::HBox:
    case EType::VBox:
      return new BoxControl(pContext, item, bounds);
    case EType::TabBox:
    {
      std::vector<const Item*> visibleChildren;

      for (const auto& child : item.children)
      {
        if (IsVisible(child))
          visibleChildren.push_back(&child);
      }

      // A tab bar needs at least two tabs
      if (visibleChildren.size() == 1)
        return CreateItemControl(pContext, *visibleChildren[0], bounds);

      PageMap pages;

      for (const auto& child : item.children)
      {
        if (!IsVisible(child))
          continue;

        const Item* pChild = &child;

        auto attachFunc = [pContext, pChild](IVTabPage* pPage, const IRECT& bounds) {
          if (IControl* pControl = CreateItemControl(pContext, *pChild))
          {
            if (auto* pBox = pControl->As<BoxControl>())
              pBox->HideLabel();

            pPage->AddChildControl(pControl, pChild->ctrlTag);
          }
        };

        auto resizeFunc = [pContext, pChild](IContainerBase* pPage, const IRECT& bounds) {
          if (pPage->NChildren() == 1)
          {
            const IRECT cell = bounds.GetPadded(-pContext->config->padding);
            pPage->GetChild(0)->SetTargetAndDrawRECTs(pChild->IsGroup() ? cell : pContext->config->boundsFunc(*pChild, cell));
          }
        };

        pages.push_back({child.label.c_str(), new IVTabPage(attachFunc, resizeFunc, config.style, 0.)});
      }

      if (pages.empty())
        return nullptr;

      return new IVTabbedPagesControl(bounds, pages, item.label.c_str(), config.style, config.tabBarHeight, 1.f);
    }
    default:
    {
      const CreateControlFunc& createFunc = config.createFuncs[static_cast<int>(item.type)];
      return createFunc ? createFunc(item, pContext->GetPlugParamIdx(item), config.style) : nullptr;
    }
  }
}

IControl* IGraphicsFaustUI::CreateControl(const IRECT& bounds)
{
  auto pContext = std::make_shared<Context>();
  pContext->root = mFaust.GetUIRoot();
  pContext->config = std::make_shared<const Config>(*mConfig);

  for (int p = 0; p < mFaust.NParams(); p++)
    pContext->plugParamIdx.push_back(mFaust.GetPlugParamIdx(p));

  const Item& root = pContext->root;

  if (root.children.empty())
    return nullptr;

  return CreateItemControl(pContext, root, bounds);
}

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE
