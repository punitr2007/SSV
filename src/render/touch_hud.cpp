/**
 * @file touch_hud.cpp
 * @brief Native Touch HUD, Multi-Touch Finger Tracking & Layout Customizer
 */

#include "touch_hud.h"
#include "gl_platform.h"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

namespace SSV {

// ----------------------------------------------------------------------------
// GLSL Shaders for 2D Touch Controls & Layout Editor
// ----------------------------------------------------------------------------
static const char* s_HudVertexShader = 
#ifdef __ANDROID__
"#version 300 es\n"
"precision highp float;\n"
#else
"#version 330 core\n"
#endif
"layout(location = 0) in vec2 aPos;\n"
"layout(location = 1) in vec2 aUV;\n"
"layout(location = 2) in vec4 aColor;\n"
"layout(location = 3) in vec4 aAccentColor;\n"
"layout(location = 4) in float aShape;\n"
"layout(location = 5) in float aPressed;\n"
"layout(location = 6) in float aSelected;\n"
"\n"
"uniform vec2 uResolution;\n"
"uniform float uGlobalOpacity;\n"
"\n"
"out vec2 vUV;\n"
"out vec4 vColor;\n"
"out vec4 vAccentColor;\n"
"out float vShape;\n"
"out float vPressed;\n"
"out float vSelected;\n"
"\n"
"void main() {\n"
"    // Transform pixel screen space [0 .. w, 0 .. h] to NDC [-1 .. 1]\n"
"    vec2 ndc = vec2((aPos.x / uResolution.x) * 2.0 - 1.0, 1.0 - (aPos.y / uResolution.y) * 2.0);\n"
"    gl_Position = vec4(ndc, 0.0, 1.0);\n"
"    vUV = aUV;\n"
"    vColor = vec4(aColor.rgb, aColor.a * uGlobalOpacity);\n"
"    vAccentColor = aAccentColor;\n"
"    vShape = aShape;\n"
"    vPressed = aPressed;\n"
"    vSelected = aSelected;\n"
"}\n";

static const char* s_HudFragmentShader = 
#ifdef __ANDROID__
"#version 300 es\n"
"precision highp float;\n"
#else
"#version 330 core\n"
#endif
"in vec2 vUV;\n"
"in vec4 vColor;\n"
"in vec4 vAccentColor;\n"
"in float vShape;\n"
"in float vPressed;\n"
"in float vSelected;\n"
"\n"
"out vec4 FragColor;\n"
"\n"
"void main() {\n"
"    vec2 uv = vUV;\n"
"    vec2 centered = uv - vec2(0.5);\n"
"    float dist = 0.0;\n"
"    float alpha = 1.0;\n"
"    vec3 finalColor = vColor.rgb;\n"
"\n"
"    // 0: Round Button (P1, P2, Settings, Reset, Cam)\n"
"    if (vShape < 0.5) {\n"
"        dist = length(centered) * 2.0;\n"
"        alpha = 1.0 - smoothstep(0.92, 1.0, dist);\n"
"        // Outer ring glow\n"
"        float ring = smoothstep(0.78, 0.85, dist) * (1.0 - smoothstep(0.95, 1.0, dist));\n"
"        // Inner core symbol glow\n"
"        float core = 1.0 - smoothstep(0.0, 0.45, dist);\n"
"        if (vPressed > 0.5) {\n"
"            finalColor = mix(vColor.rgb, vAccentColor.rgb, 0.85) + ring * vAccentColor.rgb * 0.5;\n"
"        } else {\n"
"            finalColor = vColor.rgb + ring * vAccentColor.rgb * 0.4 + core * vAccentColor.rgb * 0.2;\n"
"        }\n"
"    }\n"
"    // 1: Pedal (Throttle, Brake)\n"
"    else if (vShape < 1.5) {\n"
"        vec2 d = abs(centered) - vec2(0.42, 0.44);\n"
"        dist = length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);\n"
"        alpha = 1.0 - smoothstep(0.0, 0.08, dist);\n"
"        // Horizontal anti-slip grip ribs\n"
"        float rib = sin(uv.y * 38.0);\n"
"        float ribGlow = smoothstep(0.2, 0.8, rib) * 0.25;\n"
"        if (vPressed > 0.5) {\n"
"            finalColor = mix(vColor.rgb, vAccentColor.rgb, 0.85) + ribGlow * vAccentColor.rgb;\n"
"        } else {\n"
"            finalColor = vColor.rgb + ribGlow * vAccentColor.rgb * 0.5;\n"
"        }\n"
"    }\n"
"    // 2: Pill (Handbrake / Drift)\n"
"    else if (vShape < 2.5) {\n"
"        vec2 d = abs(centered) - vec2(0.38, 0.32);\n"
"        dist = length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);\n"
"        alpha = 1.0 - smoothstep(0.0, 0.10, dist);\n"
"        float border = smoothstep(0.0, 0.04, abs(dist - 0.02));\n"
"        if (vPressed > 0.5) {\n"
"            finalColor = mix(vColor.rgb, vAccentColor.rgb, 0.90);\n"
"        } else {\n"
"            finalColor = vColor.rgb + (1.0 - border) * vAccentColor.rgb * 0.4;\n"
"        }\n"
"    }\n"
"    // 3: Arrow Left (Steer Left)\n"
"    else if (vShape < 3.5) {\n"
"        // Rounded rect container\n"
"        vec2 d = abs(centered) - vec2(0.38, 0.38);\n"
"        dist = length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);\n"
"        alpha = 1.0 - smoothstep(0.0, 0.08, dist);\n"
"        // Chevron pointing left: (x - 0.5) + abs(y - 0.5) < 0.25\n"
"        float chev = abs(uv.y - 0.5) * 1.5 + (uv.x - 0.30);\n"
"        float chevronGlow = smoothstep(0.28, 0.24, chev) * smoothstep(0.10, 0.16, chev);\n"
"        if (vPressed > 0.5) {\n"
"            finalColor = mix(vColor.rgb, vAccentColor.rgb, 0.80) + chevronGlow * vec3(1.0);\n"
"        } else {\n"
"            finalColor = vColor.rgb + chevronGlow * vAccentColor.rgb * 0.7;\n"
"        }\n"
"    }\n"
"    // 4: Arrow Right (Steer Right)\n"
"    else if (vShape < 4.5) {\n"
"        vec2 d = abs(centered) - vec2(0.38, 0.38);\n"
"        dist = length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);\n"
"        alpha = 1.0 - smoothstep(0.0, 0.08, dist);\n"
"        // Chevron pointing right: (0.70 - x) + abs(y - 0.5) < 0.25\n"
"        float chev = abs(uv.y - 0.5) * 1.5 + (0.70 - uv.x);\n"
"        float chevronGlow = smoothstep(0.28, 0.24, chev) * smoothstep(0.10, 0.16, chev);\n"
"        if (vPressed > 0.5) {\n"
"            finalColor = mix(vColor.rgb, vAccentColor.rgb, 0.80) + chevronGlow * vec3(1.0);\n"
"        } else {\n"
"            finalColor = vColor.rgb + chevronGlow * vAccentColor.rgb * 0.7;\n"
"        }\n"
"    }\n"
"    // 5: Toolbar Button (Layout Editor)\n"
"    else {\n"
"        vec2 d = abs(centered) - vec2(0.44, 0.38);\n"
"        dist = length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);\n"
"        alpha = 1.0 - smoothstep(0.0, 0.06, dist);\n"
"        finalColor = vColor.rgb;\n"
"    }\n"
"\n"
"    // In Layout Editor Mode: draw pulsing selection outline if selected\n"
"    if (vSelected > 0.5) {\n"
"        float borderDist = abs(max(abs(centered.x), abs(centered.y)) - 0.48);\n"
"        float selectionRing = 1.0 - smoothstep(0.0, 0.05, borderDist);\n"
"        finalColor = mix(finalColor, vec3(1.0, 0.9, 0.1), selectionRing * 0.85);\n"
"    }\n"
"\n"
"    FragColor = vec4(finalColor, alpha * vColor.a);\n"
"}\n";

TouchHUD::TouchHUD() {
    m_fingerMap.fill(-1);
    m_fingerStartX.fill(0.0f);
    m_fingerStartY.fill(0.0f);
}

TouchHUD::~TouchHUD() {
    Shutdown();
}

void TouchHUD::InitDefaultControls() {
    m_controls.clear();

    // 1. Steering Left Arrow (Bottom Left)
    {
        VirtualControl ctrl;
        ctrl.id = TouchControlId::SteerLeft;
        ctrl.name = "steer_left";
        ctrl.label = "LEFT";
        ctrl.normX = 0.04f;
        ctrl.normY = 0.68f;
        ctrl.normW = 0.11f;
        ctrl.normH = 0.22f;
        ctrl.shape = ControlShape::ArrowLeft;
        ctrl.idleColor = Vector4{0.10f, 0.12f, 0.16f, 0.75f};
        ctrl.activeColor = Vector4{0.20f, 0.85f, 1.00f, 0.95f};
        ctrl.accentColor = Vector4{0.00f, 0.80f, 1.00f, 1.00f};
        ctrl.directInputScan = 0xCB; // Left Arrow
        m_controls.push_back(ctrl);
    }

    // 2. Steering Right Arrow (Bottom Left, adjacent)
    {
        VirtualControl ctrl;
        ctrl.id = TouchControlId::SteerRight;
        ctrl.name = "steer_right";
        ctrl.label = "RIGHT";
        ctrl.normX = 0.17f;
        ctrl.normY = 0.68f;
        ctrl.normW = 0.11f;
        ctrl.normH = 0.22f;
        ctrl.shape = ControlShape::ArrowRight;
        ctrl.idleColor = Vector4{0.10f, 0.12f, 0.16f, 0.75f};
        ctrl.activeColor = Vector4{0.20f, 0.85f, 1.00f, 0.95f};
        ctrl.accentColor = Vector4{0.00f, 0.80f, 1.00f, 1.00f};
        ctrl.directInputScan = 0xCD; // Right Arrow
        m_controls.push_back(ctrl);
    }

    // 3. Throttle / Gas Pedal (Bottom Right, far edge)
    {
        VirtualControl ctrl;
        ctrl.id = TouchControlId::Throttle;
        ctrl.name = "throttle";
        ctrl.label = "GAS";
        ctrl.normX = 0.88f;
        ctrl.normY = 0.58f;
        ctrl.normW = 0.09f;
        ctrl.normH = 0.36f;
        ctrl.shape = ControlShape::Pedal;
        ctrl.idleColor = Vector4{0.10f, 0.14f, 0.10f, 0.80f};
        ctrl.activeColor = Vector4{0.20f, 0.90f, 0.35f, 0.95f};
        ctrl.accentColor = Vector4{0.25f, 0.95f, 0.40f, 1.00f};
        ctrl.directInputScan = 0xC8; // Up Arrow
        m_controls.push_back(ctrl);
    }

    // 4. Brake / Reverse Pedal (Bottom Right, inside throttle)
    {
        VirtualControl ctrl;
        ctrl.id = TouchControlId::Brake;
        ctrl.name = "brake";
        ctrl.label = "BRAKE";
        ctrl.normX = 0.76f;
        ctrl.normY = 0.68f;
        ctrl.normW = 0.09f;
        ctrl.normH = 0.26f;
        ctrl.shape = ControlShape::Pedal;
        ctrl.idleColor = Vector4{0.16f, 0.10f, 0.10f, 0.80f};
        ctrl.activeColor = Vector4{0.95f, 0.25f, 0.20f, 0.95f};
        ctrl.accentColor = Vector4{1.00f, 0.30f, 0.25f, 1.00f};
        ctrl.directInputScan = 0xD0; // Down Arrow
        m_controls.push_back(ctrl);
    }

    // 5. Handbrake / Drift Pill (Above brake pedal)
    {
        VirtualControl ctrl;
        ctrl.id = TouchControlId::Handbrake;
        ctrl.name = "handbrake";
        ctrl.label = "DRIFT";
        ctrl.normX = 0.76f;
        ctrl.normY = 0.48f;
        ctrl.normW = 0.09f;
        ctrl.normH = 0.16f;
        ctrl.shape = ControlShape::Pill;
        ctrl.idleColor = Vector4{0.18f, 0.14f, 0.08f, 0.80f};
        ctrl.activeColor = Vector4{1.00f, 0.75f, 0.15f, 0.95f};
        ctrl.accentColor = Vector4{1.00f, 0.80f, 0.20f, 1.00f};
        ctrl.directInputScan = 0x39; // Spacebar
        m_controls.push_back(ctrl);
    }

    // 6. Powerplay Level 1 (Tactical Blue Circle)
    {
        VirtualControl ctrl;
        ctrl.id = TouchControlId::Powerplay1;
        ctrl.name = "powerplay1";
        ctrl.label = "P1";
        ctrl.normX = 0.64f;
        ctrl.normY = 0.68f;
        ctrl.normW = 0.09f;
        ctrl.normH = 0.18f;
        ctrl.shape = ControlShape::Round;
        ctrl.idleColor = Vector4{0.08f, 0.14f, 0.22f, 0.85f};
        ctrl.activeColor = Vector4{0.00f, 0.85f, 1.00f, 1.00f};
        ctrl.accentColor = Vector4{0.10f, 0.90f, 1.00f, 1.00f};
        ctrl.directInputScan = 0x10; // Q / Key 1
        m_controls.push_back(ctrl);
    }

    // 7. Powerplay Level 2 (Cataclysmic Red Circle)
    {
        VirtualControl ctrl;
        ctrl.id = TouchControlId::Powerplay2;
        ctrl.name = "powerplay2";
        ctrl.label = "P2";
        ctrl.normX = 0.64f;
        ctrl.normY = 0.46f;
        ctrl.normW = 0.09f;
        ctrl.normH = 0.18f;
        ctrl.shape = ControlShape::Round;
        ctrl.idleColor = Vector4{0.22f, 0.08f, 0.08f, 0.85f};
        ctrl.activeColor = Vector4{1.00f, 0.25f, 0.10f, 1.00f};
        ctrl.accentColor = Vector4{1.00f, 0.35f, 0.20f, 1.00f};
        ctrl.directInputScan = 0x12; // E / Key 2
        m_controls.push_back(ctrl);
    }

    // 8. Camera Toggle / Rearview (Top Left)
    {
        VirtualControl ctrl;
        ctrl.id = TouchControlId::Camera;
        ctrl.name = "camera";
        ctrl.label = "CAM";
        ctrl.normX = 0.12f;
        ctrl.normY = 0.04f;
        ctrl.normW = 0.07f;
        ctrl.normH = 0.11f;
        ctrl.shape = ControlShape::Round;
        ctrl.idleColor = Vector4{0.12f, 0.14f, 0.18f, 0.70f};
        ctrl.activeColor = Vector4{0.70f, 0.80f, 1.00f, 0.95f};
        ctrl.accentColor = Vector4{0.60f, 0.75f, 0.95f, 1.00f};
        ctrl.directInputScan = 0x2E; // C
        m_controls.push_back(ctrl);
    }

    // 9. Reset Car (Top Left)
    {
        VirtualControl ctrl;
        ctrl.id = TouchControlId::Reset;
        ctrl.name = "reset";
        ctrl.label = "RESET";
        ctrl.normX = 0.03f;
        ctrl.normY = 0.04f;
        ctrl.normW = 0.07f;
        ctrl.normH = 0.11f;
        ctrl.shape = ControlShape::Round;
        ctrl.idleColor = Vector4{0.12f, 0.14f, 0.18f, 0.70f};
        ctrl.activeColor = Vector4{0.95f, 0.70f, 0.20f, 0.95f};
        ctrl.accentColor = Vector4{1.00f, 0.80f, 0.30f, 1.00f};
        ctrl.directInputScan = 0x13; // R
        m_controls.push_back(ctrl);
    }

    // 10. Settings / Layout Customizer Button (Top Right Gear)
    {
        VirtualControl ctrl;
        ctrl.id = TouchControlId::Settings;
        ctrl.name = "settings";
        ctrl.label = "EDIT";
        ctrl.normX = 0.90f;
        ctrl.normY = 0.04f;
        ctrl.normW = 0.07f;
        ctrl.normH = 0.11f;
        ctrl.shape = ControlShape::Round;
        ctrl.idleColor = Vector4{0.15f, 0.15f, 0.18f, 0.75f};
        ctrl.activeColor = Vector4{1.00f, 0.90f, 0.20f, 0.95f};
        ctrl.accentColor = Vector4{1.00f, 0.95f, 0.30f, 1.00f};
        ctrl.directInputScan = 0x01; // ESC
        m_controls.push_back(ctrl);
    }
}

VirtualControl* TouchHUD::FindControl(TouchControlId id) {
    for (auto& ctrl : m_controls) {
        if (ctrl.id == id) return &ctrl;
    }
    return nullptr;
}

bool TouchHUD::Initialize(float screenW, float screenH, const std::string& configFilePath, bool initGL) {
    m_screenWidth = screenW;
    m_screenHeight = screenH;
    m_configPath = configFilePath;

    InitDefaultControls();

    if (!m_configPath.empty()) {
        LoadLayout(m_configPath);
    }

    if (initGL) {
        BuildShader();
    }
    return true;
}

void TouchHUD::Resize(float screenW, float screenH) {
    m_screenWidth = screenW;
    m_screenHeight = screenH;
}

void TouchHUD::Shutdown() {
    if (m_program) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_ebo) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }
    m_glInitialized = false;
}

