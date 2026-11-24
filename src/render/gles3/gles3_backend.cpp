#include "gles3_backend.h"
#include <iostream>

#ifdef __ANDROID__
#include <GLES3/gl3.h>
#include <EGL/egl.h>
#include <android/log.h>
#define LOG_TAG "SSV_GLES3"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#else
#define LOGI(...) printf(__VA_ARGS__)
#define LOGE(...) fprintf(stderr, __VA_ARGS__)
#endif

namespace SSV {

std::unique_ptr<IRenderBackend> CreateRenderBackend(RenderBackendType type) {
    if (type == RenderBackendType::OpenGL_ES_3) {
        return std::make_unique<GLES3RenderBackend>();
    }
    return nullptr;
}

GLES3RenderBackend::GLES3RenderBackend() = default;

GLES3RenderBackend::~GLES3RenderBackend() {
    Shutdown();
}

bool GLES3RenderBackend::Initialize(void* nativeWindow, int width, int height) {
    m_width = width;
    m_height = height;

    LOGI("[SSV::GLES3] Initializing GLES3 Render Backend (%dx%d)...\n", width, height);

#ifdef __ANDROID__
    // In Android SDL2 / NativeActivity, EGL context is managed by SDL_GL_CreateContext
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
#endif

    SetViewport(0, 0, width, height);
    SetClearColor(0.08f, 0.08f, 0.12f, 1.0f);

    m_initialized = true;
    LOGI("[SSV::GLES3] Render Backend Initialized Successfully.\n");
    return true;
}

void GLES3RenderBackend::Shutdown() {
    if (!m_initialized) return;
    LOGI("[SSV::GLES3] Shutting down GLES3 backend.\n");
    m_initialized = false;
}

void GLES3RenderBackend::Resize(int width, int height) {
    m_width = width;
    m_height = height;
    SetViewport(0, 0, width, height);
}

void GLES3RenderBackend::BeginFrame() {
    m_stats.drawCalls = 0;
    m_stats.trianglesCount = 0;
    Clear(true, true, false);
}

void GLES3RenderBackend::EndFrame() {
    // Buffer swap is performed by the platform layer (SDL_GL_SwapWindow / eglSwapBuffers)
}

void GLES3RenderBackend::SetClearColor(float r, float g, float b, float a) {
#ifdef __ANDROID__
    glClearColor(r, g, b, a);
#endif
}

void GLES3RenderBackend::Clear(bool color, bool depth, bool stencil) {
#ifdef __ANDROID__
    GLbitfield mask = 0;
    if (color) mask |= GL_COLOR_BUFFER_BIT;
    if (depth) mask |= GL_DEPTH_BUFFER_BIT;
    if (stencil) mask |= GL_STENCIL_BUFFER_BIT;
    if (mask != 0) {
        glClear(mask);
    }
#endif
}

void GLES3RenderBackend::SetViewport(int x, int y, int width, int height) {
#ifdef __ANDROID__
    glViewport(x, y, width, height);
#endif
}

void GLES3RenderBackend::SetViewProjection(const Matrix4x4& view, const Matrix4x4& projection) {
    m_viewMatrix = view;
    m_projMatrix = projection;
}

void GLES3RenderBackend::DrawIndexed(uint32_t indexCount, uint32_t startIndex) {
#ifdef __ANDROID__
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, (const void*)(uintptr_t)(startIndex * sizeof(uint32_t)));
#endif
    m_stats.drawCalls++;
    m_stats.trianglesCount += (indexCount / 3);
}

} // namespace SSV
