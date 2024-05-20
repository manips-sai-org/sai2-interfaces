class Sai2InterfacesSimvizObject extends HTMLElement {
	constructor() {
		super();
		this.objectName = this.getAttribute('objectName');
		this.redisPrefix =  this.getAttribute('RedisPrefix') || "sai2::interfaces";

		// Fetch the HTML template
		fetch('html/component_groups_templates/sai2-interfaces-simviz-object.html')
			.then(response => response.text())
			.then(template => {
				let replacedHTML = template.replaceAll('{{_prefix_}}', this.redisPrefix);
				replacedHTML = replacedHTML.replaceAll('{{_object_name_}}', this.objectName);
				this.innerHTML = replacedHTML;
			});
	}
}

// Define the custom element
customElements.define('sai2-interfaces-simviz-object', Sai2InterfacesSimvizObject);
