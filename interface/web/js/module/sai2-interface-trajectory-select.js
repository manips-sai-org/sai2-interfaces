import { registerWindowResizeCallback } from '../resize.js';

const template = document.createElement('template');
template.innerHTML = `
  <style>
    .sai2-interface-trajectory-select-top {
      display: flex;
      flex-direction: column;
      height: 95%;
    }

    .sai2-interface-trajectory-select-top .metadata {
      display: flex;
      align-items: baseline;
      flex-wrap: wrap;
      justify-content: space-around;
      flex: 1;
    }    

    .sai2-interface-trajectory-select-top .plots {
      flex: 9;
    }  
  </style>
	<div class="sai2-interface-trajectory-select-top">
    <div class="metadata">
      <label>Trajectory Duration</label>
      <input class="traj-max-time" type="number" min="0.1" step="0.1">
      <label>Trajectory Step Size</label>
      <input class="traj-step-time" type="number" min="0.01" step="0.01">
      <button class="trajectory-add-pt-btn">Add Point</button>
      <button class="trajectory-get-btn">Get Trajectory</button>
      <button class="trajectory-clear-btn">Clear Trajectory</button>
      <button class="trajectory-run-btn">Run Trajectory</button>
      <select class="point-remover chosen_select" multiple data-placeholder="Add a point..."></select>
    </div>
    <div class="grid-half">
      <div class="col traj-xy"></div>
      <div class="col traj-xz"></div>
    </div>
  </div>
`;
    
