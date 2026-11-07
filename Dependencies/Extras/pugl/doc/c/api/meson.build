# Copyright 2021 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

c_pugl_rst = custom_target(
  'C API ReST Documentation',
  command: [sphinxygen, '-f', '@INPUT0@', 'doc/c/api'],
  input: [c_index_xml] + c_rst_files,
  output: 'pugl.rst',
)
