/**
 * Defines a custom HTML element to model and execute Catmull-Rom
 * trajectory generation.
 * <pre>
 * Element Tag: &lt;sai2-interface-trajectory-select&gt;
 * HTML attributes:
 *    xLim: number[] - array of [x min, x max]
 *    yLim: number[] - array of [y min, y max]
 *    zLim: number[] - array of [z min, z max]
 *    current_pos_key: string - Redis key containing the current EE pos
 * </pre>
 * 
 * @module ./module/sai2-interface-trajectory-slider
 */

import { get_redis_val } from '../redis.js';

const template = document.createElement('template');
template.innerHTML = `
<style>
	.sai2-interface-trajectory-select-top {
		display: flex;
		flex-direction: column;
		height: 95%;
	}

	.sai2-interface-trajectory-select-top .metadata {
		flex: 1;
		display: grid;
		grid-template-columns: 1fr;
		grid-template-rows: 1fr 1fr auto auto;
		grid-template-areas:
			"trajectory-params"
			"trajectory-buttons"
			"point-remover"
			"trajectory-info";
	}

	.sai2-interface-trajectory-select-top .metadata>div {
		display: flex;
		flex-direction: row;
		flex-wrap: wrap;
		justify-content: space-around;
	}

	.sai2-interface-trajectory-select-top .metadata .trajectory-params {
		grid-area: trajectory-params;
		display: grid;
		grid-template-rows: 1fr auto;
		grid-template-columns: 1fr;
	}

	.sai2-interface-trajectory-select-top .metadata .trajectory-params>div {
		display: flex;
		justify-content: space-around;
	}

	.sai2-interface-trajectory-select-top .error-message {
		color: red;
	}

	.sai2-interface-trajectory-select-top .metadata .trajectory-buttons {
		grid-area: trajectory-buttons;
	}

	.sai2-interface-trajectory-select-top .metadata .point-remover {
		grid-area: point-remover;
	}

	.sai2-interface-trajectory-select-top .metadata .trajectory-info {
		grid-area: trajectory-info;
	}

	.sai2-interface-trajectory-select-top .plots {
		flex: 9;
	}

	.sai2-interface-trajectory-select-top-grid-half {
		flex: auto;
		display: flex;
		justify-content: space-between;
		width: 100%;
	}

	.sai2-interface-trajectory-select-top-col {
		width: 48%;
	}
</style>
<div class="sai2-interface-trajectory-select-top">
	<div class="metadata">
		<div class="trajectory-params">
			<div>
				<label>Trajectory Duration</label>
				<input class="traj-max-time" type="number" min="0.1" step="0.1">
				<label>Trajectory Step Size</label>
				<input class="traj-step-time" type="number" min="0.01" step="0.01">
			</div>
			<div>
				<label class="error-message"></label>
			</div>
		</div>
		<div class="trajectory-buttons">
			<button class="trajectory-add-pt-btn">Add Point</button>
			<button class="trajectory-get-btn">Get Trajectory</button>
			<button class="trajectory-clear-all-btn">Clear All Points</button>
			<button class="trajectory-clear-btn">Clear Trajectory</button>
			<button class="trajectory-run-btn">Run Trajectory</button>
		</div>
		<select class="point-remover chosen_select" multiple data-placeholder="Add a point..."></select>
		<div class="trajectory-info">
			<div>
				<h4>Max Velocity: </h4>
				<label>Norm: </label>
				<label class="max-vel-norm"></label>
				<label>X: </label>
				<label class="max-vel-x"></label>
				<label>Y: </label>
				<label class="max-vel-y"></label>
				<label>Z: </label>
				<label class="max-vel-z"></label>
			</div>
			<div>
				<h4>Max Acceleration:</h4>
				<label>Norm: </label>
				<label class="max-accel-norm"></label>
				<label>X: </label>
				<label class="max-accel-x"></label>
				<label>Y: </label>
				<label class="max-accel-y"></label>
				<label>Z: </label>
				<label class="max-accel-z"></label>
			</div>
		</div>
	</div>
	<div class="sai2-interface-trajectory-select-top-grid-half">
		<div class="sai2-interface-trajectory-select-top-col traj-xy"></div>
		<div class="sai2-interface-trajectory-select-top-col traj-xz"></div>
	</div>
</div>
`;

