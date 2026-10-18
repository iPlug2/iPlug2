# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

shader_files = [
  'header_320_es.glsl',
  'header_330.glsl',
  'header_420.glsl',
  'rect.frag',
  'rect.vert',
]

# Copy shader sources for GL examples
foreach shader_file : shader_files
  configure_file(copy: true, input: shader_file, output: shader_file)
endforeach

# Build SPV shader binaries for Vulkan examples
if vulkan_dep.found()
  cat = find_program('../../scripts/cat.py')
  glslang = find_program('glslangValidator')

  rect_vulkan_vert = custom_target(
    'rect.vulkan.vert',
    build_by_default: true,
    capture: true,
    command: [cat, '@INPUT@'],
    input: ['header_420.glsl', 'rect.vert'],
    output: 'rect.vulkan.vert',
  )

  rect_vulkan_frag = custom_target(
    'rect.vulkan.frag',
    build_by_default: true,
    capture: true,
    command: [cat, '@INPUT@'],
    input: ['header_420.glsl', 'rect.frag'],
    output: 'rect.vulkan.frag',
  )

  glslang_command = [glslang, '-V', '-o', '@OUTPUT@', '@INPUT@']

  rect_vert_spv = custom_target(
    'rect.vert.spv',
    build_by_default: true,
    command: glslang_command,
    input: rect_vulkan_vert,
    install: false,
    output: 'rect.vert.spv',
  )

  rect_frag_spv = custom_target(
    'rect.frag.spv',
    build_by_default: true,
    command: glslang_command,
    input: rect_vulkan_frag,
    install: false,
    output: 'rect.frag.spv',
  )
endif
