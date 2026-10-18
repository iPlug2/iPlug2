# Copyright 2021-2022 David Robillard <d@drobilla.net>
# SPDX-License-Identifier: 0BSD OR ISC

config = configuration_data()
config.set('NAME', 'pugl_embed_demo')

info_plist = configure_file(
  configuration: config,
  input: files('../../resources/Info.plist.in'),
  output: 'Info.plist',
)

subdir('MacOS')
