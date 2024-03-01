import { post_redis_key_val } from '../redis.js';
import Sai2InterfacesComponent from './sai2-interfaces-component.js';

const template = document.createElement('template');
template.innerHTML = `
<style>
	.sai2-interfaces-orientation-top {
		display: flex;
		flex-direction: row;
		flex-wrap: wrap;
		justify-content: space-evenly;
		padding: 0.5em;
	}

	.sai2-interfaces-orientation-left {
		display: flex;
		flex-direction: column;
		flex: 1;
		align-items: center;
	}

	.sai2-interfaces-orientation-left sai2-interfaces-slider {
		width: 100%;
	}

	.sai2-interfaces-orientation-left button {
		width: max-content;
	}

	.sai2-interfaces-orientation-right {
		margin: 0.5em;
		flex: 1;
	}
</style>
<div class="sai2-interfaces-orientation-top">
	<div class="sai2-interfaces-orientation-left">
	</div>
	<div class="sai2-interfaces-orientation-right">
	</div>
</div>
`;

/** C = A * B */
function block_mat_mat_mult(A, B) {
	let a_rows = A.length;
	let a_cols = A[0].length;
	let b_rows = B.length;
	let b_cols = B[0].length;

	if (a_cols != b_rows) {
		throw Error("A cols != B rows");
	}

	let C = [];
	for (let i = 0; i < a_rows; i++) {
		let C_row = [];
		for (let j = 0; j < b_cols; j++) {
			let C_ij = 0;
			for (let k = 0; k < a_cols; k++) {
				C_ij += A[i][k] * B[k][j];
			}
			C_row.push(C_ij);
		}
		C.push(C_row);
	}

	return C;
}

function xyz_fixed_angles_to_mat(alpha, beta, gamma) {
	// eq 1.41 in CS223A course reader
	let c_a = Math.cos(alpha); let s_a = Math.sin(alpha);
	let c_b = Math.cos(beta); let s_b = Math.sin(beta);
	let c_g = Math.cos(gamma); let s_g = Math.sin(gamma);
	return [
		[c_a * c_b, (c_a * s_b * s_g) - (s_a * c_g), (c_a * s_b * c_g) + (s_a * s_g)],
		[s_a * c_b, (s_a * s_b * s_g) + (c_a * c_g), (s_a * s_b * c_g) - (c_a * s_g)],
		[-s_b, c_b * s_g, c_b * c_g]
	];
}

function mat_to_xyz_fixed_angles(R) {
	let alpha, beta, gamma;

	let c_beta = Math.sqrt(R[0][0] ** 2 + R[1][0] ** 2);
	let s_beta = -R[2][0];
	if (Math.abs(c_beta ** 2) < 1e-10) {
		/* Singularity. Assuming alpha = 0, we get a matrix of the form:
			0   sin(beta) * sin(gamma)    sin(beta) * cos(gamma)
			0          cos(gamma)                -sin(gamma)
		-sin(beta)         0                         0
		We can find gamma by arctan2(-r_23, r_22).
		We can then find beta by taking the arcsin. */
		alpha = 0;
		gamma = Math.atan2(-R[1][2], R[1][1]);
		beta = Math.asin(-R[2][0]);
	} else {
		alpha = Math.atan2(R[1][0] / c_beta, R[0][0] / c_beta);
		beta = Math.atan2(s_beta, c_beta);
		gamma = Math.atan2(R[2][1] / c_beta, R[2][2] / c_beta);
	}
	return [alpha, beta, gamma];
}


class Sai2InterfacesOrientation extends Sai2InterfacesComponent {
	constructor() {
		super(template);
	}

	onMount() {
		this.key = this.getAttribute('key');
		this.refreshRate = this.getAttribute('refreshRate');

		let left_div = this.template_node.querySelector('.sai2-interfaces-orientation-left');

		this.slider = document.createElement('sai2-interfaces-slider');
		this.slider.setAttribute('size', 3);
		this.slider.setAttribute('display', '["X (γ)", "Y (β)", "Z (α)"]');
		this.slider.setAttribute('min', -3.14);
		this.slider.setAttribute('max', 3.14);
		this.slider.setAttribute('step', 0.01);
		this.slider.onvaluechange = euler_angle_delta => {
			/*
			 * We have a rotation matrix which usually means end-effector wrt to world.
			 * We will call this R_w_ee.
			 * 
			 * Our sliders specify some intermediate frame wrt world. We will use
			 * this intermediate frame as the basis for R_ee_world. 
			 * We will call this R_w_wprime.
			 * 
			 * Effectively, based on this wording, our original R_w_ee matrix should
			 * be called R_wprime_ee since it is based off frame {wprime}.
			 * 
			 * Since SAI needs the rotation matrix wrt world, we compute 
			 * R_w_ee = R_w_wprime (from sliders) * R_wprime_ee (given)
			 */

			let R_wprime_ee;

			// this.rot_mat is starting rotation matrix to base our relative offsets.
			// this is updated on initialization OR reset
			if (!this.rot_mat) {
				R_wprime_ee = this.display.value;
				this.rot_mat = this.display.value;
			} else {
				R_wprime_ee = this.rot_mat;
			}

			for (let i = 0; i < R_wprime_ee.length; i++) {
				for (let j = 0; j < R_wprime_ee[i].length; j++) {
					R_wprime_ee[i][j] = parseFloat(R_wprime_ee[i][j]);
				}
			}

			let alpha, beta, gamma;
			[gamma, beta, alpha] = euler_angle_delta;
			let R_w_wprime = xyz_fixed_angles_to_mat(alpha, beta, gamma);
			let R_w_ee = block_mat_mat_mult(R_w_wprime, R_wprime_ee);
			post_redis_key_val(this.key, R_w_ee);
		};

		this.reset_button = document.createElement('button');
		this.reset_button.innerHTML = 'Center Sliders';
		this.reset_button.onclick = () => {
			// set slider to zero
			this.slider.refresh();

			// update base rotation matrix
			this.rot_mat = this.display.value;
		};

		left_div.append(this.slider);
		left_div.append(this.reset_button);

		let right_div = this.template_node.querySelector('.sai2-interfaces-orientation-right');

		this.display = document.createElement('sai2-interfaces-display');
		this.display.setAttribute('key', this.key);
		this.display.setAttribute('display', 'Matrix Form');
		this.display.setAttribute('decimalPlaces', 3);
		this.display.setAttribute('labelPosition', 'top');
		this.display.setAttribute('refreshRate', this.refreshRate);

		right_div.append(this.display);
	}

	onUnmount() {
	}

	enableComponent() {
	}

	disableComponent() {
	}
}

customElements.define('sai2-interfaces-orientation', Sai2InterfacesOrientation);