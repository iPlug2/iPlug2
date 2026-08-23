# Copyright 2021-2023 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

project(
  'pugl',
  ['c'],
  default_options: [
    'b_ndebug=if-release',
    'buildtype=release',
    'c_std=c99',
    'cpp_std=c++14',
  ],
  license: 'ISC',
  meson_version: '>= 0.54.0',
  version: '0.5.3',
)

pugl_src_root = meson.current_source_dir()
major_version = meson.project_version().split('.')[0]
version_suffix = '-@0@'.format(major_version)
versioned_name = 'pugl' + version_suffix

#######################
# Compilers and Flags #
#######################

# Required tools
pkg = import('pkgconfig')
cc = meson.get_compiler('c')

# Enable C++ support if we're building the examples
if not get_option('examples').disabled() or not get_option('tests').disabled()
  if add_languages(['cpp'], native: false, required: false)
    cpp = meson.get_compiler('cpp')
  endif
endif

# Enable Objective C support if we're building for MacOS
if host_machine.system() == 'darwin'
  add_languages(['objc', 'objcpp'], native: false)
  objcc = meson.get_compiler('objc')
endif

# Set global warning suppressions
subdir('meson/suppressions')

# Disable deprecated API which is not used by tests or examples
add_project_arguments(
  ['-DPUGL_DISABLE_DEPRECATED'],
  language: ['c', 'cpp', 'objc', 'objcpp'],
)

#############
# Platforms #
#############

core_args = []
dl_dep = cc.find_library('dl', required: false)

# MacOS
if host_machine.system() == 'darwin'
  framework_deps = dependency(
    'appleframeworks',
    modules: ['foundation', 'corevideo'],
    required: false,
  )

  platform = 'mac'
  platform_sources = files('src/mac.m')
  core_deps = [framework_deps]
  extension = '.m'
  soversion = meson.project_version().split('.')[0]

  platform_suppressions = [
    '-DGL_SILENCE_DEPRECATION',
    '-Wno-deprecated-declarations',
  ]

  mac_languages = ['c', 'objc', 'objcpp']
  add_project_arguments(platform_suppressions, language: mac_languages)
  add_project_link_arguments(['-Wl,-framework,Cocoa'], language: mac_languages)

