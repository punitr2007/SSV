/**
 * @file vehicle_renderer.cpp
 * @brief Implementation of 3D Vehicle & Flat Plane Renderer
 */

#include "vehicle_renderer.h"
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>

#ifdef __ANDROID__
#include <android/log.h>
#include <android/asset_manager.h>
#define LOG_TAG "SSV_Render"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
namespace SSV {
namespace Platform {
    extern AAssetManager* GetAndroidAssetManager();
}
}
#else
#define LOGI(...) printf(__VA_ARGS__)
#define LOGE(...) fprintf(stderr, __VA_ARGS__)
#endif

namespace SSV {

VehicleRenderer::VehicleRenderer() = default;

VehicleRenderer::~VehicleRenderer() {
    Shutdown();
}

bool VehicleRenderer::Initialize() {
    if (!InitGLPlatform()) {
        LOGE("[VehicleRenderer] Failed to initialize GL platform.\n");
        return false;
    }

    if (!CompileShaders()) {
        LOGE("[VehicleRenderer] Failed to compile shaders.\n");
        return false;
    }

    BuildGroundMesh();
    BuildChassisMesh();
    BuildWheelMesh();
    BuildShadowDecalMesh();

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    m_initialized = true;
    LOGI("[VehicleRenderer] Successfully initialized vehicle & environment renderer (Cooked Car Mesh: %s).\n",
         m_hasCookedMesh ? "ACTIVE" : "FALLBACK");
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
    if (m_chassisTexture) { glDeleteTextures(1, &m_chassisTexture); m_chassisTexture = 0; }
    if (m_wheelTexture) { glDeleteTextures(1, &m_wheelTexture); m_wheelTexture = 0; }
    if (m_shadowTexture) { glDeleteTextures(1, &m_shadowTexture); m_shadowTexture = 0; }
    if (m_normalSpecTexture) { glDeleteTextures(1, &m_normalSpecTexture); m_normalSpecTexture = 0; }
    if (m_interiorLightsTexture) { glDeleteTextures(1, &m_interiorLightsTexture); m_interiorLightsTexture = 0; }
    if (m_damageTexture) { glDeleteTextures(1, &m_damageTexture); m_damageTexture = 0; }

    if (m_shadowVao) { glDeleteVertexArrays(1, &m_shadowVao); m_shadowVao = 0; }
    if (m_shadowVbo) { glDeleteBuffers(1, &m_shadowVbo); m_shadowVbo = 0; }
    if (m_shadowEbo) { glDeleteBuffers(1, &m_shadowEbo); m_shadowEbo = 0; }

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
        "layout(location = 3) in vec2 aTexCoord;\n"
        "uniform mat4 uViewProj;\n"
        "uniform mat4 uModel;\n"
        "out vec3 vNormal;\n"
        "out vec3 vWorldPos;\n"
        "out vec4 vColor;\n"
        "out vec2 vTexCoord;\n"
        "void main() {\n"
        "    vec4 worldPos = uModel * vec4(aPosition, 1.0);\n"
        "    vWorldPos = worldPos.xyz;\n"
        "    vNormal = mat3(uModel) * aNormal;\n"
        "    vColor = aColor;\n"
        "    vTexCoord = aTexCoord;\n"
        "    gl_Position = uViewProj * worldPos;\n"
        "}\n";

    const char* fsSource =
        "#version 300 es\n"
        "precision mediump float;\n"
        "in vec3 vNormal;\n"
        "in vec3 vWorldPos;\n"
        "in vec4 vColor;\n"
        "in vec2 vTexCoord;\n"
        "uniform vec3 uLightDir;\n"
        "uniform vec3 uEyePos;\n"
        "uniform vec4 uTint;\n"
        "uniform sampler2D uTexture;\n"
        "uniform int uUseTexture;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "    vec3 N = normalize(vNormal);\n"
        "    vec3 L = normalize(uLightDir);\n"
        "    vec3 V = normalize(uEyePos - vWorldPos);\n"
        "    vec3 H = normalize(L + V);\n"
        "    float diff = max(dot(N, L), 0.0);\n"
        "    float spec = pow(max(dot(N, H), 0.0), 24.0) * 0.40;\n"
        "    float ambient = 0.35;\n"
        "    vec4 baseColor = (uUseTexture != 0) ? texture(uTexture, vTexCoord) : vColor;\n"
        "    vec3 rgb = (baseColor.rgb * uTint.rgb) * (ambient + diff * 0.65) + vec3(spec);\n"
        "    fragColor = vec4(rgb, baseColor.a * uTint.a);\n"
        "}\n";
#else
    const char* vsSource =
        "#version 330 core\n"
        "layout(location = 0) in vec3 aPosition;\n"
        "layout(location = 1) in vec3 aNormal;\n"
        "layout(location = 2) in vec4 aColor;\n"
        "layout(location = 3) in vec2 aTexCoord;\n"
        "uniform mat4 uViewProj;\n"
        "uniform mat4 uModel;\n"
        "out vec3 vNormal;\n"
        "out vec3 vWorldPos;\n"
        "out vec4 vColor;\n"
        "out vec2 vTexCoord;\n"
        "void main() {\n"
        "    vec4 worldPos = uModel * vec4(aPosition, 1.0);\n"
        "    vWorldPos = worldPos.xyz;\n"
        "    vNormal = mat3(uModel) * aNormal;\n"
        "    vColor = aColor;\n"
        "    vTexCoord = aTexCoord;\n"
        "    gl_Position = uViewProj * worldPos;\n"
        "}\n";

    const char* fsSource =
        "#version 330 core\n"
        "in vec3 vNormal;\n"
        "in vec3 vWorldPos;\n"
        "in vec4 vColor;\n"
        "in vec2 vTexCoord;\n"
        "uniform vec3 uLightDir;\n"
        "uniform vec3 uEyePos;\n"
        "uniform vec4 uTint;\n"
        "uniform sampler2D uTexture;\n"
        "uniform int uUseTexture;\n"
        "out vec4 fragColor;\n"
        "void main() {\n"
        "    vec3 N = normalize(vNormal);\n"
        "    vec3 L = normalize(uLightDir);\n"
        "    vec3 V = normalize(uEyePos - vWorldPos);\n"
        "    vec3 H = normalize(L + V);\n"
        "    float diff = max(dot(N, L), 0.0);\n"
        "    float spec = pow(max(dot(N, H), 0.0), 24.0) * 0.40;\n"
        "    float ambient = 0.35;\n"
        "    vec4 baseColor = (uUseTexture != 0) ? texture(uTexture, vTexCoord) : vColor;\n"
        "    vec3 rgb = (baseColor.rgb * uTint.rgb) * (ambient + diff * 0.65) + vec3(spec);\n"
        "    fragColor = vec4(rgb, baseColor.a * uTint.a);\n"
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
        LOGE("Vertex Shader Error: %s\n", infoLog);
        return false;
    }

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fsSource, nullptr);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(fs, sizeof(infoLog), nullptr, infoLog);
        LOGE("Fragment Shader Error: %s\n", infoLog);
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
        LOGE("Shader Program Link Error: %s\n", infoLog);
        return false;
    }

    m_uViewProj   = glGetUniformLocation(m_program, "uViewProj");
    m_uModel      = glGetUniformLocation(m_program, "uModel");
    m_uLightDir   = glGetUniformLocation(m_program, "uLightDir");
    m_uEyePos     = glGetUniformLocation(m_program, "uEyePos");
    m_uTint       = glGetUniformLocation(m_program, "uTint");
    m_uTexture    = glGetUniformLocation(m_program, "uTexture");
    m_uUseTexture = glGetUniformLocation(m_program, "uUseTexture");

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
            if (std::abs(x - GRID_SIZE / 2) <= 1 && (z % 4 <= 2)) {
                c = colorStripe;
            }

            uint32_t baseIdx = (uint32_t)vertices.size();
            Vector3 n{0.0f, 1.0f, 0.0f};

            vertices.push_back({{x0, 0.0f, z0}, n, c, 0.0f, 0.0f});
            vertices.push_back({{x1, 0.0f, z0}, n, c, 1.0f, 0.0f});
            vertices.push_back({{x1, 0.0f, z1}, n, c, 1.0f, 1.0f});
            vertices.push_back({{x0, 0.0f, z1}, n, c, 0.0f, 1.0f});

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

    // Stride 48: Position (0), Normal (12), Color (24), UV (40)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));

    glBindVertexArray(0);
}

