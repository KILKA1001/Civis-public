#include "civ5/runtime/bnw_fresh_player_objects.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>

namespace {
void check(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}
void test_first_major_constructors() {
    using namespace civ5::runtime;
    BnwFreshGameContentResult selected;
    selected.profile.kind = ContentProfileKind::dlc;
    selected.rules = std::make_unique<civ5::content::RulesDatabase>();
    selected.activation.emplace();
    selected.activation->profile_kind = ContentProfileKind::dlc;
    selected.activation->rules_database_ready = true;
    const auto sha = identify_bytes(selected.rules->gameplay_fingerprint_data()).sha256;
    BnwFreshSlotInfoDraft slots;
    slots.selected_rules_sha256 = sha;
    slots.pregame_writes_complete = true;
    slots.slots.resize(3);
    slots.blockers.push_back({BnwFreshSlotInfoBlockerCode::player_init_pending,
                              "fixture"});
    const std::vector<civ5::content::GameplayDefine> constants{
        {"MAX_MAJOR_CIVS",1},{"MAX_PLAYERS",3}};
    auto draft = draft_bnw_fresh_first_player_objects(
        selected,slots,sha,constants);
    check(draft.constructors_complete && !draft.initialized_by_gamecore &&
              draft.player_id == 0 && draft.source_order.size() == 5 &&
              draft.source_order[0] == BnwFreshFirstPlayerObjectEvent{
                  BnwFreshFirstPlayerObjectStep::notifications_allocated,0} &&
              draft.source_order[1] == BnwFreshFirstPlayerObjectEvent{
                  BnwFreshFirstPlayerObjectStep::notifications_constructor_uninit,0} &&
              draft.source_order[2] == BnwFreshFirstPlayerObjectEvent{
                  BnwFreshFirstPlayerObjectStep::diplomacy_requests_allocated,0} &&
              draft.source_order[3] == BnwFreshFirstPlayerObjectEvent{
                  BnwFreshFirstPlayerObjectStep::diplomacy_requests_constructor_uninit,0} &&
              draft.source_order[4] == BnwFreshFirstPlayerObjectEvent{
                  BnwFreshFirstPlayerObjectStep::reset_sync_archive_pending,0},
          "CvPlayer::init constructs major objects before reset in SDK order");
    check(draft.notifications.player == -1 &&
              draft.notifications.current_lookup_index == 0 &&
              draft.notifications.begin_index == -1 &&
              draft.notifications.end_index == -1 &&
              draft.notifications.entries_empty &&
              draft.diplomacy_requests.player == -1 &&
              draft.diplomacy_requests.next_ai_player == -1 &&
              draft.diplomacy_requests.active_from_player == -1 &&
              !draft.diplomacy_requests.request_active &&
              draft.diplomacy_requests.entries_empty &&
              draft.has_blocker(BnwFreshFirstPlayerObjectBlockerCode::
                                reset_sync_archive_and_uninit_pending),
          "Constructor Uninit fields remain distinct from later Init fields");
    slots.selected_rules_sha256 = "different";
    const auto wrong = draft_bnw_fresh_first_player_objects(
        selected,slots,sha,constants);
    check(!wrong.constructors_complete && wrong.source_order.empty() &&
              wrong.has_blocker(BnwFreshFirstPlayerObjectBlockerCode::
                                selected_pregame_or_constants_unverified),
          "Rules mismatch cannot stage either object constructor");
    draft.rollback();
    check(!draft.constructors_complete && draft.source_order.empty() &&
              draft.selected_rules_sha256.empty() && slots.slots.size() == 3,
          "Rollback affects only owned first-player constructor state");
}
} // namespace

int main() { test_first_major_constructors(); }
