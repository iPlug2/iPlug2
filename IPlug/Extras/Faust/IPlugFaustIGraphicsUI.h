  #pragma once

  #include "IControls.h"
  #include "IVTabbedPagesControl.h"

  #include "faust/gui/GUI.h"
  #include "faust/gui/MetaDataUI.h"
  #include "faust/gui/ValueConverter.h"

  #include "IPlugFaust.h"

  BEGIN_IPLUG_NAMESPACE
  BEGIN_IGRAPHICS_NAMESPACE

  class IGraphicsFaustUI : public GUI, public MetaDataUI, public Meta
  {
  public:
    friend class IPlugFaust;
    class FaustPanelControl;
    
    using CreateControlFunc = std::function<IControl*(const IRECT& r, int paramIdx, const char* label)>;
    using ControlSizeFunc = std::function<IRECT(IControl* pControl, const IRECT& r)>;
    using CreatePanelFunc = std::function<FaustPanelControl*(const IRECT& r, EDirection direction, const char* label, bool isTabbedBox, ControlSizeFunc controlSizeFunc)>;

    class FaustPanelControl : public IVPanelControl
    {
    public:
      FaustPanelControl(const IRECT& bounds, const char* label, const IVStyle& style, const EDirection& dir, bool isTabbedBox, ControlSizeFunc controlSizeFunc, float padding = 10.f)
      : IVPanelControl(bounds, label, style)
      , mDirection(dir)
      , mIsTabbedBox(isTabbedBox)
      , mControlSizeFunc(controlSizeFunc)
      , mPadding(padding)
      {
        SetAttachFunc([this, label](IContainerBase* pContainer, const IRECT& bounds) {
          if (!mIsTabbedBox) {
            for (auto&& [ctrlTag, pControl] : mControlsToAttach) {
              pContainer->AddChildControl(pControl, ctrlTag, label);
            }
          }
          else {
            PageMap pageMap;
            
            for (auto&& [ctrlTag, pControl] : mControlsToAttach) {
              auto* pFaustPanelControl = pControl->As<FaustPanelControl>();
              auto* labelStr = pFaustPanelControl->GetLabelStr();
              pageMap.insert({labelStr, new IVTabPage(
              [pFaustPanelControl](IContainerBase* pPage, const IRECT& r) {
                                for (auto&& [ctrlTag2, pControl2] : pFaustPanelControl->mControlsToAttach) {
                                  pPage->AddChildControl(pControl2, ctrlTag2, pFaustPanelControl->GetLabelStr());
                                }
              },
              // Resize func for each tab
              [pFaustPanelControl](IContainerBase* pContainer, const IRECT& bounds) {
                auto nChildren = pContainer->NChildren();
                for (auto c = 0; c < nChildren; c++) {
                  auto* pChildControl = pContainer->GetChild(c);
                  auto childBounds = bounds.GetPadded(-pFaustPanelControl->mPadding).SubRect(pFaustPanelControl->mDirection, nChildren, c);
                  childBounds = pFaustPanelControl->mControlSizeFunc(pChildControl, childBounds);
                  pChildControl->SetTargetAndDrawRECTs(childBounds);
                }
              })});
            }
            pContainer->AddChildControl(new IVTabbedPagesControl(IRECT(), pageMap, ""));
          }
          OnResize();
        });
        
        SetResizeFunc([this](IContainerBase* pContainer, const IRECT& bounds) {
          if (!mIsTabbedBox) {
            auto* pFaustPanelControl = pContainer->As<FaustPanelControl>();
            auto nChildren = pContainer->NChildren();
            for (int c = 0; c < nChildren; c++) {
              auto* pChildControl = pContainer->GetChild(c);
              auto childBounds = bounds.GetPadded(-mPadding).SubRect(pFaustPanelControl->mDirection, nChildren, c);
              childBounds = pFaustPanelControl->mControlSizeFunc(pChildControl, childBounds);
              pChildControl->SetTargetAndDrawRECTs(childBounds);
            }
          }
          else {
            pContainer->GetChild(0)->SetTargetAndDrawRECTs(bounds.GetPadded(-mPadding));
          }
        });
      };
      
      void AddControl(IControl* pControl, int ctrlTag = -1)
      {
        mControlsToAttach.push_back({ctrlTag, pControl});
      }
      
    private:
      const bool mIsTabbedBox;
      float mPadding;
      EDirection mDirection;
      // controls must be attached in the attachFunc, so they are put in this queue first
      std::vector<std::pair<int, IControl*>> mControlsToAttach;
      ControlSizeFunc mControlSizeFunc;
    };
    
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

      createPanelFunc = [this](const IRECT& r, EDirection direction, const char* label, bool isTabbedBox, ControlSizeFunc controlSizeFunc) -> FaustPanelControl* {
        return new FaustPanelControl(r, label, mStyle, direction, isTabbedBox, controlSizeFunc);
      };

      // Set this func to force the size of different controls
      controlSizeFunc = [this](IControl* pControl, const IRECT& r) -> IRECT {
      //  if (auto* pKnobControl = pControl->As<IKnobControlBase>()) {
      //    return r.GetCentredInside(70, 100);
      //  } else if (auto* pSliderControl = pControl->As<ISliderControlBase>()) {
      //    if (pSliderControl->GetDirection() == EDirection::Horizontal) {
      //      return r.GetCentredInside(150, 70);
      //    } else {
      //      return r.GetCentredInside(70, 150);
      //    }
      //  } else if (auto* pToggleControl = pControl->As<ISwitchControlBase>()) {
      //    return r.GetCentredInside(100, 70);
      //  } else if (auto* pNumberBoxControl = pControl->As<IVNumberBoxControl>()) {
      //    return r.GetCentredInside(100, 70);
      //  } else if (auto* pRadioButtonControl = pControl->As<IVRadioButtonControl>()) {
      //    return r.GetCentredInside(100, 70);
      //  } else if (auto* pMenuButtonControl = pControl->As<IVMenuButtonControl>()) {
      //    return r.GetCentredInside(100, 70);
      //  } else if (auto* pLEDMeterControl = pControl->As<IVLEDMeterControl<>>()) {
      //    if (pLEDMeterControl->GetDirection() == EDirection::Horizontal) {
      //      return r.GetCentredInside(150, 70);
      //    } else {
      //      return r.GetCentredInside(70, 150);
      //    }
      //  } else if (auto* pTextControl = pControl->As<IRTTextControl<>>()) {
      //    return r.GetCentredInside(100, 70);
      //  } else if (auto* pLedControl = pControl->As<ILEDControl>()) {
      //    return r.GetCentredInside(100, 70);
      //  }

        return r;
      };
    }
    
    ~IGraphicsFaustUI()
    {
    }
    
  #pragma mark - Faust UI
    
    void openTabBox(const char *label) override
    {
      Trace("FAUSTUI: openTabBox:" , __LINE__, " %s", label);
      AddPanel(EDirection::Horizontal, label, true);
    }
    
    void openHorizontalBox(const char *label) override
    {
      Trace("FAUSTUI: openHorizontalBox:" , __LINE__, " %s", label);
      AddPanel(EDirection::Horizontal, label, false);
    }
    
    void openVerticalBox(const char *label) override
    {
      Trace("FAUSTUI: openVerticalBox:" , __LINE__, " %s", label);
      AddPanel(EDirection::Vertical, label, false);
    }
    
    void closeBox() override
    {
      Trace("FAUSTUI: closeBox", __LINE__, "");

      if (!mPanels.empty())
      {
        mPanels.pop();
      }
    }
    
    void addButton(const char *label, ffloat *zone) override
    {
      Trace("FAUSTUI: addButton:" , __LINE__, " %s", label);
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
      Trace("FAUSTUI: addCheckButton:" , __LINE__, " %s", label);
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
      Trace("FAUSTUI: addVerticalSlider:" , __LINE__, " %s", label);
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
      Trace("FAUSTUI: addHorizontalSlider:" , __LINE__, " %s", label);
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
      Trace("FAUSTUI: addNumEntry:" , __LINE__, " %s", label);
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
      Trace("FAUSTUI: addHorizontalBargraph:" , __LINE__, " %s", label);
      AddBarGraph(label, zone, min, max, EDirection::Horizontal);
    }
    
    void addVerticalBargraph(const char *label, ffloat *zone, ffloat min, ffloat max) override
    {
      Trace("FAUSTUI: addVerticalBargraph:" , __LINE__, " %s", label);
      AddBarGraph(label, zone, min, max, EDirection::Vertical);
    }
    
    // NOT used
    void addSoundfile(const char *label, const char *filename, Soundfile **sf_zone) override 
    {
      Trace("FAUSTUI: addSoundfile:" , __LINE__, " %s", label);
    }
    
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
      return mTopLevelControl;
    }
    
    void Resize(const IRECT& r)
    {
      mTopLevelControl->SetTargetAndDrawRECTs(r);
      mTopLevelControl->OnResize();
    }
    

  private:
    void AddPanel(EDirection direction, const char* label, bool isTabbedBox)
    {
      FaustPanelControl* pNewPanel;
      // Might be top level
      if (mPanels.empty())
      {
        pNewPanel = createPanelFunc(mRect, direction, label, isTabbedBox, controlSizeFunc);
        mTopLevelControl = pNewPanel;
      }
      else
      {
        pNewPanel = createPanelFunc(IRECT(), direction, label, isTabbedBox, controlSizeFunc);
        GetTopPanel()->AddControl(pNewPanel);
      }
      
      mPanels.push(pNewPanel);
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

    int GetParamIdxForZone(ffloat* zone)
    {
      return mIPlugFaust.GetParamIdxForZone(zone);
    }
    
    FaustPanelControl* GetTopPanel() { return mPanels.top(); }
    
    int mControlIdx = 0;
    IRECT mRect;
    IPlugFaust& mIPlugFaust;
    IVStyle mStyle;
    std::stack<FaustPanelControl*> mPanels;
    FaustPanelControl* mTopLevelControl = nullptr;
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
    ControlSizeFunc controlSizeFunc;
  };

  END_IGRAPHICS_NAMESPACE
  END_IPLUG_NAMESPACE
