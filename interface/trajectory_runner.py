import redis
import time
import json
from threading import Thread


class TrajectoryRunner(object):
    def __init__(self, redis_client, primitive_key, posori_controller_value, position_key, velocity_key):
        self.redis_client = redis_client
        self.running = False
        self.thread = None

        # used to set controller -> posori when we start
        self.primitive_key = primitive_key
        self.posori_controller_value = posori_controller_value
        self.position_key = position_key
        self.velocity_key = velocity_key

    def _trajectory_run_loop(self):
        segment = 0

        # set controller to posori
        self.redis_client.set(self.primitive_key, self.posori_controller_value)

        while self.running and segment < len(self.times):
            # issue desired pos & vel to redis
            self.redis_client.mset({
                self.position_key: json.dumps(self.positions[:, segment].tolist()),
                self.velocity_key: json.dumps(self.velocities[:, segment].tolist())
            })

            # wait for next segment
            # trust that time is accurate enough
            # XXX: there should probably be some sort of control for this
            segment += 1
            time.sleep(self.step_time)

        # we're done
        self.running = False


    def start(self, times, positions, velocities, step_time):
        if self.running:
            return False

        self.times = times
        self.positions = positions
        self.velocities = velocities
        self.step_time = step_time

        # start worker thread
        self.running = True
        self.thread = Thread(target=self._trajectory_run_loop, daemon=True) # XXX: may not clean up correctly 
        self.thread.start()
        return True

    def stop(self):
        self.running = False
        if self.thread and self.thread.is_alive():
            self.thread.join()
