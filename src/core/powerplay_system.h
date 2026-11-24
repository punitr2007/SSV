#pragma once
/**
 * @file powerplay_system.h
 * @brief Reconstruction of Split/Second signature Powerplay destruction mechanics.
 */

#include "math_types.h"
#include <string>
#include <vector>
#include <cstdint>

namespace SSV {
namespace Core {

enum class PowerplayType {
    Level1_Tactical,  // Blue: Bridges, cranes, billboard drops, explosive barrels
    Level2_RouteAlter // Red: Major landmarks, airport tower collapse, dam burst
};

enum class PowerplayState {
    Ready,
    Armed,
    Detonating,
    Detonated
};

struct PowerplayTrigger {
    uint32_t id = 0;
    std::string name;
    PowerplayType type = PowerplayType::Level1_Tactical;
    PowerplayState state = PowerplayState::Ready;
    Math::Vector3 worldPosition;
    float triggerRadius = 50.0f;
    float blastRadius = 35.0f;
    float blastForce = 15000.0f;
    float detonationTimeSeconds = 1.5f;
    float elapsedDetonation = 0.0f;
    bool altersRoute = false;
};

class PowerplaySystem {
    float m_meterCharge = 0.0f; // 0.0f to 3.0f (3 segments)
    std::vector<PowerplayTrigger> m_triggers;

public:
    PowerplaySystem() = default;

    void AddCharge(float amount) {
        m_meterCharge = std::clamp(m_meterCharge + amount, 0.0f, 3.0f);
    }

    float GetCharge() const { return m_meterCharge; }
    uint32_t GetAvailableSegments() const { return static_cast<uint32_t>(m_meterCharge); }

    void RegisterTrigger(const PowerplayTrigger& trigger) {
        m_triggers.push_back(trigger);
    }

    const std::vector<PowerplayTrigger>& GetTriggers() const { return m_triggers; }

    bool CanTrigger(uint32_t triggerId) const {
        for (const auto& t : m_triggers) {
            if (t.id == triggerId && t.state == PowerplayState::Ready) {
                if (t.type == PowerplayType::Level1_Tactical && m_meterCharge >= 1.0f) return true;
                if (t.type == PowerplayType::Level2_RouteAlter && m_meterCharge >= 3.0f) return true;
            }
        }
        return false;
    }

    bool ActivateTrigger(uint32_t triggerId) {
        for (auto& t : m_triggers) {
            if (t.id == triggerId && t.state == PowerplayState::Ready) {
                float cost = (t.type == PowerplayType::Level1_Tactical) ? 1.0f : 3.0f;
                if (m_meterCharge >= cost) {
                    m_meterCharge -= cost;
                    t.state = PowerplayState::Detonating;
                    t.elapsedDetonation = 0.0f;
                    return true;
                }
            }
        }
        return false;
    }

    void Update(float dt) {
        for (auto& t : m_triggers) {
            if (t.state == PowerplayState::Detonating) {
                t.elapsedDetonation += dt;
                if (t.elapsedDetonation >= t.detonationTimeSeconds) {
                    t.state = PowerplayState::Detonated;
                }
            }
        }
    }
};

} // namespace Core
} // namespace SSV
