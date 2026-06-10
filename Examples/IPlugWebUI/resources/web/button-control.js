class ButtonControl extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({ mode: 'open' });
    
    this.shadowRoot.innerHTML = `
      <style>
        :host {
          display: inline-block;
        }
        
        button {
          background-color: #f3f4f6;
          border: 2px solid #d1d5db;
          border-radius: 6px;
          color: #374151;
          cursor: pointer;
          font-family: system-ui, -apple-system, sans-serif;
          font-size: 14px;
          font-weight: 500;
          padding: 8px 16px;
          transition: all 0.2s ease;
        }

        button:hover {
          background-color: #e5e7eb;
          border-color: #9ca3af;
        }

        button:active {
          background-color: #d1d5db;
          transform: translateY(1px);
        }
      </style>
      <button><slot></slot></button>
    `;

    this.button = this.shadowRoot.querySelector('button');
    this.button.addEventListener('click', () => {
      this.dispatchEvent(new CustomEvent('button-click'));
    });
  }
}

customElements.define('button-control', ButtonControl); 
