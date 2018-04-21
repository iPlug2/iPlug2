// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

const AudioWorkletHelper = (function() {
  const landingPageLocation =
      'https://googlechromelabs.github.io/web-audio-samples/audio-worklet/';
  const sourceLocationBase =
      'https://github.com/GoogleChromeLabs/web-audio-samples/blob/gh-pages/';

  let isAudioWorkletAvailable_ = false;
  let demoFunction_ = null;

  /**
   * @private
   * @return {boolean} True if AudioWorklet is available.
   */
  function detectAudioWorklet() {
    let context = new OfflineAudioContext(1, 1, 44100);
    return Boolean(
        context.audioWorklet &&
        typeof context.audioWorklet.addModule === 'function');
  }

  /**
   * @private
   * @param {boolean} featureDetected A flag for AudioWorklet feature detection.
   */
  function updateFeatureIndicator(featureDetected) {
    const workletIndicatorDiv =
        document.querySelector('#div-worklet-indicator');
    const warningMessageDiv =
        document.querySelector('#div-warning-message');
    if (workletIndicatorDiv) {
      if (featureDetected) {
        workletIndicatorDiv.textContent = 'AudioWorklet Ready';
        workletIndicatorDiv.className = 'worklet-status-found';
      } else {
        workletIndicatorDiv.textContent = 'No AudioWorklet';
        workletIndicatorDiv.className = 'worklet-status-missing';
      }
    } else {
      console.error('"#div-worklet-indicator" div is not present.');
    }

    if (warningMessageDiv) {
      warningMessageDiv.style.display = featureDetected ? 'none' : 'block';
      if (!featureDetected) {
        warningMessageDiv.innerHTML =
            `AudioWorklet is not available in your browser. Follow
            <a href="${landingPageLocation}"> the instruction</a> to enable the
            feature.`;
      }
    }
  }

  /**
   * @private
   * @param {object} demoData The meta data for the demo. See |initializeDemo|
   * function below.
   */
  function buildPageContent(demoData) {
    const titleNavBarSpan = document.querySelector('#title-navbar');
    const titleHeading = document.querySelector('#title-header');
    const description = document.querySelector('#demo-description');
    const htmlSource = document.querySelector('#link-html-source');
    const jsSource = document.querySelector('#link-js-source');

    document.title =
        demoData.title + ' | AudioWorklet | Chrome WebAudio Samples';
    if (titleNavBarSpan) {
      titleNavBarSpan.textContent = demoData.title;
    }
    if (titleHeading) {
      titleHeading.textContent = demoData.title;
    }
    if (description) {
      description.innerHTML = demoData.description;
    }
    if (htmlSource) {
      htmlSource.textContent = demoData.htmlSource;
      htmlSource.href = sourceLocationBase + demoData.htmlSource;
    }
    if (jsSource) {
      jsSource.textContent = demoData.jsSource;
      jsSource.href = sourceLocationBase + demoData.jsSource;
    }
  }

  /**
   * @private
   */
  function enableRunDemoButton() {
    const runDemoButton = document.querySelector('#btn-run-demo');
    if (!runDemoButton) {
      console.error('"#btn-run-demo" button is not present.');
    }

    if (isAudioWorkletAvailable_ && demoFunction_) {
      runDemoButton.disabled = false;
      runDemoButton.onclick = () => {
        runDemoButton.textContent = 'Started';
        runDemoButton.disabled = true;
        demoFunction_();
      };
    }
  }

  /**
   * @private
   * @param {object} demoData The meta data for the demo. See |initializeDemo|
   * function below.
   */
  function initializeCallback(demoData) {
    isAudioWorkletAvailable_ = detectAudioWorklet();
    updateFeatureIndicator(isAudioWorkletAvailable_);

    if (demoData) {
      if (demoData && typeof demoData.demoFunction === 'function') {
        demoFunction_ = demoData.demoFunction;
      } else {
        console.error('The "demoFunction" must be a function.');
      }

      buildPageContent(demoData);
      enableRunDemoButton();  
    }
  }

  return {
    /**
     * Initialize the demo page with a given meta data
     * @param {object} demoData
     * @param {string} demoData.title The page title.
     * @param {string} demoData.description The description.
     * @param {string} demoData.htmlSource The link to the HTML file.
     * @param {string} demoData.jsSource The link to the script file.
     */
    initializeDemo: (demoData) => {
      window.addEventListener('load', () => initializeCallback(demoData));
    },

    /**
     * Check if the browser supports AudioWorklet.
     * @return {Boolean} true if the browser supports AudioWorklet.
     */
    isAvailable: () => {
      return isAudioWorkletAvailable_;
    },
  };
})();
