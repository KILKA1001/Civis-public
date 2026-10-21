#include "bnw_native_owned_startup_completion.hpp"
#include "bnw_native_completed_color_query.hpp"
#include "bnw_native_completed_selected_read.hpp"
#include "civ5/ui/bnw_live_preview_launch.hpp"
#include "civ5/runtime/detail/bnw_civis_application_state_owner.hpp"
#include "detail/civis_selected_bnw_rules_preparation.hpp"
#include "civ5/ui/bnw_worldview_title_font_issuer.hpp"
#include "civ5/content/manifest.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
#include "civ5/runtime/bnw_selected_stock_lua_bootstrap.hpp"
#include "civ5/ui/bnw_owned_camera_ui_source_input.hpp"
#include "civ5/ui/bnw_live_scene_binding.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "bnw_native_common13_source.hpp"
#include "bnw_native_completed_resource_bank_capability.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/ui/bnw_native_resource_boot_settings_owner.hpp"
#include "civ5/graphics/bnw_scene_restore_receipt.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_selected_language_input_owner.hpp"
#include "civ5/saves/loaded_save.hpp"
#include "civ5/ui/top_panel.hpp"
#include <algorithm>
#include <atomic>
#include <map>
#include <mutex>
#include <thread>
#include <cctype>
#include <charconv>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <span>
#include <cstdint>

#include <chrono>
#include <cstdio>
#include <cerrno>
#include <exception>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace civ5::ui {
namespace {
// Diagnostic stderr only. No source/currentness/cache/ready state is read.
// Save native/CRT error state BEFORE clocks or I/O and restore it afterward.
class BnwLaunchStageTrace final {
    const char* const label_;
    std::chrono::steady_clock::time_point begin_{};
    int exceptions_=0;
 public:
    explicit BnwLaunchStageTrace(const char* label)noexcept:label_(label) {
        const int saved_errno=errno;
#if defined(_WIN32)
        const DWORD saved_error=GetLastError();
#endif
        begin_=std::chrono::steady_clock::now();
        exceptions_=std::uncaught_exceptions();
        const auto stamp=std::chrono::duration_cast<std::chrono::milliseconds>(begin_.time_since_epoch()).count();
        std::fprintf(stderr,"[BNW_STAGE] BEGIN %s steady_ms=%lld elapsed_ms=0\n",label_,static_cast<long long>(stamp));
        std::fflush(stderr);
#if defined(_WIN32)
        SetLastError(saved_error);
#endif
        errno=saved_errno;
    }
    BnwLaunchStageTrace(const BnwLaunchStageTrace&)=delete;
    BnwLaunchStageTrace& operator=(const BnwLaunchStageTrace&)=delete;
    ~BnwLaunchStageTrace()noexcept {
        const int saved_errno=errno;
#if defined(_WIN32)
        const DWORD saved_error=GetLastError();
#endif
        const auto now=std::chrono::steady_clock::now();
        const auto stamp=std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(now-begin_).count();
        std::fprintf(stderr,"[BNW_STAGE] END %s steady_ms=%lld elapsed_ms=%lld unwind=%d\n",label_,
            static_cast<long long>(stamp),static_cast<long long>(elapsed),std::uncaught_exceptions()>exceptions_?1:0);
        std::fflush(stderr);
#if defined(_WIN32)
        SetLastError(saved_error);
#endif
        errno=saved_errno;
    }
};
template<class Actual> decltype(auto) bnw_launch_stage(const char* label,Actual&& actual) {
    BnwLaunchStageTrace trace(label);
    return std::forward<Actual>(actual)(); // Exactly one original call/return.
}
} // namespace

detail::BnwNativeOwnedStartupCompletion::BnwNativeOwnedStartupCompletion(
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> package,
    std::shared_ptr<const BnwNativeCompletedResourceBankCapability> resource,
    std::shared_ptr<const BnwNativeCompletedColorQuery> color,
    std::shared_ptr<const BnwNativeCompletedSelectedRead> read)
    :package_(std::move(package)),resource_(std::move(resource)),color_(std::move(color)),read_(std::move(read)) {
    if(!package_||!resource_||!color_||!read_)throw std::runtime_error("Actual startup resource return inputs absent");
}
class detail::BnwNativeOwnedStartupCompletion::Pin final {
    const BnwNativeOwnedStartupCompletion& owner_;
    const std::shared_ptr<const BnwNativeOwnedStartupCompletion> storage_;
    static void need(bool ok,const char* why){if(!ok)throw std::runtime_error(why);}
  public:
    explicit Pin(const BnwNativeOwnedStartupCompletion& owner):owner_(owner),storage_(owner.self_.lock()) {
        need(storage_&&storage_.get()==&owner_&&!storage_.owner_before(owner_.self_)&&
             !owner_.self_.owner_before(storage_)&&owner_.pins_<static_cast<std::size_t>(std::numeric_limits<long>::max()),
             "Startup resource return has no canonical producer control");
        ++owner_.pins_;
    }
    ~Pin(){--owner_.pins_;}
    void current()const {
        need(!owner_.failed_&&storage_.use_count()>static_cast<long>(owner_.pins_),"Startup completion outside owner retired");
        if(owner_.checking_){owner_.failed_=true;throw std::runtime_error("Startup completion source reentry");}
        owner_.checking_=true;
        struct Exit {const BnwNativeOwnedStartupCompletion& owner;~Exit(){owner.checking_=false;}} exit{owner_};
        try {
            const auto canonical=owner_.package_->retain_factory_owner();
            need(canonical&&canonical.get()==owner_.package_.get()&&!canonical.owner_before(owner_.package_)&&
                 !owner_.package_.owner_before(canonical),"Startup completion canonical Parent control changed");
            owner_.resource_->require_identity_current();
            owner_.color_->require_same_parent(*owner_.package_);
            owner_.read_->require_same_parent(*owner_.package_);
            owner_.resource_->require_identity_current();
            need(!owner_.failed_&&storage_.use_count()>static_cast<long>(owner_.pins_),"Startup completion outside owner retired during source check");
        }catch(...){owner_.failed_=true;throw;}
    }
};
void detail::BnwNativeOwnedStartupCompletion::require_current()const {
    if(thread_!=std::this_thread::get_id())throw std::runtime_error("Startup completion foreign creator thread");
    Pin retained(*this);retained.current();
}
void detail::BnwNativeOwnedStartupCompletion::with_actual_completed_inputs(
    const std::function<void(const std::shared_ptr<const BnwNativePackageDiscoveryOwner>&,
        const std::shared_ptr<const BnwNativeCompletedResourceBankCapability>&,
        const std::shared_ptr<const BnwNativeCompletedColorQuery>&,
        const std::shared_ptr<const BnwNativeCompletedSelectedRead>&)>& input)const {
    if(thread_!=std::this_thread::get_id())throw std::runtime_error("Startup completion loan foreign creator thread");
    Pin retained(*this);
    if(borrowing_){failed_=true;throw std::runtime_error("Startup completion loan reentry");}
    borrowing_=true;
    struct Exit {const BnwNativeOwnedStartupCompletion& owner;~Exit(){owner.borrowing_=false;}} exit{*this};
    try {
        retained.current();{const auto consume=input;retained.current();
            if(!consume)throw std::runtime_error("Startup completion input consumer absent");
            consume(package_,resource_,color_,read_);retained.current();
        }retained.current(); // Includes target copy, invocation and destruction.
    }catch(...){failed_=true;throw;}
}