void TouchHUD::BuildShader() {
    InitGLPlatform();

    GLuint vert = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vert, 1, &s_HudVertexShader, nullptr);
    glCompileShader(vert);

    GLint success = 0;
    glGetShaderiv(vert, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(vert, sizeof(log), nullptr, log);
        std::cerr << "[TouchHUD] Vertex shader compilation failed: " << log << std::endl;
        glDeleteShader(vert);
        return;
    }

    GLuint frag = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(frag, 1, &s_HudFragmentShader, nullptr);
    glCompileShader(frag);
    glGetShaderiv(frag, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(frag, sizeof(log), nullptr, log);
        std::cerr << "[TouchHUD] Fragment shader compilation failed: " << log << std::endl;
        glDeleteShader(vert);
        glDeleteShader(frag);
        return;
    }

    m_program = glCreateProgram();
    glAttachShader(m_program, vert);
    glAttachShader(m_program, frag);
    glLinkProgram(m_program);

    glDeleteShader(vert);
    glDeleteShader(frag);

    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(m_program, sizeof(log), nullptr, log);
        std::cerr << "[TouchHUD] Shader program link failed: " << log << std::endl;
        return;
    }

    m_uResolution = glGetUniformLocation(m_program, "uResolution");
    m_uGlobalOpacity = glGetUniformLocation(m_program, "uGlobalOpacity");

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    constexpr GLsizei stride = sizeof(HudVertex);
    // aPos
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(HudVertex, x));
    // aUV
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(HudVertex, u));
    // aColor
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(HudVertex, color));
    // aAccentColor
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(HudVertex, accentColor));
    // aShape
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(HudVertex, shapeType));
    // aPressed
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(HudVertex, isPressed));
    // aSelected
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(HudVertex, isSelected));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    m_glInitialized = true;
}

