#include "civ5/runtime/bnw_fresh_policies_uninit.hpp"
#include <stdexcept>

namespace {
using namespace civ5::runtime;
void check(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
BnwFreshFirstPlayerObjects objects() {
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
    const auto owner = objects();
    const auto archive = project_bnw_fresh_full_archive_reset(
        owner,"selected-sha",make_bnw_fresh_archive_defaults());
    check(archive.all_registered_member_defaults &&
          !archive.aggregate_traversal_verified,
          "policy service test needs detached archive predecessor");
    const auto fresh = make_bnw_fresh_policies_constructor_state();
    check(!fresh.latest_flavors && !fresh.has_policy &&
          !fresh.policy_ai_weights && !fresh.policies_info_bound &&
          !fresh.player_bound,
          "fresh CvPlayerPolicies constructor must leave pointers null");
    const auto fresh_draft = project_bnw_fresh_policies_uninit(
        owner,archive,"selected-sha",fresh);
    check(fresh_draft.member_effect_complete &&
          !fresh_draft.archive_predecessor_verified &&
          !fresh_draft.uninit_chain_complete &&
          !fresh_draft.initialized_by_gamecore &&
          fresh_draft.state == fresh,
          "fresh null-pointer Uninit must be a no-op without releasing gate");

    BnwFreshPoliciesServiceState allocated;
    allocated.latest_flavors = std::vector<std::int32_t>{1,2};
    allocated.has_policy = std::vector<bool>{true};
    allocated.one_shot_policy_fired = std::vector<bool>{true};
    allocated.one_shot_free_units_fired = std::vector<bool>{true};
    allocated.branch_unlocked = std::vector<bool>{true};
    allocated.branch_blocked = std::vector<bool>{true};
    allocated.branch_finished = std::vector<bool>{true};
    allocated.branches_chosen = std::vector<std::int32_t>{3};
    allocated.policy_ai_weights = std::vector<std::int32_t>{5,8};
    allocated.blocked_branch_check = std::vector<std::int32_t>{-2};
    allocated.policies_info_bound = allocated.player_bound = true;
    auto populated = project_bnw_fresh_policies_uninit(
        owner,archive,"selected-sha",allocated);
    check(populated.member_effect_complete &&
          populated.state.latest_flavors == std::nullopt &&
          populated.state.has_policy == std::nullopt &&
          populated.state.one_shot_policy_fired == std::nullopt &&
          populated.state.one_shot_free_units_fired == std::nullopt &&
          populated.state.branch_unlocked == std::nullopt &&
          populated.state.branch_blocked == std::nullopt &&
          populated.state.branch_finished == std::nullopt &&
          populated.state.branches_chosen == std::nullopt &&
          populated.state.policy_ai_weights == std::nullopt &&
          populated.state.blocked_branch_check == std::nullopt &&
          populated.state.policies_info_bound && populated.state.player_bound &&
          allocated.latest_flavors && allocated.policy_ai_weights,
          "policy Uninit must delete owned members, retain borrowed bindings and original");
    check(populated.source_order == std::vector<std::string_view>{
              "CvFlavorRecipient::m_piLatestFlavorValues",
              "m_pabHasPolicy", "m_pabHasOneShotPolicyFired",
              "m_pabHaveOneShotFreeUnitsFired", "m_pabPolicyBranchUnlocked",
              "m_pabPolicyBranchBlocked", "m_pabPolicyBranchFinished",
              "m_paePolicyBranchesChosen", "m_pPolicyAI",
              "m_paePolicyBlockedBranchCheck"},
          "CvPlayerPolicies::Uninit source-order deletion changed");
    auto wrong = project_bnw_fresh_policies_uninit(
        owner,archive,"wrong-sha",allocated);
    check(!wrong.member_effect_complete && wrong.source_order.empty(),
          "different selected rules must fail closed before mutation");
    populated.rollback();
    check(!populated.member_effect_complete && populated.source_order.empty() &&
          populated.selected_rules_sha256.empty(),
          "rollback must discard detached service effects");
}
