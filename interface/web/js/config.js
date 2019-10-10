/**
 * Shared constants module for sai2-interfaces.
 * @module ./config
 */

export const EVENT_CONTROLLER_STATUS = 'controller-status';

/**
 * Redis key for the controller status.
 * @constant
 * @type {string}
 */
export const REDIS_KEY_CONTROLLER_STATE = 'sai2::examples::control_state';

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

export const REDIS_PRIMITIVE_UPDATING = 'sai2::examples::primitive_updating';