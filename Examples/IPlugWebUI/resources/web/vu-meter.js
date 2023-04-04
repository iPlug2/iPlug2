//class VUMeter extends HTMLElement {
//    constructor() {
//        super();
//        this.attachShadow({mode: 'open'});
//        this.shadowRoot.innerHTML = `
//            <style>
//                :host {
//                    display: block;
//                    width: 30px;
//                    height: 300px;
//                }
//
//                .vu-meter {
//                    position: relative;
//                    width: 100%;
//                    height: 100%;
//                    background-color: #ddd;
//                    border: 1px solid #999;
//                    border-radius: 5px;
//                }
//
//                .needle {
//                    position: absolute;
//                    bottom: 0;
//                    left: 50%;
//                    width: 2px;
//                    height: 100%;
//                    background-color: #000;
//                    transform-origin: bottom;
//                }
//
//                .scale {
//                    position: absolute;
//                    left: 100%;
//                    height: 100%;
//                    width: 50px;
//                    display: flex;
//                    flex-direction: column;
//                    justify-content: space-between;
//                    padding-left: 5px;
//                }
//
//                .scale::before,
//                .scale::after {
//                    content: '';
//                    position: absolute;
//                    height: 1px;
//                    width: 100%;
//                    background-color: #999;
//                }
//
//                .scale::before {
//                    top: 0;
//                }
//
//                .scale::after {
//                    bottom: 0;
//                }
//
//                .red-mark {
//                    color: red;
//                }
//            </style>
//
//            <div class="vu-meter">
//                <div class="needle"></div>
//                <div class="scale"></div>
//            </div>
//        `;
//    }
//
//    connectedCallback() {
//        this.scale = this.shadowRoot.querySelector('.scale');
//        this.needle = this.shadowRoot.querySelector('.needle');
//
//        this.createMarkings();
//    }
//
//    createMarkings() {
//        for (let i = -70; i <= 3; i += 10) {
//            const mark = document.createElement('div');
//            mark.style.color = i > 0 ? 'red' : 'black';
//            mark.textContent = i;
//            this.scale.appendChild(mark);
//        }
//    }
//
//    setAudioLevel(value) {
//        const rotation = ((value + 70) / 73) * 100;
//        this.needle.style.transform = `rotate(-${100 - rotation}deg)`;
//    }
//}
//
//customElements.define('vu-meter', VUMeter);

class VUMeter extends HTMLElement {
    constructor() {
        super();
        this.attachShadow({ mode: 'open' });

        const svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
        svg.setAttribute('width', '300');
        svg.setAttribute('height', '200');
        svg.setAttribute('viewBox', '0 0 300 200');

        const bgRect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
        bgRect.setAttribute('x', '0');
        bgRect.setAttribute('y', '0');
        bgRect.setAttribute('width', '300');
        bgRect.setAttribute('height', '200');
        bgRect.setAttribute('fill', '#ddd');
        svg.appendChild(bgRect);

        const needle = document.createElementNS('http://www.w3.org/2000/svg', 'line');
        needle.setAttribute('x1', '50');
        needle.setAttribute('y1', '290');
        needle.setAttribute('x2', '50');
        needle.setAttribute('y2', '10');
        needle.setAttribute('stroke', 'black');
        needle.setAttribute('stroke-width', '2');
        svg.appendChild(needle);

        const updateNeedle = (value) => {
            const rotation = ((value + 70) / 73) * 100;
            const angle = 180 - (rotation * 180 / 100);
            needle.setAttribute('transform', `rotate(${angle}, 50, 290)`);
        };

        // Create marks and labels
        for (let i = -70; i <= 3; i += 10) {
            const tickAngle = 180 - ((((i + 70) / 73) * 100) * 180 / 100);
            const labelAngle = tickAngle - 90;
            const x1 = 50 + 140 * Math.cos(tickAngle * Math.PI / 180);
            const y1 = 290 - 140 * Math.sin(tickAngle * Math.PI / 180);
            const x2 = 50 + 130 * Math.cos(tickAngle * Math.PI / 180);
            const y2 = 290 - 130 * Math.sin(tickAngle * Math.PI / 180);

            const tick = document.createElementNS('http://www.w3.org/2000/svg', 'line');
            tick.setAttribute('x1', x1);
            tick.setAttribute('y1', y1);
            tick.setAttribute('x2', x2);
            tick.setAttribute('y2', y2);
            tick.setAttribute('stroke', i > 0 ? 'red' : 'black');
            tick.setAttribute('stroke-width', '2');
            svg.appendChild(tick);

            const labelX = 50 + 115 * Math.cos(labelAngle * Math.PI / 180);
            const labelY = 290 - 115 * Math.sin(labelAngle * Math.PI / 180);

            const label = document.createElementNS('http://www.w3.org/2000/svg', 'text');
                                                   label.setAttribute('x', labelX);
                                                   label.setAttribute('y', labelY);
                                                   label.setAttribute('text-anchor', 'middle');
                                                   label.setAttribute('dominant-baseline', 'central');
                                                   label.setAttribute('fill', i > 0 ? 'red' : 'black');
                                                   label.setAttribute('font-size', '12');
                                                   label.textContent = i;
                                                   svg.appendChild(label);
                                      }
                                      this.shadowRoot.appendChild(svg);

                                      // Simulate audio signal (replace this with actual audio input)
                                      setInterval(() => {
                                          const value = Math.floor(Math.random() * (103) - 70);
                                          updateNeedle(value);
                                      }, 100);
                                  }
                                      }

                                      customElements.define('vu-meter', VUMeter);
