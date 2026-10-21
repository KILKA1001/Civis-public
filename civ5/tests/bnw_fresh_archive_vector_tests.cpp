#include "civ5/runtime/bnw_fresh_archive_vector.hpp"
#include <stdexcept>

namespace {
void check(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
void test_member_reset_and_clear_projection() {
    using namespace civ5::runtime;
    BnwFreshFirstPlayerObjects objects;
    objects.player_id = 0;
    objects.selected_rules_sha256 = "selected-sha";
    objects.constructors_complete = true;
    objects.blockers.push_back({
        BnwFreshFirstPlayerObjectBlockerCode::reset_sync_archive_and_uninit_pending,
        "fixture"});
    BnwFreshArchiveVectorState initial;
    initial.values = {4,8,12};
    initial.pending_commands = 3;
    const auto before = initial.values;
    auto projected = project_bnw_fresh_resource_vector_reset(
        objects,"selected-sha",initial);
    check(projected.member_effect_complete &&
              !projected.archive_traversal_verified &&
              !projected.initialized_by_gamecore &&
              projected.state.values.empty() &&
              projected.state.pending_commands == 1 &&
              projected.state.archive_touched &&
              projected.source_effects.size() == 3 &&
              projected.source_effects[0] == BnwFreshArchiveVectorEvent{
                  BnwFreshArchiveVectorStep::archive_traversal_pending,
                  "m_syncArchive"} &&
              projected.source_effects[1] == BnwFreshArchiveVectorEvent{
                  BnwFreshArchiveVectorStep::member_reset_projection,
                  "m_paiNumResourceUsed"} &&
              projected.source_effects[2] == BnwFreshArchiveVectorEvent{
                  BnwFreshArchiveVectorStep::uninit_clear_projection,
                  "m_paiNumResourceUsed"} && before.size() == 3,
          "FAutoVector reset and clear mutate only owned vector/commands");
    const auto rejected = project_bnw_fresh_resource_vector_reset(
        objects,"other-sha",initial);
    check(!rejected.member_effect_complete &&
              rejected.source_effects.empty() &&
              rejected.state.values.empty(),
          "Mismatched provenance cannot stage an archive member effect");
    projected.rollback();
    check(!projected.member_effect_complete &&
              projected.source_effects.empty() &&
              projected.selected_rules_sha256.empty() &&
              initial.values == before,
          "Discarding projection preserves original vector state");
}
} // namespace

int main() { test_member_reset_and_clear_projection(); }