# Windows
elif host_machine.system() == 'windows'
  if get_option('default_library') == 'both'
    error('default_library=both is not supported on Windows')
  endif

  if cc.get_id() == 'msvc'
    msvc_args = [
      '/experimental:external',
      '/external:W0',
      '/external:anglebrackets',
    ]

    add_project_arguments(msvc_args, language: ['c', 'cpp'])
  endif

  win_args = [
    # Set minimum target version
    '-DWINVER=0x0500', # Windows 2000
    '-D_WIN32_WINNT=0x0500', # Windows 2000

    # Disable as many things from windows.h as possible
    '-DWIN32_LEAN_AND_MEAN',
    '-DNOGDICAPMASKS', # CC_*, LC_*, PC_*, CP_*, TC_*, RC_
    '-DNOSYSMETRICS', # SM_*
    '-DNOKEYSTATES', # MK_*
    '-DOEMRESOURCE', # OEM Resource values
    '-DNOATOM', # Atom Manager routines
    '-DNOCOLOR', # Screen colors
    '-DNODRAWTEXT', # DrawText() and DT_*
    '-DNOKERNEL', # All KERNEL defines and routines
    '-DNOMB', # MB_* and MessageBox()
    '-DNOMEMMGR', # GMEM_*, LMEM_*, GHND, LHND, associated routines
    '-DNOMETAFILE', # typedef METAFILEPICT
    '-DNOMINMAX', # Macros min(a,b) and max(a,b)
    '-DNOOPENFILE', # OpenFile(), OemToAnsi, AnsiToOem, and OF_*
    '-DNOSCROLL', # SB_* and scrolling routines
    '-DNOSERVICE', # All Service Controller routines, SERVICE_ equates, etc.
    '-DNOSOUND', # Sound driver routines
    '-DNOWH', # SetWindowsHook and WH_*
    '-DNOCOMM', # COMM driver routines
    '-DNOKANJI', # Kanji support stuff
    '-DNOHELP', # Help engine interface
    '-DNOPROFILER', # Profiler interface
    '-DNODEFERWINDOWPOS', # DeferWindowPos routines
    '-DNOMCX', # Modem Configuration Extensions

    # Needed only for Cairo
    # '-DNORASTEROPS', # Binary and Tertiary raster ops

    # Needed by Pugl
    # '-DNOVIRTUALKEYCODES', # VK_*
    # '-DNOWINMESSAGES', # WM_*, EM_*, LB_*, CB_*
    # '-DNOWINSTYLES', # WS_*, CS_*, ES_*, LBS_*, SBS_*, CBS_*
    # '-DNOMENUS', # MF_*
    # '-DNOICONS', # IDI_*
    # '-DNOSYSCOMMANDS', # SC_*
    # '-DNOSHOWWINDOW', # SW_*
    # '-DNOCLIPBOARD', # Clipboard routines
    # '-DNOCTLMGR', # Control and Dialog routines
    # '-DNOGDI', # All GDI defines and routines
    # '-DNOUSER', # All USER defines and routines
    # '-DNONLS', # All NLS defines and routines
    # '-DNOMSG', # typedef MSG and associated routines
    # '-DNOWINOFFSETS', # GWL_*, GCL_*, associated routines
  ]

  if cc.get_id() == 'msvc'
    win_args += [
      '-DNOTEXTMETRIC', # typedef TEXTMETRIC and associated routines
    ]
  endif

  add_project_arguments(win_args, language: ['c', 'cpp'])

  user32_dep = cc.find_library('user32')
  shlwapi_dep = cc.find_library('shlwapi')
  dwmapi_dep = cc.find_library('dwmapi')

  platform = 'win'
  platform_sources = files('src/win.c')
  core_deps = [user32_dep, shlwapi_dep, dwmapi_dep]
  extension = '.c'
  soversion = ''

else # X11
  x11_dep = cc.find_library('X11')
  core_args += '-D_POSIX_C_SOURCE=200809L'

  xcursor_dep = cc.find_library('Xcursor', required: get_option('xcursor'))
  if xcursor_dep.found()
    core_args += ['-DUSE_XCURSOR=1']
  endif

  xrandr_dep = cc.find_library('Xrandr', required: get_option('xrandr'))
  if xrandr_dep.found()
    core_args += ['-DUSE_XRANDR=1']
  endif

  use_xsync = false
  xext_dep = cc.find_library('Xext', required: get_option('xsync'))
  if xext_dep.found()
    xsync_fragment = '''#include <X11/Xlib.h>
      #include <X11/extensions/sync.h>
      int main(void) { XSyncQueryExtension(0, 0, 0); return 0; }'''
    if cc.compiles(xsync_fragment, name: 'Xsync')
      core_args += ['-DUSE_XSYNC=1']
      use_xsync = true
    elif get_option('xsync') == 'enabled'
      error('C header <X11/extensions/sync.h> not found')
    endif
  endif

  x11_args = []
  if cc.get_id() == 'clang'
    x11_args += [
      '-Wno-reserved-identifier', # FD_ZERO
    ]
  endif

  core_args += cc.get_supported_arguments(x11_args)

  platform = 'x11'
  platform_sources = files('src/x11.c')
  core_deps = [x11_dep, xcursor_dep, xrandr_dep, xext_dep]
  extension = '.c'
  soversion = meson.project_version().split('.')[0]
endif

################
# Core Library #
################

core_deps += [cc.find_library('m', required: false)]
core_name = 'pugl_@0@@1@'.format(platform, version_suffix)
includes = include_directories(['include'])
library_args = ['-DPUGL_INTERNAL']
extra_c_args = []

