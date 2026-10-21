#include "civ5/runtime/bnw_player_do_turn_revolts.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_player_do_turn_trade.hpp"
#include <climits>
#include <stdexcept>
#include <type_traits>
namespace civ5::runtime {
namespace {
void need(bool v,const char *why){if(!v)throw std::runtime_error(why);}
std::int64_t scalar(const auto &fields,std::string_view name){const typename std::remove_cvref_t<decltype(fields)>::value_type *found=nullptr;for(const auto &f:fields)if(f.name&&std::string_view(f.name)==name){need(!found,"Player revolt duplicate source scalar");found=&f;}need(found&&found->values.size()==1,"Player revolt source scalar unknown");return found->values[0];}
BnwUnhappinessQuote current_unhappiness(const LiveSessionState &s,const BnwHappinessSuffixRules &rules,std::int64_t player) {
    const auto expected=restore_bnw_unhappiness(s.world,s.setup,s.cities,rules.unhappiness(),rules.types());
    need(expected.world_cities_percent==s.unhappiness.world_cities_percent,"Player revolt raw/owned world unhappiness rate stale");
    const auto &e=expected.players.at(player),&a=s.unhappiness.players.at(player);
    need(e.happiness==a.happiness&&e.units==a.units&&e.units_modifier==a.units_modifier&&e.population_modifier==a.population_modifier&&e.cities_modifier==a.cities_modifier&&e.occupied_modifier==a.occupied_modifier&&e.capital_modifier==a.capital_modifier&&e.trait_cities_modifier==a.trait_cities_modifier&&e.trait_population_modifier==a.trait_population_modifier&&e.public_opinion==a.public_opinion&&e.half_specialists==a.half_specialists&&e.cities.size()==a.cities.size(),"Player revolt raw/owned unhappiness operands stale");
    for(const auto &[id,c]:e.cities){const auto &live=a.cities.at(id);need(c.occupied==live.occupied&&c.no_occupied_unhappiness==live.no_occupied_unhappiness&&c.specialists==live.specialists,"Player revolt serialized city unhappiness operands stale");}
    const auto &raw=s.world.all_players->players.at(player);const auto &cities=s.cities.players.at(player);
    need(scalar(raw.core.fields,"m_iCapitalCityID")==cities.capital&&raw.cities.cities.size()==cities.cities.size(),"Player revolt current capital/city registry differs");
    for(const auto &saved:raw.cities.cities){const auto id=scalar(saved.core.fields,"m_iID");const auto &city=cities.cities.at(id);need(scalar(saved.core.fields,"m_eOwner")==player&&city.owner==player&&city.id==id&&scalar(saved.core.fields,"m_iPopulation")==city.population&&scalar(saved.core.fields,"m_bPuppet")==city.puppet,"Player revolt raw/owned city population or puppet operands stale");}
    const auto q=bnw_unhappiness(s.unhappiness,s.setup,s.cities,rules.unhappiness(),rules.defines(),player);
    need(q.total>=INT32_MIN&&q.total<=INT32_MAX&&q.excess_happiness>=INT32_MIN&&q.excess_happiness<=INT32_MAX&&q.public_opinion>=INT32_MIN&&q.public_opinion<=INT32_MAX,"Player revolt live unhappiness exceeds source int");
    // Do not install the rebuilt model: city ignore flags are retained and not
    // serialized. Native helpers read the SAME verified live cache/context.
    return q;
}
std::string revision(const LiveSessionState &s,const SessionCheckpointIdentity &identity,const content::RulesDatabase &db,std::int64_t player,const BnwUnhappinessQuote &q){
    std::vector<std::uint8_t> bytes;const auto number=[&](std::int64_t v){auto u=static_cast<std::uint64_t>(v);for(unsigned b=0;b<8;++b)bytes.push_back(static_cast<std::uint8_t>(u>>(8*b)));};const auto text=[&](std::string_view v){number(v.size());bytes.insert(bytes.end(),v.begin(),v.end());};
    text(bnw_player_do_turn_prefix_revision(s,identity,db));text(bnw_player_trade_registry_revision(s,player));number(player);number(q.total);number(q.excess_happiness);number(q.public_opinion);
    const auto &raw=s.world.all_players->players.at(player);for(const auto &f:raw.traits.fields){text(f.name?f.name:"");number(f.values.size());for(auto v:f.values)number(v);}
    for(const auto &block:raw.trade_culture.blocks){text(block.name?block.name:"");for(const auto &f:block.fields){text(f.name?f.name:"");number(f.value);}}
    for(const auto &[id,c]:s.unhappiness.players.at(player).cities){number(id);number(c.ignore);number(c.occupied);number(c.no_occupied_unhappiness);for(const auto &[type,count]:c.specialists){number(type);number(count);}}
    const auto &cities=s.cities.players.at(player);number(cities.capital);for(const auto &[id,c]:cities.cities){number(id);number(c.population);number(c.puppet);}
    const auto &leader=s.world.pregame->slots.string_arrays.at("leader_keys").at(player);text(leader);
    for(const auto *rng:{&s.random.map,&s.random.gameplay}){number(static_cast<unsigned>(rng->algorithm()));number(rng->state().seed);number(rng->state().calls);number(rng->state().resets);}
    return identify_bytes(bytes).sha256;
}
}
std::string bnw_player_do_turn_revolts_revision(const LiveSessionState &s,const BnwHappinessSuffixRules &rules,const content::RulesDatabase &db,const SessionCheckpointIdentity &identity,std::int64_t player){
 need(rules.identity()==identity.rules_sha256&&identify_bytes(db.gameplay_fingerprint_data()).sha256==identity.rules_sha256,"Player revolt revision native rules identity differs");return revision(s,identity,db,player,current_unhappiness(s,rules,player));
}
BnwPlayerDoTurnRevoltsJournal stage_bnw_player_do_turn_revolts(LiveSessionState &state,const BnwPlayerDoTurnPrefixJournal &prefix,const BnwHappinessSuffixRules &native_rules,const content::RulesDatabase &db,const SessionCheckpointIdentity &identity,std::int64_t player){
    need(prefix.player()==player&&prefix.turn()==state.setup.game_turn&&prefix.after_world_revision()==bnw_player_do_turn_prefix_revision(state,identity,db),"Player revolt suffix requires SAME unchanged accepted scalar/cramped source journal");
    need(native_rules.identity()==identity.rules_sha256&&identify_bytes(db.gameplay_fingerprint_data()).sha256==identity.rules_sha256,"Player revolt suffix selected native rules identity differs");
    BnwPlayerDoTurnRevoltsJournal journal;journal.unhappiness_=current_unhappiness(state,native_rules,player);journal.before_=revision(state,identity,db,player,journal.unhappiness_);journal.before_rng_=state.random.gameplay.state();
    auto trial=state;using S=BnwPlayerDoTurnRevoltStep;
    journal.uprising_=apply_bnw_player_uprising_turn(trial,player,db,identity);journal.steps_.push_back(S::uprisings);
    journal.revolt_=apply_bnw_player_city_revolt_turn(trial,player,db,identity);journal.steps_.push_back(S::city_revolts);
    journal.maya_=apply_bnw_player_maya_turn(trial,player,db,identity);journal.steps_.push_back(S::maya);
    // Source4071-4077: isMinorCiv reads the actual CvPreGame flag array,
    // NOT an assumed slot range. Keep the precise next source obligation.
    const auto minors=trial.world.pregame->archive.flag_arrays.find("minor_nation_civs");
    need(minors!=trial.world.pregame->archive.flag_arrays.end()&&minors->second.size()==64&&player>=0&&player<64,"Player revolt next-step native minor roster unknown");
    journal.trade_required_=scalar(trial.world.all_players->players.at(player).core.fields,"m_bAlive")==1&&player!=63&&!minors->second.at(player);
    journal.after_rng_=trial.random.gameplay.state();journal.after_=revision(trial,identity,db,player,journal.unhappiness_);
    static_assert(std::is_nothrow_move_assignable_v<LiveSessionState>&&std::is_nothrow_move_constructible_v<BnwPlayerDoTurnRevoltsJournal>);
    state=std::move(trial);return journal;
}
}
