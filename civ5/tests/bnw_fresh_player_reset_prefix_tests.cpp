#include "civ5/runtime/bnw_fresh_player_reset_prefix.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_selected_early_player_reset() {
    using namespace civ5::runtime;
    BnwFreshGameContentResult selected;
    selected.profile.kind = ContentProfileKind::dlc;
    selected.rules = std::make_unique<civ5::content::RulesDatabase>();
    selected.rules->apply_sql(
        "CREATE TABLE Leaders(ID INTEGER PRIMARY KEY, Type TEXT);"
        "INSERT INTO Leaders VALUES(0,'LEADER_ZERO');"
        "INSERT INTO Leaders VALUES(2,'LEADER_TWO');");
    selected.activation.emplace();
    selected.activation->profile_kind = ContentProfileKind::dlc;
    selected.activation->rules_database_ready = true;
    const auto sha = identify_bytes(selected.rules->gameplay_fingerprint_data()).sha256;
    BnwFreshPregameRosterDraft roster;
    roster.selected_rules_sha256 = sha;
    roster.slot_prefix_complete = true;
    roster.slots.resize(3);
    const std::vector<civ5::content::GameplayDefine> constants{
        {"MAX_PLAYERS",3},{"MAX_MAJOR_CIVS",1},{"NUM_YIELD_TYPES",6}};
    const std::vector<std::int32_t> leaders{2,-1,0};
    auto draft = draft_bnw_fresh_player_early_reset(
        selected,roster,sha,constants,leaders);
    check(draft.early_arrays_complete && !draft.initialized_by_gamecore &&
              draft.players.size() == 3 && draft.source_order.size() == 25 &&
              draft.source_order.front() == BnwFreshPlayerResetEvent{
                  BnwFreshPlayerResetStep::major_notifications_pending,0} &&
              draft.source_order[8] == BnwFreshPlayerResetEvent{
                  BnwFreshPlayerResetStep::resource_info_arrays_pending,0} &&
              draft.source_order[9] == BnwFreshPlayerResetEvent{
                  BnwFreshPlayerResetStep::sync_archive_reset,1},
          "Player resets remain interleaved and major notifications precede reset");
    const auto &first = draft.players.front();
    check(first.id == 0 && first.personality == 2 &&
              first.yield_vectors.size() == 10 &&
              std::all_of(first.yield_vectors.begin(),first.yield_vectors.end(),
                          [](const auto &values) {
                              return values == std::vector<std::int32_t>(6,0);
                          }) &&
              first.proximity_to_player == std::vector<std::int32_t>({0,0,0}) &&
              first.incoming_unit_types ==
                  std::vector<std::int32_t>({-1,-1,-1}) &&
              first.incoming_unit_countdowns ==
                  std::vector<std::int32_t>({-1,-1,-1}) &&
              draft.players[1].personality == -1 &&
              draft.has_blocker(
                  BnwFreshPlayerResetBlockerCode::notifications_and_uninit_pending) &&
              draft.has_blocker(
                  BnwFreshPlayerResetBlockerCode::resource_info_and_remaining_reset_pending),
          "Selected personality and compiled fixed arrays are owned but partial");
    const std::vector<std::int32_t> missing_leader{1,-1,0};
    const auto refused = draft_bnw_fresh_player_early_reset(
        selected,roster,sha,constants,missing_leader);
    check(refused.has_blocker(
              BnwFreshPlayerResetBlockerCode::pregame_leader_info_unverified) &&
              refused.players.empty(),
          "A sparse selected Leaders hole is not a valid personality");
    draft.rollback();
    check(draft.players.empty() && draft.source_order.empty() &&
              !draft.early_arrays_complete && leaders.front() == 2,
          "Rollback discards detached player arrays without changing pregame IDs");
}
} // namespace

int main() { test_selected_early_player_reset(); }
