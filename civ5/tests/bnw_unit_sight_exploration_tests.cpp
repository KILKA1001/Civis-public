#include "civ5/runtime/bnw_unit_sight_exploration.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using namespace civ5;
using namespace civ5::runtime;
void check(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::int64_t core(const saves::SavedUnitObservation &unit,std::string_view wanted) {
    for(const auto &field:unit.core.fields)
        if(field.name && wanted==field.name && field.values.size()==1)
            return field.values.front();
    throw std::runtime_error("Selected BNW exploration unit field missing");
}
BnwSetXYRequest candidate(const LiveSessionState &state) {
    const auto &grid=state.world.map_header.grid;
    for(const auto &player:state.world.all_players->players)
        for(const auto &unit:player.units.units) {
            const auto x=core(unit,"m_iX"),y=core(unit,"m_iY");
            if(x<0 || y<0 || x>=static_cast<std::int64_t>(grid.width) ||
               y>=static_cast<std::int64_t>(grid.height)) continue;
            for(const auto [dx,dy]:{std::pair{-1,0},std::pair{1,0},
                                    std::pair{0,-1},std::pair{0,1}}) {
                const BnwSetXYRequest request{core(unit,"m_eOwner"),
                    core(unit,"m_iID"),x+dx,y+dy};
                if(preflight_bnw_unit_sight(state,request).candidate) return request;
            }
        }
    throw std::runtime_error("Selected Gandhi save has no simple sight candidate");
}
std::int64_t &dirty(saves::SavedPlayerObservation &player) {
    for(auto &block:player.strategy.blocks)
        if(block.name && std::string_view(block.name)=="economic_ai")
            for(auto &field:block.fields)
                if(field.name && std::string_view(field.name)==
                   "exploration_plots_dirty") return field.value;
    throw std::runtime_error("Selected BNW exploration dirty flag missing");
}
std::int64_t dirty_value(const saves::SavedPlayerObservation &player) {
    for(const auto &block:player.strategy.blocks)
        if(block.name && std::string_view(block.name)=="economic_ai")
            for(const auto &field:block.fields)
                if(field.name && std::string_view(field.name)==
                   "exploration_plots_dirty") return field.value;
    throw std::runtime_error("Selected BNW exploration dirty flag missing");
}
void selected(const std::filesystem::path &save,
              const std::filesystem::path &game_root) {
    const auto source=identify_file(save);
    auto bootstrap=ui::bootstrap_selected_bnw_preview(save,game_root,0);
    check(static_cast<bool>(bootstrap.preview.host),
          "Selected BNW live preview did not bootstrap");
    auto &host=*bootstrap.preview.host;
    const auto &state=host.session().state();
    const auto input=host.preview_input();
    check(static_cast<bool>(input.frame_sources.database),
          "Selected BNW rules database is absent");
    const auto &database=*input.frame_sources.database;
    const auto types=database.cache_info_types();
    const auto defines=database.cache_global_defines(content::GlobalDefineProfile::bnw);
    const auto request=candidate(state);
    const auto result=apply_bnw_unit_sight_exploration_detached(
        state,request,database,types,defines,host.identity());
    check(result.calculated && result.sight.detached && !result.can_commit() &&
          result.sight.plan.newly_visible.size()==5 &&
          !result.dirty_players.empty(),result.reason.c_str());
    for(const auto id:result.dirty_players)
        for(auto &player:result.sight.detached->world.all_players->players)
            if(player.saved_player_id==id)
                check(dirty_value(player)==1,
                      "BNW live EconomicAI discovery flag was not set");
    check(request.owner==0 && request.unit_id==16385 &&
          result.dirty_players.front()==0,
          "Selected Gandhi discovery recipient changed");
    auto cleared=state;
    auto &recipient=cleared.world.all_players->players.at(0);
    check(recipient.saved_player_id==0,
          "Selected BNW player roster order changed");
    dirty(recipient)=0;
    const auto changed=apply_bnw_unit_sight_exploration_detached(
        cleared,request,database,types,defines,host.identity());
    check(changed.calculated && changed.sight.detached &&
          dirty_value(changed.sight.detached->world.all_players->players.at(0))==1 &&
          dirty(recipient)==0,
          "BNW exploration dirty effect changed source or missed detached copy");
    auto malformed=state;
    dirty(malformed.world.all_players->players.at(0))=2;
    const auto rejected=apply_bnw_unit_sight_exploration_detached(
        malformed,request,database,types,defines,host.identity());
    check(!rejected.calculated && !rejected.sight.detached &&
          !rejected.can_commit(),
          "BNW exploration published partial state after invalid dirty flag");
    check(identify_file(save).sha256==source.sha256,
          "BNW exploration changed original save");
    std::cout << "BNW first reveals=" << result.sight.plan.newly_visible.size() <<
        " EconomicAI recipients=" << result.dirty_players.size() << '\n';
}
}

int main(int argc,char **argv) {
    try {
        check(argc==3,"Supply selected Gandhi save and game root");
        selected(argv[1],argv[2]);
        std::cout << "PASS BNW detached exploration discovery flag\n";
        return 0;
    } catch(const std::exception &error) {
        std::cerr << "FAIL BNW exploration: " << error.what() << '\n';
        return 1;
    }
}
