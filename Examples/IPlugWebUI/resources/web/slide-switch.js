class SlideSwitch extends HTMLElement {
    constructor() {
        super();
        this.attachShadow({ mode: 'open' });
        this.shadowRoot.innerHTML = `
            <style>
                :host {
                    display: inline-block;
                    width: 60px;
                }
                .container {
                    display: flex;
                    flex-direction: column;
                    align-items: center;
                    justify-content: center;
                    width: 100%;
                    height: 100%;
                }
                .switch {
                    position: relative;
                    display: inline-block;
                    width: 50px;
                    height: 24px;
                }
                .switch input {
                    display: none;
                }
                .slider {
                    position: absolute;
                    cursor: pointer;
                    top: 0;
                    left: 0;
                    right: 0;
                    bottom: 0;
                    background-color: #ccc;
                    transition: 0.1s;
                    border-radius: 34px;
                }
                .slider:before {
                    position: absolute;
                    content: "";
                    height: 16px;
                    width: 16px;
                    left: 4px;
                    bottom: 4px;
                    background-color: white;
                    transition: 0.4s;
                    border-radius: 50%;
                }
                input:checked + .slider {
                    background-color: #4cd964;
                }
                input:checked + .slider:before {
                    transform: translateX(26px);
                }
            </style>
            <div class="container">
            <label class="switch">
                <input type="checkbox">
                <span class="slider"></span>
            </label>
            </div>
        `;
    }

    connectedCallback() {
        this.updateHandleColor();
    }

    static get observedAttributes() {
        return ['handle-color'];
    }

    attributeChangedCallback(name, oldValue, newValue) {
        if (name === 'handle-color') {
            this.updateHandleColor();
        }
    }

    updateHandleColor() {
        const handleColor = this.getAttribute('handle-color');
        if (handleColor) {
            this.shadowRoot.querySelector('.slider').style.setProperty('--handle-color', handleColor);
            this.shadowRoot.querySelector('.slider').style.backgroundColor = `var(--handle-color)`;
        }
    }
}

customElements.define('slide-switch', SlideSwitch);

