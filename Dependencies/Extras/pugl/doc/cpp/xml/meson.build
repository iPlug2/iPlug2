# Copyright 2021 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

config = configuration_data()
config.set('PUGL_SRCDIR', pugl_src_root)
config.set('DOX_OUTPUT', meson.current_build_dir() / '..')

cpp_doxyfile = configure_file(
  configuration: config,
  input: '../Doxyfile.in',
  output: 'Doxyfile',
)

cpp_index_xml = custom_target(
  'cpp-index.xml',
  command: [doxygen, '@INPUT0@'],
  input: [cpp_doxyfile] + c_headers + cpp_headers,
  output: 'index.xml',
)

