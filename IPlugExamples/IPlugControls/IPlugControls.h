#ifndef __IPLUGCONTROLSDEMO__
#define __IPLUGCONTROLSDEMO__

#include "IPlug_include_in_plug_hdr.h"

class IPlugControls : public IPlug
{
public:

  IPlugControls(IPlugInstanceInfo instanceInfo);
  ~IPlugControls();

  void OnParamChange(int paramIdx);

  void ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames);

private:

  int mIISC_Indicator;
};

const int kNumPrograms = 1;

enum EParams
{
  kISwitchControl_2,
  kISwitchControl_3,
  kIInvisibleSwitchControl,
  kIRadioButtonsControl_H,
  kIRadioButtonsControl_V,
  kIContactControl,
  kIFaderControl_Horiz,
  kIFaderControl_Vert,
  kIKnobLineControl_def,
  kIKnobLineControl_lo_gear,
  kIKnobRotaterControl_def,
  kIKnobRotaterControl_restrict,
  kIKnobMultiControl_def,
  kIKnobMultiControl_Horiz,
  kIKnobRotatingMaskControl,
  kICaptionControl,
  kNumParams,   // put any controls to be controlled from the plug but not
  kInvisibleSwitchIndicator   // the user after kNumParams so they get a param id
};

enum ELayout
{
  kW = GUI_WIDTH,  // width of plugin window
  kH = GUI_HEIGHT,	// height of plugin window

  kISwitchControl_2_N = 2,  // # of sub-bitmaps.
  kISwitchControl_2_X = 64,  // position of left side of control
  kISwitchControl_2_Y = 72,  // position of top of control

  kISwitchControl_3_N = 3,
  kISwitchControl_3_X = 216,
  kISwitchControl_3_Y = 72,

  kIInvisibleSwitchControl_X = 216,
  kIInvisibleSwitchControl_Y = 192,
  kIISC_W = 48,   // width of control
  kIISC_H = 48,	// height of control

  kIISC_I_X = 75, // position of InvisibleSwitch indicator graphic
  kIISC_I_Y = 203,

  kIRadioButtonsControl_N = 2,
  kIRBC_W = 24,  // width of bitmap
  kIRBC_H = 24,  // height of one of the bitmap images
  kIRadioButtonsControl_H_X = 23,
  kIRadioButtonsControl_H_Y = 320,
  kIRBC_HN = 4,  // number of horizontal buttons
  kIRadioButtonsControl_V_X = 228,
  kIRadioButtonsControl_V_Y = 285,
  kIRBC_VN = 3,  // number of vertical buttons

  kIContactControl_N = 2,
  kIContactControl_X = 136,
  kIContactControl_Y = 424,

  kIFaderControl_Horiz_L = 200,  // fader length
  kIFaderControl_Horiz_X = 0,
  kIFaderControl_Horiz_Y = 544,

  kIFaderControl_Vert_L = 100,
  kIFaderControl_Vert_X = 216,
  kIFaderControl_Vert_Y = 512,

  kIKLC_W = 48, //knob area horizontal size
  kIKLC_H = 48, //knob area vertical size
  kIKnobLineControl_def_X = 392,
  kIKnobLineControl_def_Y = 72,

  kIKnobLineControl_lo_gear_X = 576,
  kIKnobLineControl_lo_gear_Y = 72,

  kIKnobRotaterControl_def_X = 384,
  kIKnobRotaterControl_def_Y = 184,

  kIKnobRotaterControl_restrict_X = 568,
  kIKnobRotaterControl_restrict_Y = 184,

  kIKnobMultiControl_N = 14,
  kIKnobMultiControl_def_X = 376,
  kIKnobMultiControl_def_Y = 296,

  kIKnobMultiControl_Horiz_X = 560,
  kIKnobMultiControl_Horiz_Y = 296,

  //IKnobRotatingMaskControl
  kIKRMC_X = 575,
  kIKRMC_Y = 436,

  //IBitmapOverlayControl
  kIBOC_X = 8,	//top left position of bitmap overlay
  kIBOC_Y = 400,
  kIBOC_T_X = 576, //top left position of target area
  kIBOC_T_Y = 552,
  kIBOC_T_W = 48, //width of target area (square in this example so no height required)

  //ITextControl
  kITC_X = 784,
  kITC_Y = 87,
  kITC_W = 100,
  kITC_H = 20,

  //ICaptionControl
  kICC_X = 784,
  kICC_Y = 202,
  kICC_W = 100,
  kICC_H = 20,

  //IURLControl
  kIUC_X = 824,
  kIUC_Y = 312,
  kIUC_W = 48

};

#endif
