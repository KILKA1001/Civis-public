#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include <array>
#include <cstdint>
#include <optional>
#include <string>

namespace civ5::content { class RulesDatabase; }
namespace civ5::saves { struct SavedSession; }
namespace civ5::runtime {
struct LiveSessionState;

enum class BnwDiplomacyScratchpadLoadGate : std::uint8_t {
    source_identity_inconsistent,
    not_initial_load_boundary,
    player_roster_inconsistent,
    ready_after_player_read,
};

class BnwDiplomacyScratchpadLoad {
  public:
    BnwDiplomacyScratchpadLoadGate gate() const noexcept {return gate_;}
    bool current() const noexcept {return current_;}
    std::optional<std::int8_t> value_at(std::int64_t player) const noexcept;
    const std::string &reason() const noexcept {return reason_;}
    const SessionCheckpointIdentity &identity() const noexcept {return identity_;}
    // The retained map/engine/gamecore identity and clock must all match.
    // Clock equality is necessary, but an AI update within the same turn
    // still requires invalidate() before its first side effect.
    bool matches_load_clock(const LiveSessionState &state) const noexcept;
    void invalidate() noexcept {current_=false;}
    // Publication still requires the rest of first-contact GameCore and Lua.
    bool can_publish_contact() const noexcept {return false;}
  private:
    friend BnwDiplomacyScratchpadLoad restore_bnw_diplomacy_scratchpad_after_player_read(
        const saves::SavedSession &,const LiveSessionState &,
        const content::RulesDatabase &,
        const SessionCheckpointIdentity &);
    BnwDiplomacyScratchpadLoadGate gate_=
        BnwDiplomacyScratchpadLoadGate::source_identity_inconsistent;
    bool current_=false;
    std::array<std::int8_t,63> values_{};
    SessionCheckpointIdentity identity_;
    std::array<std::uint8_t,16> map_guid_{},engine_guid_{},gamecore_guid_{};
    std::int64_t game_turn_=-1,elapsed_turns_=-1;
    std::string reason_;
};

// Expansion2 CvPlayer.cpp:21729 calls reset before reading each player;
// reset at 1121 calls CvDiplomacyAI::Init, which calls Reset at
// CvDiplomacyAI.cpp:582. Reset sets m_paeApproachScratchPad[0..62]=-1 at 935.
// DiplomacyAI::Read starts at 1034 and never reads that transient array;
// only the per-turn DoUpdateMajorCivApproaches at 2625/2631 changes it.
// The result describes an original Civ5Save or a verified CivisSave clock
// checkpoint at the instant after native player Read, before any AI turn.
// The future owner must invalidate or update it when
// CvDiplomacyAI::DoTurn starts (before DoUpdateMajorCivApproaches is called
// near line 1970), and on a direct DoUpdateMajorCivApproaches/Reset/Init call.
BnwDiplomacyScratchpadLoad restore_bnw_diplomacy_scratchpad_after_player_read(
    const saves::SavedSession &source,const LiveSessionState &state,
    const content::RulesDatabase &database,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
