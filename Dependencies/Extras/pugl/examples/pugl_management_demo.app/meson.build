# Copyright 2021-2023 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

config = configuration_data()
config.set('NAME', 'pugl_management_demo')

info_plist = configure_file(
  configuration: config,
  input: files('../../resources/Info.plist.in'),
  output: 'Info.plist',
)

subdir('MacOS')
