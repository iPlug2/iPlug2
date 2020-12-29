/*
 ==============================================================================

 This file is part of the iPlug 2 library. Copyright (C) the iPlug 2 developers.

 See LICENSE.txt for  more info.

 ==============================================================================
*/

#if defined IGRAPHICS_IMGUI

#include "IPlugPlatform.h"
#include "IGraphicsImGui.h"
#include "IGraphics_select.h"

#if defined IGRAPHICS_GL2
  #include "imgui_impl_opengl2.h"
#elif defined IGRAPHICS_GL3 || defined IGRAPHICS_GLES2 || defined IGRAPHICS_GLES3
  #include "imgui_impl_opengl3.h"
#else
  #if defined OS_MAC || defined OS_IOS
    #import <Metal/Metal.h>
    #import <QuartzCore/QuartzCore.h>
    #include "imgui_impl_metal.h"
  #else
    #error "ImGui is only supported on this platform using the OpenGL based backends"
  #endif
#endif

using namespace iplug;
using namespace igraphics;

ImGuiRenderer::ImGuiRenderer(IGraphics* pGraphics, std::function<void(IGraphics*)> drawFunc, std::function<void()> setupFunc)
: mGraphics(pGraphics)
, mDrawFunc(drawFunc)
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  
  if(setupFunc == nullptr)
  {
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    ImGui::StyleColorsDark();
  }
  else
    setupFunc();
  
  // Setup back-end capabilities flags
  io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;         // We can honor GetMouseCursor() values (optional)
  io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;          // We can honor io.WantSetMousePos requests (optional, rarely used)
  io.BackendPlatformName = "imgui_impl_igraphics";
  
  io.KeyMap[ImGuiKey_Tab] = kVK_TAB;
  io.KeyMap[ImGuiKey_LeftArrow] = kVK_LEFT;
  io.KeyMap[ImGuiKey_RightArrow] = kVK_RIGHT;
  io.KeyMap[ImGuiKey_UpArrow] = kVK_UP;
  io.KeyMap[ImGuiKey_DownArrow] = kVK_DOWN;
  io.KeyMap[ImGuiKey_PageUp] = kVK_PRIOR;
  io.KeyMap[ImGuiKey_PageDown] = kVK_NEXT;
  io.KeyMap[ImGuiKey_Home] = kVK_HOME;
  io.KeyMap[ImGuiKey_End] = kVK_END;
  io.KeyMap[ImGuiKey_Insert] = kVK_INSERT;
  io.KeyMap[ImGuiKey_Delete] = kVK_DELETE;
  io.KeyMap[ImGuiKey_Backspace] = kVK_BACK;
  io.KeyMap[ImGuiKey_Space] = kVK_SPACE;
  io.KeyMap[ImGuiKey_Enter] = kVK_RETURN;
  io.KeyMap[ImGuiKey_Escape] = kVK_ESCAPE;
  io.KeyMap[ImGuiKey_A] = 'A';
  io.KeyMap[ImGuiKey_C] = 'C';
  io.KeyMap[ImGuiKey_V] = 'V';
  io.KeyMap[ImGuiKey_X] = 'X';
  io.KeyMap[ImGuiKey_Y] = 'Y';
  io.KeyMap[ImGuiKey_Z] = 'Z';
  
  //    io.SetClipboardTextFn = [](void*, const char* str) -> void
  //    {
  //    };
  //
  //    io.GetClipboardTextFn = [](void*) -> const char*
  //    {
  //    };
  
  Init();
}

ImGuiRenderer::~ImGuiRenderer()
{
  Destroy();
  ImGui::DestroyContext();
}

void ImGuiRenderer::DoFrame()
{
  ImGuiIO &io = ImGui::GetIO();
  io.DisplaySize.x = std::round(mGraphics->Width() * mGraphics->GetDrawScale());
  io.DisplaySize.y = std::round(mGraphics->Height() * mGraphics->GetDrawScale());
  float scale = (float) mGraphics->GetScreenScale();
  io.DisplayFramebufferScale = ImVec2(scale, scale);
  io.DeltaTime = 1.f / mGraphics->FPS();
  
  if (!(io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange))
  {
    ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
    if (imgui_cursor == ImGuiMouseCursor_None || io.MouseDrawCursor)
    {
      mGraphics->HideMouseCursor();
    }
    else if (io.WantCaptureMouse)
    {
      switch(imgui_cursor)
      {
        case ImGuiMouseCursor_Arrow: mGraphics->SetMouseCursor(ECursor::ARROW); break;
        case ImGuiMouseCursor_TextInput: mGraphics->SetMouseCursor(ECursor::IBEAM); break;
        case ImGuiMouseCursor_ResizeAll: mGraphics->SetMouseCursor(ECursor::SIZEALL); break;
        case ImGuiMouseCursor_ResizeNS: mGraphics->SetMouseCursor(ECursor::SIZENS); break;
        case ImGuiMouseCursor_ResizeEW: mGraphics->SetMouseCursor(ECursor::SIZEWE); break;
        case ImGuiMouseCursor_ResizeNESW: mGraphics->SetMouseCursor(ECursor::SIZENESW); break;
        case ImGuiMouseCursor_ResizeNWSE: mGraphics->SetMouseCursor(ECursor::SIZENWSE); break;
        case ImGuiMouseCursor_Hand: mGraphics->SetMouseCursor(ECursor::HAND); break;
      }
      mGraphics->HideMouseCursor(false);
    }
  }
  
  if (io.WantSetMousePos)
  {
    mGraphics->MoveMouseCursor(io.MousePos.x, io.MousePos.y);
  }
  
  ImGui::NewFrame();
  
  if(mDrawFunc)
    mDrawFunc(mGraphics);
  
  ImGui::Render();
}

