#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

namespace {
void need(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
template<class F> void refuses(F&& action) {
    bool refused=false;
    try { action(); } catch (const std::exception&) { refused=true; }
    need(refused, "Missing expected art bootstrap/adoption refusal");
}
struct Window {
    HWND h=CreateWindowExW(0,L"STATIC",L"Civis art bootstrap CPU check",WS_OVERLAPPEDWINDOW,
        0,0,841,621,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    ~Window() { if(h) DestroyWindow(h); }
};
}
int main(int argc,char** argv) {
    try {
        need(argc==4,"Expected SAVE ORIGINAL_GAME CANONICAL_ASSETS");
        Window window; need(window.h != nullptr,"Actual owned window missing");
        std::uint64_t revision=1;
        auto client=civ5::runtime::bnw_stock_lua_win32_client_extent(
            reinterpret_cast<std::uintptr_t>(window.h),"early-art-bootstrap",[&]{return revision;});
        auto lease=civ5::ui::BnwNativeEarlyInputLease::acquire(argv[1],argv[2],argv[3],
            reinterpret_cast<std::uintptr_t>(window.h),client);
        auto package=civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);
        const auto* table=&package->resources();
        refuses([&]{(void)civ5::ui::bootstrap_selected_bnw_preview_from_early_package(package,0);});
        package->register_native_dx9_primary_prefix();
        auto settings=civ5::ui::BnwNativeResourceBootSettingsOwner::create(package);
        package->register_native_dx9_texture_quality_branch(settings);
        package->register_native_common_high_quality_prefix();
        auto boot=civ5::ui::bootstrap_selected_bnw_preview_from_early_package(package,0);
        if(!boot.preview) throw std::runtime_error(boot.preview.blocker ?
            boot.preview.blocker->reason : "No early art bootstrap result");
        std::shared_ptr<civ5::ui::SelectedBnwPreviewHost> host(std::move(boot.preview.host));
        need(&package->resources()==table && package->early_input_lease().get()==lease.get(),
             "Art bootstrap replaced early lease/table");
        package->require_same_selected_host(*host);
        const auto& database=host->database();
        constexpr std::array<const char*,5> tables{
            "ArtDefine_UnitInfos","ArtDefine_UnitInfoMemberInfos","ArtDefine_UnitMemberInfos",
            "ArtDefine_UnitMemberCombats","ArtDefine_UnitMemberCombatWeapons"};
        constexpr std::array<std::int64_t,5> minimum{103,161,161,161,166};
        for(std::size_t n=0;n<tables.size();++n) {
            std::int64_t count=-1;
            const auto report=database.query_rows({std::string("SELECT COUNT(*) AS count FROM ")+tables[n],{}},
                [&](const civ5::content::RulesRowView& row){count=row.integer("count");},1);
            need(report.all_succeeded() && count>=minimum[n],"Real final rules lost base art rows");
            std::cout<<tables[n]<<'='<<count<<'\n';
        }
        need(database.immutable_preview_fingerprints_current() &&
             !host->session().readiness().can_continue(),"Art adoption changed game readiness");
        refuses([&]{(void)civ5::ui::bootstrap_selected_bnw_preview_from_early_package(package,0);});
        auto other=civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);
        refuses([&]{other->require_same_selected_host(*host);});
        auto color=civ5::ui::BnwNativeUiColorOwner::create_from_adopted_host(host);
        need(!color->global_colors().empty() &&
             &color->selected_header()==&host->session().source().source.header,
             "Adopted Color detached from real selected host");
        refuses([&]{(void)color->quote("0,0,0,0");}); // mandatory category/Skin source still pending
        ++revision;
        refuses([&]{package->require_same_selected_host(*host);});
        refuses([&]{(void)color->global_colors();});
        std::cout<<"PASS real early table -> mutable BASE art -> DLC/postprocess -> private sealed clone "
                     "adoption/Color; duplicate/foreign/stale refused; full game/Skin still unavailable\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
