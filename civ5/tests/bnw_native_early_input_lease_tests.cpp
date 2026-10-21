#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include <iostream>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace {
void need(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template<class F> void rejects(F&& f) {
    bool refused = false;
    try { f(); } catch (const std::exception&) { refused = true; }
    need(refused, "Expected early input refusal");
}
struct Window {
    HWND value = CreateWindowExW(0, L"STATIC", L"Civis input lease CPU check",
        WS_OVERLAPPEDWINDOW, 0, 0, 160, 120, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    ~Window() { if (value) DestroyWindow(value); }
};
}
int main(int argc, char** argv) {
    try {
        need(argc == 4, "Expected SAVE ORIGINAL_GAME CANONICAL_ASSETS");
        Window window;
        need(window.value != nullptr, "Cannot create owned CPU test window");
        std::uint64_t revision = 1;
        auto client = civ5::runtime::bnw_stock_lua_win32_client_extent(
            reinterpret_cast<std::uintptr_t>(window.value), "early-input-test", [&] { return revision; });
        using Lease = civ5::ui::BnwNativeEarlyInputLease;
        rejects([&] { Lease::acquire(argv[1], argv[2], argv[3], 0, client); });
        auto lease = Lease::acquire(argv[1], argv[2], argv[3],
            reinterpret_cast<std::uintptr_t>(window.value), client);
        lease->require_current();
        need(lease->profile().can_activate && lease->profile().enabled_mods.empty() &&
             !lease->catalog().packages.empty(), "Real BNW selected input absent");
        need(&lease->selected_header() == &lease->selected_header(), "Header ownership changed");
        bool foreign_refused = false;
        std::thread other([&] { try { lease->require_current(); }
            catch (const std::exception&) { foreign_refused = true; } });
        other.join(); need(foreign_refused, "Foreign thread accepted");
        ++revision;
        rejects([&] { lease->require_current(); });
        --revision; lease->require_current();
        // The selected save is a workspace test input. Opening a writer must
        // fail at sharing negotiation; no bytes are written or truncated.
        const auto writer = CreateFileW(lease->save_path().c_str(), GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (writer != INVALID_HANDLE_VALUE) CloseHandle(writer);
        need(writer == INVALID_HANDLE_VALUE && GetLastError() == ERROR_SHARING_VIOLATION,
             "Selected input read epoch was writable");
        // The actual provider can mutate its own HWND and return the extent
        // sampled before that mutation. A pre-callback rectangle is insufficient.
        {
            RECT original{};
            need(GetWindowRect(window.value, &original), "Cannot capture original hidden window rectangle");
            bool resize_during_current = false;
            bool actual_resize_observed = false;
            auto callback_client = client;
            callback_client.current = [&] {
                const auto sampled = client.current();
                if (resize_during_current) {
                    resize_during_current = false;
                    need(SetWindowPos(window.value, nullptr, 0, 0,
                        original.right-original.left+37, original.bottom-original.top+19,
                        SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE), "Cannot resize actual hidden HWND");
                    RECT changed{};
                    actual_resize_observed = sampled && GetClientRect(window.value, &changed) &&
                        (changed.right-changed.left != sampled->width ||
                         changed.bottom-changed.top != sampled->height);
                }
                return sampled;
            };
            auto callback_lease = Lease::acquire(argv[1], argv[2], argv[3],
                reinterpret_cast<std::uintptr_t>(window.value), callback_client);
            resize_during_current = true;
            rejects([&] { callback_lease->require_current(); });
            // A provider-side API failure must not count as the expected
            // guard refusal: prove the real resize happened independently.
            need(actual_resize_observed, "Actual HWND client rectangle did not change inside provider");
            need(SetWindowPos(window.value, nullptr, 0, 0,
                original.right-original.left, original.bottom-original.top,
                SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE), "Cannot restore hidden HWND rectangle");
            callback_lease->require_current();
            lease->require_current();
        }
        {
            // The callback drops the only actual facade owner while an early
            // getter is executing. It must refuse before returning a reference.
            std::shared_ptr<const Lease> callback_lease;
            bool drop_during_current = false;
            auto callback_client = client;
            callback_client.current = [&] {
                const auto sampled = client.current();
                if (drop_during_current) {
                    drop_during_current = false;
                    callback_lease.reset();
                }
                return sampled;
            };
            callback_lease = Lease::acquire(argv[1], argv[2], argv[3],
                reinterpret_cast<std::uintptr_t>(window.value), callback_client);
            const auto* borrowed = callback_lease.get();
            drop_during_current = true;
            rejects([&] { (void)borrowed->catalog(); });
            need(!callback_lease, "Provider did not drop the actual facade owner");
        }
        {
            Window other_window;
            need(other_window.value != nullptr, "Cannot create callback-destruction HWND");
            auto other_client = civ5::runtime::bnw_stock_lua_win32_client_extent(
                reinterpret_cast<std::uintptr_t>(other_window.value), "early-callback-destroy", [&] {return revision;});
            bool destroy_during_current = false;
            auto callback_client = other_client;
            callback_client.current = [&] {
                const auto sampled = other_client.current();
                if (destroy_during_current) {
                    destroy_during_current = false;
                    need(DestroyWindow(other_window.value), "Cannot destroy actual provider HWND");
                    other_window.value = nullptr;
                }
                return sampled;
            };
            auto callback_lease = Lease::acquire(argv[1], argv[2], argv[3],
                reinterpret_cast<std::uintptr_t>(other_window.value), callback_client);
            destroy_during_current = true;
            rejects([&] { callback_lease->require_current(); });
            need(other_window.value == nullptr, "Provider did not destroy actual HWND");
        }
        DestroyWindow(window.value); window.value = nullptr;
        rejects([&] { lease->require_current(); });
        std::cout << "PASS real BNW early input/client epoch; foreign/stale/destroyed/"
                     "missing client and source-write refusals; no bootstrap/readiness claim\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n'; return 1;
    }
}
