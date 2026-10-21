#pragma once
#include <cstdint>
#include <memory>
#include <stdexcept>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::saves {struct SavedSession;struct CivisSaveSection;}
namespace civ5::runtime {
class BnwFoodPopulationGrowthCheckpoint;struct BnwPopulationTotalsCheckpointBundle;class BnwFoodPopulationCheckpointCapture;class BnwPlayerHappinessPrefixOwner;class BnwCityYieldRefreshPending;class BnwCityReligionYieldOwner;class LiveSession;class BnwCityFoodGrowthRegistry;class BnwCityPopulationPending;class BnwPlotYieldRefreshPending;class BnwPlotYieldRefreshIssuer;struct LiveSessionState;struct SessionCheckpointIdentity;
class BnwCityFoodGrowthUnavailable final:public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
enum class BnwCityFoodGrowthPhase {
 prepared,razing_returned,starvation_notification,food_stored,kept_stored,
 kept_clamped,forced_avoid_returned,ordinary_returned,population_growth,
 population_starvation,failed,retired
};
struct BnwCityFoodGrowthProgress {
 std::int32_t owner=-1,city=-1,difference_times100=0;
 std::int32_t food_times100=0,food_kept=0;
 std::uint64_t native_scalar_assignments=0,ui_revision=0;
 BnwCityFoodGrowthPhase phase=BnwCityFoodGrowthPhase::prepared;
 bool difference_observed=false,returned=false;
};
// Imported native-member component, not a scheduler/Citizens/production
// predecessor. Public progress never authorizes stores or completion.
class BnwCityFoodGrowthPending final {
 public:
 ~BnwCityFoodGrowthPending();
 BnwCityFoodGrowthPending(const BnwCityFoodGrowthPending&)=delete;
 BnwCityFoodGrowthPending& operator=(const BnwCityFoodGrowthPending&)=delete;
 BnwCityFoodGrowthProgress progress()const;
 void require_current()const;
 // Continues only this registered actual growth/starvation call, once.
 std::shared_ptr<const BnwCityPopulationPending> continue_population()const;
 std::shared_ptr<const BnwCityPopulationPending> population_pending()const;
 // Genuine runtime capture only; totals46 alone do not restore this once owner.
 std::shared_ptr<const BnwFoodPopulationCheckpointCapture> capture_population_checkpoint_before_plot()const;
 private:
 friend class BnwFoodPopulationGrowthCheckpoint;friend class ui::SelectedBnwPreviewHost;friend class BnwFoodPopulationCheckpointCapture;friend class BnwPlayerHappinessPrefixOwner;friend class BnwCityReligionYieldOwner;friend class LiveSession;friend class BnwCityFoodGrowthRegistry;friend class BnwCityPopulationPending;
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwCityFoodGrowthPending(std::unique_ptr<Impl>)noexcept;
 static std::shared_ptr<BnwCityFoodGrowthPending> create(LiveSession&,
  const std::shared_ptr<const ui::SelectedBnwPreviewHost>&,std::int64_t,std::int64_t);
 void execute();
 void store(const char*,std::int32_t,bool,BnwCityFoodGrowthPhase);
 std::int32_t threshold()const;
 void require_source_registration()const;
 void continue_population_plot(const BnwCityPopulationPending&)const;
 void continue_population_religion(const BnwCityPopulationPending&)const;
 void require_population_city_binding(const BnwCityPopulationPending&,const BnwCityYieldRefreshPending&)const;
 void bind_population_city_plot(const BnwCityPopulationPending&,const BnwCityYieldRefreshPending&,const std::shared_ptr<BnwPlotYieldRefreshPending>&)const;
 void accept_population_city_plot(const BnwCityPopulationPending&,const BnwCityYieldRefreshPending&,const BnwPlotYieldRefreshPending&)const;
 void require_population_religion_return()const;
 void require_population_player_return()const;
 void continue_population_player_religion(const BnwCityPopulationPending&)const;
 void require_population_plot_unissued(const BnwCityPopulationPending&,const BnwPlotYieldRefreshIssuer&,const LiveSessionState&,const saves::SavedSession&,const SessionCheckpointIdentity&,std::int64_t)const;
 void require_population_plot_binding(const BnwCityPopulationPending&,const BnwPlotYieldRefreshPending&)const;
 void accept_population_plot_prefix(BnwCityPopulationPending&)const;
 void require_population_child(const BnwCityPopulationPending&)const;
 void execute_population(BnwCityPopulationPending&)const;
 void require_population_capture_current_without_callbacks(const BnwFoodPopulationCheckpointCapture&)const;
 std::int32_t growth_checkpoint_threshold_for(const BnwFoodPopulationCheckpointCapture&)const;
 static std::shared_ptr<const BnwCityFoodGrowthPending> restore_growth_before_plot_from_unpublished_host(
  std::shared_ptr<ui::SelectedBnwPreviewHost>,const saves::CivisSaveSection&,
  const BnwPopulationTotalsCheckpointBundle&,const saves::CivisSaveSection&,const saves::CivisSaveSection&);
 void require_restored_growth_publication_current()const;
 void retire()noexcept;
};
class BnwCityFoodGrowthRegistry final {
 public:~BnwCityFoodGrowthRegistry();void retire()noexcept;
 private:
 friend class LiveSession;friend class BnwCityFoodGrowthPending;
 struct Impl;std::unique_ptr<Impl> impl_;
 BnwCityFoodGrowthRegistry();
};
namespace detail {
// Native expression boundaries only, never an issuing/store API.
std::int32_t bnw_growth_add_food(std::int32_t,std::int32_t);
std::int32_t bnw_growth_add_kept(std::int32_t,std::int32_t);
std::int32_t bnw_growth_clamp_kept(std::int32_t,std::int32_t,std::int32_t);
std::int32_t bnw_growth_food_change(std::int32_t,std::int32_t);
}
}
