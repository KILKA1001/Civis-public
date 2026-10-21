#pragma once
#include "civ5/runtime/bnw_creation_config.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <stdexcept>

namespace civ5::runtime {
class BnwFreshGame;
enum class BnwFreshTacticalMapError {
    wrong_thread, parent_unavailable, parent_retired, native_count_unavailable,
    native_define_unavailable, poisoned, cell_unavailable
};
class BnwFreshTacticalMapRefusal : public std::runtime_error {
 public:
    BnwFreshTacticalMapRefusal(BnwFreshTacticalMapError, const std::string&);
    BnwFreshTacticalMapError code() const noexcept { return code_; }
 private: BnwFreshTacticalMapError code_;
};

// Copies of actual stored fields. None of these diagnostics issues a Game,
// plot source, Init, player, TacticalAI or source-order continuation.
struct BnwTacticalCellSnapshot {
    std::uint32_t flags=0;
    // Enemy military/civilian, neutral military/civilian, friendly military/civilian.
    std::array<bool,6> unit_pointer_null{true,true,true,true,true,true};
    std::int32_t defense_modifier=0,deployment_score=0,target_type=0,dominance_zone=-1;
    bool operator==(const BnwTacticalCellSnapshot&) const = default;
};
struct BnwTacticalTemporaryZoneSnapshot {
    std::int32_t id=-1,territory_type=0,dominance_flag=0,owner=-1,city_id=-1,area_id=0;
    std::int32_t friendly_strength=0,enemy_strength=0,friendly_ranged_strength=0,enemy_ranged_strength=0;
    std::int32_t friendly_units=0,enemy_units=0,friendly_ranged_units=0,enemy_ranged_units=0,enemy_naval_units=0;
    std::int32_t value=0,closest_enemy_range=2147483647;
    bool water=false,naval_invasion=false,center_pointer_null=true;
    bool operator==(const BnwTacticalTemporaryZoneSnapshot&) const = default;
};
struct BnwTacticalMapSnapshot {
    bool plots_allocated=false,player_pointer_null=true,is_built=false,at_war=false,ignore_los=false;
    std::int32_t dominance_percentage=25,unit_strength_multiplier=5,tactical_range=8,
                 best_friendly_range=0,num_plots=0,turn_built=-1;
    std::uint32_t dominance_zones=0,enemy_units=0;
    BnwTacticalTemporaryZoneSnapshot temporary_zone;
    bool operator==(const BnwTacticalMapSnapshot&) const = default;
};

namespace detail {
// Shared executable storage leaf. Constructing/mutating this detached object
// never constructs BnwFreshTacticalMap or satisfies its genuine Game issuer.
// It uses real owned allocations; Init invokes the actual immutable DB getter.
class BnwTacticalMapStorage {
 public:
    BnwTacticalMapStorage();
    ~BnwTacticalMapStorage();
    BnwTacticalMapStorage(const BnwTacticalMapStorage&)=delete;
    BnwTacticalMapStorage& operator=(const BnwTacticalMapStorage&)=delete;
    BnwTacticalMapStorage(BnwTacticalMapStorage&&)=delete;
    BnwTacticalMapStorage& operator=(BnwTacticalMapStorage&&)=delete;
    BnwTacticalMapSnapshot observe() const;
    BnwTacticalCellSnapshot cell(std::int32_t) const;
    void initialize_detached(std::int32_t, const content::RulesDatabase&);
 private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace detail

// No public factory, constructor, init or NULL getter. Only the real Game may
// construct its held service AFTER all native predecessor Init operations.
class BnwFreshTacticalMap {
 public:
    ~BnwFreshTacticalMap();
    BnwFreshTacticalMap(const BnwFreshTacticalMap&)=delete;
    BnwFreshTacticalMap& operator=(const BnwFreshTacticalMap&)=delete;
    BnwTacticalMapSnapshot observe() const;
    BnwTacticalCellSnapshot cell(std::int32_t) const;
 private:
    friend class BnwFreshGame;
    // Exact db comes from this SAME Game's private config->game_rules().
    // A SHA equality alone is not the issuer; there is no public call site.
    BnwFreshTacticalMap(std::shared_ptr<const BnwCreationConfig>, const content::RulesDatabase&);
    void initialize_from_game_plot_source(std::int32_t);
    void require_current() const;
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace civ5::runtime
