#pragma once
#include <array>
#include <cstdint>
#include <string>
namespace civ5::ui {
using UiFloatColor=std::array<float,4>;
struct BnwCityBannerAppearance {
    std::uint64_t owner_id=0,frame_number=0;
    std::int64_t active_player_id=-1,player_id=-1,city_id=-1,team_id=-1,active_team_id=-1,turn=-1;
    bool minor=false,is_active_team=false;
    UiFloatColor background{},foreground{},shadow{};
    std::string owner_sha256,rules_rows_sha256,lua_sha256,xml_sha256;
};
} // namespace civ5::ui
