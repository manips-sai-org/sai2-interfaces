import redis
import time
import json
import util
from threading import Thread
import periodic_timer


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
        self.periodic_timer = periodic_timer.PeriodicTimer(
            logger_period, 
            self._logger_loop, 
            func_args=[context],
            new_thread_for_task=False
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
        

# If you want to run this script directly instead of importing it
# write your code here
if __name__ == "__main__":
    r = redis.Redis()
    rl = RedisLogger(r)
    rl.start('test.log', ['sai2::examples::current_ee_pos', 'sai2::examples::kp_pos'], logger_period=1)
    time.sleep(5.5)
    rl.stop()
    