
class BitmapKnob extends HTMLElement {
  constructor() {
      super();

      const shadow = this.attachShadow({ mode: 'open' });

      const knobContainer = document.createElement('div');
      knobContainer.classList.add('knob-container');

      const knobImage = document.createElement('img');
      knobImage.classList.add('knob-image');
      knobImage.src = this.getAttribute('bitmap-src');
      
      const style = document.createElement('style');
      style.textContent = `
          .knob-container {
              position: relative;
              overflow: hidden;
          }
          .knob-image {
              position: absolute;
              width: 100%;
              will-change: transform;
          }
      `;
      shadow.appendChild(style);

      knobContainer.appendChild(knobImage);
      shadow.appendChild(knobContainer);
      this._rotationIndex = 0;
      this._totalFrames = 60;
      this._knobHeight = 0;

      knobImage.onload = () => {
          this._knobHeight = (knobImage.naturalHeight / this._totalFrames) * 0.5;
          const knobWidth = knobImage.naturalWidth * 0.5;
          knobContainer.style.width = `${knobWidth}px`;
          knobContainer.style.height = `${this._knobHeight}px`;
          this.updateKnobPosition();
      };

      knobContainer.addEventListener('mousedown', (e) => {
          e.preventDefault();
          const startY = e.clientY;
          const startRotationIndex = this._rotationIndex;

          const sensitivity = 5;

          //document.documentElement.style.cursor = 'none'; // Hide the cursor while dragging
          const onMouseDown = () => {
              this.triggerCallback('on-mouse-down');

              //document.documentElement.style.cursor = ''; // Reset cursor style when dragging stops
          };
        
          const onMouseMove = (e) => {
              const deltaY = startY - e.clientY;
              const deltaRotation = Math.round((deltaY * sensitivity) / this._knobHeight);
              this._rotationIndex = Math.min(Math.max(startRotationIndex + deltaRotation, 0), this._totalFrames - 1);
              this.updateKnobPosition();
          };

          const onMouseUp = () => {
              document.removeEventListener('mousemove', onMouseMove);
              document.removeEventListener('mouseup', onMouseUp);
            
              this.triggerCallback('on-mouse-up');

              //document.documentElement.style.cursor = ''; // Reset cursor style when dragging stops
          };

          document.addEventListener('mousemove', onMouseMove);
          document.addEventListener('mouseup', onMouseUp);
          document.addEventListener('mousedown', onMouseDown);
      });
  }

  updateKnobPosition() {
      const knobImage = this.shadowRoot.querySelector('.knob-image');
      const offset = -this._knobHeight * this._rotationIndex;
      knobImage.style.transform = `translateY(${offset}px)`;
  }

  connectedCallback() {
      if (!this.hasAttribute('bitmap-src')) {
          throw new Error("You must provide a 'bitmap-src' attribute with the path to your bitmap image.");
      }
  }
  
  triggerCallback(attributeName) {
    const callbackName = this.getAttribute(attributeName);
    if (callbackName && typeof window[callbackName] === 'function') {
      window[callbackName]();
    }
  }

  static get observedAttributes() {
      return ['bitmap-src'];
  }

  attributeChangedCallback(name, oldValue, newValue) {
      if (name === 'bitmap-src' && oldValue !== newValue) {
          const knobImage = this.shadowRoot.querySelector('.knob-image');
          knobImage.src = newValue;
      }
  }
}

customElements.define('bitmap-knob', BitmapKnob);


// class KnobBase extends HTMLElement {
//     constructor() {
//         super();

//         this._rotationIndex = 0;
//         this._totalFrames = 60;
//         this._sensitivity = 5;

//         this.attachEventListeners();
//     }

//     attachEventListeners() {
//         this.addEventListener('mousedown', (e) => {
//             e.preventDefault();

//             const startRotationIndex = this._rotationIndex;

//             const onMouseMove = (e) => {
//                 const deltaY = e.movementY;
//                 const deltaRotation = Math.round((-deltaY * this._sensitivity) / this._totalFrames);
//                 this._rotationIndex = Math.min(Math.max(startRotationIndex + deltaRotation, 0), this._totalFrames - 1);
//                 this.updateKnobPosition();
//             };

//             const onMouseUp = () => {
//                 document.removeEventListener('mousemove', onMouseMove);
//                 document.removeEventListener('mouseup', onMouseUp);
//             };
//         });
//     }

//     updateKnobPosition() {
//         // This method should be implemented by subclasses to update the knob's position based on this._rotationIndex
//         throw new Error('updateKnobPosition method should be implemented by subclasses.');
//     }
// }

// class BitmapKnob extends KnobBase {
//     constructor() {
//         super();

//         const shadow = this.attachShadow({ mode: 'open' });

//         const style = document.createElement('style');
//         style.textContent = `
//             .knob-container {
//                 position: relative;
//                 overflow: hidden;
//             }
//             .knob-image {
//                 position: absolute;
//                 width: 100%;
//                 will-change: transform;
//             }
//         `;
//         shadow.appendChild(style);

//         const knobContainer = document.createElement('div');
//         knobContainer.classList.add('knob-container');

//         const knobImage = document.createElement('img');
//         knobImage.classList.add('knob-image');
//         knobImage.src = this.getAttribute('bitmap-src');

//         knobContainer.appendChild(knobImage);
//         shadow.appendChild(knobContainer);

//         this._knobHeight = 0;

//         knobImage.onload = () => {
//             this._knobHeight = knobImage.naturalHeight / this._totalFrames;
//             const knobWidth = knobImage.naturalWidth;
//             knobContainer.style.width = `${knobWidth}px`;
//             knobContainer.style.height = `${this._knobHeight}px`;
//             this.updateKnobPosition();
//         };
//     }

//     updateKnobPosition() {
//         const knobImage = this.shadowRoot.querySelector('.knob-image');
//         const offset = -this._knobHeight * this._rotationIndex;
//         knobImage.style.transform = `translateY(${offset}px)`;
//     }
// }

// customElements.define('knob-base', KnobBase);
// customElements.define('bitmap-knob', BitmapKnob);
