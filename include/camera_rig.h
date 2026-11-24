#pragma once
/**
 * @file camera_rig.h
 * @brief Dynamic Third-Person Chase Camera Rig for Split/Second: Velocity
 *
 * Implements smooth spring-damped tracking, laggy yaw drift following,
 * speed-dependent dynamic Field-of-View (FOV), and subtle high-speed camera shake.
 */

#include "math_types.h"
#include <algorithm>
#include <cmath>

namespace SSV {

using Math::Vector3;
using Math::Matrix4x4;
using Math::Quaternion;

struct CameraConfig {
    float distance = 5.8f;          // Metres behind car
    float height = 2.1f;            // Metres above car
    float lookAtHeight = 0.95f;     // Metres above car center
    float baseFovDeg = 62.0f;       // Idle FOV in degrees
    float maxFovDeg = 78.0f;        // Top speed FOV
    float positionSpring = 12.0f;   // Position damping rate
    float rotationSpring = 7.5f;    // Yaw damping rate
    float zNear = 0.2f;
    float zFar = 1000.0f;
};

class CameraRig {
public:
    CameraRig();
    explicit CameraRig(const CameraConfig& config);

    void Reset(const Vector3& targetPos, const Quaternion& targetRot);
    void Update(const Vector3& targetPos, const Quaternion& targetRot, float speedMph, float dt);

    Matrix4x4 GetViewMatrix() const;
    Matrix4x4 GetProjectionMatrix(float aspectRatio) const;

    const Vector3& GetPosition() const { return m_currentPos; }
    const Vector3& GetTarget() const { return m_lookTarget; }
    float GetCurrentFov() const { return m_currentFovDeg; }

private:
    CameraConfig m_config;

    Vector3 m_currentPos{0.0f, 2.5f, -6.0f};
    Vector3 m_lookTarget{0.0f, 1.0f, 0.0f};
    float m_currentYaw = 0.0f;
    float m_currentFovDeg = 62.0f;
    float m_shakeTimer = 0.0f;
    Vector3 m_shakeOffset{0.0f, 0.0f, 0.0f};
};

} // namespace SSV
