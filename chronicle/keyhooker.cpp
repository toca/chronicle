#include "keyhooker.h"

#include <Windows.h>

#include <optional>
#include <format>

#include "result.h"


thread_local KeyHooker* self;

KeyHooker::KeyHooker(HookListenerType listener)
    : neighbor(0)
    , window(0)
    , listener(listener)
{
}

KeyHooker::~KeyHooker()
{
}

std::optional<Error> KeyHooker::Start()
{
    // CreateWindow
    const char* WindowClassName = "hook_window_class";
    HMODULE instance = ::GetModuleHandleW(nullptr);
    WNDCLASSA windowClass{};
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = instance;
    windowClass.lpszClassName = WindowClassName;
    if (!::RegisterClassA(&windowClass)) {
        return Error(::GetLastError(), "Failed to RegisterClass");
    }

    window = ::CreateWindowExA(
        0,
        WindowClassName,
        "hook_window",
        0,
        0, 0,
        0, 0,
        nullptr,
        nullptr,
        instance,
        nullptr
    );

    if (!window) {
        return Error(::GetLastError(), "Failed to CreateWindowEx");
    }

    self = this;

    // Key Hook
    this->neighbor = ::SetWindowsHookExA(WH_KEYBOARD_LL, HookProc, instance, 0);


    // Message Loop
    MSG msg{};
    while (::GetMessage(&msg, window, 0, 0)) {
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
    return std::nullopt;
}

std::optional<Error> KeyHooker::Stop()
{
    // unhook
    if (!this->neighbor) {
        return Error(ERROR_INVALID_HANDLE, "Not started");
    }
    ::UnhookWindowsHookEx(this->neighbor);

    ::SendMessage(this->window, WM_DESTROY, 0, 0);

    return std::nullopt;
}



LRESULT CALLBACK KeyHooker::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_DESTROY:
            ::PostQuitMessage(0);
            return 0;
        default:
            break;
    }
    return ::DefWindowProc(hwnd, message, wParam, lParam);
}


LRESULT KeyHooker::HookProc(int code, WPARAM wParam, LPARAM lParam)
{
    if (code == HC_ACTION) {
        PKBDLLHOOKSTRUCT p = (PKBDLLHOOKSTRUCT)lParam;
        if (wParam == WM_KEYDOWN) {
            if (self->listener(*p)) {
                //::CallNextHookEx(nullptr, code, wParam, lParam);
                return 1;
                //return 0;
            }
        }
    }
    return ::CallNextHookEx(nullptr, code, wParam, lParam);
}


