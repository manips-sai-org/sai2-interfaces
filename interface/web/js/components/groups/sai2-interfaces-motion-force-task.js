// Define the custom web component
const template = document.createElement('template');
template.innerHTML = `
<sai2-interfaces-tabs name="motion_force_task">
	<sai2-interfaces-tab-content name="Motion control">
		<sai2-interfaces-motion-control/>
	</sai2-interfaces-tab-content>
	<sai2-interfaces-tab-content name="Force control">
		<sai2-interfaces-force-control />
	</sai2-interfaces-tab-content>
</sai2-interfaces-tabs>
`;

class Sai2InterfacesMotionForceTask extends HTMLElement {
	constructor() {
		super();
	}

	connectedCallback() {
		this.robotName = this.getAttribute('robotName');
		this.controllerName = this.getAttribute('controllerName');
		this.taskName = this.getAttribute('taskName');

		// Fetch the HTML template
		const template_node = template.content.cloneNode(true);
		let motion_control = template_node.querySelector('sai2-interfaces-motion-control');
		motion_control.setAttribute('robotName', this.robotName);
		motion_control.setAttribute('controllerName', this.controllerName);
		motion_control.setAttribute('taskName', this.taskName);

		let force_control = template_node.querySelector('sai2-interfaces-force-control');
		force_control.setAttribute('robotName', this.robotName);
		force_control.setAttribute('controllerName', this.controllerName);
		force_control.setAttribute('taskName', this.taskName);

		if (this.getAttribute('redisPrefix')) {
			this.redisPrefix = this.getAttribute('redisPrefix');
			motion_control.setAttribute('redisPrefix', this.redisPrefix);
			force_control.setAttribute('redisPrefix', this.redisPrefix);
		}

		this.replaceWith(template_node);

	}
}

// Define the custom element
customElements.define('sai2-interfaces-motion-force-task', Sai2InterfacesMotionForceTask);
