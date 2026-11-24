/**
 * @file vehicle_physics.cpp
 * @brief Implementation of 4-Wheel Raycast Vehicle Physics and Handling Simulation
 *
 * Implements vehicle dynamics using exact parameters reverse-engineered from SplitSecond.exe:
 *  - Mass: 3000 lbs (1360 kg) (RVA: 0x00BC202C)
 *  - TopSpeed: 200 MPH (89.4 m/s) (RVA: 0x00BA62C0)
 *  - MaxSteerAngle: 35.0 deg (speed-attenuated) (RVA: 0x00C1A5A8)
 *  - SpringK: 100.0, SpringDamping: 100.0 (RVA: 0x00BC3C84)
 *  - SpringLength: 2.0 (RVA: 0x00BC4E78)
 *  - HardPoints Z: +/-4.0 (RVA: 0x00C2382C / 0x00BB3438)
 */

#include "vehicle_physics.h"
#include <algorithm>
#include <cmath>

namespace SSV {
namespace Physics {

using Math::Vector3;
using Math::Matrix4x4;
using Math::Quaternion;
using Math::DEG_TO_RAD;
using Math::RAD_TO_DEG;

VehicleSimulation::VehicleSimulation()
    : VehicleSimulation(VehicleConfig{}) {
}

VehicleSimulation::VehicleSimulation(const VehicleConfig& config)
    : m_config(config) {
    // Setup 4 wheels: FL, FR, RL, RR
    m_wheels[0].hardPoint = m_config.hardPointFL;
    m_wheels[0].isFront = true;
    m_wheels[0].isDriven = true;

    m_wheels[1].hardPoint = m_config.hardPointFR;
    m_wheels[1].isFront = true;
    m_wheels[1].isDriven = true;

    m_wheels[2].hardPoint = m_config.hardPointRL;
    m_wheels[2].isFront = false;
    m_wheels[2].isDriven = true;

    m_wheels[3].hardPoint = m_config.hardPointRR;
    m_wheels[3].isFront = false;
    m_wheels[3].isDriven = true;

    Reset();
}

void VehicleSimulation::SetRoadCollision(const std::vector<RoadTriangle>& triangles) {
    m_roadTriangles = triangles;
    for (auto& tri : m_roadTriangles) {
        tri.minX = std::min({tri.v0.x, tri.v1.x, tri.v2.x});
        tri.maxX = std::max({tri.v0.x, tri.v1.x, tri.v2.x});
        tri.minY = std::min({tri.v0.y, tri.v1.y, tri.v2.y});
        tri.maxY = std::max({tri.v0.y, tri.v1.y, tri.v2.y});
        tri.minZ = std::min({tri.v0.z, tri.v1.z, tri.v2.z});
        tri.maxZ = std::max({tri.v0.z, tri.v1.z, tri.v2.z});

        Vector3 edge1 = tri.v1 - tri.v0;
        Vector3 edge2 = tri.v2 - tri.v0;
        Vector3 cross = edge1.Cross(edge2);
        float len = cross.Length();
        if (len > 1e-6f) {
            Vector3 n = cross * (1.0f / len);
            if (n.y < 0.0f) {
                n = n * -1.0f; // Ensure road surface normal points upward
            }
            tri.normal = n;
        } else if (tri.normal.Dot(tri.normal) < 1e-4f) {
            tri.normal = {0.0f, 1.0f, 0.0f};
        }
    }
}

void VehicleSimulation::ClearRoadCollision() {
    m_roadTriangles.clear();
}

void VehicleSimulation::Reset(const Vector3& spawnPosition) {
    m_position = spawnPosition;
    m_velocity = Vector3{0.0f, 0.0f, 0.0f};
    m_orientation = Quaternion::Identity();
    m_angularVelocity = Vector3{0.0f, 0.0f, 0.0f};

    for (auto& w : m_wheels) {
        w.suspensionLength = m_config.suspensionRestLength;
        w.steeringAngleRad = 0.0f;
        w.rotationAngleRad = 0.0f;
        w.angularVelocity = 0.0f;
        w.inContact = false;
        w.normalForce = 0.0f;
    }

    m_engineRpm = 1000.0f;
    m_currentGear = 1;
    m_isDrifting = false;
    m_slipAngle = 0.0f;
}

float VehicleSimulation::GetSpeedMph() const {
    return m_velocity.Length() * 2.23694f;
}

float VehicleSimulation::GetSpeedKmh() const {
    return m_velocity.Length() * 3.6f;
}

Matrix4x4 VehicleSimulation::GetTransform() const {
    return Matrix4x4::TRS(m_position, m_orientation, {1.0f, 1.0f, 1.0f});
}

Matrix4x4 VehicleSimulation::GetWheelTransform(size_t wheelIndex) const {
    if (wheelIndex >= m_wheels.size()) return Matrix4x4::Identity();

    const Wheel& w = m_wheels[wheelIndex];
    // Local position offset from chassis origin: hardPoint + suspension extension down
    Vector3 localOffset = w.hardPoint;
    localOffset.y -= (w.suspensionLength - m_config.wheelRadius * 0.25f);

    // Wheel rotation: combine chassis orientation, steering yaw, and rolling pitch
    Quaternion steerRot = Quaternion::FromAxisAngle({0.0f, 1.0f, 0.0f}, w.steeringAngleRad);
    Quaternion rollRot = Quaternion::FromAxisAngle({1.0f, 0.0f, 0.0f}, w.rotationAngleRad);
    Quaternion wheelWorldRot = m_orientation * steerRot * rollRot;

    Vector3 worldPos = m_position + m_orientation.Rotate(localOffset);
    return Matrix4x4::TRS(worldPos, wheelWorldRot, {1.0f, 1.0f, 1.0f});
}

void VehicleSimulation::Step(const InputState& input, float dt) {
    if (dt <= 0.0f) return;

    // Substep simulation if dt is large (e.g., > 1/30 s) for stability
    constexpr float MAX_SUBSTEP = 1.0f / 60.0f;
    float remaining = dt;
    while (remaining > 1e-5f) {
        float stepDt = std::min(remaining, MAX_SUBSTEP);
        SimulateSuspensionAndTires(input, stepDt);
        remaining -= stepDt;
    }

    // Engine & Gearbox simulation
    float speedMph = GetSpeedMph();
    if (speedMph < 35.0f) m_currentGear = 1;
    else if (speedMph < 65.0f) m_currentGear = 2;
    else if (speedMph < 100.0f) m_currentGear = 3;
    else if (speedMph < 135.0f) m_currentGear = 4;
    else if (speedMph < 170.0f) m_currentGear = 5;
    else m_currentGear = 6;

    float minRpm = 1000.0f;
    float maxRpm = 8500.0f;
    float gearBaseSpeeds[] = {0.0f, 0.0f, 35.0f, 65.0f, 100.0f, 135.0f, 170.0f};
    float gearTopSpeeds[]  = {0.0f, 35.0f, 65.0f, 100.0f, 135.0f, 170.0f, 210.0f};
    float curBase = gearBaseSpeeds[m_currentGear];
    float curTop  = gearTopSpeeds[m_currentGear];
    float gearProgress = std::clamp((speedMph - curBase) / (curTop - curBase + 1e-4f), 0.0f, 1.0f);

    float targetRpm = minRpm + gearProgress * (maxRpm - minRpm);
    if (input.throttle > 0.05f && speedMph < 5.0f) {
        targetRpm = std::max(targetRpm, 4000.0f * input.throttle); // Throttle blip / revving
    }
    m_engineRpm += (targetRpm - m_engineRpm) * std::min(1.0f, dt * 15.0f);
}

void VehicleSimulation::SimulateSuspensionAndTires(const InputState& input, float dt) {
    Vector3 forward = m_orientation.Forward();
    Vector3 up      = m_orientation.Up();
    Vector3 right   = m_orientation.Right();

    float speedMph = GetSpeedMph();
    float forwardSpeed = m_velocity.Dot(forward);

    // 1. Speed-Sensitive Steering
    // At low speeds: 35 degrees. At 200 MPH: 10 degrees.
    float steerAttenuation = 1.0f - std::min(1.0f, speedMph / m_config.topSpeedMph) * (1.0f - (m_config.highSpeedSteerDeg / m_config.maxSteerAngleDeg));
    float targetSteerAngle = input.steer * (m_config.maxSteerAngleDeg * DEG_TO_RAD) * steerAttenuation;

    for (size_t i = 0; i < 4; ++i) {
        if (m_wheels[i].isFront) {
            // Smooth steering response
            m_wheels[i].steeringAngleRad += (targetSteerAngle - m_wheels[i].steeringAngleRad) * std::min(1.0f, dt * 18.0f);
        } else {
            m_wheels[i].steeringAngleRad = 0.0f;
        }
    }

    // 2. Slip angle and drift detection
    float latVel = m_velocity.Dot(right);
    m_slipAngle = std::atan2(latVel, std::abs(forwardSpeed) + 1.0f);

    // Drifting requires both significant lateral slip angle and forward motion:
    // Pure straight-line handbraking locks the rear tires without initiating a drift.
    // A drift is triggered when handbrake or aggressive steer combines with real lateral slip angle (> 0.15 rad / ~8.6 deg) at speed (> 15 MPH).
    bool hasLateralSlip = (std::abs(m_slipAngle) > 0.15f);
    bool sufficientSpeed = (speedMph > 15.0f);
    bool initiatingInput = input.handbrake || (std::abs(input.steer) > 0.15f);

    if (sufficientSpeed && hasLateralSlip && initiatingInput) {
        m_isDrifting = true;
    } else if (speedMph < 10.0f || (!input.handbrake && std::abs(m_slipAngle) < 0.10f)) {
        m_isDrifting = false;
    }

    // 3. Forces & Torques Accumulators
    Vector3 totalForce = {0.0f, -9.81f * m_config.mass, 0.0f}; // Gravity
    Vector3 totalTorque = {0.0f, 0.0f, 0.0f};

    Vector3 rayDown = m_orientation.Rotate({0.0f, -1.0f, 0.0f});
    float maxRayLength = m_config.suspensionRestLength + m_config.wheelRadius;

    int wheelsOnGround = 0;

    for (size_t i = 0; i < 4; ++i) {
        Wheel& w = m_wheels[i];

        // World position of suspension hardpoint
        Vector3 hardPointWorld = m_position + m_orientation.Rotate(w.hardPoint);

        // Raycast down against road collision mesh with fallback to y = 0
        float t = maxRayLength + 1.0f;
        bool hitGround = false;
        Vector3 hitNormal{0.0f, 1.0f, 0.0f};

        if (!m_roadTriangles.empty()) {
            const Vector3& orig = hardPointWorld;
            const Vector3& dir = rayDown;

            for (const auto& tri : m_roadTriangles) {
                // AABB early rejection
                if (orig.x < tri.minX - 1.0f && dir.x <= 0.0f) continue;
                if (orig.x > tri.maxX + 1.0f && dir.x >= 0.0f) continue;
                if (orig.z < tri.minZ - 1.0f && dir.z <= 0.0f) continue;
                if (orig.z > tri.maxZ + 1.0f && dir.z >= 0.0f) continue;

                // Möller–Trumbore ray-triangle intersection
                Vector3 edge1 = tri.v1 - tri.v0;
                Vector3 edge2 = tri.v2 - tri.v0;
                Vector3 pvec = dir.Cross(edge2);
                float det = edge1.Dot(pvec);

                if (std::abs(det) < 1e-7f) continue;
                float invDet = 1.0f / det;

                Vector3 tvec = orig - tri.v0;
                float u = tvec.Dot(pvec) * invDet;
                if (u < 0.0f || u > 1.0f) continue;

                Vector3 qvec = tvec.Cross(edge1);
                float v = dir.Dot(qvec) * invDet;
                if (v < 0.0f || u + v > 1.0f) continue;

                float hitT = edge2.Dot(qvec) * invDet;
                if (hitT > 0.0f && hitT <= maxRayLength && hitT < t) {
                    t = hitT;
                    hitGround = true;
                    hitNormal = tri.normal;
                }
            }
        }

        // Fallback to flat ground plane y = 0
        if (!hitGround && std::abs(rayDown.y) > 1e-4f) {
            float planeT = -hardPointWorld.y / rayDown.y;
            if (planeT > 0.0f && planeT <= maxRayLength) {
                t = planeT;
                hitGround = true;
                hitNormal = {0.0f, 1.0f, 0.0f};
            }
        }

        // Velocity of wheel attachment point
        Vector3 r = hardPointWorld - m_position;
        Vector3 pointVel = m_velocity + m_angularVelocity.Cross(r);

        if (hitGround) {
            // Guarantee contact normal opposes rayDown (pushes upward against chassis)
            if (hitNormal.Dot(rayDown) > 0.0f) {
                hitNormal = hitNormal * -1.0f;
            }

            w.inContact = true;
            wheelsOnGround++;
            w.contactPoint = hardPointWorld + rayDown * t;
            w.contactNormal = hitNormal;

            // Suspension spring compression
            w.suspensionLength = std::clamp(t - m_config.wheelRadius, 0.05f, m_config.suspensionRestLength);
            float compression = m_config.suspensionRestLength - w.suspensionLength;

            // Compression velocity: upward chassis motion extends spring, downward motion compresses it
            float compressionVel = pointVel.Dot(rayDown);

            // Spring + Damper force: F_s = k * x + c * v_compression
            float springForceMagnitude = m_config.springK * compression + m_config.springDamping * compressionVel;
            if (springForceMagnitude < 0.0f) springForceMagnitude = 0.0f;
            w.normalForce = springForceMagnitude;

            Vector3 springForce = hitNormal * springForceMagnitude;
            totalForce += springForce;
            totalTorque += r.Cross(springForce);

            // Wheel coordinate frame (taking steering into account)
            Quaternion wheelYaw = m_orientation * Quaternion::FromAxisAngle({0.0f, 1.0f, 0.0f}, w.steeringAngleRad);
            Vector3 wheelForward = wheelYaw.Forward();
            Vector3 wheelRight   = wheelYaw.Right();

            float wheelForwardVel = pointVel.Dot(wheelForward);
            float wheelLateralVel = pointVel.Dot(wheelRight);

            // Longitudinal Drive / Braking Force
            float longForce = 0.0f;
            if (w.isDriven && input.throttle > 0.01f) {
                float speedLimitFactor = std::clamp(1.0f - (speedMph / m_config.topSpeedMph), 0.0f, 1.0f);
                longForce = input.throttle * (m_config.engineForce * 0.25f) * speedLimitFactor;
            }

            if (input.brake > 0.01f) {
                if (forwardSpeed > 0.5f) {
                    // Footbrake deceleration
                    longForce -= input.brake * (m_config.brakeForce * 0.25f);
                } else {
                    // Reverse gear
                    longForce -= input.brake * (m_config.reverseForce * 0.5f);
                }
            }

            if (input.handbrake && !w.isFront) {
                // Handbrake locks rear wheels
                longForce -= std::copysign(m_config.brakeForce * 0.5f, wheelForwardVel);
            }

            // Lateral Friction Force (Pacejka slip model)
            float lateralFrictionCoeff = 1.0f;
            if (m_isDrifting) {
                lateralFrictionCoeff = w.isFront ? 0.75f : m_config.handbrakeDriftSlip;
            } else if (input.handbrake && !w.isFront) {
                lateralFrictionCoeff = m_config.handbrakeDriftSlip; // Handbrake breaks rear tire traction
            }

            float maxFriction = w.normalForce * lateralFrictionCoeff;
            float targetLateralForce = -wheelLateralVel * 18.0f * (m_config.mass * 0.25f);
            float latForce = std::clamp(targetLateralForce, -maxFriction, maxFriction);

            // Apply tire forces to chassis
            Vector3 tireForce = wheelForward * longForce + wheelRight * latForce;
            totalForce += tireForce;
            totalTorque += r.Cross(tireForce);

            // Update wheel spin rotation
            float wheelAngularSpeed = wheelForwardVel / m_config.wheelRadius;
            w.rotationAngleRad += wheelAngularSpeed * dt;
        } else {
            w.inContact = false;
            w.normalForce = 0.0f;
            w.suspensionLength = m_config.suspensionRestLength;

            // Free wheel spin down
            w.rotationAngleRad += (w.isDriven ? (input.throttle * 50.0f) : 0.0f) * dt;
        }
    }

    // 4. Arcade Handling Assists (Black Rock Studio Style)
    if (wheelsOnGround >= 2) {
        // Roll & Pitch stabilizer spring: gently pushes vehicle upright
        Vector3 targetUp = {0.0f, 1.0f, 0.0f};
        Vector3 rollError = up.Cross(targetUp);
        totalTorque += rollError * (m_config.mass * 35.0f);

        // Yaw damping during straight-line travel to prevent tank slappers
        if (std::abs(input.steer) < 0.05f && !m_isDrifting) {
            totalTorque.y -= m_angularVelocity.y * (m_config.mass * 8.0f);
        }

        // Split/Second drift yaw-rate assistance: helps rotate the car into drifts
        if ((m_isDrifting || input.handbrake) && std::abs(input.steer) > 0.05f) {
            totalTorque.y += input.steer * (m_config.mass * 12.0f);
        }
    }

    // 5. Numerical Integration (Symplectic Euler)
    // Linear
    Vector3 linearAccel = totalForce / m_config.mass;
    m_velocity += linearAccel * dt;
    m_velocity *= (1.0f - 0.005f * dt); // Aerodynamic drag
    m_position += m_velocity * dt;

    // Hard ground contact clamp: prevent falling through ground plane
    float minChassisHeight = m_config.wheelRadius * 0.75f;
    if (m_position.y < minChassisHeight) {
        m_position.y = minChassisHeight;
        if (m_velocity.y < 0.0f) m_velocity.y = 0.0f;
    }

    // Rotational (Inertia tensor approximation for box: width 1.9, height 1.3, length 4.5)
    float w = 1.9f, h = 1.3f, d = 4.5f;
    float m = m_config.mass;
    Vector3 invInertia = {
        12.0f / (m * (h * h + d * d)), // Pitch (X)
        12.0f / (m * (w * w + d * d)), // Yaw (Y)
        12.0f / (m * (w * w + h * h))  // Roll (Z)
    };

    Vector3 localTorque = {
        totalTorque.Dot(right),
        totalTorque.Dot(up),
        totalTorque.Dot(forward)
    };

    Vector3 localAngularAccel = {
        localTorque.x * invInertia.x,
        localTorque.y * invInertia.y,
        localTorque.z * invInertia.z
    };

    Vector3 worldAngularAccel = right * localAngularAccel.x + up * localAngularAccel.y + forward * localAngularAccel.z;
    m_angularVelocity += worldAngularAccel * dt;
    m_angularVelocity *= (1.0f - 0.20f * dt); // Angular damping

    // Orientation integration: dq/dt = 0.5 * w * q
    Quaternion wQuat{m_angularVelocity.x, m_angularVelocity.y, m_angularVelocity.z, 0.0f};
    Quaternion qDelta = (wQuat * m_orientation) * (0.5f * dt);
    m_orientation = (m_orientation + qDelta).Normalized();
}

} // namespace Physics
} // namespace SSV
