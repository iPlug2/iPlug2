# Dependencies

For improved graphics support - WDL-OL/IPlug2 can use Cairo and Freetype, which can be installed using package managers.


## Windows - vckpkg

Use [vcpkg](https://github.com/Microsoft/vcpkg) to install third party dependencies on Windows.

```vcpkg.exe install cairo:x64-windows-static cairo:x86-windows-static```

## macOS - Homebrew 
Although it's NOT RECOMMENDED, you can use [homebrew](https://brew.sh/) to install third party static library dependencies on macOS. Whilst being simple to install, this has several limitations which are undesirable. We want to produce universal binary builds of plug-ins that support operating systems lower than the one we build on, for this reason it is better to use the **build-libs-mac.sh** script provided in the "Libraries" folder.

However, if you want to install these dependencies with homebrew for any reason, you can do so with the following command:

```
~ oli$ brew install cairo zlib bzip2 expat
```

This will install the following packagaes: cairo, libpng, freetype, fontconfig, pixman, gettext, libffi, pcre, glib, zlib, bzip2, expat

NOTE: you will need to modify common.xcconfig, so that the build setting **STATICLIBS_PATH** refers to the homebrew path, e.g. ```/usr/local```