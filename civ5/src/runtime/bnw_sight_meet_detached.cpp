#include "civ5/runtime/bnw_sight_meet_detached.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <climits>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *s) {return s?std::string_view(s):std::string_view();}
std::int64_t field(const std::vector<saves::NamedSaveInteger> &fields,
                   std::string_view key) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &item:fields) if(name(item.name)==key) {
        require(!found,"Duplicate BNW meet team field");found=&item;
    }
    require(found,"Missing BNW meet team field");return found->value;
}
std::int64_t unit_field(const saves::SavedUnitObservation &unit,
                        std::string_view key) {
    const saves::UnitCoreField *found=nullptr;
    for(const auto &item:unit.core.fields) if(name(item.name)==key) {
        require(!found,"Duplicate BNW meet unit field");found=&item;
    }
    require(found && found->values.size()==1,
            "Missing BNW meet unit identity field");
    return found->values.front();
}
std::int64_t city_field(const saves::SavedCityObservation &city,
                        std::string_view key) {
    const saves::CityCoreField *found=nullptr;
    for(const auto &item:city.core.fields) if(name(item.name)==key) {
        require(!found,"Duplicate BNW meet city field");found=&item;
    }
    require(found && found->values.size()==1,
            "Missing BNW meet city identity field");
    return found->values.front();
}
bool alive(const saves::SavedPlayerObservation &player) {
    const saves::PlayerCoreField *found=nullptr;
    for(const auto &item:player.core.fields)
        if(name(item.name)=="m_bAlive") {
            require(!found,"Duplicate BNW meet player alive field");found=&item;
        }
    require(found && found->values.size()==1 &&
            (found->values[0]==0 || found->values[0]==1),
            "BNW meet player alive state is unavailable");
    return found->values[0]==1;
}
std::vector<std::int64_t> &array(saves::TeamObservation &team,
                                 std::string_view key,std::size_t width) {
    saves::PlotArrayObservation *found=nullptr;
    for(auto &item:team.arrays) if(name(item.name)==key) {
        require(!found,"Duplicate BNW meet team array");found=&item;
    }
    require(found && found->columns==1 && found->values.size()==width,
            "BNW meet team array width is unavailable");
    return found->values;
}
const saves::SavedPlayerObservation &player(const saves::SavedSessionSnapshot &world,
                                            std::int64_t id) {
    require(world.all_players &&
            world.all_players->layout==saves::GameStateLayout::expansion2_version1 &&
            id>=0 && id<64 && world.all_players->players.size()==64,
            "BNW meet player roster is unavailable");
    const auto &found=world.all_players->players.at(static_cast<std::size_t>(id));
    require(found.index==static_cast<std::uint64_t>(id) &&
            found.saved_player_id==id && alive(found),
            "BNW meet player is absent, dead, or has changed identity");
    return found;
}
std::int64_t team_of(const saves::SavedSessionSnapshot &world,
                     std::int64_t owner) {
    (void)player(world,owner);
    require(world.pregame &&
            world.pregame->layout==saves::GameStateLayout::expansion2_version1,
            "BNW meet pregame setup is unavailable");
    const auto it=world.pregame->archive.integer_arrays.find("team_types");
    require(it!=world.pregame->archive.integer_arrays.end() &&
            it->second.size()>=64,
            "BNW meet owner-to-team mapping is unavailable");
    const auto team=it->second.at(static_cast<std::size_t>(owner));
    require(team>=0 && team<63,
            "BNW meet team lacks a turn_team_met wire slot");
    return team;
}
bool human(const saves::SavedSessionSnapshot &world,std::int64_t owner) {
    (void)player(world,owner);
    const auto it=world.pregame->archive.integer_arrays.find("slot_status");
    require(it!=world.pregame->archive.integer_arrays.end() &&
            it->second.size()>=64,
            "BNW meet human slot status is unavailable");
    const auto status=it->second.at(static_cast<std::size_t>(owner));
    // Expansion2 CvPreGame::isHuman: SS_COMPUTER=1, SS_TAKEN=3.
    // SS_OPEN and SS_OBSERVER require game-mode/claim branches not restored.
    require(status==1 || status==3,
            "BNW meet human status needs unported pregame mode/claim branch");
    return status==3;
}
bool team_human(const saves::SavedSessionSnapshot &world,std::int64_t team) {
    const auto mapping=world.pregame->archive.integer_arrays.find("team_types");
    require(mapping!=world.pregame->archive.integer_arrays.end() &&
            mapping->second.size()>=64,
            "BNW meet team human mapping is unavailable");
    bool found=false;
    for(const auto &p:world.all_players->players) {
        if(!alive(p)) continue;
        require(p.saved_player_id>=0 && p.saved_player_id<64,
                "BNW meet living player ID is invalid");
        if(mapping->second.at(static_cast<std::size_t>(p.saved_player_id))!=team)
            continue;
        found=human(world,p.saved_player_id)||found;
    }
    return found;
}
void verify_object(const saves::SavedSessionSnapshot &world,
                   const BnwSightOrderedEvent &event,bool city) {
    const auto &owner=player(world,event.player);
    require(event.plot<world.map_projection->cells.size(),
            "BNW meet candidate plot is outside map");
    const auto &cell=world.map_projection->cells.at(event.plot);
    require(cell.x<=INT32_MAX && cell.y<=INT32_MAX,
            "BNW meet candidate coordinates exceed SDK int range");
    const auto x=static_cast<std::int64_t>(cell.x);
    const auto y=static_cast<std::int64_t>(cell.y);
    std::size_t matches=0;
    if(city) {
        for(const auto &item:owner.cities.cities)
            if(city_field(item,"m_iID")==event.object_id) {
                require(city_field(item,"m_eOwner")==event.player &&
                        city_field(item,"m_iX")==x &&
                        city_field(item,"m_iY")==y &&
                        cell.owner && *cell.owner==event.player &&
                        cell.plot_city && cell.plot_city->owner==event.player &&
                        cell.plot_city->id==event.object_id,
                        "BNW meet city reference differs from owned city");
                ++matches;
            }
    } else {
        require(cell.units &&
                std::count(cell.units->begin(),cell.units->end(),
                    saves::SavedPlotUnitReference{event.player,event.object_id})==1,
                "BNW meet unit handle is absent or duplicated on plot");
        for(const auto &item:owner.units.units)
            if(unit_field(item,"m_iID")==event.object_id) {
                require(unit_field(item,"m_eOwner")==event.player &&
                        unit_field(item,"m_iX")==x &&
                        unit_field(item,"m_iY")==y && cell.units,
                        "BNW meet unit reference differs from owned unit");
                ++matches;
            }
    }
    require(matches==1,"BNW meet candidate object is absent or duplicated");
}
void mark(saves::TeamObservation &one,saves::TeamObservation &two,
          std::int64_t a,std::int64_t b,std::int64_t turn,bool &new_contact) {
    require(one.index==static_cast<std::uint64_t>(a) &&
            two.index==static_cast<std::uint64_t>(b) &&
            field(one.fields,"id")==a && field(two.fields,"id")==b,
            "BNW meet team IDs disagree with roster");
    auto &one_met=array(one,"has_met",64);
    auto &two_met=array(two,"has_met",64);
    auto &one_turn=array(one,"turn_team_met",63);
    auto &two_turn=array(two,"turn_team_met",63);
    const auto first=one_met.at(static_cast<std::size_t>(b));
    const auto second=two_met.at(static_cast<std::size_t>(a));
    require((first==0 || first==1) && first==second,
            "BNW meet prior relationship is asymmetric or invalid");
    new_contact=first==0;
    if(!new_contact) {
        require(one_turn.at(static_cast<std::size_t>(b))==
                    two_turn.at(static_cast<std::size_t>(a)),
                "BNW meet prior turn is asymmetric");
        return;
    }
    require(turn>=0 && turn<=INT32_MAX &&
            one_turn.at(static_cast<std::size_t>(b))<=turn &&
            two_turn.at(static_cast<std::size_t>(a))<=turn,
            "BNW meet game turn or prior turn is invalid");
    // CvTeam.cpp:3349-3352 executes these two writes for A before repeating
    // them for B in CvTeam::meet. Mandatory intervening effects are omitted
    // solely on this private diagnostic copy.
    one_met[static_cast<std::size_t>(b)]=1;
    one_turn[static_cast<std::size_t>(b)]=turn;
    two_met[static_cast<std::size_t>(a)]=1;
    two_turn[static_cast<std::size_t>(a)]=turn;
}
} // namespace

