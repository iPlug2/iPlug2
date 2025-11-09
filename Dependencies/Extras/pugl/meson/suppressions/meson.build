# Copyright 2020-2023 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

# Project-specific warning suppressions

warning_level = get_option('warning_level')

#####
# C #
#####

c_suppressions = []
if cc.get_id() == 'clang'
  if warning_level == 'everything'
    c_suppressions += [
      '-Wno-bad-function-cast',
      '-Wno-declaration-after-statement',
      '-Wno-padded',
      '-Wno-switch-default',
      '-Wno-switch-enum',
    ]

    if not meson.is_cross_build()
      c_suppressions += [
        '-Wno-poison-system-directories',
      ]
    endif

    if host_machine.system() == 'windows'
      c_suppressions += [
        '-Wno-deprecated-declarations',
        '-Wno-format-nonliteral',
        '-Wno-nonportable-system-include-path',
        '-Wno-unused-macros',
      ]
    endif
  endif

elif cc.get_id() == 'gcc'
  if warning_level == 'everything'
    c_suppressions += [
      '-Wno-bad-function-cast',
      '-Wno-inline',
      '-Wno-padded',
      '-Wno-suggest-attribute=const',
      '-Wno-suggest-attribute=pure',
      '-Wno-switch-default',
      '-Wno-switch-enum',
      '-Wno-unsuffixed-float-constants',
    ]

    if host_machine.system() == 'windows'
      c_suppressions += [
        '-Wno-cast-function-type',
        '-Wno-float-equal',
        '-Wno-suggest-attribute=format',
      ]
    endif
  endif

  if warning_level in ['everything', '3']
    c_suppressions += [
      '-Wno-pedantic',
    ]
  endif

elif cc.get_id() == 'msvc'
  c_suppressions += [
    '/experimental:external',
    '/external:W0',
    '/external:anglebrackets',
  ]

  if warning_level == 'everything'
    c_suppressions += [
      '/wd4061', # enumerator in switch is not explicitly handled
      '/wd4090', # different const qualitifers
      '/wd4191', # unsafe conversion from type to type
      '/wd4514', # unreferenced inline function has been removed
      '/wd4710', # function not inlined
      '/wd4711', # function selected for automatic inline expansion
      '/wd4820', # padding added after construct
      '/wd5045', # will insert Spectre mitigation for memory load
      '/wd5246', # subobject initialization should be wrapped in braces
    ]
  endif

  if warning_level in ['everything', '3']
    c_suppressions += [
      '/wd4100', # unreferenced formal parameter
      '/wd4706', # assignment within conditional expression
    ]
  endif

  if warning_level in ['everything', '3', '2']
    c_suppressions += [
      '/wd4996', # POSIX name for this item is deprecated
    ]
  endif
endif

add_project_arguments(
  cc.get_supported_arguments(c_suppressions),
  language: ['c'],
)

#######
# C++ #
#######

if is_variable('cpp')
  cpp_suppressions = []

  if cpp.get_id() == 'clang'
    if warning_level == 'everything'
      cpp_suppressions += [
        '-Wno-c++98-compat-pedantic',
        '-Wno-inline',
        '-Wno-padded',
        '-Wno-sign-conversion',
      ]

      if not meson.is_cross_build()
        cpp_suppressions += [
          '-Wno-poison-system-directories',
        ]
      endif
    endif

  elif cpp.get_id() == 'gcc'
    if warning_level == 'everything'
      cpp_suppressions += [
        '-Wno-inline',
        '-Wno-padded',
      ]

      if host_machine.system() == 'windows'
        cpp_suppressions += [
          '-Wno-cast-function-type',
          '-Wno-suggest-attribute=format',
        ]
      endif
    endif

  elif cpp.get_id() == 'msvc'
    cpp_suppressions += [
      '/experimental:external',
      '/external:W0',
      '/external:anglebrackets',
    ]

    if warning_level == 'everything'
      cpp_suppressions += [
        '/wd4061', # enumerator in switch is not explicitly handled
        '/wd4191', # unsafe conversion from type to type
        '/wd4514', # unreferenced inline function has been removed
        '/wd4625', # copy constructor implicitly deleted
        '/wd4626', # copy assignment operator implicitly deleted
        '/wd4710', # function not inlined
        '/wd4711', # function selected for automatic inline expansion
        '/wd4800', # implicit conversion to bool
        '/wd4820', # padding added after construct
        '/wd5026', # move constructor implicitly deleted
        '/wd5027', # move assignment operator implicitly deleted
        '/wd5039', # pointer to potentially throwing function passed to C
        '/wd5045', # will insert Spectre mitigation for memory load
      ]
    endif

    if warning_level in ['everything', '3']
      cpp_suppressions += [
        '/wd4706', # assignment within conditional expression
      ]
    endif

    if warning_level in ['everything', '3', '2']
      cpp_suppressions += [
        '/wd4996', # POSIX name for this item is deprecated
      ]
    endif
  endif

  add_project_arguments(
    cpp.get_supported_arguments(cpp_suppressions),
    language: ['cpp'],
  )
endif

###############
# Objective C #
###############

if is_variable('objcc')
  if warning_level == 'everything'
    objc_suppressions = [
      '-Wno-bad-function-cast',
      '-Wno-direct-ivar-access',
      '-Wno-objc-messaging-id',
      '-Wno-padded',
      '-Wno-pedantic',
      '-Wno-poison-system-directories',
      '-Wno-switch-enum',
      '-Wno-undeclared-selector',
    ]

    objc_suppressions = objcc.get_supported_arguments(objc_suppressions)
    add_project_arguments(objc_suppressions, language: ['objc'])
    add_project_arguments(objc_suppressions, language: ['objcpp'])
  endif
endif
