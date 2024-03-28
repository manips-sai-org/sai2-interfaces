/**
 * Helper functions to get/set Redis keys.
 * @module ./redis
 */

/**
 * Sets a Redis key to the specified value.
 * @param {string} key The Redis key to set
 * @param {*} val The value with which to set the Redis key
 * @returns {Promise} A promise that returns no data on success.
 */
export function post_redis_key_val(key, val) {
	let fetchOptions = {
		method: 'POST',
		headers: new Headers({ 'Content-Type': 'application/json' }),
		mode: 'same-origin',
		body: JSON.stringify({ key, val })
	};

	return fetch('/redis', fetchOptions)
		.catch(error => console.error('error posting redis key: ' + key + '. Error: ' + error));
}

/**
 * Gets one or more Redis keys.
 * @param {(string|string[])} key Redis key(s) to query
 * @returns {Promise<Object>} A promise containing Redis key: value pairs.
 */
export function get_redis_val(key) {
	let fetchOptions = {
		method: 'GET',
		headers: new Headers({ 'Content-Type': 'application/json' }),
		mode: 'same-origin'
	};

	let params = new URLSearchParams({ key: JSON.stringify(key) });

	return fetch('/redis?' + params.toString(), fetchOptions)
		.then(response => response.json())
		.catch(error => console.error('error posting redis key: ' + key + '. Error: ' + error));
}

/**
 * Gets all Redis keys.
 * @returns {Promise<string[]>} A promise that on resolution has a string[] 
 * of all available Redis keys to query and set.
 */
export function get_redis_all_keys() {
	let fetchOptions = {
		method: 'GET',
		headers: new Headers({ 'Content-Type': 'application/json' }),
		mode: 'same-origin'
	};

	return fetch('/redis/keys', fetchOptions)
		.then(response => response.json())
		.catch(error => console.error('error posting redis key: ' + key + '. Error: ' + error));
}