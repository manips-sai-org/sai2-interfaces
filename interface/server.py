from flask import Flask, jsonify, request, Response, send_file, send_from_directory
from flask_socketio import SocketIO, send, emit
from redis_cache import RedisCache
from redis_logger import RedisLogger, display_log_file
from trajectory_runner import TrajectoryRunner
from plot import PlotManager
from werkzeug.utils import secure_filename
import json 
import click
import redis
import catmullrom
import numpy as np
import os 
import sys
import time

# determine full, absolute path to web/
static_folder_path = os.path.join(os.path.dirname(os.path.realpath(sys.argv[0])), 'web')

# bypass Flask templating engine by serving our HTML as static pages
example_to_serve = None
app = Flask(__name__, static_folder=static_folder_path, static_url_path='')
app.config['UPLOAD_FOLDER'] = '/tmp'
app.config['SEND_FILE_MAX_AGE_DEFAULT'] = 0
socketio = SocketIO(app)

#### global variables, initialized in server start ####
example_to_serve = None
redis_client = None
redis_cache = None
redis_logger = None
trajectory_runner = None

########### ROUTE HANDLING ################

@app.route('/')
def get_home():
    ''' Gets the home page "/", which is the example passed in the CLI. '''
    return send_file(example_to_serve)


@socketio.on('redis')
def handle_socket_redis_call(data):
    keys = json.loads(data)
    if type(keys) == list:
        return jsonify({key: redis_cache.key_cache.get(key) for key in keys})
    return jsonify({keys: redis_cache.key_cache.get(keys)})

@app.route('/redis', methods=['GET','POST'])
def handle_redis_call():
    ''' 
    Handles get/set of Redis keys. Supports multiple getting of keys.

    Front-end documentation:
    To get a key, GET /redis with { 'key': 'your_redis_key_here' }.
    To get multiple keys, query /redis with { 'key': ['key1', 'key2' , ...] }

    To set a key, POST to /redis with a JSON object { 'key': 'key1', 'val': val }.
        `val` must be a valid JSON object or string or number.
    '''
    if request.method == 'GET':
        key_list = json.loads(request.args.get('key'))
        if type(key_list) == str:
            return jsonify(redis_cache[key_list])
        else:
            return jsonify({key: redis_cache[key] for key in key_list})
    elif request.method == 'POST':
        data = request.get_json()
        if type(data['val']) == list:
            redis_client.set(data['key'], json.dumps(data['val']))
        else:
            redis_client.set(data['key'], data['val'])

        return Response(status=200)

@app.route('/redis/keys', methods=['GET'])
def handle_get_all_redis_keys():
    '''
    Gets all SAI2 redis keys.
    
    Frontend documentation:
    GET to /redis/keys
        > Response is a sorted JSON list of keys.
    '''
    keys = list(redis_cache.key_cache.keys())
    keys.sort()
    return jsonify(keys)

@app.route('/logger/status', methods=['GET'])
def handle_logger_status():
    '''
    Gets the status of the attached logger, if any.
    There is only one logger per server.

    Frontend documentation:
    GET to /logger/status 
        > Response is a JSON object { running: True|False }
    '''
    return jsonify({'running': redis_logger.running})

@app.route('/logger/start', methods=['POST'])
def handle_logger_start():
    '''
    Starts the logger, if it is not already.

    Frontend documentation:
    POST to /logger/start with JSON object
    {
       'filename': <filename>,
       'logger_period': <frequency in seconds>,
       'keys': ['key1', 'key2', ...]
    }

    Returns 200 OK if successful start, 400 otherwise
    '''
    data = request.get_json()
    filename = os.path.join(static_folder_path, data['filename'])
    redis_keys = data['keys']
    logger_period = float(data['logger_period'])
    if redis_logger.start(filename, redis_keys, logger_period):
        return Response(status=200)
    else:
        return Response(status=400)

@app.route('/logger/stop', methods=['POST'])
def handle_logger_stop():
    '''
    Stops the logger.

    POST to /logger/stop
    Always returns 200 OK. 
    '''
    redis_logger.stop()
    return Response(status=200)

@app.route('/logger/offline', methods=['POST'])
def handle_logger_offline_plot():
    if 'file' not in request.files:
        return Response(status=200)

    file = request.files['file']
    if not file or not file.filename:
        return Response(status=200)
    
    filename = secure_filename(file.filename)
    local_path = os.path.join(app.config['UPLOAD_FOLDER'], filename)
    file.save(local_path)
    display_log_file(local_path)
    return Response(status=200)

@app.route('/plot/start', methods=['POST'])
def handle_plot_start():
    data = request.get_json()
    keys = data['keys']
    rate = data['rate']
    return jsonify({
        'plot_id': plot_manager.start_plot(keys, rate)
    })

@app.route('/plot/data', methods=['POST'])
def handle_plot_get():
    plot_id = request.get_json()['plot_id']
    avail = plot_manager.get_available_data_from_plot(plot_id)
    return jsonify({
        'plot_id': plot_id,
        'running': plot_manager.is_plot_running(plot_id),
        'data': avail
    })

@app.route('/plot/stop', methods=['POST'])
def handle_plot_stop():
    plot_id = request.get_json()['plot_id']
    if plot_manager.stop_plot(plot_id):
        return Response(status=200)
    else:
        return Response(status=500)

