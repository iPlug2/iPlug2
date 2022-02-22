#if defined IGRAPHICS_NANOVG && defined IGRAPHICS_METAL

#include "TestMPSControl.h"
#include "nanovg_mtl.h"
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

// https://developer.apple.com/documentation/metalperformanceshaders?language=objc
    
void TestMPSControl::Draw(IGraphics& g)
{
  if (@available(macOS 10.13, *))
  {
    auto* pCtx = static_cast<NVGcontext*>(g.GetDrawContext());

    if (!mFBO) {
      mFBO = nvgCreateFramebuffer(pCtx, mBitmap.W() * mBitmap.GetScale(), mBitmap.H() * mBitmap.GetScale(), 0);
    }

    auto dev = static_cast<id<MTLDevice>>(mnvgDevice(pCtx));
    auto commandQueue = static_cast<id<MTLCommandQueue>>(mnvgCommandQueue(pCtx));
    auto srcTex = static_cast<id<MTLTexture>>(mnvgImageHandle(pCtx, mBitmap.GetAPIBitmap()->GetBitmap()));
    auto dstTex = static_cast<id<MTLTexture>>(mnvgImageHandle(pCtx, mFBO->image));
    id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
  
    MPSUnaryImageKernel* pKernel = nullptr;
  
    switch (mKernelType) {
      case 0: pKernel = [[MPSImageGaussianBlur alloc] initWithDevice:dev sigma:GetValue() * 10.]; break;
      case 1: pKernel = [[MPSImageSobel alloc] initWithDevice:dev]; break;
      case 2: pKernel = [[MPSImageThresholdToZero alloc] initWithDevice:dev thresholdValue:GetValue() linearGrayColorTransform:nil]; break;
      default: break;
    }
  
    [pKernel encodeToCommandBuffer:commandBuffer sourceTexture:srcTex destinationTexture:dstTex];
    [commandBuffer commit];
    [commandBuffer waitUntilCompleted];
  
    [pKernel release];
    
    APIBitmap apibmp {mFBO->image, int(mBitmap.W() * mBitmap.GetScale()), int(mBitmap.H() * mBitmap.GetScale()), 1, 1.};
    IBitmap bmp {&apibmp, 1, false};
    
    g.DrawFittedBitmap(bmp, mRECT);
  } // macOS 10.13
}

#endif // IGRAPHICS_METAL
