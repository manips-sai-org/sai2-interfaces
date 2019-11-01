/**
 * Defines a custom HTML element to set a Redis key with
 * a set of predefined values.
 * <br>
 * <pre>
 * Example usage:
 * <sai2-interface-enum key="long_key_name" display="friendly_key_name">
 *   <option>A</option>
 *   <option>B</option>
 *   <option>C</option>
 * </sai2-interface-enum>
 * </pre>
 * @module ./module/sai2-interface-enum
 */

import { get_redis_val, post_redis_key_val } from '../redis.js';

const template = document.createElement('template');
template.innerHTML = `
  <label></label>
  <select>
  </select>
`;

customElements.define('sai2-interfaces-enum', class extends HTMLElement {
  constructor() {
    super();
    this.template = template;
    this.display = this.getAttribute('display');
    this.key = this.getAttribute('key');

    // assumption: values are strings/numbers
    this.value_map = {};

    this.get_redis_val_and_update = this.get_redis_val_and_update.bind(this);
  }

  connectedCallback() {
    let template_node = this.template.content.cloneNode(true);
    this.label_node = template_node.querySelector('label');
    this.selector_dom = template_node.querySelector('select');

    this.label_node.innerHTML = this.display;

    // insert children from parent index.html into <select>
    while (this.children.length) {
      let option = this.children[0];
      this.selector_dom.appendChild(option);
      this.value_map[option.value] = option.innerHTML;
    }

    this.selector_dom.onchange = e => {
      let raw_option = e.target.value;

      // attempt to parse as number
      let option = parseFloat(raw_option);
      if (isNaN(option))
        option = raw_option;
        
      post_redis_key_val(this.key, option);
    }

    // fetch initial value from redis
    this.get_redis_val_and_update();

    // append to document
    this.appendChild(template_node);
  }

  /**
   * Fetches the latest value of the key attribute from 
   * Redis, and updates the internal value of this enum.
   */
  get_redis_val_and_update() {
    get_redis_val(this.key).then(option => {
      if (!(option in this.value_map)) {
        alert(option + ' not found in enum');
      } else {
        this.selector_dom.value = option;
      }

      this.selector_dom.value = option;
    });
  }
});