bool VehicleRenderer::LoadCookedAssets() {
    std::vector<uint8_t> bundleData;

    // 1. Check local file candidates
    const char* candidates[] = {
        "assets/main_game.bin",
        "main_game.bin",
        "../assets/main_game.bin",
        "android/app/src/main/assets/main_game.bin",
        "D:/Codes/Win_codes/SSV/assets/main_game.bin"
    };

    for (const char* path : candidates) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            std::streamsize sz = file.tellg();
            file.seekg(0, std::ios::beg);
            bundleData.resize(static_cast<size_t>(sz));
            if (file.read(reinterpret_cast<char*>(bundleData.data()), sz)) {
                LOGI("[VehicleRenderer] Loaded main_game.bin from '%s' (%zu bytes)\n", path, bundleData.size());
                break;
            }
        }
    }

#ifdef __ANDROID__
    // 2. Fall back to APK assets on Android
    if (bundleData.empty()) {
        AAssetManager* mgr = Platform::GetAndroidAssetManager();
        if (mgr) {
            AAsset* asset = AAssetManager_open(mgr, "main_game.bin", AASSET_MODE_BUFFER);
            if (asset) {
                off_t sz = AAsset_getLength(asset);
                bundleData.resize(static_cast<size_t>(sz));
                AAsset_read(asset, bundleData.data(), sz);
                AAsset_close(asset);
                LOGI("[VehicleRenderer] Loaded main_game.bin from APK assets (%zu bytes)\n", bundleData.size());
            }
        }
    }
