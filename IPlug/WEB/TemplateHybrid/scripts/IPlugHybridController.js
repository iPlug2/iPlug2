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
   * Create a controller for DSP/UI communication
   * @param {string} pluginName - Plugin name (for script paths)
   * @param {Object} options - Configuration options
   * @param {Module} options.uiModule - Reference to UI WASM Module
   * @param {string} options.baseUrl - Base URL for loading scripts (default: '')
   */
  constructor(pluginName, options = {}) {
    this.pluginName = pluginName;
    this.pluginNameLC = pluginName.toLowerCase();
    this.uiModule = options.uiModule || options; // backwards compat: options can be uiModule directly
    if (this.uiModule === options) this.uiModule = null;
    this.baseUrl = options.baseUrl || '';
    this.audioContext = null;
    this.workletNode = null;
    this.isReady = false;
    this._ownsContext = false;

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
    this.pluginInfo = null; // Full plugin info from DSP (params, etc.)

    // SharedArrayBuffer for low-latency visualization data (optional)
    this.sabBuffer = null;
    this.sabDataView = null;
    this.sabReadIdx = null;
    this.sabCapacity = 0;
    this.usingSAB = false;
  }

  /**
   * Initialize the DSP worklet
   * @param {Object} options - Initialization options
   * @param {AudioContext} options.audioContext - Existing AudioContext (creates new if not provided)
   * @param {number} options.sampleRate - Desired sample rate (only used if creating new context)
   * @param {number} options.numInputChannels - Number of input channels
   * @param {number} options.numOutputChannels - Number of output channels
   * @param {boolean} options.isInstrument - Whether plugin is an instrument
   * @param {boolean} options.connectToOutput - Auto-connect to destination (default: true)
   * @param {boolean} options.useSAB - Use SharedArrayBuffer for visualization data (default: true if available)
   * @param {number} options.sabSize - SharedArrayBuffer size in bytes (default: 65536)
   * @returns {Promise<AudioWorkletNode>} The created worklet node
   */
  async init(options = {}) {
    this.numInputChannels = options.numInputChannels ?? 0;
    this.numOutputChannels = options.numOutputChannels ?? 2;
    this.isInstrument = options.isInstrument ?? false;
    const connectToOutput = options.connectToOutput ?? true;

    try {
      // Use provided AudioContext or create new one
      if (options.audioContext) {
        this.audioContext = options.audioContext;
        this._ownsContext = false;
      } else {
        const contextOptions = { latencyHint: 'interactive' };
        if (options.sampleRate && options.sampleRate > 0) {
          contextOptions.sampleRate = options.sampleRate;
        }
        this.audioContext = new AudioContext(contextOptions);
        this._ownsContext = true;
      }

      // Load the DSP module into AudioWorklet
      const baseUrl = this.baseUrl ? this.baseUrl.replace(/\/$/, '') + '/' : '';
      const dspModuleUrl = `${baseUrl}scripts/${this.pluginName}-dsp.js`;
      await this.audioContext.audioWorklet.addModule(dspModuleUrl);

      // Load our processor wrapper
      const processorUrl = `${baseUrl}scripts/${this.pluginName}-processor.js`;
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
      this.workletNode.port.start();

      // Connect to destination
      if (connectToOutput) {
        this.workletNode.connect(this.audioContext.destination);
      }

      // Try to set up SharedArrayBuffer for low-latency visualization data
      if (options.useSAB !== false && typeof SharedArrayBuffer !== 'undefined') {
        try {
          const sabSize = options.sabSize || 262144; // 256KB default (needed for FFT data)
          this.sabBuffer = new SharedArrayBuffer(sabSize);
          this.sabDataView = new DataView(this.sabBuffer);
          this.sabReadIdx = new Uint32Array(this.sabBuffer, 4, 1);
          this.sabCapacity = sabSize - 16; // subtract header size

          // Initialize header
          this.sabDataView.setUint32(0, 0, true);  // writeIdx = 0
          this.sabDataView.setUint32(4, 0, true);  // readIdx = 0
          this.sabDataView.setUint32(8, this.sabCapacity, true);  // capacity

          // Send SAB to processor
          this.workletNode.port.postMessage({
            type: 'attachSAB',
            sab: this.sabBuffer
          });

          this.usingSAB = true;
          console.log('IPlugHybridController: SharedArrayBuffer enabled for visualization data');
        } catch (e) {
          console.warn('IPlugHybridController: SharedArrayBuffer not available, using postMessage fallback:', e.message);
          this.usingSAB = false;
        }
      }

      this.isReady = true;

      // Start idle timer (also polls SAB if enabled)
      this._startIdleTimer();

      // Notify listeners
      this.emit('ready');
      if (this.onReadyCallback) this.onReadyCallback();

      return this.workletNode;
    } catch (err) {
      console.error('IPlugHybridController: init failed', err);
      throw err;
    }
  }

  /**
   * Connect UI module for bidirectional communication
   * @param {Module} uiModule - UI WASM Module
   */
  connectUI(uiModule) {
    this.uiModule = uiModule;
  }

  /**
   * Handle messages from DSP processor
   */
  _onDSPMessage(e) {
    const msg = e.data;
    console.log('Controller _onDSPMessage received:', msg);

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

      case 'pluginInfo':
        // Store plugin info from DSP (includes parameter metadata)
        console.log('Controller: Received pluginInfo from processor:', msg.data);
        this.pluginInfo = msg.data;
        this.emit('pluginInfo', msg.data);
        console.log('Controller: Emitted pluginInfo event');
        break;
    }
  }

  /**
   * Start idle timer (sends tick to DSP for parameter updates, polls SAB)
   */
  _startIdleTimer() {
    const tick = () => {
      if (this.workletNode) {
        this.workletNode.port.postMessage({ type: 'tick' });
      }

      // Poll SAB for visualization messages
      if (this.usingSAB) {
        this._pollSAB();
      }

      requestAnimationFrame(tick);
    };
    requestAnimationFrame(tick);
  }

  /**
   * Poll SharedArrayBuffer for visualization messages from DSP
   */
  _pollSAB() {
    if (!this.sabBuffer || !this.sabDataView) return;

    try {
      const headerSize = 16;
      const msgHeaderSize = 12;

      const writeIdx = Atomics.load(new Uint32Array(this.sabBuffer, 0, 1), 0);
      let readIdx = Atomics.load(this.sabReadIdx, 0);

      // Handle writer reset: if writeIdx < readIdx, reset readIdx to 0
      if (writeIdx < readIdx) {
        readIdx = 0;
      }

      // Process all available messages (from readIdx to writeIdx)
      while (readIdx < writeIdx) {
        const offset = headerSize + readIdx;

        // Bounds check
        if (offset + msgHeaderSize > this.sabBuffer.byteLength) break;

        // Read message header
        const msgType = this.sabDataView.getUint8(offset);
        const dataSize = this.sabDataView.getUint16(offset + 2, true);
        const ctrlTag = this.sabDataView.getInt32(offset + 4, true);
        const msgTag = this.sabDataView.getInt32(offset + 8, true);

        // Bounds check for payload
        if (offset + msgHeaderSize + dataSize > this.sabBuffer.byteLength) break;

        // Extract payload (if any)
        let payload = null;
        if (dataSize > 0) {
          payload = new Uint8Array(this.sabBuffer, offset + msgHeaderSize, dataSize);
        }

        // Route message based on type
        switch (msgType) {
          case 0: // SCVFD - Send Control Value From Delegate
            const value = payload && payload.length >= 4
              ? new DataView(payload.buffer, payload.byteOffset, 4).getFloat32(0, true)
              : 0;
            this.emit('controlValue', { ctrlTag, value });
            if (this.onControlValue) this.onControlValue(ctrlTag, value);
            if (this.uiModule?.SCVFD) this.uiModule.SCVFD(ctrlTag, value);
            break;

          case 1: // SCMFD - Send Control Message From Delegate
            this.emit('controlMsg', { ctrlTag, msgTag, data: payload });
            if (this.onControlMsg) this.onControlMsg(ctrlTag, msgTag, payload);
            if (this.uiModule?.SCMFD && this.uiModule?.HEAPU8 && payload) {
              const ptr = this.uiModule._malloc(payload.length);
              this.uiModule.HEAPU8.set(payload, ptr);
              this.uiModule.SCMFD(ctrlTag, msgTag, payload.length, ptr);
              this.uiModule._free(ptr);
            }
            break;

          case 2: // SAMFD - Send Arbitrary Message From Delegate
            this.emit('arbitraryMsg', { msgTag, data: payload });
            if (this.onArbitraryMsg) this.onArbitraryMsg(msgTag, payload);
            if (this.uiModule?.SAMFD && this.uiModule?.HEAPU8 && payload) {
              const ptr = this.uiModule._malloc(payload.length);
              this.uiModule.HEAPU8.set(payload, ptr);
              this.uiModule.SAMFD(msgTag, payload.length, ptr);
              this.uiModule._free(ptr);
            }
            break;
        }

        // Advance read index
        const alignedDataSize = (dataSize + 3) & ~3;
        const totalMsgSize = msgHeaderSize + alignedDataSize;
        readIdx += totalMsgSize;
      }

      // Update read index atomically
      Atomics.store(this.sabReadIdx, 0, readIdx);
    } catch (e) {
      console.error('_pollSAB error:', e);
    }
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
    // Only close AudioContext if we created it
    if (this._ownsContext) {
      this.audioContext?.close();
    }
    this._listeners.clear();
    this.isReady = false;
  }
}

// Export for module systems
if (typeof module !== 'undefined' && module.exports) {
  module.exports = IPlugHybridController;
}
