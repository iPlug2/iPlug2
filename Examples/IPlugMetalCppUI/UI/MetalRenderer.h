#pragma once

namespace MTK {
class View;
class ViewDelegate;
}

namespace MTL {
class Device;
class CommandQueue;
class RenderPipelineState;
class Buffer;
class Texture;
class SamplerState;
}

namespace NS {
class AutoreleasePool;
}

class CustomMTKViewDelegate;

class Renderer
{
public:
  Renderer(MTL::Device* pDevice, const char* bundleID);
  ~Renderer();
  void buildShaders(const char* bundleID);
  void buildBuffers();
  void draw(MTK::View* pView);

private:
  MTL::Device* mpDevice;
  MTL::CommandQueue* mpCommandQueue;
  MTL::RenderPipelineState* mpPSO;
  MTL::Buffer* mpVertexPositionsBuffer;
  MTL::Buffer* mpVertexColorsBuffer;
  MTL::Buffer* mpUniformsBuffer;
  MTL::Texture* mpTexture;
  MTL::SamplerState* mpSampler;
};

class MetalUI
{
public:
  void* OpenWindow(void* pParentView, const char* bundleID);
  void setSize(int width, int height);
  ~MetalUI();

  MTK::View* mpMtkView;
  MTL::Device* mpDevice;
  CustomMTKViewDelegate* mpViewDelegate = nullptr;
  NS::AutoreleasePool* mpAutoreleasePool;
};
