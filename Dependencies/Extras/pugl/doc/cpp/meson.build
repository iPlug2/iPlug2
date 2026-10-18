# Copyright 2021 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

config = configuration_data()
config.set('PUGL_VERSION', meson.project_version())

conf_py = configure_file(
  configuration: config,
  input: '../conf.py.in',
  output: 'conf.py',
)

configure_file(copy: true, input: '../deployment.rst', output: 'deployment.rst')
configure_file(copy: true, input: '../summary.rst', output: 'summary.rst')

cpp_rst_files = files(
  'index.rst',
  'overview.rst',
  'world.rst',
  'view.rst',
  'events.rst',
  'event-loop.rst',
)

foreach f : cpp_rst_files
  configure_file(copy: true, input: f, output: '@PLAINNAME@')
endforeach

subdir('xml')
subdir('api')

docs = custom_target(
  'singlehtml',
  build_by_default: true,
  command: [sphinx_build, '-M', 'singlehtml', '@OUTDIR@', '@OUTDIR@',
            '-E', '-q', '-t', 'singlehtml'],
  input: [cpp_rst_files, cpp_pugl_rst, cpp_index_xml],
  install: true,
  install_dir: docdir / 'puglpp-0',
  output: 'singlehtml',
)

docs = custom_target(
  'html',
  command: [sphinx_build, '-M', 'html', '@OUTDIR@', '@OUTDIR@',
            '-E', '-q', '-t', 'html'],
  build_by_default: true,
  input: [cpp_rst_files, cpp_pugl_rst, cpp_index_xml],
  install: true,
  install_dir: docdir / 'puglpp-0',
  output: 'html',
)
