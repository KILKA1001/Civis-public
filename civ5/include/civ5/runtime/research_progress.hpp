#pragma once
#include "civ5/runtime/player_research.hpp"

namespace civ5::runtime {
// Required world services, not optional notifications. In particular,
// acquire_technology must perform CvTeam::setHasTech(tech,true,player,true,true)
// including its world effects, callbacks, queue changes and repeat-tech resets.
// A low-level TeamTechnologies::set_has_tech call alone does not implement it.
// This interface has no default/no-op implementation. A world that has not
// implemented these services cannot advertise a playable research turn.
class ResearchProgressServices {
  public:
    virtual ~ResearchProgressServices() = default;
    virtual std::int64_t science_times100(std::int64_t player) = 0;
    virtual void game_data_and_score_dirty(std::int64_t team) = 0;
    virtual void acquire_technology(std::int64_t team, std::int64_t tech, std::int64_t player) = 0;
    virtual void city_specializations_dirty(std::int64_t player) = 0;
    virtual std::int64_t culture_per_tech(std::int64_t player) = 0;
    virtual void add_culture(std::int64_t player, std::int64_t amount) = 0;
};
struct ResearchProgressContext {
    PlayerResearchContext &players;
    const ResearchContext &setup;
    const content::ResearchRules &rules;
    const content::TechnologyRules &technologies;
    const content::GlobalDefineCache &defines;
    const content::CustomModOptions *options;
    std::span<TeamTechnologies> teams;
    ResearchProgressServices &world;
    std::int64_t active_team = -1;
    bool no_science = false;
};
struct ResearchProgressChange {
    bool changed = false, completed = false, skipped = false;
    std::int64_t previous = 0, assigned = 0, threshold = 0, overflow_granted = 0;
};
// These execute the source Set/ChangeResearchProgress family. They do not
// calculate a full player's turn, choose research or reset repeat technologies
// themselves. Callbacks observe source order; if one throws, prior mutations
// remain and callers must not blindly retry the same request.
ResearchProgressChange set_research_progress_times100(ResearchProgressContext &context,
    std::int64_t team, std::int64_t tech, std::int64_t value, std::int64_t player,
    std::int64_t player_overflow = 0, std::int64_t overflow_divisor_times100 = 100);
ResearchProgressChange change_research_progress_times100(ResearchProgressContext &context,
    std::int64_t team, std::int64_t tech, std::int64_t change, std::int64_t player,
    std::int64_t player_overflow = 0, std::int64_t overflow_divisor_times100 = 100);
ResearchProgressChange set_research_progress(ResearchProgressContext &context,
    std::int64_t team, std::int64_t tech, std::int64_t value, std::int64_t player);
ResearchProgressChange change_research_progress(ResearchProgressContext &context,
    std::int64_t team, std::int64_t tech, std::int64_t change, std::int64_t player);
// Source percent grants use TEAM cost/remaining, not player cost. VP returns
// whole beakers even when a fractional grant changes its stored hundredths.
std::int64_t change_research_progress_percent(ResearchProgressContext &context,
    std::int64_t team, std::int64_t tech, std::int64_t percent, std::int64_t player);
std::int64_t max_research_overflow(ResearchProgressContext &context,
    std::int64_t tech, std::int64_t player);
void set_player_research_overflow_times100(ResearchProgressContext &context,
    std::int64_t player, std::int64_t value);
void change_player_research_overflow_times100(ResearchProgressContext &context,
    std::int64_t player, std::int64_t change);
} // namespace civ5::runtime