customElements.define('sai2-interfaces-trajectory-select', class extends HTMLElement {
	constructor() {
		super();
		this.template = template;

		this.xy_plot = null;
		this.xy_config = null;

		this.xz_plot = null;
		this.xz_config = null;

		this.points = { x: [], y: [], z: [], idx: [] };
		this.trajectory = { x: [], y: [], z: [], t: [], v: [], a: [] };
		this.ee_pos = { x: [], y: [], z: [] };
		this.next_point_index = 1; // point 0 is reserved for EE
	}

	connectedCallback() {
		// get DOM elements
		let template_node = this.template.content.cloneNode(true);
		let xy_div = template_node.querySelector('.traj-xy');
		let xz_div = template_node.querySelector('.traj-xz');

		// top level UI items
		let addPointButton = template_node.querySelector('.trajectory-add-pt-btn');
		let getTrajectoryButton = template_node.querySelector('.trajectory-get-btn');
		let clearAllPointsTrajectoryButton = template_node.querySelector('.trajectory-clear-all-btn');
		let clearTrajectoryButton = template_node.querySelector('.trajectory-clear-btn');
		let runTrajectoryButton = template_node.querySelector('.trajectory-run-btn');
		let pointSelect = template_node.querySelector('.point-remover');
		let trajectoryMaxTimeInput = template_node.querySelector('.traj-max-time');
		let trajectoryStepSizeInput = template_node.querySelector('.traj-step-time');
		let errorMessageLabel = template_node.querySelector('.error-message');

		// max vel & accel UI items
		let maxVelNorm = template_node.querySelector('.max-vel-norm');
		let maxVelX = template_node.querySelector('.max-vel-x');
		let maxVelY = template_node.querySelector('.max-vel-y');
		let maxVelZ = template_node.querySelector('.max-vel-z');
		let maxAccelNorm = template_node.querySelector('.max-accel-norm');
		let maxAccelX = template_node.querySelector('.max-accel-x');
		let maxAccelY = template_node.querySelector('.max-accel-y');
		let maxAccelZ = template_node.querySelector('.max-accel-z');

		// grab passed-in attributes
		let xLim = JSON.parse(this.getAttribute('xLim'));
		let yLim = JSON.parse(this.getAttribute('yLim'));
		let zLim = JSON.parse(this.getAttribute('zLim'));
		let current_ee_pos_key = this.getAttribute('current_pos_key');

		let primitive_key = this.getAttribute("primitive_key");
		let primitive_value = this.getAttribute("primitive_value")
		let position_key = this.getAttribute("position_key");
		let velocity_key = this.getAttribute("velocity_key");

		// initialize template
		this.default_config = {
			grid: {},
			xAxis: { type: 'value', name: 'x', min: xLim[0], max: xLim[1] },
			legend: { type: 'scroll' },
			toolbox: {
				top: 'bottom',
				left: 'right',
				feature: {
					saveAsImage: { title: 'Save Plot' },
					dataZoom: { title: { zoom: 'Box Zoom', back: 'Reset View' } }
				}
			},
			dataset: [
				{ source: this.points },
				{ source: this.trajectory },
				{ source: this.ee_pos }
			]
		};

		// initialize select
		$('.point-remover').chosen({ width: '100%' });

		// initialize empty plot & templates
		this.xy_plot = echarts.init(xy_div);
		this.xz_plot = echarts.init(xz_div);

		this.xy_config = { ...this.default_config };
		this.xz_config = { ...this.default_config };

		// initialize tooltips
		// series 0, 2 are the control points - 2 is just the EE
		// series 1 is the computed trajectory
		this.xy_config.tooltip = {
			triggerOn: 'none',
			formatter: params => {
				if (params.seriesIndex === 0 || params.seriesIndex === 2)
					return `Point ${this.points.idx[params.dataIndex]}
            <br>X: ${this.points.x[params.dataIndex]}
            <br>Y: ${this.points.y[params.dataIndex]}`;
				else
					return `Time ${this.trajectory.t[params.dataIndex].toFixed(4)}
            <br>X: ${this.trajectory.x[params.dataIndex].toFixed(3)}
            <br>Y: ${this.trajectory.y[params.dataIndex].toFixed(3)}
            <br>V: ${this.trajectory.v[params.dataIndex].toFixed(3)}
            <br>A: ${this.trajectory.a[params.dataIndex].toFixed(3)}`;
			}
		};

		this.xz_config.tooltip = {
			triggerOn: 'none',
			formatter: params => {
				if (params.seriesIndex === 0 || params.seriesIndex === 2)
					return `Point ${this.points.idx[params.dataIndex]}
            <br>X: ${this.points.x[params.dataIndex].toFixed(3)} 
            <br>Z: ${this.points.z[params.dataIndex].toFixed(3)}`;
				else
					return `Time ${this.trajectory.t[params.dataIndex].toFixed(4)}
            <br>X: ${this.trajectory.x[params.dataIndex].toFixed(3)}
            <br>Z: ${this.trajectory.z[params.dataIndex].toFixed(3)}
            <br>V: ${this.trajectory.v[params.dataIndex].toFixed(3)}
            <br>A: ${this.trajectory.a[params.dataIndex].toFixed(3)}`;
			}
		};

		// series definition
		// control point shapes are defined by initialize_graphics
		// since we need more control than the default
		this.xy_config.series = [{
			id: 'xy',
			type: 'line',
			datasetIndex: 0,
			lineStyle: { type: 'dashed' },
			encode: { x: 'x', y: 'y' },
			symbolSize: false
		}, {
			id: 'xy-traj',
			type: 'line',
			datasetIndex: 1,
			encode: { x: 'x', y: 'y' },
			symbolSize: false,
			lineStyle: { color: 'blue' }
		}, {
			id: 'ee-pos-xy-traj',
			type: 'line',
			datasetIndex: 2,
			symbolSize: 24,
			encode: { x: 'x', y: 'y' },
			symbol: 'pin',
			itemStyle: { color: '#000' }
		}
		];

		this.xz_config.series = [{
			id: 'xz',
			type: 'line',
			datasetIndex: 0,
			lineStyle: { type: 'dashed' },
			encode: { x: 'x', y: 'z' },
			symbolSize: false,
		}, {
			id: 'xz-traj',
			type: 'line',
			datasetIndex: 1,
			encode: { x: 'x', y: 'z' },
			symbolSize: false,
			lineStyle: { color: 'blue' }
		}, {
			id: 'ee-pos-xy-traj',
			type: 'line',
			datasetIndex: 2,
			symbolSize: 24,
			encode: { x: 'x', y: 'z' },
			symbol: 'pin',
			itemStyle: { color: '#000' }
		}
		];

		this.xy_config.yAxis = { type: 'value', name: 'y', min: yLim[0], max: yLim[1] };
		this.xz_config.yAxis = { type: 'value', name: 'z', min: zLim[0], max: zLim[1] };

		this.xy_plot.setOption(this.xy_config);
		this.xz_plot.setOption(this.xz_config);

		/**
		 * initialize_graphics() is responsible for drawing the circles
		 * and making the control points draggable. If you call this method,
		 * make sure to retest removing points/clearing trajectory.
		 * 
		 * @callback 
		 */
		let initialize_graphics = () => {
			this.xy_config.graphic = [];
			this.xz_config.graphic = [];

			// generate control point graphics
			for (let i = 0; i < this.points.x.length; i++) {
				let graphic_template = {
					id: i,
					type: 'circle',
					$action: 'replace',
					shape: { cx: 0, cy: 0, r: 10 },
					z: 100,
					invisible: i === 0,  // show circles if you're not EE - EE has special marker from series 2
					draggable: i !== 0, // do not allow dragging of EE
					style: {
						fill: '#FFF',
						stroke: '#FF0000'
					},
					onmouseover: () => {
						let showTipAction = {
							type: 'showTip',
							seriesIndex: 0, // the control points will always be at index 0
							dataIndex: i
						};
						this.xy_plot.dispatchAction(showTipAction);
						this.xz_plot.dispatchAction(showTipAction);
					},
					onmouseout: () => {
						let hideTipAction = { type: 'hideTip' };
						this.xy_plot.dispatchAction(hideTipAction);
						this.xz_plot.dispatchAction(hideTipAction);
					}
				};

				let xy_graphic = {
					...graphic_template,
					position: this.xy_plot.convertToPixel('grid', [this.points.x[i], this.points.y[i]]),
					ondrag: params => {
						let pt = this.xy_plot.convertFromPixel('grid', params.target.position);
						this.points.x[i] = pt[0];
						this.points.y[i] = pt[1];
						initialize_graphics();
						this.xy_plot.setOption(this.xy_config);
						this.xz_plot.setOption(this.xz_config);
					}
				};

				let xz_graphic = {
					...graphic_template,
					position: this.xz_plot.convertToPixel('grid', [this.points.x[i], this.points.z[i]]),
					ondrag: params => {
						let pt = this.xz_plot.convertFromPixel('grid', params.target.position);
						this.points.x[i] = pt[0];
						this.points.z[i] = pt[1];
						initialize_graphics();
						this.xy_plot.setOption(this.xy_config);
						this.xz_plot.setOption(this.xz_config);
					}
				};

				this.xy_config.graphic.push(xy_graphic);
				this.xz_config.graphic.push(xz_graphic);
			}

			// generate trajectory graphics
			for (let i = 0; i < this.trajectory.x.length; i++) {
				let graphic_template = {
					type: 'circle',
					$action: 'replace',
					shape: { cx: 0, cy: 0, r: 10 },
					z: 25,
					invisible: true,
					onmouseover: () => {
						let showTipAction = {
							type: 'showTip',
							seriesIndex: 1, // the control points will always be at index 0
							dataIndex: i
						};
						this.xy_plot.dispatchAction(showTipAction);
						this.xz_plot.dispatchAction(showTipAction);
					},
					onmouseout: () => {
						let hideTipAction = { type: 'hideTip' };
						this.xy_plot.dispatchAction(hideTipAction);
						this.xz_plot.dispatchAction(hideTipAction);
					}
				};

				let xy_graphic = {
					...graphic_template,
					position: this.xy_plot.convertToPixel('grid', [this.trajectory.x[i], this.trajectory.y[i]]),
				};

				let xz_graphic = {
					...graphic_template,
					position: this.xz_plot.convertToPixel('grid', [this.trajectory.x[i], this.trajectory.z[i]]),
				};

				this.xy_config.graphic.push(xy_graphic);
				this.xz_config.graphic.push(xz_graphic);
			}
		};

		// set up event listeners
		window.addEventListener('resize', () => {
			this.xy_plot.resize();
			this.xz_plot.resize();
			initialize_graphics();
			this.xy_plot.setOption(this.xy_config);
			this.xz_plot.setOption(this.xz_config);
		});

		addPointButton.onclick = () => {
			this.points.x.push((xLim[0] + xLim[1]) / 2);
			this.points.y.push((yLim[0] + yLim[1]) / 2);
			this.points.z.push((zLim[0] + zLim[1]) / 2);
			this.points.idx.push(this.next_point_index);

			initialize_graphics();
			this.xy_plot.setOption(this.xy_config);
			this.xz_plot.setOption(this.xz_config);

			let opt = document.createElement('option');
			opt.value = this.next_point_index;
			opt.innerHTML = 'Point ' + opt.value;
			opt.selected = true;
			pointSelect.append(opt);
			$('.point-remover').trigger("chosen:updated");

			this.next_point_index++;
		};

		getTrajectoryButton.onclick = () => {
			let tf = parseFloat(trajectoryMaxTimeInput.value);
			let t_step = parseFloat(trajectoryStepSizeInput.value);

			if (t_step > tf || !t_step || !tf) {
				errorMessageLabel.innerHTML = 'Bad trajectory final time or timestep.';
				return;
			}

			errorMessageLabel.innerHTML = '';

			// collect points
			let points = [this.points.x, this.points.y, this.points.z];
			let fetchOptions = {
				method: 'POST',
				headers: new Headers({ 'Content-Type': 'application/json' }),
				mode: 'same-origin',
				body: JSON.stringify({ tf, t_step, points })
			};

			fetch('/trajectory/generate', fetchOptions)
				.then(response => response.json())
				.then(data => {
					this.trajectory.x = data.pos[0];
					this.trajectory.y = data.pos[1];
					this.trajectory.z = data.pos[2];
					this.trajectory.t = data.time;
					this.trajectory.v = data.vel;
					this.trajectory.a = data.accel;

					// set max accel/vel UI elements
					maxAccelNorm.innerHTML = data.max_accel.norm.toExponential(3);
					maxAccelX.innerHTML = data.max_accel.x.toExponential(3);
					maxAccelY.innerHTML = data.max_accel.y.toExponential(3);
					maxAccelZ.innerHTML = data.max_accel.z.toExponential(3);
					maxVelNorm.innerHTML = data.max_vel.norm.toExponential(3);
					maxVelX.innerHTML = data.max_vel.x.toExponential(3);
					maxVelY.innerHTML = data.max_vel.y.toExponential(3);
					maxVelZ.innerHTML = data.max_vel.z.toExponential(3);

					// reset draggable
					initialize_graphics();
					this.xy_plot.setOption(this.xy_config);
					this.xz_plot.setOption(this.xz_config);
				});
		};

		let _trajectory_running = false;
		runTrajectoryButton.className = 'button-enable';
		runTrajectoryButton.onclick = () => {
			// NOTE: we have the server recompute trajectory
			let tf = parseFloat(trajectoryMaxTimeInput.value);
			let t_step = parseFloat(trajectoryStepSizeInput.value);

			if (t_step > tf || !t_step || !tf) {
				errorMessageLabel.innerHTML = 'Bad trajectory final time / timestep';
				return;
			}

			errorMessageLabel.innerHTML = '';

			// collect points
			let points = [this.points.x, this.points.y, this.points.z];
			let fetchOptions = {
				method: 'POST',
				headers: new Headers({ 'Content-Type': 'application/json' }),
				mode: 'same-origin',
				body: JSON.stringify({
					tf, t_step, points, // things needed for trajectory gen
					primitive_key, primitive_value, position_key, velocity_key
				})
			};

			let running_callback = () => {
				// poll repeatedly to get ee_pos & status
				// clear both polling timers once complete
				let ee_pos_poll_id = setInterval(() => {
					get_redis_val(current_ee_pos_key).then(data => {
						this.ee_pos.x[0] = data[0];
						this.ee_pos.y[0] = data[1];
						this.ee_pos.z[0] = data[2];
						this.xy_plot.setOption(this.xy_config);
						this.xz_plot.setOption(this.xz_config);
					});
				}, 100);

				let id = setInterval(() => {
					let poll_fetch_options = {
						method: 'GET',
						headers: new Headers({ 'Content-Type': 'application/json' }),
						mode: 'same-origin'
					};

					fetch('/trajectory/run/status', poll_fetch_options)
						.then(response => response.json())
						.then(data => {
							if (!data.running) {
								clearTimeout(id);
								clearTimeout(ee_pos_poll_id);

								// now that trajectory is finished, update this.point with new ee pos
								this.points.x[0] = this.ee_pos.x[0];
								this.points.y[0] = this.ee_pos.y[0];
								this.points.z[0] = this.ee_pos.z[0];
								this.xy_plot.setOption(this.xy_config);
								this.xz_plot.setOption(this.xz_config);

								_trajectory_running = false;
								runTrajectoryButton.innerHTML = 'Start Trajectory';
								runTrajectoryButton.className = 'button-enable';
							}
						});
				}, tf / 10);
			};

			// update state & UI
			_trajectory_running = !_trajectory_running;

			if (_trajectory_running) {
				runTrajectoryButton.className = 'button-disable';
				runTrajectoryButton.innerHTML = 'Stop Trajectory';
				fetch('/trajectory/run', fetchOptions)
					.then(() => running_callback())
					.catch(error => {
						errorMessageLabel.innerHTML = 'Trajectory execution failed: ' + toString(error);
					}
					);
			} else {
				fetch('/trajectory/run/stop', fetchOptions)
					.then(response => {
						if (response.ok)
							_trajectory_running = false;
					});
			}
		};

		clearAllPointsTrajectoryButton.onclick = () => {
			this.trajectory.x.length = 0;
			this.trajectory.y.length = 0;
			this.trajectory.z.length = 0;
			this.trajectory.t.length = 0;
			this.trajectory.v.length = 0;

			let new_points = { x: [], y: [], z: [], idx: [] };
			for (let i = 0; i < this.points.idx.length; i++) {
				this.xy_config.graphic[i].$action = 'remove';
				this.xz_config.graphic[i].$action = 'remove';

				if (i == 0) {
					new_points.idx.push(this.points.idx[i]);
					new_points.x.push(this.points.x[i]);
					new_points.y.push(this.points.y[i]);
					new_points.z.push(this.points.z[i]);
				}
			}

			// setOption will wipe the old points
			this.xy_plot.setOption(this.xy_config);
			this.xz_plot.setOption(this.xz_config);

			// this will reassign the new point list
			this.points = new_points;
			this.xy_config.dataset[0].source = this.points;
			this.xz_config.dataset[0].source = this.points;
			this.xy_plot.setOption(this.xy_config);
			this.xz_plot.setOption(this.xz_config);

			// redraw draggable circles
			initialize_graphics();
			this.xy_plot.setOption(this.xy_config);
			this.xz_plot.setOption(this.xz_config);

			$(".point-remover").val('').trigger("chosen:updated");
			$(".point-remover").empty();
		}

		clearTrajectoryButton.onclick = () => {
			this.trajectory.x.length = 0;
			this.trajectory.y.length = 0;
			this.trajectory.z.length = 0;
			this.trajectory.t.length = 0;
			this.trajectory.v.length = 0;
			this.xy_plot.setOption(this.xy_config);
			this.xz_plot.setOption(this.xz_config);
		}

		pointSelect.onchange = e => {
			let new_points = { x: [], y: [], z: [], idx: [] };

			let options = [];
			for (let option of e.target.selectedOptions) {
				options.push(parseInt(option.value));
			}

			// idea: wipe old set by marking each to remove
			// recreate a new point list
			for (let i = 0; i < this.points.idx.length; i++) {
				this.xy_config.graphic[i].$action = 'remove';
				this.xz_config.graphic[i].$action = 'remove';

				if (i == 0 || options.includes(this.points.idx[i])) {
					new_points.idx.push(this.points.idx[i]);
					new_points.x.push(this.points.x[i]);
					new_points.y.push(this.points.y[i]);
					new_points.z.push(this.points.z[i]);
				}
			}

			// setOption will wipe the old points
			this.xy_plot.setOption(this.xy_config);
			this.xz_plot.setOption(this.xz_config);

			// this will reassign the new point list
			this.points = new_points;
			this.xy_config.dataset[0].source = this.points;
			this.xz_config.dataset[0].source = this.points;
			this.xy_plot.setOption(this.xy_config);
			this.xz_plot.setOption(this.xz_config);

			// redraw draggable circles
			initialize_graphics();
			this.xy_plot.setOption(this.xy_config);
			this.xz_plot.setOption(this.xz_config);
		};

		// append to document
		this.appendChild(template_node);

		// grab ee pos and initialize plot afterwards
		// goal: we want the special marker for EE pos
		// and we want a dashed line connecting the EE pos to the other control points
		// problem: EE pos is in a different series than the control points, so no line
		// solution: we have the EE pos in series 2 with special marker, but make an invisible
		//  point in series 0 (control points) right on the EE pos to get the dashed line.
		get_redis_val(current_ee_pos_key).then(data => {
			this.ee_pos.x[0] = data[0];
			this.ee_pos.y[0] = data[1];
			this.ee_pos.z[0] = data[2];
			this.points.x.push(data[0]);
			this.points.y.push(data[1]);
			this.points.z.push(data[2]);
			this.points.idx.push(0);
			initialize_graphics();
			this.xy_plot.setOption(this.xy_config);
			this.xz_plot.setOption(this.xz_config);
			this.xy_plot.resize();
			this.xz_plot.resize();
		});
	}

	refresh() {
		window.dispatchEvent(new Event('resize'));
	}
});
