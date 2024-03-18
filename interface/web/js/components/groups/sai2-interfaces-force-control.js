// Define the custom web component
class Sai2InterfacesForceControl extends HTMLElement {
	constructor() {
		super();
		this.robotName = this.getAttribute('robotName');
		this.controllerName = this.getAttribute('controllerName');
		this.taskName = this.getAttribute('taskName');
		this.redisPrefix = "sai2::interfaces::controller::" + this.robotName + "::" + this.controllerName + "::" + this.taskName;

		// Fetch the HTML template
		fetch('html/component_groups_templates/sai2-interfaces-force-control.html')
			.then(response => response.text())
			.then(template => {
				const replacedHTML = template.replaceAll('{{_prefix_}}', this.redisPrefix);
				this.innerHTML = replacedHTML;
			});
	}
}

// Define the custom element
customElements.define('sai2-interfaces-force-control', Sai2InterfacesForceControl);
