// vertical-slider.js
class VerticalSlider extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({ mode: 'open' });
    this._min = parseFloat(this.getAttribute('min')) || 0;
    this._max = parseFloat(this.getAttribute('max')) || 100;
    this._value = parseFloat(this.getAttribute('value')) || 50;
    this._thumbColor = this.getAttribute('thumb-color') || '#fff';
    this._render();
    this._addEventListeners();
  }

  _render() {
    this.shadowRoot.innerHTML = `
      <style>
        * {
          box-sizing: border-box;
        }

        :host {
          display: inline-block;
          width: 24px;
          height: 200px;
        }

        .slider-container {
          position: relative;
          width: 100%;
          height: 100%;
        }

        .slider-track {
          position: absolute;
          width: 4px;
          height: 100%;
          background-color: #ccc;
          border-radius: 2px;
          left: 50%;
          transform: translateX(-50%);
        }

        .slider-thumb {
          position: absolute;
          width: 18px;
          height: 18px;
          background-color: ${this._thumbColor};
          border: 1px solid #ccc;
          border-radius: 100%;
          box-shadow: 0 0 4px rgba(0, 0, 0, 0.2);
          left: 50%;
          transform: translate(-50%, -50%);
          cursor: pointer;
          user-select: none;
        }
      </style>
      <div class="slider-container">
        <div class="slider-track"></div>
        <div class="slider-thumb" style="top: ${this._getThumbPosition()}%;"></div>
      </div>
    `;
  }

_addEventListeners() {
    const sliderContainer = this.shadowRoot.querySelector('.slider-container');
    const sliderThumb = this.shadowRoot.querySelector('.slider-thumb');
    let isDragging = false;

    sliderThumb.addEventListener('mousedown', () => {
      isDragging = true;
      sliderThumb.style.cursor = 'none';
      document.body.style.cursor = 'none';
    });

    document.addEventListener('mousemove', (event) => {
      if (!isDragging) return;
      event.preventDefault();
      const sliderRect = sliderContainer.getBoundingClientRect();
      const mouseY = event.clientY;
      const sliderHeight = sliderRect.height;
      let thumbPosition = ((mouseY - sliderRect.top) / sliderHeight) * 100;
      thumbPosition = Math.max(0, Math.min(100, thumbPosition));
      this._value = (thumbPosition / 100) * (this._max - this._min) + this._min;
      sliderThumb.style.top = `${thumbPosition}%`;
      this.dispatchEvent(new CustomEvent('change', { detail: this._value }));
    });

    document.addEventListener('mouseup', () => {
      if (isDragging) {
        isDragging = false;
        sliderThumb.style.cursor = 'pointer';
        document.body.style.cursor = '';
      }
    });
  }

  _getThumbPosition() {
    const percentage = (this._value - this._min) / (this._max - this._min);
    return percentage * 100;
  }
}

customElements.define('vertical-slider', VerticalSlider);
