#include "civ5/runtime/bnw_fresh_tactical_map.hpp"
#include <algorithm>
#include <array>
#include <iostream>
#include <thread>
#include <type_traits>
#include <variant>
using namespace civ5;
namespace {
using Storage=runtime::detail::BnwTacticalMapStorage;
using Error=runtime::BnwFreshTacticalMapError;
static_assert(!std::is_default_constructible_v<runtime::BnwFreshTacticalMap>);
static_assert(!std::is_constructible_v<runtime::BnwFreshTacticalMap,
    std::shared_ptr<const runtime::BnwCreationConfig>,const content::RulesDatabase&>);
static_assert(!std::is_copy_constructible_v<runtime::BnwFreshTacticalMap>);
void need(bool v,const char* text) {if(!v) throw std::runtime_error(text);}
template<class F> void refuses(F f,Error expected) {
    try {f();} catch(const runtime::BnwFreshTacticalMapRefusal& e) {need(e.code()==expected,"Wrong Tactical error");return;}
    throw std::runtime_error("Unavailable Tactical operation accepted");
}
std::unique_ptr<content::RulesDatabase> fixture(const char* extra) {
    auto db=std::make_unique<content::RulesDatabase>();
    db->apply_sql("CREATE TABLE Defines(Name TEXT,Value INTEGER);");
    if(*extra) db->apply_sql(extra);
    db->seal_immutable_preview_fingerprints();return db;
}
void verify_constructor(const runtime::BnwTacticalMapSnapshot& s) {
    need(!s.plots_allocated && s.player_pointer_null && !s.is_built && !s.at_war && !s.ignore_los &&
         s.dominance_percentage==25 && s.unit_strength_multiplier==5 && s.tactical_range==8 &&
         s.best_friendly_range==0 && s.num_plots==0 && s.turn_built==-1 && s.dominance_zones==0 && s.enemy_units==0,
         "Exact native map constructor scalars/member containers");
    // Independent SDK literals: do not derive expectations from snapshot
    // default declarations, which could share the same implementation error.
    const auto& z=s.temporary_zone;
    need(z.id==-1 && z.territory_type==0 && z.dominance_flag==0 && z.owner==-1 && z.city_id==-1 && z.area_id==0 &&
         z.friendly_strength==0 && z.enemy_strength==0 && z.friendly_ranged_strength==0 && z.enemy_ranged_strength==0 &&
         z.friendly_units==0 && z.enemy_units==0 && z.friendly_ranged_units==0 && z.enemy_ranged_units==0 && z.enemy_naval_units==0 &&
         z.value==0 && z.closest_enemy_range==2147483647 && !z.water && !z.naval_invasion && z.center_pointer_null,
         "Independent native temporary-zone constructor fields20");
}
void verify_cells(const Storage& s,std::int32_t count) {
    for(std::int32_t i=0;i<count;++i) {
        const auto c=s.cell(i);
        need(c.flags==0 && std::all_of(c.unit_pointer_null.begin(),c.unit_pointer_null.end(),[](bool n){return n;}) &&
             c.defense_modifier==0 && c.deployment_score==0 && c.target_type==0 && c.dominance_zone==-1,
             "Every allocated native cell constructor+Clear field");
    }
}
void cases() {
    // Detached owned allocations exercise production storage, never Game
    // issuance. No test friend/public factory/boolean can issue the owner.
    auto db=fixture("INSERT INTO Defines VALUES('AI_TACTICAL_MAP_DOMINANCE_PERCENTAGE',-17);");
    auto allocation=std::make_shared<Storage>();std::weak_ptr<Storage> weak=allocation;
    const auto* actual=allocation.get();need(actual!=nullptr,"Actual storage object pointer");
    verify_constructor(allocation->observe());
    refuses([&]{(void)allocation->cell(0);},Error::cell_unavailable);
    const auto before=allocation->observe();
    refuses([&]{allocation->initialize_detached(-1,*db);},Error::native_count_unavailable);
    refuses([&]{allocation->initialize_detached(1048577,*db);},Error::native_count_unavailable);
    need(allocation->observe()==before,"Count preflight does not erase existing storage");
    allocation->initialize_detached(7,*db);verify_cells(*allocation,7);
    auto initialized=allocation->observe();
    need(initialized.plots_allocated && initialized.num_plots==7 && initialized.dominance_percentage==-17 &&
         initialized.unit_strength_multiplier==5 && initialized.tactical_range==8 && !initialized.is_built,
         "Native allocation/count/Define stores preserve negative, do not build map");
    refuses([&]{(void)allocation->cell(-1);},Error::cell_unavailable);
    refuses([&]{(void)allocation->cell(7);},Error::cell_unavailable);
    auto alias=allocation;allocation.reset();need(alias.get()==actual && !weak.expired(),"Held SAME allocation retains pointer/lifetime");
    alias.reset();need(weak.expired(),"No parent/storage reference cycle");
    Storage missing;auto absent=fixture("");missing.initialize_detached(0,*absent);
    need(missing.observe().plots_allocated && missing.observe().num_plots==0 && missing.observe().dominance_percentage==0,
         "Zero-length allocation still reaches cached missing Define0");
    refuses([&]{(void)missing.cell(0);},Error::cell_unavailable);
    missing.initialize_detached(3,*db);verify_cells(missing,3);
    need(missing.observe().num_plots==3,"Actual SDK re-Init replaces old array");
    for(const auto* sql:{"INSERT INTO Defines VALUES('AI_TACTICAL_MAP_DOMINANCE_PERCENTAGE',2147483647);",
                         "INSERT INTO Defines VALUES('AI_TACTICAL_MAP_DOMINANCE_PERCENTAGE',-2147483648);"}) {
        auto edge=fixture(sql);Storage s;s.initialize_detached(1,*edge);verify_cells(s,1);
        const auto val=edge->global_define("AI_TACTICAL_MAP_DOMINANCE_PERCENTAGE",content::GlobalDefineKind::integer);
        need(s.observe().dominance_percentage==std::get<std::int64_t>(val.value),"Native signed int32 endpoints");
    }
    auto wide=fixture("INSERT INTO Defines VALUES('AI_TACTICAL_MAP_DOMINANCE_PERCENTAGE',2147483648);");
    Storage partial;
    refuses([&]{partial.initialize_detached(2,*wide);},Error::native_define_unavailable);
    need(partial.observe().plots_allocated && partial.observe().num_plots==2 && partial.observe().dominance_percentage==25,
         "Late Define refusal retains earlier allocation/count and old dominance");
    verify_cells(partial,2);
    refuses([&]{partial.initialize_detached(1,*db);},Error::poisoned);
    bool foreign=false;
    std::thread thread([&]{try{partial.initialize_detached(0,*db);}catch(const runtime::BnwFreshTacticalMapRefusal& e){foreign=e.code()==Error::wrong_thread;}});
    thread.join();need(foreign,"Constructor thread guard precedes DB/state/failure reads");
    Storage retired;auto unsealed=std::make_unique<content::RulesDatabase>();
    refuses([&]{retired.initialize_detached(2,*unsealed);},Error::parent_retired);
    verify_constructor(retired.observe());
}
void actual_rules(const std::filesystem::path& installation) {
    content::Catalog catalog;
    catalog.packages.push_back(content::read_manifest(installation/"Assets/DLC/Expansion/Expansion1.Civ5Pkg"));
    catalog.packages.push_back(content::read_manifest(installation/"Assets/DLC/Expansion2/Expansion2.Civ5Pkg"));
    constexpr std::array<std::size_t,2> packages{0,1};
    auto selected=runtime::bootstrap_bnw_fresh_game_content(installation,catalog,packages);
    need(selected.content_ready() && selected.rules,"Actual selected BNW immutable rules source missing");
    selected.rules->seal_immutable_preview_fingerprints();
    Storage s;s.initialize_detached(5,*selected.rules);verify_cells(s,5);
    const auto value=selected.rules->global_define("AI_TACTICAL_MAP_DOMINANCE_PERCENTAGE",content::GlobalDefineKind::integer);
    need(value.error.empty() && s.observe().dominance_percentage==std::get<std::int64_t>(value.value),"Actual selected cached Define getter");
    std::cout<<"actual_rules_storage_cells=5 genuine_Game_issuance=UNOBSERVED genuine_map_count=UNOBSERVED built=0\n";
}
} // namespace
int main(int argc,char** argv) {
    try {need(argc==2,"Expected canonical BNW installation");cases();actual_rules(std::filesystem::path(argv[1]));
         std::cout<<"Tactical detached storage passed; Game predecessors remain mandatory\n";return 0;}
    catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
