#include <metal_stdlib>

using namespace metal;

#include "ShaderTypes.h"

struct SimplePipelineRasterizerData
{
  float4 position [[position]];
  float2 uv;
};

vertex SimplePipelineRasterizerData
simpleVertexShader(uint vertexID [[vertex_id]])
{
  SimplePipelineRasterizerData out;
  
  float2 positions[4] = {
    float2(-1, -1),
    float2( 1, -1),
    float2(-1,  1),
    float2( 1,  1)
  };
  
  out.position = float4(positions[vertexID], 0, 1);
  out.uv = (positions[vertexID] + 1.0) * 0.5;
  
  return out;
}

fragment float4 simpleFragmentShader(SimplePipelineRasterizerData in [[stage_in]],
                   constant float &radius [[buffer(0)]])
{
  float2 uv_centered = in.uv * 2.0 - 1.0;
  float distance = length(uv_centered) - radius;
  
  float circle = 1.0 - smoothstep(0.0, 0.01, distance);
  
  // Output white circle with alpha mask
  return float4(1.0, 1.0, 1.0, circle);
}
