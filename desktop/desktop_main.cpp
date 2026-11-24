#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <GL/gl.h>
#include <iostream>
#include <memory>
#include <chrono>
#include <string>

#include "game_engine.h"
#include "platform_abstraction.h"
#include "input_state.h"
#include "vehicle_physics.h"
#include "camera_rig.h"
#include "vehicle_renderer.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// WGL extension function prototypes
typedef HGLRC (WINAPI * PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC hDC, HGLRC hShareContext, const int *attribList);
typedef BOOL (WINAPI * PFNWGLSWAPINTERVALEXTPROC)(int interval);

#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001

static SSV::InputMapper g_InputMapper;
static bool g_Running = true;
static bool g_ResetRequested = false;
static int g_WindowWidth = 1280;
static int g_WindowHeight = 720;

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CLOSE:
        case WM_DESTROY:
            g_Running = false;
            PostQuitMessage(0);
            return 0;

        case WM_SIZE: {
            g_WindowWidth = LOWORD(lParam);
            g_WindowHeight = HIWORD(lParam);
            if (g_WindowHeight > 0) {
                glViewport(0, 0, g_WindowWidth, g_WindowHeight);
            }
            return 0;
        }

        case WM_KEYDOWN:
        case WM_KEYUP: {
            bool down = (msg == WM_KEYDOWN);
            switch (wParam) {
                case 'A':
                case VK_LEFT:
                    g_InputMapper.SetKeySteer(down ? -1.0f : 0.0f);
                    break;
                case 'D':
                case VK_RIGHT:
                    g_InputMapper.SetKeySteer(down ? 1.0f : 0.0f);
                    break;
                case 'W':
                case VK_UP:
                    g_InputMapper.SetKeyThrottle(down ? 1.0f : 0.0f);
                    break;
                case 'S':
                case VK_DOWN:
                    g_InputMapper.SetKeyBrake(down ? 1.0f : 0.0f);
                    break;
                case VK_SPACE:
                    g_InputMapper.SetKeyHandbrake(down);
                    break;
                case 'Q':
                case '1':
                    g_InputMapper.SetKeyPowerplay1(down);
                    break;
                case 'E':
                case '2':
                    g_InputMapper.SetKeyPowerplay2(down);
                    break;
                case 'R':
                    if (down) g_ResetRequested = true;
                    break;
                case VK_ESCAPE:
                    if (down) g_Running = false;
                    break;
            }
            return 0;
        }
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int main(int argc, char** argv) {
    std::cout << "==============================================================\n";
    std::cout << "  SPLIT/SECOND: VELOCITY - INTERACTIVE DRIVING & RENDER HARNESS\n";
    std::cout << "==============================================================\n";
    std::cout << "Controls:\n";
    std::cout << "  W / Up: Accelerate\n";
    std::cout << "  S / Down: Brake / Reverse\n";
    std::cout << "  A / D or Left / Right: Steer\n";
    std::cout << "  Space: Handbrake (Drift Mechanic)\n";
    std::cout << "  Q / 1: Powerplay Level 1\n";
    std::cout << "  E / 2: Powerplay Level 2\n";
    std::cout << "  R: Reset Car to Center\n";
    std::cout << "  ESC: Quit\n\n";

    // 1. Register Win32 Window Class
    WNDCLASSEXA wc = { sizeof(WNDCLASSEXA) };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.lpszClassName = "SSVDesktopHarness";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassExA(&wc);

    // 2. Create Window
    RECT rect = { 0, 0, g_WindowWidth, g_WindowHeight };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    HWND hwnd = CreateWindowExA(
        0, wc.lpszClassName,
        "Split/Second: Velocity - Interactive Vehicle Simulation (60 FPS)",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, wc.hInstance, nullptr
    );

    HDC hdc = GetDC(hwnd);

    // 3. Setup Pixel Format for OpenGL
    PIXELFORMATDESCRIPTOR pfd = { sizeof(PIXELFORMATDESCRIPTOR), 1 };
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;

    int pf = ChoosePixelFormat(hdc, &pfd);
    SetPixelFormat(hdc, pf, &pfd);

    HGLRC tempContext = wglCreateContext(hdc);
    wglMakeCurrent(hdc, tempContext);

    // 4. Create OpenGL 3.3 Core Profile Context
    auto wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
    HGLRC glContext = tempContext;
    if (wglCreateContextAttribsARB) {
        int attribs[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
            WGL_CONTEXT_MINOR_VERSION_ARB, 3,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0
        };
        HGLRC coreContext = wglCreateContextAttribsARB(hdc, nullptr, attribs);
        if (coreContext) {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(tempContext);
            wglMakeCurrent(hdc, coreContext);
            glContext = coreContext;
            std::cout << "[Desktop Harness] Initialized OpenGL 3.3 Core Profile.\n";
        }
    }

    std::cout << "[Desktop Harness] GL Vendor:   " << (const char*)glGetString(GL_VENDOR) << "\n";
    std::cout << "[Desktop Harness] GL Renderer: " << (const char*)glGetString(GL_RENDERER) << "\n";
    std::cout << "[Desktop Harness] GL Version:  " << (const char*)glGetString(GL_VERSION) << "\n";

    // Enable VSync
    auto wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
    if (wglSwapIntervalEXT) {
        wglSwapIntervalEXT(1);
    }

    // 5. Initialize Engine Subsystems
    SSV::Core::GameEngine engine;
    SSV::Platform::WindowDesc desc;
    desc.title = "Split/Second Desktop";
    desc.width = g_WindowWidth;
    desc.height = g_WindowHeight;
    desc.fullscreen = false;
    engine.Initialize(desc, false);

    // 6. Initialize Vehicle Simulation, Chase Camera, and 3D Renderer
    SSV::Physics::VehicleSimulation vehicleSim;
    vehicleSim.Reset({0.0f, 0.40f, 0.0f});

    SSV::CameraRig cameraRig;
    cameraRig.Reset(vehicleSim.GetPosition(), vehicleSim.GetOrientation());

    SSV::VehicleRenderer renderer;
    if (!renderer.Initialize()) {
        std::cerr << "[Desktop Harness] Error: Failed to initialize vehicle renderer!\n";
        return 1;
    }

    // 7. Main Loop with Fixed 60 Hz Timestep
    constexpr double FIXED_TIMESTEP = 1.0 / 60.0;
    auto lastTime = std::chrono::high_resolution_clock::now();
    double accumulator = 0.0;
    uint32_t currentTick = 0;
    uint32_t frameCount = 0;
    SSV::InputState lastInput;

    while (g_Running) {
        MSG msg;
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        if (g_ResetRequested) {
            vehicleSim.Reset({0.0f, 0.40f, 0.0f});
            cameraRig.Reset(vehicleSim.GetPosition(), vehicleSim.GetOrientation());
            g_ResetRequested = false;
            std::cout << "[Desktop Harness] Vehicle position reset to origin.\n";
        }

        auto currentTime = std::chrono::high_resolution_clock::now();
        double frameTime = std::chrono::duration<double>(currentTime - lastTime).count();
        lastTime = currentTime;
        accumulator += frameTime;

        // Fixed simulation ticks (60 Hz)
        while (accumulator >= FIXED_TIMESTEP) {
            lastInput = g_InputMapper.Sample(currentTick);

            vehicleSim.Step(lastInput, static_cast<float>(FIXED_TIMESTEP));
            cameraRig.Update(
                vehicleSim.GetPosition(),
                vehicleSim.GetOrientation(),
                vehicleSim.GetSpeedMph(),
                static_cast<float>(FIXED_TIMESTEP)
            );

            engine.StepFrame(static_cast<float>(FIXED_TIMESTEP));
            accumulator -= FIXED_TIMESTEP;
            currentTick++;
        }

        // Render pass
        glClearColor(0.06f, 0.07f, 0.10f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        float aspectRatio = (float)g_WindowWidth / (float)(g_WindowHeight > 0 ? g_WindowHeight : 1);
        bool isBraking = (lastInput.brake > 0.05f || lastInput.handbrake);
        renderer.RenderScene(cameraRig, vehicleSim, aspectRatio, isBraking);

        SwapBuffers(hdc);
        frameCount++;

        // Live Telemetry update on Window Title (every 10 frames)
        if (frameCount % 10 == 0) {
            char titleBuf[256];
            snprintf(titleBuf, sizeof(titleBuf),
                "Split/Second | Speed: %3.0f MPH (Gear %d, %4.0f RPM) | Drift: %s | Steer: %+1.2f | [R] Reset",
                vehicleSim.GetSpeedMph(),
                vehicleSim.GetCurrentGear(),
                vehicleSim.GetEngineRpm(),
                vehicleSim.IsDrifting() ? "DRIFTING!" : "GRIP",
                lastInput.steer
            );
            SetWindowTextA(hwnd, titleBuf);
        }
    }

    renderer.Shutdown();
    engine.Stop();
    wglMakeCurrent(nullptr, nullptr);
    wglDeleteContext(glContext);
    ReleaseDC(hwnd, hdc);
    DestroyWindow(hwnd);

    std::cout << "[Desktop Harness] Closed cleanly.\n";
    return 0;
}
