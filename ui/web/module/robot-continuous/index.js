customElements.define('robot-continuous', class extends HTMLElement {
	constructor() {
		super();
		this.template = document.currentScript.ownerDocument.querySelector('template');
	}

	connectedCallback() {
		let template_node = this.template.content.cloneNode(true);

		this.key = this.getAttribute('key');
		this.min = this.getAttribute('min');
		this.max = this.getAttribute('max');
		this.step = this.getAttribute('step');
		
		// set display
		let display = template_node.querySelector('.display');
		display.innerHTML = this.key;
		
		// set number input min, max, step
		let input_number = template_node.querySelector('input[type=number]');
		input_number.min = this.min;
		input_number.max = this.max;
		input_number.step = this.step;
		this.input_number = input_number;
		
		// set range input min, max, step
		let input_range = template_node.querySelector('input[type=range]');
		input_range.min = this.min;
		input_range.max = this.max;
		input_range.step = this.step;
		this.input_range = input_range;

		// set up listeners
		// register listeners for input textbox
		let self = this;
		input_range.oninput = function(e) {
			var value = self.get_input_slider_value();
			self.update_input_number_value(value);
			post_redis_key_val(self.key, value);
		};

		// register listeners for input slider
		input_number.oninput = function(e) {
			var value = self.get_input_number_value();
			self.update_input_slider_value(value);
			post_redis_key_val(self.key, value);
		};

		// fetch initial value from redis
		get_redis_val(self.key)
			.done(function(value) {
				console.log('setting initial value from redis. key: ' + self.key, ' value: ' + value);
				self.update_input_slider_value(value);
				self.update_input_number_value(value);
			});

		// append to document
		this.appendChild(template_node);
	}

	get_input_slider_value() {
		return parseFloat(this.input_range.value);
	}

	get_input_number_value() {
		return parseFloat(this.input_number.value);
	}

	update_input_slider_value(value) {
		$(this.input_range).val(value);
	}

	update_input_number_value(value) {
		$(this.input_number).val(value);
	}

});