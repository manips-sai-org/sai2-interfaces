(function() {

$(document).ready(function() {
	// update redis with initial x, y, z from UI
	var x = 0.;
	var y = 0.;
	var z = 0.;
	update_input_number_value('x', x);
	update_input_number_value('y', y);
	update_input_number_value('z', z);
	update_input_slider_value('x', x);
	update_input_slider_value('y', y);
	update_input_slider_value('z', z);

	var rotation_store = new RotationStore(x, y, z);
	
	// register listeners for input textbox
	$('#rotation .slider').change(function(e) {
		var axis = $(e.target).attr('key');
		var value = get_input_slider_value(axis);
		update_input_number_value(axis, value);
		rotation_store.update(axis, value);
	});

	// register listeners for input slider
	$('#rotation .number').change(function(e) {
		var axis = $(e.target).attr('key');
		var value = get_input_number_value(axis);
		update_input_slider_value(axis, value);
		rotation_store.update(axis, value);
	});
});

function axis_check(axis) {
	if (['x', 'y', 'z'].indexOf(axis) < 0) {
		throw "axis key not valid: " + axis;
	}
}

// ui logic
function get_input_slider_value(axis) {
	axis_check(axis);
	return parseFloat(get_input_slider_dom(axis)[0].value);
}

function get_input_number_value(axis) {
	axis_check(axis);
	return parseFloat(get_input_number_dom(axis)[0].value);
}

function update_input_slider_value(axis, value) {
	axis_check(axis);
	get_input_slider_dom(axis).val(value);
}

function update_input_number_value(axis, value) {
	axis_check(axis);
	get_input_number_dom(axis).val(value);
}

function get_input_slider_dom(axis) {
	axis_check(axis);
	return $('#rotation [key=' + axis + '].slider');
}

function get_input_number_dom(axis) {
	axis_check(axis);
	return $('#rotation .display_row').find('[key=' + axis + ']')
}

// store logic
class RotationStore {
	constructor(x, y, z) {
		this.redis_key = 'sai2::sai2Interfaces::desired_orientation';
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

})();
