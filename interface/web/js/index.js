import {
	EVENT_CONTROLLER_STATUS,
	REDIS_VAL_CONTROLLER_READY
} from './config.js';

// Import all ES6 modules here, so the HTML template only needs to load index.js.
import './redis.js';
import './components/sai2-interfaces-component.js';
import './components/sai2-interfaces-logger.js';
import './components/sai2-interfaces-slider.js';
import './components/sai2-interfaces-select-option.js';
import './components/sai2-interfaces-select.js';
import './components/sai2-interfaces-setkey.js';
import './components/sai2-interfaces-config-selector.js';
import './components/sai2-interfaces-toggle.js';
import './components/sai2-interfaces-plot.js';
import './components/sai2-interfaces-enum.js';
import './components/sai2-interfaces-tabs.js';
import './components/sai2-interfaces-display.js';
import './components/sai2-interfaces-trajectory-select.js';
import './components/sai2-interfaces-accordion.js';
import './components/sai2-interfaces-toggle-group.js';
import './components/sai2-interfaces-orientation.js';
import './components/sai2-interfaces-axis-setter.js';
import './components/groups/sai2-interfaces-motion-control.js';
import './components/groups/sai2-interfaces-force-control.js';
import './components/groups/sai2-interfaces-joint-control.js';

var socket = io();
socket.on('connect', () => {
	// TODO: 
});

socket.on('controller-state', data => {
	let controller_status = data === REDIS_VAL_CONTROLLER_READY;
	document.dispatchEvent(new CustomEvent(EVENT_CONTROLLER_STATUS, { ready: controller_status }));
})

socket.on('disconnect', () => {
	// TODO: disable components
	document.dispatchEvent(new CustomEvent(EVENT_CONTROLLER_STATUS, { ready: false }));

	// attempt to reconnect
	socket.open();
})