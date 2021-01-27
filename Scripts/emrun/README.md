Emrun
=====

Self-sufficient tools for command line automation of browser and Firefox OS tasks. Often used with Emscripten.

 - ffdb.py: **Firefox OS Debug Bridge**, a command line tool for operating a Firefox OS device (install and run apps, log execution, etc).
 - emrun.py: **HTML autorunner**, run .html files from the command line as if they were native executables.
 - embench.py: **Benchmark runner**, executes a given native or JS benchmark application and creates a visual .html page of the results. (planned, not yet implemented)

Design goals
------------
The tools in this repository are developed with the following areas of focus:
 - Depends on [python](https://www.python.org) and [ADB](http://developer.android.com/tools/help/adb.html) installed on the system. No other dependencies.
 - Works on Windows, OSX and Linux.
 - Highly self-sufficient and embeddable. No installation steps needed, just copy the file(s) for the tools you are interested in off this repository.
 - Usable both as a command line tool and embeddable as a library to other applications.

Documentation
-------------

Documentation on emrun is currently available here: https://kripken.github.io/emscripten-site/docs/compiling/Running-html-files-with-emrun.html

Documentation on ffdb is currently available by running `ffdb --help` after cloning the repository.
