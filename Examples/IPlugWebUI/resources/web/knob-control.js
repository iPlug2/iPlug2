class KnobControl extends HTMLElement {
  constructor() {
    super();
    
    this.paramId = 0;
    this.controlTag = '';
    this.defaultValue = 0.0;
    this.value = 0.0;

    this.label = this.getAttribute('label') || '';
    this.minValue = parseFloat(this.getAttribute('min')) || 0;
    this.maxValue = parseFloat(this.getAttribute('max')) || 100;
    const units = this.getAttribute('units') || '';
    const minAngle = parseFloat(this.getAttribute('min-angle')) || -135;
    const maxAngle = parseFloat(this.getAttribute('max-angle')) || 135;
    const circleStrokeColor = this.getAttribute('circle-stroke-color') || '#fff';
    const circleStrokeWidth = parseFloat(this.getAttribute('circle-stroke-width')) || 2;
    const circleFillColor = this.getAttribute('circle-fill-color') || '#000';
    const pointerColor = this.getAttribute('pointer-color') || '#f00';
    const pointerWidth = parseFloat(this.getAttribute('pointer-width')) || 4;
    const valueArcColor = this.getAttribute('value-arc-color') || '#f00';
    const valueArcWidth = parseFloat(this.getAttribute('value-arc-width')) || 3;
    const trackBgColor = this.getAttribute('track-bg-color') || '#999';
    
    this.attachShadow({ mode: 'open' });
    this.shadowRoot.innerHTML = `
    <style>
    :host {
      display: inline-block;
    }
    .container {
      display: flex;
      flex-direction: column;
      align-items: center;
      justify-content: center;
      width: 100%;
      height: 100%;
    }
    .label {
      margin-bottom: 8px;
      color: black;
      font-size: 14px;
      pointer-events: none;
    }
    .value {
      margin-top: 8px;
      color: white;
      font-size: 12px;
      pointer-events: none;
    }
    .hidden-cursor {
      cursor: none;
    }
    </style>
    <div class="container">
    <div class="label">${this.label}</div>
    <svg viewBox="0 0 100 100" width="80" height="80">
    <circle cx="50" cy="50" r="42" fill=${circleFillColor} stroke=${circleStrokeColor} stroke-width=${circleStrokeWidth} @mousedown="startDrag"></circle>
    <path class="track-bg" fill="none" stroke=${trackBgColor} stroke-width=${valueArcWidth} d=""></path>
    <path class="value-arc" fill="none" stroke=${valueArcColor} stroke-width=${valueArcWidth} d=""></path>
    <line class="pointer" x1="50" y1="10" x2="50" y2="50" stroke=${pointerColor} stroke-width=${pointerWidth} transform="rotate(0, 50, 50)"></line>
    </svg>
    <div class="value">0 ${units}</div>
    </div>
    `;
    
    const pointer = this.shadowRoot.querySelector('.pointer');
    const valueElement = this.shadowRoot.querySelector('.value');
    let currentValue = this.defaultValue;
    
    const valueArc = this.shadowRoot.querySelector('.value-arc');
    const trackBg = this.shadowRoot.querySelector('.track-bg');

    const polarToCartesian = (centerX, centerY, radius, angleInDegrees) => {
      const angleInRadians = (angleInDegrees - 90) * Math.PI / 180.0;
      return {
        x: centerX + (radius * Math.cos(angleInRadians)),
        y: centerY + (radius * Math.sin(angleInRadians))
      };
    };
    
    const createTrackBg = () => {
      const arcRadius = 48;
      const start = polarToCartesian(50, 50, arcRadius, minAngle);
      const end = polarToCartesian(50, 50, arcRadius, maxAngle);
      
      const largeArcFlag = maxAngle - minAngle <= 180 ? 0 : 1;
      
      const d = [
        'M', start.x, start.y,
        'A', arcRadius, arcRadius, 0, largeArcFlag, 1, end.x, end.y
      ].join(' ');
      
      trackBg.setAttribute('d', d);
    };
    
    createTrackBg();
    
    const updateValueArc = (angle) => {
      const startAngle = minAngle;
      const endAngle = angle;
      const arcRadius = 48;
      
      const start = polarToCartesian(50, 50, arcRadius, startAngle);
      const end = polarToCartesian(50, 50, arcRadius, endAngle);
      
      const largeArcFlag = endAngle - startAngle <= 180 ? 0 : 1;
      
      const d = [
        'M', start.x, start.y,
        'A', arcRadius, arcRadius, 0, largeArcFlag, 1, end.x, end.y
      ].join(' ');
      
      valueArc.setAttribute('d', d);
    };
    
    const updateValue = (value, normalized = false) => {
      let finalValue;
      if (normalized) {
        // If we receive a normalized value (0-1), convert it to our range
        finalValue = this.minValue + (value * (this.maxValue - this.minValue));
      } else {
        finalValue = value;
      }
      
      currentValue = finalValue;
      valueElement.textContent = `${finalValue.toFixed(1)} ${units}`;
      const normValue = ((finalValue - this.minValue) / (this.maxValue - this.minValue));
      const angle = minAngle + normValue * (maxAngle - minAngle);
      pointer.setAttribute('transform', `rotate(${angle}, 50, 50)`);
      updateValueArc(angle);
      
      if (typeof window['SPVFUI'] === 'function') {
        window['SPVFUI'](this.paramId, normValue);
      }
    };
    
    const startDrag = (e) => {
      
      if (e.button == 2) return; // right click (context menu
      
      if (typeof window['BPCFUI'] === 'function') {
        window['BPCFUI'](this.paramId);
      }
      
      e.preventDefault();
      const svg = e.currentTarget.parentElement;
      
      let initialY = ('touches' in e) ? e.touches[0].clientY : e.clientY;
      let initialValue = currentValue;
      
      const onMove = (e) => {
        const clientY = ('touches' in e) ? e.touches[0].clientY : e.clientY;
        const deltaY = initialY - clientY;
        const valueChange = deltaY * (this.maxValue - this.minValue) / 100;
        const value = Math.min(Math.max(initialValue + valueChange, this.minValue), this.maxValue);
        updateValue(value);
      };
      
      const onEnd = () => {
        document.removeEventListener('mousemove', onMove);
        document.removeEventListener('mouseup', onEnd);
        document.removeEventListener('touchmove', onMove);
        document.removeEventListener('touchend', onEnd);
        document.body.classList.remove('hidden-cursor');
        document.body.style.cursor = '';
        
        if (typeof window['EPCFUI'] === 'function') {
          window['EPCFUI'](this.paramId);
        }
      };
      
      document.addEventListener('mousemove', onMove);
      document.addEventListener('mouseup', onEnd);
      document.addEventListener('touchmove', onMove, { passive: false });
      document.addEventListener('touchend', onEnd);
      document.body.classList.add('hidden-cursor');
      document.body.style.cursor = 'none';
    };
    
    const onWheel = (e) => {
      e.preventDefault();
      const delta = e.deltaY < 0 ? 1 : -1;
      const valueChange = delta * (this.maxValue - this.minValue) / 100;
      const value = Math.min(Math.max(currentValue + valueChange, this.minValue), this.maxValue);
      updateValue(value);
    };
    
    this.shadowRoot.querySelector('circle').addEventListener('mousedown', startDrag);
    this.shadowRoot.querySelector('circle').addEventListener('touchstart', startDrag, { passive: false });
    this.shadowRoot.querySelector('circle').addEventListener('wheel', onWheel);
    
//    updateValue(currentValue);
    
    this.shadowRoot.querySelector('.container').__updateValue = updateValue;
    
    // Expose updateValue method with a different name to avoid conflicts
    this.updateValueFromHost = (normalizedValue) => updateValue(normalizedValue, true);
  }
  
  connectedCallback() {
    // Set the parameter ID, control tag, and initial value
    this.paramId = this.getAttribute('param-id') || -1;
    this.controlTag = this.getAttribute('control-tag') || '';
    this.defaultValue = parseFloat(this.getAttribute('default-value')) || 0.0;
  }

  attributeChangedCallback(attrName, oldVal, newVal) {
    if (attrName === 'param-id') {
      this.paramId = parseInt(newVal);
    } else if (attrName === 'control-tag') {
      this.controlTag = parseInt(newVal);
    } else if (attrName === 'min') {
      this.minValue = parseFloat(newVal);
    } else if (attrName === 'max') {
      this.maxValue = parseFloat(newVal);
    } else if (attrName === 'label') {
      this.label = newVal;
    } else if (attrName === 'default-value') {
      this.defaultValue = parseFloat(newVal);
    }
  }

  static get observedAttributes() {
    return ['param-id', 'default-value', 'control-tag', 'label', 'min', 'max'/*, 'value'*/];
  }
  
  // Getter and setter for value
  get value() {
    return this.getAttribute('value');
  }

  set value(newValue) {
    this.setAttribute('value', newValue);
  }
}

customElements.define('knob-control', KnobControl);
