/**
 * iPlug2 EMAudioWorklet Message Types
 *
 * These types define the communication protocol between the JavaScript
 * controller (main thread) and the WASM module (via Emscripten bindings).
 *
 * Message naming convention:
 * - UI suffix = From UI to DSP/Processor
 * - FD suffix = From Delegate (DSP) to UI
 */

// ============================================================================
// Message Type Identifiers
// ============================================================================

/** Messages from UI to DSP (processor) */
export type UIMessageType =
  | 'SPVFUI' // Send Parameter Value From UI
  | 'BPCFUI' // Begin Parameter Change From UI
  | 'EPCFUI' // End Parameter Change From UI
  | 'SMMFUI' // Send MIDI Message From UI
  | 'SSMFUI' // Send Sysex Message From UI
  | 'SAMFUI'; // Send Arbitrary Message From UI

/** Messages from DSP (delegate) to UI */
export type DelegateMessageType =
  | 'SPVFD' // Send Parameter Value From Delegate
  | 'SCVFD' // Send Control Value From Delegate
  | 'SCMFD' // Send Control Message From Delegate
  | 'SMMFD' // Send MIDI Message From Delegate
  | 'SSMFD' // Send Sysex Message From Delegate
  | 'SAMFD'; // Send Arbitrary Message From Delegate

// ============================================================================
// UI -> DSP Messages (via Module.setParam, Module.sendMidi, etc.)
// ============================================================================

/** Parameter value change from UI */
export interface ParamValueFromUI {
  paramIdx: number;
  value: number; // Non-normalized value
}

/** Parameter gesture begin from UI (for DAW automation recording) */
export interface BeginParamChangeFromUI {
  paramIdx: number;
}

/** Parameter gesture end from UI */
export interface EndParamChangeFromUI {
  paramIdx: number;
}

/** MIDI message from UI */
export interface MidiMsgFromUI {
  status: number;
  data1: number;
  data2: number;
}

/** Sysex message from UI */
export interface SysexMsgFromUI {
  data: Uint8Array;
}

/** Arbitrary message from UI */
export interface ArbitraryMsgFromUI {
  msgTag: number;
  ctrlTag?: number;
  data?: ArrayBuffer | Uint8Array;
}

// ============================================================================
// DSP -> UI Messages (via Module callbacks)
// ============================================================================

/** Parameter value from delegate */
export interface ParamValueFromDelegate {
  paramIdx: number;
  value: number; // Normalized value (0-1)
}

/** Control value from delegate (for ISender data like meters) */
export interface ControlValueFromDelegate {
  ctrlTag: number;
  value: number; // Normalized value (0-1)
}

/** Control message from delegate (for complex data like waveforms, FFT) */
export interface ControlMsgFromDelegate {
  ctrlTag: number;
  msgTag: number;
  dataSize: number;
  data: ArrayBuffer;
}

/** MIDI message from delegate */
export interface MidiMsgFromDelegate {
  status: number;
  data1: number;
  data2: number;
}

/** Sysex message from delegate */
export interface SysexMsgFromDelegate {
  size: number;
  data: Uint8Array;
}

/** Arbitrary message from delegate */
export interface ArbitraryMsgFromDelegate {
  msgTag: number;
  dataSize: number;
  data: ArrayBuffer;
}

// ============================================================================
// Type Guards
// ============================================================================

export function isParamValue(msg: unknown): msg is ParamValueFromDelegate {
  return (
    typeof msg === 'object' &&
    msg !== null &&
    'paramIdx' in msg &&
    'value' in msg &&
    typeof (msg as ParamValueFromDelegate).paramIdx === 'number' &&
    typeof (msg as ParamValueFromDelegate).value === 'number'
  );
}

export function isControlValue(msg: unknown): msg is ControlValueFromDelegate {
  return (
    typeof msg === 'object' &&
    msg !== null &&
    'ctrlTag' in msg &&
    'value' in msg &&
    typeof (msg as ControlValueFromDelegate).ctrlTag === 'number' &&
    typeof (msg as ControlValueFromDelegate).value === 'number'
  );
}

export function isMidiMsg(msg: unknown): msg is MidiMsgFromDelegate {
  return (
    typeof msg === 'object' &&
    msg !== null &&
    'status' in msg &&
    'data1' in msg &&
    'data2' in msg
  );
}
