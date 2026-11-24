#pragma once
/**
 * @file state_machine.h
 * @brief Reverse engineered state machine matching Black Rock Studio GameStates namespace.
 *
 * Implements all 10 verified game state classes:
 *  - cBootupState (VTABLE: 0x00BA2AA4)
 *  - cIntroState (VTABLE: 0x00BA32DC)
 *  - cIntroSequence (VTABLE: 0x00BA37EC)
 *  - cFrontendLoadState (VTABLE: 0x00BA3398)
 *  - cFrontendRunState (VTABLE: 0x00BA38C0)
 *  - cInGameLoadState (VTABLE: 0x00BA3110)
 *  - cInGameRunState (VTABLE: 0x00BA36A4)
 *  - cInGameMovieState (VTABLE: 0x00BA3518)
 *  - cLoadDemoState (VTABLE: 0x00BA3894)
 */

#include <string>
#include <memory>
#include <vector>
#include <iostream>

namespace SSV {
namespace Core {

class cGameStateManager;

class cGameState {
protected:
    cGameStateManager* m_manager = nullptr;
    std::string m_name;

public:
    explicit cGameState(std::string name) : m_name(std::move(name)) {}
    virtual ~cGameState() = default;

    void SetManager(cGameStateManager* mgr) { m_manager = mgr; }
    const std::string& GetName() const { return m_name; }

    virtual void Enter() {}
    virtual void Update(float dt) {}
    virtual void Render() {}
    virtual void Exit() {}
};

class cBootupState : public cGameState {
    float m_elapsed = 0.0f;
public:
    cBootupState();
    void Enter() override;
    void Update(float dt) override;
    void Exit() override;
};

class cIntroState : public cGameState {
    float m_elapsed = 0.0f;
public:
    cIntroState();
    void Enter() override;
    void Update(float dt) override;
    void Exit() override;
};

class cIntroSequence : public cGameState {
    float m_elapsed = 0.0f;
public:
    cIntroSequence();
    void Enter() override;
    void Update(float dt) override;
    void Exit() override;
};

class cFrontendLoadState : public cGameState {
    bool m_loaded = false;
public:
    cFrontendLoadState();
    void Enter() override;
    void Update(float dt) override;
    void Exit() override;
};

class cFrontendRunState : public cGameState {
    float m_idleTimer = 0.0f;
public:
    cFrontendRunState();
    void Enter() override;
    void Update(float dt) override;
    void Exit() override;
    void StartRace();
};

class cInGameLoadState : public cGameState {
    float m_loadProgress = 0.0f;
public:
    cInGameLoadState();
    void Enter() override;
    void Update(float dt) override;
    void Exit() override;
};

class cInGameMovieState : public cGameState {
    float m_movieDuration = 2.0f;
    float m_elapsed = 0.0f;
public:
    cInGameMovieState();
    void Enter() override;
    void Update(float dt) override;
    void Exit() override;
};

class cInGameRunState : public cGameState {
    float m_raceTimer = 0.0f;
    uint32_t m_currentLap = 1;
    uint32_t m_totalLaps = 3;
public:
    cInGameRunState();
    void Enter() override;
    void Update(float dt) override;
    void Render() override;
    void Exit() override;

    float GetRaceTime() const { return m_raceTimer; }
    uint32_t GetCurrentLap() const { return m_currentLap; }
    void CompleteLap();
};

class cLoadDemoState : public cGameState {
public:
    cLoadDemoState();
    void Enter() override;
    void Update(float dt) override;
    void Exit() override;
};

class cGameStateManager {
    std::unique_ptr<cGameState> m_currentState;
    std::unique_ptr<cGameState> m_nextState;
    std::vector<std::string> m_history;

public:
    cGameStateManager() = default;

    void ChangeState(std::unique_ptr<cGameState> newState);
    void Update(float dt);
    void Render();

    cGameState* GetCurrentState() const { return m_currentState.get(); }
    const std::vector<std::string>& GetHistory() const { return m_history; }
};

} // namespace Core
} // namespace SSV
