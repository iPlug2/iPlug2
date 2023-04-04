class ComboBox extends HTMLElement {
    constructor() {
        super();
        this.attachShadow({ mode: 'open' });
    }

    connectedCallback() {
        const options = this.parseOptions(this.getAttribute('options'));
        this.render(options);
    }

    parseOptions(optionsText) {
        return optionsText.split(',').map((option, index) => ({
            value: `option${index + 1}`,
            label: option.trim()
        }));
    }

    render(options) {
        this.shadowRoot.innerHTML = `
            <style>
            .combo-box {
                display: inline-block;
                position: relative;
            }
            select {
                width: 100%;
            }
            </style>
            <div class="combo-box">
                <select>
                    ${options.map(option => `<option value="${option.value}">${option.label}</option>`).join('')}
                </select>
            </div>
        `;
    }
}

customElements.define('combo-box', ComboBox);