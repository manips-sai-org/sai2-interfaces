/**
 * Defines a custom HTML element to select between "windows". Effectively a TabView,
 * without the tabs and using a select element instead.
 * 
 * Sets a Redis key to the new tab name when the user switches tabs/selects a new item.
 * The direct application is to switch between control primitives.
 * 
 * <pre>
 * Example usage:
 * &lt;sai2-interface-select key=""&gt;
 *   &lt;option value="key_name_1"&gt;Friendly Name 1&lt;/option&gt;
 *   &lt;option value="key_name_2"&gt;Friendly Name 2&lt;/option&gt;
 * &lt;/sai2-interface-select&gt;
 * 
 * &lt;div class="key_name_1"&gt;&lt;/div&gt;
 * &lt;div class="key_name_2"&gt;&lt;/div&gt;
 * </pre>
 * 
 * So when key_name_2 is selected, the div .key_name_1 is hidden.
 * @module ./module/sai2-interface-select 
 */

import { get_redis_val, post_redis_key_val } from '../redis.js';

const template = document.createElement('template');
template.innerHTML = `
  <select class="primitive_selector">
  </select>
`;

customElements.define('sai2-interfaces-select', class extends HTMLElement {
    constructor() {
      super();
      this.template = template;
      this.get_redis_val_and_update = this.get_redis_val_and_update.bind(this);
    }

    connectedCallback() {
      let template_node = this.template.content.cloneNode(true);
      let current_primitive_key = this.getAttribute("currentPrimitiveKey");

      this.selector_dom = template_node.querySelector('select');

      // generate options
      for (let child of this.children) {
        let option = document.createElement('option');
        option.value = child.getAttribute("key");
        option.innerHTML = child.getAttribute("name");
        this.selector_dom.appendChild(option);
      }

      this.selector_dom.onchange = e => {
        let option = e.target.value;
        post_redis_key_val(current_primitive_key, option);
        this.show_module(option);
      };

      // fetch initial value from redis
      this.get_redis_val_and_update();

      // append to document
      this.prepend(template_node);
    }

    show_module(option) {
      // hide all modules
      $('sai2-interfaces-select-option').hide();

      // find & reshow
      for (let child of this.children) {
        if (child.getAttribute("key") === option) {
          this.selector_dom.value = option;
          
          // TODO: timing hack 
          setTimeout(() => {
            if (typeof child.refresh === 'function') {
              $(child).show();

              setTimeout(() => {
                child.refresh();
              }, 100);
            }
          }, 100);
        }
      }
    }

    // read from redis on page load
    get_redis_val_and_update() {
      get_redis_val(this.getAttribute('currentPrimitiveKey')).then(option => {
        this.show_module(option);
        this.selector_dom.value = option;
      });
    }
});