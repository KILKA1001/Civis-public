#include "civ5/runtime/bnw_team_tech_player_research.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include <algorithm>
#include <climits>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void require(bool condition,const char *reason) {
    if(!condition) throw std::runtime_error(reason);
}
template<class T> T &named(std::vector<T> &items,std::string_view name) {
    T *found=nullptr;
    for(auto &item:items)
        if(item.name && name==item.name) {
            require(!found,"Duplicate BNW tech player field");
            found=&item;
        }
    require(found,"Missing BNW tech player field");
    return *found;
}
struct CityRanks {
    saves::PlotArrayObservation *base=nullptr,*yield=nullptr;
};
CityRanks city_ranks(saves::SavedCityObservation &city,std::int64_t owner) {
    require(city.core.layout==bnw && city.production.layout==bnw,
            "BNW city rank cache profile changed");
    const auto &owner_field=named(city.core.fields,"m_eOwner");
    require(owner_field.values.size()==1 && owner_field.values[0]==owner,
            "BNW city rank cache owner disagrees with player");
    auto &before=named(city.production.blocks,"before_strategy");
    auto &base=named(before.arrays,"m_abBaseYieldRankValid");
    auto &yield=named(before.arrays,"m_abYieldRankValid");
    require(base.columns==1 && yield.columns==1 &&
            base.values.size()==6 && yield.values.size()==6,
            "BNW city yield-rank cache does not match six source yields");
    for(const auto *array:{&base,&yield})
        for(const auto value:array->values)
            require(value==0 || value==1,
                    "BNW city yield-rank cache contains non-boolean value");
    return {&base,&yield};
}
struct Target {
    std::int64_t player=-1;
    PlayerResearchState *owned_queue=nullptr;
    ResearchTurnPlayer *owned_flags=nullptr;
    saves::PlotArrayObservation *raw_queue=nullptr;
    saves::NamedSaveInteger *raw_count=nullptr;
    saves::TeamHashedValues *raw_flag=nullptr;
    std::vector<CityRanks> cities;
    BnwTeamTechPlayerResearchEffect effect;
};
Target preflight_player(LiveSessionState &state,const content::InfoTypeCatalog &types,
                        std::int64_t player,std::int64_t technology,
                        std::int64_t active_player,std::int64_t active_team) {
    require(state.player_research.profile==content::TechnologyProfile::bnw &&
            state.research_turn.profile==content::TechnologyProfile::bnw &&
            state.player_research.players.contains(player) &&
            state.research_turn.players.contains(player),
            "BNW tech player research owned contexts are incomplete");
    auto &saved=state.world.all_players->players[static_cast<std::size_t>(player)];
    require(saved.index==static_cast<std::uint64_t>(player) &&
            saved.saved_player_id==player && saved.knowledge.layout==bnw &&
            saved.administration.layout==bnw && saved.cities.layout==bnw,
            "BNW tech player saved identity or profile changed");
    Target target;
    target.player=player;
    target.effect.player=player;
    target.owned_queue=&state.player_research.players.at(player);
    target.owned_flags=&state.research_turn.players.at(player);
    require(target.owned_flags->unmapped_researching==0,
            "BNW tech player researching flags contain unknown technologies");
    auto &pre_city=named(saved.administration.blocks,"pre_city");
    target.raw_queue=&named(pre_city.arrays,"m_researchQueue");
    target.raw_count=&named(pre_city.fields,"m_researchQueue_wire_count");
    require(target.raw_queue->columns==1 &&
            target.raw_queue->values==target.owned_queue->queue &&
            target.raw_count->value>=INT32_MIN &&
            target.raw_count->value<=INT32_MAX &&
            ((target.raw_count->value<=0 && target.raw_queue->values.empty()) ||
             (target.raw_count->value>0 &&
              static_cast<std::uint64_t>(target.raw_count->value)==
                  target.raw_queue->values.size())),
            "BNW tech player raw and owned research queues disagree");
    auto &techs=named(saved.knowledge.blocks,"player_techs");
    auto &flags=named(techs.hashes,"researching_tech");
    require(flags.wire_count>=0 &&
            static_cast<std::uint64_t>(flags.wire_count)==flags.rows.size(),
            "BNW researching technology hash span is malformed");
    const content::InfoTypeEntry* mapped=nullptr;
    for(const auto& entry:types.entries())if(entry.second.table=="Technologies"&&entry.second.id==technology){
        require(!mapped,"BNW researching technology ID ambiguous");mapped=&entry.second;
    }
    require(mapped&&mapped->hash==content::saved_info_type_hash(mapped->type),
            "BNW actual researching hash is ambiguous");
    for(auto &row:flags.rows)
        if(row.type_hash==mapped->hash) {
            require(!target.raw_flag && row.values.size()==1 &&
                    (row.values[0]==0 || row.values[0]==1),
                    "BNW Agriculture researching hash row is malformed");
            target.raw_flag=&row;
        }
    const bool researching=target.owned_flags->researching.contains(technology);
    require(target.raw_flag &&
            (target.raw_flag->values[0]!=0)==researching,
            "BNW Agriculture raw and owned researching flags disagree");
    target.effect.popped=researching;
    if(researching) {
        target.effect.popup_dirty=player==active_player;
        target.effect.research_buttons_dirty=
            state.setup.players.at(player).team==active_team;
        if(target.effect.popup_dirty)
            target.effect.dirty_order.push_back(
                BnwTeamTechResearchDirtyBit::popup);
        if(target.effect.research_buttons_dirty)
            for(const auto bit:{BnwTeamTechResearchDirtyBit::research_buttons,
                                BnwTeamTechResearchDirtyBit::game_data,
                                BnwTeamTechResearchDirtyBit::score})
                target.effect.dirty_order.push_back(bit);
    }
    target.cities.reserve(saved.cities.cities.size());
    for(auto &city:saved.cities.cities) {
        const auto ranks=city_ranks(city,player);
        target.cities.push_back(ranks);
        ++target.effect.cities_invalidated;
        for(const auto *array:{ranks.base,ranks.yield})
            if(std::any_of(array->values.begin(),array->values.end(),
                           [](std::int64_t value){return value!=0;}))
                target.effect.rank_flags_changed=true;
    }
    return target;
}

