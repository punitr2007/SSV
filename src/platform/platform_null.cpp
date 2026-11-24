#include "platform_abstraction.h"
#include <chrono>
#include <thread>
#include <fstream>
#include <filesystem>
#include <iostream>

namespace SSV {
namespace Platform {

// ============================================================================
// Standard C++ Time Provider
// ============================================================================
class StandardTimeProvider : public ITimeProvider {
    std::chrono::high_resolution_clock::time_point m_startTime;
public:
    StandardTimeProvider() : m_startTime(std::chrono::high_resolution_clock::now()) {}

    uint64_t GetTimeMicroseconds() override {
        auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(now - m_startTime).count();
    }

    double GetTimeSeconds() override {
        return static_cast<double>(GetTimeMicroseconds()) / 1000000.0;
    }

    void SleepMicroseconds(uint64_t us) override {
        std::this_thread::sleep_for(std::chrono::microseconds(us));
    }
};

// ============================================================================
// Standard Cross-Platform FileSystem Provider
// ============================================================================
class StandardFileSystem : public IFileSystem {
    std::string m_baseDir;
    std::string m_saveDir;
public:
    StandardFileSystem() {
        m_baseDir = std::filesystem::current_path().string();
        m_saveDir = (std::filesystem::current_path() / "saves").string();
    }

    std::string GetBaseDirectory() const override { return m_baseDir; }
    std::string GetSaveDirectory() const override { return m_saveDir; }

    bool FileExists(const std::string& path) const override {
        return std::filesystem::exists(path);
    }

    std::vector<uint8_t> ReadAllBytes(const std::string& path) const override {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return {};
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        std::vector<uint8_t> buffer(static_cast<size_t>(size));
        if (file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            return buffer;
        }
        return {};
    }

    bool WriteAllBytes(const std::string& path, const uint8_t* data, size_t size) override {
        std::filesystem::create_directories(std::filesystem::path(path).parent_path());
        std::ofstream file(path, std::ios::binary);
        if (!file.is_open()) return false;
        file.write(reinterpret_cast<const char*>(data), size);
        return file.good();
    }
};

// ============================================================================
// Headless / Mock Implementations
// ============================================================================
class NullInputSystem : public IInputSystem {
public:
    void Poll() override {}
    bool IsKeyDown(KeyCode) const override { return false; }
    bool IsGamepadButtonDown(uint32_t, GamepadButton) const override { return false; }
    GamepadState GetGamepadState(uint32_t) const override { return {}; }
    void SetVibration(uint32_t, float, float) override {}
};

class NullWindow : public IWindow {
    uint32_t m_w = 1280;
    uint32_t m_h = 720;
    bool m_active = true;
    uint32_t m_ticks = 0;
public:
    bool Initialize(const WindowDesc& desc) override {
        m_w = desc.width;
        m_h = desc.height;
        m_active = true;
        return true;
    }
    bool ProcessEvents() override {
        // In headless mode, simulate event processing
        m_ticks++;
        return true;
    }
    void SwapBuffers() override {}
    uint32_t GetWidth() const override { return m_w; }
    uint32_t GetHeight() const override { return m_h; }
    bool IsActive() const override { return m_active; }
    void* GetNativeHandle() const override { return nullptr; }
};

class NullGraphicsDevice : public IGraphicsDevice {
public:
    bool Initialize(IWindow*) override { return true; }
    void BeginFrame() override {}
    void EndFrame() override {}
    void Clear(uint32_t, float, uint32_t) override {}
    void SetViewport(uint32_t, uint32_t, uint32_t, uint32_t) override {}
    void DrawPrimitive(PrimitiveType, uint32_t, uint32_t) override {}
    void Shutdown() override {}
};

class NullAudioSubsystem : public IAudioSubsystem {
public:
    bool Initialize() override { return true; }
    void Update() override {}
    void PlayEvent(const std::string&, float) override {}
    void StopAllEvents() override {}
    void SetListenerPosition(float, float, float, float, float, float) override {}
    void Shutdown() override {}
};

PlatformContext CreatePlatform(bool headless) {
    PlatformContext ctx;
    ctx.time = std::make_unique<StandardTimeProvider>();
    ctx.fileSystem = std::make_unique<StandardFileSystem>();
    ctx.input = std::make_unique<NullInputSystem>();
    ctx.window = std::make_unique<NullWindow>();
    ctx.graphics = std::make_unique<NullGraphicsDevice>();
    ctx.audio = std::make_unique<NullAudioSubsystem>();
    return ctx;
}

} // namespace Platform
} // namespace SSV
