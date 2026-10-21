#include "civ5/runtime/bnw_diplomacy_scratchpad_load.hpp"
#include "civ5/runtime/bnw_game_clock_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <variant>

namespace {
void check(bool condition,const char *reason) {
    if(!condition) throw std::runtime_error(reason);
}
struct Cleanup {
    std::filesystem::path path;
    ~Cleanup() {std::error_code error;std::filesystem::remove(path,error);}
};
void selected(const std::filesystem::path &save,
              const std::filesystem::path &game_root) {
    const auto original_sha=civ5::runtime::identify_file(save).sha256;
    auto boot=civ5::ui::bootstrap_selected_bnw_preview(save,game_root,0);
    check(!boot.preview.blocker && boot.preview.host,
          "Selected Gandhi live host is unavailable");
    auto &host=*boot.preview.host;
    auto source=host.session().source();
    auto state=host.session().state();
    const auto &identity=host.identity();
    const auto original=civ5::runtime::
        restore_bnw_diplomacy_scratchpad_after_player_read(
            source,state,host.database(),identity);
    check(original.current() && original.matches_load_clock(state) &&
          original.value_at(0)==-1,
          "Original Gandhi post-Read scratchpad did not reset");

    // Equal clocks/rosters do not bind a scratchpad to another map, engine
    // or GameCore origin. Exercise both initial restoration and retained use.
    const auto rejects_foreign_world=[&](const auto &current_state,const auto &scratchpad){
        for(int which=0;which<3;++which){
            auto foreign_world=current_state;
            if(which==0)foreign_world.world.map_header.guid[0]^=1;
            else if(which==1)foreign_world.world.pregame->engine_guid[0]^=1;
            else foreign_world.world.pregame->gamecore_guid[0]^=1;
            const auto foreign=civ5::runtime::restore_bnw_diplomacy_scratchpad_after_player_read(
                source,foreign_world,host.database(),identity);
            check(!foreign.current() && foreign.gate()==civ5::runtime::
                      BnwDiplomacyScratchpadLoadGate::source_identity_inconsistent,
                  "Same-clock foreign world received a post-Read scratchpad");
            check(!scratchpad.matches_load_clock(foreign_world),
                  "Retained scratchpad accepted same-clock foreign world identity");
        }
        check(scratchpad.matches_load_clock(current_state),"Foreign-world refusal changed genuine scratchpad");
    };
    rejects_foreign_world(state,original);

    source.source.container=civ5::saves::SaveContainerKind::civis;
    const auto unchanged=civ5::runtime::
        restore_bnw_diplomacy_scratchpad_after_player_read(
            source,state,host.database(),identity);
    check(unchanged.current() && unchanged.value_at(63)==std::nullopt,
          "Unchanged Civis import lost its post-Read scratchpad");

    ++state.setup.game_turn;
    ++state.maintenance.elapsed_turns;
    ++state.world.pregame->archive.integers.at("game_turn");
    auto &prefix=std::get<civ5::saves::Expansion2PrefixObservation>(
        state.world.game_state.prefix);
    ++prefix.scalars[1];
    const auto no_clock=civ5::runtime::
        restore_bnw_diplomacy_scratchpad_after_player_read(
            source,state,host.database(),identity);
    check(!no_clock.current() && no_clock.gate()==civ5::runtime::
              BnwDiplomacyScratchpadLoadGate::not_initial_load_boundary,
          "Advanced Civis world without clock section was accepted");

    auto section=civ5::runtime::make_bnw_game_clock_checkpoint(
        state,source.snapshot,identity);
    source.source.live_bnw_game_clock=section.bytes;
    const auto advanced=civ5::runtime::
        restore_bnw_diplomacy_scratchpad_after_player_read(
            source,state,host.database(),identity);
    check(advanced.current() && advanced.matches_load_clock(state) &&
          advanced.value_at(0)==-1 &&
          advanced.value_at(62)==-1,
          "Advanced Civis clock did not restore transient scratchpad");
    rejects_foreign_world(state,advanced);

    // Exercise the physical Civis writer and the normal selected-save loader,
    // not just the scratchpad helper against a synthetic in-memory source.
    auto document=civ5::saves::make_civis_import_document(
        host.session().source().source,original_sha);
    document.sections.push_back(section);
    const auto suffix=std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    Cleanup output{std::filesystem::current_path()/
        ("bnw-diplomacy-scratchpad-"+suffix+".CivisSave")};
    civ5::saves::write_civis_save(output.path,document);
    auto resumed=civ5::ui::bootstrap_selected_bnw_preview(
        output.path,game_root,0);
    check(!resumed.preview.blocker && resumed.preview.host,
          "Advanced Civis clock failed normal selected-save load");
    const auto &loaded=resumed.preview.host->session();
    check(loaded.state().diplomacy_scratchpad.current() &&
          loaded.state().diplomacy_scratchpad.matches_load_clock(
              loaded.state()) &&
          loaded.state().diplomacy_scratchpad.value_at(0)==-1 &&
          loaded.state().setup.game_turn==state.setup.game_turn &&
          !loaded.readiness().can_continue() &&
          civ5::runtime::identify_file(save).sha256==original_sha,
          "Advanced Civis import lost the scratchpad, clock or readiness gate");
    rejects_foreign_world(loaded.state(),loaded.state().diplomacy_scratchpad);

    ++state.setup.game_turn;
    check(!civ5::runtime::restore_bnw_diplomacy_scratchpad_after_player_read(
              source,state,host.database(),identity).current(),
          "Divergent setup clock was accepted");
    --state.setup.game_turn;
    source.source.live_bnw_game_clock->back()=2;
    check(!civ5::runtime::restore_bnw_diplomacy_scratchpad_after_player_read(
              source,state,host.database(),identity).current(),
          "Malformed Civis clock section was accepted");
    source.source.live_bnw_game_clock=section.bytes;
    source.source.container=civ5::saves::SaveContainerKind::civ5;
    check(!civ5::runtime::restore_bnw_diplomacy_scratchpad_after_player_read(
              source,state,host.database(),identity).current(),
          "Original Civ5 container accepted an advanced live clock");
    std::cout<<"PASS selected Gandhi diplomacy scratchpad after advanced Civis clock\n";
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected BNW save and game root");
        selected(argv[1],argv[2]);
        return 0;
    } catch(const std::exception &error) {
        std::cerr<<"FAIL BNW Civis diplomacy scratchpad: "<<error.what()<<'\n';
        return 1;
    }
}
