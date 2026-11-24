#pragma once
/**
 * @file game_engine.h
 * @brief Core Game Engine coordinating HAL, State Machine, and Simulation.
 *
 * Reconstructs cGame singleton allocated at 0x00A1761B, constructed at 0x00A12CE0,
 * and executed via Run() at 0x00A129A0.
 */

#include "platform_abstraction.h"
#include "state_machine.h"
#include "powerplay_system.h"
#include <memory>
#include <atomic>

namespace SSV {
namespace Core {

class GameEngine {
    Platform::PlatformContext m_platform;
    cGameStateManager m_stateManager;
    PowerplaySystem m_powerplay;
    std::atomic<bool> m_isRunning{false};
    double m_lastFrameTime = 0.0;
    float m_fixedTimeStep = 1.0f / 60.0f; // 60 Hz simulation rate
    float m_accumulatedTime = 0.0f;

public:
    GameEngine() = default;
    ~GameEngine() = default;

    /**
     * Initializes engine subsystems, window, graphics, audio, and boots the initial state.
     */
    bool Initialize(const Platform::WindowDesc& desc, bool headless = false);

    /**
     * Executes the decoupled game loop until quit is signaled.
     */
    int Run();

    /**
     * Executes a single discrete engine frame (useful for deterministic tests).
     */
    void StepFrame(float dt);

    void Stop() { m_isRunning = false; }
    bool IsRunning() const { return m_isRunning; }

    cGameStateManager& GetStateManager() { return m_stateManager; }
    PowerplaySystem& GetPowerplaySystem() { return m_powerplay; }
    Platform::PlatformContext& GetPlatform() { return m_platform; }
};

} // namespace Core
} // namespace SSV