void TouchHUD::SetEditMode(bool enable) {
    m_editMode = enable;
    if (!m_editMode) {
        m_selectedControlIndex = -1;
        m_dragPointerId = -1;
        if (!m_configPath.empty()) {
            SaveLayout(m_configPath);
        }
    }
}

void TouchHUD::ToggleEditMode() {
    SetEditMode(!m_editMode);
}

void TouchHUD::SelectControl(TouchControlId id) {
    for (size_t i = 0; i < m_controls.size(); ++i) {
        if (m_controls[i].id == id) {
            m_selectedControlIndex = (int)i;
            return;
        }
    }
}

void TouchHUD::AdjustSelectedSize(float delta) {
    if (m_selectedControlIndex >= 0 && m_selectedControlIndex < (int)m_controls.size()) {
        VirtualControl& ctrl = m_controls[m_selectedControlIndex];
        ctrl.scale = std::clamp(ctrl.scale + delta, 0.50f, 2.00f);
    }
}

void TouchHUD::CycleOpacity() {
    if (m_opacity > 0.80f) {
        m_opacity = 0.40f;
    } else if (m_opacity > 0.50f) {
        m_opacity = 0.85f;
    } else {
        m_opacity = 0.60f;
    }
}

void TouchHUD::OnPointerDown(int32_t pointerId, float screenX, float screenY) {
    if (pointerId >= 0 && (size_t)pointerId < MAX_TRACKED_FINGERS) {
        m_fingerStartX[pointerId] = screenX;
        m_fingerStartY[pointerId] = screenY;
    }

    // 1. In Edit Mode: check toolbar actions along top center
    if (m_editMode) {
        float toolbarY = 0.03f * m_screenHeight;
        float toolbarH = 0.08f * m_screenHeight;
        if (screenY >= toolbarY && screenY <= (toolbarY + toolbarH)) {
            float btnW = 0.10f * m_screenWidth;
            // [SAVE / EXIT] at x = 0.28
            float saveX = 0.28f * m_screenWidth;
            if (screenX >= saveX && screenX <= (saveX + btnW)) {
                SetEditMode(false);
                return;
            }
            // [RESET] at x = 0.40
            float resetX = 0.40f * m_screenWidth;
            if (screenX >= resetX && screenX <= (resetX + btnW)) {
                ResetToDefaultLayout();
                return;
            }
            // [SIZE -] at x = 0.52
            float smX = 0.52f * m_screenWidth;
            if (screenX >= smX && screenX <= (smX + 0.06f * m_screenWidth)) {
                AdjustSelectedSize(-0.10f);
                return;
            }
            // [SIZE +] at x = 0.60
            float spX = 0.60f * m_screenWidth;
            if (screenX >= spX && screenX <= (spX + 0.06f * m_screenWidth)) {
                AdjustSelectedSize(0.10f);
                return;
            }
            // [OPACITY] at x = 0.68
            float opX = 0.68f * m_screenWidth;
            if (screenX >= opX && screenX <= (opX + btnW)) {
                CycleOpacity();
                return;
            }
        }

        // Test which virtual button was touched to select & start dragging
        for (int i = (int)m_controls.size() - 1; i >= 0; --i) {
            if (m_controls[i].Contains(screenX, screenY, m_screenWidth, m_screenHeight)) {
                m_selectedControlIndex = i;
                m_dragPointerId = pointerId;
                float px, py, pw, ph;
                m_controls[i].GetPixelRect(m_screenWidth, m_screenHeight, px, py, pw, ph);
                m_dragOffsetX = screenX - (m_controls[i].normX * m_screenWidth);
                m_dragOffsetY = screenY - (m_controls[i].normY * m_screenHeight);
                return;
            }
        }
        return;
    }

    // 2. Normal Gameplay Mode:
    // Check Settings button first
    auto* settingsBtn = FindControl(TouchControlId::Settings);
    if (settingsBtn && settingsBtn->Contains(screenX, screenY, m_screenWidth, m_screenHeight)) {
        ToggleEditMode();
        return;
    }

    // Check action controls
    for (size_t i = 0; i < m_controls.size(); ++i) {
        if (m_controls[i].Contains(screenX, screenY, m_screenWidth, m_screenHeight)) {
            m_controls[i].isHeld = true;
            m_controls[i].activePointerId = pointerId;
            if (pointerId >= 0 && (size_t)pointerId < MAX_TRACKED_FINGERS) {
                m_fingerMap[pointerId] = (int)i;
            }
            break;
        }
    }
}

