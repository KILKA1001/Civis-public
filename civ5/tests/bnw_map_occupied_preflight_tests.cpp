#include "civ5/runtime/bnw_map_occupied_preflight.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
void check(bool valid,const char *message) {
    if(!valid) throw std::runtime_error(message);
}
void selected(const std::filesystem::path &path,
              const std::filesystem::path &game_root) {
    const auto original=runtime::identify_file(path).sha256;
    auto boot=ui::bootstrap_selected_bnw_preview(path,game_root,0);
    check(!boot.preview.blocker && boot.preview.host,
          "Selected Gandhi BNW preview unavailable");
    const auto &world=boot.preview.host->session().state().world;
    const auto report=runtime::preflight_bnw_map_occupied(world);
    check(report.shape_valid && report.units.size()==7 &&
          report.anchors.size()==1 && report.requires_unit_services &&
          report.requires_anchor_service,
          "Selected Gandhi map blocker roster changed");
    std::set<std::uint64_t> occupied;
    for(const auto &unit:report.units) {
        occupied.insert(unit.plot);
        check(unit.saved_unit_found && unit.at_saved_plot,
              "Selected Gandhi plot unit is not in saved owner roster");
        std::cout << "unit plot=" << unit.plot << " xy=" << unit.x << ',' << unit.y
                  << " owner=" << unit.handle.owner << " id=" << unit.handle.id
                  << " plot_owner=" << unit.plot_owner
                  << " delayed=" << unit.delayed_death
                  << " cargo=" << unit.transport_link
                  << " combat=" << unit.combat_link << '\n';
    }
    check(occupied.size()==7,"Selected Gandhi occupied plot count changed");
    const auto &anchor=report.anchors.front();
    std::cout << "anchor plot=" << anchor.plot << " xy=" << anchor.x << ','
              << anchor.y << " type=" << anchor.old_anchor
              << " data=" << anchor.old_data << '\n';
    auto invalid=world;
    auto &plot=invalid.map_plots->plots.at(report.units.front().plot);
    bool changed=false;
    for(auto &array:plot.arrays)
        if(array.name && std::string_view(array.name)=="units") {
            array.values.front()=99;
            changed=true;break;
        }
    check(changed && !runtime::preflight_bnw_map_occupied(invalid).shape_valid,
          "BNW map preflight accepted mismatched raw/projected unit roster");
    invalid=world;
    invalid.map_projection->cells.at(report.units.front().plot).units.reset();
    check(!runtime::preflight_bnw_map_occupied(invalid).shape_valid,
          "BNW map preflight accepted unresolved plot roster");
    invalid=world;
    auto &anchored=invalid.map_plots->plots.at(anchor.plot);
    changed=false;
    for(auto &field:anchored.fields)
        if(field.name && std::string_view(field.name)=="world_anchor") {
            field.value=128;
            changed=true;break;
        }
    check(changed && !runtime::preflight_bnw_map_occupied(invalid).shape_valid,
          "BNW map preflight accepted an out-of-range world anchor");
    check(runtime::identify_file(path).sha256==original,
          "Selected Gandhi source changed during read-only preflight");
    std::cout << "PASS selected Gandhi map occupied preflight\n";
}
} // namespace

int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi BNW save and game root");
        selected(argv[1],argv[2]);return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW map occupied preflight: " << error.what() << '\n';
        return 1;
    }
}
