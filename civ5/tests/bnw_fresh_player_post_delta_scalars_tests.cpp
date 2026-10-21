#include "civ5/runtime/bnw_fresh_player_post_delta_scalars.hpp"
#include <set>
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
BnwFreshPlayerPostOperationsDraft post_operations() {
    BnwFreshPlayerPostOperationsDraft result;
    result.selected_rules_sha256 = "selected-sha";
    result.detached_effect_complete = true;
    result.source_order = {
        {BnwFreshPlayerPostOperationsEventKind::plots_size_zero,
         "m_aiPlots.clear"},
        {BnwFreshPlayerPostOperationsEventKind::conquered_by_words_zero,
         "m_bfEverConqueredBy.ClearAll"}};
    return result;
}
} // namespace

int main() {
    const auto writes = bnw_fresh_player_post_delta_scalar_writes();
    check(writes.size() == 234 &&
          writes.front() == BnwFreshPlayerScalarWrite{
              "m_iStartingX",-2147483647,false,733} &&
          writes.back() == BnwFreshPlayerScalarWrite{
              "m_eID",-1,false,968},
          "source-generated scalar interval must remain complete");
    std::set<std::string_view> unique_names;
    std::uint16_t last_line = 732;
    for (const auto &write : writes) {
        check(unique_names.insert(write.sdk_member).second &&
              write.sdk_line > last_line && write.sdk_line <= 968,
              "scalar writes must be unique and source ordered");
        last_line = write.sdk_line;
    }
    const auto find = [writes](std::string_view name) {
        for (const auto &write : writes)
            if (write.sdk_member == name) return write;
        throw std::runtime_error("missing scalar write");
    };
    check(find("m_iAdvancedStartPoints").value == -1 &&
          find("m_iHighestUnitLevel").value == 1 &&
          find("m_iMedianTechPercentage").value == 50 &&
          find("m_eFaithPurchaseType").value == 0 &&
          find("m_bDynamicTurnsSimultMode").value == 1 &&
          find("m_bDynamicTurnsSimultMode").boolean_value,
          "nonzero, enum and boolean SDK defaults must survive generation");

    std::map<std::string,std::int64_t> initial;
    for (const auto &write : writes)
        initial.emplace(std::string(write.sdk_member),77);
    const auto objects = first_player();
    const auto predecessor = post_operations();
    auto draft = project_bnw_fresh_player_post_delta_scalars(
        objects,predecessor,"selected-sha",initial);
    check(draft.detached_value_projection_complete &&
          draft.source_order.size() == writes.size() &&
          draft.fields.size() == writes.size() &&
          draft.blocker ==
              BnwFreshPlayerPostDeltaScalarsBlocker::archive_clear_delta_pending &&
          draft.blocked_at == "archive.clearDelta" &&
          !draft.archive_clear_delta_verified &&
          !draft.archive_reset_traversal_verified &&
          !draft.player_uninit_complete &&
          !draft.initialized_by_gamecore &&
          initial.at("m_iStartingX") == 77,
          "detached values may project but archive and game readiness stay blocked");
    for (std::size_t i = 0; i < writes.size(); ++i)
        check(draft.source_order[i] == writes[i] &&
              draft.fields.at(std::string(writes[i].sdk_member)) == writes[i].value,
              "every explicit scalar write must execute in source order");

    auto incomplete = initial;
    incomplete.erase("m_iTotalPopulation");
    const auto rejected = project_bnw_fresh_player_post_delta_scalars(
        objects,predecessor,"selected-sha",std::move(incomplete));
    check(!rejected.detached_value_projection_complete &&
          rejected.blocker ==
              BnwFreshPlayerPostDeltaScalarsBlocker::field_set_invalid &&
          rejected.fields.empty() && rejected.source_order.empty(),
          "missing field must reject the entire projection");
    check(project_bnw_fresh_player_post_delta_scalars(
              objects,predecessor,"wrong-sha",initial).blocker ==
              BnwFreshPlayerPostDeltaScalarsBlocker::predecessor_or_rules_invalid,
          "selected rules provenance is mandatory");
    draft.rollback();
    check(!draft.detached_value_projection_complete &&
          draft.fields.empty() && draft.source_order.empty() &&
          draft.selected_rules_sha256.empty(),
          "rollback must discard all projected scalar state");
}
