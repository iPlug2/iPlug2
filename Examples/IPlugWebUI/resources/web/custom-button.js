class CustomButton extends HTMLElement {
  constructor() {
    super();

    const shadow = this.attachShadow({ mode: 'open' });

    const style = document.createElement('style');
    style.textContent = `
      button {
        width: 100px;
        height: 100px;
        background-color: var(--button-color, red);
        border: none;
        cursor: pointer;
        transition: background-color 100ms ease-in-out, border 100ms ease-in-out;
      }

      button:focus {
        outline: none;
      }

      button:hover {
        border: 1px solid black;
      }
    `;

    const button = document.createElement('button');
    button.textContent = this.getAttribute('label') || 'Click me';

    const buttonColor = this.getAttribute('color');
    if (buttonColor) {
      button.style.setProperty('--button-color', buttonColor);
    }

    shadow.appendChild(style);
    shadow.appendChild(button);

    button.addEventListener('mousedown', () => {
      this.flashWhite(button);
      this.triggerCallback('on-flash');
    });

    button.addEventListener('mouseover', () => {
      this.triggerCallback('on-hover');
    });
  }

  flashWhite(button) {
    const originalColor = button.style.getPropertyValue('--button-color');
    button.style.backgroundColor = 'white';
    setTimeout(() => {
      button.style.backgroundColor = originalColor;
    }, 50);
  }

  triggerCallback(attributeName) {
    const callbackName = this.getAttribute(attributeName);
    if (callbackName && typeof window[callbackName] === 'function') {
      window[callbackName]();
    }
  }
}

customElements.define('custom-button', CustomButton);