#endif

    if (bundleData.empty()) {
        LOGE("[VehicleRenderer] main_game.bin not found on disk or APK assets.\n");
        return false;
    }

    CookedBundleReader reader;
    if (!reader.Open(bundleData.data(), bundleData.size())) {
        LOGE("[VehicleRenderer] Failed to parse main_game.bin header/TOC.\n");
        return false;
    }

    // 3. Load Musclecar_01 Mesh
    const CookedResourceEntry* meshEntry = reader.FindEntry("models/vehicles/musclecar_01");
    if (!meshEntry) {
        LOGE("[VehicleRenderer] Resource 'models/vehicles/musclecar_01' not found in cooked bundle.\n");
        return false;
    }

    const uint8_t* meshBlob = reader.GetEntryData(meshEntry);
    const auto* meshHdr = reinterpret_cast<const GPUMeshHeader*>(meshBlob);
    const uint8_t* vboBytes = meshBlob + meshHdr->vboOffset;
    const uint8_t* eboBytes = meshBlob + meshHdr->eboOffset;

    glGenVertexArrays(1, &m_chassisVao);
    glBindVertexArray(m_chassisVao);

    glGenBuffers(1, &m_chassisVbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_chassisVbo);
    glBufferData(GL_ARRAY_BUFFER, meshHdr->vboSize, vboBytes, GL_STATIC_DRAW);

    glGenBuffers(1, &m_chassisEbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_chassisEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, meshHdr->eboSize, eboBytes, GL_STATIC_DRAW);

    // Stride 48: Position (0), Normal (12), Color (24), UV (40)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, meshHdr->vertexStride, (void*)0);

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, meshHdr->vertexStride, (void*)12);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, meshHdr->vertexStride, (void*)24);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, meshHdr->vertexStride, (void*)40);

    if (meshHdr->vertexStride >= 64) {
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, meshHdr->vertexStride, (void*)48);
    }

    glBindVertexArray(0);

    m_chassisIndexCount = meshHdr->indexCount;
    m_chassisIndexType = (meshHdr->indexFormat == 1) ? GL_UNSIGNED_INT : GL_UNSIGNED_SHORT;
    m_hasCookedMesh = true;

    LOGI("[VehicleRenderer] Successfully bound Cooked Vehicle Mesh (%u vertices, %u indices, %u stride).\n",
         meshHdr->vertexCount, meshHdr->indexCount, meshHdr->vertexStride);

    // 4. Helper to upload texture from cooked bundle
    auto uploadTex = [&](const char* resName) -> GLuint {
        const CookedResourceEntry* entry = reader.FindEntry(resName);
        if (!entry) return 0;
        const uint8_t* blob = reader.GetEntryData(entry);
        const auto* hdr = reinterpret_cast<const GPUTextureHeader*>(blob);
        const uint8_t* pixels = blob + sizeof(GPUTextureHeader);

        GLuint tid = 0;
        glGenTextures(1, &tid);
        glBindTexture(GL_TEXTURE_2D, tid);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glTexImage2D(GL_TEXTURE_2D, 0, hdr->glInternalFormat,
                     hdr->width, hdr->height, 0,
                     hdr->glFormat, hdr->glType, pixels);

        glBindTexture(GL_TEXTURE_2D, 0);
        LOGI("[VehicleRenderer] Uploaded Cooked Texture '%s' (%ux%u).\n", resName, hdr->width, hdr->height);
        return tid;
    };

    m_chassisTexture       = uploadTex("textures/vehicles/musclecar_01_diffuse");
    m_wheelTexture         = uploadTex("textures/vehicles/musclecar_01_wheel");
    m_shadowTexture        = uploadTex("textures/vehicles/musclecar_01_shadow");
    m_normalSpecTexture    = uploadTex("textures/vehicles/musclecar_01_normal_spec");
    m_interiorLightsTexture= uploadTex("textures/vehicles/musclecar_01_interior_lights");
    m_damageTexture        = uploadTex("textures/vehicles/musclecar_01_damage");

    return true;
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
            verts.push_back({faces[i].p[j], faces[i].n, color, 0.0f, 0.0f});
        }
        inds.push_back(baseIdx + 0);
        inds.push_back(baseIdx + 2);
        inds.push_back(baseIdx + 1);
        inds.push_back(baseIdx + 0);
        inds.push_back(baseIdx + 3);
        inds.push_back(baseIdx + 2);
    }
}

