/**
 * Emscripten Module Types for iPlug2 EMAudioWorklet
 *
 * This file augments the base Emscripten Module type with iPlug2-specific
 * bindings and callbacks.
 */

/**
 * Base Emscripten Module interface
 * This covers common Emscripten runtime methods
 */
export interface EmscriptenModule {
  /** Whether the runtime has been initialized */
  calledRun?: boolean;

  /** Allocate memory on the WASM heap */
  _malloc(size: number): number;

  /** Free memory on the WASM heap */
  _free(ptr: number): void;

  /** Call a C function by name */
  ccall(
    ident: string,
    returnType: string | null,
    argTypes: string[],
    args: unknown[],
    opts?: { async?: boolean }
  ): unknown;

  /** Create a wrapped C function */
  cwrap(
    ident: string,
    returnType: string | null,
    argTypes: string[]
  ): (...args: unknown[]) => unknown;

  /** Set a value at a memory location */
  setValue(ptr: number, value: number, type: string): void;

  /** Convert a C string pointer to JavaScript string */
  UTF8ToString(ptr: number, maxBytesToRead?: number): string;

  /** Locate a file for loading */
  locateFile?(path: string, scriptDirectory: string): string;

  /** Called when runtime is initialized */
  onRuntimeInitialized?(): void;

  /** Post-run callbacks */
  postRun?: Array<() => void>;

  /** Pre-run callbacks */
  preRun?: Array<() => void>;

  /** Canvas element for IGraphics rendering */
  canvas?: HTMLCanvasElement;

  /** Print function for stdout */
  print?(text: string): void;

  /** Print function for stderr */
  printErr?(text: string): void;
}

/**
 * iPlug2 EMAudioWorklet Module interface
 * Extends the base Emscripten module with iPlug2-specific bindings
 */
export interface IPlugEmModule extends EmscriptenModule {
  // =========================================================================
  // Audio Control (exported from C++)
  // =========================================================================

  /** Start audio processing (connects worklet to destination) */
  startAudio(): void;

  /** Stop audio processing */
  stopAudio(): void;

  // =========================================================================
  // Parameter Control (exported from C++)
  // =========================================================================

  /** Set a parameter value from the UI */
  setParam(paramIdx: number, value: number): void;

  /** Begin parameter change gesture (for DAW automation) */
  beginParamChange?(paramIdx: number): void;

  /** End parameter change gesture */
  endParamChange?(paramIdx: number): void;

  // =========================================================================
  // MIDI (exported from C++)
  // =========================================================================

  /** Send a MIDI message from the UI */
  sendMidi(status: number, data1: number, data2: number): void;

  // =========================================================================
  // Idle/Update (exported from C++)
  // =========================================================================

  /** Called on idle tick to flush DSP->UI message queues */
  onIdleTick(): void;

  // =========================================================================
  // Audio Context Access
  // =========================================================================

  /** Get the Emscripten handle for the AudioContext */
  getAudioContext(): number;

  /** Get the Emscripten handle for the AudioWorkletNode */
  getWorkletNode(): number;

  /** The actual AudioContext object (set after creation) */
  audioContext?: AudioContext;

  /** The actual AudioWorkletNode object (set after creation) */
  workletNode?: AudioWorkletNode;

  // =========================================================================
  // Lifecycle Callbacks (set by JavaScript)
  // =========================================================================

  /** Called when audio worklet is ready */
  onAudioWorkletReady?: () => void;

  // =========================================================================
  // Delegate Message Callbacks (C++ -> JavaScript)
  // Set these to receive messages from the DSP
  // =========================================================================

  /** Send Parameter Value From Delegate */
  SPVFD?: (paramIdx: number, value: number) => void;

  /** Send Control Value From Delegate (for meters, visualizers) */
  SCVFD?: (ctrlTag: number, normalizedValue: number) => void;

  /** Send Control Message From Delegate (for complex data) */
  SCMFD?: (ctrlTag: number, msgTag: number, data: ArrayBuffer | string) => void;

  /** Send MIDI Message From Delegate */
  SMMFD?: (status: number, data1: number, data2: number) => void;

  /** Send Arbitrary Message From Delegate */
  SAMFD?: (msgTag: number, data: ArrayBuffer | string) => void;

  // =========================================================================
  // Filesystem
  // =========================================================================

  /** Initialize audio worklet with sample rate */
  _initAudioWorklet?(sampleRate: number): void;

  /** Signal that filesystem is ready */
  _iplug_fsready?(): void;

  /** Sync filesystem */
  _iplug_syncfs?(): void;
}

/**
 * Factory function type for creating isolated Module instances
 * Used for multi-instance web component support
 */
export type IPlugModuleFactory = (
  moduleOverrides?: Partial<IPlugEmModule>
) => Promise<IPlugEmModule>;

/**
 * Global Module declaration
 * Augments the global scope when using the traditional Emscripten pattern
 */
declare global {
  // eslint-disable-next-line no-var
  var Module: IPlugEmModule;
}
