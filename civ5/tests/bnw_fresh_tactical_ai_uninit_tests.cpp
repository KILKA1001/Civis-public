#include "civ5/runtime/bnw_fresh_player_uninit_prefix.hpp"
#include "civ5/runtime/bnw_fresh_tactical_ai_uninit.hpp"
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
} // namespace

int main() {
    const auto objects = first_player();
    const std::vector<civ5::content::GameplayDefine> constants{{"MAX_MAJOR_CIVS",3}};
    auto input = make_bnw_fresh_player_uninit_prefix_input(constants);
    check(input.has_value(),"native constants required");
    const auto prefix = project_bnw_fresh_player_uninit_prefix(
        objects,"selected-sha",std::move(*input));
    check(prefix.detached_effects_complete &&
          prefix.flavor_manager.member_effect_complete &&
          !prefix.archive_traversal_verified,
          "source-ordered FlavorManager predecessor required");

    const auto fresh = make_bnw_fresh_tactical_ai_constructor_state();
    check(fresh.current_zone_index == -1 &&
          fresh.opaque_member_snapshot.empty(),
          "constructor sets current zone index to -1");
    auto draft = project_bnw_fresh_tactical_ai_uninit(
        objects,prefix.archive,prefix.flavor_manager,"selected-sha",fresh);
    check(draft.member_effect_complete && draft.state == fresh &&
          draft.source_order == std::vector<std::string_view>{
              "CvTacticalAI::Uninit"} &&
          !draft.archive_predecessor_verified &&
          !draft.remaining_services_complete &&
          !draft.initialized_by_gamecore,
          "empty Uninit must preserve owner and blockers");

    BnwFreshTacticalAiServiceState populated;
    populated.current_zone_index = 4;
    populated.opaque_member_snapshot = {
        std::byte{0x17},std::byte{0x3b},std::byte{0x81}};
    draft = project_bnw_fresh_tactical_ai_uninit(
        objects,prefix.archive,prefix.flavor_manager,"selected-sha",populated);
    check(draft.member_effect_complete && draft.state == populated &&
          populated.opaque_member_snapshot.size() == 3,
          "Uninit must not call Reset or discard populated tactical state");

    auto missing = prefix.flavor_manager;
    missing.member_effect_complete = false;
    const auto refused = project_bnw_fresh_tactical_ai_uninit(
        objects,prefix.archive,missing,"selected-sha",populated);
    check(!refused.member_effect_complete && refused.source_order.empty(),
          "missing FlavorManager predecessor must fail closed");
    const auto wrong = project_bnw_fresh_tactical_ai_uninit(
        objects,prefix.archive,prefix.flavor_manager,"wrong-sha",populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "selected rules mismatch must fail closed");
    draft.rollback();
    check(!draft.member_effect_complete && draft.source_order.empty() &&
          draft.selected_rules_sha256.empty(),
          "rollback must discard detached observation");
}
