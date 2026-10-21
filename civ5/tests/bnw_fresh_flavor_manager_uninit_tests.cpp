#include "civ5/runtime/bnw_fresh_player_uninit_prefix.hpp"
#include "civ5/runtime/bnw_fresh_flavor_manager_uninit.hpp"
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
    check(input.has_value(),"selected native constants required");
    const auto prefix = project_bnw_fresh_player_uninit_prefix(
        objects,"selected-sha",std::move(*input));
    check(prefix.detached_effects_complete &&
          prefix.player_techs.member_effect_complete &&
          !prefix.archive_traversal_verified,
          "source-ordered PlayerTechs predecessor required");

    const auto fresh = make_bnw_fresh_flavor_manager_constructor_state();
    check(!fresh.player_bound && !fresh.personality_flavor &&
          !fresh.active_flavor && fresh.target_recipients.empty(),
          "constructor must leave pointers indeterminate/null as in SDK");
    auto draft = project_bnw_fresh_flavor_manager_uninit(
        objects,prefix.archive,prefix.player_techs,"selected-sha",fresh);
    check(draft.member_effect_complete && draft.state == fresh &&
          draft.source_order == std::vector<std::string_view>{
              "m_piPersonalityFlavor","m_piActiveFlavor",
              "m_FlavorTargetList"} &&
          !draft.archive_predecessor_verified &&
          !draft.remaining_services_complete &&
          !draft.initialized_by_gamecore,
          "fresh Uninit must preserve SDK order and readiness blockers");

    BnwFreshFlavorManagerServiceState populated;
    populated.player_bound = true;
    populated.personality_flavor = std::vector<std::int32_t>{2,3,5};
    populated.active_flavor = std::vector<std::int32_t>{7,11,13};
    populated.target_recipients = {4,1,4};
    draft = project_bnw_fresh_flavor_manager_uninit(
        objects,prefix.archive,prefix.player_techs,"selected-sha",populated);
    check(draft.member_effect_complete && draft.state.player_bound == true &&
          !draft.state.personality_flavor && !draft.state.active_flavor &&
          draft.state.target_recipients.empty() &&
          populated.target_recipients == std::vector<std::int32_t>{4,1,4},
          "Uninit deletes arrays and clears detached recipients, not caller");

    auto missing = prefix.player_techs;
    missing.member_effect_complete = false;
    const auto refused = project_bnw_fresh_flavor_manager_uninit(
        objects,prefix.archive,missing,"selected-sha",populated);
    check(!refused.member_effect_complete && refused.source_order.empty(),
          "missing PlayerTechs predecessor must fail closed");
    const auto wrong = project_bnw_fresh_flavor_manager_uninit(
        objects,prefix.archive,prefix.player_techs,"wrong-sha",populated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "rules mismatch must fail closed");
    draft.rollback();
    check(!draft.member_effect_complete && draft.source_order.empty() &&
          draft.selected_rules_sha256.empty(),
          "rollback must discard detached manager projection");
}
