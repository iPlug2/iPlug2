# IPlugWebUI
A basic volume control effect plug-in which uses a platform web view to host an HTML/CSS GUI

# CMake instructions
To setup the build (change CMAKE_BUILD_TYPE as desired):
```
  mkdir build-cmake
  cd build-cmake
  cmake .. -DCMAKE_BUILD_TYPE=Debug
```
To build a binary (change target as desired (app/clap/vst2/vst3)):
```
  cmake --build . --target app -j
```