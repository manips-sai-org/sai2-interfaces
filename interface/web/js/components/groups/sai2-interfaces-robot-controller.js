const valid_task_types = ['motion_force_task', 'joint_task']

class Sai2InterfacesRobotController extends HTMLElement {
	constructor() {
		super();
		this.robot_name = this.getAttribute('robotName');
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

		const redis_key_prefix = 'sai2::interfaces::controller::' + this.robot_name + '::';

		let controller_tabs = document.createElement('sai2-interfaces-tabs');
		controller_tabs.setAttribute('tabsPosition', 'left');
		controller_tabs.setAttribute('color', '#cc7a00');
		controller_tabs.setAttribute('name', this.robot_name + '_controller_selection');
		controller_tabs.setAttribute('key', redis_key_prefix + 'active_controller_name');

		for (let i = 0; i < controller_names.length; i++) {
			let controller_tab_content = document.createElement('sai2-interfaces-tab-content');
			controller_tab_content.setAttribute('name', controller_display_names[i]);
			controller_tab_content.setAttribute('value', controller_names[i]);

			if (controller_task_types[i].length == 1) {
				let task_ui_type = controller_task_types[i][0] == 'motion_force_task' ? 'sai2-interfaces-motion-force-task' : 'sai2-interfaces-joint-task';
				let task_ui_element = document.createElement(task_ui_type);
				task_ui_element.setAttribute('robotName', this.robot_name);
				task_ui_element.setAttribute('controllerName', controller_names[i]);
				task_ui_element.setAttribute('taskName', controller_task_names[i][0]);

				controller_tab_content.appendChild(task_ui_element);
			} else {
				let task_tabs = document.createElement('sai2-interfaces-tabs');
				task_tabs.setAttribute('name', controller_display_names[i] + '_task_selection');
				task_tabs.setAttribute('color', '#730099');

				for (let j = 0; j < controller_task_types[i].length; j++) {
					let task_tab_content = document.createElement('sai2-interfaces-tab-content');
					task_tab_content.setAttribute('name', task_display_names[i][j]);

					let task_ui_type = controller_task_types[i][j] == 'motion_force_task' ? 'sai2-interfaces-motion-force-task' : 'sai2-interfaces-joint-task';
					let task_ui_element = document.createElement(task_ui_type);
					task_ui_element.setAttribute('robotName', this.robot_name);
					task_ui_element.setAttribute('controllerName', controller_names[i]);
					task_ui_element.setAttribute('taskName', controller_task_names[i][j]);

					task_tab_content.appendChild(task_ui_element);
					task_tabs.appendChild(task_tab_content);
				}
				controller_tab_content.appendChild(task_tabs);
			}
			controller_tabs.appendChild(controller_tab_content);
		}

		let logging_button = document.createElement('sai2-interfaces-toggle');
		logging_button.setAttribute('key', redis_key_prefix + 'logging_on');
		logging_button.setAttribute('display', 'Logging');

		let logging_div = document.createElement('div');
		logging_div.classList.add('row');
		logging_div.classList.add('mt-3');
		logging_div.classList.add('p-2');

		logging_div.appendChild(logging_button);
		controller_tabs.appendChild(logging_div);

		this.replaceWith(controller_tabs);

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
