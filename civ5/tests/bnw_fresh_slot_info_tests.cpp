#include "civ5/runtime/bnw_fresh_slot_info.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>

namespace {
void check(bool value, const char *message) {
    if (!value) throw std::runtime_error(message);
}

void test_selected_slot_writes() {
    using namespace civ5::runtime;
    BnwFreshGameContentResult selected;
    selected.profile.kind = ContentProfileKind::dlc;
    selected.rules = std::make_unique<civ5::content::RulesDatabase>();
    selected.rules->apply_sql(R"sql(
        CREATE TABLE Civilizations(ID INTEGER PRIMARY KEY, DefaultPlayerColor TEXT);
        INSERT INTO Civilizations VALUES(10,'PLAYERCOLOR_BLUE');
        INSERT INTO Civilizations VALUES(11,'PLAYERCOLOR_BLUE');
        INSERT INTO Civilizations VALUES(99,'PLAYERCOLOR_BARBARIAN');
        CREATE TABLE Leaders(ID INTEGER PRIMARY KEY, Type TEXT);
        INSERT INTO Leaders VALUES(0,'LEADER_BARBARIAN');
        CREATE TABLE HandicapInfos(ID INTEGER PRIMARY KEY, Type TEXT);
        INSERT INTO HandicapInfos VALUES(0,'HANDICAP_0');
        INSERT INTO HandicapInfos VALUES(1,'HANDICAP_1');
        INSERT INTO HandicapInfos VALUES(2,'HANDICAP_2');
        CREATE TABLE PlayerColors(ID INTEGER PRIMARY KEY, Type TEXT);
        INSERT INTO PlayerColors VALUES(0,'PLAYERCOLOR_BARBARIAN');
        INSERT INTO PlayerColors VALUES(1,'PLAYERCOLOR_BLUE');
        INSERT INTO PlayerColors VALUES(2,'PLAYERCOLOR_MINOR');
        CREATE TABLE MinorCivilizations(ID INTEGER PRIMARY KEY, DefaultPlayerColor TEXT);
        INSERT INTO MinorCivilizations VALUES(5,'PLAYERCOLOR_MINOR');
    )sql");
    selected.activation.emplace();
    selected.activation->profile_kind = ContentProfileKind::dlc;
    selected.activation->rules_database_ready = true;
    const auto sha = identify_bytes(selected.rules->gameplay_fingerprint_data()).sha256;
    const std::vector<civ5::content::GameplayDefine> constants{
        {"MAX_TEAMS",4}, {"MAX_PLAYERS",4}, {"MAX_CIV_PLAYERS",3},
        {"MAX_MAJOR_CIVS",2}, {"BARBARIAN_PLAYER",3},
        {"BARBARIAN_TEAM",3}, {"BARBARIAN_CIVILIZATION",99},
        {"BARBARIAN_HANDICAP",0}, {"BARBARIAN_LEADER",0},
        {"MINOR_CIVILIZATION",11}, {"MINOR_CIV_HANDICAP",1},
        {"AI_HANDICAP",2}};
    const std::vector<BnwFreshPregameSlot> pregame{
        {BnwFreshSlotStatus::taken,0},
        {BnwFreshSlotStatus::computer,1},
        {BnwFreshSlotStatus::closed,2},
        {BnwFreshSlotStatus::closed,3}};
    const auto roster = draft_bnw_fresh_pregame_roster(
        selected,sha,constants,pregame,1);
    const std::vector<BnwFreshMajorColorSlot> initial_colors{
        {BnwFreshSlotStatus::taken,10,-1},
        {BnwFreshSlotStatus::computer,11,-1}};
    const auto colors = draft_bnw_fresh_player_colors(
        selected,sha,initial_colors,99);
    const std::vector<std::int32_t> minor_ids{5};
    auto draft = draft_bnw_fresh_slot_info(
        selected,roster,colors,sha,constants,minor_ids);
    check(draft.pregame_writes_complete && !draft.initialized_by_gamecore &&
              draft.slots.size() == 4 && draft.source_order.size() == 18 &&
              draft.source_order[0] == BnwFreshSlotWriteEvent{
                  0,BnwFreshSlotWriteField::player_color,1} &&
              draft.source_order[1] == BnwFreshSlotWriteEvent{
                  1,BnwFreshSlotWriteField::player_color,2} &&
              draft.source_order[2] == BnwFreshSlotWriteEvent{
                  1,BnwFreshSlotWriteField::handicap,2} &&
              draft.source_order[3] == BnwFreshSlotWriteEvent{
                  2,BnwFreshSlotWriteField::status,
                  static_cast<std::int32_t>(BnwFreshSlotStatus::computer)} &&
              draft.source_order[10] == BnwFreshSlotWriteEvent{
                  3,BnwFreshSlotWriteField::team,3} &&
              draft.source_order.back() == BnwFreshSlotWriteEvent{
                  3,BnwFreshSlotWriteField::minor_civ,0},
          "Major, active minor and barbarian setters retain SDK player order");
    check(draft.slots[2].civilization == 11 &&
              draft.slots[2].player_color == 2 &&
              draft.slots[2].minor_civ == true &&
              draft.slots[3].civilization == 99 &&
              draft.slots[3].leader == 0 &&
              draft.has_blocker(BnwFreshSlotInfoBlockerCode::player_init_pending),
          "Selected Info resolves minor and barbarian fields without player init");
    const auto missing = draft_bnw_fresh_slot_info(
        selected,roster,colors,sha,constants,{});
    check(missing.has_blocker(
              BnwFreshSlotInfoBlockerCode::minor_pregame_types_missing) &&
              missing.slots.empty() && missing.source_order.empty(),
          "Missing caller-selected minor type cannot publish partial writes");
    selected.rules->apply_sql("DROP TABLE MinorCivilizations;");
    const auto changed_sha = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    auto changed_roster = roster;
    auto changed_colors = colors;
    changed_roster.selected_rules_sha256 = changed_sha;
    changed_colors.selected_rules_sha256 = changed_sha;
    const auto missing_info = draft_bnw_fresh_slot_info(
        selected,changed_roster,changed_colors,changed_sha,constants,minor_ids);
    check(missing_info.has_blocker(
              BnwFreshSlotInfoBlockerCode::selected_info_unverified) &&
              missing_info.slots.empty(),
          "Missing selected minor Info cannot create partial pregame state");
    draft.rollback();
    check(draft.slots.empty() && draft.source_order.empty() &&
              !draft.pregame_writes_complete && pregame[2].status ==
                  BnwFreshSlotStatus::closed,
          "Rollback retains caller-owned pregame slots");
}
} // namespace

int main() { test_selected_slot_writes(); }
