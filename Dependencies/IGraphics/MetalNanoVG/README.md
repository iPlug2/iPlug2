MetalNanoVG
===========

MetalNanoVG is the native [Metal](https://developer.apple.com/metal/) port of [NanoVG](https://github.com/memononen/nanovg) that tries to get the most out of Apple's Graphics APIs.

### Donation
If you found this project useful, please consider donating to show your support ❤️ 

[![Donate](https://www.paypalobjects.com/en_US/i/btn/btn_donateCC_LG.gif)](https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=3366Q3AVUJLTQ)

Precautions
===========

 * Works only on [macOS 10.11+](https://support.apple.com/en-us/HT205073), tvOS 9.0+ and [iOS 8.0+](https://developer.apple.com/library/content/documentation/DeviceInformation/Reference/iOSDeviceCompatibility/DeviceCompatibilityMatrix/DeviceCompatibilityMatrix.html#//apple_ref/doc/uid/TP40013599-CH17-SW1).
 * Simulator support is available since iOS 13 and requires Xcode 11+ running on macOS 10.15+.
 * Not all Apple hardwares are supported even if meets the OS requirement.
 * [ARC](https://en.wikipedia.org/wiki/Automatic_Reference_Counting) is required.

Advantages
==========

 * Shared buffers between CPU and GPU.
 * Various Metal states are cached whenever possible.
 * Low overheads compared to OpenGL.
 * Pre-compiled shaders. (no need to compile shaders at runtime)
 * Seamless integration with powerful Metal features such as [Metal Performance Shaders](https://developer.apple.com/documentation/metalperformanceshaders).

Installation
============

 1. Download both `NanoVG` and `MetalNanoVG` source codes.
 2. Add both `NanoVG` and `MetalNanoVG`'s `src` directories to the header search
    path.
 3. Add `NanoVG`'s `src/nanovg.c` and `MetalNanoVG`'s `src/nanovg_mtl.m` to
    the `Compile Sources` section in Xcode.
 4. Link the `Metal` and `QuartzCore` frameworks.
 5. For best performance, disable *GPU Frame Capture* and *Metal API Validation* as described [here](https://developer.apple.com/library/content/documentation/Miscellaneous/Conceptual/MetalProgrammingGuide/Dev-Technique/Dev-Technique.html#//apple_ref/doc/uid/TP40014221-CH8-SW3).

 Done.

Usage
=====

 1. Include the headers.

```C
#include "nanovg.h"
#include "nanovg_mtl.h"
```

 2. Pass the `CAMetalLayer` object when creating the NanoVG context.

 ```C
NVGcontext* ctx = nvgCreateMTL(metalLayer, NVG_ANTIALIAS | NVG_STENCIL_STROKES);
 ```

Benchmark
=========

The following table depicts a simple CPU usage benchmark of running the
NanoVG demo app on iOS devices with full Retina resolution. Both Metal and
OpenGL ES2 implementations get constant 60 FPS.

  |            | iPhone 6s+  | iPad Pro 12.7" (2015) |
  | ---------- | ----------- | --------------------- |
  | Resolution | 1080 x 1920 | 2732 * 2048           |
  | Metal      | 20%         | 20%                   |
  | OpenGL ES2 | 35%         | 33%                   |

Example
=======
MetalNanoVG was originally created to improve the performance of the iOS app [Fog of World](https://fogofworld.com).

![Screenshot of Fog of World](http://media.fogofworld.com.s3.amazonaws.com/github/fogofworld_screenshot.jpg)
