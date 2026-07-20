# Copyright 2021 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

cpp_headers = files(
  'pugl/pugl.hpp',

  'pugl/cairo.hpp',
  'pugl/gl.hpp',
  'pugl/stub.hpp',
  'pugl/vulkan.hpp',
)

install_headers(cpp_headers, subdir: 'puglpp' + version_suffix / 'pugl')
