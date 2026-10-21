#include "civ5/runtime/bnw_fresh_archive_values.hpp"
#include <stdexcept>

namespace {
void check(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
}

int main() {
    using namespace civ5::runtime;
    BnwFreshFirstPlayerObjects objects;
    objects.player_id = 0;
    objects.selected_rules_sha256 = "selected-sha";
    objects.constructors_complete = true;
    objects.blockers.push_back({
        BnwFreshFirstPlayerObjectBlockerCode::reset_sync_archive_and_uninit_pending,
        "fixture"});
    BnwFreshArchiveObservableValues initial;
    initial.starting_x = 27;
    initial.alive = true;
    initial.player_id = 8;
    initial.city_yield_change = {1,2,3};
    initial.resource_used = {4,5};
    initial.resource_pending_commands = 3;

    auto draft = project_bnw_fresh_archive_observable_reset(
        objects,"selected-sha",initial);
    check(draft.member_defaults_complete &&
          !draft.archive_traversal_verified && !draft.uninit_complete &&
          !draft.initialized_by_gamecore &&
          draft.selected_rules_sha256 == "selected-sha" &&
          draft.registered_members == std::vector<std::string_view>{
              "m_iStartingX", "m_bAlive", "m_eID",
              "m_aiCityYieldChange", "m_paiNumResourceUsed"} &&
          draft.values == BnwFreshArchiveObservableValues{} &&
          initial.starting_x == 27 && initial.alive && initial.player_id == 8 &&
          initial.city_yield_change.size() == 3 && initial.resource_used.size() == 2,
          "registered archive defaults must change only detached projection");

    auto rejected = project_bnw_fresh_archive_observable_reset(
        objects,"other-sha",initial);
    check(!rejected.member_defaults_complete && rejected.registered_members.empty(),
          "mismatched selected identity must not stage archive values");
    objects.initialized_by_gamecore = true;
    rejected = project_bnw_fresh_archive_observable_reset(
        objects,"selected-sha",initial);
    check(!rejected.member_defaults_complete && rejected.registered_members.empty(),
          "already initialized player must not be reset by diagnostic projection");

    draft.rollback();
    check(!draft.member_defaults_complete && draft.registered_members.empty() &&
          draft.selected_rules_sha256.empty() &&
          initial.resource_pending_commands == 3,
          "rollback must discard detached archive effects only");
}
