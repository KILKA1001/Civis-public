#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/saves/civis_save.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <cstdint>
#include <map>

namespace civ5::runtime {

// Proposed common ID. Expansion2 CvPlayer::Read/reset creates this transient
// at zero; the original Civ5Save player core does not serialize it.
inline constexpr std::uint32_t bnw_culture_bomb_timer_checkpoint_section_id=32;
using BnwCultureBombTimers=std::map<std::int64_t,std::int32_t>;

saves::CivisSaveSection make_bnw_culture_bomb_timer_checkpoint(
    const BnwCultureBombTimers &timers,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

// The destination is untouched on all validation failures. Call only after
// the selected source's CvPlayer::Read reset hook created the zero baseline.
void restore_bnw_culture_bomb_timer_checkpoint(
    const saves::CivisSaveSection &section,
    BnwCultureBombTimers &timers,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime
