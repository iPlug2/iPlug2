// Copyright 2019-2020 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

/* The fragment shader uses the UV coordinates to calculate whether it is in
   the T, R, B, or L border.  These are then mixed with the border color, and
   their inverse is mixed with the fill color, to calculate the fragment color.
   For example, if we are in the top border, then T=1, so the border mix factor
   TRBL=1, and the fill mix factor (1-TRBL) is 0.

   The use of pixel units here is handy because the border width can be
   specified precisely in pixels to draw sharp lines.  The border width is just
   hardcoded, but could be made a uniform or vertex attribute easily enough. */

precision mediump float;

INTER(location = 0) NOPERSPECTIVE in vec2 f_uv;
INTER(location = 1) NOPERSPECTIVE in vec2 f_size;
INTER(location = 2) NOPERSPECTIVE in vec4 f_fillColor;

layout(location = 0) out vec4 FragColor;

void
main()
{
  const float borderWidth = 2.0;

  vec4  borderColor = f_fillColor + vec4(0.0, 0.4, 0.4, 0.0);
  float t           = step(borderWidth, f_uv[1]);
  float r           = step(borderWidth, f_size.x - f_uv[0]);
  float b           = step(borderWidth, f_size.y - f_uv[1]);
  float l           = step(borderWidth, f_uv[0]);
  float fillMix     = t * r * b * l;
  float borderMix   = 1.0 - fillMix;
  vec4  fill        = fillMix * f_fillColor;
  vec4  border      = borderMix * borderColor;

  FragColor = fill + border;
}
