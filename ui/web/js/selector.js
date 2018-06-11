$(document).ready(function() {
	var selector_dom = $('.primitive_selector');

	selector_dom.change(function(e) {
		var option = e.target.value;
		// update redis
		post_redis_key_val(REDIS_KEY_CURRENT_PRIMITIVE, option);
		show_module(option);
	});

	function show_module(option) {
		// hide all modules
		$('.modules').hide();
		// show selected module
		$(document.getElementById(option)).show();
	}

	// read from redis on page load
	function get_redis_val_and_update() {
		get_redis_val(REDIS_KEY_CURRENT_PRIMITIVE).done(function(option) {
			show_module(option);
			selector_dom.val(option);
		});
	}

	// register listeners for controller ready event
	document.addEventListener(EVENT_NOT_READY, function() {
		console.log('not ready');
	});

	document.addEventListener(EVENT_READY, function() {
		console.log('controller ready. reading from redis.')
		// fetch new redis value and update UI
		get_redis_val_and_update();
	});

	// fetch initial value from redis
	get_redis_val_and_update();
});
