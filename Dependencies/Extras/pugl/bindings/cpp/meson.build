# Copyright 2021 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

subdir('include')

puglpp_dep = declare_dependency(
  dependencies: core_deps + [pugl_dep],
  include_directories: include_directories('include'),
  link_with: libpugl,
)

puglpp_versioned_name = 'puglpp-@0@'.format(major_version)

pkg.generate(
  description: 'Pugl GUI library C++ bindings',
  filebase: puglpp_versioned_name,
  name: 'Pugl++',
  subdirs: [puglpp_versioned_name],
  version: meson.project_version(),
)

meson.override_dependency(puglpp_versioned_name, puglpp_dep)
