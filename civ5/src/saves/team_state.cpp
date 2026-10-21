#include "civ5/saves/vp_team_profile.hpp"
#include <stdexcept>
#include <string_view>
#include <unordered_map>

namespace civ5::saves {
namespace {
struct TeamReader {
    runtime::BinaryReader &in;
    GameStateBudget budget;
    bool vp;
    const TeamWireDimensions &d;
    const VpTeamContentDimensions *vp_rows;
    void claim(std::uint64_t count, std::uint64_t bytes = 0) {
        if (count > budget.elements || (bytes && count > in.remaining() / bytes))
            throw std::runtime_error("Team state exceeds input or element budget");
        budget.elements -= count;
    }
    std::int64_t number(char type) {
        if (type == 'i')
            return in.i32();
        if (type == 'I')
            return in.u32();
        const auto n = in.u8();
        if (type == '?' && n > 1)
            throw std::runtime_error("Unverified team boolean");
        return n;
    }
    void scalar(TeamObservation &t, const char *name, std::int64_t value) {
        claim(1);
        t.fields.push_back({name, value});
    }
    void fields(TeamObservation &t, char type, std::initializer_list<const char *> names) {
        for (auto name : names)
            scalar(t, name, number(type));
    }
    std::vector<std::int64_t> values(std::uint64_t n, char type = 'i') {
        claim(n, (type == 'i' || type == 'I') ? 4 : 1);
        std::vector<std::int64_t> v;
        v.reserve(static_cast<std::size_t>(n));
        for (std::uint64_t i = 0; i < n; ++i)
            v.push_back(number(type));
        return v;
    }
    void array(TeamObservation &t, const char *name, std::uint64_t rows, char type = 'i',
               std::uint64_t columns = 1) {
        if (!columns || rows > budget.elements / columns)
            throw std::runtime_error("Team array dimensions exceed budget");
        t.arrays.push_back({name, columns, values(rows * columns, type)});
    }
    void counted(TeamObservation &t, const char *name, const char *count_name, char type = 'i',
                 bool sentinel = true) {
        const auto n = in.u32();
        scalar(t, count_name, n);
        array(t, name, (sentinel && n == UINT32_MAX) ? 0 : n, type);
    }
    TeamHashedArray hashed(const char *name, char type = 'i', bool signed_count = false,
                           std::uint64_t width = 1) {
        TeamHashedArray a{};
        a.name = name;
        a.wire_count = signed_count ? number('i') : number('I');
        const auto n = a.wire_count > 0 ? static_cast<std::uint64_t>(a.wire_count) : 0;
        claim(n, 4);
        a.rows.reserve(static_cast<std::size_t>(n));
        for (std::uint64_t i = 0; i < n; ++i) {
            TeamHashedValues row;
            row.type_hash = in.u32();
            if (row.type_hash)
                row.values = values(width, type);
            a.rows.push_back(std::move(row));
        }
        return a;
    }
    void common_prefix(TeamObservation &t) {
        if (vp) {
            counted(t, "members", "members_wire_count");
            fields(t, 'i',
                   {"alive_count",
                    "ever_alive_count",
                    "cities",
                    "population",
                    "land",
                    "nuke_interception",
                    "extra_water_see_from",
                    "map_trading",
                    "tech_trading",
                    "gold_trading",
                    "embassy_trading",
                    "open_borders_trading",
                    "defensive_pact_trading",
                    "research_agreement_trading",
                    "city_working_change",
                    "automaton_workers_change",
                    "bridge_building",
                    "city_less_embark_cost",
                    "city_no_embark_cost",
                    "water_work",
                    "river_trade",
                    "border_obstacle",
                    "victory_points",
                    "embarked_extra_moves",
                    "can_embark",
                    "defensive_embark",
                    "embarked_all_water",
                    "natural_wonders",
                    "happiness_from_improvements",
                    "best_route",
                    "minor_civs_attacked",
                    "building_defense_modifier"});
            fields(t, '?', {"map_centering", "has_world_congress_tech"});
        } else {
            const auto version = in.u32();
            scalar(t, "version", version);
            if (version != 1)
                throw std::runtime_error("Unverified BNW team version");
            fields(t, 'i',
                   {"members_count",
                    "alive_count",
                    "ever_alive_count",
                    "cities",
                    "population",
                    "land",
                    "nuke_interception",
                    "extra_water_see_from",
                    "map_trading",
                    "tech_trading",
                    "gold_trading",
                    "embassy_trading",
                    "open_borders_trading",
                    "defensive_pact_trading",
                    "research_agreement_trading",
                    "trade_agreement_trading",
                    "permanent_alliance_trading",
                    "bridge_building",
                    "water_work",
                    "river_trade",
                    "border_obstacle",
                    "victory_points",
                    "embarked_extra_moves",
                    "can_embark",
                    "defensive_embark",
                    "embarked_all_water",
                    "natural_wonders",
                    "best_route",
                    "minor_civs_attacked"});
            fields(t, '?',
                   {"map_centering", "broken_peace_treaty", "united_nations", "has_world_congress_tech",
                    "broken_military_promise", "broken_expansion_promise", "broken_border_promise",
                    "broken_city_state_promise"});
        }
        fields(t, 'i', {"id", "era", "liberated_by", "killed_by"});
        const auto slots = vp ? d.fixed_teams : d.teams;
        array(t, "tech_share", slots);
        if (!vp)
            array(t, "turns_at_war", slots);
        array(t, "turns_locked_at_war", slots);
        array(t, "extra_moves", d.domains);
        if (vp)
            array(t, "vote_eligibility", vp_rows->vote_sources);
        else
            t.hashes.push_back(hashed("vote_eligibility"));
        array(t, "turn_made_peace", slots);
        array(t, "ignore_warning", slots);
        if (vp) {
            for (auto name : {"has_met", "found_player_territory", "at_war_initial", "aggressor_initial",
                              "permanent_war_peace", "embassy", "open_borders", "defensive_pact",
                              "research_agreement", "force_peace", "won_latest_war"})
                array(t, name, std::string_view(name) == "found_player_territory" ? d.fixed_players : slots,
                      'B');
        } else {
            for (auto name :
                 {"has_met", "found_player_territory", "at_war", "permanent_war_peace", "embassy",
                  "open_borders", "defensive_pact", "research_agreement", "trade_agreement", "force_peace"})
                array(t, name, std::string_view(name) == "found_player_territory" ? d.players : slots, '?');
        }
    }
    void vp_projects(TeamObservation &t) {
        const auto &c = *vp_rows;
        array(t, "can_launch", c.victories, '?');
        array(t, "victory_achieved", c.victories, '?');
        array(t, "small_award_achieved", c.small_awards, '?');
        array(t, "trade_tech", c.techs, '?');
        array(t, "route_change", c.routes);
        array(t, "build_time_change", c.builds);
        array(t, "project_count", c.projects);
        array(t, "project_default_art", c.projects);
        claim(c.projects, 4);
        for (std::uint64_t i = 0; i < c.projects; ++i) {
            TeamProjectArt p;
            p.key = static_cast<std::int64_t>(i);
            p.wire_count = in.u32();
            p.source_count = static_cast<std::int64_t>(*p.wire_count);
            p.values = values(*p.wire_count == UINT32_MAX ? 0 : *p.wire_count);
            t.project_art.push_back(std::move(p));
        }
        array(t, "project_making", c.projects);
        array(t, "unit_class_count", c.unit_classes);
        array(t, "building_class_count", c.building_classes);
        array(t, "obsolete_building_count", c.buildings);
        array(t, "terrain_trade", c.terrains);
        array(t, "victory_countdown", c.victories);
        array(t, "turn_team_met", d.fixed_teams);
        array(t, "turn_war_started", d.fixed_teams);
    }
    void bnw_projects(TeamObservation &t) {
        for (auto name : {"can_launch", "victory_achieved", "small_award_achieved"})
            t.hashes.push_back(hashed(name, '?'));
        t.hashes.push_back(hashed("route_change"));
        t.hashes.push_back(hashed("build_time_change"));
        auto counts = hashed("project_count");
        std::unordered_map<std::uint64_t, std::int64_t> by_hash;
        for (const auto &r : counts.rows)
            if (r.type_hash)
                by_hash[r.type_hash] = r.values[0];
        t.hashes.push_back(std::move(counts));
        t.hashes.push_back(hashed("project_default_art"));
        const auto raw = in.i32();
        scalar(t, "project_art_wire_count", raw);
        const auto n = raw > 0 ? static_cast<std::uint64_t>(raw) : 0;
        claim(n, 4);
        for (std::uint64_t i = 0; i < n; ++i) {
            TeamProjectArt p;
            p.key = in.u32();
            const auto at = by_hash.find(static_cast<std::uint64_t>(p.key));
            if (at == by_hash.end())
                throw std::runtime_error("Missing saved project count for BNW art hash");
            p.source_count = at->second;
            p.values = values(p.source_count > 0 ? static_cast<std::uint64_t>(p.source_count) : 0);
            t.project_art.push_back(std::move(p));
        }
        t.hashes.push_back(hashed("project_making"));
        for (auto name : {"unit_class_count", "building_class_count", "obsolete_building_count"})
            t.hashes.push_back(hashed(name, 'i', true));
        t.hashes.push_back(hashed("terrain_trade"));
        t.hashes.push_back(hashed("victory_countdown"));
        array(t, "turn_team_met", d.civ_teams);
    }
    void techs(TeamObservation &t) {
        const auto version = in.u32();
        scalar(t, "tech_version", version);
        if (version != 1)
            throw std::runtime_error("Unverified team technology version");
        if (vp) {
            const auto mod = in.u32(), sentinel = in.u32();
            scalar(t, "tech_mod_version", mod);
            scalar(t, "tech_sentinel", sentinel);
            if (mod != 151 || sentinel != 0xdeadbeef)
                throw std::runtime_error("Unverified VP team technology prefix");
        }
        fields(t, 'i', {"last_tech"});
        if (vp)
            fields(t, 'i', {"num_techs"});
        const auto n = in.i32();
        scalar(t, "saved_techs", n);
        if (n < 0)
            throw std::runtime_error("Negative saved technology count");
        array(t, "tech_hashes", static_cast<std::uint64_t>(n), 'I');
        array(t, "has_tech", static_cast<std::uint64_t>(n), '?');
        array(t, "no_trade_tech", static_cast<std::uint64_t>(n), '?');
        array(t, vp ? "research_progress_times100" : "research_progress", static_cast<std::uint64_t>(n));
        if (vp)
            array(t, "eureka_counter", static_cast<std::uint64_t>(n));
        array(t, "tech_count", static_cast<std::uint64_t>(n));
    }
    void vp_tail(TeamObservation &t) {
        const auto &c = *vp_rows;
        array(t, "trade_route_domain_range", d.domains);
        array(t, "feature_yields", c.features, 'i', d.yields);
        array(t, "terrain_yields", c.terrains, 'i', d.yields);
        if (d.yields > budget.elements / 3)
            throw std::runtime_error("Team improvement yield width exceeds budget");
        array(t, "improvement_yields", c.improvements, 'i', 3 * d.yields);
        counted(t, "revealed_resources", "revealed_resources_wire_count");
        fields(t, 'i', {"vassalage_trading", "master"});
        fields(t, '?', {"voluntary_vassal"});
        fields(t, 'i', {"turns_vassal", "cities_when_vassal", "population_when_vassal"});
        array(t, "turns_since_vassal_ended", d.fixed_teams);
        array(t, "turns_since_vassal_tax_set", d.major);
        array(t, "vassal_tax", d.major);
        fields(t, 'i', {"corporations_enabled"});
        array(t, "at_war_final", d.fixed_teams, 'B');
        array(t, "aggressor_final", d.fixed_teams, 'B');
        fields(t, '?', {"can_build_ocean_crossing_unit"});
    }
    TeamObservation team(std::uint64_t index) {
        TeamObservation t;
        t.index = index;
        t.start_offset = in.position();
        common_prefix(t);
        if (vp)
            vp_projects(t);
        else
            bnw_projects(t);
        techs(t);
        if (vp)
            vp_tail(t);
        else {
            for (auto name : {"improvement_yields", "improvement_dry_yields", "improvement_fresh_yields"})
                t.hashes.push_back(hashed(name, 'i', true, d.yields));
            counted(t, "revealed_resources", "revealed_resources_wire_count", 'I', false);
        }
        t.end_offset = in.position();
        return t;
    }
};
} // namespace
TeamsObservation observe_teams(runtime::BinaryReader &input, GameStateLayout layout, TeamWireDimensions d,
                               GameStateBudget budget) {
    if (layout != GameStateLayout::vp542_corpus && layout != GameStateLayout::expansion2_version1)
        throw std::runtime_error("Unverified team layout");
    const bool vp = layout == GameStateLayout::vp542_corpus;
    const auto *vp_rows = vp_team_dimensions(d);
    if (vp != (vp_rows != nullptr))
        throw std::runtime_error("Team content dimensions do not match source layout");
    TeamReader reader{input, budget, vp, d, vp_rows};
    reader.claim(d.teams, 4);
    TeamsObservation result{};
    result.layout = layout;
    result.dimensions = d;
    result.start_offset = input.position();
    result.teams.reserve(static_cast<std::size_t>(d.teams));
    for (std::uint64_t i = 0; i < d.teams; ++i)
        result.teams.push_back(reader.team(i));
    result.end_offset = input.position();
    return result;
}
} // namespace civ5::saves
