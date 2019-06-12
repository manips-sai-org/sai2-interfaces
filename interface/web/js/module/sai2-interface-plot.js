import { get_redis_val, get_redis_all_keys } from '../redis.js';
import { registerWindowResizeCallback } from '../resize.js';

const template = document.createElement('template');
template.innerHTML = `
  <style>
    .sai2-interface-plot-top {
      display: flex;
      flex-direction: column;
      height: 100%;
    }

    .sai2-interface-plot-top .metadata {
      display: flex;
      align-items: baseline;
      flex-wrap: wrap;
      justify-content: space-around;
    }

    .sai2-interface-plot-top .plot-div {
      flex: 1;
    }
  </style>
	<div class="sai2-interface-plot-top">
    <div class="metadata">
      <select class="x_key chosen_select" data-placeholder="Select x key..."></select>
      <select class="y_key chosen_select" multiple data-placeholder="Select y key..."></select>
      <h3>Rate:&nbsp;</h3>
      <input class="query_rate" type="number" step="0.1">
      <button class="plot_button"></button>
      <label class="error-label" style="color:red;"><label>
    </div>
    <div class="plot-div"></div>
	</div>
`;

    
customElements.define('sai2-interface-plot', class extends HTMLElement {
  constructor() {
    super();
    this.template = template;

    this.chart = null;
    this.chart_config = null;
    this.plotting = false;
    
    this.x_key = null;
    this.y_key_list = [];
    this.plot_start_time = null;

    this.fetchDataCallback = this.fetchDataCallback.bind(this);
  }

  fetchDataCallback() {
    let keys_to_fetch = [];
    if (this.x_key == 'Time')
      keys_to_fetch = this.y_key_list;
    else
      keys_to_fetch = [this.x_key, ...this.y_key_list];

    get_redis_val(keys_to_fetch).then(data => {
      // assumption: x is scalar valued
      let x_data = null;
      if (this.x_key == 'Time') 
        x_data = (performance.now() - this.plot_start_time) / 1000;
      else 
        x_data = data[this.x_key];

      // helper function to add series for y if necessary
      let add_data = (x_key, y_key, y_val) => {
        if (!(y_key in this.chart_config.dataset.source)) {
          this.chart_config.dataset.source[y_key] = [];
          this.chart_config.series.push({
            name: y_key,
            type: 'line',
            symbolSize: false,
            encode: { x: x_key, y: y_key }
          });
        }
        this.chart_config.dataset.source[y_key].push(y_val);
      };

      // check if x is setup as a series
      if (!(this.x_key in this.chart_config.dataset.source)) {
        this.chart_config.xAxis.name = this.x_key;
        this.chart_config.dataset.source[this.x_key] = [];
      }

      // add x point
      this.chart_config.dataset.source[this.x_key].push(x_data);

      // add y point(s)
      for (let y_key of this.y_key_list) {
        let y_data = data[y_key];
        if (Array.isArray(y_data)) {
          for (let i = 0; i < y_data.length; i++) {
            let y_key_element = y_key + '[' + i + ']';
            add_data(this.x_key, y_key_element, y_data[i]);
          }
        } else {
          add_data(this.x_key, y_key, y_data);
        }
      }

      this.chart.setOption(this.chart_config);
    });
  }

  connectedCallback() {
    // attributes
    this.plot_type = this.getAttribute('plotType') || 'scatter';

    // get DOM elements
    let template_node = this.template.content.cloneNode(true);
    let xkey_select = template_node.querySelector('.x_key');
    let ykey_select = template_node.querySelector('.y_key');
    let query_rate_input = template_node.querySelector('.query_rate');
    let plot_button = template_node.querySelector('.plot_button');
    let plot_div = template_node.querySelector('.plot-div');
    let error_label = template_node.querySelector('.error-label');

    plot_button.innerHTML = this.plotting ? 'Stop' : 'Start';
    plot_button.className = 'button-enable';

    // populate keys list
    get_redis_all_keys().then(keys => {
      // add builtin time key
      let time_opt = document.createElement('option');
      time_opt.value = 'Time';
      time_opt.innerHTML = 'Time';
      xkey_select.append(time_opt);

      for (let key of keys.values()) {
        let opt = document.createElement('option');
        opt.value = key;
        opt.innerHTML = key;
        xkey_select.append(opt);

        let opt2 = opt.cloneNode();
        opt2.innerHTML = key;
        ykey_select.append(opt2);
      }

      $('.x_key').chosen({ width: '100%' });
      $('.y_key').chosen({ width: '100%' });

      $('.x_key').trigger("chosen:updated");
      $('.y_key').trigger("chosen:updated");
    });

    // initialize empty plot & templates
    this.chart = echarts.init(plot_div);
    this.chart_config = {
      dataset: { source: {} },
      xAxis: { type:'value' },
      yAxis: { type:'value' },
      series: [],
      tooltip: { trigger: 'axis' },
      legend: { type: 'scroll' },
      toolbox: {
        top: 'bottom',
        left: 'right',
        feature: {
          saveAsImage: { title: 'Save Plot'},
          dataZoom: { title: { zoom: 'Box Zoom', back: 'Reset View' } }
        }
      },
    };

    this.chart.setOption(this.chart_config);
    
    // register event listeners
    xkey_select.onchange = e => {
      this.x_key = e.target.value;
    };

    plot_button.onclick = () => {
      this.x_key = xkey_select.value;
      this.y_key_list = [];

      for (let option of ykey_select.options)
        if (option.selected)
          this.y_key_list.push(option.value);

      if (!this.x_key || this.y_key_list.length == 0) {
        error_label.innerHTML = 'Please select the x and y keys to plot.';
        return;
      }

      error_label.innerHTML = '';

      this.plotting = !this.plotting;
      if (this.plotting) {
        xkey_select.disabled = true;
        ykey_select.disabled = true;
    
        // reset chart config
        this.chart_config.dataset.source = {};
        this.chart_config.series = [];
        this.chart_config.legend = { type: 'scroll' };
        this.chart.setOption(this.chart_config);

        plot_button.innerHTML = 'Stop';
        plot_button.className = 'button-disable';

        // determine rate. convert from sec -> ms
        let query_rate = 1000 * parseFloat(query_rate_input.value);

        // set up plot timer callback
        this.plot_start_time = performance.now();
        this.plotIntervalID = setInterval(this.fetchDataCallback, query_rate);
      } else {
        plot_button.innerHTML = 'Start';
        plot_button.className = 'button-enable';
        xkey_select.disabled = false;
        ykey_select.disabled = false;
            
        // clear plot timer callback
        clearInterval(this.plotIntervalID);
      }
    };
      
    // append to document
    this.appendChild(template_node);

    // set up size listeners
    registerWindowResizeCallback(() => {
      this.chart.resize();
    });

    // prime the pump: echarts plot needs to be
    // manually told to resize, but echarts needs an initial size
    // after we attach it to the DOM. so we wait a bit
    // and then resize
    setTimeout(() => this.chart.resize(), 250);
  }
});
