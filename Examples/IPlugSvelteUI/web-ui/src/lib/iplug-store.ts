/**
 * iPlug2 Svelte Store
 *
 * Provides reactive Svelte stores for iPlug2 plugin parameters and control values.
 * Works in both WebView and WASM/EMAudioWorklet environments.
 *
 * @example
 * ```svelte
 * <script>
 *   import { params, controlValues, setParam } from '$lib/iplug-store';
 *
 *   // Reactive parameter access
 *   $: gainValue = $params.get(0) ?? 0;
 *
 *   // Set parameter with gesture tracking
 *   function onKnobChange(value: number) {
 *     setParam(0, value);
 *   }
 * </script>
 * ```
 */

import { writable, derived, type Readable, type Writable } from 'svelte/store';
import { onParamChange, onControlValue, SPVFUI, BPCFUI, EPCFUI, isWasmMode, isWebViewMode } from './iplug';

// ============================================================================
// Store Types
// ============================================================================

export interface PluginState {
  /** Whether the plugin is ready */
  ready: boolean;
  /** Current environment mode */
  mode: 'wasm' | 'webview' | 'unknown';
  /** Error message if any */
  error: string | null;
}

// ============================================================================
// Core Stores
// ============================================================================

/** Plugin state store */
export const pluginState: Writable<PluginState> = writable({
  ready: false,
  mode: 'unknown',
  error: null
});

/** Parameter values (paramIdx -> value) */
export const params: Writable<Map<number, number>> = writable(new Map());

/** Control values for meters/visualizers (ctrlTag -> value) */
export const controlValues: Writable<Map<number, number>> = writable(new Map());

// ============================================================================
// Initialization
// ============================================================================

/**
 * Initialize the iPlug store
 * Call this once when your app starts
 */
export function initIPlugStore(): void {
  // Detect mode
  const mode = isWasmMode() ? 'wasm' : isWebViewMode() ? 'webview' : 'unknown';

  pluginState.update(state => ({
    ...state,
    mode,
    ready: mode !== 'unknown'
  }));

  // Subscribe to parameter changes from plugin
  onParamChange((paramIdx, value) => {
    params.update(map => {
      map.set(paramIdx, value);
      return new Map(map); // Create new Map for reactivity
    });
  });

  // Subscribe to control value changes (meters, etc.)
  onControlValue((ctrlTag, value) => {
    controlValues.update(map => {
      map.set(ctrlTag, value);
      return new Map(map);
    });
  });
}

// ============================================================================
// Parameter Helpers
// ============================================================================

/**
 * Set a parameter value (with optional gesture tracking)
 * @param paramIdx Parameter index
 * @param value Parameter value
 */
export function setParam(paramIdx: number, value: number): void {
  SPVFUI(paramIdx, value);
  // Optimistically update local store
  params.update(map => {
    map.set(paramIdx, value);
    return new Map(map);
  });
}

/**
 * Begin a parameter change gesture
 * Call this when starting to drag a control
 */
export function beginParamGesture(paramIdx: number): void {
  BPCFUI(paramIdx);
}

/**
 * End a parameter change gesture
 * Call this when releasing a control
 */
export function endParamGesture(paramIdx: number): void {
  EPCFUI(paramIdx);
}

/**
 * Create a derived store for a single parameter
 * @param paramIdx Parameter index
 * @param defaultValue Default value if not set
 * @returns Readable store with the parameter value
 */
export function createParamStore(paramIdx: number, defaultValue = 0): Readable<number> {
  return derived(params, $params => $params.get(paramIdx) ?? defaultValue);
}

/**
 * Create a derived store for a single control value (meter, etc.)
 * @param ctrlTag Control tag
 * @param defaultValue Default value if not set
 * @returns Readable store with the control value
 */
export function createControlValueStore(ctrlTag: number, defaultValue = 0): Readable<number> {
  return derived(controlValues, $cv => $cv.get(ctrlTag) ?? defaultValue);
}

// ============================================================================
// Bindable Parameter Store
// ============================================================================

/**
 * Create a two-way bindable store for a parameter
 * Useful for direct binding with Svelte's bind:value
 *
 * @example
 * ```svelte
 * <script>
 *   const gain = createBindableParam(0, -70);
 * </script>
 * <input type="range" bind:value={$gain} min="-70" max="0" />
 * ```
 */
export function createBindableParam(paramIdx: number, defaultValue = 0): Writable<number> {
  const { subscribe } = derived(params, $params => $params.get(paramIdx) ?? defaultValue);

  return {
    subscribe,
    set: (value: number) => {
      setParam(paramIdx, value);
    },
    update: (fn: (value: number) => number) => {
      params.update(map => {
        const current = map.get(paramIdx) ?? defaultValue;
        const newValue = fn(current);
        SPVFUI(paramIdx, newValue);
        map.set(paramIdx, newValue);
        return new Map(map);
      });
    }
  };
}

// ============================================================================
// Auto-initialization
// ============================================================================

// Auto-initialize when the module is imported
// This ensures callbacks are set up early
if (typeof window !== 'undefined') {
  // Defer initialization to allow pluginController to be set up
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', initIPlugStore);
  } else {
    // DOM already loaded, init immediately
    initIPlugStore();
  }
}
