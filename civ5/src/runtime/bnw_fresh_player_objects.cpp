#include "civ5/runtime/bnw_fresh_player_objects.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <limits>
#include <optional>
#include <utility>

namespace civ5::runtime {
namespace {
std::optional<std::int32_t> native_constant(
    std::span<const content::GameplayDefine> constants,std::string_view name) {
    std::optional<std::int32_t> result;
    for (const auto &item : constants) {
        if (item.name != name) continue;
        if (result || item.value <= 0 ||
            item.value > std::numeric_limits<std::int32_t>::max())
            return std::nullopt;
        result = static_cast<std::int32_t>(item.value);
    }
    return result;
}
} // namespace

bool BnwFreshFirstPlayerObjects::has_blocker(
    BnwFreshFirstPlayerObjectBlockerCode code) const {
    return std::any_of(blockers.begin(),blockers.end(),
                       [code](const auto &item) { return item.code == code; });
}
void BnwFreshFirstPlayerObjects::rollback() noexcept {
    player_id = -1;
    notifications = {};
    diplomacy_requests = {};
    source_order.clear();
    selected_rules_sha256.clear();
    blockers.clear();
    constructors_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshFirstPlayerObjects draft_bnw_fresh_first_player_objects(
    const BnwFreshGameContentResult &selected,
    const BnwFreshSlotInfoDraft &slot_info,
    std::string_view selected_rules_sha256,
    std::span<const content::GameplayDefine> native_constants) {
    BnwFreshFirstPlayerObjects draft;
    const auto major = native_constant(native_constants,"MAX_MAJOR_CIVS");
    const auto players = native_constant(native_constants,"MAX_PLAYERS");
    if (!selected.content_ready() || !selected.rules || !selected.activation ||
        selected.profile.kind == ContentProfileKind::vanilla ||
        selected.activation->profile_kind != selected.profile.kind ||
        !slot_info.pregame_writes_complete || slot_info.initialized_by_gamecore ||
        !slot_info.has_blocker(BnwFreshSlotInfoBlockerCode::player_init_pending) ||
        selected_rules_sha256.empty() ||
        slot_info.selected_rules_sha256 != selected_rules_sha256 ||
        identify_bytes(selected.rules->gameplay_fingerprint_data()).sha256 !=
            selected_rules_sha256 || !major || !players ||
        *major > *players ||
        slot_info.slots.size() != static_cast<std::size_t>(*players)) {
        draft.blockers.push_back({
            BnwFreshFirstPlayerObjectBlockerCode::selected_pregame_or_constants_unverified,
            "First major player requires complete selected CvPreGame writes"});
        return draft;
    }
    draft.selected_rules_sha256 = std::string(selected_rules_sha256);
    draft.player_id = 0;
    // CvNotifications::CvNotifications calls Uninit. The constructor does
    // not allocate MAX_NOTIFICATIONS entries; that happens only in Init.
    draft.source_order.push_back({
        BnwFreshFirstPlayerObjectStep::notifications_allocated,0});
    draft.notifications = {};
    draft.source_order.push_back({
        BnwFreshFirstPlayerObjectStep::notifications_constructor_uninit,0});
    // CvDiplomacyRequests::CvDiplomacyRequests also calls Uninit and leaves
    // its request vector empty and active state false.
    draft.source_order.push_back({
        BnwFreshFirstPlayerObjectStep::diplomacy_requests_allocated,0});
    draft.diplomacy_requests = {};
    draft.source_order.push_back({
        BnwFreshFirstPlayerObjectStep::diplomacy_requests_constructor_uninit,0});
    draft.source_order.push_back({
        BnwFreshFirstPlayerObjectStep::reset_sync_archive_pending,0});
    draft.constructors_complete = true;
    draft.blockers.push_back({
        BnwFreshFirstPlayerObjectBlockerCode::reset_sync_archive_and_uninit_pending,
        "CvPlayer::reset must reset sync archive and uninit all services before containers"});
    return draft;
}
} // namespace civ5::runtime
