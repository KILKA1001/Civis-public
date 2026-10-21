#pragma once
#include <cstdint>
#include <memory>
#include <tuple>
namespace civ5::ui {class SelectedBnwPreviewHost;class BnwCityProductionUiAdapter;class SelectedCityTrainCreateSource;}
namespace civ5::runtime {
class LiveSession;class BnwCityCurrentProjectProductionDifference;
using BnwCityProductionTurnKey=std::tuple<std::int32_t,std::int32_t,std::int32_t,std::uint64_t,std::uint64_t>;
enum class BnwCityProductionTurnPhase { project_store_pending, city_screen_predicate_pending, city_screen_dirty_pending, banner_event_pending, overflow_pending, feature_pending, pop_order_pending, complete_component, quarantined, unit_store_pending };
struct BnwCityProductionTurnProgress {
 std::int32_t turn=0,owner=-1,city=-1,process=-1;
 std::int32_t overflow_before=0,feature_before=0;
 std::int32_t difference_times100=0,project_before=0,production_needed=INT32_MAX;
 std::uint8_t native_calls=0,raw_writes=0;
 BnwCityProductionTurnPhase phase=BnwCityProductionTurnPhase::overflow_pending;
};
namespace detail {
// Supplied native arithmetic only; never a source/turn/city mutation issuer.
std::int32_t bnw_project_modifier_math(std::int32_t industrial,std::int32_t supply,std::int32_t player_space,std::int32_t city_space);
std::int32_t bnw_production_supply_math(std::int32_t handicap_free,
 std::int32_t per_city,std::int32_t city_count,std::int32_t population_percent,
 std::int32_t population,std::int32_t units,std::int32_t trade_units,std::int32_t cap);
}
class BnwCityProductionTurnCheckpoint;
// LiveSession is the sole issuer/writer. The imported component does NOT
// attest doCheckProduction, growth, route updates or full doProduction.
class BnwCityProductionTurnPending final {
 public:
 ~BnwCityProductionTurnPending();
 BnwCityProductionTurnPending(const BnwCityProductionTurnPending&)=delete;
 BnwCityProductionTurnProgress progress()const;
 void require_current()const;
 static constexpr bool full_production_ready()noexcept{return false;}
 private:
 friend class ui::SelectedCityTrainCreateSource;friend class BnwSelectedStockLuaSourceOwner;friend class LiveSession;friend class BnwCityCurrentProjectProductionDifference;friend class ui::BnwCityProductionUiAdapter;
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwCityProductionTurnPending(std::unique_ptr<Impl>)noexcept;
 static std::shared_ptr<BnwCityProductionTurnPending> prepare(LiveSession&,
  std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
 static std::shared_ptr<BnwCityProductionTurnPending> prepare_project(const LiveSession&,std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
 static std::shared_ptr<BnwCityProductionTurnPending> prepare_project_read(const LiveSession&,std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
 struct TrainCompletionRead {std::int32_t needed_times100=0,stored_times100=0,difference_without_overflow=0;};
 static TrainCompletionRead read_train_completion_without_overflow(const std::shared_ptr<const ui::SelectedBnwPreviewHost>&,std::int32_t,std::int32_t);
 static std::shared_ptr<BnwCityProductionTurnPending> prepare_train(const LiveSession&,std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
 static std::shared_ptr<BnwCityProductionTurnPending> prepare_mode(const LiveSession&,std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t,bool,bool=false,bool=false);
 std::int32_t private_current_difference()const;
 BnwCityProductionTurnKey source_key()const;
 void advance_one();
 void begin_ui_effect();
 void accept_city_screen_predicate(bool);
 void accept_city_screen_publication();
 bool private_train_mode()const;
 void begin_train_banner();
 void accept_train_banner();
 void fail_ui_effect()noexcept;
 void invalidate()noexcept;
 std::shared_ptr<const BnwCityProductionTurnCheckpoint> checkpoint(bool history_only=false)const;
 static std::shared_ptr<BnwCityProductionTurnPending> restore_quarantined(const BnwCityProductionTurnCheckpoint&);
};
class BnwCityProductionTurnCheckpoint final {
 private:
 friend class BnwCityProductionTurnPending;friend class LiveSession;
 struct Impl;std::shared_ptr<const Impl> impl_;
 explicit BnwCityProductionTurnCheckpoint(std::shared_ptr<const Impl>)noexcept;
};
}