void apply_queue(Target& target,std::int64_t technology) {
            if(target.effect.popped) {
                auto &owned=target.owned_queue->queue;
                auto &raw=target.raw_queue->values;
                if(const auto found=std::find(owned.begin(),owned.end(),technology);
                   found!=owned.end()) owned.erase(found);
                if(const auto found=std::find(raw.begin(),raw.end(),technology);
                   found!=raw.end()) raw.erase(found);
                target.raw_count->value=static_cast<std::int64_t>(raw.size());
                target.owned_flags->researching.erase(technology);
                target.raw_flag->values[0]=0;
            }

}
void invalidate_ranks(Target& target) {
            for(const auto &ranks:target.cities) {
                std::fill(ranks.base->values.begin(),ranks.base->values.end(),0);
                std::fill(ranks.yield->values.begin(),ranks.yield->values.end(),0);
            }
}
} // namespace

BnwTeamTechPlayerResearchResult run_bnw_team_tech_player_research(
    LiveSessionState &state,const BnwTeamTechLuaAttempt &lua,
    const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    std::int64_t team,std::int64_t technology) {
    BnwTeamTechPlayerResearchResult result;
    const auto prefix=plan_bnw_team_tech_after_lua(
        state,lua,technologies,database,types,team,technology);
    if(prefix.first_pending!=BnwTeamTechAfterLuaStep::best_route ||
       prefix.proven_no_effect.size()!=4) {
        result.blocker=prefix.blocker.empty() ?
            "BNW tech player loop post-Lua prefix is incomplete" : prefix.blocker;
        return result;
    }
    const auto *previous=state.setup.extensions.find<BnwTeamTechBestRouteMarker>();
    if(!previous || previous->team!=team || previous->technology!=technology ||
       state.setup.extensions.find<BnwTeamTechPlayerResearchMarker>()) {
        result.blocker="BNW tech player loop requires one completed best-route effect";
        return result;
    }
    try {
        require(state.world.all_players && state.world.all_players->layout==bnw &&
                state.world.all_players->players.size()==64 &&
                state.setup.players.size()==64 && state.world.pregame &&
                state.world.pregame->layout==bnw &&
                types.report().all_succeeded() &&
                !types.report().hash_collisions && !types.report().name_overwrites,
                "BNW tech player loop roster or type cache is incomplete");
        const auto active=state.world.pregame->archive.integers.find("active_player");
        require(active!=state.world.pregame->archive.integers.end() &&
                state.setup.players.contains(active->second),
                "BNW active player is unavailable for research UI intents");
        const auto active_team=state.setup.players.at(active->second).team;
        std::vector<Target> targets;
        for(std::int64_t player=0;player<64;++player) {
            const auto roster=state.setup.players.find(player);
            require(roster!=state.setup.players.end(),
                    "BNW tech player roster has a missing slot");
            if(roster->second.alive && roster->second.team==team)
                targets.push_back(preflight_player(state,types,player,technology,
                                                   active->second,active_team));
        }
        result.effects.reserve(targets.size());
        bool any_queue=false,any_city=false;
        for(const auto &target:targets) {
            result.effects.push_back(target.effect);
            any_queue|=target.effect.popped;
            any_city|=target.effect.rank_flags_changed;
        }
        if(any_queue)
            result.checkpoint_sections={
                saves::CivisSaveSectionId::live_bnw_research_queue,
                saves::CivisSaveSectionId::live_team_research,
                saves::CivisSaveSectionId::live_research_turn,
                saves::CivisSaveSectionId::live_research_selection};
        if(any_city)
            result.checkpoint_sections.push_back(
                saves::CivisSaveSectionId::live_bnw_city_production);
        state.setup.extensions.emplace<BnwTeamTechPlayerResearchMarker>(
            BnwTeamTechPlayerResearchMarker{team,technology,result.effects});
        // All following writes are scalar, erase on integer vectors/sets, or
        // assignments inside prevalidated fixed-size arrays.
        for(auto &target:targets) {
            apply_queue(target,technology);
            invalidate_ranks(target);
        }
        result.applied=true;
    } catch(const std::exception &error) {
        result.blocker=error.what();
    }
    return result;
}
} // namespace civ5::runtime

