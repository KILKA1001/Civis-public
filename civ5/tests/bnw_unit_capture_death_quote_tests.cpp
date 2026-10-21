#include "civ5/runtime/bnw_unit_capture_death_quote.hpp"
#include "civ5/content/info_types.hpp"
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
    check(rejected,"Invalid BNW capture definition was accepted");
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
void change(saves::SavedUnitObservation &unit,std::string_view wanted,
            std::int64_t value) {
    for(auto &field:unit.core.fields)
        if(field.name && wanted==field.name) {
            field.values={value};return;
        }
    throw std::runtime_error("Synthetic BNW unit field missing");
}
saves::UnitTailNode tail_scalar(const char *name,const char *encoding,
                                std::int64_t value) {
    saves::UnitTailNode node{};
    node.name=name;node.encoding=encoding;node.values={value};return node;
}
saves::SavedSessionSnapshot world(std::uint32_t unit_hash,
                                  std::uint32_t class_hash) {
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
    auto counts=std::vector<std::int64_t>(64);counts[0]=1;
    area.arrays.push_back({"units_per_player",1,std::move(counts)});
    source.pregame.emplace();source.pregame->layout=bnw;
    source.pregame->archive.integer_arrays["team_types"]=
        std::vector<std::int64_t>(64,0);
    source.teams.emplace();source.teams->layout=bnw;
    source.teams->teams.resize(64);
    for(std::size_t t=0;t<64;++t) source.teams->teams[t].index=t;
    saves::TeamHashedArray hashed{};
    hashed.name="unit_class_count";hashed.wire_count=1;
    hashed.rows.push_back({class_hash,{1}});
    source.teams->teams[0].hashes.push_back(std::move(hashed));
    source.all_players.emplace();source.all_players->layout=bnw;
    source.all_players->players.resize(64);
    for(std::size_t p=0;p<64;++p) {
        auto &player=source.all_players->players[p];
        player.index=p;player.saved_player_id=p;
        player.units.layout=bnw;player.units.header.layout=bnw;
    }
    auto &player=source.all_players->players[0];player.core.layout=bnw;
    saves::PlayerCoreField classes{};
    classes.name="m_paiUnitClassCount";
    classes.encoding=saves::PlayerCoreEncoding::vector_i32;
    classes.wire_count=1;classes.values={1};
    player.core.fields.push_back(std::move(classes));
    scalar(player.core,"m_iNumBuilders",1);
    scalar(player.core,"m_iUnhappinessFromUnits",0);
    scalar(player.core,"m_iExtraUnitCost",0);
    scalar(player.core,"m_iNumNukeUnits",0);
    scalar(player.core,"m_iNumMilitaryUnits",0);
    scalar(player.core,"m_iCombatExperience",0);
    scalar(player.core,"m_iNavalCombatExperience",0);
    scalar(player.core,"m_iLifetimeCombatExperience",20);
    auto &units=player.units;
    units.header.source_slots=1;
    units.header.fields={{"num_slots",1},{"last_index",0},
        {"free_list_head",-1},{"free_list_count",0},{"current_id",8192}};
    units.header.next_free_indices={-1};units.header.wire_unit_count=1;
    units.units.emplace_back();
    auto &unit=units.units.back();unit.core.layout=bnw;unit.tail.layout=bnw;
    unit_scalar(unit,"m_iID",saves::UnitCoreEncoding::i32,0);
    unit_scalar(unit,"m_eOwner",saves::UnitCoreEncoding::i32,0);
    unit_scalar(unit,"m_eOriginalOwner",saves::UnitCoreEncoding::i32,0);
    unit_scalar(unit,"m_iX",saves::UnitCoreEncoding::i32,0);
    unit_scalar(unit,"m_iY",saves::UnitCoreEncoding::i32,0);
    unit_scalar(unit,"m_eUnitType",saves::UnitCoreEncoding::i32,0);
    unit_scalar(unit,"idUnitType",saves::UnitCoreEncoding::hash32,unit_hash);
    unit_scalar(unit,"m_iExperience",saves::UnitCoreEncoding::i32,20);
    unit_scalar(unit,"m_eCapturingPlayer",saves::UnitCoreEncoding::i32,-1);
    unit_scalar(unit,"m_bEmbarked",saves::UnitCoreEncoding::boolean,0);
    unit_scalar(unit,"m_bCapturedAsIs",saves::UnitCoreEncoding::boolean,0);
    saves::UnitTailNode religion{};
    religion.name="religion";religion.encoding="record";
    religion.children.push_back(tail_scalar("version","u32",2));
    religion.children.push_back(tail_scalar("religion","i32",-1));
    religion.children.push_back(tail_scalar("strength","i32",0));
    religion.children.push_back(tail_scalar("spreads_left","i32",0));
    unit.tail.blocks.push_back(std::move(religion));
    return source;
}
void test() {
    const auto unit_hash=content::saved_info_type_hash("UNIT_WORKER");
    const auto class_hash=content::saved_info_type_hash("UNITCLASS_WORKER");
    runtime::BnwUnitClassDeathRules classes;
    classes.class_slots=1;classes.class_hashes={class_hash};
    classes.units.emplace(0,runtime::BnwUnitClassBinding{
        0,0,unit_hash,class_hash});
    runtime::BnwUnitSupportRules support;
    support.units.emplace(0,runtime::BnwUnitSupportRule{
        0,unit_hash,true,false,100,0,0,-1});
    runtime::BnwDeathXpRules xp_rules;
    xp_rules.loss_percent=50;
    xp_rules.units.emplace(0,runtime::BnwDeathXpUnitRule{
        0,unit_hash,runtime::BnwDeathXpDomain::land});
    auto source=world(unit_hash,class_hash);
    const auto removal=runtime::plan_bnw_unit_removal(source,0,0);
    const auto class_plan=runtime::plan_bnw_unit_class_death(
        source,removal,classes);
    const auto support_plan=runtime::plan_bnw_unit_support_death(
        source,class_plan,classes,support);
    const auto xp=runtime::quote_bnw_unit_death_xp(
        source,support_plan,classes,support,xp_rules,1);
    const auto capture=runtime::quote_bnw_unit_capture_death_no_player(
        source,xp,classes,support,xp_rules);
    check(capture.old_player==0 && capture.original_owner==0 &&
          capture.old_type==0 && capture.capturing_player==-1 &&
          capture.capture_type==-1 && !capture.capture_valid &&
          !capture.can_commit_kill && capture.x==0 && capture.y==0 &&
          capture.religion==-1 && capture.religious_strength==0 &&
          capture.spreads_left==0 && !capture.embarked,
          "BNW no-capture definition differs from SDK branch");
    auto &unit=source.all_players->players[0].units.units[0];
    change(unit,"m_bCapturedAsIs",1);
    const auto as_is=runtime::quote_bnw_unit_capture_death_no_player(
        source,xp,classes,support,xp_rules);
    check(as_is.captured_as_is && as_is.capture_type==0 &&
          !as_is.capture_valid,
          "BNW captured-as-is with NO_PLAYER should still be invalid");
    change(unit,"m_bCapturedAsIs",0);
    change(unit,"m_eCapturingPlayer",1);
    rejects([&]{(void)runtime::quote_bnw_unit_capture_death_no_player(
        source,xp,classes,support,xp_rules);});
    change(unit,"m_eCapturingPlayer",-1);
    unit.tail.blocks[0].children.pop_back();
    rejects([&]{(void)runtime::quote_bnw_unit_capture_death_no_player(
        source,xp,classes,support,xp_rules);});
    unit.tail.blocks[0].children.push_back(
        tail_scalar("spreads_left","i32",0));
    change(unit,"m_eOriginalOwner",64);
    rejects([&]{(void)runtime::quote_bnw_unit_capture_death_no_player(
        source,xp,classes,support,xp_rules);});
    change(unit,"m_eOriginalOwner",0);
    auto stale=xp;stale.loss++;
    rejects([&]{(void)runtime::quote_bnw_unit_capture_death_no_player(
        source,stale,classes,support,xp_rules);});
    std::cout << "PASS BNW detached no-player capture definition\n";
}
} // namespace
int main() {
    try {test();return 0;}
    catch(const std::exception &error) {
        std::cerr << "FAIL BNW capture definition: " << error.what() << '\n';
        return 1;
    }
}
