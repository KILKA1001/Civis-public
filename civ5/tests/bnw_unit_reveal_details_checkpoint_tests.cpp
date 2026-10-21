#include "civ5/runtime/bnw_unit_reveal_details_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool value,const char *reason) {
    if(!value) throw std::runtime_error(reason);
}
std::vector<std::int64_t> &array(
    std::vector<saves::PlotArrayObservation> &arrays,std::string_view key) {
    for(auto &item:arrays) if(item.name && key==item.name) return item.values;
    throw std::runtime_error("BNW reveal-detail test array missing");
}
const std::vector<std::int64_t> &array(
    const std::vector<saves::PlotArrayObservation> &arrays,
    std::string_view key) {
    for(const auto &item:arrays)
        if(item.name && key==item.name) return item.values;
    throw std::runtime_error("BNW reveal-detail test array missing");
}
std::vector<std::int64_t> &city_revealed(saves::SavedCityObservation &city) {
    for(auto &item:city.core.fields)
        if(item.name && std::string_view(item.name)=="m_abRevealed")
            return item.values;
    throw std::runtime_error("BNW reveal-detail test city reveal missing");
}
void selected(const std::filesystem::path &path) {
    saves::SavedSessionDecodeContext context;
    context.decode_map_regions=true;context.decode_teams=true;
    context.decode_all_players=true;
    context.all_players=saves::AllPlayersContext{};
    auto imported=saves::read_saved_session(path,context);
    const auto source=imported.snapshot;
    auto target=source;
    constexpr std::int64_t team=0;
    auto &plot=target.map_plots->plots.at(0);
    array(plot.arrays,"revealed_owners")[team]=7;
    array(plot.arrays,*plot.version>=6?"revealed_improvement_hashes":
                                   "revealed_improvements")[team]=123456;
    array(plot.arrays,"revealed_routes")[team]=1;
    auto &territory=array(target.teams->teams.at(team).arrays,
                          "found_player_territory");
    territory[7]=1-territory[7];
    std::size_t player_index=64,city_index=0;
    for(std::size_t p=0;p<64 && player_index==64;++p)
        if(!target.all_players->players[p].cities.cities.empty()) {
            player_index=p;
            auto &city=target.all_players->players[p].cities.cities[0];
            auto &reveal=city_revealed(city);
            reveal[team]=1-reveal[team];
        }
    check(player_index<64,"Selected Gandhi has no city reveal fixture");
    const runtime::SessionCheckpointIdentity identity{
        runtime::identify_bytes(imported.source.bytes).sha256,
        std::string(64,'a')};
    const auto section=runtime::make_bnw_unit_reveal_details_checkpoint(
        target,source,identity,team);
    check(section.id==runtime::bnw_unit_reveal_details_checkpoint_section_id,
          "BNW reveal-detail section ID changed");
    auto restored=source;
    runtime::restore_bnw_unit_reveal_details_checkpoint(
        section,restored,identity);
    auto repeat=runtime::make_bnw_unit_reveal_details_checkpoint(
        restored,source,identity,team);
    check(repeat.bytes==section.bytes &&
          city_revealed(restored.all_players->players[player_index]
                                  .cities.cities[city_index])[team]==
          city_revealed(target.all_players->players[player_index]
                                .cities.cities[city_index])[team] &&
          array(restored.map_plots->plots[0].arrays,"revealed_owners")[team]==7 &&
          array(restored.teams->teams[team].arrays,
                "found_player_territory")[7]==territory[7],
          "BNW reveal-detail roundtrip lost plot, city or team state");
    auto stale=source;
    array(stale.map_plots->plots[0].arrays,"revealed_routes")[team]=99;
    bool rejected=false;
    try {runtime::restore_bnw_unit_reveal_details_checkpoint(
        section,stale,identity);}
    catch(const std::exception &) {rejected=true;}
    check(rejected && array(stale.map_plots->plots[0].arrays,
                            "revealed_routes")[team]==99,
          "BNW reveal-detail accepted stale source or changed it");
    auto corrupt=section;
    corrupt.bytes.back()^=1;
    auto intact=source;
    rejected=false;
    try {runtime::restore_bnw_unit_reveal_details_checkpoint(
        corrupt,intact,identity);}
    catch(const std::exception &) {rejected=true;}
    check(rejected &&
          array(intact.map_plots->plots[0].arrays,"revealed_owners")[team]==
          array(source.map_plots->plots[0].arrays,"revealed_owners")[team],
          "BNW reveal-detail accepted corrupt target or published partial data");
    auto foreign=target;
    array(foreign.map_plots->plots[0].arrays,"revealed_owners")[team+1]=3;
    rejected=false;
    try {(void)runtime::make_bnw_unit_reveal_details_checkpoint(
        foreign,source,identity,team);}
    catch(const std::exception &) {rejected=true;}
    check(rejected,"BNW reveal-detail accepted another team plot mutation");
    std::cout<<"selected Gandhi reveal-detail plot/city/team roundtrip PASS\n";
}
}
int main(int argc,char **argv) {
    try {
        check(argc==2,"Supply selected Gandhi save");
        selected(argv[1]);
        return 0;
    } catch(const std::exception &error) {
        std::cerr<<"FAIL BNW reveal-detail checkpoint: "<<error.what()<<'\n';
        return 1;
    }
}
