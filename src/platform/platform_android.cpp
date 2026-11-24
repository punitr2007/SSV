#if defined(__ANDROID__)

#include "platform_abstraction.h"
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <GLES3/gl3.h>

#define LOG_TAG "SSV_Platform"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

namespace SSV {
namespace Platform {

// Global Android Asset Manager pointer, initialized via JNI
static AAssetManager* g_AssetManager = nullptr;
static std::string g_InternalStoragePath = "/data/data/com.disney.splitsecond/files";

void SetAndroidAssetManager(AAssetManager* mgr, const char* internalPath) {
    g_AssetManager = mgr;
    if (internalPath) {
        g_InternalStoragePath = internalPath;
    }
    LOGI("Android AssetManager set. Storage path: %s", g_InternalStoragePath.c_str());
}

// ============================================================================
// Android Time Provider
// ============================================================================
class AndroidTimeProvider : public ITimeProvider {
public:
    AndroidTimeProvider() : m_start(std::chrono::steady_clock::now()) {}

    virtual uint64_t GetTimeMicroseconds() override {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(now - m_start).count();
    }

    virtual double GetTimeSeconds() override {
        return static_cast<double>(GetTimeMicroseconds()) / 1000000.0;
    }

    virtual void SleepMicroseconds(uint64_t us) override {
        std::this_thread::sleep_for(std::chrono::microseconds(us));
    }

private:
    std::chrono::steady_clock::time_point m_start;
};

// ============================================================================
// Android Filesystem (AAssetManager Bridge for APK Assets)
// ============================================================================
class AndroidFileSystem : public IFileSystem {
public:
    virtual std::string GetBaseDirectory() const override {
        return ""; // Relative to APK assets/
    }

    virtual std::string GetSaveDirectory() const override {
        return g_InternalStoragePath + "/saves";
    }

    virtual bool FileExists(const std::string& path) const override {
        // First check APK assets
        if (g_AssetManager) {
            AAsset* asset = AAssetManager_open(g_AssetManager, path.c_str(), AASSET_MODE_UNKNOWN);
            if (asset) {
                AAsset_close(asset);
                return true;
            }
        }
        // Then check internal storage
        std::string fullPath = g_InternalStoragePath + "/" + path;
        std::ifstream f(fullPath);
        return f.good();
    }

    virtual std::vector<uint8_t> ReadAllBytes(const std::string& path) const override {
        std::vector<uint8_t> buffer;

        // Try opening from APK assets
        if (g_AssetManager) {
            AAsset* asset = AAssetManager_open(g_AssetManager, path.c_str(), AASSET_MODE_BUFFER);
            if (asset) {
                off_t length = AAsset_getLength(asset);
                buffer.resize(length);
                AAsset_read(asset, buffer.data(), length);
                AAsset_close(asset);
                LOGI("Read %zu bytes from APK asset: %s", buffer.size(), path.c_str());
                return buffer;
            }
        }

        // Try reading from app storage
        std::string fullPath = g_InternalStoragePath + "/" + path;
        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
        if (file.is_open()) {
            std::streamsize size = file.tellg();
            file.seekg(0, std::ios::beg);
            buffer.resize(size);
            if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
                LOGI("Read %zu bytes from storage: %s", buffer.size(), fullPath.c_str());
            }
        } else {
            LOGE("Failed to find file: %s (in assets or storage)", path.c_str());
        }

        return buffer;
    }

    virtual bool WriteAllBytes(const std::string& path, const uint8_t* data, size_t size) override {
        std::string fullPath = g_InternalStoragePath + "/" + path;
        std::ofstream file(fullPath, std::ios::binary);
        if (!file.is_open()) {
            LOGE("Failed to write to %s", fullPath.c_str());
            return false;
        }
        file.write(reinterpret_cast<const char*>(data), size);
        return true;
    }
};

// ============================================================================
// Android Input System (Virtual Touchscreen Controls & Gamepad)
// ============================================================================
class AndroidInputSystem : public IInputSystem {
public:
    virtual void Poll() override {
        // Touch events are updated asynchronously via JNI / NativeActivity callbacks
    }

    virtual bool IsKeyDown(KeyCode key) const override {
        return false;
    }

    virtual bool IsGamepadButtonDown(uint32_t padIndex, GamepadButton button) const override {
        if (padIndex != 0) return false;
        return (m_padState.buttons & (1 << static_cast<uint32_t>(button))) != 0;
    }

    virtual GamepadState GetGamepadState(uint32_t padIndex) const override {
        if (padIndex == 0) return m_padState;
        return GamepadState{};
    }

    virtual void SetVibration(uint32_t padIndex, float leftMotor, float rightMotor) override {
        // Android Haptic Feedback through JNI Vibrator
    }

