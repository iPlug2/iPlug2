# Copyright 2021-2023 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

c_headers = files(
  'pugl/attributes.h',
  'pugl/pugl.h',

  'pugl/cairo.h',
  'pugl/gl.h',
  'pugl/glu.h',
  'pugl/stub.h',
  'pugl/vulkan.h',
)

install_headers(c_headers, subdir: versioned_name / 'pugl')