@app.route('/trajectory/generate', methods=['POST'])
def handle_trajectory_generate():
    '''
    Generates a trajectory and returns the points, timestamps, velocity, and
    acceleration data. Does not issue any commands to the controller.

    Frontend documentation:
    POST to /trajectory/generate with JSON object
    {
        'tf': <final time>,
        't_step': <time step>,
        'points': [[x1, x2, ....], [y1, y2, ....], [z1, z2, ...]]
    }

    Returns JSON object of 
    {
        'time': [t1, t2, ....],
        'pos':  [[x1, x2, ...], [y1, y2, ...], [z1, z2, ...]],
        'vel': [[x1, x2, ...], [y1, y2, ...], [z1, z2, ...]],
        'max_vel': { 'norm': n, 'x': x, 'y': y, 'z': z },
        'accel': [[x1, x2, ...], [y1, y2, ...], [z1, z2, ...]],
        'max_accel':  { 'norm': n, 'x': x, 'y': y, 'z': z },
    }
    '''
    data = request.get_json()
    tf = data['tf']
    t_step = data['t_step']
    P = np.array(data['points'])
    (t_traj, P_traj, V_traj, A_traj) = catmullrom.compute_catmullrom_spline_trajectory(tf, P, t_step)
    V_traj_norm = np.linalg.norm(V_traj, axis=0)
    A_traj_norm = np.linalg.norm(A_traj, axis=0)

    # since this is purely visualization, send only 50
    MAX_POINTS = 50
    if len(t_traj) > MAX_POINTS:
        idx = np.round(np.linspace(0, len(t_traj) - 1, MAX_POINTS)).astype(int)
        t_traj = t_traj[idx]
        P_traj = P_traj[:, idx]

    return jsonify({
        'time': t_traj.tolist(),
        'pos': P_traj.tolist(),
        'vel': V_traj_norm.tolist(),
        'max_vel' : {
            'norm': np.max(V_traj_norm),
            'x': np.max(V_traj[0, :]),
            'y': np.max(V_traj[1, :]),
            'z': np.max(V_traj[2, :])
        },
        'accel': A_traj_norm.tolist(),
        'max_accel': {
            'norm': np.max(A_traj_norm),
            'x': np.max(A_traj[0, :]),
            'y': np.max(A_traj[1, :]),
            'z': np.max(A_traj[2, :])
        }
    })

@app.route('/trajectory/run', methods=['POST'])
def handle_trajectory_run():
    '''
    Executes/runs/issues commands to the controller for a
    given trajectory.

    POST to /trajectory/run with JSON object
    {
        'primitive_key': key, # which redis key contains which primitive is active
        'primitive_value':  value, # what primitive to run under
        'position_key': pos_key, # redis key to set desired pos
        'velocity_key': vel_key, # redis key to set desired velocity
    }
    '''
    global trajectory_runner

    # parse request
    data = request.get_json()
    primitive_key = data['primitive_key']
    primitive_value = data['primitive_value']
    position_key = data['position_key']
    velocity_key = data['velocity_key']
    tf = data['tf']
    t_step = data['t_step']
    P = np.array(data['points'])

    # compute and run trajectory
    (t_traj, P_traj, V_traj, _) = catmullrom.compute_catmullrom_spline_trajectory(tf, P, t_step)
    trajectory_runner = TrajectoryRunner(
        redis_client, 
        primitive_key, 
        primitive_value, 
        position_key, 
        velocity_key
    )

    if trajectory_runner.start(t_traj, P_traj, V_traj, t_step):
        return Response(status=200)
    else:
        return Response(status=500)
    
@app.route('/trajectory/run/status', methods=['GET'])
def handle_trajectory_run_status():
    ''' 
    Gets the status of the trajectory runner.

    GET to /trajectory/run/status, returns
    { 'running': True|False }
    '''
    return jsonify({'running': (trajectory_runner and trajectory_runner.running)})

@app.route('/trajectory/run/stop', methods=['POST'])
def handle_trajectory_run_stop():
    '''
    Stops the trajectory runner. 

    GET to /trajectory/run/stop
    '''
    trajectory_runner.stop()
    return Response(status=200)


############ CLI + Server Init ##############
@click.command()
@click.option("-hp", "--http_port", help="HTTP Port (default: 8000)", default=8000, type=click.INT)
@click.option("-rh", "--redis_host", help="Redis hostname (default: localhost)", default="localhost", type=click.STRING)
@click.option("-rp", "--redis_port", help="Redis port (default: 6379)", default=6379, type=click.INT)
@click.option("-rd", "--redis_db", help="Redis database number (default: 0)", default=0, type=click.INT)
@click.option("-rate", "--cache-refresh-rate", help="How often to load keys from Redis (default: 0.0333)", default=0.0333, type=click.FLOAT)
@click.argument('example', type=click.Path(exists=True, file_okay=True, dir_okay=False, readable=True))
def server(http_port, redis_host, redis_port, redis_db, cache_refresh_rate, example):
    global redis_client, redis_cache, redis_logger, example_to_serve, plot_manager
    example_to_serve = os.path.realpath(example)
    redis_client = redis.Redis(host=redis_host, port=redis_port, db=redis_db, decode_responses=True)
    redis_cache = RedisCache(redis_client, refresh_rate=cache_refresh_rate)
    redis_logger = RedisLogger(redis_client)
    plot_manager = PlotManager(redis_cache)

    redis_cache.start()
    socketio.run(app, port=http_port, debug=True)


if __name__ == "__main__":
    server()
