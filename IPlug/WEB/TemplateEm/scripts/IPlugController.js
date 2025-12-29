/**
 * IPlugController - Simplified AudioWorklet controller for iPlug2
 * Uses Emscripten's native AudioWorklet support instead of WAM SDK.
 *
 * This controller manages the UI-side communication with the audio worklet.
 *
 * @example Event emitter pattern (recommended):
 * ```javascript
 * const controller = new IPlugController('MyPlugin');
 * controller.on('paramChange', ({ paramIdx, value }) => {
 *   console.log(`Parameter ${paramIdx} = ${value}`);
 * });
 * await controller.init();
 * controller.startAudio();
 * ```
 *
 * @example Legacy callback pattern:
 * ```javascript
 * const controller = new IPlugController('MyPlugin');
 * controller.onParamChange = (paramIdx, value) => { ... };
 * await controller.init();
 * ```
 */

class IPlugController {
  /**
   * Create a new IPlugController instance
   * @param {string} pluginName - Plugin name for identification
   * @param {Object} [moduleRef] - Optional Module reference for isolated instances
   */
  constructor(pluginName, moduleRef) {
    this.pluginName = pluginName;
    this.instanceId = crypto.randomUUID();
    this.audioContext = null;
    this.isReady = false;
    this.onReadyCallback = null;
    this.idleTimerId = null;

    // Module reference (use global Module by default, or isolated instance)
    this._module = moduleRef || (typeof Module !== 'undefined' ? Module : null);

    // Event emitter storage
    this._listeners = new Map();

    // Legacy message handlers from DSP -> UI
    this.onParamChange = null;
    this.onControlValue = null;
    this.onControlMsg = null;
    this.onMidiMsg = null;
    this.onArbitraryMsg = null;
  }

  // ===========================================================================
  // Event Emitter API
  // ===========================================================================

  /**
   * Subscribe to an event
   * @param {string} event - Event name ('ready', 'paramChange', 'controlValue', 'controlMsg', 'midiMsg', 'arbitraryMsg', 'stateChange')
   * @param {Function} listener - Callback function
   * @returns {Function} Unsubscribe function
   */
  on(event, listener) {
    if (!this._listeners.has(event)) {
      this._listeners.set(event, new Set());
    }
    this._listeners.get(event).add(listener);
    return () => this.off(event, listener);
  }

  /**
   * Unsubscribe from an event
   * @param {string} event - Event name
   * @param {Function} listener - Callback function to remove
   */
  off(event, listener) {
    const listeners = this._listeners.get(event);
    if (listeners) {
      listeners.delete(listener);
    }
  }

  /**
   * Emit an event to all listeners
   * @param {string} event - Event name
   * @param {*} data - Event data
   */
  emit(event, data) {
    const listeners = this._listeners.get(event);
    if (listeners) {
      listeners.forEach(listener => {
        try {
          listener(data);
        } catch (err) {
          console.error(`IPlugController: Error in ${event} listener:`, err);
        }
      });
    }
  }

