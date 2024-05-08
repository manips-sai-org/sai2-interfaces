// Define the custom web component
class Sai2InterfacesMotionControl extends HTMLElement {
	constructor() {
		super();
		this.robotName = this.getAttribute('robotName');
		this.controllerName = this.getAttribute('controllerName');
		this.taskName = this.getAttribute('taskName');
		this.redisPrefix = "sai2::interfaces::controller::";
		this.min_goal_position = this.getAttribute('minGoalPosition');
		this.max_goal_position = this.getAttribute('maxGoalPosition');
		if (this.getAttribute('redisPrefix')) {
			this.redisPrefix = this.getAttribute('redisPrefix') + "::controller::";
		}
		this.redisPrefix += this.robotName + "::" + this.controllerName + "::" + this.taskName;

		// Fetch the HTML template
		fetch('html/component_groups_templates/sai2-interfaces-motion-control.html')
			.then(response => response.text())
			.then(template => {
				let replacedHTML = template.replaceAll('{{_prefix_}}', this.redisPrefix);
				
				if (this.min_goal_position) {
					replacedHTML = replacedHTML.replaceAll('{{_min_goal_position_}}', this.min_goal_position);
				} else {
					replacedHTML = replacedHTML.replaceAll('{{_min_goal_position_}}', "[-0.5,-0.5,0.0]");
				}
				if (this.max_goal_position) {
					replacedHTML = replacedHTML.replaceAll('{{_max_goal_position_}}', this.max_goal_position);
				} else {
					replacedHTML = replacedHTML.replaceAll('{{_max_goal_position_}}', "[0.5,0.5,0.8]");
				}
				
				this.innerHTML = replacedHTML;
			});
	}
}

// Define the custom element
customElements.define('sai2-interfaces-motion-control', Sai2InterfacesMotionControl);
