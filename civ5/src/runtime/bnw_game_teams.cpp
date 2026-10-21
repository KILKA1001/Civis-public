#include "civ5/runtime/bnw_game_teams.hpp"
#include <algorithm>
#include <climits>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void require(bool condition,const char *reason) {
    if(!condition) throw std::runtime_error(reason);
}
std::int64_t field(const saves::TeamObservation &team,std::string_view name) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &entry:team.fields)
        if(entry.name && name==entry.name) {
            require(!found,"Duplicate BNW team field");
            found=&entry;
        }
    require(found,"Missing BNW team field");
    return found->value;
}
const std::vector<std::int64_t> &array(const saves::TeamObservation &team,
                                       std::string_view name,std::size_t count) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &entry:team.arrays)
        if(entry.name && name==entry.name) {
            require(!found,"Duplicate BNW team array");
            found=&entry;
        }
    require(found && found->columns==1 && found->values.size()==count,
            "Missing or malformed BNW team relation array");
    return found->values;
}
std::vector<std::int64_t> &mutable_array(saves::TeamObservation &team,
                                         std::string_view name,std::size_t count) {
    saves::PlotArrayObservation *found=nullptr;
    for(auto &entry:team.arrays)
        if(entry.name && name==entry.name) {
            require(!found,"Duplicate BNW team array");
            found=&entry;
        }
    require(found && found->columns==1 && found->values.size()==count,
            "Missing or malformed BNW team relation array");
    return found->values;
}
void append(BnwTeamTurnPlan &plan,BnwTeamTurnStepKind kind,
            std::int64_t team,std::int64_t target=-1,
            std::int64_t before=0,std::int64_t after=0) {
    plan.steps.push_back({kind,team,target,before,after});
}
bool is_minor(const LiveSessionState &state,
              const std::vector<bool> &minor_civs,std::int64_t team) {
    // CvTeam::isMinorCiv checks live player slots 22..63 in order, returning
    // false on the first living nonminor member of this team.
    for(std::int64_t player=22;player<64;++player) {
        const auto found=state.setup.players.find(player);
        require(found!=state.setup.players.end(),"BNW team player roster is incomplete");
        if(found->second.alive && found->second.team==team)
            return minor_civs[static_cast<std::size_t>(player)];
    }
    return false;
}
std::int64_t define(const content::GlobalDefineCache &rules,std::string_view name) {
    require(rules.profile()==content::GlobalDefineProfile::bnw,
            "BNW team turn requires activated BNW defines");
    const auto *entry=rules.find(name);
    require(entry && entry->lookup.found && entry->lookup.error.empty() &&
            std::holds_alternative<std::int64_t>(entry->lookup.value),
            "BNW team technology percent define unavailable");
    const auto value=std::get<std::int64_t>(entry->lookup.value);
    require(value>=0 && value<=INT32_MAX,
            "BNW team technology percent define exceeds source range");
    return value;
}
const saves::NamedSaveFlag &circumnavigated(const saves::GameStateObservation &game) {
    const saves::NamedSaveFlag *found=nullptr;
    for(const auto &entry:game.flags)
        if(entry.name && std::string_view(entry.name)=="circumnavigated") {
            require(!found,"Duplicate BNW circumnavigation flag");
            found=&entry;
        }
    require(found,"Missing BNW circumnavigation flag");
    return *found;
}
bool can_circumnavigate(const LiveSessionState &state,std::int64_t team) {
    const auto &map=state.world.map_header;
    require(map.layout==bnw && map.grid.width>0 && map.grid.height>0,
            "BNW team turn map header unavailable");
    const auto plots=map.grid.size();
    require(plots<=static_cast<std::uint64_t>(INT64_MAX/2) &&
            map.land_plots>=0 && static_cast<std::uint64_t>(map.land_plots)<=plots,
            "BNW circumnavigation map counts invalid");
    if(team==63 || circumnavigated(state.world.game_state).value ||
       (!map.wrap_x && !map.wrap_y) ||
       map.land_plots>static_cast<std::int64_t>(plots*2/3))
        return false;
    require(state.world.map_projection &&
            state.world.map_projection->grid.width==map.grid.width &&
            state.world.map_projection->grid.height==map.grid.height &&
            state.world.map_projection->cells.size()==plots,
            "BNW circumnavigation requires the live map projection");
    const auto &cells=state.world.map_projection->cells;
    const auto revealed=[&](std::uint64_t x,std::uint64_t y) {
        const auto index=map.grid.index(x,y);
        const auto &cell=cells.at(static_cast<std::size_t>(index));
        require(cell.index==index && cell.x==x && cell.y==y &&
                cell.revealed_teams.has_value(),
                "BNW circumnavigation revealed plot state unavailable");
        return cell.revealed_teams->contains(team);
    };
    if(map.wrap_x) {
        for(std::uint64_t x=0;x<map.grid.width;++x) {
            bool seen=false;
            for(std::uint64_t y=0;y<map.grid.height;++y)
                if(revealed(x,y)) {seen=true;break;}
            if(!seen) return false;
        }
    } else {
        for(std::uint64_t y=0;y<map.grid.height;++y) {
            bool seen=false;
            for(std::uint64_t x=0;x<map.grid.width;++x)
                if(revealed(x,y)) {seen=true;break;}
            if(!seen) return false;
        }
    }
    return true;
}
std::uint64_t technology_grants(const LiveSessionState &state,
    const content::TechnologyRules &rules,std::int64_t recipient,
    std::int64_t percent) {
    std::int64_t majors=0;
    const auto &flags=state.world.pregame->archive.flag_arrays.at("minor_nation_civs");
    for(std::int64_t team=0;team<63;++team) {
        const auto &observation=state.world.teams->teams[static_cast<std::size_t>(team)];
        if(field(observation,"alive_count")>0 && !is_minor(state,flags,team))
            ++majors;
    }
    const auto threshold=std::max<std::int64_t>(1,majors*percent/100);
    std::uint64_t grants=0;
    for(std::uint64_t tech=0;tech<rules.collection.slots;++tech) {
        require(rules.find(tech),"BNW technology rule slot missing");
        if(state.technologies[static_cast<std::size_t>(recipient)].has_tech(
               static_cast<std::int64_t>(tech)))
            continue;
        std::int64_t owners=0;
        for(std::int64_t team=0;team<63;++team) {
            const auto &observation=state.world.teams->teams[static_cast<std::size_t>(team)];
            if(field(observation,"alive_count")>0 && !is_minor(state,flags,team) &&
               state.technologies[static_cast<std::size_t>(team)].has_tech(
                   static_cast<std::int64_t>(tech)))
                ++owners;
        }
        if(owners>=threshold) ++grants;
    }
    return grants;
}
void verify_no_trade_mapping(const LiveSessionState &state,
    const content::TechnologyRules &rules,const content::InfoTypeCatalog &types) {
    require(types.report().all_succeeded() && types.report().hash_collisions==0,
            "BNW active info-type catalog is ambiguous");
    for(std::size_t id=0;id<64;++id) {
        const auto &team=state.world.teams->teams[id];
        if(field(team,"alive_count")==0) continue;
        const auto saved=field(team,"saved_techs");
        require(saved>=0 && saved<=INT32_MAX,
                "BNW saved team technology count invalid");
        const auto count=static_cast<std::size_t>(saved);
        const auto &hashes=array(team,"tech_hashes",count);
        const auto &raw=array(team,"no_trade_tech",count);
        const auto &owned=state.technologies[id];
        std::set<std::uint64_t> matched;
        for(std::size_t row=0;row<count;++row) {
            require(hashes[row]>=0 && hashes[row]<=UINT32_MAX,
                    "BNW saved technology hash exceeds source u32");
            if(hashes[row]==0) {
                require(raw[row]==0,
                        "BNW sentinel technology has a raw no-trade flag");
                continue;
            }
            const auto *type=types.find_hash(static_cast<std::uint64_t>(hashes[row]));
            require(type && type->table=="Technologies" && type->id>=0 &&
                    static_cast<std::uint64_t>(type->id)<owned.slots(),
                    "BNW saved technology hash has no unique active slot");
            const auto slot=static_cast<std::uint64_t>(type->id);
            const auto *rule=rules.find(slot);
            require(rule && rule->base.type==type->type &&
                    matched.insert(slot).second,
                    "BNW technology rule or saved row identity differs");
            require(owned.at(type->id).no_trade==(raw[row]!=0),
                    "BNW raw and owned no-trade technology slot differs");
        }
        for(const auto &[slot,value]:owned.nondefault_values())
            if(value.no_trade)
                require(matched.contains(slot),
                        "BNW owned no-trade technology lacks a saved row");
    }
}
void apply_plan(LiveSessionState &state,const BnwTeamTurnPlan &plan) {
    for(const auto &step:plan.steps) {
        auto &team=state.world.teams->teams.at(static_cast<std::size_t>(step.team));
        switch(step.kind) {
        case BnwTeamTurnStepKind::turns_at_war:
            // SDK SetNumTurnsAtWar: GetID is m_eID, not the caller's
            // relation target. Nonzero self writes are actually refused.
            if(step.target!=field(team,"id") || step.after==0)
                mutable_array(team,"turns_at_war",64).at(
                    static_cast<std::size_t>(step.target))=step.after;
            break;
        case BnwTeamTurnStepKind::turns_locked_at_war:
            // SDK SetNumTurnsLockedIntoWar has the SAME self/zero guard.
            if(step.target!=field(team,"id") || step.after==0)
                mutable_array(team,"turns_locked_at_war",64).at(
                    static_cast<std::size_t>(step.target))=step.after;
            break;
        case BnwTeamTurnStepKind::clear_no_trade_technology:
            state.technologies.at(static_cast<std::size_t>(step.team)).set_no_trade(
                step.target,false);
            break;
        case BnwTeamTurnStepKind::small_awards_gate: {
            const auto count=field(team,"saved_techs");
            auto &raw=mutable_array(team,"no_trade_tech",
                                    static_cast<std::size_t>(count));
            std::fill(raw.begin(),raw.end(),0);
            break;
        }
        case BnwTeamTurnStepKind::barbarian_technology_gate:
        case BnwTeamTurnStepKind::minor_technology_gate:
        case BnwTeamTurnStepKind::circumnavigation_gate:
            break; // preflight proved these calls have no effect
        }
    }
}
} // namespace

