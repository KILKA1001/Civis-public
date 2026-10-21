#include "material_world_validation.hpp"
#include "gandhi_minimap_live_ice.hpp"
#include "bnw_warrior_world_binding.hpp"
#include "bnw_warrior_art_probe.hpp"
#include "saved_land_style_prior_audit.hpp"
#include "material_preview_settings.hpp"
#include "bnw_client_capture_evidence.hpp"
#include "civ5/content/loose_resource_plan.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/terrain/terrain_source_rivers.hpp"
#include "civ5/ui/top_panel_saved_state.hpp"
#include "civ5/ui/saved_session_frame.hpp"
#include "civ5/ui/bnw_live_preview_launch.hpp"
#include "civ5/ui/bnw_owned_camera_ui_source_input.hpp"
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include "civ5/ui/civis_resource_bank_ui_snapshot.hpp"
#include "civ5/ui/civis_resource_bank_font_color_snapshot.hpp"
#include "civ5/ui/bnw_worldview_title_font_issuer.hpp"
#include "civ5/runtime/bnw_selected_language_input_owner.hpp"
#include "civ5/ui/bnw_native_early_input_lease.hpp"
#include "civ5/graphics/bnw_owned_camera_renderer_window_lease.hpp"
#include "civ5/ui/bnw_native_package_discovery_owner.hpp"
#include "civ5/graphics/bnw_scene_restore_receipt.hpp"
#include "civ5/graphics/bnw_scene_capture_baseline.hpp"
#include <array>
#include <charconv>
#include <set>
#include <fstream>
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


// Diagnostic fragments consume declarations from the preceding fragment.
// clang-format off
#include "water_camera_replay.hpp"
#include "water_world_scene.hpp"
#include "scheduled_material_preview.hpp"
#include "gl_bnw_warrior_owned_scene.hpp"
#include "camera_preview.hpp"
#include "material_world_precision.hpp"
#include "material_world_loading_validation.hpp"
#include "material_world_refresh_validation.hpp"
// clang-format on

namespace {

// Actual cold required roots consumed by the SAME selected bank's authored
// UI/font snapshots. Winner XML is loaded/parsed before bank publication;
// downstream context hydration and font layers retain their genuine owners.
// Optional palette/city-banner inputs are still resolved by their consumers.
const std::array<civ5::assets::CivisRequiredResource,8>& civis_bank_required_ui_roots(){
    using civ5::assets::CivisRequiredResource;
    using civ5::assets::CivisResourceParse;
    using civ5::assets::CivisResourceSelection;
    static const std::array<CivisRequiredResource,8> roots{{
        {"Assets/UI/InGame/InGame.xml",CivisResourceParse::xml,CivisResourceSelection::winner},
        {"Assets/UI/InGame/WorldView/WorldView.xml",CivisResourceParse::xml,CivisResourceSelection::winner},
        {"Assets/UI/InGame/WorldView/NotificationPanel.xml",CivisResourceParse::xml,CivisResourceSelection::winner},
        {"Assets/UI/Fonts/Tw Cent MT/TwCenMT14.ggxml",CivisResourceParse::xml,CivisResourceSelection::winner},
        {"Assets/UI/Fonts/Tw Cent MT/TwCenMT16.ggxml",CivisResourceParse::xml,CivisResourceSelection::winner},
        {"Assets/UI/Fonts/Tw Cent MT/TwCenMT18.ggxml",CivisResourceParse::xml,CivisResourceSelection::winner},
        {"Assets/UI/Fonts/Tw Cent MT/TwCenMT20.ggxml",CivisResourceParse::xml,CivisResourceSelection::winner},
        {"Assets/UI/Fonts/Tw Cent MT/TwCenMT24.ggxml",CivisResourceParse::xml,CivisResourceSelection::winner}
    }};
    return roots;
}

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

void require_civis_launch_rendered(const civ5::ui::BnwLivePreviewOutcome& outcome) {
    if(outcome.rendered)return;
    std::string reason=outcome.blocker ? outcome.blocker->reason : "Startup did not render";
    if(outcome.language_selection_required) {
        reason += "\nChoose a language with --ui-language TYPE or use --civis-settings FILE. Available languages:";
        for(const auto& choice:outcome.language_selection_required->available_languages)
            reason += "\n  "+choice.type+"  "+choice.display_name;
        throw std::runtime_error("Civis language selection required: "+reason);
    }
    throw std::runtime_error("Selected BNW live preview blocked: "+reason);
}

void write_immutable_preview_fingerprint_diagnostic(
    const civ5::ui::LiveBnwPreviewInput *input, const fs::path &out, const char *phase) {
    if (!input) return;
    const auto *database = input->frame_sources.database;
    check(database && input->scene_identity,
          "Immutable preview fingerprint diagnostic lacks SAME database/scene identity");
    check(database->immutable_preview_fingerprints_current(),
          "Immutable preview rules are not current on their owning thread");
    const auto digest = database->gameplay_fingerprint_sha256();
    check(digest == input->scene_identity->rules_sha256,
          "Immutable preview canonical rules differ from restored scene identity");
    const auto serializations = database->immutable_preview_fingerprint_serializations();
    check(database->immutable_preview_fingerprints_current(),
          "Immutable preview rules changed during fingerprint diagnostic");
    const auto target = out / (std::string("immutable-preview-fingerprint-") + phase + ".json");
    check(!fs::exists(target), "Preserve previous immutable preview diagnostic");
    blend_validation::text(target,
        "{\"schema\":\"civis-own-preview-immutable-fingerprint-v1\",\"phase\":\"" +
        std::string(phase) + "\",\"canonicalRulesSha256\":\"" + digest +
        "\",\"sceneRulesDigestEqual\":true,\"sameImplCurrent\":true,\"serializations\":" +
        std::to_string(serializations) +
        ",\"fullGraphicsParity\":false}\n");
}
// Validation-only state owner. A selected save can replace the exact serialized
// fields; the remaining derived rates are still controlled preview inputs.
struct PreviewPanelFixture {
    unsigned samples = 0;
    std::optional<civ5::ui::TopPanelSavedProjection> saved_projection;
    std::optional<std::int32_t> saved_game_turn;
    civ5::ui::TopPanelState state(bool smoke, bool russian_ui = false) {
        ++samples;
        civ5::ui::TopPanelState result;
        if (saved_projection) {
            // The saved session is a partial observation, not a running game.
            // It must not inherit the controlled preview rates below.
            result.game_turn = saved_game_turn.value_or(0);
            result.turn_date = "N/A";
            result.player.emplace();
            civ5::ui::apply_top_panel_saved_projection(*result.player, *saved_projection);
            result.available_metrics.emplace();
            return result;
        }
        result.game_turn = saved_game_turn.value_or(smoke ? static_cast<int>(samples) : 1);
        result.turn_date = russian_ui ? "4000 Рі. РґРѕ РЅ.СЌ." : "4000 BC";
        auto &player = result.player.emplace();
        player.city_count = 2;
        player.science_per_turn = 5;
        player.gold = 100;
        player.gold_per_turn = 2;
        player.excess_happiness = 3;
        player.golden_age_progress = 10;
        player.golden_age_threshold = 20;
        player.culture = 30;
        player.next_policy_cost = 100;
        player.culture_per_turn = 4;
        player.strategic_resources = {{"[ICON_RES_IRON]", 2, 0, true, "TXT_KEY_RESOURCE_IRON", 2}};
        player.unit_production_maintenance_mod = 25;
        player.units_supplied = 4;
        player.units_out_of_supply = 1;
        if (smoke) {
            if (samples >= 11 && samples <= 20)
                player.city_count = 0;
            if (samples >= 26 && samples <= 40)
                player.unit_production_maintenance_mod = 0;
            if (samples >= 21 && samples <= 25)
                result.player.reset();
        }
        return result;
    }
};
std::string preview_ui_language(const fs::path &settings_directory) {
    std::ifstream input(settings_directory / "config.ini");
    if (!input)
        return "en_US";
    std::optional<std::string> language;
    for (std::string line; std::getline(input, line);) {
        const auto separator = line.find('=');
        if (separator == std::string::npos)
            continue;
        const auto trim = [](std::string_view text) {
            const auto first = text.find_first_not_of(" \t\r\n");
            if (first == std::string_view::npos)
                return std::string_view{};
            const auto last = text.find_last_not_of(" \t\r\n");
            return text.substr(first, last - first + 1);
        };
        if (trim(std::string_view(line).substr(0, separator)) != "Language")
            continue;
        if (language)
            throw std::runtime_error("Selected UI config.ini duplicates Language");
        language = std::string(trim(std::string_view(line).substr(separator + 1)));
    }
    if (!language || (*language != "en_US" && *language != "ru_RU"))
        throw std::runtime_error("Selected UI language is missing or unsupported");
    return *language;
}
ObservedFinalMaterialFields observed_gandhi_final_r(
    const fs::path &folder, const fs::path &camera_pose, const fs::path &reference_tga,
    const RetailPreviewCamera &camera) {
    constexpr std::string_view height_sha =
        "01406a41cec8962f0c61483c549e48765baa1a47e649bbe03b74f2291a4927d9";
    constexpr std::string_view material_sha =
        "c5236527424a3f6203447f7e9ae87819f56bd95059e792e0f826bb5e6e7fa87b";
    constexpr std::string_view pose_sha =
        "9539b66e6edbf01b4128ecb0060dd49ebf6f1ea9ba3252828f286fa6d5bede70";
    constexpr std::string_view tga_sha =
        "5f588c60ccf9919545c0684401374ca7dcfe5e04f54ebd490abaec1aebe1cd87";
    check(camera.width == 1274 && camera.height == 925,
          "Observed final R requires the pinned G0 client raster");
    const auto pose_bytes = input_bytes(camera_pose);
    std::vector<std::uint8_t> normalized_pose;
    normalized_pose.reserve(pose_bytes.size());
    for (std::size_t i = 0; i < pose_bytes.size(); ++i)
        if (pose_bytes[i] != '\r' || i + 1 == pose_bytes.size() || pose_bytes[i + 1] != '\n')
            normalized_pose.push_back(pose_bytes[i]);
    check(civ5::runtime::identify_bytes(normalized_pose).sha256 == pose_sha &&
              civ5::runtime::identify_file(reference_tga).sha256 == tga_sha,
          "Observed final R requires the exact G0 camera and TGA");
    const auto height_path = folder / "height_u16.bin";
    const auto material_path = folder / "material_u8.bin";
    check(fs::is_regular_file(height_path) && fs::is_regular_file(material_path) &&
              civ5::runtime::identify_file(height_path).sha256 == height_sha &&
              civ5::runtime::identify_file(material_path).sha256 == material_sha,
          "Observed final R fields differ from the captured Gandhi fixture");
    const auto raw_heights = input_bytes(height_path);
    auto materials = input_bytes(material_path);
    constexpr std::size_t count = 2560u * 1792u;
    check(raw_heights.size() == count * 2 && materials.size() == count,
          "Observed final R field dimensions differ");
    std::vector<std::uint16_t> heights(count);
    for (std::size_t i = 0; i < count; ++i)
        heights[i] = std::uint16_t(raw_heights[2 * i]) |
                     (std::uint16_t(raw_heights[2 * i + 1]) << 8);
    return {std::move(heights), std::move(materials)};
}
} // namespace

