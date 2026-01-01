/**
 * PluginChainManager - Manages multiple iPlug2 WASM plugins in an audio chain
 *
 * This class handles:
 * - Loading multiple plugin bundles dynamically
 * - Shared AudioContext management
 * - Audio routing between plugins (serial chain or custom graph)
 * - Unified MIDI routing
 * - Master output metering
 *
 * Usage:
 *   const chain = new PluginChainManager();
 *   await chain.init();
 *   await chain.loadPlugin('IPlugInstrument', { position: 0 });
 *   await chain.loadPlugin('IPlugEffect', { position: 1 });
 *   chain.connectChain();
 *   chain.start();
 */

class PluginChainManager {
  constructor(options = {}) {
    this.audioContext = null;
    this.plugins = new Map();  // id -> PluginInstance
    this.chain = [];           // Ordered array of plugin IDs for serial chain
    this.masterGain = null;
    this.masterAnalyserL = null;
    this.masterAnalyserR = null;
    this.isRunning = false;
    this._listeners = new Map();

    // Options
    this.sampleRate = options.sampleRate || 0;  // 0 = browser default
    this.latencyHint = options.latencyHint || 'interactive';
    this.baseUrl = options.baseUrl || '';
  }

  /**
   * Initialize the chain manager with a shared AudioContext
   */
  async init() {
    const contextOptions = { latencyHint: this.latencyHint };
    if (this.sampleRate > 0) {
      contextOptions.sampleRate = this.sampleRate;
    }

    this.audioContext = new AudioContext(contextOptions);

    // Create master output section
    this.masterGain = this.audioContext.createGain();
    this.masterGain.gain.value = 1.0;

    // Master metering (stereo split)
    const splitter = this.audioContext.createChannelSplitter(2);
    this.masterGain.connect(splitter);

    this.masterAnalyserL = this.audioContext.createAnalyser();
    this.masterAnalyserL.fftSize = 256;
    this.masterAnalyserL.smoothingTimeConstant = 0.8;
    splitter.connect(this.masterAnalyserL, 0);

    this.masterAnalyserR = this.audioContext.createAnalyser();
    this.masterAnalyserR.fftSize = 256;
    this.masterAnalyserR.smoothingTimeConstant = 0.8;
    splitter.connect(this.masterAnalyserR, 1);

    // Connect master to destination
    this.masterGain.connect(this.audioContext.destination);

    this.emit('init', { audioContext: this.audioContext });
    return this.audioContext;
  }

  /**
   * Load a plugin into the chain
   * @param {string} pluginName - Plugin name (must match bundle name)
   * @param {Object} options - Load options
   * @param {number} options.position - Position in chain (default: end)
   * @param {string} options.id - Custom ID (default: pluginName_timestamp)
   * @param {HTMLElement} options.container - Container element for UI (optional)
   * @returns {Promise<PluginInstance>}
   */
  async loadPlugin(pluginName, options = {}) {
    const id = options.id || `${pluginName}_${Date.now()}`;
    const position = options.position ?? this.chain.length;

    // Load the plugin bundle script if not already loaded
    await this._loadPluginBundle(pluginName);

    // Get the controller class (set by bundle)
    const ControllerClass = window.IPlugWasmController;
    if (!ControllerClass) {
      throw new Error(`Plugin bundle for ${pluginName} did not register IPlugWasmController`);
    }

    // Create controller instance
    const controller = new ControllerClass(pluginName, {
      baseUrl: this.baseUrl ? `${this.baseUrl}${pluginName}/` : `${pluginName}/`
    });

    // Get plugin info from bundle globals (set during bundle load)
    const bundleInfo = window[`${pluginName}_BundleInfo`] || {};
    const numInputs = bundleInfo.numInputs ?? 2;
    const numOutputs = bundleInfo.numOutputs ?? 2;
    const isInstrument = bundleInfo.isInstrument ?? false;

    // Initialize DSP worklet
    await controller.init({
      audioContext: this.audioContext,
      numInputChannels: numInputs,
      numOutputChannels: numOutputs,
      isInstrument: isInstrument,
      connectToOutput: false  // We manage routing ourselves
    });

    // Create bypass gain node for this plugin
    const bypassGain = this.audioContext.createGain();
    bypassGain.gain.value = 1.0;

    // Create plugin instance wrapper
    const instance = {
      id,
      pluginName,
      controller,
      workletNode: controller.getWorkletNode(),
      bypassGain,
      bypassed: false,
      numInputs,
      numOutputs,
      isInstrument,
      uiElement: null  // Set later if UI is created
    };

    // Store instance
    this.plugins.set(id, instance);

    // Insert into chain at position
    this.chain.splice(position, 0, id);

    // Create UI element if container provided
    if (options.container) {
      await this._createPluginUI(instance, options.container);
    }

    this.emit('pluginLoaded', { id, pluginName, instance });

    return instance;
  }

