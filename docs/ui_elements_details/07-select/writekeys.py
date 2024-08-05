import redis 

r = redis.Redis()
r.set('sai2::interfaces::tutorial::mode', 'joint')
r.set('sai2::interfaces::tutorial::q', '[1,2,3]')
r.set('sai2::interfaces::tutorial::ee_pos', '[0,1,1]')
r.set('sai2::interfaces::tutorial::ee_ori', '[45,60,15]')
