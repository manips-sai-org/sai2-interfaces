customElements.define('robot-discrete', class extends HTMLElement {
	constructor() {
		super();
		this.template = document.currentScript.ownerDocument.querySelector('template');
	}

	connectedCallback() {
		let template_node = this.template.content.cloneNode(true);
		
		this.key = this.getAttribute('key');
		this.value = this.getAttribute('value');
		
		// set display
		let button = template_node.querySelector('button');
		button.innerHTML = this.key + ' : ' + this.value;
		
		// set up listeners
		// register listeners for input textbox
		let self = this;
		button.onclick = function(e) {
			post_redis_key_val(self.key, self.value);
		};

		// append to document
		this.appendChild(template_node);
	}
});