void TouchHUD::OnPointerMove(int32_t pointerId, float screenX, float screenY) {
    if (m_editMode && m_dragPointerId == pointerId && m_selectedControlIndex >= 0) {
        VirtualControl& ctrl = m_controls[m_selectedControlIndex];
        float newPixelX = screenX - m_dragOffsetX;
        float newPixelY = screenY - m_dragOffsetY;

        ctrl.normX = std::clamp(newPixelX / m_screenWidth, 0.0f, 1.0f - ctrl.normW);
        ctrl.normY = std::clamp(newPixelY / m_screenHeight, 0.0f, 1.0f - ctrl.normH);
        return;
    }

    // In normal mode: allow sliding across adjacent steering or pedal zones
    if (!m_editMode && pointerId >= 0 && (size_t)pointerId < MAX_TRACKED_FINGERS) {
        int currentCtrlIdx = m_fingerMap[pointerId];
        if (currentCtrlIdx >= 0 && (size_t)currentCtrlIdx < m_controls.size()) {
            // Check if finger drifted to neighbor
            if (!m_controls[currentCtrlIdx].Contains(screenX, screenY, m_screenWidth, m_screenHeight)) {
                m_controls[currentCtrlIdx].isHeld = false;
                m_controls[currentCtrlIdx].activePointerId = -1;
                m_fingerMap[pointerId] = -1;

                for (size_t i = 0; i < m_controls.size(); ++i) {
                    if (m_controls[i].Contains(screenX, screenY, m_screenWidth, m_screenHeight)) {
                        m_controls[i].isHeld = true;
                        m_controls[i].activePointerId = pointerId;
                        m_fingerMap[pointerId] = (int)i;
                        break;
                    }
                }
            }
        }
    }
}

