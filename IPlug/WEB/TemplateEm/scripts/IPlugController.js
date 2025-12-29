/**
 * IPlugController - Simplified AudioWorklet controller for iPlug2
 * Uses Emscripten's native AudioWorklet support instead of WAM SDK.
 *
 * This controller manages the UI-side communication with the audio worklet.
 */

class IPlugController {
  constructor(pluginName) {
    this.pluginName = pluginName;
    this.audioContext = null;
    this.isReady = false;
    this.onReadyCallback = null;
    this.idleTimerId = null;

    // Message handlers from DSP -> UI
    this.onParamChange = null;
    this.onControlValue = null;
    this.onControlMsg = null;
    this.onMidiMsg = null;
    this.onArbitraryMsg = null;
  }

  /**
   * Initialize the plugin - loads the WASM module and sets up audio
   * @param {Object} options - Configuration options
   * @param {number} options.sampleRate - Desired sample rate (0 = default)
   * @returns {Promise} Resolves when ready
   */
  async init(options = {}) {
    return new Promise((resolve, reject) => {
      // Set up callbacks that the C++ code will use to notify us
      Module.onAudioWorkletReady = () => {
        this.isReady = true;
        this.audioContext = Module.getAudioContext();
        this._startIdleTimer();
        if (this.onReadyCallback) this.onReadyCallback();
        resolve();
      };

      // Set up delegate message handlers
      Module.SPVFD = (paramIdx, value) => {
        if (this.onParamChange) this.onParamChange(paramIdx, value);
      };

      Module.SCVFD = (ctrlTag, normalizedValue) => {
        if (this.onControlValue) this.onControlValue(ctrlTag, normalizedValue);
      };

      Module.SCMFD = (ctrlTag, msgTag, data) => {
        if (this.onControlMsg) this.onControlMsg(ctrlTag, msgTag, data);
      };

      Module.SMMFD = (status, data1, data2) => {
        if (this.onMidiMsg) this.onMidiMsg(status, data1, data2);
      };

      Module.SAMFD = (msgTag, data) => {
        if (this.onArbitraryMsg) this.onArbitraryMsg(msgTag, data);
      };

      // Initialize via main() which sets up the audio worklet
      if (Module.calledRun) {
        // Module already initialized, just trigger audio init
        if (Module._initAudioWorklet) {
          Module._initAudioWorklet(options.sampleRate || 0);
        }
      }
      // Otherwise, Module will call main() which handles initialization
    });
  }

  /**
   * Start audio processing (call after user gesture)
   */
  startAudio() {
    if (Module.startAudio) {
      Module.startAudio();
    }
  }

  /**
   * Stop audio processing
   */
  stopAudio() {
    if (Module.stopAudio) {
      Module.stopAudio();
    }
  }

  /**
   * Set a parameter value from the UI
   * @param {number} paramIdx - Parameter index
   * @param {number} value - Parameter value (non-normalized)
   */
  setParam(paramIdx, value) {
    if (Module.setParam) {
      Module.setParam(paramIdx, value);
    }
  }

  /**
   * Send a MIDI message from the UI
   * @param {number} status - MIDI status byte
   * @param {number} data1 - MIDI data byte 1
   * @param {number} data2 - MIDI data byte 2
   */
  sendMidi(status, data1, data2) {
    if (Module.sendMidi) {
      Module.sendMidi(status, data1, data2);
    }
  }

  /**
   * Send an arbitrary message to the DSP
   * @param {string} verb - Message type identifier
   * @param {string} res - Resource/property identifier
   * @param {*} data - Message data
   */
  sendMessage(verb, res, data) {
    // For compatibility with existing code patterns
    if (verb === "SMMFUI") {
      const parts = res.split(":");
      this.sendMidi(parseInt(parts[0]), parseInt(parts[1]), parseInt(parts[2]));
    } else if (verb === "TICK") {
      if (Module.onIdleTick) Module.onIdleTick();
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
    const tick = () => {
      if (Module.onIdleTick) Module.onIdleTick();
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

// Export for use as global or module
if (typeof window !== 'undefined') {
  window.IPlugController = IPlugController;
}
