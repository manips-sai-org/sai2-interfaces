// poll controller ready state every 0.5 seconds
setInterval(function() {
	get_redis_val(REDIS_KEY_CONTROLLER_STATE).done(function(val) {
		if (val === REDIS_VAL_CONTROLLER_INITIALIZING) {
			// not ready, emit events to children to disable UI
			var event = new Event(EVENT_NOT_READY);
			document.dispatchEvent(event);
		} else if (val === REDIS_VAL_CONTROLLER_INITIALIZED) {
			// ready, emit events to children to refetch from redis
			var event = new Event(EVENT_READY);
			document.dispatchEvent(event);
			// clear this key value, so that we only fire this event once
			post_redis_key_val(REDIS_KEY_CONTROLLER_STATE, REDIS_VAL_CONTROLLER_READY);
		}
	});
}, 250);