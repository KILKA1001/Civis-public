#pragma once
#include "civ5/runtime/bnw_city_food_growth_prefix.hpp"
#include "civ5/runtime/bnw_city_population.hpp"
#include "civ5/runtime/bnw_population_totals_checkpoint.hpp"
#include "civ5/runtime/simulation_random.hpp"
#include <span>
#include <thread>
#include <vector>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
enum class BnwFoodPopulationStoreKind : std::uint8_t {
 food,food_kept,unassigned_citizens,city_population,population_rank,
 highest_population,area_population,area_player_population,player_population,
 team_population,game_population
};
// Read-only observation of a private actually executed native assignment.
// This DTO has no constructor/installer authority over a Pending.
struct BnwFoodPopulationStoreRecord {
 BnwFoodPopulationStoreKind kind=BnwFoodPopulationStoreKind::food;
 std::int32_t actor=-1,full_object=-1,before=0,after=0,projection_before=0;
 std::uint64_t revision_before=0,revision_after=0;
 BnwCityFoodGrowthPhase food_phase=BnwCityFoodGrowthPhase::prepared;
 BnwCityPopulationPhase population_phase=BnwCityPopulationPhase::prepared;
 bool population=false,conditional=false,stored=false,projection=false,returned=false;
 bool operator==(const BnwFoodPopulationStoreRecord&)const=default;
};
// Creator-thread runtime capability from ONE genuine registered Food parent
// and its actual still-unissued Population plot return. No wire47 or restore
// grant is provided. Destruction/continuation/foreign source invalidates it.
class BnwFoodPopulationCheckpointCapture final {
 public:
 ~BnwFoodPopulationCheckpointCapture();
 BnwFoodPopulationCheckpointCapture(const BnwFoodPopulationCheckpointCapture&)=delete;
 BnwFoodPopulationCheckpointCapture& operator=(const BnwFoodPopulationCheckpointCapture&)=delete;
 void require_current()const;
 const BnwPopulationTotalsCheckpointBundle& components()const;
 std::span<const BnwFoodPopulationStoreRecord> ordered_stores()const;
 static constexpr bool restores_food_once_owner()noexcept{return false;}
 private:
 friend class BnwFoodPopulationGrowthCheckpoint;friend class BnwCityFoodGrowthPending;
 BnwFoodPopulationCheckpointCapture(std::shared_ptr<const BnwCityFoodGrowthPending>,
  std::shared_ptr<const BnwCityPopulationPending>,std::shared_ptr<const ui::SelectedBnwPreviewHost>,
  BnwPopulationTotalsCheckpointBundle,saves::CivisSaveSection,std::vector<BnwFoodPopulationStoreRecord>,
  SessionCheckpointIdentity,BnwCityFoodGrowthProgress,BnwCityPopulationProgress,
  std::uint64_t,std::uint64_t,std::int64_t,bool,SimulationRandomState,SimulationRandomState);
 const std::thread::id thread_=std::this_thread::get_id();
 const std::shared_ptr<const BnwCityFoodGrowthPending> parent_;
 const std::shared_ptr<const BnwCityPopulationPending> child_;
 const std::shared_ptr<const ui::SelectedBnwPreviewHost> host_;
 const BnwPopulationTotalsCheckpointBundle components_;
 const saves::CivisSaveSection clock_;
 const std::vector<BnwFoodPopulationStoreRecord> stores_;
 const SessionCheckpointIdentity identity_;
 const BnwCityFoodGrowthProgress food_;
 const BnwCityPopulationProgress population_;
 const std::uint64_t generation_,revision_;
 const std::int64_t turn_;
 const bool sent_auto_moves_;
 const SimulationRandomState map_rng_,game_rng_;
};
}