BnwTeamTurnPlan plan_bnw_game_teams_prefix(const LiveSessionState &state) {
    const auto &world=state.world;
    require(state.setup.profile==content::TechnologyProfile::bnw &&
            world.game_state.layout==bnw && world.teams &&
            world.teams->layout==bnw && world.pregame &&
            world.pregame->layout==bnw,
            "BNW team turn requires one decoded BNW world");
    const auto &teams=*world.teams;
    require(teams.dimensions.teams==64 && teams.dimensions.civ_teams==63 &&
            teams.dimensions.major==22 && teams.teams.size()==64 &&
            state.technologies.size()==64 && state.setup.players.size()==64,
            "BNW team turn roster dimensions differ from Expansion2");
    const auto minor=world.pregame->archive.flag_arrays.find("minor_nation_civs");
    require(minor!=world.pregame->archive.flag_arrays.end() &&
            minor->second.size()==64,
            "BNW team turn minor-player flags unavailable");

    BnwTeamTurnPlan plan;
    for(std::int64_t id=0;id<64;++id) {
        const auto &team=teams.teams[static_cast<std::size_t>(id)];
        const auto &tech=state.technologies[static_cast<std::size_t>(id)];
        require(team.index==static_cast<std::uint64_t>(id) &&
                field(team,"id")==id && tech.team()==id &&
                tech.profile()==content::TechnologyProfile::bnw &&
                tech.remap().all_mapped() && tech.remap().duplicates==0 &&
                tech.remap().holes==0,
                "BNW team turn identity or technology mapping changed");
        const auto alive=field(team,"alive_count");
        require(alive>=0 && alive<=64,"BNW team alive count is invalid");
        if(alive==0) continue;
        ++plan.alive_teams;

        const auto saved_techs=field(team,"saved_techs");
        require(saved_techs>=0 && saved_techs<=INT32_MAX,
                "BNW saved team technology count invalid");
        const auto &raw_no_trade=array(team,"no_trade_tech",
                                       static_cast<std::size_t>(saved_techs));
        for(const auto value:raw_no_trade)
            require(value==0 || value==1,
                    "BNW raw no-trade technology flag invalid");
        for(const auto &[slot,value]:tech.nondefault_values()) {
            (void)value;
            require(slot<tech.slots(),"BNW no-trade technology outside rules");
        }

        if(id==63) {
            append(plan,BnwTeamTurnStepKind::barbarian_technology_gate,id);
        } else {
            const auto &war=array(team,"at_war",64);
            const auto &turns=array(team,"turns_at_war",64);
            const auto &locked=array(team,"turns_locked_at_war",64);
            for(std::int64_t other=0;other<63;++other) {
                const auto i=static_cast<std::size_t>(other);
                require(war[i]==0 || war[i]==1,
                        "BNW team war relation is not a source boolean");
                require(turns[i]>=INT32_MIN && turns[i]<=INT32_MAX &&
                        locked[i]>=0 && locked[i]<=INT32_MAX,
                        "BNW team war counter exceeds source range");
                const auto next=war[i] ? turns[i]+1 : 0;
                require(next<=INT32_MAX,
                        "BNW team war counter would overflow source i32");
                // Preserve source arithmetic BEFORE its setter guard.
                // A nonzero self target is a real no-store branch; zero
                // self writes remain allowed, including stale peace data.
                if((other!=id || next==0) && next!=turns[i])
                    append(plan,BnwTeamTurnStepKind::turns_at_war,
                           id,other,turns[i],next);
                if(locked[i]>0 && (other!=id || locked[i]-1==0))
                    append(plan,BnwTeamTurnStepKind::turns_locked_at_war,
                           id,other,locked[i],locked[i]-1);
            }
            if(is_minor(state,minor->second,id))
                append(plan,BnwTeamTurnStepKind::minor_technology_gate,id);
        }
        for(const auto &[slot,value]:tech.nondefault_values())
            if(value.no_trade)
                append(plan,BnwTeamTurnStepKind::clear_no_trade_technology,
                       id,static_cast<std::int64_t>(slot),1,0);
        append(plan,BnwTeamTurnStepKind::small_awards_gate,id);
        append(plan,BnwTeamTurnStepKind::circumnavigation_gate,id);
    }
    return plan;
}

