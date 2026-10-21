#include "civ5/runtime/bnw_fresh_archive_full.hpp"
#include <algorithm>
#include <utility>

namespace civ5::runtime {
namespace {
using K = BnwArchiveMemberKind;
BnwArchiveValue default_value(K kind) {
    switch (kind) {
    case K::signed_scalar: return std::int32_t{};
    case K::unsigned_scalar: return std::uint32_t{};
    case K::boolean: return bool{};
    case K::text: return std::string{};
    case K::int_vector: return std::vector<std::int32_t>{};
    case K::bool_vector: return std::vector<bool>{};
    case K::options_vector:
        return std::vector<std::pair<std::uint32_t,std::int32_t>>{};
    case K::yield_array_vector:
        return std::vector<std::array<std::int32_t,6>>{};
    }
    return std::int32_t{};
}
bool right_type(const BnwArchiveMemberState &entry) {
    switch (entry.kind) {
    case K::signed_scalar: return std::holds_alternative<std::int32_t>(entry.value);
    case K::unsigned_scalar: return std::holds_alternative<std::uint32_t>(entry.value);
    case K::boolean: return std::holds_alternative<bool>(entry.value);
    case K::text: return std::holds_alternative<std::string>(entry.value);
    case K::int_vector:
        return std::holds_alternative<std::vector<std::int32_t>>(entry.value);
    case K::bool_vector: return std::holds_alternative<std::vector<bool>>(entry.value);
    case K::options_vector:
        return std::holds_alternative<
            std::vector<std::pair<std::uint32_t,std::int32_t>>>(entry.value);
    case K::yield_array_vector:
        return std::holds_alternative<std::vector<std::array<std::int32_t,6>>>(entry.value);
    }
    return false;
}
bool vector_kind(K kind) {
    return kind == K::int_vector || kind == K::bool_vector ||
           kind == K::options_vector || kind == K::yield_array_vector;
}
bool valid_archive(const std::vector<BnwArchiveMemberState> &entries) {
    if (entries.size() != kBnwPlayerArchiveMembers.size()) return false;
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto &member = entries[i];
        const auto &info = kBnwPlayerArchiveMembers[i];
        if (member.name != info.name || member.kind != info.kind ||
            !right_type(member) ||
            (!vector_kind(member.kind) && member.pending_vector_commands))
            return false;
    }
    return true;
}
bool first_player_pending(const BnwFreshFirstPlayerObjects &objects,
                          std::string_view sha) {
    return objects.constructors_complete && !objects.initialized_by_gamecore &&
           objects.player_id == 0 && !sha.empty() &&
           objects.selected_rules_sha256 == sha &&
           std::any_of(objects.blockers.begin(), objects.blockers.end(),
               [](const auto &blocker) {
                   return blocker.code == BnwFreshFirstPlayerObjectBlockerCode::
                       reset_sync_archive_and_uninit_pending;
               });
}
} // namespace

void BnwFreshFullArchiveProjection::rollback() noexcept {
    defaults.clear();
    later_writes.clear();
    reset_prefix_writes.clear();
    selected_rules_sha256.clear();
    all_registered_member_defaults = false;
    aggregate_traversal_verified = false;
    uninit_services_complete = false;
    reset_prefix_staged = false;
    player_reset_complete = false;
    initialized_by_gamecore = false;
}

std::vector<BnwArchiveMemberState> make_bnw_fresh_archive_defaults() {
    std::vector<BnwArchiveMemberState> result;
    result.reserve(kBnwPlayerArchiveMembers.size());
    for (const auto &info : kBnwPlayerArchiveMembers)
        result.push_back({info.name, info.kind, default_value(info.kind), 0});
    return result;
}

