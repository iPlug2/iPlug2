class ColorPickerButton extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({ mode: 'open' });
    this.shadowRoot.innerHTML = `
      <style>
        .color-picker-button {
          display: inline-block;
          width: 100px;
          height: 100px;
          background-color: red;
          cursor: pointer;
          border: none;
          outline: none;
        }
        .color-picker-popup {
          display: none;
          position: absolute;
          background-color: white;
          border: 1px solid #ccc;
          padding: 10px;
          z-index: 1;
        }
        .color-picker-popup ul {
          list-style: none;
          padding: 0;
          margin: 0;
        }
        .color-picker-popup li {
          cursor: pointer;
          padding: 5px;
          transition: background-color 0.3s;
        }
        .color-picker-popup li:hover {
          background-color: #f0f0f0;
        }
      </style>
      <button class="color-picker-button"></button>
      <div class="color-picker-popup">
        <ul>
          <li data-color="red">Red</li>
          <li data-color="orange">Orange</li>
          <li data-color="yellow">Yellow</li>
          <li data-color="green">Green</li>
          <li data-color="blue">Blue</li>
          <li data-color="indigo">Indigo</li>
          <li data-color="violet">Violet</li>
        </ul>
      </div>
    `;

    this.buttonElement = this.shadowRoot.querySelector('.color-picker-button');
    this.popupElement = this.shadowRoot.querySelector('.color-picker-popup');
    this.colorListItems = this.shadowRoot.querySelectorAll('.color-picker-popup li');

    this.buttonElement.addEventListener('click', () => this.toggleColorPicker());
    this.colorListItems.forEach(item => {
      item.addEventListener('click', (event) => this.changeButtonColor(event));
    });
  }

  toggleColorPicker() {
    if (this.popupElement.style.display === 'none') {
      this.popupElement.style.display = 'block';
    } else {
      this.popupElement.style.display = 'none';
    }
  }

  changeButtonColor(event) {
    const selectedColor = event.target.getAttribute('data-color');
    this.buttonElement.style.backgroundColor = selectedColor;
    this.toggleColorPicker();
  }
}

customElements.define('color-picker-button', ColorPickerButton);
