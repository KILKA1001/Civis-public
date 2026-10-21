#pragma once
#include "civ5/runtime/bnw_unit_order_owners.hpp"
#include "civ5/runtime/simulation_random.hpp"
#include "civ5/saves/civis_save.hpp"
#include <memory>
namespace civ5::runtime {
class BnwNativeUnitAllocatorCore;
enum class BnwUnitMissionContinuationPhase:std::uint32_t {
 cache_cleared_before_evaluating_flags=1,evaluating_flags_stored_before_head=2,
 empty_head_awake_exit_flags_stored_before_combat_warned=3,
 empty_head_awake_native_return_completed=4
};
// Version4 additionally carries the authenticated Game completion journal.
// Phase4 bytes alone remain quarantined: only the same Core and genuine Game
// pristine OriginalPayload Read installer can restore earned return history.
// No queue body or changed activity is manufactured.
// Section43 is a PRIVATE reserved proposal until Root adopts the actual
// container and fresh-session restore wiring. Bytes do not grant live effects.
inline constexpr std::uint32_t bnw_unit_mission_continuation_section_id=43;
// Read-only canonical byte validation only. Shape validates its own embedded
// identity; binding additionally compares the supplied payload/rules identity.
// Neither function restores state, checks live readiness, or issues a grant.
void validate_bnw_unit_mission_continuation_checkpoint_shape(const saves::CivisSaveSection&);
void validate_bnw_unit_mission_continuation_checkpoint_binding(const saves::CivisSaveSection&,const SessionCheckpointIdentity&);
class BnwUnitMissionContinuationCheckpoint final {
 public:
  const saves::CivisSaveSection&section()const noexcept{return section_;}
  BnwUnitMissionContinuationCheckpoint(const BnwUnitMissionContinuationCheckpoint&)=delete;
  BnwUnitMissionContinuationCheckpoint&operator=(const BnwUnitMissionContinuationCheckpoint&)=delete;
 private:
  friend class BnwNativeUnitAllocatorCore;
  friend class BnwUnitCombatWarnedRestorePlan;
  friend void validate_bnw_unit_mission_continuation_checkpoint_shape(const saves::CivisSaveSection&);
  friend void validate_bnw_unit_mission_continuation_checkpoint_binding(const saves::CivisSaveSection&,const SessionCheckpointIdentity&);
  struct Record {
   SessionCheckpointIdentity identity;
   std::array<std::uint8_t,16>map_guid{};
   std::uint64_t width=0,height=0;bool wrap_x=false,wrap_y=false;
   std::int32_t actor=-1,full_id=-1,slot=-1,game_turn=-1,steps=0,eta=-1;
   std::uint64_t core_begin=0,core_end=0,tail_begin=0,tail_end=0;
   std::string before_world_revision,current_world_revision;
   SimulationRandomState map_rng,game_rng;
   BnwUnitMissionContinuationPhase phase=BnwUnitMissionContinuationPhase::cache_cleared_before_evaluating_flags;
   std::int32_t flags_loaded=0,flags_before=0,flags_current=0;
   std::uint64_t native_archive_index=0,changes=0;bool dirty=false;
   BnwUnitRetainedPath before_path,current_path;
   bool combat_warned_before=false,combat_warned_after=false;
   std::uint64_t combat_warned_before_ordinal=0,combat_warned_ordinal=0;
  };
  explicit BnwUnitMissionContinuationCheckpoint(const Record&);
  static saves::CivisSaveSection encode(const Record&);
  static Record decode(const saves::CivisSaveSection&,const SessionCheckpointIdentity&);
  saves::CivisSaveSection section_;
};
} // namespace civ5::runtime