  /**
   * Initialize the plugin - loads the WASM module and sets up audio
   * @param {Object} options - Configuration options
   * @param {number} options.sampleRate - Desired sample rate (0 = default)
   * @returns {Promise} Resolves when ready
   */
  async init(options = {}) {
    const mod = this._module;
    if (!mod) {
      throw new Error('IPlugController: Module not available');
    }

    return new Promise((resolve, reject) => {
      // Set up callbacks that the C++ code will use to notify us
      mod.onAudioWorkletReady = () => {
        this.isReady = true;
        this.audioContext = mod.audioContext;
        this._startIdleTimer();

        // Emit ready event
        this.emit('ready', undefined);
        if (this.onReadyCallback) this.onReadyCallback();
        resolve();
      };

      // Set up delegate message handlers with both event emitter and legacy callbacks
      mod.SPVFD = (paramIdx, value) => {
        this.emit('paramChange', { paramIdx, value });
        if (this.onParamChange) this.onParamChange(paramIdx, value);
      };

      mod.SCVFD = (ctrlTag, normalizedValue) => {
        this.emit('controlValue', { ctrlTag, value: normalizedValue });
        if (this.onControlValue) this.onControlValue(ctrlTag, normalizedValue);
      };

      mod.SCMFD = (ctrlTag, msgTag, data) => {
        this.emit('controlMsg', { ctrlTag, msgTag, data });
        if (this.onControlMsg) this.onControlMsg(ctrlTag, msgTag, data);
      };

      mod.SMMFD = (status, data1, data2) => {
        this.emit('midiMsg', { status, data1, data2 });
        if (this.onMidiMsg) this.onMidiMsg(status, data1, data2);
      };

      mod.SAMFD = (msgTag, data) => {
        this.emit('arbitraryMsg', { msgTag, data });
        if (this.onArbitraryMsg) this.onArbitraryMsg(msgTag, data);
      };

      // Initialize via main() which sets up the audio worklet
      if (mod.calledRun) {
        // Module already initialized, just trigger audio init
        if (mod._initAudioWorklet) {
          mod._initAudioWorklet(options.sampleRate || 0);
        }
      }
      // Otherwise, Module will call main() which handles initialization
    });
  }

  /**
   * Start audio processing (call after user gesture)
   */
  startAudio() {
    const mod = this._module;
    if (mod && mod.startAudio) {
      mod.startAudio();
    }
  }

  /**
   * Stop audio processing
   */
  stopAudio() {
    const mod = this._module;
    if (mod && mod.stopAudio) {
      mod.stopAudio();
    }
  }

  // ===========================================================================
  // Parameter Control
  // ===========================================================================

  /**
   * Set a parameter value from the UI
   * @param {number} paramIdx - Parameter index
   * @param {number} value - Parameter value (non-normalized)
   */
  setParam(paramIdx, value) {
    const mod = this._module;
    if (mod && mod.setParam) {
      mod.setParam(paramIdx, value);
    }
  }

  /**
   * Begin a parameter change gesture (for DAW automation recording)
   * Call this when the user starts dragging a control
   * @param {number} paramIdx - Parameter index
   */
  beginParamChange(paramIdx) {
    const mod = this._module;
    if (mod && mod.beginParamChange) {
      mod.beginParamChange(paramIdx);
    }
  }

  /**
   * End a parameter change gesture
   * Call this when the user releases a control
   * @param {number} paramIdx - Parameter index
   */
  endParamChange(paramIdx) {
    const mod = this._module;
    if (mod && mod.endParamChange) {
      mod.endParamChange(paramIdx);
    }
  }

  // ===========================================================================
  // MIDI
  // ===========================================================================

  /**
   * Send a MIDI message from the UI
   * @param {number} status - MIDI status byte
   * @param {number} data1 - MIDI data byte 1
   * @param {number} data2 - MIDI data byte 2
   */
  sendMidi(status, data1, data2) {
    const mod = this._module;
    if (mod && mod.sendMidi) {
      mod.sendMidi(status, data1, data2);
    }
  }

  /**
   * Send an arbitrary message to the DSP
   * @param {string} verb - Message type identifier
   * @param {string} res - Resource/property identifier
   * @param {*} data - Message data
   */
  sendMessage(verb, res, data) {
    const mod = this._module;
    // For compatibility with existing code patterns
    if (verb === "SMMFUI") {
      const parts = res.split(":");
      this.sendMidi(parseInt(parts[0]), parseInt(parts[1]), parseInt(parts[2]));
    } else if (verb === "TICK") {
      if (mod && mod.onIdleTick) mod.onIdleTick();
    }
    // Add other message types as needed
  }

