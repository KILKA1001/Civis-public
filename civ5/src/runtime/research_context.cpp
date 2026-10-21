#include "civ5/runtime/research_context.hpp"
#include "civ5/runtime/vp_research_binding.hpp"
#include "research_binding_helpers.hpp"
#include <stdexcept>

namespace civ5::runtime {
namespace {
template <class T> const T &named(const std::vector<T> &fields, std::string_view name) {
    const T *found = nullptr;
    for (const auto &field : fields) if (field.name && std::string_view(field.name) == name) {
        if (found) throw std::runtime_error("Duplicate research state field: " + std::string(name));
        found = &field;
    }
    if (!found) throw std::runtime_error("Missing research state field: " + std::string(name));
    return *found;
}
} // namespace
bool ResearchContext::is_human(const ResearchPlayerState &player) const {
    if (player.slot_status == 3) return true; // SS_TAKEN
    if (player.slot_status == 4) return player.slot_claim == 2; // assigned observer
    if (player.slot_status != 0) return false;
    if (game_type == 2) return true; // hotseat short circuits PitBoss
    if (!pitboss) throw std::runtime_error("Open player needs the GAMEOPTION_PITBOSS lookup");
    return *pitboss || game_type == 3; // email follows PitBoss in source
}
std::int64_t ResearchContext::game_option_value(const content::RulesDatabase &rules, std::string_view name) {
    name = name.substr(0,name.find('\0'));
    if (name.empty()) return 0;
    for (const auto &option : game_options) {
        const auto saved_name = std::string_view(option.name).substr(0,option.name.find('\0'));
        if (saved_name == name) return option.value;
    }
    std::optional<std::int64_t> value;
    const auto query = rules.query_rows({"Select \"Default\" from GameOptions where Type = ? LIMIT 1",
        {std::string(name)}}, [&](const auto &row) { value = row.integer_at(0); }, 1);
    if (!query.all_succeeded()) throw std::runtime_error("Game option default lookup failed: " + query.error);
    if (value && game_turn > 0) game_options.push_back({std::string(name),*value});
    return value.value_or(0);
}
ResearchContext restore_research_context(const saves::SavedSessionSnapshot &snapshot,
                                          const content::RulesDatabase &rules) {
    if (!snapshot.pregame || !snapshot.all_players || !snapshot.teams)
        throw std::runtime_error("Research context needs saved setup, players and teams");
    const auto layout = snapshot.game_state.layout;
    if (snapshot.pregame->layout != layout || snapshot.all_players->layout != layout || snapshot.teams->layout != layout)
        throw std::runtime_error("Mixed research state layouts");
    ResearchContext result;
    if (layout == saves::GameStateLayout::vp542_corpus) result.profile = content::TechnologyProfile::vp542;
    else if (layout != saves::GameStateLayout::expansion2_version1) throw std::runtime_error("Unknown research state layout");
    const auto &p = *snapshot.pregame;
    result.game_type = p.archive.integers.at("game_type");
    result.game_turn = p.archive.integers.at("game_turn");
    const auto map_seed=p.archive.integers.at("map_random_seed");
    if(map_seed<0)throw std::runtime_error("Invalid map random seed in saved pregame");
    result.map_random_seed=static_cast<std::uint64_t>(map_seed);
    result.speed = p.archive.integers.at("game_speed");
    result.start_era = p.archive.integers.at("era");
    result.game_handicap = named(snapshot.game_state.controls, "handicap").value;
    if (result.profile == content::TechnologyProfile::vp542)
        result.current_era = named(snapshot.game_state.controls, "game_era").value;
    result.world_percent = p.world.values.integers.at("research_percent");
    result.game_options = p.game_options;
    for (const auto &saved : snapshot.teams->teams) {
        const auto id = named(saved.fields, "id").value;
        std::int64_t members;
        if (result.profile == content::TechnologyProfile::vp542) {
            const auto count = named(saved.arrays, "members").values.size();
            if (count > INT64_MAX) throw std::overflow_error("Team member count exceeds int64");
            members = static_cast<std::int64_t>(count);
        } else members = named(saved.fields, "members_count").value;
        if (id < 0 || !result.team_members.emplace(id, members).second)
            throw std::runtime_error("Invalid or duplicate research team ID");
    }
    const auto setting = [&](const char *name, std::int64_t id) {
        const auto &array = p.archive.integer_arrays.at(name);
        if (id < 0 || static_cast<std::uint64_t>(id) >= array.size())
            throw std::runtime_error("Saved player is outside setup array: " + std::string(name));
        return array[static_cast<std::size_t>(id)];
    };
    bool needs_pitboss = false;
    for (const auto &saved : snapshot.all_players->players) {
        const auto &id_field = named(saved.core.fields, "m_eID");
        const auto &alive_field = named(saved.core.fields, "m_bAlive");
        const auto &ever_alive_field = named(saved.core.fields, "m_bEverAlive");
        if (id_field.values.size() != 1 || id_field.values[0] != saved.saved_player_id ||
            alive_field.values.size() != 1 || (alive_field.values[0] != 0 && alive_field.values[0] != 1) ||
            ever_alive_field.values.size() != 1 ||
            (ever_alive_field.values[0] != 0 && ever_alive_field.values[0] != 1))
            throw std::runtime_error("Malformed research player scalar");
        const auto id = saved.saved_player_id;
        ResearchPlayerState player{setting("team_types", id), setting("handicaps", id),
            setting("slot_status", id), setting("slot_claims", id), alive_field.values[0] != 0,{},false};
        player.ever_alive=ever_alive_field.values[0]!=0;
        if (!result.players.emplace(id, player).second) throw std::runtime_error("Duplicate research player ID");
        if (player.alive && !result.team_members.contains(player.team))
            throw std::runtime_error("Alive research player has no owning team");
        needs_pitboss = needs_pitboss || (player.alive && player.slot_status == 0 && result.game_type != 2);
    }
    for (const auto &option : result.game_options) {
        const auto name = std::string_view(option.name).substr(0, option.name.find('\0'));
        if (name == "GAMEOPTION_PITBOSS") { result.pitboss = option.value != 0; break; }
    }
    if (needs_pitboss && !result.pitboss) {
        result.pitboss = result.game_option_value(rules,"GAMEOPTION_PITBOSS") != 0;
    }
    return result;
}
BoundTeamResearch bind_team_research(const ResearchContext &context, const content::ResearchRules &rules,
    const content::GlobalDefineCache &defines, const content::CustomModOptions *options,
    const TeamTechnologies &team) {
    if(context.profile==content::TechnologyProfile::vp542) {
        if(!options)throw std::runtime_error("Research binding requires complete Info/CustomMods caches");
        return bind_vp_team_research(context,rules,defines,*options,team);
    }
    if (context.profile != team.profile() || rules.profile != context.profile ||
        defines.profile() != content::GlobalDefineProfile::bnw)
        throw std::runtime_error("Research binding profiles differ");
    if (!rules.all_succeeded())
        throw std::runtime_error("Research binding requires complete Info/CustomMods caches");
    using research_binding_detail::average;
    using research_binding_detail::define;
    BoundTeamResearch result;
    auto &f = result.factors;
    f.members = context.team_members.at(team.team());
    f.world_percent = context.world_percent;
    std::vector<std::int64_t> handicaps;
    for (const auto &[id, player] : context.players) {
        if (!player.alive || player.team != team.team()) continue;
        (void)context.is_human(player);
        handicaps.push_back(player.handicap);
    }
    result.alive_players = handicaps.size();
    result.handicap_id = handicaps.empty() ? define(defines, "STANDARD_HANDICAP") : average(handicaps);
    const auto *handicap = rules.handicaps.find(result.handicap_id);
    if (handicap) {
        f.team_handicap_percent=handicap->percent;
    }
    else result.missing_infos.push_back("team_handicap");
    const auto percent = [&](const auto &collection, std::int64_t id, const char *name) {
        const auto *info = collection.find(id);
        if (info) return info->percent;
        throw std::runtime_error("Missing BNW research Info: " + std::string(name));
    };
    f.speed_percent = percent(rules.speeds, context.speed, "game_speed");
    f.start_era_percent = percent(rules.eras, context.start_era, "start_era");
    f.extra_team_member_modifier = define(defines, "TECH_COST_EXTRA_TEAM_MEMBER_MODIFIER");
    return result;
}
} // namespace civ5::runtime
