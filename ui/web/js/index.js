$( document ).ready(function() {
	// update redis with initial x, y, z from UI
	var x = get_input_value('x');
	var y = get_input_value('y');
	var z = get_input_value('z');
	update_display_value('x', x);
	update_display_value('y', y);
	update_display_value('z', z);

	var position_store = new PositionStore(x, y, z);
	$('#position .slider').change(function(e) {
		var axis = $(e.target).attr('key');
		var value = get_input_value(axis);
		position_store.update(axis, value);
		update_display_value(axis, value);
	});
});

function axis_check(axis) {
	if (['x', 'y', 'z'].indexOf(axis) < 0) {
		throw "axis key not valid: " + axis;
	}
}

// ui logic
function get_input_value(axis) {
	axis_check(axis);
	return parseFloat(get_input_dom(axis)[0].value);
}

function update_display_value(axis, value) {
	axis_check(axis);
	get_display_dom(axis).text(value);
}

function get_input_dom(axis) {
	axis_check(axis);
	return $('#position [key=' + axis + '].slider');
}

function get_display_dom(axis) {
	axis_check(axis);
	return $('#position .display_row').find('[key=' + axis + ']')
}

// store logic

class PositionStore {
	constructor(x, y, z) {
		this.redis_key = 'sai2::sai2Interfaces::desired_position';
		this.x = x;
		this.y = y;
		this.z = z;
	}

	update(axis, value) {
		axis_check(axis);
		this[axis] = parseFloat(value);
		var redis_value = [this.x, this.y, this.z];
		post_redis_key_val(this.redis_key, redis_value);
	}
}

function post_redis_key_val(key, val) {
	var data = {};
	data[key] = JSON.stringify(val);
	$.ajax({
		method: "POST",
		url: "/",
		data: data
	});
}