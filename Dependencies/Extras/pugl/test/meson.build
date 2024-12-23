# Copyright 2021-2023 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

# Suppress some additional C warnings in tests
test_c_args = []
if get_option('warning_level') == 'everything'
  if cc.get_id() == 'clang'
    test_c_args += [
      '-Wno-float-equal',
    ]
  elif cc.get_id() == 'gcc'
    test_c_args += [
      '-Wno-float-equal',
    ]
  endif

  test_c_args = cc.get_supported_arguments(test_c_args)
endif

# Suppress some additional C++ warnings in tests
test_cpp_args = []
if get_option('warning_level') == 'everything' and is_variable('cpp')
  if cpp.get_id() == 'clang'
    test_cpp_args += [
      '-Wno-documentation', # Cairo
      '-Wno-documentation-unknown-command', # Cairo
      '-Wno-old-style-cast',
    ]
  endif

  test_cpp_args = cpp.get_supported_arguments(test_cpp_args)
endif

# Check licensing metadata
if not meson.is_subproject() and get_option('lint')
  reuse = find_program('reuse', required: false)
  if reuse.found()
    test('REUSE', reuse, args: ['--root', pugl_src_root, 'lint'], suite: 'data')
  endif
endif

basic_exclusive_tests = [
  'local_copy_paste',
  'remote_copy_paste',
]

basic_tests = [
  'cursor',
  'realize',
  'redisplay',
  'show_hide',
  'size',
  'strerror',
  'stub',
  'stub_hints',
  'timer',
  'update',
  'view',
  'world',
]

cairo_tests = [
  'cairo'
]

gl_tests = [
  'gl',
  'gl_free_unrealized',
  'gl_hints',
]

vulkan_tests = [
  'vulkan'
]

includes = [
  '.',
  '../include',
]

foreach test : basic_tests
  test(
    test,
    executable(
      'test_' + test,
      'test_@0@.c'.format(test),
      c_args: test_c_args,
      dependencies: [pugl_dep, pugl_stub_dep],
      include_directories: include_directories(includes),
    ),
    suite: 'unit',
  )
endforeach

foreach test : basic_exclusive_tests
  test(
    test,
    executable(
      'test_' + test,
      'test_@0@.c'.format(test),
      c_args: test_c_args,
      dependencies: [pugl_dep, pugl_stub_dep],
      include_directories: include_directories(includes),
    ),
    is_parallel: false,
    suite: 'unit',
  )
endforeach

if opengl_dep.found()
  foreach test : gl_tests
    test(
      test,
      executable(
        'test_' + test,
        'test_@0@.c'.format(test),
        c_args: test_c_args,
        dependencies: [pugl_dep, pugl_gl_dep],
        include_directories: include_directories(includes),
      ),
      suite: 'unit',
    )
  endforeach
endif

if cairo_dep.found()
  foreach test : cairo_tests
    test(
      test,
      executable(
        'test_' + test,
        'test_@0@.c'.format(test),
        c_args: test_c_args + cairo_args,
        dependencies: [pugl_dep, pugl_cairo_dep],
        include_directories: include_directories(includes),
      ),
      suite: 'unit',
    )
  endforeach
endif

if vulkan_dep.found()
  foreach test : vulkan_tests
    test(
      test,
      executable(
        'test_' + test,
        'test_@0@.c'.format(test),
        c_args: test_c_args,
        cpp_args: test_cpp_args,
        dependencies: [pugl_dep, pugl_vulkan_dep],
        include_directories: include_directories(includes),
      ),
      suite: 'unit',
    )
  endforeach
endif

unified_args = []
unified_deps = [core_deps]
if cairo_dep.found()
  unified_args += ['-DWITH_CAIRO']
  unified_deps += [cairo_dep]
endif

if opengl_dep.found()
  unified_args += ['-DWITH_OPENGL']
  unified_deps += [opengl_dep]
endif

if vulkan_dep.found()
  unified_args += ['-DWITH_VULKAN']
  unified_deps += [vulkan_deps]
endif

if host_machine.system() == 'darwin'
  add_languages(['objcpp'], native: false)

  objcpp = meson.get_compiler('objcpp')

  objcpp_args = []
  if objcpp.get_id() == 'clang'
    objcpp_args += [
      '-Wno-c++98-compat',
      '-Wno-c++98-compat-pedantic',
      '-Wno-deprecated-declarations',
      '-Wno-direct-ivar-access',
    ]
  endif

  unified_args += objcpp.get_supported_arguments(
    test_cpp_args + objcpp_args)

  test(
    'inline_objcpp',
    executable(
      'test_inline_objcpp',
      'test_inline_objcpp.mm',
      dependencies: unified_deps,
      include_directories: include_directories(includes),
      objcpp_args: unified_args,
    ),
    suite: 'unit',
  )

elif is_variable('cpp')
  unified_args = []

  if cpp.get_id() == 'clang'
    unified_args += [
      '-Wno-old-style-cast',
      '-Wno-switch-default',
      '-Wno-switch-enum',
      '-Wno-unused-macros', # Mac
    ]
  elif cpp.get_id() == 'gcc'
    unified_args += [
      '-Wno-conditionally-supported',
      '-Wno-old-style-cast',
      '-Wno-switch-default',
      '-Wno-switch-enum',
      '-Wno-useless-cast',
    ]
  elif cpp.get_id() == 'msvc'
    unified_args += [
      '/wd4464', # relative include path contains '..'
    ]
  endif

  test(
    'inline_cpp',
    executable(
      'test_inline_cpp',
      'test_inline_cpp.cpp',
      cpp_args: test_cpp_args + unified_args,
      dependencies: unified_deps,
      include_directories: include_directories(includes),
    ),
    suite: 'unit',
  )
endif
