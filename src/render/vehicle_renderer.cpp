/**
 * @file vehicle_renderer.cpp
 * @brief Implementation of 3D Vehicle & Flat Plane Renderer
 */

#include "vehicle_renderer.h"
#include <cmath>
#include <iostream>

namespace SSV {

VehicleRenderer::VehicleRenderer() = default;

VehicleRenderer::~VehicleRenderer() {
    Shutdown();
}

bool VehicleRenderer::Initialize() {
    if (!InitGLPlatform()) {
        std::cerr << "[VehicleRenderer] Failed to initialize GL platform.\n";
        return false;
    }

    if (!CompileShaders()) {
        std::cerr << "[VehicleRenderer] Failed to compile shaders.\n";
        return false;
    }

    BuildGroundMesh();
    BuildChassisMesh();
    BuildWheelMesh();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    m_initialized = true;
    std::cout << "[VehicleRenderer] Successfully initialized vehicle & environment renderer.\n";
    return true;
}

void VehicleRenderer::Shutdown() {
    if (!m_initialized) return;

    if (m_groundVao) { glDeleteVertexArrays(1, &m_groundVao); m_groundVao = 0; }
    if (m_groundVbo) { glDeleteBuffers(1, &m_groundVbo); m_groundVbo = 0; }
    if (m_groundEbo) { glDeleteBuffers(1, &m_groundEbo); m_groundEbo = 0; }

    if (m_chassisVao) { glDeleteVertexArrays(1, &m_chassisVao); m_chassisVao = 0; }
    if (m_chassisVbo) { glDeleteBuffers(1, &m_chassisVbo); m_chassisVbo = 0; }
    if (m_chassisEbo) { glDeleteBuffers(1, &m_chassisEbo); m_chassisEbo = 0; }

    if (m_wheelVao) { glDeleteVertexArrays(1, &m_wheelVao); m_wheelVao = 0; }
    if (m_wheelVbo) { glDeleteBuffers(1, &m_wheelVbo); m_wheelVbo = 0; }
    if (m_wheelEbo) { glDeleteBuffers(1, &m_wheelEbo); m_wheelEbo = 0; }

    if (m_program) { glDeleteProgram(m_program); m_program = 0; }

    m_initialized = false;
}

bool VehicleRenderer::CompileShaders() {
#ifdef __ANDROID__
    const char* vsSource =
        "#version 300 es\n"
        "precision highp float;\n"
        "layout(location = 0) in vec3 aPosition;\n"
        "layout(location = 1) in vec3 aNormal;\n"
        "layout(location = 2) in vec4 aColor;\n"
        "uniform mat4 uViewProj;\n"
        "uniform mat4 uModel;\n"
        "out vec3 vNormal;\n"
        "out vec3 vWorldPos;\n"
        "out vec4 vColor;\n"
        "void main() {\n"
        "    vec4 worldPos = uModel * vec4(aPosition, 1.0);\n"
        "    vWorldPos = worldPos.xyz;\n"
        "    vNormal = mat3(uModel) * aNormal;\n"
        "    vColor = aColor;\n"
        "    gl_Position = uViewProj * worldPos;\n"
        "}\n";

    const char* fsSource =
        "#version 300 es\n"
        "precision mediump float;\n"
        "in vec3 vNormal;\n"
        "in vec3 vWorldPos;\n"
        "in vec4 vColor;\n"
        "uniform vec3 uLightDir;\n"
        "uniform vec3 uEyePos;\n"
        "uniform vec4 uTint;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "    vec3 N = normalize(vNormal);\n"
        "    vec3 L = normalize(uLightDir);\n"
        "    vec3 V = normalize(uEyePos - vWorldPos);\n"
        "    vec3 H = normalize(L + V);\n"
        "    float diff = max(dot(N, L), 0.0);\n"
        "    float spec = pow(max(dot(N, H), 0.0), 24.0) * 0.40;\n"
        "    float ambient = 0.35;\n"
        "    vec3 rgb = (vColor.rgb * uTint.rgb) * (ambient + diff * 0.65) + vec3(spec);\n"
        "    fragColor = vec4(rgb, vColor.a * uTint.a);\n"
        "}\n";
#else
    const char* vsSource =
        "#version 330 core\n"
        "layout(location = 0) in vec3 aPosition;\n"
        "layout(location = 1) in vec3 aNormal;\n"
        "layout(location = 2) in vec4 aColor;\n"
        "uniform mat4 uViewProj;\n"
        "uniform mat4 uModel;\n"
        "out vec3 vNormal;\n"
        "out vec3 vWorldPos;\n"
        "out vec4 vColor;\n"
        "void main() {\n"
        "    vec4 worldPos = uModel * vec4(aPosition, 1.0);\n"
        "    vWorldPos = worldPos.xyz;\n"
        "    vNormal = mat3(uModel) * aNormal;\n"
        "    vColor = aColor;\n"
        "    gl_Position = uViewProj * worldPos;\n"
        "}\n";

    const char* fsSource =
        "#version 330 core\n"
        "in vec3 vNormal;\n"
        "in vec3 vWorldPos;\n"
        "in vec4 vColor;\n"
        "uniform vec3 uLightDir;\n"
        "uniform vec3 uEyePos;\n"
        "uniform vec4 uTint;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "    vec3 N = normalize(vNormal);\n"
        "    vec3 L = normalize(uLightDir);\n"
        "    vec3 V = normalize(uEyePos - vWorldPos);\n"
        "    vec3 H = normalize(L + V);\n"
        "    float diff = max(dot(N, L), 0.0);\n"
        "    float spec = pow(max(dot(N, H), 0.0), 24.0) * 0.40;\n"
        "    float ambient = 0.35;\n"
        "    vec3 rgb = (vColor.rgb * uTint.rgb) * (ambient + diff * 0.65) + vec3(spec);\n"
        "    fragColor = vec4(rgb, vColor.a * uTint.a);\n"
        "}\n";
#endif

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vsSource, nullptr);
    glCompileShader(vs);
    GLint success = 0;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(vs, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Vertex Shader Error: " << infoLog << "\n";
        return false;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSource, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fs, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Fragment Shader Error: " << infoLog << "\n";
        return false;
    }

