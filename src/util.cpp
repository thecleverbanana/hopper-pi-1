// util.cpp
#include <cmath>
#include "util.h"

// Helper to compute quaternion conjugate (inverse for unit quat)
void conjugate(float qi, float qj, float qk, float qr, float &ci, float &cj, float &ck, float &cr) {
    ci = -qi;
    cj = -qj;
    ck = -qk;
    cr = qr;
}

// Quaternion multiply: q1 * q2
void quat_multiply(float qi1, float qj1, float qk1, float qr1,
                   float qi2, float qj2, float qk2, float qr2,
                   float &qio, float &qjo, float &qko, float &qro) {
    qio = qr1*qi2 + qi1*qr2 + qj1*qk2 - qk1*qj2;
    qjo = qr1*qj2 - qi1*qk2 + qj1*qr2 + qk1*qi2;
    qko = qr1*qk2 + qi1*qj2 - qj1*qi2 + qk1*qr2;
    qro = qr1*qr2 - qi1*qi2 - qj1*qj2 - qk1*qk2;
}

// Rotate vector (vx,vy,vz) by quaternion (qi,qj,qk,qr)
void rotate_vector(float vx, float vy, float vz,
                   float qi, float qj, float qk, float qr,
                   float &rx, float &ry, float &rz) {
    float ci, cj, ck, cr;
    conjugate(qi, qj, qk, qr, ci, cj, ck, cr);

    // v as quat: (0, vx, vy, vz)
    float tmpi, tmpj, tmpk, tmpr;
    quat_multiply(qi, qj, qk, qr, 0, vx, vy, vz, tmpi, tmpj, tmpk, tmpr);

    float outi, outj, outk, outr;
    quat_multiply(tmpi, tmpj, tmpk, tmpr, ci, cj, ck, cr, outi, outj, outk, outr);

    rx = outi;
    ry = outj;
    rz = outk;
}

void quat_to_euler(float qi, float qj, float qk, float qr,
                   float &roll, float &pitch, float &yaw)
{
    // roll (x-axis rotation)
    float sinr_cosp = 2.0f * (qr * qi + qj * qk);
    float cosr_cosp = 1.0f - 2.0f * (qi * qi + qj * qj);
    roll = std::atan2(sinr_cosp, cosr_cosp);

    // pitch (y-axis rotation)
    float sinp = 2.0f * (qr * qj - qk * qi);
    if (std::fabs(sinp) >= 1.0f)
        pitch = std::copysign(M_PI / 2.0f, sinp); // use 90° if out of range
    else
        pitch = std::asin(sinp);

    // yaw (z-axis rotation)
    float siny_cosp = 2.0f * (qr * qk + qi * qj);
    float cosy_cosp = 1.0f - 2.0f * (qj * qj + qk * qk);
    yaw = std::atan2(siny_cosp, cosy_cosp);
}
