/**
 * @module ./module/sai2-interface-accordion
 */

import { get_redis_val, post_redis_key_val } from '../redis.js';

const template = document.createElement('template');
template.innerHTML = `
  <style>
    .sai2-interfaces-accordion-btn {
      color: white;
      cursor: pointer;
      padding: 18px;
      width: 100%;
      border: none;
      text-align: left;
      outline: none;
      transition: 0.4s;
    }

    .sai2-interfaces-content {
      padding: 0 18px;
      background-color: white;
      max-height: 0;
      overflow: hidden;
      transition: max-height 0.2s ease-out;
    }
  </style>
  <button class="sai2-interfaces-accordion-btn">
  </button>
  <div class="sai2-interfaces-accordion-content">
  </div>
`;

customElements.define('sai2-interfaces-accordion', class extends HTMLElement {
    constructor() {
      super();
      this.template = template;
      this.active = false;
    }

    connectedCallback() {
      let template_node = this.template.content.cloneNode(true);

      let toggleKey = this.getAttribute('key');
      let displayName = this.getAttribute('displayName');

      let button = template_node.querySelector('button');
      let panel = template_node.querySelector('div');
      
      button.innerHTML = displayName;
      button.addEventListener('click', () => {
        post_redis_key_val(toggleKey, this.active ? 0 : 1);
        this.active = !this.active;

        if (this.active)
        {
          button.classList.remove("button-disable");
          button.classList.add("button-enable");
          $(panel).show();
        }
        else
        {
          button.classList.add("button-disable");
          button.classList.remove("button-enable");
          $(panel).hide();
        }
      });

      get_redis_val(toggleKey).then(data => {
        this.active = !!data;
        if (this.active)
        {
          button.classList.remove("button-disable");
          button.classList.add("button-enable");
          $(panel).show();
        }
        else
        {
          button.classList.add("button-disable");
          button.classList.remove("button-enable");
          $(panel).hide();
        }
      });

      // warning: event handlers are not torn down
      panel.innerHTML = this.innerHTML;
      while (this.firstChild)
        this.firstChild.remove();


      // append to document
      this.prepend(template_node);
    }
});