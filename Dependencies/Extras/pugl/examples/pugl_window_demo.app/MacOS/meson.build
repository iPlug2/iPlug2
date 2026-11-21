# Copyright 2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

executable(
  'pugl_window_demo',
  '../../pugl_window_demo.c',
  c_args: example_defines + example_c_args,
  cpp_args: example_defines + example_cpp_args,
  dependencies: [pugl_dep, pugl_gl_dep],
  include_directories: include_directories('../../..'),
)
