import redis 
import sys
import numpy as np
import time
import json

def rot_x(theta):
    c_theta = np.cos(theta)
    s_theta = np.sin(theta)
    return np.array([[1, 0, 0], [0, c_theta, -s_theta], [0, s_theta, c_theta]])

def rot_y(theta):
    c_theta = np.cos(theta)
    s_theta = np.sin(theta)
    return np.array([[c_theta, 0, s_theta], [0, 1, 0], [-s_theta, 0, c_theta]])

def rot_z(theta):
    c_theta = np.cos(theta)
    s_theta = np.sin(theta)
    return np.array([[c_theta, -s_theta, 0], [s_theta, c_theta, 0], [0, 0, 1]])
    
def zyx_euler_angles_to_mat(alpha, beta, gamma):
    return rot_z(alpha) @ rot_y(beta) @ rot_x(gamma)

if len(sys.argv) != 2:
    print('usage: python3 {} <# of joints>'.format(sys.argv[0]))
    exit(0)

try:
    num_joints = int(sys.argv[1])
except:
    print('usage: python3 {} <# of joints>'.format(sys.argv[0]))
    exit(0)

# mode key
MODE_KEY = 'sai2::interfaces::tutorial::mode'

# joint task keys
JOINT_KEY = 'sai2::interfaces::tutorial::q'
KP_GAIN_KEY = 'sai2::interfaces::tutorial::joint_kp'
KV_GAIN_KEY = 'sai2::interfaces::tutorial::joint_kv'
JOINT_USE_INTERPOLATION_KEY = 'sai2::interfaces::tutorial::joint_use_interpolation'
JOINT_INTERPOLATION_MAX_VEL_KEY = 'sai2::interfaces::tutorial::joint_interpolation_max_vel'
JOINT_INTERPOLATION_MAX_ACCEL_KEY = 'sai2::interfaces::tutorial::joint_interpolation_max_accel'
JOINT_INTERPOLATION_MAX_JERK_KEY = 'sai2::interfaces::tutorial::joint_interpolation_max_jerk'
JOINT_DYNAMIC_DECOUPLING_KEY = 'sai2::interfaces::tutorial::joint_dynamic_decoupling'

# posori task keys
EE_POS_KEY = 'sai2::interfaces::tutorial::ee_pos'
EE_ORI_KEY = 'sai2::interfaces::tutorial::ee_ori'
EE_POS_KP_KEY = 'sai2::interfaces::tutorial::ee_pos_kp'
EE_POS_KV_KEY = 'sai2::interfaces::tutorial::ee_pos_kv'
EE_ORI_KP_KEY = 'sai2::interfaces::tutorial::ee_ori_kp'
EE_ORI_KV_KEY = 'sai2::interfaces::tutorial::ee_ori_kv'
EE_ROTMAT_KEY = 'sai2::interfaces::tutorial::ee_rotmat'
POSORI_USE_INTERPOLATION_KEY = 'sai2::interfaces::tutorial::posori_use_interpolation'
POSORI_INTERPOLATION_MAX_LINEAR_VEL_KEY = 'sai2::interfaces::tutorial::posori_interpolation_max_linear_vel'
POSORI_INTERPOLATION_MAX_LINEAR_ACCEL_KEY = 'sai2::interfaces::tutorial::posori_interpolation_max_linear_accel'
POSORI_INTERPOLATION_MAX_LINEAR_JERK_KEY = 'sai2::interfaces::tutorial::posori_interpolation_max_linear_jerk'
POSORI_INTERPOLATION_MAX_ANGULAR_VEL_KEY = 'sai2::interfaces::tutorial::posori_interpolation_max_angular_vel'
POSORI_INTERPOLATION_MAX_ANGULAR_ACCEL_KEY = 'sai2::interfaces::tutorial::posori_interpolation_max_angular_accel'
POSORI_INTERPOLATION_MAX_ANGULAR_JERK_KEY = 'sai2::interfaces::tutorial::posori_interpolation_max_angular_jerk'
POSORI_DYNAMIC_DECOUPLING_KEY = 'sai2::interfaces::tutorial::posori_dynamic_decoupling'
POSORI_USE_VELOCITY_SATURATION_KEY = 'sai2::interfaces::tutorial::posori_use_velocity_saturation'
POSORI_LINEAR_VELOCITY_SATRURATION_KEY = 'sai2::interfaces::tutorial::posori_linear_velocity_saturation'
POSORI_ANGULAR_VELOCITY_SATRURATION_KEY = 'sai2::interfaces::tutorial::posori_angular_velocity_saturation'

