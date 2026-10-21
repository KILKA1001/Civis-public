#include "civ5/runtime/bnw_fresh_player_operations_uninit.hpp"
#include <stdexcept>

namespace {
using namespace civ5::runtime;
void check(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
BnwFreshFirstPlayerObjects first_player() {
    BnwFreshFirstPlayerObjects result;
    result.player_id = 0;
    result.selected_rules_sha256 = "selected-sha";
    result.constructors_complete = true;
    return result;
}
BnwFreshPlayerEntitiesDraft entity_predecessor() {
    BnwFreshPlayerEntitiesDraft result;
    result.selected_rules_sha256 = "selected-sha";
    result.detached_effects_complete = true;
    result.source_order = {{"m_cities.Uninit",false},
                           {"m_units.Uninit",false}};
    return result;
}
} // namespace

int main() {
    const auto objects = first_player();
    const auto entities = entity_predecessor();
    auto fresh = project_bnw_fresh_player_operations_uninit(
        objects,entities,"selected-sha",{});
    check(fresh.detached_effect_complete &&
          fresh.state.mapped_pointer_nonnull.empty() &&
          fresh.source_order ==
              std::vector<BnwFreshPlayerOperationsEvent>{
                  {BnwFreshPlayerOperationsEventKind::map_clear,0}} &&
          !fresh.archive_traversal_verified &&
          !fresh.initialized_by_gamecore,
          "fresh constructor map must execute source clear without readiness");

    BnwFreshPlayerOperationsState null_entries;
    null_entries.mapped_pointer_nonnull.emplace(9,false);
    null_entries.mapped_pointer_nonnull.emplace(-3,false);
    null_entries.mapped_pointer_nonnull.emplace(2,false);
    auto null_draft = project_bnw_fresh_player_operations_uninit(
        objects,entities,"selected-sha",null_entries);
    check(null_draft.detached_effect_complete &&
          null_draft.state.mapped_pointer_nonnull.empty() &&
          null_draft.source_order ==
              std::vector<BnwFreshPlayerOperationsEvent>{
                  {BnwFreshPlayerOperationsEventKind::delete_null_pointer,-3},
                  {BnwFreshPlayerOperationsEventKind::delete_null_pointer,2},
                  {BnwFreshPlayerOperationsEventKind::delete_null_pointer,9},
                  {BnwFreshPlayerOperationsEventKind::map_clear,0}} &&
          null_entries.mapped_pointer_nonnull.size() == 3,
          "null deletion and map iteration must preserve SDK key order");

    auto live = null_entries;
    live.mapped_pointer_nonnull[2] = true;
    const auto blocked = project_bnw_fresh_player_operations_uninit(
        objects,entities,"selected-sha",live);
    check(!blocked.detached_effect_complete &&
          blocked.blocker ==
              BnwFreshPlayerOperationsBlocker::live_operation_destructor_pending &&
          blocked.blocked_at == "delete(iter->second)" &&
          blocked.source_order.empty() &&
          blocked.state.mapped_pointer_nonnull.empty() &&
          live.mapped_pointer_nonnull.at(2),
          "live operation must fail before any earlier map entry is deleted");

    auto bad_predecessor = entities;
    bad_predecessor.source_order.pop_back();
    check(project_bnw_fresh_player_operations_uninit(
              objects,bad_predecessor,"selected-sha",{}).blocker ==
              BnwFreshPlayerOperationsBlocker::predecessor_or_rules_invalid &&
          project_bnw_fresh_player_operations_uninit(
              objects,entities,"wrong-sha",{}).blocker ==
              BnwFreshPlayerOperationsBlocker::predecessor_or_rules_invalid,
          "source order and selected rules identity are mandatory");
    null_draft.rollback();
    check(!null_draft.detached_effect_complete &&
          null_draft.source_order.empty() &&
          null_draft.selected_rules_sha256.empty() &&
          null_draft.state.mapped_pointer_nonnull.empty(),
          "rollback discards detached map and events");
}
