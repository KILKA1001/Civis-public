#pragma once
#include "civ5/saves/game_trade.hpp"
#include <cstdint>
#include <span>
#include <memory>
#include <stdexcept>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
enum class BnwTradeCityStrategy {need_land,no_need_land,need_sea,no_need_sea,international_origin,international_destination};
class BnwCityTradeStrategyUnavailable final:public std::runtime_error {public:using std::runtime_error::runtime_error;};
class BnwCityCurrentTradeStrategy final {
 public:bool value(BnwTradeCityStrategy)const;
 private:struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwCityCurrentTradeStrategy(std::shared_ptr<Impl>)noexcept;
 void require_current()const;
 friend BnwCityCurrentTradeStrategy inspect_bnw_city_current_trade_strategy(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t,std::int64_t);
};
BnwCityCurrentTradeStrategy inspect_bnw_city_current_trade_strategy(std::shared_ptr<const ui::SelectedBnwPreviewHost>,std::int64_t owner,std::int64_t city);
namespace detail {
// Pure native widths, no current-world admission or completion capability.
std::int32_t bnw_trade_remaining_math(std::int64_t signed_capacity,std::int64_t used);
std::int32_t bnw_trade_international_count_math(std::span<const saves::TradeRouteObservation> raw,
 std::span<const saves::TradeRouteObservation> mirror,std::int64_t x,std::int64_t y,BnwTradeCityStrategy);
}
}
