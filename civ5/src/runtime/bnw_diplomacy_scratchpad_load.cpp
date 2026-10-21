#include "civ5/runtime/bnw_diplomacy_scratchpad_load.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_game_clock_checkpoint.hpp"
#include <exception>
#include <variant>

namespace civ5::runtime {

std::optional<std::int8_t> BnwDiplomacyScratchpadLoad::value_at(
    std::int64_t player) const noexcept {
    if(!current_ || player<0 || player>=static_cast<std::int64_t>(values_.size()))
        return std::nullopt;
    return values_[static_cast<std::size_t>(player)];
}

bool BnwDiplomacyScratchpadLoad::matches_load_clock(
    const LiveSessionState &state) const noexcept {
    if(!current_ || state.setup.profile!=content::TechnologyProfile::bnw ||
       state.world.map_header.layout!=saves::GameStateLayout::expansion2_version1 ||
       state.world.game_state.layout!=saves::GameStateLayout::expansion2_version1 ||
       state.world.map_header.guid!=map_guid_ || state.setup.game_turn!=game_turn_ ||
       state.maintenance.elapsed_turns!=elapsed_turns_ ||
       !state.world.pregame ||
       state.world.pregame->layout!=saves::GameStateLayout::expansion2_version1 ||
       state.world.pregame->engine_guid!=engine_guid_ ||
       state.world.pregame->gamecore_guid!=gamecore_guid_ ||
       !std::holds_alternative<saves::Expansion2PrefixObservation>(
           state.world.game_state.prefix)) return false;
    const auto turn=state.world.pregame->archive.integers.find("game_turn");
    return turn!=state.world.pregame->archive.integers.end() &&
        turn->second==game_turn_ &&
        std::get<saves::Expansion2PrefixObservation>(
            state.world.game_state.prefix).scalars[1]==elapsed_turns_;
}

BnwDiplomacyScratchpadLoad restore_bnw_diplomacy_scratchpad_after_player_read(
    const saves::SavedSession &source,const LiveSessionState &state,
    const content::RulesDatabase &database,
    const SessionCheckpointIdentity &identity) {
    BnwDiplomacyScratchpadLoad result;
    if(identity.payload_sha256.size()!=64 ||
       identity.rules_sha256.size()!=64 ||
       identify_bytes(source.source.bytes).sha256!=identity.payload_sha256 ||
       identify_bytes(database.gameplay_fingerprint_data()).sha256!=
           identity.rules_sha256) {
        result.reason_="BNW diplomacy scratchpad source/rules identity is unavailable";
        return result;
    }
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    if(!source.snapshot.pregame || !state.world.pregame ||
       source.snapshot.game_state.layout!=bnw ||
       state.world.game_state.layout!=bnw ||
       !std::holds_alternative<saves::Expansion2PrefixObservation>(
           source.snapshot.game_state.prefix) ||
       !std::holds_alternative<saves::Expansion2PrefixObservation>(
           state.world.game_state.prefix)) {
        result.gate_=BnwDiplomacyScratchpadLoadGate::not_initial_load_boundary;
        result.reason_="BNW scratchpad requires a restored post-Read game clock";
        return result;
    }
    const auto &turns=source.snapshot.pregame->archive.integers;
    if(source.snapshot.map_header.layout!=bnw || state.world.map_header.layout!=bnw ||
       source.snapshot.pregame->layout!=bnw || state.world.pregame->layout!=bnw ||
       source.snapshot.map_header.guid!=state.world.map_header.guid ||
       source.snapshot.pregame->engine_guid!=state.world.pregame->engine_guid ||
       source.snapshot.pregame->gamecore_guid!=state.world.pregame->gamecore_guid) {
        result.reason_="BNW diplomacy scratchpad immutable world identity differs";
        return result;
    }
    const auto source_turn=turns.find("game_turn");
    const auto &owned_turns=state.world.pregame->archive.integers;
    const auto owned_turn=owned_turns.find("game_turn");
    const auto source_elapsed=std::get<saves::Expansion2PrefixObservation>(
        source.snapshot.game_state.prefix).scalars[1];
    const auto owned_elapsed=std::get<saves::Expansion2PrefixObservation>(
        state.world.game_state.prefix).scalars[1];
    if(source_turn==turns.end() ||
       owned_turn==owned_turns.end() ||
       state.setup.game_turn!=owned_turn->second ||
       state.maintenance.elapsed_turns!=owned_elapsed ||
       source_turn->second<0 || source_elapsed<0 ||
       owned_turn->second<source_turn->second || owned_elapsed<source_elapsed ||
       owned_turn->second-source_turn->second!=owned_elapsed-source_elapsed) {
        result.gate_=BnwDiplomacyScratchpadLoadGate::not_initial_load_boundary;
        result.reason_="BNW scratchpad clock owners or progression disagree";
        return result;
    }
    if(source.source.container==saves::SaveContainerKind::civ5) {
        if(owned_turn->second!=source_turn->second ||
           std::get<saves::Expansion2PrefixObservation>(
               source.snapshot.game_state.prefix).scalars!=
               std::get<saves::Expansion2PrefixObservation>(
                   state.world.game_state.prefix).scalars) {
            result.gate_=BnwDiplomacyScratchpadLoadGate::not_initial_load_boundary;
            result.reason_="BNW original post-Read clock boundary changed";
            return result;
        }
    } else if(source.source.container==saves::SaveContainerKind::civis) {
        // A Civis container retains the immutable source payload. Its clock
        // overlay must independently reconstruct the current player-Read
        // clock before this transient AI value can be reset to -1.
        if(source.source.live_bnw_game_clock) {
            auto restored=source.snapshot;
            BnwGameClockState clock;
            try {
                restore_bnw_game_clock_checkpoint(
                    {static_cast<std::uint32_t>(
                         saves::CivisSaveSectionId::live_bnw_game_clock),
                     0,*source.source.live_bnw_game_clock},
                    restored,clock,identity);
            } catch(const std::exception &) {
                result.gate_=BnwDiplomacyScratchpadLoadGate::not_initial_load_boundary;
                result.reason_="BNW scratchpad Civis clock section is invalid";
                return result;
            }
            const auto restored_turn=restored.pregame->archive.integers.find("game_turn");
            if(restored_turn==restored.pregame->archive.integers.end() ||
               restored_turn->second!=owned_turn->second ||
               std::get<saves::Expansion2PrefixObservation>(
                   restored.game_state.prefix).scalars[1]!=owned_elapsed) {
                result.gate_=BnwDiplomacyScratchpadLoadGate::not_initial_load_boundary;
                result.reason_="BNW scratchpad Civis clock differs from restored world";
                return result;
            }
        } else if(owned_turn->second!=source_turn->second ||
                  owned_elapsed!=source_elapsed) {
            result.gate_=BnwDiplomacyScratchpadLoadGate::not_initial_load_boundary;
            result.reason_="BNW scratchpad advanced Civis clock has no checkpoint";
            return result;
        }
    } else {
        result.gate_=BnwDiplomacyScratchpadLoadGate::not_initial_load_boundary;
        result.reason_="BNW scratchpad save container is unsupported";
        return result;
    }
    if(!source.snapshot.pregame || !source.snapshot.all_players ||
       !state.world.pregame || !state.world.all_players ||
       !state.world.teams ||
       source.snapshot.pregame->layout!=bnw ||
       state.world.pregame->layout!=bnw ||
       source.snapshot.all_players->layout!=bnw ||
       state.world.all_players->layout!=bnw ||
       source.snapshot.all_players->players.size()!=64 ||
       state.world.all_players->players.size()!=64 ||
       state.world.teams->dimensions.civ_teams!=63 ||
       state.world.teams->dimensions.major!=22 ||
       state.setup.profile!=content::TechnologyProfile::bnw ||
       state.setup.players.size()!=64) {
        result.gate_=BnwDiplomacyScratchpadLoadGate::player_roster_inconsistent;
        result.reason_="BNW diplomacy scratchpad player roster/profile is unavailable";
        return result;
    }
    for(std::size_t i=0;i<64;++i) {
        const auto &saved=source.snapshot.all_players->players[i];
        const auto &owned=state.world.all_players->players[i];
        if(saved.index!=i || owned.index!=i ||
           saved.saved_player_id!=static_cast<std::int64_t>(i) ||
           owned.saved_player_id!=static_cast<std::int64_t>(i) ||
           saved.diplomacy.layout!=bnw || owned.diplomacy.layout!=bnw ||
           !state.setup.players.contains(static_cast<std::int64_t>(i))) {
            result.gate_=BnwDiplomacyScratchpadLoadGate::player_roster_inconsistent;
            result.reason_="BNW diplomacy scratchpad player IDs or diplomacy profiles differ";
            return result;
        }
    }
    result.values_.fill(-1);
    result.identity_=identity;
    result.map_guid_=state.world.map_header.guid;
    result.engine_guid_=state.world.pregame->engine_guid;
    result.gamecore_guid_=state.world.pregame->gamecore_guid;
    result.game_turn_=owned_turn->second;
    result.elapsed_turns_=state.maintenance.elapsed_turns;
    result.current_=true;
    result.gate_=BnwDiplomacyScratchpadLoadGate::ready_after_player_read;
    result.reason_="BNW DiplomacyAI::Init/Reset supplies -1 after player Read";
    return result;
}

} // namespace civ5::runtime
