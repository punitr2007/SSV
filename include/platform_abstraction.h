#pragma once
/**
 * @file platform_abstraction.h
 * @brief Hardware and OS Abstraction Layer (HAL) for Split/Second: Velocity Reimplementation.
 *
 * Isolates all platform-specific dependencies:
 *  - Win32 Windowing, Message Loop, and DPI scaling
 *  - Graphics Device (Direct3D 9 / OpenGL / Vulkan abstraction)
 *  - Audio Subsystem (FMOD Ex / OpenAL / Miniaudio abstraction)
 *  - Input Subsystem (DirectInput 8 / XInput / SDL GameController abstraction)
 *  - Filesystem & Registry (Safe relative pathing, save game roaming paths)
 *  - Threading & Concurrency (Decoupled simulation and UI message pump)
 */

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace SSV {
namespace Platform {

// ============================================================================
// 1. Timing & Clock Abstraction
// ============================================================================
class ITimeProvider {
public:
    virtual ~ITimeProvider() = default;
    virtual uint64_t GetTimeMicroseconds() = 0;
    virtual double GetTimeSeconds() = 0;
    virtual void SleepMicroseconds(uint64_t us) = 0;
};

// ============================================================================
// 2. Input System Abstraction
// ============================================================================
enum class KeyCode : uint16_t {
    Unknown = 0,
    Escape, Enter, Space, Backspace, Tab,
    Up, Down, Left, Right,
    W, A, S, D,
    LeftShift, RightShift, LeftCtrl, RightCtrl,
    F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12
};

enum class GamepadButton : uint16_t {
    None = 0,
    A, B, X, Y,
    LeftBumper, RightBumper,
    LeftTrigger, RightTrigger,
    Start, Back, Guide,
    DPadUp, DPadDown, DPadLeft, DPadRight
};

struct GamepadState {
    bool connected = false;
    uint32_t buttons = 0;
    float leftTrigger = 0.0f;
    float rightTrigger = 0.0f;
    float leftStickX = 0.0f;
    float leftStickY = 0.0f;
    float rightStickX = 0.0f;
    float rightStickY = 0.0f;
};

class IInputSystem {
public:
    virtual ~IInputSystem() = default;
    virtual void Poll() = 0;
    virtual bool IsKeyDown(KeyCode key) const = 0;
    virtual bool IsGamepadButtonDown(uint32_t padIndex, GamepadButton button) const = 0;
    virtual GamepadState GetGamepadState(uint32_t padIndex) const = 0;
    virtual void SetVibration(uint32_t padIndex, float leftMotor, float rightMotor) = 0;
};

// ============================================================================
// 3. Window & Event System Abstraction
// ============================================================================
struct WindowDesc {
    std::string title = "Split/Second: Velocity";
    uint32_t width = 1280;
    uint32_t height = 720;
    bool fullscreen = false;
    bool vsync = true;
};

class IWindow {
public:
    virtual ~IWindow() = default;
    virtual bool Initialize(const WindowDesc& desc) = 0;
    virtual bool ProcessEvents() = 0; // Returns false on WM_QUIT / Close
    virtual void SwapBuffers() = 0;
    virtual uint32_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;
    virtual bool IsActive() const = 0;
    virtual void* GetNativeHandle() const = 0;
};

// ============================================================================
// 4. Graphics Device Abstraction
// ============================================================================
enum class PrimitiveType {
    TriangleList,
    TriangleStrip,
    LineList,
    PointList
};

class IGraphicsDevice {
public:
    virtual ~IGraphicsDevice() = default;
    virtual bool Initialize(IWindow* window) = 0;
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;
    virtual void Clear(uint32_t color, float depth, uint32_t stencil) = 0;
    virtual void SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) = 0;
    virtual void DrawPrimitive(PrimitiveType type, uint32_t startVertex, uint32_t primitiveCount) = 0;
    virtual void Shutdown() = 0;
};

// ============================================================================
// 5. Audio Subsystem Abstraction
// ============================================================================
class IAudioSubsystem {
public:
    virtual ~IAudioSubsystem() = default;
    virtual bool Initialize() = 0;
    virtual void Update() = 0;
    virtual void PlayEvent(const std::string& eventPath, float volume = 1.0f) = 0;
    virtual void StopAllEvents() = 0;
    virtual void SetListenerPosition(float x, float y, float z, float fwdX, float fwdY, float fwdZ) = 0;
    virtual void Shutdown() = 0;
};

// ============================================================================
// 6. Filesystem & Path Abstraction
// ============================================================================
class IFileSystem {
public:
    virtual ~IFileSystem() = default;
    virtual std::string GetBaseDirectory() const = 0;
    virtual std::string GetSaveDirectory() const = 0;
    virtual bool FileExists(const std::string& path) const = 0;
    virtual std::vector<uint8_t> ReadAllBytes(const std::string& path) const = 0;
    virtual bool WriteAllBytes(const std::string& path, const uint8_t* data, size_t size) = 0;
};

// ============================================================================
// 7. Unified Platform Context
// ============================================================================
struct PlatformContext {
    std::unique_ptr<ITimeProvider> time;
    std::unique_ptr<IFileSystem> fileSystem;
    std::unique_ptr<IInputSystem> input;
    std::unique_ptr<IWindow> window;
    std::unique_ptr<IGraphicsDevice> graphics;
    std::unique_ptr<IAudioSubsystem> audio;
};

/**
 * Creates the appropriate platform implementation.
 * @param headless If true, creates a mock/null platform suitable for CLI or unit tests.
 */
PlatformContext CreatePlatform(bool headless = false);

} // namespace Platform
} // namespace SSV