BnwFreshFullArchiveProjection project_bnw_fresh_full_archive_reset(
    const BnwFreshFirstPlayerObjects &objects,
    std::string_view selected_rules_sha256,
    std::vector<BnwArchiveMemberState> initial) {
    BnwFreshFullArchiveProjection result;
    if (!first_player_pending(objects, selected_rules_sha256) ||
        !valid_archive(initial))
        return result;
    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.defaults = std::move(initial);
    for (auto &member : result.defaults) {
        member.value = default_value(member.kind);
        member.pending_vector_commands = 0;
    }
    result.all_registered_member_defaults = true;
    // Direct source writes from CvPlayer::uninit are recorded in source order.
    // They are not applied to `defaults`: service Uninit callbacks, archives'
    // aggregate traversal and later reset writes are still unavailable.
    bool service_frontier_recorded = false;
    for (const auto &clear : kBnwPlayerUninitVectorClears) {
        if (clear.after_service_callbacks && !service_frontier_recorded) {
            result.later_writes.push_back({
                BnwArchiveExplicitWriteKind::uninit_service_callbacks_pending,
                "CvPlayer::uninit services", 0});
            service_frontier_recorded = true;
        }
        result.later_writes.push_back({
            clear.after_service_callbacks ?
                BnwArchiveExplicitWriteKind::uninit_vector_clear_after_services :
                BnwArchiveExplicitWriteKind::uninit_vector_clear_before_services,
            clear.name, 0});
    }
    result.later_writes.push_back({
        BnwArchiveExplicitWriteKind::uninit_archive_clear_delta_pending,
        "m_syncArchive", 0});
    for (const auto &write : kBnwPlayerUninitScalarWrites)
        result.later_writes.push_back({
            BnwArchiveExplicitWriteKind::uninit_scalar_after_services,
            write.name, write.value});
    return result;
}

bool stage_bnw_fresh_archive_reset_prefix(
    BnwFreshFullArchiveProjection &projection,
    const BnwFreshPlayerResetPrefix &early_reset) {
    if (!projection.all_registered_member_defaults ||
        projection.aggregate_traversal_verified ||
        projection.uninit_services_complete || projection.reset_prefix_staged ||
        projection.initialized_by_gamecore || !early_reset.early_arrays_complete ||
        early_reset.initialized_by_gamecore ||
        early_reset.selected_rules_sha256 != projection.selected_rules_sha256 ||
        early_reset.players.empty() || early_reset.players.front().id != 0)
        return false;
    const auto &player = early_reset.players.front();
    if (!player.options.empty()) return false;
    std::vector<BnwArchiveResetPrefixWrite> staged;
    staged.reserve(20);
    const auto push = [&](std::string_view name, BnwArchiveValue value) {
        const bool registered = std::any_of(
            kBnwPlayerArchiveMembers.begin(), kBnwPlayerArchiveMembers.end(),
            [&](const auto &member) { return member.name == name; });
        if (!registered) return false;
        staged.push_back({name,std::move(value)});
        return true;
    };
    if (!push("m_eID",player.id) ||
        !push("m_ePersonalityType",player.personality) ||
        !push("m_bEverPoppedGoody",player.ever_popped_goody)) return false;
    constexpr std::array<std::string_view,10> yields{
        "m_aiCityYieldChange", "m_aiCoastalCityYieldChange",
        "m_aiCapitalYieldChange", "m_aiCapitalYieldPerPopChange",
        "m_aiSeaPlotYield", "m_aiYieldRateModifier",
        "m_aiCapitalYieldRateModifier", "", // GreatWork is not registered.
        "m_aiExtraYieldThreshold", "m_aiSpecialistExtraYield"};
    for (std::size_t i=0; i<yields.size(); ++i)
        if (!yields[i].empty() && !push(yields[i],player.yield_vectors[i]))
            return false;
    if (!push("m_aiProximityToPlayer",player.proximity_to_player) ||
        !push("m_aiResearchAgreementCounter",player.research_agreement_counter) ||
        !push("m_aiIncomingUnitTypes",player.incoming_unit_types) ||
        !push("m_aiIncomingUnitCountdowns",player.incoming_unit_countdowns) ||
        !push("m_aiMinorFriendshipAnchors",player.minor_friendship_anchors) ||
        !push("m_aOptions",
              std::vector<std::pair<std::uint32_t,std::int32_t>>{}) ||
        !push("m_strReligionKey",player.religion_key) ||
        !push("m_strScriptData",player.script_data))
        return false;
    projection.reset_prefix_writes = std::move(staged);
    projection.reset_prefix_staged = true;
    return true;
}

} // namespace civ5::runtime
