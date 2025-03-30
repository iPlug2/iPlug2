#pragma once

#include "IControls.h"
#include "IVTabbedPagesControl.h"

#include "faust/gui/GUI.h"
#include "faust/gui/MetaDataUI.h"
#include "faust/gui/ValueConverter.h"

#include "IPlugFaust.h"

BEGIN_IPLUG_NAMESPACE

class FaustPanelControl : public IVPanelControl
{
public:
  FaustPanelControl(const IRECT& bounds, const char* label, const IVStyle& style, const EDirection& dir, float padding = 10.f)
  : IVPanelControl(bounds, label, style)
  , mDirection(dir)
  , mPadding(padding)
  {
    SetAttachFunc([this, label](IContainerBase* pContainer, const IRECT& bounds) {
     for (int c = 0; c < mControlsToAttach.size(); c++) {
       pContainer->AddChildControl(mControlsToAttach[c].second, mControlsToAttach[c].first, label);
     }
      OnResize();
    });
    
    SetResizeFunc([this](IContainerBase* pContainer, const IRECT& bounds) {
      mControlIdx = 0;
       for (int c = 0; c < pContainer->NChildren(); c++) {
         auto* pChildControl = pContainer->GetChild(c);
         auto bounds = GetNextRect();
         if (auto* pKnobControl = pChildControl->As<IKnobControlBase>()) {
           bounds = bounds.GetCentredInside(100, 70);
         }
         pChildControl->SetTargetAndDrawRECTs(bounds);
       }
    });
  };
  
  void AddControl(IControl* pControl, int ctrlTag = -1)
  {
    mControlsToAttach.push_back({ctrlTag, pControl});
  }
  
  IRECT GetNextRect()
  {
    return mWidgetBounds.GetPadded(-mPadding).SubRect(mDirection, (int) mControlsToAttach.size(), mControlIdx++);
  }
  
private:
  float mPadding;
  EDirection mDirection;
  int mControlIdx = 0;
  std::vector<std::pair<int, IControl*>> mControlsToAttach;
};

class IGraphicsFaustUI : public GUI, public MetaDataUI, public Meta
{
  friend class IPlugFaust;
  
  using CreateControlFunc = std::function<IControl*(const IRECT& r, int paramIdx, const char* label)>;
  using CreatePanelFunc = std::function<FaustPanelControl*(const IRECT& r, EDirection direction, const char* label)>;

public:
  IGraphicsFaustUI(IPlugFaust& IPlugFaust, const IVStyle& style = DEFAULT_STYLE, int ctrlTagStart = 1000)
  : mIPlugFaust(IPlugFaust)
  , mStyle(style)
  , mCtrlTagIdx(ctrlTagStart)
  {
#pragma mark - Default control creation funcs
    createKnobFunc = [this](const IRECT& r, int paramIdx, const char* label) -> IControl* {
      return new IVKnobControl(r, paramIdx, label, mStyle);
    };
    
    createHSliderFunc = [this](const IRECT& r, int paramIdx, const char* label) -> IControl* {
      return new IVSliderControl(r, paramIdx, label, mStyle, true, EDirection::Horizontal);
    };
    
    createVSliderFunc = [this](const IRECT& r, int paramIdx, const char* label) -> IControl* {
      return new IVSliderControl(r, paramIdx, label, mStyle, true, EDirection::Vertical);
    };
    
    createButtonFunc = [this](const IRECT& r, int paramIdx, const char* label) -> IControl* {
      return new IVToggleControl(r, paramIdx, label, mStyle, "Off", "On");
    };
    
    createToggleFunc = [this](const IRECT& r, int paramIdx, const char* label) -> IControl* {
      return new IVToggleControl(r, paramIdx, label, mStyle, "Off", "On");
    };
    
    createNumericalEntryFunc = [this](const IRECT& r, int paramIdx, const char* label) -> IControl* {
      return new IVNumberBoxControl(r, paramIdx, nullptr, label, mStyle);
    };
    
    createRadioButtonFunc = [this](const IRECT& r, int paramIdx, const char* label) -> IControl* {
      return new IVRadioButtonControl(r, paramIdx, {}, label, mStyle);
    };
    
    createMenuButtonFunc = [this](const IRECT& r, int paramIdx, const char* label) -> IControl* {
      return new IVMenuButtonControl(r, paramIdx, label, mStyle);
    };

    createHBarGraphFunc = [this](const IRECT& r, int paramIdx, const char* label) -> IControl* {
      return new IVLEDMeterControl<>(r, label, mStyle, EDirection::Horizontal);
    };

    createVBarGraphFunc = [this](const IRECT& r, int paramIdx, const char* label) -> IControl* {
      return new IVLEDMeterControl<>(r, label, mStyle, EDirection::Vertical);
    };

    createNumericalDisplayFunc = [this](const IRECT& r, int paramIdx, const char* label) -> IControl* {
      return new IRTTextControl<1, float>(r);
    };

    createLedFunc = [this](const IRECT& r, int paramIdx, const char* label) -> IControl* {
      return new ILEDControl(r);
    };

    createPanelFunc = [this](const IRECT& r, EDirection direction, const char* label) -> FaustPanelControl* {
      return new FaustPanelControl(r, label, mStyle, direction);
    };
  }
  
