function post_redis_key_val(key, val) {
	var data = {};
	if (typeof val !== 'string') {
		val = JSON.stringify(val);
	}
	data[key] = val;
	return $.ajax({
		method: "POST",
		url: "/redis",
		data: data
	}).fail(function(data) {
		alert('set redis error: ' + toString(data));
	});
}

function get_redis_val(key) {
	return $.ajax({
		method: "GET",
		url: "/redis",
		data: {
			key: key
		}
	}).fail(function(data) {
		alert('get redis error: ' + toString(data));
	});
}
