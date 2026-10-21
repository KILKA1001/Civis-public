#include "civ5/runtime/bnw_live_player_state_export.hpp"
#include "civ5/runtime/bnw_military_request_checkpoint.hpp"
#include <algorithm>
#include <stdexcept>

namespace civ5::runtime {

BnwPlayerStatePartialExport make_bnw_live_player_state_partial_export(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity,
    std::string_view original_source_sha256) {
    if(source.source.live_bnw_exploration_dirty)
        throw std::runtime_error(
            "BNW ID22 exploration dirty overlaps full ID23 player strategy");
    // ID24 and ID23 codecs validate the same source/owned roster and wire
    // structures before the composed document is returned. No state is written.
    auto core=make_bnw_live_player_core_partial_export(
        state,source,identity,original_source_sha256);
    const auto strategy=make_bnw_player_strategy_checkpoint(
        state.world,source.snapshot,identity,true);
    auto &parts=core.document.sections;
    parts.erase(std::remove_if(parts.begin(),parts.end(),[](const auto &part) {
        return part.id==bnw_player_strategy_checkpoint_section_id;
    }),parts.end());
    parts.push_back(strategy);
    if(source.source.live_bnw_military_requests)
        require_bnw_military_request_checkpoint_history_retained(
            {bnw_military_request_checkpoint_section_id,0,*source.source.live_bnw_military_requests},
            state,source.snapshot,identity);
    parts.erase(std::remove_if(parts.begin(),parts.end(),[](const auto&part){
        return part.id==bnw_military_request_checkpoint_section_id;
    }),parts.end());
    if(!state.military_request_counters.empty())
        parts.push_back(make_bnw_military_request_checkpoint(state,source.snapshot,identity));

    BnwPlayerStatePartialExport result;
    result.document=std::move(core.document);
    result.uncovered_domains=std::move(core.uncovered_domains);
    const auto old=std::string_view(
        "player strategies, economy, religion, policies and AI state");
    const auto at=std::find(result.uncovered_domains.begin(),
                            result.uncovered_domains.end(),old);
    if(at==result.uncovered_domains.end())
        throw std::runtime_error("BNW partial export coverage contract changed");
    *at="player non-strategy economy, religion, policy effects and AI state";
    result.uncovered_domains.push_back(
        "variable strategy collections, live AI caches and pending strategy effects");
    return result;
}

} // namespace civ5::runtime
