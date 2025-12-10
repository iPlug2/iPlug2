#include <metal_stdlib>
using namespace metal;

struct Uniforms {
  float time;
  float2 resolution;
  float2 mouse;
  float2 mouseButtons;
};

struct VertexOut {
  float4 position [[position]];
  float2 texCoord;
};

vertex VertexOut shaderVertexFunc(uint vertexID [[vertex_id]],
                                   constant float2* vertices [[buffer(0)]]) {
  VertexOut out;
  out.position = float4(vertices[vertexID], 0.0, 1.0);
  out.texCoord = vertices[vertexID] * 0.5 + 0.5;
  return out;
}

fragment half4 shaderFragmentFunc(VertexOut in [[stage_in]],
                                   constant Uniforms& uniforms [[buffer(0)]]) {
  float2 uv = in.texCoord;
  half3 col = half3(uv.x, uv.y, 0.5 + 0.5 * sin(uniforms.time));
  return half4(col, 1.0);
}
