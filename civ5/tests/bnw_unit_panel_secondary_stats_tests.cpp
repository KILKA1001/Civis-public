#include "civ5/ui/bnw_unit_panel_secondary_stats.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace civ5;
namespace {
void check(bool ok,const char *message) { if (!ok) throw std::runtime_error(message); }
std::int64_t &scalar(saves::UnitCoreObservation &core,std::string_view name) {
    for (auto &field : core.fields) if (field.name && std::string_view(field.name)==name && field.values.size()==1)
        return field.values.front();
    throw std::runtime_error("Expected current selected unit scalar");
}
void run(int argc,char **argv) {
    check(argc==3 || argc==4,"Expected actual save/game root/optional installed SDK root");
    if (argc==4) for (const auto &source : runtime::kBnwUnitMovementSources)
        check(runtime::identify_file(std::filesystem::path(argv[3])/source.path).sha256==source.sha256,
            "Actual native Range/base-ranged getter source pin differs");
    check(runtime::identify_file(std::filesystem::path(argv[2]) /
        "Assets/DLC/Expansion2/UI/InGame/WorldView/UnitPanel.lua").sha256==
        "5b4be7346a6570196e94e638094aa3d14227e8637d7e335a73983d7b35764524",
        "Actual selected original UnitPanel branch source differs");
    auto bootstrap=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);
    check(bootstrap.preview.host && !bootstrap.preview.blocker,"Actual selected source unavailable");
    auto &host=*bootstrap.preview.host;
    const runtime::BnwUnitOrderKey requested{0,16385}; // Explicit source-test selection input.
    auto presentation=runtime::start_bnw_unit_presentation_client(host.session().state(),host.identity(),host.database());
    runtime::select_bnw_unit_presentation(host.session().state(),presentation,host.database(),requested,
        presentation.client_revision());
    check(presentation.selected_unit()==requested,"Actual native selection owner did not select requested source unit");
    const auto selected=presentation.selected_unit()->second;
    ui::BnwUnitPanelSecondaryStatsBridge bridge(host.session(),host.database(),host.types(),host.identity(),0);
    const auto ready=host.session().readiness().blockers;
    const auto source=bridge.capture(host.session(),selected,31);
    check(source && source->values.info_domain==2 && source->values.range && source->values.base_ranged_combat &&
        source->values.base_ranged_combat==0 && bridge.matches(host.session(),*source,selected,31),
        "Actual selected warrior native Info reads unavailable");
    check(host.session().readiness().blockers==ready && !host.session().readiness().can_continue(),
        "Read-only secondary stats changed turn readiness");
    std::cout << "actual_selected_land_unit=" << selected << " native_range=" << *source->values.range
        << " base_ranged=" << *source->values.base_ranged_combat << '\n';
    // Explicitly detached air fixture using actual selected gameplay Info.
    // No air unit, live selection, UI frame or render receipt is fabricated.
    const auto *fighter=host.types().find("UNIT_FIGHTER");
    check(fighter && fighter->table=="Units","Actual selected fighter Info unavailable");
    std::int64_t info_range=-1, info_combat=-1;
    const auto query=host.database().query_rows({"SELECT Range,RangedCombat FROM Units WHERE ID=?",{fighter->id}},
        [&](const content::RulesRowView &row) { info_range=row.integer("Range"); info_combat=row.integer("RangedCombat"); },2);
    check(query.all_succeeded() && info_range>=0 && info_combat>0,"Actual source air fixture Info invalid");
    const auto air=ui::read_bnw_unit_panel_secondary_info(host.database(),host.types(),fighter->hash,2);
    check(air.unit_type==fighter->id && air.info_domain==1 && air.range==info_range+2 &&
        air.base_ranged_combat==info_combat && air.rebase_range_multiplier &&
        air.range_blocker.empty() && air.base_ranged_blocker.empty() && air.rebase_blocker.empty(),
        "Detached real-Info air getter fixture differs from native sources");
    const auto unknown=ui::read_bnw_unit_panel_secondary_info(host.database(),host.types(),0,0);
    check(!unknown.range && !unknown.base_ranged_combat && !unknown.range_blocker.empty() &&
        !unknown.base_ranged_blocker.empty(),"Unbound native Info became zero/range strength");
    const auto overflow=ui::read_bnw_unit_panel_secondary_info(host.database(),host.types(),fighter->hash,
        std::numeric_limits<std::int64_t>::max());
    check(!overflow.range && !overflow.range_blocker.empty() && overflow.base_ranged_combat==info_combat,
        "Invalid extra range erased independent native base-ranged read or became zero");
    std::cout << "DETACHED_ACTUAL_DB_AIR_FIXTURE native_range=" << *air.range
        << " base_ranged=" << *air.base_ranged_combat
        << " rebase_multiplier=" << *air.rebase_range_multiplier << " no_live_air_claim=1\n";
    auto &state=host.session().mutable_state();
    saves::UnitCoreObservation *core=nullptr;
    for (auto &owner : state.world.all_players->players) if (owner.saved_player_id==0)
        for (auto &unit : owner.units.units) if (scalar(unit.core,"m_iID")==selected) core=&unit.core;
    check(core,"Current selected source unit is absent");
    const auto original_extra=scalar(*core,"m_iExtraRange");
    ++scalar(*core,"m_iExtraRange"); // Labeled same-live-owner fault probe, no source/save write.
    check(!bridge.matches(host.session(),*source,selected,31),"Current extra range mutation accepted stale native quote");
    const auto changed=bridge.capture(host.session(),selected,31);
    check(changed && changed->values.range==*source->values.range+1 &&
        changed->values.base_ranged_combat==source->values.base_ranged_combat,
        "Native same-frame range requery did not follow actual current operand");
    scalar(*core,"m_iExtraRange")=original_extra;
    const auto original_type=state.maintenance.players.at(0).units.at(selected).type;
    state.maintenance.players.at(0).units.at(selected).type=-1;
    check(!bridge.capture(host.session(),selected,32),"Mismatched current maintenance binding accepted");
    state.maintenance.players.at(0).units.at(selected).type=original_type;
    check(!bridge.capture(host.session(),selected,0) && !bridge.capture(host.session(),-1,32) &&
        !bridge.matches(host.session(),*source,selected,32),"Invalid source selection/frame accepted");
    const auto generation=host.session().state_generation();
    const auto checkpoint=host.session().checkpoint(); host.session().restore(checkpoint);
    host.session().restore(checkpoint);
    check(host.session().state_generation()>generation+1 && !bridge.capture(host.session(),selected,33),
        "Repeated replaced borrowed source owner accepted");
    ui::BnwUnitPanelSecondaryStatsBridge rebound(host.session(),host.database(),host.types(),host.identity(),0);
    check(rebound.capture(host.session(),selected,33).has_value(),"Explicit secondary read owner rebind failed");
    std::cout << "PASS actual selected source read + detached real-DB air fixture; current/lifecycle guards\n";
}
}
int main(int argc,char **argv) {
    try { run(argc,argv); } catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
