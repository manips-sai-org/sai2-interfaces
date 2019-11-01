
import { get_redis_val, post_redis_key_val } from '../redis.js';
import Sai2InterfacesComponent from './sai2-interfaces-component.js';

const template = document.createElement('template');
template.innerHTML = `
  <style>
    .sai2-interfaces-toggle-group-top {
      display: flex;
      flex-direction: column;
      flex-wrap: wrap;
    }
  </style>
  <div class="sai2-interfaces-toggle-group-top">
  <label>
    <input type="checkbox">
    <span class="checkable"></span>
  </label>
    <div class="sai2-interfaces-toggle-group-container"></div>
  </div>
`;

class Sai2InterfacesToggleGroup extends Sai2InterfacesComponent {
  constructor() {
    super(template);
    this.enabled = false;
  }

  onMount() {
    this.key = this.getAttribute("key");
    this.display = this.getAttribute("name");

    this.container = this.template_node.querySelector(".sai2-interfaces-toggle-group-container");
    this.checkbox = this.template_node.querySelector("input");
    this.label = this.template_node.querySelector("span");
    this.label.innerHTML = this.display;

    if (this.key) {
      get_redis_val(this.key).then(value => {
        this.enabled = value;
        this.checkbox.checked = value;
        this.updateGroups();
      });
    }
    
    // move all children to internal container
    // warning: event handlers not torn down
    this.container.innerHTML = this.innerHTML;
    while (this.firstChild)
      this.firstChild.remove();

    this.checkbox.onchange = e => {
      this.enabled = e.target.checked;

      // push update to redis if not in memory
      if (this.key) {
        post_redis_key_val(this.key, this.enabled ? 1 : 0);
      }
      
      this.updateGroups();
    };

    if (!this.key) {
      setTimeout(() => this.updateGroups(), 100);
    }
  }

  updateGroups() {
    if (this.enabled) {
      $(this).find('sai2-interfaces-toggle-group-enabled').show();
      $(this).find('sai2-interfaces-toggle-group-disabled').hide();
    } else {
      $(this).find('sai2-interfaces-toggle-group-enabled').hide();
      $(this).find('sai2-interfaces-toggle-group-disabled').show();
    }
  }

  onUnmount() {
  }

  enableComponent() {
  }

  disableComponent() {
  }
}

customElements.define('sai2-interfaces-toggle-group', Sai2InterfacesToggleGroup);
customElements.define('sai2-interfaces-toggle-group-enabled', class extends HTMLDivElement{}, { extends: 'div' });
customElements.define('sai2-interfaces-toggle-group-disabled', class extends HTMLDivElement{}, { extends: 'div' })