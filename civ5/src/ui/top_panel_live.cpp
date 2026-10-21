#include "civ5/ui/top_panel_live.hpp"
#include "civ5/ui/overview_screens.hpp"
#include <limits>
#include "civ5/runtime/wide_arithmetic.hpp"
#include <stdexcept>

namespace civ5::ui {
namespace {
constexpr const char *unavailable_text = "N/A";

std::string text(const TopPanelTextLocalizer &localizer, std::string_view key,
                 std::vector<std::string> arguments = {}) {
    if (!localizer.text) throw std::runtime_error("TopPanel live localization is unbound");
    return localizer.text(key, arguments);
}

std::string hundredths(std::int64_t value) {
    const bool negative = value < 0;
    const auto magnitude = negative ? std::uint64_t(-(value + 1)) + 1 : std::uint64_t(value);
    auto result = std::to_string(magnitude / 100);
    if (const auto fraction = magnitude % 100) {
        result += '.';
        if (fraction < 10) result += '0';
        result += std::to_string(fraction);
        while (result.back() == '0') result.pop_back();
    }
    return negative ? "-" + result : result;
}
std::int64_t floor_whole(std::int64_t value) { return value / 100 - (value % 100 < 0); }
std::string native_science_tooltip(const TopPanelLiveFrame &live, const TopPanelLivePlayer &player,
    const TopPanelTextLocalizer &localizer) {
    if (live.no_science) return text(localizer, "TXT_KEY_TOP_PANEL_SCIENCE_OFF_TOOLTIP");
    const auto &quote = *player.native_science;
    const auto &extra = *player.native_breakdown;
    std::string result;
    if (!live.no_basic_help) result = text(localizer, "TXT_KEY_TP_SCIENCE",
        {std::to_string(player.values.science_per_turn)}) + "[NEWLINE][NEWLINE]";
    bool first = true;
    const auto source = [&](std::string_view key, std::int64_t value, bool newline = false) {
        if (!value) return;
        if (!first) result += "[NEWLINE]";
        first = false;
        result += text(localizer, key, {hundredths(value)});
        if (newline) result += "[NEWLINE]";
    };
    source("TXT_KEY_TP_SCIENCE_FROM_BUDGET_DEFICIT", quote.budget_deficit, true);
    source("TXT_KEY_TP_SCIENCE_FROM_CITIES", extra.city_science_without_trade_times100);
    source("TXT_KEY_TP_SCIENCE_FROM_ITR", runtime::wide::subtract(quote.cities,
        extra.city_science_without_trade_times100));
    source("TXT_KEY_TP_SCIENCE_FROM_MINORS", quote.other_players);
    source("TXT_KEY_TP_SCIENCE_FROM_HAPPINESS", quote.happiness);
    source("TXT_KEY_TP_SCIENCE_FROM_RESEARCH_AGREEMENTS", quote.agreements);
    if (!live.no_basic_help) result += "[NEWLINE][NEWLINE]" + text(localizer,
        "TXT_KEY_TP_TECH_CITY_COST", {std::to_string(extra.technology_city_cost_modifier)});
    return result;
}
std::string native_gold_tooltip(const TopPanelLiveFrame &live, const TopPanelLivePlayer &player,
    const TopPanelTextLocalizer &localizer) {
    const auto &quote = *player.native_gold;
    const auto &income = quote.income;
    const auto &cost = *quote.expenses;
    const auto &extra = *player.native_breakdown;
    const auto diplomacy_income = std::max<std::int64_t>(income.diplomacy, 0);
    const auto diplomacy_expense = std::max<std::int64_t>(-income.diplomacy, 0);
    const auto total_income = runtime::wide::subtract(income.total_times100,
        std::min<std::int64_t>(income.diplomacy, 0));
    const auto total_expense = runtime::wide::add(cost.total, diplomacy_expense / 100);
    std::string result;
    if (extra.anarchy_turns > 0) result = text(localizer, "TXT_KEY_TP_ANARCHY",
        {std::to_string(extra.anarchy_turns)}) + "[NEWLINE][NEWLINE]";
    if (!live.no_basic_help) result += text(localizer, "TXT_KEY_TP_AVAILABLE_GOLD",
        {std::to_string(player.values.gold)}) + "[NEWLINE][NEWLINE]";
    const auto line = [&](std::string_view key, std::string value) {
        result += "[NEWLINE]  [ICON_BULLET]" + text(localizer, key, {std::move(value)});
    };
    result += "[COLOR:150:255:150:255]+" + text(localizer, "TXT_KEY_TP_TOTAL_INCOME",
        {std::to_string(floor_whole(total_income))});
    line("TXT_KEY_TP_CITY_OUTPUT", hundredths(extra.city_gold_without_trade_times100));
    line("TXT_KEY_TP_GOLD_FROM_CITY_CONNECTIONS", std::to_string(floor_whole(income.connections)));
    line("TXT_KEY_TP_GOLD_FROM_ITR", std::to_string(floor_whole(runtime::wide::subtract(
        income.cities, extra.city_gold_without_trade_times100))));
    if (income.traits > 0) line("TXT_KEY_TP_GOLD_FROM_TRAITS", std::to_string(floor_whole(income.traits)));
    if (diplomacy_income > 0) line("TXT_KEY_TP_GOLD_FROM_OTHERS", hundredths(diplomacy_income));
    if (income.religion > 0) line("TXT_KEY_TP_GOLD_FROM_RELIGION", hundredths(income.religion));
    result += "[/COLOR][NEWLINE][COLOR:255:150:150:255][NEWLINE]-" + text(localizer,
        "TXT_KEY_TP_TOTAL_EXPENSES", {std::to_string(total_expense)});
    if (cost.units.total) line("TXT_KEY_TP_UNIT_MAINT", std::to_string(cost.units.total));
    if (cost.supply) line("TXT_KEY_TP_GOLD_UNIT_SUPPLY", std::to_string(cost.supply));
    if (cost.buildings) line("TXT_KEY_TP_GOLD_BUILDING_MAINT", std::to_string(cost.buildings));
    if (cost.improvements) line("TXT_KEY_TP_GOLD_TILE_MAINT", std::to_string(cost.improvements));
    if (diplomacy_expense > 0) line("TXT_KEY_TP_GOLD_TO_OTHERS", hundredths(diplomacy_expense));
    result += "[/COLOR]";
    if (runtime::wide::add(total_income, runtime::wide::multiply_divide(player.values.gold, 100, 1)) < 0)
        result += "[NEWLINE][COLOR:255:60:60:255]" + text(localizer,
            "TXT_KEY_TP_LOSING_SCIENCE_FROM_DEFICIT") + "[/COLOR]";
    if (!live.no_basic_help) result += "[NEWLINE][NEWLINE]" + text(localizer, "TXT_KEY_TP_GOLD_EXPLANATION");
    return result;
}

bool complete(const TopPanelTradeQuote &quote) {
    return quote.details_resolved && quote.used >= 0 && quote.available >= 0 &&
           quote.idle_land_units >= 0 && quote.idle_sea_units >= 0 &&
           (!quote.idle_land_units || !quote.land_unit_description_key.empty()) &&
           (!quote.idle_sea_units || !quote.sea_unit_description_key.empty());
}
bool complete(const TopPanelTourismQuote &quote) {
    return quote.influence_resolved && quote.per_turn >= 0 && quote.great_works >= 0 &&
           quote.great_work_slots >= quote.great_works && quote.influential_civilizations >= 0 &&
           quote.required_civilizations >= 0;
}
bool complete(const TopPanelFaithQuote &quote) {
    return quote.progression_resolved && quote.balance >= 0 && quote.anarchy_turns >= 0 &&
           !quote.progression_text.empty();
}
bool tourism_rate_known(const TopPanelTourismQuote &quote) {
    return (quote.per_turn_resolved || complete(quote)) && quote.per_turn >= 0;
}
bool tourism_works_known(const TopPanelTourismQuote &quote) {
    return (quote.great_works_resolved || complete(quote)) && quote.great_works >= 0 &&
           quote.great_work_slots >= quote.great_works;
}
bool faith_balance_known(const TopPanelFaithQuote &quote) {
    return (quote.balance_resolved || complete(quote)) && quote.balance >= 0;
}
bool faith_rate_known(const TopPanelFaithQuote &quote) {
    return quote.per_turn_resolved || complete(quote);
}

std::string trade_tooltip(const TopPanelTradeQuote &quote, const TopPanelTextLocalizer &localizer) {
    if (quote.used < 0 || quote.available < 0 || quote.idle_land_units < 0 || quote.idle_sea_units < 0 ||
        (quote.idle_land_units && quote.land_unit_description_key.empty()) ||
        (quote.idle_sea_units && quote.sea_unit_description_key.empty()))
        throw std::runtime_error("Incomplete BNW trade quote");
    std::string result;
    if (quote.idle_land_units)
        result += text(localizer, "TXT_KEY_TOP_PANEL_INTERNATIONAL_TRADE_ROUTES_TT_UNASSIGNED",
                       {std::to_string(quote.idle_land_units),
                        text(localizer, quote.land_unit_description_key)});
    if (quote.idle_sea_units)
        result += text(localizer, "TXT_KEY_TOP_PANEL_INTERNATIONAL_TRADE_ROUTES_TT_UNASSIGNED",
                       // Retail Expansion2 TopPanel.lua passes the land count
                       // in its sea-unit branch. Preserve that UI behavior.
                       {std::to_string(quote.idle_land_units),
                        text(localizer, quote.sea_unit_description_key)});
    if (!result.empty()) result += "[NEWLINE]";
    result += text(localizer, "TXT_KEY_TOP_PANEL_INTERNATIONAL_TRADE_ROUTES_TT",
                   {std::to_string(quote.used), std::to_string(quote.available)});
    if (!quote.established_by_player.empty())
        result += "[NEWLINE][NEWLINE]" + text(localizer, "TXT_KEY_TOP_PANEL_ITR_ESTABLISHED_BY_PLAYER_TT") +
                  "[NEWLINE]" + quote.established_by_player;
    if (!quote.established_by_other.empty())
        result += "[NEWLINE][NEWLINE]" + text(localizer, "TXT_KEY_TOP_PANEL_ITR_ESTABLISHED_BY_OTHER_TT") +
                  "[NEWLINE]" + quote.established_by_other;
    return result;
}

std::string tourism_tooltip(const TopPanelTourismQuote &quote, const TopPanelTextLocalizer &localizer) {
    if (!tourism_works_known(quote))
        throw std::runtime_error("Invalid BNW tourism quote");
    std::string result = text(localizer, "TXT_KEY_TOP_PANEL_TOURISM_TOOLTIP_1",
                              {std::to_string(quote.great_works)}) + "[NEWLINE]" +
                         text(localizer, "TXT_KEY_TOP_PANEL_TOURISM_TOOLTIP_2",
                              {std::to_string(quote.great_work_slots - quote.great_works)});
    if (complete(quote) && quote.cultural_victory_enabled) {
        const auto influential = text(localizer, "TXT_KEY_CO_VICTORY_INFLUENTIAL_OF",
                                      {std::to_string(quote.influential_civilizations),
                                       std::to_string(quote.required_civilizations)});
        result += "[NEWLINE][NEWLINE]" +
                  text(localizer, "TXT_KEY_TOP_PANEL_TOURISM_TOOLTIP_3", {influential});
    }
    return result;
}

std::string faith_tooltip(const TopPanelFaithQuote &quote, const TopPanelTextLocalizer &localizer) {
    if (quote.balance < 0 || quote.anarchy_turns < 0 || quote.progression_text.empty())
        throw std::runtime_error("Incomplete BNW faith quote");
    std::string result;
    if (quote.anarchy_turns)
        result = text(localizer, "TXT_KEY_TP_ANARCHY", {std::to_string(quote.anarchy_turns)}) +
                 "[NEWLINE][NEWLINE]";
    result += text(localizer, "TXT_KEY_TP_FAITH_ACCUMULATED", {std::to_string(quote.balance)}) + "[NEWLINE]";
    bool source = false;
    const auto append = [&](std::string_view key, int amount) {
        if (!amount) return;
        result += "[NEWLINE]" + text(localizer, key, {std::to_string(amount)});
        source = true;
    };
    append("TXT_KEY_TP_FAITH_FROM_CITIES", quote.from_cities);
    append("TXT_KEY_TP_FAITH_FROM_MINORS", quote.from_minors);
    append("TXT_KEY_TP_FAITH_FROM_RELIGION", quote.from_religion);
    if (source) result += "[NEWLINE]";
    result += "[NEWLINE]" + quote.progression_text;
    return result;
}

std::string partial_faith_tooltip(const TopPanelFaithQuote &quote,
                                  const TopPanelTextLocalizer &localizer) {
    std::string result = faith_balance_known(quote)
        ? text(localizer, "TXT_KEY_TP_FAITH_ACCUMULATED", {std::to_string(quote.balance)})
        : unavailable_text;
    if (quote.sources_resolved) {
        bool source = false;
        const auto append = [&](std::string_view key, int amount) {
            if (!amount) return;
            result += "[NEWLINE][NEWLINE]" + text(localizer, key, {std::to_string(amount)});
            source = true;
        };
        append("TXT_KEY_TP_FAITH_FROM_CITIES", quote.from_cities);
        append("TXT_KEY_TP_FAITH_FROM_MINORS", quote.from_minors);
        append("TXT_KEY_TP_FAITH_FROM_RELIGION", quote.from_religion);
        if (source) result += "[NEWLINE]";
    } else {
        result += "[NEWLINE]" + std::string(unavailable_text);
    }
    result += "[NEWLINE][NEWLINE]";
    result += quote.progression_resolved && !quote.progression_text.empty()
        ? quote.progression_text : unavailable_text;
    return result;
}

int whole(std::int64_t value, const char *name) {
    value /= 100;
    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        throw std::runtime_error(std::string("TopPanel live value is outside int range: ") + name);
    return static_cast<int>(value);
}

int checked(std::int64_t value, const char *name) {
    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        throw std::runtime_error(std::string("TopPanel live value is outside int range: ") + name);
    return static_cast<int>(value);
}
} // namespace

TopPanelLiveFrame capture_top_panel_live(
    const runtime::ResearchContext &game, const runtime::CityYieldContext &cities,
    const runtime::TreasuryContext &treasury,
    const std::map<std::int64_t, runtime::ScienceQuote> &science,
    const std::map<std::int64_t, runtime::TreasuryNetQuote> &gold,
    std::optional<std::string> turn_date) {
    if (game.profile != content::TechnologyProfile::bnw ||
        cities.profile != content::TechnologyProfile::bnw ||
        treasury.profile != content::TechnologyProfile::bnw)
        throw std::runtime_error("TopPanel live capture requires matching BNW state");
    TopPanelLiveFrame result;
    result.turn_date = std::move(turn_date);
    for (const auto &[id, player] : game.players) {
        if (!player.alive) continue;
        const auto city = cities.players.find(id);
        const auto wallet = treasury.players.find(id);
        if (city == cities.players.end() || wallet == treasury.players.end()) continue;
        auto &out = result.players[id];
        if (city->second.cities.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
            throw std::runtime_error("TopPanel live city count exceeds int range");
        out.city_count = static_cast<int>(city->second.cities.size());
        out.values.city_count = *out.city_count;
        out.values.gold = whole(wallet->second.balance_times100, "gold");
        out.gold_balance = out.values.gold;
        if (const auto quote = gold.find(id); quote != gold.end()) {
            out.native_gold = quote->second;
            out.values.gold_per_turn = whole(quote->second.net_times100, "gold_per_turn");
            out.values.gold_from_cities_times100 = checked(quote->second.income.cities, "gold_from_cities");
            out.values.gold_per_turn_from_diplomacy = whole(quote->second.income.diplomacy, "diplomacy");
            out.values.city_connection_gold_times100 = checked(quote->second.income.connections, "connections");
            if (quote->second.expenses) {
                out.values.unit_cost = checked(quote->second.expenses->units.total, "unit_cost");
                out.values.unit_supply = checked(quote->second.expenses->supply, "unit_supply");
                out.values.building_maintenance = checked(quote->second.expenses->buildings, "buildings");
                out.values.improvement_maintenance = checked(quote->second.expenses->improvements, "improvements");
                out.available.insert(TopPanelMetric::gold);
            }
        }
        if (const auto quote = science.find(id); quote != science.end() && quote->second.gold_rate_times100) {
            out.native_science = quote->second;
            out.values.science_per_turn = whole(quote->second.total_times100, "science_per_turn");
            out.values.science_from_budget_deficit_times100 = checked(quote->second.budget_deficit, "budget_deficit");
            out.values.science_from_cities_times100 = checked(quote->second.cities, "science_cities");
            out.values.science_from_other_players_times100 = checked(quote->second.other_players, "science_players");
            out.values.science_from_happiness_times100 = checked(quote->second.happiness, "science_happiness");
            out.values.science_from_research_agreements_times100 = checked(quote->second.agreements, "agreements");
            if (!out.available.contains(TopPanelMetric::gold))
                out.values.gold_per_turn = whole(*quote->second.gold_rate_times100, "gold_rate_for_science");
            out.available.insert(TopPanelMetric::science);
        }
    }
    return result;
}

std::optional<TopPanelPresentation> present_top_panel_frame(
    const UiFrameSnapshot &frame, const TopPanelTextLocalizer &localizer) {
    const auto *live = frame.find<TopPanelLiveFrame>();
    if (!live || !frame.active_player_id()) return std::nullopt;
    const auto it = live->players.find(*frame.active_player_id());
    if (it == live->players.end() || !it->second.city_count) return std::nullopt;

    const auto &player = it->second;
    TopPanelState state;
    state.player = player.values;
    state.player->city_count = *player.city_count;
    state.expansion2_ui = live->expansion2_ui;
    state.no_science = live->no_science;
    state.no_happiness = live->no_happiness;
    state.no_policies = live->no_policies;
    state.no_religion = live->no_religion;
    state.no_basic_help = live->no_basic_help;
    state.head_selected_city = live->head_selected_city;
    state.city_screen_up = live->city_screen_up;
    state.game_turn = frame.turn().value_or(0);
    state.turn_date = live->turn_date.value_or(unavailable_text);
    state.available_metrics = player.available;
    // BNW values require an explicit quote for each independently displayed
    // field. A stale/general availability flag cannot turn default-zero
    // values into a successful display after an active-player transition.
    state.available_metrics->erase(TopPanelMetric::trade);
    state.available_metrics->erase(TopPanelMetric::tourism);
    state.available_metrics->erase(TopPanelMetric::faith);
    const bool trade_complete = player.trade && complete(*player.trade) &&
        player.outgoing_trade_routes && frame.turn() &&
        player.trade->owner_player_id == *frame.active_player_id() &&
        player.trade->owner_turn == *frame.turn() &&
        player.trade->owner_frame_number == frame.frame() &&
        player.trade->used == *player.outgoing_trade_routes;
    if (trade_complete) {
        state.player->international_trade_routes_used = player.trade->used;
        state.player->international_trade_routes_available = player.trade->available;
        state.available_metrics->insert(TopPanelMetric::trade);
    }
    if (player.tourism && tourism_rate_known(*player.tourism)) {
        state.player->tourism = player.tourism->per_turn;
        state.available_metrics->insert(TopPanelMetric::tourism);
    }
    if (player.faith && faith_balance_known(*player.faith) &&
        faith_rate_known(*player.faith) && !live->no_religion) {
        state.player->faith = player.faith->balance;
        state.player->faith_per_turn = player.faith->per_turn;
        state.available_metrics->insert(TopPanelMetric::faith);
    }
    if (live->no_religion) state.available_metrics->insert(TopPanelMetric::faith);
    const bool native_current = player.native_breakdown && frame.turn() &&
        player.native_breakdown->owner_player_id == *frame.active_player_id() &&
        player.native_breakdown->owner_turn == *frame.turn() &&
        player.native_breakdown->owner_frame_number == frame.frame();
    if (player.native_breakdown && !native_current) {
        state.available_metrics->erase(TopPanelMetric::gold);
        state.available_metrics->erase(TopPanelMetric::science);
    }
    auto result = present_top_panel(state, localizer);
    if (!result) return result;
    if (result->show_info_stack && native_current) {
        if (player.native_gold && player.native_gold->expenses &&
            state.available_metrics->contains(TopPanelMetric::gold))
            result->gold_tooltip = native_gold_tooltip(*live, player, localizer);
        if (player.native_science && state.available_metrics->contains(TopPanelMetric::science))
            result->science_tooltip = native_science_tooltip(*live, player, localizer);
    }
    if (result->show_info_stack && player.gold_balance &&
        !state.available_metrics->contains(TopPanelMetric::gold)) {
        // The retail label requires a per-turn rate. Show the independently
        // known balance and mark that rate unavailable instead of inventing a
        // zero rate or a complete income/expense tooltip.
        result->gold_text = "[ICON_GOLD][COLOR:255:235:0:255]" +
                            std::to_string(*player.gold_balance) + "[/COLOR] (N/A)";
        result->gold_tooltip = text(localizer, "TXT_KEY_TP_AVAILABLE_GOLD",
                                    {std::to_string(*player.gold_balance)}) +
                               "[NEWLINE]N/A";
    }
    if (result->show_info_stack && live->expansion2_ui) {
        if (trade_complete)
            result->international_trade_routes_tooltip = trade_tooltip(*player.trade, localizer);
        else if (player.outgoing_trade_routes) {
            // The live route list proves the numerator independently of the
            // maximum capacity and Lua-derived route/unit tooltip details.
            const auto used = std::to_string(*player.outgoing_trade_routes);
            result->international_trade_routes_text = text(localizer,
                "TXT_KEY_TOP_PANEL_INTERNATIONAL_TRADE_ROUTES", {used, unavailable_text});
            result->international_trade_routes_tooltip = text(localizer,
                "TXT_KEY_TOP_PANEL_INTERNATIONAL_TRADE_ROUTES_TT", {used, unavailable_text});
            // The overview is captured at this same frame boundary. Names are
            // safe to display only when every outgoing route has two uniquely
            // matched live cities and agrees with the owned route count.
            const auto *overview = frame.find<OverviewFrameState>();
            if (overview && overview->player_id == *frame.active_player_id() &&
                overview->named_outgoing_trade_routes &&
                overview->named_outgoing_trade_routes->size() ==
                    static_cast<std::size_t>(*player.outgoing_trade_routes) &&
                !overview->named_outgoing_trade_routes->empty()) {
                *result->international_trade_routes_tooltip += "[NEWLINE][NEWLINE]" +
                    text(localizer, "TXT_KEY_TOP_PANEL_ITR_ESTABLISHED_BY_PLAYER_TT");
                for (const auto &route : *overview->named_outgoing_trade_routes)
                    *result->international_trade_routes_tooltip += "[NEWLINE]" + route;
            }
            if (overview && overview->player_id == *frame.active_player_id() &&
                overview->named_incoming_trade_routes &&
                !overview->named_incoming_trade_routes->empty()) {
                *result->international_trade_routes_tooltip += "[NEWLINE][NEWLINE]" +
                    text(localizer, "TXT_KEY_TOP_PANEL_ITR_ESTABLISHED_BY_OTHER_TT");
                for (const auto &route : *overview->named_incoming_trade_routes)
                    *result->international_trade_routes_tooltip += "[NEWLINE]" + route;
            }
        }
        if (player.tourism && tourism_works_known(*player.tourism))
            result->tourism_tooltip = tourism_tooltip(*player.tourism, localizer);
        else if (player.tourism && tourism_rate_known(*player.tourism))
            result->tourism_tooltip = unavailable_text;
        if (player.faith && !live->no_religion &&
            (faith_balance_known(*player.faith) || faith_rate_known(*player.faith) ||
             player.faith->sources_resolved || player.faith->progression_resolved)) {
            if (!faith_balance_known(*player.faith) || !faith_rate_known(*player.faith)) {
                result->faith_text = "[ICON_PEACE]" +
                    (faith_balance_known(*player.faith)
                        ? std::to_string(player.faith->balance) : unavailable_text) +
                    " (" + (faith_rate_known(*player.faith)
                        ? "+" + std::to_string(player.faith->per_turn)
                        : std::string(unavailable_text)) + ")";
            }
            result->faith_tooltip = complete(*player.faith)
                ? faith_tooltip(*player.faith, localizer)
                : partial_faith_tooltip(*player.faith, localizer);
        }
    }
    if (!frame.turn()) result->current_turn_text = unavailable_text;
    return result;
}

UiCommandResult dispatch_top_panel_action(TopPanelAction action, UiScreenController &controller,
                                           const UiFrameSnapshot &frame) {
    switch (action) {
    case TopPanelAction::open_civilopedia:
        return controller.dispatch(UiOpenScreen{"CivilopediaScreen"}, frame);
    case TopPanelAction::open_game_menu:
        return controller.dispatch(UiOpenScreen{"GameMenu"}, frame);
    case TopPanelAction::exit_city_screen:
        return controller.dispatch(UiCloseScreen{"CityView"}, frame);
    case TopPanelAction::open_policy_choice:
        return controller.dispatch(UiOpenScreen{"SocialPolicyPopup"}, frame);
    case TopPanelAction::open_technology_tree:
        return controller.dispatch(UiOpenScreen{"TechTree"}, frame);
    }
    return {UiCommandStatus::invalid, "Unknown TopPanel action"};
}

} // namespace civ5::ui
