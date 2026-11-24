#pragma once
/**
 * @file input_state.h
 * @brief Three-layer decoupled input architecture: Raw Events -> InputMapper -> InputState -> Sim Tick
 *
 * The fixed-timestep 60Hz physics and game simulation exclusively consumes normalized InputState.
 * Supports tick-level recording and deterministic replay for physics tuning and automated regressions.
 */

#include <cstdint>
#include <cmath>
#include <vector>
#include <unordered_map>
#include <string>
#include <algorithm>

namespace SSV {

// ============================================================================
// Layer 3: Normalized Simulation Input Frame (Consumed by 60 Hz Sim Tick)
// ============================================================================
struct InputState {
    uint32_t tick = 0;              // Fixed 60 Hz tick index

    float steer = 0.0f;             // [-1.0f (Full Left) .. +1.0f (Full Right)]
    float throttle = 0.0f;          // [0.0f (Idle) .. 1.0f (Full Gas)]
    float brake = 0.0f;             // [0.0f (None) .. 1.0f (Full Brake / Reverse)]
    bool handbrake = false;         // Space / Button B (Initiates drift)

    bool powerplay1 = false;        // Level 1 Powerplay (Blue icon detonation)
    bool powerplay2 = false;        // Level 2 Powerplay (Red super-event / route change)

    bool lookBehind = false;        // Rearview mirror camera toggle
    bool changeCamera = false;      // Cycle cockpit / chase / bumper camera
    bool pause = false;             // Pause menu toggle

    void Reset() {
        steer = 0.0f;
        throttle = 0.0f;
        brake = 0.0f;
        handbrake = false;
        powerplay1 = false;
        powerplay2 = false;
        lookBehind = false;
        changeCamera = false;
        pause = false;
    }
};

// ============================================================================
// Layer 1 & 2: Touch Pointer Tracking & Device Input Mapper
// ============================================================================
struct TouchPointer {
    int32_t id = -1;                // Tracking by pointer ID (NOT index)
    float startX = 0.0f;            // Anchor position (floating joystick center)
    float startY = 0.0f;
    float currentX = 0.0f;
    float currentY = 0.0f;
    bool active = false;
};

class InputMapper {
public:
    InputMapper() = default;

    // --- Configuration Tuning ---
    float deadZone = 0.05f;         // Stick center deadzone
    float steeringSensitivity = 1.0f;
    float steeringCurve = 1.5f;     // Non-linear response (power curve for precision at high speed)
    bool invertSteering = false;

    // --- Raw Event Ingestion: Keyboard / Gamepad ---
    void SetKeySteer(float val) { m_keySteer = std::clamp(val, -1.0f, 1.0f); }
    void SetKeyThrottle(float val) { m_keyThrottle = std::clamp(val, 0.0f, 1.0f); }
    void SetKeyBrake(float val) { m_keyBrake = std::clamp(val, 0.0f, 1.0f); }
    void SetKeyHandbrake(bool val) { m_keyHandbrake = val; }
    void SetKeyPowerplay1(bool val) { m_keyPP1 = val; }
    void SetKeyPowerplay2(bool val) { m_keyPP2 = val; }

    // --- Raw Event Ingestion: Gamepad (XInput / Android GameController) ---
    void SetGamepadAxes(float leftStickX, float rightTrigger, float leftTrigger) {
        m_gamepadSteer = ApplyDeadZoneAndCurve(leftStickX);
        m_gamepadThrottle = std::clamp(rightTrigger, 0.0f, 1.0f);
        m_gamepadBrake = std::clamp(leftTrigger, 0.0f, 1.0f);
    }

    void SetGamepadButtons(bool handbrake, bool pp1, bool pp2) {
        m_gamepadHandbrake = handbrake;
        m_gamepadPP1 = pp1;
        m_gamepadPP2 = pp2;
    }

    // --- Raw Event Ingestion: Android Multitouch (Tracked by Pointer ID) ---
    void OnTouchDown(int32_t pointerId, float normX, float normY) {
        TouchPointer& pt = m_pointers[pointerId];
        pt.id = pointerId;
        pt.startX = normX;
        pt.startY = normY;
        pt.currentX = normX;
        pt.currentY = normY;
        pt.active = true;

        // Left half of screen: Floating Steering Anchor
        if (normX < 0.5f && m_steeringPointerId == -1) {
            m_steeringPointerId = pointerId;
        }
    }

    void OnTouchMove(int32_t pointerId, float normX, float normY) {
        auto it = m_pointers.find(pointerId);
        if (it != m_pointers.end() && it->second.active) {
            it->second.currentX = normX;
            it->second.currentY = normY;
        }
    }