  /**
   * Resume audio context (required after user gesture in some browsers)
   * @returns {Promise}
   */
  async resumeAudio() {
    if (this.audioContext && this.audioContext.state === 'suspended') {
      await this.audioContext.resume();
    }
  }

  /**
   * Get the current audio context state
   * @returns {string} Audio context state
   */
  getAudioState() {
    return this.audioContext ? this.audioContext.state : 'closed';
  }

  _startIdleTimer() {
    // Start a timer to pump idle messages to the DSP
    // Using requestAnimationFrame for smooth ~60fps updates
    if (this.idleTimerId) cancelAnimationFrame(this.idleTimerId);
    const mod = this._module;
    const tick = () => {
      if (mod && mod.onIdleTick) mod.onIdleTick();
      this.idleTimerId = requestAnimationFrame(tick);
    };
    this.idleTimerId = requestAnimationFrame(tick);
  }

  _stopIdleTimer() {
    if (this.idleTimerId) {
      cancelAnimationFrame(this.idleTimerId);
      this.idleTimerId = null;
    }
  }

  /**
   * Clean up resources
   */
  destroy() {
    this._stopIdleTimer();
    this.stopAudio();
    if (this.audioContext) {
      this.audioContext.close();
      this.audioContext = null;
    }
  }
}

/**
 * IPlugWebComponent - Base class for creating iPlug2 web components with Shadow DOM
 *
 * @example
 * ```javascript
 * class MyPluginElement extends IPlugWebComponent {
 *   get pluginName() { return 'MyPlugin'; }
 *   get wasmPath() { return './MyPlugin.js'; }
 * }
 * customElements.define('my-plugin', MyPluginElement);
 *
 * // Usage in HTML:
 * <my-plugin></my-plugin>
 * ```
 */
class IPlugWebComponent extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({ mode: 'open' });
    this._module = null;
    this._controller = null;
    this._canvas = null;
  }

  /** Override to return your plugin name */
  get pluginName() {
    return 'IPlugPlugin';
  }

  /** Override to return path to the WASM JS file */
  get wasmPath() {
    return `./${this.pluginName}.js`;
  }

  async connectedCallback() {
    // Create canvas in shadow DOM
    this._canvas = document.createElement('canvas');
    this._canvas.style.width = '100%';
    this._canvas.style.height = '100%';
    this._canvas.oncontextmenu = e => e.preventDefault();
    this.shadowRoot.appendChild(this._canvas);

    // Load WASM module with isolated Module instance
    await this._loadModule();
  }

  disconnectedCallback() {
    if (this._controller) {
      this._controller.destroy();
    }
  }

  async _loadModule() {
    // Create isolated Module instance for this web component
    const shadowRoot = this.shadowRoot;
    const canvas = this._canvas;

    // Module configuration - isolated per instance
    this._module = {
      canvas: canvas,
      // Shadow DOM canvas getter for C++ code
      getCanvas: () => canvas,
      shadowDOMRoot: shadowRoot,
      shadowDOMContainer: shadowRoot,
      printErr: text => console.error(`[${this.pluginName}] stderr:`, text),
      print: text => console.log(`[${this.pluginName}] stdout:`, text),
    };

    // Load the WASM script
    const script = document.createElement('script');
    script.src = this.wasmPath;

    // Make Module available to the script
    const originalModule = window.Module;
    window.Module = this._module;

    await new Promise((resolve, reject) => {
      script.onload = resolve;
      script.onerror = reject;
      document.head.appendChild(script);
    });

    // Restore original Module
    if (originalModule) {
      window.Module = originalModule;
    }

    // Create controller for this instance
    this._controller = new IPlugController(this.pluginName, this._module);
    await this._controller.init();
  }

  /** Get the controller instance for this plugin */
  get controller() {
    return this._controller;
  }
}

// Export for use as global or module
if (typeof window !== 'undefined') {
  window.IPlugController = IPlugController;
  window.IPlugWebComponent = IPlugWebComponent;
}