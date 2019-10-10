import threading 
import signal 
import time
import periodic_timer
import util


class RedisCache(object):
    def __init__(self, redis_client, refresh_rate=0.0333, keys=[], key_patterns=[]):
        self.refresh_rate = refresh_rate
        self.redis_client = redis_client
        self.refresh_keys = threading.Event()
        self.pipe_keys = keys 
        self.key_patterns = key_patterns
        self.key_cache = {}
        self.running = False

        ctx = { 'first_run': True }
        self.periodic_timer = periodic_timer.PeriodicTimer(
            period=refresh_rate,
            func=self._update_cache,
            func_args=[ctx]
        )

        if not keys and not key_patterns:
            self.key_patterns = ['*']


    def __getitem__(self, key):
        return self.key_cache.get(key)

    def __str__(self):
        return str(self.key_cache)

    def refresh(self):
        self.refresh_keys.set()

    def start(self):
        if self.running:
            return False
        
        self.running = True
        self.periodic_timer.start()
        return True

    def stop(self):
        self.running = False
        self.periodic_timer.stop()

    def _update_cache(self, ctx):
        pipe = self.redis_client.pipeline(transaction=True)
        first_run = ctx['first_run']

        if first_run or self.refresh_keys.is_set():
            self._key_list = []
            self._key_list += self.pipe_keys
            for pattern in self.key_patterns:
                raw_keys = self.redis_client.keys(pattern)
                self._key_list += [raw_key if raw_key else '' for raw_key in raw_keys]

            ctx['first_run'] = False
            self.refresh_keys.clear()

        # rebuild pipeline
        for key in self._key_list:
            pipe.get(key)

        key_values = pipe.execute()
        for key, value in zip(self._key_list, key_values):
            self.key_cache[key] = util.try_parse_json(value)

if __name__ == "__main__":
    import redis
    
    r = redis.Redis()
    rc = RedisCache(r, 0.1)
    rc.start()

    for _ in range(10):
        print(str(rc))
        time.sleep(1)

    rc.stop()
