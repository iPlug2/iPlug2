/**
 * iPlug2 Svelte Communication Layer
 *
 * This module provides a unified API that works in both environments:
 * 1. WebView mode (desktop/mobile apps) - uses IPlugSendMsg() native bridge
 * 2. WASM mode (EMAudioWorklet in browser) - uses window.pluginController
 *
 * Usage in Svelte components is identical regardless of environment.
 */

// Extend window type for our globals
declare global {
  interface Window {
    pluginController?: {
      setParam: (paramIdx: number, value: number) => void;
      beginParamChange: (paramIdx: number) => void;
      endParamChange: (paramIdx: number) => void;
      sendMidi: (status: number, data1: number, data2: number) => void;
      on: (event: string, callback: (...args: any[]) => void) => () => void;
    };
  }
  // WebView bridge function
  function IPlugSendMsg(msg: object): void;
}

// ============================================================================
// Environment Detection
// ============================================================================

/**
 * Check if running in WASM/EMAudioWorklet mode
 * (browser with pluginController from IPlugController.js)
 */
export function isWasmMode(): boolean {
  return typeof window !== 'undefined' &&
         typeof window.pluginController?.setParam === 'function';
}

/**
 * Check if running in WebView mode
 * (native app with IPlugSendMsg bridge)
 */
export function isWebViewMode(): boolean {
  return typeof IPlugSendMsg === 'function';
}

/**
 * Get the current environment mode
 */
export function getMode(): 'wasm' | 'webview' | 'unknown' {
  if (isWasmMode()) return 'wasm';
  if (isWebViewMode()) return 'webview';
  return 'unknown';
}

// ============================================================================
// FROM UI TO PLUGIN
// ============================================================================

/**
 * Send Parameter Value From UI
 * @param paramIdx Parameter index
 * @param value Parameter value (non-normalized)
 */
export function SPVFUI(paramIdx: number, value: number): void {
  if (paramIdx < 0) {
    console.warn("SPVFUI: paramIdx must be >= 0");
    return;
  }

  if (isWasmMode()) {
    window.pluginController!.setParam(paramIdx, value);
  } else if (isWebViewMode()) {
    IPlugSendMsg({
      msg: "SPVFUI",
      paramIdx: parseInt(String(paramIdx)),
      value: value
    });
  } else {
    console.warn("SPVFUI: No communication channel available");
  }
}

/**
 * Begin Parameter Change From UI (for DAW automation recording)
 * @param paramIdx Parameter index
 */
export function BPCFUI(paramIdx: number): void {
  if (paramIdx < 0) {
    console.warn("BPCFUI: paramIdx must be >= 0");
    return;
  }

  if (isWasmMode()) {
    window.pluginController!.beginParamChange(paramIdx);
  } else if (isWebViewMode()) {
    IPlugSendMsg({
      msg: "BPCFUI",
      paramIdx: parseInt(String(paramIdx))
    });
  }
}

/**
 * End Parameter Change From UI
 * @param paramIdx Parameter index
 */
export function EPCFUI(paramIdx: number): void {
  if (paramIdx < 0) {
    console.warn("EPCFUI: paramIdx must be >= 0");
    return;
  }

  if (isWasmMode()) {
    window.pluginController!.endParamChange(paramIdx);
  } else if (isWebViewMode()) {
    IPlugSendMsg({
      msg: "EPCFUI",
      paramIdx: parseInt(String(paramIdx))
    });
  }
}

/**
 * Send MIDI Message From UI
 * @param status MIDI status byte
 * @param data1 MIDI data byte 1
 * @param data2 MIDI data byte 2
 */
export function SMMFUI(status: number, data1: number, data2: number): void {
  if (isWasmMode()) {
    window.pluginController!.sendMidi(status, data1, data2);
  } else if (isWebViewMode()) {
    IPlugSendMsg({
      msg: "SMMFUI",
      status,
      data1,
      data2
    });
  }
}

