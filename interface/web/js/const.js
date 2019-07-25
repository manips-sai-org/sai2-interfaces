/**
 * Shared constants module for sai2-interfaces.
 * @module ./const
 */

/**
 * Redis key for the controller status.
 * @constant
 * @type {string}
 */
export const REDIS_KEY_CONTROLLER_STATE = 'sai2::examples::control_state';

/**
 * The value of REDIS_KEY_CONTROLLER_STATE in Redis when the controller is
 * initializing. 
 * @constant
 * @type {string}
 */
export const REDIS_VAL_CONTROLLER_INITIALIZING = 'initializing';

/**
 * The value of REDIS_KEY_CONTROLLER_STATE in Redis when the controller has
 * initialized, but is not ready to take commands. 
 * @constant
 * @type {string}
 */
export const REDIS_VAL_CONTROLLER_INITIALIZED = 'initialized';

/**
 * The value of REDIS_KEY_CONTROLLER_STATE in Redis when the controller is
 * ready to take commands.
 * @type {string}
 */
export const REDIS_VAL_CONTROLLER_READY = 'ready';

/**
 * The current controller primitive, e.g. posori or joint control.
 * @constant
 * @type {string}
 */
export const REDIS_KEY_CURRENT_PRIMITIVE = 'sai2::examples::primitive';

/**
 * The UI will issue EVENT_NOT_READY events to disable
 * all children UI elements if the controller is not ready. 
 * @constant
 * @type {string}
 */
export const EVENT_NOT_READY = 'not_ready';

/**
 * The UI will issue EVENT_READY events to enable 
 * all children UI elements if the controller is ready.
 * @constant
 * @type {string}
 */
export const EVENT_READY = 'ready';