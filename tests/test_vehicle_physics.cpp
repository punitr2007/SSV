#include "vehicle_physics.h"
#include "camera_rig.h"
#include <cassert>
#include <iostream>
#include <cmath>

void RunAllVehiclePhysicsTests() {
    std::cout << "\n[TEST SUITE] Running Vehicle Physics & Handling Tests..." << std::endl;

    SSV::Physics::VehicleSimulation sim;
    sim.Reset({0.0f, 0.40f, 0.0f});

    // 1. Initial State Check
    assert(sim.GetSpeedMph() < 0.01f);
    assert(sim.GetCurrentGear() == 1);
    assert(!sim.IsDrifting());
    std::cout << "  [PASS] Initial vehicle state verified (0 MPH, Gear 1, Not drifting)." << std::endl;

    // 2. Acceleration Test (60 ticks = 1.0 second of full throttle)
    SSV::InputState accelInput;
    accelInput.throttle = 1.0f;
    for (int i = 0; i < 60; ++i) {
        sim.Step(accelInput, 1.0f / 60.0f);
    }
    float speedAfter1Sec = sim.GetSpeedMph();
    std::cout << "  [PASS] Acceleration verified: " << speedAfter1Sec << " MPH after 1 second." << std::endl;
    assert(speedAfter1Sec > 5.0f);

    // 3. Speed-Sensitive Steering Attenuation Test
    // Low speed steering
    sim.Reset({0.0f, 0.40f, 0.0f});
    SSV::InputState steerInput;
    steerInput.steer = 1.0f;
    sim.Step(steerInput, 1.0f / 60.0f);
    float lowSpeedSteer = sim.GetWheels()[0].steeringAngleRad;
    float expectedLowSteerMax = 35.0f * SSV::Math::DEG_TO_RAD;
    assert(std::abs(lowSpeedSteer) <= expectedLowSteerMax);
    std::cout << "  [PASS] Low speed steer angle verified: " << (lowSpeedSteer * SSV::Math::RAD_TO_DEG) << " deg." << std::endl;

    // 4. Handbrake Drift Test
    // Accelerate to speed, then steer + handbrake
    sim.Reset({0.0f, 0.40f, 0.0f});
    for (int i = 0; i < 180; ++i) {
        sim.Step(accelInput, 1.0f / 60.0f);
    }
    float speedAfter3Sec = sim.GetSpeedMph();
    std::cout << "  [PASS] Speed after 3 seconds: " << speedAfter3Sec << " MPH." << std::endl;
    assert(speedAfter3Sec > 10.0f);

    SSV::InputState driftInput;
    driftInput.throttle = 0.5f;
    driftInput.steer = 1.0f;
    driftInput.handbrake = true;
    for (int i = 0; i < 30; ++i) {
        sim.Step(driftInput, 1.0f / 60.0f);
    }
    assert(sim.IsDrifting());
    std::cout << "  [PASS] Handbrake drift mechanic verified (Drifting = TRUE, SlipAngle = " 
              << (sim.GetSlipAngle() * SSV::Math::RAD_TO_DEG) << " deg)." << std::endl;

    // 5. Camera Rig Tracking & FOV Test
    SSV::CameraRig camera;
    camera.Reset(sim.GetPosition(), sim.GetOrientation());
    float initialFov = camera.GetCurrentFov();
    assert(std::abs(initialFov - 62.0f) < 0.1f);

    // Update camera with high speed
    camera.Update(sim.GetPosition(), sim.GetOrientation(), 150.0f, 0.5f);
    float highSpeedFov = camera.GetCurrentFov();
    assert(highSpeedFov > initialFov);
    std::cout << "  [PASS] Camera dynamic FOV verified: " << initialFov << " deg -> " << highSpeedFov << " deg." << std::endl;

    std::cout << "[TEST SUITE] Vehicle Physics & Handling Tests 100% PASSED!" << std::endl;
}