    m_program = glCreateProgram();
    glAttachShader(m_program, vs);
    glAttachShader(m_program, fs);
    glLinkProgram(m_program);

    glDeleteShader(vs);
    glDeleteShader(fs);

    glGetProgramiv(m_program, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(m_program, sizeof(infoLog), nullptr, infoLog);
        std::cerr << "Shader Program Link Error: " << infoLog << "\n";
        return false;
    }

    m_uViewProj  = glGetUniformLocation(m_program, "uViewProj");
    m_uModel     = glGetUniformLocation(m_program, "uModel");
    m_uLightDir  = glGetUniformLocation(m_program, "uLightDir");
    m_uEyePos    = glGetUniformLocation(m_program, "uEyePos");
    m_uTint      = glGetUniformLocation(m_program, "uTint");

    return true;
}

void VehicleRenderer::BuildGroundMesh() {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    constexpr int GRID_SIZE = 80;      // 80x80 tiles
    constexpr float TILE_SIZE = 10.0f; // 800m x 800m arena
    constexpr float HALF_SIZE = (GRID_SIZE * TILE_SIZE) * 0.5f;

    Vector4 colorDark  = {0.10f, 0.11f, 0.14f, 1.0f}; // Dark asphalt
    Vector4 colorLight = {0.16f, 0.18f, 0.22f, 1.0f}; // Light asphalt
    Vector4 colorStripe= {0.85f, 0.65f, 0.15f, 1.0f}; // Yellow runway stripe

    for (int z = 0; z < GRID_SIZE; ++z) {
        for (int x = 0; x < GRID_SIZE; ++x) {
            float x0 = -HALF_SIZE + x * TILE_SIZE;
            float x1 = x0 + TILE_SIZE;
            float z0 = -HALF_SIZE + z * TILE_SIZE;
            float z1 = z0 + TILE_SIZE;

            Vector4 c = ((x + z) % 2 == 0) ? colorDark : colorLight;
            // Center runway yellow line
            if (std::abs(x - GRID_SIZE / 2) <= 1 && (z % 4 <= 2)) {
                c = colorStripe;
            }

            uint32_t baseIdx = (uint32_t)vertices.size();
            Vector3 n{0.0f, 1.0f, 0.0f};

            vertices.push_back({{x0, 0.0f, z0}, n, c});
            vertices.push_back({{x1, 0.0f, z0}, n, c});
            vertices.push_back({{x1, 0.0f, z1}, n, c});
            vertices.push_back({{x0, 0.0f, z1}, n, c});

            indices.push_back(baseIdx + 0);
            indices.push_back(baseIdx + 2);
            indices.push_back(baseIdx + 1);
            indices.push_back(baseIdx + 0);
            indices.push_back(baseIdx + 3);
            indices.push_back(baseIdx + 2);
        }
    }

    m_groundIndexCount = (uint32_t)indices.size();

    glGenVertexArrays(1, &m_groundVao);
    glBindVertexArray(m_groundVao);

    glGenBuffers(1, &m_groundVbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_groundVbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &m_groundEbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_groundEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    // Attribute 0: Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    // Attribute 1: Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    // Attribute 2: Color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

    glBindVertexArray(0);
}

static void AddBox(std::vector<Vertex>& verts, std::vector<uint32_t>& inds,
                   const Vector3& center, const Vector3& extents, const Vector4& color) {
    float hx = extents.x * 0.5f;
    float hy = extents.y * 0.5f;
    float hz = extents.z * 0.5f;

    struct Face {
        Vector3 n;
        Vector3 p[4];
    };

    Face faces[6] = {
        // Front (+Z)
        {{0, 0, 1}, {{center.x - hx, center.y - hy, center.z + hz},
                     {center.x + hx, center.y - hy, center.z + hz},
                     {center.x + hx, center.y + hy, center.z + hz},
                     {center.x - hx, center.y + hy, center.z + hz}}},
        // Back (-Z)
        {{0, 0, -1}, {{center.x + hx, center.y - hy, center.z - hz},
                      {center.x - hx, center.y - hy, center.z - hz},
                      {center.x - hx, center.y + hy, center.z - hz},
                      {center.x + hx, center.y + hy, center.z - hz}}},
        // Top (+Y)
        {{0, 1, 0}, {{center.x - hx, center.y + hy, center.z - hz},
                     {center.x + hx, center.y + hy, center.z - hz},
                     {center.x + hx, center.y + hy, center.z + hz},
                     {center.x - hx, center.y + hy, center.z + hz}}},
        // Bottom (-Y)
        {{0, -1, 0}, {{center.x - hx, center.y - hy, center.z + hz},
                      {center.x + hx, center.y - hy, center.z + hz},
                      {center.x + hx, center.y - hy, center.z - hz},
                      {center.x - hx, center.y - hy, center.z - hz}}},
        // Right (+X)
        {{1, 0, 0}, {{center.x + hx, center.y - hy, center.z + hz},
                     {center.x + hx, center.y - hy, center.z - hz},
                     {center.x + hx, center.y + hy, center.z - hz},
                     {center.x + hx, center.y + hy, center.z + hz}}},
        // Left (-X)
        {{-1, 0, 0}, {{center.x - hx, center.y - hy, center.z - hz},
                      {center.x - hx, center.y - hy, center.z + hz},
                      {center.x - hx, center.y + hy, center.z + hz},
                      {center.x - hx, center.y + hy, center.z - hz}}}
    };

    for (int i = 0; i < 6; ++i) {
        uint32_t baseIdx = (uint32_t)verts.size();
        for (int j = 0; j < 4; ++j) {
            verts.push_back({faces[i].p[j], faces[i].n, color});
        }
        inds.push_back(baseIdx + 0);
        inds.push_back(baseIdx + 2);
        inds.push_back(baseIdx + 1);
        inds.push_back(baseIdx + 0);
        inds.push_back(baseIdx + 3);
        inds.push_back(baseIdx + 2);
    }
}

void VehicleRenderer::BuildChassisMesh() {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    // Split/Second Signature Car Body Palette:
    Vector4 carBodyBlue   = {0.05f, 0.35f, 0.88f, 1.0f}; // Velocity electric blue
    Vector4 carTrimBlack  = {0.12f, 0.12f, 0.14f, 1.0f}; // Matte carbon trim
    Vector4 carCockpit    = {0.18f, 0.22f, 0.28f, 1.0f}; // Tinted cockpit glass
    Vector4 headlightCyan = {0.80f, 0.95f, 1.00f, 1.0f}; // Xenon headlights
    Vector4 taillightRed  = {0.95f, 0.08f, 0.08f, 1.0f}; // LED red taillights

    // 1. Lower Main Chassis
    AddBox(vertices, indices, {0.0f, 0.35f, 0.0f}, {1.85f, 0.38f, 4.20f}, carBodyBlue);

    // 2. Cabin / Cockpit Roof
    AddBox(vertices, indices, {0.0f, 0.72f, -0.30f}, {1.35f, 0.42f, 2.10f}, carCockpit);

    // 3. Front Hood / Wedge
    AddBox(vertices, indices, {0.0f, 0.38f, 1.40f}, {1.70f, 0.26f, 1.40f}, carBodyBlue);

    // 4. Front Splitter / Bumper
    AddBox(vertices, indices, {0.0f, 0.16f, 2.05f}, {1.88f, 0.10f, 0.35f}, carTrimBlack);

    // 5. Rear Diffuser
    AddBox(vertices, indices, {0.0f, 0.20f, -2.05f}, {1.82f, 0.14f, 0.30f}, carTrimBlack);

    // 6. Rear Wing / Spoiler
    AddBox(vertices, indices, {-0.60f, 0.75f, -1.90f}, {0.08f, 0.45f, 0.15f}, carTrimBlack); // Left strut
    AddBox(vertices, indices, { 0.60f, 0.75f, -1.90f}, {0.08f, 0.45f, 0.15f}, carTrimBlack); // Right strut
    AddBox(vertices, indices, { 0.00f, 0.98f, -1.95f}, {1.75f, 0.06f, 0.45f}, carBodyBlue);  // Wing blade

    // 7. Headlights (Twin Xenon front lenses)
    AddBox(vertices, indices, {-0.65f, 0.40f, 2.08f}, {0.35f, 0.12f, 0.06f}, headlightCyan);
    AddBox(vertices, indices, { 0.65f, 0.40f, 2.08f}, {0.35f, 0.12f, 0.06f}, headlightCyan);

    // 8. Taillights (Full-width LED rear strip)
    AddBox(vertices, indices, {0.0f, 0.45f, -2.08f}, {1.65f, 0.10f, 0.06f}, taillightRed);

    m_chassisIndexCount = (uint32_t)indices.size();

    glGenVertexArrays(1, &m_chassisVao);
    glBindVertexArray(m_chassisVao);

    glGenBuffers(1, &m_chassisVbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_chassisVbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &m_chassisEbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_chassisEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

    glBindVertexArray(0);
}

void VehicleRenderer::BuildWheelMesh() {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    constexpr int SEGMENTS = 18;
    constexpr float RADIUS = 0.35f;
    constexpr float WIDTH = 0.28f;
    constexpr float HALF_W = WIDTH * 0.5f;

    Vector4 tireColor = {0.10f, 0.10f, 0.11f, 1.0f};  // Rubber tire
    Vector4 rimColor  = {0.70f, 0.72f, 0.75f, 1.0f};  // Alloy silver rim

    // Generate cylinder centered at origin aligned along X axis (wheel width is along X)
    // Tread strip
    for (int i = 0; i < SEGMENTS; ++i) {
        float theta0 = (2.0f * Math::PI * i) / SEGMENTS;
        float theta1 = (2.0f * Math::PI * (i + 1)) / SEGMENTS;

        float y0 = std::sin(theta0) * RADIUS;
        float z0 = std::cos(theta0) * RADIUS;
        float y1 = std::sin(theta1) * RADIUS;
        float z1 = std::cos(theta1) * RADIUS;

        Vector3 n0{0.0f, std::sin(theta0), std::cos(theta0)};
        Vector3 n1{0.0f, std::sin(theta1), std::cos(theta1)};

        uint32_t base = (uint32_t)vertices.size();

        vertices.push_back({{-HALF_W, y0, z0}, n0, tireColor});
        vertices.push_back({{ HALF_W, y0, z0}, n0, tireColor});
        vertices.push_back({{ HALF_W, y1, z1}, n1, tireColor});
        vertices.push_back({{-HALF_W, y1, z1}, n1, tireColor});

        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 1);
        indices.push_back(base + 0);
        indices.push_back(base + 3);
        indices.push_back(base + 2);
    }

    // Outer Rim Cap (+X)
    uint32_t centerOuter = (uint32_t)vertices.size();
    vertices.push_back({{HALF_W, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, rimColor});
    for (int i = 0; i <= SEGMENTS; ++i) {
        float theta = (2.0f * Math::PI * i) / SEGMENTS;
        float y = std::sin(theta) * (RADIUS * 0.75f);
        float z = std::cos(theta) * (RADIUS * 0.75f);
        vertices.push_back({{HALF_W, y, z}, {1.0f, 0.0f, 0.0f}, rimColor});
        if (i > 0) {
            indices.push_back(centerOuter);
            indices.push_back(centerOuter + i + 1);
            indices.push_back(centerOuter + i);
        }
    }

    // Inner Rim Cap (-X)
    uint32_t centerInner = (uint32_t)vertices.size();
    vertices.push_back({{-HALF_W, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, rimColor});
    for (int i = 0; i <= SEGMENTS; ++i) {
        float theta = (2.0f * Math::PI * i) / SEGMENTS;
        float y = std::sin(theta) * (RADIUS * 0.75f);
        float z = std::cos(theta) * (RADIUS * 0.75f);
        vertices.push_back({{-HALF_W, y, z}, {-1.0f, 0.0f, 0.0f}, rimColor});
        if (i > 0) {
            indices.push_back(centerInner);
            indices.push_back(centerInner + i);
            indices.push_back(centerInner + i + 1);
        }
    }

    m_wheelIndexCount = (uint32_t)indices.size();

    glGenVertexArrays(1, &m_wheelVao);
    glBindVertexArray(m_wheelVao);

    glGenBuffers(1, &m_wheelVbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_wheelVbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &m_wheelEbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_wheelEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

    glBindVertexArray(0);
}

void VehicleRenderer::RenderScene(const CameraRig& camera, const Physics::VehicleSimulation& vehicle, float aspectRatio, bool isBraking) {
    if (!m_initialized) return;

    glUseProgram(m_program);

    Matrix4x4 view = camera.GetViewMatrix();
    Matrix4x4 proj = camera.GetProjectionMatrix(aspectRatio);
    Matrix4x4 viewProj = view * proj;

    glUniformMatrix4fv(m_uViewProj, 1, GL_FALSE, &viewProj.m[0][0]);

    // Directional sun light
    glUniform3f(m_uLightDir, 0.45f, 0.85f, 0.28f);

    Vector3 camPos = camera.GetPosition();
    glUniform3f(m_uEyePos, camPos.x, camPos.y, camPos.z);

    // 1. Render Ground Plane
    Matrix4x4 groundModel = Matrix4x4::Identity();
    glUniformMatrix4fv(m_uModel, 1, GL_FALSE, &groundModel.m[0][0]);
    glUniform4f(m_uTint, 1.0f, 1.0f, 1.0f, 1.0f);

    glBindVertexArray(m_groundVao);
    glDrawElements(GL_TRIANGLES, m_groundIndexCount, GL_UNSIGNED_INT, nullptr);

    // 2. Render Vehicle Chassis
    Matrix4x4 chassisModel = vehicle.GetTransform();
    glUniformMatrix4fv(m_uModel, 1, GL_FALSE, &chassisModel.m[0][0]);

    // If braking, tint taillights glowing red
    if (isBraking) {
        glUniform4f(m_uTint, 1.15f, 0.95f, 0.95f, 1.0f);
    } else {
        glUniform4f(m_uTint, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    glBindVertexArray(m_chassisVao);
    glDrawElements(GL_TRIANGLES, m_chassisIndexCount, GL_UNSIGNED_INT, nullptr);

    // 3. Render 4 Wheels with their individual dynamic transforms
    glBindVertexArray(m_wheelVao);
    glUniform4f(m_uTint, 1.0f, 1.0f, 1.0f, 1.0f);

    for (size_t i = 0; i < 4; ++i) {
        Matrix4x4 wheelModel = vehicle.GetWheelTransform(i);
        glUniformMatrix4fv(m_uModel, 1, GL_FALSE, &wheelModel.m[0][0]);
        glDrawElements(GL_TRIANGLES, m_wheelIndexCount, GL_UNSIGNED_INT, nullptr);
    }

    glBindVertexArray(0);
}

} // namespace SSV
