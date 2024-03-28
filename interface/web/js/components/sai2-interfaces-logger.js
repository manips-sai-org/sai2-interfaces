/**
 * Defines a custom HTML element to issue HTTP requests to start/stop
 * a background logger.
 * <br>
 * Example usage:
 * &lt;sai2-interfaces-logger /&gt;
 * <br>
 * Note: there are no available attributes to set.
 * 
 * @module ./module/sai2-interfaces-logger
 */


import { get_redis_all_keys } from '../redis.js';
import Sai2InterfacesComponent from './sai2-interfaces-component.js';


const template = document.createElement('template');
template.innerHTML = `
<style>
	.sai2-interface-logger-top {
		display: flex;
		flex-direction: row;
		align-items: baseline;
		justify-content: space-evenly;
		flex-wrap: wrap;
	}

	.sai2-interface-logger-top>input {
		width: 32%;
	}

	.sai2-interface-logger-top>div {
		width: 32%;
	}
</style>
<div class="sai2-interface-logger-top">
	<input type="text" class="logfile" placeholder="log filename">
	<input type="number" step="0.01" min="0" placeholder="0.01" class="logperiod" placeholder="(period, in seconds)">
	<div>
		<select multiple class="chosen_select" data-placeholder="Select keys to log..."></select>
	</div>
	<button class="log_start_stop"></button><br>
	<input type=file class="logger_offline_plot">
	<button class="log_offline_btn">Offline Plot</button>
</div>
`;

class Sai2InterfacesLogger extends Sai2InterfacesComponent {
	constructor() {
		super(template);
	}

	onMount() {
		let button = this.template_node.querySelector('.log_start_stop');
		let logfile_input = this.template_node.querySelector('.logfile');
		let logperiod_input = this.template_node.querySelector('.logperiod');
		let keys_select = this.template_node.querySelector('select');
		let log_offline_input = this.template_node.querySelector('.logger_offline_plot');
		let log_offline_button = this.template_node.querySelector('.log_offline_btn');

		this.getLoggerStatus().then(status => {
			this.logging = status['running'];
			button.innerHTML = this.logging ? 'stop logging' : 'start logging';
			button.className = this.logging ? "button-disable" : "button-enable";
		});

		button.innerHTML = this.logging ? 'stop logging' : 'start logging';
		button.className = this.logging ? "button-disable" : "button-enable";

		// populate keys list
		get_redis_all_keys().then(keys => {
			for (let key of keys.values()) {
				let opt = document.createElement('option');
				opt.value = key;
				opt.innerHTML = key;
				keys_select.append(opt);
			}

			$('.chosen_select').chosen({ width: '100%', search_contains: true });
		});

		// set up listeners
		button.onclick = () => {
			this.logging = !this.logging;

			// default to log.txt
			let filename = logfile_input.value || 'log.txt';

			if (this.logging) {
				// get selected keys
				let selected_keys = [];
				for (let option of keys_select.options)
					if (option.selected)
						selected_keys.push(option.value);

				// get logger period. default to 0.01s
				let logger_period = parseFloat(logperiod_input.value)
				if (isNaN(logger_period) || logger_period < 0) {
					logger_period = 0.01;
					logperiod_input.value = 0.01;
				}

				this.start_logging(filename, selected_keys, logger_period).then(() => {
					button.innerHTML = 'stop logging';
					button.className = "button-disable";
				});
			} else {
				// prompt user for log file download
				let link = document.createElement('a');
				link.download = filename;
				link.href = filename;
				document.body.appendChild(link);
				link.click();
				link.remove();

				this.stop_logging().then(() => {
					button.innerHTML = 'start logging';
					button.className = "button-enable"
				});
			}
		};

		// offline plotting initialization
		log_offline_button.onclick = () => {
			let data = new FormData();
			data.append('file', log_offline_input.files[0]);
			fetch('/logger/offline', { method: 'POST', body: data });
		};
	}

	onUnmount() {
	}

	enableComponent() {
	}

	disableComponent() {
	}

	/**
   * Sends a HTTP request to the server to query logger status.
   * @returns {Promise<Object>} Promise with JSON object containig boolean key 'running'. 
   */
	getLoggerStatus() {
		let fetchOptions = {
			method: 'GET',
			headers: new Headers({ 'Content-Type': 'application/json' }),
			mode: 'same-origin'
		};

		return fetch('/logger/status', fetchOptions)
			.then(response => response.json())
			.catch(data => alert('logger error: ' + toString(data)));
	}

	/**
	 * Sends a start logging HTTP request to the backend.
	 * 
	 * @param {string} filename Log file to write to. Need not exist.
	 * @param {string[]} key_list List of Redis keys to query
	 * @param {number} period How often to log, in seconds
	 * @returns {Promise} Empty promise. Use this to execute actions after successful request.
	 */
	start_logging(filename, key_list, period) {
		let fetchOptions = {
			method: 'POST',
			headers: new Headers({ 'Content-Type': 'application/json' }),
			mode: 'same-origin',
			body: JSON.stringify({
				filename: filename,
				keys: key_list,
				logger_period: period
			})
		};

		return fetch('/logger/start', fetchOptions)
			.then(response => response.ok)
			.catch(data => alert('logger redis error: ' + toString(data)));
	}

	/**
	 * Sends a stop logging HTTP request to the backend.
	 * @returns {Promise} Empty promise. Use this to execute actions after successful request.
	 */
	stop_logging() {
		let fetchOptions = {
			method: 'POST',
			headers: new Headers({ 'Content-Type': 'application/json' }),
			mode: 'same-origin'
		};

		return fetch('/logger/stop', fetchOptions)
			.catch(data => alert('logger redis error: ' + toString(data)));
	}
}


customElements.define('sai2-interfaces-logger', Sai2InterfacesLogger);