#version 330 core

uniform float radius;

in vec2 uv;
out vec4 fragColor;

void main() {
    vec2 uv_centered = uv * 2.0 - 1.0; // Convert back to [-1,1] range
    float distance = length(uv_centered) - radius;
    
    float circle = 1.0 - smoothstep(0.0, 0.01, distance);
    
    // Output white circle with alpha mask
    fragColor = vec4(1.0, 1.0, 1.0, circle);
} 
