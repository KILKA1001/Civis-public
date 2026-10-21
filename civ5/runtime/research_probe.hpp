#pragma once
#include "civ5/runtime/research_progress.hpp"
#include <array>
#include <charconv>
#include <sstream>

namespace civ5::validation {
struct ResearchProbeInput { std::int64_t player, tech, change, science; };
inline ResearchProbeInput parse_research_probe(std::string_view text) {
    std::array<std::int64_t,4> values{};
    for (std::size_t i=0; i<values.size(); ++i) {
        const auto separator=text.find(',');
        if ((i==values.size()-1) != (separator==std::string_view::npos))
            throw std::invalid_argument("Research probe requires player,tech,change-times100,science-times100");
        const auto part=text.substr(0,separator);
        const auto parsed=std::from_chars(part.data(),part.data()+part.size(),values[i]);
        if (parsed.ec!=std::errc{} || parsed.ptr!=part.data()+part.size())
            throw std::invalid_argument("Research probe requires exact signed 64-bit decimal values");
        if (separator!=std::string_view::npos) text.remove_prefix(separator+1);
    }
    return {values[0],values[1],values[2],values[3]};
}
struct ResearchProbeCompletionRequired : std::runtime_error {
    ResearchProbeCompletionRequired() : std::runtime_error("Full world technology acquisition is not implemented") {}
};
// A diagnostic on a disposable owned copy. It permits partial progress and
// stops at world acquisition; it never pretends to implement technology effects.
struct ResearchProbeWorld final : runtime::ResearchProgressServices {
    std::int64_t science = 0;
    std::vector<const char *> events;
    std::int64_t science_times100(std::int64_t) override { events.push_back("science_query"); return science; }
    void game_data_and_score_dirty(std::int64_t) override { events.push_back("game_data_and_score_dirty"); }
    void acquire_technology(std::int64_t,std::int64_t,std::int64_t) override {
        events.push_back("acquire_technology_required"); throw ResearchProbeCompletionRequired{};
    }
    void city_specializations_dirty(std::int64_t) override { throw std::logic_error("Probe crossed world completion boundary"); }
    std::int64_t culture_per_tech(std::int64_t) override { throw std::logic_error("Probe crossed world completion boundary"); }
    void add_culture(std::int64_t,std::int64_t) override { throw std::logic_error("Probe crossed world completion boundary"); }
};
inline std::string research_probe(const ResearchProbeInput &input, const saves::SavedSessionSnapshot &snapshot,
    const runtime::ResearchContext &setup, const content::RulesDatabase &database,
    const content::ResearchRules &rules, const content::TechnologyRules &techs,
    const content::GlobalDefineCache &defines, const content::CustomModOptions *options,
    const std::vector<runtime::TeamTechnologies> &source_teams) {
    auto copied_setup=setup;
    auto players=runtime::restore_player_research_context(snapshot,copied_setup,defines);
    auto teams=source_teams;
    const auto team_id=copied_setup.players.at(input.player).team;
    const auto find_team=[&]() -> const runtime::TeamTechnologies & {
        for (const auto &team : teams) if (team.team()==team_id) return team;
        throw std::runtime_error("Probe player has no team");
    };
    const auto before=find_team().at(input.tech);
    const auto overflow_before=players.players.at(input.player).overflow_times100;
    const auto cost=runtime::player_research_cost(players,copied_setup,rules,techs,defines,options,teams,input.player,input.tech);
    const auto active_player=snapshot.pregame->archive.integers.at("active_player");
    const auto active_team=copied_setup.players.contains(active_player) ? copied_setup.players.at(active_player).team : -1;
    const bool no_science=copied_setup.game_option_value(database,"GAMEOPTION_NO_SCIENCE")==1;
    ResearchProbeWorld world; world.science=input.science;
    runtime::ResearchProgressContext context{players,copied_setup,rules,techs,defines,options,teams,world,active_team,no_science};
    const char *status="applied";
    try {
        const auto result=runtime::change_research_progress_times100(context,team_id,input.tech,input.change,input.player);
        if (result.skipped) status="skipped";
        else if (!result.changed) status="unchanged";
    } catch (const ResearchProbeCompletionRequired &) { status="completion_requires_world"; }
    const auto after=find_team().at(input.tech);
    std::ostringstream out;
    out << "{\"controlled_input\":true,\"disposable_copy\":true,\"status\":\"" << status
        << "\",\"player\":" << input.player << ",\"team\":" << team_id << ",\"tech\":" << input.tech
        << ",\"change_times100\":" << input.change << ",\"science_times100\":" << input.science
        << ",\"science_is_calculated\":false,\"no_science\":" << (no_science?"true":"false")
        << ",\"active_team\":" << active_team << ",\"threshold_times100\":" << cost.completion_threshold_times100
        << ",\"progress_before\":" << before.progress_times100 << ",\"progress_after\":" << after.progress_times100
        << ",\"overflow_before\":" << overflow_before << ",\"overflow_after\":" << players.players.at(input.player).overflow_times100
        << ",\"known_before\":" << (before.has_tech?"true":"false") << ",\"known_after\":" << (after.has_tech?"true":"false")
        << ",\"no_trade_before\":" << (before.no_trade?"true":"false") << ",\"no_trade_after\":" << (after.no_trade?"true":"false") << ",\"events\":[";
    for (std::size_t i=0; i<world.events.size(); ++i) { if (i) out << ','; out << '"' << world.events[i] << '"'; }
    out << "]}";
    return out.str();
}
} // namespace civ5::validation
