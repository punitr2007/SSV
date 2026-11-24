/**
 * @file camera_rig.cpp
 * @brief Implementation of Dynamic Chase Camera Rig
 */

#include "camera_rig.h"
#include <cmath>
#include <algorithm>

namespace SSV {

using Math::Vector3;
using Math::Matrix4x4;
using Math::Quaternion;
using Math::DEG_TO_RAD;

CameraRig::CameraRig()
    : CameraRig(CameraConfig{}) {
}

CameraRig::CameraRig(const CameraConfig& config)
    : m_config(config), m_currentFovDeg(config.baseFovDeg) {
}

void CameraRig::Reset(const Vector3& targetPos, const Quaternion& targetRot) {
    Vector3 forward = targetRot.Forward();
    m_currentYaw = std::atan2(forward.x, forward.z);

    Vector3 offset = forward * (-m_config.distance) + Vector3{0.0f, m_config.height, 0.0f};
    m_currentPos = targetPos + offset;
    m_lookTarget = targetPos + Vector3{0.0f, m_config.lookAtHeight, 0.0f} + forward * 2.0f;
    m_currentFovDeg = m_config.baseFovDeg;
    m_shakeTimer = 0.0f;
    m_shakeOffset = {0.0f, 0.0f, 0.0f};
}

void CameraRig::Update(const Vector3& targetPos, const Quaternion& targetRot, float speedMph, float dt) {
    if (dt <= 0.0f) return;

    Vector3 targetForward = targetRot.Forward();

    // 1. Target yaw with smooth angular interpolation (handles wrap-around)
    float desiredYaw = std::atan2(targetForward.x, targetForward.z);
    float diffYaw = desiredYaw - m_currentYaw;
    while (diffYaw > Math::PI)  diffYaw -= 2.0f * Math::PI;
    while (diffYaw < -Math::PI) diffYaw += 2.0f * Math::PI;

    // Smooth yaw tracking (allows car to drift sideways relative to camera)
    float yawLerp = std::min(1.0f, dt * m_config.rotationSpring);
    m_currentYaw += diffYaw * yawLerp;

    // 2. Compute camera ideal position using smoothed yaw direction
    Vector3 camDir = {
        std::sin(m_currentYaw),
        0.0f,
        std::cos(m_currentYaw)
    };

    // Dynamic distance stretch at high speeds
    float speedRatio = std::clamp(speedMph / 200.0f, 0.0f, 1.0f);
    float dynamicDistance = m_config.distance + speedRatio * 0.8f;
    float dynamicHeight = m_config.height + speedRatio * 0.25f;

    Vector3 idealPos = targetPos - camDir * dynamicDistance + Vector3{0.0f, dynamicHeight, 0.0f};

    // Smooth position interpolation
    float posLerp = std::min(1.0f, dt * m_config.positionSpring);
    m_currentPos += (idealPos - m_currentPos) * posLerp;

    // 3. Look target: car position + hood lookahead
    Vector3 idealLookTarget = targetPos + Vector3{0.0f, m_config.lookAtHeight, 0.0f} + targetForward * (1.5f + speedRatio * 2.0f);
    m_lookTarget += (idealLookTarget - m_lookTarget) * std::min(1.0f, dt * (m_config.positionSpring * 1.5f));

    // 4. Dynamic Field of View
    float targetFov = m_config.baseFovDeg + speedRatio * (m_config.maxFovDeg - m_config.baseFovDeg);
    m_currentFovDeg += (targetFov - m_currentFovDeg) * std::min(1.0f, dt * 6.0f);

    // 5. High-speed camera vibration
    if (speedMph > 80.0f) {
        m_shakeTimer += dt * 35.0f;
        float intensity = ((speedMph - 80.0f) / 120.0f) * 0.035f;
        m_shakeOffset = {
            std::sin(m_shakeTimer * 1.3f) * intensity,
            std::cos(m_shakeTimer * 1.7f) * intensity,
            0.0f
        };
    } else {
        m_shakeOffset = {0.0f, 0.0f, 0.0f};
    }
}

Matrix4x4 CameraRig::GetViewMatrix() const {
    Vector3 eye = m_currentPos + m_shakeOffset;
    Vector3 target = m_lookTarget;
    Vector3 up = {0.0f, 1.0f, 0.0f};
    return Matrix4x4::LookAtLH(eye, target, up);
}

Matrix4x4 CameraRig::GetProjectionMatrix(float aspectRatio) const {
    float fovRad = m_currentFovDeg * DEG_TO_RAD;
    return Matrix4x4::PerspectiveFovLH(fovRad, aspectRatio, m_config.zNear, m_config.zFar);
}

} // namespace SSV
