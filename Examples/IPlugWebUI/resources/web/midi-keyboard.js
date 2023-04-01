class MidiKeyboard extends HTMLElement {
  constructor() {
    super();

    this.attachShadow({ mode: 'open' });
    this.startNote = parseInt(this.getAttribute('start-note') || 48);
    this.endNote = parseInt(this.getAttribute('end-note') || 72);
    this.callback = this.getAttribute('callback') || null;

    this.svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
    this.shadowRoot.appendChild(this.svg);

    this.keys = this.generateKeys();
    this.drawKeys();

    this.svg.addEventListener('mousedown', this.handleMouseDown.bind(this));
    this.svg.addEventListener('mousemove', this.handleMouseMove.bind(this));
    this.svg.addEventListener('mouseup', this.handleMouseUp.bind(this));
    this.svg.addEventListener('mouseleave', this.handleMouseUp.bind(this));
  }

  connectedCallback() {
    this.style.display = 'block';
    this.style.width = this.style.width || '100%';
    this.style.height = this.style.height || '100px';

    const resizeObserver = new ResizeObserver((entries) => {
      for (const entry of entries) {
        this.svg.setAttribute('width', entry.contentRect.width);
        this.svg.setAttribute('height', entry.contentRect.height);
        this.drawKeys();
      }
    });

    resizeObserver.observe(this);
  }

  generateKeys() {
    const keys = [];

    for (let note = this.startNote; note < this.endNote; note++) {
      const isWhiteKey = [0, 2, 4, 5, 7, 9, 11].includes(note % 12);
      keys.push({
        note,
        isWhiteKey,
        isBlackKey: !isWhiteKey,
        active: false,
      });
    }

    return keys;
  }

  drawKeys() {
    const whiteKeys = this.keys.filter((key) => key.isWhiteKey);
    const blackKeys = this.keys.filter((key) => key.isBlackKey);

    const whiteKeyWidth = this.svg.width.baseVal.value / whiteKeys.length;
    const blackKeyWidth = whiteKeyWidth * 0.6;
    const blackKeyHeight = this.svg.height.baseVal.value * 0.6;

    this.svg.innerHTML = '';

    whiteKeys.forEach((key, index) => {
      const rect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
      rect.setAttribute('x', index * whiteKeyWidth);
      rect.setAttribute('y', 0);
      rect.setAttribute('width', whiteKeyWidth);
      rect.setAttribute('height', this.svg.height.baseVal.value);
      rect.setAttribute('fill', key.active ? 'lightgray' : 'white');
      rect.setAttribute('stroke', 'black');
      this.svg.appendChild(rect);
      key.element = rect;
    });

    blackKeys.forEach((key, index) => {
      const rect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
      const xOffset = (index + Math.floor(index / 2)) * whiteKeyWidth;
      rect.setAttribute('x', xOffset + whiteKeyWidth - blackKeyWidth / 2);
      rect.setAttribute('y', 0);
      rect.setAttribute('width', blackKeyWidth);
      rect.setAttribute('height', blackKeyHeight);
      rect.setAttribute('fill', key.active ? 'darkgray' : 'black');
      rect.setAttribute('stroke', 'black');
      this.svg.appendChild(rect);
      key.element = rect;
    });
  }
      
  getKeyAtPoint(x, y) {
    const elements = this.keys.map((key) => key.element);
    const element = elements.find((el) => {
      const rect = el.getBoundingClientRect();
      return x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom;
    });

    return this.keys.find((key) => key.element === element) || null;
  }

  handleMouseDown(event) {
    event.preventDefault();
    const rect = this.svg.getBoundingClientRect();
    const x = event.clientX;
    const y = event.clientY;
    const key = this.getKeyAtPoint(x, y);

    if (key) {
      key.active = true;
      this.drawKeys();
      const velocity = this.calculateVelocity(y, key.element);
      this.triggerCallback(key.note, velocity);
    }

    this.mouseDown = true;
  }

  handleMouseMove(event) {
    if (!this.mouseDown) return;

    const rect = this.svg.getBoundingClientRect();
    const x = event.clientX;
    const y = event.clientY;
    const key = this.getKeyAtPoint(x, y);

    if (key && !key.active) {
      this.keys.forEach((k) => k.active = false);
      key.active = true;
      this.drawKeys();
      const velocity = this.calculateVelocity(y, key.element);
      this.triggerCallback(key.note, velocity);
    }
  }

  handleMouseUp(event) {
    if (!this.mouseDown) return;

    this.keys.forEach((key) => {
      if (key.active) {
        key.active = false;
        this.triggerCallback(key.note, 0);
      }
    });
    this.drawKeys();
    this.mouseDown = false;
  }

  calculateVelocity(y, element) {
    const rect = element.getBoundingClientRect();
    const relativeY = y - rect.y;
    const velocity = Math.round((1 - relativeY / rect.height) * 127);
    return velocity;
  }

  triggerCallback(note, velocity) {
    if (this.callback && typeof window[this.callback] === 'function') {
      window[this.callback](note, velocity);
    }
  }
}

customElements.define('midi-keyboard', MidiKeyboard);
