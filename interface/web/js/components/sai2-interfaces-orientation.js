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


class Sai2InterfacesOrientation extends Sai2InterfacesComponent {
  constructor() {
    super(template);
  }

  xyz_fixed_angles_to_mat(alpha, beta, gamma) {
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

  mat_to_xyz_fixed_angles(R) {
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
      let rot_mat;

      // this.rot_mat is starting rotation matrix to base our relative offsets.
      // this is updated on initialization OR reset
      if (!this.rot_mat) {
        rot_mat = this.display.value;
        this.rot_mat = this.display.value;
      } else {
        rot_mat = this.rot_mat;
      }

      for (let i = 0; i < rot_mat.length; i++) {
        for (let j = 0; j < rot_mat[i].length; j++) {
          rot_mat[i][j] = parseFloat(rot_mat[i][j]);
        }
      }

      // compute current euler angle from rot mat
      let alpha, beta, gamma;
      [alpha, beta, gamma] = this.mat_to_xyz_fixed_angles(rot_mat);

      // add delta, noting that slider repr is (gamma, beta, alpha)
      alpha += euler_angle_delta[2];
      beta += euler_angle_delta[1];
      gamma += euler_angle_delta[0];

      // compute new rot_mat
      let new_rot_mat = this.xyz_fixed_angles_to_mat(alpha, beta, gamma);
      post_redis_key_val(this.key, new_rot_mat);
    };

    this.reset_button = document.createElement('button');
    this.reset_button.innerHTML = 'Reset Orientation Offset';
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
    this.display.setAttribute('display', 'Rotation Matrix');
    this.display.setAttribute('decimalPlaces', 3);
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