  ~IGraphicsFaustUI()
  {
  }
  
#pragma mark - Faust UI
  
  void openTabBox(const char *label) override
  {
//    AddTabbedPages(label); // Not yet working
    AddPanel(EDirection::Horizontal, label);
  }
  
  void openHorizontalBox(const char *label) override
  {
    AddPanel(EDirection::Horizontal, label);
  }
  
  void openVerticalBox(const char *label) override
  {
    AddPanel(EDirection::Vertical, label);
  }
  
  void closeBox() override
  {
    if (!mPageMaps.empty())
    {
      auto pageMap = GetTopPageMap();
      auto* pControl = new IVTabbedPagesControl(IRECT(), pageMap);
      mPageMaps.pop();
      GetTopPanel()->AddControl(pControl);
    }
    else
    {
      mPanels.pop();
    }
  }
  
  void addButton(const char *label, ffloat *zone) override
  {
    if (!isHidden(zone))
    {
      auto paramIdx = GetParamIdxForZone(zone);
      auto* pControl = createButtonFunc(IRECT(), paramIdx, label);
      pControl->SetTooltip(fTooltip[zone].c_str());
      
      GetTopPanel()->AddControl(pControl);
    }
  }
  
  void addCheckButton(const char *label, ffloat *zone) override
  {
    if (!isHidden(zone))
    {
      auto paramIdx = GetParamIdxForZone(zone);
      auto* pControl = createToggleFunc(IRECT(), paramIdx, label);
      pControl->SetTooltip(fTooltip[zone].c_str());
      GetTopPanel()->AddControl(pControl);
    }
  }
  
  void addVerticalSlider(const char *label, ffloat *zone, ffloat init, ffloat min, ffloat max, ffloat step) override
  {
    if (!isHidden(zone))
    {
      auto paramIdx = GetParamIdxForZone(zone);
      auto* pControl = isKnob(zone) ? createKnobFunc(IRECT(), paramIdx, label) : createVSliderFunc(IRECT(), paramIdx, label);
      pControl->SetTooltip(fTooltip[zone].c_str());
      GetTopPanel()->AddControl(pControl);
    }
  }
  
  void addHorizontalSlider(const char *label, ffloat *zone, ffloat init, ffloat min, ffloat max, ffloat step) override
  {
    if (!isHidden(zone))
    {
      auto paramIdx = GetParamIdxForZone(zone);
      auto* pControl = isKnob(zone) ? createKnobFunc(IRECT(), paramIdx, label) : createHSliderFunc(IRECT(), paramIdx, label);
      pControl->SetTooltip(fTooltip[zone].c_str());
      GetTopPanel()->AddControl(pControl);
    }
  }
  
  void addNumEntry(const char *label, ffloat *zone, ffloat init, ffloat min, ffloat max, ffloat step) override
  {
    if (!isHidden(zone))
    {
      auto paramIdx = GetParamIdxForZone(zone);
      IControl* pControl = nullptr;
            
      if (isMenu(zone)) {
        pControl = createMenuButtonFunc(IRECT(), paramIdx, label);
      }
      else {
        pControl = createNumericalEntryFunc(IRECT(), paramIdx, label);
      }
      
      pControl->SetTooltip(fTooltip[zone].c_str());
      GetTopPanel()->AddControl(pControl);
    }
  }
  