if get_option('default_library') == 'static'
  extra_c_args += ['-DPUGL_STATIC']
  add_project_arguments(
    extra_c_args,
    language: ['c', 'cpp', 'objc', 'objcpp'],
  )
endif

common_sources = files(
  'src/common.c',
  'src/internal.c',
)

libpugl = library(
  core_name,
  common_sources + platform_sources,
  c_args: library_args + core_args,
  dependencies: core_deps,
  gnu_symbol_visibility: 'hidden',
  include_directories: includes,
  install: true,
  soversion: soversion,
  version: meson.project_version(),
)

pugl_dep = declare_dependency(
  compile_args: extra_c_args,
  dependencies: core_deps,
  include_directories: ['include'],
  link_with: libpugl,
)

pkg.generate(
  libpugl,
  description: 'Pugl GUI library core',
  filebase: versioned_name,
  name: 'Pugl',
  subdirs: [versioned_name],
  version: meson.project_version(),
)

meson.override_dependency(versioned_name, pugl_dep)

############
# Backends #
############

# Stub
if get_option('stub')
  name = 'pugl_' + platform + '_stub' + version_suffix
  sources = files('src/' + platform + '_stub' + extension)

  stub_backend = library(
    name,
    sources,
    c_args: library_args,
    dependencies: [pugl_dep],
    gnu_symbol_visibility: 'hidden',
    include_directories: includes,
    install: true,
    soversion: soversion,
    version: meson.project_version(),
  )

  pugl_stub_dep = declare_dependency(
    compile_args: extra_c_args,
    link_with: stub_backend,
    dependencies: [pugl_dep],
  )

  stub_pkg_name = 'pugl-stub-@0@'.format(major_version)

  pkg.generate(
    stub_backend,
    description: 'Native window pugl graphics backend',
    filebase: stub_pkg_name,
    name: 'Pugl Stub',
    subdirs: [versioned_name],
    version: meson.project_version(),
  )

  meson.override_dependency(stub_pkg_name, pugl_stub_dep)
endif

# OpenGL
opengl_dep = dependency(
  'GL',
  include_type: 'system',
  required: get_option('opengl'),
)
if opengl_dep.found()
  name = 'pugl_' + platform + '_gl' + version_suffix
  sources = files('src/' + platform + '_gl' + extension)

  gl_backend = library(
    name,
    sources,
    c_args: library_args,
    dependencies: [pugl_dep, opengl_dep],
    gnu_symbol_visibility: 'hidden',
    include_directories: includes,
    install: true,
    soversion: soversion,
    version: meson.project_version(),
  )

  pugl_gl_dep = declare_dependency(
    compile_args: extra_c_args,
    link_with: gl_backend,
    dependencies: [pugl_dep, opengl_dep],
  )

  gl_pkg_name = 'pugl-gl-@0@'.format(major_version)

  pkg.generate(
    gl_backend,
    description: 'Pugl GUI library with OpenGL backend',
    filebase: gl_pkg_name,
    name: 'Pugl OpenGL',
    subdirs: [versioned_name],
    version: meson.project_version(),
  )

  meson.override_dependency(gl_pkg_name, pugl_gl_dep)
endif

# Cairo
cairo_dep = dependency(
  'cairo',
  include_type: 'system',
  required: get_option('cairo'),
)
if cairo_dep.found()
  name = 'pugl_' + platform + '_cairo' + version_suffix
  sources = files('src/' + platform + '_cairo' + extension)

  cairo_args = []
  if cc.get_id() == 'clang'
    cairo_args += [
      '-Wno-documentation',
      '-Wno-documentation-unknown-command',
      '-Wno-reserved-identifier',
    ]
  endif

  cairo_args = cc.get_supported_arguments(cairo_args)

  cairo_backend = library(
    name,
    sources,
    c_args: library_args + cairo_args,
    dependencies: [pugl_dep, cairo_dep],
    gnu_symbol_visibility: 'hidden',
    include_directories: includes,
    install: true,
    objc_args: library_args + cairo_args,
    soversion: soversion,
    version: meson.project_version(),
  )

  pugl_cairo_dep = declare_dependency(
    compile_args: extra_c_args,
    dependencies: [pugl_dep, cairo_dep],
    link_with: cairo_backend,
  )

  cairo_pkg_name = 'pugl-cairo-@0@'.format(major_version)

  pkg.generate(
    cairo_backend,
    description: 'Pugl GUI library with Cairo backend',
    filebase: cairo_pkg_name,
    name: 'Pugl Cairo',
    subdirs: [versioned_name],
    version: meson.project_version(),
  )

  meson.override_dependency(cairo_pkg_name, pugl_cairo_dep)
