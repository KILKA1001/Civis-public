#pragma once
#include "civ5/runtime/bnw_city_production_turn.hpp"
#include <memory>
#include <span>
#include "civ5/runtime/bnw_city_dirty_event.hpp"
namespace civ5::runtime {class BnwSelectedStockLuaSourceOwner;}
namespace civ5::ui {
class SelectedBnwPreviewHost;
namespace detail {
// Supplied descriptor validation only, no operation/epoch/production issuer.
void require_production_city_screen_delta(
 const runtime::BnwCityProductionTurnProgress&,const runtime::BnwCityProductionTurnProgress&,
 std::uint64_t old_revision,std::uint64_t new_revision,
 std::uint64_t old_occurrence,std::uint64_t new_occurrence,std::uint64_t generation,
 std::span<const runtime::BnwCityDirtyEvent> before,std::span<const runtime::BnwCityDirtyEvent> after);
}
// Real canonical selection + CityScreen publication. The mandatory stock
// SpecificCityInfoDirty return remains unavailable; no caller callback/ack.
class BnwCityProductionUiAdapter final {
 public:
 static std::shared_ptr<BnwCityProductionUiAdapter> from_selected_host(std::shared_ptr<const SelectedBnwPreviewHost>);
 ~BnwCityProductionUiAdapter();
 BnwCityProductionUiAdapter(const BnwCityProductionUiAdapter&)=delete;
 BnwCityProductionUiAdapter& operator=(const BnwCityProductionUiAdapter&)=delete;
 void require_current()const;
 runtime::BnwCityProductionTurnProgress continue_city(std::shared_ptr<const runtime::BnwCityProductionTurnPending>);
 private:
 friend class runtime::BnwSelectedStockLuaSourceOwner;
 runtime::BnwCityProductionTurnKey require_same_stock_pending(
  const SelectedBnwPreviewHost*,const std::shared_ptr<const runtime::BnwCityProductionTurnPending>&)const;
 void fail_stock_pending(const std::shared_ptr<const runtime::BnwCityProductionTurnPending>&)noexcept;
 struct Impl;std::unique_ptr<Impl> impl_;
 explicit BnwCityProductionUiAdapter(std::unique_ptr<Impl>)noexcept;
};
}
