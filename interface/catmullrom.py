import numpy as np


def compute_catmullrom_spline_trajectory(tf, P, t_step, alpha=0.5):
    '''
        P: np.array in the form of [P0, P1, ... ]. m x n, m = dim, n = # points
        alpha: float. 0.5 = centripetal
    '''
    # compute time for each control point
    m,n = np.shape(P)
    dP = np.diff(P)
    norm_dP = np.linalg.norm(dP, axis=0)
    
    t_nominal = np.zeros(n)
    for i in range(n - 1):
        t_nominal[i + 1] = t_nominal[i] + (norm_dP[i] ** alpha)
    
    # scale time by requested duration
    t = tf * (t_nominal / t_nominal[-1])
    
    # compute Catmull-Rom velocity constraints
    # Interior velocity constraint is (P[i + 1] - P[i - 1]) / (t[i + 1] - t[i - 1])
    # Start/end velocities are zero
    # note: technically not "velocity" - it's across two timesteps
    vel = np.zeros(np.shape(P))
    for i in range(1, n - 1):
        vel[:, i] = (P[:, i + 1] - P[:, i - 1]) / (t[i + 1] - t[i - 1])
        
    # compute coefficients for each pair of points
    coeff = np.zeros((m, 4, n - 1))
    for i in range(n - 1):
        '''
        Given two points P[i] and P[i+1] on timesteps t[i] and t[i+1],
        we want to find some f(t) = a * t^3 + b * t^2 + c * t + d,
        where P[i], P[i+1], a, b, c, d in R^m and t in R.
        
        A Catmull-Rom spline imposes the following constraints:
        f(t[i]) = P[i]
        f(t[i+1]) = P[i+1]
        f'(t[i]) = (P[i + 1] - P[i - 1]) / (t[i + 1] - t[i - 1])
        f'(t[i+1]) = (P[i + 2] - P[i]) / (t[i + 2] - t[i])
        
        If we expand this into matrix form, we get:
        [
                t[i]^3         t[i]^2       t[i]    1
              3 * t[i]^2      2 * t[i]       1      0
              t[i + 1]^3     t[i + 1]^2   t[i + 1]  1
            3 * t[i + 1]^2  2 * t[i + 1]     1      0
        ]
        [ a, b, c, d ]' = 
        [
            P[i]
            P[i+1]
            (P[i + 1] - P[i - 1]) / (t[i + 1] - t[i - 1])
            (P[i + 2] - P[i]) / (t[i + 2] - t[i])
        ]
        
        This is now in Ax = b form. However, this formulation is only true when
        P[i] is scalar valued. We extend to this m dimensions by multiplying each element in 
        A by the identity matrix in dimension m. This therefore yields the below implementation
        for A.
        
        However, this means that our coefficient vector, x, is a vector of 4*m. This is easy to fix:
        we simply take the first m elements, call it 'a', the second to 'b', etc. This ordering is called
        "Fortran" ordering, so we call reshape with order='F'. Note that this ordering is default in MATLAB.
        
        We save these coefficients into a 3D matrix with shape m by 4 by n - 1.
        Intuitively, coeffs[:, :, i] gets the spline coefficients for P[i], P[i+1] 
        with form [a, b, c, d].
        '''
        
        A = np.vstack((
            np.hstack(((t[i] ** 3) * np.eye(m), (t[i] ** 2) * np.eye(m), t[i] * np.eye(m), np.eye(m))),
            np.hstack((3 * (t[i] ** 2) * np.eye(m), 2 * t[i] * np.eye(m), np.eye(m), np.zeros((m, m)))),
            np.hstack(((t[i + 1] ** 3) * np.eye(m), (t[i + 1] ** 2) * np.eye(m), t[i + 1] * np.eye(m), np.eye(m))),
            np.hstack((3 * (t[i + 1] ** 2) * np.eye(m), 2 * t[i + 1] * np.eye(m), np.eye(m), np.zeros((m, m)))),
        ))
        
        b = np.vstack((
            np.reshape(P[:, i], (m, 1)),
            np.reshape(vel[:, i], (m, 1)),
            np.reshape(P[:, i + 1], (m, 1)),
            np.reshape(vel[:, i + 1], (m, 1))
        ))
        
        x = np.linalg.pinv(A) @ b
        coeff[:, :, i] = np.reshape(x, (m, 4), order='F')
    
    # determine trajectory
    t_traj = np.arange(0, tf, t_step)
    P_traj = np.zeros((m, len(t_traj)))
    V_traj = np.zeros((m, len(t_traj)))
    A_traj = np.zeros((m, len(t_traj)))

    current_segment = 0
    for i in range(len(t_traj)):
        t_i_traj = t_traj[i]

        # t contains start of segments
        if t_i_traj > t[current_segment + 1]:
            current_segment += 1

        a = coeff[:, 0, current_segment]
        b = coeff[:, 1, current_segment]
        c = coeff[:, 2, current_segment]
        d = coeff[:, 3, current_segment]

        # at^3 + bt^2 + ct + d
        P_traj[:, i] =  a * (t_i_traj ** 3) + b * (t_i_traj ** 2) + (c * t_i_traj) + d
        V_traj[:, i] = (3 * a * (t_i_traj ** 2)) + (2 * b * t_i_traj) + c
        A_traj[:, i] = (6 * a * t_i_traj) + 2 * b

    return (t_traj, P_traj, V_traj, A_traj)
    