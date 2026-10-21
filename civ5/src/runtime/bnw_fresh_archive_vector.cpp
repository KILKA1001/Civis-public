#include "civ5/runtime/bnw_fresh_archive_vector.hpp"
#include <utility>

namespace civ5::runtime {

void BnwFreshArchiveVectorProjection::rollback() noexcept {
    state.values.clear();
    state.pending_commands = 0;
    state.archive_touched = false;
    source_effects.clear();
    selected_rules_sha256.clear();
    member_effect_complete = false;
    archive_traversal_verified = false;
    initialized_by_gamecore = false;
}

BnwFreshArchiveVectorProjection project_bnw_fresh_resource_vector_reset(
    const BnwFreshFirstPlayerObjects &objects,
    std::string_view selected_rules_sha256,
    BnwFreshArchiveVectorState initial) {
    BnwFreshArchiveVectorProjection result;
    if (!objects.constructors_complete || objects.initialized_by_gamecore ||
        objects.player_id != 0 || selected_rules_sha256.empty() ||
        objects.selected_rules_sha256 != selected_rules_sha256 ||
        !objects.has_blocker(BnwFreshFirstPlayerObjectBlockerCode::
                              reset_sync_archive_and_uninit_pending))
        return result;
    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.state = std::move(initial);
    result.source_effects.push_back({
        BnwFreshArchiveVectorStep::archive_traversal_pending,
        "m_syncArchive"});
    // FirePlace FAutoVariable<std::vector<T>>::reset clears m_commands and
    // m_value. It does not call touch(). The aggregate archive reset call
    // remains unknown, so this is a member projection only.
    result.state.pending_commands = 0;
    result.state.values.clear();
    result.source_effects.push_back({
        BnwFreshArchiveVectorStep::member_reset_projection,
        "m_paiNumResourceUsed"});
    // The first CvPlayer::uninit statement is this vector's clear().
    // FAutoVector::clear touches its owner and appends one CLEAR command.
    result.state.archive_touched = true;
    result.state.pending_commands = 1;
    result.source_effects.push_back({
        BnwFreshArchiveVectorStep::uninit_clear_projection,
        "m_paiNumResourceUsed"});
    result.member_effect_complete = true;
    return result;
}
} // namespace civ5::runtime
