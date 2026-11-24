#include "state_machine.h"
#include "powerplay_system.h"
#include <cassert>
#include <iostream>

using namespace SSV::Core;

void TestStateMachineTransitions() {
    std::cout << "[TEST] Running State Machine Transitions..." << std::endl;

    cGameStateManager sm;

    // Initial state: Bootup
    sm.ChangeState(std::make_unique<cBootupState>());
    sm.Update(0.016f);
    assert(sm.GetCurrentState() != nullptr);
    assert(sm.GetCurrentState()->GetName() == "cBootupState");

    // Advance past bootup threshold
    sm.Update(0.15f);
    sm.Update(0.016f); // Tick to apply transition
    assert(sm.GetCurrentState()->GetName() == "cIntroState");

    // Advance past intro logo threshold
    sm.Update(0.15f);
    sm.Update(0.016f); // Tick to apply transition
    assert(sm.GetCurrentState()->GetName() == "cIntroSequence");

    // Advance past intro sequence
    sm.Update(0.15f);
    sm.Update(0.016f); // Tick to apply transition
    assert(sm.GetCurrentState()->GetName() == "cFrontendLoadState");

    // Complete frontend loading
    sm.Update(0.016f);
    sm.Update(0.016f); // Tick to apply transition
    assert(sm.GetCurrentState()->GetName() == "cFrontendRunState");

    // Trigger race start
    auto* frontend = dynamic_cast<cFrontendRunState*>(sm.GetCurrentState());
    assert(frontend != nullptr);
    frontend->StartRace();

    // Verify transition to cInGameLoadState
    sm.Update(0.016f);
    assert(sm.GetCurrentState()->GetName() == "cInGameLoadState");

    // Advance track load progress
    sm.Update(0.3f);
    sm.Update(0.016f); // Tick to apply transition
    assert(sm.GetCurrentState()->GetName() == "cInGameMovieState");

    // Advance track fly-through
    sm.Update(0.15f);
    sm.Update(0.016f); // Tick to apply transition
    assert(sm.GetCurrentState()->GetName() == "cInGameRunState");

    // Verify active race state
    auto* inGame = dynamic_cast<cInGameRunState*>(sm.GetCurrentState());
    assert(inGame != nullptr);
    assert(inGame->GetCurrentLap() == 1);

    // Simulate lap completion
    inGame->CompleteLap();
    assert(inGame->GetCurrentLap() == 2);
    inGame->CompleteLap();
    assert(inGame->GetCurrentLap() == 3);
    inGame->CompleteLap(); // Finish race -> should transition back to frontend

    sm.Update(0.016f);
    assert(sm.GetCurrentState()->GetName() == "cFrontendLoadState");

    std::cout << "  -> All 8 state transitions verified successfully." << std::endl;
}

void TestPowerplayMechanics() {
    std::cout << "[TEST] Running Powerplay Trigger Mechanics..." << std::endl;

    PowerplaySystem pp;
    assert(pp.GetCharge() == 0.0f);
    assert(pp.GetAvailableSegments() == 0);

    // Register a Level 1 trigger (Cranes/Explosive Barrels)
    PowerplayTrigger t1;
    t1.id = 101;
    t1.name = "Crane_Collapse";
    t1.type = PowerplayType::Level1_Tactical;
    t1.state = PowerplayState::Ready;
    t1.detonationTimeSeconds = 0.5f;
    pp.RegisterTrigger(t1);

    // Register a Level 2 trigger (Airport Tower Collapse / Route Alteration)
    PowerplayTrigger t2;
    t2.id = 201;
    t2.name = "Airport_Tower_Drop";
    t2.type = PowerplayType::Level2_RouteAlter;
    t2.state = PowerplayState::Ready;
    t2.detonationTimeSeconds = 1.0f;
    t2.altersRoute = true;
    pp.RegisterTrigger(t2);

    // Cannot trigger without sufficient meter
    assert(!pp.CanTrigger(101));
    assert(!pp.CanTrigger(201));

    // Add 1.5 segments (drifting / drafting)
    pp.AddCharge(1.5f);
    assert(pp.GetAvailableSegments() == 1);
    assert(pp.CanTrigger(101));
    assert(!pp.CanTrigger(201));

    // Activate trigger 101
    bool activated = pp.ActivateTrigger(101);
    assert(activated);
    assert(pp.GetCharge() == 0.5f);

    // Update detonation progress
    pp.Update(0.25f);
    assert(pp.GetTriggers()[0].state == PowerplayState::Detonating);
    pp.Update(0.3f);
    assert(pp.GetTriggers()[0].state == PowerplayState::Detonated);

    // Add full 3 segments
    pp.AddCharge(2.6f);
    assert(pp.GetCharge() == 3.0f);
    assert(pp.CanTrigger(201));

    bool activatedRoute = pp.ActivateTrigger(201);
    assert(activatedRoute);
    assert(pp.GetCharge() == 0.0f);
    assert(pp.GetTriggers()[1].state == PowerplayState::Detonating);

    std::cout << "  -> Powerplay trigger and detonation mechanics passed." << std::endl;
}

void RunAllStateTests() {
    TestStateMachineTransitions();
    TestPowerplayMechanics();
}
