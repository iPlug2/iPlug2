# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

executable(
  'pugl_vulkan_demo',
  [
    '../../pugl_vulkan_demo.c',
    '../../file_utils.c',
  ],
  c_args: example_defines + example_c_args,
  dependencies: [pugl_dep, pugl_vulkan_dep],
  include_directories: include_directories('../../..'),
)
