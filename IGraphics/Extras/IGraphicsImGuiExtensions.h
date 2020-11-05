#pragma once

#if defined IGRAPHICS_IMGUI

#include <imgui_internal.h>
using namespace ImGui;

static const int COLOR_EDIT_FLAGS = ImGuiColorEditFlags_Uint8|ImGuiColorEditFlags_DisplayHex|ImGuiColorEditFlags_InputRGB|ImGuiColorEditFlags_PickerHueBar|ImGuiColorEditFlags_AlphaBar;

bool ImGuiIVColorEditor(const char* label, IColor& color)
{
  float rgbaf[4];
  color.GetRGBAf(rgbaf);
  
  if(ImGui::ColorEdit4(label, rgbaf, COLOR_EDIT_FLAGS))
  {
    color = IColor::FromRGBAf(rgbaf);
    return true;
  }
  
  return false;
}

bool ImGuiIVColorSpecEditor(const char* label, IVColorSpec& spec)
{
  bool change = false;
  
  for(int i = 0; i < kNumVColors; i++)
  {
    float rgbaf[4];
    spec.GetColor((EVColor) i).GetRGBAf(rgbaf);
    
    if(ImGui::ColorEdit4(kVColorStrs[i], rgbaf, COLOR_EDIT_FLAGS))
    {
      spec.SetColor((EVColor) i, IColor::FromRGBAf(rgbaf));
      change = true;
    }
  }
  
  return change;
}

bool ImGuiITextEditor(const char* label, IText& text)
{
  bool change = false;
    
  ImGui::PushID(label);
  
  change = ImGui::SliderFloat("Size", &text.mSize, 5.f, 200.f);
  change |= ImGuiIVColorEditor("Color", text.mFGColor);
  change |= ImGui::SliderFloat("Angle", &text.mAngle, 0.f, 360.f, "%.02f");
  int align = (int) text.mAlign;
  if(ImGui::Combo("h align", &align, kEAlignStrs, 3))
  {
    text.mAlign = (EAlign) align;
    change = true;
  }
  
  int valign = (int) text.mVAlign;
  if(ImGui::Combo("v align", &valign, kEVAlignStrs, 3))
  {
    text.mVAlign = (EVAlign) valign;
    change = true;
  }
  
  ImGui::PopID();

  return change;
}

bool ImGuiIVStyleEditor(const char* label, IVStyle& style)
{
  bool change = false;
  
  ImGuiTabBarFlags tab_bar_flags = ImGuiTabBarFlags_None;
  if (ImGui::BeginTabBar("label", tab_bar_flags))
  {
    if (ImGui::BeginTabItem("Misc"))
    {
      change |= ImGui::Checkbox("Show Label", &style.showLabel);
      change |= ImGui::Checkbox("Show Value", &style.showValue);
      change |= ImGui::Checkbox("Hide Cursor", &style.hideCursor);
      change |= ImGui::Checkbox("Draw Frame", &style.drawFrame);
      change |= ImGui::Checkbox("Draw Shadows", &style.drawShadows);
      change |= ImGui::SliderFloat("Roundness", &style.roundness, 0.f, 1.f, "%.02f");
      change |= ImGui::SliderFloat("Frame Thickness", &style.frameThickness, 0.f, 10.f, "%.02f");
      change |= ImGui::SliderFloat("Shadow Offset", &style.shadowOffset, 1.f, 10.f, "%.02f");
      change |= ImGui::SliderFloat("Widget Fraction", &style.widgetFrac, 0.1f, 1.f, "%.02f");
      change |= ImGui::SliderFloat("Widget Angle", &style.angle, 0.f, 360.f, "%.02f");
      ImGui::EndTabItem();
    }
    
    if (ImGui::BeginTabItem("Colors"))
    {
      change |= ImGuiIVColorSpecEditor("Colors:", style.colorSpec);
      ImGui::EndTabItem();
    }
    
    if (ImGui::BeginTabItem("Label Text"))
    {
      change |= ImGuiITextEditor("Label Text:", style.labelText);
      ImGui::EndTabItem();
    }
    
    if (ImGui::BeginTabItem("Value Text"))
    {
      change |= ImGuiITextEditor("Value Text:", style.valueText);
      ImGui::EndTabItem();
    }
  }
  ImGui::EndTabBar();
  
  return change;
}

#endif
