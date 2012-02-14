#ifndef __IPLUGGUIRESIZE__
#define __IPLUGGUIRESIZE__

#include "IPlug_include_in_plug_hdr.h"

class IGUIResizeButton : public IInvisibleSwitchControl  
{
private:
  
public:
  IGUIResizeButton(IPlugBase* pPlug, IRECT pR)
	:	IInvisibleSwitchControl(pPlug, pR, -1)
  {
	}
	
	~IGUIResizeButton() {}
	
  bool Draw(IGraphics* pGraphics)
  {
    pGraphics->FillIRect(&COLOR_BLACK, &mRECT, &mBlend);
    return true;
  }
  
	void OnMouseDown(int x, int y, IMouseMod* pMod)
	{
    mPlug->GetGUI()->Resize(640, 480);
	}
};

class IPlugGUIResize : public IPlug
{
public:
  
  IPlugGUIResize(IPlugInstanceInfo instanceInfo);
  ~IPlugGUIResize();
  
  void Reset();
  void OnParamChange(int paramIdx);
  void OnWindowResize();
  
  void CreateControls(IGraphics* pGraphics, int size);
  
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);
  
private:
  
  double mGain;
};

#endif
