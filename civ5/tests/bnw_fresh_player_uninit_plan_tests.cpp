#include "civ5/runtime/bnw_fresh_player_uninit_plan.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>

namespace {
void check(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
void test_first_player_uninit_frontier() {
    using namespace civ5::runtime;
    BnwFreshGameContentResult selected;
    selected.profile.kind = ContentProfileKind::dlc;
    selected.rules = std::make_unique<civ5::content::RulesDatabase>();
    selected.activation.emplace();
    selected.activation->profile_kind = ContentProfileKind::dlc;
    selected.activation->rules_database_ready = true;
    const auto sha = identify_bytes(selected.rules->gameplay_fingerprint_data()).sha256;
    BnwFreshFirstPlayerObjects objects;
    objects.player_id = 0;
    objects.selected_rules_sha256 = sha;
    objects.constructors_complete = true;
    objects.blockers.push_back({
        BnwFreshFirstPlayerObjectBlockerCode::reset_sync_archive_and_uninit_pending,
        "fixture"});
    auto plan = plan_bnw_fresh_first_player_uninit(selected,objects,sha);
    check(plan.preflight_complete && !plan.sync_archive_reset_verified &&
              !plan.uninit_complete && !plan.initialized_by_gamecore &&
              plan.planned_source_order.size() == 50 &&
              plan.planned_source_order[0] == BnwFreshPlayerUninitEvent{
                  BnwFreshPlayerUninitKind::sync_archive_reset,"m_syncArchive"} &&
              plan.planned_source_order[1] == BnwFreshPlayerUninitEvent{
                  BnwFreshPlayerUninitKind::vector_clear,"m_paiNumResourceUsed"} &&
              plan.planned_source_order[21] == BnwFreshPlayerUninitEvent{
                  BnwFreshPlayerUninitKind::vector_clear,"m_pabGetsScienceFromPlayer"} &&
              plan.planned_source_order[22] == BnwFreshPlayerUninitEvent{
                  BnwFreshPlayerUninitKind::service_uninit,"m_pPlayerPolicies"} &&
              plan.planned_source_order[43] == BnwFreshPlayerUninitEvent{
                  BnwFreshPlayerUninitKind::service_uninit,"m_pCityConnections"} &&
              plan.planned_source_order[44] == BnwFreshPlayerUninitEvent{
                  BnwFreshPlayerUninitKind::optional_service_uninit,"m_pNotifications"} &&
              plan.planned_source_order[49].kind ==
                  BnwFreshPlayerUninitKind::later_player_state_reset,
          "Uninit dependency plan preserves archive, vectors and service order");
    check(plan.has_blocker(BnwFreshPlayerUninitBlockerCode::
                           sync_archive_reset_semantics_unavailable) &&
              plan.has_blocker(BnwFreshPlayerUninitBlockerCode::
                               service_uninit_effects_pending),
          "Undocumented archive reset and service callbacks remain required");
    auto wrong = objects;
    wrong.selected_rules_sha256 = "other";
    const auto rejected = plan_bnw_fresh_first_player_uninit(
        selected,wrong,sha);
    check(!rejected.preflight_complete &&
              rejected.planned_source_order.empty() &&
              rejected.has_blocker(BnwFreshPlayerUninitBlockerCode::
                                   first_player_or_rules_unverified),
          "Mismatched selected rules cannot stage reset operations");
    plan.rollback();
    check(plan.planned_source_order.empty() && !plan.preflight_complete &&
              plan.selected_rules_sha256.empty() && objects.constructors_complete,
          "Rollback cannot mutate the prior constructor draft");
}
} // namespace

int main() { test_first_player_uninit_frontier(); }
