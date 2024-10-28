class Sai2InterfacesSimvizRobot extends HTMLElement {
	constructor() {
		super();
		this.robotName = this.getAttribute('robotName');
		this.redisPrefix =  this.getAttribute('RedisPrefix') + "::" || "";

		// Fetch the HTML template
		fetch('html/component_groups_templates/sai2-interfaces-simviz-robot.html')
			.then(response => response.text())
			.then(template => {
				let replacedHTML = template.replaceAll('{{_prefix_}}', this.redisPrefix);
				replacedHTML = replacedHTML.replaceAll('{{_robot_name_}}', this.robotName);
				this.innerHTML = replacedHTML;
			});
	}
}

// Define the custom element
customElements.define('sai2-interfaces-simviz-robot', Sai2InterfacesSimvizRobot);