void TouchHUD::OnPointerUp(int32_t pointerId) {
    if (m_editMode && m_dragPointerId == pointerId) {
        m_dragPointerId = -1;
        return;
    }

    if (pointerId >= 0 && (size_t)pointerId < MAX_TRACKED_FINGERS) {
        int ctrlIdx = m_fingerMap[pointerId];
        if (ctrlIdx >= 0 && (size_t)ctrlIdx < m_controls.size()) {
            m_controls[ctrlIdx].isHeld = false;
            m_controls[ctrlIdx].activePointerId = -1;
        }
        m_fingerMap[pointerId] = -1;
    }
}

void TouchHUD::OnCancel() {
    m_fingerMap.fill(-1);
    m_dragPointerId = -1;
    for (auto& ctrl : m_controls) {
        ctrl.isHeld = false;
        ctrl.activePointerId = -1;
    }
}

void TouchHUD::UpdateInputState(InputState& outState, bool& outResetTriggered) {
    outResetTriggered = false;

    for (const auto& ctrl : m_controls) {
        if (!ctrl.isHeld) continue;

        switch (ctrl.id) {
            case TouchControlId::SteerLeft:
                outState.steer -= 1.0f;
                break;
            case TouchControlId::SteerRight:
                outState.steer += 1.0f;
                break;
            case TouchControlId::Throttle:
                outState.throttle = 1.0f;
                break;
            case TouchControlId::Brake:
                outState.brake = 1.0f;
                break;
            case TouchControlId::Handbrake:
                outState.handbrake = true;
                break;
            case TouchControlId::Powerplay1:
                outState.powerplay1 = true;
                break;
            case TouchControlId::Powerplay2:
                outState.powerplay2 = true;
                break;
            case TouchControlId::Camera:
                outState.changeCamera = true;
                break;
            case TouchControlId::Reset:
                outResetTriggered = true;
                break;
            default:
                break;
        }
    }

    outState.steer = std::clamp(outState.steer, -1.0f, 1.0f);
}

