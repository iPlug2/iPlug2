if get_option('docs')
	doxygen = find_program('doxygen')

	conf_data = configuration_data()
	conf_data.set('PACKAGE_VERSION', meson.project_version())
	conf_data.set('top_srcdir', meson.project_source_root())
	conf_data.set('top_builddir', meson.project_build_root())

	doxyfile = configure_file(input : 'Doxyfile.in',
		       		  output : 'Doxyfile',
		       		  configuration : conf_data,
		       		  install: false)

	doxygen_target = custom_target('doc',
		                    input: doxyfile,
		                    output: 'html',
		                    command: [doxygen, doxyfile],
		                    install: get_option('install_docs'),
		                    install_dir: get_option('datadir') / 'doc' / 'rtaudio')

	subdir('images')
endif
