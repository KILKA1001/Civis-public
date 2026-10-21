#include "civ5/runtime/bnw_fresh_policies_uninit.hpp"
#include <algorithm>
#include <utility>

namespace civ5::runtime {

void BnwFreshPoliciesUninitDraft::rollback() noexcept {
    state = {};
    source_order.clear();
    selected_rules_sha256.clear();
    member_effect_complete = false;
    archive_predecessor_verified = false;
    uninit_chain_complete = false;
    initialized_by_gamecore = false;
}

BnwFreshPoliciesServiceState make_bnw_fresh_policies_constructor_state() {
    // CvPolicyClasses.cpp:1977-1992 initializes every deleted pointer NULL.
    // CvFlavorRecipient.cpp (CvFlavorManager.cpp:17-21) does the same for
    // m_piLatestFlavorValues. Borrowed m_pPolicies/m_pPlayer are also NULL.
    return {};
}

BnwFreshPoliciesUninitDraft project_bnw_fresh_policies_uninit(
    const BnwFreshFirstPlayerObjects &objects,
    const BnwFreshFullArchiveProjection &archive,
    std::string_view selected_rules_sha256,
    BnwFreshPoliciesServiceState initial) {
    BnwFreshPoliciesUninitDraft result;
    const bool pending = std::any_of(
        objects.blockers.begin(),objects.blockers.end(),[](const auto &blocker) {
            return blocker.code == BnwFreshFirstPlayerObjectBlockerCode::
                reset_sync_archive_and_uninit_pending;
        });
    if (!objects.constructors_complete || objects.initialized_by_gamecore ||
        objects.player_id != 0 || !pending || selected_rules_sha256.empty() ||
        objects.selected_rules_sha256 != selected_rules_sha256 ||
        archive.selected_rules_sha256 != selected_rules_sha256 ||
        !archive.all_registered_member_defaults ||
        archive.aggregate_traversal_verified || archive.initialized_by_gamecore)
        return result;
    result.selected_rules_sha256 = std::string(selected_rules_sha256);
    result.state = std::move(initial);
    // CvPolicyClasses.cpp:2041-2054. SAFE_DELETE[_ARRAY] sets each pointer
    // to NULL; the CvPolicyAI destructor at CvPolicyAI.cpp:24 is empty.
    result.state.latest_flavors.reset();
    result.source_order.push_back("CvFlavorRecipient::m_piLatestFlavorValues");
    result.state.has_policy.reset();
    result.source_order.push_back("m_pabHasPolicy");
    result.state.one_shot_policy_fired.reset();
    result.source_order.push_back("m_pabHasOneShotPolicyFired");
    result.state.one_shot_free_units_fired.reset();
    result.source_order.push_back("m_pabHaveOneShotFreeUnitsFired");
    result.state.branch_unlocked.reset();
    result.source_order.push_back("m_pabPolicyBranchUnlocked");
    result.state.branch_blocked.reset();
    result.source_order.push_back("m_pabPolicyBranchBlocked");
    result.state.branch_finished.reset();
    result.source_order.push_back("m_pabPolicyBranchFinished");
    result.state.branches_chosen.reset();
    result.source_order.push_back("m_paePolicyBranchesChosen");
    result.state.policy_ai_weights.reset();
    result.source_order.push_back("m_pPolicyAI");
    result.state.blocked_branch_check.reset();
    result.source_order.push_back("m_paePolicyBlockedBranchCheck");
    result.member_effect_complete = true;
    return result;
}

} // namespace civ5::runtime
