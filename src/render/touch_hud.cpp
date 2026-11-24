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
"uniform sampler2D uAtlas;\n"
"\n"
"out vec4 FragColor;\n"
"\n"
"void main() {\n"
"    // ------------------------------------------------------------------------\n"
"    // Mode 5.0+: Textured Atlas Quad (Font Glyph or Vector Icon)\n"
"    // ------------------------------------------------------------------------\n"
"    if (vShape > 4.5) {\n"
"        float alphaMask = texture(uAtlas, vUV).a;\n"
"        if (alphaMask < 0.01) discard;\n"
"        FragColor = vec4(vColor.rgb, vColor.a * alphaMask);\n"
"        return;\n"
"    }\n"
"\n"
"    // ------------------------------------------------------------------------\n"
"    // Mode 0.0 .. 4.0: High-End Translucent Dark Glass Control Card\n"
"    // ------------------------------------------------------------------------\n"
"    vec2 centered = vUV - vec2(0.5);\n"
"    float dist = 0.0;\n"
"    float alpha = 1.0;\n"
"    float border = 0.0;\n"
"\n"
"    // 0: Round / Circular Action Card (P1, P2, Cam, Reset, Settings)\n"
"    if (vShape < 0.5) {\n"
"        dist = length(centered) * 2.0;\n"
"        alpha = 1.0 - smoothstep(0.95, 1.0, dist);\n"
"        border = smoothstep(0.86, 0.93, dist) * (1.0 - smoothstep(0.97, 1.0, dist));\n"
"    }\n"
"    // 1: Pedal Card (Gas, Brake)\n"
"    else if (vShape < 1.5) {\n"
"        vec2 d = abs(centered) - vec2(0.40, 0.44);\n"
"        dist = length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);\n"
"        alpha = 1.0 - smoothstep(0.0, 0.07, dist);\n"
"        border = 1.0 - smoothstep(0.0, 0.035, abs(dist - 0.015));\n"
"    }\n"
"    // 2: Pill Card (Handbrake / Drift)\n"
"    else if (vShape < 2.5) {\n"
"        vec2 d = abs(centered) - vec2(0.38, 0.32);\n"
"        dist = length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);\n"
"        alpha = 1.0 - smoothstep(0.0, 0.08, dist);\n"
"        border = 1.0 - smoothstep(0.0, 0.035, abs(dist - 0.015));\n"
"    }\n"
"    // 3: Rounded Rectangle Card (Steer Left, Steer Right)\n"
"    else if (vShape < 3.5) {\n"
"        vec2 d = abs(centered) - vec2(0.40, 0.40);\n"
"        dist = length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);\n"
"        alpha = 1.0 - smoothstep(0.0, 0.07, dist);\n"
"        border = 1.0 - smoothstep(0.0, 0.035, abs(dist - 0.015));\n"
"    }\n"
"    // 4: Toolbar Card (Edit Mode)\n"
"    else {\n"
"        vec2 d = abs(centered) - vec2(0.44, 0.38);\n"
"        dist = length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);\n"
"        alpha = 1.0 - smoothstep(0.0, 0.06, dist);\n"
"        border = 1.0 - smoothstep(0.0, 0.03, abs(dist - 0.01));\n"
"    }\n"
"\n"
"    if (alpha <= 0.005) discard;\n"
"\n"
"    // Sleek frosted glass gradient: lighter at top, deeper dark at bottom\n"
"    float topGradient = clamp((0.5 - centered.y) * 0.15, -0.05, 0.15);\n"
"    vec3 baseGlass = vColor.rgb + vec3(topGradient);\n"
"\n"
"    vec3 finalColor;\n"
"    if (vPressed > 0.5) {\n"
"        // Pressed state: vibrant glowing neon border and rich saturated glass\n"
"        vec3 glowBorder = mix(vColor.rgb, vAccentColor.rgb, 0.95);\n"
"        finalColor = mix(baseGlass, glowBorder, border * 0.90);\n"
"    } else {\n"
"        // Idle state: subtle crisp stroke\n"
"        vec3 strokeCol = mix(vec3(1.0), vAccentColor.rgb, 0.25);\n"
"        finalColor = mix(baseGlass, strokeCol, border * 0.55);\n"
"    }\n"
"\n"
"    // Edit Mode Selection highlight (pulsing gold outline)\n"
"    if (vSelected > 0.5) {\n"
"        float selectDist = abs(max(abs(centered.x), abs(centered.y)) - 0.46);\n"
"        float selectRing = 1.0 - smoothstep(0.0, 0.04, selectDist);\n"
"        finalColor = mix(finalColor, vec3(1.0, 0.85, 0.1), selectRing * 0.90);\n"
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

    // 1. Steering Left Arrow Card (Bottom Left)
    {
        VirtualControl ctrl;
        ctrl.id = TouchControlId::SteerLeft;
        ctrl.name = "steer_left";
        ctrl.label = "LEFT";
        ctrl.normX = 0.03f;
        ctrl.normY = 0.66f;
        ctrl.normW = 0.11f;
        ctrl.normH = 0.22f;
        ctrl.shape = ControlShape::Card;
        ctrl.idleColor = Vector4{0.07f, 0.10f, 0.15f, 0.65f};
        ctrl.activeColor = Vector4{0.12f, 0.24f, 0.32f, 0.88f};
        ctrl.accentColor = Vector4{0.00f, 0.85f, 1.00f, 1.00f};
        ctrl.directInputScan = 0xCB; // Left Arrow
        m_controls.push_back(ctrl);
    }

    // 2. Steering Right Arrow Card (Bottom Left, adjacent)
    {
        VirtualControl ctrl;
        ctrl.id = TouchControlId::SteerRight;
        ctrl.name = "steer_right";
        ctrl.label = "RIGHT";
        ctrl.normX = 0.15f;
        ctrl.normY = 0.66f;
        ctrl.normW = 0.11f;
        ctrl.normH = 0.22f;
        ctrl.shape = ControlShape::Card;
        ctrl.idleColor = Vector4{0.07f, 0.10f, 0.15f, 0.65f};
        ctrl.activeColor = Vector4{0.12f, 0.24f, 0.32f, 0.88f};
        ctrl.accentColor = Vector4{0.00f, 0.85f, 1.00f, 1.00f};
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
        ctrl.normY = 0.56f;
        ctrl.normW = 0.09f;
        ctrl.normH = 0.38f;
        ctrl.shape = ControlShape::Pedal;
        ctrl.idleColor = Vector4{0.06f, 0.12f, 0.08f, 0.70f};
        ctrl.activeColor = Vector4{0.12f, 0.32f, 0.16f, 0.90f};
        ctrl.accentColor = Vector4{0.22f, 0.95f, 0.35f, 1.00f};
        ctrl.directInputScan = 0xC8; // Up Arrow
        m_controls.push_back(ctrl);
    }

    // 4. Brake / Reverse Pedal (Bottom Right, inside throttle)
    {
        VirtualControl ctrl;
        ctrl.id = TouchControlId::Brake;
        ctrl.name = "brake";
        ctrl.label = "BRAKE";
        ctrl.normX = 0.77f;
        ctrl.normY = 0.68f;
        ctrl.normW = 0.09f;
        ctrl.normH = 0.26f;
        ctrl.shape = ControlShape::Pedal;
        ctrl.idleColor = Vector4{0.14f, 0.07f, 0.07f, 0.70f};
        ctrl.activeColor = Vector4{0.34f, 0.12f, 0.12f, 0.90f};
        ctrl.accentColor = Vector4{1.00f, 0.25f, 0.20f, 1.00f};
        ctrl.directInputScan = 0xD0; // Down Arrow
        m_controls.push_back(ctrl);
    }

    // 5. Handbrake / Drift Pill (Above brake pedal)
    {
        VirtualControl ctrl;
        ctrl.id = TouchControlId::Handbrake;
        ctrl.name = "handbrake";
        ctrl.label = "DRIFT";
        ctrl.normX = 0.77f;
        ctrl.normY = 0.48f;
        ctrl.normW = 0.09f;
        ctrl.normH = 0.17f;
        ctrl.shape = ControlShape::Pill;
        ctrl.idleColor = Vector4{0.14f, 0.10f, 0.05f, 0.70f};
        ctrl.activeColor = Vector4{0.32f, 0.22f, 0.08f, 0.90f};
        ctrl.accentColor = Vector4{1.00f, 0.70f, 0.10f, 1.00f};
        ctrl.directInputScan = 0x39; // Spacebar
        m_controls.push_back(ctrl);
    }

    // 6. Powerplay Level 1 (Tactical Blue Circle)
    {
        VirtualControl ctrl;
        ctrl.id = TouchControlId::Powerplay1;
        ctrl.name = "powerplay1";
        ctrl.label = "P1";
        ctrl.normX = 0.65f;
        ctrl.normY = 0.70f;
        ctrl.normW = 0.09f;
        ctrl.normH = 0.18f;
        ctrl.shape = ControlShape::Round;
        ctrl.idleColor = Vector4{0.06f, 0.12f, 0.20f, 0.75f};
        ctrl.activeColor = Vector4{0.10f, 0.28f, 0.45f, 0.92f};
        ctrl.accentColor = Vector4{0.00f, 0.85f, 1.00f, 1.00f};
        ctrl.directInputScan = 0x10; // Q / Key 1
        m_controls.push_back(ctrl);
    }

    // 7. Powerplay Level 2 (Cataclysmic Red Circle)
    {
        VirtualControl ctrl;
        ctrl.id = TouchControlId::Powerplay2;
        ctrl.name = "powerplay2";
        ctrl.label = "P2";
        ctrl.normX = 0.65f;
        ctrl.normY = 0.48f;
        ctrl.normW = 0.09f;
        ctrl.normH = 0.18f;
        ctrl.shape = ControlShape::Round;
        ctrl.idleColor = Vector4{0.18f, 0.06f, 0.06f, 0.75f};
        ctrl.activeColor = Vector4{0.45f, 0.12f, 0.10f, 0.92f};
        ctrl.accentColor = Vector4{1.00f, 0.20f, 0.05f, 1.00f};
        ctrl.directInputScan = 0x12; // E / Key 2
        m_controls.push_back(ctrl);
    }

    // 8. Camera Toggle / Rearview (Top Left)
    {
        VirtualControl ctrl;
        ctrl.id = TouchControlId::Camera;
        ctrl.name = "camera";
        ctrl.label = "CAM";
        ctrl.normX = 0.11f;
        ctrl.normY = 0.04f;
        ctrl.normW = 0.065f;
        ctrl.normH = 0.11f;
        ctrl.shape = ControlShape::Round;
        ctrl.idleColor = Vector4{0.08f, 0.10f, 0.14f, 0.65f};
        ctrl.activeColor = Vector4{0.16f, 0.24f, 0.36f, 0.88f};
        ctrl.accentColor = Vector4{0.65f, 0.80f, 1.00f, 1.00f};
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
        ctrl.normW = 0.065f;
        ctrl.normH = 0.11f;
        ctrl.shape = ControlShape::Round;
        ctrl.idleColor = Vector4{0.08f, 0.10f, 0.14f, 0.65f};
        ctrl.activeColor = Vector4{0.30f, 0.22f, 0.08f, 0.88f};
        ctrl.accentColor = Vector4{1.00f, 0.85f, 0.25f, 1.00f};
        ctrl.directInputScan = 0x13; // R
        m_controls.push_back(ctrl);
    }

    // 10. Settings / Layout Customizer Button (Top Right Gear)
    {
        VirtualControl ctrl;
        ctrl.id = TouchControlId::Settings;
        ctrl.name = "settings";
        ctrl.label = "EDIT";
        ctrl.normX = 0.91f;
        ctrl.normY = 0.04f;
        ctrl.normW = 0.065f;
        ctrl.normH = 0.11f;
        ctrl.shape = ControlShape::Round;
        ctrl.idleColor = Vector4{0.10f, 0.10f, 0.12f, 0.65f};
        ctrl.activeColor = Vector4{0.32f, 0.28f, 0.08f, 0.88f};
        ctrl.accentColor = Vector4{1.00f, 0.90f, 0.20f, 1.00f};
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
    if (m_atlasTexture) {
        glDeleteTextures(1, &m_atlasTexture);
        m_atlasTexture = 0;
    }
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
    m_uAtlas = glGetUniformLocation(m_program, "uAtlas");

    // Setup Geometry Buffers
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    constexpr GLsizei stride = sizeof(HudVertex);
    // aPos (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(HudVertex, x));
    // aUV (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(HudVertex, u));
    // aColor (location 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(HudVertex, color));
    // aAccentColor (location 3)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(HudVertex, accentColor));
    // aShape (location 4)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(HudVertex, shapeType));
    // aPressed (location 5)
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(HudVertex, isPressed));
    // aSelected (location 6)
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(HudVertex, isSelected));

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Setup Font & Vector Icon Atlas Texture
    glGenTextures(1, &m_atlasTexture);
    glBindTexture(GL_TEXTURE_2D, m_atlasTexture);

    std::vector<uint8_t> rgbaBuffer(HUD_ATLAS_WIDTH * HUD_ATLAS_HEIGHT * 4, 255);
    size_t outIdx = 0;
    for (size_t i = 0; i < sizeof(g_HudAtlasRLE); i += 2) {
        uint8_t count = g_HudAtlasRLE[i];
        uint8_t alpha = g_HudAtlasRLE[i + 1];
        for (uint8_t k = 0; k < count; ++k) {
            rgbaBuffer[outIdx + 0] = 255;
            rgbaBuffer[outIdx + 1] = 255;
            rgbaBuffer[outIdx + 2] = 255;
            rgbaBuffer[outIdx + 3] = alpha;
            outIdx += 4;
        }
    }

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, HUD_ATLAS_WIDTH, HUD_ATLAS_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgbaBuffer.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);

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
            // [SAVE / DONE] at x = 0.28
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

