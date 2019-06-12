import redis
import time
import json
from threading import Thread


class RedisLogger(object):
    def __init__(self, redis_client):
        self.redis_client = redis_client
        self.thread = None
        self.running = False
        self.filename = ''
        self.logger_period = 0
        self.redis_keys = []
        self.log_start_time = None

    def _get_redis_key(self, key):
        redis_str = self.redis_client.get(key)
        try:
            return json.loads(redis_str)
        except:
            return redis_str


    def _logger_loop(self):
        with open(self.filename, 'w+') as f:
            f.write('Logger Frequency: {} sec\n'.format(self.logger_period))

            header = []
            header_written = False
            while self.running:
                # ensure first data point starts time 0
                if self.log_start_time is None:
                    self.log_start_time = time.time()
                    current_time = 0
                else:
                    current_time = time.time() - self.log_start_time

                # iterate through keys, write headers, and populate data
                values = []
                for key in self.redis_keys:
                    redis_val = self._get_redis_key(key)
                    if type(redis_val) == list:
                        values += redis_val
                        # if we haven't written the header to the file,
                        # expand the given key into key[len(key)]
                        if not header_written:
                            header.append(key + '[{}]'.format(len(redis_val)))

                    # just append regular strings
                    else:
                        values.append(redis_val)
                        if not header_written:
                            header.append(key) 

                # write header if we haven't already
                # we can't write header right away since we don't know if
                # keys are vector or scalar valued
                # matrices are NOT supported
                if not header_written:
                    f.write('Time\t' +  '\t'.join(str(h) for h in header) + '\n')
                    header_written = True

                # write keys
                f.write(str(current_time) + '\t' + '\t'.join(str(v) for v in values) + '\n')

                # wait for next cycle
                time.sleep(self.logger_period)

    def start(self, filename, redis_keys, logger_period=1):
        if self.running:
            return False

        self.filename = filename
        self.redis_keys = redis_keys
        self.logger_period = logger_period

        # start thread
        self.running = True
        self.thread = Thread(target=self._logger_loop, daemon=True) # XXX: may not clean up correctly 
        self.thread.start()
        return True

    def stop(self):
        self.running = False
        self.thread.join()
        

### TEST ###
if __name__ == "__main__":
    r = redis.Redis()
    rl = RedisLogger(r)
    rl.start('test.log', ['sai2::examples::current_ee_pos', 'sai2::sai2Interfaces::kp_pos'], logger_period=1)
    time.sleep(5)
    rl.stop()