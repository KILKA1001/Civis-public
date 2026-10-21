#pragma once
#include "civ5/runtime/bnw_game_activation_execution.hpp"

namespace civ5::runtime {
class BnwEndTurnInputBinding {
  public:
    const SessionCheckpointIdentity &identity() const noexcept {return identity_;}
    const std::array<std::uint8_t,16> &map_guid() const noexcept {return map_guid_;}
  private:
    friend BnwEndTurnInputBinding bind_bnw_end_turn_import(
        const saves::SavedSession &,BnwGameActivationOwner &);
    BnwEndTurnInputBinding(SessionCheckpointIdentity identity,
        std::array<std::uint8_t,16> map_guid):identity_(std::move(identity)),map_guid_(map_guid) {}
    SessionCheckpointIdentity identity_;
    std::array<std::uint8_t,16> map_guid_{};
};
// CvPlayer::reset sets the unsaved processed-auto-moves flag to false before
// Read. Seed the SAME activation owner, never a competing copy of this flag.
// A Civis continuation needs a durable transient owner and cannot use this.
BnwEndTurnInputBinding bind_bnw_end_turn_import(
    const saves::SavedSession &,BnwGameActivationOwner &);

struct BnwEndTurnRequest {
    std::int64_t player=-1,expected_turn=-1;
    bool end_turn=true;
    std::string expected_input_revision;
};
struct BnwEndTurnRequestDraft {
    BnwEndTurnRequestDraft(const LiveSessionState &source,const BnwGameActivationOwner &prior)
        :state(source),owner(prior) {}
    LiveSessionState state;
    BnwGameActivationOwner owner;
    bool flags_changed=false,requires_auto_move_processing=false;
    std::string input_revision;
};
struct BnwEndTurnRequestExecution {
    std::unique_ptr<BnwEndTurnRequestDraft> draft;
    std::string blocker;
};
std::string bnw_end_turn_input_revision(const LiveSessionState &,
    const BnwGameActivationOwner &,const BnwEndTurnInputBinding &,
    const content::RulesDatabase &,std::int64_t player);
// Complete source setEndTurn/setAutoMoves branch for the active human in
// sequential single-player. This only changes request flags on a private
// draft. It never processes missions, deactivates a player, advances a turn,
// publishes engine events, or grants continuation/persistence readiness.
BnwEndTurnRequestExecution execute_bnw_end_turn_request_draft(
    const LiveSessionState &,const BnwGameActivationOwner &,
    const BnwEndTurnInputBinding &,const content::RulesDatabase &,
    const BnwEndTurnRequest &);
}