  /**
   * Load plugin bundle script dynamically
   */
  async _loadPluginBundle(pluginName) {
    // Check if already loaded
    if (window[`${pluginName}_BundleLoaded`]) {
      return;
    }

    const bundleUrl = this.baseUrl
      ? `${this.baseUrl}${pluginName}/scripts/${pluginName}-bundle.js`
      : `${pluginName}/scripts/${pluginName}-bundle.js`;

    await new Promise((resolve, reject) => {
      const script = document.createElement('script');
      script.src = bundleUrl;
      script.onload = () => {
        window[`${pluginName}_BundleLoaded`] = true;
        resolve();
      };
      script.onerror = () => reject(new Error(`Failed to load bundle: ${bundleUrl}`));
      document.head.appendChild(script);
    });
  }

  /**
   * Create UI element for a plugin instance
   */
  async _createPluginUI(instance, container) {
    // Create the web component element
    const tagName = `iplug-${instance.pluginName.toLowerCase()}`;
    const element = document.createElement(tagName);
    element.dataset.instanceId = instance.id;

    container.appendChild(element);

    // Wait for UI ready
    await new Promise(resolve => {
      element.addEventListener('uiready', resolve, { once: true });
    });

    // Connect controller to UI
    element.connectController(instance.controller);
    instance.uiElement = element;

    return element;
  }

  /**
   * Remove a plugin from the chain
   * @param {string} id - Plugin instance ID
   */
  removePlugin(id) {
    const instance = this.plugins.get(id);
    if (!instance) return;

    // Disconnect from audio graph
    instance.workletNode.disconnect();
    instance.bypassGain.disconnect();

    // Remove UI element
    if (instance.uiElement) {
      instance.uiElement.remove();
    }

    // Destroy controller
    instance.controller.destroy();

    // Remove from chain and map
    const chainIdx = this.chain.indexOf(id);
    if (chainIdx >= 0) {
      this.chain.splice(chainIdx, 1);
    }
    this.plugins.delete(id);

    // Reconnect remaining chain
    if (this.isRunning) {
      this.connectChain();
    }

    this.emit('pluginRemoved', { id });
  }

  /**
   * Connect plugins in serial chain order
   * Call this after loading all plugins or when chain order changes
   */
  connectChain() {
    // Disconnect all existing connections
    for (const [id, instance] of this.plugins) {
      instance.workletNode.disconnect();
      instance.bypassGain.disconnect();
    }

    if (this.chain.length === 0) {
      return;
    }

    // Find first instrument (source) or use external input
    let currentNode = null;

    for (let i = 0; i < this.chain.length; i++) {
      const id = this.chain[i];
      const instance = this.plugins.get(id);

      if (!instance) continue;

      if (instance.isInstrument) {
        // Instrument is a source - connect its output to bypass gain
        instance.workletNode.connect(instance.bypassGain);
        currentNode = instance.bypassGain;
      } else {
        // Effect - connect from previous node to input
        if (currentNode) {
          currentNode.connect(instance.workletNode);
        }
        instance.workletNode.connect(instance.bypassGain);
        currentNode = instance.bypassGain;
      }
    }

    // Connect final output to master
    if (currentNode) {
      currentNode.connect(this.masterGain);
    }

    this.emit('chainConnected', { chain: this.chain });
  }

  /**
   * Connect an external audio source to the first effect in chain
   * @param {AudioNode} sourceNode - Source node to connect
   */
  connectInput(sourceNode) {
    // Find first non-instrument plugin
    for (const id of this.chain) {
      const instance = this.plugins.get(id);
      if (instance && !instance.isInstrument) {
        sourceNode.connect(instance.workletNode);
        return;
      }
    }
    // If no effects, connect directly to master
    sourceNode.connect(this.masterGain);
  }

  /**
   * Reorder plugins in the chain
   * @param {string[]} newOrder - Array of plugin IDs in new order
   */
  reorderChain(newOrder) {
    // Validate all IDs exist
    for (const id of newOrder) {
      if (!this.plugins.has(id)) {
        throw new Error(`Unknown plugin ID: ${id}`);
      }
    }
    this.chain = [...newOrder];
    this.connectChain();
    this.emit('chainReordered', { chain: this.chain });
  }

