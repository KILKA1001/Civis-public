#pragma once

#include "civ5/runtime/bnw_team_tech_suffix_callbacks.hpp"
#include "civ5/saves/civis_save.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace civ5::runtime {

// Private reserved ID 44. IDs 41-43 belong to unit jump checkpoints. The
// common loader must reject 44 until the world/Lua/escrow sections can be
// committed and restored atomically; a receipt-only file is not playable.
inline constexpr std::uint32_t bnw_team_tech_suffix_receipt_section_id=44;

struct BnwTeamTechSuffixCheckpointBinding {
    SessionCheckpointIdentity source;
    std::uint64_t revision=0;
    // Digests of the *same* committed world, gameplay Lua VM and external
    // callback escrow. The caller must derive these from the actual checkpoint
    // sections; placeholder or in-memory-only values cannot authorize reload.
    std::string world_sha256, lua_vm_sha256, external_escrow_sha256;
};

// Derives the three mutable digests from actual sections of one document.
// The world section IDs must be sorted and complete for the revision; the
// common owner decides that set and the Lua/escrow section IDs. A section
// missing from this same document is a hard failure.
BnwTeamTechSuffixCheckpointBinding bind_bnw_team_tech_suffix_document(
    const saves::CivisSaveDocument &document,
    const SessionCheckpointIdentity &source,
    std::uint64_t revision,
    const std::vector<std::uint32_t> &world_section_ids,
    std::uint32_t lua_vm_section_id,
    std::uint32_t external_escrow_section_id);

// Only an already attempted callback is representable. Successful and
// uncertain attempts both prohibit replay after restart. A confirmed engine
// callback does not mean the complete technology grant has been published.
saves::CivisSaveSection make_bnw_team_tech_suffix_receipt_checkpoint(
    const BnwTeamTechSuffixCallbackMarker &marker,
    const BnwTeamTechSuffixCheckpointBinding &binding);

// Decode and compare every identity/revision/digest before touching state.
// The restored marker makes quote_bnw_team_tech_suffix_callbacks reject a
// retry. The owner must load the matching world, VM and escrow atomically.
void restore_bnw_team_tech_suffix_receipt_checkpoint(
    const saves::CivisSaveSection &section,
    LiveSessionState &state,
    const BnwTeamTechSuffixCheckpointBinding &expected);

} // namespace civ5::runtime
