# IPlugFaustDSP
A plug-in that uses FAUST to implent its DSP and JIT-compile FAUST code in debug builds.

## NOTES

* Since this project watches a FAUST .dsp file on disk, It must be able to access the file via the filesystem. In the case of macOS, if the app is running in the app-sandbox, it's likely that it can't load the .dsp file and will trigger an assertion.

* The macro FAUST_LIBRARY_PATH can be set to specify at the install paths for faust libraries like stdfaust.lib. By default it looks in the standard faust install path, so if you didn't install faust the compilation may fail.