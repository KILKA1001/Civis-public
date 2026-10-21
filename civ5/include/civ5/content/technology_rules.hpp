#pragma once
#include "civ5/content/info_metadata.hpp"
#include "civ5/content/rules_database.hpp"

namespace civ5::content {
enum class TechnologyProfile { bnw, vp542 };
enum class TechnologyFieldKind { integer, boolean };
struct TechnologyField { std::string_view column; TechnologyFieldKind kind; };
std::span<const TechnologyField> technology_fields(TechnologyProfile profile);

// Source default-filled arrays represented without allocation through MAX(ID).
struct RuleValues {
    std::uint64_t extent = 0;
    std::int64_t default_value = 0;
    std::map<std::uint64_t, std::int64_t> values;
    std::int64_t at(std::uint64_t index) const;
};
struct RuleMatrix {
    std::uint64_t rows = 0, columns = 0;
    std::map<std::pair<std::uint64_t, std::uint64_t>, std::int64_t> values;
    std::int64_t at(std::uint64_t row, std::uint64_t column) const;
};
struct TechnologyInfo {
    InfoMetadata base;
    std::map<std::string, std::int64_t, std::less<>> integers;
    std::map<std::string, bool, std::less<>> booleans;
    std::int64_t era = -1, first_free_unit_class = -1;
    InfoText quote;
    std::string audio_intro, audio_intro_header;
    RuleValues domain_extra_moves, trade_route_domain_extra_range, free_promotions, flavors;
    RuleValues or_prerequisites, and_prerequisites;
    std::optional<RuleMatrix> specialist_yield_changes;
};
struct TechnologyLoadError {
    std::uint64_t slot = 0;
    std::string stage, message;
    int sqlite_code = 0;
};
struct TechnologyBudget {
    std::uint64_t technologies = 1024 * 1024, relation_rows = 16 * 1024 * 1024, error_samples = 16;
};
struct TechnologyRules {
    TechnologyProfile profile = TechnologyProfile::bnw;
    InfoCollectionReport collection;
    std::map<std::uint64_t, TechnologyInfo> entries;
    std::uint64_t relation_rows = 0, errors = 0;
    bool localization_supplied = false;
    std::vector<TechnologyLoadError> error_samples;
    bool all_succeeded() const { return collection.all_succeeded() && errors == 0; }
    const TechnologyInfo *find(std::uint64_t slot) const;
};
// Uses the same post-remap rules/type cache as the imported session. Domain
// minimum is a caller-selected source enum size, never a compiled host ceiling.
// SQL failures retain partial objects and report errors. Original out-of-array
// writes become diagnosed, retained sparse entries; no memory corruption.
TechnologyRules load_technology_rules(const RulesDatabase &rules, const InfoTypeCatalog &types,
                                      TechnologyProfile profile, std::uint64_t minimum_domains,
                                      const InfoLocalizer &localizer = {}, const TechnologyBudget &budget = {});
} // namespace civ5::content
