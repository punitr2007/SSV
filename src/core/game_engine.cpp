#include "game_engine.h"
#include <iostream>

namespace SSV {
namespace Core {

bool GameEngine::Initialize(const Platform::WindowDesc& desc, bool headless) {
    m_platform = Platform::CreatePlatform(headless);

    if (!m_platform.window->Initialize(desc)) {
        return false;
    }

    if (!m_platform.graphics->Initialize(m_platform.window.get())) {
        return false;
    }

    if (!m_platform.audio->Initialize()) {
        return false;
    }

    // Start in cBootupState (matching 0x00BA2AA4)
    m_stateManager.ChangeState(std::make_unique<cBootupState>());
    m_lastFrameTime = m_platform.time->GetTimeSeconds();
    m_isRunning = true;
    return true;
}

void GameEngine::StepFrame(float dt) {
    // 1. Process OS and Windowing events
    if (!m_platform.window->ProcessEvents()) {
        m_isRunning = false;
        return;
    }

    // 2. Poll input devices
    m_platform.input->Poll();

    // 3. Update game simulation
    m_stateManager.Update(dt);
    m_powerplay.Update(dt);
    m_platform.audio->Update();

    // 4. Render pass
    m_platform.graphics->BeginFrame();
    m_stateManager.Render();
    m_platform.graphics->EndFrame();
    m_platform.window->SwapBuffers();
}

int GameEngine::Run() {
    while (m_isRunning) {
        double currentTime = m_platform.time->GetTimeSeconds();
        double frameTime = currentTime - m_lastFrameTime;
        m_lastFrameTime = currentTime;

        // Clamp excessive lag spikes
        if (frameTime > 0.25) {
            frameTime = 0.25;
        }

        m_accumulatedTime += static_cast<float>(frameTime);
        while (m_accumulatedTime >= m_fixedTimeStep) {
            StepFrame(m_fixedTimeStep);
            m_accumulatedTime -= m_fixedTimeStep;
            if (!m_isRunning) break;
        }

        // Avoid pegged CPU in idle/headless loops
        m_platform.time->SleepMicroseconds(1000); // 1 ms sleep
    }

    m_platform.graphics->Shutdown();
    m_platform.audio->Shutdown();
    return 0;
}

} // namespace Core
} // namespace SSV