endif

# Vulkan
vulkan_dep = dependency(
  'vulkan',
  include_type: 'system',
  required: get_option('vulkan'),
)
if vulkan_dep.found()
  name = 'pugl_' + platform + '_vulkan' + version_suffix
  sources = files('src/' + platform + '_vulkan' + extension)

  thread_dep = dependency('threads', include_type: 'system')

  vulkan_deps = [pugl_dep, vulkan_dep, dl_dep]
  vulkan_c_args = library_args
  vulkan_link_args = []
  if platform == 'mac'
    vulkan_deps += [
      dependency('Metal', include_type: 'system', modules: 'foundation'),
      dependency('QuartzCore', include_type: 'system', modules: 'foundation'),
    ]
  endif

  vulkan_backend = library(
    name,
    sources,
    c_args: library_args,
    dependencies: vulkan_deps,
    gnu_symbol_visibility: 'hidden',
    include_directories: includes,
    install: true,
    soversion: soversion,
    version: meson.project_version(),
  )

  pugl_vulkan_dep = declare_dependency(
    compile_args: extra_c_args,
    dependencies: [pugl_dep, vulkan_dep, thread_dep],
    link_with: vulkan_backend,
  )

  vulkan_pkg_name = 'pugl-vulkan-@0@'.format(major_version)

  pkg.generate(
    vulkan_backend,
    description: 'Pugl GUI library with Vulkan backend',
    filebase: vulkan_pkg_name,
    name: 'Pugl Vulkan',
    subdirs: [versioned_name],
    version: meson.project_version(),
  )

  meson.override_dependency(vulkan_pkg_name, pugl_vulkan_dep)
endif

###############################
# Public Headers and Bindings #
###############################

subdir('include')
subdir('bindings/cpp')

######################
# Tests and Examples #
######################

if not get_option('tests').disabled()
  subdir('test')
endif

if not get_option('examples').disabled()
  subdir('examples')
endif

#################
# Documentation #
#################

if not get_option('docs').disabled()
  subdir('doc')
else
  build_docs = false
endif

if meson.version().version_compare('>=0.53.0')
  summary(
    {
      'Platform': platform,
    },
    section: 'Configuration',
  )

  if platform == 'x11'
    summary(
      {
        'Cursor support (XCursor)': xcursor_dep.found(),
        'Refresh rate support (XRandR)': xrandr_dep.found(),
        'Timer support (XSync)': use_xsync,
      },
      bool_yn: true,
      section: 'Configuration',
    )
  endif

  summary(
    {
      'Cairo': cairo_dep.found(),
      'OpenGL': opengl_dep.found(),
      'Stub': get_option('stub'),
      'Vulkan': vulkan_dep.found(),
    },
    bool_yn: true,
    section: 'Backends',
  )

  summary(
    {
      'Tests': not get_option('tests').disabled(),
      'Examples': not get_option('examples').disabled(),
      'Documentation': build_docs,
    },
    bool_yn: true,
    section: 'Components',
  )

  summary(
    {
      'Install prefix': get_option('prefix'),
      'Headers': get_option('prefix') / get_option('includedir'),
      'Libraries': get_option('prefix') / get_option('libdir'),
    },
    section: 'Directories',
  )

  if not get_option('examples').disabled()
    summary(
      {
        'Executables': get_option('prefix') / get_option('bindir'),
      },
      section: 'Directories',
    )
  endif
endif
