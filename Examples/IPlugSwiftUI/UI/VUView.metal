//
//  VUView.metal
//  AttenuatorAU
//
//  Created by Vlad Gorlov on 05.04.20.
//  Copyright © 2020 Vlad Gorlov. All rights reserved.
//

#include <metal_stdlib>
using namespace metal;

struct ColoredVertex {
   float4 position [[position]];
   float4 color;
};

vertex ColoredVertex
vertex_line(uint vid [[vertex_id]],
                constant vector_float2 *positions [[buffer(0)]],
                constant vector_float4 *color [[buffer(1)]],
                constant vector_float2 *viewportSizePointer [[buffer(2)]]) {
   
   vector_float2 viewportSize = *viewportSizePointer;
   vector_float2 pixelSpacePosition = positions[vid].xy;
   
   ColoredVertex vert;
   vert.position = vector_float4(0.0, 0.0, 0.0, 1.0);
   vert.position.xy = (pixelSpacePosition / (viewportSize / 2.0)) - 1.0;
   vert.color = *color;
   return vert;
}

fragment float4
fragment_line(ColoredVertex vert [[stage_in]]) {
   return vert.color;
}
