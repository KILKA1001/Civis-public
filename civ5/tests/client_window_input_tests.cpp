#include "civ5/ui/client_window_input.hpp"
#include "civ5/ui/win32_client_window.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5::ui;
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
void portable() {
    ClientWindowInput input;
    input.set_viewport({800, 600, 1200, 900, 144});
    const auto &view = input.viewport();
    check(view.framebuffer_point({400, 300}) == ClientPoint{600, 450}, "Framebuffer ratio differs");
    check(view.dip_point({400, 300}) == ClientPoint{400, 300}, "DPI conversion doubled scale");
    check(view.framebuffer_point({-1, -1}) == ClientPoint{-2, -2}, "Negative pointer rounded into image");
    check(input.keyboard_owner() == ClientInputOwner::none, "Unmeasured resize accepts camera keys");
    const ClientRect bar{0, 0, 1200, 48};
    input.set_ui(std::span(&bar, 1), false);
    auto down = input.pointer(ClientPointerKind::left_down, {400, 300});
    check(down.owner == ClientInputOwner::camera && down.capture, "Map press not captured");
    check(input.pointer(ClientPointerKind::move, {400, 10}).owner == ClientInputOwner::camera,
          "Camera gesture stolen by control crossing");
    check(input.set_ui(std::span(&bar, 1), true), "Modal failed to cancel map gesture");
    auto blocked = input.pointer(ClientPointerKind::move, {400, 300});
    check(blocked.owner == ClientInputOwner::ui && blocked.cancel_camera, "Modal leaked camera input");
    check(input.keyboard_owner() == ClientInputOwner::ui, "Modal leaked camera keys");
    check(input.pointer(ClientPointerKind::left_up, {400, 300}).owner == ClientInputOwner::none,
          "Cancelled gesture release became UI action");
    input.set_ui(std::span(&bar, 1), false);
    check(input.pointer(ClientPointerKind::left_down, {400, 10}).owner == ClientInputOwner::ui,
          "Measured UI press passed to camera");
    check(input.pointer(ClientPointerKind::move, {400, 300}).owner == ClientInputOwner::ui,
          "UI drag escaped to map");
    check(input.pointer(ClientPointerKind::left_up, {-20, 700}).owner == ClientInputOwner::ui,
          "Captured outside release lost its owner");
    input.set_ui({}, false, true);
    check(input.keyboard_owner() == ClientInputOwner::ui, "Text focus leaked camera keys");
    input.cancel();
    check(input.keyboard_owner() == ClientInputOwner::camera, "Blur retained text focus");
    input.pointer(ClientPointerKind::left_down, {400, 300});
    check(input.set_viewport({0, 0, 0, 0, 192}), "Minimize did not cancel camera drag");
    check(!input.pointer(ClientPointerKind::wheel, {1, 1}).framebuffer, "Minimized frame mapped pointer");
    input.set_viewport({1920, 1080, 1920, 1080, 192});
    input.set_ui({}, false);
    check(input.viewport().framebuffer_point({1919, 1079}) == ClientPoint{1919, 1079},
          "Per-monitor physical coordinates double scaled");
}
#ifdef _WIN32
LRESULT CALLBACK original(HWND window, UINT message, WPARAM w, LPARAM l) {
    return DefWindowProcW(window, message, w, l);
}
void native() {
    const auto instance = GetModuleHandleW(nullptr);
    WNDCLASSW cls{}; cls.lpfnWndProc = &original; cls.hInstance = instance;
    cls.lpszClassName = L"CivisClientWindowTest";
    check(RegisterClassW(&cls) != 0, "Cannot register test window");
    const auto window = CreateWindowExW(0, cls.lpszClassName, L"", WS_OVERLAPPEDWINDOW,
                                      0, 0, 800, 600, nullptr, nullptr, instance, nullptr);
    check(window != nullptr, "Cannot create hidden test window");
    SetWindowLongPtrW(window, GWLP_USERDATA, 12345);
    ClientWindowInput input;
    std::vector<ClientWindowEvent> events;
    {
        Win32ClientWindowBinding binding(window, input, [&](const auto &event) { events.push_back(event); });
        binding.set_framebuffer_size(1200, 900);
        const ClientRect bar{0, 0, 1200, 30};
        input.set_ui(std::span(&bar, 1), false);
        SendMessageW(window, WM_LBUTTONDOWN, MK_LBUTTON, MAKELPARAM(100, 100));
        check(events.back().owner == ClientInputOwner::camera && GetCapture() == window,
              "Native camera press/capture not delivered");
        input.set_ui({}, true);
        SendMessageW(window, WM_MOUSEMOVE, MK_LBUTTON, MAKELPARAM(110, 110));
        const auto modal_event = std::find_if(events.rbegin(), events.rend(), [](const auto &e) {
            return e.kind == ClientWindowEventKind::pointer;
        });
        check(modal_event != events.rend() && modal_event->owner == ClientInputOwner::ui &&
              modal_event->pointer.cancel_camera && GetCapture() != window,
              "Native modal camera cancellation missing");
        POINT screen{100, 100}; check(ClientToScreen(window, &screen), "Cannot query wheel screen point");
        SendMessageW(window, WM_MOUSEWHEEL, MAKEWPARAM(0, 120), MAKELPARAM(screen.x, screen.y));
        check(events.back().pointer_kind == ClientPointerKind::wheel &&
              events.back().wheel_delta == 120 && events.back().owner == ClientInputOwner::ui &&
              events.back().pointer.framebuffer == input.viewport().framebuffer_point({100, 100}),
              "Native wheel coordinate conversion differs from pointer");
        SendMessageW(window, WM_KEYDOWN, 'W', 0);
        check(events.back().owner == ClientInputOwner::ui, "Native modal leaked key");
        SendMessageW(window, WM_KILLFOCUS, 0, 0);
        check(input.gesture_owner() == ClientInputOwner::none, "Focus loss retained gesture");
        check(!binding.failed() && GetWindowLongPtrW(window, GWLP_USERDATA) == 12345,
              "Native binding failed or replaced renderer userdata");
    }
    check(reinterpret_cast<WNDPROC>(GetWindowLongPtrW(window, GWLP_WNDPROC)) == &original,
          "Original window procedure not restored");
    DestroyWindow(window);
    UnregisterClassW(cls.lpszClassName, instance);
}
#endif
} // namespace
int main() {
    try { portable();
#ifdef _WIN32
        native();
#endif
        std::cout << "client window/input tests passed\n";
        return 0;
    } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
