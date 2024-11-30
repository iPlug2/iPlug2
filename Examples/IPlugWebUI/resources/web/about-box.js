class AboutBox extends HTMLElement {
    constructor() {
      super();
      this.attachShadow({ mode: "open" });

      const template = document.getElementById("about-box-template");
      this.shadowRoot.appendChild(template.content.cloneNode(true));

      this.aboutBox = this.shadowRoot.querySelector(".about-box");
      this.closeBtn = this.shadowRoot.getElementById("close-btn");

      this.closeBtn.addEventListener("click", () => {
        this.hide();
      });
    }

    show() {
      this.style.display = "block";
      setTimeout(() => {
        this.aboutBox.style.opacity = "1";
      }, 100);
    }

    hide() {
      this.aboutBox.style.opacity = "0";
      setTimeout(() => {
        this.style.display = "none";
      }, 500);
    }
  }

  customElements.define("about-box", AboutBox);
