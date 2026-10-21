#pragma once
#include "civ5/runtime/bnw_city_management_checkpoint.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
class BnwCityStrategyObsoleteEndUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
namespace detail {
// Supplied arithmetic only. No trigger/Lua/strategy/publication authority.
struct BnwCityStrategyFlavorMath {
 std::vector<std::int32_t> latest,delta;
 std::int32_t minimum=0,maximum=0;
 // Building, unit, project, process; native Info slot order. nullopt is an
 // actual null Info slot, not a missing flavor cache silently converted to 0.
 std::array<std::vector<std::optional<std::vector<std::int32_t>>>,4> coefficients;
};
struct BnwCityStrategyFlavorResult {
 std::vector<std::int32_t> latest;
 std::array<std::vector<std::int32_t>,4> weights;
};
BnwCityStrategyFlavorResult bnw_city_strategy_end_flavor_math(const BnwCityStrategyFlavorMath&);
// Supplied native ADD/clamp/Reset arithmetic only; never a Lua result/grant.
BnwCityStrategyFlavorResult bnw_city_strategy_start_flavor_math(const BnwCityStrategyFlavorMath&);
}
class BnwCityStrategyObsoleteEndOwner;
class BnwCityStrategyObsoleteEndStage final {
 public:
 std::shared_ptr<const saves::SavedSessionSnapshot> read_world()const;
 // Existing ID27, not a new serializer or complete world persistence.
 saves::CivisSaveSection management_section()const;
 static constexpr bool live_publication_ready()noexcept{return false;}
 static constexpr bool city_entry_phase_ready()noexcept{return false;}
 private:
 friend class BnwCityStrategyObsoleteEndOwner;
 friend class BnwCityEntryPending;
 detail::BnwCityStrategyFlavorResult computed_flavor_result()const;
 struct Impl;std::shared_ptr<const Impl> impl_;
 explicit BnwCityStrategyObsoleteEndStage(std::shared_ptr<const Impl>)noexcept;
};
class BnwCityStrategyObsoleteEndOwner final {
 public:
 // Selector ONLY. The owner independently requires actual scheduled end and
 // actual TEAM obsolete tech; no caller predicate/epoch/Lua result exists.
 BnwCityStrategyObsoleteEndStage prepare(std::int64_t native_strategy_id)const;
 static constexpr bool live_publication_ready()noexcept{return false;}
 private:
 friend class BnwCityStrategyObsoleteEndStage;
 friend class BnwCityEntryPending;
 // Only the registered ordered Pending can reach false-END after its real helper.
 BnwCityStrategyObsoleteEndStage prepare_false_end(std::int64_t)const;
 // A candidate effect only. The registered ordered Pending must obtain a
 // genuine CityStrategyCanActivate result before any live START publication.
 // No public selector/boolean/Lua DTO can call or authorize this transition.
 BnwCityStrategyObsoleteEndStage prepare_start_candidate(std::int64_t)const;
 enum class EffectPath { obsolete_end, ordered_false_end, ordered_start_candidate };
 BnwCityStrategyObsoleteEndStage prepare_effect(std::int64_t,EffectPath)const;
 friend BnwCityStrategyObsoleteEndOwner bind_bnw_city_strategy_obsolete_end(
   std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwCityStrategyObsoleteEndOwner(std::shared_ptr<Impl>)noexcept;
 void require_current()const;
};
BnwCityStrategyObsoleteEndOwner bind_bnw_city_strategy_obsolete_end(
 std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t city);
}
