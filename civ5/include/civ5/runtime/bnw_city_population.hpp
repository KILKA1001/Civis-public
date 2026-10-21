#pragma once
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>
#include <vector>
namespace civ5::saves {struct SavedSession;}
namespace civ5::runtime {
class LiveSession;class BnwCityYieldRefreshPending;class BnwCityFoodGrowthPending;class BnwPlotYieldRefreshPending;class BnwPlotYieldRefreshIssuer;
struct LiveSessionState;struct SessionCheckpointIdentity;
class BnwCityPopulationUnavailable final:public std::runtime_error {public:using std::runtime_error::runtime_error;};
enum class BnwCityPopulationPhase {prepared,remove_default_specialist_required,remove_worst_worked_or_specialist_required,population_stored,ranks_invalidated,highest_population_stored,area_stored,player_stored,team_stored,game_stored,plot_yield_child_required,plot_call,update_religion_required,religion_city_call,religion_yields,player_update_religion_required,player_happiness,player_founder_religion_required,failed};
struct BnwCityPopulationProgress {
 std::int32_t owner=-1,city=-1,change=0,before=0,after=0;
 std::uint64_t assignments=0,rank_assignments=0,ui_revision=0;
 std::uint8_t religion_reset_assignments=0,religion_yield=0;
 std::uint8_t religion_city_slots=0,religion_city_null_skips=0,religion_city_returned_calls=0;
 std::uint64_t religion_assignments=0;
 std::uint8_t player_happiness_stores=0;
 bool player_religion_attempted=false;
 bool attempted=false,unassigned_removal_returned=false;
 BnwCityPopulationPhase phase=BnwCityPopulationPhase::prepared;
};
// No constructor, caller delta/state/permission or success acknowledgement.
// Only an actual retained food Pending can issue this native continuation.
class BnwCityPopulationPending final {
 public:
 ~BnwCityPopulationPending()=default;
 BnwCityPopulationPending(const BnwCityPopulationPending&)=delete;
 BnwCityPopulationPending& operator=(const BnwCityPopulationPending&)=delete;
 BnwCityPopulationProgress progress()const;
 void require_current()const;
 // Actual registered single native call; no plot/delta/permission supplied.
 void continue_plot()const;
 // Native UpdateReligion: fresh city37 first, then real religion setters.
 void continue_religion()const;
 private:
 friend class LiveSession;friend class BnwCityYieldRefreshPending;friend class BnwCityFoodGrowthPending;friend class BnwPlotYieldRefreshPending;
 explicit BnwCityPopulationPending(std::weak_ptr<const BnwCityFoodGrowthPending>);
 void check_owned_prefix()const;
 void require_unissued_plot_source(const BnwPlotYieldRefreshIssuer&,const LiveSessionState&,const saves::SavedSession&,const SessionCheckpointIdentity&,std::int64_t)const;
 void require_plot_binding(const BnwPlotYieldRefreshPending&)const;
 void require_city_binding(const BnwCityYieldRefreshPending&)const;
 void bind_city_plot(const BnwCityYieldRefreshPending&,const std::shared_ptr<BnwPlotYieldRefreshPending>&)const;
 void accept_city_plot(const BnwCityYieldRefreshPending&,const BnwPlotYieldRefreshPending&)const;
 bool religion_attempted_=false;
 bool plot_attempted_=false;
 struct Binding {const std::int64_t* value;std::int64_t expected;};
 const std::thread::id thread_=std::this_thread::get_id();
 std::weak_ptr<const BnwCityFoodGrowthPending> parent_;
 BnwCityPopulationProgress progress_;
 std::vector<Binding> owned_writes_;
};
namespace detail {
// Pure native signed expression, never an issuer or a detached gameplay stage.
std::int32_t bnw_population_add(std::int32_t,std::int32_t);
}
}
