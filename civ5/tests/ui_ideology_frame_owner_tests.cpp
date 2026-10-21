#include "civ5/ui/ideology_frame_owner.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>

namespace {
void check(bool condition, const char *reason) {
    if (!condition) throw std::runtime_error(reason);
}
std::size_t branch_hits(const civ5::ui::IdeologyLayoutPlan &plan) {
    return std::count_if(plan.hits.begin(), plan.hits.end(), [](const auto &hit) {
        const auto *gameplay = std::get_if<civ5::ui::UiGameplayCommand>(&hit.command);
        return gameplay && gameplay->template as<civ5::ui::ProgressionChooseIdeology>();
    });
}
}

int main(int argc, char **argv) {
    try {
        using namespace civ5;
        check(argc == 3, "Supply selected Gandhi save and original game root");
        const std::filesystem::path game_root(argv[2]);
        auto bootstrap = ui::bootstrap_selected_bnw_preview(argv[1], game_root, 0);
        if (bootstrap.preview.blocker)
            throw std::runtime_error("Selected BNW bootstrap: " +
                                     bootstrap.preview.blocker->reason);
        check(bootstrap.preview.host != nullptr, "Selected Gandhi host is absent");
        auto &host = *bootstrap.preview.host;
        auto input = host.preview_input();
        const auto scan = [&](const std::filesystem::path &directory) {
            auto index = std::make_shared<assets::LooseResourceIndex>(game_root);
            index->scan({directory, {}, false});
            return index;
        };
        auto base = scan(game_root / "Assets/UI");
        auto expansion = scan(game_root / "Assets/DLC/Expansion/UI");
        auto bnw = scan(game_root / "Assets/DLC/Expansion2/UI");
        assets::ResourceMountTable resources(assets::resource_lookup(base));
        resources.mount({1, 0, 0, assets::resource_lookup(expansion), true, true, {}, {}});
        resources.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {}, {}});
        const auto source = ui::read_progression_screen_source(
            resources, ui::ProgressionScreen::ideology);
        const auto plain = ui::capture_live_session_ui_frame(host.session(), 1,
            host.active_player_id(), input.frame_sources);
        const auto *progression = ui::progression_for_active_player(plain);
        check(progression && !plain.find<ui::ProgressionIdeologyPopupEvent>(),
              "Selected Gandhi has no real ideology event or progression rules");
        std::map<std::string, std::int64_t> ids;
        for (const auto &branch : progression->branches)
            if (branch.ideology) ids.emplace(branch.type, branch.id);
        check(ids.size() == 3 &&
              ids.contains("POLICY_BRANCH_AUTOCRACY") &&
              ids.contains("POLICY_BRANCH_FREEDOM") &&
              ids.contains("POLICY_BRANCH_ORDER"),
              "Activated BNW ideology IDs are missing");
        int adopter_calls = 0;
        int free_policy_calls = 0;
        const auto counts = [&](const runtime::LiveSession &,
                                const ui::ProgressionIdeologyPopupEvent &) {
            ++adopter_calls;
            return std::optional<std::vector<ui::IdeologyAdopterCount>>{
                {{ids.at("POLICY_BRANCH_ORDER"), 0},
                 {ids.at("POLICY_BRANCH_AUTOCRACY"), 1},
                 {ids.at("POLICY_BRANCH_FREEDOM"), 0}}};
        };
        const auto free_policies = [&](const runtime::LiveSession &,
            const ui::ProgressionIdeologyPopupEvent &) {
            ++free_policy_calls;
            return std::optional<std::vector<ui::IdeologyFreePolicyCount>>{
                {{ids.at("POLICY_BRANCH_ORDER"), 0},
                 {ids.at("POLICY_BRANCH_AUTOCRACY"), 2},
                 {ids.at("POLICY_BRANCH_FREEDOM"), 1}}};
        };
        auto no_event = ui::make_bnw_ideology_layout_frame_provider(
            host.session(), host.active_player_id(), input.frame_sources,
            source, counts, {}, free_policies);
        const auto absent = no_event();
        check(!absent.find<ui::ProgressionIdeologyPopupEvent>() &&
              !absent.find<ui::IdeologyLayoutEvidence>() && adopter_calls == 0 &&
              free_policy_calls == 0 &&
              branch_hits(ui::build_ideology_layout(source, absent, 1280, 960)) == 0,
              "Selected Gandhi invented ideology event, counts, or rows");

        // This event exists only inside the test's explicit popup owner. It is
        // never inferred from the selected Gandhi save or policy rule table.
        std::optional<ui::ProgressionIdeologyPopupEvent> event{
            ui::ProgressionIdeologyPopupEvent{host.active_player_id(), 77}};
        auto with_event = input.frame_sources;
        with_event.active_ideology_popup = [&](const runtime::LiveSession &) {
            return event;
        };
        auto provider = ui::make_bnw_ideology_layout_frame_provider(
            host.session(), host.active_player_id(), with_event, source,
            counts, {}, free_policies);
        const auto active = provider();
        const auto *evidence = active.find<ui::IdeologyLayoutEvidence>();
        check(active.find<ui::ProgressionIdeologyPopupEvent>() && evidence &&
              evidence->player_id == host.active_player_id() &&
              evidence->turn == host.session().state().setup.game_turn &&
              evidence->frame == active.frame() && evidence->event_token == 77 &&
              evidence->selected_source_path == source.selected_source_path &&
              adopter_calls == 1 && free_policy_calls == 1 &&
              evidence->free_policy_counts &&
              evidence->free_policy_counts->size() == 3,
              "Owner counts lost popup/player/turn/frame/source identity");
        const auto plan = ui::build_ideology_layout(source, active, 1280, 960);
        check(branch_hits(plan) == 3 &&
              std::all_of(plan.hits.begin() + 1, plan.hits.end(),
                  [](const auto &hit) { return !hit.enabled; }),
              "Owner evidence failed to place disabled ideology rows");
        ui::ProgressionGameplayBackend backend;
        const auto command = std::get<ui::UiGameplayCommand>(
            ui::progression_choose_ideology(ids.at("POLICY_BRANCH_AUTOCRACY")));
        check(backend.availability(command, active).status ==
                  ui::UiCommandStatus::unavailable &&
              backend.execute(command, active).status ==
                  ui::UiCommandStatus::unavailable,
              "Read-only ideology rows activated adoption");
        const auto tenet_policy = std::find_if(progression->policies.begin(),
            progression->policies.end(), [](const auto &policy) {
                return policy.branch_type == "POLICY_BRANCH_AUTOCRACY";
            });
        check(tenet_policy != progression->policies.end(),
              "Selected rules have no Autocracy policy for owner Help validation");
        const auto authored_counts = [&](const runtime::LiveSession &, const auto &) {
            return std::optional<std::vector<ui::IdeologyAdopterCount>>{
                {{ids.at("POLICY_BRANCH_AUTOCRACY"), 1},
                 {ids.at("POLICY_BRANCH_FREEDOM"), 0},
                 {ids.at("POLICY_BRANCH_ORDER"), 0}}};
        };
        auto help_provider = ui::make_bnw_ideology_layout_frame_provider(
            host.session(), host.active_player_id(), with_event, source,
            authored_counts, [&](const runtime::LiveSession &, const auto &) {
                return std::optional<std::vector<ui::IdeologyTenetHelpRow>>{
                    {{tenet_policy->id, tenet_policy->type, tenet_policy->branch_type,
                      "OWNER_HELP_KEY", "Owner localized Help", 1, 180, 20}}};
            });
        const auto help_frame = help_provider();
        const auto *help_evidence = help_frame.find<ui::IdeologyLayoutEvidence>();
        check(help_evidence && help_evidence->tenet_help &&
              help_evidence->tenet_help->size() == 1,
              "Current owner Help row was not published with popup evidence");
        auto invalid_help = ui::make_bnw_ideology_layout_frame_provider(
            host.session(), host.active_player_id(), with_event, source,
            authored_counts, [&](const runtime::LiveSession &, const auto &) {
                return std::optional<std::vector<ui::IdeologyTenetHelpRow>>{
                    {{-1, tenet_policy->type, tenet_policy->branch_type,
                      "OWNER_HELP_KEY", "Owner localized Help", 1, 180, 20}}};
            });
        const auto invalid_frame = invalid_help();
        const auto *invalid_evidence = invalid_frame.find<ui::IdeologyLayoutEvidence>();
        check(invalid_evidence && !invalid_evidence->tenet_help,
              "Unknown tenet policy ID produced owner Help evidence");
        event.reset();
        const auto closed = provider();
        check(closed.frame() > active.frame() &&
              !closed.find<ui::ProgressionIdeologyPopupEvent>() &&
              !closed.find<ui::IdeologyLayoutEvidence>() && adopter_calls == 1 &&
              free_policy_calls == 1,
              "Closed popup reused evidence from an earlier live frame");
        auto bad_free = ui::make_bnw_ideology_layout_frame_provider(
            host.session(), host.active_player_id(), with_event, source,
            counts, {}, [&](const runtime::LiveSession &, const auto &) {
                return std::optional<std::vector<ui::IdeologyFreePolicyCount>>{
                    {{ids.at("POLICY_BRANCH_AUTOCRACY"), 2},
                     {ids.at("POLICY_BRANCH_AUTOCRACY"), 1},
                     {ids.at("POLICY_BRANCH_ORDER"), 0}}};
            });
        event = ui::ProgressionIdeologyPopupEvent{host.active_player_id(), 79};
        const auto bad_free_frame = bad_free();
        const auto *bad_free_evidence = bad_free_frame.find<ui::IdeologyLayoutEvidence>();
        check(bad_free_evidence && !bad_free_evidence->free_policy_counts,
              "Duplicate GameCore free-policy branch values were published");

        auto duplicated = ui::make_bnw_ideology_layout_frame_provider(
            host.session(), host.active_player_id(), with_event, source,
            [&](const runtime::LiveSession &, const auto &) {
                return std::optional<std::vector<ui::IdeologyAdopterCount>>{
                    {{ids.at("POLICY_BRANCH_AUTOCRACY"), 1},
                     {ids.at("POLICY_BRANCH_AUTOCRACY"), 0},
                     {ids.at("POLICY_BRANCH_ORDER"), 0}}};
            });
        event = ui::ProgressionIdeologyPopupEvent{host.active_player_id(), 78};
        check(!duplicated().find<ui::IdeologyLayoutEvidence>(),
              "Duplicate branch IDs produced ideology evidence");
        auto impossible = ui::make_bnw_ideology_layout_frame_provider(
            host.session(), host.active_player_id(), with_event, source,
            [&](const runtime::LiveSession &, const auto &) {
                return std::optional<std::vector<ui::IdeologyAdopterCount>>{
                    {{ids.at("POLICY_BRANCH_AUTOCRACY"), 1000},
                     {ids.at("POLICY_BRANCH_FREEDOM"), 0},
                     {ids.at("POLICY_BRANCH_ORDER"), 0}}};
            });
        check(!impossible().find<ui::IdeologyLayoutEvidence>(),
              "Adopter count exceeding live roster produced evidence");
        const auto original_turn = host.session().state().setup.game_turn;
        auto changing_turn = ui::make_bnw_ideology_layout_frame_provider(
            host.session(), host.active_player_id(), with_event, source,
            [&](const runtime::LiveSession &, const auto &) {
                ++host.session().mutable_state().setup.game_turn;
                return std::optional<std::vector<ui::IdeologyAdopterCount>>{
                    {{ids.at("POLICY_BRANCH_AUTOCRACY"), 1},
                     {ids.at("POLICY_BRANCH_FREEDOM"), 0},
                     {ids.at("POLICY_BRANCH_ORDER"), 0}}};
            });
        check(!changing_turn().find<ui::IdeologyLayoutEvidence>(),
              "Turn changed during owner callback but stale evidence was published");
        host.session().mutable_state().setup.game_turn = original_turn;
        auto throwing = ui::make_bnw_ideology_layout_frame_provider(
            host.session(), host.active_player_id(), with_event, source,
            [&](const runtime::LiveSession &, const auto &)
                -> std::optional<std::vector<ui::IdeologyAdopterCount>> {
                throw std::runtime_error("Current GameCore roster unavailable");
            });
        check(!throwing().find<ui::IdeologyLayoutEvidence>(),
              "Failed GameCore adopter reader produced evidence");
        std::cout << "PASS selected Gandhi absence and owner-scoped ideology evidence\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL ideology frame owner: " << error.what() << '\n';
        return 1;
    }
}
