import { get_redis_all_keys } from '../redis.js';


const template = document.createElement('template');

template.innerHTML = `
  <style>
    .sai2-interface-logger-top {
      display: flex;
      flex-direction: row;
      align-items: baseline;
      justify-content: space-evenly;
      flex-wrap: wrap;
    }

    .sai2-interface-logger-top > input {
      width: 32%;
    }

    .sai2-interface-logger-top > div {
      width: 32%;
    }

  </style>
  <div class="sai2-interface-logger-top">
    <input type="text" class="logfile" placeholder="log filename">
    <input type="number" step="0.01" min="0" placeholder="1"
      class="logperiod" placeholder="(period, in seconds)">
    <div>
      <select multiple class="chosen_select" data-placeholder="Select keys to log..."></select>
    </div>
    <button></button>
  </div>
`;

customElements.define('sai2-interface-logger', class extends HTMLElement {
  constructor() {
    super();
    this.template = template;
    this.getLoggerStatus = this.getLoggerStatus.bind(this);
  }

  connectedCallback() {
    let template_node = this.template.content.cloneNode(true);
    
    let button = template_node.querySelector('button');
    let logfile_input = template_node.querySelector('.logfile');
    let logperiod_input = template_node.querySelector('.logperiod');
    let keys_select = template_node.querySelector('select');

    this.getLoggerStatus().then(status => {
      this.logging = status['running'];
      button.innerHTML = this.logging ? 'stop logging' : 'start logging';
      button.className = this.logging ? "button-disable" : "button-enable";
    });

    button.innerHTML = this.logging ? 'stop logging' : 'start logging';
    button.className = this.logging ? "button-disable" : "button-enable";

    // populate keys list
    get_redis_all_keys().then(keys => {
      for (let key of keys.values()) {
        let opt = document.createElement('option');
        opt.value = key;
        opt.innerHTML = key;
        keys_select.append(opt);
      }

      $('.chosen_select').chosen({width: '100%'});
    });

    // set up listeners
    button.onclick = () => {
      this.logging = !this.logging;
      if (this.logging) {
        // default to log.txt
        let filename = logfile_input.value || 'log.txt';

        // get selected keys
        let selected_keys = [];
        for (let option of keys_select.options)
          if (option.selected)
            selected_keys.push(option.value);

        // get logger period. default to 0.1s
        let logger_period = logperiod_input.value || 0.1;

        this.start_logging(filename, selected_keys, logger_period).then(() => {
          button.innerHTML = 'stop logging';
          button.className = "button-disable";
        });
      } else {
        this.stop_logging().then(() => {
          button.innerHTML = 'start logging';
          button.className = "button-enable"
        });
      }
    };
    
    // append to document
    this.appendChild(template_node);
  }
  
  getLoggerStatus() {
    let fetchOptions = {
      method: 'GET',
      headers: new Headers({'Content-Type': 'application/json'}),
      mode: 'same-origin'
    };

    return fetch('/logger/status', fetchOptions)
      .then(response => response.json())
      .catch(data => alert('logger error: ' + toString(data)));
  }

  start_logging(filename, key_list, period) {
    let fetchOptions = {
      method: 'POST',
      headers: new Headers({'Content-Type': 'application/json'}),
      mode: 'same-origin',
      body: JSON.stringify({
        filename: filename,
        keys: key_list,
        logger_period: period
      })
    };
  
    return fetch('/logger/start', fetchOptions)
      .then(response => response.ok)
      .catch(data => alert('logger redis error: ' + toString(data)));
  }

  stop_logging() {
    let fetchOptions = {
      method: 'POST',
      headers: new Headers({'Content-Type': 'application/json'}),
      mode: 'same-origin'
    };
  
    return fetch('/logger/stop', fetchOptions)
      .catch(data => alert('logger redis error: ' + toString(data)));
  }
});