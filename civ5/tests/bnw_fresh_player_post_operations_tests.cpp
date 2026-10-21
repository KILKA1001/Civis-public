#include "civ5/runtime/bnw_fresh_player_post_operations.hpp"
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
BnwFreshPlayerOperationsDraft operation_predecessor() {
    BnwFreshPlayerOperationsDraft result;
    result.selected_rules_sha256 = "selected-sha";
    result.detached_effect_complete = true;
    result.source_order = {{BnwFreshPlayerOperationsEventKind::map_clear,0}};
    return result;
}
} // namespace

int main() {
    const auto objects = first_player();
    const auto predecessor = operation_predecessor();
    BnwFreshPlayerPostOperationsState populated;
    populated.plot_indices = {9,3,9};
    populated.ever_conquered_by_words = {0xffffffffu,2u,7u,1u};
    auto draft = project_bnw_fresh_player_post_operations(
        objects,predecessor,"selected-sha",populated);
    check(draft.detached_effect_complete &&
          draft.state.plot_indices.empty() &&
          draft.state.ever_conquered_by_words ==
              std::array<std::uint32_t,4>{0,0,0,0} &&
          draft.source_order ==
              std::vector<BnwFreshPlayerPostOperationsEvent>{
                  {BnwFreshPlayerPostOperationsEventKind::plots_size_zero,
                   "m_aiPlots.clear"},
                  {BnwFreshPlayerPostOperationsEventKind::conquered_by_words_zero,
                   "m_bfEverConqueredBy.ClearAll"}} &&
          populated.plot_indices.size() == 3 &&
          populated.ever_conquered_by_words[0] == 0xffffffffu &&
          !draft.archive_clear_delta_verified &&
          !draft.archive_traversal_verified &&
          !draft.initialized_by_gamecore,
          "plot vector and four DWORDs must clear in source order on detached state");
    const auto empty = project_bnw_fresh_player_post_operations(
        objects,predecessor,"selected-sha",{});
    check(empty.detached_effect_complete && empty.source_order.size() == 2,
          "fresh empty state still invokes both SDK calls");

    auto malformed = populated;
    malformed.plot_indices.resize(
        BnwFreshPlayerPostOperationsState::kPlotsInitialCapacity + 1);
    const auto oversized = project_bnw_fresh_player_post_operations(
        objects,predecessor,"selected-sha",malformed);
    check(!oversized.detached_effect_complete &&
          oversized.blocker ==
              BnwFreshPlayerPostOperationsBlocker::plot_vector_capacity_unverified &&
          oversized.source_order.empty() &&
          malformed.plot_indices.size() ==
              BnwFreshPlayerPostOperationsState::kPlotsInitialCapacity + 1,
          "unverified vector capacity must fail before clearing either field");
    auto incomplete = predecessor;
    incomplete.source_order.clear();
    check(project_bnw_fresh_player_post_operations(
              objects,incomplete,"selected-sha",{}).blocker ==
              BnwFreshPlayerPostOperationsBlocker::predecessor_or_rules_invalid &&
          project_bnw_fresh_player_post_operations(
              objects,predecessor,"wrong-sha",{}).blocker ==
              BnwFreshPlayerPostOperationsBlocker::predecessor_or_rules_invalid,
          "source predecessor and selected identity are mandatory");
    draft.rollback();
    check(!draft.detached_effect_complete && draft.source_order.empty() &&
          draft.selected_rules_sha256.empty() &&
          draft.state.plot_indices.empty(),
          "rollback must drop owned state and ordered events");
}