int main(int argc, char **argv) {
    try {
        std::optional<fs::path> ui_mount_plan_path;
        std::vector<fs::path> ui_archives;
        std::optional<fs::path> ui_skin_directory;
        std::optional<fs::path> saved_fow_save, saved_grid, saved_feature2, native_ice_records;
        std::optional<fs::path> owned_warrior_sdk_cache;
        std::optional<fs::path> selected_gandhi_live_ice_record;
        std::optional<fs::path> retail_camera_pose_path, retail_reference_tga_path;
        std::optional<RetailPreviewCamera> retail_camera;
        auto bnw_live_request = civ5::ui::consume_bnw_live_preview_options(argc, argv);
        bool bnw_civis_resource_bank=false;
        bool bnw_early_initial_smoke=false;
        bool bnw_early_lifetime_gates=false;
        bool bnw_worldview_font_source_gates=false;
        bool bnw_production_component_validation=false;
        std::optional<fs::path> native_minimap_edge_records_cli;
        std::optional<fs::path> native_minimap_bounds_cli;
        bool focus_revealed_ice = false;
        bool audit_land_height_bindings = false;
        bool g0_windowed_selected_marker_visible = false;
        std::optional<std::uint8_t> marsh_prior_byte;
        std::optional<fs::path> material_settings_directory, retail_final_r_fields;
        std::optional<std::pair<civ5::materials::MaterialCacheAdapterInput,
                                civ5::materials::MaterialCacheDx9Capabilities>> observed_adapter;
        // Remove the optional UI-resource arguments before validating the
        // legacy positional diagnostic modes. The caller owns this explicit
        // package order; no native DLC/mod precedence is inferred here.
        for (int i = 1; i < argc;) {
            const std::string_view option(argv[i]);
            if(option=="--bnw-civis-resource-bank"){
                check(bnw_live_request&&!bnw_civis_resource_bank,"Civis bank requires one selected BNW launch");
                bnw_civis_resource_bank=true;
                for(int j=i;j+1<argc;++j)argv[j]=argv[j+1];--argc;continue;
            }
            if(option=="--ui-language"||option=="--civis-settings") {
                check(bnw_live_request.has_value(),"Civis language settings require a selected BNW launch");
                check(i+1<argc,"Civis language settings require a value");
                // Consume only for legacy positional dispatch. The actual
                // Boot owner authenticates these original process arguments;
                // this copied argv value grants no resource/settings authority.
                for(int j=i;j+2<argc;++j)argv[j]=argv[j+2];
                argc-=2;continue;
            }
            if(option=="--bnw-live-production-component-validation") {
                check(bnw_live_request.has_value()&&!bnw_production_component_validation,
                      "Production component validation requires a single genuine selected BNW launch");
#ifndef CIVIS_ALPHA_RESEARCH_RESTART_FIXTURE
                throw std::runtime_error("Production validation actual observed-preview supplier not compiled");
#endif
                bnw_production_component_validation=true;
                for(int j=i;j+1<argc;++j)argv[j]=argv[j+1];--argc;continue;
            }
            if(option=="--bnw-worldview-font-source-gates") {
                check(!bnw_worldview_font_source_gates,"--bnw-worldview-font-source-gates may appear only once");
                bnw_worldview_font_source_gates=true;
                for(int j=i;j+1<argc;++j) argv[j]=argv[j+1];
                --argc;
                continue;
            }
            if(option=="--bnw-early-lifetime-gates") {
                check(!bnw_early_lifetime_gates,"--bnw-early-lifetime-gates may appear only once");
                bnw_early_lifetime_gates=true;
                for(int j=i;j+1<argc;++j) argv[j]=argv[j+1];
                --argc;
                continue;
            }
            if(option=="--bnw-early-initial-smoke") {
                check(!bnw_early_initial_smoke,"--bnw-early-initial-smoke may appear only once");
                bnw_early_initial_smoke=true;
                for(int j=i;j+1<argc;++j) argv[j]=argv[j+1];
                --argc;
                continue;
            }
            if (option == "--focus-revealed-ice") {
                if (focus_revealed_ice)
                    throw std::runtime_error("--focus-revealed-ice may appear only once");
                focus_revealed_ice = true;
                for (int j = i; j + 1 < argc; ++j)
                    argv[j] = argv[j + 1];
                --argc;
                continue;
            }
            if (option == "--g0-windowed-selected-marker-visible") {
                if (g0_windowed_selected_marker_visible)
                    throw std::runtime_error("--g0-windowed-selected-marker-visible may appear only once");
                g0_windowed_selected_marker_visible = true;
                for (int j = i; j + 1 < argc; ++j)
                    argv[j] = argv[j + 1];
                --argc;
                continue;
            }
            if (option == "--audit-land-height-bindings") {
                if (audit_land_height_bindings)
                    throw std::runtime_error("--audit-land-height-bindings may appear only once");
                audit_land_height_bindings = true;
                for (int j = i; j + 1 < argc; ++j)
                    argv[j] = argv[j + 1];
                --argc;
                continue;
            }
            if (option != "--ui-mount-plan" && option != "--ui-archive" &&
                option != "--ui-skin-directory" &&
                option != "--saved-fow-save" && option != "--saved-grid" &&
                option != "--saved-feature2" && option != "--native-ice-records" &&
                option != "--selected-gandhi-live-ice-record" &&
                option != "--native-minimap-edge-records" &&
                option != "--native-minimap-bounds-fixture" &&
                option != "--retail-camera-pose" && option != "--retail-reference-tga" &&
                option != "--marsh-prior-byte" &&
                option != "--owned-warrior-sdk-cache" &&
                option != "--observed-adapter" && option != "--material-settings-directory" &&
                option != "--retail-final-r-fields") {
                ++i;
                continue;
            }
            if (i + 1 >= argc)
                throw std::runtime_error(std::string(option) + " requires a path");
            if (option == "--observed-adapter") {
                if (observed_adapter)
                    throw std::runtime_error("--observed-adapter may appear only once");
                std::array<std::uint32_t, 7> values{};
                std::string_view remaining(argv[i + 1]);
                for (std::size_t field_index = 0; field_index < values.size(); ++field_index) {
                    const auto comma = remaining.find(',');
                    const auto field = remaining.substr(0, comma);
                    if ((field_index + 1 == values.size()) != (comma == std::string_view::npos))
                        throw std::runtime_error("Observed adapter field count differs");
                    const auto parsed = std::from_chars(field.data(), field.data() + field.size(),
                                                        values[field_index]);
                    if (field.empty() || parsed.ec != std::errc{} || parsed.ptr != field.data() + field.size())
                        throw std::runtime_error("Invalid observed adapter field");
                    remaining = comma == std::string_view::npos ? std::string_view{} : remaining.substr(comma + 1);
                }
                if (!remaining.empty() || values[2] > INT32_MAX || values[3] > 1 || values[6] > INT32_MAX)
                    throw std::runtime_error("Invalid observed adapter profile");
                observed_adapter.emplace(
                    civ5::materials::MaterialCacheAdapterInput{
                        values[0], values[1], static_cast<std::int32_t>(values[2]), values[3] == 1},
                    civ5::materials::MaterialCacheDx9Capabilities{
                        values[4], values[5], static_cast<std::int32_t>(values[6])});
            } else if (option == "--marsh-prior-byte") {
                if (marsh_prior_byte)
                    throw std::runtime_error("--marsh-prior-byte may appear only once");
                unsigned value = 0;
                const std::string_view field(argv[i + 1]);
                const auto parsed = std::from_chars(field.data(), field.data() + field.size(), value);
                if (field.empty() || parsed.ec != std::errc{} || parsed.ptr != field.data() + field.size() ||
                    value > 255)
                    throw std::runtime_error("Invalid marsh prior byte");
                marsh_prior_byte = static_cast<std::uint8_t>(value);
            } else if (option == "--saved-feature2") {
                if (saved_feature2)
                    throw std::runtime_error("--saved-feature2 may appear only once");
                saved_feature2 = fs::path(argv[i + 1]);
            } else if (option == "--native-ice-records") {
                if (native_ice_records)
                    throw std::runtime_error("--native-ice-records may appear only once");
                native_ice_records = fs::path(argv[i + 1]);
            } else if (option == "--selected-gandhi-live-ice-record") {
                if (selected_gandhi_live_ice_record)
                    throw std::runtime_error("--selected-gandhi-live-ice-record may appear only once");
                selected_gandhi_live_ice_record = fs::path(argv[i + 1]);
            } else if (option == "--native-minimap-edge-records") {
                if (native_minimap_edge_records_cli)
                    throw std::runtime_error("--native-minimap-edge-records may appear only once");
                native_minimap_edge_records_cli = fs::path(argv[i + 1]);
            } else if (option == "--native-minimap-bounds-fixture") {
                if (native_minimap_bounds_cli)
                    throw std::runtime_error("--native-minimap-bounds-fixture may appear only once");
                native_minimap_bounds_cli = fs::path(argv[i + 1]);
            } else if (option == "--retail-camera-pose") {
                if (retail_camera_pose_path)
                    throw std::runtime_error("--retail-camera-pose may appear only once");
                retail_camera_pose_path = fs::path(argv[i + 1]);
            } else if (option == "--retail-reference-tga") {
                if (retail_reference_tga_path)
                    throw std::runtime_error("--retail-reference-tga may appear only once");
                retail_reference_tga_path = fs::path(argv[i + 1]);
            } else if (option == "--retail-final-r-fields") {
                if (retail_final_r_fields)
                    throw std::runtime_error("--retail-final-r-fields may appear only once");
                retail_final_r_fields = fs::path(argv[i + 1]);
            } else if (option == "--owned-warrior-sdk-cache") {
                if (owned_warrior_sdk_cache)
                    throw std::runtime_error("--owned-warrior-sdk-cache may appear only once");
                owned_warrior_sdk_cache = fs::path(argv[i + 1]);
            } else if (option == "--material-settings-directory") {
                if (material_settings_directory)
                    throw std::runtime_error("--material-settings-directory may appear only once");
                material_settings_directory = fs::path(argv[i + 1]);
            } else if (option == "--ui-mount-plan") {
                if (ui_mount_plan_path)
                    throw std::runtime_error("--ui-mount-plan may appear only once");
                ui_mount_plan_path = fs::path(argv[i + 1]);
            } else if (option == "--ui-archive") {
                ui_archives.emplace_back(argv[i + 1]);
            } else if (option == "--ui-skin-directory") {
                if (ui_skin_directory)
                    throw std::runtime_error("--ui-skin-directory may appear only once");
                ui_skin_directory = fs::path(argv[i + 1]);
            } else {
                auto &destination = option == "--saved-fow-save" ? saved_fow_save : saved_grid;
                if (destination)
                    throw std::runtime_error(std::string(option) + " may appear only once");
                destination = fs::path(argv[i + 1]);
            }
            for (int j = i; j + 2 < argc; ++j)
                argv[j] = argv[j + 2];
            argc -= 2;
        }
        const bool scheduled_materials =
            argc > 1 && std::string_view(argv[argc - 1]) == "--scheduled-materials";
        if (scheduled_materials)
            --argc;
        if (retail_camera_pose_path.has_value() != retail_reference_tga_path.has_value())
            throw std::runtime_error("Retail preview requires both a passive camera pose and its TGA");
        if (retail_camera_pose_path) {
            if (!saved_fow_save || !saved_grid || focus_revealed_ice)
                throw std::runtime_error("Retail camera requires a saved FOW map without diagnostic ice focus");
            retail_camera.emplace(load_retail_preview_camera(*retail_camera_pose_path,
                                                               *retail_reference_tga_path));
        }
        if (native_ice_records && (!saved_fow_save || !saved_grid ||
                                   !fs::is_regular_file(*native_ice_records)))
            throw std::runtime_error("Native ice records require an existing saved FOW input");
        fs::path sequence_reference;
        if (argc >= 12 && std::string_view(argv[argc - 2]) == "--fow-sequence") {
            sequence_reference = argv[argc - 1];
            argc -= 2;
        }
        const bool river_preview = argc >= 12 && std::string_view(argv[argc - 1]) == "--preview-rivers";
        if (river_preview)
            --argc;
        fs::path river_shaders, river_reference;
        if (argc >= 13 && std::string_view(argv[argc - 3]) == "--rivers") {
            river_shaders = argv[argc - 2];
            river_reference = argv[argc - 1];
            argc -= 3;
        }
        check(argc == 10 || argc == 11 ||
                  (argc == 12 && (std::string(argv[10]) == "--water-scene" ||
                                  std::string(argv[10]) == "--water-sky-scene" ||
                                  std::string(argv[10]) == "--water-defaults-scene" ||
                                  std::string(argv[10]) == "--water-animation-scene" ||
                                  std::string(argv[10]) == "--camera-preview" ||
                                  std::string(argv[10]) == "--camera-preview-smoke" ||
                                  std::string(argv[10]) == "--camera-preview-colors" ||
                                  std::string(argv[10]) == "--camera-preview-colors-smoke" ||
                                  std::string(argv[10]) == "--camera-preview-lighting" ||
                                  std::string(argv[10]) == "--camera-preview-lighting-smoke" ||
                                  std::string(argv[10]) == "--camera-preview-fog" ||
                                  std::string(argv[10]) == "--camera-preview-fog-smoke" ||
                                  std::string(argv[10]) == "--camera-preview-fow" ||
                                  std::string(argv[10]) == "--camera-preview-fow-smoke" ||
                                  std::string(argv[10]) == "--material-precision-patches")) ||
                  (argc == 13 && std::string(argv[10]) == "--material-precision-patches") ||
                  (argc == 14 && std::string(argv[10]) == "--water-current-scene") ||
                  (argc == 15 && (std::string(argv[10]) == "--water-reflection-scene" ||
                                  std::string(argv[10]) == "--water-frame-scene" ||
                                  std::string(argv[10]) == "--water-camera-scene" ||
                                  std::string(argv[10]) == "--water-startup-scene" ||
                                  std::string(argv[10]) == "--water-motion-scene" ||
                                  std::string(argv[10]) == "--water-input-scene")) ||
                  (argc == 16 && (std::string(argv[10]) == "--water-camera-replay" ||
                                  std::string(argv[10]) == "--water-color-keys-scene")) ||
                  (argc == 17 && std::string(argv[10]) == "--water-lighting-scene") ||
                  (argc == 18 && std::string(argv[10]) == "--water-fog-scene") ||
                  (argc == 20 && std::string(argv[10]) == "--water-fow-scene") ||
                  ((argc == 21 || argc == 22) && std::string(argv[10]) == "--water-fow-replay") ||
                  (argc == 21 && std::string(argv[10]) == "--water-fow-animation") ||
                  ((argc == 22 || argc == 23) && std::string(argv[10]) == "--water-fow-animation-replay") ||
                  (argc == 21 && std::string(argv[10]) == "--water-fow-camera") ||
                  (argc == 22 && std::string(argv[10]) == "--water-fow-camera-replay"),
              "Usage: gl_material_world asset-root primitive-world-input "
              "blend-shaders normal-shaders shadow-shaders lighting-shaders "
              "land-shaders original-normal-geometry fresh-output "
              "[diagnostic-cache-replay-folder | "
              "--camera-preview/--camera-preview-smoke prior-scene-folder | "
              "--water-world-checks | --water-scene water-shaders | "
              "--water-sky-scene water-shaders | "
              "--water-defaults-scene water-shaders | --water-animation-scene "
              "water-shaders | "
              "--material-precision-patches request-indices [precision-probes] | "
              "--water-current-scene water-shaders current-shaders "
              "original-current-fixture | "
              "--water-reflection-scene/--water-frame-scene/--water-camera-scene/"
              "--water-startup-scene/"
              "--water-motion-scene/--water-input-scene water-shaders "
              "current-shaders "
              "original-current-fixture original-reflection-raster | "
              "--water-camera-replay water-shaders "
              "current-shaders original-current-fixture original-reflection-raster "
              "prior-scene-folder] [--ui-mount-plan PLAN] [--ui-archive RELATIVE_FPK]...");
        const fs::path root(argv[1]), out(argv[9]);
        constexpr std::string_view selected_gandhi_sha =
            "b3a54d5c54d2085236e1c156830643f82dfd78853aee5e1b661c32530a6e82ab";
        constexpr std::string_view selected_native_edges_sha =
            "e6174af670b2589a5548bdeccf5740efd8d2c53a612fa20527a5b3aae26e3b9c";
        // The same tracked JSON is checked out with CRLF on the root Windows
        // workspace and LF in managed worktrees. Both byte streams were
        // verified against the captured one-marker payload.
        constexpr std::string_view selected_native_city_markers_crlf_sha =
            "2fed64bd3747fedbf2ec49abfed2310ccd58b30e556ad167a49697a40cbfabcb";
        constexpr std::string_view selected_native_city_markers_lf_sha =
            "044f1f27780510717bcba8feb64a3c5bf3d0ab3c520a53203cd761fe890e2a73";
        const auto saved_save_sha256 = saved_fow_save
            ? civ5::runtime::identify_file(*saved_fow_save).sha256 : std::string{};
        auto scene_capture_sha256=saved_save_sha256;
        if(bnw_live_request && saved_fow_save &&
           civ5::saves::read_loaded_save(*saved_fow_save).container==
               civ5::saves::SaveContainerKind::civis &&
           civ5::graphics::has_gandhi_bnw_scene_capture_baseline(
               civ5::saves::read_civis_save(*saved_fow_save)))
            scene_capture_sha256=std::string(selected_gandhi_sha);
        if (selected_gandhi_live_ice_record) {
            check(!native_ice_records && saved_grid && saved_fow_save &&
                      fs::is_regular_file(*selected_gandhi_live_ice_record),
                  "Selected Gandhi live ice requires one saved map and no other ice record");
            const auto capture_path = root.parent_path() /
                "tools/civ5_architecture/fixtures/"
                "gandhi-b3a54d5c-kind9-packed-live-20260929.zip";
            check(fs::is_regular_file(capture_path) &&
                      civ5::runtime::identify_file(capture_path).sha256 ==
                          civ5::graphics::gandhi_live_ice_capture_sha256,
                  "Captured Gandhi live ice VB/IB fixture is missing or changed");
            const auto record_sha = civ5::runtime::identify_file(
                *selected_gandhi_live_ice_record).sha256;
            civ5::graphics::validate_gandhi_live_ice_record(
                scene_capture_sha256, record_sha,
                civ5::ui::read_minimap_native_ice_records(*selected_gandhi_live_ice_record));
            native_ice_records = *selected_gandhi_live_ice_record;
        }
        fs::path native_minimap_edge_records_path;
        fs::path native_minimap_city_marker_fixture_path;
        std::optional<civ5::ui::MinimapNativeBoundsFixture> native_minimap_bounds;
        if (scene_capture_sha256 == selected_gandhi_sha) {
            native_minimap_edge_records_path = native_minimap_edge_records_cli.value_or(
                root.parent_path() / "tools/civ5_architecture/fixtures/"
                    "gandhi-b3a54d5c-native-minimap-edge-records.bin");
            check(fs::is_regular_file(native_minimap_edge_records_path) &&
                      civ5::runtime::identify_file(native_minimap_edge_records_path).sha256 ==
                          selected_native_edges_sha,
                  "Captured same-save native minimap edge fixture is missing or changed");
            native_minimap_city_marker_fixture_path =
                root.parent_path() / "tools/civ5_architecture/fixtures/"
                    "gandhi-b3a54d5c-native-city-markers.json";
            const auto city_fixture_sha = civ5::runtime::identify_file(
                native_minimap_city_marker_fixture_path).sha256;
            check(fs::is_regular_file(native_minimap_city_marker_fixture_path) &&
                      (city_fixture_sha == selected_native_city_markers_crlf_sha ||
                       city_fixture_sha == selected_native_city_markers_lf_sha),
                  "Captured same-save native city marker fixture is missing or changed");
            const auto bounds_fixture_path = native_minimap_bounds_cli.value_or(root.parent_path() /
                "tools/civ5_architecture/fixtures/"
                "gandhi-b3a54d5c-native-minimap-bounds.bin");
            check(fs::is_regular_file(bounds_fixture_path) &&
                      civ5::runtime::identify_file(bounds_fixture_path).sha256 ==
                          "f0756c1aaedd1a56d39c475e3856974a056f1962865a4ed56cabd41ba04535d6",
                  "Captured same-save native minimap bounds fixture is missing or changed");
            native_minimap_bounds.emplace(civ5::ui::MinimapNativeBoundsFixture::read(
                bounds_fixture_path, selected_gandhi_sha,
                "7c9f93d512c49a7ab2bc70f51c6caa10a82ddd697ac11d8a24ff1bb7ce7c3686",
                "5f588c60ccf9919545c0684401374ca7dcfe5e04f54ebd490abaec1aebe1cd87"));
        } else if (native_minimap_edge_records_cli || native_minimap_bounds_cli) {
            throw std::runtime_error("Native minimap records require the captured Gandhi save");
        }
        const bool scheduled_preview = scheduled_materials && argc == 11 &&
                                       (std::string_view(argv[10]) == "--camera-preview-fow" ||
                                        std::string_view(argv[10]) == "--camera-preview-fow-smoke");
        check(!owned_warrior_sdk_cache || (scheduled_preview && bnw_live_request &&
                  saved_fow_save && saved_feature2 && marsh_prior_byte == 255 &&
                  !retail_final_r_fields && fs::is_directory(*owned_warrior_sdk_cache)),
              "Owned Warrior requires scheduled live BNW, source feature2/prior255 and generated terrain");
        if (g0_windowed_selected_marker_visible) {
            const auto second_kind5_fixture = root.parent_path() /
                "tools/civ5_architecture/fixtures/"
                "gandhi-b3a54d5c-second-kind5-alpha255-20260929.json";
            check(scheduled_preview && scene_capture_sha256 == selected_gandhi_sha &&
                      retail_camera_pose_path && retail_reference_tga_path && retail_camera &&
                      retail_camera->width == 1914 && retail_camera->height == 1045 &&
                      native_minimap_bounds && !focus_revealed_ice &&
                      civ5::runtime::identify_file(*retail_camera_pose_path).sha256 ==
                          "f89a363ee332b63d106f7bea18f03613b819646ec39d95d3a9c4fa5f9f3a53ad" &&
                      civ5::runtime::identify_file(*retail_reference_tga_path).sha256 ==
                          "b0f36fd73898915cede19acc5bdea43c802c8cd38e4355904511a9b30db99b44" &&
                      fs::is_regular_file(second_kind5_fixture) &&
                      civ5::runtime::identify_file(second_kind5_fixture).sha256 ==
                          "2d9bd46150916e90647933711d1e850d833f51b990b32ea4a5af3605a96e9bca",
                  "Visible G0 selected marker requires the exact windowed capture and alpha-255 source");
        }
        check(!retail_final_r_fields ||
                  (scheduled_preview && scene_capture_sha256 == selected_gandhi_sha &&
                   retail_camera_pose_path && retail_reference_tga_path && retail_camera &&
                   saved_feature2 && river_preview && !focus_revealed_ice),
              "Observed final R requires the selected Gandhi scheduled preview and pinned G0 camera");
        std::optional<ObservedFinalMaterialFields> observed_final_r;
        if (retail_final_r_fields)
            observed_final_r.emplace(observed_gandhi_final_r(
                *retail_final_r_fields, *retail_camera_pose_path,
                *retail_reference_tga_path, *retail_camera));
        check(!observed_adapter || scheduled_preview,
              "Recorded adapter data requires a scheduled FOW preview");
        check(!audit_land_height_bindings || scheduled_preview,
              "LAND height binding audit requires a scheduled FOW preview");
        check(!material_settings_directory || observed_adapter.has_value(),
              "Explicit material settings require a recorded adapter");
        const bool preview_fow =
            (argc == 12 || scheduled_preview) && (std::string(argv[10]) == "--camera-preview-fow" ||
                                                  std::string(argv[10]) == "--camera-preview-fow-smoke");
        const bool preview_fog =
            preview_fow || (argc == 12 && (std::string(argv[10]) == "--camera-preview-fog" ||
                                           std::string(argv[10]) == "--camera-preview-fog-smoke"));
        const bool preview_lighting =
            preview_fog || (argc == 12 && (std::string(argv[10]) == "--camera-preview-lighting" ||
                                           std::string(argv[10]) == "--camera-preview-lighting-smoke"));
        const bool preview_colors =
            preview_lighting || (argc == 12 && (std::string(argv[10]) == "--camera-preview-colors" ||
                                                std::string(argv[10]) == "--camera-preview-colors-smoke"));
        const bool fow_animation_replay =
            (argc == 22 || argc == 23) && std::string(argv[10]) == "--water-fow-animation-replay";
        const bool fow_animated =
            fow_animation_replay || (argc == 21 && std::string(argv[10]) == "--water-fow-animation");
        const bool fow_camera_replay = argc == 22 && std::string(argv[10]) == "--water-fow-camera-replay";
        const bool fow_camera =
            fow_camera_replay || (argc == 21 && std::string(argv[10]) == "--water-fow-camera");
        const bool fow_replay = fow_animation_replay || fow_camera_replay ||
                                ((argc == 21 || argc == 22) && std::string(argv[10]) == "--water-fow-replay");
        const bool fow_scene = fow_animated || fow_camera || fow_replay ||
                               (argc == 20 && std::string(argv[10]) == "--water-fow-scene");
        check(river_shaders.empty() || fow_scene, "Connected rivers require the complete FOW scene");
        check(sequence_reference.empty() ||
                  (fow_scene && !river_shaders.empty() && !fow_animated && !fow_camera),
              "Combined sequence requires a complete river scene and separate reference");
        check(!scheduled_materials || scheduled_preview ||
                  (fow_scene && !fow_replay && !sequence_reference.empty()),
              "Scheduled materials require a fresh FOW preview or complete FOW/river sequence");
        const fs::path fow_animation_reference = fow_animated ? fs::path(argv[20]) : fs::path{};
        const fs::path fow_camera_reference = fow_camera ? fs::path(argv[20]) : fs::path{};
        const bool fog_scene = fow_scene || (argc == 18 && std::string(argv[10]) == "--water-fog-scene");
        const bool lighting_scene =
            fog_scene || (argc == 17 && std::string(argv[10]) == "--water-lighting-scene");
        const bool color_scene =
            lighting_scene || (argc == 16 && std::string(argv[10]) == "--water-color-keys-scene");
        const bool preview =
            preview_colors || (argc == 12 && (std::string(argv[10]) == "--camera-preview" ||
                                              std::string(argv[10]) == "--camera-preview-smoke"));
        check((!ui_mount_plan_path && ui_archives.empty()) || preview,
              "UI resource options require a camera preview mode");
        check(saved_fow_save.has_value() == saved_grid.has_value() && (!saved_fow_save || preview_fow),
              "Saved FOW requires a saved grid and a FOW camera preview");
        check(!bnw_worldview_font_source_gates || bnw_early_initial_smoke,
              "WorldView font source gates require bounded genuine EARLY initial smoke");
        check(!bnw_early_lifetime_gates || bnw_early_initial_smoke,
              "EARLY lifetime gates require explicit bounded initial smoke");
        check(!bnw_early_initial_smoke || (bnw_live_request &&
                  (scheduled_preview || preview) && std::string_view(argv[10]).ends_with("-smoke")),
              "EARLY initial smoke requires a genuine selected BNW camera smoke mode");
        check(!bnw_live_request || saved_fow_save,
              "Selected BNW live preview requires a saved FOW camera preview");
        check(saved_feature2.has_value() == marsh_prior_byte.has_value() &&
                  (!saved_feature2 || (saved_fow_save && scheduled_preview)),
              "Saved feature-2 requires a saved scheduled FOW preview and explicit marsh prior byte");
        check(!river_preview || preview_fow, "River preview requires the complete FOW scene");
        std::optional<civ5::content::LooseResourcePlan> ui_plan;
        if (ui_mount_plan_path)
            ui_plan = civ5::content::read_loose_resource_plan(*ui_mount_plan_path);
        std::optional<civ5::assets::ResourceMountTable> ui_resources_owned;
        if (ui_plan || !ui_archives.empty() || ui_skin_directory) {
            if (ui_plan) {
                // Preserve the plan's default source and explicit loose
                // mounts. Archive overlays below are caller-owned additions.
                ui_resources_owned.emplace(std::move(ui_plan->resources));
            } else {
                const auto loose = std::make_shared<civ5::assets::LooseSource>(root);
                ui_resources_owned.emplace(civ5::assets::resource_lookup(loose),
                                           civ5::assets::resource_lookup_all(loose),
                                           civ5::assets::resource_keys(loose));
            }
            const civ5::assets::AssetEnvironment environment(root);
            std::vector<std::string> mounted_archives;
            std::uint64_t next_ui_mount_id = UINT64_MAX;
            const auto mount_archive = [&](const std::string &relative, bool preferred) {
                const auto normalized = civ5::assets::normalize_asset_name(relative);
                if (std::find(mounted_archives.begin(), mounted_archives.end(), normalized) !=
                    mounted_archives.end())
                    throw std::runtime_error("Repeated UI archive: " + relative);
                // Plan IDs are caller-owned, including the upper uint64 range.
                // Never re-register and replace an existing loose mount.
                while (std::any_of(ui_resources_owned->mounts().begin(), ui_resources_owned->mounts().end(),
                                   [&](const auto &mount) { return mount.id == next_ui_mount_id; })) {
                    if (next_ui_mount_id == 0)
                        throw std::runtime_error("No unused UI mount ID");
                    --next_ui_mount_id;
                }
                const auto opened = environment.open_fpk_archive(relative);
                const auto *source = std::get_if<std::shared_ptr<civ5::assets::FpkSource>>(&opened);
                if (!source)
                    throw std::runtime_error("Cannot open UI FPK " + relative + ": " +
                                             std::get<civ5::assets::FpkDiagnostic>(opened).reason);
                // (0,0) appends without sorting into one of the plan's groups.
                ui_resources_owned->mount({next_ui_mount_id,
                                           0,
                                           0,
                                           civ5::assets::resource_lookup(*source),
                                           true,
                                           preferred,
                                           civ5::assets::resource_lookup_all(*source),
                                           civ5::assets::resource_keys(*source)});
                mounted_archives.push_back(normalized);
            };
            // A supplied loose plan can override the base UI archive. Extra
            // explicit archives overlay that plan in exact argument order.
            mount_archive("Resource/DX9/UITextures.fpk", !ui_plan.has_value());
            for (std::size_t i = 0; i < ui_archives.size(); ++i) {
                if (ui_archives[i].is_absolute())
                    throw std::runtime_error("--ui-archive must be relative to the asset root");
                mount_archive(civ5::assets::path_utf8(ui_archives[i]), true);
            }
            if (ui_skin_directory) {
                if (ui_skin_directory->is_absolute())
                    throw std::runtime_error("--ui-skin-directory must be relative to the asset root");
                const auto asset_root = fs::canonical(root);
                const auto directory = fs::canonical(root / *ui_skin_directory);
                auto relative = directory.lexically_relative(asset_root);
                if (relative.empty() || relative.native().starts_with(L".."))
                    throw std::runtime_error("UI skin directory escapes the asset root");
                const auto skin_source = std::make_shared<civ5::assets::LooseResourceIndex>(asset_root);
                skin_source->scan({directory});
                if (skin_source->indexed_files() == 0)
                    throw std::runtime_error("UI skin directory is empty");
                while (std::any_of(ui_resources_owned->mounts().begin(), ui_resources_owned->mounts().end(),
                                   [&](const auto &mount) { return mount.id == next_ui_mount_id; })) {
                    if (next_ui_mount_id == 0)
                        throw std::runtime_error("No unused UI mount ID");
                    --next_ui_mount_id;
                }
                ui_resources_owned->mount({next_ui_mount_id,
                                           0, 0, civ5::assets::resource_lookup(skin_source),
                                           true, true, civ5::assets::resource_lookup_all(skin_source),
                                           civ5::assets::resource_keys(skin_source)});
            }
        }
        const auto *ui_resources_override = ui_resources_owned ? &*ui_resources_owned : nullptr;
        if(bnw_civis_resource_bank){
            check(!bnw_live_request->g0_selected_unit_receipt_root&&!bnw_early_lifetime_gates&&!ui_resources_override,
                  "Civis bank preview cannot combine G0/native package lifetime/borrowed resource diagnostic routes");
        }
        if (preview)
            SetProcessDPIAware();
        check(!fs::exists(out), "Fresh material world GPU output required");
        auto input = primitive_input(argv[2]);
        std::optional<civ5::saves::SavedMapTeamArtObservation> saved_art;
        std::optional<civ5::graphics::BnwRendererSceneReceipt> scene_receipt;
        std::optional<civ5::saves::SavedMapProjection> saved_minimap;
        std::optional<civ5::ui::TopPanelSavedProjection> saved_panel;
        civ5::ui::LiveUiHost::FrameProvider saved_ui_provider;
        std::optional<std::int32_t> saved_panel_turn;
        std::optional<std::int32_t> saved_turn_zero_year;
        std::optional<std::vector<civ5::terrain::RiverEdge>> saved_rivers;
        std::vector<TerrainFeature2Source> marsh_sources;
        std::vector<std::uint8_t> feature2_bytes;
        std::optional<TerrainMarshStamp> marsh_stamp;
        if (saved_fow_save) {
            const auto grid = input_bytes(*saved_grid);
            auto &source = input.worlds.at(0);
            check(grid == source.cells, "Saved terrain grid differs from the controlled world source");
            civ5::saves::SavedSessionDecodeContext save_context;
            save_context.decode_map_regions = true;
            save_context.decode_teams = true;
            save_context.decode_all_players = true;
            save_context.decode_supporting_classes = true;
            save_context.all_players = civ5::saves::AllPlayersContext{};
            if(bnw_live_request) {
                const auto container=civ5::saves::read_loaded_save(*saved_fow_save);
                if(container.container==civ5::saves::SaveContainerKind::civis &&
                   civ5::graphics::bnw_treasury_presentation_digest(
                       civ5::saves::read_civis_save(*saved_fow_save))) {
                    // Decode immutable map/art now. The launcher restores and
                    // validates this exact pair in LiveSession before drawing.
                    save_context.permit_live_treasury_overlay=true;
                    save_context.permit_live_bnw_treasury_balance_overlay=true;
                }
            }
            const auto saved = civ5::saves::read_saved_session(*saved_fow_save, save_context);
            const auto &snapshot = saved.snapshot;
            const auto &map = snapshot.map_header;
            check(snapshot.pregame && snapshot.map_plots && snapshot.map_terrain &&
                      source.input.mode == civ5::terrain::game_world_grid_mode(map.wrap_x) &&
                      source.input.wrap_x == map.wrap_x &&
                      map.grid.width == static_cast<std::uint64_t>(source.input.map_width) &&
                      map.grid.height == static_cast<std::uint64_t>(source.input.map_height) &&
                      source.input.mode == 1 &&
                      grid.size() == static_cast<std::size_t>(source.input.map_width) *
                                         (static_cast<std::size_t>(source.input.map_height) + 4) * 8,
                  "Saved FOW map does not match the controlled world layout");
            std::size_t verified_masks = 0;
            for (std::uint64_t x = 0; x < map.grid.width; ++x)
                for (std::uint64_t y = 0; y < map.grid.height; ++y) {
                    const auto &cell = snapshot.map_terrain->cells[static_cast<std::size_t>(x * map.grid.height + y)];
                    if (!cell.native_terrain_mask_known) continue;
                    const auto at = static_cast<std::size_t>((x * (map.grid.height + 4) + y + 2) * 8);
                    const auto mask = std::uint32_t(grid[at]) | std::uint32_t(grid[at + 1]) << 8 |
                                      std::uint32_t(grid[at + 2]) << 16 | std::uint32_t(grid[at + 3]) << 24;
                    check(mask == cell.native_terrain_mask, "Saved FOW save and terrain source differ");
                    ++verified_masks;
                }
            check(verified_masks > 0, "Saved FOW map has no independently classified terrain cells");
            saved_art = civ5::saves::observe_saved_map_team_art(*snapshot.pregame, *snapshot.map_plots);
            check(!bnw_live_request ||
                      bnw_live_request->active_player_id ==
                          static_cast<std::int64_t>(saved_art->active_player),
                  "Selected BNW live player differs from saved scene fog owner");
            check(snapshot.map_projection.has_value(), "Saved minimap terrain projection is missing");
            saved_minimap = *snapshot.map_projection;
            const auto turn = snapshot.pregame->archive.integers.find("game_turn");
            check(turn != snapshot.pregame->archive.integers.end() && turn->second >= 0 &&
                      turn->second <= std::numeric_limits<std::int32_t>::max(),
                  "Saved TopPanel game turn is missing or outside int32 range");
            saved_panel_turn = static_cast<std::int32_t>(turn->second);
            if (*saved_panel_turn == 0) {
                const auto *prefix = std::get_if<civ5::saves::Expansion2PrefixObservation>(
                    &snapshot.game_state.prefix);
                check(prefix != nullptr, "Saved turn-zero date requires the Expansion2 game prefix");
                constexpr std::size_t start_year_index = 4;
                static_assert(std::string_view(civ5::saves::Expansion2PrefixObservation::scalar_names[start_year_index]) ==
                              "start_year");
                const auto year = prefix->scalars[start_year_index];
                check(year >= std::numeric_limits<std::int32_t>::min() + 1LL &&
                          year <= std::numeric_limits<std::int32_t>::max(),
                      "Saved turn-zero start year is outside the supported range");
                saved_turn_zero_year = static_cast<std::int32_t>(year);
            }
            saved_panel = civ5::ui::project_top_panel_saved_player(
                snapshot, static_cast<std::int64_t>(saved_art->active_player));
            if (!bnw_live_request &&
                snapshot.game_state.layout == civ5::saves::GameStateLayout::expansion2_version1)
                saved_ui_provider = civ5::ui::make_saved_session_frame_provider(
                    snapshot, static_cast<std::int64_t>(saved_art->active_player));
            check(snapshot.map_terrain->cells.size() == map.grid.size(),
                  "Saved river plots differ from the terrain grid");
            std::vector<civ5::terrain::TerrainRiverPlot> river_plots;
            river_plots.reserve(snapshot.map_terrain->cells.size());
            for (const auto &cell : snapshot.map_terrain->cells) {
                check(cell.rivers.has_value(), "Saved river plot is unresolved");
                const auto &r = *cell.rivers;
                river_plots.push_back({r.is_river, r.west, r.northwest, r.northeast,
                                       r.flow_e, r.flow_se, r.flow_sw});
            }
            const auto grids = civ5::terrain::build_world_grid_layout(
                source.input.map_width, source.input.map_height, source.input.mode,
                source.input.factory);
            saved_rivers = civ5::terrain::build_terrain_river_edges(grids.source_grid, river_plots);
            const auto fixture_rivers = input.materials.at(0).edges.size();
            input.materials.at(0).edges = *saved_rivers;
            source.cells = grid;
            std::cout << "Saved map matched " << verified_masks << " decoded terrain cells and replaced "
                      << fixture_rivers << " fixture river edges with " << saved_rivers->size()
                      << " saved edges\n";
            if (bnw_live_request)
                scene_receipt=civ5::graphics::capture_bnw_renderer_scene_receipt(
                    *saved_fow_save,saved_save_sha256,saved,*saved_art,
                    civ5::ui::BnwSceneOverlayPolicy::treasury_presentation);
        }
        const civ5::ui::BnwPrepareRendererScene prepare_scene =
            [&](const civ5::ui::LiveBnwPreviewInput *input,
                civ5::graphics::BnwRendererSceneReceipt &receipt) {
                if (!input || !saved_minimap ||
                    !civ5::graphics::reconcile_bnw_renderer_scene_projection(
                        *saved_minimap,receipt,input->session.state()))
                    return false;
                // The launcher verifies a copy. Keep that verified revision
                // for the render callback and its unit/art bindings too.
                scene_receipt=receipt;
                return true;
            };
        if (saved_feature2) {
            feature2_bytes = blend_validation::read(*saved_feature2);
            check(feature2_bytes.size() % 20 == 0 && feature2_bytes.size() <=
                      static_cast<std::size_t>(input.worlds.at(0).input.map_width) *
                      input.worlds.at(0).input.map_height * 20,
                  "Saved feature-2 record extent differs from the map");
            const auto word = [&](std::size_t at) {
                std::uint32_t value = 0;
                for (unsigned i = 0; i < 4; ++i)
                    value |= std::uint32_t(feature2_bytes.at(at + i)) << (i * 8);
                return std::bit_cast<std::int32_t>(value);
            };
            for (std::size_t at = 0; at < feature2_bytes.size(); at += 20)
                marsh_sources.push_back({{word(at), word(at + 4), word(at + 8)},
                                         word(at + 12), word(at + 16)});
            const civ5::assets::AssetEnvironment environment(root);
            const auto textures = archive(environment, "Resource/DX9/TerrainTextures.fpk");
            const auto found = textures->find(civ5::assets::AssetKey("marsh_stamp.dds"));
            check(std::holds_alternative<civ5::assets::AssetHandle>(found), "Authored marsh mask missing");
            TerrainMarshStamp::Storage prior{};
            prior[0] = *marsh_prior_byte;
            marsh_stamp = TerrainMarshStamp::prepare(civ5::assets::DdsData::parse(
                std::get<civ5::assets::AssetHandle>(found).read_bytes(65536)), prior);
        }
        const auto geometry = blend_validation::read(argv[8]);
        check(geometry.size() == 152, "Original cache normal geometry extent");
        const std::array<fs::path, 4> shaders{argv[3], argv[4], argv[5], argv[6]};
        const auto cubes = cube_assets(root);
        auto context = std::make_shared<GlContext>();
        std::cerr << "GL_DEVICE " << context->description() << '\n' << std::flush;
        Control control(context);
        auto neutral = neutral_scene(), varied = fixtures();
        GlLandMaterialInputs plain_inputs, varied_inputs;
        for (unsigned i = 1; i < 10; ++i) {
            plain_inputs.pixel[i] = control.image(neutral[i]);
            varied_inputs.pixel[i] = control.image(varied[i]);
        }
        const auto lut = identity_volume();
        const auto volume = control.image(lut);
        GlMaterialWorldLighting light{
            {.75f, 1, .5f}, {.125f, -.25f, .75f}, {volume, volume, volume, volume, volume}};
        fs::create_directories(out);
        if (selected_gandhi_live_ice_record)
            blend_validation::text(out / "selected-gandhi-live-ice-source.json",
                "{\"schema\":\"civ5-selected-gandhi-live-ice-source-v1\",\"source\":\"passive_retail_dx9_kind9_packed_same_map\",\"saveSha256\":\"" +
                saved_save_sha256 + "\",\"captureSha256\":\"" +
                std::string(civ5::graphics::gandhi_live_ice_capture_sha256) +
                "\",\"recordSha256\":\"" +
                std::string(civ5::graphics::gandhi_live_ice_record_sha256) +
                "\",\"regionCount\":1,\"triangleCount\":31042,"
                "\"sourceSaveIdentityIndependentlyProven\":false,"
                "\"generatorReproductionProven\":false,"
                "\"fullMinimapParityProven\":false,\"fullGraphicsParity\":false}\n");
        if (saved_panel) {
            std::ostringstream report;
            const auto number = [&](const std::optional<std::int32_t> &value) {
                if (value) report << *value;
                else report << "null";
            };
            report << "{\"schema\":\"civ5-saved-top-panel-preview-v1\",\"activePlayer\":"
                   << saved_art->active_player << ",\"cityCount\":";
            number(saved_panel->city_count);
            report << ",\"unitProductionMaintenanceMod\":";
            number(saved_panel->unit_production_maintenance_mod);
            report << ",\"gold\":";
            number(saved_panel->gold);
            report << ",\"gameTurn\":" << *saved_panel_turn;
            report << ",\"startYear\":";
            if (saved_turn_zero_year) report << *saved_turn_zero_year;
            else report << "null";
            report << ",\"derivedRatesRecovered\":false,\"projectionPartial\":true}\n";
            blend_validation::text(out / "saved-top-panel.json", report.str());
        }
        const auto write_ui_session_status = [&] {
            if (!saved_ui_provider && !bnw_live_request) return;
            const bool live = bnw_live_request.has_value();
            std::ostringstream session_status;
            session_status << "{\"schema\":\"civ5-preview-ui-session-v1\",\"source\":\""
                           << (live ? "bnw_live_session" : "bnw_save")
                           << "\",\"sourceSha256\":\"" << saved_save_sha256
                           << "\",\"activePlayer\":" << saved_art->active_player
                           << ",\"gameTurn\":" << *saved_panel_turn
                           << ",\"readOnly\":true,\"liveSessionRestored\":"
                           << (live ? "true" : "false")
                           << ",\"gameplayBackendConnected\":false,\"blocker\":\""
                           << (live
                               ? "GameCore continuation, full turns and persistence are unavailable"
                               : "BNW rules bootstrap and GameCore continuation are unavailable")
                           << "\"}\n";
            blend_validation::text(out / "ui-session-status.json", session_status.str());
        };
        if (saved_rivers) {
            std::vector<std::uint8_t> river_bytes;
            river_bytes.reserve(saved_rivers->size() * 16);
            for (const auto &edge : *saved_rivers)
                for (const auto value : {edge.x, edge.y, edge.z, edge.direction})
                    for (unsigned byte = 0; byte < 4; ++byte)
                        river_bytes.push_back(static_cast<std::uint8_t>(static_cast<std::uint32_t>(value) >> (byte * 8)));
            blend_validation::write(out / "saved-river-edges.bin", river_bytes);
        }
        if (saved_feature2) blend_validation::write(out / "saved-feature2-sources.bin", feature2_bytes);
        std::unique_ptr<GlDistanceFog> distance_fog;
        if (fog_scene || preview_fog) {
            distance_fog = std::make_unique<GlDistanceFog>(*context, load_distance_fog(root));
            save_distance_fog(*distance_fog, out / "distance-fog");
            blend_validation::text(out / "original-distance-fog.txt",
                                   "777AC0 FogColor.csv/FogRamp.dds; 453E96 "
                                   "camera zoom; LAND slot2/water slot8.\n");
        }
        if (lighting_scene || preview_lighting) {
            std::array<float, 2> estimate{};
            const auto settings = terrain_lighting_defaults(X87Precision::double_precision);
            light = GlMaterialWorldLighting::from_terrain(settings, [&](float squared) {
                const float seed = _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(squared)));
                estimate = {squared, seed};
                return seed;
            });
            std::array<float, 14> values{};
            static_assert(sizeof(TerrainLighting) == 56);
            std::memcpy(values.data(), &settings, sizeof(settings));
            blend_validation::write(out / "terrain-lighting.f32", values);
            blend_validation::write(out / "terrain-lighting-estimate.f32", estimate);
            std::array<float, 6> cache{};
            std::copy(light.sun_direction.begin(), light.sun_direction.end(), cache.begin());
            std::copy(light.half_vector.begin(), light.half_vector.end(), cache.begin() + 3);
            blend_validation::write(out / "terrain-cache-lighting.f32", cache);
            blend_validation::text(out / "original-terrain-lighting.txt",
                                   "5494EA startup directions; 4372F0 cache identity camera.\n"
                                   "key4A PS c1: shadow direction; key4B PS c5: normalized sun plus +Z.\n");
            if (lighting_scene) {
                const fs::path original(argv[16]);
                check(blend_validation::read(original / "lighting.bin") ==
                              blend_validation::read(out / "terrain-lighting.f32") &&
                          blend_validation::read(original / "estimate.bin") ==
                              blend_validation::read(out / "terrain-lighting-estimate.f32"),
                      "Independent original terrain lighting/host instruction state differs");
                save(out / "original-terrain-frame.bin", blend_validation::read(original / "frame.bin"));
            }
        }
        std::shared_ptr<const GlColorKeys> color_keys;
        if (color_scene || preview_colors) {
            const auto authored = color_key_validation::load(root);
            color_keys = std::make_shared<const GlColorKeys>(*context, authored);
            light.color_correct = {};
            const auto colors = out / "color-keys";
            fs::create_directory(colors);
            save(colors / "identity.bgra", ColorKeyVolume::identity().bgra());
            std::ostringstream requests;
            for (unsigned i = 0; i < 25; ++i) {
                const auto &asset = authored.volumes()[i];
                const auto &texture = color_keys->at(asset.request.system, asset.request.style);
                const auto bytes = texture.read_mip_raw(0);
                check(texture.depth() == 32 && texture.plan().internal_format == GL_RGBA8 &&
                          std::equal(bytes.begin(), bytes.end(), asset.volume.bgra().begin(),
                                     asset.volume.bgra().end()),
                      "Authored ColorKey GPU byte identity");
                std::ostringstream name;
                name << std::setfill('0') << std::setw(2) << i << ".bgra";
                save(colors / name.str(), bytes);
                requests << i << '\t' << asset.authored << '\t' << asset.request.native_path << '\n';
            }
            blend_validation::text(colors / "requests.tsv", requests.str());
            blend_validation::text(out / "authored-color-keys.txt",
                                   "key4B slot8: Terrain. LAND slots6/7: TerrainFOW/TerrainShadow.\n"
                                   "25 owned linear A8R8G8B8 volumes, 32 cubed, one mip.\n");
        }
        blend_validation::write(out / "explicit-lut.f32", lut.values);
        blend_validation::write(out / "original-normal-geometry.bin", geometry);
        const auto land_sampling =
            fow_scene || preview_fow ? LandMipSampling::d3d9_rgb565 : LandMipSampling::d3d9_trilinear;
        blend_validation::text(out / "land-mip-fragment.glsl",
                               land_material_fragment_source(LandPixelProgram::cached_diffuse,
                                                             LandShadowArithmetic::measured_fma_t_x_z_y,
                                                             land_sampling));
        std::ostringstream execution;
        execution << context->description() << '\n';
        if (scheduled_preview) {
            const bool smoke = std::string_view(argv[10]) == "--camera-preview-fow-smoke";
            std::optional<material_preview_settings::Paths> explicit_paths;
            if (material_settings_directory) {
                explicit_paths = material_preview_settings::Paths{
                    *material_settings_directory / "GraphicsSettingsDX9.ini",
                    *material_settings_directory / "UserSettings.ini"};
                check(fs::is_regular_file(explicit_paths->graphics) &&
                          fs::is_regular_file(explicit_paths->game),
                      "Explicit material settings files are missing");
            }
            const auto profile = observed_adapter
                ? material_preview_settings::from_observed_adapter(
                      observed_adapter->first, observed_adapter->second,
                      explicit_paths ? &*explicit_paths : nullptr)
                : material_preview_settings::discover();
            blend_validation::text(out / "material-settings.json",
                                   material_preview_settings::report(profile));
            const bool low_texture_quality = profile.texture_quality_override == 0;
            PreviewPanelFixture panel;
            panel.saved_projection = saved_panel;
            panel.saved_game_turn = saved_panel_turn;
            const auto ui_language = material_settings_directory
                ? preview_ui_language(*material_settings_directory) : std::string("en_US");
            std::optional<bnw_capture_evidence::Producer> capture_producer;
            if (bnw_live_request) {
                const std::set<std::string> stock_archives{
                    "Resource/DX9/MiscTextures.fpk", "Resource/DX9/Expansion1UITextures.fpk",
                    "Resource/DX9/Expansion2UITextures.fpk"};
                const bool stock_ui = !ui_mount_plan_path &&
                    (!ui_skin_directory || ui_skin_directory->generic_string() == "Assets/DLC/Expansion2/UI/InGame") &&
                    std::all_of(ui_archives.begin(),ui_archives.end(),[&](const auto &p) {
                        return stock_archives.contains(p.generic_string());
                    });
                capture_producer.emplace(out,*bnw_live_request,*saved_fow_save,profile,
                    ui_language=="ru_RU"?"RU":ui_language=="en_US"?"EN":"unknown",stock_ui);
                bnw_live_request->observe_selected_profile = [&](const auto &b) { capture_producer->observe(b); };
            }
            std::shared_ptr<PreparedCameraViewer> prepared_viewer;
            const auto render = [&](std::shared_ptr<const civ5::ui::SelectedBnwPreviewHost> font_source_parent,
                                    const civ5::ui::LiveBnwPreviewInput *live_input
#ifdef CIVIS_ALPHA_RESEARCH_RESTART_FIXTURE
                                    ,const civ5::ui::SelectedBnwReadOnlyPreview* observed_preview=nullptr,
                                    const fs::path& restart_actual_input={},const fs::path& restart_actual_game_root={}
#endif
                                    ,std::shared_ptr<civ5::ui::BnwWorldViewFontSourceScope> own_fonts={}
                                    ) {
                if(prepared_viewer) prepared_viewer->require_initial_ticket();
            const auto map_rng_before = live_input
                ? std::optional(live_input->session.state().random.map.state()) : std::nullopt;
            const auto game_rng_before = live_input
                ? std::optional(live_input->session.state().random.gameplay.state()) : std::nullopt;
            std::size_t marsh_writes = 0;
            std::optional<civ5::graphics::BnwWarriorOwnedTerrain> owned_terrain;
            const auto world = [&] {
                if (owned_warrior_sdk_cache) {
                    check(live_input && scene_receipt && !low_texture_quality,
                          "Owned Warrior requires a live scene and the supported full texture profile");
                    owned_terrain.emplace(civ5::graphics::generate_bnw_gandhi_warrior_owned_terrain(
                        *saved_fow_save, root, argv[2], *saved_feature2, live_input->session.state().world));
                    marsh_writes = owned_terrain->receipt().marsh_writes;
                    return owned_terrain->world();
                }
                return build_world(root, input, 0,
                    marsh_stamp ? &*marsh_stamp : nullptr, marsh_sources,
                    [&](auto, std::int32_t, auto) { ++marsh_writes; }, saved_rivers.has_value(),
                    marsh_stamp.has_value(), low_texture_quality,
                    observed_final_r ? &*observed_final_r : nullptr);
            }();
            std::size_t low_texture_count = 0;
            for (const auto &texture : world->assets().textures())
                if (texture.asset.candidate().provenance.archive_relative_path ==
                    "Resource/DX9_Low/TerrainTexturesDX9Low.fpk")
                    ++low_texture_count;
            std::ostringstream texture_report;
            texture_report << "{\"schema\":\"civ5-terrain-texture-provider-v1\",\"lowOverlayMounted\":"
                           << (low_texture_quality ? "true" : "false")
                           << ",\"totalTextures\":" << world->assets().textures().size()
                           << ",\"lowTextures\":" << low_texture_count << "}\n";
            blend_validation::text(out / "terrain-texture-provider.json", texture_report.str());
            if (saved_fow_save)
                saved_land_style_prior_audit::write(*world, input.worlds.at(0),
                                                    out / "saved-land-style-prior.json");
            if (saved_feature2) {
                std::ostringstream report;
                report << "{\"schema\":\"civ5-saved-feature2-marsh-preview-v1\",\"records\":"
                       << marsh_sources.size() << ",\"writes\":" << marsh_writes
                       << ",\"controlledPriorByte\":" << unsigned(*marsh_prior_byte)
                       << ",\"stage\":\"before_coast_and_rivers\""
                       << ",\"fullGraphicsParity\":false}\n";
                blend_validation::text(out / "saved-feature2.json", report.str());
            }
            write_field(world->terrain(), out, "");
            if (observed_final_r) {
                check(civ5::runtime::identify_file(out / "height.bin").sha256 ==
                          "01406a41cec8962f0c61483c549e48765baa1a47e649bbe03b74f2291a4927d9" &&
                          civ5::runtime::identify_file(out / "material.bin").sha256 ==
                          "c5236527424a3f6203447f7e9ae87819f56bd95059e792e0f826bb5e6e7fa87b",
                      "Observed final R did not survive MaterialWorld page import");
                blend_validation::text(out / "observed-final-r.json",
                    "{\"schema\":\"civ5-g1-observed-final-r-diagnostic-v1\","
                    "\"source\":\"captured_retail_dx9_same_save\","
                    "\"fieldStage\":\"final_after_coast_rivers\","
                    "\"syntheticSourceDescriptors\":true,\"fullGraphicsParity\":false}\n");
            }
            plain_inputs.pixel[2] = distance_fog->ramp().id();
            GlMaterialWorldRenderer renderer(
                context, world, cubes, {false, true, true}, LightingCubeSampling::measured_coordinates,
                LandShadowArithmetic::measured_fma_t_x_z_y, LandMipSampling::d3d9_rgb565, color_keys);
            ScheduledMaterialPreview land(context, renderer, world, light, smoke, profile,
                                          audit_land_height_bindings);
                std::unique_ptr<civ5::graphics::GlBnwWarriorOwnedScene> owned_warrior;
                if (owned_terrain) {
                    namespace assets = civ5::assets;
                    const auto &identities = live_input->production_rules.info_identity.at("Units");
                    std::optional<std::int64_t> warrior_type;
                    for (const auto &[id,type] : identities)
                        if (type && *type == "UNIT_WARRIOR") {
                            check(!warrior_type, "Activated Warrior type is ambiguous");
                            warrior_type = id;
                        }
                    check(warrior_type.has_value(), "Activated BNW Warrior type unavailable");
                    const assets::AssetEnvironment environment(root);
                    const auto models = archive(environment,"Resource/Common/UnitModels.fpk");
                    const auto config_handle = models->find(assets::AssetKey("UnitVisConfig.ini"));
                    check(std::holds_alternative<assets::AssetHandle>(config_handle),
                          "Actual UnitVisConfig FPK entry missing");
                    const auto configuration = civ5::graphics::read_bnw_unit_visual_configuration(
                        std::get<assets::AssetHandle>(config_handle));
                    civ5::graphics::BnwImportedUnitVisualOwner units(
                        live_input->session.state().world,configuration,*warrior_type);
                    const assets::LooseSource loose(root);
                    const auto xml = loose.find(assets::AssetKey("Assets/Units/Civ5ArtDefines_UnitMembers.xml"));
                    check(std::holds_alternative<assets::AssetHandle>(xml), "Selected member XML missing");
                    std::optional<civ5::graphics::BnwUnitMemberTransformRules> rules;
                    for (const auto &spec : civ5::graphics::bnw_gandhi_warrior_art) {
                        const auto current = civ5::graphics::read_bnw_unit_member_transform_rules(
                            std::get<assets::AssetHandle>(xml),spec.art_type,configuration.scene_spacing);
                        check(!rules || *rules == current.rules, "Warrior members require distinct transforms");
                        rules = current.rules;
                    }
                    owned_warrior = std::make_unique<civ5::graphics::GlBnwWarriorOwnedScene>(
                        context,root,*owned_warrior_sdk_cache,*scene_receipt,live_input->session.state(),
                        *owned_terrain,std::move(units),*rules,0,16385);
                    owned_warrior->select_sdk_sample(0);
                }
                if (live_input && scene_receipt &&
                    saved_save_sha256 == civ5::graphics::gandhi_live_ice_save_sha256) {
                    const auto warrior =
                        civ5::graphics::selected_gandhi_warrior_from_live_scene(
                            *scene_receipt,live_input->session.state(),
                            live_input->active_player_id);
                    if (warrior) {
                        const auto gpu_mips = civ5::graphics::probe_bnw_gandhi_warrior_art(
                            root,*context);
                        blend_validation::text(out / "bnw-warrior-art-probe.json",
                            "{\"schema\":\"civ5-bnw-gandhi-warrior-art-probe-v1\",\"source\":\"restored_live_scene_and_installed_bnw_assets\",\"unitId\":16385,\"owner\":0,\"plotIndex\":592,\"plotXY\":[32,14],\"visible\":true,\"formationMembers\":12,\"gr2MeshesLocated\":3,\"bc3TexturesUploaded\":6,\"bc3MipsReadBack\":" +
                            std::to_string(gpu_mips) +
                            ",\"generatedSceneOwner\":" + (owned_warrior ? std::string("true") : std::string("false")) +
                            ",\"modelDrawable\":false,\"blocker\":\"Native animation, material and complete world draw acceptance pending\",\"fullGraphicsParity\":false}\n");
                    } else {
                        blend_validation::text(out / "bnw-warrior-art-probe.json",
                            "{\"schema\":\"civ5-bnw-gandhi-warrior-art-probe-v1\",\"source\":\"restored_live_scene\",\"visible\":false,\"modelDrawable\":false,\"blocker\":\"unit absent or hidden in restored scene\",\"fullGraphicsParity\":false}\n");
                    }
                }
                write_immutable_preview_fingerprint_diagnostic(live_input, out, "before");
                // Materialize SAME-scope resources and selected language before
                // any argument can move that scope. The deferred panel callback
                // retains its actual language decision independently of the move.
                const auto* camera_ui_resources = own_fonts ? &own_fonts->resources() : ui_resources_override;
                const std::string camera_language = own_fonts ? own_fonts->locale().language : ui_language;
                const bool camera_panel_ru = camera_language == "ru_RU";
                auto* camera_ui_snapshot_owner = live_input && ui_resources_override && !own_fonts ? &ui_resources_owned : nullptr;
                run_camera_preview(context, world, land, control, plain_inputs, root, out, smoke,
                                   [&, camera_panel_ru] { return panel.state(smoke, camera_panel_ru); }, true,
                                   distance_fog.get(), true, color_keys.get(), river_preview, {},
                                   camera_ui_resources, saved_art ? &*saved_art : nullptr,
                                   saved_minimap ? &*saved_minimap : nullptr,
                                   !saved_panel,
                                   camera_language, saved_turn_zero_year,
                                   native_ice_records.value_or(fs::path{}), focus_revealed_ice,
                                   native_minimap_edge_records_path, scene_capture_sha256,
                                   native_minimap_city_marker_fixture_path,
                                   saved_ui_provider, nullptr, live_input,
                                   retail_camera ? &*retail_camera : nullptr,
                                   native_minimap_bounds && !focus_revealed_ice
                                       ? &*native_minimap_bounds : nullptr,
                                   g0_windowed_selected_marker_visible,
                                   live_input && scene_receipt ? &*scene_receipt : nullptr,
                                   capture_producer && live_input ? capture_producer->hook(live_input)
                                       : civ5::graphics::BnwClientCaptureHook{},
                                   capture_producer ? capture_producer->ui_sources() : nullptr,
                                   owned_warrior.get(),prepared_viewer,bnw_early_initial_smoke,
                                   std::move(font_source_parent),
                                   camera_ui_snapshot_owner,
                                   bnw_worldview_font_source_gates,bnw_production_component_validation
#ifdef CIVIS_ALPHA_RESEARCH_RESTART_FIXTURE
                                   ,observed_preview,restart_actual_input,restart_actual_game_root
#endif
                                   ,std::move(own_fonts));
                write_immutable_preview_fingerprint_diagnostic(live_input, out, "after");
                land.write_report(out);
                if (owned_terrain) {
                    const auto &generation = owned_terrain->receipt();
                    check(map_rng_before == live_input->session.state().random.map.state() &&
                              game_rng_before == live_input->session.state().random.gameplay.state(),
                          "Shared Warrior preview consumed simulation RNG");
                    blend_validation::text(out / "bnw-warrior-terrain-owner.json",
                        "{\"schema\":\"bnw-warrior-shared-preview-terrain-v1\",\"sharedLandWaterUnitWorld\":true,"
                        "\"simulationRngUnchanged\":true,\"sourceSaveSha256\":\"" + generation.source_save_sha256 +
                        "\",\"sourcePayloadSha256\":\"" + generation.source_payload_sha256 + "\","
                        "\"verifiedMapMasks\":" + std::to_string(generation.verified_masks) +
                        ",\"marshWrites\":" + std::to_string(generation.marsh_writes) +
                        ",\"heightPoolSha256\":\"" + generation.final_height_pool_sha256 +
                        "\",\"originalLoadingRngRecovered\":false,\"modelDrawable\":false,\"fullGraphicsParity\":false}\n");
                }
            };
            if (bnw_live_request) {
                check(scene_receipt.has_value(),
                      "Selected BNW renderer has no verified restored scene receipt");
                // Configure final layout on the already owned GlContext HWND
                // BEFORE input acquisition, discovery and real EARLY bootstrap.
                prepared_viewer=PreparedCameraViewer::create(context,
                    retail_camera ? &*retail_camera : nullptr,true);
                prepared_viewer->require_initial_ticket();
                // Capture from the real renderer and original private viewport
                // ticket before EARLY/native UI acquisition. NativeRuntime bind
                // remains a separate typed consumer; this is no UI/GPU seal.
                const auto renderer_window_owner=
                    prepared_viewer->viewport_transaction->capture_renderer_window_owner(context);
                renderer_window_owner->require_current();
                prepared_viewer->write_window_state(out/"early-viewer-preparation.json","prepared-before-acquisition");
                std::shared_ptr<const civ5::ui::BnwNativePackageDiscoveryOwner> early_package;
                std::shared_ptr<civ5::assets::CivisResourceBankOwner> own_bank;
                std::shared_ptr<const civ5::runtime::BnwSelectedLanguageInputOwner> own_language;
                try {
                    const auto lease=bnw_launch_stage("EARLY_lease_acquire",[&]{return civ5::ui::BnwNativeEarlyInputLease::acquire(
                        bnw_live_request->save_path,bnw_live_request->game_root,root,
                        reinterpret_cast<std::uintptr_t>(prepared_viewer->window),
                        prepared_viewer->early_client_provider());});
                    if(bnw_civis_resource_bank){
                        own_bank=bnw_launch_stage("CivisBank_capture",[&]{return civ5::assets::CivisResourceBankOwner::capture(
                            lease,std::span<const civ5::assets::CivisRequiredResource>{civis_bank_required_ui_roots()});});
                        own_language=civ5::runtime::BnwSelectedLanguageInputOwner::capture_from_civis_resources(own_bank);
                        own_language->require_current();
                    }else early_package=bnw_launch_stage("Package_create",[&]{return civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);});
                    prepared_viewer->require_initial_ticket();
                } catch(const civ5::runtime::CivisLanguageSelectionRequired& selection) {
                    civ5::ui::BnwLivePreviewOutcome required;
                    required.blocker=civ5::ui::SelectedBnwPreviewBlocker{civ5::ui::SelectedBnwPreviewStage::selected_save,selection.what()};
                    required.language_selection_required.emplace();
                    for(const auto& choice:selection.available_languages())
                        required.language_selection_required->available_languages.push_back({choice.type,choice.display_name});
                    require_civis_launch_rendered(required);throw;
                } catch(const std::exception& e) {
                    throw std::runtime_error(std::string("Selected BNW initial input blocked (no legacy fallback): ")+e.what());
                }
                civ5::ui::BnwLivePreviewOutcome outcome;
                if(bnw_civis_resource_bank){
                    own_language->require_current();
                    const auto bank_ui=civ5::ui::CivisResourceBankUiSnapshot::capture(own_bank);
                    const auto bank_fonts=civ5::ui::CivisResourceBankFontColorSnapshot::capture(own_bank,bank_ui);
                    const auto scope=civ5::ui::BnwWorldViewFontSourceScope::create_from_civis_resources(
                        own_bank,bank_ui,bank_fonts,own_language->language());
                    own_language->require_current();
                    const auto actual_save=bnw_live_request->save_path;
                    const auto actual_root=bnw_live_request->game_root;
                    const civ5::ui::BnwObservedCivisStockUiSourceCameraRender observed_bank_render=
                        [&render,prepared_viewer,renderer_window_owner,actual_save,actual_root,own_language](
                            std::shared_ptr<civ5::ui::BnwOwnedCameraUiSourceInput> source,
                            const civ5::ui::SelectedBnwReadOnlyPreview& actual_preview,
                            std::shared_ptr<civ5::runtime::BnwSelectedStockLuaSourceOwner> stock_owner,
                            std::shared_ptr<civ5::ui::BnwWorldViewFontSourceScope> own_scope){
                            own_language->require_current();source->require_current();
                            check(stock_owner&&own_scope&&!prepared_viewer->stock_source_owner,"Civis stock renderer binding missing or repeated");
                            prepared_viewer->stock_source_owner=stock_owner;prepared_viewer->renderer_window_owner=renderer_window_owner;
                            struct Clear{std::shared_ptr<PreparedCameraViewer> viewer;
                                ~Clear(){viewer->retire_gui_screen();viewer->stock_source_owner.reset();viewer->renderer_window_owner.reset();}} clear{prepared_viewer};
                            const auto host=source->parent();const auto* input=&source->input();source->require_current();
                            render(host,input
#ifdef CIVIS_ALPHA_RESEARCH_RESTART_FIXTURE
                                ,&actual_preview,actual_save,actual_root
#endif
                                ,std::move(own_scope));
                            (void)actual_preview;(void)actual_save;(void)actual_root;
                            source->require_current();own_language->require_current();
                        };
                    outcome=civ5::ui::launch_selected_bnw_camera_preview_from_civis_resources_with_stock_renderer_with_observed_ui_source(
                        *bnw_live_request,*saved_fow_save,own_bank,bank_ui,bank_fonts,scope,renderer_window_owner,
                        prepared_viewer->early_client_provider(),observed_bank_render,&*scene_receipt,prepare_scene);
                    require_civis_launch_rendered(outcome);
                    own_language->require_current();
                }else{
                // Named route owns registration/settings/Common exactly once.
#ifdef CIVIS_ALPHA_RESEARCH_RESTART_FIXTURE
                const auto restart_actual_input=bnw_live_request->save_path;
                const auto restart_actual_game_root=bnw_live_request->game_root;
                const civ5::ui::BnwObservedStockUiSourceCameraRender observed_render=
                    [&render,prepared_viewer,renderer_window_owner,restart_actual_input,restart_actual_game_root](
                        std::shared_ptr<civ5::ui::BnwOwnedCameraUiSourceInput> source,
                        const civ5::ui::SelectedBnwReadOnlyPreview& actual_preview,
                        std::shared_ptr<civ5::runtime::BnwSelectedStockLuaSourceOwner> stock_owner) {
                        source->require_current();
                        check(stock_owner&&!prepared_viewer->stock_source_owner,
                              "Observed stock renderer owner missing or already installed");
                        prepared_viewer->stock_source_owner=stock_owner;
                        prepared_viewer->renderer_window_owner=renderer_window_owner;
                        struct Clear {
                            std::shared_ptr<PreparedCameraViewer> viewer;
                            ~Clear(){viewer->retire_gui_screen();viewer->stock_source_owner.reset();viewer->renderer_window_owner.reset();}
                        } clear{prepared_viewer};
                        const auto actual_host=source->parent();
                        const auto* actual_input=&source->input();source->require_current();
                        render(actual_host,actual_input,&actual_preview,restart_actual_input,restart_actual_game_root);
                        source->require_current();
                    };
                outcome=civ5::ui::launch_selected_bnw_camera_preview_from_early_package_with_stock_renderer_with_observed_ui_source(
                    *bnw_live_request,*saved_fow_save,early_package,renderer_window_owner,
                    prepared_viewer->early_client_provider(),observed_render,&*scene_receipt,prepare_scene);
#else
                const civ5::ui::BnwOwnedStockHostCameraRender stock_render=
                    [&render,prepared_viewer,renderer_window_owner](std::shared_ptr<const civ5::ui::SelectedBnwPreviewHost> host,
                        const civ5::ui::LiveBnwPreviewInput* input,
                        std::shared_ptr<civ5::runtime::BnwSelectedStockLuaSourceOwner> stock_owner) {
                        check(stock_owner&&!prepared_viewer->stock_source_owner,
                              "Actual stock renderer owner missing or already installed");
                        prepared_viewer->stock_source_owner=stock_owner;
                        prepared_viewer->renderer_window_owner=renderer_window_owner;
                        struct Clear {
                            std::shared_ptr<PreparedCameraViewer> viewer;
                            ~Clear(){viewer->retire_gui_screen();viewer->stock_source_owner.reset();viewer->renderer_window_owner.reset();}
                        } clear{prepared_viewer};
                        render(std::move(host),input);
                    };
                outcome = bnw_live_request->g0_selected_unit_receipt_root
                    ? civ5::ui::launch_selected_bnw_camera_preview_from_early_package_with_owned_host(
                        *bnw_live_request,*saved_fow_save,early_package,render,
                        &*scene_receipt,prepare_scene)
                    : civ5::ui::launch_selected_bnw_camera_preview_from_early_package_with_stock_renderer(
                        *bnw_live_request,*saved_fow_save,early_package,renderer_window_owner,
                        prepared_viewer->early_client_provider(),stock_render,
                        &*scene_receipt,prepare_scene);
#endif
                }
                require_civis_launch_rendered(outcome);
                prepared_viewer->require_initial_ticket();
                prepared_viewer->write_window_state(out/"early-viewer-after-render.json","accepted-after-render-before-teardown");
                blend_validation::text(out / "early-viewer-launch-outcome.json",
                    "{\"schema\":\"civis-early-viewer-launch-outcome-v1\",\"namedLaunchAcceptedCurrent\":true,"
                    "\"sameParentOwnedHWND\":true,\"full63\":false,\"nativePixelParity\":false}\n");
                if(bnw_early_lifetime_gates)
                    verify_prepared_viewer_lifetime(prepared_viewer,early_package,out);
                // Parent teardown is after named-launch final currentness and
                // its acceptance receipt. Never close inside initial callback.
                prepared_viewer.reset();
            } else render({},nullptr);
            write_ui_session_status();
            // A restored BNW frame supplies its own TopPanel presentation;
            // the saved/synthetic preview fixture must not be sampled then.
            check(!smoke || panel.samples == (bnw_live_request ? 0U : 60U),
                  "TopPanel must use exactly one source per displayed frame");
            return 0;
        }
        if (preview) {
            const fs::path previous(argv[11]);
            check(fs::is_regular_file(previous / "original-distance-fog.txt") == preview_fog,
                  "Camera preview fog mode must match its scene producer");
            if (preview_fog) {
                for (const char *name : {"original-distance-fog.txt", "distance-fog/color.csv",
                                         "distance-fog/ramp.dds", "distance-fog/startup.bin"})
                    check(blend_validation::read(previous / name) == blend_validation::read(out / name),
                          "Camera preview fog resources differ from scene producer");
                plain_inputs.pixel[2] = distance_fog->ramp().id();
            }
            check(fs::is_regular_file(previous / "original-terrain-lighting.txt") == preview_lighting,
                  "Camera preview lighting mode must match its cache producer");
            if (preview_lighting)
                for (const char *name : {"original-terrain-lighting.txt", "terrain-lighting.f32",
                                         "terrain-lighting-estimate.f32", "terrain-cache-lighting.f32"})
                    check(blend_validation::read(previous / name) == blend_validation::read(out / name),
                          "Camera preview lighting differs from cache producer");
            check(fs::is_regular_file(previous / "authored-color-keys.txt") == preview_colors,
                  "Camera preview ColorKey mode must match its cache producer");
            if (preview_colors) {
                check(input_bytes(previous / "authored-color-keys.txt") ==
                              input_bytes(out / "authored-color-keys.txt") &&
                          input_bytes(previous / "color-keys/requests.tsv") ==
                              input_bytes(out / "color-keys/requests.tsv"),
                      "Camera preview ColorKey bank/source order differs from cache "
                      "producer");
                for (unsigned i = 0; i < 25; ++i) {
                    std::ostringstream name;
                    name << "color-keys/" << std::setfill('0') << std::setw(2) << i << ".bgra";
                    check(input_bytes(previous / name.str()) == input_bytes(out / name.str()),
                          "Camera preview ColorKey bytes differ from cache producer");
                }
            }
            const auto world = build_world(root, input, 0, nullptr, {}, {}, saved_rivers.has_value());
            if (river_preview) {
                write_field(world->terrain(), out, "");
                for (const char *file : {"height.bin", "material.bin"})
                    check(input_bytes(out / file) == input_bytes(previous / "0" / file),
                          "River preview source differs from its cache producer");
            }
            CameraReplayLand land(context, world, cubes, argv[11], color_keys, land_sampling);
            const bool smoke = std::string(argv[10]) == "--camera-preview-smoke" ||
                                   std::string(argv[10]) == "--camera-preview-colors-smoke" ||
                                   std::string(argv[10]) == "--camera-preview-lighting-smoke" ||
                                   std::string(argv[10]) == "--camera-preview-fog-smoke" ||
                                   std::string(argv[10]) == "--camera-preview-fow-smoke";
            PreviewPanelFixture panel;
            panel.saved_projection = saved_panel;
            panel.saved_game_turn = saved_panel_turn;
            std::shared_ptr<PreparedCameraViewer> prepared_viewer;
            const auto render = [&](std::shared_ptr<const civ5::ui::SelectedBnwPreviewHost> font_source_parent,
                                    const civ5::ui::LiveBnwPreviewInput *live_input
#ifdef CIVIS_ALPHA_RESEARCH_RESTART_FIXTURE
                                    ,const civ5::ui::SelectedBnwReadOnlyPreview* observed_preview=nullptr,
                                    const fs::path& restart_actual_input={},const fs::path& restart_actual_game_root={}
#endif
                                    ,std::shared_ptr<civ5::ui::BnwWorldViewFontSourceScope> own_fonts={}
                                    ) {
                if(prepared_viewer) prepared_viewer->require_initial_ticket();
                if (live_input && scene_receipt &&
                    saved_save_sha256 == civ5::graphics::gandhi_live_ice_save_sha256) {
                    const auto warrior =
                        civ5::graphics::selected_gandhi_warrior_from_live_scene(
                            *scene_receipt,live_input->session.state(),
                            live_input->active_player_id);
                    if (warrior) {
                        const auto gpu_mips = civ5::graphics::probe_bnw_gandhi_warrior_art(
                            root,*context);
                        blend_validation::text(out / "bnw-warrior-art-probe.json",
                            "{\"schema\":\"civ5-bnw-gandhi-warrior-art-probe-v1\",\"source\":\"restored_live_scene_and_installed_bnw_assets\",\"unitId\":16385,\"owner\":0,\"plotIndex\":592,\"plotXY\":[32,14],\"visible\":true,\"formationMembers\":12,\"gr2MeshesLocated\":3,\"bc3TexturesUploaded\":6,\"bc3MipsReadBack\":" +
                            std::to_string(gpu_mips) +
                            ",\"modelDrawable\":false,\"blocker\":\"Native formation member transforms and world placement unavailable\",\"fullGraphicsParity\":false}\n");
                    } else {
                        blend_validation::text(out / "bnw-warrior-art-probe.json",
                            "{\"schema\":\"civ5-bnw-gandhi-warrior-art-probe-v1\",\"source\":\"restored_live_scene\",\"visible\":false,\"modelDrawable\":false,\"blocker\":\"unit absent or hidden in restored scene\",\"fullGraphicsParity\":false}\n");
                    }
                }
                write_immutable_preview_fingerprint_diagnostic(live_input, out, "before");
                const auto* camera_ui_resources = own_fonts ? &own_fonts->resources() : ui_resources_override;
                const std::string camera_language = own_fonts ? own_fonts->locale().language : "en_US";
                auto* camera_ui_snapshot_owner = live_input && ui_resources_override && !own_fonts ? &ui_resources_owned : nullptr;
                run_camera_preview(context, world, land, control, plain_inputs, root, out, smoke,
                                   [&] { return panel.state(smoke); },
                                   preview_lighting, distance_fog.get(), preview_fow, color_keys.get(),
                                   river_preview, {}, camera_ui_resources, saved_art ? &*saved_art : nullptr,
                                   saved_minimap ? &*saved_minimap : nullptr,
                                   !saved_panel,
                                   camera_language, saved_turn_zero_year,
                                   native_ice_records.value_or(fs::path{}), focus_revealed_ice,
                                   native_minimap_edge_records_path, scene_capture_sha256,
                                   native_minimap_city_marker_fixture_path,
                                   saved_ui_provider, nullptr, live_input,
                                   retail_camera ? &*retail_camera : nullptr,
                                   native_minimap_bounds && !focus_revealed_ice
                                       ? &*native_minimap_bounds : nullptr,
                                   false, live_input && scene_receipt ? &*scene_receipt : nullptr,
                                   {},{},nullptr,prepared_viewer,bnw_early_initial_smoke,
                                   std::move(font_source_parent),
                                   camera_ui_snapshot_owner,
                                   bnw_worldview_font_source_gates,bnw_production_component_validation
#ifdef CIVIS_ALPHA_RESEARCH_RESTART_FIXTURE
                                   ,observed_preview,restart_actual_input,restart_actual_game_root
#endif
                                   ,std::move(own_fonts));
                write_immutable_preview_fingerprint_diagnostic(live_input, out, "after");
            };
            if (bnw_live_request) {
                check(scene_receipt.has_value(),
                      "Selected BNW renderer has no verified restored scene receipt");
                // Configure final layout on the already owned GlContext HWND
                // BEFORE input acquisition, discovery and real EARLY bootstrap.
                prepared_viewer=PreparedCameraViewer::create(context,
                    retail_camera ? &*retail_camera : nullptr,true);
                prepared_viewer->require_initial_ticket();
                // Capture from the real renderer and original private viewport
                // ticket before EARLY/native UI acquisition. NativeRuntime bind
                // remains a separate typed consumer; this is no UI/GPU seal.
                const auto renderer_window_owner=
                    prepared_viewer->viewport_transaction->capture_renderer_window_owner(context);
                renderer_window_owner->require_current();
                prepared_viewer->write_window_state(out/"early-viewer-preparation.json","prepared-before-acquisition");
                std::shared_ptr<const civ5::ui::BnwNativePackageDiscoveryOwner> early_package;
                std::shared_ptr<civ5::assets::CivisResourceBankOwner> own_bank;
                std::shared_ptr<const civ5::runtime::BnwSelectedLanguageInputOwner> own_language;
                try {
                    const auto lease=bnw_launch_stage("EARLY_lease_acquire",[&]{return civ5::ui::BnwNativeEarlyInputLease::acquire(
                        bnw_live_request->save_path,bnw_live_request->game_root,root,
                        reinterpret_cast<std::uintptr_t>(prepared_viewer->window),
                        prepared_viewer->early_client_provider());});
                    if(bnw_civis_resource_bank){
                        own_bank=bnw_launch_stage("CivisBank_capture",[&]{return civ5::assets::CivisResourceBankOwner::capture(
                            lease,std::span<const civ5::assets::CivisRequiredResource>{civis_bank_required_ui_roots()});});
                        own_language=civ5::runtime::BnwSelectedLanguageInputOwner::capture_from_civis_resources(own_bank);
                        own_language->require_current();
                    }else early_package=bnw_launch_stage("Package_create",[&]{return civ5::ui::BnwNativePackageDiscoveryOwner::create(lease);});
                    prepared_viewer->require_initial_ticket();
                } catch(const civ5::runtime::CivisLanguageSelectionRequired& selection) {
                    civ5::ui::BnwLivePreviewOutcome required;
                    required.blocker=civ5::ui::SelectedBnwPreviewBlocker{civ5::ui::SelectedBnwPreviewStage::selected_save,selection.what()};
                    required.language_selection_required.emplace();
                    for(const auto& choice:selection.available_languages())
                        required.language_selection_required->available_languages.push_back({choice.type,choice.display_name});
                    require_civis_launch_rendered(required);throw;
                } catch(const std::exception& e) {
                    throw std::runtime_error(std::string("Selected BNW initial input blocked (no legacy fallback): ")+e.what());
                }
                civ5::ui::BnwLivePreviewOutcome outcome;
                if(bnw_civis_resource_bank){
                    own_language->require_current();
                    const auto bank_ui=civ5::ui::CivisResourceBankUiSnapshot::capture(own_bank);
                    const auto bank_fonts=civ5::ui::CivisResourceBankFontColorSnapshot::capture(own_bank,bank_ui);
                    const auto scope=civ5::ui::BnwWorldViewFontSourceScope::create_from_civis_resources(
                        own_bank,bank_ui,bank_fonts,own_language->language());
                    own_language->require_current();
                    const auto actual_save=bnw_live_request->save_path;
                    const auto actual_root=bnw_live_request->game_root;
                    const civ5::ui::BnwObservedCivisStockUiSourceCameraRender observed_bank_render=
                        [&render,prepared_viewer,renderer_window_owner,actual_save,actual_root,own_language](
                            std::shared_ptr<civ5::ui::BnwOwnedCameraUiSourceInput> source,
                            const civ5::ui::SelectedBnwReadOnlyPreview& actual_preview,
                            std::shared_ptr<civ5::runtime::BnwSelectedStockLuaSourceOwner> stock_owner,
                            std::shared_ptr<civ5::ui::BnwWorldViewFontSourceScope> own_scope){
                            own_language->require_current();source->require_current();
                            check(stock_owner&&own_scope&&!prepared_viewer->stock_source_owner,"Civis stock renderer binding missing or repeated");
                            prepared_viewer->stock_source_owner=stock_owner;prepared_viewer->renderer_window_owner=renderer_window_owner;
                            struct Clear{std::shared_ptr<PreparedCameraViewer> viewer;
                                ~Clear(){viewer->retire_gui_screen();viewer->stock_source_owner.reset();viewer->renderer_window_owner.reset();}} clear{prepared_viewer};
                            const auto host=source->parent();const auto* input=&source->input();source->require_current();
                            render(host,input
#ifdef CIVIS_ALPHA_RESEARCH_RESTART_FIXTURE
                                ,&actual_preview,actual_save,actual_root
#endif
                                ,std::move(own_scope));
                            (void)actual_preview;(void)actual_save;(void)actual_root;
                            source->require_current();own_language->require_current();
                        };
                    outcome=civ5::ui::launch_selected_bnw_camera_preview_from_civis_resources_with_stock_renderer_with_observed_ui_source(
                        *bnw_live_request,*saved_fow_save,own_bank,bank_ui,bank_fonts,scope,renderer_window_owner,
                        prepared_viewer->early_client_provider(),observed_bank_render,&*scene_receipt,prepare_scene);
                    require_civis_launch_rendered(outcome);
                    own_language->require_current();
                }else{
                // Named route owns registration/settings/Common exactly once.
#ifdef CIVIS_ALPHA_RESEARCH_RESTART_FIXTURE
                const auto restart_actual_input=bnw_live_request->save_path;
                const auto restart_actual_game_root=bnw_live_request->game_root;
                const civ5::ui::BnwObservedStockUiSourceCameraRender observed_render=
                    [&render,prepared_viewer,renderer_window_owner,restart_actual_input,restart_actual_game_root](
                        std::shared_ptr<civ5::ui::BnwOwnedCameraUiSourceInput> source,
                        const civ5::ui::SelectedBnwReadOnlyPreview& actual_preview,
                        std::shared_ptr<civ5::runtime::BnwSelectedStockLuaSourceOwner> stock_owner) {
                        source->require_current();
                        check(stock_owner&&!prepared_viewer->stock_source_owner,
                              "Observed stock renderer owner missing or already installed");
                        prepared_viewer->stock_source_owner=stock_owner;
                        prepared_viewer->renderer_window_owner=renderer_window_owner;
                        struct Clear {
                            std::shared_ptr<PreparedCameraViewer> viewer;
                            ~Clear(){viewer->retire_gui_screen();viewer->stock_source_owner.reset();viewer->renderer_window_owner.reset();}
                        } clear{prepared_viewer};
                        const auto actual_host=source->parent();
                        const auto* actual_input=&source->input();source->require_current();
                        render(actual_host,actual_input,&actual_preview,restart_actual_input,restart_actual_game_root);
                        source->require_current();
                    };
                outcome=civ5::ui::launch_selected_bnw_camera_preview_from_early_package_with_stock_renderer_with_observed_ui_source(
                    *bnw_live_request,*saved_fow_save,early_package,renderer_window_owner,
                    prepared_viewer->early_client_provider(),observed_render,&*scene_receipt,prepare_scene);
#else
                const civ5::ui::BnwOwnedStockHostCameraRender stock_render=
                    [&render,prepared_viewer,renderer_window_owner](std::shared_ptr<const civ5::ui::SelectedBnwPreviewHost> host,
                        const civ5::ui::LiveBnwPreviewInput* input,
                        std::shared_ptr<civ5::runtime::BnwSelectedStockLuaSourceOwner> stock_owner) {
                        check(stock_owner&&!prepared_viewer->stock_source_owner,
                              "Actual stock renderer owner missing or already installed");
                        prepared_viewer->stock_source_owner=stock_owner;
                        prepared_viewer->renderer_window_owner=renderer_window_owner;
                        struct Clear {
                            std::shared_ptr<PreparedCameraViewer> viewer;
                            ~Clear(){viewer->retire_gui_screen();viewer->stock_source_owner.reset();viewer->renderer_window_owner.reset();}
                        } clear{prepared_viewer};
                        render(std::move(host),input);
                    };
                outcome = bnw_live_request->g0_selected_unit_receipt_root
                    ? civ5::ui::launch_selected_bnw_camera_preview_from_early_package_with_owned_host(
                        *bnw_live_request,*saved_fow_save,early_package,render,
                        &*scene_receipt,prepare_scene)
                    : civ5::ui::launch_selected_bnw_camera_preview_from_early_package_with_stock_renderer(
                        *bnw_live_request,*saved_fow_save,early_package,renderer_window_owner,
                        prepared_viewer->early_client_provider(),stock_render,
                        &*scene_receipt,prepare_scene);
#endif
                }
                require_civis_launch_rendered(outcome);
                prepared_viewer->require_initial_ticket();
                prepared_viewer->write_window_state(out/"early-viewer-after-render.json","accepted-after-render-before-teardown");
                blend_validation::text(out / "early-viewer-launch-outcome.json",
                    "{\"schema\":\"civis-early-viewer-launch-outcome-v1\",\"namedLaunchAcceptedCurrent\":true,"
                    "\"sameParentOwnedHWND\":true,\"full63\":false,\"nativePixelParity\":false}\n");
                if(bnw_early_lifetime_gates)
                    verify_prepared_viewer_lifetime(prepared_viewer,early_package,out);
                // Parent teardown is after named-launch final currentness and
                // its acceptance receipt. Never close inside initial callback.
                prepared_viewer.reset();
            } else render({},nullptr);
            write_ui_session_status();
            check(!smoke || panel.samples == (bnw_live_request ? 0U : 60U),
                  "TopPanel must use exactly one source per displayed frame");
            return 0;
        }
        if ((argc == 12 || argc == 13) && std::string(argv[10]) == "--material-precision-patches") {
            material_world_precision(context, root, input, shaders, argv[7], cubes, geometry, light, control,
                                     argv[11], argc == 13 ? fs::path(argv[12]) : fs::path{}, out);
            return 0;
        }
        if (argc == 11 && std::string_view(argv[10]) == "--material-loading-checks") {
            validate_material_world_loading(context, root, input, shaders, argv[7], cubes, geometry, light,
                                            plain_inputs, out);
            return 0;
        }
        if (argc == 11 && std::string(argv[10]) == "--water-world-checks") {
            const auto world = build_world(root, input, 0);
            const std::shared_ptr<const TerrainWorld> terrain(world, &world->terrain());
            const auto waves = civ5::assets::LeanData::parse(
                water_asset(root, "Resource/Common/TerrainModels.fpk", "waterbumps16.lean"));
            const auto color = civ5::assets::DdsData::parse(
                water_asset(root, "Resource/DX9/Upgrade1Textures.fpk", "waterdepthcolor_new.dds"));
            const auto checks = water_owner_checks(context, terrain, waves, color);
            execution << "PASS water world owner checks=" << checks << '\n';
            blend_validation::text(out / "water-owner-checks.txt", execution.str());
            std::cout << execution.str() << std::flush;
            return 0;
        }
        if (scheduled_materials) {
            const auto profile = material_preview_settings::discover();
            blend_validation::text(out / "material-settings.json",
                                   material_preview_settings::report(profile));
            const auto world = build_world(root, input, 0, nullptr, {}, {}, false, false,
                                           profile.texture_quality_override == 0);
            GlMaterialWorldRenderer renderer(
                context, world, cubes, {false, true, true}, LightingCubeSampling::measured_coordinates,
                LandShadowArithmetic::measured_fma_t_x_z_y, LandMipSampling::d3d9_rgb565, color_keys);
            const auto height = world->terrain().height_field().extract_a8_texture(0, 0);
            // Active diffuse roles 0,1,3,4 use temporary_size in the original texture plan.
            // Final and higher RGB565 targets are allocated per actual completed job.
            NativeWorld original(shaders, argv[7], world->assets(), cubes,
                                 profile.configured.material.quality.temporary_size, geometry, height,
                                 renderer.source_bindings(), argv[15], world->terrain().land_styles().masks,
                                 argv[16]);
            D3DDEVICE_CREATION_PARAMETERS creation{};
            hr(original.cache.device->GetCreationParameters(&creation), "Scheduled scene original device");
            check(creation.DeviceType == D3DDEVTYPE_HAL &&
                      (creation.BehaviorFlags & D3DCREATE_HARDWARE_VERTEXPROCESSING),
                  "Scheduled scene original shaders require HAL/HWVP");
            D3DADAPTER_IDENTIFIER9 adapter{};
            hr(original.cache.api->GetAdapterIdentifier(creation.AdapterOrdinal, 0, &adapter),
               "Scheduled scene original adapter");
            blend_validation::text(out / "gl-device.txt", context->description());
            blend_validation::text(out / "d3d9-device.txt",
                                   std::string(adapter.Description) + '\n' + adapter.Driver + "\nHAL/HWVP\n");
            original.distance_fog = std::make_unique<distance_fog_validation::NativeDistanceFog>(
                original.cache.device.value, argv[17]);
            plain_inputs.pixel[2] = distance_fog->ramp().id();
            ScheduledMaterialScene land(renderer, original, world, light, profile);
            const auto folder = out / "0";
            fs::create_directory(folder);
            write_field(world->terrain(), folder, "");
            render_water_scene(context, world, land, original, control, plain_inputs, overview_frame(), root,
                               argv[11], folder, true, true, false, argv[12], argv[13], argv[14], true, true,
                               true, false, true, true, distance_fog.get(), argv[18], argv[19], {}, {},
                               color_keys.get(), {}, river_shaders, river_reference, sequence_reference);
            blend_validation::text(
                out / "scheduled-materials.txt",
                "PASS real input/controller -> shared camera -> retained cache jobs -> "
                "LAND/water/river/FOW.\n"
                "Independent D3D9 shaders regenerate each actual cache job; no cache replay.\n"
                "Actual DX9 adapter and saved INI quality/pools/loading; original camera-event budget controls.\n"
                "Initial scene and full sequence start separate cold cache epochs.\n"
                "Arbitrary overlapping views, other cache modes, complete recovery and full "
                "graphics parity remain open.\n");
            return 0;
        }
        if (fow_replay) {
            const auto cache_argument = fow_animation_replay || fow_camera_replay ? 21 : 20;
            const fs::path previous(argv[cache_argument]);
            for (const char *name : {"original-distance-fog.txt", "original-terrain-lighting.txt",
                                     "authored-color-keys.txt", "terrain-cache-lighting.f32"})
                check(blend_validation::read(previous / name) == blend_validation::read(out / name),
                      "FOW diagnostic cache producer settings differ");
            const auto world = build_world(root, input, 0);
            CameraReplayLand land(context, world, cubes, previous, color_keys, land_sampling);
            const auto height = world->terrain().height_field().extract_a8_texture(0, 0);
            NativeWorld original(shaders, argv[7], world->assets(), cubes, 64, geometry, height,
                                 land.source.source_bindings(), argv[15],
                                 world->terrain().land_styles().masks, argv[16]);
            camera_replay_native_caches(original, *world, previous);
            original.distance_fog = std::make_unique<distance_fog_validation::NativeDistanceFog>(
                original.cache.device.value, argv[17]);
            plain_inputs.pixel[2] = distance_fog->ramp().id();
            const auto folder = out / "0";
            fs::create_directory(folder);
            if (!river_shaders.empty())
                write_field(world->terrain(), folder, "");
            render_water_scene(context, world, land, original, control, plain_inputs, overview_frame(), root,
                               argv[11], folder, true, true, false, argv[12], argv[13], argv[14], true, true,
                               true, false, true, true, distance_fog.get(), argv[18], argv[19],
                               argc == cache_argument + 2 ? fs::path(argv[cache_argument + 1]) : fs::path{},
                               fow_animation_reference, color_keys.get(), fow_camera_reference, river_shaders,
                               river_reference, sequence_reference);
            blend_validation::text(
                out / "diagnostic-replay.txt",
                "Diagnostic only: each API reuses its own prior cache bases.\n"
                "World fields, camera, overlay, FOW, LAND/water constants and GPU targets are rebuilt.\n"
                "This does not replace full cache-generation verification.\n");
            return 0;
        }
        if (argc == 16 && !color_scene) {
            const auto world = build_world(root, input, 0);
            const fs::path previous(argv[15]);
            CameraReplayLand land(context, world, cubes, previous);
            const auto height = world->terrain().height_field().extract_a8_texture(0, 0);
            NativeWorld original(shaders, argv[7], world->assets(), cubes, 64, geometry, height,
                                 land.source.source_bindings());
            camera_replay_native_caches(original, *world, previous);
            const auto folder = out / "0";
            fs::create_directory(folder);
            render_water_scene(context, world, land, original, control, plain_inputs, overview_frame(), root,
                               argv[11], folder, true, true, false, argv[12], argv[13], argv[14], true, true);
            blend_validation::text(out / "diagnostic-replay.txt",
                                   "Diagnostic only; each API replays its own prior "
                                   "cache bases. Full cache generation was "
                                   "skipped.\n"
                                   "Source fields, camera, water geometry, constants "
                                   "and GPU textures are rebuilt.\n"
                                   "Prior cache folder: " +
                                       previous.string() + "\n");
            std::cout << "PASS diagnostic camera raster replay\n";
            return 0;
        }
        if (argc == 11 && std::string_view(argv[10]) == "--marsh-refresh") {
            validate_material_world_refresh(context, root, input, shaders, argv[7], cubes, geometry,
                                            light, plain_inputs, out);
            return 0;
        }
        if (argc == 11) {
            // Diagnostic only. Source fields are rebuilt from primitive inputs;
            // stored native cache bases isolate the LAND sampler. This branch never
            // supplies expected cache output to GlMaterialWorldRenderer::prepare.
            auto world = build_world(root, input, 0);
            GlMaterialWorldRenderer owner(context, world, cubes, {false, true, true});
            const auto bytes = input_bytes(fs::path(argv[10]) / "0/scene-cache-bases-d3d9.bin");
            WorldReader cache_reader{bytes};
            std::vector<GlTexture> textures;
            for (std::size_t index = 0; index < world->cache().patches.size(); ++index) {
                const auto base = cache_reader.blob();
                check(base.size() == 8192, "Replay base64 extent");
                auto texture = GlTexture::render_target(*context, 64, 64, 7, RenderTextureFormat::rgb565);
                context->api().TextureSubImage2D(texture.id(), 0, 0, 0, 64, 64,
                                                 texture.plan().transfer_format, texture.plan().transfer_type,
                                                 base.data());
                const auto copied = texture.read_mip_raw(0);
                check(std::equal(copied.begin(), copied.end(), base.begin(), base.end()),
                      "Replay native base transfer");
                texture.generate_mip_chain();
                textures.push_back(std::move(texture));
            }
            cache_reader.end();
            SceneTarget target(context, 1280, 960);
            const auto view = overview_frame();
            for (auto profile : {LandMipSampling::ordinary, LandMipSampling::d3d9_trilinear}) {
                std::map<std::pair<int, int>, std::unique_ptr<GlLandMaterialRenderer>> renderers;
                target.begin();
                for (std::size_t index = 0; index < world->cache().patches.size(); ++index) {
                    const auto p = world->terrain().land().patch(index / world->cache().rows,
                                                                 index % world->cache().rows);
                    const auto key = std::pair{p.source_samples_x(), p.source_samples_y()};
                    if (!renderers.contains(key)) {
                        const LandGrid grid(key.first, key.second, 1);
                        const LandMaterialMesh mesh(LandVertexInput::local_grid, grid.vertices(),
                                                    grid.indices());
                        renderers.emplace(
                            key, std::make_unique<GlLandMaterialRenderer>(
                                     context, mesh, 1, LandShadowArithmetic::measured_fma_t_x_z_y, profile));
                    }
                    auto images = plain_inputs;
                    images.pixel[0] = textures[index].id();
                    images.height = owner.height_texture(index).id();
                    renderers.at(key)->draw(owner.land_constants(index, view), images, 1280, 960);
                }
                write(out / (profile == LandMipSampling::ordinary ? "replay-ordinary.f32"
                                                                  : "replay-d3d9-trilinear.f32"),
                      target.pixels());
            }
            std::cout << "PASS diagnostic LAND replay from stored native bases\n";
            return 0;
        }
        for (unsigned wi = 0; wi < 2; ++wi) {
            auto world = build_world(root, input, wi);
            const auto folder = out / std::to_string(wi);
            fs::create_directory(folder);
            std::weak_ptr<const MaterialWorld> retained_world = world;
            {
                write_field(world->terrain(), folder, "");
                Writer layout;
                cache_layout(layout, world->cache());
                save(folder / "cache-layout.bin", layout.bytes);
                Writer styles;
                for (auto v : world->terrain().land_styles().masks)
                    styles.word(v);
                save(folder / "style-masks.bin", styles.bytes);
                GlMaterialWorldRenderer renderer(
                    context, world, cubes, {false, true, true}, LightingCubeSampling::measured_coordinates,
                    LandShadowArithmetic::measured_fma_t_x_z_y, land_sampling, color_keys);
                const auto field = world->terrain().height_field();
                std::map<unsigned, Bytes> windows;
                for (int x = 0; x < field.domain().texture_columns(); ++x)
                    for (int y = 0; y < field.domain().texture_rows(); ++y) {
                        auto bytes = field.extract_a8_texture(x, y);
                        save(folder / ("height-window-" + std::to_string(x + 16 * y) + ".bin"), bytes);
                        windows.emplace(x + 16 * y, std::move(bytes));
                    }
                std::map<unsigned, std::unique_ptr<NativeWorld>> native;
                const auto original = [&](unsigned size) -> NativeWorld & {
                    auto found = native.find(size);
                    if (found == native.end())
                        found =
                            native
                                .emplace(size, std::make_unique<NativeWorld>(
                                                   shaders, argv[7], world->assets(), cubes, size, geometry,
                                                   windows.begin()->second, renderer.source_bindings(),
                                                   color_scene ? fs::path(argv[15]) : fs::path{},
                                                   world->terrain().land_styles().masks,
                                                   lighting_scene ? fs::path(argv[16]) : fs::path{}))
                                .first;
                    return *found->second;
                };
                auto &requests = input.materials[wi].patches;
                for (unsigned ordinal = 0; ordinal < requests.size(); ++ordinal) {
                    auto &q = requests[ordinal];
                    auto settings = q.settings;
                    settings.cache_mode = settings.parent_flag = 0;
                    auto job = renderer.prepare(
                        q.index, settings, [&](int, int, float value) { return q.scores.take(value); },
                        [&](int, int, float value) { return q.vertices.take(value); }, light, 4, true);
                    q.scores.end();
                    q.vertices.end();
                    const auto path = folder / ("patch-" + std::to_string(ordinal));
                    fs::create_directory(path);
                    output_patch(job.patch, path);
                    blend_validation::write(path / "plan.bin", serialize_plan(job.plan));
                    blend_validation::write(path / "coefficients.f32", job.frame.material_coefficients);
                    blend_validation::write(path / "final-gl.bin",
                                            renderer.final_texture(q.index).read_mip_raw(0));
                    auto &n = original(settings.cache_size);
                    n.execute(q.index, job, windows.at(job.height_table_slot));
                    blend_validation::write(path / "final-d3d9.bin", n.cache.pixels(n.final_id, 0));
                    for (unsigned slot : {0u, 1u, 3u, 4u}) {
                        blend_validation::write(
                            path / ("temporary-" + std::to_string(slot) + "-gl.bin"),
                            renderer
                                .temporary_texture(settings.cache_size, static_cast<MaterialCacheRole>(slot))
                                .read_mip_raw(0));
                        blend_validation::write(path / ("temporary-" + std::to_string(slot) + "-d3d9.bin"),
                                                n.cache.pixels(renderer.source_bindings().textures[slot], 0));
                    }
                    const auto &land_layout = world->terrain().land();
                    const auto p =
                        land_layout.patch(q.index / land_layout.rows(), q.index % land_layout.rows());
                    const LandGrid grid(p.source_samples_x(), p.source_samples_y(), 1);
                    const LandMaterialMesh mesh(LandVertexInput::local_grid, grid.vertices(), grid.indices());
                    for (unsigned variant : {0u, 1u}) {
                        const auto f = patch_frame(world->terrain(), q.index, variant);
                        const auto constants = renderer.land_constants(q.index, f);
                        control.attach({control.targets[6]});
                        control.gl.Viewport(0, 0, 64, 64);
                        for (auto flag : {GL_DEPTH_TEST, GL_BLEND, GL_CULL_FACE, GL_SCISSOR_TEST, GL_DITHER,
                                          GL_FRAMEBUFFER_SRGB, GL_RASTERIZER_DISCARD})
                            control.gl.Disable(flag);
                        renderer.draw(q.index, f, variant ? varied_inputs : plain_inputs, 64, 64);
                        blend_validation::write(path / ("land-" + std::to_string(variant) + "-gl.f32"),
                                                control.pixels(control.targets[6]));
                        n.land.begin_target();
                        const auto pixels = n.land.draw(mesh, 1, constants, n.images(q.index, variant != 0),
                                                        n.heights.at(job.height_table_slot).value);
                        blend_validation::write(path / ("land-" + std::to_string(variant) + "-d3d9.f32"),
                                                pixels);
                        save(path / ("land-" + std::to_string(variant) + "-constants.bin"),
                             frame_bytes(constants));
                    }
                    blend_validation::write(
                        path / "land-feedback.f32",
                        renderer.capture(q.index, patch_frame(world->terrain(), q.index, 0), false));
                    // Preserve both direct paths before replacing the GL cache in this
                    // control.
                    const auto original_base = n.final_pixels(q.index);
                    const auto &cache_texture = renderer.final_texture(q.index);
                    context->api().TextureSubImage2D(
                        cache_texture.id(), 0, 0, 0, settings.cache_size, settings.cache_size,
                        cache_texture.plan().transfer_format, cache_texture.plan().transfer_type,
                        original_base.data());
                    check(cache_texture.read_mip_raw(0) == original_base,
                          "Identical patch cache base upload");
                    cache_texture.generate_mip_chain();
                    for (unsigned variant : {0u, 1u}) {
                        control.attach({control.targets[6]});
                        renderer.draw(q.index, patch_frame(world->terrain(), q.index, variant),
                                      variant ? varied_inputs : plain_inputs, 64, 64);
                        blend_validation::write(
                            path / ("land-" + std::to_string(variant) + "-same-cache-gl.f32"),
                            control.pixels(control.targets[6]));
                    }
                    execution << "PASS world=" << wi << "; patch=" << q.index
                              << "; size=" << settings.cache_size << "; slot=" << job.height_table_slot
                              << '\n';
                    std::cout << "PASS world " << wi << " connected patch " << q.index << '\n' << std::flush;
                }
                const auto host_normal = [](int, int, float squared) {
                    return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(squared)));
                };
                unsigned guards = 0;
                const auto old_id = renderer.final_texture(0).id();
                const auto old_final = renderer.final_texture(0).read_mip_raw(0),
                           old_height = renderer.height_texture(0).read_mip_raw(0);
                const auto reject = [&](std::size_t index, MaterialPatchSettings settings,
                                        GlMaterialWorldLighting lighting, unsigned samples,
                                        MaterialNormalSeed score = {}, MaterialNormalSeed vertex = {}) {
                    bool caught = false;
                    try {
                        renderer.prepare(index, settings, score, vertex, lighting, samples, true);
                    } catch (const std::invalid_argument &) {
                        caught = true;
                    }
                    if (!caught || renderer.final_texture(0).id() != old_id || renderer.prepared(1))
                        std::cerr << "World guard " << guards << ": caught=" << caught
                                  << "; unchanged-id=" << (renderer.final_texture(0).id() == old_id)
                                  << "; fresh-published=" << renderer.prepared(1) << '\n';
                    check(caught && renderer.prepared(0) && !renderer.prepared(1) &&
                              renderer.final_texture(0).id() == old_id &&
                              renderer.final_texture(0).read_mip_raw(0) == old_final &&
                              renderer.height_texture(0).read_mip_raw(0) == old_height,
                          "Rejected world cache job changed published cache or source "
                          "height");
                    ++guards;
                };
                const MaterialPatchSettings valid_settings{64, 0, 0, 0, X87Precision::double_precision};
                for (unsigned kind = 0; kind < 4; ++kind) {
                    auto bad = valid_settings;
                    if (kind == 0)
                        bad.cache_size = 7;
                    if (kind == 1)
                        bad.cache_mode = 1;
                    if (kind == 2)
                        bad.parent_flag = 1;
                    if (kind == 3)
                        bad.precision = static_cast<X87Precision>(99);
                    reject(0, bad, light, 4, host_normal, host_normal);
                }
                reject(world->cache().patches.size(), valid_settings, light, 4, host_normal, host_normal);
                reject(0, valid_settings, light, 33, host_normal, host_normal);
                reject(0, valid_settings, light, 4, {}, host_normal);
                reject(0, valid_settings, light, 4, host_normal, {});
                reject(
                    0, valid_settings, light, 4,
                    [](int, int, float) { return std::numeric_limits<float>::quiet_NaN(); }, host_normal);
                auto bad_light = light;
                bad_light.sun_direction[0] = std::numeric_limits<float>::quiet_NaN();
                reject(0, valid_settings, bad_light, 4, host_normal, host_normal);
                bad_light = light;
                bad_light.color_correct[0] = renderer.final_texture(0).id();
                reject(0, valid_settings, bad_light, 4, host_normal, host_normal);
                reject(1, valid_settings, bad_light, 4, host_normal, host_normal);
                execution << "PASS owner-preflight-guards=" << guards << "; world=" << wi << '\n';
                // One full source-world view, using this host's explicit RSQRTSS
                // estimates for all additional patches. Selected controls above
                // separately retain the original instruction observations.
                if (wi == 0) {
                    const auto host_seed = [](int, int, float squared) {
                        return _mm_cvtss_f32(_mm_rsqrt_ss(_mm_set_ss(squared)));
                    };
                    auto view = overview_frame();
                    auto scene_inputs = plain_inputs;
                    Writer edges;
                    auto &n = original(64);
                    for (std::size_t index = 0; index < world->cache().patches.size(); ++index) {
                        auto job = renderer.prepare(index, {64, 0, 0, 0, X87Precision::double_precision},
                                                    host_seed, host_seed, light, 4, true);
                        n.execute(index, job, windows.at(job.height_table_slot));
                        const auto p = world->terrain().land().patch(index / world->cache().rows,
                                                                     index % world->cache().rows);
                        const auto feedback = renderer.capture(index, view, false);
                        save_edges(edges, index, p.source_samples_x(), p.source_samples_y(), feedback);
                        if (index % 56 == 0)
                            std::cout << "prepared complete world patch " << index << '/'
                                      << world->cache().patches.size() << '\n'
                                      << std::flush;
                    }
                    save(folder / "scene-feedback-edges.bin", edges.bytes);
                    if (distance_fog) {
                        n.distance_fog = std::make_unique<distance_fog_validation::NativeDistanceFog>(
                            n.cache.device.value, fs::path(argv[17]));
                        const auto &dds = distance_fog->assets().ramp();
                        check(n.distance_fog->ramp->GetLevelCount() == dds.mip_count(), "D3DX fog mip count");
                        for (unsigned mip = 0; mip < dds.mip_count(); ++mip) {
                            const auto pixels = n.distance_fog->level(mip);
                            check(pixels == distance_fog->ramp().read_mip_raw(mip),
                                  "Original D3DX/GL fog mip mismatch");
                            save(out / "distance-fog" / ("ramp-mip" + std::to_string(mip) + "-d3dx.bin"),
                                 pixels);
                        }
                        scene_inputs.pixel[2] = distance_fog->ramp().id();
                        view = distance_fog->land_constants(view, 0, X87Precision::double_precision);
                        check(view.fog_color == n.distance_fog->overview[0] &&
                                  view.fog == n.distance_fog->overview[1],
                              "Original overview fog constants differ");
                    }
                    SceneTarget target(context, 1280, 960);
                    target.begin();
                    for (std::size_t index = 0; index < world->cache().patches.size(); ++index)
                        renderer.draw(index, view, scene_inputs, 1280, 960);
                    blend_validation::write(folder / "scene-gl.f32", target.pixels());
                    NativeSceneTarget native_target(n, 1280, 960);
                    native_target.begin();
                    for (std::size_t index = 0; index < world->cache().patches.size(); ++index) {
                        const auto p = world->terrain().land().patch(index / world->cache().rows,
                                                                     index % world->cache().rows);
                        const LandGrid grid(p.source_samples_x(), p.source_samples_y(), 1);
                        const LandMaterialMesh mesh(LandVertexInput::local_grid, grid.vertices(),
                                                    grid.indices());
                        auto c = renderer.land_constants(index, view);
                        if (n.distance_fog) {
                            c.fog_color = n.distance_fog->overview[0];
                            c.fog = n.distance_fog->overview[1];
                        }
                        n.land.bind(mesh, 1, c, n.images(index, false),
                                    n.heights.at(p.texture_table_slot()).value);
                        n.land.submit(mesh);
                    }
                    blend_validation::write(folder / "scene-d3d9.f32", native_target.pixels());
                    if (argc == 12 || argc == 14 || argc == 15 || color_scene)
                        render_water_scene(
                            context, world, renderer, n, control, scene_inputs, view, root, argv[11], folder,
                            std::string(argv[10]) != "--water-scene",
                            std::string(argv[10]) == "--water-defaults-scene" ||
                                std::string(argv[10]) == "--water-animation-scene" || argc >= 14,
                            std::string(argv[10]) == "--water-animation-scene" || argc >= 14,
                            argc >= 14 ? fs::path(argv[12]) : fs::path{},
                            argc >= 14 ? fs::path(argv[13]) : fs::path{},
                            argc >= 15 ? fs::path(argv[14]) : fs::path{},
                            std::string(argv[10]) == "--water-frame-scene" ||
                                std::string(argv[10]) == "--water-camera-scene" ||
                                std::string(argv[10]) == "--water-startup-scene" ||
                                std::string(argv[10]) == "--water-motion-scene" ||
                                std::string(argv[10]) == "--water-input-scene" || color_scene,
                            std::string(argv[10]) == "--water-camera-scene" ||
                                std::string(argv[10]) == "--water-startup-scene" ||
                                std::string(argv[10]) == "--water-motion-scene" ||
                                std::string(argv[10]) == "--water-input-scene" || color_scene,
                            std::string(argv[10]) == "--water-startup-scene" ||
                                std::string(argv[10]) == "--water-motion-scene" ||
                                std::string(argv[10]) == "--water-input-scene" || color_scene,
                            std::string(argv[10]) == "--water-motion-scene",
                            std::string(argv[10]) == "--water-input-scene" || color_scene, lighting_scene,
                            distance_fog.get(), fow_scene ? fs::path(argv[18]) : fs::path{},
                            fow_scene ? fs::path(argv[19]) : fs::path{}, {}, fow_animation_reference,
                            color_keys.get(), fow_camera_reference, river_shaders, river_reference,
                            sequence_reference);
                    // This isolation runs only after both complete direct images.
                    // Original final bases replace GL caches; each API keeps its
                    // actual automatic mip generation and full LAND shader path.
                    Writer gl_bases, native_bases, direct_mips;
                    for (std::size_t index = 0; index < world->cache().patches.size(); ++index) {
                        const auto &texture = renderer.final_texture(index);
                        gl_bases.blob(texture.read_mip_raw(0));
                        for (unsigned mip = 0; mip < 7; ++mip)
                            direct_mips.blob(texture.read_mip_raw(mip));
                        const auto bytes = n.final_pixels(index);
                        native_bases.blob(bytes);
                        context->api().TextureSubImage2D(texture.id(), 0, 0, 0, 64, 64,
                                                         texture.plan().transfer_format,
                                                         texture.plan().transfer_type, bytes.data());
                        require(texture.read_mip_raw(0) == bytes, "World identical cache base upload");
                        texture.generate_mip_chain();
                    }
                    save(folder / "scene-cache-bases-gl.bin", gl_bases.bytes);
                    save(folder / "scene-cache-bases-d3d9.bin", native_bases.bytes);
                    save(folder / "scene-direct-gl-mips.bin", direct_mips.bytes);
                    target.begin();
                    for (std::size_t index = 0; index < world->cache().patches.size(); ++index)
                        renderer.draw(index, view, scene_inputs, 1280, 960);
                    blend_validation::write(folder / "scene-same-cache-gl.f32", target.pixels());
                    target.begin();
                    draw_ordinary_mip_control(context, *world, renderer, view, scene_inputs);
                    blend_validation::write(folder / "scene-ordinary-mip-negative.f32", target.pixels());
                    // Separate base-only sampling control. Both APIs receive the same
                    // native RGB565 bases; direct full-mip scene results above remain
                    // intact.
                    for (std::size_t index = 0; index < world->cache().patches.size(); ++index)
                        context->api().TextureParameteri(renderer.final_texture(index).id(),
                                                         GL_TEXTURE_MAX_LEVEL, 0);
                    target.begin();
                    for (std::size_t index = 0; index < world->cache().patches.size(); ++index)
                        renderer.draw(index, view, scene_inputs, 1280, 960);
                    blend_validation::write(folder / "scene-base-only-gl.f32", target.pixels());
                    native_target.begin();
                    hr(n.cache.device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE),
                       "Native base-only LAND control");
                    for (std::size_t index = 0; index < world->cache().patches.size(); ++index) {
                        const auto p = world->terrain().land().patch(index / world->cache().rows,
                                                                     index % world->cache().rows);
                        const LandGrid grid(p.source_samples_x(), p.source_samples_y(), 1);
                        const LandMaterialMesh mesh(LandVertexInput::local_grid, grid.vertices(),
                                                    grid.indices());
                        n.land.bind(mesh, 1, renderer.land_constants(index, view), n.images(index, false),
                                    n.heights.at(p.texture_table_slot()).value);
                        n.land.submit(mesh);
                        context->api().TextureParameteri(renderer.final_texture(index).id(),
                                                         GL_TEXTURE_MAX_LEVEL, 6);
                    }
                    blend_validation::write(folder / "scene-base-only-d3d9.f32", native_target.pixels());
                    mip_validation::NativeMipReader mip_reader(n.cache.device.value);
                    Writer native_mips, gl_mips;
                    for (std::size_t index = 0; index < world->cache().patches.size(); ++index) {
                        auto *texture = n.finals.at(static_cast<unsigned>(index)).value;
                        check(texture->GetAutoGenFilterType() == D3DTEXF_LINEAR,
                              "Original automatic mip filter");
                        for (unsigned mip = 0; mip < 7; ++mip) {
                            const auto values = mip_reader.read(texture, mip);
                            native_mips.blob(std::span(reinterpret_cast<const std::uint8_t *>(values.data()),
                                                       values.size() * 4));
                            gl_mips.blob(renderer.final_texture(index).read_mip_raw(mip));
                        }
                    }
                    save(folder / "scene-native-mip-texels.f32", native_mips.bytes);
                    save(folder / "scene-gl-same-base-mips.bin", gl_mips.bytes);
                    save(folder / "scene-frame.bin", frame_bytes(view));
                    execution << "PASS complete-world-patches=" << world->cache().patches.size() << '\n';
                }
                const auto probe = renderer.capture(0, patch_frame(world->terrain(), 0, 0), false);
                const auto owner_frame = patch_frame(world->terrain(), 0, 0);
                world.reset();
                check(!retained_world.expired() && renderer.capture(0, owner_frame, false) == probe &&
                          renderer.prepared(0),
                      "Renderer lost world/height/cache after caller destruction");
                execution << "PASS world-retained-after-caller-reset=" << wi << '\n';
            }
            check(retained_world.expired(), "Renderer leaked its immutable world owner");
            execution << "PASS world-released-with-renderer=" << wi << '\n';
        }
        blend_validation::text(out / "execution.txt", execution.str());
        std::cout << "PASS owned material worlds through cache and LAND\n";
    } catch (const std::exception &e) {
        if (argc > 9) {
            std::ofstream failure(fs::path(argv[9]) / "failure.txt");
            failure << e.what() << '\n';
        }
        std::cerr << "FAIL " << e.what() << '\n';
        return 1;
    }
}
