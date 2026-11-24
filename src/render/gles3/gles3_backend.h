#pragma once

#include "render_backend.h"

namespace SSV {

class GLES3RenderBackend : public IRenderBackend {
public:
    GLES3RenderBackend();
    virtual ~GLES3RenderBackend() override;

    virtual bool Initialize(void* nativeWindow, int width, int height) override;
    virtual void Shutdown() override;
    virtual void Resize(int width, int height) override;

    virtual void BeginFrame() override;
    virtual void EndFrame() override;

    virtual void SetClearColor(float r, float g, float b, float a) override;
    virtual void Clear(bool color = true, bool depth = true, bool stencil = false) override;
    virtual void SetViewport(int x, int y, int width, int height) override;

    virtual void SetViewProjection(const Matrix4x4& view, const Matrix4x4& projection) override;
    virtual void DrawIndexed(uint32_t indexCount, uint32_t startIndex = 0) override;

    virtual RenderBackendType GetType() const override { return RenderBackendType::OpenGL_ES_3; }
    virtual const RenderStats& GetStats() const override { return m_stats; }

private:
    bool InitShaders();

    int m_width = 1280;
    int m_height = 720;
    uint32_t m_defaultProgram = 0;
    uint32_t m_uViewProjLocation = 0;
    uint32_t m_uModelLocation = 0;
    
    Matrix4x4 m_viewMatrix = Matrix4x4::Identity();
    Matrix4x4 m_projMatrix = Matrix4x4::Identity();
    RenderStats m_stats;
    bool m_initialized = false;
};

} // namespace SSV
