/*
  Shared browser host controls for iPlug2 Wasm examples.
  The template-specific page supplies controller creation and plugin capability
  access; this module owns the footer UI, test signal routing, MIDI and meters.
*/

(function() {
  'use strict';

  const CONTROL_MARKUP = `
  <div id="controls" data-iplug-host-control="true">
    <button id="startBtn" class="power-btn" disabled title="Start Audio" aria-label="Start Audio" aria-pressed="false">&#9211;</button>
    <div id="status" class="host-status hidden"></div>

    <div id="sourceSection" class="control-section">
      <div class="separator"></div>
      <div class="control-group">
        <label>Source</label>
        <select id="sourceSelect" disabled>
          <option value="none">None</option>
          <option value="audioin">Audio In</option>
          <option value="tone">Tone</option>
          <option value="noise">Noise</option>
          <option value="file">File</option>
        </select>
      </div>
      <div id="toneControls" class="control-group hidden">
        <button id="toneSettingsBtn" class="settings-btn" type="button" popovertarget="tonePopover" title="Tone settings" aria-label="Tone settings">&#9881;</button>
      </div>
      <div id="audioInControls" class="control-group hidden">
        <button id="audioInSettingsBtn" class="settings-btn" type="button" popovertarget="audioInPopover" title="Audio Input settings" aria-label="Audio Input settings">&#9881;</button>
      </div>
      <div id="noiseControls" class="control-group hidden">
        <button id="noiseSettingsBtn" class="settings-btn" type="button" popovertarget="noisePopover" title="Noise settings" aria-label="Noise settings">&#9881;</button>
      </div>
      <div id="fileControls" class="control-group hidden">
        <button id="fileSettingsBtn" class="settings-btn" type="button" popovertarget="filePopover" title="File settings" aria-label="File settings">&#9881;</button>
        <button id="playPauseBtn" disabled title="Play/stop file">&#9654;</button>
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
      <span class="midi-port-icon" title="MIDI" aria-label="MIDI" role="img">
        <svg viewBox="0 0 24 24" width="16" height="16" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">
          <circle cx="12" cy="12" r="9"/>
          <circle cx="12" cy="6.5" r="1.2" fill="currentColor" stroke="none"/>
          <circle cx="7" cy="9" r="1.2" fill="currentColor" stroke="none"/>
          <circle cx="17" cy="9" r="1.2" fill="currentColor" stroke="none"/>
          <circle cx="8.5" cy="14" r="1.2" fill="currentColor" stroke="none"/>
          <circle cx="15.5" cy="14" r="1.2" fill="currentColor" stroke="none"/>
        </svg>
      </span>
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
  <div id="tonePopover" popover class="iplug-host-popover tone-popover" data-iplug-host-control="true">
    <div class="tone-popover-row">
      <label for="waveformSelect">Waveform</label>
      <select id="waveformSelect">
        <option value="sine">Sin</option>
        <option value="sawtooth" selected>Saw</option>
        <option value="square">Sqr</option>
        <option value="triangle">Tri</option>
      </select>
    </div>
    <div class="tone-popover-row">
      <label for="freqL">Freq L</label>
      <input type="range" id="freqL" min="20" max="2000" value="220" class="freq-slider" title="Left frequency">
      <input type="number" id="freqLInput" min="20" max="2000" step="0.1" value="220" class="freq-input" title="Left frequency (Hz)">
      <span class="freq-unit">Hz</span>
    </div>
    <div class="tone-popover-row">
      <label for="freqR">Freq R</label>
      <input type="range" id="freqR" min="20" max="2000" value="277" class="freq-slider" title="Right frequency">
      <input type="number" id="freqRInput" min="20" max="2000" step="0.1" value="277" class="freq-input" title="Right frequency (Hz)">
      <span class="freq-unit">Hz</span>
    </div>
    <div class="tone-popover-row tone-popover-check">
      <input type="checkbox" id="linkCheck">
      <label for="linkCheck">Link L/R frequencies</label>
    </div>
  </div>
  <div id="noisePopover" popover class="iplug-host-popover noise-popover" data-iplug-host-control="true">
    <div class="noise-popover-row">
      <label for="noiseTypeSelect">Type</label>
      <select id="noiseTypeSelect">
        <option value="white">White</option>
        <option value="pink">Pink</option>
      </select>
    </div>
  </div>
  <div id="audioInPopover" popover class="iplug-host-popover audio-in-popover" data-iplug-host-control="true">
    <div class="audio-in-popover-row">
      <label for="audioInDeviceSelect">Device</label>
      <select id="audioInDeviceSelect"><option value="">Default</option></select>
    </div>
    <div class="audio-in-popover-row">
      <label for="audioInChannels">Channels</label>
      <select id="audioInChannels">
        <option value="1">Mono</option>
        <option value="2" selected>Stereo</option>
      </select>
    </div>
    <div class="audio-in-popover-row checkbox-row">
      <label class="checkbox-label">
        <input type="checkbox" id="audioInEC"> Echo cancel
      </label>
      <label class="checkbox-label">
        <input type="checkbox" id="audioInNS"> Noise suppr.
      </label>
      <label class="checkbox-label">
        <input type="checkbox" id="audioInAGC"> Auto gain
      </label>
    </div>
  </div>
  <div id="filePopover" popover class="iplug-host-popover file-popover" data-iplug-host-control="true">
    <div class="file-popover-row">
      <button id="loadFileBtn" type="button">&#128193; Load file</button>
      <span id="fileName" class="file-name">No file</span>
    </div>
    <div class="file-popover-row">
      <input type="range" id="scrubBar" min="0" max="0" step="0.01" value="0" disabled class="scrub-bar" title="Seek">
    </div>
    <div class="file-popover-row time-row">
      <span id="currentTime" class="time-display">0:00</span>
      <span class="time-separator">/</span>
      <span id="totalTime" class="time-display">0:00</span>
      <label class="loop-label">
        <input type="checkbox" id="loopCheck" checked> Loop
      </label>
    </div>
  </div>
  <input type="file" id="fileInput" data-iplug-host-control="true" accept="audio/*" style="display: none">
  `;

  const PREFS_PREFIX = 'iplug-host-controls';

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
      this.audioInputStream = null;
      this.audioInputNode = null;
      this.isFilePlaying = false;
      this.freqLinked = false;
      this.playbackOffset = 0;
      this.playStartCtxTime = 0;
      this.scrubUpdateId = null;
      this.scrubBarDragging = false;

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

      this.prefsKey = `${PREFS_PREFIX}/${options.pluginName || 'default'}`;
      this.prefs = this.loadPreferences();
      this.midiPrefsApplied = false;

      this.installMarkup();
      this.bindDom();
      this.applyPreferences();
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
      this.freqLInput = document.getElementById('freqLInput');
      this.freqRInput = document.getElementById('freqRInput');
      this.linkCheck = document.getElementById('linkCheck');
      this.toneSettingsBtn = document.getElementById('toneSettingsBtn');
      this.tonePopover = document.getElementById('tonePopover');
      this.loadFileBtn = document.getElementById('loadFileBtn');
      this.fileInput = document.getElementById('fileInput');
      this.fileName = document.getElementById('fileName');
      this.playPauseBtn = document.getElementById('playPauseBtn');
      this.loopCheck = document.getElementById('loopCheck');
      this.fileSettingsBtn = document.getElementById('fileSettingsBtn');
      this.filePopover = document.getElementById('filePopover');
      this.audioInControls = document.getElementById('audioInControls');
      this.audioInSettingsBtn = document.getElementById('audioInSettingsBtn');
      this.audioInPopover = document.getElementById('audioInPopover');
      this.noiseControls = document.getElementById('noiseControls');
      this.noiseSettingsBtn = document.getElementById('noiseSettingsBtn');
      this.noisePopover = document.getElementById('noisePopover');
      this.noiseTypeSelect = document.getElementById('noiseTypeSelect');
      this.audioInDeviceSelect = document.getElementById('audioInDeviceSelect');
      this.audioInChannelsSelect = document.getElementById('audioInChannels');
      this.audioInECCheck = document.getElementById('audioInEC');
      this.audioInNSCheck = document.getElementById('audioInNS');
      this.audioInAGCCheck = document.getElementById('audioInAGC');
      this.scrubBar = document.getElementById('scrubBar');
      this.currentTimeDisplay = document.getElementById('currentTime');
      this.totalTimeDisplay = document.getElementById('totalTime');
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
      this.sourceSelect.addEventListener('change', async () => {
        this.stopCurrentSource();
        this.updateSourceControls();
        await this.startCurrentSource();
        this.savePreferences();
      });
      this.waveformSelect.addEventListener('change', () => {
        this.updateWaveform();
        this.savePreferences();
      });
      this.linkCheck.addEventListener('change', () => this.toggleFrequencyLink());
      this.freqL.addEventListener('input', () => {
        this.applyFrequency('left', parseFloat(this.freqL.value));
        this.savePreferences();
      });
      this.freqR.addEventListener('input', () => {
        this.applyFrequency('right', parseFloat(this.freqR.value));
        this.savePreferences();
      });
      this.freqLInput.addEventListener('change', () => {
        this.applyFrequency('left', parseFloat(this.freqLInput.value));
        this.savePreferences();
      });
      this.freqRInput.addEventListener('change', () => {
        this.applyFrequency('right', parseFloat(this.freqRInput.value));
        this.savePreferences();
      });
      this.gainSlider.addEventListener('input', () => {
        this.updateGain();
        this.savePreferences();
      });
      this.loadFileBtn.addEventListener('click', () => this.fileInput.click());
      this.fileInput.addEventListener('change', (event) => this.loadAudioFile(event));
      this.playPauseBtn.addEventListener('click', () => this.toggleFilePlayback());
      this.loopCheck.addEventListener('change', () => {
        if (this.fileSource) this.fileSource.loop = this.loopCheck.checked;
        this.savePreferences();
      });
      this.fullscreenBtn.addEventListener('click', () => this.toggleFullscreen());
      this.liveEditBtn.addEventListener('click', () => this.toggleLiveEdit());
      this.midiInSelect.addEventListener('change', () => {
        this.connectMidiInput(this.midiInSelect.value);
        this.savePreferences();
      });
      this.midiOutSelect.addEventListener('change', () => {
        this.connectMidiOutput(this.midiOutSelect.value);
        this.savePreferences();
      });

      this.setupPopover(this.tonePopover, this.toneSettingsBtn);
      this.setupPopover(this.filePopover, this.fileSettingsBtn);
      this.setupPopover(this.audioInPopover, this.audioInSettingsBtn);
      this.setupPopover(this.noisePopover, this.noiseSettingsBtn);

      this.noiseTypeSelect?.addEventListener('change', () => {
        this.savePreferences();
        if (this.sourceSelect.value === 'noise' && this.audioStarted) {
          this.stopAndDisconnect(this.noiseNode);
          this.noiseNode = null;
          this.startNoise();
        }
      });

      // Refresh device list whenever the audio-in popover opens (labels become
      // available only after permission has been granted), and on hot-plug.
      if (this.audioInPopover) {
        this.audioInPopover.addEventListener('beforetoggle', (e) => {
          if (e.newState === 'open') this.populateAudioInDevices();
        });
      }
      if (navigator.mediaDevices?.addEventListener) {
        navigator.mediaDevices.addEventListener('devicechange', () => this.populateAudioInDevices());
      }

      this.audioInDeviceSelect?.addEventListener('change', () => this.handleAudioInSettingsChange());
      this.audioInChannelsSelect?.addEventListener('change', () => this.handleAudioInSettingsChange());
      this.audioInECCheck?.addEventListener('change', () => this.handleAudioInSettingsChange());
      this.audioInNSCheck?.addEventListener('change', () => this.handleAudioInSettingsChange());
      this.audioInAGCCheck?.addEventListener('change', () => this.handleAudioInSettingsChange());

      if (this.scrubBar) {
        this.scrubBar.addEventListener('input', () => {
          this.scrubBarDragging = true;
          if (this.currentTimeDisplay) {
            this.currentTimeDisplay.textContent = this.formatTime(parseFloat(this.scrubBar.value));
          }
        });
        this.scrubBar.addEventListener('change', () => {
          this.seekTo(parseFloat(this.scrubBar.value));
          this.scrubBarDragging = false;
        });
      }
    }

    setupPopover(popover, anchor) {
      if (!popover) return;
      popover.addEventListener('beforetoggle', (e) => {
        if (e.newState === 'open') this.positionPopover(popover, anchor);
      });
      window.addEventListener('resize', () => {
        if (popover.matches(':popover-open')) this.positionPopover(popover, anchor);
      });
    }

    positionPopover(popover, anchor) {
      if (!popover || !anchor) return;
      const rect = anchor.getBoundingClientRect();
      // Anchor above the trigger (the host controls live in a bottom bar).
      const popWidth = popover.offsetWidth || 260;
      const left = clamp(rect.left, 8, window.innerWidth - popWidth - 8);
      popover.style.left = `${left}px`;
      popover.style.bottom = `${window.innerHeight - rect.top + 8}px`;
      popover.style.top = 'auto';
      popover.style.right = 'auto';
      popover.style.margin = '0';
    }

    syncFreqInputs() {
      if (this.freqLInput && document.activeElement !== this.freqLInput) {
        this.freqLInput.value = this.freqL.value;
      }
      if (this.freqRInput && document.activeElement !== this.freqRInput) {
        this.freqRInput.value = this.freqR.value;
      }
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

    loadPreferences() {
      try {
        const raw = window.localStorage?.getItem(this.prefsKey);
        return raw ? JSON.parse(raw) : {};
      } catch (_) {
        return {};
      }
    }

    applyPreferences() {
      const p = this.prefs;
      if (typeof p.source === 'string' && this.sourceSelect.querySelector(`option[value="${p.source}"]`)) {
        // 'file' needs a user-chosen buffer that we can't restore, so fall back to 'none'.
        this.sourceSelect.value = p.source === 'file' ? 'none' : p.source;
        this.updateSourceControls();
      }
      if (typeof p.waveform === 'string') this.waveformSelect.value = p.waveform;
      if (typeof p.freqL === 'number') this.freqL.value = p.freqL;
      if (typeof p.freqR === 'number') this.freqR.value = p.freqR;
      if (typeof p.gain === 'number') this.gainSlider.value = p.gain;
      if (typeof p.loop === 'boolean') this.loopCheck.checked = p.loop;
      if (p.freqLinked) {
        this.freqLinked = true;
        this.linkCheck.checked = true;
        this.freqR.value = this.freqL.value;
      }
      if (typeof p.audioInDeviceId === 'string' && this.audioInDeviceSelect) {
        // The deviceId option may not be populated yet; queue the value so
        // populateAudioInDevices() can re-select it once the list is built.
        this.audioInDeviceSelect.value = p.audioInDeviceId;
      }
      if (typeof p.audioInChannels === 'number' && this.audioInChannelsSelect) {
        this.audioInChannelsSelect.value = String(p.audioInChannels);
      }
      if (this.audioInECCheck) this.audioInECCheck.checked = Boolean(p.audioInEC);
      if (this.audioInNSCheck) this.audioInNSCheck.checked = Boolean(p.audioInNS);
      if (this.audioInAGCCheck) this.audioInAGCCheck.checked = Boolean(p.audioInAGC);
      if (typeof p.noiseType === 'string' && this.noiseTypeSelect) {
        this.noiseTypeSelect.value = p.noiseType;
      }
      this.syncFreqInputs();
    }

    savePreferences() {
      try {
        const data = {
          source: this.sourceSelect.value,
          waveform: this.waveformSelect.value,
          freqL: parseFloat(this.freqL.value),
          freqR: parseFloat(this.freqR.value),
          freqLinked: this.freqLinked,
          gain: parseFloat(this.gainSlider.value),
          loop: this.loopCheck.checked,
          // Until MIDI has enumerated, keep whatever was previously stored so an
          // early save (e.g. tweaking gain on page load) doesn't wipe the IDs.
          midiInId: this.midiPrefsApplied ? (this.midiInSelect.value || '') : (this.prefs.midiInId || ''),
          midiOutId: this.midiPrefsApplied ? (this.midiOutSelect.value || '') : (this.prefs.midiOutId || ''),
          audioInDeviceId: this.audioInDeviceSelect?.value || '',
          audioInChannels: parseInt(this.audioInChannelsSelect?.value, 10) || 2,
          audioInEC: Boolean(this.audioInECCheck?.checked),
          audioInNS: Boolean(this.audioInNSCheck?.checked),
          audioInAGC: Boolean(this.audioInAGCCheck?.checked),
          noiseType: this.noiseTypeSelect?.value || 'white'
        };
        this.prefs = data;
        window.localStorage?.setItem(this.prefsKey, JSON.stringify(data));
      } catch (_) {
        // Storage may be unavailable (private mode, quota, etc.); ignore.
      }
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
      this.startBtn.classList.add('on');
      this.startBtn.title = 'Stop Audio';
      this.startBtn.setAttribute('aria-label', 'Stop Audio');
      this.startBtn.setAttribute('aria-pressed', 'true');
      await this.startCurrentSource();
    }

    stopAudio() {
      this.resetAudioRouting();

      if (this.controller) {
        this.controller.stopAudio();
      }

      this.audioStarted = false;
      this.startBtn.classList.remove('on');
      this.startBtn.title = 'Start Audio';
      this.startBtn.setAttribute('aria-label', 'Start Audio');
      this.startBtn.setAttribute('aria-pressed', 'false');
      this.sourceSelect.disabled = true;
      this.outputVuMeters.classList.add('hidden');
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
      this.audioInControls.classList.toggle('hidden', source !== 'audioin');
      this.noiseControls.classList.toggle('hidden', source !== 'noise');
      this.gainControl.classList.toggle('hidden', source === 'none');
    }

    stopCurrentSource() {
      this.stopAndDisconnect(this.oscillatorL);
      this.oscillatorL = null;
      this.stopAndDisconnect(this.oscillatorR);
      this.oscillatorR = null;
      this.stopAndDisconnect(this.noiseNode);
      this.noiseNode = null;
      this.stopFileSource({ preserveOffset: true });
      this.stopAudioInput();
    }

    stopAudioInput() {
      if (this.audioInputNode) {
        disconnectNode(this.audioInputNode);
        this.audioInputNode = null;
      }
      if (this.audioInputStream) {
        this.audioInputStream.getTracks().forEach((track) => track.stop());
        this.audioInputStream = null;
      }
    }

    stopAndDisconnect(node) {
      if (!node) return;
      try { node.stop(); } catch (_) { /* already stopped */ }
      disconnectNode(node);
    }

    stopFileSource({ preserveOffset = false } = {}) {
      if (this.fileSource) {
        if (this.isFilePlaying && preserveOffset) {
          const ctx = this.controller?.getAudioContext?.();
          if (ctx) this.playbackOffset = this.computePlaybackPosition(ctx);
        }
        this.fileSource.onended = null;
        this.stopAndDisconnect(this.fileSource);
        this.fileSource = null;
      }
      this.isFilePlaying = false;
      this.playPauseBtn.textContent = String.fromCharCode(9654);
      this.stopScrubUpdater();
    }

    async startCurrentSource() {
      const source = this.sourceSelect.value;
      if (!this.controller || source === 'none') return;

      if (source === 'tone') {
        this.startTone();
      } else if (source === 'noise') {
        this.startNoise();
      } else if (source === 'audioin') {
        await this.startAudioInput();
      }
    }

    async startAudioInput() {
      if (!this.gainNode) return;
      if (!navigator.mediaDevices?.getUserMedia) {
        alert('Audio input is not supported in this browser.');
        this.revertSourceToNone();
        return;
      }
      try {
        this.audioInputStream = await navigator.mediaDevices.getUserMedia({
          audio: this.buildAudioInputConstraints()
        });
        // The user may have switched source away while the permission prompt
        // was open; bail if so to avoid creating a stray live mic node.
        if (this.sourceSelect.value !== 'audioin' || !this.gainNode) {
          this.audioInputStream.getTracks().forEach((track) => track.stop());
          this.audioInputStream = null;
          return;
        }
        const ctx = this.controller.getAudioContext();
        this.audioInputNode = ctx.createMediaStreamSource(this.audioInputStream);
        this.audioInputNode.connect(this.gainNode);
        // Labels become available only after permission has been granted, so
        // refresh the device list now to populate them in the picker.
        this.populateAudioInDevices();
      } catch (err) {
        console.error('Failed to access audio input:', err);
        alert('Could not access audio input. Check your browser microphone permissions.');
        this.revertSourceToNone();
      }
    }

    buildAudioInputConstraints() {
      // Fall back to the saved pref so the first getUserMedia call after a
      // page reload (before the device list has been enumerated) still
      // honors the user's previous device choice.
      const deviceId = this.audioInDeviceSelect?.value || this.prefs.audioInDeviceId || '';
      const channels = parseInt(this.audioInChannelsSelect?.value, 10) || 2;
      const constraints = {
        echoCancellation: Boolean(this.audioInECCheck?.checked),
        noiseSuppression: Boolean(this.audioInNSCheck?.checked),
        autoGainControl: Boolean(this.audioInAGCCheck?.checked),
        channelCount: { ideal: channels }
      };
      if (deviceId) constraints.deviceId = { ideal: deviceId };
      return constraints;
    }

    async populateAudioInDevices() {
      if (!navigator.mediaDevices?.enumerateDevices || !this.audioInDeviceSelect) return;
      try {
        const devices = await navigator.mediaDevices.enumerateDevices();
        const inputs = devices.filter((d) => d.kind === 'audioinput');
        const previous = this.audioInDeviceSelect.value || this.prefs.audioInDeviceId || '';
        this.audioInDeviceSelect.innerHTML = '<option value="">Default</option>';
        for (const device of inputs) {
          const opt = document.createElement('option');
          opt.value = device.deviceId;
          opt.textContent = device.label || `Microphone (${(device.deviceId || '').slice(0, 8)})`;
          this.audioInDeviceSelect.appendChild(opt);
        }
        if (previous && this.audioInDeviceSelect.querySelector(`option[value="${previous}"]`)) {
          this.audioInDeviceSelect.value = previous;
        }
      } catch (err) {
        console.error('Failed to enumerate audio devices:', err);
      }
    }

    async handleAudioInSettingsChange() {
      this.savePreferences();
      if (this.sourceSelect.value === 'audioin' && this.audioStarted) {
        this.stopAudioInput();
        await this.startAudioInput();
      }
    }

    revertSourceToNone() {
      this.sourceSelect.value = 'none';
      this.updateSourceControls();
      this.savePreferences();
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
      const type = this.noiseTypeSelect?.value || 'white';
      const bufferSize = 2 * ctx.sampleRate;
      const noiseBuffer = ctx.createBuffer(2, bufferSize, ctx.sampleRate);

      for (let channel = 0; channel < 2; channel++) {
        const data = noiseBuffer.getChannelData(channel);
        if (type === 'pink') this.fillPinkNoise(data);
        else this.fillWhiteNoise(data);
      }

      this.noiseNode = ctx.createBufferSource();
      this.noiseNode.buffer = noiseBuffer;
      this.noiseNode.loop = true;
      this.noiseNode.connect(this.gainNode);
      this.noiseNode.start();
    }

    fillWhiteNoise(data) {
      for (let i = 0; i < data.length; i++) {
        data[i] = Math.random() * 2 - 1;
      }
    }

    fillPinkNoise(data) {
      // Paul Kellet's pink-noise approximation: a parallel bank of IIR
      // filters driven by white noise that yields a near 1/f spectrum.
      let b0 = 0, b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0, b6 = 0;
      for (let i = 0; i < data.length; i++) {
        const white = Math.random() * 2 - 1;
        b0 = 0.99886 * b0 + white * 0.0555179;
        b1 = 0.99332 * b1 + white * 0.0750759;
        b2 = 0.96900 * b2 + white * 0.1538520;
        b3 = 0.86650 * b3 + white * 0.3104856;
        b4 = 0.55000 * b4 + white * 0.5329522;
        b5 = -0.7616 * b5 - white * 0.0168980;
        data[i] = (b0 + b1 + b2 + b3 + b4 + b5 + b6 + white * 0.5362) * 0.11;
        b6 = white * 0.115926;
      }
    }

    startFilePlayback(offset) {
      if (!this.audioBuffer || !this.gainNode) return;

      // Defensive: tear down any stale source so a rapid seek/play race
      // can't leave the previous buffer connected alongside the new one.
      if (this.fileSource) {
        this.fileSource.onended = null;
        this.stopAndDisconnect(this.fileSource);
        this.fileSource = null;
      }

      const ctx = this.controller.getAudioContext();
      const duration = this.audioBuffer.duration;
      let startOffset = offset != null ? offset : this.playbackOffset;
      if (startOffset >= duration) startOffset = 0;

      const source = ctx.createBufferSource();
      source.buffer = this.audioBuffer;
      source.loop = this.loopCheck.checked;
      source.connect(this.gainNode);
      source.onended = () => {
        if (this.fileSource !== source) return; // superseded by seek/stop
        if (!source.loop) {
          this.isFilePlaying = false;
          this.playPauseBtn.textContent = String.fromCharCode(9654);
          this.playbackOffset = 0;
          this.fileSource = null;
          this.stopScrubUpdater();
          this.updateScrubUi(0);
        }
      };
      source.start(0, startOffset);

      this.fileSource = source;
      this.playStartCtxTime = ctx.currentTime;
      this.playbackOffset = startOffset;
      this.isFilePlaying = true;
      this.playPauseBtn.textContent = String.fromCharCode(9632);
      this.startScrubUpdater();
    }

    pauseFilePlayback() {
      this.stopFileSource({ preserveOffset: true });
    }

    seekTo(offset) {
      if (!this.audioBuffer) return;
      const target = clamp(offset, 0, this.audioBuffer.duration);
      if (this.isFilePlaying) {
        this.stopFileSource({ preserveOffset: false });
        this.startFilePlayback(target);
      } else {
        this.playbackOffset = target;
        this.updateScrubUi(target);
      }
    }

    computePlaybackPosition(ctx) {
      if (!this.audioBuffer) return 0;
      const elapsed = ctx.currentTime - this.playStartCtxTime;
      const raw = this.playbackOffset + elapsed;
      const duration = this.audioBuffer.duration;
      if (this.fileSource?.loop) return raw % duration;
      return Math.min(raw, duration);
    }

    startScrubUpdater() {
      if (this.scrubUpdateId) return;
      const ctx = this.controller?.getAudioContext?.();
      if (!ctx) return;
      const tick = () => {
        if (!this.isFilePlaying) {
          this.scrubUpdateId = null;
          return;
        }
        this.updateScrubUi(this.computePlaybackPosition(ctx));
        this.scrubUpdateId = requestAnimationFrame(tick);
      };
      this.scrubUpdateId = requestAnimationFrame(tick);
    }

    stopScrubUpdater() {
      if (this.scrubUpdateId) {
        cancelAnimationFrame(this.scrubUpdateId);
        this.scrubUpdateId = null;
      }
    }

    updateScrubUi(seconds) {
      if (this.scrubBar && !this.scrubBarDragging) {
        this.scrubBar.value = seconds;
      }
      if (this.currentTimeDisplay) {
        this.currentTimeDisplay.textContent = this.formatTime(seconds);
      }
    }

    formatTime(seconds) {
      const safe = isFinite(seconds) && seconds > 0 ? seconds : 0;
      const m = Math.floor(safe / 60);
      const s = Math.floor(safe % 60);
      return `${m}:${s.toString().padStart(2, '0')}`;
    }

    updateWaveform() {
      if (this.oscillatorL) this.oscillatorL.type = this.waveformSelect.value;
      if (this.oscillatorR) this.oscillatorR.type = this.waveformSelect.value;
    }

    toggleFrequencyLink() {
      this.freqLinked = this.linkCheck.checked;
      if (this.freqLinked) {
        this.applyFrequency('left', parseFloat(this.freqL.value));
      }
      this.savePreferences();
    }

    applyFrequency(side, requested) {
      const min = parseFloat(this.freqL.min);
      const max = parseFloat(this.freqL.max);
      const freq = clamp(isFinite(requested) ? requested : min, min, max);

      this.setFrequencyControls(side, freq);
      if (this.freqLinked) {
        const other = side === 'left' ? 'right' : 'left';
        this.setFrequencyControls(other, freq);
      }
    }

    setFrequencyControls(side, freq) {
      const slider = side === 'left' ? this.freqL : this.freqR;
      const input = side === 'left' ? this.freqLInput : this.freqRInput;
      const osc = side === 'left' ? this.oscillatorL : this.oscillatorR;

      slider.value = freq;
      // Avoid clobbering an in-progress edit while the user is typing.
      if (input && document.activeElement !== input) {
        input.value = freq;
      }
      if (osc) osc.frequency.value = freq;
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
        // Stop any current playback before swapping buffers.
        this.stopFileSource({ preserveOffset: false });

        const ctx = this.controller.getAudioContext();
        const arrayBuffer = await file.arrayBuffer();
        this.audioBuffer = await ctx.decodeAudioData(arrayBuffer);
        this.fileName.textContent = file.name;
        this.fileName.title = file.name;
        this.playPauseBtn.disabled = false;
        this.playbackOffset = 0;
        if (this.scrubBar) {
          this.scrubBar.max = this.audioBuffer.duration;
          this.scrubBar.value = 0;
          this.scrubBar.disabled = false;
        }
        if (this.totalTimeDisplay) {
          this.totalTimeDisplay.textContent = this.formatTime(this.audioBuffer.duration);
        }
        this.updateScrubUi(0);
      } catch (err) {
        console.error('Failed to load audio file:', err);
        this.fileName.textContent = 'Error';
      }
    }

    toggleFilePlayback() {
      if (this.isFilePlaying) {
        this.pauseFilePlayback();
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

      const desiredIn = currentInValue || (!this.midiPrefsApplied ? (this.prefs.midiInId || '') : '');
      const desiredOut = currentOutValue || (!this.midiPrefsApplied ? (this.prefs.midiOutId || '') : '');

      if (desiredIn && this.midiInSelect.querySelector(`option[value="${desiredIn}"]`)) {
        this.midiInSelect.value = desiredIn;
        if (desiredIn !== currentInValue) this.connectMidiInput(desiredIn);
      }
      if (desiredOut && this.midiOutSelect.querySelector(`option[value="${desiredOut}"]`)) {
        this.midiOutSelect.value = desiredOut;
        if (desiredOut !== currentOutValue) this.connectMidiOutput(desiredOut);
      }

      this.midiPrefsApplied = true;
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
