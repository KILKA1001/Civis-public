#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/saves/session_snapshot.hpp"
namespace civ5::runtime {
struct LiveSessionState;
inline constexpr std::uint32_t bnw_military_request_checkpoint_section_id=44;
// Version1 persists ONLY the earned sparse ResetCounters prefix. Absent
// players remain unknown. Nonzero requests need a later genuine producer codec.
saves::CivisSaveSection make_bnw_military_request_checkpoint(
    const LiveSessionState&,const saves::SavedSessionSnapshot&,
    const SessionCheckpointIdentity&);
void restore_bnw_military_request_checkpoint(const saves::CivisSaveSection&,
    LiveSessionState&,const saves::SavedSessionSnapshot&,
    const SessionCheckpointIdentity&);
void validate_bnw_military_request_checkpoint_shape(const saves::CivisSaveSection&);
// Re-save retains every previously earned actor. The historical ID23 hash
// grants no current world; a new capture validates the actual current ID23.
void require_bnw_military_request_checkpoint_history_retained(
    const saves::CivisSaveSection&,const LiveSessionState&,
    const saves::SavedSessionSnapshot&,const SessionCheckpointIdentity&);
} // namespace civ5::runtime
