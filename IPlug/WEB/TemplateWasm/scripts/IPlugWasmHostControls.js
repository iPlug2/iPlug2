/*
  Shared browser host controls for iPlug2 Wasm examples.
  The template-specific page supplies controller creation and plugin capability
  access; this module owns the footer UI, test signal routing, MIDI and meters.
*/

(function() {
  'use strict';

  const CONTROL_MARKUP = `
  <div id="controls" data-iplug-host-control="true">
    <button id="startBtn" disabled>Start Audio</button>
    <div id="status" class="host-status hidden"></div>

    <div id="sourceSection" class="control-section">
      <div class="separator"></div>
      <div class="control-group">
        <label>Source</label>
        <select id="sourceSelect" disabled>
          <option value="none">None</option>
          <option value="tone">Tone</option>
          <option value="noise">Noise</option>
          <option value="file">File</option>
        </select>
      </div>
      <div id="toneControls" class="control-group hidden">
        <select id="waveformSelect">
          <option value="sine">Sin</option>
          <option value="sawtooth" selected>Saw</option>
          <option value="square">Sqr</option>
          <option value="triangle">Tri</option>
        </select>
        <input type="range" id="freqL" min="20" max="2000" value="220" class="freq-slider" title="Left frequency">
        <button id="linkBtn" class="link-btn" title="Link L/R frequencies">&#128279;</button>
        <input type="range" id="freqR" min="20" max="2000" value="277" class="freq-slider" title="Right frequency">
      </div>
      <div id="fileControls" class="control-group hidden">
        <button id="loadFileBtn" title="Load audio file">&#128193;</button>
        <span id="fileName" class="file-name">No file</span>
        <button id="playPauseBtn" disabled title="Play/stop file">&#9654;</button>
        <input type="checkbox" id="loopCheck" checked title="Loop">
      </div>
      <div id="gainControl" class="control-group hidden">
        <label>Gain</label>
        <input type="range" id="gainSlider" min="0" max="100" value="50">
      </div>
      <div id="inputVuMeters" class="vu-meters">
        <span class="vu-label">IN</span>
        <div class="vu-bar-container"><div id="vuInL" class="vu-bar"></div></div>
        <div class="vu-bar-container"><div id="vuInR" class="vu-bar"></div></div>
      </div>
    </div>

    <div id="midiSection" class="control-section hidden">
      <div class="separator"></div>
      <div id="midiInGroup" class="control-group">
        <select id="midiInSelect" title="MIDI Input">
          <option value="">In: None</option>
        </select>
        <span id="midiInLed" class="midi-activity" title="MIDI In"></span>
      </div>
      <div id="midiOutGroup" class="control-group">
        <select id="midiOutSelect" title="MIDI Output">
          <option value="">Out: None</option>
        </select>
        <span id="midiOutLed" class="midi-activity" title="MIDI Out"></span>
      </div>
    </div>

    <div class="spacer"></div>

    <div id="outputVuMeters" class="vu-meters hidden">
      <span class="vu-label">OUT</span>
      <div class="vu-bar-container"><div id="vuOutL" class="vu-bar"></div></div>
      <div class="vu-bar-container"><div id="vuOutR" class="vu-bar"></div></div>
    </div>

    <button id="liveEditBtn" class="live-edit-btn hidden" title="Toggle live layout edit" aria-pressed="false">Edit</button>
    <button id="fullscreenBtn" title="Fullscreen">&#9974;</button>
    <div id="pluginParams" class="plugin-params hidden"></div>
  </div>
  <input type="file" id="fileInput" data-iplug-host-control="true" accept="audio/*" style="display: none">
  `;

  function clamp(value, min, max) {
    return Math.max(min, Math.min(max, value));
  }

  function disconnectNode(node, destination) {
    if (!node) return;
    try {
      if (destination) node.disconnect(destination);
      else node.disconnect();
    } catch (_) {
      // Some browser AudioNode implementations throw if the edge is absent.
    }
  }

  class IPlugWasmHostControls {
    constructor(options = {}) {
      this.options = options;
      this.controller = null;
      this.audioContext = null;
      this.audioStarted = false;

      this.oscillatorL = null;
      this.oscillatorR = null;
      this.noiseNode = null;
      this.fileSource = null;
      this.audioBuffer = null;
      this.gainNode = null;
      this.channelMerger = null;
      this.isFilePlaying = false;
      this.freqLinked = false;

      this.outAnalyserL = null;
      this.outAnalyserR = null;
      this.outSplitter = null;
      this.outVuDataL = null;
      this.outVuDataR = null;
      this.inAnalyserL = null;
      this.inAnalyserR = null;
      this.inSplitter = null;
      this.inVuDataL = null;
      this.inVuDataR = null;
      this.vuAnimationId = null;

      this.midiAccess = null;
      this.currentMidiIn = null;
      this.currentMidiOut = null;
      this.midiInTimeout = null;
      this.midiOutTimeout = null;
      this.midiOutUnsubscribe = null;
      this.liveEditActive = false;

      this.installMarkup();
      this.bindDom();
      this.bindEvents();
      this.initMidi();
      this.setReady(Boolean(options.ready));
      this.setStatus(options.initialStatus || '');

      if (options.showStatus) {
        this.status.classList.remove('hidden');
      }
    }

    installMarkup() {
      if (document.getElementById('controls')) return;

      const template = document.createElement('template');
      template.innerHTML = CONTROL_MARKUP.trim();
      document.body.appendChild(template.content);
    }

    bindDom() {
      this.startBtn = document.getElementById('startBtn');
      this.status = document.getElementById('status');
      this.sourceSelect = document.getElementById('sourceSelect');
      this.toneControls = document.getElementById('toneControls');
      this.fileControls = document.getElementById('fileControls');
      this.gainControl = document.getElementById('gainControl');
      this.waveformSelect = document.getElementById('waveformSelect');
      this.freqL = document.getElementById('freqL');
      this.freqR = document.getElementById('freqR');
      this.linkBtn = document.getElementById('linkBtn');
      this.loadFileBtn = document.getElementById('loadFileBtn');
      this.fileInput = document.getElementById('fileInput');
      this.fileName = document.getElementById('fileName');
      this.playPauseBtn = document.getElementById('playPauseBtn');
      this.loopCheck = document.getElementById('loopCheck');
      this.gainSlider = document.getElementById('gainSlider');
      this.fullscreenBtn = document.getElementById('fullscreenBtn');
      this.outputVuMeters = document.getElementById('outputVuMeters');
      this.vuOutLBar = document.getElementById('vuOutL');
      this.vuOutRBar = document.getElementById('vuOutR');
      this.inputVuMeters = document.getElementById('inputVuMeters');
      this.vuInLBar = document.getElementById('vuInL');
      this.vuInRBar = document.getElementById('vuInR');
      this.sourceSection = document.getElementById('sourceSection');
      this.midiSection = document.getElementById('midiSection');
      this.midiInGroup = document.getElementById('midiInGroup');
      this.midiOutGroup = document.getElementById('midiOutGroup');
      this.midiInSelect = document.getElementById('midiInSelect');
      this.midiOutSelect = document.getElementById('midiOutSelect');
      this.midiInLed = document.getElementById('midiInLed');
      this.midiOutLed = document.getElementById('midiOutLed');
      this.liveEditBtn = document.getElementById('liveEditBtn');
    }

    bindEvents() {
      this.startBtn.addEventListener('click', () => this.toggleAudio());
      this.sourceSelect.addEventListener('change', () => {
        this.stopCurrentSource();
        this.updateSourceControls();
        this.startCurrentSource();
      });
      this.waveformSelect.addEventListener('change', () => this.updateWaveform());
      this.linkBtn.addEventListener('click', () => this.toggleFrequencyLink());
      this.freqL.addEventListener('input', () => this.updateFrequency('left'));
      this.freqR.addEventListener('input', () => this.updateFrequency('right'));
      this.gainSlider.addEventListener('input', () => this.updateGain());
      this.loadFileBtn.addEventListener('click', () => this.fileInput.click());
      this.fileInput.addEventListener('change', (event) => this.loadAudioFile(event));
      this.playPauseBtn.addEventListener('click', () => this.toggleFilePlayback());
      this.loopCheck.addEventListener('change', () => {
        if (this.fileSource) this.fileSource.loop = this.loopCheck.checked;
      });
      this.fullscreenBtn.addEventListener('click', () => this.toggleFullscreen());
      this.liveEditBtn.addEventListener('click', () => this.toggleLiveEdit());
      this.midiInSelect.addEventListener('change', () => this.connectMidiInput(this.midiInSelect.value));
      this.midiOutSelect.addEventListener('change', () => this.connectMidiOutput(this.midiOutSelect.value));
    }

    setReady(ready = true) {
      this.startBtn.disabled = !ready;
    }

    setStatus(text) {
      if (!this.status) return;
      this.status.textContent = text || '';
      this.status.classList.toggle('hidden', !this.options.showStatus && !text);
    }

    refreshLiveEditAvailability() {
      const available = this.options.isLiveEditAvailable ? Boolean(this.options.isLiveEditAvailable()) : false;
      this.liveEditBtn.classList.toggle('hidden', !available);

      if (!available && this.liveEditActive) {
        this.liveEditActive = false;
        this.liveEditBtn.setAttribute('aria-pressed', 'false');
        this.liveEditBtn.classList.remove('active');
      }
    }

    toggleLiveEdit() {
      if (!this.options.setLiveEditEnabled) return;

      const next = !this.liveEditActive;
      if (!this.options.setLiveEditEnabled(next)) return;

      this.liveEditActive = next;
      this.liveEditBtn.setAttribute('aria-pressed', String(next));
      this.liveEditBtn.classList.toggle('active', next);
    }

    getCapabilities() {
      const caps = this.options.getCapabilities ? this.options.getCapabilities() : (this.options.capabilities || {});
      return {
        numInputs: Number(caps.numInputs ?? 0),
        numOutputs: Number(caps.numOutputs ?? 2),
        isInstrument: Boolean(caps.isInstrument),
        receivesMidi: Boolean(caps.receivesMidi),
        sendsMidi: Boolean(caps.sendsMidi)
      };
    }

    async getController(caps) {
      if (this.controller) return this.controller;
      if (!this.options.createController) {
        throw new Error('IPlugWasmHostControls requires createController()');
      }

      this.audioContext = new AudioContext({ latencyHint: 'interactive' });
      this.controller = await this.options.createController({
        audioContext: this.audioContext,
        capabilities: caps
      });
      return this.controller;
    }

    async toggleAudio() {
      this.startBtn.disabled = true;

      try {
        if (this.audioStarted) {
          this.stopAudio();
        } else {
          await this.startAudio();
        }
      } catch (err) {
        console.error('Failed to toggle audio:', err);
        this.setStatus('Audio start failed');
        alert('Failed to start audio. Make sure your browser supports AudioWorklet.');
      } finally {
        this.startBtn.disabled = false;
      }
    }

    async startAudio() {
      const caps = this.getCapabilities();
      const controller = await this.getController(caps);
      this.configureAudioRouting(controller, caps);
      controller.startAudio();

      this.audioStarted = true;
      this.startBtn.textContent = 'Stop';
      this.setStatus('Audio running');
    }

    stopAudio() {
      this.resetAudioRouting();

      if (this.controller) {
        this.controller.stopAudio();
      }

      this.audioStarted = false;
      this.startBtn.textContent = 'Start Audio';
      this.sourceSelect.disabled = true;
      this.sourceSelect.value = 'none';
      this.updateSourceControls();
      this.outputVuMeters.classList.add('hidden');
      this.setStatus('Audio stopped');
    }

    configureAudioRouting(controller, caps) {
      this.resetAudioRouting();

      const ctx = controller.getAudioContext();
      const workletNode = controller.getWorkletNode();
      const hasInputs = caps.numInputs > 0 && !caps.isInstrument;
      const channelCount = Math.max(2, caps.numOutputs || 2);

      workletNode.connect(ctx.destination);

      if (hasInputs) {
        this.sourceSection.classList.remove('hidden');
        this.sourceSelect.disabled = false;

        this.gainNode = ctx.createGain();
        this.gainNode.gain.value = parseFloat(this.gainSlider.value) / 100;
        this.gainNode.connect(workletNode);

        this.channelMerger = ctx.createChannelMerger(2);
        this.channelMerger.connect(this.gainNode);

        this.inSplitter = ctx.createChannelSplitter(2);
        this.gainNode.connect(this.inSplitter);

        this.inAnalyserL = ctx.createAnalyser();
        this.inAnalyserL.fftSize = 256;
        this.inAnalyserL.smoothingTimeConstant = 0.8;
        this.inSplitter.connect(this.inAnalyserL, 0);

        this.inAnalyserR = ctx.createAnalyser();
        this.inAnalyserR.fftSize = 256;
        this.inAnalyserR.smoothingTimeConstant = 0.8;
        this.inSplitter.connect(this.inAnalyserR, 1);

        this.inVuDataL = new Float32Array(this.inAnalyserL.fftSize);
        this.inVuDataR = new Float32Array(this.inAnalyserR.fftSize);
      } else {
        this.sourceSection.classList.add('hidden');
      }

      const hasMidi = caps.receivesMidi || caps.sendsMidi;
      this.midiSection.classList.toggle('hidden', !hasMidi);
      this.midiInGroup.classList.toggle('hidden', !caps.receivesMidi);
      this.midiOutGroup.classList.toggle('hidden', !caps.sendsMidi);

      this.outSplitter = ctx.createChannelSplitter(channelCount);
      workletNode.connect(this.outSplitter);

      this.outAnalyserL = ctx.createAnalyser();
      this.outAnalyserL.fftSize = 256;
      this.outAnalyserL.smoothingTimeConstant = 0.8;
      this.outSplitter.connect(this.outAnalyserL, 0);

      this.outAnalyserR = ctx.createAnalyser();
      this.outAnalyserR.fftSize = 256;
      this.outAnalyserR.smoothingTimeConstant = 0.8;
      this.outSplitter.connect(this.outAnalyserR, 1);

      this.outVuDataL = new Float32Array(this.outAnalyserL.fftSize);
      this.outVuDataR = new Float32Array(this.outAnalyserR.fftSize);

      this.outputVuMeters.classList.remove('hidden');
      this.startVuMetering();

      if (this.midiOutUnsubscribe) this.midiOutUnsubscribe();
      this.midiOutUnsubscribe = controller.on('midiMsg', ({ status, data1, data2 }) => {
        this.showMidiOutActivity();
        if (this.currentMidiOut) {
          this.currentMidiOut.send([status, data1, data2]);
        }
      });
    }

    resetAudioRouting() {
      this.stopCurrentSource();
      this.stopVuMetering();

      const workletNode = this.controller?.getWorkletNode?.();
      if (this.outSplitter) {
        disconnectNode(workletNode, this.outSplitter);
      }
      disconnectNode(this.gainNode);
      disconnectNode(this.channelMerger);
      disconnectNode(this.inSplitter);
      disconnectNode(this.outSplitter);
      disconnectNode(this.inAnalyserL);
      disconnectNode(this.inAnalyserR);
      disconnectNode(this.outAnalyserL);
      disconnectNode(this.outAnalyserR);

      this.gainNode = null;
      this.channelMerger = null;
      this.inSplitter = null;
      this.outSplitter = null;
      this.inAnalyserL = null;
      this.inAnalyserR = null;
      this.outAnalyserL = null;
      this.outAnalyserR = null;
      this.inVuDataL = null;
      this.inVuDataR = null;
      this.outVuDataL = null;
      this.outVuDataR = null;
    }

    startVuMetering() {
      const updateVu = () => {
        this.updateVuPair(this.outAnalyserL, this.outAnalyserR, this.outVuDataL, this.outVuDataR, this.vuOutLBar, this.vuOutRBar);
        this.updateVuPair(this.inAnalyserL, this.inAnalyserR, this.inVuDataL, this.inVuDataR, this.vuInLBar, this.vuInRBar);
        this.vuAnimationId = requestAnimationFrame(updateVu);
      };
      updateVu();
    }

    stopVuMetering() {
      if (this.vuAnimationId) {
        cancelAnimationFrame(this.vuAnimationId);
        this.vuAnimationId = null;
      }
    }

    updateVuPair(analyserL, analyserR, dataL, dataR, barL, barR) {
      if (!analyserL || !analyserR || !dataL || !dataR) return;

      analyserL.getFloatTimeDomainData(dataL);
      analyserR.getFloatTimeDomainData(dataR);

      const dbL = this.rmsDb(dataL);
      const dbR = this.rmsDb(dataR);
      const pctL = clamp(((dbL + 60) / 60) * 100, 0, 100);
      const pctR = clamp(((dbR + 60) / 60) * 100, 0, 100);

      barL.style.height = pctL + '%';
      barR.style.height = pctR + '%';
      barL.className = 'vu-bar' + (dbL > -6 ? ' hot' : dbL > -12 ? ' warm' : '');
      barR.className = 'vu-bar' + (dbR > -6 ? ' hot' : dbR > -12 ? ' warm' : '');
    }

    rmsDb(data) {
      let sum = 0;
      for (let i = 0; i < data.length; i++) {
        sum += data[i] * data[i];
      }
      const rms = Math.sqrt(sum / data.length);
      return rms > 0 ? 20 * Math.log10(rms) : -100;
    }

    updateSourceControls() {
      const source = this.sourceSelect.value;
      this.toneControls.classList.toggle('hidden', source !== 'tone');
      this.fileControls.classList.toggle('hidden', source !== 'file');
      this.gainControl.classList.toggle('hidden', source === 'none');
    }

    stopCurrentSource() {
      if (this.oscillatorL) {
        this.oscillatorL.stop();
        this.oscillatorL = null;
      }
      if (this.oscillatorR) {
        this.oscillatorR.stop();
        this.oscillatorR = null;
      }
      disconnectNode(this.noiseNode);
      this.noiseNode = null;
      if (this.fileSource) {
        this.fileSource.stop();
        this.fileSource = null;
      }
      this.isFilePlaying = false;
      this.playPauseBtn.textContent = String.fromCharCode(9654);
    }

    startCurrentSource() {
      const source = this.sourceSelect.value;
      if (!this.controller || source === 'none') return;

      if (source === 'tone') {
        this.startTone();
      } else if (source === 'noise') {
        this.startNoise();
      }
    }

    startTone() {
      if (!this.channelMerger) return;
      const ctx = this.controller.getAudioContext();
      const waveform = this.waveformSelect.value;

      this.oscillatorL = ctx.createOscillator();
      this.oscillatorL.type = waveform;
      this.oscillatorL.frequency.value = parseFloat(this.freqL.value);

      this.oscillatorR = ctx.createOscillator();
      this.oscillatorR.type = waveform;
      this.oscillatorR.frequency.value = parseFloat(this.freqR.value);

      this.oscillatorL.connect(this.channelMerger, 0, 0);
      this.oscillatorR.connect(this.channelMerger, 0, 1);
      this.oscillatorL.start();
      this.oscillatorR.start();
    }

    startNoise() {
      if (!this.gainNode) return;
      const ctx = this.controller.getAudioContext();
      const bufferSize = 2 * ctx.sampleRate;
      const noiseBuffer = ctx.createBuffer(2, bufferSize, ctx.sampleRate);

      for (let channel = 0; channel < 2; channel++) {
        const data = noiseBuffer.getChannelData(channel);
        for (let i = 0; i < bufferSize; i++) {
          data[i] = Math.random() * 2 - 1;
        }
      }

      this.noiseNode = ctx.createBufferSource();
      this.noiseNode.buffer = noiseBuffer;
      this.noiseNode.loop = true;
      this.noiseNode.connect(this.gainNode);
      this.noiseNode.start();
    }

    startFilePlayback() {
      if (!this.audioBuffer || !this.gainNode) return;

      const ctx = this.controller.getAudioContext();
      this.fileSource = ctx.createBufferSource();
      this.fileSource.buffer = this.audioBuffer;
      this.fileSource.loop = this.loopCheck.checked;
      this.fileSource.connect(this.gainNode);
      this.fileSource.onended = () => {
        if (!this.fileSource?.loop) {
          this.isFilePlaying = false;
          this.playPauseBtn.textContent = String.fromCharCode(9654);
          this.fileSource = null;
        }
      };
      this.fileSource.start();
      this.isFilePlaying = true;
      this.playPauseBtn.textContent = String.fromCharCode(9632);
    }

    updateWaveform() {
      if (this.oscillatorL) this.oscillatorL.type = this.waveformSelect.value;
      if (this.oscillatorR) this.oscillatorR.type = this.waveformSelect.value;
    }

    toggleFrequencyLink() {
      this.freqLinked = !this.freqLinked;
      this.linkBtn.classList.toggle('active', this.freqLinked);
      if (this.freqLinked) {
        this.freqR.value = this.freqL.value;
        if (this.oscillatorR) this.oscillatorR.frequency.value = parseFloat(this.freqL.value);
      }
    }

    updateFrequency(side) {
      const source = side === 'left' ? this.freqL : this.freqR;
      const other = side === 'left' ? this.freqR : this.freqL;
      const osc = side === 'left' ? this.oscillatorL : this.oscillatorR;
      const otherOsc = side === 'left' ? this.oscillatorR : this.oscillatorL;
      const freq = parseFloat(source.value);

      if (osc) osc.frequency.value = freq;
      if (this.freqLinked) {
        other.value = freq;
        if (otherOsc) otherOsc.frequency.value = freq;
      }
    }

    updateGain() {
      if (this.gainNode) {
        this.gainNode.gain.value = parseFloat(this.gainSlider.value) / 100;
      }
    }

    async loadAudioFile(event) {
      const file = event.target.files[0];
      if (!file || !this.controller) return;

      try {
        const ctx = this.controller.getAudioContext();
        const arrayBuffer = await file.arrayBuffer();
        this.audioBuffer = await ctx.decodeAudioData(arrayBuffer);
        this.fileName.textContent = file.name.length > 12 ? file.name.slice(0, 10) + '...' : file.name;
        this.fileName.title = file.name;
        this.playPauseBtn.disabled = false;
      } catch (err) {
        console.error('Failed to load audio file:', err);
        this.fileName.textContent = 'Error';
      }
    }

    toggleFilePlayback() {
      if (this.isFilePlaying) {
        if (this.fileSource) {
          this.fileSource.stop();
          this.fileSource = null;
        }
        this.isFilePlaying = false;
        this.playPauseBtn.textContent = String.fromCharCode(9654);
      } else {
        this.startFilePlayback();
      }
    }

    async initMidi() {
      try {
        if (!navigator.requestMIDIAccess) throw new Error('requestMIDIAccess unavailable');
        this.midiAccess = await navigator.requestMIDIAccess({ sysex: false });
        this.updateMidiDevices();
        this.midiAccess.onstatechange = () => this.updateMidiDevices();
      } catch (err) {
        console.log('WebMIDI not available:', err.message);
        this.midiInSelect.disabled = true;
        this.midiOutSelect.disabled = true;
        this.midiInSelect.innerHTML = '<option>MIDI unavailable</option>';
        this.midiOutSelect.innerHTML = '<option>MIDI unavailable</option>';
      }
    }

    updateMidiDevices() {
      const currentInValue = this.midiInSelect.value;
      const currentOutValue = this.midiOutSelect.value;

      this.midiInSelect.innerHTML = '<option value="">In: None</option>';
      for (const input of this.midiAccess.inputs.values()) {
        const opt = document.createElement('option');
        opt.value = input.id;
        opt.textContent = 'In: ' + (input.name.length > 20 ? input.name.slice(0, 18) + '...' : input.name);
        opt.title = input.name;
        this.midiInSelect.appendChild(opt);
      }

      this.midiOutSelect.innerHTML = '<option value="">Out: None</option>';
      for (const output of this.midiAccess.outputs.values()) {
        const opt = document.createElement('option');
        opt.value = output.id;
        opt.textContent = 'Out: ' + (output.name.length > 18 ? output.name.slice(0, 16) + '...' : output.name);
        opt.title = output.name;
        this.midiOutSelect.appendChild(opt);
      }

      if (currentInValue && this.midiInSelect.querySelector(`option[value="${currentInValue}"]`)) {
        this.midiInSelect.value = currentInValue;
      }
      if (currentOutValue && this.midiOutSelect.querySelector(`option[value="${currentOutValue}"]`)) {
        this.midiOutSelect.value = currentOutValue;
      }
    }

    showMidiInActivity() {
      this.midiInLed.classList.add('active');
      clearTimeout(this.midiInTimeout);
      this.midiInTimeout = setTimeout(() => this.midiInLed.classList.remove('active'), 100);
    }

    showMidiOutActivity() {
      this.midiOutLed.classList.add('active');
      clearTimeout(this.midiOutTimeout);
      this.midiOutTimeout = setTimeout(() => this.midiOutLed.classList.remove('active'), 100);
    }

    handleMidiMessage(event) {
      this.showMidiInActivity();
      if (!this.controller || event.data.length < 1) return;

      const status = event.data[0];
      const data1 = event.data.length > 1 ? event.data[1] : 0;
      const data2 = event.data.length > 2 ? event.data[2] : 0;
      this.controller.sendMidi(status, data1, data2);
    }

    connectMidiInput(inputId) {
      if (this.currentMidiIn) {
        this.currentMidiIn.onmidimessage = null;
        this.currentMidiIn = null;
      }

      if (!inputId || !this.midiAccess) return;

      const input = this.midiAccess.inputs.get(inputId);
      if (input) {
        input.onmidimessage = (event) => this.handleMidiMessage(event);
        this.currentMidiIn = input;
      }
    }

    connectMidiOutput(outputId) {
      this.currentMidiOut = null;
      if (!outputId || !this.midiAccess) return;

      const output = this.midiAccess.outputs.get(outputId);
      if (output) this.currentMidiOut = output;
    }

    toggleFullscreen() {
      if (document.fullscreenElement) {
        document.exitFullscreen();
        return;
      }

      const target = typeof this.options.fullscreenTarget === 'function'
        ? this.options.fullscreenTarget()
        : (this.options.fullscreenTarget || document.documentElement);

      target?.requestFullscreen?.();
    }
  }

  window.IPlugWasmHostControls = IPlugWasmHostControls;
})();
