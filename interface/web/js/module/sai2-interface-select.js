import { REDIS_KEY_CURRENT_PRIMITIVE, EVENT_NOT_READY, EVENT_READY } from '../const.js';
import { get_redis_val, post_redis_key_val } from '../redis.js';

const template = document.createElement('template');
template.innerHTML = `
  <select class="primitive_selector">
  </select>
`;

customElements.define('sai2-interface-select', class extends HTMLElement {
    constructor() {
      super();
      this.template = template;
      this.get_redis_val_and_update = this.get_redis_val_and_update.bind(this);
    }

    connectedCallback() {
      let template_node = this.template.content.cloneNode(true);
      this.selector_dom = template_node.querySelector('select');

      // insert children from parent index.html into <select>
      while (this.children.length) {
        let option = this.children[0];
        this.selector_dom.appendChild(option);
      }

      this.selector_dom.onchange = e => {
        let option = e.target.value;
        post_redis_key_val(REDIS_KEY_CURRENT_PRIMITIVE, option);
        this.show_module(option);
      };

      // do nothing on EVENT_NOT_READY, so no need to register callback
      // register for controller ready
      document.addEventListener(EVENT_READY, () => {
        this.get_redis_val_and_update();
      });

      // fetch initial value from redis
      this.get_redis_val_and_update();

      // append to document
      this.appendChild(template_node);
    }

    show_module(option) {
      // hide all modules
      $('.module').hide();

      // show selected module
      $(document.getElementById(option)).show();
    }

    // read from redis on page load
    get_redis_val_and_update() {
      get_redis_val(REDIS_KEY_CURRENT_PRIMITIVE).then(option => {
        this.show_module(option);
        this.selector_dom.value = option;
      });
    }
});