// Copyright 2019-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/* The vertex shader is trivial, but forwards scaled UV coordinates (in pixels)
   to the fragment shader for drawing the border. */

precision mediump float;

UBO(binding = 0) uniform UniformBufferObject
{
  mat4 projection;
}
ubo;

layout(location = 0) in vec2 v_position;
layout(location = 1) in vec2 v_origin;
layout(location = 2) in vec2 v_size;
layout(location = 3) in vec4 v_fillColor;

INTER(location = 0) NOPERSPECTIVE out vec2 f_uv;
INTER(location = 1) NOPERSPECTIVE out vec2 f_size;
INTER(location = 2) NOPERSPECTIVE out vec4 f_fillColor;

void
main()
{
  // clang-format off
  mat4 m = mat4(v_size[0],   0.0,         0.0, 0.0,
                0.0,         v_size[1],   0.0, 0.0,
                0.0,         0.0,         1.0, 0.0,
                v_origin[0], v_origin[1], 0.0, 1.0);
  // clang-format on

  mat4 MVP = ubo.projection * m;

  f_uv        = v_position * v_size;
  f_size      = v_size;
  f_fillColor = v_fillColor;

  gl_Position = MVP * vec4(v_position, 0.0, 1.0);
}
