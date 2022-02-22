#include <metal_stdlib>

using namespace metal;

#include "ShaderTypes.h"

// Vertex shader outputs and fragment shader inputs for simple pipeline
struct SimplePipelineRasterizerData
{
  float4 position [[position]];
  float4 color;
};

// Vertex shader which passes position and color through to rasterizer.
vertex SimplePipelineRasterizerData
simpleVertexShader(const uint vertexID [[ vertex_id ]],
                   const device SimpleVertex *vertices [[ buffer(VertexInputIndexVertices) ]])
{
  SimplePipelineRasterizerData out;
  
  out.position = vector_float4(0.0, 0.0, 0.0, 1.0);
  out.position.xy = vertices[vertexID].position.xy;
  
  out.color = vertices[vertexID].color;
  
  return out;
}

// Fragment shader that just outputs color passed from rasterizer.
fragment float4 simpleFragmentShader(SimplePipelineRasterizerData in [[stage_in]])
{
  return in.color;
}
