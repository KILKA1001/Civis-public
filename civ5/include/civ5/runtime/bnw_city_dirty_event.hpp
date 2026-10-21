#pragma once
#include <cstdint>
namespace civ5::runtime {
// SDK void setDirty notifications. Ordered repetitions are intentional.
enum class BnwCityDirtyKind:std::int32_t { city_info=18, city_screen=21, colored_plots=24, help=14, selection_buttons=6, plot_list_buttons=5 };
struct BnwCityDirtyEvent {
 std::uint64_t occurrence=0,generation=0;
 std::int32_t owner=-1,city=-1;
 std::int64_t plot=-1;
 std::int8_t yield=-1;
 BnwCityDirtyKind kind=BnwCityDirtyKind::city_info;
 bool value=true;
 bool operator==(const BnwCityDirtyEvent&)const=default;
};
}

namespace civ5::runtime {
// Different native API from setDirty: SDK CityUpdateTypes PRODUCTION=2.
enum class BnwSpecificCityInfoKind:std::int32_t {production=2,banner=0,garrison=4};
struct BnwSpecificCityInfoEvent {
 std::uint64_t occurrence=0,generation=0;
 std::int32_t owner=-1,city=-1;
 BnwSpecificCityInfoKind kind=BnwSpecificCityInfoKind::production;
 bool operator==(const BnwSpecificCityInfoEvent&)const=default;
};
}
