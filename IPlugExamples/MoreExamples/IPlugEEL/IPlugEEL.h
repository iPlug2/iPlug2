#ifndef __IPLUGEEL__
#define __IPLUGEEL__

#include "IPlug_include_in_plug_hdr.h"
#include "../../WDL/eel2/ns-eel.h"
#include "IControl.h"

#define MAX_ALG_LENGTH 65536

class AlgDisplay : public IControl
{
public:
  AlgDisplay(IPlugBase* pPlug, IRECT pR, IText* pText, const char* str = "")
  : IControl(pPlug, pR)
  {
    mDisablePrompt = true;
    mText = *pText;
    mStr.Set(str);
  }
  
  ~AlgDisplay() {}

  bool Draw(IGraphics* pGraphics)
  {  
    return pGraphics->DrawIText(&mText, mStr.Get(), &mRECT);
  }
  
  void OnMouseDown(int x, int y, IMouseMod* pMod)
  {
    mPlug->GetGUI()->CreateTextEntry(this, &mText, &mRECT, mStr.Get());
  }
  
  void TextFromTextEntry(const char* txt)
  {
    mStr.Set(txt, MAX_ALG_LENGTH);
    
    //TODO: update alg
    
    SetDirty(false);
  }
  
protected:
  WDL_String mStr;
};

class IPlugEEL : public IPlug
{
public:
  IPlugEEL(IPlugInstanceInfo instanceInfo);
  ~IPlugEEL();

  void Reset();
  void OnParamChange(int paramIdx);
  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:
  double mGain;
  double* mVmOutput;
  
  NSEEL_VMCTX vm;
  NSEEL_CODEHANDLE codehandle;
  char codetext[MAX_ALG_LENGTH]; // this needs to be larger than might be anticipated as eel2 uses this buffer as workspace
  AlgDisplay* mTextControl;
};

#endif
