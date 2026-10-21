#pragma once

#include "civ5/saves/map_state.hpp"
#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace civ5::runtime {

struct LiveSessionState;
struct BnwCityHandle {
    std::int64_t owner=-1,id=-1;
    bool operator==(const BnwCityHandle &) const = default;
    auto operator<=>(const BnwCityHandle &) const = default;
};
struct BnwCityLocation {
    std::int64_t x=0,y=0;
    bool operator==(const BnwCityLocation &) const = default;
};
// Factual unresolved source operands, never an admitted equal-distance order.
struct BnwNearbyCityOrderGap {
    BnwCityHandle origin;
    std::int64_t distance=0;
    std::vector<BnwCityHandle> tied;
    bool operator==(const BnwNearbyCityOrderGap &) const = default;
};
class BnwNearbyCityOrderUnavailable final:public std::runtime_error {
  public:
    explicit BnwNearbyCityOrderUnavailable(BnwNearbyCityOrderGap gap)
        : std::runtime_error("BNW nearby-city equal-distance order needs live pointer order"),
          gap_(std::move(gap)) {}
    const BnwNearbyCityOrderGap &gap() const noexcept { return gap_; }
  private:
    BnwNearbyCityOrderGap gap_;
};
struct BnwNearbyCityCache {
    bool current=false;
    saves::MapGrid grid;
    bool wrap_x=false,wrap_y=false;
    std::map<BnwCityHandle,BnwCityLocation> locations;
    std::map<BnwCityHandle,std::vector<BnwCityHandle>> nearby;
    std::optional<BnwNearbyCityOrderGap> order_gap;
};

// Expansion2 CvCity.cpp:13762 calls CvCityManager::OnCityCreated after Read.
// This resolves BNW CvPlot::isCity against the now-loaded city roster and
// rebuilds CvCityManager's gameplay proximity index from that same roster.
// The SDK's equal-distance order depends on pointer ordering in std::map;
// without that live evidence we reject a tie instead of inventing an order.
// A failed rebuild leaves the cache invalid. A typed tie stores only the
// unresolved factual operands, never a guessed live pointer/ID order.
void rebuild_bnw_nearby_cities(LiveSessionState &state,
                               bool city_roster_current);

// Independent CvPlot::isCity load hook. Validates the entire same raw/owned
// city and map roster before any plot bit store; it grants no proximity order.
// Direct proximity rebuild retains its no-partial-plot-flags failure contract.
void resolve_bnw_city_plot_links(LiveSessionState &state,
                                 bool city_roster_current);

// Checks the current owned city/map identities again before returning a list.
// City mutations invalidate the cache at the action boundary as well.
const std::vector<BnwCityHandle> &bnw_nearby_cities(
    const LiveSessionState &state,BnwCityHandle city);

} // namespace civ5::runtime
