
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
</div>
`;

class Sai2InterfacesToggleGroup extends HTMLElement {
	constructor() {
		super();
		this.template = template;
		this.enabled = false;
	}

	connectedCallback() {
		let template_node = this.template.content.cloneNode(true);

		this.key = this.getAttribute("key");
		this.display = this.getAttribute("name");

		this.container = template_node.querySelector(".sai2-interfaces-toggle-group-container");
		this.checkbox = template_node.querySelector("input");
		this.label = template_node.querySelector("span");
		this.label.innerHTML = this.display;

		if (this.key) {
			get_redis_val(this.key).then(value => {
				this.enabled = value;
				this.checkbox.checked = value;
				this.updateGroups();
			});
		}

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

		this.prepend(template_node);
	}

	updateGroups() {
		let enabled_group = this.querySelectorAll(':scope > sai2-interfaces-toggle-group-enabled');
		let disabled_group = this.querySelectorAll(':scope > sai2-interfaces-toggle-group-disabled');

		enabled_group = $(enabled_group);
		disabled_group = $(disabled_group);

		if (this.enabled) {
			enabled_group.each(function () {
				for (let child of this.children) {
					if (typeof child.refresh == 'function') {
						child.refresh();
					}
				}
			});
			enabled_group.show();
			disabled_group.hide();
		} else {
			enabled_group.hide();
			disabled_group.show();
			disabled_group.each(function () {
				for (let child of this.children) {
					if (typeof child.refresh == 'function') {
						child.refresh();
					}
				}
			});
		}
	}

	refresh() {
		for (let child of this.children) {
			if (typeof child.refresh == 'function') {
				child.refresh();
			}
		}
	}
}

class ToggleGroupChildEnabled extends HTMLElement {
	refresh() {
		for (let child of this.children) {
			if (typeof child.refresh == 'function') {
				child.refresh();
			}
		}
	}
}

class ToggleGroupChildDisabled extends HTMLElement {
	refresh() {
		for (let child of this.children) {
			if (typeof child.refresh == 'function') {
				child.refresh();
			}
		}
	}
}

customElements.define('sai2-interfaces-toggle-group', Sai2InterfacesToggleGroup);
customElements.define('sai2-interfaces-toggle-group-enabled', ToggleGroupChildEnabled);
customElements.define('sai2-interfaces-toggle-group-disabled', ToggleGroupChildDisabled);