void VehicleRenderer::BuildProceduralChassisMesh() {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    Vector4 carBodyBlue   = {0.05f, 0.35f, 0.88f, 1.0f};
    Vector4 carTrimBlack  = {0.12f, 0.12f, 0.14f, 1.0f};
    Vector4 carCockpit    = {0.18f, 0.22f, 0.28f, 1.0f};
    Vector4 headlightCyan = {0.80f, 0.95f, 1.00f, 1.0f};
    Vector4 taillightRed  = {0.95f, 0.08f, 0.08f, 1.0f};

    AddBox(vertices, indices, {0.0f, 0.35f, 0.0f}, {1.85f, 0.38f, 4.20f}, carBodyBlue);
    AddBox(vertices, indices, {0.0f, 0.72f, -0.30f}, {1.35f, 0.42f, 2.10f}, carCockpit);
    AddBox(vertices, indices, {0.0f, 0.38f, 1.40f}, {1.70f, 0.26f, 1.40f}, carBodyBlue);
    AddBox(vertices, indices, {0.0f, 0.16f, 2.05f}, {1.88f, 0.10f, 0.35f}, carTrimBlack);
    AddBox(vertices, indices, {0.0f, 0.20f, -2.05f}, {1.82f, 0.14f, 0.30f}, carTrimBlack);
    AddBox(vertices, indices, {-0.60f, 0.75f, -1.90f}, {0.08f, 0.45f, 0.15f}, carTrimBlack);
    AddBox(vertices, indices, { 0.60f, 0.75f, -1.90f}, {0.08f, 0.45f, 0.15f}, carTrimBlack);
    AddBox(vertices, indices, { 0.00f, 0.98f, -1.95f}, {1.75f, 0.06f, 0.45f}, carBodyBlue);
    AddBox(vertices, indices, {-0.65f, 0.40f, 2.08f}, {0.35f, 0.12f, 0.06f}, headlightCyan);
    AddBox(vertices, indices, { 0.65f, 0.40f, 2.08f}, {0.35f, 0.12f, 0.06f}, headlightCyan);
    AddBox(vertices, indices, {0.0f, 0.45f, -2.08f}, {1.65f, 0.10f, 0.06f}, taillightRed);

    m_chassisIndexCount = (uint32_t)indices.size();
    m_chassisIndexType = GL_UNSIGNED_INT;

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

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));

    glBindVertexArray(0);
}

