import { get_redis_val, post_redis_key_val } from './redis.js';
import {
  REDIS_KEY_CONTROLLER_STATE, 
  REDIS_VAL_CONTROLLER_INITIALIZING,
  REDIS_VAL_CONTROLLER_INITIALIZED,
  REDIS_VAL_CONTROLLER_READY,
  EVENT_NOT_READY,
  EVENT_READY
} from './const.js';
/*
// poll controller ready state every 0.5 seconds
setInterval(() => {
	get_redis_val(REDIS_KEY_CONTROLLER_STATE).then((val) => {
		if (val === REDIS_VAL_CONTROLLER_INITIALIZING) {
			// not ready, emit events to children to disable UI
			let event = new Event(EVENT_NOT_READY);
			document.dispatchEvent(event);
		} else if (val === REDIS_VAL_CONTROLLER_INITIALIZED) {
			// ready, emit events to children to refetch from redis
			let event = new Event(EVENT_READY);
      document.dispatchEvent(event);
      
			// clear this key value, so that we only fire this event once
			post_redis_key_val(REDIS_KEY_CONTROLLER_STATE, REDIS_VAL_CONTROLLER_READY);
		}
	});
}, 1000);*/