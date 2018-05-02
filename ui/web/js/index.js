function post_redis_key_val(key, val) {
	var data = {};
	data[key] = JSON.stringify(val);
	$.ajax({
		method: "POST",
		url: "/",
		data: data
	});
}