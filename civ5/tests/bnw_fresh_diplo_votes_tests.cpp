#include "civ5/runtime/bnw_fresh_diplo_votes.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>

namespace {
void check(bool condition, const char *message) {
    if (!condition) throw std::runtime_error(message);
}

void test_first_alive_diplomatic_votes() {
    using namespace civ5::runtime;
    BnwFreshGameContentResult selected;
    selected.profile.kind = ContentProfileKind::dlc;
    selected.rules = std::make_unique<civ5::content::RulesDatabase>();
    selected.rules->apply_sql(R"sql(
        CREATE TABLE LeagueSpecialSessions(
            ID INTEGER PRIMARY KEY, UnitedNations INTEGER,
            HostDelegates INTEGER, CivDelegates INTEGER,
            CityStateDelegates INTEGER);
        INSERT INTO LeagueSpecialSessions VALUES(0,0,1,1,0);
        INSERT INTO LeagueSpecialSessions VALUES(1,1,2,4,2);
        INSERT INTO LeagueSpecialSessions VALUES(2,1,3,5,1);
        CREATE TABLE Defines(Name TEXT PRIMARY KEY, Value TEXT);
        INSERT INTO Defines VALUES('DIPLO_VICTORY_CIV_DELEGATES_COEFFICIENT','1.443');
        INSERT INTO Defines VALUES('DIPLO_VICTORY_CIV_DELEGATES_CONSTANT','7.000');
        INSERT INTO Defines VALUES('DIPLO_VICTORY_CS_DELEGATES_COEFFICIENT','16.023');
        INSERT INTO Defines VALUES('DIPLO_VICTORY_CS_DELEGATES_CONSTANT','-13.758');
    )sql");
    selected.activation.emplace();
    selected.activation->profile_kind = ContentProfileKind::dlc;
    selected.activation->rules_database_ready = true;
    auto sha = identify_bytes(selected.rules->gameplay_fingerprint_data()).sha256;
    auto draft = draft_bnw_fresh_zero_ever_alive_diplo_votes(selected, sha);
    check(draft.zero_ever_alive_votes_computed &&
              !draft.league_callback_complete &&
              draft.votes_needed == 3 &&
              draft.host_delegates == 3 &&
              draft.civ_delegates == 5 &&
              draft.city_state_delegates == 1 &&
              draft.visited_special_session_ids ==
                  std::vector<std::int32_t>({0,1,2}) &&
              draft.has_blocker(
                  BnwFreshDiploVotesBlockerCode::league_callback_and_ui_pending),
          "Last UN Info row supplies the first-alive zero-count vote cap");
    auto mismatch = draft_bnw_fresh_zero_ever_alive_diplo_votes(selected, "other");
    check(mismatch.has_blocker(
              BnwFreshDiploVotesBlockerCode::selected_rules_unverified) &&
              !mismatch.zero_ever_alive_votes_computed,
          "A different rule fingerprint cannot produce votes");

    selected.rules->apply_sql(R"sql(
        UPDATE Defines SET Value = '0'
        WHERE Name = 'DIPLO_VICTORY_CIV_DELEGATES_COEFFICIENT';
    )sql");
    sha = identify_bytes(selected.rules->gameplay_fingerprint_data()).sha256;
    auto undefined = draft_bnw_fresh_zero_ever_alive_diplo_votes(selected, sha);
    check(undefined.has_blocker(
              BnwFreshDiploVotesBlockerCode::diplomatic_defines_unverified) &&
              undefined.visited_special_session_ids.empty() &&
              !undefined.zero_ever_alive_votes_computed,
          "Zero coefficient at log(0) cannot publish an undefined SDK cast");

    draft.rollback();
    check(!draft.zero_ever_alive_votes_computed && draft.votes_needed == 0 &&
              draft.visited_special_session_ids.empty() &&
              !draft.league_callback_complete,
          "Detached vote computation can be discarded without state mutation");
}
} // namespace

int main() { test_first_alive_diplomatic_votes(); }
