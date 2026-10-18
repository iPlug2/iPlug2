# Copyright 2021-2023 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

executable(
  'pugl_management_demo',
  ['../../pugl_management_demo.c'],
  c_args: example_defines + example_c_args + cairo_args,
  cpp_args: example_defines + example_cpp_args,
  dependencies: [pugl_dep, pugl_cairo_dep],
  include_directories: include_directories('../../..'),
)
