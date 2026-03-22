# Web Framework Selection

Guide for choosing and setting up web frameworks for iPlug2 WebView UIs.

## Raw HTML/CSS/JS (No Build Step)

The simplest approach. Place files directly in `resources/web/`.

### File Structure

```
resources/web/
  index.html       # Entry point
  script.js        # Bridge functions (SPVFUI, SPVFD, etc.)
  knob-control.js  # Custom web components (optional)
  style.css        # Styles (optional, can be inline)
```

### Bridge File (`script.js`)

Copy the bridge functions from `Examples/IPlugWebUI/resources/web/script.js`. This provides all `SPVFUI`, `BPCFUI`, `EPCFUI`, `SAMFUI`, `SMMFUI` functions and the `SPVFD`, `SCVFD`, `SAMFD`, `SMMFD` handlers.

### Web Components Pattern

Use custom elements with Shadow DOM for encapsulated, reusable controls:

```javascript
class KnobControl extends HTMLElement {
  constructor() {
    super();
    this.paramId = 0;
    this.attachShadow({ mode: 'open' });
    this.shadowRoot.innerHTML = `
      <style>/* scoped styles */</style>
      <div class="container">
        <svg viewBox="0 0 100 100" width="80" height="80">
          <circle cx="50" cy="50" r="42" fill="#000" stroke="#fff"/>
          <line class="pointer" x1="50" y1="10" x2="50" y2="50" stroke="#f00"/>
        </svg>
      </div>`;
    // Add mouse/touch handlers for parameter control
  }
  connectedCallback() {
    this.paramId = this.getAttribute('param-id') || -1;
  }
  updateValueFromHost(normalizedValue) {
    // Update visual state from SPVFD callback
  }
  static get observedAttributes() { return ['param-id', 'label', 'min', 'max']; }
}
customElements.define('knob-control', KnobControl);
```

Usage in HTML: `<knob-control param-id="0" label="Gain"></knob-control>`

See `Examples/IPlugWebUI/resources/web/knob-control.js` for a complete implementation.

### CSS Best Practices for Plugin UIs

```css
* {
  -webkit-touch-callout: none;
  -webkit-user-select: none;
  user-select: none;
}
body {
  overflow: hidden;
  margin: 0;
  padding: 0;
}
p, h1, .label {
  pointer-events: none;
}
```

## p5.js (Creative Coding)

Bundle `p5.min.js` directly in `resources/web/`. No build step needed.

### File Structure

```
resources/web/
  index.html      # Loads p5 and sketch
  p5.min.js       # p5.js library
  sketch.js       # Your p5 sketch
  script.js       # Bridge functions
```

### Sketch Pattern

```javascript
function setup() {
  createCanvas(windowWidth, windowHeight, WEBGL);
  noStroke();
}

function draw() {
  // Use parameter values received via SPVFD
  background(0);
  // ... drawing code
}

function windowResized() {
  resizeCanvas(windowWidth, windowHeight);
}
```

### Connecting Parameters

Define `OnParamChange` in your HTML/JS to receive parameter updates:

```javascript
let paramValues = {};

function OnParamChange(paramIdx, value) {
  paramValues[paramIdx] = value;
}

function draw() {
  // Use paramValues in your sketch
  let gain = paramValues[0] || 0;
}
```

### WebGL Shaders

p5.js supports GLSL shaders. See `Examples/IPlugP5js/resources/web/sketch.js` for a shader uniform example. Load shaders in `preload()`, set uniforms in `draw()`.

**Custom URL scheme required:** Set `SetCustomUrlScheme("iplug2")` in the C++ constructor when using p5.js, as it needs server-like URL resolution for loading shader files.

## Svelte + Vite + TypeScript

The recommended approach for complex, modern UIs with component architecture.

### Directory Structure

```
YourPlugin/
  YourPlugin.h
  YourPlugin.cpp
  config.h
  resources/web/          # Build output (git-ignored or committed)
    index.html
    assets/
  web-ui/                 # Source (always committed)
    package.json
    vite.config.js
    src/
      main.ts             # Entry point
      App.svelte           # Root component
      lib/
        iplug.ts           # Bridge module
        Knob.svelte        # Knob component
        VUMeter.svelte     # VU meter component
      types/
        iplug.d.ts         # TypeScript declarations
```

### Vite Configuration

