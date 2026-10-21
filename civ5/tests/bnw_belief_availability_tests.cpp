#include "civ5/runtime/bnw_belief_availability.hpp"

#include <iostream>
#include <stdexcept>

namespace {
void require(bool ok, const char *message) {
    if (!ok) throw std::runtime_error(message);
}
}

int main() {
    try {
        using namespace civ5;
        content::RulesDatabase rules;
        rules.apply_sql(R"(
            CREATE TABLE Beliefs(ID INTEGER PRIMARY KEY, Type TEXT,
                Pantheon INTEGER, Founder INTEGER, Follower INTEGER,
                Enhancer INTEGER, Reformation INTEGER);
            INSERT INTO Beliefs VALUES(0,'BELIEF_A',1,0,0,0,0);
            INSERT INTO Beliefs VALUES(1,'BELIEF_B',1,0,0,0,0);
            INSERT INTO Beliefs VALUES(2,'BELIEF_FOUNDER',0,1,0,0,0);
            INSERT INTO Beliefs VALUES(3,'BELIEF_C',1,0,0,0,0);
            INSERT INTO Beliefs VALUES(4,'BELIEF_FOLLOWER',0,0,1,0,0);
            INSERT INTO Beliefs VALUES(5,'BELIEF_ENHANCER',0,0,0,1,0);
            INSERT INTO Beliefs VALUES(6,'BELIEF_SHARED',0,1,1,0,0);
            INSERT INTO Beliefs VALUES(7,'BELIEF_REFORMATION',0,0,0,0,1);
        )");
        const auto types = rules.cache_info_types();
        saves::GameReligionCultureObservation roster;
        roster.layout = saves::GameStateLayout::expansion2_version1;
        roster.religions_version = 4;
        roster.culture_version = 2;
        roster.start_offset = 100;
        roster.culture_offset = 200;
        roster.end_offset = 250;
        saves::ReligionObservation first;
        first.version = 4;
        first.religion = 1;
        first.beliefs.version = 2;
        first.beliefs.belief_hashes = {
            content::saved_info_type_hash("BELIEF_B")};
        roster.religions.push_back(first);
        auto result = runtime::quote_bnw_available_pantheon_beliefs(
            rules, types, roster);
        require(result.resolved && result.belief_ids ==
                    std::vector<std::int64_t>({0, 3}),
                "SDK pantheon filter excludes all claimed beliefs and non-pantheon rows");
        saves::ReligionObservation second = first;
        second.religion = 2;
        second.beliefs.belief_hashes = {
            content::saved_info_type_hash("BELIEF_C"), 0};
        roster.religions.push_back(second);
        result = runtime::quote_bnw_available_pantheon_beliefs(
            rules, types, roster);
        require(result.resolved && result.belief_ids ==
                    std::vector<std::int64_t>({0}),
                "every current religion claims its resolved beliefs; zero hash claims none");
        const auto quote = [&](runtime::BnwBeliefCategory category) {
            return runtime::quote_bnw_available_beliefs(
                rules, types, roster, category);
        };
        require(quote(runtime::BnwBeliefCategory::founder).belief_ids ==
                    std::vector<std::int64_t>({2, 6}) &&
                quote(runtime::BnwBeliefCategory::follower).belief_ids ==
                    std::vector<std::int64_t>({4, 6}) &&
                quote(runtime::BnwBeliefCategory::enhancer).belief_ids ==
                    std::vector<std::int64_t>({5}) &&
                quote(runtime::BnwBeliefCategory::bonus).belief_ids ==
                    std::vector<std::int64_t>({0, 2, 4, 5, 6}) &&
                quote(runtime::BnwBeliefCategory::reformation).belief_ids ==
                    std::vector<std::int64_t>({7}),
                "SDK category flags and bonus union preserve GameCore slot order");
        saves::ReligionObservation third = first;
        third.religion = 3;
        third.beliefs.belief_hashes = {
            content::saved_info_type_hash("BELIEF_SHARED")};
        roster.religions.push_back(third);
        require(quote(runtime::BnwBeliefCategory::founder).belief_ids ==
                    std::vector<std::int64_t>({2}) &&
                quote(runtime::BnwBeliefCategory::follower).belief_ids ==
                    std::vector<std::int64_t>({4}) &&
                quote(runtime::BnwBeliefCategory::bonus).belief_ids ==
                    std::vector<std::int64_t>({0, 2, 4, 5}),
                "one claimed multi-category belief disappears from every SDK list");
        require(!quote(static_cast<runtime::BnwBeliefCategory>(99)).resolved,
                "unknown category cannot produce an availability claim");
        roster.religions_version = 1;
        require(!runtime::quote_bnw_available_pantheon_beliefs(
                    rules, types, roster).resolved,
                "old save without serialized current-religion roster blocks quote");
        roster.religions_version = 4;
        roster.religions[1].beliefs.belief_hashes = {0x98765432};
        require(!runtime::quote_bnw_available_pantheon_beliefs(
                    rules, types, roster).resolved,
                "unresolved saved belief hash blocks the complete availability list");
        roster.religions[1].beliefs.belief_hashes = {
            content::saved_info_type_hash("BELIEF_C")};
        roster.religions[1].beliefs.version.reset();
        require(!runtime::quote_bnw_available_pantheon_beliefs(
                    rules, types, roster).resolved,
                "incomplete belief serialization blocks the quote");
        roster.religions[1].beliefs.version = 2;
        rules.apply_sql("UPDATE Beliefs SET ID=8 WHERE Type='BELIEF_C';");
        require(!runtime::quote_bnw_available_pantheon_beliefs(
                    rules, types, roster).resolved,
                "rules changed after type-cache activation block a stale quote");
        std::cout << "bnw belief availability tests passed\n";
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
