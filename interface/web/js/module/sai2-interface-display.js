import { get_redis_val } from '../redis.js';


const template = document.createElement('template');
template.innerHTML = `
  <style>
    .sai2-interface-display-top {
      display: flex;
      flex-direction: row;
      flex-wrap: wrap;
      width: 100%;
    }

    .sai2-interface-display-top div {
      display: flex;
      flex-direction: row;
      justify-content: space-between;
      width: 100%;
      align-items: center;
    }

    .sai2-interface-display-top div label {
      flex: 1;
      text-align: left;
    }

    .sai2-interface-display-top div input[type=number] {
      flex: 1;
      width: inherit;
    }

  </style>
  <div class="sai2-interface-display-top">
  </div>
`;

customElements.define('sai2-interface-display', class extends HTMLElement {
  constructor() {
    super();
    this.template = template;
    this.connectedCallback = this.connectedCallback.bind(this);
    this.update_value = this.update_value.bind(this);
  }

  update_value() {
    get_redis_val(this.key).then(value => {
      for (let i = 0; i < this.value_inputs.length; i++) {
        let current_value = (Array.isArray(value)) ? value[i] : value;
        this.value_inputs[i].value = current_value.toFixed(this.decimalPlaces);
      }
    });
  }

  connectedCallback() {
    let template_node = this.template.content.cloneNode(true);
    this.key = this.getAttribute('key');
    this.refreshRate = this.getAttribute('refreshRate');
    this.decimalPlaces = this.getAttribute('decimalPlaces');

    // if we can parse as a JSON array, attempt to do so
    let raw_disp = this.getAttribute('display');
    try {
      this.display = JSON.parse(raw_disp);
    } catch (e) {
      this.display = raw_disp;
    }

    this.value_inputs = [];

    // create sliders
    let container = template_node.querySelector('div');
    get_redis_val(this.key).then(value => {
      // determine iteration bounds: 1 if scalar key, array size if vector
      let len = (Array.isArray(value)) ? value.length : 1;
      
      for (let i = 0; i < len; i++) {
        /** 
         * The following js should be equivalent of this:
         * <div>
         * 	 <label>item name</label>
         *   <label>item value</label>
         * <div>
         */

        let key_value_div = document.createElement('div');
        let key_label = document.createElement('label');
        let value_input = document.createElement('input');

        // assign key name based on display attribute
        if (Array.isArray(value)) {
          if (Array.isArray(this.display)) {
            key_label.innerHTML = this.display[i];
          } else {
            key_label.innerHTML = (this.display || this.key) + "[" + i + "]";
          }
        } else {
            key_label.innerHTML = this.display || this.key;
        }

        // set display value
        let current_value = (Array.isArray(value)) ? value[i] : value;
        value_input.type = 'number';
        value_input.className = 'value';
        value_input.disabled = true;
        value_input.value = current_value.toFixed(this.decimalPlaces);

        // add them all together
        key_value_div.append(key_label);
        key_value_div.append(value_input);
        container.append(key_value_div);

        // add to value_labels for later update
        this.value_inputs.push(value_input);
      }

      // TODO: find a way to call clearInterval when this element is not in view
      this.poll_handle = setInterval(this.update_value, this.refreshRate * 1000);
    });

    // append to document
    this.appendChild(template_node);
  }
});