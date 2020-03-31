import redis 

r = redis.Redis()
r.flushall()
r.set('sai2::redis::apples', 1)
r.set('sai2::redis::bananas', 2)
r.set('sai2::redis::oranges', 3)
