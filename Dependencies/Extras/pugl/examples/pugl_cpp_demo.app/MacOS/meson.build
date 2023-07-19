# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

executable(
  'pugl_cpp_demo',
  '../../pugl_cpp_demo.cpp',
  c_args: example_defines + example_c_args,
  cpp_args: example_defines + example_cpp_args,
  dependencies: [puglpp_dep, pugl_gl_dep],
  include_directories: include_directories('../../..'),
)
