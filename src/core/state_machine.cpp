#include "state_machine.h"

namespace SSV {
namespace Core {

// ============================================================================
// State Implementations
// ============================================================================

cBootupState::cBootupState() : cGameState("cBootupState") {}
void cBootupState::Enter() {
    m_elapsed = 0.0f;
}
void cBootupState::Update(float dt) {
    m_elapsed += dt;
    // When initial configuration and archives are mounted, transition to cIntroState
    if (m_elapsed >= 0.1f && m_manager) {
        m_manager->ChangeState(std::make_unique<cIntroState>());
    }
}
void cBootupState::Exit() {}

cIntroState::cIntroState() : cGameState("cIntroState") {}
void cIntroState::Enter() {
    m_elapsed = 0.0f;
}
void cIntroState::Update(float dt) {
    m_elapsed += dt;
    // Bink logos (Disney Interactive & Black Rock Studio) complete -> transition
    if (m_elapsed >= 0.1f && m_manager) {
        m_manager->ChangeState(std::make_unique<cIntroSequence>());
    }
}
void cIntroState::Exit() {}

cIntroSequence::cIntroSequence() : cGameState("cIntroSequence") {}
void cIntroSequence::Enter() {
    m_elapsed = 0.0f;
}
void cIntroSequence::Update(float dt) {
    m_elapsed += dt;
    if (m_elapsed >= 0.1f && m_manager) {
        m_manager->ChangeState(std::make_unique<cFrontendLoadState>());
    }
}
void cIntroSequence::Exit() {}

cFrontendLoadState::cFrontendLoadState() : cGameState("cFrontendLoadState") {}
void cFrontendLoadState::Enter() {
    m_loaded = false;
}
void cFrontendLoadState::Update(float dt) {
    // Mount frontendpowerplays.ark and load Scaleform Flash menus
    m_loaded = true;
    if (m_loaded && m_manager) {
        m_manager->ChangeState(std::make_unique<cFrontendRunState>());
    }
}
void cFrontendLoadState::Exit() {}

cFrontendRunState::cFrontendRunState() : cGameState("cFrontendRunState") {}
void cFrontendRunState::Enter() {
    m_idleTimer = 0.0f;
}
void cFrontendRunState::Update(float dt) {
    m_idleTimer += dt;
    // Inactivity timeout triggers attract demo
    if (m_idleTimer > 120.0f && m_manager) {
        m_manager->ChangeState(std::make_unique<cLoadDemoState>());
    }
}
void cFrontendRunState::Exit() {}
void cFrontendRunState::StartRace() {
    if (m_manager) {
        m_manager->ChangeState(std::make_unique<cInGameLoadState>());
    }
}

cInGameLoadState::cInGameLoadState() : cGameState("cInGameLoadState") {}
void cInGameLoadState::Enter() {
    m_loadProgress = 0.0f;
}
void cInGameLoadState::Update(float dt) {
    m_loadProgress += dt * 4.0f;
    if (m_loadProgress >= 1.0f && m_manager) {
        m_manager->ChangeState(std::make_unique<cInGameMovieState>());
    }
}
void cInGameLoadState::Exit() {}

cInGameMovieState::cInGameMovieState() : cGameState("cInGameMovieState") {}
void cInGameMovieState::Enter() {
    m_elapsed = 0.0f;
}
void cInGameMovieState::Update(float dt) {
    m_elapsed += dt;
    // Track intro fly-through camera finishes -> enter active race
    if (m_elapsed >= 0.1f && m_manager) {
        m_manager->ChangeState(std::make_unique<cInGameRunState>());
    }
}
void cInGameMovieState::Exit() {}

cInGameRunState::cInGameRunState() : cGameState("cInGameRunState") {}
void cInGameRunState::Enter() {
    m_raceTimer = 0.0f;
    m_currentLap = 1;
}
void cInGameRunState::Update(float dt) {
    m_raceTimer += dt;
}
void cInGameRunState::Render() {
    // In-game 3D rendering pass
}
void cInGameRunState::CompleteLap() {
    m_currentLap++;
    if (m_currentLap > m_totalLaps && m_manager) {
        // Race finished -> return to menus
        m_manager->ChangeState(std::make_unique<cFrontendLoadState>());
    }
}
void cInGameRunState::Exit() {}

cLoadDemoState::cLoadDemoState() : cGameState("cLoadDemoState") {}
void cLoadDemoState::Enter() {}
void cLoadDemoState::Update(float dt) {}
void cLoadDemoState::Exit() {}

// ============================================================================
// State Manager Implementation
// ============================================================================

void cGameStateManager::ChangeState(std::unique_ptr<cGameState> newState) {
    m_nextState = std::move(newState);
}

void cGameStateManager::Update(float dt) {
    if (m_nextState) {
        if (m_currentState) {
            m_currentState->Exit();
        }
        m_currentState = std::move(m_nextState);
        m_currentState->SetManager(this);
        m_history.push_back(m_currentState->GetName());
        m_currentState->Enter();
    }

    if (m_currentState) {
        m_currentState->Update(dt);
    }
}

void cGameStateManager::Render() {
    if (m_currentState) {
        m_currentState->Render();
    }
}

} // namespace Core
} // namespace SSV
