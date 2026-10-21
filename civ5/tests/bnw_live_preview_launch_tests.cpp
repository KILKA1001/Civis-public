#include "civ5/ui/bnw_live_preview_launch.hpp"
#include "civ5/ui/world_view.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_game_activation_execution.hpp"
#include "civ5/saves/loaded_save.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <algorithm>
#include <filesystem>
#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using namespace civ5;

namespace {
std::optional<ui::BnwLivePreviewRequest> parse(std::vector<std::string> values,
                                                std::vector<std::string> *remaining = nullptr) {
    std::vector<char *> argv;
    for (auto &value : values) argv.push_back(value.data());
    int argc = static_cast<int>(argv.size());
    auto parsed = ui::consume_bnw_live_preview_options(argc, argv.data());
    if (remaining)
        for (int i = 0; i < argc; ++i) remaining->push_back(argv[static_cast<std::size_t>(i)]);
    return parsed;
}
void rejects(std::vector<std::string> values) {
    bool rejected = false;
    try { (void)parse(std::move(values)); }
    catch (const std::invalid_argument &) { rejected = true; }
    assert(rejected);
}
} // namespace

int main(int argc, char **argv) {
    assert(argc == 4 || argc == 5);
    const std::filesystem::path save(argv[1]), game_root(argv[2]), split_root(argv[3]);
    std::vector<std::string> remaining;
    assert(!parse({"preview", "--camera-preview", "scene"}, &remaining));
    assert((remaining == std::vector<std::string>{"preview", "--camera-preview", "scene"}));
    remaining.clear();
    const auto options = parse({"preview", "--camera-preview", "--bnw-live-save",
        "chosen.Civ5Save", "--bnw-game-root", "C:/game with spaces",
        "--bnw-active-player", "0", "scene"}, &remaining);
    assert(options && options->save_path == "chosen.Civ5Save" &&
           options->game_root == "C:/game with spaces" &&
           options->active_player_id == 0 &&
           (remaining == std::vector<std::string>{"preview", "--camera-preview", "scene"}));
    const auto civis_options = parse({"preview", "--bnw-live-save", "chosen.CivisSave",
        "--bnw-game-root", "C:/game", "--bnw-active-player", "0"});
    assert(civis_options && civis_options->save_path == "chosen.CivisSave");
    remaining.clear();
    const auto g0_options = parse({"preview", "--bnw-live-save", "chosen.Civ5Save",
        "--bnw-game-root", "C:/game", "--bnw-active-player", "0",
        "--bnw-g0-selected-unit-receipt-root", "C:/receipt", "scene"}, &remaining);
    assert(g0_options && g0_options->g0_selected_unit_receipt_root ==
               std::filesystem::path("C:/receipt") &&
           (remaining == std::vector<std::string>{"preview", "scene"}));
    rejects({"preview", "--bnw-live-save", "chosen.Civ5Save"});
    rejects({"preview", "--bnw-g0-selected-unit-receipt-root", "C:/receipt"});
    rejects({"preview", "--bnw-live-save", "x", "--bnw-game-root", "root",
             "--bnw-active-player", "0", "--bnw-g0-selected-unit-receipt-root", "a",
             "--bnw-g0-selected-unit-receipt-root", "b"});
    rejects({"preview", "--bnw-live-save", "x", "--bnw-game-root", "root",
             "--bnw-active-player", "-1"});
    rejects({"preview", "--bnw-live-save", "x", "--bnw-live-save", "y",
             "--bnw-game-root", "root", "--bnw-active-player", "0"});

    ui::BnwLivePreviewRequest request{save, game_root, 0, std::nullopt, {}};
    int renders = 0;
    bool expect_g0_selection = false;
    const auto render = [&](const ui::LiveBnwPreviewInput *input) {
        ++renders;
        assert(input && input->active_player_id == 0);
        assert(input->scene_identity && input->activation_owner &&
               input->activation_owner->identity.payload_sha256 == input->scene_identity->payload_sha256 &&
               input->activation_owner->identity.rules_sha256 == input->scene_identity->rules_sha256 &&
               !input->activation_owner->deferred_fog && input->activation_owner->pending.empty());
        auto foreign = *input;
        runtime::BnwGameActivationOwner wrong_owner;
        foreign.activation_owner = &wrong_owner;
        bool wrong_rejected = false;
        try { (void)ui::bind_live_bnw_preview(std::move(foreign)); }
        catch (const std::invalid_argument &) { wrong_rejected = true; }
        assert(wrong_rejected);
        auto binding = ui::bind_live_bnw_preview(*input);
        const auto frame = binding.frames();
        const auto *world = frame.find<ui::WorldViewState>();
        assert(frame.active_player_id() == 0 && frame.turn() == 0 && world &&
               !world->end_turn_available && !input->session.readiness().can_continue());
        if (expect_g0_selection) {
            assert(world->selected_unit && world->selected_unit->id == 16385 &&
                   world->selected_unit->name == "Воины" &&
                   world->selected_unit->movement == "2/2 [ICON_MOVES]" &&
                   world->selected_unit->strength == "8 [ICON_STRENGTH]" &&
                   world->selected_unit->stat_boxes &&
                   world->selected_unit->stat_boxes->xp &&
                   world->selected_unit->stat_boxes->movement &&
                   world->selected_unit->stat_boxes->strength &&
                   !world->selected_unit->stat_boxes->ranged &&
                   world->selected_unit->actions.empty());
            auto base = std::make_shared<assets::LooseResourceIndex>(game_root);
            auto expansion = std::make_shared<assets::LooseResourceIndex>(game_root);
            auto bnw = std::make_shared<assets::LooseResourceIndex>(game_root);
            base->scan({game_root / "Assets/UI/InGame", {}, false});
            expansion->scan({game_root / "Assets/DLC/Expansion/UI/InGame", {}, false});
            bnw->scan({game_root / "Assets/DLC/Expansion2/UI/InGame", {}, false});
            assets::ResourceMountTable resources(assets::resource_lookup(base));
            resources.mount({1, 0, 0, assets::resource_lookup(expansion), true, true, {}, {}});
            resources.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {}, {}});
            const auto registry = ui::UiContextRegistry::from_resources(resources);
            const auto skin = ui::WorldViewSkin::from_resources(resources, registry);
            const auto plan = ui::plan_world_view(skin, registry, frame, 1274, 925,
                                                  binding.backend.get());
            assert(std::any_of(plan.paints.begin(), plan.paints.end(),
                [&](const ui::WorldViewPaint &paint) {
                    return paint.kind == ui::WorldViewPaintKind::text &&
                        paint.text == "ВОИНЫ"; // Actual selected RU UnitPanel.lua uses Locale.ToUpper.
                }));
            const auto authored_stat_frame = [&](int x, int y, int width, int height) {
                return std::count_if(plan.paints.begin(), plan.paints.end(),
                    [&](const ui::WorldViewPaint &paint) {
                        return paint.kind == ui::WorldViewPaintKind::rectangle &&
                            paint.style == "Grid9FrameTurns" &&
                            paint.destination.x == x && paint.destination.y == y &&
                            paint.destination.width == width &&
                            paint.destination.height == height;
                    }) == 1;
            };
            assert(authored_stat_frame(217, 827, 275, 12) &&
                   authored_stat_frame(232, 839, 260, 27) &&
                   authored_stat_frame(232, 866, 260, 27) &&
                   !authored_stat_frame(232, 893, 260, 27));
            assert(std::none_of(plan.hits.begin(), plan.hits.end(),
                [](const ui::WorldViewHit &hit) {
                    const auto *command = std::get_if<ui::UiGameplayCommand>(&hit.command);
                    const auto *action = command ? command->as<ui::WorldViewAction>() : nullptr;
                    return action && action->kind == ui::WorldViewActionKind::unit_action;
                }));
        } else {
            assert(!world->selected_unit);
        }
    };
    auto result = ui::launch_selected_bnw_camera_preview(request, {}, render);
    assert(!result.rendered && result.blocker &&
           result.blocker->stage == ui::SelectedBnwPreviewStage::selected_save && renders == 0);
    result = ui::launch_selected_bnw_camera_preview(request,
        game_root / "CivilizationV.exe", render);
    assert(!result.rendered && result.blocker &&
           result.blocker->reason.find("differs") != std::string::npos && renders == 0);
    // argv[3] can now be the complete canonical asset installation. Use a
    // genuinely absent child, without writing/removing any installation files.
    const auto missing_root=split_root/("civis-launch-missing-installation-"+
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    std::error_code missing_error;
    const bool missing_exists=std::filesystem::exists(missing_root,missing_error);
    assert(!missing_error && !missing_exists);
    request.game_root = missing_root;
    result = ui::launch_selected_bnw_camera_preview(request, save, render);
    assert(!result.rendered && result.blocker &&
           result.blocker->stage == ui::SelectedBnwPreviewStage::content_catalog && renders == 0);
    request.game_root = game_root;
    unsigned observed_profiles = 0;
    request.observe_selected_profile = [&](const ui::SelectedBnwBootstrapResult &b) {
        assert(b.preview.host && !b.preview.blocker && b.activation && b.postprocess &&
               b.activation->rules_database_ready && b.postprocess->all_succeeded &&
               b.postprocess->transaction_released);
        assert(b.preview.host->identity().payload_sha256 ==
               runtime::identify_bytes(b.preview.host->session().source().source.bytes).sha256);
        ++observed_profiles;
    };
    result = ui::launch_selected_bnw_camera_preview(request, save, render);
    assert(observed_profiles == 1);
    request.observe_selected_profile = {};
    assert(result.rendered && !result.blocker && renders == 1 &&
           !result.readiness.can_continue());

    if (argc == 5) {
        request.g0_selected_unit_receipt_root = argv[4];
        auto bad_receipt = request;
        bad_receipt.g0_selected_unit_receipt_root =
            game_root / "absent-G0-receipt";
        result = ui::launch_selected_bnw_camera_preview(bad_receipt, save, render);
        assert(!result.rendered && result.blocker && renders == 1 &&
               result.blocker->reason.find("receipt") != std::string::npos);
        auto wrong_player = request;
        wrong_player.active_player_id = 1;
        result = ui::launch_selected_bnw_camera_preview(wrong_player, save, render);
        assert(!result.rendered && result.blocker && renders == 1 &&
               result.blocker->reason.find("player zero") != std::string::npos);
        expect_g0_selection = true;
        result = ui::launch_selected_bnw_camera_preview(request, save, render);
        if (result.blocker)
            std::cerr << "G0 diagnostic blocker: " << result.blocker->reason << '\n';
        assert(result.rendered && !result.blocker && renders == 2 &&
               !result.readiness.can_continue());
        expect_g0_selection = false;
        request.g0_selected_unit_receipt_root.reset();
    }

    const auto imported = std::filesystem::temp_directory_path() /
        ("civis-bnw-launch-" + std::to_string(
            std::chrono::steady_clock::now().time_since_epoch().count()) + ".CivisSave");
    const auto source = saves::read_loaded_save(save);
    auto document = saves::make_civis_import_document(
        source, runtime::identify_file(save).sha256);
    saves::write_civis_save(imported, document);
    request.save_path = imported;
    result = ui::launch_selected_bnw_camera_preview(request, save, render);
    const int ordinary_renders = argc == 5 ? 3 : 2;
    assert(result.rendered && !result.blocker && renders == ordinary_renders &&
           !result.readiness.can_continue());

    for (const auto id : {saves::CivisSaveSectionId::live_bnw_map_plots,
                          saves::CivisSaveSectionId::live_bnw_exploration_dirty,
                          saves::CivisSaveSectionId::live_bnw_player_strategy,
                          saves::CivisSaveSectionId::live_bnw_player_core,
                          saves::CivisSaveSectionId::live_bnw_city_core,
                          saves::CivisSaveSectionId::live_bnw_city_production,
                          saves::CivisSaveSectionId::live_bnw_city_management,
                          saves::CivisSaveSectionId::live_bnw_city_tail}) {
        auto overlay = document;
        overlay.sections.push_back({static_cast<std::uint32_t>(id), 0, {1}});
        const auto overlaid = imported.string() + ".CivisSave";
        saves::write_civis_save(overlaid, overlay);
        request.save_path = overlaid;
        result = ui::launch_selected_bnw_camera_preview(request, save, render);
        assert(!result.rendered && result.blocker && renders == ordinary_renders &&
               result.blocker->reason.find("overlays require") != std::string::npos);
        std::filesystem::remove(overlaid);
    }
    std::filesystem::remove(imported);
    std::cout << "selected BNW read-only camera launch adapter PASS\n";
}
