/**
 * IPlugHybridController - Manages communication between UI and DSP modules
 *
 * This controller:
 * - Creates the AudioContext and AudioWorkletNode
 * - Loads the DSP module into the AudioWorklet
 * - Routes messages between DSP and UI
 * - Provides a simple API for external integration
 */

class IPlugHybridController {
  /**
   * Create a new IPlugHybridController
   * @param {string} pluginName - The plugin name (used for script paths)
   * @param {Object} uiModule - Reference to the UI WASM Module (optional)
   */
  constructor(pluginName, uiModule = null) {
    this.pluginName = pluginName;
    this.pluginNameLC = pluginName.toLowerCase();
    this.uiModule = uiModule;
    this.audioContext = null;
    this.workletNode = null;
    this.isReady = false;

    // Event listeners
    this._listeners = new Map();

    // Legacy callbacks (for backwards compatibility)
    this.onParamChange = null;
    this.onControlValue = null;
    this.onControlMsg = null;
    this.onMidiMsg = null;
    this.onSysexMsg = null;
    this.onArbitraryMsg = null;
    this.onReadyCallback = null;

    // Plugin info (populated after init)
    this.numInputChannels = 0;
    this.numOutputChannels = 2;
    this.isInstrument = false;
  }

  /**
   * Initialize the audio system
   * @param {Object} options - Initialization options
   * @param {number} options.sampleRate - Desired sample rate (0 = use default)
   * @param {number} options.numInputChannels - Number of input channels
   * @param {number} options.numOutputChannels - Number of output channels
   * @param {boolean} options.isInstrument - Whether plugin is an instrument
   * @returns {Promise} Resolves when audio is ready
   */
  async init(options = {}) {
    this.numInputChannels = options.numInputChannels ?? 0;
    this.numOutputChannels = options.numOutputChannels ?? 2;
    this.isInstrument = options.isInstrument ?? false;

    return new Promise(async (resolve, reject) => {
      try {
        // Create AudioContext
        const contextOptions = {
          latencyHint: 'interactive'
        };
        if (options.sampleRate && options.sampleRate > 0) {
          contextOptions.sampleRate = options.sampleRate;
        }

        this.audioContext = new AudioContext(contextOptions);

        // Load the DSP module into AudioWorklet
        // The DSP module is a single JS file with embedded WASM (SINGLE_FILE=1)
        const dspModuleUrl = `scripts/${this.pluginName}-dsp.js`;
        await this.audioContext.audioWorklet.addModule(dspModuleUrl);

        // Load our processor wrapper
        const processorUrl = `scripts/${this.pluginName}-processor.js`;
        await this.audioContext.audioWorklet.addModule(processorUrl);

        // Create the AudioWorkletNode
        this.workletNode = new AudioWorkletNode(
          this.audioContext,
          `${this.pluginNameLC}-processor`,
          {
            numberOfInputs: this.numInputChannels > 0 ? 1 : 0,
            numberOfOutputs: 1,
            outputChannelCount: [this.numOutputChannels],
            processorOptions: {
              numInputChannels: this.numInputChannels,
              numOutputChannels: this.numOutputChannels,
              isInstrument: this.isInstrument
            }
          }
        );

        // Handle messages from DSP (via processor port)
        this.workletNode.port.onmessage = this._onDSPMessage.bind(this);

        // Connect to destination
        this.workletNode.connect(this.audioContext.destination);

        this.isReady = true;

        // Start idle timer
        this._startIdleTimer();

        // Notify listeners
        this.emit('ready');
        if (this.onReadyCallback) this.onReadyCallback();

        resolve();
      } catch (err) {
        console.error('IPlugHybridController: init failed', err);
        reject(err);
      }
    });
  }

  /**
   * Handle messages from DSP processor
   */
  _onDSPMessage(e) {
    const msg = e.data;

    switch (msg.verb) {
      case 'SPVFD': // Send Parameter Value From Delegate
        this.emit('paramChange', { paramIdx: msg.paramIdx, value: msg.value });
        if (this.onParamChange) this.onParamChange(msg.paramIdx, msg.value);
        // Forward to UI WASM module
        if (this.uiModule?.SPVFD) this.uiModule.SPVFD(msg.paramIdx, msg.value);
        break;

      case 'SCVFD': // Send Control Value From Delegate
        this.emit('controlValue', { ctrlTag: msg.ctrlTag, value: msg.value });
        if (this.onControlValue) this.onControlValue(msg.ctrlTag, msg.value);
        if (this.uiModule?.SCVFD) this.uiModule.SCVFD(msg.ctrlTag, msg.value);
        break;

      case 'SCMFD': // Send Control Message From Delegate
        this.emit('controlMsg', { ctrlTag: msg.ctrlTag, msgTag: msg.msgTag, data: msg.data });
        if (this.onControlMsg) this.onControlMsg(msg.ctrlTag, msg.msgTag, msg.data);
        if (this.uiModule?.SCMFD && msg.data) {
          const dataArray = new Uint8Array(msg.data);
          const ptr = this.uiModule._malloc(dataArray.length);
          this.uiModule.HEAPU8.set(dataArray, ptr);
          this.uiModule.SCMFD(msg.ctrlTag, msg.msgTag, dataArray.length, ptr);
          this.uiModule._free(ptr);
        }
        break;

      case 'SAMFD': // Send Arbitrary Message From Delegate
        this.emit('arbitraryMsg', { msgTag: msg.msgTag, data: msg.data });
        if (this.onArbitraryMsg) this.onArbitraryMsg(msg.msgTag, msg.data);
        if (this.uiModule?.SAMFD && msg.data) {
          const dataArray = new Uint8Array(msg.data);
          const ptr = this.uiModule._malloc(dataArray.length);
          this.uiModule.HEAPU8.set(dataArray, ptr);
          this.uiModule.SAMFD(msg.msgTag, dataArray.length, ptr);
          this.uiModule._free(ptr);
        }
        break;

      case 'SMMFD': // Send MIDI Message From Delegate (legacy format)
        // Parse "status:data1:data2" format
        if (typeof msg.data === 'string') {
          const parts = msg.data.split(':').map(Number);
          this.emit('midiMsg', { status: parts[0], data1: parts[1], data2: parts[2] });
          if (this.onMidiMsg) this.onMidiMsg(parts[0], parts[1], parts[2]);
          if (this.uiModule?.SMMFD) this.uiModule.SMMFD(parts[0], parts[1], parts[2]);
        }
        break;

      case 'SSMFD': // Send Sysex Message From Delegate
        this.emit('sysexMsg', { data: msg.data });
        if (this.onSysexMsg) this.onSysexMsg(msg.data);
        if (this.uiModule?.SSMFD && msg.data) {
          const dataArray = new Uint8Array(msg.data);
          const ptr = this.uiModule._malloc(dataArray.length);
          this.uiModule.HEAPU8.set(dataArray, ptr);
          this.uiModule.SSMFD(dataArray.length, ptr);
          this.uiModule._free(ptr);
        }
        break;

      case 'StartIdleTimer':
        // DSP is ready, notify UI to start idle timer
        if (this.uiModule?.StartIdleTimer) this.uiModule.StartIdleTimer();
        break;
    }
  }

