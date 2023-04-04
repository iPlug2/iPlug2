class KnobControl extends HTMLElement {
    constructor() {
        super();

        const label = this.getAttribute('label') || '';
        const minValue = parseFloat(this.getAttribute('min')) || 0;
        const maxValue = parseFloat(this.getAttribute('max')) || 100;
        const units = this.getAttribute('units') || '';
        const minAngle = parseFloat(this.getAttribute('min-angle')) || -135;
        const maxAngle = parseFloat(this.getAttribute('max-angle')) || 135;

        this.attachShadow({ mode: 'open' });
        this.shadowRoot.innerHTML = `
            <style>
                :host {
                    display: inline-block;
                    width: 80px;
                    height: 110px;
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
                    color: white;
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
                <div class="label">${label}</div>
                <svg viewBox="0 0 100 100" width="60" height="60">
                    <circle cx="50" cy="50" r="42" fill="#333" stroke="#fff" stroke-width="2" @mousedown="startDrag"></circle>
                    <path class="value-arc" fill="none" stroke="#fff" stroke-width="2" d=""></path>
                    <line class="pointer" x1="50" y1="10" x2="50" y2="50" stroke="#fff" stroke-width="4" transform="rotate(0, 50, 50)"></line>
                </svg>
                <div class="value">0 ${units}</div>
            </div>
        `;

        const pointer = this.shadowRoot.querySelector('.pointer');
        const valueElement = this.shadowRoot.querySelector('.value');
        let currentValue = minValue;

        const valueArc = this.shadowRoot.querySelector('.value-arc');

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

        const updateValue = (value) => {
            currentValue = value;
            valueElement.textContent = `${value.toFixed(1)} ${units}`;
            const angle = minAngle + ((value - minValue) / (maxValue - minValue)) * (maxAngle - minAngle);
            pointer.setAttribute('transform', `rotate(${angle}, 50, 50)`);
            updateValueArc(angle);
        };

        const startDrag = (e) => {

            if (e.button == 2) return; // right click (context menu

            e.preventDefault();
            const svg = e.currentTarget.parentElement;

            let initialY = ('touches' in e) ? e.touches[0].clientY : e.clientY;
            let initialValue = currentValue;

            const onMove = (e) => {
                const clientY = ('touches' in e) ? e.touches[0].clientY : e.clientY;
                const deltaY = initialY - clientY;
                const valueChange = deltaY * (maxValue - minValue) / 100;
                const value = Math.min(Math.max(initialValue + valueChange, minValue), maxValue);
                updateValue(value);
            };

            const onEnd = () => {
                document.removeEventListener('mousemove', onMove);
                document.removeEventListener('mouseup', onEnd);
                document.removeEventListener('touchmove', onMove);
                document.removeEventListener('touchend', onEnd);
                document.body.classList.remove('hidden-cursor');
                document.body.style.cursor = '';
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
            const valueChange = delta * (maxValue - minValue) / 100;
            const value = Math.min(Math.max(currentValue + valueChange, minValue), maxValue);
            updateValue(value);
        };

        this.shadowRoot.querySelector('circle').addEventListener('mousedown', startDrag);
        this.shadowRoot.querySelector('circle').addEventListener('touchstart', startDrag, { passive: false });
        this.shadowRoot.querySelector('circle').addEventListener('wheel', onWheel);

        updateValue(currentValue);
    }
}

function polarToCartesian(centerX, centerY, radius, angleInDegrees) {
    const angleInRadians = (angleInDegrees - 90) * Math.PI / 180.0;
    return {
        x: centerX + (radius * Math.cos(angleInRadians)),
        y: centerY + (radius * Math.sin(angleInRadians))
    };
}

customElements.define('knob-control', KnobControl);
