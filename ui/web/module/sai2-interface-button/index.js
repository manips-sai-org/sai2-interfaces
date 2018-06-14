// need self executing function, 
// because otherwise template will be declared multiple times
(function() {
// declare this template outside of constructor
// because on firefox, document.currentScript does not work 
// need to query by id, instead of 'template'
// because on firefox, querying by template results in wrong template
var template = document.currentScript.ownerDocument.querySelector('#template-button');

customElements.define('sai2-interface-button', class extends HTMLElement {
	constructor() {
		super();
		this.template = template;
	}

	connectedCallback() {
		let template_node = this.template.content.cloneNode(true);
		
		this.key = this.getAttribute('key');
		this.display = this.getAttribute('display');
		this.value = this.getAttribute('value');
		
		// set display
		let button = template_node.querySelector('button');
		let default_display = this.key + ' : ' + this.value;
		button.innerHTML = this.display || default_display;
		
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
})();