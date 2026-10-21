#include "civ5/runtime/bnw_fresh_city_specialization_init.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_selected_city_specialization_reset() {
    using namespace civ5::runtime;
    BnwFreshGameContentResult selected;
    selected.profile.kind = ContentProfileKind::dlc;
    selected.rules = std::make_unique<civ5::content::RulesDatabase>();
    selected.rules->apply_sql(
        "CREATE TABLE CitySpecializations(ID INTEGER PRIMARY KEY, Type TEXT);"
        "INSERT INTO CitySpecializations VALUES(0,'SPECIALIZATION_0');"
        "INSERT INTO CitySpecializations VALUES(2,'SPECIALIZATION_2');");
    selected.activation.emplace();
    selected.activation->profile_kind = ContentProfileKind::dlc;
    selected.activation->rules_database_ready = true;
    const auto sha = identify_bytes(selected.rules->gameplay_fingerprint_data()).sha256;
    BnwFreshMilitaryAIInit military;
    military.selected_rules_sha256 = sha;
    military.arrays_and_reset_complete = true;
    military.players.resize(2);
    military.players[0].player_id = 0;
    military.players[1].player_id = 1;
    military.blockers.push_back({
        BnwFreshMilitaryInitBlockerCode::diplomacy_handle_and_remaining_ai_pending,
        "fixture"});
    auto draft = draft_bnw_fresh_city_specialization_init(
        selected,military,sha);
    check(draft.city_specialization_init_complete &&
              !draft.initialized_by_gamecore &&
              draft.selected_specialization_slots == 3 &&
              draft.players.size() == 2 && draft.source_order.size() == 10 &&
              draft.source_order[0] == BnwFreshCitySpecializationEvent{
                  BnwFreshCitySpecializationStep::military_ai_dependency_pending,0} &&
              draft.source_order[4] == BnwFreshCitySpecializationEvent{
                  BnwFreshCitySpecializationStep::wonder_production_ai_pending,0} &&
              draft.source_order[5] == BnwFreshCitySpecializationEvent{
                  BnwFreshCitySpecializationStep::military_ai_dependency_pending,1},
          "Selected Info slot count and per-player Init order persist");
    const auto &first = draft.players.front();
    check(!first.specializations_dirty && !first.interrupt_wonders &&
              first.next_specialization == -1 && first.next_wonder == -1 &&
              first.wonder_city_id == -1 && first.next_wonder_weight == 0 &&
              first.last_turn_evaluated == 0 &&
              first.yield_weights.empty() &&
              first.production_subtype_weights.empty() &&
              draft.has_blocker(
                  BnwFreshCitySpecializationBlockerCode::military_handle_and_wonder_ai_pending),
          "CvCitySpecializationAI Reset fields remain detached from GameCore");
    selected.rules->apply_sql("DROP TABLE CitySpecializations;");
    const auto changed_sha = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    military.selected_rules_sha256 = changed_sha;
    const auto missing = draft_bnw_fresh_city_specialization_init(
        selected,military,changed_sha);
    check(missing.has_blocker(
              BnwFreshCitySpecializationBlockerCode::selected_specialization_info_unverified) &&
              missing.players.empty(),
          "Missing selected CitySpecializations Info blocks every player");
    draft.rollback();
    check(draft.players.empty() && draft.source_order.empty() &&
              !draft.city_specialization_init_complete &&
              military.players.size() == 2,
          "Rollback does not alter MilitaryAI draft");
}
} // namespace

int main() { test_selected_city_specialization_reset(); }
