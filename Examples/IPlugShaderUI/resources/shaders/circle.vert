#version 330 core

layout(location = 0) in vec2 apos;

out vec2 uv;

void main() {
    gl_Position = vec4(apos, 0.0, 1.0);
    uv = (apos + 1.0) * 0.5; // Calculate UVs from position
}
