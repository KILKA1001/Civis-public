#pragma once
#include "civ5/runtime/bnw_fresh_archive_full.hpp"
#include "civ5/runtime/bnw_fresh_city_specialization_init.hpp"
#include <memory>
#include <optional>
#include <cstddef>
#include <span>
#include <string_view>
#include <utility>

namespace civ5::runtime {
class BnwNewGamePlayerSetup;
class BnwFreshTacticalMap;
struct BnwFreshGameBuilderInputs;
struct BnwFreshGameBuilderResult;
// Owned single first-player gameplay prefix, issued only by the real builder.
// Existing builder stops after Tactical Reset. Its closed SAME Game child can
// complete actual Tactical Init; Homeland/Player/world Init remain mandatory.
class BnwFreshFirstPlayerState final {
public:
    ~BnwFreshFirstPlayerState();
    BnwFreshFirstPlayerState(const BnwFreshFirstPlayerState&)=delete;
    BnwFreshFirstPlayerState& operator=(const BnwFreshFirstPlayerState&)=delete;
    const BnwArchiveMemberState& registered_member(std::string_view) const;
    const BnwArchiveValue& unregistered_scalar(std::string_view) const;
    const BnwFreshEconomicAIState& economic() const;
    const BnwFreshMilitaryAIState& military() const;
    const BnwFreshCitySpecializationState& city_specialization() const;
    std::span<const std::int32_t> wonder_latest_flavors() const;
    std::span<const std::pair<std::int32_t,std::int32_t>> wonder_weights() const;
    std::span<const std::pair<std::int32_t,std::int32_t>> wonder_buildables() const;
    bool wonder_is_city() const;
    std::span<const std::int32_t> grand_strategy_priorities() const;
    std::span<const std::int32_t> grand_strategy_guesses() const;
    std::span<const std::int32_t> grand_strategy_guess_confidence() const;
    std::int32_t grand_strategy_turns_since_active_set() const;
    std::int32_t grand_strategy_active() const;
    // Immutable native Init values. Empty cells retain source uninitialized or
    // NULL row state; this read does not grant diplomacy DoTurn/personality.
    std::optional<std::int32_t> diplomacy_array_value(std::string_view,std::size_t,std::size_t) const;
    std::int32_t diplomacy_scalar(std::string_view) const;
    std::uint64_t diplomacy_reset_write_count() const;
    std::int32_t religions_prophets_spawned() const;
    std::int32_t religions_founding_religion() const;
    // Bound selected collection span only, not future Belief Entry authority.
    std::int32_t religion_ai_belief_slots() const;
    std::span<const std::int32_t> tech_latest_flavors() const;
    std::span<const std::pair<std::int32_t,std::int32_t>> tech_weights() const;
    bool tech_researching(std::size_t) const;
    std::int32_t tech_reset_value(std::string_view,std::size_t) const;
    bool tech_pregame_minor() const;
    std::int32_t tech_pregame_civilization() const;
    std::int32_t tech_pregame_leader() const;
    // Read-only reached native fields; no Game pointer or AI decision authority.
    std::int32_t tactical_reset_value(std::string_view) const;
    bool tactical_reset_map_is_null() const;
    bool tactical_init_map_is_bound() const;
    std::int32_t tactical_init_integer(std::string_view)const;
    float tactical_init_flavor_dampening()const;
    std::span<const std::pair<std::string_view,std::int32_t>> tactical_init_cached_types()const;
    std::span<const std::int32_t> policy_latest_flavors() const;
    std::span<const std::pair<std::int32_t,std::int32_t>> policy_weights() const;
    bool policy_flag(std::string_view,std::size_t) const;
    std::int32_t policy_reset_value(std::string_view,std::size_t) const;
    std::int32_t policy_scalar(std::string_view) const;
    std::span<const std::int32_t> great_work_yield_change() const;
    std::span<const std::int32_t> siphon_luxury_count() const;
    std::string_view embarked_graphic_override() const;
    std::span<const std::string_view> source_order() const;
    std::string_view selected_rules_sha256() const;
private:
    friend class BnwNewGamePlayerSetup;
    // Only the authentic SAME Game child can supply these actual source reads.
    // Raw borrowed map identity is safe because that Game owns both children.
    void complete_tactical_init_from_game(const BnwFreshTacticalMap&,
        const content::RulesDatabase&,std::int32_t actual_game_handicap)const;
    struct Impl;
    std::unique_ptr<Impl> impl_;
    explicit BnwFreshFirstPlayerState(std::unique_ptr<Impl>);
    static std::shared_ptr<const BnwFreshFirstPlayerState> bind(
        const BnwFreshGameBuilderResult&,const BnwFreshGameContentResult&,const BnwFreshGameBuilderInputs&);
    friend BnwFreshGameBuilderResult draft_bnw_fresh_gamecore(
        const BnwFreshGameContentResult&,const BnwFreshGameBuilderInputs&);
};
} // namespace civ5::runtime