  /**
   * Set bypass state for a plugin
   * @param {string} id - Plugin instance ID
   * @param {boolean} bypassed - Bypass state
   */
  setBypass(id, bypassed) {
    const instance = this.plugins.get(id);
    if (!instance) return;

    instance.bypassed = bypassed;

    // For true bypass, we'd need to reconnect the graph
    // For simplicity, we just mute the plugin's contribution
    // A more sophisticated approach would route around the plugin
    instance.bypassGain.gain.value = bypassed ? 0 : 1;

    this.emit('bypassChanged', { id, bypassed });
  }

  /**
   * Set master output gain
   * @param {number} gain - Gain value (0-1 typical, can exceed for boost)
   */
  setMasterGain(gain) {
    if (this.masterGain) {
      this.masterGain.gain.value = gain;
    }
  }

  /**
   * Get master output levels (for metering)
   * @returns {{ left: number, right: number }} RMS levels in dB
   */
  getMasterLevels() {
    if (!this.masterAnalyserL || !this.masterAnalyserR) {
      return { left: -Infinity, right: -Infinity };
    }

    const dataL = new Float32Array(this.masterAnalyserL.fftSize);
    const dataR = new Float32Array(this.masterAnalyserR.fftSize);

    this.masterAnalyserL.getFloatTimeDomainData(dataL);
    this.masterAnalyserR.getFloatTimeDomainData(dataR);

    const rmsL = Math.sqrt(dataL.reduce((sum, v) => sum + v * v, 0) / dataL.length);
    const rmsR = Math.sqrt(dataR.reduce((sum, v) => sum + v * v, 0) / dataR.length);

    return {
      left: rmsL > 0 ? 20 * Math.log10(rmsL) : -Infinity,
      right: rmsR > 0 ? 20 * Math.log10(rmsR) : -Infinity
    };
  }

  /**
   * Send MIDI to all plugins that accept MIDI
   * @param {number} status - MIDI status byte
   * @param {number} data1 - MIDI data byte 1
   * @param {number} data2 - MIDI data byte 2
   */
  sendMidi(status, data1, data2) {
    for (const [id, instance] of this.plugins) {
      instance.controller.sendMidi(status, data1, data2);
    }
  }

  /**
   * Send MIDI to a specific plugin
   * @param {string} id - Plugin instance ID
   * @param {number} status - MIDI status byte
   * @param {number} data1 - MIDI data byte 1
   * @param {number} data2 - MIDI data byte 2
   */
  sendMidiTo(id, status, data1, data2) {
    const instance = this.plugins.get(id);
    if (instance) {
      instance.controller.sendMidi(status, data1, data2);
    }
  }

  /**
   * Set parameter on a specific plugin
   * @param {string} id - Plugin instance ID
   * @param {number} paramIdx - Parameter index
   * @param {number} value - Normalized value (0-1)
   */
  setParam(id, paramIdx, value) {
    const instance = this.plugins.get(id);
    if (instance) {
      instance.controller.setParam(paramIdx, value);
    }
  }

  /**
   * Get plugin instance by ID
   * @param {string} id - Plugin instance ID
   * @returns {PluginInstance|undefined}
   */
  getPlugin(id) {
    return this.plugins.get(id);
  }

  /**
   * Get all plugin instances in chain order
   * @returns {PluginInstance[]}
   */
  getChain() {
    return this.chain.map(id => this.plugins.get(id)).filter(Boolean);
  }

  /**
   * Start audio processing
   */
  start() {
    if (this.audioContext.state === 'suspended') {
      this.audioContext.resume();
    }
    this.isRunning = true;
    this.emit('start');
  }

  /**
   * Stop audio processing
   */
  stop() {
    if (this.audioContext.state === 'running') {
      this.audioContext.suspend();
    }
    this.isRunning = false;
    this.emit('stop');
  }

  /**
   * Destroy the chain manager and all plugins
   */
  destroy() {
    this.stop();

    for (const [id] of this.plugins) {
      this.removePlugin(id);
    }

    if (this.audioContext) {
      this.audioContext.close();
      this.audioContext = null;
    }

    this._listeners.clear();
    this.emit('destroy');
  }

  // Event emitter methods
  on(event, listener) {
    if (!this._listeners.has(event)) {
      this._listeners.set(event, new Set());
    }
    this._listeners.get(event).add(listener);
    return () => this.off(event, listener);
  }

  off(event, listener) {
    this._listeners.get(event)?.delete(listener);
  }

  emit(event, data) {
    this._listeners.get(event)?.forEach(fn => fn(data));
  }
}

// Export to global scope
window.PluginChainManager = PluginChainManager;