// ============================================================================
// FROM PLUGIN TO UI (callbacks set by plugin/controller)
// ============================================================================

// Callback storage for Svelte reactive updates
type ParamChangeCallback = (paramIdx: number, value: number) => void;
type ControlValueCallback = (ctrlTag: number, value: number) => void;
type ControlMsgCallback = (ctrlTag: number, msgTag: number, dataSize: number, data: ArrayBuffer | string | null) => void;
type MidiMsgCallback = (status: number, data1: number, data2: number) => void;
type ArbitraryMsgCallback = (msgTag: number, dataSize: number, data: ArrayBuffer | string | null) => void;

const callbacks = {
  paramChange: [] as ParamChangeCallback[],
  controlValue: [] as ControlValueCallback[],
  controlMsg: [] as ControlMsgCallback[],
  midiMsg: [] as MidiMsgCallback[],
  arbitraryMsg: [] as ArbitraryMsgCallback[]
};

/**
 * Subscribe to parameter changes from the plugin
 * @returns Unsubscribe function
 */
export function onParamChange(callback: ParamChangeCallback): () => void {
  callbacks.paramChange.push(callback);
  return () => {
    const idx = callbacks.paramChange.indexOf(callback);
    if (idx >= 0) callbacks.paramChange.splice(idx, 1);
  };
}

/**
 * Subscribe to control value changes (meters, visualizers)
 * @returns Unsubscribe function
 */
export function onControlValue(callback: ControlValueCallback): () => void {
  callbacks.controlValue.push(callback);
  return () => {
    const idx = callbacks.controlValue.indexOf(callback);
    if (idx >= 0) callbacks.controlValue.splice(idx, 1);
  };
}

/**
 * Subscribe to control messages (complex data)
 * @returns Unsubscribe function
 */
export function onControlMsg(callback: ControlMsgCallback): () => void {
  callbacks.controlMsg.push(callback);
  return () => {
    const idx = callbacks.controlMsg.indexOf(callback);
    if (idx >= 0) callbacks.controlMsg.splice(idx, 1);
  };
}

// ============================================================================
// Global handlers (called by WebView bridge or IPlugController)
// ============================================================================

/** Send Parameter Value From Delegate */
export function SPVFD(paramIdx: number, val: number): void {
  callbacks.paramChange.forEach(cb => cb(paramIdx, val));
}

/** Send Control Value From Delegate */
export function SCVFD(ctrlTag: number, val: number): void {
  callbacks.controlValue.forEach(cb => cb(ctrlTag, val));
}

/** Send Control Message From Delegate */
export function SCMFD(ctrlTag: number, msgTag: number, dataSize: number, msg: ArrayBuffer | string | null): void {
  callbacks.controlMsg.forEach(cb => cb(ctrlTag, msgTag, dataSize, msg));
}

/** Send MIDI Message From Delegate */
export function SMMFD(status: number, data1: number, data2: number): void {
  callbacks.midiMsg.forEach(cb => cb(status, data1, data2));
}

/** Send Arbitrary Message From Delegate */
export function SAMFD(msgTag: number, dataSize: number, msg: ArrayBuffer | string | null): void {
  callbacks.arbitraryMsg.forEach(cb => cb(msgTag, dataSize, msg));
}

// Legacy alias
export const OnParamChange = SPVFD;

// ============================================================================
// Global exposure (for WebView bridge and IPlugController callbacks)
// ============================================================================

// Make message handlers globally available
globalThis.SPVFD = SPVFD;
globalThis.SCVFD = SCVFD;
globalThis.SCMFD = SCMFD;
globalThis.SMMFD = SMMFD;
globalThis.SAMFD = SAMFD;
globalThis.OnParamChange = OnParamChange;

// UI functions (not needed globally for WASM, but kept for WebView compatibility)
globalThis.SPVFUI = SPVFUI;
globalThis.BPCFUI = BPCFUI;
globalThis.EPCFUI = EPCFUI; 