initial_joint_values = 2 * np.pi * np.random.rand(num_joints)
initial_pos_values = 3 * np.random.rand(3)
initial_ori_values = 2 * np.pi * np.random.rand(3)

r = redis.Redis()

# mode
r.set(MODE_KEY, 'joint')

# joint task keys
r.set(JOINT_KEY, str(initial_joint_values.tolist()))
r.set(KP_GAIN_KEY, '100')
r.set(KV_GAIN_KEY, '2')
r.set(JOINT_USE_INTERPOLATION_KEY, '0')
r.set(JOINT_INTERPOLATION_MAX_VEL_KEY, str(np.pi / 3))
r.set(JOINT_INTERPOLATION_MAX_ACCEL_KEY, str(np.pi / 3))
r.set(JOINT_INTERPOLATION_MAX_JERK_KEY, str(np.pi / 3))
r.set(JOINT_DYNAMIC_DECOUPLING_KEY, 'full')

# posori task keys
r.set(EE_POS_KEY, str(initial_pos_values.tolist()))
r.set(EE_ORI_KEY, str(initial_ori_values.tolist()))
r.set(EE_POS_KP_KEY, '100')
r.set(EE_POS_KV_KEY, '2')
r.set(EE_ORI_KP_KEY, '75')
r.set(EE_ORI_KV_KEY, '3')
r.set(POSORI_USE_INTERPOLATION_KEY, '0')
r.set(POSORI_INTERPOLATION_MAX_LINEAR_VEL_KEY, str(np.pi / 3))
r.set(POSORI_INTERPOLATION_MAX_LINEAR_ACCEL_KEY, str(np.pi / 3))
r.set(POSORI_INTERPOLATION_MAX_LINEAR_JERK_KEY, str(np.pi / 3))
r.set(POSORI_INTERPOLATION_MAX_ANGULAR_VEL_KEY, str(np.pi / 3))
r.set(POSORI_INTERPOLATION_MAX_ANGULAR_ACCEL_KEY, str(np.pi / 3))
r.set(POSORI_INTERPOLATION_MAX_ANGULAR_JERK_KEY, str(np.pi / 3))
r.set(POSORI_USE_VELOCITY_SATURATION_KEY, '0')
r.set(POSORI_LINEAR_VELOCITY_SATRURATION_KEY, str(np.pi / 3))
r.set(POSORI_ANGULAR_VELOCITY_SATRURATION_KEY, str(np.pi / 3))
r.set(POSORI_DYNAMIC_DECOUPLING_KEY, 'full')

# joint task keys
print('{} set to {}'.format(MODE_KEY, r.get(MODE_KEY)))
print('{} set to {}'.format(JOINT_KEY, r.get(JOINT_KEY)))
print('{} set to {}'.format(KP_GAIN_KEY, r.get(KP_GAIN_KEY)))
print('{} set to {}'.format(KV_GAIN_KEY, r.get(KV_GAIN_KEY)))
print('{} set to {}'.format(JOINT_USE_INTERPOLATION_KEY, r.get(JOINT_USE_INTERPOLATION_KEY)))
print('{} set to {}'.format(JOINT_INTERPOLATION_MAX_VEL_KEY, r.get(JOINT_INTERPOLATION_MAX_VEL_KEY)))
print('{} set to {}'.format(JOINT_INTERPOLATION_MAX_ACCEL_KEY, r.get(JOINT_INTERPOLATION_MAX_ACCEL_KEY)))
print('{} set to {}'.format(JOINT_INTERPOLATION_MAX_JERK_KEY, r.get(JOINT_INTERPOLATION_MAX_JERK_KEY)))
print('{} set to {}'.format(JOINT_DYNAMIC_DECOUPLING_KEY, r.get(JOINT_DYNAMIC_DECOUPLING_KEY)))