BnwSightMeetResult apply_bnw_sight_meets_detached(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity) {
    BnwSightMeetResult result;
    try {
        require(identity.payload_sha256.size()==64 &&
                identify_bytes(source.source.bytes).sha256==identity.payload_sha256,
                "BNW meet original payload differs from session identity");
        auto ordered=trace_bnw_unit_sight_event_order(
            state,request,database,types,defines,identity);
        require(ordered.calculated && ordered.exploration.sight.detached,
                "BNW meet needs ordered detached sight state");
        auto copy=std::move(*ordered.exploration.sight.detached);
        require(copy.world.teams &&
                copy.world.teams->layout==saves::GameStateLayout::expansion2_version1 &&
                copy.world.teams->dimensions.teams==64 &&
                copy.world.teams->dimensions.civ_teams==63 &&
                copy.world.teams->teams.size()==64 &&
                copy.world.map_projection && copy.world.all_players,
                "BNW meet owned team/map/player profile is incomplete");
        const auto turn=copy.setup.game_turn;
        const auto saved_turn=copy.world.pregame->archive.integers.find("game_turn");
        require(saved_turn!=copy.world.pregame->archive.integers.end() &&
                saved_turn->second==turn && turn>=0 && turn<=INT32_MAX,
                "BNW meet current game turn differs across owned views");
        const auto revealing=ordered.exploration.sight.plan.team;
        require(revealing>=0 && revealing<63,
                "BNW revealing team lacks a turn_team_met wire slot");
        for(const auto &event:ordered.events) {
            const bool city=event.effect==BnwSightOrderedEffect::city_meet_candidate;
            if(!city && event.effect!=BnwSightOrderedEffect::unit_meet_candidate)
                continue;
            verify_object(state.world,event,city);
            const auto other=team_of(state.world,event.player);
            const auto revealing_human=team_human(state.world,revealing);
            const auto other_human=human(state.world,event.player);
            // CvPlot.cpp:7737-7755 suppresses AI discovery of a human unit or
            // city. For unit, CvUnit::isHuman resolves through its owner.
            if(!revealing_human && other_human) continue;
            bool new_contact=false;
            if(other!=revealing) {
                mark(copy.world.teams->teams.at(static_cast<std::size_t>(revealing)),
                     copy.world.teams->teams.at(static_cast<std::size_t>(other)),
                     revealing,other,turn,new_contact);
            } else
                require(array(copy.world.teams->teams.at(
                            static_cast<std::size_t>(revealing)),"has_met",64)
                            .at(static_cast<std::size_t>(revealing))==1,
                        "BNW meet self relationship is not initialized");
            result.contacts.push_back({event.plot,revealing,other,event.player,
                event.object_id,city,new_contact});
        }
        result.detached=std::move(copy);
        result.calculated=true;
        result.reason="BNW meet flags/turns are detached diagnostics; updateTechShare, diplomacy, Lua, UI and persistence remain";
    } catch(const std::exception &error) {
        result.calculated=false;
        result.detached.reset();
        result.contacts.clear();
        result.reason=error.what();
    }
    return result;
}
} // namespace civ5::runtime
