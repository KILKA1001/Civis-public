#pragma once
#include "civ5/content/technology_rules.hpp"
#include "civ5/runtime/profile_extensions.hpp"
#include "civ5/saves/team_state.hpp"
#include <functional>
#include <vector>

namespace civ5::runtime {
struct TeamTechnologyValue {
    bool has_tech = false, no_trade = false;
    std::int64_t progress_times100 = 0, count = 0;
    bool operator==(const TeamTechnologyValue &) const = default;
};
struct TeamTechnologyRemap {
    std::uint64_t saved = 0, mapped = 0, sentinels = 0, unknown = 0, outside = 0;
    std::uint64_t duplicates = 0, holes = 0;
    bool last_tech_outside = false;
    bool all_mapped() const { return unknown == 0 && outside == 0 && !last_tech_outside; }
};
class TeamTechnologies;
TeamTechnologies restore_team_technologies(const saves::TeamObservation &saved,
    const content::TechnologyRules &rules, const content::InfoTypeCatalog &types,
    std::uint64_t row_budget = 16 * 1024 * 1024);

// Owned CvTeamTechs state, independent of the save buffer and SQLite lifetime.
// Slots are logical; only nondefault values occupy memory. Both profiles store
// research in hundredths, despite BNW's serialized field name.
class TeamTechnologies {
  public:
    TeamTechnologies(content::TechnologyProfile profile, std::int64_t team, std::uint64_t slots);
    content::TechnologyProfile profile() const { return profile_; }
    std::int64_t team() const { return team_; }
    std::uint64_t slots() const { return slots_; }
    std::uint64_t occupied_slots() const;
    // Includes profile-owned values without exposing their storage in the
    // shared technology row. Useful for diagnostics and future persistence.
    std::vector<std::uint64_t> occupied_indices() const;
    const std::map<std::uint64_t, TeamTechnologyValue> &nondefault_values() const { return values_; }
    ProfileExtensions &profile_extensions() { return extensions_; }
    const ProfileExtensions &profile_extensions() const { return extensions_; }
    const TeamTechnologyRemap &remap() const { return remap_; }
    std::int64_t last_tech() const { return last_; }
    TeamTechnologyValue at(std::int64_t tech) const;
    bool has_tech(std::int64_t tech) const; // NO_TECH (-1) is true.
    std::int64_t progress_times100(std::int64_t tech) const; // NO_TECH -> 0.
    std::int64_t progress(std::int64_t tech) const;
    std::int64_t known_count() const;
    bool all_researched() const;

    // These are the low-level CvTeamTechs operations, NOT CvTeam::setHasTech
    // or research completion. The caller must apply those wider game effects.
    // Hook runs after a changed flag and last-tech update; exceptions propagate
    // with the mutation retained. Import never invokes hooks.
    using HasTechHook = std::function<void(std::int64_t, std::int64_t, bool)>;
    bool set_has_tech(std::int64_t tech, bool value, const HasTechHook &hook = {});
    void set_last_tech(std::int64_t tech);
    void set_no_trade(std::int64_t tech, bool value);
    // Raw owned-state write, also needed by repeat-tech reset paths. Gameplay
    // callers use research_progress.hpp to run cost/completion side effects.
    void set_progress_times100(std::int64_t tech, std::int64_t value);
    void increment_count(std::int64_t tech);
    // Raw checkpoint import, with no acquisition hook or turn side effects.
    // Replaces only shared fields; profile state remains in extensions_.
    void replace_live_values(std::int64_t last,std::map<std::uint64_t,TeamTechnologyValue> values);

  private:
    friend TeamTechnologies restore_team_technologies(const saves::TeamObservation &,
        const content::TechnologyRules &, const content::InfoTypeCatalog &, std::uint64_t);
    std::uint64_t index(std::int64_t tech) const;
    void store(std::uint64_t slot, TeamTechnologyValue value);
    content::TechnologyProfile profile_;
    std::int64_t team_;
    std::uint64_t slots_;
    std::int64_t last_ = -1;
    std::map<std::uint64_t, TeamTechnologyValue> values_;
    ProfileExtensions extensions_;
    TeamTechnologyRemap remap_;
};
std::vector<TeamTechnologies> restore_team_technologies(const saves::TeamsObservation &saved,
    const content::TechnologyRules &rules, const content::InfoTypeCatalog &types,
    std::uint64_t row_budget = 16 * 1024 * 1024);
} // namespace civ5::runtime
