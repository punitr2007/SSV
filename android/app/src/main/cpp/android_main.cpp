#include <android/log.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstring>

#include "game_engine.h"
#include "platform_abstraction.h"
#include "gles3_backend.h"

#define LOG_TAG "SSV_Main"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace SSV {
namespace Platform {
    extern void SetAndroidAssetManager(AAssetManager* mgr, const char* internalPath);
}
}

struct EngineContext {
    struct android_app* app;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    int32_t width;
    int32_t height;
    bool animating;
};

static int InitDisplay(struct EngineContext* engine) {
    const EGLint attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_NONE
    };

    const EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(display, nullptr, nullptr);

    EGLConfig config;
    EGLint numConfigs;
    eglChooseConfig(display, attribs, &config, 1, &numConfigs);

    EGLSurface surface = eglCreateWindowSurface(display, config, engine->app->window, nullptr);
    EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOGE("Unable to eglMakeCurrent");
        return -1;
    }

    EGLint w = 0, h = 0;
    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->width = w;
    engine->height = h;

    LOGI("EGL Surface created: %dx%d (OpenGL ES 3.0)", w, h);
    glViewport(0, 0, w, h);

    return 0;
}

static void TerminateDisplay(struct EngineContext* engine) {
    if (engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
        }
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }
        eglTerminate(engine->display);
    }
    engine->animating = false;
    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
}

static int32_t HandleInput(struct android_app* app, AInputEvent* event) {
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        struct EngineContext* engine = (struct EngineContext*)app->userData;
        int action = AMotionEvent_getAction(event) & AMOTION_EVENT_ACTION_MASK;
        size_t pointerCount = AMotionEvent_getPointerCount(event);

        for (size_t i = 0; i < pointerCount; ++i) {
            float x = AMotionEvent_getX(event, i) / (float)engine->width;
            float y = AMotionEvent_getY(event, i) / (float)engine->height;
            (void)action;
            (void)x;
            (void)y;
        }
        return 1;
    }
    return 0;
}

static void HandleCmd(struct android_app* app, int32_t cmd) {
    struct EngineContext* engine = (struct EngineContext*)app->userData;
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (engine->app->window != nullptr) {
                InitDisplay(engine);
                engine->animating = true;
            }
            break;
        case APP_CMD_TERM_WINDOW:
            TerminateDisplay(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            engine->animating = true;
            break;
        case APP_CMD_LOST_FOCUS:
            engine->animating = false;
            break;
        default:
            break;
    }
}

void android_main(struct android_app* state) {
    struct EngineContext engine{};
    memset(&engine, 0, sizeof(engine));
    state->userData = &engine;
    state->onAppCmd = HandleCmd;
    state->onInputEvent = HandleInput;
    engine.app = state;

    LOGI("==============================================================");
    LOGI("  SPLIT/SECOND: VELOCITY - NATIVE ANDROID ARM64 PORT STARTING ");
    LOGI("==============================================================");

    // Initialize asset manager and storage path
    const char* internalPath = state->activity->internalDataPath;
    SSV::Platform::SetAndroidAssetManager(state->activity->assetManager, internalPath);

    // Boot Game Engine
    SSV::Core::GameEngine gameEngine;
    SSV::Platform::WindowDesc windowDesc;
    windowDesc.title = "Split/Second";
    windowDesc.width = 1920;
    windowDesc.height = 1080;
    windowDesc.fullscreen = true;

    LOGI("Booting SSV Game Engine...");
    gameEngine.Initialize(windowDesc, false);

    // Main Android Event & Render Loop
    while (true) {
        int ident;
        int events;
        struct android_poll_source* source;

        // Process OS / input events
        while ((ident = ALooper_pollOnce(engine.animating ? 0 : -1, nullptr, &events, (void**)&source)) >= 0) {
            if (source != nullptr) {
                source->process(state, source);
            }
            if (state->destroyRequested != 0) {
                LOGI("Destroy requested by Android OS.");
                TerminateDisplay(&engine);
                gameEngine.Stop();
                return;
            }
        }

        // Render frame if surface is active
        if (engine.animating && engine.display != EGL_NO_DISPLAY) {
            // Update simulation tick
            gameEngine.StepFrame(1.0f / 60.0f);

            // Clear frame buffer
            glClearColor(0.08f, 0.08f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Swap EGL buffers
            eglSwapBuffers(engine.display, engine.surface);
        }
    }
}
