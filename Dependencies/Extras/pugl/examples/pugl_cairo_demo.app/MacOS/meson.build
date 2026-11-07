# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

executable(
  'pugl_cairo_demo',
  ['../../pugl_cairo_demo.c'],
  c_args: example_defines + example_c_args + cairo_args,
  cpp_args: example_defines + example_cpp_args,
  dependencies: [pugl_dep, pugl_cairo_dep],
  include_directories: include_directories('../../..'),
)