  void addHorizontalBargraph(const char *label, ffloat *zone, ffloat min, ffloat max) override
  {
    AddBarGraph(label, zone, min, max, EDirection::Horizontal);
  }
  
  void addVerticalBargraph(const char *label, ffloat *zone, ffloat min, ffloat max) override
  {
    AddBarGraph(label, zone, min, max, EDirection::Vertical);
  }
  
  // NOT used
  void addSoundfile(const char *label, const char *filename, Soundfile **sf_zone) override {}
  
  void declare(FAUSTFLOAT* zone, const char* key, const char* value) override
  {
    MetaDataUI::declare(zone, key, value);
  }
  
  virtual void declare(const char* key, const char* value) override
  {
    
  }
  
#pragma mark -
  
  IContainerBase* CreateFaustUIContainer(const IRECT&r)
  {
    mRect = r;
    mIPlugFaust.BuildUI(this);
    return mControlToAttach;
  }
  

private:
  void AddPanel(EDirection direction, const char* label)
  {
    FaustPanelControl* pNewPanel;
    // Might be top level
    if (mPanels.empty())
    {
      pNewPanel = createPanelFunc(mRect, direction, label);
      mControlToAttach = pNewPanel;
    }
    else
    {
      pNewPanel = createPanelFunc(GetNextRect(), direction, label);
      GetTopPanel()->AddControl(pNewPanel);
    }
    
    mPanels.push(pNewPanel);
  }
  
  void AddTabbedPages(const char* label)
  {
    if (mPageMaps.empty() || GetTopPageMap().at(label))
    {
      mPageMaps.push(PageMap({}));
    }
    
    GetTopPageMap().insert({label, new IVTabPage([](IVTabPage* pPage, const IRECT& r) {
      // TODO
    })});
  }
  
  void AddBarGraph(const char* label, ffloat* zone, ffloat min, ffloat max, EDirection direction)
  {
    if (!isHidden(zone))
    {
      IControl* pControl = nullptr;
      
      if (isLed(zone)) {
        pControl = createLedFunc(IRECT(), kNoParameter, label);
      }
      else if (isNumerical(zone)) {
        pControl = createNumericalDisplayFunc(IRECT(), kNoParameter, label);
      }
      else {
        pControl = direction == EDirection::Horizontal ? createHBarGraphFunc(IRECT(), kNoParameter, label)
                                                       : createVBarGraphFunc(IRECT(), kNoParameter, label);
      }
      
      pControl->SetTooltip(fTooltip[zone].c_str());
      GetTopPanel()->AddControl(pControl, mCtrlTagIdx++);
    }
  }

  IRECT GetNextRect()
  {
    return mRect.GetGridCell(mControlIdx++, 10, 1); // TODO: need to know how many panels
  }

  int GetParamIdxForZone(ffloat* zone)
  {
    return mIPlugFaust.GetParamIdxForZone(zone);
  }
  
  FaustPanelControl* GetTopPanel() { return mPanels.top(); }
  
  PageMap& GetTopPageMap() { return mPageMaps.top(); }

  int mControlIdx = 0;
  IRECT mRect;
  IPlugFaust& mIPlugFaust;
  IVStyle mStyle;
  std::stack<FaustPanelControl*> mPanels;
  std::stack<PageMap> mPageMaps;

  FaustPanelControl* mControlToAttach = nullptr;
  int mCtrlTagIdx;

public:
  CreateControlFunc createKnobFunc;
  CreateControlFunc createHSliderFunc;
  CreateControlFunc createVSliderFunc;
  CreateControlFunc createButtonFunc;
  CreateControlFunc createToggleFunc;
  CreateControlFunc createNumericalEntryFunc;
  CreateControlFunc createRadioButtonFunc;
  CreateControlFunc createMenuButtonFunc;
  CreateControlFunc createHBarGraphFunc;
  CreateControlFunc createVBarGraphFunc;
  CreateControlFunc createLedFunc;
  CreateControlFunc createNumericalDisplayFunc;
  CreatePanelFunc createPanelFunc;
};

END_IPLUG_NAMESPACE
