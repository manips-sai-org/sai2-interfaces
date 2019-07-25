/**
 * Helper module to allow multiple listeners to
 * window.onresize(). You should be able to bypass
 * this with window.onresize.addEventListener, but this needs to be tested.
 * @deprecated
 * @module ./resize
 */

 /**
  * Global list of callbacks to execute when window.onResize is fired.
  * @callback  onResizeCallback
  * @type {onResizeCallback[]}
  */
var resize_callbacks = [];

/**
 * Next ID to assign when a callback is added.
 * @type {number}
 */
var handle_id = 1;

/**
 * Global variable to signal if window.onresize has been initialized.
 * @type {boolean}
 */
var init = false;

/**
 * Adds a callback to the list of callbacks who need to be called
 * when the window resizes.
 * 
 * @param {function(void):void} callback Function to be called when window resizes
 * @returns {int} A handle which identifies your callback. 
 *  You need this handle if you need to unregister your callback.
 */
export function registerWindowResizeCallback(callback) {
  if (!init) {
    window.onresize = () => {
      for (let callback_obj of resize_callbacks) 
        callback_obj.callback();
    };

    init = true;
  }

  resize_callbacks.push({handle: handle_id, callback});
  let ret_handle_id = handle_id;
  handle_id++;
  return ret_handle_id;
}

/**
 * Removes a resize window callback.
 * @param {int} callback_handle The handle (int) given by registerWindowResizeCallback
 * @returns {boolean} True on successful removal, false if not found or not initialized
 */
export function removeWindowResizeCallback(callback_handle) {
  if (!init) 
    return false;

  for (let i = 0; i < resize_callbacks.length; i++) {
    if (resize_callbacks.handle == callback_handle) {
      resize_callbacks.splice(i, 1);
      return true;
    }
  }
  return false;
}