customElements.define('sai2-interface-trajectory-select', class extends HTMLElement {
  constructor() {
    super();
    this.template = template;

    this.xy_plot = null;
    this.xy_config = null;

    this.xz_plot = null;
    this.xz_config = null;

    this.points = { x: [], y: [], z: [], idx: [] };
    this.trajectory = { x: [], y: [], z: [], t:[], v: [] };
    this.next_point_index = 0;
  }

  connectedCallback() {
    // get DOM elements
    let template_node = this.template.content.cloneNode(true);
    let top_level_div = template_node.querySelector('.sai2-interface-trajectory-select-top');
    let xy_div = template_node.querySelector('.traj-xy');
    let xz_div = template_node.querySelector('.traj-xz');

    let addPointButton = template_node.querySelector('.trajectory-add-pt-btn');
    let getTrajectoryButton = template_node.querySelector('.trajectory-get-btn');
    let clearTrajectoryButton = template_node.querySelector('.trajectory-clear-btn');
    let runTrajectoryButton = template_node.querySelector('.trajectory-run-btn');
    let pointSelect = template_node.querySelector('.point-remover');
    let trajectoryMaxTimeInput = template_node.querySelector('.traj-max-time');
    let trajectoryStepSizeInput = template_node.querySelector('.traj-step-time');

    // grab passed-in attributes
    let xLim = JSON.parse(this.getAttribute('xLim'));
    let yLim = JSON.parse(this.getAttribute('yLim'));
    let zLim = JSON.parse(this.getAttribute('zLim'));

    let primitive_key = "sai2::examples::primitive";
    let primitive_value = "primitive_trajectory_task";
    let position_key = "sai2::examples::desired_position";
    let velocity_key = "sai2::examples::desired_velocity";

    // initialize template
    this.default_config = {
      grid: {},
      xAxis: { type:'value', name: 'x', min: xLim[0], max: xLim[1] }, 
      legend: { type: 'scroll' },
      toolbox: {
        top: 'bottom',
        left: 'right',
        feature: {
          saveAsImage: { title: 'Save Plot'},
          dataZoom: { title: { zoom: 'Box Zoom', back: 'Reset View' } }
        }
      },
      dataset: [
        { source: this.points },
        { source: this.trajectory }
      ]
    };

    // initialize select
    $('.point-remover').chosen({ width: '100%' });

    // initialize empty plot & templates
    this.xy_plot = echarts.init(xy_div);
    this.xz_plot = echarts.init(xz_div);

    this.xy_config = {...this.default_config};
    this.xz_config = {...this.default_config};

    this.xy_config.tooltip = { 
      triggerOn: 'none',
      formatter: params => {
        if (params.seriesIndex === 0)
          return `Point ${this.points.idx[params.dataIndex]}
            <br>X: ${this.points.x[params.dataIndex]}
            <br>Y: ${this.points.y[params.dataIndex]}`;
        else
          return `Time ${this.trajectory.t[params.dataIndex].toFixed(4)}
            <br>X: ${this.trajectory.x[params.dataIndex].toFixed(2)}
            <br>Y: ${this.trajectory.y[params.dataIndex].toFixed(2)}
            <br>V: ${this.trajectory.v[params.dataIndex].toFixed(2)}`;
      }
    };

    this.xz_config.tooltip = { 
      triggerOn: 'none',
      formatter: params => {
        if (params.seriesIndex === 0)
          return `Point ${this.points.idx[params.dataIndex]}
            <br>X: ${this.points.x[params.dataIndex].toFixed(2)} 
            <br>Z: ${this.points.z[params.dataIndex].toFixed(2)}`;
        else
          return `Time ${this.trajectory.t[params.dataIndex].toFixed(4)}
            <br>X: ${this.trajectory.x[params.dataIndex].toFixed(2)}
            <br>Z: ${this.trajectory.z[params.dataIndex].toFixed(2)}
            <br>V: ${this.trajectory.v[params.dataIndex].toFixed(2)}`;
      }
    };
    
    this.xy_config.series = [{
        id: 'xy',
        type: 'line',
        datasetIndex: 0,
        lineStyle: { type: 'dashed' },
        encode: { x: 'x', y: 'y' },
        symbolSize: 24
      }, {
        id: 'xy-traj',
        type: 'line',
        datasetIndex: 1,
        encode: { x: 'x', y: 'y'},
        symbolSize: false,
        lineStyle: { color: 'blue' }
      }
    ];

    this.xz_config.series = [{
        id: 'xz',
        type: 'line',
        datasetIndex: 0,
        lineStyle: {type: 'dashed'},
        encode: { x: 'x', y: 'z' },
        symbolSize: 24,
      }, {
        id: 'xz-traj',
        type: 'line',
        datasetIndex: 1,
        encode: { x: 'x', y: 'z'},
        symbolSize: false,
        lineStyle: { color: 'blue' }
      }
    ];

    this.xy_config.yAxis = { type:'value', name: 'y', min: yLim[0], max: yLim[1] };
    this.xz_config.yAxis = { type:'value', name: 'z', min: zLim[0], max: zLim[1] };

    this.xy_plot.setOption(this.xy_config);
    this.xz_plot.setOption(this.xz_config);

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
          invisible: true,
          draggable: true,
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
            console.log(params);
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
    registerWindowResizeCallback(() => {
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

      // XXX: handle more gracefully with an error message
      if (t_step > tf || !t_step || !tf) {
        alert('Bad t_step');
        return;
      }

      // collect points
      let points = [this.points.x, this.points.y, this.points.z];
      let fetchOptions = {
        method: 'POST',
        headers: new Headers({'Content-Type': 'application/json'}),
        mode: 'same-origin',
        body: JSON.stringify({tf, t_step, points})
      };

      fetch('/trajectory/generate', fetchOptions)
        .then(response => response.json())
        .then(data => {
          this.trajectory.x = data.pos[0];
          this.trajectory.y = data.pos[1];
          this.trajectory.z = data.pos[2];
          this.trajectory.t = data.time;
          this.trajectory.v = data.vel;
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

      // XXX: handle more gracefully with an error message
      if (t_step > tf || !t_step || !tf) {
        alert('Bad t_step');
        return;
      }

      // collect points
      let points = [this.points.x, this.points.y, this.points.z];
      let fetchOptions = {
        method: 'POST',
        headers: new Headers({'Content-Type': 'application/json'}),
        mode: 'same-origin',
        body: JSON.stringify({
          tf, t_step, points, // things needed for trajectory gen
          primitive_key, primitive_value, position_key, velocity_key
        })
      };

      let running_callback = () => {
        // poll repeatedly to get status
        let id = setInterval(() => {
          let poll_fetch_options = {
            method: 'GET',
            headers: new Headers({'Content-Type': 'application/json'}),
            mode: 'same-origin'
          };

          fetch('/trajectory/run/status', poll_fetch_options)
            .then(response => response.json())
            .then(data => {
              if (!data.running) {
                clearTimeout(id);
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
            console.log(error);
            alert('Running trajectory failed!');
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

    clearTrajectoryButton.onclick = () => {
      this.trajectory.x.length = 0;
      this.trajectory.y.length = 0;
      this.trajectory.z.length = 0;
      this.trajectory.t.length = 0;
      this.trajectory.v.length = 0;
      let _dataset = {
        dataset: [
          { source: this.points },
          { source: this.trajectory }
        ]
      };

      this.xy_plot.setOption(_dataset);
      this.xz_plot.setOption(_dataset);
    }

    pointSelect.onchange = e => {
      let options = [];
      for (let option of e.target.selectedOptions) {
        options.push(parseInt(option.value));
      }

      for (let i = this.points.idx.length - 1; i >= 0; i--) {
        if (options.includes(this.points.idx[i]))
          continue;
        
        this.points.x.splice(i, 1);
        this.points.y.splice(i, 1);
        this.points.z.splice(i, 1);
        this.points.idx.splice(i, 1);
      }

      initialize_graphics();
      this.xy_plot.setOption(this.xy_config);
      this.xz_plot.setOption(this.xz_config);
    };

    // append to document
    this.appendChild(template_node);

    // notify plots to resize, one after the other
    setTimeout(() => {
      this.xy_plot.resize();
      this.xz_plot.resize();
    }, 250);
  }
});
