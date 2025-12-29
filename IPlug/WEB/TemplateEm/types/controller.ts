/**
 * IPlugController Type Definitions
 *
 * Types for the JavaScript controller that bridges the UI with the WASM module.
 */

import type { IPlugEmModule } from './module';

/**
 * Controller initialization options
 */
export interface ControllerInitOptions {
  /** Desired sample rate (0 = use browser default) */
  sampleRate?: number;

  /** Module overrides for isolated instances */
  moduleOverrides?: Partial<IPlugEmModule>;
}

/**
 * Event types emitted by the controller
 */
export interface ControllerEventMap {
  /** Audio worklet is ready */
  ready: void;

  /** Parameter value changed from DSP */
  paramChange: { paramIdx: number; value: number };

  /** Control value changed from DSP (meters, etc.) */
  controlValue: { ctrlTag: number; value: number };

  /** Control message from DSP (complex data) */
  controlMsg: { ctrlTag: number; msgTag: number; data: ArrayBuffer };

  /** MIDI message from DSP */
  midiMsg: { status: number; data1: number; data2: number };

  /** Arbitrary message from DSP */
  arbitraryMsg: { msgTag: number; data: ArrayBuffer };

  /** Audio context state changed */
  stateChange: { state: AudioContextState };
}

/**
 * Event listener callback type
 */
export type ControllerEventListener<K extends keyof ControllerEventMap> = (
  event: ControllerEventMap[K]
) => void;

/**
 * Legacy callback handlers for DSP -> UI messages
 * @deprecated Use event emitter pattern instead
 */
export interface ControllerCallbacks {
  /** Parameter value changed */
  onParamChange?: (paramIdx: number, value: number) => void;

  /** Control value changed */
  onControlValue?: (ctrlTag: number, normalizedValue: number) => void;

  /** Control message received */
  onControlMsg?: (ctrlTag: number, msgTag: number, data: ArrayBuffer | string) => void;

  /** MIDI message received */
  onMidiMsg?: (status: number, data1: number, data2: number) => void;

  /** Arbitrary message received */
  onArbitraryMsg?: (msgTag: number, data: ArrayBuffer | string) => void;
}

/**
 * Audio context state (matches Web Audio API)
 */
export type AudioContextState = 'suspended' | 'running' | 'closed';

/**
 * IPlugController interface
 */
export interface IPlugController extends ControllerCallbacks {
  /** Unique instance identifier */
  readonly instanceId: string;

  /** Plugin name */
  readonly pluginName: string;

  /** Audio context (available after init) */
  readonly audioContext: AudioContext | null;

  /** Whether the controller is ready */
  readonly isReady: boolean;

  // =========================================================================
  // Lifecycle
  // =========================================================================

  /**
   * Initialize the plugin
   * @param options Configuration options
   * @returns Promise that resolves when ready
   */
  init(options?: ControllerInitOptions): Promise<void>;

  /**
   * Clean up resources
   */
  destroy(): void;

  // =========================================================================
  // Audio Control
  // =========================================================================

  /**
   * Start audio processing (requires user gesture)
   */
  startAudio(): void;

  /**
   * Stop audio processing
   */
  stopAudio(): void;

  /**
   * Resume audio context (required after user gesture in some browsers)
   */
  resumeAudio(): Promise<void>;

  /**
   * Get the current audio context state
   */
  getAudioState(): AudioContextState;

  // =========================================================================
  // Parameter Control
  // =========================================================================

  /**
   * Set a parameter value
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

  // =========================================================================
  // MIDI
  // =========================================================================

  /**
   * Send a MIDI message
   * @param status MIDI status byte
   * @param data1 MIDI data byte 1
   * @param data2 MIDI data byte 2
   */
  sendMidi(status: number, data1: number, data2: number): void;

  // =========================================================================
  // Event Emitter
  // =========================================================================

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
   * Emit an event
   * @param event Event name
   * @param data Event data
   */
  emit<K extends keyof ControllerEventMap>(event: K, data: ControllerEventMap[K]): void;

  // =========================================================================
  // Legacy
  // =========================================================================

  /**
   * Callback when ready
   * @deprecated Use on('ready', ...) instead
   */
  onReadyCallback?: () => void;
}

/**
 * IPlugController constructor type
 */
export interface IPlugControllerConstructor {
  new (pluginName: string, module?: IPlugEmModule): IPlugController;
}