```javascript
// web-ui/vite.config.js
import { defineConfig } from 'vite'
import { svelte } from '@sveltejs/vite-plugin-svelte'

export default defineConfig({
  plugins: [svelte()],
  build: {
    outDir: '../resources/web',
    emptyOutDir: true
  },
  base: './'  // Critical: use relative paths for bundled assets
})
```

### TypeScript Bridge Module (`iplug.ts`)

Export typed bridge functions and register them globally:

```typescript
export function SPVFUI(paramIdx: number, value: number) {
  IPlugSendMsg({ msg: "SPVFUI", paramIdx: parseInt(String(paramIdx)), value });
}
export function BPCFUI(paramIdx: number) {
  IPlugSendMsg({ msg: "BPCFUI", paramIdx: parseInt(String(paramIdx)) });
}
export function EPCFUI(paramIdx: number) {
  IPlugSendMsg({ msg: "EPCFUI", paramIdx: parseInt(String(paramIdx)) });
}

// Plugin-to-UI handlers (override in components)
export function SPVFD(paramIdx: number, val: number) {
  OnParamChange(paramIdx, val);
}
export function SAMFD(msgTag: number, dataSize: number, msg: string) {}
export function SCVFD(ctrlTag: number, val: number) {}
export function SCMFD(ctrlTag: number, msgTag: number, dataSize: number, msg: string) {}

// Register all on globalThis
globalThis.SPVFUI = SPVFUI;
globalThis.BPCFUI = BPCFUI;
globalThis.EPCFUI = EPCFUI;
globalThis.SPVFD = SPVFD;
globalThis.SAMFD = SAMFD;
globalThis.SCVFD = SCVFD;
globalThis.SCMFD = SCMFD;
```

See `Examples/IPlugSvelteUI/web-ui/src/lib/iplug.ts` for the complete implementation.

### TypeScript Declarations (`iplug.d.ts`)

```typescript
declare global {
  function IPlugSendMsg(message: any): void;
  interface Window {
    SPVFUI: (paramIdx: number, value: number) => void;
    BPCFUI: (paramIdx: number) => void;
    EPCFUI: (paramIdx: number) => void;
    SPVFD: (paramIdx: number, value: number) => void;
    SCVFD: (ctrlTag: number, value: number) => void;
    SCMFD: (ctrlTag: number, msgTag: number, dataSize: number, msg: string) => void;
    SAMFD: (msgTag: number, dataSize: number, msg: string) => void;
    OnParamChange: (paramIdx: number, value: number) => void;
  }
}
export {};
```

### Component Pattern (Svelte)

Handle parameter updates by overriding global callbacks in the component:

```svelte
<script lang="ts">
  import { SPVFUI, BPCFUI, EPCFUI } from './lib/iplug';

  let knobValue = 0;

  // Receive parameter changes from plugin
  globalThis.SPVFD = (paramIdx, val) => {
    if (paramIdx === 0) knobValue = val;
  };
</script>
```

See `Examples/IPlugSvelteUI/web-ui/src/lib/Knob.svelte` and `VUMeter.svelte` for complete component examples.

## Other Frameworks (React, Vue, etc.)

Follow the Svelte/Vite pattern. Key requirements:

1. Use Vite with `base: './'` and output to `../resources/web`
2. Create a bridge module that registers all functions on `globalThis`
3. Set `SetCustomUrlScheme("iplug2")` if the framework uses absolute imports
4. TypeScript declarations for `IPlugSendMsg` and bridge functions

## Hot Reloading Setup

### Steps

1. Set up your web project: `cd web-ui && npm install`
2. Start dev server: `npm run dev` (defaults to `http://localhost:5173`)
3. In plugin constructor, switch to dev URL:
   ```cpp
   mEditorInitFunc = [&]() {
     // LoadIndexHtml(__FILE__, GetBundleID());  // production
     LoadURL("http://localhost:5173/");           // development
     EnableScroll(false);
   };
   ```
4. Build the plugin and run. UI changes hot-reload instantly.
5. For production: `npm run build`, switch back to `LoadIndexHtml`.

### Code Signing Note

Debug builds with hardened runtime (macOS) may fail to load `file://` URLs from outside the app sandbox. Either disable hardened runtime in debug, or use `LoadURL` with a local dev server as a workaround.

### Production Build

```bash
cd web-ui
npm run build   # Outputs bundled assets to resources/web/
```

The plugin C++ code loads from the bundle in release builds via `LoadIndexHtml(__FILE__, GetBundleID())`.
