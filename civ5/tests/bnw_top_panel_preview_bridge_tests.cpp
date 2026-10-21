#include "civ5/ui/bnw_top_panel_preview_bridge.hpp"
#include "civ5/ui/bnw_client_frame.hpp"
#include "civ5/ui/overview_screens.hpp"
#include "civ5/ui/ui_owner_fences.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/content/loose_resource_plan.hpp"
#include <iostream>
#include <algorithm>
#include <memory>
#include <stdexcept>
using namespace civ5;
namespace {
void check(bool value, const char *message) { if (!value) throw std::runtime_error(message); }
ui::TopPanelLocalizationCatalog locale(const std::filesystem::path &root, bool russian) {
    auto base = std::make_shared<assets::LooseResourceIndex>(root);
    auto expansion = std::make_shared<assets::LooseResourceIndex>(root);
    auto bnw = std::make_shared<assets::LooseResourceIndex>(root);
    const auto language = russian ? "RU_RU" : "en_US";
    base->scan({root / "Assets/Gameplay/XML/NewText" / (russian ? "RU_RU" : "EN_US"), {}, false});
    expansion->scan({root / "Assets/DLC/Expansion/Gameplay/XML/Text" / language, {}, false});
    bnw->scan({root / "Assets/DLC/Expansion2/Gameplay/XML/Text" / language, {}, false});
    assets::ResourceMountTable resources(assets::resource_lookup(base), {}, assets::resource_keys(base));
    resources.mount({1, 0, 0, assets::resource_lookup(expansion), true, true, {}, assets::resource_keys(expansion)});
    resources.mount({2, 0, 0, assets::resource_lookup(bnw), true, true, {}, assets::resource_keys(bnw)});
    return ui::TopPanelLocalizationCatalog::read_active_bnw_resources(resources,
        russian ? "Language_RU_RU" : "Language_en_US");
}
void run(const char *save, const char *root) {
    auto bootstrap = ui::bootstrap_selected_bnw_preview(save, root, 0);
    check(bootstrap.preview.host && !bootstrap.preview.blocker, "Actual selected source bootstrap failed");
    auto &host = *bootstrap.preview.host;
    ui::BnwTopPanelPreviewBridge bridge(host.session(), host.database(), host.identity(), 0);
    if (const auto reason = bridge.blocker(host.session())) throw std::runtime_error(*reason);
    const auto bound_generation=host.session().state_generation();
    const auto before = host.session().state().maintenance.players.at(0);
    const auto ready_before = host.session().readiness().blockers;
    const auto quotes = bridge.capture(host.session(), 17);
    check(quotes.gold.contains(0) && quotes.science.contains(0) && quotes.bnw_native_breakdowns.contains(0),
        "Actual native gold/science quotes are unavailable");
    check(quotes.gold.size() == 1 && quotes.science.size() == 1 && quotes.trade.empty() &&
        quotes.faith.empty() && quotes.tourism.empty(), "Unproved fields or foreign players were promoted");
    check(host.session().state().maintenance.players.at(0).cached_unit_expense == before.cached_unit_expense &&
        host.session().state().maintenance.players.at(0).cached_supply_expense == before.cached_supply_expense &&
        host.session().readiness().blockers == ready_before && !host.session().readiness().can_continue(),
        "Read-only quotes mutated maintenance or full-turn readiness");
    check(bridge.matches(host.session(),quotes,17),"Actual native TopPanel quote requery differs");
    auto altered=quotes;
    ++altered.gold.at(0).income.cities;
    --altered.gold.at(0).income.diplomacy; // Same total, different tooltip operands.
    check(!bridge.matches(host.session(),altered,17),"Equal gold total concealed changed source breakdown");
    altered=quotes;
    ++altered.science.at(0).cities;
    --altered.science.at(0).agreements;
    check(!bridge.matches(host.session(),altered,17),"Equal science total concealed changed source breakdown");
    altered=quotes;
    altered.gold.at(0).income.profile_extensions.emplace<int>(1);
    check(!bridge.matches(host.session(),altered,17),"Unknown native quote extension was accepted");
    altered=quotes;
    altered.science.at(0).profile_extensions.emplace<int>(1);
    check(!bridge.matches(host.session(),altered,17),"Unknown science quote extension was accepted");
    check(!bridge.matches(host.session(),{},17) && !bridge.matches(host.session(),quotes,0) &&
        !bridge.matches(host.session(),quotes,18),"Missing native quote or wrong frame accepted");
    auto input = host.preview_input();
    check(bool(input.frame_sources.quotes) && bool(input.frame_sources.quotes_current),
        "Production preview input did not bind native quote/current readers");
    const auto snapshot = ui::capture_live_session_ui_frame(host.session(), 17, 0, input.frame_sources);
    const auto *panel = snapshot.find<ui::TopPanelLiveFrame>();
    check(panel && panel->players.contains(0) && panel->players.at(0).native_breakdown,
        "Production frame consumer did not adopt native query breakdown");
    const auto count = host.session().state().cities.players.at(0).cities.size();
    std::cout << "actual_selected_player=0 city_count=" << count << " gold_rate_times100="
        << quotes.gold.at(0).net_times100 << " science_times100=" << quotes.science.at(0).total_times100 << '\n';
    for (bool russian : {false, true}) {
        const auto catalog = locale(root, russian);
        const ui::TopPanelTextLocalizer text{.text = [&](std::string_view key, const std::vector<std::string> &args) {
            return catalog.format(key, args);
        }};
        const auto actual = ui::present_top_panel_frame(snapshot, text);
        check(actual.has_value(), "Actual source TopPanel was not presentable in EN/RU");
        if (!count) check(!actual->show_info_stack && !actual->gold_text && !actual->science_text &&
            !actual->tourism_text && !actual->faith_text, "Actual no-city source must hide the authored info stack");
        if (count) check(actual->show_info_stack && actual->gold_text && actual->science_text &&
            actual->gold_text->find("N/A") == std::string::npos &&
            actual->science_text->find("N/A") == std::string::npos &&
            actual->gold_tooltip && actual->science_tooltip &&
            actual->gold_tooltip->find("N/A") == std::string::npos &&
            actual->science_tooltip->find("N/A") == std::string::npos,
            "Actual city-bearing Gandhi source still displays unavailable native gold/science");
        // Presentation-only fixture: expose the actual native quotes to exercise
        // selected EN/RU tooltip rows. This does not create a live/source city.
        auto fixture = *panel;
        fixture.players.at(0).city_count = 1;
        fixture.players.at(0).values.city_count = 1;
        const auto frame = snapshot.with(std::make_shared<const ui::TopPanelLiveFrame>(fixture));
        const auto shown = ui::present_top_panel_frame(frame, text);
        check(shown && shown->gold_text && shown->science_text && shown->gold_tooltip && shown->science_tooltip &&
            shown->gold_text->find("N/A") == std::string::npos && shown->science_text->find("N/A") == std::string::npos,
            "Native gold/science rates remained unavailable");
        check(shown->gold_tooltip->find(catalog.format("TXT_KEY_TP_GOLD_FROM_CITY_CONNECTIONS", {"0"})) != std::string::npos &&
            shown->gold_tooltip->find(catalog.format("TXT_KEY_TP_GOLD_FROM_ITR", {"0"})) != std::string::npos &&
            shown->science_tooltip->find(catalog.format("TXT_KEY_TP_TECH_CITY_COST",
                {std::to_string(quotes.bnw_native_breakdowns.at(0).technology_city_cost_modifier)})) != std::string::npos,
            "Native BNW tooltip omitted original selected EN/RU source rows");
        // Bounded presentation fixture tests exact retail BNW distinctions;
        // values below do not mutate or attest the actual source world.
        auto &fixture_player = fixture.players.at(0);
        fixture_player.native_gold->income = {.cities=1250, .diplomacy=-400, .connections=0,
            .religion=200, .traits=300, .total_times100=1350, .profile_extensions={}};
        fixture_player.native_gold->expenses = runtime::MaintenanceQuote{};
        auto &expense = *fixture_player.native_gold->expenses;
        expense.units.total=2; expense.supply=1; expense.buildings=3; expense.improvements=4; expense.total=10;
        fixture_player.native_breakdown->city_gold_without_trade_times100=1050;
        fixture_player.native_science->cities=1500;
        fixture_player.native_breakdown->city_science_without_trade_times100=1000;
        const auto split = ui::present_top_panel_frame(snapshot.with(
            std::make_shared<const ui::TopPanelLiveFrame>(fixture)), text);
        check(split && split->gold_tooltip && split->science_tooltip &&
            split->gold_tooltip->find(catalog.format("TXT_KEY_TP_TOTAL_INCOME", {"17"})) != std::string::npos &&
            split->gold_tooltip->find(catalog.format("TXT_KEY_TP_TOTAL_EXPENSES", {"14"})) != std::string::npos &&
            split->gold_tooltip->find(catalog.format("TXT_KEY_TP_GOLD_FROM_RELIGION", {"2"})) != std::string::npos &&
            split->gold_tooltip->find(catalog.format("TXT_KEY_TP_GOLD_FROM_TRAITS", {"3"})) != std::string::npos &&
            split->gold_tooltip->find(catalog.format("TXT_KEY_TP_CITY_OUTPUT", {"10.5"})) != std::string::npos &&
            split->science_tooltip->find(catalog.format("TXT_KEY_TP_SCIENCE_FROM_CITIES", {"10"})) != std::string::npos &&
            split->science_tooltip->find(catalog.format("TXT_KEY_TP_SCIENCE_FROM_ITR", {"5"})) != std::string::npos,
            "BNW native tooltip lost city/trade split, religion/traits or negative diplomacy");
        fixture.players.at(0).native_breakdown->owner_frame_number = 16;
        const auto stale = ui::present_top_panel_frame(snapshot.with(
            std::make_shared<const ui::TopPanelLiveFrame>(fixture)), text);
        check(stale && stale->gold_text && stale->gold_text->find("N/A") != std::string::npos &&
            stale->science_text == "N/A", "Stale native query stamp remained actionable/displayed");
        std::cout << (russian ? "RU" : "EN") << " actual_gold=" << actual->gold_text.value_or("<hidden>")
            << " actual_science=" << actual->science_text.value_or("<hidden>")
            << " actual_stack_hidden=" << !actual->show_info_stack
            << " native_tooltip_fixture=PASS\n";
    }
    auto stale_sources = input.frame_sources;
    stale_sources.quotes = [&](const runtime::LiveSession &, std::uint64_t) {
        auto stale_quotes = quotes;
        stale_quotes.bnw_native_breakdowns.at(0).owner_frame_number = 16;
        return stale_quotes;
    };
    const auto stale_capture = ui::capture_live_session_ui_frame(host.session(), 17, 0, stale_sources);
    const auto *stale_top = stale_capture.find<ui::TopPanelLiveFrame>();
    check(stale_top && !stale_top->players.at(0).native_gold && !stale_top->players.at(0).native_science &&
        !stale_top->players.at(0).available.contains(ui::TopPanelMetric::gold) &&
        !stale_top->players.at(0).available.contains(ui::TopPanelMetric::science),
        "Shared frame consumer retained explicitly stale native quotes");
    const auto *stale_overview = stale_capture.find<ui::OverviewFrameState>();
    check(stale_overview && std::none_of(stale_overview->pages.at("EconomicGeneralInfo").rows.begin(),
        stale_overview->pages.at("EconomicGeneralInfo").rows.end(), [](const ui::OverviewRow &row) {
            return row.label == "NetGoldValue" || row.label == "GrossGoldValue";
        }), "Explicitly stale native gold quote leaked into economic overview");
    // Actual retained presentation consumers; selection is absent until a real
    // selection owner provides one. Only TopPanel data is needed for this probe.
    ui::BnwClientFrameOwner frame_owner(host.session(),host.identity().payload_sha256,0,input.frame_sources);
    const auto retained=frame_owner.capture({64,64,64,64,96},{});
    const auto game_root=std::filesystem::path(root);
    const auto registry=ui::UiContextRegistry::from_documents(
        content::read_xml(game_root/"Assets/DLC/Expansion2/UI/InGame/InGame.xml"),
        content::read_xml(game_root/"Assets/UI/InGame/WorldView/WorldView.xml"));
    ui::LiveUiHost ui_host(registry,frame_owner.ui_provider(),nullptr);
    const auto retained_ui=ui_host.begin_frame(64,64);
    check(retained && frame_owner.is_current(retained) && ui_host.is_current_frame(retained_ui) &&
        ui::ui_owners_current(snapshot),"Actual TopPanel presentation readers are initially stale");
    const auto reject_retained=[&] {
        bool composed=false;
        check(!ui::ui_owners_current(snapshot) && !frame_owner.is_current(retained) &&
            !ui_host.is_current_frame(retained_ui) &&
            !frame_owner.present(retained,[&](const ui::BnwClientFrame &) { composed=true; }) && !composed &&
            frame_owner.dispatch(ui_host,retained_ui,retained,ui::UiOpenScreen{"GameMenu"}).status==
                ui::UiCommandStatus::invalid,
            "Changed native TopPanel input allowed retained frame/presentation/action");
    };
    // Explicit same-generation fault probes deliberately bypass mutable_state's
    // dirty notification. This is not a production mutator or cache publication.
    // The lease points to current allocated state; no replaced address is used.
    const auto fault_lease=host.session().read_state_lease();
    auto &fault=const_cast<runtime::LiveSessionState &>(*fault_lease);
    fault.treasury.players.at(0).connections_times100+=100;
    const auto changed_gold=bridge.capture(host.session(),17);
    check(host.session().state_generation()==bound_generation && host.session().bnw_unhappiness_current() &&
        changed_gold.gold.at(0).net_times100==quotes.gold.at(0).net_times100+100 &&
        !bridge.matches(host.session(),quotes,17),"Unnotified native treasury update retained old gold quote");
    reject_retained();
    fault.treasury.players.at(0).connections_times100-=100;
    check(bridge.matches(host.session(),quotes,17) && frame_owner.is_current(retained),
        "Restored exact native treasury operands remain incorrectly stale");
    auto &science_component=fault.cities.players.at(0).cities.begin()->second.yields.at(fault.cities.science).misc;
    ++science_component;
    const auto changed_science=bridge.capture(host.session(),17);
    check(host.session().state_generation()==bound_generation && host.session().bnw_unhappiness_current() &&
        changed_science.science.at(0).total_times100==quotes.science.at(0).total_times100+100 &&
        !bridge.matches(host.session(),quotes,17),"Unnotified native city update retained old science quote");
    reject_retained();
    --science_component;
    check(bridge.matches(host.session(),quotes,17) && frame_owner.is_current(retained),
        "Restored exact native city operands remain incorrectly stale");
    // Positive wallet balance does not affect either rate quote. The shared
    // captured scalar fence must still reject the stale gold label/tooltip.
    fault.treasury.players.at(0).balance_times100+=100;
    check(bridge.matches(host.session(),quotes,17),"Balance-only probe unexpectedly changed native rate quotes");
    reject_retained();
    fault.treasury.players.at(0).balance_times100-=100;
    check(bridge.matches(host.session(),quotes,17) && frame_owner.is_current(retained) &&
        ui::ui_owners_current(snapshot),"Restored exact scalar scope remains incorrectly stale");
    std::cout << "PASS same-generation native gold/science requery + retained consumers + balance scalar fence\n";
    const auto dead = std::find_if(host.session().state().setup.players.begin(),
        host.session().state().setup.players.end(), [](const auto &entry) { return !entry.second.alive; });
    check(dead != host.session().state().setup.players.end(), "Actual roster has no inactive owner fixture");
    ui::BnwTopPanelPreviewBridge dead_player(host.session(), host.database(), host.identity(), dead->first);
    check(dead_player.capture(host.session(), 18).gold.empty(), "Dead/unowned selected player accepted");
    check(bridge.capture(host.session(), 0).gold.empty(), "Zero frame quote accepted");
    const auto checkpoint = host.session().checkpoint();
    (void)host.session().mutable_state();
    check(bridge.blocker(host.session()) && bridge.capture(host.session(), 18).gold.empty() && !bridge.matches(host.session(),quotes,17), "Dirty live owner accepted");
    host.session().rebuild_bnw_unhappiness_cache(runtime::load_bnw_unhappiness_rules(host.database()), host.defines());
    check(host.session().state_generation()>bound_generation &&
        bridge.blocker(host.session()) && bridge.capture(host.session(),18).gold.empty() &&
        !ui::ui_owners_current(snapshot),
        "Query-cache replacement retained old borrowed source/UI frame");
    // Reacquire only after replacement: no reference into the freed state is used.
    host.session().mutable_state().treasury.religions.push_back(runtime::TreasuryReligion{});
    host.session().rebuild_bnw_unhappiness_cache(runtime::load_bnw_unhappiness_rules(host.database()), host.defines());
    ui::BnwTopPanelPreviewBridge religion_probe(host.session(),host.database(),host.identity(),0);
    const auto hook_blocker=religion_probe.blocker(host.session());
    check(hook_blocker && hook_blocker->find("founder religion Lua")!=std::string::npos &&
        religion_probe.capture(host.session(),18).gold.empty(),
        "Unresolved founder Lua service was treated as disabled");
    host.session().restore(checkpoint);
    check(host.session().state_generation()>bound_generation+1 &&
        bridge.blocker(host.session()) && bridge.capture(host.session(), 19).science.empty() && !bridge.matches(host.session(),quotes,17) &&
        religion_probe.blocker(host.session()) && !ui::ui_owners_current(snapshot),
        "Repeated replacement/possible reused state address accepted");
    ui::BnwTopPanelPreviewBridge rebound(host.session(), host.database(), host.identity(), 0);
    check(!rebound.blocker(host.session()) && rebound.capture(host.session(), 20).gold.contains(0), "Explicit rebind failed");
    std::cout << "PASS actual source -> native queries -> production frame -> EN/RU; lifecycle/hook guards; full turn blocked\n";
}
}
int main(int argc, char **argv) {
    try { if (argc != 3) throw std::runtime_error("Expected real save and installed game root"); run(argv[1], argv[2]); }
    catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
