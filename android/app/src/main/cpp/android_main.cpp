#include <android/log.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <chrono>
#include <array>

#include "game_engine.h"
#include "platform_abstraction.h"
#include "vehicle_physics.h"
#include "camera_rig.h"
#include "vehicle_renderer.h"

#define LOG_TAG "SSV_Main"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace SSV {
namespace Platform {
    extern void SetAndroidAssetManager(AAssetManager* mgr, const char* internalPath);
}
}

static SSV::InputState g_CurrentInput{};
static SSV::Physics::VehicleSimulation g_VehicleSim;
static SSV::CameraRig g_CameraRig;
static SSV::VehicleRenderer g_Renderer;
static bool g_RendererInitialized = false;

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

    // Initialize 3D Vehicle & Environment Renderer
    if (!g_RendererInitialized) {
        if (g_Renderer.Initialize()) {
            g_RendererInitialized = true;
            LOGI("[SSV_Main] Vehicle & Environment 3D Renderer initialized on Android.");
        } else {
            LOGE("[SSV_Main] Failed to initialize 3D Renderer!");
        }
    }

    // Spawn vehicle on real 3D track mesh if loaded, otherwise fallback to flat plane
    if (g_Renderer.HasTrackMesh()) {
        g_VehicleSim.SetRoadCollision(g_Renderer.GetRoadTriangles());
        g_VehicleSim.Reset(g_Renderer.GetTrackSpawnPoint());
        g_CameraRig.Reset(g_VehicleSim.GetPosition(), g_VehicleSim.GetOrientation());
        LOGI("[SSV_Main] Vehicle spawned on 3D Track at (%.2f, %.2f, %.2f) with %zu collision triangles.",
             g_Renderer.GetTrackSpawnPoint().x,
             g_Renderer.GetTrackSpawnPoint().y,
             g_Renderer.GetTrackSpawnPoint().z,
             g_Renderer.GetRoadTriangles().size());
    } else {
        g_VehicleSim.Reset({0.0f, 0.40f, 0.0f});
        g_CameraRig.Reset(g_VehicleSim.GetPosition(), g_VehicleSim.GetOrientation());
    }

    return 0;
}

static void TerminateDisplay(struct EngineContext* engine) {
    if (g_RendererInitialized) {
        g_Renderer.Shutdown();
        g_RendererInitialized = false;
    }

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
        int rawAction = AMotionEvent_getAction(event);
        int action = rawAction & AMOTION_EVENT_ACTION_MASK;
        int pointerIndex = (rawAction & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        size_t pointerCount = AMotionEvent_getPointerCount(event);

        if (action == AMOTION_EVENT_ACTION_UP || action == AMOTION_EVENT_ACTION_CANCEL) {
            // Touch released
            g_CurrentInput.steer = 0.0f;
            g_CurrentInput.throttle = 0.0f;
            g_CurrentInput.brake = 0.0f;
            g_CurrentInput.handbrake = false;
            return 1;
        }

        // Multi-touch virtual zones
        float steerVal = 0.0f;
        float throttleVal = 0.0f;
        float brakeVal = 0.0f;
        bool handbrakeVal = false;

        for (size_t i = 0; i < pointerCount; ++i) {
            // When a secondary finger lifts, skip the pointer being released
            if (action == AMOTION_EVENT_ACTION_POINTER_UP && (int)i == pointerIndex) {
                continue;
            }

            float x = AMotionEvent_getX(event, i) / (float)engine->width;
            float y = AMotionEvent_getY(event, i) / (float)engine->height;

            if (x < 0.45f) {
                // Left zone: Steering slider (center around 0.225)
                float s = (x - 0.225f) / 0.16f;
                steerVal = std::clamp(s, -1.0f, 1.0f);
            } else {
                // Right zone: Throttle (top) / Brake (bottom) / Handbrake (far right)
                if (x > 0.82f && y > 0.35f) {
                    handbrakeVal = true;
                }
                if (y < 0.50f) {
                    throttleVal = 1.0f;
                } else {
                    brakeVal = 1.0f;
                }
            }
        }

        g_CurrentInput.steer = steerVal;
        g_CurrentInput.throttle = throttleVal;
        g_CurrentInput.brake = brakeVal;
        g_CurrentInput.handbrake = handbrakeVal;
        return 1;
    }
    return 0;
}

