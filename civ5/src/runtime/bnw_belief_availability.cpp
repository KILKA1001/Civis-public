#include "civ5/runtime/bnw_belief_availability.hpp"

#include <limits>
#include <set>

namespace civ5::runtime {
namespace {

BnwBeliefAvailability blocked(std::string reason) {
    return {false, {}, std::move(reason)};
}

bool matches_category(const content::RulesRowView &row,
                      BnwBeliefCategory category) {
    const auto enabled = [&](std::string_view column) {
        return row.integer(column) != 0;
    };
    switch (category) {
    case BnwBeliefCategory::pantheon: return enabled("Pantheon");
    case BnwBeliefCategory::founder: return enabled("Founder");
    case BnwBeliefCategory::follower: return enabled("Follower");
    case BnwBeliefCategory::enhancer: return enabled("Enhancer");
    case BnwBeliefCategory::reformation: return enabled("Reformation");
    case BnwBeliefCategory::bonus:
        return enabled("Pantheon") || enabled("Founder") ||
            enabled("Follower") || enabled("Enhancer");
    }
    return false;
}

bool has_category_columns(const content::RulesRowView &row,
                          BnwBeliefCategory category) {
    switch (category) {
    case BnwBeliefCategory::pantheon: return row.has_column("Pantheon");
    case BnwBeliefCategory::founder: return row.has_column("Founder");
    case BnwBeliefCategory::follower: return row.has_column("Follower");
    case BnwBeliefCategory::enhancer: return row.has_column("Enhancer");
    case BnwBeliefCategory::reformation: return row.has_column("Reformation");
    case BnwBeliefCategory::bonus:
        return row.has_column("Pantheon") && row.has_column("Founder") &&
            row.has_column("Follower") && row.has_column("Enhancer");
    }
    return false;
}

} // namespace

BnwBeliefAvailability quote_bnw_available_beliefs(
    const content::RulesDatabase &rules,
    const content::InfoTypeCatalog &types,
    const saves::GameReligionCultureObservation &religions,
    BnwBeliefCategory category) {
    // Version 1 has no serialized m_CurrentReligions list. A decoded version
    // 2+ list is complete because the source reader consumes its declared
    // count or fails the import.
    if (religions.layout != saves::GameStateLayout::expansion2_version1 ||
        !religions.religions_version || *religions.religions_version < 2 ||
        !religions.culture_version ||
        religions.culture_offset <= religions.start_offset ||
        religions.end_offset < religions.culture_offset ||
        !types.report().all_succeeded())
        return blocked("Complete BNW religion roster or selected type catalog is unavailable");
    std::set<std::int64_t> claimed;
    for (const auto &religion : religions.religions) {
        if (!religion.version || !religion.beliefs.version)
            return blocked("BNW religion or belief serialization is incomplete");
        for (const auto hash : religion.beliefs.belief_hashes) {
            // Native ReadHashed maps zero to NO_BELIEF (-1); it cannot claim
            // any real Beliefs entry.
            if (!hash) continue;
            const auto *entry = types.find_hash(hash);
            if (!entry || entry->table != "Beliefs" || entry->id < 0)
                return blocked("Saved BNW belief hash is unresolved in selected rules");
            claimed.insert(entry->id);
        }
    }
    std::vector<std::int64_t> available;
    std::string malformed;
    const auto report = rules.prefetch_info("Beliefs",
        [&](std::uint64_t slot, std::int64_t authored_id,
            const content::RulesRowView &row) {
            // GameCore's loop indexes the prefetched collection by slot;
            // overlapping authored IDs or a divergent hash map could give a
            // different answer, so this read-only quote closes on either.
            const auto type = row.text("Type");
            const auto *entry = type ? types.find(*type) : nullptr;
            if (slot > static_cast<std::uint64_t>(
                    std::numeric_limits<std::int64_t>::max()) ||
                authored_id != static_cast<std::int64_t>(slot) ||
                !has_category_columns(row, category) || !entry ||
                entry->table != "Beliefs" || entry->id != authored_id) {
                malformed = "Selected Beliefs slots or type IDs differ from GameCore";
                return false;
            }
            if (matches_category(row, category) &&
                !claimed.contains(authored_id))
                available.push_back(authored_id);
            return true;
        });
    if (!report.all_succeeded() || !malformed.empty() ||
        report.overlapping_ids != 0)
        return blocked(!malformed.empty() ? malformed :
            "Selected BNW Beliefs collection is incomplete");
    return {true, std::move(available), {}};
}

BnwPantheonBeliefAvailability quote_bnw_available_pantheon_beliefs(
    const content::RulesDatabase &rules,
    const content::InfoTypeCatalog &types,
    const saves::GameReligionCultureObservation &religions) {
    return quote_bnw_available_beliefs(rules, types, religions,
        BnwBeliefCategory::pantheon);
}

} // namespace civ5::runtime
