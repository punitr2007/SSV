#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <xinput.h>
#include "platform_abstraction.h"
#include <iostream>

namespace SSV {
namespace Platform {

class Win32Window : public IWindow {
    HWND m_hwnd = nullptr;
    HINSTANCE m_hinstance = nullptr;
    uint32_t m_width = 1280;
    uint32_t m_height = 720;
    bool m_active = false;
    bool m_shouldClose = false;

    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        Win32Window* self = reinterpret_cast<Win32Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        switch (msg) {
            case WM_ACTIVATE:
                if (self) self->m_active = (LOWORD(wParam) != WA_INACTIVE);
                return 0;
            case WM_CLOSE:
            case WM_DESTROY:
                if (self) self->m_shouldClose = true;
                PostQuitMessage(0);
                return 0;
            default:
                break;
        }
        return DefWindowProcA(hwnd, msg, wParam, lParam);
    }

public:
    ~Win32Window() override {
        if (m_hwnd) {
            DestroyWindow(m_hwnd);
            m_hwnd = nullptr;
        }
        if (m_hinstance) {
            UnregisterClassA("SplitSecondVelocityClass", m_hinstance);
        }
    }

    bool Initialize(const WindowDesc& desc) override {
        m_width = desc.width;
        m_height = desc.height;
        m_hinstance = GetModuleHandleA(nullptr);

        WNDCLASSEXA wc = {};
        wc.cbSize = sizeof(WNDCLASSEXA);
        wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
        wc.lpfnWndProc = WndProc;
        wc.hInstance = m_hinstance;
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = "SplitSecondVelocityClass";

        if (!RegisterClassExA(&wc)) {
            return false;
        }

        RECT rect = { 0, 0, static_cast<LONG>(m_width), static_cast<LONG>(m_height) };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

        m_hwnd = CreateWindowExA(
            0,
            wc.lpszClassName,
            desc.title.c_str(),
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rect.right - rect.left, rect.bottom - rect.top,
            nullptr, nullptr, m_hinstance, nullptr
        );

        if (!m_hwnd) return false;

        SetWindowLongPtr(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
        m_active = true;
        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);
        return true;
    }

    bool ProcessEvents() override {
        MSG msg = {};
        // Decoupled message pump matching original binary PeekMessageA loop at 0x0044F9E7
        while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                m_shouldClose = true;
                return false;
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }
        return !m_shouldClose;
    }

    void SwapBuffers() override {}
    uint32_t GetWidth() const override { return m_width; }
    uint32_t GetHeight() const override { return m_height; }
    bool IsActive() const override { return m_active; }
    void* GetNativeHandle() const override { return static_cast<void*>(m_hwnd); }
};

} // namespace Platform
} // namespace SSV
#endif
