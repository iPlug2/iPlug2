#pragma once

// these are macros to shorten the instantiation of IControls
// for a paramater ID MyParam, define constants named MyParam_X, MyParam_Y, MyParam_W, MyParam_H to specify the Control's IRect
// then when instantiating a Control you can just call MakeIRect(MyParam) to specify the IRect
#define MakeIRect(a) IRECT(a##_X, a##_Y, a##_X + a##_W, a##_Y + a##_H)
#define MakeIRectHOffset(a, xoffs) IRECT(a##_X + xoffs, a##_Y, a##_X + a##_W + xoffs, a##_Y + a##_H)
#define MakeIRectVOffset(a, yoffs) IRECT(a##_X, a##_Y + yoffs, a##_X + a##_W, a##_Y + a##_H + yoffs)
#define MakeIRectHVOffset(a, xoffs, yoffs) IRECT(a##_X + xoffs, a##_Y + yoffs, a##_X + a##_W + xoffs, a##_Y + a##_H + yoffs)

#ifdef AAX_API
#include "AAX_Enums.h"

static uint32_t GetAAXModifiersFromIMouseMod(const IMouseMod& mod)
{
  uint32_t aax_mods = 0;
  
  if (mod.A) aax_mods |= AAX_eModifiers_Option; // ALT Key on Windows, ALT/Option key on mac
  
#ifdef OS_WIN
  if (mod.C) aax_mods |= AAX_eModifiers_Command;
#else
  if (mod.C) aax_mods |= AAX_eModifiers_Control;
  if (mod.R) aax_mods |= AAX_eModifiers_Command;
#endif
  if (mod.S) aax_mods |= AAX_eModifiers_Shift;
  if (mod.R) aax_mods |= AAX_eModifiers_SecondaryButton;
  
  return aax_mods;
}

//static void GetIMouseModFromAAXModifiers(uint32_t aax_mods, IMouseMod* pModOut)
//{
//  if (aax_mods & AAX_eModifiers_Option) pModOut->A = true; // ALT Key on Windows, ALT/Option key on mac
//#ifdef OS_WIN
//  if (aax_mods & AAX_eModifiers_Command) pModOut->C = true;
//#else
//  if (aax_mods & AAX_eModifiers_Control) pModOut->C = true;
//  if (aax_mods & AAX_eModifiers_Command) pModOut->R = true;
//#endif
//  if (aax_mods & AAX_eModifiers_Shift) pModOut->S = true;
//  if (aax_mods & AAX_eModifiers_SecondaryButton) pModOut->R = true;
//}
#endif