void VehicleRenderer::BuildChassisMesh() {
    if (LoadCookedAssets()) {
        return;
    }
    LOGI("[VehicleRenderer] Falling back to procedural sports car chassis mesh.\n");
    BuildProceduralChassisMesh();
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

        vertices.push_back({{-HALF_W, y0, z0}, n0, tireColor, 0.0f, 0.0f});
        vertices.push_back({{ HALF_W, y0, z0}, n0, tireColor, 1.0f, 0.0f});
        vertices.push_back({{ HALF_W, y1, z1}, n1, tireColor, 1.0f, 1.0f});
        vertices.push_back({{-HALF_W, y1, z1}, n1, tireColor, 0.0f, 1.0f});

        indices.push_back(base + 0);
        indices.push_back(base + 2);
        indices.push_back(base + 1);
        indices.push_back(base + 0);
        indices.push_back(base + 3);
        indices.push_back(base + 2);
    }

    // Outer Rim Cap (+X)
    uint32_t centerOuter = (uint32_t)vertices.size();
    vertices.push_back({{HALF_W, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, rimColor, 0.5f, 0.5f});
    for (int i = 0; i <= SEGMENTS; ++i) {
        float theta = (2.0f * Math::PI * i) / SEGMENTS;
        float y = std::sin(theta) * (RADIUS * 0.75f);
        float z = std::cos(theta) * (RADIUS * 0.75f);
        vertices.push_back({{HALF_W, y, z}, {1.0f, 0.0f, 0.0f}, rimColor, 0.5f + y, 0.5f + z});
        if (i > 0) {
            indices.push_back(centerOuter);
            indices.push_back(centerOuter + i + 1);
            indices.push_back(centerOuter + i);
        }
    }

    // Inner Rim Cap (-X)
    uint32_t centerInner = (uint32_t)vertices.size();
    vertices.push_back({{-HALF_W, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, rimColor, 0.5f, 0.5f});
    for (int i = 0; i <= SEGMENTS; ++i) {
        float theta = (2.0f * Math::PI * i) / SEGMENTS;
        float y = std::sin(theta) * (RADIUS * 0.75f);
        float z = std::cos(theta) * (RADIUS * 0.75f);
        vertices.push_back({{-HALF_W, y, z}, {-1.0f, 0.0f, 0.0f}, rimColor, 0.5f + y, 0.5f + z});
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

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));

    glBindVertexArray(0);
}

void VehicleRenderer::BuildShadowDecalMesh() {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    Vector4 white = {1.0f, 1.0f, 1.0f, 1.0f};
    Vector3 up = {0.0f, 1.0f, 0.0f};

    // Ground plane decal quad spanning [-1, +1] in X and Z
    vertices.push_back({{-1.0f, 0.0f, -1.0f}, up, white, 0.0f, 0.0f});
    vertices.push_back({{ 1.0f, 0.0f, -1.0f}, up, white, 1.0f, 0.0f});
    vertices.push_back({{ 1.0f, 0.0f,  1.0f}, up, white, 1.0f, 1.0f});
    vertices.push_back({{-1.0f, 0.0f,  1.0f}, up, white, 0.0f, 1.0f});

    indices.push_back(0); indices.push_back(1); indices.push_back(2);
    indices.push_back(0); indices.push_back(2); indices.push_back(3);

    m_shadowIndexCount = (uint32_t)indices.size();

    glGenVertexArrays(1, &m_shadowVao);
    glBindVertexArray(m_shadowVao);

    glGenBuffers(1, &m_shadowVbo);
    glBindBuffer(GL_ARRAY_BUFFER, m_shadowVbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glGenBuffers(1, &m_shadowEbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_shadowEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, u));

    glBindVertexArray(0);
}

