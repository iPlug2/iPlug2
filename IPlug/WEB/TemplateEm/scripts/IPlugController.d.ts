/**
 * Type declarations for IPlugController.js
 *
 * Ambient module declaration for TypeScript projects using the JavaScript controller.
 */

import type {
  ControllerInitOptions,
  ControllerCallbacks,
  ControllerEventMap,
  ControllerEventListener,
  AudioContextState
} from '../types/controller';

/**
 * IPlugController - Simplified AudioWorklet controller for iPlug2
 *
 * Manages UI-side communication with the WASM audio worklet.
 *
 * @example
 * ```typescript
 * const controller = new IPlugController('MyPlugin');
 * controller.onParamChange = (idx, val) => console.log(`Param ${idx} = ${val}`);
 * await controller.init({ sampleRate: 48000 });
 * controller.startAudio();
 * ```
 */
declare class IPlugController implements ControllerCallbacks {
  /** Unique instance identifier */
  readonly instanceId: string;

  /** Plugin name */
  readonly pluginName: string;

  /** Audio context (available after init) */
  readonly audioContext: AudioContext | null;

  /** Whether the controller is initialized and ready */
  readonly isReady: boolean;

  // Legacy callback properties
  onParamChange: ((paramIdx: number, value: number) => void) | null;
  onControlValue: ((ctrlTag: number, normalizedValue: number) => void) | null;
  onControlMsg: ((ctrlTag: number, msgTag: number, data: ArrayBuffer | string) => void) | null;
  onMidiMsg: ((status: number, data1: number, data2: number) => void) | null;
  onArbitraryMsg: ((msgTag: number, data: ArrayBuffer | string) => void) | null;
  onReadyCallback: (() => void) | null;

  /**
   * Create a new IPlugController instance
   * @param pluginName The plugin name (used for Module reference)
   */
  constructor(pluginName: string);

  /**
   * Initialize the controller and WASM module
   * @param options Initialization options
   * @returns Promise that resolves when ready
   */
  init(options?: ControllerInitOptions): Promise<void>;

  /**
   * Start audio processing (connects worklet to destination)
   * Must be called after a user gesture
   */
  startAudio(): void;

  /**
   * Stop audio processing
   */
  stopAudio(): void;

  /**
   * Set a parameter value from the UI
   * @param paramIdx Parameter index
   * @param value Parameter value (non-normalized)
   */
  setParam(paramIdx: number, value: number): void;

  /**
   * Begin parameter change gesture (for DAW automation recording)
   * @param paramIdx Parameter index
   */
  beginParamChange(paramIdx: number): void;

  /**
   * End parameter change gesture
   * @param paramIdx Parameter index
   */
  endParamChange(paramIdx: number): void;

  /**
   * Send a MIDI message to the DSP
   * @param status MIDI status byte
   * @param data1 MIDI data byte 1
   * @param data2 MIDI data byte 2
   */
  sendMidi(status: number, data1: number, data2: number): void;

  /**
   * Send a message (legacy compatibility)
   * @param verb Message type
   * @param res Resource identifier
   * @param data Message data
   */
  sendMessage(verb: string, res: string, data?: unknown): void;

  /**
   * Resume audio context after user gesture
   * @returns Promise that resolves when resumed
   */
  resumeAudio(): Promise<void>;

  /**
   * Get current audio context state
   */
  getAudioState(): AudioContextState;

  /**
   * Subscribe to an event
   * @param event Event name
   * @param listener Callback function
   * @returns Unsubscribe function
   */
  on<K extends keyof ControllerEventMap>(
    event: K,
    listener: ControllerEventListener<K>
  ): () => void;

  /**
   * Unsubscribe from an event
   * @param event Event name
   * @param listener Callback function
   */
  off<K extends keyof ControllerEventMap>(
    event: K,
    listener: ControllerEventListener<K>
  ): void;

  /**
   * Emit an event (internal use)
   */
  emit<K extends keyof ControllerEventMap>(event: K, data: ControllerEventMap[K]): void;

  /**
   * Clean up resources
   */
  destroy(): void;
}

export default IPlugController;
export { IPlugController };

// Global declaration for script tag usage
declare global {
  interface Window {
    IPlugController: typeof IPlugController;
  }
}