void TouchHUD::AppendControlQuad(const VirtualControl& ctrl,
                                 std::vector<HudVertex>& outVertices,
                                 std::vector<uint16_t>& outIndices) {
    float px, py, pw, ph;
    ctrl.GetPixelRect(m_screenWidth, m_screenHeight, px, py, pw, ph);

    uint16_t baseIdx = (uint16_t)outVertices.size();

    float shapeVal = (float)(int)ctrl.shape;
    float pressedVal = ctrl.isHeld ? 1.0f : 0.0f;
    float selectedVal = (m_editMode && m_selectedControlIndex >= 0 &&
                         m_controls[m_selectedControlIndex].id == ctrl.id) ? 1.0f : 0.0f;

    Vector4 col = ctrl.isHeld ? ctrl.activeColor : ctrl.idleColor;

    HudVertex v0{ px,      py,      0.0f, 0.0f, col, ctrl.accentColor, shapeVal, pressedVal, selectedVal };
    HudVertex v1{ px + pw, py,      1.0f, 0.0f, col, ctrl.accentColor, shapeVal, pressedVal, selectedVal };
    HudVertex v2{ px + pw, py + ph, 1.0f, 1.0f, col, ctrl.accentColor, shapeVal, pressedVal, selectedVal };
    HudVertex v3{ px,      py + ph, 0.0f, 1.0f, col, ctrl.accentColor, shapeVal, pressedVal, selectedVal };

    outVertices.push_back(v0);
    outVertices.push_back(v1);
    outVertices.push_back(v2);
    outVertices.push_back(v3);

    outIndices.push_back(baseIdx + 0);
    outIndices.push_back(baseIdx + 1);
    outIndices.push_back(baseIdx + 2);
    outIndices.push_back(baseIdx + 0);
    outIndices.push_back(baseIdx + 2);
    outIndices.push_back(baseIdx + 3);
}

