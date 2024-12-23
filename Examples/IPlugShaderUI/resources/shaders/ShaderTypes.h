#ifndef ShaderTypes_h
#define ShaderTypes_h

#include <simd/simd.h>

typedef enum VertexInputIndex
{
  VertexInputIndexVertices = 0,
  VertexInputIndexAspectRatio = 1,
} VertexInputIndex;

typedef enum TextureInputIndex
{
  TextureInputIndexColor = 0,
} TextureInputIndex;

typedef struct
{
  vector_float2 position;
  vector_float4 color;
} SimpleVertex;

typedef struct
{
  vector_float2 position;
  vector_float2 texcoord;
} TextureVertex;

#endif /* ShaderTypes_h */
