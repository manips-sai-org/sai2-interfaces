customElements.define('robot-log', class extends HTMLElement {
	constructor() {
		super();
		this.logging = false;
		this.template = document.currentScript.ownerDocument.querySelector('template');
	}

	connectedCallback() {
		let template_node = this.template.content.cloneNode(true);
		
		let button = template_node.querySelector('button');
		let display = template_node.querySelector('.display');
		button.innerHTML = 'start logging';

		// set up listeners
		let self = this;
		button.onclick = function(e) {
			self.logging = !self.logging;
			if (self.logging) {
				console.log('start logging');
				self.start_logging()
					.done(function(data) {
						button.innerHTML = 'stop logging';
					});
			} else {
				console.log('stop logging');
				self.stop_logging()
					.done(function(data) {
						button.innerHTML = 'start logging';
					});
			}
		};

		// append to document
		this.appendChild(template_node);
	}

	start_logging() {
		return $.ajax({
			method: "POST",
			contentType: "application/json; charset=utf-8",
			url: "/log/start"
		}).fail(function(data) {
			alert('log error: ' + toString(data));
		});
	}

	stop_logging() {
		return $.ajax({
			method: "POST",
			contentType: "application/json; charset=utf-8",
			url: "/log/stop"
		}).fail(function(data) {
			alert('log error: ' + toString(data));
		});
	}
});