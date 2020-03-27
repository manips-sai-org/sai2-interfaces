import redis 
import sys
import numpy as np

if len(sys.argv) != 2:
    print('usage: python3 {} <# of joints>'.format(sys.argv[0]))
    exit(0)

try:
    num_joints = int(sys.argv[1])
except:
    print('usage: python3 {} <# of joints>'.format(sys.argv[0]))
    exit(0)

JOINT_KEY = 'sai2::interfaces::tutorial::q'
KP_GAIN_KEY = 'sai2::interfaces::tutorial::joint_kp'
KV_GAIN_KEY = 'sai2::interfaces::tutorial::joint_kv'

initial_joint_values = 2 * np.pi * np.random.rand(num_joints)

r = redis.Redis()
r.set(JOINT_KEY, str(initial_joint_values.tolist()))
r.set(KP_GAIN_KEY, '100')
r.set(KV_GAIN_KEY, '2')

print('{} set to {}'.format(JOINT_KEY, r.get(JOINT_KEY)))
print('{} set to {}'.format(KP_GAIN_KEY, r.get(KP_GAIN_KEY)))
print('{} set to {}'.format(KV_GAIN_KEY, r.get(KV_GAIN_KEY)))