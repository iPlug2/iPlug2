// svg-component.js
class SVGButton extends HTMLElement {
    constructor() {
      super();
  
      // Create a shadow DOM
      this.attachShadow({ mode: 'open' });
  
      // Set default values
      this._svgUrl = this.getAttribute('svg-url') || '';
      this._bgColor = this.getAttribute('bg-color') || '#1d1a1f';
      this._callback = this.getAttribute('callback') || null;
    }
  
    connectedCallback() {
      this.render();
    }
  
    static get observedAttributes() {
      return ['svg-url', 'bg-color', 'callback'];
    }
  
    attributeChangedCallback(name, oldValue, newValue) {
      switch (name) {
        case 'svg-url':
          this._svgUrl = newValue;
          break;
        case 'bg-color':
          this._bgColor = newValue;
          break;
        case 'callback':
          this._callback = newValue;
          break;
      }
      this.render();
    }
  
    handleClick() {
      if (this._callback && typeof window[this._callback] === 'function') {
        window[this._callback]();
      }
    }
  
    render() {
      this.shadowRoot.innerHTML = `
        <style>
          .custom-button {
            background-color: ${this._bgColor};
            border: none;
            cursor: pointer;
            display: inline-flex;
            align-items: center;
            justify-content: center;
            padding: 0;
            border-radius: 4px;
            width: 100%;
            height: 100%;
          }
          .custom-button:hover {
            background-color: ${this._bgColor === '#f0f0f0' ? '#e0e0e0' : this._bgColor};
          }
          img {
            max-width: 100%;
          }
        </style>
        <button class="custom-button">
          <img src="${this._svgUrl}" alt="SVG Icon">
        </button>
      `;
  
      // Attach the click event listener
      this.shadowRoot.querySelector('.custom-button').addEventListener('click', this.handleClick.bind(this));
    }
  }
  
  // Register the custom element
  customElements.define('svg-button', SVGButton);
  