void VehicleRenderer::RenderScene(const CameraRig& camera, const Physics::VehicleSimulation& vehicle, float aspectRatio, bool isBraking) {
    if (!m_initialized) return;

    glUseProgram(m_program);

    Matrix4x4 view = camera.GetViewMatrix();
    Matrix4x4 proj = camera.GetProjectionMatrix(aspectRatio);
    Matrix4x4 viewProj = view * proj;

    glUniformMatrix4fv(m_uViewProj, 1, GL_FALSE, &viewProj.m[0][0]);
    glUniform3f(m_uLightDir, 0.45f, 0.85f, 0.28f);

    Vector3 camPos = camera.GetPosition();
    glUniform3f(m_uEyePos, camPos.x, camPos.y, camPos.z);

    // =========================================================================
    // PASS 1: Render Ground Arena Plane (Opaque)
    // =========================================================================
    Matrix4x4 groundModel = Matrix4x4::Identity();
    glUniformMatrix4fv(m_uModel, 1, GL_FALSE, &groundModel.m[0][0]);
    glUniform4f(m_uTint, 1.0f, 1.0f, 1.0f, 1.0f);
    glUniform1i(m_uUseTexture, 0);

    glBindVertexArray(m_groundVao);
    glDrawElements(GL_TRIANGLES, m_groundIndexCount, GL_UNSIGNED_INT, nullptr);

    // =========================================================================
    // PASS 2: Ground Contact Shadow Decal (Alpha Blended, No Depth Write)
    // =========================================================================
    if (m_shadowTexture != 0 && m_shadowVao != 0) {
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        Vector3 pos = vehicle.GetPosition();
        Quaternion ori = vehicle.GetOrientation();
        // Sized to vehicle footprint: width 2.5m, length 5.2m, placed at y = 0.02m to avoid z-fighting
        Matrix4x4 shadowModel = Matrix4x4::TRS({pos.x, 0.02f, pos.z}, ori, {1.25f, 1.0f, 2.6f});
        glUniformMatrix4fv(m_uModel, 1, GL_FALSE, &shadowModel.m[0][0]);
        glUniform4f(m_uTint, 0.0f, 0.0f, 0.0f, 0.75f);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_shadowTexture);
        glUniform1i(m_uTexture, 0);
        glUniform1i(m_uUseTexture, 1);

        glBindVertexArray(m_shadowVao);
        glDrawElements(GL_TRIANGLES, m_shadowIndexCount, GL_UNSIGNED_INT, nullptr);

        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
    }

    // =========================================================================
    // PASS 3: Render Opaque Vehicle Chassis (Extracted Musclecar_01)
    // =========================================================================
    Matrix4x4 chassisModel = vehicle.GetTransform();
    glUniformMatrix4fv(m_uModel, 1, GL_FALSE, &chassisModel.m[0][0]);

    if (isBraking) {
        glUniform4f(m_uTint, 1.15f, 0.95f, 0.95f, 1.0f);
    } else {
        glUniform4f(m_uTint, 1.0f, 1.0f, 1.0f, 1.0f);
    }

    if (m_chassisTexture != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_chassisTexture);
        glUniform1i(m_uTexture, 0);
        glUniform1i(m_uUseTexture, 1);
    } else {
        glUniform1i(m_uUseTexture, 0);
    }

    glBindVertexArray(m_chassisVao);
    glDrawElements(GL_TRIANGLES, m_chassisIndexCount, m_chassisIndexType, nullptr);

    // =========================================================================
    // PASS 4: Emissive Lighting Overlay (Taillight / Brake Glow)
    // =========================================================================
    if (m_interiorLightsTexture != 0 && isBraking) {
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE); // Additive blending for emissive glow

        glUniform4f(m_uTint, 1.5f, 0.15f, 0.15f, 1.0f); // High-intensity brake red
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_interiorLightsTexture);
        glUniform1i(m_uTexture, 0);
        glUniform1i(m_uUseTexture, 1);

        glBindVertexArray(m_chassisVao);
        glDrawElements(GL_TRIANGLES, m_chassisIndexCount, m_chassisIndexType, nullptr);

        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
    }

    // =========================================================================
    // PASS 5: Render 4 Wheels Articulated at .params Hardpoints
    // =========================================================================
    if (m_wheelTexture != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_wheelTexture);
        glUniform1i(m_uTexture, 0);
        glUniform1i(m_uUseTexture, 1);
    } else {
        glUniform1i(m_uUseTexture, 0);
    }
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