static void HandleCmd(struct android_app* app, int32_t cmd) {
    struct EngineContext* engine = (struct EngineContext*)app->userData;
    LOGI("[SSV_Cmd] HandleCmd event: %d", cmd);
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
            LOGI("[SSV_Cmd] Gained focus.");
            engine->animating = true;
            break;
        case APP_CMD_LOST_FOCUS:
            LOGI("[SSV_Cmd] Lost focus (keeping surface active).");
            break;
        case APP_CMD_CONFIG_CHANGED:
        case APP_CMD_WINDOW_RESIZED: {
            if (engine->display != EGL_NO_DISPLAY && engine->surface != EGL_NO_SURFACE) {
                EGLint newW = 0, newH = 0;
                eglQuerySurface(engine->display, engine->surface, EGL_WIDTH, &newW);
                eglQuerySurface(engine->display, engine->surface, EGL_HEIGHT, &newH);
                if (newW > 0 && newH > 0) {
                    engine->width = newW;
                    engine->height = newH;
                    glViewport(0, 0, newW, newH);
                    LOGI("[SSV_Cmd] Surface resized to: %dx%d", newW, newH);
                }
            }
            break;
        }
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
    windowDesc.width = 2696;
    windowDesc.height = 1260;
    windowDesc.fullscreen = true;

    LOGI("Booting SSV Game Engine...");
    gameEngine.Initialize(windowDesc, false);

    uint32_t frameCount = 0;
    constexpr float FIXED_DT = 1.0f / 60.0f;
    std::array<float, 60> frameTimesMs{};

    // Main Android Event & Render Loop
    while (true) {
        int ident;
        int events;
        struct android_poll_source* source;

        // Poll events: block if surface is not ready; non-blocking (0) if animating
        int timeout = (engine.animating && engine.display != EGL_NO_DISPLAY) ? 0 : -1;
        while ((ident = ALooper_pollOnce(timeout, nullptr, &events, (void**)&source)) >= 0) {
            if (source != nullptr) {
                source->process(state, source);
            }
            if (state->destroyRequested != 0) {
                LOGI("Destroy requested by Android OS.");
                TerminateDisplay(&engine);
                gameEngine.Stop();
                return;
            }
            // Once we wake up and process any pending message, drain remaining events without blocking
            timeout = 0;
        }

        // Render frame if surface is active
        if (engine.animating && engine.display != EGL_NO_DISPLAY && g_RendererInitialized) {
            auto frameStart = std::chrono::high_resolution_clock::now();

            // 1. Fixed 60Hz Physics Simulation Step
            g_VehicleSim.Step(g_CurrentInput, FIXED_DT);
            g_CameraRig.Update(
                g_VehicleSim.GetPosition(),
                g_VehicleSim.GetOrientation(),
                g_VehicleSim.GetSpeedMph(),
                FIXED_DT
            );
            gameEngine.StepFrame(FIXED_DT);

            // 2. Clear Framebuffer
            glClearColor(0.06f, 0.07f, 0.10f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // 3. Render 3D Scene
            float aspect = (float)engine.width / (float)(engine.height > 0 ? engine.height : 1);
            bool isBraking = (g_CurrentInput.brake > 0.05f || g_CurrentInput.handbrake);
            g_Renderer.RenderScene(g_CameraRig, g_VehicleSim, aspect, isBraking);

            // 4. Swap EGL buffers
            eglSwapBuffers(engine.display, engine.surface);

            auto frameEnd = std::chrono::high_resolution_clock::now();
            float cpuMs = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
            frameTimesMs[frameCount % 60] = cpuMs;
            frameCount++;

            // Periodic telemetry reporting to Logcat (every 60 frames = ~1 sec)
            if (frameCount % 60 == 0) {
                std::array<float, 60> sorted = frameTimesMs;
                std::sort(sorted.begin(), sorted.end());
                float avgMs = 0.0f;
                for (float t : sorted) avgMs += t;
                avgMs /= 60.0f;
                float p50 = sorted[30];
                float p95 = sorted[57];
                float p99 = sorted[59];
                float vsyncBudgetMs = 16.67f;
                float headroomPct = std::max(0.0f, (vsyncBudgetMs - avgMs) / vsyncBudgetMs * 100.0f);

                LOGI("[SSV_Live] FPS: 60.0 | CPU ms: avg=%.2f, p50=%.2f, p95=%.2f, p99=%.2f (Headroom: %.1f%%) | Speed: %3.0f MPH (Gear %d, %4.0f RPM) | Drift: %s | Steer: %+1.2f",
                     avgMs, p50, p95, p99, headroomPct,
                     g_VehicleSim.GetSpeedMph(),
                     g_VehicleSim.GetCurrentGear(),
                     g_VehicleSim.GetEngineRpm(),
                     g_VehicleSim.IsDrifting() ? "DRIFTING!" : "GRIP",
                     g_CurrentInput.steer);
            }
        }
    }
}
