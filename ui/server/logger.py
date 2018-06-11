# read redis keys and dump them to a file
import redis
import time
import signal
import sys
import os
import json
from threading import Thread

thread = 0
runloop = True

# handle ctrl-C and close the files
def signal_handler(signal, frame):
    global runloop
    runloop = False
    stop_logging()
    print('Exiting logger')

signal.signal(signal.SIGINT, signal_handler)

def start_logging(dir, redis_server):
    global thread
    # stop existing logging threads
    # stop_logging()

    runloop = True
    os.makedirs(dir, exist_ok=True)
    log_file_name = 'logs-' + time_str() + '.txt'
    log_path = os.path.join(dir, log_file_name)
    thread = Thread(target=log, args=(log_path, redis_server))
    thread.start()
    print('started logging')

def stop_logging():
    global threads
    global runloop
    runloop = False
    if type(thread) is 'threading.Thread': thread.join()
    print('killed log thread')

def log(log_path, redis_server):
    header = 'time;joint_pos;joint_vel;ee_pos;ee_rot;ee_des_pos;ee_des_rot;command_torques\n'
    log_file = open(log_path, 'w')
    log_file.write(header)

    # redis keys used in SAI2
    LOG_TIME_KEY = "sai2::iiwaForceControl::iiwaBot::simulation::data_log::time"
    LOG_JOINT_POS_KEY = "sai2::iiwaForceControl::iiwaBot::simulation::data_log::joint_pos"
    LOG_JOINT_VEL_KEY = "sai2::iiwaForceControl::iiwaBot::simulation::data_log::joint_vel"
    LOG_EE_POS_KEY = "sai2::iiwaForceControl::iiwaBot::simulation::data_log::ee_pos"
    LOG_EE_VEL_KEY = "sai2::iiwaForceControl::iiwaBot::simulation::data_log::ee_vel"
    LOG_COMMAND_TORQUE_KEY = "sai2::iiwaForceControl::iiwaBot::simulation::data_log::command_torques"

    # data logging frequency
    logger_frequency = 1000.0  # Hz
    logger_period = 1.0 / logger_frequency
    t_init = time.time()
    t = t_init
    counter = 0
    last_timestamp = 0

    print('Start Logging Data to: %s' % log_path)
    while(runloop):
        # if same data, don't log
        timestamp = redis_server.get(LOG_TIME_KEY)
        joint_pos = redis_server.get(LOG_JOINT_POS_KEY)
        joint_vel = redis_server.get(LOG_JOINT_VEL_KEY)
        ee_pos = redis_server.get(LOG_EE_POS_KEY)
        ee_vel = redis_server.get(LOG_EE_VEL_KEY)
        command_torque = redis_server.get(LOG_COMMAND_TORQUE_KEY)
        
        line = '%s;%s;%s;%s;%s;%s\n' % (timestamp, joint_pos, joint_vel, ee_pos, ee_vel, command_torque)
        log_file.write(line)

        t += logger_period
        counter += 1
        last_timestamp = timestamp
        time_sleep = max(0.0, t - time.time())
        time.sleep(time_sleep)

    elapsed_time = time.time() - t_init
    print("Elapsed time : ", elapsed_time, " seconds")
    print("Loop cycles  : ", counter)
    print("Frequency    : ", counter/elapsed_time, " Hz")

    log_file.close()

def time_str():
    return time.strftime('%Y-%m-%d-%H-%M-%S')

def array_to_str(array):
    return ' '.join([str(_) for _ in json.loads(array)])


if __name__ == '__main__':
    redis_client = redis.Redis(host='localhost', port=6379, db=0, decode_responses=True)
    start_logging('../logs', redis_client)
    stop_logging()

