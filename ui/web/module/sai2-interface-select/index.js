(function() {
var template = document.currentScript.ownerDocument.querySelector('#template-select');

customElements.define('sai2-interface-select', class extends HTMLElement {
	constructor() {
		super();
		this.template = template;
	}

	connectedCallback() {
		let template_node = this.template.content.cloneNode(true);
		this.selector_dom = template_node.querySelector('select');

		// insert children from parent index.html into <select>
		while (this.children.length) {
			var option = this.children[0];
			this.selector_dom.appendChild(option);
		}

		let self = this;
		$(this.selector_dom).change(function(e) {
			var option = e.target.value;
			// update redis
			post_redis_key_val(REDIS_KEY_CURRENT_PRIMITIVE, option);
			self.show_module(option);
		});

		// register listeners for controller ready event
		document.addEventListener(EVENT_NOT_READY, function() {
			console.log('not ready');
		});

		document.addEventListener(EVENT_READY, function() {
			console.log('controller ready. reading from redis.')
			// fetch new redis value and update UI
			self.get_redis_val_and_update();
		});

		// fetch initial value from redis
		this.get_redis_val_and_update();

		// append to document
		this.appendChild(template_node);
	}

	show_module(option) {
		// hide all modules
		$('.modules').hide();
		// show selected module
		$(document.getElementById(option)).show();
	}

	// read from redis on page load
	get_redis_val_and_update() {
		let self = this;
		get_redis_val(REDIS_KEY_CURRENT_PRIMITIVE).done(function(option) {
			self.show_module(option);
			self.selector_dom.value = option;
		});
	}
});
})();