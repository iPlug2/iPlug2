# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

config = configuration_data()
config.set('NAME', 'pugl_vulkan_cpp_demo')

info_plist = configure_file(
  configuration: config,
  input: files('../../resources/Info.plist.in'),
  output: 'Info.plist',
)

shaders = [
  'rect.frag.spv',
  'rect.vert.spv',
]

cp = find_program('../../scripts/cp.py')

custom_target(
  'rect.vert.spv',
  build_by_default: true,
  command: [cp, '@INPUT@', '@OUTPUT@'],
  input: rect_vert_spv,
  output: 'rect.vert.spv',
)

custom_target(
  'rect.frag.spv',
  build_by_default: true,
  command: [cp, '@INPUT@', '@OUTPUT@'],
  input: rect_frag_spv,
  output: 'rect.frag.spv',
)

subdir('MacOS')