    // Touch Event Injection from Android Native Activity
    void InjectTouch(float x, float y, int action, int pointerId) {
        // Action: 0 = DOWN, 1 = UP, 2 = MOVE
        // Left half of screen (x < 0.5): Virtual Steering Stick
        if (x < 0.5f) {
            if (action == 1) { // UP
                m_padState.leftStickX = 0.0f;
                m_padState.leftStickY = 0.0f;
            } else {
                // Map x [0.0 .. 0.5] to [-1.0 .. 1.0]
                m_padState.leftStickX = (x / 0.25f) - 1.0f;
            }
        } else {
            // Right half of screen: Throttle, Brake, Powerplay
            // Top Right: Powerplay Trigger 1 (A button)
            // Bottom Right: Throttle (Right Trigger)
            if (action == 1) {
                m_padState.rightTrigger = 0.0f;
                m_padState.buttons &= ~(1 << static_cast<uint32_t>(GamepadButton::A));
            } else {
                if (y > 0.5f) {
                    m_padState.rightTrigger = 1.0f; // Accelerate
                } else {
                    m_padState.buttons |= (1 << static_cast<uint32_t>(GamepadButton::A)); // Detonate Powerplay
                }
            }
        }
    }

private:
    GamepadState m_padState{true, 0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
};

// ============================================================================
// Android Window & Lifecycle
// ============================================================================
class AndroidWindow : public IWindow {
public:
    virtual bool Initialize(const WindowDesc& desc) override {
        m_width = desc.width;
        m_height = desc.height;
        LOGI("Android Window initialized: %ux%u", m_width, m_height);
        return true;
    }

    virtual bool ProcessEvents() override {
        return m_active;
    }

    virtual void SwapBuffers() override {
        // EGL swap buffer handled by native activity
    }

    virtual uint32_t GetWidth() const override { return m_width; }
    virtual uint32_t GetHeight() const override { return m_height; }
    virtual bool IsActive() const override { return m_active; }
    virtual void* GetNativeHandle() const override { return nullptr; }

    void SetActive(bool active) { m_active = active; }
    void SetDimensions(uint32_t w, uint32_t h) { m_width = w; m_height = h; }

private:
    uint32_t m_width = 1920;
    uint32_t m_height = 1080;
    bool m_active = true;
};

// ============================================================================
// Android Graphics Device (OpenGL ES 3.0)
// ============================================================================
class AndroidGraphicsDevice : public IGraphicsDevice {
public:
    virtual bool Initialize(IWindow* window) override {
        LOGI("Android OpenGL ES 3.0 Graphics Device Initializing...");
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        return true;
    }

    virtual void BeginFrame() override {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    virtual void EndFrame() override {}

    virtual void Clear(uint32_t color, float depth, uint32_t stencil) override {
        float r = ((color >> 16) & 0xFF) / 255.0f;
        float g = ((color >> 8) & 0xFF) / 255.0f;
        float b = (color & 0xFF) / 255.0f;
        float a = ((color >> 24) & 0xFF) / 255.0f;
        glClearColor(r, g, b, a);
        glClearDepthf(depth);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) override {
        glViewport(x, y, width, height);
    }

    virtual void DrawPrimitive(PrimitiveType type, uint32_t startVertex, uint32_t primitiveCount) override {
        GLenum mode = GL_TRIANGLES;
        if (type == PrimitiveType::TriangleStrip) mode = GL_TRIANGLE_STRIP;
        else if (type == PrimitiveType::LineList) mode = GL_LINES;
        else if (type == PrimitiveType::PointList) mode = GL_POINTS;

        uint32_t count = (type == PrimitiveType::TriangleList) ? primitiveCount * 3 : primitiveCount;
        glDrawArrays(mode, startVertex, count);
    }

    virtual void Shutdown() override {
        LOGI("Android Graphics Device Shutdown.");
    }
};

// ============================================================================
// Android Audio Subsystem (FMOD Core / OpenSL ES Bridge)
// ============================================================================
class AndroidAudioSubsystem : public IAudioSubsystem {
public:
    virtual bool Initialize() override {
        LOGI("Android Audio Subsystem Initialized via OpenSL ES / FMOD.");
        return true;
    }

    virtual void Update() override {}

    virtual void PlayEvent(const std::string& eventPath, float volume) override {
        LOGI("Play audio event: %s (vol=%.2f)", eventPath.c_str(), volume);
    }

    virtual void StopAllEvents() override {}

    virtual void SetListenerPosition(float x, float y, float z, float fwdX, float fwdY, float fwdZ) override {}

    virtual void Shutdown() override {}
};

PlatformContext CreatePlatform(bool headless) {
    if (headless) {
        // Return null platform
        extern PlatformContext CreateNullPlatform();
        return CreateNullPlatform();
    }

    PlatformContext ctx;
    ctx.time = std::make_unique<AndroidTimeProvider>();
    ctx.fileSystem = std::make_unique<AndroidFileSystem>();
    ctx.input = std::make_unique<AndroidInputSystem>();
    ctx.window = std::make_unique<AndroidWindow>();
    ctx.graphics = std::make_unique<AndroidGraphicsDevice>();
    ctx.audio = std::make_unique<AndroidAudioSubsystem>();
    return ctx;
}

} // namespace Platform
} // namespace SSV

#endif // __ANDROID__
