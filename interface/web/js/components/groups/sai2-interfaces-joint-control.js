class Sai2InterfacesJointControl extends HTMLElement {
	constructor() {
		super();
		this.robotName = this.getAttribute('robotName');
		this.controllerName = this.getAttribute('controllerName');
		this.taskName = this.getAttribute('taskName');
		this.redisPrefix = "sai2::interfaces::controller::" + this.robotName + "::" + this.controllerName + "::" + this.taskName;

		// Fetch the HTML template
		fetch('component_groups_templates/sai2-interfaces-joint-control.html')
			.then(response => response.text())
			.then(template => {
				const replacedHTML = template.replaceAll('{{_prefix_}}', this.redisPrefix);
				this.innerHTML = replacedHTML;
			});
	}
}

// Define the custom element
customElements.define('sai2-interfaces-joint-control', Sai2InterfacesJointControl);