bool ImGuiRenderer::OnMouseDown(float x, float y, const IMouseMod &mod)
{
  ImGuiIO &io = ImGui::GetIO();  
  io.MouseDown[0] = mod.L;
  io.MouseDown[1] = mod.R;
  return io.WantCaptureMouse;
}

bool ImGuiRenderer::OnMouseUp(float x, float y, const IMouseMod &mod)
{
  ImGuiIO &io = ImGui::GetIO();
  io.MouseDown[0] = false;
  io.MouseDown[1] = false;
  return io.WantCaptureMouse;
}

bool ImGuiRenderer::OnMouseWheel(float x, float y, const IMouseMod& mod, float delta)
{
  ImGuiIO &io = ImGui::GetIO();
  io.MouseWheel += (delta * 0.1f);
  return io.WantCaptureMouse;
}

void ImGuiRenderer::OnMouseMove(float x, float y, const IMouseMod& mod)
{
  ImGuiIO &io = ImGui::GetIO();
  io.MousePos = ImVec2(x * mGraphics->GetDrawScale(), y * mGraphics->GetDrawScale());
}

bool ImGuiRenderer::OnKeyDown(float x, float y, const IKeyPress& keyPress)
{
  ImGuiIO &io = ImGui::GetIO();
  
  if(keyPress.VK != kVK_BACK)
    io.AddInputCharactersUTF8(keyPress.utf8);
  
  io.KeysDown[keyPress.VK] = true;
  
  io.KeyCtrl = keyPress.C;
  io.KeyShift = keyPress.S;
  io.KeyAlt = keyPress.A;
  
  return io.WantCaptureKeyboard;
}

bool ImGuiRenderer::OnKeyUp(float x, float y, const IKeyPress& keyPress)
{
  ImGuiIO &io = ImGui::GetIO();
  io.KeysDown[keyPress.VK] = false;
  return io.WantCaptureKeyboard;
}

void ImGuiRenderer::Init()
{
#if defined IGRAPHICS_GL2
  ImGui_ImplOpenGL2_Init();
#elif defined IGRAPHICS_GL3
  ImGui_ImplOpenGL3_Init();
#elif defined IGRAPHICS_GLES2 || defined IGRAPHICS_GLES3
  const char* glsl_version = "#version 100";
  //const char* glsl_version = "#version 300 es";
  ImGui_ImplOpenGL3_Init(glsl_version);
#elif defined IGRAPHICS_METAL
  ImGui_ImplMetal_Init(MTLCreateSystemDefaultDevice());
#endif
}

void ImGuiRenderer::Destroy()
{
#if defined IGRAPHICS_GL2
  ImGui_ImplOpenGL2_Shutdown();
#elif defined IGRAPHICS_GL3 || defined IGRAPHICS_GLES2 || defined IGRAPHICS_GLES3
  ImGui_ImplOpenGL3_Shutdown();
#elif defined IGRAPHICS_METAL
  ImGui_ImplMetal_Shutdown();
#endif
}

void ImGuiRenderer::NewFrame()
{
#if defined IGRAPHICS_GL2
  ImGui_ImplOpenGL2_NewFrame();
  this->DoFrame();
  ImGui_ImplOpenGL2_RenderDrawData(ImGui::GetDrawData());
#elif defined IGRAPHICS_GL3 || defined IGRAPHICS_GLES2 || defined IGRAPHICS_GLES3
  ImGui_ImplOpenGL3_NewFrame();
  this->DoFrame();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#else
  //Metal rendering handled in IGRAPHICS_IMGUIVIEW
#endif
}

#include "imgui.cpp"
#include "imgui_widgets.cpp"
#include "imgui_draw.cpp"
#include "imgui_demo.cpp"
#include "imgui_tables.cpp"

#if defined IGRAPHICS_GL2
  #include "imgui_impl_opengl2.cpp"
#elif defined IGRAPHICS_GL3 || defined IGRAPHICS_GLES2 || defined IGRAPHICS_GLES3
  #include "imgui_impl_opengl3.cpp"
//#elif defined IGRAPHICS_METAL
//  #include "imgui_impl_metal.mm"
#endif

#endif //IGRAPHICS_IMGUI
