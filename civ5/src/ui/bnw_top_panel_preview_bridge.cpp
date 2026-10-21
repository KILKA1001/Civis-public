#include "civ5/ui/bnw_top_panel_preview_bridge.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <limits>
#include <stdexcept>
#include <tuple>

namespace civ5::ui {
namespace {
bool same_income(const runtime::TreasuryIncomeQuote &a, const runtime::TreasuryIncomeQuote &b) {
    return a.profile_extensions.values.empty() && b.profile_extensions.values.empty() &&
        std::tie(a.cities,a.diplomacy,a.connections,a.religion,a.traits,a.total_times100)==
        std::tie(b.cities,b.diplomacy,b.connections,b.religion,b.traits,b.total_times100);
}
bool same_expenses(const runtime::MaintenanceQuote &a, const runtime::MaintenanceQuote &b) {
    return a.profile_extensions.values.empty() && b.profile_extensions.values.empty() &&
        std::tie(a.units.free_units,a.units.paid_units,a.units.base_times100,a.units.extra_times100,
            a.units.total,a.units.multiplier,a.units.exponent,a.supply,a.buildings,a.improvements,a.total)==
        std::tie(b.units.free_units,b.units.paid_units,b.units.base_times100,b.units.extra_times100,
            b.units.total,b.units.multiplier,b.units.exponent,b.supply,b.buildings,b.improvements,b.total);
}
bool same_gold(const runtime::TreasuryNetQuote &a, const runtime::TreasuryNetQuote &b) {
    return a.anarchy==b.anarchy && a.net_times100==b.net_times100 && same_income(a.income,b.income) &&
        a.expenses.has_value()==b.expenses.has_value() && (!a.expenses || same_expenses(*a.expenses,*b.expenses));
}
bool same_science(const runtime::ScienceQuote &a, const runtime::ScienceQuote &b) {
    return a.profile_extensions.values.empty() && b.profile_extensions.values.empty() &&
        std::tie(a.anarchy,a.research_agreements_excluded,a.cities,a.religion,a.traits,a.other_players,
            a.happiness,a.agreements,a.budget_deficit,a.total_times100,a.gold_rate_times100)==
        std::tie(b.anarchy,b.research_agreements_excluded,b.cities,b.religion,b.traits,b.other_players,
            b.happiness,b.agreements,b.budget_deficit,b.total_times100,b.gold_rate_times100);
}
bool same_breakdown(const TopPanelNativeBreakdown &a, const TopPanelNativeBreakdown &b) {
    return std::tie(a.city_science_without_trade_times100,a.city_gold_without_trade_times100,
        a.anarchy_turns,a.technology_city_cost_modifier,a.owner_player_id,a.owner_turn,a.owner_frame_number)==
        std::tie(b.city_science_without_trade_times100,b.city_gold_without_trade_times100,
        b.anarchy_turns,b.technology_city_cost_modifier,b.owner_player_id,b.owner_turn,b.owner_frame_number);
}
std::int32_t narrow(std::int64_t value) {
    if (value < std::numeric_limits<std::int32_t>::min() ||
        value > std::numeric_limits<std::int32_t>::max())
        throw std::runtime_error("BNW TopPanel input exceeds int32");
    return static_cast<std::int32_t>(value);
}
std::int64_t anarchy_turns(const runtime::LiveSessionState &state, std::int64_t player) {
    if (!state.world.all_players) throw std::runtime_error("BNW TopPanel roster is absent");
    std::optional<std::int64_t> value;
    for (const auto &owner : state.world.all_players->players) {
        if (owner.saved_player_id != player) continue;
        for (const auto &field : owner.core.fields) {
            if (!field.name || std::string_view(field.name) != "m_iAnarchyNumTurns") continue;
            if (value || field.values.size() != 1 || field.values[0] < 0)
                throw std::runtime_error("BNW TopPanel anarchy owner is ambiguous");
            value = field.values[0];
        }
    }
    if (!value || (*value > 0) != state.treasury.players.at(player).anarchy)
        throw std::runtime_error("BNW TopPanel anarchy owners disagree");
    return *value;
}
}
BnwTopPanelPreviewBridge::BnwTopPanelPreviewBridge(const runtime::LiveSession &session,
    const content::RulesDatabase &database, const runtime::SessionCheckpointIdentity &identity,
    std::int64_t active_player) : immutable_rules_current_(database.immutable_preview_fingerprint_currentness()), session_(&session), state_(&session.state()), state_generation_(session.state_generation()), player_(active_player) {
    if (identity.payload_sha256 != runtime::identify_bytes(session.source().source.bytes).sha256 ||
        identity.rules_sha256 != database.gameplay_fingerprint_sha256())
        throw std::invalid_argument("BNW TopPanel source/rules identity mismatch");
    const auto types = database.cache_info_types();
    if (!types.report().all_succeeded()) throw std::runtime_error("BNW TopPanel Info cache is incomplete");
    city_rules_ = runtime::load_city_yield_rules(database, content::TechnologyProfile::bnw);
    maintenance_rules_ = runtime::load_maintenance_rules(database, content::TechnologyProfile::bnw, types);
    treasury_rules_ = runtime::load_treasury_rules(database, content::TechnologyProfile::bnw, types);
    defines_ = database.cache_global_defines(content::GlobalDefineProfile::bnw);
    if (defines_.errors()) throw std::runtime_error("BNW TopPanel define cache failed");
}
std::optional<std::string> BnwTopPanelPreviewBridge::blocker(const runtime::LiveSession &session) const {
    if (&session != session_ || &session.state() != state_ || session.state_generation()!=state_generation_)
        return "BNW TopPanel borrowed session/state has expired; detach and rebind";
    if(immutable_rules_current_ && !immutable_rules_current_())
        return "BNW TopPanel immutable rules scope is no longer current";
    const auto &state = session.state();
    if (state.setup.profile != content::TechnologyProfile::bnw ||
        state.cities.profile != content::TechnologyProfile::bnw ||
        state.treasury.profile != content::TechnologyProfile::bnw ||
        state.maintenance.profile != content::TechnologyProfile::bnw ||
        state.science.profile != content::TechnologyProfile::bnw)
        return "BNW TopPanel current owner profiles differ";
    const auto player = state.setup.players.find(player_);
    if (player == state.setup.players.end() || !player->second.alive ||
        !state.cities.players.contains(player_) || !state.treasury.players.contains(player_) ||
        !state.maintenance.players.contains(player_) || !state.science.players.contains(player_))
        return "BNW TopPanel active player is not owned/alive";
    if (!session.bnw_unhappiness_current()) return "BNW TopPanel query inputs require current unhappiness";
    // No handler can select a valid religion when the actual owned collection
    // is empty. With religions present, absence of a Lua service is unresolved;
    // it must never be interpreted as proof that scripts are disabled.
    if (!state.treasury.religions.empty()) return "BNW TopPanel founder religion Lua query is unresolved";
    return std::nullopt;
}
LiveSessionUiQuotes BnwTopPanelPreviewBridge::capture(const runtime::LiveSession &session,
    std::uint64_t frame) const {
    LiveSessionUiQuotes result;
    if (!frame || blocker(session)) return result;
    const auto &state = session.state();
    auto maintenance = state.maintenance;
    const runtime::MaintenanceWorld world{state.setup, state.player_research, state.cities, state.technologies};
    try {
        auto gold = runtime::net_gold_times100(state.treasury, world, maintenance,
            treasury_rules_, city_rules_, maintenance_rules_, defines_, nullptr, player_);
        auto science = runtime::science_times100(state.science, world, state.treasury,
            maintenance, treasury_rules_, city_rules_, maintenance_rules_, defines_, nullptr, player_);
        TopPanelNativeBreakdown breakdown;
        for (const auto &[city, unused] : state.cities.players.at(player_).cities) {
            breakdown.city_gold_without_trade_times100 = runtime::wide::add(
                breakdown.city_gold_without_trade_times100, runtime::city_yield_times100(
                    state.cities, city_rules_, defines_, player_, city, state.cities.gold, true).total_times100);
            breakdown.city_science_without_trade_times100 = runtime::wide::add(
                breakdown.city_science_without_trade_times100, runtime::city_yield_times100(
                    state.cities, city_rules_, defines_, player_, city, state.cities.science, true).total_times100);
        }
        if (!state.world.pregame) return {};
        breakdown.technology_city_cost_modifier = narrow(
            state.world.pregame->world.values.integers.at("num_cities_tech_cost_mod"));
        breakdown.anarchy_turns = narrow(anarchy_turns(state, player_));
        breakdown.owner_player_id = player_;
        breakdown.owner_turn = narrow(state.setup.game_turn);
        breakdown.owner_frame_number = frame;
        result.gold.emplace(player_, std::move(gold));
        result.science.emplace(player_, std::move(science));
        result.bnw_native_breakdowns.emplace(player_, breakdown);
    } catch (const runtime::MaintenanceUnavailable &) { return {}; }
      catch (const runtime::TreasuryUnavailable &) { return {}; }
      catch (const runtime::ScienceUnavailable &) { return {}; }
      catch (const runtime::CityYieldUnavailable &) { return {}; }
    return result;
}
bool BnwTopPanelPreviewBridge::matches(const runtime::LiveSession &session,
    const LiveSessionUiQuotes &quotes, std::uint64_t frame) const noexcept {
    try {
        if (!frame || blocker(session)) return false;
        const auto expected_gold=quotes.gold.find(player_);
        const auto expected_science=quotes.science.find(player_);
        const auto expected_breakdown=quotes.bnw_native_breakdowns.find(player_);
        if (expected_gold==quotes.gold.end() || expected_science==quotes.science.end() ||
            expected_breakdown==quotes.bnw_native_breakdowns.end()) return false;
        const auto current=capture(session,frame);
        const auto gold=current.gold.find(player_);
        const auto science=current.science.find(player_);
        const auto breakdown=current.bnw_native_breakdowns.find(player_);
        return gold!=current.gold.end() && science!=current.science.end() &&
            breakdown!=current.bnw_native_breakdowns.end() &&
            same_gold(expected_gold->second,gold->second) && same_science(expected_science->second,science->second) &&
            same_breakdown(expected_breakdown->second,breakdown->second);
    } catch (...) { return false; }
}
} // namespace civ5::ui
