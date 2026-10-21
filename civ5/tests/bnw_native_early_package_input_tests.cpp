#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include <iostream>
#include <filesystem>
#include <vector>
#include <stdexcept>
#include <thread>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
using namespace civ5;
namespace {
void check(bool value, const char* message) {
    if (!value) throw std::runtime_error(message);
}
template<class F> void refuses(F&& action) {
    bool refused = false;
    try { action(); } catch (const std::exception&) { refused = true; }
    check(refused, "Early package accepted missing/stale/foreign lease");
}
}
int main(int argc, char** argv) {
    try {
        check(argc == 4, "Actual save/original install/canonical payload required");
        struct Window { HWND h = nullptr; ~Window() { if (h) DestroyWindow(h); } } window;
        window.h = CreateWindowExW(0, L"STATIC", L"", WS_OVERLAPPEDWINDOW,
                                  0, 0, 841, 621, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
        check(window.h && !IsWindowVisible(window.h), "Actual owned hidden HWND required");
        std::uint64_t revision = 5;
        auto client = runtime::bnw_stock_lua_win32_client_extent(
            reinterpret_cast<std::uintptr_t>(window.h), "actual-early-package-input",
            [&] { return revision; });
        refuses([&] { ui::BnwNativePackageDiscoveryOwner::create(
            std::shared_ptr<const ui::BnwNativeEarlyInputLease>{}); });
        // No Rules bootstrap, SelectedHost, SQL, or Color owner has been made.
        auto lease = ui::BnwNativeEarlyInputLease::acquire(
            argv[1], argv[2], argv[3], reinterpret_cast<std::uintptr_t>(window.h), client);
        auto package = ui::BnwNativePackageDiscoveryOwner::create(lease);
        check(package->early_input_lease().get() == lease.get(), "Package detached from exact early lease");
        const auto* table = &package->resources();
        const auto& frame = package->current();
        const auto early_mounts = package->resources().mounts();
        const auto early_keys = package->resources().keys();
        // Audited installed canonical fixture: includes EUI and VPUI. Saved
        // GUID selection authenticates membership; it does not shrink this scan.
        // The supplied Original view must name this SAME physical DLC directory.
        check(std::filesystem::equivalent(lease->game_root()/"Assets"/"DLC",
              lease->canonical_payload_root()/"Assets"/"DLC"),
              "Early package oracle requires the audited shared canonical installed fixture");
        const std::vector<std::string> expected_installed_descriptors{
            "DLC/DLC_01/Mongol.Civ5Pkg",
            "DLC/DLC_02/SpainInca.Civ5Pkg",
            "DLC/DLC_03/Polynesia.Civ5Pkg",
            "DLC/DLC_04/Denmark.Civ5Pkg",
            "DLC/DLC_05/Korea.Civ5Pkg",
            "DLC/DLC_06/AncientWonders.Civ5Pkg",
            "DLC/DLC_07/CivComplete.Civ5Pkg",
            "DLC/DLC_Deluxe/Babylon.Civ5Pkg",
            "DLC/DLC_SP_Maps/DLC_SP_Maps.Civ5Pkg",
            "DLC/DLC_SP_Maps_2/DLC_SP_Maps_2.Civ5Pkg",
            "DLC/DLC_SP_Maps_3/DLC_SP_Maps_3.Civ5Pkg",
            "DLC/Expansion/Expansion1.Civ5Pkg",
            "DLC/Expansion2/Expansion2.Civ5Pkg",
            "DLC/Shared/Upgrade1.Civ5Pkg",
            "DLC/Tablet/Tablet.Civ5Pkg",
            "DLC/UI_bc1/EUI_0.Civ5Pkg",
            "DLC/UI_bc1/EUI_1.Civ5Pkg",
            "DLC/UI_bc1/EUI_2.Civ5Pkg",
            "DLC/VPUI/VPUI_0.Civ5Pkg",
            "DLC/VPUI/VPUI_1.Civ5Pkg",
            "DLC/VPUI/VPUI_2.Civ5Pkg",
        };
        constexpr std::size_t expected_installed_skins = 11;
        const auto expected_count = expected_installed_descriptors.size();
        const bool early_matches = frame.original_descriptor_order == expected_installed_descriptors &&
              frame.packages == expected_count && frame.descriptor_direct_reads == expected_count &&
              frame.descriptor_index_misses == expected_count && frame.skin_contexts == expected_installed_skins &&
              early_mounts.empty() && early_keys.empty() &&
              frame.archive_registration_requests == 0 && frame.archives_registered == 0;
        if (!early_matches) {
            std::cerr << "DIAGNOSTIC pre-DB actual packages=" << frame.packages
                      << " direct_reads=" << frame.descriptor_direct_reads
                      << " index_misses=" << frame.descriptor_index_misses
                      << " skin_contexts=" << frame.skin_contexts
                      << " mounts=" << early_mounts.size()
                      << " keys=" << early_keys.size()
                      << " archive_requests=" << frame.archive_registration_requests
                      << " registered_archives=" << frame.archives_registered << '\n';
            for(std::size_t ordinal=0;ordinal<frame.original_descriptor_order.size();++ordinal)
                std::cerr << "DIAGNOSTIC actual Original descriptor[" << ordinal << "]="
                          << frame.original_descriptor_order[ordinal] << '\n';
        }
        check(early_matches, "Pre-DB discovery fabricated registrations or selected universe");
        const auto registry = package->color_provider().current();
        check(registry && registry->unresolved_provider && !frame.color_source_set_closed &&
              !frame.native_skin_loading_complete, "Pre-DB discovery removed native Color/Skin guard");
        package->register_native_dx9_primary_prefix();
        check(&package->resources() == table && package->current().archives_registered == 32,
              "Primary registration replaced the early table");
        auto settings = ui::BnwNativeResourceBootSettingsOwner::create(package);
        settings->require_same_resources(*package);
        const auto setting = settings->current();
        check(setting.effective_texture_quality.has_value() && !setting.material_adapter_ready,
              "Settings reused Color or manufactured material startup");
        bool foreign_refused = false;
        std::thread foreign([&] {
            try { (void)package->early_input_lease(); }
            catch (const std::exception&) { foreign_refused = true; }
        });
        foreign.join();
        check(foreign_refused, "Early package accepted foreign thread");
        ++revision;
        refuses([&] { (void)package->current(); });
        refuses([&] { (void)settings->current(); });
        std::cout << "PASS: real lease-only pre-DB discovery and SAME early32 table/settings; "
                     "foreign thread/client revision refused; no host adoption, Color/Skin or gameplay readiness\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