void TouchHUD::AppendToolbarQuad(float x, float y, float w, float h, const std::string& /*label*/, const Vector4& col,
                                 std::vector<HudVertex>& outVertices, std::vector<uint16_t>& outIndices) {
    uint16_t baseIdx = (uint16_t)outVertices.size();

    HudVertex v0{ x,     y,     0.0f, 0.0f, col, Vector4{1, 1, 1, 1}, 5.0f, 0.0f, 0.0f };
    HudVertex v1{ x + w, y,     1.0f, 0.0f, col, Vector4{1, 1, 1, 1}, 5.0f, 0.0f, 0.0f };
    HudVertex v2{ x + w, y + h, 1.0f, 1.0f, col, Vector4{1, 1, 1, 1}, 5.0f, 0.0f, 0.0f };
    HudVertex v3{ x,     y + h, 0.0f, 1.0f, col, Vector4{1, 1, 1, 1}, 5.0f, 0.0f, 0.0f };

    outVertices.push_back(v0);
    outVertices.push_back(v1);
    outVertices.push_back(v2);
    outVertices.push_back(v3);

    outIndices.push_back(baseIdx + 0);
    outIndices.push_back(baseIdx + 1);
    outIndices.push_back(baseIdx + 2);
    outIndices.push_back(baseIdx + 0);
    outIndices.push_back(baseIdx + 2);
    outIndices.push_back(baseIdx + 3);
}

