from flask import Flask, jsonify, request, Response, send_file, send_from_directory
from redis_logger import RedisLogger
from trajectory_runner import TrajectoryRunner
import json 
import click
import redis
import catmullrom
import numpy as np
import os 
import sys

# determine full, absolute path to web/
static_folder_path = os.path.join(os.path.dirname(os.path.realpath(sys.argv[0])), 'web')

# bypass Flask templating engine by serving our HTML as static pages
example_to_serve = None
app = Flask(__name__, static_folder=static_folder_path, static_url_path='')


#### global variables, initialized in server start ####
example_to_serve = None
redis_client = None
redis_logger = None
trajectory_runner = None

###########    UTILITY     ################
def get_redis_key(key):
    ''' 
    Retrieves a key from redis and attempts JSON parsing.
    We don't want to send a JSON object hiding in a string to the
    frontend, who would then be forced to double unwrap.

    :param key: A string containing the Redis key to query
    :returns: A string if Redis key is scalar, or a list/dict
    '''
    redis_str = redis_client.get(key)
    try:
        return json.loads(redis_str)
    except:
        return redis_str

########### ROUTE HANDLING ################

@app.route('/')
def get_home():
    ''' Gets the home page "/", which is the example passed in the CLI. '''
    global example_to_serve
    return send_file(example_to_serve)

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
            return jsonify(get_redis_key(key_list))
        else:
            return jsonify({key: get_redis_key(key) for key in key_list})
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
    all_keys = [key for key in redis_client.scan_iter('sai2::*')]
    all_keys.sort()
    return jsonify(all_keys)

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
    filename = data['filename']
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
@click.group()
def server():
    pass

@server.command()
@click.option("-hp", "--http_port", help="HTTP Port (default: 8000)", default=8000, type=click.INT)
@click.option("-rh", "--redis_host", help="Redis hostname (default: localhost)", default="localhost", type=click.STRING)
@click.option("-rp", "--redis_port", help="Redis port (default: 6379)", default=6379, type=click.INT)
@click.option("-rd", "--redis_db", help="Redis database number (default: 0)", default=0, type=click.INT)
@click.argument('example', type=click.Path(exists=True, file_okay=True, dir_okay=False, readable=True))
def start(http_port, redis_host, redis_port, redis_db, example):
    global redis_client, redis_logger, example_to_serve
    example_to_serve = os.path.realpath(example)
    redis_client = redis.Redis(host=redis_host, port=redis_port, db=redis_db, decode_responses=True)
    redis_logger = RedisLogger(redis_client)
    app.run(port=http_port, debug=True)


if __name__ == "__main__":
    server()