void TouchHUD::AppendCardQuad(float px, float py, float pw, float ph, float shapeType,
                             const Vector4& fillColor, const Vector4& strokeColor,
                             bool isPressed, bool isSelected,
                             std::vector<HudVertex>& outVertices, std::vector<uint16_t>& outIndices) {
    uint16_t baseIdx = (uint16_t)outVertices.size();

    float pressedVal = isPressed ? 1.0f : 0.0f;
    float selectedVal = isSelected ? 1.0f : 0.0f;

    HudVertex v0{ px,      py,      0.0f, 0.0f, fillColor, strokeColor, shapeType, pressedVal, selectedVal };
    HudVertex v1{ px + pw, py,      1.0f, 0.0f, fillColor, strokeColor, shapeType, pressedVal, selectedVal };
    HudVertex v2{ px + pw, py + ph, 1.0f, 1.0f, fillColor, strokeColor, shapeType, pressedVal, selectedVal };
    HudVertex v3{ px,      py + ph, 0.0f, 1.0f, fillColor, strokeColor, shapeType, pressedVal, selectedVal };

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

void TouchHUD::AppendAtlasQuad(float px, float py, float pw, float ph,
                              float u0, float v0, float u1, float v1,
                              const Vector4& color,
                              std::vector<HudVertex>& outVertices, std::vector<uint16_t>& outIndices) {
    uint16_t baseIdx = (uint16_t)outVertices.size();

    HudVertex vert0{ px,      py,      u0, v0, color, Vector4{1, 1, 1, 1}, 5.0f, 0.0f, 0.0f };
    HudVertex vert1{ px + pw, py,      u1, v0, color, Vector4{1, 1, 1, 1}, 5.0f, 0.0f, 0.0f };
    HudVertex vert2{ px + pw, py + ph, u1, v1, color, Vector4{1, 1, 1, 1}, 5.0f, 0.0f, 0.0f };
    HudVertex vert3{ px,      py + ph, u0, v1, color, Vector4{1, 1, 1, 1}, 5.0f, 0.0f, 0.0f };

    outVertices.push_back(vert0);
    outVertices.push_back(vert1);
    outVertices.push_back(vert2);
    outVertices.push_back(vert3);

    outIndices.push_back(baseIdx + 0);
    outIndices.push_back(baseIdx + 1);
    outIndices.push_back(baseIdx + 2);
    outIndices.push_back(baseIdx + 0);
    outIndices.push_back(baseIdx + 2);
    outIndices.push_back(baseIdx + 3);
}

void TouchHUD::AppendIcon(HudIconId icon, float cx, float cy, float size,
                          const Vector4& color,
                          std::vector<HudVertex>& outVertices, std::vector<uint16_t>& outIndices) {
    int idx = (int)icon;
    if (idx < 0 || idx >= 16) return;
    const auto& uv = g_IconUVs[idx];
    float x = cx - size * 0.5f;
    float y = cy - size * 0.5f;
    AppendAtlasQuad(x, y, size, size, uv.u0, uv.v0, uv.u1, uv.v1, color, outVertices, outIndices);
}

void TouchHUD::AppendText(const std::string& text, float cx, float cy, float scale,
                          const Vector4& color,
                          std::vector<HudVertex>& outVertices, std::vector<uint16_t>& outIndices) {
    if (text.empty()) return;

    float totalWidth = 0.0f;
    for (char c : text) {
        int idx = (int)(unsigned char)c - HUD_GLYPH_START;
        if (idx >= 0 && idx < HUD_GLYPH_COUNT) {
            totalWidth += g_GlyphMetrics[idx].advance * scale;
        } else {
            totalWidth += 8.0f * scale;
        }
    }

    float startX = cx - totalWidth * 0.5f;
    float startY = cy - (13.0f * scale);

    float curX = startX;
    for (char c : text) {
        int idx = (int)(unsigned char)c - HUD_GLYPH_START;
        if (idx >= 0 && idx < HUD_GLYPH_COUNT) {
            const auto& m = g_GlyphMetrics[idx];
            if (m.width > 0.0f) {
                float gw = m.width * scale;
                float gh = m.height * scale;
                AppendAtlasQuad(curX, startY, gw, gh, m.u0, m.v0, m.u1, m.v1, color, outVertices, outIndices);
            }
            curX += m.advance * scale;
        } else {
            curX += 8.0f * scale;
        }
    }
}

void TouchHUD::AppendControlQuad(const VirtualControl& ctrl,
                                 std::vector<HudVertex>& outVertices,
                                 std::vector<uint16_t>& outIndices) {
    float px, py, pw, ph;
    ctrl.GetPixelRect(m_screenWidth, m_screenHeight, px, py, pw, ph);

    float cx = px + pw * 0.5f;
    float cy = py + ph * 0.5f;

    bool isPressed = ctrl.isHeld;
    bool isSelected = (m_editMode && m_selectedControlIndex >= 0 &&
                       m_controls[m_selectedControlIndex].id == ctrl.id);

    // 1. Base Glass Card
    float shapeVal = (float)(int)ctrl.shape;
    Vector4 cardFill = isPressed ? ctrl.activeColor : ctrl.idleColor;
    AppendCardQuad(px, py, pw, ph, shapeVal, cardFill, ctrl.accentColor, isPressed, isSelected, outVertices, outIndices);

    // 2. High-Impact Ink Color
    Vector4 inkColor = isPressed ? ctrl.accentColor : Vector4{0.90f, 0.93f, 0.98f, 0.88f};

    // 3. Render Crisp Vector Icons & Bold Typography
    switch (ctrl.id) {
        case TouchControlId::Throttle: {
            float iconSize = std::min(pw * 0.72f, ph * 0.40f);
            AppendIcon(HudIconId::GasChevrons, cx, cy - ph * 0.16f, iconSize, inkColor, outVertices, outIndices);
            AppendText("GAS", cx, cy + ph * 0.28f, 1.25f, inkColor, outVertices, outIndices);
            break;
        }
        case TouchControlId::Brake: {
            float iconSize = std::min(pw * 0.72f, ph * 0.40f);
            AppendIcon(HudIconId::BrakeChevrons, cx, cy - ph * 0.16f, iconSize, inkColor, outVertices, outIndices);
            AppendText("BRAKE", cx, cy + ph * 0.26f, 1.05f, inkColor, outVertices, outIndices);
            break;
        }
        case TouchControlId::SteerLeft: {
            float iconSize = std::min(pw * 0.55f, ph * 0.50f);
            AppendIcon(HudIconId::SteerLeft, cx, cy - ph * 0.10f, iconSize, inkColor, outVertices, outIndices);
            AppendText("LEFT", cx, cy + ph * 0.28f, 0.95f, inkColor, outVertices, outIndices);
            break;
        }
        case TouchControlId::SteerRight: {
            float iconSize = std::min(pw * 0.55f, ph * 0.50f);
            AppendIcon(HudIconId::SteerRight, cx, cy - ph * 0.10f, iconSize, inkColor, outVertices, outIndices);
            AppendText("RIGHT", cx, cy + ph * 0.28f, 0.95f, inkColor, outVertices, outIndices);
            break;
        }
        case TouchControlId::Handbrake: {
            float iconSize = std::min(pw * 0.55f, ph * 0.50f);
            AppendIcon(HudIconId::Drift, cx, cy - ph * 0.12f, iconSize, inkColor, outVertices, outIndices);
            AppendText("DRIFT", cx, cy + ph * 0.25f, 0.95f, inkColor, outVertices, outIndices);
            break;
        }
        case TouchControlId::Powerplay1: {
            float iconSize = std::min(pw * 0.78f, ph * 0.78f);
            AppendIcon(HudIconId::TacticalP1, cx, cy - ph * 0.05f, iconSize, inkColor, outVertices, outIndices);
            AppendText("LVL 1", cx, cy + ph * 0.33f, 0.85f, inkColor, outVertices, outIndices);
            break;
        }
        case TouchControlId::Powerplay2: {
            float iconSize = std::min(pw * 0.78f, ph * 0.78f);
            AppendIcon(HudIconId::CataclysmP2, cx, cy - ph * 0.05f, iconSize, inkColor, outVertices, outIndices);
            AppendText("LVL 2", cx, cy + ph * 0.33f, 0.85f, inkColor, outVertices, outIndices);
            break;
        }
        case TouchControlId::Camera: {
            float iconSize = std::min(pw * 0.55f, ph * 0.50f);
            AppendIcon(HudIconId::Camera, cx, cy - ph * 0.10f, iconSize, inkColor, outVertices, outIndices);
            AppendText("CAM", cx, cy + ph * 0.27f, 0.80f, inkColor, outVertices, outIndices);
            break;
        }
        case TouchControlId::Reset: {
            float iconSize = std::min(pw * 0.55f, ph * 0.50f);
            AppendIcon(HudIconId::Reset, cx, cy - ph * 0.10f, iconSize, inkColor, outVertices, outIndices);
            AppendText("RESET", cx, cy + ph * 0.27f, 0.70f, inkColor, outVertices, outIndices);
            break;
        }
        case TouchControlId::Settings: {
            float iconSize = std::min(pw * 0.55f, ph * 0.50f);
            AppendIcon(HudIconId::Settings, cx, cy - ph * 0.10f, iconSize, inkColor, outVertices, outIndices);
            AppendText("EDIT", cx, cy + ph * 0.27f, 0.80f, inkColor, outVertices, outIndices);
            break;
        }
        default:
            break;
    }
}

void TouchHUD::AppendToolbarButton(float x, float y, float w, float h,
                                   const std::string& label, HudIconId icon,
                                   const Vector4& btnColor, const Vector4& accentColor,
                                   std::vector<HudVertex>& outVertices, std::vector<uint16_t>& outIndices) {
    AppendCardQuad(x, y, w, h, 4.0f, btnColor, accentColor, false, false, outVertices, outIndices);

    float cx = x + w * 0.5f;
    float cy = y + h * 0.5f;

    if (icon != HudIconId::Count) {
        float iconSize = h * 0.50f;
        if (!label.empty()) {
            AppendIcon(icon, cx - w * 0.24f, cy, iconSize, Vector4{1, 1, 1, 1}, outVertices, outIndices);
            AppendText(label, cx + w * 0.16f, cy, 0.90f, Vector4{1, 1, 1, 1}, outVertices, outIndices);
        } else {
            AppendIcon(icon, cx, cy, iconSize, Vector4{1, 1, 1, 1}, outVertices, outIndices);
        }
    } else {
        AppendText(label, cx, cy, 0.95f, Vector4{1, 1, 1, 1}, outVertices, outIndices);
    }
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
    vertices.reserve(512);
    indices.reserve(1024);

    // 1. Append Virtual Controls
    for (const auto& ctrl : m_controls) {
        AppendControlQuad(ctrl, vertices, indices);
    }

    // 2. In Edit Mode: Append Sleek Top Toolbar Card (inspired by NFSU2 Revived)
    if (m_editMode) {
        float toolbarY = 0.025f * m_screenHeight;
        float toolbarH = 0.085f * m_screenHeight;
        float cardX = 0.20f * m_screenWidth;
        float cardW = 0.60f * m_screenWidth;

        // Container Card
        AppendCardQuad(cardX, toolbarY, cardW, toolbarH, 4.0f,
                       Vector4{0.06f, 0.09f, 0.14f, 0.92f}, Vector4{0.20f, 0.85f, 1.00f, 0.80f},
                       false, false, vertices, indices);

        // Hamburger Drag Handle (Left)
        float handleW = 0.045f * m_screenWidth;
        AppendCardQuad(cardX + 0.008f * m_screenWidth, toolbarY + 0.01f * m_screenHeight,
                       handleW, toolbarH - 0.02f * m_screenHeight, 4.0f,
                       Vector4{0.12f, 0.16f, 0.22f, 0.85f}, Vector4{1, 1, 1, 0.35f},
                       false, false, vertices, indices);
        AppendIcon(HudIconId::Hamburger, cardX + 0.008f * m_screenWidth + handleW * 0.5f,
                   toolbarY + toolbarH * 0.5f, toolbarH * 0.45f,
                   Vector4{0.00f, 0.85f, 1.00f, 1.0f}, vertices, indices);

        // Buttons
        float btnY = toolbarY + 0.012f * m_screenHeight;
        float btnH = toolbarH - 0.024f * m_screenHeight;
        float btnW = 0.095f * m_screenWidth;

        // [SAVE] Green
        AppendToolbarButton(cardX + 0.065f * m_screenWidth, btnY, btnW, btnH, "SAVE", HudIconId::Checkmark,
                            Vector4{0.10f, 0.32f, 0.16f, 0.90f}, Vector4{0.25f, 0.95f, 0.40f, 1.0f},
                            vertices, indices);

        // [RESET] Amber
        AppendToolbarButton(cardX + 0.170f * m_screenWidth, btnY, btnW, btnH, "RESET", HudIconId::Reset,
                            Vector4{0.32f, 0.20f, 0.06f, 0.90f}, Vector4{1.00f, 0.75f, 0.20f, 1.0f},
                            vertices, indices);

        // [SIZE -] Cyan
        float smW = 0.055f * m_screenWidth;
        AppendToolbarButton(cardX + 0.275f * m_screenWidth, btnY, smW, btnH, "", HudIconId::Minus,
                            Vector4{0.10f, 0.22f, 0.32f, 0.90f}, Vector4{0.20f, 0.85f, 1.00f, 1.0f},
                            vertices, indices);

        // [SIZE +] Cyan
        AppendToolbarButton(cardX + 0.340f * m_screenWidth, btnY, smW, btnH, "", HudIconId::Plus,
                            Vector4{0.10f, 0.22f, 0.32f, 0.90f}, Vector4{0.20f, 0.85f, 1.00f, 1.0f},
                            vertices, indices);

        // [OPACITY] Purple
        int opacPercent = (int)std::round(m_opacity * 100.0f);
        std::string opacLabel = std::to_string(opacPercent) + "%";
        AppendToolbarButton(cardX + 0.405f * m_screenWidth, btnY, btnW, btnH, opacLabel, HudIconId::Opacity,
                            Vector4{0.26f, 0.14f, 0.36f, 0.90f}, Vector4{0.80f, 0.45f, 1.00f, 1.0f},
                            vertices, indices);

        // Helper Subtitle Banner below Toolbar
        AppendText("DRAG TO POSITION - TAP BUTTON TO RESIZE", cardX + cardW * 0.5f, toolbarY + toolbarH + 16.0f,
                   0.85f, Vector4{1.0f, 1.0f, 1.0f, 0.75f}, vertices, indices);
    }

    if (vertices.empty()) return;

    // Direct GLES 3.0 / GL 3.3 Alpha Blended Rendering
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    glUseProgram(m_program);
    glUniform2f(m_uResolution, screenW, screenH);
    glUniform1f(m_uGlobalOpacity, m_opacity);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_atlasTexture);
    glUniform1i(m_uAtlas, 0);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(HudVertex), vertices.data(), GL_DYNAMIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint16_t), indices.data(), GL_DYNAMIC_DRAW);

    glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_SHORT, 0);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
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
        out << "control " << (int)ctrl.id << " "
            << ctrl.normX << " "
            << ctrl.normY << " "
            << ctrl.normW << " "
            << ctrl.normH << " "
            << ctrl.scale << "\n";
    }

    out.close();
    return true;
}

bool TouchHUD::LoadLayout(const std::string& configFilePath) {
    if (configFilePath.empty()) return false;

    std::ifstream in(configFilePath);
    if (!in.is_open()) return false;

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "opacity") {
            float op = 0.85f;
            if (iss >> op) {
                SetOpacity(op);
            }
        } else if (token == "control") {
            int idInt = 0;
            float nx, ny, nw, nh, sc;
            if (iss >> idInt >> nx >> ny >> nw >> nh >> sc) {
                auto* ctrl = FindControl((TouchControlId)idInt);
                if (ctrl) {
                    ctrl->normX = nx;
                    ctrl->normY = ny;
                    ctrl->normW = nw;
                    ctrl->normH = nh;
                    ctrl->scale = std::clamp(sc, 0.50f, 2.00f);
                }
            }
        }
    }

    in.close();
    return true;
}

} // namespace SSV