void TouchHUD::Render(float screenW, float screenH) {
    if (!m_glInitialized) {
        BuildShader();
        if (!m_glInitialized) return;
    }

    m_screenWidth = screenW;
    m_screenHeight = screenH;

    std::vector<HudVertex> vertices;
    std::vector<uint16_t> indices;
    vertices.reserve(128);
    indices.reserve(256);

    // 1. Append Virtual Controls
    for (const auto& ctrl : m_controls) {
        AppendControlQuad(ctrl, vertices, indices);
    }

    // 2. In Edit Mode: Append Layout Editor Top Toolbar
    if (m_editMode) {
        float toolbarY = 0.03f * m_screenHeight;
        float toolbarH = 0.08f * m_screenHeight;
        float btnW = 0.10f * m_screenWidth;

        // [DONE / SAVE] Green button
        AppendToolbarQuad(0.28f * m_screenWidth, toolbarY, btnW, toolbarH, "DONE", Vector4{0.15f, 0.65f, 0.25f, 0.90f}, vertices, indices);
        // [RESET] Amber button
        AppendToolbarQuad(0.40f * m_screenWidth, toolbarY, btnW, toolbarH, "RESET", Vector4{0.75f, 0.45f, 0.10f, 0.90f}, vertices, indices);
        // [SIZE -] Cyan button
        AppendToolbarQuad(0.52f * m_screenWidth, toolbarY, 0.06f * m_screenWidth, toolbarH, "SIZE-", Vector4{0.15f, 0.45f, 0.70f, 0.90f}, vertices, indices);
        // [SIZE +] Cyan button
        AppendToolbarQuad(0.60f * m_screenWidth, toolbarY, 0.06f * m_screenWidth, toolbarH, "SIZE+", Vector4{0.15f, 0.45f, 0.70f, 0.90f}, vertices, indices);
        // [OPACITY] Purple button
        AppendToolbarQuad(0.68f * m_screenWidth, toolbarY, btnW, toolbarH, "OPAC", Vector4{0.55f, 0.25f, 0.75f, 0.90f}, vertices, indices);
    }

    if (vertices.empty()) return;

    // Direct GLES 3.0 Alpha Blended Rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glUseProgram(m_program);
    glUniform2f(m_uResolution, screenW, screenH);
    glUniform1f(m_uGlobalOpacity, m_opacity);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(HudVertex), vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint16_t), indices.data(), GL_DYNAMIC_DRAW);

    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_SHORT, 0);

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
}

void TouchHUD::ResetToDefaultLayout() {
    InitDefaultControls();
}

bool TouchHUD::SaveLayout(const std::string& configFilePath) {
    if (configFilePath.empty()) return false;

    std::ofstream out(configFilePath);
    if (!out.is_open()) {
        std::cerr << "[TouchHUD] Failed to write layout file: " << configFilePath << std::endl;
        return false;
    }

    out << "# SSV Virtual Touch Controller Layout Configuration\n";
    out << "opacity " << m_opacity << "\n";

    for (const auto& ctrl : m_controls) {
        out << "control " << ctrl.name << " "
            << ctrl.normX << " "
            << ctrl.normY << " "
            << ctrl.normW << " "
            << ctrl.normH << " "
            << ctrl.scale << "\n";
    }

    out.close();
    std::cout << "[TouchHUD] Successfully saved layout configuration to: " << configFilePath << std::endl;
    return true;
}

bool TouchHUD::LoadLayout(const std::string& configFilePath) {
    if (configFilePath.empty()) return false;

    std::ifstream in(configFilePath);
    if (!in.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::stringstream ss(line);
        std::string token;
        ss >> token;

        if (token == "opacity") {
            float op = 0.85f;
            if (ss >> op) {
                m_opacity = std::clamp(op, 0.20f, 1.0f);
            }
        } else if (token == "control") {
            std::string name;
            float nx = 0, ny = 0, nw = 0, nh = 0, sc = 1.0f;
            if (ss >> name >> nx >> ny >> nw >> nh >> sc) {
                for (auto& ctrl : m_controls) {
                    if (ctrl.name == name) {
                        ctrl.normX = std::clamp(nx, 0.0f, 0.95f);
                        ctrl.normY = std::clamp(ny, 0.0f, 0.95f);
                        ctrl.normW = std::clamp(nw, 0.02f, 0.40f);
                        ctrl.normH = std::clamp(nh, 0.02f, 0.40f);
                        ctrl.scale = std::clamp(sc, 0.50f, 2.00f);
                        break;
                    }
                }
            }
        }
    }

    std::cout << "[TouchHUD] Loaded custom layout from: " << configFilePath << std::endl;
    return true;
}

} // namespace SSV
