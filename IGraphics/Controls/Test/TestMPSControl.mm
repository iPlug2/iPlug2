#ifdef IGRAPHICS_METAL

#include "TestMPSControl.h"
#include "nanovg_mtl.h"
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>

// https://developer.apple.com/documentation/metalperformanceshaders?language=objc
    
void TestMPSControl::Draw(IGraphics& g)
{
  if (@available(macOS 10.13, *))
  {
    if (!g.CheckLayer(mLayer))
    {
      NVGcontext* pCtx = static_cast<NVGcontext*>(g.GetDrawContext());
      id<MTLDevice> dev = static_cast<id<MTLDevice>>(mnvgDevice(pCtx));
      id<MTLCommandQueue> commandQueue = static_cast<id<MTLCommandQueue>>(mnvgCommandQueue(pCtx));
      id<MTLTexture> srcTex = static_cast<id<MTLTexture>>(mnvgImageHandle(pCtx, mBitmap.GetAPIBitmap()->GetBitmap()));
      
      g.StartLayer(IRECT(0,0, mBitmap));
      mLayer = g.EndLayer();
      
      id<MTLTexture> dstTex = static_cast<id<MTLTexture>>(mnvgImageHandle(pCtx, mLayer->GetAPIBitmap()->GetBitmap()));
      id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
      
      MPSUnaryImageKernel* pKernel = nullptr;
      
      switch (mKernelType) {
        case 0: pKernel = [[MPSImageGaussianBlur alloc] initWithDevice:dev sigma:mValue * 10.]; break;
        case 1: pKernel = [[MPSImageSobel alloc] initWithDevice:dev]; break;
        case 2: pKernel = [[MPSImageThresholdToZero alloc] initWithDevice:dev thresholdValue:mValue linearGrayColorTransform:nil]; break;
        default: break;
      }
      
      [pKernel encodeToCommandBuffer:commandBuffer sourceTexture:srcTex destinationTexture:dstTex];
      [commandBuffer commit];
      [commandBuffer waitUntilCompleted];
      
      [pKernel release];
    }
    
    g.DrawFittedLayer(mLayer, mRECT, &mBlend);
    
  } // macOS 10.13
}

#endif // IGRAPHICS_METAL
