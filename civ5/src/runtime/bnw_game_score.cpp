#include "civ5/runtime/bnw_game_score.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <map>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <type_traits>

namespace civ5::runtime {
namespace {
constexpr std::size_t civ_players=63, civ_teams=63, major_players=22;

void require(bool condition,const std::string &reason) {
    if(!condition) throw std::runtime_error(reason);
}
std::int32_t source_i32(std::int64_t value,const std::string &what) {
    require(value>=INT32_MIN && value<=INT32_MAX,what+" is outside source i32 range");
    return static_cast<std::int32_t>(value);
}
std::int32_t add(std::int32_t a,std::int32_t b,const std::string &what) {
    return source_i32(static_cast<std::int64_t>(a)+b,what);
}
std::int32_t multiply(std::int32_t a,std::int32_t b,const std::string &what) {
    return source_i32(static_cast<std::int64_t>(a)*b,what);
}
std::int32_t scalar(const std::vector<saves::NamedSaveInteger> &values,
                    std::string_view name) {
    const saves::NamedSaveInteger *result=nullptr;
    for(const auto &value:values)
        if(value.name && std::string_view(value.name)==name) {
            require(!result,"Duplicate BNW score field "+std::string(name));
            result=&value;
        }
    require(result,"Missing BNW score field "+std::string(name));
    return source_i32(result->value,std::string(name));
}
std::int32_t scalar(const std::vector<saves::PlayerCoreField> &values,
                    std::string_view name) {
    const saves::PlayerCoreField *result=nullptr;
    for(const auto &value:values)
        if(value.name && std::string_view(value.name)==name) {
            require(!result,"Duplicate BNW score player field "+std::string(name));
            result=&value;
        }
    require(result && result->values.size()==1,
            "Missing or invalid BNW score player field "+std::string(name));
    return source_i32(result->values.front(),std::string(name));
}
const saves::PlayerStrategyBlock &policies(const saves::SavedPlayerObservation &p) {
    const saves::PlayerStrategyBlock *result=nullptr;
    for(const auto &block:p.strategy.blocks)
        if(block.name && std::string_view(block.name)=="policies") {
            require(!result,"Duplicate BNW policy block");
            result=&block;
        }
    require(result,"Missing BNW policy block");
    return *result;
}
const saves::CityProductionRecord &buildings(const saves::SavedCityObservation &city) {
    const saves::CityProductionRecord *result=nullptr;
    for(const auto &block:city.production.blocks)
        if(block.name && std::string_view(block.name)=="buildings") {
            require(!result,"Duplicate BNW city buildings block");
            result=&block;
        }
    require(result,"Missing BNW city buildings block");
    return *result;
}
const saves::CityTailNode &child(const std::vector<saves::CityTailNode> &nodes,
                                 std::string_view name) {
    const saves::CityTailNode *result=nullptr;
    for(const auto &node:nodes)
        if(node.name && std::string_view(node.name)==name) {
            require(!result,"Duplicate BNW city religion node "+std::string(name));
            result=&node;
        }
    require(result,"Missing BNW city religion node "+std::string(name));
    return *result;
}
std::int32_t node_scalar(const std::vector<saves::CityTailNode> &nodes,
                         std::string_view name) {
    const auto &node=child(nodes,name);
    require(node.values.size()==1,"Invalid BNW city religion scalar");
    return source_i32(node.values[0],std::string(name));
}
std::int32_t majority_religion(const saves::SavedCityObservation &city) {
    const auto &religions=child(city.tail.blocks,"religions");
    const auto &status=child(religions.children,"m_ReligionStatus");
    std::int32_t total=0,best_followers=-1,best_pressure=0,best_religion=-1;
    for(const auto &entry:status.children) {
        require(entry.name && std::string_view(entry.name)=="religion",
                "Unexpected BNW city religion entry");
        const auto followers=node_scalar(entry.children,"m_iFollowers");
        const auto pressure=node_scalar(entry.children,"m_iPressure");
        const auto religion=node_scalar(entry.children,"m_eReligion");
        require(followers>=0,"Negative BNW city religion followers");
        total=add(total,followers,"BNW city religion followers");
        if(followers>best_followers ||
           (followers==best_followers && pressure>best_pressure)) {
            best_followers=followers;
            best_pressure=pressure;
            best_religion=religion;
        }
    }
    return static_cast<std::int64_t>(best_followers)*2>=total
        ?best_religion:-1;
}
std::int32_t great_works(const saves::SavedPlayerObservation &player) {
    std::int32_t total=0;
    for(const auto &city:player.cities.cities) {
        const auto &block=buildings(city);
        const saves::PlotArrayObservation *slots=nullptr;
        for(const auto &array:block.arrays)
            if(array.name && std::string_view(array.name)=="m_aBuildingGreatWork") {
                require(!slots,"Duplicate BNW great work array");
                slots=&array;
            }
        require(slots && slots->columns==3 && slots->values.size()%3==0,
                "Missing or invalid BNW great work array");
        total=add(total,source_i32(slots->values.size()/3,"BNW great work count"),
                  "BNW great work count");
    }
    return total;
}
std::int32_t define(const content::GlobalDefineCache &cache,std::string_view name) {
    const auto *entry=cache.find(name);
    require(entry && entry->kind==content::GlobalDefineKind::integer &&
            entry->lookup.found && entry->lookup.error.empty() &&
            std::holds_alternative<std::int64_t>(entry->lookup.value),
            "Missing BNW score GlobalDefine "+std::string(name));
    return source_i32(std::get<std::int64_t>(entry->lookup.value),std::string(name));
}
struct Multipliers {
    std::int32_t city,population,land,wonder,policy,great_work,belief,
                 religion_cities,tech;
};
Multipliers multipliers(const content::GlobalDefineCache &cache) {
    require(cache.profile()==content::GlobalDefineProfile::bnw,
            "BNW score requires BNW GlobalDefines");
    return {define(cache,"SCORE_CITY_MULTIPLIER"),
            define(cache,"SCORE_POPULATION_MULTIPLIER"),
            define(cache,"SCORE_LAND_MULTIPLIER"),
            define(cache,"SCORE_WONDER_MULTIPLIER"),
            define(cache,"SCORE_POLICY_MULTIPLIER"),
            define(cache,"SCORE_GREAT_WORK_MULTIPLIER"),
            define(cache,"SCORE_BELIEF_MULTIPLIER"),
            define(cache,"SCORE_RELIGION_CITIES_MULTIPLIER"),
            define(cache,"SCORE_TECH_MULTIPLIER")};
}
std::int32_t score_cities(std::int32_t count,std::int32_t weight,
                          std::int32_t map_mod,const std::string &what) {
    const auto weighted=multiply(count,weight,what);
    return source_i32(static_cast<std::int64_t>(weighted)*map_mod,what)/100;
}
std::int32_t policy_count(const saves::SavedPlayerObservation &player,
                          const content::InfoTypeCatalog &types,
                          std::uint64_t policy_slots) {
    const auto &block=policies(player);
    const saves::TeamHashedArray *array=nullptr;
    for(const auto &candidate:block.hashes)
        if(candidate.name && std::string_view(candidate.name)=="has_policy") {
            require(!array,"Duplicate BNW has_policy array");
            array=&candidate;
        }
    require(array,"Missing BNW has_policy array");
    std::map<std::int64_t,bool> owned;
    for(const auto &row:array->rows) {
        require(row.type_hash ? row.values.size()==1 : row.values.empty(),
                "Invalid BNW has_policy hash row");
        if(!row.type_hash) continue;
        const auto *type=types.find_hash(row.type_hash);
        require(type && type->table=="Policies" && type->id>=0 &&
                static_cast<std::uint64_t>(type->id)<policy_slots,
                "Unresolved BNW saved policy hash");
        require(row.values[0]==0 || row.values[0]==1,
                "Invalid BNW saved policy flag");
        owned[type->id]=row.values[0]!=0; // ReadHashedDataArray: last row wins.
    }
    std::int32_t count=0;
    for(const auto &[id,yes]:owned)
        if(yes) count=add(count,1,"BNW owned policy count");
    return count;
}
std::int32_t created_religion(const saves::GameReligionCultureObservation &religions,
                              std::int32_t player) {
    std::int32_t result=-1;
    for(const auto &religion:religions.religions)
        if(religion.founder==player && !religion.pantheon)
            result=source_i32(religion.religion,"BNW created religion");
    return result;
}
std::int32_t religion_score(const LiveSessionState &state,std::int32_t player,
                             const Multipliers &m) {
    const auto created=created_religion(state.world.religion_culture,player);
    if(created<=0) return 0; // RELIGION_PANTHEON
    const saves::ReligionObservation *owned=nullptr;
    for(const auto &religion:state.world.religion_culture.religions)
        if(religion.religion==created) {
            require(!owned,"Duplicate BNW founded religion");
            owned=&religion;
        }
    require(owned,"BNW founded religion missing from game religion list");
    // Expansion2 ReadHashed appends even an unresolved NO_BELIEF slot.
    auto score=multiply(source_i32(owned->beliefs.belief_hashes.size(),
                                  "BNW religion belief count"),
                        m.belief,"BNW belief score");
    std::int32_t following=0;
    for(const auto &p:state.world.all_players->players) {
        const auto id=source_i32(p.saved_player_id,"BNW player ID");
        const auto it=state.setup.players.find(id);
        require(it!=state.setup.players.end(),"BNW religion player setup missing");
        if(!it->second.alive) continue;
        for(const auto &city:p.cities.cities)
            if(majority_religion(city)==created)
                following=add(following,1,"BNW cities following religion");
    }
    return add(score,multiply(following,m.religion_cities,
                              "BNW religion city score"),
               "BNW religion score");
}
std::int32_t game_turn(const LiveSessionState &state) {
    require(state.setup.game_turn>=0 && state.setup.game_turn<=INT32_MAX,
            "BNW score turn unavailable");
    return static_cast<std::int32_t>(state.setup.game_turn);
}
void replay_score(saves::SavedPlayerObservation &player,std::int32_t turn,
                  std::int32_t value) {
    saves::PlayerTailNode *names=nullptr,*series=nullptr;
    for(auto &node:player.tail.fields) {
        if(node.name && std::string_view(node.name)=="m_ReplayDataSets") {
            require(!names,"Duplicate BNW replay dataset names");names=&node;
        }
        if(node.name && std::string_view(node.name)=="m_ReplayDataSetValues") {
            require(!series,"Duplicate BNW replay dataset values");series=&node;
        }
    }
    require(names && series && names->children.size()==series->children.size(),
            "BNW replay dataset names/values unavailable");
    std::size_t index=names->children.size();
    constexpr std::string_view key="REPLAYDATASET_SCORE";
    for(std::size_t i=0;i<names->children.size();++i)
        if(std::string_view(reinterpret_cast<const char*>(
               names->children[i].bytes.data()),names->children[i].bytes.size())==key) {
            index=i;break; // Source getReplayDataSetIndex returns first match.
        }
    if(index==names->children.size()) {
        saves::PlayerTailNode name{};
        name.name="name";name.encoding="string32";
        name.wire_count=static_cast<std::int64_t>(key.size());
        name.bytes.assign(key.begin(),key.end());
        names->children.push_back(std::move(name));
        saves::PlayerTailNode turns{};
        turns.name="turns";turns.encoding="turn_data";turns.wire_count=0;
        series->children.push_back(std::move(turns));
        names->wire_count=static_cast<std::int64_t>(names->children.size());
        series->wire_count=static_cast<std::int64_t>(series->children.size());
    }
    auto &turns=series->children[index];
    require(turns.name && std::string_view(turns.name)=="turns" &&
            turns.encoding && std::string_view(turns.encoding)=="turn_data" &&
            turns.values.size()%2==0,
            "BNW score replay turn map invalid");
    auto &values=turns.values;
    auto pos=values.begin();
    for(;pos!=values.end();pos+=2)
        if(*pos>=turn) break;
    if(pos!=values.end() && *pos==turn) *(pos+1)=value;
    else values.insert(pos,{turn,value});
    turns.wire_count=static_cast<std::int64_t>(values.size()/2);
}
std::array<std::int32_t,civ_players> read_source_scores(const LiveSessionState& state,const content::RulesDatabase& rules,const content::InfoTypeCatalog& types,const content::GlobalDefineCache& defines,ResearchContext& options,std::optional<std::size_t> selected){
    require(state.world.game_state.layout==saves::GameStateLayout::expansion2_version1 &&
            state.world.all_players && state.world.teams &&
            state.world.all_players->layout==saves::GameStateLayout::expansion2_version1 &&
            state.world.teams->layout==saves::GameStateLayout::expansion2_version1,
            "BNW score requires a complete BNW world");
    require(state.world.all_players->players.size()>=civ_players &&
            state.world.teams->teams.size()>=civ_teams &&
            state.world.game_state.roster &&
            state.world.game_state.roster->arrays[1].size()>=civ_players &&
            state.world.game_state.roster->arrays[2].size()>=civ_players &&
            state.world.game_state.roster->arrays[3].size()>=civ_players &&
            state.world.game_state.roster->arrays[4].size()>=civ_teams &&
            state.world.game_state.roster->arrays[5].size()>=civ_teams &&
            state.world.game_state.roster->arrays[6].size()>=civ_teams,
            "BNW score roster dimensions unavailable");
    require(std::holds_alternative<saves::Expansion2PrefixObservation>(
                state.world.game_state.prefix),
            "BNW score map modifier unavailable");
    const auto map_mod=source_i32(std::get<saves::Expansion2PrefixObservation>(
        state.world.game_state.prefix).scalars[28],"BNW map score modifier");
    const auto m=multipliers(defines);
    require(types.report().all_succeeded(),"BNW score InfoType catalog unavailable");
    const auto policy_info=rules.prefetch_info("Policies");
    require(policy_info.all_succeeded(),"BNW score Policies Info unavailable");
    const bool no_policies=options.game_option_value(rules,"GAMEOPTION_NO_POLICIES")==1;
    const bool no_religion=options.game_option_value(rules,"GAMEOPTION_NO_RELIGION")==1;
    const bool no_science=options.game_option_value(rules,"GAMEOPTION_NO_SCIENCE")==1;
    std::array<std::int32_t,civ_players> scores{};
    for(std::size_t id=0;id<civ_players;++id) {
        if(selected&&id!=*selected)continue;
        const auto &player=state.world.all_players->players[id];
        require(player.saved_player_id==static_cast<std::int64_t>(id),
                "BNW score player ID/order mismatch");
        const auto found=state.setup.players.find(id);
        require(found!=state.setup.players.end(),"BNW score player setup missing");
        const auto &status=found->second;
        const auto team=status.team;
        require(team>=0 && team<static_cast<std::int64_t>(state.world.teams->teams.size()),
                "BNW score player team unavailable");
        const auto members=scalar(state.world.teams->teams[team].fields,"members_count");
        if(!status.alive || !members) continue;
        require(members>0,"BNW score team members invalid");
        std::int32_t score=0;
        score=add(score,score_cities(source_i32(player.cities.cities.size(),
            "BNW player city count"),m.city,map_mod,"BNW city score"),"BNW player score");
        score=add(score,score_cities(scalar(player.core.fields,"m_iTotalPopulation"),
            m.population,map_mod,"BNW population score"),"BNW player score");
        score=add(score,score_cities(scalar(player.core.fields,"m_iTotalLand"),
            m.land,map_mod,"BNW land score"),"BNW player score");
        score=add(score,multiply(scalar(player.core.fields,"m_iNumWonders"),
            m.wonder,"BNW wonder score"),"BNW player score");
        if(!no_policies) {
            const auto count=status.policies_resolved
                ?source_i32(status.policies.size(),"BNW owned policy count")
                :policy_count(player,types,policy_info.slots);
            score=add(score,multiply(count,m.policy,"BNW policy score"),
                "BNW player score");
        }
        score=add(score,multiply(great_works(player),m.great_work,
                  "BNW great work score"),"BNW player score");
        if(!no_religion)
            score=add(score,religion_score(state,id,m),"BNW player score");
        if(!no_science) {
            const TeamTechnologies *techs=nullptr;
            for(const auto &owned:state.technologies)
                if(owned.team()==team) {
                    require(!techs,"Duplicate BNW team technology owner");
                    techs=&owned;
                }
            require(techs && techs->profile()==content::TechnologyProfile::bnw,
                    "BNW score team technologies unavailable");
            score=add(score,multiply(source_i32(techs->known_count(),
                      "BNW known technology count"),m.tech,"BNW technology score"),
                      "BNW player score");
            score=add(score,scalar(player.core.fields,"m_iScoreFromFutureTech"),
                      "BNW player score");
        }
        for(const auto *field:{"m_iScenarioScore1","m_iScenarioScore2",
                               "m_iScenarioScore3","m_iScenarioScore4"})
            score=add(score,scalar(player.core.fields,field),"BNW player score");
        scores[id]=score;
    }
    return scores;
}
} // namespace

struct BnwGameScorePreparedWrites::Impl {
    struct ReplayPatch {std::size_t names_slot=0,series_slot=0;saves::PlayerTailNode names,series;};
    const LiveSessionState* state=nullptr;
    std::size_t dirty_slot=0,changed_scores=0;
    bool dirty=false;
    ResearchContext options;
    std::array<std::vector<std::int64_t>,6> arrays;
    std::array<std::size_t,civ_players> player_ids{};
    std::array<bool,civ_players> player_changed{};
    std::array<ReplayPatch,civ_players> replays;
};
BnwGameScorePreparedWrites::BnwGameScorePreparedWrites(std::unique_ptr<Impl> p)noexcept:impl_(std::move(p)){}
BnwGameScorePreparedWrites::~BnwGameScorePreparedWrites()=default;

std::unique_ptr<BnwGameScorePreparedWrites> BnwGameScorePreparedWrites::prepare(const LiveSessionState &state,
                           const content::RulesDatabase &rules,
                           const content::InfoTypeCatalog &types,
                           const content::GlobalDefineCache &defines) {
    const saves::NamedSaveFlag *score_dirty=nullptr;
    for(const auto &flag:state.world.game_state.flags)
        if(flag.name && std::string_view(flag.name)=="score_dirty") {
            require(!score_dirty,"Duplicate BNW score dirty flag");
            score_dirty=&flag;
        }
    require(score_dirty,"Missing BNW score dirty flag");
    auto prepared=std::unique_ptr<BnwGameScorePreparedWrites>(
        new BnwGameScorePreparedWrites(std::make_unique<Impl>()));
    auto& plan=*prepared->impl_;plan.state=&state;
    plan.dirty_slot=static_cast<std::size_t>(score_dirty-state.world.game_state.flags.data());
    plan.dirty=score_dirty->value;
    if(!plan.dirty) return prepared; // Genuine source condition; no fake clean flag.
    auto options=state.setup;
    const auto scores=read_source_scores(state,rules,types,defines,options,std::nullopt);
    // CvGame::updateScore uses >= while scanning in ascending source ID order.
    // Ties therefore rank the later player/team first.
    std::array<std::int32_t,civ_players> player_rank{};
    for(std::size_t id=0;id<civ_players;++id) player_rank[id]=static_cast<std::int32_t>(id);
    std::sort(player_rank.begin(),player_rank.end(),[&](auto a,auto b) {
        return scores[a]!=scores[b] ? scores[a]>scores[b] : a>b;
    });
    std::array<std::int32_t,civ_teams> team_scores{};
    for(std::size_t team=0;team<civ_teams;++team) {
        require(state.world.teams->teams[team].index==team,
                "BNW score team ID/order mismatch");
        std::int32_t score=0;
        for(std::size_t id=0;id<major_players;++id)
            if(state.setup.players.at(id).team==static_cast<std::int64_t>(team))
                score=add(score,scores[id],"BNW team major player score");
        for(std::size_t id=0;id<civ_players;++id)
            if(state.setup.players.at(id).team==static_cast<std::int64_t>(team))
                score=add(score,scores[id],"BNW team player score");
        team_scores[team]=score;
    }
    std::array<std::int32_t,civ_teams> team_rank{};
    for(std::size_t id=0;id<civ_teams;++id) team_rank[id]=static_cast<std::int32_t>(id);
    std::sort(team_rank.begin(),team_rank.end(),[&](auto a,auto b) {
        return team_scores[a]!=team_scores[b] ? team_scores[a]>team_scores[b] : a>b;
    });
    // Stage ONLY six rank/score vectors and two replay nodes per player.
    // The actual LiveSession/world/players/map are never cloned or replaced.
    plan.options=std::move(options);
    for(std::size_t i=0;i<6;++i)plan.arrays[i]=state.world.game_state.roster->arrays[i+1];
    for(std::size_t rank=0;rank<civ_players;++rank) {
        const auto id=static_cast<std::size_t>(player_rank[rank]);
        plan.player_ids[rank]=id;plan.player_changed[rank]=plan.arrays[2][id]!=scores[id];
        if(plan.player_changed[rank])++plan.changed_scores;
        plan.arrays[0][rank]=static_cast<std::int64_t>(id);
        plan.arrays[1][id]=static_cast<std::int64_t>(rank);
        plan.arrays[2][id]=scores[id];
        const auto& fields=state.world.all_players->players[id].tail.fields;
        std::size_t names=fields.size(),series=fields.size();
        for(std::size_t slot=0;slot<fields.size();++slot) {
            const auto& n=fields[slot];
            if(n.name&&std::string_view(n.name)=="m_ReplayDataSets") {
                require(names==fields.size(),"Duplicate BNW score replay names");names=slot;
            }
            if(n.name&&std::string_view(n.name)=="m_ReplayDataSetValues") {
                require(series==fields.size(),"Duplicate BNW score replay values");series=slot;
            }
        }
        require(names<fields.size()&&series<fields.size(),"Missing BNW score replay nodes");
        // This local pair is a value staging buffer, not a world/Host authority.
        saves::SavedPlayerObservation scratch{};
        scratch.tail.fields={fields[names],fields[series]};
        replay_score(scratch,game_turn(state),scores[id]);
        plan.replays[rank]={names,series,std::move(scratch.tail.fields[0]),std::move(scratch.tail.fields[1])};
    }
    for(std::size_t rank=0;rank<civ_teams;++rank) {
        const auto id=static_cast<std::size_t>(team_rank[rank]);
        plan.arrays[3][rank]=static_cast<std::int64_t>(id);
        plan.arrays[4][id]=static_cast<std::int64_t>(rank);
        plan.arrays[5][id]=team_scores[id];
    }
    return prepared;
}
bool BnwGameScorePreparedWrites::dirty()const noexcept{return impl_->dirty;}
std::size_t BnwGameScorePreparedWrites::changed_player_scores()const noexcept{return impl_->changed_scores;}
void BnwGameScorePreparedWrites::require_same_state(const LiveSessionState& state)const {
    require(impl_->state==&state,"BNW prepared score belongs to another allocation");
    require(impl_->dirty_slot<state.world.game_state.flags.size(),"BNW prepared score flag slot changed");
    const auto& flag=state.world.game_state.flags[impl_->dirty_slot];
    require(flag.name&&std::string_view(flag.name)=="score_dirty"&&flag.value==impl_->dirty,
            "BNW prepared score dirty predecessor changed");
}
void BnwGameScorePreparedWrites::clear_dirty(LiveSessionState& state)noexcept {
    state.world.game_state.flags[impl_->dirty_slot].value=false;
}
bool BnwGameScorePreparedWrites::store_player_score(LiveSessionState& state,std::size_t rank)noexcept {
    const auto id=impl_->player_ids[rank];auto& a=state.world.game_state.roster->arrays;
    a[1][rank]=impl_->arrays[0][rank];a[2][id]=impl_->arrays[1][id];
    if(impl_->player_changed[rank])a[3][id]=impl_->arrays[2][id];
    return impl_->player_changed[rank];
}
void BnwGameScorePreparedWrites::store_player_replay(LiveSessionState& state,std::size_t rank)noexcept {
    auto& fields=state.world.all_players->players[impl_->player_ids[rank]].tail.fields;
    auto& patch=impl_->replays[rank];
    static_assert(std::is_nothrow_swappable_v<saves::PlayerTailNode>);
    // Swap only the two prepared replay nodes, preserving the player/tail roster.
    std::swap(fields[patch.names_slot],patch.names);std::swap(fields[patch.series_slot],patch.series);
}
void BnwGameScorePreparedWrites::store_team_scores(LiveSessionState& state)noexcept {
    auto& a=state.world.game_state.roster->arrays;
    for(std::size_t rank=0;rank<civ_teams;++rank) {
        const auto id=static_cast<std::size_t>(impl_->arrays[3][rank]);
        a[4][rank]=impl_->arrays[3][rank];a[5][id]=impl_->arrays[4][id];a[6][id]=impl_->arrays[5][id];
    }
}
void update_bnw_game_score(LiveSessionState& state,const content::RulesDatabase& rules,
    const content::InfoTypeCatalog& types,const content::GlobalDefineCache& defines) {
    auto plan=BnwGameScorePreparedWrites::prepare(state,rules,types,defines);
    plan->require_same_state(state);if(!plan->dirty())return;
    plan->clear_dirty(state);
    for(std::size_t rank=0;rank<civ_players;++rank) {
        (void)plan->store_player_score(state,rank);plan->store_player_replay(state,rank);
    }
    plan->store_team_scores(state);state.setup=std::move(plan->impl_->options);
    // This generic diagnostic API has no genuine UI receiver and grants no
    // retained/full-phase completion. The actual cursor supplies mandatory UI.
}
} // namespace civ5::runtime

namespace civ5::runtime {
std::int32_t BnwGameScorePreparedWrites::read_player_score(const LiveSessionState& state,const content::RulesDatabase& rules,const content::InfoTypeCatalog& types,const content::GlobalDefineCache& defines,std::int32_t actor){require(actor>=0&&actor<static_cast<std::int32_t>(civ_players),"BNW fresh score actor invalid");auto options=state.setup;return read_source_scores(state,rules,types,defines,options,static_cast<std::size_t>(actor)).at(static_cast<std::size_t>(actor));}
}
