#include <metal_stdlib>
using namespace metal;

// Simple vertex structure
struct VertexIn {
    float3 position [[attribute(0)]];
    float2 texcoord [[attribute(1)]];
};

// Vertex output structure
struct VertexOut {
    float4 position [[position]];
    float2 texcoord;
};

// A basic passthrough vertex shader.
vertex VertexOut vertexMain(VertexIn in [[stage_in]]) {
    VertexOut out;
    // Convert 3D position to 4D homogeneous coordinate.
    out.position = float4(in.position, 1.0);
    out.texcoord = in.texcoord;
    return out;
}

// Fragment shader that creates a checkerboard pattern.
fragment float4 fragmentMain(VertexOut in [[stage_in]])
{
    // Normalize texture coordinates (should already be in [0,1])
    float2 uv = in.texcoord;

    // Define how many checkers you want along one axis.
    // Adjust this value to change the size of the checkers.
    float scale = 10.0;
    
    // Scale the UV coordinates to the desired grid size.
    uv *= scale;
    
    // Determine which cell we are in by taking the floor.
    int cellX = int(floor(uv.x));
    int cellY = int(floor(uv.y));
    
    // Alternate colors: if the sum of the cell indices is even, choose one color; if odd, the other.
    bool isEven = ((cellX + cellY) % 2) == 0;
    
    // Define colors for the checkerboard.
    // Here we use white for even cells and black for odd cells.
    float3 color = isEven ? float3(1.0, 1.0, 1.0) : float3(0.0, 0.0, 0.0);
    
    return float4(color, 1.0);
}
