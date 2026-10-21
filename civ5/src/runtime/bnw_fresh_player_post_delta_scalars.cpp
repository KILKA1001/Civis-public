#include "civ5/runtime/bnw_fresh_player_post_delta_scalars.hpp"
#include <array>
#include <utility>

namespace civ5::runtime {
#include "civ5/runtime/bnw_fresh_player_post_delta_scalars.generated.hpp"

std::span<const BnwFreshPlayerScalarWrite>
bnw_fresh_player_post_delta_scalar_writes() noexcept {
    return kBnwFreshPostDeltaWrites;
}

void BnwFreshPlayerPostDeltaScalarsDraft::rollback() noexcept {
    fields.clear();
    source_order.clear();
    selected_rules_sha256.clear();
    blocker = BnwFreshPlayerPostDeltaScalarsBlocker::none;
    blocked_at = {};
    detached_value_projection_complete = false;
    archive_clear_delta_verified = false;
    archive_reset_traversal_verified = false;
    player_uninit_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshPlayerPostDeltaScalarsDraft
project_bnw_fresh_player_post_delta_scalars(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshPlayerPostOperationsDraft &predecessor,
    std::string_view selected_rules_sha256,
    std::map<std::string,std::int64_t> initial_fields) {
    BnwFreshPlayerPostDeltaScalarsDraft result;
    if (!objects.constructors_complete || objects.initialized_by_gamecore ||
        objects.player_id != 0 || selected_rules_sha256.empty() ||
        objects.selected_rules_sha256 != selected_rules_sha256 ||
        predecessor.selected_rules_sha256 != selected_rules_sha256 ||
        !predecessor.detached_effect_complete ||
        predecessor.source_order !=
            std::vector<BnwFreshPlayerPostOperationsEvent>{
                {BnwFreshPlayerPostOperationsEventKind::plots_size_zero,
                 "m_aiPlots.clear"},
                {BnwFreshPlayerPostOperationsEventKind::conquered_by_words_zero,
                 "m_bfEverConqueredBy.ClearAll"}} ||
        predecessor.archive_clear_delta_verified ||
        predecessor.archive_traversal_verified ||
        predecessor.initialized_by_gamecore) {
        result.blocker =
            BnwFreshPlayerPostDeltaScalarsBlocker::predecessor_or_rules_invalid;
        result.blocked_at = "archive.clearDelta";
        return result;
    }
    if (initial_fields.size() != kBnwFreshPostDeltaWrites.size()) {
        result.blocker = BnwFreshPlayerPostDeltaScalarsBlocker::field_set_invalid;
        result.blocked_at = "CvPlayer::uninit scalar fields";
        return result;
    }
    for (const auto &write : kBnwFreshPostDeltaWrites) {
        if (!initial_fields.contains(std::string(write.sdk_member))) {
            result.blocker = BnwFreshPlayerPostDeltaScalarsBlocker::field_set_invalid;
            result.blocked_at = write.sdk_member;
            return result;
        }
    }

    result.fields = std::move(initial_fields);
    result.source_order.reserve(kBnwFreshPostDeltaWrites.size());
    for (const auto &write : kBnwFreshPostDeltaWrites) {
        result.fields.at(std::string(write.sdk_member)) = write.value;
        result.source_order.push_back(write);
    }
    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.detached_value_projection_complete = true;
    result.blocker = BnwFreshPlayerPostDeltaScalarsBlocker::archive_clear_delta_pending;
    result.blocked_at = "archive.clearDelta";
    return result;
}

} // namespace civ5::runtime
