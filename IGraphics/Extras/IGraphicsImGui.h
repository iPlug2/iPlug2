#pragma once

#if defined IGRAPHICS_IMGUI
#include "imgui.h"
#include "IGraphicsStructs.h"

BEGIN_IPLUG_NAMESPACE
BEGIN_IGRAPHICS_NAMESPACE

class IGraphics;

class ImGuiRenderer
{
public:
  ImGuiRenderer(IGraphics* pGraphics, std::function<void(IGraphics*)> drawFunc, std::function<void()> setupFunc);
  ~ImGuiRenderer();
  
  /** Initialise ImGui backend */
  void Init();
  
  /** Destroy ImGui backend */
  void Destroy();
  
  /** Per Frame ImGui backend-stuff */
  void NewFrame();
  
  /** Frame processing that is the same across platforms */
  void DoFrame();
  
  bool OnMouseDown(float x, float y, const IMouseMod &mod);
  bool OnMouseUp(float x, float y, const IMouseMod &mod);
  bool OnMouseWheel(float x, float y, const IMouseMod &mod, float delta);
  void OnMouseMove(float x, float y, const IMouseMod &mod);
  bool OnKeyDown(float x, float y, const IKeyPress &key);
  bool OnKeyUp(float x, float y, const IKeyPress &key);

  std::function<void(IGraphics*)> GetDrawFunc()
  {
    return mDrawFunc;
  }
  
private:
  IGraphics* mGraphics;
  std::function<void(IGraphics*)> mDrawFunc = nullptr;
  friend IGraphics;
};

END_IGRAPHICS_NAMESPACE
END_IPLUG_NAMESPACE

#endif
