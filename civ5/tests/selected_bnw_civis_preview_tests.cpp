#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_live_rng_export.hpp"
#include "civ5/runtime/bnw_ui_transient_checkpoint.hpp"
#include "civ5/ui/bnw_client_frame.hpp"
#include "bnw_selected_city_test_locale.hpp"
#include <iostream>
#include "civ5/runtime/bnw_game_activation_execution.hpp"
#include "civ5/runtime/simulation_random_checkpoint.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <stdexcept>

namespace {
namespace fs = std::filesystem;

void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

struct TemporaryDirectory {
    fs::path path;
    ~TemporaryDirectory() { std::error_code ignored; fs::remove_all(path, ignored); }
};

void run(const fs::path &source, const fs::path &game_root) {
    const TemporaryDirectory temporary{fs::temp_directory_path() /
        ("civis-bnw-preview-" + std::to_string(
             std::chrono::steady_clock::now().time_since_epoch().count()))};
    fs::create_directories(temporary.path);
    const auto loaded = civ5::saves::read_loaded_save(source);
    const auto original_hash = civ5::runtime::identify_file(source).sha256;
    auto document = civ5::saves::make_civis_import_document(loaded, original_hash);
    const auto imported = temporary.path / "imported.CivisSave";
    civ5::saves::write_civis_save(imported, document);

    auto original = civ5::ui::bootstrap_selected_bnw_preview(source, game_root, 0, true);
    check(static_cast<bool>(original.preview) && original.original_source_file_verified,
          "Selected original BNW fixture could not be restored");
    auto restored = civ5::ui::bootstrap_selected_bnw_preview(imported, game_root, 0);
    check(static_cast<bool>(restored.preview) &&
          !restored.original_source_file_verified &&
          restored.preview.host->session().source().source.container ==
              civ5::saves::SaveContainerKind::civis,
          "Imported Civis source must open read-only without claiming external source-file identity");
    check(restored.preview.host->identity().payload_sha256 ==
              original.preview.host->identity().payload_sha256 &&
          restored.preview.host->identity().rules_sha256 ==
              original.preview.host->identity().rules_sha256 &&
          restored.preview.host->session().state().setup.game_turn ==
              original.preview.host->session().state().setup.game_turn &&
          !restored.preview.host->session().readiness().can_continue(),
          "Civis preview must preserve selected payload/rules/turn and remain non-continuable");

    check(original.preview.host->activation_owner().busy_state.has_value() &&
          !restored.preview.host->activation_owner().busy_state.has_value(),
          "Civis preview invented original-import transient defaults");

    auto& retained=*original.preview.host->activation_owner().busy_state;
    check(retained.units().contains({0,16385}),"Selected checkpoint fixture unit changed");
    retained.set_unit_mission_timer({0,16385},7);
    retained.set_unit_combat_city({0,16385},{-1,8193}); // exact native null/stale ID preserved
    const auto partial=original.preview.host->export_import_clock_checkpoint();
    check(!partial.full_persistence && !partial.uncovered_domains.empty(),"Partial checkpoint claimed complete persistence");
    const auto checkpoint_path=temporary.path/"actual-transients.CivisSave";
    civ5::saves::write_civis_save(checkpoint_path,partial.document);
    const auto checkpoint_loaded=civ5::saves::read_loaded_save(checkpoint_path);
    check(checkpoint_loaded.live_bnw_ui_transients.has_value(),"Container lost actual transient section");
    auto checkpoint_host=civ5::ui::bootstrap_selected_bnw_preview(checkpoint_path,game_root,0,true);
    check(static_cast<bool>(checkpoint_host.preview) && !checkpoint_host.original_source_file_verified &&
          checkpoint_host.preview.host->activation_owner().busy_state.has_value(),
          "Civis host did not adopt serialized transient owner");
    const auto& busy=*checkpoint_host.preview.host->activation_owner().busy_state;
    check(busy.units().at({0,16385}).mission_timer==7 &&
          busy.units().at({0,16385}).combat_city.owner==-1 &&
          busy.units().at({0,16385}).combat_city.id==8193 &&
          !checkpoint_host.preview.host->session().readiness().can_continue(),
          "Civis restore replaced busy fields or enabled full continuation");
    const auto second=checkpoint_host.preview.host->export_import_clock_checkpoint();
    const auto second_path=temporary.path/"second-transients.CivisSave";
    civ5::saves::write_civis_save(second_path,second.document);
    auto second_host=civ5::ui::bootstrap_selected_bnw_preview(second_path,game_root,0,true);
    check(static_cast<bool>(second_host.preview) &&
          second_host.preview.host->activation_owner().busy_state.has_value() &&
          second_host.preview.host->activation_owner().busy_state->units().at({0,16385}).mission_timer==7,
          "Second save/restore lost actual transient state");
    // Restore the actual selected client and dispatch through its SAME Host
    // movement owner. A fresh-import owner must never be rebound for Civis.
    std::shared_ptr<civ5::ui::SelectedBnwPreviewHost> playable(std::move(second_host.preview.host));
    playable->activation_owner().busy_state->set_unit_mission_timer({0,16385},0);
    auto client=civ5::ui::BnwClientFrameOwner::from_selected_host(playable,
        civ5::test_detail::selected_city_frame_sources(playable));
    const auto input_registry=civ5::ui::UiContextRegistry::from_documents(
        civ5::content::read_xml_metadata(R"(<Context><LuaContext ID="WorldView" FileName="WorldView"/></Context>)"),
        civ5::content::read_xml_metadata("<Context/>"));
    auto receiver=client->create_selected_ui_host(input_registry,nullptr);
    auto frame=client->capture({1024,768,1024,768,96},{});
    const auto actual_ui=receiver->begin_frame(1024,768);
    const auto before_generation=playable->session().state_generation();
    const auto before_map_rng=playable->session().state().random.map.state();
    const auto before_game_rng=playable->session().state().random.gameplay.state();
    const auto command=civ5::ui::UiGameplayCommand::make("WorldView",civ5::ui::BnwManualCycleSelectionCommand{});
    const auto result=client->dispatch(*receiver,actual_ui,frame,command);
    check(result.status==civ5::ui::UiCommandStatus::navigation,
          "Civis manual cycle lacks adopted actual movement/busy owners");
    const auto after=client->capture({1024,768,1024,768,96},{});
    if(after->stamp.selection.unit)
        check(!client->is_current(frame) &&
              client->dispatch(*receiver,actual_ui,frame,command).status==civ5::ui::UiCommandStatus::invalid,
              "Restored manual cycle retained a replayable old frame");
    else std::cout<<"Restored fixture has no eligible manual unit; changed-selection branch UNOBSERVED\n";
    check(playable->session().state_generation()==before_generation &&
          playable->session().state().random.map.state()==before_map_rng &&
          playable->session().state().random.gameplay.state()==before_game_rng,
          "Restored manual cycle changed world or RNG");
    const auto transient_id=static_cast<std::uint32_t>(civ5::saves::CivisSaveSectionId::live_bnw_ui_transients);
    auto corrupted=partial.document;
    for(auto& s:corrupted.sections)if(s.id==transient_id)s.bytes.at(76)^=1;
    const auto corrupt_path=temporary.path/"corrupt-transients.CivisSave";
    civ5::saves::write_civis_save(corrupt_path,corrupted);
    const auto rejected=civ5::ui::bootstrap_selected_bnw_preview(corrupt_path,game_root,0);
    check(!rejected.preview && rejected.preview.blocker &&
          rejected.preview.blocker->stage==civ5::ui::SelectedBnwPreviewStage::live_restore,
          "Malformed transient owner fell back to import defaults");
    original.preview.host->activation_owner().completed.emplace(
        original.preview.host->session().state().setup.game_turn,0);
    bool activation_rejected=false;
    try{(void)original.preview.host->export_import_clock_checkpoint();}catch(const std::exception&){activation_rejected=true;}
    check(activation_rejected,"Unserialized activation history was silently lost");
    original.preview.host->activation_owner().completed.clear();
    check(civ5::runtime::identify_file(source).sha256==original_hash,"Checkpoint altered original Civ5Save");

    const auto prefix_id = static_cast<std::uint32_t>(
        civ5::saves::CivisSaveSectionId::source_prefix);
    auto no_prefix = document;
    std::erase_if(no_prefix.sections, [&](const auto &section) {
        return section.id == prefix_id;
    });
    const auto absent = temporary.path / "no-prefix.CivisSave";
    civ5::saves::write_civis_save(absent, no_prefix);
    auto blocked = civ5::ui::bootstrap_selected_bnw_preview(absent, game_root, 0);
    check(!blocked.preview && blocked.preview.blocker &&
          blocked.preview.blocker->stage ==
              civ5::ui::SelectedBnwPreviewStage::source_contract,
          "Civis import without original setup must not open a preview");

    auto bad_identity = original.preview.host->identity();
    bad_identity.rules_sha256.assign(64, '0');
    auto mismatched = document;
    mismatched.sections.push_back(civ5::runtime::make_simulation_random_checkpoint(
        original.preview.host->session().state().random, bad_identity));
    const auto wrong_rules = temporary.path / "wrong-rules.CivisSave";
    civ5::saves::write_civis_save(wrong_rules, mismatched);
    blocked = civ5::ui::bootstrap_selected_bnw_preview(wrong_rules, game_root, 0);
    check(!blocked.preview && blocked.preview.blocker &&
          blocked.preview.blocker->stage == civ5::ui::SelectedBnwPreviewStage::live_restore,
          "Civis overlay bound to other rules must fail before publishing LiveSession");

    auto newer_overlay = document;
    newer_overlay.sections.push_back({static_cast<std::uint32_t>(
        civ5::saves::CivisSaveSectionId::live_bnw_exploration_dirty), 0, {1}});
    const auto malformed_newer = temporary.path / "malformed-newer.CivisSave";
    civ5::saves::write_civis_save(malformed_newer, newer_overlay);
    blocked = civ5::ui::bootstrap_selected_bnw_preview(malformed_newer, game_root, 0);
    check(!blocked.preview && blocked.preview.blocker &&
          blocked.preview.blocker->stage == civ5::ui::SelectedBnwPreviewStage::live_restore,
          "New BNW overlay must reach its validating LiveSession consumer");

    auto unsupported = document;
    unsupported.sections.push_back({static_cast<std::uint32_t>(
        civ5::saves::CivisSaveSectionId::vp_research_profile), 0, {1}});
    const auto profile = temporary.path / "unsupported-profile.CivisSave";
    civ5::saves::write_civis_save(profile, unsupported);
    blocked = civ5::ui::bootstrap_selected_bnw_preview(profile, game_root, 0);
    check(!blocked.preview && blocked.preview.blocker &&
          blocked.preview.blocker->stage ==
              civ5::ui::SelectedBnwPreviewStage::selected_save,
          "Unsupported or mismatched profile section must not reach BNW preview");
}

} // namespace

int main(int argc, char **argv) {
    if (argc != 3) throw std::runtime_error("Expected selected Gandhi save and installed game root");
    run(argv[1], argv[2]);
}
