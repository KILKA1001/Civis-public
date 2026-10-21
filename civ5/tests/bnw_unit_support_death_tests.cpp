#include "civ5/runtime/bnw_unit_support_death.hpp"
#include "civ5/content/info_types.hpp"
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
template<class F> void rejects(F &&f) {
    bool rejected=false;
    try {f();} catch(const std::exception &) {rejected=true;}
    check(rejected,"Invalid BNW support death state was accepted");
}
void scalar(saves::PlayerCoreObservation &core,const char *name,
            std::int64_t value) {
    saves::PlayerCoreField field{};
    field.name=name;field.encoding=saves::PlayerCoreEncoding::i32;
    field.values={value};core.fields.push_back(std::move(field));
}
std::int64_t read(const saves::PlayerCoreObservation &core,
                  std::string_view wanted) {
    for(const auto &field:core.fields)
        if(field.name && wanted==field.name && field.values.size()==1)
            return field.values[0];
    throw std::runtime_error("Synthetic BNW player counter missing");
}
void set(saves::PlayerCoreObservation &core,std::string_view wanted,
         std::int64_t value) {
    for(auto &field:core.fields)
        if(field.name && wanted==field.name && field.values.size()==1) {
            field.values[0]=value;return;
        }
    throw std::runtime_error("Synthetic BNW player counter missing");
}
saves::SavedSessionSnapshot world(const runtime::BnwUnitClassDeathRules &classes,
                                  std::int64_t unit_type) {
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    saves::SavedSessionSnapshot source{};
    source.game_state.layout=bnw;source.map_header.layout=bnw;
    source.map_header.grid={1,1};
    source.map_plots.emplace();source.map_plots->layout=bnw;
    source.map_plots->plots.emplace_back();
    source.map_plots->plots[0].fields.push_back({"area",0});
    source.map_plots->plots[0].arrays.push_back({"units",2,{0,0}});
    source.map_projection.emplace();source.map_projection->grid={1,1};
    source.map_projection->cells.emplace_back();
    source.map_projection->cells[0].units=
        std::vector<saves::SavedPlotUnitReference>{{0,0}};
    source.map_regions.emplace();source.map_regions->layout=bnw;
    source.map_regions->containers.emplace_back();
    auto &areas=source.map_regions->containers.back();
    areas.name="areas";areas.wire_count=1;areas.records.emplace_back();
    auto &area=areas.records.back();
    area.fields={{"id",0},{"units",1}};
    auto area_counts=std::vector<std::int64_t>(64);area_counts[0]=1;
    area.arrays.push_back({"units_per_player",1,std::move(area_counts)});
    source.pregame.emplace();source.pregame->layout=bnw;
    source.pregame->archive.integer_arrays["team_types"]=
        std::vector<std::int64_t>(64,0);
    source.teams.emplace();source.teams->layout=bnw;
    source.teams->teams.resize(64);
    for(std::size_t t=0;t<64;++t) source.teams->teams[t].index=t;
    saves::TeamHashedArray hashed{};
    hashed.name="unit_class_count";hashed.wire_count=2;
    for(std::size_t i=0;i<2;++i)
        hashed.rows.push_back({classes.class_hashes[i],
                              {static_cast<std::int64_t>(i==
                                  static_cast<std::size_t>(unit_type))}});
    source.teams->teams[0].hashes.push_back(std::move(hashed));
    source.all_players.emplace();source.all_players->layout=bnw;
    source.all_players->players.resize(64);
    for(std::size_t p=0;p<64;++p) {
        auto &player=source.all_players->players[p];
        player.index=p;player.saved_player_id=p;
        player.units.layout=bnw;player.units.header.layout=bnw;
    }
    auto &player=source.all_players->players[0];
    player.core.layout=bnw;
    saves::PlayerCoreField player_classes{};
    player_classes.name="m_paiUnitClassCount";
    player_classes.encoding=saves::PlayerCoreEncoding::vector_i32;
    player_classes.wire_count=2;
    player_classes.values={unit_type==0?1:0,unit_type==1?1:0};
    player.core.fields.push_back(std::move(player_classes));
    scalar(player.core,"m_iNumBuilders",unit_type==0?1:0);
    scalar(player.core,"m_iUnhappinessFromUnits",unit_type==0?2:0);
    scalar(player.core,"m_iExtraUnitCost",unit_type==0?3:0);
    scalar(player.core,"m_iNumNukeUnits",unit_type==1?1:0);
    scalar(player.core,"m_iNumMilitaryUnits",unit_type==1?1:0);
    auto &units=player.units;
    units.header.source_slots=1;
    units.header.fields={{"num_slots",1},{"last_index",0},
        {"free_list_head",-1},{"free_list_count",0},{"current_id",8192}};
    units.header.next_free_indices={-1};units.header.wire_unit_count=1;
    units.units.emplace_back();
    auto &unit=units.units.back();unit.core.layout=bnw;unit.tail.layout=bnw;
    const auto put=[&](const char *name,saves::UnitCoreEncoding encoding,
                       std::int64_t value) {
        saves::UnitCoreField field{};
        field.name=name;field.encoding=encoding;field.values={value};
        unit.core.fields.push_back(std::move(field));
    };
    put("m_iID",saves::UnitCoreEncoding::i32,0);
    put("m_eOwner",saves::UnitCoreEncoding::i32,0);
    put("m_iX",saves::UnitCoreEncoding::i32,0);
    put("m_iY",saves::UnitCoreEncoding::i32,0);
    put("m_eUnitType",saves::UnitCoreEncoding::i32,unit_type);
    put("idUnitType",saves::UnitCoreEncoding::hash32,
        classes.units.at(unit_type).unit_hash);
    return source;
}
void test() {
    content::RulesDatabase database;
    database.apply_sql(
        "CREATE TABLE Domains(ID INTEGER,Type TEXT);"
        "INSERT INTO Domains VALUES(0,'DOMAIN_LAND'),(1,'DOMAIN_AIR');"
        "CREATE TABLE UnitClasses(ID INTEGER,Type TEXT);"
        "INSERT INTO UnitClasses VALUES(0,'UNITCLASS_WORKER'),"
        "(1,'UNITCLASS_BOMBER');"
        "CREATE TABLE Units(ID INTEGER,Type TEXT,Class TEXT,Domain TEXT,"
        "WorkRate INTEGER,Unhappiness INTEGER,ExtraMaintenanceCost INTEGER,"
        "NukeDamageLevel INTEGER,MilitarySupport INTEGER);"
        "INSERT INTO Units VALUES"
        "(0,'UNIT_WORKER','UNITCLASS_WORKER','DOMAIN_LAND',100,2,3,-1,0),"
        "(1,'UNIT_BOMBER','UNITCLASS_BOMBER','DOMAIN_AIR',0,0,0,1,1);");
    const auto types=database.cache_info_types();
    const auto classes=runtime::load_bnw_unit_class_death_rules(database,types);
    const auto support=runtime::load_bnw_unit_support_rules(
        database,types,classes);
    check(support.units.size()==2 && support.units.at(0).work_rate==100 &&
          support.units.at(1).military_support,
          "BNW support rule loader did not bind active Units rows");
    for(std::int64_t unit_type=0;unit_type<2;++unit_type) {
        auto source=world(classes,unit_type);
        const auto removal=runtime::plan_bnw_unit_removal(source,0,0);
        const auto class_plan=runtime::plan_bnw_unit_class_death(
            source,removal,classes);
        const auto plan=runtime::plan_bnw_unit_support_death(
            source,class_plan,classes,support);
        auto result=runtime::project_bnw_unit_support_death(
            source,plan,classes,support);
        const auto &core=result.all_players->players[0].core;
        check(read(core,"m_iNumBuilders")==0 &&
              read(core,"m_iUnhappinessFromUnits")==0 &&
              read(core,"m_iExtraUnitCost")==0 &&
              read(core,"m_iNumNukeUnits")==0 &&
              read(core,"m_iNumMilitaryUnits")==0 &&
              core.fields[0].values[unit_type]==0 &&
              result.teams->teams[0].hashes[0].rows[unit_type].values[0]==0 &&
              result.all_players->players[0].units.units.size()==1,
              "BNW support/class counter projection changed wrong state");
        check(read(source.all_players->players[0].core,"m_iExtraUnitCost")==
                  (unit_type==0?3:0),
              "BNW support death projection mutated source");
        check(plan.needs_active_player_ui_check,
              "BNW support death omitted active-player UI intent");
        auto stale=plan;stale.before[0]++;
        rejects([&]{(void)runtime::project_bnw_unit_support_death(
            source,stale,classes,support);});
    }
    {
        auto source=world(classes,0);
        const auto removal=runtime::plan_bnw_unit_removal(source,0,0);
        const auto class_plan=runtime::plan_bnw_unit_class_death(
            source,removal,classes);
        set(source.all_players->players[0].core,"m_iNumBuilders",0);
        rejects([&]{(void)runtime::plan_bnw_unit_support_death(
            source,class_plan,classes,support);});
        source=world(classes,0);
        auto bad_rules=support;bad_rules.units[0].unit_hash++;
        rejects([&]{(void)runtime::plan_bnw_unit_support_death(
            source,class_plan,classes,bad_rules);});
        source=world(classes,0);
        set(source.all_players->players[0].core,"m_iExtraUnitCost",INT32_MIN);
        rejects([&]{(void)runtime::plan_bnw_unit_support_death(
            source,class_plan,classes,support);});
    }
    std::cout << "PASS BNW detached unit support death counters\n";
}
}
int main() {
    try {test();return 0;}
    catch(const std::exception &error) {
        std::cerr << "FAIL BNW unit support death: " << error.what() << '\n';
        return 1;
    }
}
