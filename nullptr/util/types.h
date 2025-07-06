#pragma once

static float pi_f = 3.141592;
static double pi_d = 3.141592;

struct Vector3 {
    float x = 0, y = 0, z = 0;

    Vector3() = default;
    Vector3(float X, float Y, float Z) : x(X), y(Y), z(Z) {}

    bool IsZero(float epsilon = 0.001f) const {
        return (fabsf(x) < epsilon) &&
            (fabsf(y) < epsilon) &&
            (fabsf(z) < epsilon);
    }

    void Normalize() {
        // Yaw
        while (y > 180.0f) y -= 360.0f;
        while (y < -180.0f) y += 360.0f;

        // Clamp pitch
        if (x > 89.0f) x = 89.0f;
        if (x < -89.0f) x = -89.0f;

        z = 0;
    }
};

inline Vector3 operator-(const Vector3& a, const Vector3& b) {
    return { a.x - b.x, a.y - b.y, a.z - b.z };
}
inline Vector3 operator+(const Vector3& a, const Vector3& b) {
    return { a.x + b.x, a.y + b.y, a.z + b.z };
}
inline Vector3 operator/(const Vector3& a, float scalar) {
    return { a.x / scalar, a.y / scalar, a.z / scalar };
}
inline Vector3 operator*(const Vector3& a, float scalar) {
    return { a.x * scalar, a.y * scalar, a.z * scalar };
}