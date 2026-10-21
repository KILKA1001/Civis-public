#include "civ5/runtime/bnw_unit_death_xp_quote.hpp"
#include "civ5/content/info_types.hpp"
#include <climits>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
template<class F> void rejects(F &&f) {
    bool rejected=false;
    try {f();} catch(const std::exception &) {rejected=true;}
    check(rejected,"Invalid BNW death XP quote was accepted");
}
void scalar(saves::PlayerCoreObservation &core,const char *name,
            std::int64_t value) {
    saves::PlayerCoreField field{};
    field.name=name;field.encoding=saves::PlayerCoreEncoding::i32;
    field.values={value};core.fields.push_back(std::move(field));
}
void unit_scalar(saves::SavedUnitObservation &unit,const char *name,
                 saves::UnitCoreEncoding encoding,std::int64_t value) {
    saves::UnitCoreField field{};
    field.name=name;field.encoding=encoding;field.values={value};
    unit.core.fields.push_back(std::move(field));
}
std::int64_t &counter(saves::PlayerCoreObservation &core,std::string_view wanted) {
    for(auto &field:core.fields)
        if(field.name && wanted==field.name && field.values.size()==1)
            return field.values[0];
    throw std::runtime_error("Synthetic BNW player counter missing");
}
saves::SavedSessionSnapshot world(const runtime::BnwUnitClassDeathRules &classes,
                                  std::int64_t unit_type) {
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
    scalar(player.core,"m_iCombatExperience",0);
    scalar(player.core,"m_iNavalCombatExperience",0);
    scalar(player.core,"m_iLifetimeCombatExperience",100);
    auto &units=player.units;
    units.header.source_slots=1;
    units.header.fields={{"num_slots",1},{"last_index",0},
        {"free_list_head",-1},{"free_list_count",0},{"current_id",8192}};
    units.header.next_free_indices={-1};units.header.wire_unit_count=1;
    units.units.emplace_back();
    auto &unit=units.units.back();unit.core.layout=bnw;unit.tail.layout=bnw;
    unit_scalar(unit,"m_iID",saves::UnitCoreEncoding::i32,0);
    unit_scalar(unit,"m_eOwner",saves::UnitCoreEncoding::i32,0);
    unit_scalar(unit,"m_iX",saves::UnitCoreEncoding::i32,0);
    unit_scalar(unit,"m_iY",saves::UnitCoreEncoding::i32,0);
    unit_scalar(unit,"m_eUnitType",saves::UnitCoreEncoding::i32,unit_type);
    unit_scalar(unit,"idUnitType",saves::UnitCoreEncoding::hash32,
                classes.units.at(unit_type).unit_hash);
    unit_scalar(unit,"m_iExperience",saves::UnitCoreEncoding::i32,20);
    return source;
}
runtime::BnwUnitSupportDeath support_plan(
    const saves::SavedSessionSnapshot &source,
    const runtime::BnwUnitClassDeathRules &classes,
    const runtime::BnwUnitSupportRules &support) {
    const auto removal=runtime::plan_bnw_unit_removal(source,0,0);
    const auto class_plan=runtime::plan_bnw_unit_class_death(
        source,removal,classes);
    return runtime::plan_bnw_unit_support_death(
        source,class_plan,classes,support);
}
void test() {
    content::RulesDatabase database;
    database.apply_sql(
        "CREATE TABLE Domains(ID INTEGER,Type TEXT);"
        "INSERT INTO Domains VALUES(0,'DOMAIN_LAND'),(1,'DOMAIN_SEA'),"
        "(2,'DOMAIN_HOVER');"
        "CREATE TABLE UnitClasses(ID INTEGER,Type TEXT);"
        "INSERT INTO UnitClasses VALUES(0,'UNITCLASS_WORKER'),"
        "(1,'UNITCLASS_TRIREME');"
        "CREATE TABLE Units(ID INTEGER,Type TEXT,Class TEXT,Domain TEXT,"
        "WorkRate INTEGER,Unhappiness INTEGER,ExtraMaintenanceCost INTEGER,"
        "NukeDamageLevel INTEGER,MilitarySupport INTEGER);"
        "INSERT INTO Units VALUES"
        "(0,'UNIT_WORKER','UNITCLASS_WORKER','DOMAIN_LAND',100,2,3,-1,0),"
        "(1,'UNIT_TRIREME','UNITCLASS_TRIREME','DOMAIN_SEA',0,0,0,-1,1);"
        "CREATE TABLE Defines(Name TEXT,Value INTEGER);"
        "INSERT INTO Defines VALUES('UNIT_DEATH_XP_GREAT_GENERAL_LOSS',50);");
    const auto types=database.cache_info_types();
    const auto defines=database.cache_global_defines(
        content::GlobalDefineProfile::bnw);
    const auto classes=runtime::load_bnw_unit_class_death_rules(database,types);
    const auto support=runtime::load_bnw_unit_support_rules(
        database,types,classes);
    const auto xp=runtime::load_bnw_death_xp_rules(
        database,types,defines,classes);
    check(xp.loss_percent==50 && xp.units.at(0).domain==
        runtime::BnwDeathXpDomain::land && xp.units.at(1).domain==
        runtime::BnwDeathXpDomain::sea,
        "BNW death XP loader did not bind active domain and define");
    for(std::int64_t type=0;type<2;++type) {
        auto source=world(classes,type);
        auto &player=source.all_players->players[0];
        const auto support_death=support_plan(source,classes,support);
        const auto quote=runtime::quote_bnw_unit_death_xp(
            source,support_death,classes,support,xp,1);
        check(quote.triggered && quote.loss==10 &&
              quote.naval==(type==1) && !quote.setter_branch_unported &&
              quote.current_before==0 && quote.current_after==0 &&
              quote.lifetime_before==100 && quote.lifetime_after==90,
              "BNW death XP no-setter branch quoted wrong loss");
        auto projected=runtime::project_bnw_unit_death_xp_safe(
            source,quote,classes,support,xp);
        check(counter(projected.all_players->players[0].core,
                      "m_iLifetimeCombatExperience")==90 &&
              counter(player.core,"m_iLifetimeCombatExperience")==100 &&
              counter(projected.all_players->players[0].core,
                      type==1?"m_iNavalCombatExperience":
                              "m_iCombatExperience")==0,
              "BNW death XP detached projection changed wrong state");
        auto stale=quote;stale.loss++;
        rejects([&]{(void)runtime::project_bnw_unit_death_xp_safe(
            source,stale,classes,support,xp);});
        counter(player.core,type==1?"m_iNavalCombatExperience":
                                  "m_iCombatExperience")=12;
        const auto blocked=runtime::quote_bnw_unit_death_xp(
            source,support_death,classes,support,xp,1);
        check(blocked.setter_branch_unported && blocked.current_after==2,
              "BNW death XP failed to identify setter threshold branch");
        rejects([&]{(void)runtime::project_bnw_unit_death_xp_safe(
            source,blocked,classes,support,xp);});
        const auto skipped=runtime::quote_bnw_unit_death_xp(
            source,support_death,classes,support,xp,-1);
        check(!skipped.triggered && skipped.loss==0,
              "BNW death XP applied to uncredited kill");
    }
    {
        auto source=world(classes,0);
        const auto plan=support_plan(source,classes,support);
        auto bad_rules=xp;bad_rules.units[0].unit_hash++;
        rejects([&]{(void)runtime::quote_bnw_unit_death_xp(
            source,plan,classes,support,bad_rules,1);});
        bad_rules=xp;bad_rules.units[0].domain=
            runtime::BnwDeathXpDomain::hover;
        rejects([&]{(void)runtime::quote_bnw_unit_death_xp(
            source,plan,classes,support,bad_rules,1);});
        bad_rules=xp;bad_rules.loss_percent=INT32_MAX;
        rejects([&]{(void)runtime::quote_bnw_unit_death_xp(
            source,plan,classes,support,bad_rules,1);});
        counter(source.all_players->players[0].core,
                "m_iLifetimeCombatExperience")=INT32_MIN;
        rejects([&]{(void)runtime::quote_bnw_unit_death_xp(
            source,plan,classes,support,xp,1);});
    }
    std::cout << "PASS BNW detached unit death XP quote\n";
}
} // namespace
int main() {
    try {test();return 0;}
    catch(const std::exception &error) {
        std::cerr << "FAIL BNW unit death XP quote: " << error.what() << '\n';
        return 1;
    }
}
