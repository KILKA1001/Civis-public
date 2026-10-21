#include "civ5/runtime/bnw_unit_class_death.hpp"
#include "civ5/content/info_types.hpp"
#include <iostream>
#include <stdexcept>

namespace {
using namespace civ5;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
template<class F> void rejects(F &&f) {
    bool rejected=false;
    try {f();} catch(const std::exception &) {rejected=true;}
    check(rejected,"Invalid BNW unit-class death input was accepted");
}
saves::SavedSessionSnapshot synthetic(std::uint32_t unit_hash,
                                       std::uint32_t class0_hash,
                                       std::uint32_t class1_hash) {
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
    saves::TeamHashedArray team_classes{};
    team_classes.name="unit_class_count";team_classes.wire_count=2;
    team_classes.rows.push_back({class0_hash,{1}});
    team_classes.rows.push_back({class1_hash,{0}});
    source.teams->teams[0].hashes.push_back(std::move(team_classes));
    source.all_players.emplace();source.all_players->layout=bnw;
    source.all_players->players.resize(64);
    for(std::size_t p=0;p<64;++p) {
        auto &player=source.all_players->players[p];
        player.index=p;player.saved_player_id=static_cast<std::int64_t>(p);
        player.units.layout=bnw;player.units.header.layout=bnw;
    }
    auto &player=source.all_players->players[0];
    player.core.layout=bnw;
    saves::PlayerCoreField player_classes{};
    player_classes.name="m_paiUnitClassCount";
    player_classes.encoding=saves::PlayerCoreEncoding::vector_i32;
    player_classes.wire_count=2;player_classes.values={1,0};
    player.core.fields.push_back(std::move(player_classes));
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
        field.name=name;field.encoding=encoding;field.values.push_back(value);
        unit.core.fields.push_back(std::move(field));
    };
    put("m_iID",saves::UnitCoreEncoding::i32,0);
    put("m_eOwner",saves::UnitCoreEncoding::i32,0);
    put("m_iX",saves::UnitCoreEncoding::i32,0);
    put("m_iY",saves::UnitCoreEncoding::i32,0);
    put("m_eUnitType",saves::UnitCoreEncoding::i32,0);
    put("idUnitType",saves::UnitCoreEncoding::hash32,unit_hash);
    return source;
}
void test() {
    content::RulesDatabase database;
    database.apply_sql(
        "CREATE TABLE UnitClasses(ID INTEGER,Type TEXT);"
        "INSERT INTO UnitClasses VALUES(0,'UNITCLASS_TEST_A'),"
        "(1,'UNITCLASS_TEST_B');"
        "CREATE TABLE Units(ID INTEGER,Type TEXT,Class TEXT);"
        "INSERT INTO Units VALUES(0,'UNIT_TEST_A','UNITCLASS_TEST_A'),"
        "(1,'UNIT_TEST_B','UNITCLASS_TEST_B');");
    const auto types=database.cache_info_types();
    const auto rules=runtime::load_bnw_unit_class_death_rules(database,types);
    check(rules.class_slots==2 && rules.units.size()==2 &&
          rules.units.at(0).unit_class==0 &&
          rules.units.at(1).unit_class==1,
          "BNW unit-class rules did not bind active class IDs");
    // Encoding regression against real SQL PrefetchCollection semantics.
    // This is a format/formula fixture, not a live-game readiness proof.
    content::RulesDatabase sparse_database;
    sparse_database.apply_sql("CREATE TABLE UnitClasses(ID INTEGER,Type TEXT);"
        "INSERT INTO UnitClasses VALUES(0,'UNITCLASS_TEST_A'),(3,'UNITCLASS_TEST_B');"
        "CREATE TABLE Units(ID INTEGER,Type TEXT,Class TEXT);"
        "INSERT INTO Units VALUES(0,'UNIT_TEST_A','UNITCLASS_TEST_A'),(1,'UNIT_TEST_B','UNITCLASS_TEST_B');");
    const auto sparse_types=sparse_database.cache_info_types();
    const auto sparse=runtime::load_bnw_unit_class_death_rules(sparse_database,sparse_types);
    check(sparse.class_slots==4&&sparse.class_hashes.size()==4&&sparse.class_hashes[0]==rules.class_hashes[0]&&
        sparse.class_hashes[1]==0&&sparse.class_hashes[2]==0&&sparse.class_hashes[3]==rules.class_hashes[1]&&
        sparse.units.at(1).unit_class==3,"Native SQL holes did not retain their NULL UnitClass slots");
    auto sparse_source=synthetic(content::saved_info_type_hash("UNIT_TEST_A"),sparse.class_hashes[0],sparse.class_hashes[3]);
    auto& sparse_team=sparse_source.teams->teams[0].hashes[0];sparse_team.wire_count=4;
    sparse_team.rows.insert(sparse_team.rows.begin()+1,{0,{}});sparse_team.rows.insert(sparse_team.rows.begin()+1,{0,{}});
    auto& sparse_player=sparse_source.all_players->players[0].core.fields[0];sparse_player.wire_count=4;sparse_player.values={1,0,0,0};
    const auto sparse_removal=runtime::plan_bnw_unit_removal(sparse_source,0,0);
    const auto sparse_plan=runtime::plan_bnw_unit_class_death(sparse_source,sparse_removal,sparse);
    check(sparse_plan.unit_class==0&&sparse_plan.team_count_before==1&&sparse_plan.player_count_before==1,"Native NULL hashed slots obscured the actual live class counter");
    auto bad_null=sparse_source;bad_null.teams->teams[0].hashes[0].rows[1].values={0};
    rejects([&]{runtime::plan_bnw_unit_class_death(bad_null,sparse_removal,sparse);});
    auto source=synthetic(content::saved_info_type_hash("UNIT_TEST_A"),
        rules.class_hashes[0],rules.class_hashes[1]);
    const auto removal=runtime::plan_bnw_unit_removal(source,0,0);
    const auto plan=runtime::plan_bnw_unit_class_death(source,removal,rules);
    check(plan.team==0 && plan.unit_class==0 &&
          plan.team_count_before==1 && plan.player_count_before==1,
          "BNW unit-class death preflight chose wrong counters");
    auto projected=runtime::project_bnw_unit_class_death(source,plan,rules);
    check(projected.teams->teams[0].hashes[0].rows[0].values[0]==0 &&
          projected.all_players->players[0].core.fields[0].values[0]==0 &&
          projected.all_players->players[0].units.units.size()==1 &&
          source.teams->teams[0].hashes[0].rows[0].values[0]==1,
          "Detached class decrement changed wrong state or source");
    {
        auto bad=source;
        bad.teams->teams[0].hashes[0].rows[0].type_hash=rules.class_hashes[1];
        rejects([&]{(void)runtime::plan_bnw_unit_class_death(
            bad,removal,rules);});
        bad=source;
        bad.teams->teams[0].hashes[0].rows[0].values[0]=0;
        rejects([&]{(void)runtime::plan_bnw_unit_class_death(
            bad,removal,rules);});
        bad=source;
        bad.pregame->archive.integer_arrays["team_types"][0]=63;
        rejects([&]{(void)runtime::plan_bnw_unit_class_death(
            bad,removal,rules);});
        bad=source;
        bad.all_players->players[0].units.units[0].core.fields.back()
            .values[0]=123456;
        rejects([&]{(void)runtime::plan_bnw_unit_class_death(
            bad,removal,rules);});
    }
    {
        auto wrong=plan;wrong.team_count_before=2;
        rejects([&]{(void)runtime::project_bnw_unit_class_death(
            source,wrong,rules);});
        auto incomplete=rules;incomplete.class_hashes.pop_back();
        rejects([&]{(void)runtime::plan_bnw_unit_class_death(
            source,removal,incomplete);});
    }
    std::cout << "PASS BNW detached unit-class death counters\n";
}
}
int main() {
    try {test();return 0;}
    catch(const std::exception &error) {
        std::cerr << "FAIL BNW unit-class death: " << error.what() << '\n';
        return 1;
    }
}
