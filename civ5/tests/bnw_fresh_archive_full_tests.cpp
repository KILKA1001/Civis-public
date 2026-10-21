#include "civ5/runtime/bnw_fresh_archive_full.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace {
using namespace civ5::runtime;
void check(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
BnwFreshFirstPlayerObjects first_player() {
    BnwFreshFirstPlayerObjects objects;
    objects.player_id = 0;
    objects.selected_rules_sha256 = "selected-sha";
    objects.constructors_complete = true;
    objects.blockers.push_back({
        BnwFreshFirstPlayerObjectBlockerCode::reset_sync_archive_and_uninit_pending,
        "fixture"});
    return objects;
}
void dirty(BnwArchiveMemberState &member) {
    switch (member.kind) {
    case BnwArchiveMemberKind::signed_scalar: member.value = std::int32_t{7}; break;
    case BnwArchiveMemberKind::unsigned_scalar: member.value = std::uint32_t{7}; break;
    case BnwArchiveMemberKind::boolean: member.value = true; break;
    case BnwArchiveMemberKind::text: member.value = std::string{"dirty"}; break;
    case BnwArchiveMemberKind::int_vector:
        member.value = std::vector<std::int32_t>{7}; break;
    case BnwArchiveMemberKind::bool_vector:
        member.value = std::vector<bool>{true}; break;
    case BnwArchiveMemberKind::options_vector:
        member.value = std::vector<std::pair<std::uint32_t,std::int32_t>>{{2,7}}; break;
    case BnwArchiveMemberKind::yield_array_vector:
        member.value = std::vector<std::array<std::int32_t,6>>{{7,7,7,7,7,7}}; break;
    }
    if (member.kind == BnwArchiveMemberKind::int_vector ||
        member.kind == BnwArchiveMemberKind::bool_vector ||
        member.kind == BnwArchiveMemberKind::options_vector ||
        member.kind == BnwArchiveMemberKind::yield_array_vector)
        member.pending_vector_commands = 3;
}
bool has_write(const BnwFreshFullArchiveProjection &projection,
               BnwArchiveExplicitWriteKind kind, std::string_view name,
               std::int64_t value) {
    return std::any_of(projection.later_writes.begin(),projection.later_writes.end(),
        [&](const auto &write) {
            return write.kind == kind && write.member == name &&
                   write.scalar_value == value;
        });
}
} // namespace