# posori task keys
print('{} set to {}'.format(EE_POS_KEY, r.get(EE_POS_KEY)))
print('{} set to {}'.format(EE_ORI_KEY, r.get(EE_ORI_KEY)))
print('{} set to {}'.format(EE_POS_KP_KEY, r.get(EE_POS_KP_KEY)))
print('{} set to {}'.format(EE_POS_KV_KEY, r.get(EE_POS_KV_KEY)))
print('{} set to {}'.format(EE_ORI_KP_KEY, r.get(EE_ORI_KP_KEY)))
print('{} set to {}'.format(EE_ORI_KV_KEY, r.get(EE_ORI_KV_KEY)))
print('{} set to {}'.format(EE_ROTMAT_KEY, r.get(EE_ROTMAT_KEY)))
print('{} set to {}'.format(POSORI_USE_INTERPOLATION_KEY, r.get(POSORI_USE_INTERPOLATION_KEY)))
print('{} set to {}'.format(POSORI_INTERPOLATION_MAX_LINEAR_VEL_KEY, r.get(POSORI_INTERPOLATION_MAX_LINEAR_VEL_KEY)))
print('{} set to {}'.format(POSORI_INTERPOLATION_MAX_LINEAR_ACCEL_KEY, r.get(POSORI_INTERPOLATION_MAX_LINEAR_ACCEL_KEY)))
print('{} set to {}'.format(POSORI_INTERPOLATION_MAX_LINEAR_JERK_KEY, r.get(POSORI_INTERPOLATION_MAX_LINEAR_JERK_KEY)))
print('{} set to {}'.format(POSORI_INTERPOLATION_MAX_ANGULAR_VEL_KEY, r.get(POSORI_INTERPOLATION_MAX_ANGULAR_VEL_KEY)))
print('{} set to {}'.format(POSORI_INTERPOLATION_MAX_ANGULAR_ACCEL_KEY, r.get(POSORI_INTERPOLATION_MAX_ANGULAR_ACCEL_KEY)))
print('{} set to {}'.format(POSORI_INTERPOLATION_MAX_ANGULAR_JERK_KEY, r.get(POSORI_INTERPOLATION_MAX_ANGULAR_JERK_KEY)))
print('{} set to {}'.format(POSORI_DYNAMIC_DECOUPLING_KEY, r.get(POSORI_DYNAMIC_DECOUPLING_KEY)))
print('{} set to {}'.format(POSORI_USE_VELOCITY_SATURATION_KEY, r.get(POSORI_USE_VELOCITY_SATURATION_KEY)))
print('{} set to {}'.format(POSORI_LINEAR_VELOCITY_SATRURATION_KEY, r.get(POSORI_LINEAR_VELOCITY_SATRURATION_KEY)))
print('{} set to {}'.format(POSORI_ANGULAR_VELOCITY_SATRURATION_KEY, r.get(POSORI_ANGULAR_VELOCITY_SATRURATION_KEY)))

while True:
    raw_ori = r.get(EE_ORI_KEY)
    gamma, beta, alpha = json.loads(raw_ori)
    rmat = zyx_euler_angles_to_mat(alpha, beta, gamma)
    r.set(EE_ROTMAT_KEY, str(rmat.tolist()))
    time.sleep(0.5)
