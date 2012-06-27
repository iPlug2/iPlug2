#include "IPlugControls.h"
#include "IPlug_include_in_plug_src.h"
#include "IControl.h"
#include "resource.h"

IPlugControls::IPlugControls(IPlugInstanceInfo instanceInfo)
  : IPLUG_CTOR(kNumParams, kNumPrograms, instanceInfo)
{
  TRACE;

  // Define parameter ranges, display units, labels.

  GetParam(kISwitchControl_2)->InitBool("ISwitchControl 2 image multi", 0, "images");
  GetParam(kISwitchControl_3)->InitInt("ISwitchControl 3 image multi", 0, 0, 2, "");

  GetParam(kIInvisibleSwitchControl)->InitBool("IInvisibleSwitchControl", false, "");
  mIISC_Indicator = false;

  GetParam(kIRadioButtonsControl_H)->InitInt("IRadioButtonsControl Horiz", 1, 1, 4, "button");
  GetParam(kIRadioButtonsControl_V)->InitInt("IRadioButtonsControl Vert", 1, 1, 3, "button");

  GetParam(kIContactControl)->InitBool("IContactControl", 0, "");

  GetParam(kIFaderControl_Horiz)->InitDouble("IFaderControl Horiz", .0, .0, 12., .1, "");
  GetParam(kIFaderControl_Vert)->InitDouble("IFaderControl Vert", .0, .0, 12., .1, "");

  GetParam(kIKnobLineControl_def)->InitDouble("IKnobLineControl Default", .0, .0, 12., .1, "");
  GetParam(kIKnobLineControl_lo_gear)->InitDouble("IKnobLineControl Lo Gear", .0, .0, 12., .1, "");

  GetParam(kIKnobRotaterControl_def)->InitDouble("IKnobRotaterControl Default", 2., .0, 5., .1, "");
  GetParam(kIKnobRotaterControl_restrict)->InitDouble("IKnobRotaterControl Restricted", 2., .0, 5., .1, "");

  GetParam(kIKnobMultiControl_def)->InitInt("IKnobMultiControl Default", 1, 1, 14, "");
  GetParam(kIKnobMultiControl_Horiz)->InitInt("IKnobMultiControl Horiz", 1, 1, 14, "");

  GetParam(kIKnobRotatingMaskControl)->InitDouble("IKnobRotatingMaskControl", .0, .0, 10., 0.1, "");

  GetParam(kICaptionControl)->InitInt("ICaptionControl", 1, 1, 14, "label");

  // Instantiate a graphics engine.

  IGraphics* pGraphics = MakeGraphics(this, kW, kH); // MakeGraphics(this, kW, kH);
  pGraphics->AttachBackground(BG_ID, BG_FN);

  // Attach controls to the graphics engine.  Controls are automatically associated
  // with a parameter if you construct the control with a parameter index.


  // Attach the ISwitchControl 2 image "switch"
  IBitmap bitmap = pGraphics->LoadIBitmap(ISWITCHCONTROL_2_ID, ISWITCHCONTROL_2_FN, kISwitchControl_2_N);
  pGraphics->AttachControl(new ISwitchControl(this, kISwitchControl_2_X, kISwitchControl_2_Y, kISwitchControl_2, &bitmap));

  // Attach the ISwitchControl 2 image "switch"
  bitmap = pGraphics->LoadIBitmap(ISWITCHCONTROL_3_ID, ISWITCHCONTROL_3_FN, kISwitchControl_3_N);
  pGraphics->AttachControl(new ISwitchControl(this, kISwitchControl_3_X, kISwitchControl_3_Y, kISwitchControl_3, &bitmap));


  // Attach the IInvisibleSwitchControl target area (IRECT expects the top left, then the bottom right coordinates)
  pGraphics->AttachControl(new IInvisibleSwitchControl(this, IRECT(kIInvisibleSwitchControl_X, kIInvisibleSwitchControl_Y, kIInvisibleSwitchControl_X + kIISC_W, kIInvisibleSwitchControl_Y + kIISC_H), kIInvisibleSwitchControl));

  // Attach a graphic for the InvisibleSwitchControl indicator, not associated with any parameter,
  // which we keep an index for, so we can push updates from the plugin class.

  bitmap = pGraphics->LoadIBitmap(IRADIOBUTTONSCONTROL_ID, IRADIOBUTTONSCONTROL_FN, kIRadioButtonsControl_N);

  mIISC_Indicator = pGraphics->AttachControl(new IBitmapControl(this, kIISC_I_X, kIISC_I_Y, &bitmap));

  //Attach the horizontal IRadioButtonsControl
  bitmap = pGraphics->LoadIBitmap(IRADIOBUTTONSCONTROL_ID, IRADIOBUTTONSCONTROL_FN, kIRadioButtonsControl_N);
  pGraphics->AttachControl(new IRadioButtonsControl(this, IRECT(kIRadioButtonsControl_H_X, kIRadioButtonsControl_H_Y, kIRadioButtonsControl_H_X + ((kIRBC_W*kIRBC_HN) + 10), kIRadioButtonsControl_H_Y + (kIRBC_H*kIRBC_HN)), kIRadioButtonsControl_H, kIRBC_HN, &bitmap, kHorizontal)); // added 10 to button area to show spreading the buttons out a bit

  //Attach the vertical IRadioButtonsControl
  bitmap = pGraphics->LoadIBitmap(IRADIOBUTTONSCONTROL_ID, IRADIOBUTTONSCONTROL_FN, kIRadioButtonsControl_N);
  pGraphics->AttachControl(new IRadioButtonsControl(this, IRECT(kIRadioButtonsControl_V_X, kIRadioButtonsControl_V_Y, kIRadioButtonsControl_V_X + (kIRBC_W*kIRBC_VN), kIRadioButtonsControl_V_Y + (kIRBC_H*kIRBC_VN)), kIRadioButtonsControl_V, kIRBC_VN, &bitmap));


  //Attach the IContactControl
  bitmap = pGraphics->LoadIBitmap(ICONTACTCONTROL_ID, ICONTACTCONTROL_FN, kIContactControl_N);
  pGraphics->AttachControl(new IContactControl(this, kIContactControl_X, kIContactControl_Y, kIContactControl, &bitmap));

  //Attach the horizontal IFaderControl
  bitmap = pGraphics->LoadIBitmap(IFADERCONTROL_HORIZ_ID, IFADERCONTROL_HORIZ_FN);
  pGraphics->AttachControl(new IFaderControl(this, kIFaderControl_Horiz_X, kIFaderControl_Horiz_Y, kIFaderControl_Horiz_L, kIFaderControl_Horiz, &bitmap, kHorizontal));

  //Attach the vertical IFaderControl
  bitmap = pGraphics->LoadIBitmap(IFADERCONTROL_VERT_ID, IFADERCONTROL_VERT_FN);
  pGraphics->AttachControl(new IFaderControl(this, kIFaderControl_Vert_X, kIFaderControl_Vert_Y, kIFaderControl_Vert_L, kIFaderControl_Vert, &bitmap)); // kVertical is default

  IColor lineColor = IColor(255, 0, 0, 128);

  //Attach the default IKnobLineControl
  pGraphics->AttachControl(new IKnobLineControl(this, IRECT(kIKnobLineControl_def_X, kIKnobLineControl_def_Y, kIKnobLineControl_def_X + kIKLC_W, kIKnobLineControl_def_Y + kIKLC_H), kIKnobLineControl_def, &lineColor));

  //Attach the low geared IKnobLineControl
  pGraphics->AttachControl(new IKnobLineControl(this, IRECT(kIKnobLineControl_lo_gear_X, kIKnobLineControl_lo_gear_Y, kIKnobLineControl_lo_gear_X + kIKLC_W, kIKnobLineControl_lo_gear_Y + kIKLC_H), kIKnobLineControl_lo_gear, &lineColor, 0., 0., (-0.75*PI), (0.75*PI), kVertical, 2.0));

  //Attach the IKnobRotaterControl default rotation range
  bitmap = pGraphics->LoadIBitmap(IKNOBROTATERCONTROL_ID, IKNOBROTATERCONTROL_FN);
  pGraphics->AttachControl(new IKnobRotaterControl(this, kIKnobRotaterControl_def_X, kIKnobRotaterControl_def_Y, kIKnobRotaterControl_def, &bitmap));

  //Attach the IKnobRotaterControl restricted rotation range
  bitmap = pGraphics->LoadIBitmap(IKNOBROTATERCONTROL_ID, IKNOBROTATERCONTROL_FN);
  pGraphics->AttachControl(new IKnobRotaterControl(this, kIKnobRotaterControl_restrict_X, kIKnobRotaterControl_restrict_Y, kIKnobRotaterControl_restrict, &bitmap, -0.5*PI, 1.0*PI)); //restrict range going anticlockwise to 90 degrees and let clockwise movement go full range

  //Attach the default IKnobMultiControl
  bitmap = pGraphics->LoadIBitmap(IKNOBMULTICONTROL_ID, IKNOBMULTICONTROL_FN, kIKnobMultiControl_N);
  pGraphics->AttachControl(new IKnobMultiControl(this, kIKnobMultiControl_def_X, kIKnobMultiControl_def_Y, kIKnobMultiControl_def, &bitmap));

  //Attach the horizontally moused IKnobMultiControl
  bitmap = pGraphics->LoadIBitmap(IKNOBMULTICONTROL_ID, IKNOBMULTICONTROL_FN, kIKnobMultiControl_N);
  pGraphics->AttachControl(new IKnobMultiControl(this, kIKnobMultiControl_Horiz_X, kIKnobMultiControl_Horiz_Y, kIKnobMultiControl_Horiz, &bitmap, kHorizontal));

  //Attach the IKnobRotatingMaskControl (specify the min and max angle in degrees, not radians, in this control)
  IBitmap base = pGraphics->LoadIBitmap(IKRMC_BASE_ID, IKRMC_BASE_FN);
  IBitmap mask = pGraphics->LoadIBitmap(IKRMC_MASK_ID, IKRMC_MASK_FN);
  IBitmap top = pGraphics->LoadIBitmap(IKRMC_TOP_ID, IKRMC_TOP_FN);
  pGraphics->AttachControl(new IKnobRotatingMaskControl(this, kIKRMC_X, kIKRMC_Y, kIKnobRotatingMaskControl, &base, &mask, &top, -135., 135.));

  //Attach IBitmapOverlayControl
  bitmap = pGraphics->LoadIBitmap(IBOC_ID, IBOC_FN);
  pGraphics->AttachControl(new IBitmapOverlayControl(this, kIBOC_X, kIBOC_Y, &bitmap, IRECT(kIBOC_T_X, kIBOC_T_Y, (kIBOC_T_X + kIBOC_T_W), (kIBOC_T_Y + kIBOC_T_W))));

  IText text = IText(14);

  //Attach ITextControl
  pGraphics->AttachControl(new ITextControl(this, IRECT(kITC_X, kITC_Y, (kITC_X + kITC_W), (kITC_Y + kITC_H)), &text, "Display text strings"));

  //Attach ICaptionControl
  pGraphics->AttachControl(new ICaptionControl(this, IRECT(kICC_X, kICC_Y, (kICC_X + kICC_W), (kICC_Y + kICC_H)), kICaptionControl, &text));

  //Attach IURLControl
  pGraphics->AttachControl(new IURLControl(this, IRECT(kIUC_X, kIUC_Y, (kIUC_X + kIUC_W), (kIUC_Y + kIUC_W)), "https://github.com/audio-plugins/wdl-ce/wiki"));

  // Attach the graphics engine to the plugin.

  AttachGraphics(pGraphics);

  // No cleanup necessary, the graphics engine manages all of its resources and cleans up when closed.

  MakeDefaultPreset((char *) "-", kNumPrograms);
}

IPlugControls::~IPlugControls() {}

void IPlugControls::ProcessDoubleReplacing(double** inputs, double** outputs, int nFrames)
{
  // Mutex is already locked for us.

  double* in1 = inputs[0];
  double* in2 = inputs[1];
  double* out1 = outputs[0];
  double* out2 = outputs[1];

  for (int s = 0; s < nFrames; ++s, ++in1, ++in2, ++out1, ++out2)
  {
    *out1 = *in1;
    *out2 = *in2;
  }
}

void IPlugControls::OnParamChange(int paramIdx)
{
  IMutexLock lock(this);

  if (paramIdx == kIInvisibleSwitchControl)
  {
    if (GetGUI())
    {
      GetGUI()->SetControlFromPlug(mIISC_Indicator, GetParam(paramIdx)->Bool());
    }
  }

  if (paramIdx == kIKnobMultiControl_def)
  {
    if (GetGUI())
    {
      GetGUI()->SetParameterFromPlug(kICaptionControl, GetParam(paramIdx)->Int(), false);
    }
    InformHostOfParamChange(kICaptionControl, (GetParam(paramIdx)->Int())/14); //inform host of new normalized value
  }

}


