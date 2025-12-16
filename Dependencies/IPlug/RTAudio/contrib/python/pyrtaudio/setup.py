#!/bin/env python

import os
from distutils.core import setup, Extension

if hasattr(os, 'uname'):
    OSNAME = os.uname()[0]
else:
    OSNAME = 'Windows'


define_macros = []
libraries = []
extra_link_args = []
extra_compile_args = ['-I../../../']
sources = ['rtaudiomodule.cpp', '../../../RtAudio.cpp']


if OSNAME == 'Linux':
    define_macros=[("__LINUX_ALSA__", ''),
                   ('__LINUX_JACK__', '')]
    libraries = ['asound', 'jack', 'pthread']

elif OSNAME == 'Darwin':
    define_macros = [('__MACOSX_CORE__', '')]
    libraries = ['pthread', 'stdc++']
    extra_link_args = ['-framework', 'CoreAudio']

elif OSNAME == 'Windows':
    define_macros = [('__WINDOWS_DS__', None),
                     ('__WINDOWS_ASIO__', None),
		     ('__LITTLE_ENDIAN__',None),
		     ('WIN32',None)]
    libraries = ['winmm', 'dsound', 'Advapi32','Ole32','User32']
    sources += ['../../../include/asio.cpp',
                '../../../include/asiodrivers.cpp',
                '../../../include/asiolist.cpp',
                '../../../include/iasiothiscallresolver.cpp']
    extra_compile_args.append('-I../../../include/')
    extra_compile_args.append('-EHsc')


audio = Extension('rtaudio',
                 sources=sources,
                 libraries=libraries,
                 define_macros=define_macros,
		 extra_compile_args = extra_compile_args,
                 extra_link_args = extra_link_args,
                 )


setup(name = 'rtaudio',
      version = '0.1',
      description = 'Python RtAudio interface',
      ext_modules = [audio])

