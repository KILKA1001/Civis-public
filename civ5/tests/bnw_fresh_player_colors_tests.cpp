#include "civ5/runtime/bnw_fresh_player_colors.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_init_players_major_color_pass() {
    using namespace civ5::runtime;
    BnwFreshGameContentResult selected;
    selected.profile.kind = ContentProfileKind::dlc;
    selected.rules = std::make_unique<civ5::content::RulesDatabase>();
    selected.rules->apply_sql(R"sql(
        CREATE TABLE PlayerColors(ID INTEGER PRIMARY KEY, Type TEXT);
        INSERT INTO PlayerColors VALUES(0,'PLAYERCOLOR_BARBARIAN');
        INSERT INTO PlayerColors VALUES(1,'PLAYERCOLOR_BLUE');
        INSERT INTO PlayerColors VALUES(2,'PLAYERCOLOR_GREEN');
        INSERT INTO PlayerColors VALUES(3,'PLAYERCOLOR_RED');
        CREATE TABLE Civilizations(ID INTEGER PRIMARY KEY, DefaultPlayerColor TEXT);
        INSERT INTO Civilizations VALUES(10,'PLAYERCOLOR_BLUE');
        INSERT INTO Civilizations VALUES(11,'PLAYERCOLOR_BLUE');
        INSERT INTO Civilizations VALUES(99,'PLAYERCOLOR_BARBARIAN');
    )sql");
    selected.activation.emplace();
    selected.activation->profile_kind = ContentProfileKind::dlc;
    selected.activation->rules_database_ready = true;
    const auto sha = identify_bytes(
        selected.rules->gameplay_fingerprint_data()).sha256;
    const std::vector<BnwFreshMajorColorSlot> slots{
        {BnwFreshSlotStatus::taken,10,-1},
        {BnwFreshSlotStatus::computer,11,-1},
        {BnwFreshSlotStatus::closed,-1,-1}};
    auto colors = draft_bnw_fresh_player_colors(selected, sha, slots, 99);
    check(colors.major_colors_complete && colors.barbarian_color == 0 &&
              colors.major_colors == std::vector<std::int32_t>({1,3,-1}) &&
              colors.has_blocker(BnwFreshColorsBlockerCode::remaining_init_players_pending),
          "Duplicate major color takes last free nonbarbarian ID in source loop");
    const auto wrong = draft_bnw_fresh_player_colors(
        selected, "other-rules", slots, 99);
    check(wrong.has_blocker(BnwFreshColorsBlockerCode::selected_rules_unverified) &&
              wrong.major_colors.empty(),
          "Initial colors require the same selected rules as the map");
    auto missing = slots;
    missing[1].civilization_id = 100;
    const auto refused = draft_bnw_fresh_player_colors(
        selected, sha, missing, 99);
    check(refused.has_blocker(BnwFreshColorsBlockerCode::selected_color_info_missing) &&
              refused.major_colors.empty(),
          "Missing selected Civilization Info cannot publish partial colors");
    colors.rollback();
    check(!colors.major_colors_complete && colors.major_colors.empty() &&
              colors.barbarian_color == -1,
          "Detached major color result can be discarded atomically");
}
} // namespace

int main() { test_init_players_major_color_pass(); }
