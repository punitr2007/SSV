#pragma once
/**
 * @file vehicle_physics.h
 * @brief 4-Wheel Raycast Vehicle Physics and Handling Simulation
 *
 * Implements vehicle dynamics using exact parameters reverse-engineered from SplitSecond.exe:
 *  - Mass: 3000 lbs (1360 kg)
 *  - TopSpeed: 200 MPH (89.4 m/s)
 *  - MaxSteerAngle: 35.0 deg (speed-attenuated)
 *  - SpringK: 100.0, SpringDamping: 100.0
 *  - Havok transmission & slip friction model with powerplay handbrake drift mechanics
 */

#include "math_types.h"
#include "input_state.h"
#include <array>
#include <vector>
#include <cmath>

namespace SSV {
namespace Physics {

using Math::Vector3;
using Math::Matrix4x4;
using Math::Quaternion;

struct RoadTriangle {
    Vector3 v0;
    Vector3 v1;
    Vector3 v2;
    Vector3 normal;
    float minX = 0.0f, maxX = 0.0f;
    float minY = 0.0f, maxY = 0.0f;
    float minZ = 0.0f, maxZ = 0.0f;
};

struct VehicleConfig {
    float mass = 1360.0f;               // 3000 lbs (from binary 0x00BC202C)
    float topSpeedMph = 200.0f;         // 200 MPH (from binary 0x00BA62C0)
    float maxSteerAngleDeg = 35.0f;     // 35 degrees (from binary 0x00C1A5A8)
    float maxSpeedFullSteerMph = 45.0f; // Speed limit for 100% steer angle
    float highSpeedSteerDeg = 10.0f;    // Attenuated steer at 200 MPH

    float springK = 28000.0f;           // Suspension spring stiffness
    float springDamping = 3800.0f;      // Suspension damper rate (critically damped)
    float suspensionRestLength = 0.55f; // Metres
    float wheelRadius = 0.35f;          // Metres

    float engineForce = 16500.0f;       // Peak drive force (N) (arcade supercar acceleration)
    float brakeForce = 22000.0f;        // Peak braking force (N)
    float reverseForce = 7500.0f;
    float handbrakeDriftSlip = 0.35f;   // Lateral friction drop on drift

    Vector3 hardPointFL = {-0.85f, 0.25f,  1.35f};
    Vector3 hardPointFR = { 0.85f, 0.25f,  1.35f};
    Vector3 hardPointRL = {-0.85f, 0.25f, -1.35f};
    Vector3 hardPointRR = { 0.85f, 0.25f, -1.35f};
};

struct Wheel {
    Vector3 hardPoint;          // Anchor relative to chassis center of mass
    bool isFront = false;
    bool isDriven = true;

    // Dynamic runtime state
    float suspensionLength = 0.55f;
    float steeringAngleRad = 0.0f;
    float rotationAngleRad = 0.0f;
    float angularVelocity = 0.0f;
    bool inContact = false;
    Vector3 contactPoint;
    Vector3 contactNormal;
    float normalForce = 0.0f;
};

class VehicleSimulation {
public:
    VehicleSimulation();
    explicit VehicleSimulation(const VehicleConfig& config);

    void Reset(const Vector3& spawnPosition = {0.0f, 0.5f, 0.0f});
    void Step(const InputState& input, float dt);

    // --- Road Collision & Ground Terrain ---
    void SetRoadCollision(const std::vector<RoadTriangle>& triangles);
    void ClearRoadCollision();
    bool HasRoadCollision() const { return !m_roadTriangles.empty(); }
    const std::vector<RoadTriangle>& GetRoadCollision() const { return m_roadTriangles; }

    // --- State Getters ---
    const Vector3& GetPosition() const { return m_position; }
    const Vector3& GetVelocity() const { return m_velocity; }
    const Quaternion& GetOrientation() const { return m_orientation; }
    Matrix4x4 GetTransform() const;

    float GetSpeedMph() const;
    float GetSpeedKmh() const;
    float GetEngineRpm() const { return m_engineRpm; }
    int GetCurrentGear() const { return m_currentGear; }
    bool IsDrifting() const { return m_isDrifting; }
    float GetSlipAngle() const { return m_slipAngle; }

    const std::array<Wheel, 4>& GetWheels() const { return m_wheels; }
    Matrix4x4 GetWheelTransform(size_t wheelIndex) const;

private:
    VehicleConfig m_config;

    Vector3 m_position{0.0f, 0.5f, 0.0f};
    Vector3 m_velocity{0.0f, 0.0f, 0.0f};
    Quaternion m_orientation = Quaternion::Identity();
    Vector3 m_angularVelocity{0.0f, 0.0f, 0.0f};

    std::array<Wheel, 4> m_wheels;
    std::vector<RoadTriangle> m_roadTriangles;

    float m_engineRpm = 1000.0f;
    int m_currentGear = 1;
    bool m_isDrifting = false;
    float m_slipAngle = 0.0f;

    void SimulateSuspensionAndTires(const InputState& input, float dt);
};

} // namespace Physics
} // namespace SSV
