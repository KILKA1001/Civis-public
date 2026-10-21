#include "civ5/runtime/bnw_fresh_player_uninit_prefix.hpp"
#include "civ5/runtime/bnw_fresh_player_tail_containers.hpp"
#include <algorithm>
#include <limits>
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
    result.blockers.push_back({
        BnwFreshFirstPlayerObjectBlockerCode::reset_sync_archive_and_uninit_pending,
        "fixture"});
    return result;
}
const BnwArchiveMemberState *find_member(
    const BnwFreshFullArchiveProjection &archive, std::string_view name) {
    const auto it = std::find_if(archive.defaults.begin(),archive.defaults.end(),
        [name](const BnwArchiveMemberState &member) {
            return member.name == name;
        });
    return it == archive.defaults.end() ? nullptr : &*it;
}
} // namespace

int main() {
    const auto objects = first_player();
    const std::vector<civ5::content::GameplayDefine> constants{{"MAX_MAJOR_CIVS",3}};
    auto input = make_bnw_fresh_player_uninit_prefix_input(constants);
    check(input.has_value(),"selected native constants required");
    const auto prefix = project_bnw_fresh_player_uninit_prefix(
        objects,"selected-sha",std::move(*input));
    check(prefix.detached_effects_complete &&
          prefix.post_connections.member_effect_complete &&
          !prefix.archive_traversal_verified,
          "post-DangerPlots predecessor required");
    const auto archive = project_bnw_fresh_full_archive_reset(
        objects,"selected-sha",make_bnw_fresh_archive_defaults());
    check(archive.all_registered_member_defaults &&
          !archive.aggregate_traversal_verified,
          "isolated pre-tail archive projection required");
    const std::vector<std::string_view> names{
        "m_ppaaiSpecialistExtraYield",
        "m_ppaaiImprovementYieldChange",
        "m_ppaaiBuildingClassYieldMod"};
    for (const auto name : names) {
        const auto *before = find_member(archive,name);
        check(before && before->pending_vector_commands == 0,
              "archive reset projection must start with no vector commands");
    }

    BnwFreshPlayerTailContainersState populated;
    populated.unit_cycle_ids = {3,7,11};
    populated.research_queue = {4,9};
    populated.city_names = {"Alpha","Beta"};
    auto draft = project_bnw_fresh_player_tail_containers(
        objects,archive,prefix.post_connections,
        "selected-sha",populated);
    check(draft.detached_container_effects_complete &&
          !draft.archive_touch_intents_materialized &&
          !draft.archive_traversal_verified &&
          !draft.cities_uninit_complete && !draft.initialized_by_gamecore &&
          draft.state.unit_cycle_ids.empty() &&
          draft.state.research_queue.empty() && draft.state.city_names.empty() &&
          populated.unit_cycle_ids.size() == 3 &&
          populated.research_queue.size() == 2 &&
          populated.city_names.size() == 2,
          "tail containers must clear only detached state");
    check(draft.source_order.size() == 12,
          "three vector triplets and three lists required");
    for (std::size_t i = 0; i < names.size(); ++i) {
        const auto *after = find_member(draft.archive,names[i]);
        check(after && after->pending_vector_commands == 1 &&
              std::get<std::vector<std::array<std::int32_t,6>>>(
                  after->value).empty() &&
              draft.source_order[i*3] == BnwFreshTailEvent{
                  BnwFreshTailEventKind::archive_touch_intent,names[i]} &&
              draft.source_order[i*3+1] == BnwFreshTailEvent{
                  BnwFreshTailEventKind::vector_clear_command,names[i]} &&
              draft.source_order[i*3+2] == BnwFreshTailEvent{
                  BnwFreshTailEventKind::vector_value_clear,names[i]} &&
              find_member(archive,names[i])->pending_vector_commands == 0,
              "FAutoVector clear must touch, append CLEAR, then erase value");
    }
    check(draft.source_order[9] == BnwFreshTailEvent{
              BnwFreshTailEventKind::unit_cycle_list_clear,"m_UnitCycle"} &&
          draft.source_order[10] == BnwFreshTailEvent{
              BnwFreshTailEventKind::research_queue_clear,"m_researchQueue"} &&
          draft.source_order[11] == BnwFreshTailEvent{
              BnwFreshTailEventKind::city_names_clear,"m_cityNames"},
          "LinkedList clear order changed");

    auto malformed = archive;
    auto missing = std::find_if(malformed.defaults.begin(),malformed.defaults.end(),
        [](const BnwArchiveMemberState &member) {
            return member.name == "m_ppaaiBuildingClassYieldMod";
        });
    check(missing != malformed.defaults.end(),"fixture member must exist");
    malformed.defaults.erase(missing);
    auto refused = project_bnw_fresh_player_tail_containers(
        objects,malformed,prefix.post_connections,"selected-sha",populated);
    check(!refused.detached_container_effects_complete &&
          refused.source_order.empty() && refused.archive.defaults.empty(),
          "missing third archive vector must roll back prior two clears");
    malformed = archive;
    auto overflow = std::find_if(malformed.defaults.begin(),malformed.defaults.end(),
        [](const BnwArchiveMemberState &member) {
            return member.name == "m_ppaaiSpecialistExtraYield";
        });
    overflow->pending_vector_commands = std::numeric_limits<std::uint32_t>::max();
    refused = project_bnw_fresh_player_tail_containers(
        objects,malformed,prefix.post_connections,"selected-sha",populated);
    check(!refused.detached_container_effects_complete &&
          refused.archive.defaults.empty(),
          "command count overflow must fail closed");
    auto bad_post = prefix.post_connections;
    bad_post.member_effect_complete = false;
    refused = project_bnw_fresh_player_tail_containers(
        objects,archive,bad_post,"selected-sha",populated);
    check(!refused.detached_container_effects_complete &&
          refused.source_order.empty(),
          "missing post-DangerPlots predecessor must fail closed");
    draft.rollback();
    check(!draft.detached_container_effects_complete &&
          draft.archive.defaults.empty() && draft.source_order.empty() &&
          draft.selected_rules_sha256.empty(),
          "rollback must discard archive and list projections");
}