  /**
   * Start idle timer (sends tick to DSP for parameter updates)
   */
  _startIdleTimer() {
    const tick = () => {
      if (this.workletNode) {
        this.workletNode.port.postMessage({ type: 'tick' });
      }
      requestAnimationFrame(tick);
    };
    requestAnimationFrame(tick);
  }

  // ----- UI -> DSP Communication -----

  /**
   * Set a parameter value
   * @param {number} paramIdx - Parameter index
   * @param {number} value - Parameter value (normalized 0-1)
   */
  setParam(paramIdx, value) {
    this.workletNode?.port.postMessage({
      type: 'param',
      paramIdx,
      value
    });
  }

  /**
   * Send a MIDI message
   * @param {number} status - MIDI status byte
   * @param {number} data1 - MIDI data1 byte
   * @param {number} data2 - MIDI data2 byte
   */
  sendMidi(status, data1, data2) {
    this.workletNode?.port.postMessage({
      type: 'midi',
      status,
      data1,
      data2
    });
  }

  /**
   * Send a SysEx message
   * @param {Uint8Array} data - SysEx data
   */
  sendSysex(data) {
    this.workletNode?.port.postMessage({
      type: 'sysex',
      data: data.buffer
    });
  }

  /**
   * Send an arbitrary message
   * @param {number} msgTag - Message tag
   * @param {number} ctrlTag - Control tag
   * @param {Uint8Array|null} data - Optional data
   */
  sendArbitraryMsg(msgTag, ctrlTag, data = null) {
    this.workletNode?.port.postMessage({
      type: 'arbitrary',
      msgTag,
      ctrlTag,
      data: data?.buffer ?? null
    });
  }

  /**
   * Send idle tick to DSP (called internally by idle timer)
   */
  sendIdleTick() {
    this.workletNode?.port.postMessage({ type: 'tick' });
  }

  // ----- Audio Control -----

  /**
   * Start audio processing (resume AudioContext)
   */
  startAudio() {
    this.audioContext?.resume();
  }

  /**
   * Stop audio processing (suspend AudioContext)
   */
  stopAudio() {
    this.audioContext?.suspend();
  }

  /**
   * Get the AudioContext
   * @returns {AudioContext}
   */
  getAudioContext() {
    return this.audioContext;
  }

  /**
   * Get the AudioWorkletNode
   * @returns {AudioWorkletNode}
   */
  getWorkletNode() {
    return this.workletNode;
  }

  /**
   * Connect the worklet node to another audio node
   * @param {AudioNode} destination - Destination node
   * @param {number} output - Output index (default 0)
   * @param {number} input - Input index (default 0)
   */
  connect(destination, output = 0, input = 0) {
    this.workletNode?.connect(destination, output, input);
  }

  /**
   * Disconnect the worklet node
   * @param {AudioNode} destination - Destination node (optional)
   */
  disconnect(destination = null) {
    if (destination) {
      this.workletNode?.disconnect(destination);
    } else {
      this.workletNode?.disconnect();
    }
  }

  // ----- Event Emitter -----

  /**
   * Register an event listener
   * @param {string} event - Event name
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
   * Remove an event listener
   * @param {string} event - Event name
   * @param {Function} listener - Callback function
   */
  off(event, listener) {
    this._listeners.get(event)?.delete(listener);
  }

  /**
   * Emit an event
   * @param {string} event - Event name
   * @param {any} data - Event data
   */
  emit(event, data) {
    this._listeners.get(event)?.forEach(fn => fn(data));
  }

  /**
   * Destroy the controller and clean up resources
   */
  destroy() {
    this.workletNode?.disconnect();
    this.audioContext?.close();
    this._listeners.clear();
    this.isReady = false;
  }
}

// Export for module systems
if (typeof module !== 'undefined' && module.exports) {
  module.exports = IPlugHybridController;
}
