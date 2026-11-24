#pragma once
/**
 * @file touch_hud.h
 * @brief Native Touch HUD, Multi-Touch Finger Tracking & Layout Customizer
 *
 * Inspired by high-performance recompilation HUDs (NFSU2-Revived-Android):
 *  - Zero GC overhead multi-touch tracking by pointer ID (up to 16 fingers).
 *  - Screen-space vertex generation with GPU-rendered procedural rounded shapes,
 *    accent glowing borders, and Split/Second styled tactical action icons.
 *  - Interactive On-Screen Layout Editor allowing players to drag, reposition,
 *    scale (0.5x - 2.0x), and adjust opacity of every individual button.
 *  - Automatic persistence of custom button positions to local disk config.
 */

#include "math_types.h"
#include "input_state.h"
#include <string>
#include <vector>
#include <array>
#include <memory>
#include <cstdint>

namespace SSV {

using Math::Vector3;
using Math::Vector4;

enum class TouchControlId {
    SteerLeft = 0,
    SteerRight,
    Throttle,
    Brake,
    Handbrake,
    Powerplay1,
    Powerplay2,
    Camera,
    Reset,
    Settings,
    Count
};

enum class ControlShape {
    Round = 0,
    Pedal,
    Pill,
    ArrowLeft,
    ArrowRight,
    ToolbarBtn
};

struct VirtualControl {
    TouchControlId id = TouchControlId::Throttle;
    std::string name;
    std::string label;

    // Normalized coordinates [0.0 .. 1.0] relative to screen dimensions
    float normX = 0.0f;
    float normY = 0.0f;
    float normW = 0.1f;
    float normH = 0.1f;
    float scale = 1.0f;

    ControlShape shape = ControlShape::Round;
    Vector4 idleColor{0.10f, 0.12f, 0.16f, 0.70f};
    Vector4 activeColor{0.20f, 0.85f, 1.00f, 0.95f};
    Vector4 accentColor{0.00f, 0.70f, 1.00f, 1.00f};

    bool isHeld = false;
    int32_t activePointerId = -1;
    int directInputScan = 0;

    void GetPixelRect(float screenW, float screenH, float& outX, float& outY, float& outW, float& outH) const {
        float effectiveW = normW * screenW * scale;
        float effectiveH = normH * screenH * scale;
        // Center-anchored scaling
        float centerX = (normX + normW * 0.5f) * screenW;
        float centerY = (normY + normH * 0.5f) * screenH;
        outX = centerX - effectiveW * 0.5f;
        outY = centerY - effectiveH * 0.5f;
        outW = effectiveW;
        outH = effectiveH;
    }

    bool Contains(float px, float py, float screenW, float screenH) const {
        float x, y, w, h;
        GetPixelRect(screenW, screenH, x, y, w, h);
        return (px >= x && px <= (x + w) && py >= y && py <= (y + h));
    }
};

struct HudVertex {
    float x = 0.0f;
    float y = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    Vector4 color{1.0f, 1.0f, 1.0f, 1.0f};
    Vector4 accentColor{1.0f, 1.0f, 1.0f, 1.0f};
    float shapeType = 0.0f;
    float isPressed = 0.0f;
    float isSelected = 0.0f;
};

class TouchHUD {
public:
    TouchHUD();
    ~TouchHUD();

    bool Initialize(float screenW, float screenH, const std::string& configFilePath = "", bool initGL = true);
    void Resize(float screenW, float screenH);
    void Shutdown();

    // Touch / Mouse event processing
    void OnPointerDown(int32_t pointerId, float screenX, float screenY);
    void OnPointerMove(int32_t pointerId, float screenX, float screenY);
    void OnPointerUp(int32_t pointerId);
    void OnCancel();

    // Ingests touched state into simulation InputState
    void UpdateInputState(InputState& outState, bool& outResetTriggered);

    // Renders the virtual touch controller overlay in screen space
    void Render(float screenW, float screenH);

    // Layout Editor / Customizer Mode
    bool IsEditMode() const { return m_editMode; }
    void SetEditMode(bool enable);
    void ToggleEditMode();
    void SelectControl(TouchControlId id);
    void AdjustSelectedSize(float delta);
    void CycleOpacity();
    void SetOpacity(float opacity) { m_opacity = std::clamp(opacity, 0.20f, 1.0f); }
    float GetOpacity() const { return m_opacity; }

    // Configuration Persistence
    void ResetToDefaultLayout();
    bool SaveLayout(const std::string& configFilePath);
    bool LoadLayout(const std::string& configFilePath);

    // Direct access for testing
    const std::vector<VirtualControl>& GetControls() const { return m_controls; }
    VirtualControl* FindControl(TouchControlId id);

private:
    void InitDefaultControls();
    void BuildShader();
    void AppendControlQuad(const VirtualControl& ctrl, std::vector<HudVertex>& outVertices, std::vector<uint16_t>& outIndices);
    void AppendToolbarQuad(float x, float y, float w, float h, const std::string& label, const Vector4& col,
                           std::vector<HudVertex>& outVertices, std::vector<uint16_t>& outIndices);

    float m_screenWidth = 2160.0f;
    float m_screenHeight = 1080.0f;
    float m_opacity = 0.85f;
    bool m_editMode = false;
    std::string m_configPath;

    std::vector<VirtualControl> m_controls;

    // Multi-touch tracking: pointer ID -> control index (-1 if none)
    static constexpr size_t MAX_TRACKED_FINGERS = 16;
    std::array<int, MAX_TRACKED_FINGERS> m_fingerMap{};
    std::array<float, MAX_TRACKED_FINGERS> m_fingerStartX{};
    std::array<float, MAX_TRACKED_FINGERS> m_fingerStartY{};

    // Layout Editor State
    int m_selectedControlIndex = -1;
    int32_t m_dragPointerId = -1;
    float m_dragOffsetX = 0.0f;
    float m_dragOffsetY = 0.0f;

    // GL Resources
    uint32_t m_program = 0;
    uint32_t m_vao = 0;
    uint32_t m_vbo = 0;
    uint32_t m_ebo = 0;
    int32_t m_uResolution = -1;
    int32_t m_uGlobalOpacity = -1;
    bool m_glInitialized = false;
};

} // namespace SSV
