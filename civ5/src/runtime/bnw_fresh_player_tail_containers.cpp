#include "civ5/runtime/bnw_fresh_player_tail_containers.hpp"
#include <algorithm>
#include <array>
#include <limits>

namespace civ5::runtime {
namespace {
constexpr std::array<std::string_view,3> kYieldVectors{
    "m_ppaaiSpecialistExtraYield",
    "m_ppaaiImprovementYieldChange",
    "m_ppaaiBuildingClassYieldMod"};
} // namespace

void BnwFreshPlayerTailContainersDraft::rollback() noexcept {
    archive.rollback();
    state = {};
    source_order.clear();
    selected_rules_sha256.clear();
    detached_container_effects_complete = false;
    archive_touch_intents_materialized = false;
    archive_traversal_verified = false;
    cities_uninit_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshPlayerTailContainersDraft project_bnw_fresh_player_tail_containers(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    const BnwFreshPostConnectionsUninitDraft &post_connections,
    std::string_view selected_rules_sha256,
    BnwFreshPlayerTailContainersState initial) {
    BnwFreshPlayerTailContainersDraft result;
    if (!objects.constructors_complete || objects.initialized_by_gamecore ||
        objects.player_id != 0 || selected_rules_sha256.empty() ||
        objects.selected_rules_sha256 != selected_rules_sha256 ||
        archive.selected_rules_sha256 != selected_rules_sha256 ||
        !archive.all_registered_member_defaults ||
        archive.aggregate_traversal_verified || archive.initialized_by_gamecore ||
        post_connections.selected_rules_sha256 != selected_rules_sha256 ||
        !post_connections.member_effect_complete ||
        post_connections.archive_predecessor_verified ||
        post_connections.later_containers_complete ||
        post_connections.initialized_by_gamecore)
        return result;

    result.archive = archive;
    result.state = std::move(initial);
    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    for (const auto member_name : kYieldVectors) {
        auto it = std::find_if(result.archive.defaults.begin(),
                               result.archive.defaults.end(),
            [member_name](const BnwArchiveMemberState &member) {
                return member.name == member_name;
            });
        if (it == result.archive.defaults.end() ||
            it->kind != BnwArchiveMemberKind::yield_array_vector ||
            !std::holds_alternative<
                std::vector<std::array<std::int32_t,6>>>(it->value) ||
            it->pending_vector_commands ==
                std::numeric_limits<std::uint32_t>::max())
            return {};
        result.source_order.push_back({
            BnwFreshTailEventKind::archive_touch_intent,member_name});
        ++it->pending_vector_commands;
        result.source_order.push_back({
            BnwFreshTailEventKind::vector_clear_command,member_name});
        std::get<std::vector<std::array<std::int32_t,6>>>(it->value).clear();
        result.source_order.push_back({
            BnwFreshTailEventKind::vector_value_clear,member_name});
    }
    // CvUnitCycler.cpp:21-24 calls CLinkList<int>::clear; LinkedList.h:
    // 126-143 deletes each node and resets head/tail/length.
    result.state.unit_cycle_ids.clear();
    result.source_order.push_back({
        BnwFreshTailEventKind::unit_cycle_list_clear,"m_UnitCycle"});
    result.state.research_queue.clear();
    result.source_order.push_back({
        BnwFreshTailEventKind::research_queue_clear,"m_researchQueue"});
    result.state.city_names.clear();
    result.source_order.push_back({
        BnwFreshTailEventKind::city_names_clear,"m_cityNames"});
    result.detached_container_effects_complete = true;
    return result;
}

} // namespace civ5::runtime
