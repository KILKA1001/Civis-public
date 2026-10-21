#include "../validation/bnw_warrior_world_binding.hpp"
#include "../validation/bnw_warrior_art_probe.hpp"
#include <stdexcept>

void require(bool yes,const char *why) {
    if(!yes) throw std::runtime_error(why);
}

int main() {
    using namespace civ5::graphics;
    BnwRendererSceneReceipt receipt;
    receipt.projection_reconciled=true;
    receipt.active_player=0;
    receipt.active_team=0;
    receipt.game_turn=0;
    receipt.cells.resize(593);
    receipt.active_team_art.resize(593);
    auto &cell=receipt.cells[592];
    cell.index=592; cell.x=32; cell.y=14;
    cell.units=std::vector<civ5::saves::SavedPlotUnitReference>{{0,16385}};
    receipt.active_team_art[592].revealed=true;
    receipt.active_team_art[592].visibility_count=1;
    const auto bound=selected_gandhi_warrior_from_scene(receipt,true);
    require(bound && bound->plot_index==592 && bound->x==32 && bound->y==14 &&
            bound->visible && !bound->model_drawable,
            "Verified Warrior scene did not bind as resource-only");
    require(!selected_gandhi_warrior_from_scene(receipt,false),
            "Hidden Warrior was bound");
    cell.units->clear();
    require(!selected_gandhi_warrior_from_scene(receipt,true),
            "Removed Warrior was bound");
    cell.units->push_back({0,7});
    require(!selected_gandhi_warrior_from_scene(receipt,true),
            "Wrong unit ID was bound");
    cell.units->at(0).id=16385;
    cell.index=591;
    require(!selected_gandhi_warrior_from_scene(receipt,true),
            "Wrong plot was bound");
    cell.index=592;
    receipt.active_team_art[592].visibility_count=0;
    require(!selected_gandhi_warrior_from_scene(receipt,true),
            "Zero-visibility Warrior was bound");
    receipt.active_team_art[592].visibility_count=1;
    receipt.projection_reconciled=false;
    require(!selected_gandhi_warrior_from_scene(receipt,true),
            "Unreconciled scene was bound");
    require_bnw_warrior_asset_sha("source-sha","source-sha");
    bool rejected=false;
    try { require_bnw_warrior_asset_sha("wrong-sha","source-sha"); }
    catch(const std::runtime_error &) { rejected=true; }
    require(rejected,"Wrong Warrior art SHA was accepted");
    int members=0;
    for(const auto &art:bnw_gandhi_warrior_art) members+=art.count;
    require(members==12,"Warrior formation member count differs");
}
