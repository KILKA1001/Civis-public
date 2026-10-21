#include "civ5/runtime/bnw_fresh_archive_values.hpp"
#include <algorithm>
#include <utility>

namespace civ5::runtime {

void BnwFreshArchiveObservableReset::rollback() noexcept {
    values = {};
    registered_members.clear();
    selected_rules_sha256.clear();
    member_defaults_complete = false;
    archive_traversal_verified = false;
    uninit_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshArchiveObservableReset project_bnw_fresh_archive_observable_reset(
    const BnwFreshFirstPlayerObjects &objects,
    std::string_view selected_rules_sha256,
    BnwFreshArchiveObservableValues initial) {
    BnwFreshArchiveObservableReset result;
    const bool reset_pending = std::any_of(
        objects.blockers.begin(), objects.blockers.end(), [](const auto &blocker) {
            return blocker.code == BnwFreshFirstPlayerObjectBlockerCode::
                reset_sync_archive_and_uninit_pending;
        });
    if (!objects.constructors_complete || objects.initialized_by_gamecore ||
        objects.player_id != 0 || selected_rules_sha256.empty() ||
        objects.selected_rules_sha256 != selected_rules_sha256 ||
        !reset_pending)
        return result;

    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.values = std::move(initial);
    // Registration order from CvPlayer's constructor, not a claim about the
    // missing archive traversal implementation's iteration order.
    result.registered_members = {
        "m_iStartingX", "m_bAlive", "m_eID",
        "m_aiCityYieldChange", "m_paiNumResourceUsed"};
    result.values.starting_x = std::int32_t{};
    result.values.alive = bool{};
    result.values.player_id = std::int32_t{};
    result.values.city_yield_change.clear();
    result.values.resource_pending_commands = 0;
    result.values.resource_used.clear();
    result.member_defaults_complete = true;
    return result;
}

} // namespace civ5::runtime
