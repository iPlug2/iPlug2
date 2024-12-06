project('RtAudio', 'cpp',
	version: '6.0.1',

	default_options: ['warning_level=3',
			'c_std=c99',
			'cpp_std=c++11',
			'default_library=both'])

fs = import('fs')
pkg = import('pkgconfig')

rt_h = fs.read('RtAudio.h').strip().split('\n')
foreach line : rt_h
	if line.startswith('#define RTAUDIO_VERSION_MAJOR')
		rt_version_major = line.split()[2]
	elif line.startswith('#define RTAUDIO_VERSION_MINOR')
		rt_version_minor = line.split()[2]
	elif line.startswith('#define RTAUDIO_VERSION_PATCH')
		rt_version_patch = line.split()[2]
	elif line.startswith('#define RTAUDIO_VERSION_BETA')
		rt_version_beta = line.split()[2]
	endif
endforeach
if rt_version_beta.to_int() > 0
	rt_version = rt_version_major + '.' + rt_version_minor + '.' + rt_version_patch + 'beta' + rt_version_beta
else
	rt_version = rt_version_major + '.' + rt_version_minor + '.' + rt_version_patch
endif

assert(meson.project_version() == rt_version, 'Meson\'s RtAudio version does not match the version in header file.')

ac_file = fs.read('configure.ac').strip().split('\n')
foreach line : ac_file
	if line.startswith('m4_define([lt_current],')
		lt_current = line.substring(-2,-1).to_int()
	elif line.startswith('m4_define([lt_revision],')
		lt_revision = line.substring(-2,-1).to_int()
	elif line.startswith('m4_define([lt_age],')
		lt_age = line.substring(-2,-1).to_int()
	endif
endforeach

so_version = '@0@.@1@.@2@'.format(lt_current - lt_age, lt_age, lt_revision)

src = ['RtAudio.cpp', 'rtaudio_c.cpp']
incdir = include_directories('include')

install_headers('RtAudio.h', 'rtaudio_c.h', subdir: 'rtaudio')

compiler = meson.get_compiler('cpp')

deps = []
defines = ['-DRTAUDIO_EXPORT']

if compiler.has_function('gettimeofday', prefix: '#include <sys/time.h>')
	defines += '-DHAVE_GETTIMEOFDAY'
endif

if get_option('debug') == true
	defines += '-D__RTAUDIO_DEBUG__'
endif

deps += dependency('threads')

alsa_dep = dependency('alsa', required: get_option('alsa'))
if alsa_dep.found()
	defines += '-D__LINUX_ALSA__'
	deps += alsa_dep
endif

jack_dep = dependency('jack', required: get_option('jack'))
if jack_dep.found()
	defines += '-D__UNIX_JACK__'
	deps += jack_dep
endif

if get_option('oss') == true
	defines += '-D__LINUX_OSS__'
endif

pulsesimple_dep = dependency('libpulse-simple', required: get_option('pulse'))
if pulsesimple_dep.found()
	defines += '-D__LINUX_PULSE__'
	deps += pulsesimple_dep
endif

core_dep = dependency('appleframeworks', modules: ['CoreAudio', 'CoreFoundation'], required: get_option('core'))
if core_dep.found()
	defines += '-D__MACOSX_CORE__'
	deps += core_dep
endif

dsound_dep = compiler.find_library('dsound', required: get_option('dsound'))
if dsound_dep.found()
	defines += '-D__WINDOWS_DS__'
	deps += dsound_dep
endif


wasapi_found = compiler.check_header('audioclient.h', required: get_option('wasapi'))
if wasapi_found
	deps += compiler.find_library('mfplat', required: true)
	deps += compiler.find_library('mfuuid', required: true)
	deps += compiler.find_library('ksuser', required: true)
	deps += compiler.find_library('wmcodecdspuuid', required: true)
	defines += '-D__WINDOWS_WASAPI__'
endif

asio_found = compiler.check_header('windows.h', required: get_option('asio'))
if asio_found
	src += ['include/asio.cpp',
		'include/asiolist.cpp',
		'include/asiodrivers.cpp',
		'include/iasiothiscallresolver.cpp']
	defines += '-D__WINDOWS_ASIO__'
endif

if host_machine.system() == 'windows'
	deps += compiler.find_library('ole32', required: true)
	deps += compiler.find_library('winmm', required: true)
endif

rtaudio = library('rtaudio', src,
		version: so_version,
		include_directories: incdir,
		cpp_args: defines,
		dependencies: deps,
		gnu_symbol_visibility: 'hidden',
		install: true)
rtaudio_dep = declare_dependency(include_directories : '.',
  link_with : rtaudio)
meson.override_dependency('rtaudio', rtaudio_dep)

subdir('tests')
subdir('doc')

pkg.generate(rtaudio,
	description: 'RtAudio - a set of C++ classes that provide a common API for realtime audio input/output',
	subdirs: 'rtaudio')

summary({'ALSA': alsa_dep.found(),
	'OSS': get_option('oss'),
	'JACK': jack_dep.found(),
	'PulseAudio': pulsesimple_dep.found(),
	'CoreAudio': core_dep.found(),
	'DirectAudio': dsound_dep.found(),
	'WASAPI': wasapi_found,
	'ASIO': asio_found}, bool_yn: true, section: 'Audio Backends')
