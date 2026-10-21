#include "civ5/runtime/bnw_live_player_state_export.hpp"
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
    bool failed=false;
    try {f();} catch(const std::exception &) {failed=true;}
    check(failed,"Invalid BNW player-state export was accepted");
}
saves::SavedSessionDecodeContext context(bool overlays=false) {
    saves::SavedSessionDecodeContext result;
    result.decode_map_regions=true;result.decode_teams=true;
    result.decode_all_players=true;result.decode_supporting_classes=true;
    result.all_players=saves::AllPlayersContext{};
    result.permit_live_bnw_player_strategy_overlay=overlays;
    result.permit_live_bnw_player_core_overlay=overlays;
    return result;
}
runtime::LiveSessionState state_from_world(saves::SavedSessionSnapshot world) {
    auto random=runtime::restore_session_random(world.progression);
    return {std::move(world),{},{},{},{},{},{},{},{},{},
            std::move(random),{},{},{},{},{},{},{},{},{},{},{}};
}
std::int64_t &core(saves::SavedSessionSnapshot &world,std::string_view wanted) {
    for(auto &part:world.all_players->players.at(0).core.fields)
        if(part.name && std::string_view(part.name)==wanted)
            return part.values.at(0);
    throw std::runtime_error("Selected BNW player core field missing");
}
std::int64_t &strategy(saves::SavedSessionSnapshot &world,
                       std::string_view block,std::string_view wanted) {
    for(auto &part:world.all_players->players.at(0).strategy.blocks)
        if(part.name && std::string_view(part.name)==block)
            for(auto &field:part.fields)
                if(field.name && std::string_view(field.name)==wanted)
                    return field.value;
    throw std::runtime_error("Selected BNW player strategy field missing");
}
const saves::CivisSaveSection &section(
    const saves::CivisSaveDocument &document,std::uint32_t id) {
    const saves::CivisSaveSection *result=nullptr;
    for(const auto &part:document.sections)
        if(part.id==id) {
            check(!result,"Duplicate BNW player section");result=&part;
        }
    check(result,"Missing BNW player section");return *result;
}
void restore_pair(saves::SavedSessionSnapshot &world,
                  const saves::SavedSession &source,
                  const runtime::SessionCheckpointIdentity &identity) {
    check(source.source.live_bnw_player_core &&
          source.source.live_bnw_player_strategy,
          "BNW composed player sections missing after import");
    // Matches LiveSession::restore_bnw: ID24 before ID23, both against the
    // immutable source. Build locally so a failed second restore is not
    // published to an owned session.
    auto detached=source.snapshot;
    runtime::restore_bnw_player_core_checkpoint(
        {runtime::bnw_player_core_checkpoint_section_id,0,
         *source.source.live_bnw_player_core},
        detached,source.snapshot,identity);
    runtime::restore_bnw_player_strategy_checkpoint(
        {runtime::bnw_player_strategy_checkpoint_section_id,0,
         *source.source.live_bnw_player_strategy},
        detached,source.snapshot,identity);
    world=std::move(detached);
}
void selected(const std::filesystem::path &path) {
    const auto source_hash=runtime::identify_file(path).sha256;
    const auto imported=saves::read_saved_session(path,context());
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,std::string(64,'a')};
    auto state=state_from_world(imported.snapshot);
    auto &score=core(state.world,"m_iTotalLandScored");
    auto &dirty=strategy(state.world,"economic_ai","exploration_plots_dirty");
    const auto first_turns=strategy(state.world,"grand_strategy_ai",
                                    "turns_since_active_set");
    ++score;dirty=1-dirty;
    const auto first_score=score,first_dirty=dirty;
    auto first_export=runtime::make_bnw_live_player_state_partial_export(
        state,imported,identity,source_hash);
    check(!first_export.full_persistence &&
          first_export.uncovered_domains.size()>=15,
          "BNW composed export concealed incomplete persistence");
    check(section(first_export.document,
                  runtime::bnw_player_core_checkpoint_section_id).flags==0 &&
          section(first_export.document,
                  runtime::bnw_player_strategy_checkpoint_section_id).flags==0,
          "BNW composed export lacks ID23 or ID24");
    {
        auto overlap=imported;
        overlap.source.live_bnw_exploration_dirty=std::vector<std::uint8_t>{1};
        rejects([&]{(void)runtime::make_bnw_live_player_state_partial_export(
            state,overlap,identity,source_hash);});
        overlap=imported;
        overlap.source.live_bnw_player_proximity=std::vector<std::uint8_t>{1};
        rejects([&]{(void)runtime::make_bnw_live_player_state_partial_export(
            state,overlap,identity,source_hash);});
    }
    {
        auto changed=identity;changed.rules_sha256[0]='b';
        // The rules hash is caller-supplied for this diagnostic export; a
        // different hash is rejected on restore against the active rules.
        const auto foreign=runtime::make_bnw_live_player_state_partial_export(
            state,imported,changed,source_hash);
        rejects([&]{auto detached=imported.snapshot;
            runtime::restore_bnw_player_strategy_checkpoint(
                section(foreign.document,
                        runtime::bnw_player_strategy_checkpoint_section_id),
                detached,imported.snapshot,identity);});
    }
    {
        const auto old=core(state.world,"m_bAlive");
        core(state.world,"m_bAlive")=1-old;
        rejects([&]{(void)runtime::make_bnw_live_player_state_partial_export(
            state,imported,identity,source_hash);});
        core(state.world,"m_bAlive")=old;
        auto &blocks=state.world.all_players->players.at(0).strategy.blocks;
        auto saved=std::move(blocks.back());blocks.pop_back();
        rejects([&]{(void)runtime::make_bnw_live_player_state_partial_export(
            state,imported,identity,source_hash);});
        blocks.push_back(std::move(saved));
    }
    const auto suffix=std::to_string(
        std::chrono::steady_clock::now().time_since_epoch().count());
    const auto first=std::filesystem::current_path()/
        ("bnw-player-state-"+suffix+"-1.CivisSave");
    const auto second=std::filesystem::current_path()/
        ("bnw-player-state-"+suffix+"-2.CivisSave");
    struct Cleanup {std::filesystem::path first,second;~Cleanup(){
        std::error_code error;
        std::filesystem::remove(first,error);
        std::filesystem::remove(second,error);
    }} cleanup{first,second};
    saves::write_civis_save(first,first_export.document);
    first_export.document.sections.clear();state.world={};
    {
        const auto reopened=saves::read_saved_session(first,context(true));
        check(reopened.source.source_sha256==source_hash &&
              reopened.source.bytes==imported.source.bytes,
              "BNW composed save lost source identity or payload");
        saves::SavedSessionSnapshot restored;
        restore_pair(restored,reopened,identity);
        check(core(restored,"m_iTotalLandScored")==first_score &&
              strategy(restored,"economic_ai","exploration_plots_dirty")==
                  first_dirty,
              "BNW ID23/ID24 values did not survive write/exit/load");
        auto resumed=state_from_world(std::move(restored));
        ++core(resumed.world,"m_iTotalLandScored");
        ++strategy(resumed.world,"grand_strategy_ai","turns_since_active_set");
        auto second_export=runtime::make_bnw_live_player_state_partial_export(
            resumed,reopened,identity,source_hash);
        saves::write_civis_save(second,second_export.document);
    }
    {
        const auto reopened=saves::read_saved_session(second,context(true));
        const auto document=saves::read_civis_save(second);
        (void)section(document,runtime::bnw_player_core_checkpoint_section_id);
        (void)section(document,runtime::bnw_player_strategy_checkpoint_section_id);
        saves::SavedSessionSnapshot restored;
        restore_pair(restored,reopened,identity);
        check(core(restored,"m_iTotalLandScored")==first_score+1 &&
              strategy(restored,"economic_ai","exploration_plots_dirty")==
                  first_dirty &&
              strategy(restored,"grand_strategy_ai","turns_since_active_set")==
                  first_turns+1,
              "BNW re-save did not replace both player sections");
    }
    check(runtime::identify_file(path).sha256==source_hash,
          "BNW composed export altered original Civ5Save");
    std::cout << "PASS BNW composed ID23/ID24 player-state roundtrip\n";
}
} // namespace
int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi BNW save");
        selected(argv[1]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW player-state export: " << error.what() << '\n';
        return 1;
    }
}
