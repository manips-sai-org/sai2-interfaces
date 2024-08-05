/**
 * Defines a custom HTML element to set a Redis key with a specified value
 * when clicking the button.
 * 
 * Example usage:
 * <sai2-interface-button key="long_key_name" value="value_to_set" label="Button Label"/>
 * 
 * @module ./module/sai2-interface-button 
 */

import { post_redis_key_val } from '../redis.js';

customElements.define('sai2-interfaces-setkey', class extends HTMLElement {
	constructor() {
		super();
	}

	connectedCallback() {
		const key = this.getAttribute('key');
		const valueToSet = this.getAttribute('value');
		const label = this.getAttribute('label') || 'Button';

		const button = document.createElement('button');
		button.textContent = label;

		button.addEventListener('click', () => {
			post_redis_key_val(key, valueToSet);
		});

		this.appendChild(button);
	}
});