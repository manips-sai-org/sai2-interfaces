import select
import threading 
import time
import linuxfd


class PeriodicTimer(object):
    '''
    Uses linuxfd to run a function periodically. 
    
    Based on work here: https://abelbeck.wordpress.com/2014/01/24/more-on-linuxfd-an-example/
    '''

    def __init__(self, period, func, func_args=[], func_kwargs={}, new_thread_for_task=False, epoll_timeout=0.002):
        self.period = period 
        self.new_thread_for_task = new_thread_for_task

        # save func info for callback
        self.func = func 
        self.func_args = func_args 
        self.func_kwargs = func_kwargs

        # thread needed to trigger func when it's time to run
        self.shutdown_event = threading.Event()
        self.thread = threading.Thread(target=self._epoll_thread)

        # get timerfd & epollfd fds
        self.tfd = linuxfd.timerfd(rtc=True, nonBlocking=True)
        self.tfd.settime(self.period, self.period)

        self.epoll = select.epoll()
        self.epoll.register(self.tfd.fileno(), select.EPOLLIN)
        self.epoll_timeout = epoll_timeout

    def start(self):
        self.thread.start()

    def stop(self):
        self.shutdown_event.set()
        self.thread.join()
        
    def _epoll_thread(self):
        '''
        Normally, you'd set timeout=-1, but this will block indefinitely.
        We want to wake up occasionally so the thread can exit pretty close to when
        a user in a different thread invokes stop(). We also want to wake up
        when we get a single event so we can invoke our callback.
        '''

        while not self.shutdown_event.is_set():
            events = self.epoll.poll(timeout=self.epoll_timeout, maxevents=1)
            for fd, event in events:
                if fd == self.tfd.fileno() and event & select.EPOLLIN:
                    self.tfd.read()
                    if self.new_thread_for_task:
                        threading.Thread(target=self.func, args=self.func_args, kwargs=self.func_kwargs).start()
                    else:
                        self.func(*self.func_args, **self.func_kwargs)

    def __enter__(self):
        self.start()
        return self
    
    def __exit__(self, exc_type, exc_value, traceback):
        self.stop()

if __name__ == "__main__":
    '''
    To benchmark the timer, we collect all the times that we hit a custom callback.
    We then run an adjacent difference to determine the time taken between invocations.
    We can then determine the mean & std. dev between invocations, and verify that our
    callbacks are being invoked within a reasonable limit.
    '''
    import numpy as np

    TIMER_TEST_PERIOD = 10 # sec
    
    def print_metrics(times_list):
        times_list.sort()
        durations = np.diff(times_list)
        percentiles = [0, 25, 50, 75, 100]
        print("Iterations: {}".format(len(durations)))
        print("{}: {}".format(percentiles, np.percentile(durations, percentiles) ))
        print("Mean: {} | Std.dev: {}".format(np.mean(durations), np.std(durations)))

    def callback(times_list):
        times_list.append(time.monotonic())
        time.sleep(1)

    periodic_timer_list = []
    with PeriodicTimer(1e-3, callback, func_args=[periodic_timer_list]) as t:
        time.sleep(TIMER_TEST_PERIOD)

    print_metrics(periodic_timer_list)
