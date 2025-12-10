#version 450

// Standard iPlug2 shader uniforms
layout(binding = 0) uniform Uniforms {
  float uTime;
  vec2 uResolution;
  vec2 uMouse;
  vec2 uMouseButtons;
};

layout(location = 0) out vec4 FragColor;

void main() {
  vec2 uv = gl_FragCoord.xy / uResolution;
  FragColor = vec4(uv.x, uv.y, sin(uTime) * 0.5 + 0.5, 1.0);
}
