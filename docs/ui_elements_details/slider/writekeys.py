import redis 

r = redis.Redis()
r.set('sai2::interfaces::tutorial::scalar_key', '5')
r.set('sai2::interfaces::tutorial::vector_key', '[3,4]')
