# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

shaders = [
  'header_320_es.glsl',
  'header_330.glsl',
  'header_420.glsl',
  'rect.vert',
  'rect.frag',
]

foreach shader : shaders
  configure_file(
    copy: true,
    input: files('..' / '..' / 'shaders' / shader),
    output: shader,
  )
endforeach
