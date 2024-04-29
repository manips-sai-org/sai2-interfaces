const valid_task_types = ['motion_force_task', 'joint_task']

class Sai2InterfacesRobotController extends HTMLElement {
	constructor() {
		super();
		this.robot_name = this.getAttribute('robotName');
		this.redis_prefix = this.getAttribute('redisPrefix') || 'sai2::interfaces';
	}

	connectedCallback() {

		const controller_names = JSON.parse(this.getAttribute('controllerNames'));
		const controller_task_types = JSON.parse(this.getAttribute('controllerTaskTypes'));
		const controller_task_names = JSON.parse(this.getAttribute('controllerTaskNames'));

		const controller_display_names = controller_names.map(name => name.replace(/_/g, ' ').replace(/\b\w/g, c => c.toUpperCase()));
		const task_display_names = controller_task_names.map(task_names => task_names.map(name => name.replace(/_/g, ' ').replace(/\b\w/g, c => c.toUpperCase())));

		if (!this.checkAttributesValidity(controller_names, controller_task_types, controller_task_names)) {
			const errorMessage = document.createElement('div');
			errorMessage.textContent = 'Attributes of sai2-interfaces-robot-controller are not valid';
			errorMessage.style.color = 'red';
			errorMessage.style.fontWeight = 'bold';
			errorMessage.style.fontSize = 'larger';
			this.appendChild(errorMessage);
			return
		}

		const redis_key_prefix_controller_robot = this.redis_prefix + '::controller::' + this.robot_name + '::';

		let htmlString = `<sai2-interfaces-tabs tabsPosition="left" color="#cc7a00" name="${this.robot_name}_controller_selection" key="${redis_key_prefix_controller_robot}active_controller_name">`;

		for (let i = 0; i < controller_names.length; i++) {
			let controller_tab_content = `<sai2-interfaces-tab-content name="${controller_display_names[i]}" value="${controller_names[i]}">`;

			if (controller_task_types[i].length == 1) {
				let task_ui_type = controller_task_types[i][0] == 'motion_force_task' ? 'sai2-interfaces-motion-force-task' : 'sai2-interfaces-joint-task';
				let task_ui_element = `<${task_ui_type} robotName="${this.robot_name}" controllerName="${controller_names[i]}" taskName="${controller_task_names[i][0]}" redisPrefix="${this.redis_prefix}"/>`;
				controller_tab_content += task_ui_element;
			} else {
				let task_tabs = `<sai2-interfaces-tabs name="${controller_display_names[i]}_task_selection" color="#730099">`;

				for (let j = 0; j < controller_task_types[i].length; j++) {
					let task_tab_content = `<sai2-interfaces-tab-content name="${task_display_names[i][j]}">`;
					let task_ui_type = controller_task_types[i][j] == 'motion_force_task' ? 'sai2-interfaces-motion-force-task' : 'sai2-interfaces-joint-task';
					let task_ui_element = `<${task_ui_type} robotName="${this.robot_name}" controllerName="${controller_names[i]}" taskName="${controller_task_names[i][j]}" redisPrefix="${this.redis_prefix}"/>`;
					task_tab_content += task_ui_element;
					task_tab_content += `</sai2-interfaces-tab-content>`;
					task_tabs += task_tab_content;
				}
				task_tabs += `</sai2-interfaces-tabs>`;
				controller_tab_content += task_tabs;
			}
			controller_tab_content += `</sai2-interfaces-tab-content>`;
			htmlString += controller_tab_content;
		}

		htmlString += `
		<sai2-interfaces-tab-inline-content>
			<div class="row mt-3 p-2">
				<sai2-interfaces-toggle key="${redis_key_prefix_controller_robot}logging_on" display="Logging"/>
			</div>
		</sai2-interfaces-tab-inline-content>`;
		htmlString += `</sai2-interfaces-tabs>`;

		this.innerHTML = htmlString;

	}

	checkAttributesValidity(controller_names, controller_task_types, controller_task_names) {
		if (controller_names.length == 0) {
			console.error("Controller names are empty in custom html element sai2-interfaces-robot-controller");
			return false;
		}

		if (controller_names.length != controller_task_types.length || controller_names.length != controller_task_names.length) {
			console.error("Controller names, task types and task names do not have the same length in custom html element sai2-interfaces-robot-controller");
			return false;
		}

		for (let i = 0; i < controller_task_types.length; i++) {
			if (controller_task_names[i].length != controller_task_types[i].length) {
				console.error("Task names do not have the same length as task types in custom html element sai2-interfaces-robot-controller");
				return false;
			}
			for (let j = 0; j < controller_task_types[i].length; j++) {
				if (!valid_task_types.includes(controller_task_types[i][j])) {
					console.error("Invalid task type in custom html element sai2-interfaces-robot-controller");
					return false;
				}
			}
		}

		return true;
	}
}

// Define the custom element
customElements.define('sai2-interfaces-robot-controller', Sai2InterfacesRobotController);
