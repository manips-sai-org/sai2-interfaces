import redis
import time
import json
import util
from threading import Thread
import multitimer
import numpy as np
import matplotlib.pyplot as plt 
import os
import re

class RedisLogger(object):
    '''
    The RedisLogger logs a list of Redis keys in the background given a
    frequency and output file.
    '''

    def __init__(self, redis_client):
        '''
        Constructs a new RedisLogger instance.

        :param redis_client: redis.Redis connection to Redis key-value store
        '''
        self.redis_client = redis_client
        self.periodic_timer = None
        self.running = False
        self.filename = ''
        self.logger_period = 0
        self.redis_keys = []
        self.log_start_time = None

    def _logger_loop(self, ctx):
        pipe = self.redis_client.pipeline(transaction=True)

        header = []
        header_written = ctx['header_written']

        if not header_written:
            self.file_fd.write('Logger Frequency: {} sec\n'.format(self.logger_period))

        # ensure first data point starts time 0
        if self.log_start_time is None:
            self.log_start_time = time.time()
            current_time = 0
        else:
            current_time = time.time() - self.log_start_time

        # ensure pipeline is built
        for key in self.redis_keys:
            pipe.get(key)

        # iterate through keys, write headers, and populate data
        raw_values = pipe.execute()
        values = []
        for key, raw_value in zip(self.redis_keys, raw_values):
            value = util.try_parse_json(raw_value if raw_value else '')
            if type(value) == list:
                values += value 
                # if we haven't written the header to the file,
                # expand the given key into key[len(key)]
                if not header_written:
                    header.append(key + '[{}]'.format(len(value)))
            else:
                values.append(value)
                if not header_written:
                    header.append(key)

        # write header if we haven't already
        # we can't write header right away since we don't know if
        # keys are vector or scalar valued
        # matrices are NOT supported
        if not header_written:
            self.file_fd.write('Time\t' +  '\t'.join(str(h) for h in header) + '\n')
            ctx['header_written'] = True

        # write keys
        self.file_fd.write(str(current_time) + '\t' + '\t'.join(str(v) for v in values) + '\n')


    def start(self, filename, redis_keys, logger_period=1):
        '''
        Starts the logger by spinning off a thread. If the logger is 
        already running, this call is ignored.

        :param filename: A string containing the filename to write keys to.
        :param redis_keys: A list of strings containing keys to poll Redis.
        :param logger_period: A float containing how often to poll Redis, in seconds.
            Default is 1 second.
        :returns: True if thread started, False if already running or failure.
        '''
        if self.running:
            return False

        self.filename = filename
        self.redis_keys = redis_keys
        self.logger_period = logger_period

        # start the timer 
        self.running = True
        self.file_fd = open(filename, 'w+')
        context = { 'header_written': False }
        self.periodic_timer = multitimer.MultiTimer(
            logger_period, 
            function=self._logger_loop, 
            kwargs={'ctx': context}
        )
        self.periodic_timer.start()

        return True

    def stop(self):
        '''
        Stops the logger. No-op if not running.
        '''
        if self.running:
            self.running = False
            self.periodic_timer.stop()
            self.file_fd.close()
        
def display_log_file(log_file):
    '''
    Parses and displays a valid log file from the RedisLogger class. Spins off
    a new thread so that it won't block the main server thread. 

    :param log_file: The log file output from RedisLogger.
    :returns: The thread object that was created for the new plot
    '''
    def _display_this_thread(log_file):
        keys = []
        data = []
        with open(log_file, 'r') as f:
            for idx, line in enumerate(f):
                if idx == 0: 
                    # frequency line
                    continue 
                elif idx == 1:
                    # keys line. assumption: no brackets in key name.
                    raw_keys = line.split()
                    assert(raw_keys[0].lower() == 'time')
                    keys.append(raw_keys[0])

                    for raw_key in raw_keys[1:]:
                        match = re.match(r'(.*)\[(\d.*)\]', raw_key)
                        if match:
                            name = match.group(1)
                            size = int(match.group(2))
                            for i in range(size):
                                keys.append('{}[{}]'.format(name, i))
                        else:
                            keys.append(raw_key)
                else:
                    # generic data line
                    data.append([float(val) for val in line.split()])

        # file is parsed, show in pyplot window
        data = np.array(data)
        _, m = data.shape
        plt.figure()
        ax = plt.subplot(111)
        plt.xlabel(keys[0] + ' (s)')
        for i in range(1, m):
            ax.plot(data[:, 0], data[:, i], label=keys[i])

        # move legend to the bottom of the plot
        # See https://stackoverflow.com/a/4701285
        box = ax.get_position()
        ax.set_position([box.x0, box.y0 + box.height * 0.1, 
                        box.width, box.height * 0.9]) 
        ax.legend(loc='upper center', bbox_to_anchor=(0.5, -0.11), ncol=2)
        plt.show()

    t = Thread(target=_display_this_thread, args=[log_file])
    t.start()
    return t

# If you want to run this script directly instead of importing it
# write your code here
if __name__ == "__main__":
    r = redis.Redis()
    rl = RedisLogger(r)
    rl.start('test.log', ['sai2::examples::current_ee_pos', 'sai2::examples::kp_pos'], logger_period=1e-3)
    time.sleep(5.5)
    rl.stop()