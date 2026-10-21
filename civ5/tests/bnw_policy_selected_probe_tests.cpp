#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/ui/progression_screens.hpp"
#include "civ5/ui/screen_compositor.hpp"
#include <algorithm>
#include <filesystem>
#include <iostream>
#include <map>
#include <memory>
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}
}

int main(int argc, char **argv) {
    try {
        using namespace civ5;
        check(argc == 3, "Supply selected BNW save and original game root");
        auto bootstrap = ui::bootstrap_selected_bnw_preview(argv[1], argv[2], 0);
        if (bootstrap.preview.blocker)
            throw std::runtime_error("Selected BNW bootstrap: " +
                                     bootstrap.preview.blocker->reason);
        check(bootstrap.preview.host != nullptr, "Selected BNW host is absent");
        auto &host = *bootstrap.preview.host;
        auto input = host.preview_input();
        check(input.frame_sources.database && input.frame_sources.technologies,
              "Selected BNW preview has no activated progression rules");
        const auto &database = *input.frame_sources.database;
        const auto defines = database.cache_global_defines(content::GlobalDefineProfile::bnw);
        check(!defines.errors(), "Selected BNW defines have SQL errors");
        const auto progression = ui::capture_progression_frame(host.session().state().setup,
            host.session().state().player_research,host.session().state().technologies,
            *input.frame_sources.technologies,database,host.active_player_id(),{},
            &host.session().state().world);
        const auto frame = ui::capture_live_session_ui_frame(host.session(), 1,
            host.active_player_id(), input.frame_sources);
        const auto *live_progression = ui::progression_for_active_player(frame);
        check(progression.policy_ownership_resolved &&
              progression.policies.size() == 111 &&
              progression.branches.size() == 12 &&
              live_progression && live_progression->policy_ownership_resolved &&
              live_progression->policies.size() == progression.policies.size() &&
              !host.session().state().setup.players.at(0).policies_resolved,
              "Selected Gandhi progression policy ownership or rules are unavailable");
        std::size_t policy_tech_prerequisites = 0;
        const auto tech_report = database.query_rows(
            {"SELECT TechPrereq FROM Policies WHERE TechPrereq IS NOT NULL AND TechPrereq <> ''"},
            [&](const content::RulesRowView &row) {
                const auto type = row.text("TechPrereq").value_or("");
                check(std::any_of(input.frame_sources.technologies->entries.begin(),
                      input.frame_sources.technologies->entries.end(),
                      [&](const auto &entry) { return entry.second.base.type == type; }),
                      "Selected policy technology is absent from selected technology IDs");
                ++policy_tech_prerequisites;
            });
        check(tech_report.all_succeeded(), "Selected policy technology query failed");
        std::map<std::string, std::size_t> reasons;
        std::size_t observed_costs = 0, adopted = 0;
        for (const auto &policy : progression.policies) {
            if (!policy.branch_type.empty())
                check(std::any_of(progression.branches.begin(), progression.branches.end(),
                      [&](const auto &branch) { return branch.type == policy.branch_type; }),
                      "Selected UI policy branch has no selected branch ID");
            const auto preview = ui::preview_bnw_policy_choice(host.session(), database,
                *input.frame_sources.technologies, defines,
                host.active_player_id(), policy.id);
            check(preview.policy_type.empty() || preview.policy_type == policy.type,
                  "UI policy ID differs from selected-rule policy ID");
            check(preview.branch_type.empty() || preview.branch_type == policy.branch_type,
                  "UI policy branch differs from selected-rule branch");
            if (policy.adopted) ++adopted;
            if (preview.culture && preview.next_cost) {
                check(*preview.culture == 0 && *preview.next_cost == 15,
                      "UI policy payment differs from selected Gandhi live source");
                ++observed_costs;
            }
            if (preview.passes_known_rules) ++reasons["known-rule pass"];
            else ++reasons[preview.blocker];
        }
        check(observed_costs > 0 && !reasons.empty(),
              "Selected Gandhi policy cost or blockers were not observed");
        const auto candidate = std::find_if(progression.policies.begin(),
            progression.policies.end(), [](const auto &policy) {
                return !policy.adopted && !policy.branch_type.empty();
            });
        check(candidate != progression.policies.end(),
              "Selected Gandhi has no policy action candidate to gate");
        const runtime::BnwResearchChoiceRules research_rules{
            *input.frame_sources.technologies, defines, progression.team_id};
        ui::BnwLiveProgressionBackend backend(host.session(), research_rules,
            {}, {}, {}, &database);
        const auto action = std::get<ui::UiGameplayCommand>(
            ui::progression_choose_policy(candidate->id));
        check(backend.availability(action, frame).reason ==
                  "Live BNW policy rules or ownership are unresolved" &&
              backend.execute(action, frame).status ==
                  ui::UiCommandStatus::unavailable,
              "Read-only saved policy evidence cannot authorize adoption");
        const std::filesystem::path game_root(argv[2]);
        const auto scan_ui = [&](const std::filesystem::path &directory) {
            auto index = std::make_shared<assets::LooseResourceIndex>(game_root);
            index->scan({directory, {}, false});
            return index;
        };
        auto base_ui = scan_ui(game_root / "Assets/UI");
        auto expansion_ui = scan_ui(game_root / "Assets/DLC/Expansion/UI");
        auto bnw_ui = scan_ui(game_root / "Assets/DLC/Expansion2/UI");
        assets::ResourceMountTable resources(assets::resource_lookup(base_ui));
        resources.mount({1, 0, 0, assets::resource_lookup(expansion_ui), true, true, {}, {}});
        resources.mount({2, 0, 0, assets::resource_lookup(bnw_ui), true, true, {}, {}});
        const auto registry = ui::UiContextRegistry::from_resources(resources);
        const auto *policy_context = registry.find("SocialPolicyPopup");
        check(policy_context && policy_context->modal,
              "Selected BNW policy popup must own modal input focus");
        const auto policy_source = ui::read_progression_screen_source(
            resources, ui::ProgressionScreen::policies);
        check(policy_source.selected_source_path.find("Expansion2") != std::string::npos,
              "Selected BNW policy screen must use Expansion2 XML");
        ui::LiveUiSources ui_sources;
        ui_sources.progression[static_cast<std::size_t>(ui::ProgressionScreen::policies)] =
            &policy_source;
        ui::LiveUiHost ui_host(registry,
            ui::make_live_session_frame_provider(host.session(), host.active_player_id(),
                                                 input.frame_sources),
            &backend, ui_sources);
        ui::UiScreenCompositor compositor(resources, registry, &backend);
        auto live_frame = ui_host.begin_frame(1280, 960);
        check(ui_host.dispatch(ui::progression_open_command(ui::ProgressionScreen::policies),
                               live_frame).status == ui::UiCommandStatus::navigation,
              "Selected BNW policy screen did not open");
        auto presented = ui::present_live_ui_frame(ui_host, compositor, 1280, 960);
        check(presented.composed.focused_context == "SocialPolicyPopup" &&
              presented.live.progression && !presented.live.progression->items.empty(),
              "Selected Gandhi policy frame was not presented");
        check(presented.composed.policy_scroll_regions.size() == 9 &&
              std::none_of(presented.composed.unresolved.begin(),
                  presented.composed.unresolved.end(), [](const auto &reason) {
                      return reason.find("temporary non-native layout") != std::string::npos;
                  }),
              "Selected BNW branch ScrollPanels were not laid out from XML");
        const auto tradition = std::find_if(
            presented.composed.policy_scroll_regions.begin(),
            presented.composed.policy_scroll_regions.end(),
            [](const auto &region) { return region.panel_id == "TraditionPanel"; });
        check(tradition != presented.composed.policy_scroll_regions.end() &&
              tradition->bounds.x == 155 && tradition->bounds.y == 304 &&
              tradition->bounds.width == 190 && tradition->bounds.height == 204,
              "Selected BNW Stack and ScrollPanel anchors differ from the viewport plan");
        const auto policy_hit = std::find_if(presented.composed.hits.begin(),
            presented.composed.hits.end(), [](const auto &hit) {
                const auto *gameplay = std::get_if<ui::UiGameplayCommand>(&hit.command);
                return gameplay && gameplay->template as<ui::ProgressionChoosePolicy>();
            });
        const auto close_hit = std::find_if(presented.composed.hits.begin(),
            presented.composed.hits.end(), [](const auto &hit) {
                const auto *close = std::get_if<ui::UiCloseScreen>(&hit.command);
                return close && close->id == "SocialPolicyPopup";
            });
        if (policy_hit == presented.composed.hits.end() ||
            close_hit == presented.composed.hits.end())
            for (const auto &reason : presented.composed.unresolved)
                std::cerr << "policy layout: " << reason << '\n';
        check(policy_hit != presented.composed.hits.end() && !policy_hit->enabled &&
              close_hit != presented.composed.hits.end() && close_hit->enabled,
              "Policy choice must stay blocked while the selected close control works");
        check(close_hit->bounds.x == 169 && close_hit->bounds.y == 822 &&
              close_hit->bounds.width == 150 && close_hit->bounds.height == 32,
              "Selected BNW CloseButton anchor differs from the screen plan");
        check(policy_hit->bounds.width <= 48 && policy_hit->bounds.height <= 48,
              "Policy choice hit must use the selected PolicyIcon dimensions");
        for (const auto &hit : presented.composed.hits)
            check(hit.bounds.x >= 0 && hit.bounds.y >= 0 &&
                  hit.bounds.width > 0 && hit.bounds.height > 0 &&
                  hit.bounds.x + hit.bounds.width <= 1280 &&
                  hit.bounds.y + hit.bounds.height <= 960,
                  "Policy screen hit escaped the actual viewport");
        const auto blocked_click = ui::click_composed_ui(ui_host, presented.live,
            presented.composed, policy_hit->bounds.x + 2, policy_hit->bounds.y + 2);
        check(blocked_click.consumed &&
              blocked_click.command.status == ui::UiCommandStatus::unavailable &&
              ui_host.screens().focused_screen() == "SocialPolicyPopup" &&
              !host.session().state().setup.players.at(host.active_player_id()).policies_resolved,
              "Disabled Gandhi policy hit escaped focus or changed live state");
        const auto outside_click = ui::click_composed_ui(ui_host, presented.live,
            presented.composed, 0, 0, 0, 0);
        check(outside_click.consumed &&
              outside_click.command.status == ui::UiCommandStatus::invalid,
              "Modal policy popup must consume outside world clicks");
        const auto mismatched_click = ui::click_composed_ui(ui_host, presented.live,
            presented.composed, 0, 0,
            close_hit->bounds.x + 2, close_hit->bounds.y + 2);
        check(mismatched_click.consumed &&
              mismatched_click.command.status == ui::UiCommandStatus::invalid &&
              ui_host.screens().focused_screen() == "SocialPolicyPopup",
              "Modal press outside/release on Close must not activate Close");
        const auto first_region = presented.composed.policy_scroll_regions.front();
        check(first_region.max_offset > 0 && first_region.offset == 0,
              "Authored policy branch viewport has no scroll range");
        const auto scrolled = ui::wheel_composed_ui(ui_host, presented.live,
            presented.composed, first_region.bounds.x + 2,
            first_region.bounds.y + 2, -120);
        check(scrolled.consumed && scrolled.command.status ==
                  ui::UiCommandStatus::navigation,
              "Focused policy ScrollPanel did not consume a wheel step");
        const auto stale_scroll = ui::wheel_composed_ui(ui_host, presented.live,
            presented.composed, first_region.bounds.x + 2,
            first_region.bounds.y + 2, -120);
        check(stale_scroll.consumed && stale_scroll.command.status ==
                  ui::UiCommandStatus::invalid,
              "Old policy scroll geometry accepted a second wheel event");
        auto after_scroll = ui::present_live_ui_frame(ui_host, compositor, 1280, 960);
        const auto moved_region = std::find_if(
            after_scroll.composed.policy_scroll_regions.begin(),
            after_scroll.composed.policy_scroll_regions.end(),
            [&](const auto &value) { return value.panel_id == first_region.panel_id; });
        check(moved_region != after_scroll.composed.policy_scroll_regions.end() &&
              moved_region->offset == 68 &&
              std::count_if(after_scroll.composed.policy_scroll_regions.begin(),
                  after_scroll.composed.policy_scroll_regions.end(),
                  [](const auto &value) { return value.offset != 0; }) == 1,
              "Wheel scroll must move only the focused authored branch panel");
        const auto outside_wheel = ui::wheel_composed_ui(ui_host, after_scroll.live,
            after_scroll.composed, 0, 0, -120);
        check(outside_wheel.consumed && outside_wheel.command.status ==
                  ui::UiCommandStatus::invalid,
              "Focused policy screen must consume wheel input outside branch panels");
        auto resized = ui::present_live_ui_frame(ui_host, compositor, 1400, 1000);
        check(std::all_of(resized.composed.policy_scroll_regions.begin(),
                  resized.composed.policy_scroll_regions.end(),
                  [](const auto &value) { return value.offset == 0; }),
              "Policy scroll offsets must reset on viewport resize");
        const auto resized_close = std::find_if(resized.composed.hits.begin(),
            resized.composed.hits.end(), [](const auto &hit) {
                const auto *close = std::get_if<ui::UiCloseScreen>(&hit.command);
                return close && close->id == "SocialPolicyPopup";
            });
        check(resized_close != resized.composed.hits.end(),
              "Selected policy CloseButton disappeared after resize");
        const auto closed = ui::click_composed_ui(ui_host, resized.live,
            resized.composed, resized_close->bounds.x + 2,
            resized_close->bounds.y + 2,
            resized_close->bounds.x + 2,
            resized_close->bounds.y + 2);
        check(closed.consumed && closed.command.status == ui::UiCommandStatus::navigation &&
              !ui_host.screens().focused_screen(),
              "Measured policy close hit did not release modal focus");
        const auto stale = ui::click_composed_ui(ui_host, presented.live,
            presented.composed, policy_hit->bounds.x + 2, policy_hit->bounds.y + 2);
        check(stale.consumed && stale.command.status == ui::UiCommandStatus::invalid,
              "Old policy frame accepted input after focus changed");
        auto compact = ui_host.begin_frame(800, 600);
        check(ui_host.dispatch(ui::progression_open_command(ui::ProgressionScreen::policies),
                               compact).status == ui::UiCommandStatus::navigation,
              "Selected policy screen did not reopen in compact viewport");
        const auto compact_view = ui::present_live_ui_frame(ui_host, compositor, 800, 600);
        check(compact_view.composed.hits.empty() &&
              compact_view.composed.policy_scroll_regions.empty() &&
              std::any_of(compact_view.composed.unresolved.begin(),
                  compact_view.composed.unresolved.end(), [](const auto &reason) {
                      return reason.find("does not fit the viewport") != std::string::npos;
                  }) &&
              ui::click_composed_ui(ui_host, compact_view.live,
                  compact_view.composed, 0, 0).consumed,
              "Incomplete compact policy layout must fail closed under modal focus");
        std::cout << "selected policies=" << progression.policies.size()
                  << " adopted=" << adopted
                  << " policy-tech-prerequisites=" << policy_tech_prerequisites
                  << " turn=" << host.session().state().setup.game_turn << '\n';
        for (const auto &[reason, count] : reasons)
            std::cout << count << " x " << reason << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL selected BNW policy probe: " << error.what() << '\n';
        return 1;
    }
}
