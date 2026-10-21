#include "civ5/runtime/bnw_deficit_selection.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {
using namespace civ5;
using namespace civ5::runtime;
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void check(bool ok,const char *message) {
    if (!ok) throw std::runtime_error(message);
}
template<class F> void rejects(F action) {
    bool caught=false;
    try {action();} catch (const std::exception &) {caught=true;}
    check(caught,"Expected BNW deficit selection blocker");
}
void field(saves::SavedUnitObservation &unit,const char *name,std::int64_t value) {
    saves::UnitCoreField result{};
    result.name=name;result.values={value};
    unit.core.fields.push_back(std::move(result));
}
void unit(saves::SavedPlayerObservation &player,std::int64_t id,
          std::int64_t x,std::int64_t type,std::int64_t base,
          std::int64_t moves=60,std::int64_t damage=0) {
    saves::SavedUnitObservation result{};
    result.index=player.units.units.size();result.core.layout=bnw;
    field(result,"m_iID",id);field(result,"m_eOwner",5);
    field(result,"m_iX",x);field(result,"m_iY",0);
    field(result,"m_iBaseCombat",base);field(result,"m_iMoves",moves);
    field(result,"m_iLevel",1);field(result,"m_iDamage",damage);
    field(result,"idUnitType",type);
    player.units.units.push_back(std::move(result));
}
saves::SavedSessionSnapshot snapshot() {
    saves::SavedSessionSnapshot result;
    result.game_state.layout=bnw;
    auto &all=result.all_players.emplace();all.layout=bnw;
    auto &player=all.players.emplace_back();
    player.saved_player_id=5;
    player.units.layout=player.cities.layout=bnw;
    player.cities.cities.emplace_back();
    unit(player,10,10,1,10); // land, intrinsic 100 -> power 112
    unit(player,11,11,2,10); // land, intrinsic 50 -> power 56
    unit(player,12,12,2,10); // tie; first source unit wins
    unit(player,13,13,3,10); // sea, intrinsic 40 -> power 45
    unit(player,14,14,99,0); // noncombat is excluded before type resolution
    unit(player,15,15,4,10,0); // immobile this turn
    unit(player,16,16,4,10); // live plot in combat
    return result;
}
BnwDeficitMilitaryContext military() {
    BnwDeficitMilitaryContext result;
    result.land_defense=0;result.naval_defense=1;
    for (std::int64_t x=10;x<=16;++x)
        result.plot_unit_fighting[{x,0}]=x==16;
    return result;
}
} // namespace

int main() {
    try {
        auto world=snapshot();
        BnwDeficitRules rules;
        rules.max_hit_points=100;
        rules.units={{1,{BnwDeficitDomain::land,100}},
                     {2,{BnwDeficitDomain::land,50}},
                     {3,{BnwDeficitDomain::sea,40}},
                     {4,{BnwDeficitDomain::land,20}}};
        auto ai=military();
        SimulationRandom random(RandomAlgorithm::bnw,{0,0,0});
        const auto first=choose_bnw_deficit_candidate(world,5,rules,ai,random);
        check(first.combat_units==6 && first.cities==1 &&
              first.random_drawn && first.random_allows_disband &&
              first.land && first.land->unit_id==11 && first.land->power==56 &&
              first.land->source_index==1 &&
              first.naval && first.naval->unit_id==13 &&
              first.selected && first.selected->unit_id==11 &&
              random.state().calls==1,
              "BNW forced disband did not honor domain, canScrap, power tie and defense priority");
        auto damaged=world;
        auto &damaged_fields=damaged.all_players->players.front().units.units.front().core.fields;
        for (auto &field:damaged_fields)
            if (field.name && std::string_view(field.name)=="m_iDamage")
                field.values={75};
        random=SimulationRandom(RandomAlgorithm::bnw,{0,0,0});
        const auto health_choice=choose_bnw_deficit_candidate(
            damaged,5,rules,ai,random);
        check(health_choice.selected && health_choice.selected->unit_id==10 &&
              health_choice.selected->power==28,
              "BNW unit health did not reduce GetPower before candidate scoring");
        ai.land_defense=1;ai.naval_defense=0;
        random=SimulationRandom(RandomAlgorithm::bnw,{0,0,0});
        const auto score_choice=choose_bnw_deficit_candidate(world,5,rules,ai,random);
        check(score_choice.selected && score_choice.selected->unit_id==13,
              "BNW land/naval score did not break lower-priority defense tie");
        ai.land_defense=2;ai.naval_defense=3;
        random=SimulationRandom(RandomAlgorithm::bnw,{0,0,0});
        const auto defended=choose_bnw_deficit_candidate(world,5,rules,ai,random);
        check(defended.random_drawn && !defended.land && !defended.naval &&
              !defended.selected && random.state().calls==1,
              "Needed/critical defense still scanned forced candidates");
        ai=military();rules.unit_delete_disabled=true;
        random=SimulationRandom(RandomAlgorithm::bnw,{0,0,0});
        const auto disabled=choose_bnw_deficit_candidate(world,5,rules,ai,random);
        check(disabled.random_drawn && !disabled.selected,
              "UNIT_DELETE_DISABLED did not suppress canScrap");
        rules.unit_delete_disabled=false;
        random=SimulationRandom(RandomAlgorithm::bnw,{2,0,0}); // first draw 51
        auto incomplete=ai;incomplete.plot_unit_fighting.clear();
        const auto roll_reject=choose_bnw_deficit_candidate(
            world,5,rules,incomplete,random);
        check(roll_reject.random_drawn && !roll_reject.random_allows_disband &&
              !roll_reject.selected && random.state().calls==1,
              "BNW RNG >=50 still evaluated candidate plots or skipped draw");
        random=SimulationRandom(RandomAlgorithm::bnw,{0,0,0});
        rejects([&]{(void)choose_bnw_deficit_candidate(
            world,5,rules,incomplete,random);});
        check(random.state().calls==0,
              "Missing live plot context consumed gameplay RNG");
        incomplete=ai;incomplete.land_defense.reset();
        rejects([&]{(void)choose_bnw_deficit_candidate(
            world,5,rules,incomplete,random);});
        check(random.state().calls==0,
              "Missing live military AI context consumed gameplay RNG");
        auto malformed=world;
        auto &fields=malformed.all_players->players.front().units.units.front().core.fields;
        fields.erase(fields.begin()+5); // m_iMoves
        rejects([&]{(void)choose_bnw_deficit_candidate(
            malformed,5,rules,ai,random);});
        check(random.state().calls==0,
              "Missing saved unit field consumed gameplay RNG");
        auto excessive_level=world;
        for (auto &field:excessive_level.all_players->players.front().units.units.front().core.fields)
            if (field.name && std::string_view(field.name)=="m_iLevel")
                field.values={INT64_MAX};
        rejects([&]{(void)choose_bnw_deficit_candidate(
            excessive_level,5,rules,ai,random);});
        check(random.state().calls==0,
              "Oversized unit level overflowed before rejecting or consumed RNG");
        auto peaceful=world;
        peaceful.all_players->players.front().cities.cities.resize(6);
        const auto no_disband=choose_bnw_deficit_candidate(
            peaceful,5,rules,ai,random);
        check(!no_disband.random_drawn && !no_disband.selected &&
              random.state().calls==0,
              "Combat-unit count <= city count consumed gameplay RNG");
        std::cout << "PASS BNW deficit candidate selection\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL BNW deficit candidate selection: " << error.what() << '\n';
        return 1;
    }
}