std::shared_ptr<const detail::BnwNativeOwnedStartupCompletion> detail::BnwNativeDefaultCommonProduction::advance_owned_initial_category_prefix(
    const std::shared_ptr<const BnwNativePackageDiscoveryOwner>& package) {
    if(!package)throw std::invalid_argument("Owned startup prefix has no actual Package");
    const auto actual_package=package->retain_factory_owner();
    if(!actual_package||actual_package.get()!=package.get()||actual_package.owner_before(package)||package.owner_before(actual_package))
        throw std::runtime_error("Owned startup requires canonical factory Package control");
    const auto lease=actual_package->early_input_lease();
    const auto* same_resources=&actual_package->resources();
    const auto require_same_package=[&] {
        BnwLaunchStageTrace boundary_trace("startup.package_boundary");
        lease->require_current();
        if(actual_package->early_input_lease().get()!=lease.get()||
           &actual_package->resources()!=same_resources)
            throw std::runtime_error("Owned startup prefix replaced actual lease/resource table");
        lease->require_current();
    };
    require_same_package();
    // Execute the genuine Default/Common query -> retained filename/read ->
    // owned Parser/Core/metadata success-only factory before any manager.
    bnw_launch_stage("default_receivers",[&]{return actual_package->prepare_native_early_package_receivers();});
    require_same_package();
    // Genuine standalone policy is derived from SAME immutable EarlyLease
    // BNW/enabled-DLC profile. Preserve real constructor fields and perform
    // actual disable/enable transitions before the first category query.
    bnw_launch_stage("configured_activation_policy",[&]{return actual_package->prepare_native_owned_configured_activation_policy();});
    require_same_package();
    // Actual first Common Paks query precedes constructor/cache append and
    // manager attachment; the retained outputs drive real archive operations.
    bnw_launch_stage("first_common_manager",[&]{return actual_package->prepare_native_owned_first_common_category_manager();});
    require_same_package();
    bnw_launch_stage("first_common_paks_effects",[&]{return actual_package->resume_native_owned_first_common_paks_effects();});
    require_same_package();
    // Query Directory/MapDirectory only after actual Paks effects, then run
    // real mode7 loose/File admission, Skin/Paks preference and Name effects.
    // Its preceding activation came from the actual configured lease policy.
    bnw_launch_stage("first_common_directory_file_effects",[&]{return actual_package->resume_native_owned_first_common_category_effects();});
    require_same_package();
    // Consume the already returned first Common journal, then execute every
    // actual returned Common and configured Gameplay package in native order.
    // UISkin construction/loading has its own genuine late issuer and fence.
    bnw_launch_stage("common13_gameplay",[&]{return actual_package->drive_native_owned_common_gameplay_categories();});
    require_same_package();
    //Issue closed factory/nthUISkin occurrences from SAME real Loaded/Parser.
    //The following genuine new Skin HeaderGuard retains its actual resource
    //loan; base Color returns BEFORE798CE0 constructor/strong roster traversal.
    bnw_launch_stage("completed_skin_occurrences",[&]{return actual_package->prepare_native_owned_skin_occurrences();});
    require_same_package();
    // Actual returned owner is held OUTSIDE both Bank and Skin callbacks.
    std::shared_ptr<const detail::BnwNativeCompletedResourceBankCapability> completed_owner;
    bnw_launch_stage("completedSkin_operation",[&]{
    actual_package->with_native_owned_skin_operation([&] {
        //Private barrier borrows SAME genuine active Header/Loaded fence and
        //compares captured lease/resource identities without public reentry.
        const auto require_same_active_skin=[&] {
            actual_package->require_native_owned_skin_operation_current(*lease,*same_resources);
        };
        require_same_active_skin();
        actual_package->prepare_native_owned_color_bank();
        require_same_active_skin();
        actual_package->construct_native_owned_skin_instances();
        require_same_active_skin();
        actual_package->bind_native_owned_color_bank_constructed_roster();
        require_same_active_skin();
        // Capture SAME genuine lease/language/settings policy only after the
        // real constructor transfer and Bank roster bind, within this fence.
        const auto skin_boot_policy=actual_package->capture_native_owned_skin_boot_policy();
        require_same_active_skin();
        const auto actual_completed=actual_package->drive_native_owned_skin_base_game_boot(skin_boot_policy);
        require_same_active_skin();
        if(!actual_completed||completed_owner)throw std::runtime_error("Owned startup completed owner absent/replayed");
        actual_completed->require_identity_current();completed_owner=actual_completed;
        require_same_active_skin();
    });
    });
    if(!completed_owner)throw std::runtime_error("Owned startup did not retain actual returned resource owner");
    completed_owner->require_identity_current();
    require_same_package();
    completed_owner->require_identity_current();
    // Actual whole Skin and original Bank scope have returned normally.
    // Issue same-Parent typed inputs only after their genuine outer fence ends.
    const auto color=actual_package->capture_native_owned_completed_color_query(completed_owner);
    completed_owner->require_identity_current();require_same_package();
    const auto read=actual_package->capture_native_owned_completed_selected_read(completed_owner);
    completed_owner->require_identity_current();require_same_package();
    const auto result=std::shared_ptr<detail::BnwNativeOwnedStartupCompletion>(
        new detail::BnwNativeOwnedStartupCompletion(actual_package,completed_owner,color,read));
    result->self_=result;result->require_current();require_same_package();result->require_current();
    return result; // Only a canonical resource return, never full startup ready.
}
namespace {
constexpr std::string_view g0_save_sha =
    "b3a54d5c54d2085236e1c156830643f82dfd78853aee5e1b661c32530a6e82ab";
constexpr std::string_view g0_exe_sha =
    "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686";
constexpr std::string_view g0_tga_sha =
    "5f588c60ccf9919545c0684401374ca7dcfe5e04f54ebd490abaec1aebe1cd87";
constexpr std::string_view g0_manifest_sha =
    "c0a1ffd9b1cacaa827f6f52d0178d608ff8fdcec3dc6b57c70b6347c05292c3f";
// Git checkouts of this tracked JSON can use LF rather than root-workspace
// CRLF. Both exact byte streams carry the same captured receipt.
constexpr std::string_view g0_manifest_lf_sha =
    "a866a86fa1d082e90f09d483be24e99dc7250b4df48c711837bc668092f00f39";
constexpr std::string_view g0_warrior_units_sha =
    "9ea1ca5753e4709c7d818e09faab659b0073ead26cec8449662f4199b2c0bcf0";
constexpr std::string_view g0_unit_atlas_xml_sha =
    "cb803a650ce1825081c85fb53652f174105bc2cfea1147374b9281f56f9435ef";
constexpr std::string_view g0_unit_panel_xml_sha =
    "ca357d8755c2362a48f9d34371697bc83150dae2f5f2c73be11d2c211cb428fe";
constexpr std::string_view g0_unit_panel_lua_sha =
    "5b4be7346a6570196e94e638094aa3d14227e8637d7e335a73983d7b35764524";
constexpr std::string_view g0_icon_support_lua_sha =
    "c9815b6fb6ee5d9fbba1a523b56437e8d4fa798ab8633f9cfc7d9783be5434b4";
constexpr std::string_view g0_base_ui_fpk_sha =
    "4d80faba3cd32ae6584444d6bddfda5ba479671a48bafbd7d259a94b2ab17a82";
constexpr std::string_view g0_bnw_ui_fpk_sha =
    "13607c8427bea649663ced5a216593c5e251981ef7e5b5c1f925fd982311048c";

std::optional<std::int64_t> core_scalar(const saves::UnitCoreObservation &core,
                                        std::string_view name) {
    const auto field = std::find_if(core.fields.begin(), core.fields.end(),
        [&](const saves::UnitCoreField &entry) { return entry.name == name; });
    if (field == core.fields.end() || field->values.size() != 1) return std::nullopt;
    return field->values.front();
}

// The original G0 screenshot visibly selects a warrior. Its pinned save has
// exactly one owned unit, so the ID is a source-backed *frame diagnostic*.
// Recheck every frame: an ordinary preview or a changed game state must never
// inherit a stale selection from that original screenshot.
std::optional<std::int64_t> g0_selected_warrior(const runtime::LiveSession &session) {
    const auto &state = session.state();
    if (state.setup.game_turn != 0 || !state.world.all_players ||
        !state.world.map_projection ||
        state.setup.players.find(0) == state.setup.players.end() ||
        !state.setup.players.at(0).alive)
        return std::nullopt;
    const auto army = state.maintenance.players.find(0);
    if (army == state.maintenance.players.end() || army->second.units.size() != 1)
        return std::nullopt;
    const auto maintained = army->second.units.find(16385);
    if (maintained == army->second.units.end() || maintained->second.type != 83)
        return std::nullopt;
    const saves::SavedUnitObservation *warrior = nullptr;
    std::size_t count = 0;
    for (const auto &owner : state.world.all_players->players) {
        if (owner.saved_player_id != 0) continue;
        count += owner.units.units.size();
        if (owner.units.units.size() == 1) warrior = &owner.units.units.front();
    }
    if (count != 1 || !warrior ||
        warrior->core.layout != saves::GameStateLayout::expansion2_version1 ||
        core_scalar(warrior->core, "m_iID") != 16385 ||
        core_scalar(warrior->core, "m_eOwner") != 0 ||
        core_scalar(warrior->core, "m_eUnitType") != 83 ||
        core_scalar(warrior->core, "m_iX") != 32 ||
        core_scalar(warrior->core, "m_iY") != 14 ||
        core_scalar(warrior->core, "m_iDamage") != 0 ||
        core_scalar(warrior->core, "m_iMoves") != 120 ||
        core_scalar(warrior->core, "m_iBaseCombat") != 8 ||
        core_scalar(warrior->core, "m_bEmbarked") != 0 ||
        core_scalar(warrior->core, "m_bDeathDelay") != 0 ||
        core_scalar(warrior->core, "idUnitType") != 3920984880LL)
        return std::nullopt;
    const auto &map = *state.world.map_projection;
    if (map.grid.width <= 32 || map.grid.height <= 14) return std::nullopt;
    const auto plot = map.grid.index(32, 14);
    if (plot >= map.cells.size() || !map.cells[plot].units ||
        std::count_if(map.cells[plot].units->begin(), map.cells[plot].units->end(),
            [](const saves::SavedPlotUnitReference &unit) {
                return unit.owner == 0 && unit.id == 16385;
            }) != 1)
        return std::nullopt;
    return 16385;
}

std::string g0_warrior_name(const std::filesystem::path &game_root) {
    auto base = std::make_shared<assets::LooseResourceIndex>(game_root);
    auto expansion = std::make_shared<assets::LooseResourceIndex>(game_root);
    auto bnw = std::make_shared<assets::LooseResourceIndex>(game_root);
    base->scan({game_root / "Assets/Gameplay/XML/NewText/RU_RU", {}, false});
    expansion->scan({game_root / "Assets/DLC/Expansion/Gameplay/XML/Text/RU_RU", {}, false});
    bnw->scan({game_root / "Assets/DLC/Expansion2/Gameplay/XML/Text/RU_RU", {}, false});
    assets::ResourceMountTable resources(assets::resource_lookup(base), {},
                                        assets::resource_keys(base));
    resources.mount({1, 0, 0, assets::resource_lookup(expansion), true, true,
                     {}, assets::resource_keys(expansion)});
    resources.mount({2, 0, 0, assets::resource_lookup(bnw), true, true,
                     {}, assets::resource_keys(bnw)});
    const auto catalog = TopPanelLocalizationCatalog::read_active_bnw_resources(
        resources, "Language_RU_RU");
    const auto row = catalog.rows.find("TXT_KEY_UNIT_WARRIOR");
    if (row == catalog.rows.end())
        throw std::runtime_error("G0 active RU warrior localization is missing");
    const auto name = row->second.substr(0, row->second.find('|'));
    if (name != "Воины")
        throw std::runtime_error("G0 active RU warrior name differs from original frame");
    return name;
}

std::optional<std::string> g0_receipt_mismatch(
    const BnwLivePreviewRequest &request, std::string_view ui_sha,
    std::string_view scene_sha) {
    if (!request.g0_selected_unit_receipt_root) return std::nullopt;
    BnwLaunchStageTrace stage_trace("g0_receipt_mismatch");
    if (request.active_player_id != 0 || ui_sha != g0_save_sha ||
        scene_sha != g0_save_sha)
        return "G0 selected-unit diagnostic requires the exact original Gandhi save and player zero";
    const auto &root = *request.g0_selected_unit_receipt_root;
    try {
        const auto manifest_sha = runtime::identify_file(root /
            "gandhi-bnw-ru-1274x925-nohover-manifest.json").sha256;
        if (runtime::identify_file(request.game_root / "CivilizationV.exe").sha256 !=
                g0_exe_sha ||
            (manifest_sha != g0_manifest_sha && manifest_sha != g0_manifest_lf_sha) ||
            runtime::identify_file(root /
                "gandhi-bnw-ru-1274x925-nohover.tga").sha256 != g0_tga_sha ||
            runtime::identify_file(request.game_root /
                "Assets/DLC/Expansion2/Gameplay/XML/Units/CIV5Units.xml").sha256 !=
                g0_warrior_units_sha ||
            runtime::identify_file(request.game_root /
                "Assets/Gameplay/XML/GameInfo/CIV5IconTextureAtlases.xml").sha256 !=
                g0_unit_atlas_xml_sha ||
            runtime::identify_file(request.game_root /
                "Assets/DLC/Expansion2/UI/InGame/WorldView/UnitPanel.xml").sha256 !=
                g0_unit_panel_xml_sha ||
            runtime::identify_file(request.game_root /
                "Assets/DLC/Expansion2/UI/InGame/WorldView/UnitPanel.lua").sha256 !=
                g0_unit_panel_lua_sha ||
            runtime::identify_file(request.game_root /
                "Assets/UI/IconSupport.lua").sha256 != g0_icon_support_lua_sha ||
            bnw_launch_stage("g0_base_ui_archive_hash",[&]{return runtime::identify_file(request.game_root /
                "Resource/DX9/UITextures.fpk").sha256;}) != g0_base_ui_fpk_sha ||
            bnw_launch_stage("g0_expansion2_ui_archive_hash",[&]{return runtime::identify_file(request.game_root /
                "Resource/DX9/Expansion2UITextures.fpk").sha256;}) != g0_bnw_ui_fpk_sha)
            return "G0 selected-unit diagnostic EXE, frame receipt or portrait source differs";
    } catch (const std::exception &error) {
        return std::string("G0 selected-unit diagnostic receipt cannot be read: ") +
            error.what();
    }
    return std::nullopt;
}

std::optional<std::string> scene_mismatch(
    const std::filesystem::path &ui_save,
    const std::filesystem::path &scene_save,
    std::string_view ui_sha256,
    std::string_view scene_sha256,
    const graphics::BnwRendererSceneReceipt *receipt) {
    const auto is_civis = [](const std::filesystem::path &path) {
        auto extension = path.extension().string();
        std::transform(extension.begin(), extension.end(), extension.begin(),
            [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return extension == ".civissave";
    };
    const bool ui_civis = is_civis(ui_save);
    const bool scene_civis = is_civis(scene_save);
    const auto overlay_mismatch = [&](const std::filesystem::path &path)
        -> std::optional<std::string> {
        const auto document=saves::read_civis_save(path);
        if (const auto id = unbound_bnw_scene_section(document)) {
            const auto digest=graphics::bnw_treasury_presentation_digest(document);
            if(digest && ui_civis && scene_civis &&
               ui_sha256==scene_sha256 && receipt &&
               receipt->scene_file_sha256==scene_sha256 &&
               receipt->treasury_presentation_sha256==*digest)
                return std::nullopt;
            return "Civis live overlays require a renderer scene restored to "
                   "the same revision; unbound section " + std::to_string(*id);
        }
        return std::nullopt;
    };
    if (ui_civis) {
        if (const auto mismatch = overlay_mismatch(ui_save)) return mismatch;
    }
    if (scene_civis) {
        if (const auto mismatch = overlay_mismatch(scene_save)) return mismatch;
    }
    if (ui_sha256 == scene_sha256 && ui_civis == scene_civis) {
        if (ui_civis) (void)saves::read_loaded_save(ui_save);
        return std::nullopt;
    }
    if (!ui_civis || scene_civis)
        return "live UI save differs from the renderer's saved world source";
    const auto ui = saves::read_loaded_save(ui_save);
    if (ui.container != saves::SaveContainerKind::civis ||
        !ui.source_sha256 || *ui.source_sha256 != scene_sha256)
        return "live UI save differs from the renderer's saved world source";
    const auto scene = saves::read_loaded_save(scene_save);
    if (scene.container != saves::SaveContainerKind::civ5 ||
        !ui.source_prefix || ui.source_prefix != scene.source_prefix ||
        runtime::identify_bytes(ui.bytes).sha256 !=
            runtime::identify_bytes(scene.bytes).sha256 ||
        saves::make_save_metadata_section({std::string(scene_sha256), ui.header}).bytes !=
            saves::make_save_metadata_section({std::string(scene_sha256), scene.header}).bytes)
        return "Civis import payload, setup or header differs from the renderer's original scene";
    return std::nullopt;
}

// This is an operation guard, not a UI/resource authority. It starts before
// the first lease/provider callback. Its retained package prevents key reuse;
// no iterator or borrowed guard survives an unlocked callback. A caught nested
// invocation (including a foreign-thread invocation on this package) poisons
// the outer operation. Different-package recursive launches on the same thread
// are also refused before either boot can start.
struct EarlyLaunchOperationState {
    std::atomic<bool> poisoned{false};
};
std::mutex early_launch_mutex;
std::map<const BnwNativePackageDiscoveryOwner*,
         std::weak_ptr<EarlyLaunchOperationState>> early_launch_active;
thread_local std::weak_ptr<EarlyLaunchOperationState> early_launch_thread;
class EarlyLaunchOperation final {
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> package_;
    std::shared_ptr<EarlyLaunchOperationState> state_;
  public:
    explicit EarlyLaunchOperation(
        std::shared_ptr<const BnwNativePackageDiscoveryOwner> package)
        : package_(std::move(package)) {
        if (!package_) throw std::invalid_argument("BNW early package is missing");
        std::lock_guard lock(early_launch_mutex);
        if (const auto thread = early_launch_thread.lock()) {
            thread->poisoned.store(true);
            throw std::runtime_error("BNW early launch recursive operation refused");
        }
        auto &slot = early_launch_active[package_.get()];
        if (const auto active = slot.lock()) {
            active->poisoned.store(true);
            throw std::runtime_error("BNW early package launch already active");
        }
        state_ = std::make_shared<EarlyLaunchOperationState>();
        slot = state_;
        early_launch_thread = state_;
    }
    EarlyLaunchOperation(const EarlyLaunchOperation&) = delete;
    EarlyLaunchOperation& operator=(const EarlyLaunchOperation&) = delete;
    ~EarlyLaunchOperation() {
        std::lock_guard lock(early_launch_mutex);
        const auto found = early_launch_active.find(package_.get());
        if (found != early_launch_active.end() && found->second.lock() == state_)
            early_launch_active.erase(found);
        early_launch_thread.reset();
    }
    void require_current() const {
        if (state_->poisoned.load())
            throw std::runtime_error("BNW early launch authorization retired by reentry");
    }
};

// Same real synchronous launch exclusion as the EARLY route. This state is
// operation lifetime only, never a resource/native/completed capability.
std::map<const assets::CivisResourceBankOwner*,std::weak_ptr<EarlyLaunchOperationState>> civis_launch_active;
class CivisLaunchOperation final {
    const std::thread::id creator_=std::this_thread::get_id();
    std::shared_ptr<assets::CivisResourceBankOwner> bank_;
    std::shared_ptr<EarlyLaunchOperationState> state_;
 public:
    explicit CivisLaunchOperation(std::shared_ptr<assets::CivisResourceBankOwner> bank):bank_(std::move(bank)) {
        if(!bank_)throw std::invalid_argument("Civis launch bank absent");
        std::lock_guard lock(early_launch_mutex);
        if(const auto active=early_launch_thread.lock()){
            active->poisoned.store(true);throw std::runtime_error("Civis launch recursive operation refused");
        }
        auto& slot=civis_launch_active[bank_.get()];
        if(const auto active=slot.lock()){
            active->poisoned.store(true);throw std::runtime_error("Civis bank launch already active");
        }
        state_=std::make_shared<EarlyLaunchOperationState>();slot=state_;early_launch_thread=state_;
    }
    CivisLaunchOperation(const CivisLaunchOperation&)=delete;
    CivisLaunchOperation&operator=(const CivisLaunchOperation&)=delete;
    ~CivisLaunchOperation(){
        std::lock_guard lock(early_launch_mutex);
        const auto found=civis_launch_active.find(bank_.get());
        if(found!=civis_launch_active.end()&&found->second.lock()==state_)civis_launch_active.erase(found);
        early_launch_thread.reset();
    }
    void require_current()const {
        if(creator_!=std::this_thread::get_id()||state_->poisoned.load())
            throw std::runtime_error("Civis launch operation retired or foreign thread");
    }
};

// Private semantic launch fence only. These values come from the actual live
// host BEFORE callbacks. It never becomes an external renderer/capture receipt,
// does not set projection_reconciled, and asserts no GPU application.
graphics::BnwRendererSceneReceipt live_scene_fence(
    const SelectedBnwPreviewHost &host) {
    const auto &state = host.session().state();
    const auto &world = state.world;
    if (!world.map_projection || !world.pregame || !world.map_plots)
        throw std::runtime_error("BNW early live world cannot be fenced");
    const auto art=saves::observe_saved_map_team_art(*world.pregame,*world.map_plots);
    graphics::BnwRendererSceneReceipt result;
    result.map_guid=world.map_header.guid;
    result.width=world.map_header.grid.width;
    result.height=world.map_header.grid.height;
    result.wrap_x=world.map_header.wrap_x;
    result.wrap_y=world.map_header.wrap_y;
    result.game_turn=state.setup.game_turn;
    result.active_player=art.active_player;
    result.active_team=art.active_team;
    result.active_team_art=art.plots;
    result.cells.reserve(world.map_projection->cells.size());
    for (const auto &cell:world.map_projection->cells)
        result.cells.push_back(graphics::bnw_scene_cell_revision(cell));
    return result;
}
} // namespace

std::optional<BnwLivePreviewRequest> consume_bnw_live_preview_options(
    int &argc, char **argv) {
    if (argc < 1 || !argv) throw std::invalid_argument("BNW preview argument array is missing");
    BnwLivePreviewRequest request;
    bool save = false, root = false, player = false, receipt = false;
    for (int i = 1; i < argc;) {
        const std::string_view option(argv[i]);
        if (option != "--bnw-live-save" && option != "--bnw-game-root" &&
            option != "--bnw-active-player" &&
            option != "--bnw-g0-selected-unit-receipt-root") {
            ++i;
            continue;
        }
        if (i + 1 >= argc || !argv[i + 1])
            throw std::invalid_argument(std::string(option) + " requires a value");
        const std::string_view value(argv[i + 1]);
        if (value.empty() || value.starts_with("--"))
            throw std::invalid_argument(std::string(option) + " requires a value");
        if (option == "--bnw-live-save") {
            if (save) throw std::invalid_argument("--bnw-live-save repeated");
            request.save_path = argv[i + 1];
            save = true;
        } else if (option == "--bnw-game-root") {
            if (root) throw std::invalid_argument("--bnw-game-root repeated");
            request.game_root = argv[i + 1];
            root = true;
        } else if (option == "--bnw-g0-selected-unit-receipt-root") {
            if (receipt)
                throw std::invalid_argument("--bnw-g0-selected-unit-receipt-root repeated");
            request.g0_selected_unit_receipt_root = argv[i + 1];
            receipt = true;
        } else {
            if (player) throw std::invalid_argument("--bnw-active-player repeated");
            const auto *first = value.data();
            const auto *last = first + value.size();
            std::int64_t id = -1;
            const auto parsed = std::from_chars(first, last, id);
            if (parsed.ec != std::errc{} || parsed.ptr != last || id < 0)
                throw std::invalid_argument("--bnw-active-player must be a nonnegative integer");
            request.active_player_id = id;
            player = true;
        }
        for (int j = i; j + 2 < argc; ++j) argv[j] = argv[j + 2];
        argc -= 2;
    }
    if (!save && !root && !player && !receipt) return std::nullopt;
    if (!save || !root || !player)
        throw std::invalid_argument("BNW live preview requires save, game root, and active player");
    return request;
}

// Private real producer engine; its friend access installs only the existing
// authenticated G0 input decorations. No public input mutation path exists.
class BnwLivePreviewLaunchEngine final {
 public:
 static BnwLivePreviewOutcome run(BnwLivePreviewRequest,std::filesystem::path,
     std::function<void(const LiveBnwPreviewInput*)>,
     std::optional<graphics::BnwRendererSceneReceipt>,BnwPrepareRendererScene,bool,
     std::shared_ptr<const BnwNativePackageDiscoveryOwner>,const EarlyLaunchOperation*,
     BnwOwnedHostCameraRender={},BnwOwnedUiSourceCameraRender={},
     std::shared_ptr<const detail::BnwNativeOwnedStartupCompletion>* = nullptr,
     std::shared_ptr<const SelectedBnwPreviewHost>* = nullptr,BnwObservedUiSourceCameraRender={},
     std::shared_ptr<assets::CivisResourceBankOwner> = {},const CivisLaunchOperation* = nullptr,
     std::shared_ptr<const detail::CivisSelectedBnwRulesPreparation> = {});
};
// All public inputs (including callbacks/receipt) are pinned BEFORE the first
// external provider callback. Callback replacement of caller variables cannot
// destroy its executing function or substitute this launch's original inputs.
BnwLivePreviewOutcome BnwLivePreviewLaunchEngine::run(
    BnwLivePreviewRequest request,
    std::filesystem::path scene_save_path,
    std::function<void(const LiveBnwPreviewInput *)> render,
    std::optional<graphics::BnwRendererSceneReceipt> pinned_receipt,
    BnwPrepareRendererScene prepare_scene, bool immutable_preview_fingerprints,
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> package,
    const EarlyLaunchOperation *operation,BnwOwnedHostCameraRender owned_render,BnwOwnedUiSourceCameraRender owned_source_render,
    std::shared_ptr<const detail::BnwNativeOwnedStartupCompletion>* completed_return,
    std::shared_ptr<const SelectedBnwPreviewHost>* completed_host_return,BnwObservedUiSourceCameraRender observed_source_render,
    std::shared_ptr<assets::CivisResourceBankOwner> bank,const CivisLaunchOperation* bank_operation,
    std::shared_ptr<const detail::CivisSelectedBnwRulesPreparation> prepared_rules) {
    if(prepared_rules&&!bank)throw std::invalid_argument("Prepared Civis rules require their actual Bank lane");
    if(bank && (package || operation || !bank_operation))throw std::invalid_argument("Civis and native launch inputs cannot be mixed");
    const auto *scene_receipt=pinned_receipt ? &*pinned_receipt : nullptr;
    if (!render&&!owned_render&&!owned_source_render&&!observed_source_render) throw std::invalid_argument("BNW live camera renderer callback is missing");
    BnwLivePreviewOutcome outcome;
    if (request.active_player_id < 0)
        return {false, SelectedBnwPreviewBlocker{
            SelectedBnwPreviewStage::active_player, "local active player is not selected"}, {}};
    if (scene_save_path.empty())
        return {false, SelectedBnwPreviewBlocker{
            SelectedBnwPreviewStage::selected_save,
            "BNW live preview requires the renderer's saved FOW scene source"}, {}};
    std::string scene_sha256, ui_sha256;
    try {
        scene_sha256 = runtime::identify_file(scene_save_path).sha256;
        ui_sha256 = runtime::identify_file(request.save_path).sha256;
        if (const auto mismatch = scene_mismatch(request.save_path, scene_save_path,
                                                 ui_sha256, scene_sha256,
                                                 scene_receipt))
            return {false, SelectedBnwPreviewBlocker{
                SelectedBnwPreviewStage::selected_save,
                *mismatch}, {}};
        if (const auto mismatch = g0_receipt_mismatch(request, ui_sha256, scene_sha256))
            return {false, SelectedBnwPreviewBlocker{
                SelectedBnwPreviewStage::selected_save, *mismatch}, {}};
    } catch (const std::exception &e) {
        return {false, SelectedBnwPreviewBlocker{
            SelectedBnwPreviewStage::selected_save,
            std::string("cannot verify live UI and scene saves: ") + e.what()}, {}};
    }
    const auto check_sources = [&] {
        if (runtime::identify_file(scene_save_path).sha256 != scene_sha256 ||
            runtime::identify_file(request.save_path).sha256 != ui_sha256)
            throw std::runtime_error("live UI or renderer scene save changed during launch");
        if (const auto mismatch=g0_receipt_mismatch(request,ui_sha256,scene_sha256))
            throw std::runtime_error(*mismatch);
    };
    std::shared_ptr<const BnwNativeEarlyInputLease> lease;
    const assets::ResourceMountTable *same_table=nullptr;
    std::shared_ptr<const detail::BnwNativeOwnedStartupCompletion> completed_package;
    std::optional<SelectedBnwBootstrapResult> completed_bootstrap;
    const auto check_package = [&] {
        if(bank){bank_operation->require_current();bank->require_boundary();bank_operation->require_current();return;}
        if (!package) return;
        operation->require_current();
        if(completed_package) {
            completed_package->with_actual_completed_inputs([&](const auto& actual_parent,const auto& cap,
                const auto& color,const auto& read) {
                if(actual_parent.get()!=package.get()||actual_parent.owner_before(package)||package.owner_before(actual_parent))
                    throw std::runtime_error("Completed launch changed canonical Parent control");
                cap->require_identity_current();color->require_same_parent(*actual_parent);read->require_same_parent(*actual_parent);
                cap->require_identity_current();
            });operation->require_current();return;
        }
        if (package->early_input_lease().get()!=lease.get() ||
            &package->resources()!=same_table)
            throw std::runtime_error("BNW early launch replaced initial lease/table");
        lease->require_current();
        operation->require_current(); // AFTER all client-provider callbacks
    };
    if(bank){
        if(!scene_receipt || !prepare_scene || scene_receipt->scene_file_sha256!=scene_sha256)
            throw std::runtime_error("Civis launch requires actual renderer receipt and reconciliation");
        check_package();const auto recipe=bank->source_recipe();check_package();
        if(!std::filesystem::equivalent(request.save_path,recipe.save_path)||
           !std::filesystem::equivalent(request.game_root,recipe.game_root))
            throw std::runtime_error("Civis launch request differs from actual bank save/game");
    }
    if (package) {
        // No extension shortcut: the real lease's profile/prefix guards and
        // shared scene_mismatch above decide whether Civis input is supported.
        if (!scene_receipt || !prepare_scene)
            throw std::runtime_error("BNW early launch requires actual renderer receipt and reconciliation");
        if (scene_receipt->scene_file_sha256!=scene_sha256)
            throw std::runtime_error("BNW early launch initial renderer scene file differs");
        lease=package->early_input_lease();
        if (!std::filesystem::equivalent(request.save_path,lease->save_path()) ||
            !std::filesystem::equivalent(request.game_root,lease->game_root()))
            throw std::runtime_error("BNW early launch request differs from actual lease save/game");
        same_table=&package->resources();
        check_package();
        const auto fresh=package->current();
        operation->require_current();
        if (fresh.primary_dx9_registration_complete ||
            fresh.low_resolution_source_branch_complete ||
            fresh.common_high_quality_registration_complete)
            throw std::runtime_error("BNW early launch requires a fresh unregistered package");
        bnw_launch_stage("primary_default",[&]{return detail::BnwNativeDefaultCommonProduction::primary(package);});
        check_package();
        auto settings=bnw_launch_stage("BootSettings_create",[&]{return BnwNativeResourceBootSettingsOwner::create(package);});
        check_package();
        // The actual process/local-settings language is established before
        // Common descriptor loading or the SQL host. No preview default can
        // supply this input; missing choice is a first-start product result.
        try {
            const auto selected=settings->selected_language_input();
            selected->require_current();settings->require_same_resources(*package);
            check_package();
        } catch(const runtime::CivisLanguageSelectionRequired& selection) {
            BnwLivePreviewOutcome required{false,SelectedBnwPreviewBlocker{
                SelectedBnwPreviewStage::selected_save,selection.what()}, {}};
            required.language_selection_required.emplace();
            for(const auto& language:selection.available_languages())
                required.language_selection_required->available_languages.push_back(
                    {language.type,language.display_name});
            check_package();return required;
        }
        bnw_launch_stage("BootSettings_quality_branch",[&]{return package->register_native_dx9_texture_quality_branch(std::move(settings));});
        check_package();
        // Existing phase correctly refuses the still-unimplemented low-source
        // Common scan. No high-quality bool or branch override is supplied.
        bnw_launch_stage("Common13_prefix",[&]{return package->register_native_common_high_quality_prefix();});
        check_package();
        const auto completed_startup=bnw_launch_stage("completed_prefix_and_skin",[&]{return detail::BnwNativeDefaultCommonProduction::advance_owned_initial_category_prefix(package);});
        completed_startup->require_current();completed_package=completed_startup;
        if(completed_return) {
            if(*completed_return)throw std::runtime_error("Actual startup result receiver already consumed");
            *completed_return=completed_startup; // Closed factory owner only, not a caller DTO.
        }
        // Consume the actual selected BASENAME while the completed owners,
        // selected node and native read loan remain alive. This parses XML
        // syntax only; no AudioParams schema, model or audio readiness follows.
        completed_startup->with_actual_completed_inputs(
            [&](const auto& actual_package,const auto& actual_resource,
                const auto& actual_color,const auto& actual_read) {
                if(actual_package.get()!=package.get()||
                   actual_package.owner_before(package)||package.owner_before(actual_package))
                    throw std::runtime_error("Completed XML consumer has another actual canonical Parent");
                (void)actual_color;
                actual_resource->require_identity_current();
                actual_read->require_same_parent(*actual_package);
                actual_read->with_selected_basename_bytes("AudioParams.xml",8ULL*1024*1024,
                    [](std::span<const std::uint8_t> actual_bytes) {
                        const auto actual_xml=content::read_xml_metadata(
                            std::string(actual_bytes.begin(),actual_bytes.end()));
                        (void)actual_xml; // Parser document is destroyed inside the actual read loan.
                    });
                actual_read->require_same_parent(*actual_package);
                actual_resource->require_identity_current();
            });
        check_package();check_sources();completed_startup->require_current();
        completed_startup->with_actual_completed_inputs([&](const auto& actual_parent,const auto& cap,
            const auto& color,const auto& read) {
            completed_bootstrap.emplace(bnw_launch_stage("completedHost_bootstrap",[&]{return bootstrap_selected_bnw_preview_from_completed_resources(
                actual_parent,cap,color,read,request.active_player_id);}));
        });
        check_package();check_sources();completed_startup->require_current();
        if(!completed_bootstrap)throw std::runtime_error("Completed Host borrower returned no bootstrap result");
        if(completed_bootstrap->preview.blocker)
            return {false,std::move(completed_bootstrap->preview.blocker),{}};
        if(!completed_bootstrap->preview.host)throw std::runtime_error("Completed Host bootstrap returned no actual Host");
        package->require_same_selected_host(*completed_bootstrap->preview.host);
        completed_bootstrap->preview.host->require_same_early_package(*package);
        check_package();check_sources();completed_startup->require_current();
        // Real GUI layout is created inside the actual renderer/compositor.
        // Continue through the existing scene/session/render guards so that
        // producer can run; no screen dimensions or UI readiness are supplied here.
    }
    auto bootstrap = bank ? bnw_launch_stage("civisHost_bootstrap",[&]{
        return prepared_rules ? prepared_rules->restore_host_owned(bank,request.active_player_id) :
            bootstrap_selected_bnw_preview_from_civis_resources(bank,request.active_player_id);}) :
        completed_bootstrap ? std::move(*completed_bootstrap) : package ?
        bnw_launch_stage("earlyHost_bootstrap",[&]{return bootstrap_selected_bnw_preview_from_early_package(package,request.active_player_id);}) :
        bnw_launch_stage("normalHost_bootstrap",[&]{return bootstrap_selected_bnw_preview(request.save_path,request.game_root,
            request.active_player_id,immutable_preview_fingerprints);});
    if (bootstrap.preview.blocker)
        return {false, std::move(bootstrap.preview.blocker), {}};
    try {
        if (runtime::identify_file(scene_save_path).sha256 != scene_sha256 ||
            runtime::identify_file(request.save_path).sha256 != ui_sha256)
            return {false, SelectedBnwPreviewBlocker{
                SelectedBnwPreviewStage::selected_save,
                "live UI or renderer scene save changed during BNW import"}, {}};
        if (const auto mismatch = g0_receipt_mismatch(request, ui_sha256, scene_sha256))
            return {false, SelectedBnwPreviewBlocker{
                SelectedBnwPreviewStage::selected_save, *mismatch}, {}};
    } catch (const std::exception &e) {
        return {false, SelectedBnwPreviewBlocker{
            SelectedBnwPreviewStage::selected_save,
            std::string("cannot recheck live UI and scene saves: ") + e.what()}, {}};
    }
    if (!bootstrap.preview.host)
        throw std::runtime_error("BNW bootstrap returned no selected host");
    auto *same_host=bootstrap.preview.host.get();
    const auto generation=same_host->session().state_generation();
    const auto state_lease=same_host->session().read_state_lease();
    const auto *same_source=&same_host->session().source();
    const auto identity=same_host->identity();
    const auto baseline=(package||bank) ? std::optional{live_scene_fence(*same_host)} : std::nullopt;
    const auto check_host = [&] {
        if (!package&&!bank) return;
        check_package();
        if(bank)same_host->require_same_civis_resources(*bank);
        else{package->require_same_selected_host(*same_host);same_host->require_same_early_package(*package);}
        check_sources();
        if(bank)bank_operation->require_current();else operation->require_current();
        const auto &session=same_host->session();
        if (session.state_generation()!=generation ||
            session.read_state_lease().get()!=state_lease.get() ||
            &session.source()!=same_source ||
            same_host->active_player_id()!=request.active_player_id ||
            same_host->identity().payload_sha256!=identity.payload_sha256 ||
            same_host->identity().rules_sha256!=identity.rules_sha256 ||
            runtime::identify_bytes(session.source().source.bytes).sha256!=identity.payload_sha256)
            throw std::runtime_error("BNW early launch selected session/identity replaced");
        if (const auto mismatch=graphics::bnw_renderer_scene_revision_mismatch(
                *baseline,session.state().world,session.state().setup.game_turn,
                request.active_player_id))
            throw std::runtime_error("BNW early launch live world changed: "+*mismatch);
    };
    check_host();
    if (request.observe_selected_profile) {
        try { request.observe_selected_profile(bootstrap); }
        catch (const std::exception &error) {
            return {false, SelectedBnwPreviewBlocker{
                SelectedBnwPreviewStage::selected_save,
                std::string("selected profile observation failed: ") + error.what()}, {}};
        }
    }
    check_host();
    SelectedBnwReadOnlyPreview preview(std::move(bootstrap.preview));
    if(completed_package&&completed_host_return) {
        if(*completed_host_return)throw std::runtime_error("Completed Host output receiver already consumed");
        preview.with_owned_host_camera_input([&](std::shared_ptr<const SelectedBnwPreviewHost> actual_host,
            const LiveBnwPreviewInput*) {
            if(!actual_host||actual_host.get()!=same_host)
                throw std::runtime_error("Completed Preview replaced actual adopted Host");
            check_host();*completed_host_return=std::move(actual_host);check_host();
        });
    }
    outcome.readiness = preview.session().readiness();
    std::optional<graphics::BnwRendererSceneReceipt> reconciled_scene;
    if (scene_receipt) {
        auto restored_scene=*scene_receipt;
        std::optional<std::string> mismatch;
        if(!restored_scene.treasury_presentation_sha256.empty()) {
            const auto &loaded=preview.session().source().source;
            if(loaded.container!=saves::SaveContainerKind::civis ||
               !loaded.live_treasury || !loaded.live_bnw_treasury_balance)
                mismatch="live owner did not restore the paired treasury sections";
            else {
                saves::CivisSaveDocument pair;
                pair.sections={{10,0,*loaded.live_treasury},
                               {30,0,*loaded.live_bnw_treasury_balance}};
                const auto digest=graphics::bnw_treasury_presentation_digest(pair);
                if(!digest || *digest!=restored_scene.treasury_presentation_sha256)
                    mismatch="live treasury presentation pair differs from renderer receipt";
                else restored_scene.treasury_presentation_verified=true;
            }
        }
        if (!prepare_scene)
            mismatch="renderer scene reconciliation callback is missing";
        else if(!mismatch) {
            try {
                bool prepared=false;
                preview.with_camera_input([&](const LiveBnwPreviewInput *input) {
                    check_host();
                    prepared=prepare_scene(input,restored_scene);
                    check_host();
                });
                if (!prepared || !restored_scene.projection_reconciled)
                    mismatch="renderer did not apply verified live plot load hooks";
            } catch (const std::exception &error) {
                mismatch=std::string("renderer scene reconciliation failed: ")+
                    error.what();
            }
        }
        if (!mismatch && restored_scene.scene_file_sha256!=scene_sha256)
            mismatch="renderer scene file changed";
        else if (!mismatch && restored_scene.restored_payload_sha256!=runtime::identify_bytes(
                     preview.session().source().source.bytes).sha256)
            mismatch="restored payload differs";
        else if (!mismatch)
            mismatch=graphics::bnw_renderer_scene_revision_mismatch(
                restored_scene,preview.session().state().world,
                preview.session().state().setup.game_turn,
                request.active_player_id);
        if (mismatch)
            return {false, SelectedBnwPreviewBlocker{
                SelectedBnwPreviewStage::selected_save,
                "renderer scene and live UI were not restored to the same "
                "restored world revision: " + *mismatch}, outcome.readiness};
        reconciled_scene=std::move(restored_scene);
    }
    const auto check_render_scene = [&] {
        check_host();
        if (!package&&!bank) return;
        if (!reconciled_scene || !reconciled_scene->projection_reconciled ||
            reconciled_scene->scene_file_sha256!=scene_sha256 ||
            reconciled_scene->restored_payload_sha256!=identity.payload_sha256)
            throw std::runtime_error("BNW early renderer receipt is unavailable/stale");
        if (const auto mismatch=graphics::bnw_renderer_scene_revision_mismatch(
                *reconciled_scene,preview.session().state().world,
                preview.session().state().setup.game_turn,request.active_player_id))
            throw std::runtime_error("BNW early renderer/live revision differs: "+*mismatch);
    };
    check_render_scene();
    // The actual stack Preview remains inside both authentic source loans.
    // An application target is moved into this scope and destroyed here on
    // normal AND exceptional callback return, before the Preview can retire.
    const auto invoke_observed = [&](const std::shared_ptr<BnwOwnedCameraUiSourceInput>& source) {
        std::optional<BnwObservedUiSourceCameraRender> target;
        try {
            check_render_scene();source->require_current();
            target.emplace(std::move(observed_source_render));
            check_render_scene();source->require_current();
            if(!*target)throw std::invalid_argument("Observed stock source renderer target absent");
            bnw_launch_stage("observed_owned_render",[&]{return (*target)(source,preview);});
            source->require_current();check_render_scene();
            target.reset();
            source->require_current();check_render_scene();
        }catch(...){target.reset();throw;}
    };
    if (request.g0_selected_unit_receipt_root) {
        // LiveSession keeps the decoded payload here; the two original save
        // files were checked by SHA above, before and after bootstrap.
        if (!g0_selected_warrior(preview.session()))
            return {false, SelectedBnwPreviewBlocker{
                SelectedBnwPreviewStage::selected_save,
                "G0 selected warrior is absent from the restored owner or plot"},
                outcome.readiness};
        std::string warrior_name;
        try {
            warrior_name = g0_warrior_name(request.game_root);
        } catch (const std::exception &error) {
            return {false, SelectedBnwPreviewBlocker{
                SelectedBnwPreviewStage::selected_save,
                std::string("G0 selected warrior localization unavailable: ") +
                    error.what()}, outcome.readiness};
        }
        preview.with_owned_host_camera_input([&](std::shared_ptr<const SelectedBnwPreviewHost> parent,const LiveBnwPreviewInput *input) {
            auto diagnostic = *input;
            auto *session = &input->session;
            diagnostic.frame_sources.selected_unit = [session] {
                return g0_selected_warrior(*session);
            };
            // This exact crop is reconstructed from the pinned BC008 atlas
            // and matches every visible opaque portrait pixel in the G0 TGA.
            // It is a receipt-specific observation, not a general Lua API feed.
            diagnostic.frame_sources.selected_unit_portrait = [session](
                const runtime::LiveSession &current, std::uint64_t frame_number)
                -> std::optional<LiveWorldViewUnitPortraitFeed> {
                if (&current != session || g0_selected_warrior(current) != 16385)
                    return std::nullopt;
                return LiveWorldViewUnitPortraitFeed{0, 16385, 0, frame_number,
                    {"indexed:UnitPortraitsAncientMedieval.dds", {768, 0, 256, 256}}};
            };
            // The exact G0 warrior is a non-embarked combat unit. Active
            // UnitPanel.lua shows XP, movement and strength frames for it;
            // the pinned screenshot confirms those frames and no ranged row.
            // This publishes only frame visibility, never stat values.
            diagnostic.frame_sources.selected_unit_stat_boxes = [session](
                const runtime::LiveSession &current, std::uint64_t frame_number)
                -> std::optional<LiveWorldViewUnitStatBoxesFeed> {
                if (&current != session || g0_selected_warrior(current) != 16385)
                    return std::nullopt;
                return LiveWorldViewUnitStatBoxesFeed{
                    0, 16385, 0, frame_number, {true, true, true, false}};
            };
            diagnostic.frame_sources.localize_info = [warrior_name](std::string_view key) {
                return key == "TXT_KEY_UNIT_WARRIOR" ? warrior_name : std::string(key);
            };
            // BNW Lua derives legal actions from Game.CanHandleAction; the
            // saved frame supplies no such live action owner or backend.
            diagnostic.frame_sources.selected_unit_actions = {};
            check_render_scene();
            if(owned_source_render||observed_source_render){
                // Minted by the real preview source producer, then changed only
                // inside this exact already authenticated G0 engine branch.
                preview.with_owned_camera_ui_source_input([&](std::shared_ptr<BnwOwnedCameraUiSourceInput> source){
                    check_render_scene();
                    source->install_checked_launch_input(std::move(diagnostic));
                    source->require_current();
                    if(observed_source_render)invoke_observed(source);else bnw_launch_stage("owned_ui_source_render",[&]{return owned_source_render(std::move(source));});
                    check_render_scene();
                });
            }else if(owned_render)bnw_launch_stage("G0_owned_host_render",[&]{return owned_render(std::move(parent),&diagnostic);});else render(&diagnostic);
            check_render_scene();
        });
    } else {
        preview.with_owned_host_camera_input([&](std::shared_ptr<const SelectedBnwPreviewHost> parent,const LiveBnwPreviewInput *input) {
            check_render_scene();
            if(owned_source_render||observed_source_render){
                preview.with_owned_camera_ui_source_input([&](std::shared_ptr<BnwOwnedCameraUiSourceInput> source){
                    check_render_scene();
                    if(observed_source_render)invoke_observed(source);else bnw_launch_stage("owned_ui_source_render",[&]{return owned_source_render(std::move(source));});
                    check_render_scene();
                });
            }else if(owned_render)bnw_launch_stage("owned_host_render",[&]{return owned_render(std::move(parent),input);});else render(input);
            check_render_scene();
        });
    }
    // Critical publication boundary: real WM_CLOSE/retirement during render
    // invalidates the initial lease even if that callback otherwise returned.
    check_render_scene();
    outcome.rendered = true;
    return outcome;
}

BnwLivePreviewOutcome launch_selected_bnw_camera_preview(
    const BnwLivePreviewRequest &request, const std::filesystem::path &scene_save_path,
    const std::function<void(const LiveBnwPreviewInput *)> &render,
    const graphics::BnwRendererSceneReceipt *scene_receipt,
    const BnwPrepareRendererScene &prepare_scene) {
    return launch_selected_bnw_camera_preview(request,scene_save_path,render,
        scene_receipt,prepare_scene,false);
}
BnwLivePreviewOutcome launch_selected_bnw_camera_preview(
    const BnwLivePreviewRequest &request, const std::filesystem::path &scene_save_path,
    const std::function<void(const LiveBnwPreviewInput *)> &render,
    const graphics::BnwRendererSceneReceipt *scene_receipt,
    const BnwPrepareRendererScene &prepare_scene,bool immutable_preview_fingerprints) {
    if (const auto active=early_launch_thread.lock()) {
        active->poisoned.store(true);
        throw std::runtime_error("BNW legacy camera launch cannot recurse inside initial EARLY launch");
    }
    BnwLivePreviewRequest pinned_request{request.save_path,request.game_root,
        request.active_player_id,request.g0_selected_unit_receipt_root,{}};
    const auto pinned_scene=scene_save_path;
    const auto pinned_receipt=scene_receipt ? std::optional{*scene_receipt} : std::nullopt;
    pinned_request.observe_selected_profile=request.observe_selected_profile;
    auto pinned_render=render;
    auto pinned_prepare=prepare_scene;
    return BnwLivePreviewLaunchEngine::run(std::move(pinned_request),pinned_scene,
        std::move(pinned_render),pinned_receipt,std::move(pinned_prepare),
        immutable_preview_fingerprints,{},nullptr);
}
BnwLivePreviewOutcome launch_selected_bnw_camera_preview_from_early_package(
    const BnwLivePreviewRequest &request, const std::filesystem::path &scene_save_path,
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> package,
    const std::function<void(const LiveBnwPreviewInput *)> &render,
    const graphics::BnwRendererSceneReceipt *scene_receipt,
    const BnwPrepareRendererScene &prepare_scene) {
    if (!render) throw std::invalid_argument("BNW live camera renderer callback is missing");
    try {
        // Guard construction invokes no external callback. Enter before any
        // copied function target can reenter, then pin every borrowed argument
        // before the first actual lease/source/provider callback.
        EarlyLaunchOperation operation(package);
        // Request's observer is deliberately excluded from this value copy.
        BnwLivePreviewRequest pinned_request{request.save_path,request.game_root,
            request.active_player_id,request.g0_selected_unit_receipt_root,{}};
        const auto pinned_scene=scene_save_path;
        const auto pinned_receipt=scene_receipt ? std::optional{*scene_receipt} : std::nullopt;
        // A callable's legal copy constructor can destroy caller receipt/path
        // storage or catch a nested launch. All value inputs are already owned;
        // check the operation immediately after EACH potentially active copy.
        pinned_request.observe_selected_profile=request.observe_selected_profile;
        operation.require_current();
        auto pinned_render=render;
        operation.require_current();
        auto pinned_prepare=prepare_scene;
        operation.require_current();
        return BnwLivePreviewLaunchEngine::run(std::move(pinned_request),pinned_scene,
            std::move(pinned_render),pinned_receipt,std::move(pinned_prepare),
            true,std::move(package),&operation);
    } catch (const std::exception &error) {
        return {false,SelectedBnwPreviewBlocker{SelectedBnwPreviewStage::selected_save,
            std::string("BNW initial EARLY launch refused: ")+error.what()}, {}};
    } catch (...) {
        return {false,SelectedBnwPreviewBlocker{SelectedBnwPreviewStage::selected_save,
            "BNW initial EARLY launch callback failed with unknown exception"}, {}};
    }
}
BnwLivePreviewOutcome launch_selected_bnw_camera_preview_from_early_package_with_owned_host(
    const BnwLivePreviewRequest &request, const std::filesystem::path &scene_save_path,
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> package,
    const BnwOwnedHostCameraRender &render,
    const graphics::BnwRendererSceneReceipt *scene_receipt,
    const BnwPrepareRendererScene &prepare_scene) {
    if (!render) throw std::invalid_argument("BNW live camera renderer callback is missing");
    try {
        // Guard construction invokes no external callback. Enter before any
        // copied function target can reenter, then pin every borrowed argument
        // before the first actual lease/source/provider callback.
        EarlyLaunchOperation operation(package);
        // Request's observer is deliberately excluded from this value copy.
        BnwLivePreviewRequest pinned_request{request.save_path,request.game_root,
            request.active_player_id,request.g0_selected_unit_receipt_root,{}};
        const auto pinned_scene=scene_save_path;
        const auto pinned_receipt=scene_receipt ? std::optional{*scene_receipt} : std::nullopt;
        // A callable's legal copy constructor can destroy caller receipt/path
        // storage or catch a nested launch. All value inputs are already owned;
        // check the operation immediately after EACH potentially active copy.
        pinned_request.observe_selected_profile=request.observe_selected_profile;
        operation.require_current();
        auto pinned_render=render;
        operation.require_current();
        auto pinned_prepare=prepare_scene;
        operation.require_current();
        return BnwLivePreviewLaunchEngine::run(std::move(pinned_request),pinned_scene,
            {},pinned_receipt,std::move(pinned_prepare),
            true,std::move(package),&operation,std::move(pinned_render));
    } catch (const std::exception &error) {
        return {false,SelectedBnwPreviewBlocker{SelectedBnwPreviewStage::selected_save,
            std::string("BNW initial EARLY launch refused: ")+error.what()}, {}};
    } catch (...) {
        return {false,SelectedBnwPreviewBlocker{SelectedBnwPreviewStage::selected_save,
            "BNW initial EARLY launch callback failed with unknown exception"}, {}};
    }
}
BnwLivePreviewOutcome launch_selected_bnw_camera_preview_from_early_package_with_owned_ui_source(
    const BnwLivePreviewRequest &request, const std::filesystem::path &scene_save_path,
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> package,
    const BnwOwnedUiSourceCameraRender &render,
    const graphics::BnwRendererSceneReceipt *scene_receipt,
    const BnwPrepareRendererScene &prepare_scene) {
    if (!render) throw std::invalid_argument("BNW live camera renderer callback is missing");
    try {
        // Guard construction invokes no external callback. Enter before any
        // copied function target can reenter, then pin every borrowed argument
        // before the first actual lease/source/provider callback.
        EarlyLaunchOperation operation(package);
        // Request's observer is deliberately excluded from this value copy.
        BnwLivePreviewRequest pinned_request{request.save_path,request.game_root,
            request.active_player_id,request.g0_selected_unit_receipt_root,{}};
        const auto pinned_scene=scene_save_path;
        const auto pinned_receipt=scene_receipt ? std::optional{*scene_receipt} : std::nullopt;
        // A callable's legal copy constructor can destroy caller receipt/path
        // storage or catch a nested launch. All value inputs are already owned;
        // check the operation immediately after EACH potentially active copy.
        pinned_request.observe_selected_profile=request.observe_selected_profile;
        operation.require_current();
        auto pinned_render=render;
        operation.require_current();
        auto pinned_prepare=prepare_scene;
        operation.require_current();
        return BnwLivePreviewLaunchEngine::run(std::move(pinned_request),pinned_scene,
            {},pinned_receipt,std::move(pinned_prepare),
            true,std::move(package),&operation,{},std::move(pinned_render));
    } catch (const std::exception &error) {
        return {false,SelectedBnwPreviewBlocker{SelectedBnwPreviewStage::selected_save,
            std::string("BNW initial EARLY launch refused: ")+error.what()}, {}};
    } catch (...) {
        return {false,SelectedBnwPreviewBlocker{SelectedBnwPreviewStage::selected_save,
            "BNW initial EARLY launch callback failed with unknown exception"}, {}};
    }
}
BnwLivePreviewOutcome launch_selected_bnw_camera_preview_from_early_package_with_stock_renderer(
    const BnwLivePreviewRequest& request, const std::filesystem::path& scene_save_path,
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> package,
    std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window,
    const runtime::BnwStockLuaClientExtentProvider& client,
    const BnwOwnedStockHostCameraRender& render,
    const graphics::BnwRendererSceneReceipt* scene_receipt,
    const BnwPrepareRendererScene& prepare_scene) {
    if(!render || !window || !client.current)
        throw std::invalid_argument("BNW stock renderer requires actual Window source, client provider and render callback");
    if(request.g0_selected_unit_receipt_root)
        throw std::invalid_argument("BNW stock production launch cannot consume a diagnostic G0 route");
    try {
        // Requires Category72+mandatory73 callback-free, creator-first factory
        // control identity observer BEFORE any EARLY/provider currentness.
        if(!package)throw std::invalid_argument("BNW stock renderer Package is missing");
        auto canonical_package=package->retain_factory_owner();
        if(!canonical_package || canonical_package.get()!=package.get() ||
           canonical_package.owner_before(package) || package.owner_before(canonical_package))
            throw std::runtime_error("BNW stock renderer incoming Package is not the canonical factory control");
        package=std::move(canonical_package);
        EarlyLaunchOperation operation(package);
        BnwLivePreviewRequest pinned_request{request.save_path,request.game_root,
            request.active_player_id,request.g0_selected_unit_receipt_root,{}};
        const auto pinned_scene=scene_save_path;
        const auto pinned_receipt=scene_receipt ? std::optional{*scene_receipt} : std::nullopt;
        const auto package_pin=package;
        auto window_pin=window;
        // Own every incoming application target before the first real
        // EARLY/Window/provider currentness callback. These operation-only
        // checks invoke no provider; all value inputs are already owned.
        pinned_request.observe_selected_profile=request.observe_selected_profile;
        operation.require_current();
        auto pinned_render=render;
        operation.require_current();
        auto pinned_prepare=prepare_scene;
        operation.require_current();
        auto pinned_client=client;
        operation.require_current();
        // Now acquire genuine inputs and authenticate all copy side effects.
        const auto lease=package_pin->early_input_lease();
        const auto game_root=lease->game_root();
        const auto asset_root=lease->canonical_payload_root();
        const auto catalog=lease->catalog();
        const auto profile=lease->profile();
        operation.require_current();
        std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> canonical_window;
        window_pin->with_actual_window_loan([&](const auto& loan) {
            if(loan.window()!=lease->actual_hwnd())
                throw std::runtime_error("BNW stock renderer Window occurrence differs from actual EARLY input");
            const auto canonical=loan.retain_owner();
            if(!canonical || canonical.get()!=window_pin.get() ||
               canonical.owner_before(window_pin) || window_pin.owner_before(canonical))
                throw std::runtime_error("BNW stock renderer incoming Window owner is not the canonical producer control");
            canonical_window=canonical;
        });
        operation.require_current();
        window_pin=std::move(canonical_window);
        std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner> source_owner;
        std::shared_ptr<const SelectedBnwPreviewHost> selected_parent;
        // Outside Engine/target scopes: the real completed resource owner and
        // canonical Parent survive copied target destruction and final checks.
        std::shared_ptr<const detail::BnwNativeOwnedStartupCompletion> completed_return;
        const auto check_binding=[&] {
            operation.require_current();
            if(completed_return)completed_return->require_current();
            if(completed_return) {
                completed_return->with_actual_completed_inputs([&](const auto& actual_parent,const auto& cap,
                    const auto& color,const auto& read) {
                    if(actual_parent.get()!=package_pin.get()||actual_parent.owner_before(package_pin)||package_pin.owner_before(actual_parent))
                        throw std::runtime_error("BNW stock launch changed actual completed Parent control");
                    cap->require_identity_current();color->require_same_parent(*actual_parent);read->require_same_parent(*actual_parent);
                    cap->require_identity_current();
                });
            }else {
                lease->require_current();
                if(package_pin->early_input_lease().get()!=lease.get())
                    throw std::runtime_error("BNW stock launch changed actual EARLY input lease");
            }
            if(selected_parent)package_pin->require_same_selected_host(*selected_parent);
            window_pin->require_current();
            operation.require_current();
            if(completed_return)completed_return->require_current();
        };
        check_binding();
        BnwLivePreviewOutcome outcome;
        {
            BnwOwnedHostCameraRender bound_render=[&](
                std::shared_ptr<const SelectedBnwPreviewHost> parent,
                const LiveBnwPreviewInput* input) {
                if(!parent || source_owner)
                    throw std::runtime_error("BNW stock renderer must bind exactly one genuine selected host");
                if(selected_parent) {
                    if(selected_parent.get()!=parent.get()||selected_parent.owner_before(parent)||parent.owner_before(selected_parent))
                        throw std::runtime_error("BNW stock renderer replaced retained completed Host control");
                }else selected_parent=std::move(parent);
                check_binding();
                // The host is the actual shared owner supplied by the preview,
                // not the bootstrap unique_ptr observer or a shared alias.
                source_owner=runtime::BnwSelectedStockLuaSourceOwner::create_selected(
                    game_root,catalog,profile,selected_parent->identity());
                check_binding();
                // True start owns one VM and yields before first native screen
                // query. No diagnostic probe or per-frame VM is created.
                source_owner->start_stock_bootstrap(selected_parent,asset_root);
                check_binding();
                source_owner->attach_stock_renderer_window_owner(window_pin,pinned_client);
                check_binding();
                if(completed_return) {
                    completed_return->with_actual_completed_inputs([&](const auto& actual_parent,const auto& cap,
                        const auto& color,const auto& read) {
                        if(actual_parent.get()!=package_pin.get()||actual_parent.owner_before(package_pin)||package_pin.owner_before(actual_parent))
                            throw std::runtime_error("BNW stock Color attachment changed canonical completed Parent");
                        cap->require_identity_current();color->require_same_parent(*actual_parent);read->require_same_parent(*actual_parent);
                        selected_parent->require_same_early_package(*actual_parent);window_pin->require_current();
                        source_owner->attach_stock_completed_color_query(color);
                        window_pin->require_current();selected_parent->require_same_early_package(*actual_parent);
                        color->require_same_parent(*actual_parent);read->require_same_parent(*actual_parent);cap->require_identity_current();
                    });check_binding();
                }
                // The real compositor creates the closed GUI screen inside
                // pinned_render. SAME VM advancement belongs after its actual
                // screen attachment, never before this producer has run.
                check_binding();
                // Explicit owner argument supports the same persistent commands
                // and event callbacks throughout this synchronous render loop.
                bnw_launch_stage("stock_owned_render",[&]{return pinned_render(selected_parent,input,source_owner);});
                check_binding();
                (void)source_owner->stock_bootstrap_progress();
                check_binding();
            };
            outcome=BnwLivePreviewLaunchEngine::run(std::move(pinned_request),pinned_scene,
                {},pinned_receipt,std::move(pinned_prepare),true,package,&operation,
                std::move(bound_render),{},&completed_return,&selected_parent);
        } // copied engine/render targets are destroyed before final bookends
        // Explicitly destroy all remaining application callable copies while
        // the operation and its authentic owner pins are still observable.
        pinned_render={};
        check_binding();
        pinned_client.current={};
        check_binding();
        pinned_prepare={};
        check_binding();
        pinned_request.observe_selected_profile={};
        check_binding();
        if(source_owner)(void)source_owner->stock_bootstrap_progress();
        check_binding();
        // The owner remains alive through Engine's final host/scene checks and
        // the named render-loop return. Progress grants no complete UI ready.
        return outcome;
    } catch(const std::exception& error) {
        return {false,SelectedBnwPreviewBlocker{SelectedBnwPreviewStage::selected_save,
            std::string("BNW stock renderer launch refused: ")+error.what()}, {}};
    } catch(...) {
        return {false,SelectedBnwPreviewBlocker{SelectedBnwPreviewStage::selected_save,
            "BNW stock renderer callback failed with unknown exception"}, {}};
    }
}

BnwLivePreviewOutcome launch_selected_bnw_camera_preview_from_early_package_with_stock_renderer_with_observed_ui_source(
    const BnwLivePreviewRequest& request, const std::filesystem::path& scene_save_path,
    std::shared_ptr<const BnwNativePackageDiscoveryOwner> package,
    std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window,
    const runtime::BnwStockLuaClientExtentProvider& client,
    const BnwObservedStockUiSourceCameraRender& render,
    const graphics::BnwRendererSceneReceipt* scene_receipt,
    const BnwPrepareRendererScene& prepare_scene) {
    if(!render || !window || !client.current)
        throw std::invalid_argument("BNW stock renderer requires actual Window source, client provider and render callback");
    if(request.g0_selected_unit_receipt_root)
        throw std::invalid_argument("BNW stock production launch cannot consume a diagnostic G0 route");
    try {
        // Requires Category72+mandatory73 callback-free, creator-first factory
        // control identity observer BEFORE any EARLY/provider currentness.
        if(!package)throw std::invalid_argument("BNW stock renderer Package is missing");
        auto canonical_package=package->retain_factory_owner();
        if(!canonical_package || canonical_package.get()!=package.get() ||
           canonical_package.owner_before(package) || package.owner_before(canonical_package))
            throw std::runtime_error("BNW stock renderer incoming Package is not the canonical factory control");
        package=std::move(canonical_package);
        EarlyLaunchOperation operation(package);
        BnwLivePreviewRequest pinned_request{request.save_path,request.game_root,
            request.active_player_id,request.g0_selected_unit_receipt_root,{}};
        const auto pinned_scene=scene_save_path;
        const auto pinned_receipt=scene_receipt ? std::optional{*scene_receipt} : std::nullopt;
        const auto package_pin=package;
        auto window_pin=window;
        // Own every incoming application target before the first real
        // EARLY/Window/provider currentness callback. These operation-only
        // checks invoke no provider; all value inputs are already owned.
        pinned_request.observe_selected_profile=request.observe_selected_profile;
        operation.require_current();
        auto pinned_render=render;
        operation.require_current();
        auto pinned_prepare=prepare_scene;
        operation.require_current();
        auto pinned_client=client;
        operation.require_current();
        // Now acquire genuine inputs and authenticate all copy side effects.
        const auto lease=package_pin->early_input_lease();
        const auto game_root=lease->game_root();
        const auto asset_root=lease->canonical_payload_root();
        const auto catalog=lease->catalog();
        const auto profile=lease->profile();
        operation.require_current();
        std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> canonical_window;
        window_pin->with_actual_window_loan([&](const auto& loan) {
            if(loan.window()!=lease->actual_hwnd())
                throw std::runtime_error("BNW stock renderer Window occurrence differs from actual EARLY input");
            const auto canonical=loan.retain_owner();
            if(!canonical || canonical.get()!=window_pin.get() ||
               canonical.owner_before(window_pin) || window_pin.owner_before(canonical))
                throw std::runtime_error("BNW stock renderer incoming Window owner is not the canonical producer control");
            canonical_window=canonical;
        });
        operation.require_current();
        window_pin=std::move(canonical_window);
        std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner> source_owner;
        std::shared_ptr<const SelectedBnwPreviewHost> selected_parent;
        // Outside Engine/target scopes: the real completed resource owner and
        // canonical Parent survive copied target destruction and final checks.
        std::shared_ptr<const detail::BnwNativeOwnedStartupCompletion> completed_return;
        const auto check_binding=[&] {
            operation.require_current();
            if(completed_return)completed_return->require_current();
            if(completed_return) {
                completed_return->with_actual_completed_inputs([&](const auto& actual_parent,const auto& cap,
                    const auto& color,const auto& read) {
                    if(actual_parent.get()!=package_pin.get()||actual_parent.owner_before(package_pin)||package_pin.owner_before(actual_parent))
                        throw std::runtime_error("BNW stock launch changed actual completed Parent control");
                    cap->require_identity_current();color->require_same_parent(*actual_parent);read->require_same_parent(*actual_parent);
                    cap->require_identity_current();
                });
            }else {
                lease->require_current();
                if(package_pin->early_input_lease().get()!=lease.get())
                    throw std::runtime_error("BNW stock launch changed actual EARLY input lease");
            }
            if(selected_parent)package_pin->require_same_selected_host(*selected_parent);
            window_pin->require_current();
            operation.require_current();
            if(completed_return)completed_return->require_current();
        };
        check_binding();
        BnwLivePreviewOutcome outcome;
        {
            // Move the owned application target into Engine's observed target.
            // Its destruction then occurs inside the genuine Preview/UI-source
            // borrower, before those source holders and stack Preview unwind.
            BnwObservedUiSourceCameraRender bound_render=[&,observed_callback=std::move(pinned_render)](
                std::shared_ptr<BnwOwnedCameraUiSourceInput> source,
                const SelectedBnwReadOnlyPreview& actual_preview) {
                source->require_current();
                auto parent=source->parent();
                source->require_current();
                if(!parent || source_owner)
                    throw std::runtime_error("BNW stock renderer must bind exactly one genuine selected host");
                if(selected_parent) {
                    if(selected_parent.get()!=parent.get()||selected_parent.owner_before(parent)||parent.owner_before(selected_parent))
                        throw std::runtime_error("BNW stock renderer replaced retained completed Host control");
                }else selected_parent=std::move(parent);
                check_binding();
                // The host is the actual shared owner supplied by the preview,
                // not the bootstrap unique_ptr observer or a shared alias.
                source_owner=runtime::BnwSelectedStockLuaSourceOwner::create_selected(
                    game_root,catalog,profile,selected_parent->identity());
                check_binding();
                // True start owns one VM and yields before first native screen
                // query. No diagnostic probe or per-frame VM is created.
                source_owner->start_stock_bootstrap(selected_parent,asset_root);
                check_binding();
                source_owner->attach_stock_renderer_window_owner(window_pin,pinned_client);
                check_binding();
                if(completed_return) {
                    completed_return->with_actual_completed_inputs([&](const auto& actual_parent,const auto& cap,
                        const auto& color,const auto& read) {
                        if(actual_parent.get()!=package_pin.get()||actual_parent.owner_before(package_pin)||package_pin.owner_before(actual_parent))
                            throw std::runtime_error("BNW stock Color attachment changed canonical completed Parent");
                        cap->require_identity_current();color->require_same_parent(*actual_parent);read->require_same_parent(*actual_parent);
                        selected_parent->require_same_early_package(*actual_parent);window_pin->require_current();
                        source_owner->attach_stock_completed_color_query(color);
                        window_pin->require_current();selected_parent->require_same_early_package(*actual_parent);
                        color->require_same_parent(*actual_parent);read->require_same_parent(*actual_parent);cap->require_identity_current();
                    });check_binding();
                }
                // The real compositor creates the closed GUI screen inside
                // pinned_render. SAME VM advancement belongs after its actual
                // screen attachment, never before this producer has run.
                check_binding();
                // Explicit owner argument supports the same persistent commands
                // and event callbacks throughout this synchronous render loop.
                bnw_launch_stage("observed_stock_owned_render",[&]{return observed_callback(source,actual_preview,source_owner);});
                source->require_current();
                check_binding();
                (void)source_owner->stock_bootstrap_progress();
                check_binding();
            };
            outcome=BnwLivePreviewLaunchEngine::run(std::move(pinned_request),pinned_scene,
                {},pinned_receipt,std::move(pinned_prepare),true,package,&operation,
                {},{},&completed_return,&selected_parent,std::move(bound_render));
        } // copied engine/render targets are destroyed before final bookends
        // Explicitly destroy all remaining application callable copies while
        // the operation and its authentic owner pins are still observable.
        pinned_render={};
        check_binding();
        pinned_client.current={};
        check_binding();
        pinned_prepare={};
        check_binding();
        pinned_request.observe_selected_profile={};
        check_binding();
        if(source_owner)(void)source_owner->stock_bootstrap_progress();
        check_binding();
        // The owner remains alive through Engine's final host/scene checks and
        // the named render-loop return. Progress grants no complete UI ready.
        return outcome;
    } catch(const std::exception& error) {
        return {false,SelectedBnwPreviewBlocker{SelectedBnwPreviewStage::selected_save,
            std::string("BNW stock renderer launch refused: ")+error.what()}, {}};
    } catch(...) {
        return {false,SelectedBnwPreviewBlocker{SelectedBnwPreviewStage::selected_save,
            "BNW stock renderer callback failed with unknown exception"}, {}};
    }
}


BnwLivePreviewOutcome launch_selected_bnw_camera_preview_from_civis_resources_with_stock_renderer_with_observed_ui_source(
    const BnwLivePreviewRequest& request,const std::filesystem::path& scene_save_path,
    std::shared_ptr<assets::CivisResourceBankOwner> supplied_bank,
    std::shared_ptr<const CivisResourceBankUiSnapshot> supplied_ui,
    std::shared_ptr<const CivisResourceBankFontColorSnapshot> supplied_fonts,
    std::shared_ptr<BnwWorldViewFontSourceScope> supplied_scope,
    std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> supplied_window,
    const runtime::BnwStockLuaClientExtentProvider& client,
    const BnwObservedCivisStockUiSourceCameraRender& render,
    const graphics::BnwRendererSceneReceipt* scene_receipt,const BnwPrepareRendererScene& prepare_scene,
    std::shared_ptr<const detail::CivisSelectedBnwRulesPreparation> supplied_rules) {
    if(!supplied_bank||!supplied_ui||!supplied_fonts||!supplied_scope||!supplied_window||!render||!client.current||!scene_receipt||!prepare_scene)
        throw std::invalid_argument("Civis launch requires real Bank/UI/fonts/Window/client/scene/render sources");
    if(request.g0_selected_unit_receipt_root)throw std::invalid_argument("Civis stock launch cannot consume G0 diagnostic inputs");
    try {
        const auto bank=supplied_bank->retain_owner();
        if(bank.get()!=supplied_bank.get()||bank.owner_before(supplied_bank)||supplied_bank.owner_before(bank))
            throw std::runtime_error("Civis launch Bank canonical control differs");
        CivisLaunchOperation operation(bank);
        auto rules=supplied_rules?supplied_rules->retain_owner():std::shared_ptr<const detail::CivisSelectedBnwRulesPreparation>{};
        if(rules&&(rules.get()!=supplied_rules.get()||rules.owner_before(supplied_rules)||supplied_rules.owner_before(rules)))
            throw std::runtime_error("Civis launch prepared rules control differs");
        operation.require_current();
        // Own every target and value before the first boundary/client callback.
        BnwLivePreviewRequest pinned_request{request.save_path,request.game_root,request.active_player_id,{},request.observe_selected_profile};
        const auto pinned_scene=scene_save_path;
        const std::optional<graphics::BnwRendererSceneReceipt> pinned_receipt{*scene_receipt};
        auto pinned_render=render;auto pinned_prepare=prepare_scene;auto pinned_client=client;
        operation.require_current();
        if(rules){rules->require_bank_owned(*bank);operation.require_current();}
        const auto ui=supplied_ui->retain_owner();const auto fonts=supplied_fonts->retain_owner();
        const auto scope=supplied_scope->retain_owner();
        if(ui.get()!=supplied_ui.get()||ui.owner_before(supplied_ui)||supplied_ui.owner_before(ui)||
           fonts.get()!=supplied_fonts.get()||fonts.owner_before(supplied_fonts)||supplied_fonts.owner_before(fonts)||
           scope.get()!=supplied_scope.get()||scope.owner_before(supplied_scope)||supplied_scope.owner_before(scope))
            throw std::runtime_error("Civis launch UI/font/scope canonical control differs");
        std::shared_ptr<const graphics::BnwOwnedCameraRendererWindowLease> window;
        auto window_pin=supplied_window;
        std::shared_ptr<const SelectedBnwPreviewHost> selected_parent;
        std::shared_ptr<runtime::BnwSelectedStockLuaSourceOwner> source_owner;
        const auto check_binding=[&]{
            operation.require_current();bank->require_boundary();operation.require_current();
            ui->require_same_bank(*bank);fonts->require_same_bank(*bank);fonts->require_same_ui(*ui);
            if(selected_parent)selected_parent->require_same_civis_resources(*bank);
            window_pin->require_current();operation.require_current();
        };
        const auto check_scope=[&]{
            check_binding();scope->require_current();
            if(&scope->resources()!=&bank->resources())throw std::runtime_error("Civis launch font scope belongs to another resource table");
            check_binding();
        };
        check_scope();
        // Explicit cold demand of the existing Camera readers, BEFORE any
        // retained Lua/native operation. Actual bank winners/all hits keep
        // their original provenance; this never mounts a second loose source.
        const auto selected_language=scope->locale().language;check_scope();
        if(selected_language!="en_US"&&selected_language!="ru_RU")
            throw std::runtime_error("Civis camera cold source language unsupported");
        const auto language_directory=selected_language=="ru_RU"?"RU_RU":"EN_US";
        const auto dlc_language=selected_language=="ru_RU"?"RU_RU":"en_US";
        const auto cold_current=[&]{operation.require_current();bank->require_current();};
        const auto stage_optional=[&](const std::string& key,bool all=false){
            cold_current();const assets::AssetKey actual(key);
            if(bank->has_indexed(actual)){
                if(all)bank->load_all(actual);else bank->load(actual);
            }
            cold_current();
        };
        const auto& actual_resources=bank->resources();
        for(const auto& prefix:{std::string("Assets/Gameplay/XML/NewText/")+language_directory,
                std::string("Assets/DLC/Expansion/Gameplay/XML/Text/")+dlc_language,
                std::string("Assets/DLC/Expansion2/Gameplay/XML/Text/")+dlc_language}){
            const auto keys=actual_resources.keys(prefix);cold_current();
            if(keys.empty()||keys.size()>4096)throw std::runtime_error("Civis camera selected text closure absent or excessive");
            for(const auto& key:keys){
                auto extension=std::filesystem::path(key).extension().string();
                std::transform(extension.begin(),extension.end(),extension.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
                if(extension!=".xml"||key.find("/LeaderDialog/")!=std::string::npos||key.find("Scenario")!=std::string::npos)continue;
                // TopPanel's exact language-path reader resolves_all when a
                // basename winner names another physical language document.
                stage_optional(key,true);
            }
        }
        // Existing SystemPedia entry/class readers use these exact sparse
        // base/Expansion/Expansion2 paths, preserving absent optional sources.
        for(const auto* prefix:{"Assets/Gameplay/XML/","Assets/DLC/Expansion/Gameplay/XML/","Assets/DLC/Expansion2/Gameplay/XML/"}){
            for(const auto* suffix:{"Technologies/CIV5Technologies.xml","Units/CIV5Units.xml",
                    "Buildings/CIV5Buildings.xml","GameInfo/CIV5Policies.xml","Terrain/CIV5Improvements.xml",
                    "Religions/Civ5Religions.xml","GameInfo/CIV5Resolutions.xml","Terrain/CIV5Resources.xml",
                    "Terrain/CIV5Terrains.xml","Terrain/CIV5Features.xml","Buildings/CIV5BuildingClasses.xml"})
                stage_optional(std::string(prefix)+suffix);
        }
        for(const auto* key:{"Assets/DLC/Expansion2/Gameplay/XML/GameInfo/CIV5Projects.xml",
                "Assets/DLC/Expansion/Gameplay/XML/Buildings/CIV5Buildings_Expansion.xml",
                "Assets/DLC/Expansion2/Gameplay/XML/Buildings/CIV5Buildings_Expansion2.xml",
                "Assets/DLC/Expansion2/Gameplay/XML/Buildings/CIV5Buildings_Inherited_Expansion2.xml",
                "Assets/DLC/Expansion/Gameplay/XML/Buildings/CIV5BuildingClasses_Expansion.xml",
                "Assets/DLC/Expansion2/Gameplay/XML/Buildings/CIV5BuildingClasses_Expansion2.xml",
                "Assets/DLC/Expansion2/Gameplay/XML/Buildings/CIV5BuildingClasses_Inherited_Expansion2.xml",
                "Assets/Gameplay/XML/GameInfo/CIV5IconFontMapping.xml",
                "Assets/DLC/Expansion2/Gameplay/XML/GameInfo/CIV5IconFontMapping_Expansion2.xml",
                "Grid9Black8.dds","OverSupplyLimit.dds","Divider.dds",
                "MiniMapWater.dds","MiniMapShallowWater.dds","MiniMapGrass.dds","MiniMapFog.dds",
                "MiniMapMountain.dds","MiniMapIce.dds","MiniMapCulture.dds"})stage_optional(key);
        const auto icon_key=assets::AssetKey("Assets/DLC/Expansion2/UI/Fonts/Font Icons/FontIcons.ggxml");
        bank->load(icon_key,assets::CivisResourceParse::xml);cold_current();
        const auto icon_source=bank->capability().loaded(icon_key);cold_current();
        if(!icon_source.xml)throw std::runtime_error("Civis camera actual icon font XML absent");
        const auto icon_font=GlyphGenFont::from_xml(*icon_source.xml);cold_current();
        if(icon_font.textures.empty()||icon_font.textures.size()>64)throw std::runtime_error("Civis camera icon texture declaration invalid");
        for(const auto& texture:icon_font.textures){
            const auto key=texture.source();
            if(key.size()<4||assets::normalize_asset_name(key).substr(key.size()-4)!=".dds")
                throw std::runtime_error("Civis camera icon texture is not authored DDS");
            stage_optional(key);stage_optional(key.substr(0,key.size()-4)+"-index.dds");
        }
        check_scope();
        window_pin->with_actual_window_loan([&](const auto& loan){
            if(loan.window()!=bank->actual_hwnd())throw std::runtime_error("Civis launch Window differs from actual bank input");
            const auto canonical=loan.retain_owner();
            if(!canonical||canonical.get()!=window_pin.get()||canonical.owner_before(window_pin)||window_pin.owner_before(canonical))
                throw std::runtime_error("Civis launch Window canonical control differs");
            window=canonical;
        });
        check_binding();window_pin=std::move(window);check_binding();
        BnwLivePreviewOutcome outcome;
        {
            BnwObservedUiSourceCameraRender bound_render=[&,callback=std::move(pinned_render)](
                std::shared_ptr<BnwOwnedCameraUiSourceInput> source,const SelectedBnwReadOnlyPreview& actual_preview){
                source->require_current();const auto parent=source->parent();source->require_current();
                if(!parent||selected_parent||source_owner)throw std::runtime_error("Civis launch must bind exactly one genuine Preview Host");
                selected_parent=parent;scope->require_same_civis_resource_host(*selected_parent);check_binding();
                source_owner=runtime::BnwSelectedStockLuaSourceOwner::create_selected_from_civis_resources(selected_parent,ui,fonts);
                check_binding();
                if(!request.application)throw std::runtime_error("Civis stock launch requires actual frontend application");
                request.application->enter_loaded_game_owned(selected_parent,source_owner,*bank);check_binding();
                source_owner->attach_stock_renderer_window_owner(window_pin,pinned_client);check_binding();
                check_scope();
                // Camera owns scope retirement on normal/unwind return. Its
                // retired font publication is never renewed by launch bookends.
                bnw_launch_stage("observed_civis_stock_render",[&]{return callback(source,actual_preview,source_owner,scope);});
                source->require_current();check_binding();(void)source_owner->stock_bootstrap_progress();check_binding();
            };
            outcome=BnwLivePreviewLaunchEngine::run(std::move(pinned_request),pinned_scene,{},pinned_receipt,
                std::move(pinned_prepare),true,{},nullptr,{}, {},nullptr,nullptr,std::move(bound_render),bank,&operation,std::move(rules));
        }
        pinned_render={};check_binding();pinned_client.current={};check_binding();
        pinned_prepare={};check_binding();pinned_request.observe_selected_profile={};check_binding();
        if(source_owner){(void)source_owner->stock_bootstrap_progress();}check_binding();
        return outcome;
    }catch(const std::exception& error){
        return {false,SelectedBnwPreviewBlocker{SelectedBnwPreviewStage::selected_save,std::string("Civis stock renderer launch refused: ")+error.what()}, {}};
    }catch(...){
        return {false,SelectedBnwPreviewBlocker{SelectedBnwPreviewStage::selected_save,"Civis stock renderer callback failed with unknown exception"}, {}};
    }
}

} // namespace civ5::ui
