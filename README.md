# jsfx
JSFX

This is the source code to JSFX (originally known as JesuSonic) the real-time programmable FX system. The historical web page for Jesusonic is here: https://www.cockos.com/jsfx/

Development of JSFX began in 2004: it was a text-mode, linux-only, standalone processor application. In the decades since, it has evolved to be module loaded by REAPER, the text-mode UI has gradually been eliminated in favor of modern windowing systems, and many new features have been added.

This source release includes:

- Code to compile jsfx.dll/jsfx.so/jsfx.dylib, via jsfx/Makefile (macOS/Linux) and jsfx.vcxproj (VS2013, can easily be updated for newer VS), which can allow one to modify REAPER's built-in JSFX support.

- Legacy ReaJS VST2: jsfx/reajs-vst2: updated for latest JSFX features; Windows only, requires VST2 SDK.
