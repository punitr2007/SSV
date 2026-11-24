#include "touch_hud.h"
#include <cassert>
#include <iostream>
#include <cstdio>

void RunAllTouchHudTests() {
    std::cout << "[TEST] Running Touch HUD & Layout Customizer Tests..." << std::endl;

    SSV::TouchHUD hud;
    constexpr float screenW = 2160.0f;
    constexpr float screenH = 1080.0f;

    bool ok = hud.Initialize(screenW, screenH, "", false);
    assert(ok);
    assert(hud.GetControls().size() == 10);
    std::cout << "  [PASS] TouchHUD initialized with 10 default virtual controls." << std::endl;

    // 1. Throttle / Gas Pedal Touch Test
    auto* throttleCtrl = hud.FindControl(SSV::TouchControlId::Throttle);
    assert(throttleCtrl != nullptr);

    float tx, ty, tw, th;
    throttleCtrl->GetPixelRect(screenW, screenH, tx, ty, tw, th);
    hud.OnPointerDown(0, tx + tw * 0.5f, ty + th * 0.5f);

    SSV::InputState input;
    bool resetTriggered = false;
    hud.UpdateInputState(input, resetTriggered);
    assert(input.throttle == 1.0f);
    assert(throttleCtrl->isHeld);
    std::cout << "  [PASS] PointerDown on Throttle pedal activated gas (throttle = 1.0)." << std::endl;

    hud.OnPointerUp(0);
    input.Reset();
    hud.UpdateInputState(input, resetTriggered);
    assert(input.throttle == 0.0f);
    assert(!throttleCtrl->isHeld);
    std::cout << "  [PASS] PointerUp released gas pedal." << std::endl;

    // 2. Multi-Touch Simultaneous Steer & Drift Test
    auto* steerLeftCtrl = hud.FindControl(SSV::TouchControlId::SteerLeft);
    auto* handbrakeCtrl = hud.FindControl(SSV::TouchControlId::Handbrake);
    assert(steerLeftCtrl && handbrakeCtrl);

    float sx, sy, sw, sh;
    steerLeftCtrl->GetPixelRect(screenW, screenH, sx, sy, sw, sh);
    float hx, hy, hw, hh;
    handbrakeCtrl->GetPixelRect(screenW, screenH, hx, hy, hw, hh);

    hud.OnPointerDown(0, sx + sw * 0.5f, sy + sh * 0.5f); // Finger 0 on Steer Left
    hud.OnPointerDown(1, hx + hw * 0.5f, hy + hh * 0.5f); // Finger 1 on Handbrake

    input.Reset();
    hud.UpdateInputState(input, resetTriggered);
    assert(input.steer == -1.0f);
    assert(input.handbrake == true);
    std::cout << "  [PASS] Multi-touch finger 0 (Steer Left) + finger 1 (Handbrake) combined successfully." << std::endl;

    hud.OnPointerUp(0);
    hud.OnPointerUp(1);
    input.Reset();
    hud.UpdateInputState(input, resetTriggered);
    assert(input.steer == 0.0f);
    assert(!input.handbrake);

    // 3. Powerplay 1 and 2 Activation
    auto* pp1Ctrl = hud.FindControl(SSV::TouchControlId::Powerplay1);
    auto* pp2Ctrl = hud.FindControl(SSV::TouchControlId::Powerplay2);
    assert(pp1Ctrl && pp2Ctrl);

    float p1x, p1y, p1w, p1h;
    pp1Ctrl->GetPixelRect(screenW, screenH, p1x, p1y, p1w, p1h);
    hud.OnPointerDown(2, p1x + p1w * 0.5f, p1y + p1h * 0.5f);
    input.Reset();
    hud.UpdateInputState(input, resetTriggered);
    assert(input.powerplay1 == true);
    std::cout << "  [PASS] Powerplay Level 1 triggered." << std::endl;
    hud.OnPointerUp(2);

    float p2x, p2y, p2w, p2h;
    pp2Ctrl->GetPixelRect(screenW, screenH, p2x, p2y, p2w, p2h);
    hud.OnPointerDown(3, p2x + p2w * 0.5f, p2y + p2h * 0.5f);
    input.Reset();
    hud.UpdateInputState(input, resetTriggered);
    assert(input.powerplay2 == true);
    std::cout << "  [PASS] Powerplay Level 2 triggered." << std::endl;
    hud.OnPointerUp(3);

    // 4. Layout Customizer & Settings Persistence Test
    assert(!hud.IsEditMode());
    hud.ToggleEditMode();
    assert(hud.IsEditMode());
    std::cout << "  [PASS] Toggled Edit Mode." << std::endl;

    // Resize Throttle
    hud.SelectControl(SSV::TouchControlId::Throttle);
    float originalScale = throttleCtrl->scale;
    hud.AdjustSelectedSize(0.25f);
    assert(throttleCtrl->scale == originalScale + 0.25f);
    std::cout << "  [PASS] Adjusted control scale from " << originalScale << " to " << throttleCtrl->scale << std::endl;

    // Move Throttle in Edit Mode via Pointer Drag
    float origNormX = throttleCtrl->normX;
    hud.OnPointerDown(0, tx + tw * 0.5f, ty + th * 0.5f);
    hud.OnPointerMove(0, tx + tw * 0.5f - 100.0f, ty + th * 0.5f);
    hud.OnPointerUp(0);
    assert(throttleCtrl->normX != origNormX);
    std::cout << "  [PASS] Repositioned control normX from " << origNormX << " to " << throttleCtrl->normX << std::endl;

    // Save layout to disk
    const char* testCfg = "test_touch_layout.cfg";
    bool saved = hud.SaveLayout(testCfg);
    assert(saved);

    // Reset layout
    hud.ResetToDefaultLayout();
    throttleCtrl = hud.FindControl(SSV::TouchControlId::Throttle);
    assert(throttleCtrl->scale == originalScale);
    assert(throttleCtrl->normX == origNormX);
    std::cout << "  [PASS] ResetToDefaultLayout restored factory settings." << std::endl;

    // Load saved custom layout
    bool loaded = hud.LoadLayout(testCfg);
    assert(loaded);
    assert(throttleCtrl->scale == originalScale + 0.25f);
    std::cout << "  [PASS] LoadLayout successfully restored custom positions and scale from disk." << std::endl;

    std::remove(testCfg);
    hud.SetEditMode(false);

    std::cout << "[TEST] All Touch HUD & Layout Customizer Tests 100% PASSED!" << std::endl;
}
