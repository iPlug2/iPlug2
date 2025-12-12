doxygen_images = files('ccrma.gif',
		       'mcgill.gif')

foreach f : doxygen_images
	df = configure_file(input: f,
			    output: '@PLAINNAME@',
			    copy: true,
			    install: get_option('install_docs'),
			    install_dir : get_option('datadir') / 'doc' / 'rtaudio')
endforeach