int main() {
    auto objects = first_player();
    const auto defaults = make_bnw_fresh_archive_defaults();
    check(defaults.size() == 214 &&
          defaults.front().name == "m_iStartingX" &&
          defaults.back().name == "m_iNumFreePoliciesEver",
          "generated BNW archive registration order differs");
    auto dirty_values = defaults;
    for (auto &entry : dirty_values) dirty(entry);
    const auto original = dirty_values;
    auto projection = project_bnw_fresh_full_archive_reset(
        objects,"selected-sha",dirty_values);
    check(projection.all_registered_member_defaults &&
          !projection.aggregate_traversal_verified &&
          !projection.uninit_services_complete &&
          !projection.player_reset_complete &&
          !projection.initialized_by_gamecore &&
          projection.defaults.size() == defaults.size() &&
          projection.later_writes.size() == 195,
          "complete member defaults must retain archive and service blockers");
    for (std::size_t i=0; i<defaults.size(); ++i)
        check(projection.defaults[i].name == defaults[i].name &&
              projection.defaults[i].kind == defaults[i].kind &&
              projection.defaults[i].value == defaults[i].value &&
              projection.defaults[i].pending_vector_commands == 0 &&
              dirty_values[i].value == original[i].value &&
              dirty_values[i].pending_vector_commands == original[i].pending_vector_commands,
              "one of the 214 registered defaults or ownership checks differs");
    check(has_write(projection,BnwArchiveExplicitWriteKind::uninit_vector_clear_before_services,
                    "m_paiNumResourceUsed",0) &&
          has_write(projection,BnwArchiveExplicitWriteKind::uninit_service_callbacks_pending,
                    "CvPlayer::uninit services",0) &&
          has_write(projection,BnwArchiveExplicitWriteKind::uninit_vector_clear_after_services,
                    "m_ppaaiSpecialistExtraYield",0) &&
          has_write(projection,BnwArchiveExplicitWriteKind::uninit_archive_clear_delta_pending,
                    "m_syncArchive",0) &&
          has_write(projection,BnwArchiveExplicitWriteKind::uninit_scalar_after_services,
                    "m_iStartingX",-std::numeric_limits<std::int32_t>::max()) &&
          has_write(projection,BnwArchiveExplicitWriteKind::uninit_scalar_after_services,
                    "m_iHighestUnitLevel",1) &&
          has_write(projection,BnwArchiveExplicitWriteKind::uninit_scalar_after_services,
                    "m_iCityDistanceHighwaterMark",1) &&
          has_write(projection,BnwArchiveExplicitWriteKind::uninit_scalar_after_services,
                    "m_eID",-1),
          "source-authored uninit values or callback frontier differ");
    const auto callback = std::find_if(projection.later_writes.begin(),
        projection.later_writes.end(),[](const auto &event) {
            return event.kind == BnwArchiveExplicitWriteKind::
                uninit_service_callbacks_pending;
        });
    const auto clear_delta = std::find_if(projection.later_writes.begin(),
        projection.later_writes.end(),[](const auto &event) {
            return event.kind == BnwArchiveExplicitWriteKind::
                uninit_archive_clear_delta_pending;
        });
    check(callback != projection.later_writes.end() &&
          clear_delta != projection.later_writes.end() && callback < clear_delta &&
          projection.later_writes.front().kind == BnwArchiveExplicitWriteKind::
              uninit_vector_clear_before_services &&
          projection.later_writes.back().member == "m_eID",
          "CvPlayer::uninit source order differs");

    BnwFreshPlayerResetPrefix prefix;
    prefix.selected_rules_sha256 = "selected-sha";
    prefix.early_arrays_complete = true;
    auto &first = prefix.players.emplace_back();
    first.id = 0;
    first.personality = 8;
    first.ever_popped_goody = false;
    for (auto &yields : first.yield_vectors) yields.assign(6,0);
    first.proximity_to_player.assign(4,0);
    first.research_agreement_counter.assign(4,0);
    first.incoming_unit_types.assign(4,-1);
    first.incoming_unit_countdowns.assign(4,-1);
    first.minor_friendship_anchors.assign(4,0);
    check(stage_bnw_fresh_archive_reset_prefix(projection,prefix) &&
          projection.reset_prefix_staged && !projection.player_reset_complete &&
          projection.reset_prefix_writes.size() == 20 &&
          projection.reset_prefix_writes.front().member == "m_eID" &&
          projection.reset_prefix_writes[1].member == "m_ePersonalityType" &&
          std::get<std::int32_t>(projection.reset_prefix_writes[1].value) == 8 &&
          projection.reset_prefix_writes[3].member == "m_aiCityYieldChange" &&
          projection.reset_prefix_writes.back().member == "m_strScriptData",
          "registered CvPlayer::reset prefix writes differ from selected draft");
    check(!stage_bnw_fresh_archive_reset_prefix(projection,prefix),
          "reset prefix must not be staged twice");
    prefix.selected_rules_sha256 = "wrong-sha";
    auto other = project_bnw_fresh_full_archive_reset(
        objects,"selected-sha",dirty_values);
    check(!stage_bnw_fresh_archive_reset_prefix(other,prefix) &&
          !other.reset_prefix_staged && other.reset_prefix_writes.empty(),
          "reset prefix must reject different selected rules identity");

    auto bad = dirty_values;
    bad.erase(bad.begin());
    check(!project_bnw_fresh_full_archive_reset(objects,"selected-sha",bad)
               .all_registered_member_defaults,
          "missing registered member must fail closed");
    bad = dirty_values;
    bad[0].name = "m_iDifferent";
    check(!project_bnw_fresh_full_archive_reset(objects,"selected-sha",bad)
               .all_registered_member_defaults,
          "reordered or wrong archive member must fail closed");
    bad = dirty_values;
    bad[0].value = true;
    check(!project_bnw_fresh_full_archive_reset(objects,"selected-sha",bad)
               .all_registered_member_defaults,
          "wrong archive value type must fail closed");
    check(!project_bnw_fresh_full_archive_reset(objects,"wrong-sha",dirty_values)
               .all_registered_member_defaults,
          "wrong selected rules identity must fail closed");
    projection.rollback();
    check(!projection.all_registered_member_defaults &&
          projection.defaults.empty() && projection.later_writes.empty() &&
          projection.reset_prefix_writes.empty(),
          "projection rollback must discard all member effects");
}
