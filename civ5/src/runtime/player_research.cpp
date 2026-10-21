#include "civ5/runtime/player_research.hpp"
#include "civ5/runtime/vp_player_research_profile.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::runtime {
namespace {
using content::TechnologyProfile;
template <class T> const T &named(const std::vector<T> &fields, std::string_view name) {
    const T *found = nullptr;
    for (const auto &field : fields) if (field.name && std::string_view(field.name) == name) {
        if (found) throw std::runtime_error("Duplicate player research field: " + std::string(name));
        found = &field;
    }
    if (!found) throw std::runtime_error("Missing player research field: " + std::string(name));
    return *found;
}
template <class T> std::int64_t scalar(const std::vector<T> &fields, std::string_view name) {
    const auto &v = named(fields, name).values;
    if (v.size() != 1) throw std::runtime_error("Invalid player research scalar: " + std::string(name));
    return v.front();
}
std::int64_t size64(std::size_t size) {
    if (size > INT64_MAX) throw std::overflow_error("Research collection exceeds int64");
    return static_cast<std::int64_t>(size);
}
std::int64_t define(const content::GlobalDefineCache &defines, std::string_view name) {
    const auto *v = defines.find(name);
    if (!v || !v->lookup.error.empty() || !std::holds_alternative<std::int64_t>(v->lookup.value))
        throw std::runtime_error("Unresolved player research define: " + std::string(name));
    return std::get<std::int64_t>(v->lookup.value);
}
const TeamTechnologies &team_techs(std::span<const TeamTechnologies> teams, std::int64_t id) {
    const TeamTechnologies *found = nullptr;
    for (const auto &t : teams) if (t.team() == id) {
        if (found) throw std::runtime_error("Duplicate research team");
        found = &t;
    }
    if (!found) throw std::runtime_error("Missing research team");
    return *found;
}
void validate(const PlayerResearchContext &state, const ResearchContext &base, const content::ResearchRules &rules,
    const content::TechnologyRules &techs, const content::GlobalDefineCache &defines,
    const content::CustomModOptions *options, std::span<const TeamTechnologies> teams) {
    const bool vp = base.profile == TechnologyProfile::vp542;
    if (state.profile != base.profile || rules.profile != base.profile || techs.profile != base.profile ||
        defines.profile() != (vp ? content::GlobalDefineProfile::vp542 : content::GlobalDefineProfile::bnw) ||
        !rules.all_succeeded() || !techs.all_succeeded() || (vp && (!options || !options->all_succeeded())))
        throw std::runtime_error("Player research requires matching complete rules");
    for (const auto &team : teams)
        if (team.profile() != base.profile || team.slots() != techs.collection.slots)
            throw std::runtime_error("Player research team technology profile/span mismatch");
}
bool member(const std::vector<std::int64_t> &members, std::int64_t id) {
    return std::find(members.begin(), members.end(), id) != members.end();
}
} // namespace
PlayerResearchContext restore_player_research_context(const saves::SavedSessionSnapshot &snapshot,
    const ResearchContext &base, const content::GlobalDefineCache &defines) {
    const bool vp = base.profile == TechnologyProfile::vp542;
    const auto layout = vp ? saves::GameStateLayout::vp542_corpus : saves::GameStateLayout::expansion2_version1;
    if (!snapshot.pregame || !snapshot.teams || !snapshot.all_players ||
        snapshot.game_state.layout != layout || snapshot.pregame->layout != layout ||
        snapshot.teams->layout != layout || snapshot.all_players->layout != layout ||
        defines.profile() != (vp ? content::GlobalDefineProfile::vp542 : content::GlobalDefineProfile::bnw))
        throw std::runtime_error("Player research needs matching saved session and define profile");
    PlayerResearchContext result;
    result.profile = base.profile;
    result.world_city_modifier = snapshot.pregame->world.values.integers.at("num_cities_tech_cost_mod");
    // These are compile-time source domains, not database Defines. Reuse the
    // selected wire contract; do not impose it on subsequently owned worlds.
    const auto &wire = snapshot.teams->dimensions;
    if (wire.civ_teams > INT64_MAX || wire.major > INT64_MAX || wire.players > INT64_MAX ||
        wire.major > wire.players || wire.civ_teams >= wire.teams)
        throw std::runtime_error("Invalid source research roster domains");
    result.barbarian_team = static_cast<std::int64_t>(wire.civ_teams);
    const auto civ_team_count = static_cast<std::int64_t>(wire.civ_teams);
    const auto major_count = static_cast<std::int64_t>(wire.major), player_count = static_cast<std::int64_t>(wire.players);
    const auto &minor_flags = snapshot.pregame->archive.flag_arrays.at("minor_nation_civs");
    for (const auto &saved : snapshot.all_players->players) {
        const auto id = saved.saved_player_id;
        if (saved.core.layout != layout || !base.players.contains(id) || id < 0 || static_cast<std::uint64_t>(id) >= minor_flags.size())
            throw std::runtime_error("Player research roster is not bound to saved setup");
        PlayerResearchState p;
        p.pregame_minor = minor_flags[static_cast<std::size_t>(id)];
        p.overflow_times100 = scalar(saved.core.fields, "m_iOverflowResearch");
        p.capital = scalar(saved.core.fields, "m_iCapitalCityID");
        p.queue = named(named(saved.administration.blocks, "pre_city").arrays, "m_researchQueue").values;
        if (vp) {
            auto &vp_state=p.profile_extensions.emplace<VpPlayerResearchProfileState>();
            vp_state.city_cost_modifier = scalar(saved.core.fields, "m_iTechCostXCitiesModifier");
            vp_state.cs_allies = scalar(saved.core.fields, "m_iCSAllies");
            vp_state.science_from_league = scalar(saved.core.fields, "m_iScienceRateFromLeague");
            for (const auto &child : named(saved.espionage.blocks, "player_espionage").children)
                if (std::string_view(child.name) == "spy") ++vp_state.spies;
        } else if (!saved.core.version || *saved.core.version >= 6)
            p.max_effective_cities = scalar(saved.core.fields, "m_iMaxEffectiveCities");
        // Earlier BNW versions retain the constructor's maximum of one. Its
        // ignore-city flag is not serialized and also retains reset(false).
        for (const auto &saved_city : saved.cities.cities) {
            const auto city = scalar(saved_city.core.fields, "m_iID");
            ResearchCityState c;
            c.puppet = scalar(saved_city.core.fields, "m_bPuppet") != 0;
            c.razing = scalar(saved_city.core.fields, "m_iRazingTurns") > 0;
            if (vp) {
                c.ignore_happiness = scalar(saved_city.core.fields, "m_bIgnoreCityForHappiness") != 0;
                auto &vp_city=ensure_vp_research_city_profile(c);
                const auto &spies = named(named(saved_city.tail.blocks, "espionage").children, "m_aiSpyAssignment").values;
                for (std::size_t i = 0; i < spies.size(); ++i)
                    if (spies[i] != -1) vp_city.assigned_spy_players.insert(size64(i));
            }
            if (!p.cities.emplace(city, std::move(c)).second) throw std::runtime_error("Duplicate research city ID");
        }
        if (!result.players.emplace(id, std::move(p)).second) throw std::runtime_error("Duplicate research player ID");
        if (id < major_count) result.major_players.insert(id);
        if (id >= major_count && id < player_count) result.bnw_minor_scan_players.insert(id);
    }
    for (const auto &saved : snapshot.teams->teams) {
        const auto id = named(saved.fields, "id").value;
        if (!base.team_members.contains(id)) throw std::runtime_error("Research team is not bound to setup");
        ResearchTeamState t;
        t.alive_count = named(saved.fields, "alive_count").value;
        t.liberated_by = named(saved.fields, "liberated_by").value;
        if (vp) {
            t.master = named(saved.fields, "master").value;
            t.members = named(saved.arrays, "members").values;
        }
        const auto flags = [&](const char *name, auto &set) {
            const auto &v = named(saved.arrays, name).values;
            // Original VP fixed arrays include uninitialized tail beyond the
            // wire team's roster. Only imported team IDs are live relations.
            for (const auto &[other, count] : base.team_members) {
                if (other < 0 || static_cast<std::uint64_t>(other) >= v.size())
                    throw std::runtime_error("Research relation is outside saved team array");
                if (v[static_cast<std::size_t>(other)] != 0) set.insert(other);
            }
        };
        flags("has_met", t.met); flags("embassy", t.embassy);
        flags(vp ? "at_war_final" : "at_war", t.at_war);
        if (!result.teams.emplace(id, std::move(t)).second) throw std::runtime_error("Duplicate research team ID");
        if (id >= 0 && id < civ_team_count) result.civ_teams.insert(id);
    }
    if (result.players.size() != base.players.size() || result.teams.size() != base.team_members.size())
        throw std::runtime_error("Player research roster does not match base context");
    for (const auto &saved : snapshot.leagues.leagues) {
        ResearchLeagueState l;
        for (const auto &m : saved.members) l.members.push_back(m.player);
        for (const auto &r : saved.active_resolutions)
            l.discovered_tech_modifiers.push_back(named(r.effects.integers, "member_discovered_tech_mod").value);
        result.leagues.push_back(std::move(l));
    }
    return result;
}
bool PlayerResearchContext::team_is_minor(const ResearchContext &base, std::int64_t team) const {
    if (profile == TechnologyProfile::vp542) {
        for (auto id : teams.at(team).members) if (players.at(id).pregame_minor) return true;
    } else {
        // BNW stops at the first alive matching player in its minor-slot loop,
        // including an early false for a matching non-minor player.
        for (auto id : bnw_minor_scan_players) {
            const auto &p = base.players.at(id);
            if (p.alive && p.team == team) return players.at(id).pregame_minor;
        }
    }
    return false;
}
bool PlayerResearchContext::team_is_major(const ResearchContext &base, std::int64_t team) const {
    if (team == barbarian_team || team_is_minor(base, team)) return false;
    if (profile == TechnologyProfile::vp542)
        for (auto id : teams.at(team).members) if (base.players.at(id).slot_status == 4) return false;
    return true;
}
bool PlayerResearchContext::embassy_at(std::int64_t from, std::int64_t to) const {
    const auto &a = teams.at(from), &b = teams.at(to);
    if ((a.liberated_by == to || b.liberated_by == from) && !a.at_war.contains(to)) return true;
    return a.master == to || b.master == from || a.embassy.contains(to);
}
bool PlayerResearchContext::spy_at(const ResearchContext &base, std::int64_t from, std::int64_t to) const {
    for (auto mine : major_players) if (base.players.at(mine).team == from)
        for (auto other : major_players) if (base.players.at(other).team == to) {
            const auto &p = players.at(other);
            const auto capital = p.cities.find(p.capital);
            if (capital != p.cities.end())
                if (const auto *vp_city=find_vp_research_city_profile(capital->second);
                    vp_city && vp_city->assigned_spy_players.contains(mine)) return true;
        }
    return false;
}
PlayerResearchModifier player_research_modifier(PlayerResearchContext &state, const ResearchContext &base,
    const content::ResearchRules &rules, const content::TechnologyRules &techs,
    const content::GlobalDefineCache &defines, const content::CustomModOptions *options,
    std::span<const TeamTechnologies> teams, std::int64_t player, std::int64_t tech) {
    validate(state,base,rules,techs,defines,options,teams);
    PlayerResearchModifier result;
    if (tech == -1) return result;
    const auto *info = tech < 0 ? nullptr : techs.find(static_cast<std::uint64_t>(tech));
    if (!info) throw std::runtime_error("Player research modifier needs a valid technology Info");
    const bool vp = base.profile == TechnologyProfile::vp542;
    auto &p = state.players.at(player);
    const auto &b = base.players.at(player);
    const auto &own_team = team_techs(teams,b.team);
    for (auto id : state.civ_teams) {
        const auto &t = state.teams.at(id);
        if (t.alive_count <= 0 || state.team_is_minor(base,id) || (vp && id == b.team)) continue;
        bool borrow;
        if (vp && options->enabled("DIPLOMACY_TECH_BONUSES")) {
            const auto *vp_state=p.profile_extensions.find<VpPlayerResearchProfileState>();
            borrow = (vp_state && vp_state->spies) ? state.spy_at(base,b.team,id)
                                                    : state.embassy_at(b.team,id);
        }
        else borrow = state.teams.at(b.team).met.contains(id);
        if (borrow && team_techs(teams,id).has_tech(tech)) ++result.known_teams;
        if (!vp || borrow) ++result.possible_teams;
    }
    if (result.possible_teams > 0) {
        if (vp && state.team_is_major(base,b.team)) {
            const bool human = base.is_human(b) && !options->enabled("HUMAN_USES_AI_HANDICAP");
            const auto handicap_id = base.is_human(b) && options->enabled("HUMAN_USES_AI_HANDICAP")
                ? define(defines,"AI_HANDICAP") : b.handicap;
            const auto *h = rules.handicaps.find(handicap_id);
            result.catch_up = h ? h->tech_catch_up : 0;
            if (!h) result.missing_infos.push_back("player_handicap");
            if (!human) {
                const auto *ai = rules.handicaps.find(base.game_handicap);
                result.catch_up = wide::add(result.catch_up,ai ? ai->ai_tech_catch_up : 0);
                if (!ai) result.missing_infos.push_back("game_handicap");
            }
            result.catch_up = wide::multiply_divide(result.catch_up,base.current_era,1);
        }
        result.value = wide::add(result.value,wide::multiply_divide(
            wide::add(define(defines,"TECH_COST_TOTAL_KNOWN_TEAM_MODIFIER"),result.catch_up),
            result.known_teams,result.possible_teams));
    }
    const auto limit = define(defines,"NUM_OR_TECH_PREREQS");
    if (info->or_prerequisites.default_value != -1) throw std::runtime_error("Nonstandard prerequisite array default");
    if (limit > 0) {
        if (static_cast<std::uint64_t>(limit) > info->or_prerequisites.extent)
            throw std::out_of_range("Research modifier prerequisite bound exceeds source array");
        for (const auto &[i, prerequisite] : info->or_prerequisites.values) {
            if (i >= static_cast<std::uint64_t>(limit)) break;
            if (prerequisite != -1 && own_team.has_tech(prerequisite)) ++result.known_or_prerequisites;
        }
    }
    result.value = wide::add(result.value,wide::multiply_divide(result.known_or_prerequisites,
        define(defines,"TECH_COST_KNOWN_PREREQ_MODIFIER"),1));
    for (const auto &league : state.leagues) {
        if (!member(league.members,player)) continue;
        std::int64_t effect = 0;
        for (auto v : league.discovered_tech_modifiers) effect = wide::add(effect,v);
        if (effect == 0) continue;
        for (auto id : league.members) if (team_techs(teams,base.players.at(id).team).has_tech(tech)) {
            result.league = wide::add(result.league,effect); break;
        }
    }
    if (result.league != 0) {
        if (vp && state.team_is_major(base,b.team) && options->enabled("BALANCE_VP")) {
            auto &vp_state=vp_player_research_profile(p);
            vp_state.science_from_league = vp_state.cs_allies > 0 ? std::min<std::int64_t>(50,
                wide::multiply_divide(vp_state.cs_allies,define(defines,"SCHOLAR_MINOR_ALLY_MULTIPLIER"),1)) : 0;
            result.league = wide::add(result.league,vp_state.science_from_league);
        }
        result.value = wide::multiply_divide(result.value,wide::add(100,result.league),100);
    }
    return result;
}
std::int64_t bnw_max_effective_cities(PlayerResearchState &player, bool include_puppets) {
    auto cities = size64(player.cities.size());
    std::int64_t puppets = 0;
    for (const auto &[id,c] : player.cities) {
        if (c.puppet) { --cities; ++puppets; }
        if (c.ignore_happiness || c.razing) --cities;
    }
    if (cities == 0) cities = 1;
    player.max_effective_cities = std::max(player.max_effective_cities,cities);
    return include_puppets ? wide::add(player.max_effective_cities,puppets) : player.max_effective_cities;
}
PlayerResearchCost player_research_cost(PlayerResearchContext &state, const ResearchContext &base,
    const content::ResearchRules &rules, const content::TechnologyRules &techs,
    const content::GlobalDefineCache &defines, const content::CustomModOptions *options,
    std::span<const TeamTechnologies> teams, std::int64_t player, std::int64_t tech,
    bool ignore_cities, std::int64_t city_offset) {
    validate(state,base,rules,techs,defines,options,teams);
    const bool vp = base.profile == TechnologyProfile::vp542;
    if (!vp && (ignore_cities || city_offset != 0)) throw std::runtime_error("City cost options are VP-only");
    const auto &team = team_techs(teams,base.players.at(player).team);
    PlayerResearchCost result;
    result.team = bind_team_research(base,rules,defines,options,team);
    result.team_cost = team_research_cost(techs,team,tech,result.team.factors);
    result.modifier = player_research_modifier(state,base,rules,techs,defines,options,teams,player,tech);
    auto &p = state.players.at(player);
    const auto modifier = std::max<std::int64_t>(1,result.modifier.value);
    if (vp) {
        result.cost = wide::multiply_divide(result.team_cost,100,modifier);
        if (!ignore_cities) {
            result.city_modifier_times100 = vp_research_city_modifier_times100(p,state.world_city_modifier,
                define(defines,"NUM_CITIES_COST_MOD_SCALING"),!options->enabled("BALANCE_PUPPET_CHANGES"),city_offset);
            result.cost = wide::multiply_divide(result.cost,wide::add(10000,result.city_modifier_times100),10000);
        }
        result.completion_threshold_times100 = wide::multiply_divide(result.cost,100,1);
    } else {
        auto cost = wide::multiply_divide(result.team_cost,10000,modifier);
        const auto city_mod = wide::multiply_divide(state.world_city_modifier,bnw_max_effective_cities(p,true),1);
        result.city_modifier_times100 = wide::multiply_divide(city_mod,100,1);
        cost = wide::multiply_divide(cost,wide::add(100,city_mod),100);
        result.completion_threshold_times100 = cost;
        result.cost = wide::add(cost/100,cost%100 != 0 ? 1 : 0);
    }
    return result;
}
PlayerResearchQuote player_research_quote(PlayerResearchContext &state, const ResearchContext &base,
    const content::ResearchRules &rules, const content::TechnologyRules &techs,
    const content::GlobalDefineCache &defines, const content::CustomModOptions *options,
    std::span<const TeamTechnologies> teams, std::int64_t player, std::int64_t tech,
    bool ignore_cities, std::int64_t city_offset) {
    PlayerResearchQuote result;
    static_cast<PlayerResearchCost &>(result) = player_research_cost(state,base,rules,techs,defines,options,teams,player,tech,ignore_cities,city_offset);
    const auto &team = team_techs(teams,base.players.at(player).team);
    const auto overflow = state.players.at(player).overflow_times100;
    result.progress = base.profile == TechnologyProfile::vp542 ? wide::add(team.progress_times100(tech),overflow)
                                                              : wide::add(team.progress(tech),overflow/100);
    return result;
}
} // namespace civ5::runtime