namespace civ5::runtime {
struct BnwTeamTechPlayerResearchPrepared::Impl {
 LiveSessionState* source=nullptr;
 std::int64_t team=-1,technology=-1;
 std::uint64_t generation=0,before_revision=0,after_revision=0;
 bool applied=false;
 std::vector<Target> targets;
 std::vector<BnwTeamTechPlayerResearchEffect> effects;
 std::vector<std::vector<std::int64_t>> queues;
 std::vector<std::set<std::int64_t>> flags;
 std::vector<saves::TeamHashedArray> raw_flags;
 std::vector<std::int64_t> counts;
 BnwTeamTechResearchPublication* publication=nullptr;
 std::size_t dirty_count=0;
};
BnwTeamTechPlayerResearchPrepared::BnwTeamTechPlayerResearchPrepared(std::unique_ptr<Impl> p):impl_(std::move(p)){}
BnwTeamTechPlayerResearchPrepared::~BnwTeamTechPlayerResearchPrepared()=default;
std::unique_ptr<BnwTeamTechPlayerResearchPrepared> BnwTeamTechPlayerResearchPrepared::prepare(
 LiveSessionState& s,const content::InfoTypeCatalog& types,const content::TechnologyRules& rules,
 std::int64_t team,std::int64_t technology,std::uint64_t generation){
 auto p=std::make_unique<Impl>();p->source=&s;p->team=team;p->technology=technology;p->generation=generation;p->before_revision=s.ui_source_revision;
 require(s.world.all_players&&s.world.all_players->layout==bnw&&s.world.all_players->players.size()==64&&
  s.setup.players.size()==64&&s.world.pregame&&s.world.pregame->layout==bnw&&
  types.report().all_succeeded()&&!types.report().hash_collisions&&!types.report().name_overwrites&&
  rules.profile==content::TechnologyProfile::bnw&&rules.all_succeeded(),"BNW genuine research loop profile/catalogue unavailable");
 require(!s.setup.extensions.find<BnwTeamTechPlayerResearchMarker>()&&!s.setup.extensions.find<BnwTeamTechResearchPublication>(),
  "BNW research/rank prefix already issued or foreign publication present");
 const auto active=detail::bnw_game_active_player_read(s),active_team=detail::bnw_game_active_team_read(s);
 for(std::int32_t actor=0;actor<64;++actor){auto& raw=s.world.all_players->players.at(static_cast<std::size_t>(actor));
  const auto& alive=named(raw.core.fields,"m_bAlive").values;
  const auto& id=named(raw.core.fields,"m_eID").values;
  require(raw.index==static_cast<std::uint64_t>(actor)&&raw.saved_player_id==actor&&id==std::vector<std::int64_t>{actor}&&
   alive.size()==1&&(alive[0]==0||alive[0]==1)&&s.setup.players.at(actor).alive==(alive[0]!=0)&&
   s.setup.players.at(actor).team==detail::bnw_player_team_read(s,actor),"BNW real member raw/owned ID/alive/team differs");
  if(!alive[0]||s.setup.players.at(actor).team!=team)continue;
  auto target=preflight_player(s,types,actor,technology,active,active_team);
  auto& knowledge=named(raw.knowledge.blocks,"player_techs");auto& researching=named(knowledge.hashes,"researching_tech");
  std::set<std::int64_t> actual,mapped;
  for(const auto& row:researching.rows){
   if(!row.type_hash){require(row.values.empty(),"BNW malformed empty researching hash");continue;}
   const auto* info=types.find_hash(row.type_hash);
   require(info&&info->table=="Technologies"&&info->id>=0&&rules.find(static_cast<std::uint64_t>(info->id))&&
    mapped.insert(info->id).second&&row.values.size()==1&&(row.values[0]==0||row.values[0]==1),
    "BNW complete researching remap unresolved/duplicate/nonboolean");
   if(row.values[0])actual.insert(info->id);
  }
  require(actual==target.owned_flags->researching,"BNW full raw/owned researching flags disagree");
  for(const auto tech:target.owned_queue->queue)require(tech>=0&&rules.find(static_cast<std::uint64_t>(tech)),
   "BNW queue contains unresolved technology ID");
  // Actual city registry validation includes owned/raw roster and FLTA free
  // slots even for an empty city list. Sort genuine city handles by slot.
  (void)resolve_bnw_city_registry(s.world,s.cities,actor,0);
  target.cities.clear();
  std::vector<std::pair<std::int32_t,saves::SavedCityObservation*>> city_order;
  for(auto& city:raw.cities.cities){const auto& city_id=named(city.core.fields,"m_iID").values;
   require(city_id.size()==1&&city_id[0]>=0&&city_id[0]<=INT32_MAX,"BNW actual city ID malformed");
   const auto value=static_cast<std::int32_t>(city_id[0]);
   require(resolve_bnw_city_registry(s.world,s.cities,actor,value)==&city,"BNW city rank registry/source identity differs");
   city_order.emplace_back(value,&city);
  }
  std::sort(city_order.begin(),city_order.end(),[](const auto& a,const auto& b){return (a.first&8191)<(b.first&8191);});
  for(const auto& entry:city_order)target.cities.push_back(city_ranks(*entry.second,actor));
  auto queue=target.owned_queue->queue;auto flags=target.owned_flags->researching;auto desired_raw=researching;
  if(target.effect.popped){const auto found=std::find(queue.begin(),queue.end(),technology);if(found!=queue.end())queue.erase(found);flags.erase(technology);}
  if(target.effect.popped)for(auto& row:desired_raw.rows){const auto* info=types.find_hash(row.type_hash);if(info&&info->id==technology&&info->table=="Technologies")row.values[0]=0;}
  p->counts.push_back(target.effect.popped?static_cast<std::int64_t>(queue.size()):target.raw_count->value);
  p->queues.push_back(std::move(queue));p->flags.push_back(std::move(flags));p->raw_flags.push_back(std::move(desired_raw));
  p->dirty_count+=target.effect.dirty_order.size();p->effects.push_back(target.effect);p->targets.push_back(std::move(target));
 }
 require(s.ui_source_revision<=UINT64_MAX-p->targets.size(),"BNW research/rank revision overflow");
 return std::unique_ptr<BnwTeamTechPlayerResearchPrepared>(new BnwTeamTechPlayerResearchPrepared(std::move(p)));
}
void BnwTeamTechPlayerResearchPrepared::apply(LiveSessionState& s){
 auto& p=*impl_;
 require(!p.applied&&p.source==&s&&s.ui_source_revision==p.before_revision,"BNW research/rank source changed or already attempted");
 require(!s.setup.extensions.find<BnwTeamTechPlayerResearchMarker>()&&!s.setup.extensions.find<BnwTeamTechResearchPublication>(),
  "BNW research/rank publication appeared before stores");
 // Allocate all retained marker and queue storage BEFORE first world write.
 BnwTeamTechResearchPublication publication;publication.events.resize(p.dirty_count);
 p.publication=&s.setup.extensions.emplace<BnwTeamTechResearchPublication>(std::move(publication));
 s.setup.extensions.emplace<BnwTeamTechPlayerResearchMarker>(BnwTeamTechPlayerResearchMarker{p.team,p.technology,p.effects});
 p.applied=true;
 for(auto& target:p.targets){
  apply_queue(target,p.technology);
  // Popup (after the real false flag), then research/game/score dirty bits.
  // This is an actual owned queue publication, not external rendering ack.
  for(const auto kind:target.effect.dirty_order){p.publication->events.at(p.publication->written++)={target.player,p.team,kind,p.generation};}
  invalidate_ranks(target);
  if(target.effect.popped||target.effect.rank_flags_changed||!target.effect.dirty_order.empty())++s.ui_source_revision;
 }
 p.after_revision=s.ui_source_revision;require_stored(s);
}
void BnwTeamTechPlayerResearchPrepared::require_stored(const LiveSessionState& s)const{
 const auto& p=*impl_;
 require(p.applied&&p.source==&s&&s.ui_source_revision>=p.after_revision&&
  s.setup.extensions.find<BnwTeamTechResearchPublication>()==p.publication&&p.publication->written==p.dirty_count,
  "BNW research/rank actual publication/source revision changed");
 std::size_t event=0;
 for(std::size_t i=0;i<p.targets.size();++i){const auto& target=p.targets[i];
  require(target.raw_count->value==p.counts[i]&&target.raw_queue->values==p.queues[i]&&target.owned_queue->queue==p.queues[i]&&
   target.owned_flags->researching==p.flags[i]&&target.raw_flag->values==std::vector<std::int64_t>{0},
   "BNW native popResearch paired stored queue/flags changed");
  const auto& saved=s.world.all_players->players.at(static_cast<std::size_t>(target.player));
  const saves::TeamHashedArray* current_flags=nullptr;
  for(const auto& block:saved.knowledge.blocks)if(block.name&&std::string_view(block.name)=="player_techs")
   for(const auto& values:block.hashes)if(values.name&&std::string_view(values.name)=="researching_tech")current_flags=&values;
  require(current_flags&&current_flags->wire_count==p.raw_flags[i].wire_count&&current_flags->rows.size()==p.raw_flags[i].rows.size(),
   "BNW native stored full researching hash shape changed");
  for(std::size_t row=0;row<current_flags->rows.size();++row)
   require(current_flags->rows[row].type_hash==p.raw_flags[i].rows[row].type_hash&&current_flags->rows[row].values==p.raw_flags[i].rows[row].values,
    "BNW native stored full raw/owned researching rows changed");
  for(const auto kind:target.effect.dirty_order)require(p.publication->events.at(event++)==
   BnwTeamTechResearchDirtyEvent{target.player,p.team,kind,p.generation},"BNW native research dirty publication order changed");
  for(const auto& city:target.cities)for(const auto* valid:{city.base,city.yield})
   require(valid->values==std::vector<std::int64_t>(6,0),"BNW native city yield rank valid flags not invalidated");
 }
}
const std::vector<BnwTeamTechPlayerResearchEffect>& BnwTeamTechPlayerResearchPrepared::effects()const{
 require(impl_->applied,"BNW research/rank effect not actually stored");return impl_->effects;
}
}
