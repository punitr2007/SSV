#pragma once
/**
 * @file vehicle_renderer.h
 * @brief Complete 3D Vehicle and Environment Rendering System
 *
 * Renders procedural tarmac/grid ground plane, detailed extracted Split/Second vehicle
 * chassis with diffuse textures, and 4 animated wheels reacting to suspension travel.
 * Compatible with Desktop OpenGL 3.3 Core and Android OpenGL ES 3.0.
 */

#include "math_types.h"
#include "camera_rig.h"
#include "vehicle_physics.h"
#include "gl_platform.h"
#include "cooked_bundle.h"
#include <vector>

namespace SSV {

using Math::Vector3;
using Math::Vector4;
using Math::Matrix4x4;

struct Vertex {
    Vector3 position;
    Vector3 normal;
    Vector4 color;
    float u = 0.0f;
    float v = 0.0f;
};

class VehicleRenderer {
public:
    VehicleRenderer();
    ~VehicleRenderer();

    bool Initialize();
    void RenderScene(const CameraRig& camera, const Physics::VehicleSimulation& vehicle, float aspectRatio, bool isBraking = false);
    void Shutdown();

private:
    void BuildGroundMesh();
    void BuildChassisMesh();
    void BuildWheelMesh();
    bool LoadCookedAssets();
    void BuildProceduralChassisMesh();
    bool CompileShaders();

    // Shader program & uniform locations
    GLuint m_program = 0;
    GLint m_uViewProj = -1;
    GLint m_uModel = -1;
    GLint m_uLightDir = -1;
    GLint m_uEyePos = -1;
    GLint m_uTint = -1;
    GLint m_uTexture = -1;
    GLint m_uUseTexture = -1;

    // Ground plane GL buffers
    GLuint m_groundVao = 0;
    GLuint m_groundVbo = 0;
    GLuint m_groundEbo = 0;
    uint32_t m_groundIndexCount = 0;

    // Chassis GL buffers & textures
    GLuint m_chassisVao = 0;
    GLuint m_chassisVbo = 0;
    GLuint m_chassisEbo = 0;
    uint32_t m_chassisIndexCount = 0;
    GLenum m_chassisIndexType = GL_UNSIGNED_INT;
    GLuint m_chassisTexture = 0;
    bool m_hasCookedMesh = false;

    // Wheel GL buffers
    GLuint m_wheelVao = 0;
    GLuint m_wheelVbo = 0;
    GLuint m_wheelEbo = 0;
    uint32_t m_wheelIndexCount = 0;

    bool m_initialized = false;
};

} // namespace SSV
