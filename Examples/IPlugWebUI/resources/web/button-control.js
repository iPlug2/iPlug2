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
          background-color: var(--surface, #ffffff);
          border: 1px solid var(--border, #c9ced6);
          border-radius: 6px;
          color: var(--text, #18202d);
          cursor: pointer;
          font-family: system-ui, -apple-system, sans-serif;
          font-size: var(--control-font-size, 14px);
          font-weight: 500;
          line-height: 1.2;
          min-height: var(--control-min-height, 36px);
          padding: var(--control-padding-block, 8px) var(--control-padding-inline, 16px);
          white-space: nowrap;
          box-shadow: 0 1px 1px rgba(24, 32, 45, 0.05);
          transition: all 0.2s ease;
        }

        button:hover {
          background-color: var(--surface-hover, #e8edf3);
          border-color: var(--border-strong, #98a2b3);
        }

        button:active {
          background-color: var(--surface-subtle, #eef1f5);
          transform: translateY(1px);
        }

        button:focus-visible {
          outline: 2px solid var(--accent, #2563eb);
          outline-offset: 2px;
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
