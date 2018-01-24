# Dependencies

For improved graphics support - WDL-OL/IPlug2 can use Cairo and Freetype, which can be installed using package managers.


## Windows - vckpkg

Use [vcpkg](https://github.com/Microsoft/vcpkg) to install third party dependencies on Windows.

```vcpkg.exe install cairo:x64-windows-static cairo:x86-windows-static```

## macOS - Homebrew
Use [homebrew](https://brew.sh/) to install third party dependencies on macOS. If you prefer you can build these libraries from source and specify their locations in common.xcconfig, but you will save yourself a lot of time by using homebrew.  

```
~ oli$ brew install cairo zlib bzip2 expat
```

This will install the following packagaes: cairo, libpng, freetype, fontconfig, pixman, gettext, libffi, pcre, glib, zlib, bzip2, expat