BnwTeamTurnSafeBranch preflight_bnw_game_teams_safe_branch(
    const LiveSessionState &state,const BnwTeamTurnActiveRules &rules) {
    BnwTeamTurnSafeBranch result;
    result.plan=plan_bnw_game_teams_prefix(state);
    require(rules.technologies.profile==content::TechnologyProfile::bnw &&
            rules.technologies.all_succeeded() &&
            rules.technologies.collection.holes==0 &&
            rules.technologies.collection.slots<=INT32_MAX,
            "BNW team turn requires complete activated technologies");
    for(const auto &team:state.technologies)
        require(team.slots()==rules.technologies.collection.slots,
                "BNW team technology capacity differs from active rules");
    const auto types=rules.database.cache_info_types();
    verify_no_trade_mapping(state,rules.technologies,types);
    const auto barbarian_percent=define(rules.defines,"BARBARIAN_TECH_PERCENT");
    const auto minor_percent=define(rules.defines,"MINOR_CIV_TECH_PERCENT");
    const auto awards=rules.database.prefetch_info("SmallAwards");
    require(awards.all_succeeded(),"BNW SmallAwards rules unavailable");
    result.awards_present=awards.slots>0;

    for(const auto &step:result.plan.steps) {
        switch(step.kind) {
        case BnwTeamTurnStepKind::barbarian_technology_gate: {
            const auto grants=technology_grants(state,rules.technologies,
                                                 step.team,barbarian_percent);
            result.barbarian_grants+=grants;
            if(grants && result.blocker.empty())
                result.blocker="BNW barbarian technology acquisition requires setHasTech effects";
            break;
        }
        case BnwTeamTurnStepKind::minor_technology_gate: {
            const auto grants=technology_grants(state,rules.technologies,
                                                 step.team,minor_percent);
            result.minor_grants+=grants;
            if(grants && result.blocker.empty())
                result.blocker="BNW minor technology acquisition requires setHasTech effects";
            break;
        }
        case BnwTeamTurnStepKind::small_awards_gate:
            if(result.awards_present && result.blocker.empty())
                result.blocker="BNW SmallAwards effects are unavailable";
            break;
        case BnwTeamTurnStepKind::circumnavigation_gate:
            if(can_circumnavigate(state,step.team)) {
                result.circumnavigation_possible=true;
                if(result.blocker.empty())
                    result.blocker="BNW circumnavigation effects are unavailable";
            }
            break;
        case BnwTeamTurnStepKind::turns_at_war:
        case BnwTeamTurnStepKind::turns_locked_at_war:
        case BnwTeamTurnStepKind::clear_no_trade_technology:
            break;
        }
    }
    result.safe=result.blocker.empty();
    return result;
}

BnwTurnHandler make_bnw_game_teams_handler(const BnwTeamTurnActiveRules &rules) {
    return {BnwTurnPhase::game_teams,
            [rules](LiveSessionState &state,std::int64_t player) -> LiveActionResult {
                if(player!=-1)
                    return {LiveActionOutcome::rejected,
                            "BNW teams turn requires the game phase"};
                try {
                    const auto preflight=preflight_bnw_game_teams_safe_branch(state,rules);
                    if(!preflight.safe)
                        return {LiveActionOutcome::rejected,preflight.blocker};
                    auto draft=state;
                    apply_plan(draft,preflight.plan);
                    state=std::move(draft);
                    return {LiveActionOutcome::committed,{}};
                } catch(const std::exception &error) {
                    return {LiveActionOutcome::rejected,error.what()};
                }
            }};
}

BnwTurnHandler make_bnw_game_teams_handler() {
    return {BnwTurnPhase::game_teams,
            [](LiveSessionState &,std::int64_t) -> LiveActionResult {
                return {LiveActionOutcome::rejected,
                        "BNW team turn requires activated rules and a complete safe-branch preflight"};
            }};
}

} // namespace civ5::runtime
