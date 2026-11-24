#pragma once

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>
#include <vector>
#include "math_types.h"

namespace SSV {

using Math::Vector2;
using Math::Vector3;
using Math::Vector4;
using Math::Matrix4x4;

enum class RenderBackendType {
    Null,
    OpenGL_ES_3,
    Vulkan,
    Direct3D9
};

struct VertexFormat {
    Vector3 position;
    Vector3 normal;
    Vector2 texCoord;
    Vector4 color;
};

struct RenderStats {
    uint32_t drawCalls = 0;
    uint32_t trianglesCount = 0;
    float frameTimeMs = 0.0f;
};

class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    virtual bool Initialize(void* nativeWindow, int width, int height) = 0;
    virtual void Shutdown() = 0;
    virtual void Resize(int width, int height) = 0;

    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;

    virtual void SetClearColor(float r, float g, float b, float a) = 0;
    virtual void Clear(bool color = true, bool depth = true, bool stencil = false) = 0;
    virtual void SetViewport(int x, int y, int width, int height) = 0;

    virtual void SetViewProjection(const Matrix4x4& view, const Matrix4x4& projection) = 0;
    virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0) = 0;

    virtual RenderBackendType GetType() const = 0;
    virtual const RenderStats& GetStats() const = 0;
};

// Factory function
std::unique_ptr<IRenderBackend> CreateRenderBackend(RenderBackendType type);

} // namespace SSV