    void OnTouchUp(int32_t pointerId) {
        auto it = m_pointers.find(pointerId);
        if (it != m_pointers.end()) {
            it->second.active = false;
        }
        if (m_steeringPointerId == pointerId) {
            m_steeringPointerId = -1;
            m_touchSteer = 0.0f;
        }
    }

    // Evaluates current inputs into a clean InputState frame for the current sim tick
    InputState Sample(uint32_t currentTick) {
        InputState state;
        state.tick = currentTick;

        // 1. Process Floating Touch Steering
        if (m_steeringPointerId != -1) {
            const TouchPointer& pt = m_pointers[m_steeringPointerId];
            if (pt.active) {
                // Maximum deflection radius = 15% screen width
                float deltaX = (pt.currentX - pt.startX) / 0.15f;
                m_touchSteer = ApplyDeadZoneAndCurve(deltaX);
            }
        }

        // 2. Process Touch Right-Zone Action Buttons
        bool touchThrottle = false;
        bool touchBrake = false;
        bool touchPP1 = false;
        bool touchPP2 = false;

        for (const auto& kv : m_pointers) {
            const TouchPointer& pt = kv.second;
            if (pt.active && pt.currentX >= 0.5f) {
                if (pt.currentY > 0.65f) {
                    // Bottom right quadrant: Gas pedal
                    touchThrottle = true;
                } else if (pt.currentY > 0.40f) {
                    // Mid right: Footbrake / Reverse
                    touchBrake = true;
                } else {
                    // Top right: Powerplays (left of mid-line = PP1, right = PP2)
                    if (pt.currentX < 0.75f) touchPP1 = true;
                    else touchPP2 = true;
                }
            }
        }

        // 3. Combine with Keyboard & Gamepad (Max / Priority merge)
        state.steer = std::clamp(m_keySteer + m_gamepadSteer + m_touchSteer, -1.0f, 1.0f);
        state.throttle = std::clamp(std::max({m_keyThrottle, m_gamepadThrottle, touchThrottle ? 1.0f : 0.0f}), 0.0f, 1.0f);
        state.brake = std::clamp(std::max({m_keyBrake, m_gamepadBrake, touchBrake ? 1.0f : 0.0f}), 0.0f, 1.0f);
        state.handbrake = m_keyHandbrake || m_gamepadHandbrake;
        state.powerplay1 = m_keyPP1 || m_gamepadPP1 || touchPP1;
        state.powerplay2 = m_keyPP2 || m_gamepadPP2 || touchPP2;

        return state;
    }

private:
    float ApplyDeadZoneAndCurve(float input) {
        float sign = (input < 0.0f) ? -1.0f : 1.0f;
        float absVal = std::abs(input);
        if (absVal < deadZone) return 0.0f;

        // Remap [deadZone .. 1.0] to [0.0 .. 1.0]
        float normalized = (absVal - deadZone) / (1.0f - deadZone);
        normalized = std::clamp(normalized, 0.0f, 1.0f);

        // Apply non-linear power response curve
        float curved = std::pow(normalized, steeringCurve);
        return sign * curved * (invertSteering ? -1.0f : 1.0f);
    }

    float m_keySteer = 0.0f;
    float m_keyThrottle = 0.0f;
    float m_keyBrake = 0.0f;
    bool m_keyHandbrake = false;
    bool m_keyPP1 = false;
    bool m_keyPP2 = false;

    float m_gamepadSteer = 0.0f;
    float m_gamepadThrottle = 0.0f;
    float m_gamepadBrake = 0.0f;
    bool m_gamepadHandbrake = false;
    bool m_gamepadPP1 = false;
    bool m_gamepadPP2 = false;

    std::unordered_map<int32_t, TouchPointer> m_pointers;
    int32_t m_steeringPointerId = -1;
    float m_touchSteer = 0.0f;
};

// ============================================================================
// Deterministic Input Recorder & Replay
// ============================================================================
class InputReplay {
public:
    void RecordFrame(const InputState& state) {
        m_frames.push_back(state);
    }

    bool HasNext(uint32_t playbackIndex) const {
        return playbackIndex < m_frames.size();
    }

    InputState GetFrame(uint32_t playbackIndex) const {
        if (playbackIndex < m_frames.size()) {
            return m_frames[playbackIndex];
        }
        return InputState{};
    }

    size_t FrameCount() const { return m_frames.size(); }
    void Clear() { m_frames.clear(); }

private:
    std::vector<InputState> m_frames;
};

} // namespace SSV
