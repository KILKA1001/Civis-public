#include "civ5/runtime/bnw_live_player_core_export.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <chrono>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
template<class F> void rejects(F &&f) {
    bool rejected=false;
    try {f();} catch(const std::exception &) {rejected=true;}
    check(rejected,"Invalid BNW partial export was accepted");
}
saves::PlayerCoreField &field(saves::SavedPlayerObservation &player,
                              std::string_view wanted) {
    for(auto &part:player.core.fields)
        if(part.name && std::string_view(part.name)==wanted) return part;
    throw std::runtime_error("Selected BNW player core field missing");
}
saves::SavedSessionDecodeContext context(bool permit_overlay=false) {
    saves::SavedSessionDecodeContext result;
    result.decode_map_regions=true;result.decode_teams=true;
    result.decode_all_players=true;result.decode_supporting_classes=true;
    result.all_players=saves::AllPlayersContext{};
    result.permit_live_bnw_player_core_overlay=permit_overlay;
    return result;
}
runtime::LiveSessionState state_from_world(saves::SavedSessionSnapshot world) {
    auto random=runtime::restore_session_random(world.progression);
    return {std::move(world),{},{},{},{},{},{},{},{},{},
            std::move(random),{},{},{},{},{},{},{},{},{},{},{}};
}
const saves::CivisSaveSection &section(
    const saves::CivisSaveDocument &document,std::uint32_t id) {
    const saves::CivisSaveSection *found=nullptr;
    for(const auto &part:document.sections)
        if(part.id==id) {
            check(!found,"Duplicate BNW player-core section");found=&part;
        }
    check(found,"Missing BNW player-core section");return *found;
}
void selected(const std::filesystem::path &path) {
    const auto original=runtime::identify_file(path);
    const auto imported=saves::read_saved_session(path,context());
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,std::string(64,'a')};
    auto state=state_from_world(imported.snapshot);
    auto &score=field(state.world.all_players->players[0],
                      "m_iTotalLandScored").values.at(0);
    ++score;
    const auto expected=score;
    auto export_one=runtime::make_bnw_live_player_core_partial_export(
        state,imported,identity,original.sha256);
    check(!export_one.full_persistence &&
          export_one.uncovered_domains.size()>=15 &&
          export_one.document.sections.size()>=3,
          "BNW player-core export concealed incomplete world coverage");
    const auto &core=section(export_one.document,
        runtime::bnw_player_core_checkpoint_section_id);
    check(core.bytes.size()>100 && core.flags==0,
          "BNW player-core section missing from partial document");
    auto wrong_identity=identity;wrong_identity.payload_sha256[0]=
        wrong_identity.payload_sha256[0]=='0'?'1':'0';
    rejects([&]{(void)runtime::make_bnw_live_player_core_partial_export(
        state,imported,wrong_identity,original.sha256);});
    rejects([&]{(void)runtime::make_bnw_live_player_core_partial_export(
        state,imported,identity,"not-a-sha256");});
    auto overlapping=imported;
    overlapping.source.live_bnw_player_proximity=std::vector<std::uint8_t>{1};
    rejects([&]{(void)runtime::make_bnw_live_player_core_partial_export(
        state,overlapping,identity,original.sha256);});
    auto &player=state.world.all_players->players[0];
    const auto saved_id=player.saved_player_id;
    player.saved_player_id=42;
    rejects([&]{(void)runtime::make_bnw_live_player_core_partial_export(
        state,imported,identity,original.sha256);});
    player.saved_player_id=saved_id;
    const auto suffix=std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto first=std::filesystem::current_path()/
        ("bnw-live-player-core-export-"+suffix+"-1.CivisSave");
    const auto second=std::filesystem::current_path()/
        ("bnw-live-player-core-export-"+suffix+"-2.CivisSave");
    struct Cleanup {
        std::filesystem::path first,second;
        ~Cleanup() {
            std::error_code ignored;
            std::filesystem::remove(first,ignored);
            std::filesystem::remove(second,ignored);
        }
    } cleanup{first,second};
    saves::write_civis_save(first,export_one.document);
    export_one.document.sections.clear();
    state.world={};
    {
        const auto reopened=saves::read_saved_session(first,context(true));
        check(reopened.source.live_bnw_player_core &&
              reopened.source.bytes==imported.source.bytes &&
              reopened.source.source_sha256==original.sha256,
              "BNW partial save lost payload or source identity");
        auto restored=reopened.snapshot;
        runtime::restore_bnw_player_core_checkpoint(
            {runtime::bnw_player_core_checkpoint_section_id,0,
             *reopened.source.live_bnw_player_core},
            restored,reopened.snapshot,identity);
        check(field(restored.all_players->players[0],
                    "m_iTotalLandScored").values.at(0)==expected,
              "BNW PlayerCore value did not survive write/exit/load");
        auto resumed=state_from_world(std::move(restored));
        auto &score_two=field(resumed.world.all_players->players[0],
                              "m_iTotalLandScored").values.at(0);
        ++score_two;
        auto export_two=runtime::make_bnw_live_player_core_partial_export(
            resumed,reopened,identity,original.sha256);
        saves::write_civis_save(second,export_two.document);
    }
    {
        const auto reopened=saves::read_saved_session(second,context(true));
        const auto document=saves::read_civis_save(second);
        (void)section(document,runtime::bnw_player_core_checkpoint_section_id);
        auto restored=reopened.snapshot;
        runtime::restore_bnw_player_core_checkpoint(
            {runtime::bnw_player_core_checkpoint_section_id,0,
             *reopened.source.live_bnw_player_core},
            restored,reopened.snapshot,identity);
        check(field(restored.all_players->players[0],
                    "m_iTotalLandScored").values.at(0)==expected+1,
              "BNW second partial save failed to replace prior ID24");
    }
    check(runtime::identify_file(path).sha256==original.sha256,
          "BNW partial export altered the source Civ5Save");
    std::cout << "PASS BNW live PlayerCore partial export roundtrip\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi BNW save");
        selected(argv[1]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW live PlayerCore export: " << error.what() << '\n';
        return 1;
    }
}
