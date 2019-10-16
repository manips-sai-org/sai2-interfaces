/**
 * Defines a custom HTML element to issue HTTP requests to start/stop
 * a background logger.
 * <br>
 * Example usage:
 * &lt;sai2-interfaces-logger /&gt;
 * <br>
 * Note: there are no available attributes to set.
 * 
 * @module ./module/sai2-interfaces-logger
 */


import Sai2InterfacesComponent from './sai2-interfaces-component.js';


const template = document.createElement('template');
template.innerHTML = `<div></div>`;

class Sai2InterfacesSelectOption extends Sai2InterfacesComponent {
  constructor() {
    super(template);
  }

  onMount() {
  }

  refresh() {
    for (let child of this.querySelectorAll("*")) {
      if (typeof child.refresh === 'function') {
        child.refresh();
      }
    }
  }

  onUnmount() {
  }

  enableComponent() {
  }

  disableComponent() {
  }
}


customElements.define('sai2-interfaces-select-option', Sai2InterfacesSelectOption);