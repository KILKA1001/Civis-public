#pragma once
#include "civ5/runtime/bnw_end_turn_request.hpp"
#include <array>

namespace civ5::runtime {
struct BnwUpdateMovesSourceFile {std::string_view path,sha256;};
inline constexpr std::array<BnwUpdateMovesSourceFile,7> kBnwUpdateMovesSourceFiles{{
    {"CvGameCoreDLL_Expansion2/CvGame.cpp","1318f7522c1bb551f3a2a7b0452c953bbbcad4291d1fe80f4791613f8f9fae3c"},
    {"CvGameCoreDLL_Expansion2/CvPreGame.cpp","6fabb3a68f6eea348d0a232998ee6b4f81aa3f4a7b58aa0ae2f9ec32d9383946"},
    {"CvGameCoreDLL_Expansion2/CvPlayer.cpp","9635fe516e590e133c3907ee25c13128fb666e3e7817c967e247affb700a952d"},
    {"CvGameCoreDLL_Expansion2/CvUnit.cpp","646e6c26ffb5ca20b77320d1bb7d78af440072eec210a760c2c76275f5e80b82"},
    {"CvGameCoreDLL_Expansion2/CvUnitMission.cpp","ff19fb1100a54c32bff3605c298ff61d7b49fa281505a2117518012b2f3856e7"},
    {"CvGameCoreDLLUtil/include/ICvDLLUtility.h","beb1fe373c0bed051b7e2482a315120be16db39b7a1e470931988aa891e95239"},
    {"CvGameCoreDLLUtil/include/CvEnums.h","6781f34a6a6a9fac1cbaad3a3477afabf540669af9f860f57ad54b967c22da9b"}
}};
// A typed receipt of CvGame::Read's actual last-AI-turn reset. It is not an
// EXE barrier receipt, initial-unit-AI receipt, or permission to run missions.
class BnwUpdateMovesImportOwner {
  public:
    const SessionCheckpointIdentity &identity() const noexcept {return identity_;}
    const std::array<std::uint8_t,16> &map_guid() const noexcept {return map_guid_;}
    std::int64_t last_ai_civs_processed() const noexcept {return last_ai_civs_processed_;}
    std::int64_t initial_ai_processed(std::size_t player) const {return initial_ai_processed_.at(player);}
    bool process_player_auto_moves() const noexcept {return process_player_auto_moves_;}
    bool force_ending_turn() const noexcept {return force_ending_turn_;}
    void set_force_ending_turn(bool value) noexcept {force_ending_turn_=value;}
  private:
    friend struct BnwUiTransientCheckpointAccess;
    friend struct BnwUpdateMovesLocalOperations;
    friend BnwUpdateMovesImportOwner bind_bnw_update_moves_original_import(
        const saves::SavedSession &,const SessionCheckpointIdentity &);
    BnwUpdateMovesImportOwner(SessionCheckpointIdentity identity,std::array<std::uint8_t,16> guid)
        :identity_(std::move(identity)),map_guid_(guid) {initial_ai_processed_.fill(-1);}
    SessionCheckpointIdentity identity_;
    std::array<std::uint8_t,16> map_guid_{};
    std::int64_t last_ai_civs_processed_=-1; // CvGame.cpp:9449-9451, after Read
    std::array<std::int64_t,64> initial_ai_processed_; // CvPlayer::reset:966, not Read
    // Native standalone per-session equivalent of updateMoves function static.
    // Sequential SP always sets it true after a delivered all-AI receipt.
    bool process_player_auto_moves_=false;
    bool force_ending_turn_=false; // CvGame::Read -> reset:969, not serialized
};
BnwUpdateMovesImportOwner bind_bnw_update_moves_original_import(
    const saves::SavedSession &,const SessionCheckpointIdentity &);

struct BnwUpdateMovesUnitEvidence {
    std::int64_t id=-1,moves=0,activity=-1,automate=-1,unit_ai=-1;
    std::uint64_t mission_count=0;
    bool turn_processed=false,can_move=false,automated=false,needs_ai_update=false;
};
// Exact hasUnitsThatNeedAIUpdate predicate and saved mission/automation inputs.
// It does not infer IsBusy: mission timer and combat-city/city-combat links
// require their actual retained runtime owners. A queue is never inferred empty.
std::vector<BnwUpdateMovesUnitEvidence> observe_bnw_update_moves_units(
    const LiveSessionState &,const BnwUpdateMovesImportOwner &,std::int64_t player);

enum class BnwUpdateMovesPrefixBlocker {
    invalid_input,active_ai_branch,send_ai_civs_processed_owner_missing,all_ai_civs_processed_owner_missing
};
struct BnwUpdateMovesPrefixGate {
    BnwUpdateMovesPrefixBlocker reason=BnwUpdateMovesPrefixBlocker::invalid_input;
    std::string blocker;
    bool source_ai_scan_completed=false;
    std::vector<std::int64_t> source_players_to_process;
    std::int64_t last_ai_civs_processed=-1,attempted_turn=-1;
};
// Bounded native prefix of CvGame::updateMoves, not CvGame::update. Performs
// the source AI-first roster scan. On fresh original import the first next
// operation is SendAICivsProcessed, BEFORE writing the last-AI-turn field and
// BEFORE checking allAICivsProcessedThisTurn/selecting humans/ForceReset.
// No implementation of these pure-virtual EXE services exists in the SDK.
// This gate deliberately has no committed callback or success/draft channel:
// the standalone caller must implement a retained local engine delivery owner
// first; no original EXE/DLL execution is required.
// Inputs, processed-auto-moves, cache controls and the active turn are unchanged.
BnwUpdateMovesPrefixGate gate_bnw_update_moves_original_draft(
    const LiveSessionState &,const BnwGameActivationOwner &,
    const BnwEndTurnInputBinding &,const BnwUpdateMovesImportOwner &,
    const content::RulesDatabase &,std::int64_t player,
    std::string_view expected_input_revision);
} // namespace civ5::runtime
