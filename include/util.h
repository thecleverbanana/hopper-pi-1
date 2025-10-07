// util.h
#ifndef UTIL_H
#define UTIL_H

// Helper to compute quaternion conjugate (inverse for unit quat)
void conjugate(float qi, float qj, float qk, float qr, float &ci, float &cj, float &ck, float &cr);

// Quaternion multiply: q1 * q2
void quat_multiply(float qi1, float qj1, float qk1, float qr1,
                   float qi2, float qj2, float qk2, float qr2,
                   float &qio, float &qjo, float &qko, float &qro);

// Rotate vector (vx,vy,vz) by quaternion (qi,qj,qk,qr)
void rotate_vector(float vx, float vy, float vz,
                   float qi, float qj, float qk, float qr,
                   float &rx, float &ry, float &rz);

void quat_to_euler(float qi, float qj, float qk, float qr,
                   float &roll, float &pitch, float &yaw);

#endif // UTIL_H