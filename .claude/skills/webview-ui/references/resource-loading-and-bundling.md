# Resource Loading and Bundling

How web content is loaded, served, and packaged in iPlug2 WebView plugins.

## Loading Methods

### LoadIndexHtml (Recommended)

The primary method for loading bundled web UIs. Handles debug vs release paths automatically:

```cpp
mEditorInitFunc = [&]() {
  LoadIndexHtml(__FILE__, GetBundleID());
  EnableScroll(false);
};
```

- **Debug (desktop):** Resolves `__FILE__` to the plugin source directory, loads `Resources/web/index.html` from the filesystem. Allows editing HTML/JS without rebuilding the plugin.
- **Release (desktop):** Loads `index.html` from the app/plugin bundle via `LoadFile("index.html", bundleID)`.
- **iOS:** Always loads from the bundle.

**Code signing caveat:** Debug builds with hardened runtime may fail to load `file://` URLs outside the sandbox. Disable hardened runtime in debug builds, or use `LoadURL` with a dev server.

### LoadFile

Direct file loading with platform-specific path handling:

```cpp
// macOS/iOS: filename only, loaded from bundle's web/ subfolder
LoadFile("index.html", "com.yourcompany.yourplugin");

// Windows: requires absolute path
LoadFile("C:\\path\\to\\index.html");
```

### LoadURL

Load from a URL. Used for hot-reload development or remote content:

```cpp
LoadURL("http://localhost:5173/");  // Vite dev server
```

### LoadHTML

Load a raw HTML string directly. Useful for simple displays:

```cpp
LoadHTML("<h1>Status: Connected</h1><p>Latency: 2.1ms</p>");
```

Replaces all current WebView content. Used in `IPlugOSCEditor` to update an embedded log display.

## Custom URL Scheme

```cpp
SetCustomUrlScheme("iplug2");  // Call before WebView opens (in constructor)
```

Makes web content served as if from an `iplug2://` origin instead of `file://`. This is needed when:
- Web frameworks generate absolute path imports (`/assets/index.js` instead of `./assets/index.js`)
- Using ES modules that require same-origin policy
- Loading shaders or other resources via relative paths in p5.js

Svelte/Vite with `base: './'` often works without it, but enabling it is safer.

## Build Configuration

### Xcode / xcconfig

For full WebView UIs, the project xcconfig must define:

```
GCC_PREPROCESSOR_DEFINITIONS = WEBVIEW_EDITOR_DELEGATE NO_IGRAPHICS $(inherited)
```

This tells `IPlugDelegate_select.h` to use `WebViewEditorDelegate` instead of `IGEditorDelegate`.

Link against WebKit on macOS:
```
OTHER_LDFLAGS = -framework WebKit $(inherited)
```

### Windows

Requires Edge Chromium (WebView2 runtime). The CMake configuration (`Scripts/cmake/WebView.cmake`) handles:
- WebView2 SDK download
- WIL (Windows Implementation Libraries) dependency
- Platform-specific source file compilation

### CMake

For CMake-based builds, include the WebView module:

```cmake
include(${IPLUG2_DIR}/Scripts/cmake/WebView.cmake)
```

This adds platform-specific sources, enables ARC on macOS/iOS, and links required frameworks.

## Vite Build Configuration

For framework-based projects (Svelte, React, Vue):

```javascript
// web-ui/vite.config.js
import { defineConfig } from 'vite'
import { svelte } from '@sveltejs/vite-plugin-svelte'

export default defineConfig({
  plugins: [svelte()],
  build: {
    outDir: '../resources/web',  // Output directly to resources
    emptyOutDir: true
  },
  base: './'  // Critical: relative paths for bundled assets
})
```

`base: './'` ensures all asset references in the built HTML use relative paths (`./assets/index.js`), which works with both `file://` loading and custom URL schemes.

## Recommended Directory Layout

### Raw HTML Project

```
YourPlugin/
  YourPlugin.h
  YourPlugin.cpp
  config.h
  resources/
    web/
      index.html          # Entry point
      script.js           # Bridge functions
      knob-control.js     # Custom elements
      style.css           # Styles
```

### Framework Project (Svelte/React/Vue)

```
YourPlugin/
  YourPlugin.h
  YourPlugin.cpp
  config.h
  resources/
    web/                  # Build output (may be git-ignored)
      index.html
      assets/
        index-[hash].js
        index-[hash].css
  web-ui/                 # Framework source
    package.json
    vite.config.js
    src/
      main.ts
      App.svelte
      lib/
        iplug.ts          # Bridge module
      types/
        iplug.d.ts        # TypeScript declarations
```

### p5.js Project

```
YourPlugin/
  YourPlugin.h
  YourPlugin.cpp
  config.h
  resources/
    web/
      index.html
      p5.min.js           # Bundled library
      sketch.js           # Your sketch
      script.js           # Bridge functions
      uniforms.vert       # Shader files (optional)
      uniforms.frag
```

## Resource Packaging

### macOS

The `web/` folder under `Resources/` is automatically included in the app bundle. Xcode copies it during the "Copy Bundle Resources" build phase.

### Windows

Web resources must be packaged alongside the plugin binary. The CMake configuration handles copying `resources/web/` to the output directory. For manual builds, ensure the `web/` folder is accessible at the expected path.

### iOS

Same as macOS -- resources are bundled automatically. `LoadFile("index.html", bundleID)` loads from the bundle.
