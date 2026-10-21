#include "civ5/runtime/bnw_unit_movement_query.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <algorithm>
#include <iostream>
#include <stdexcept>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class Core>std::int64_t &scalar(Core &core,std::string_view name){for(auto &field:core.fields)if(field.name && std::string_view(field.name)==name && field.values.size()==1)return field.values[0];throw std::runtime_error("Movement fixture scalar absent");}
saves::PlayerTraitsField &trait(saves::SavedPlayerObservation &player,std::string_view name){for(auto &field:player.traits.fields)if(field.name && std::string_view(field.name)==name)return field;throw std::runtime_error("Movement fixture trait absent");}
saves::PlotArrayObservation &extra(saves::TeamObservation &team){for(auto &field:team.arrays)if(field.name && std::string_view(field.name)=="extra_moves")return field;throw std::runtime_error("Movement fixture team array absent");}
}
int main(int argc,char **argv) {
    check(argc==3 || argc==4,"Expected save/game root/optional SDK root");
    if(argc==4)for(const auto &file:kBnwUnitMovementSources)check(identify_file(std::filesystem::path(argv[3])/file.path).sha256==file.sha256,"Movement SDK hash differs");
    auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);check(selected.preview.host && !selected.preview.blocker,"Selected missing");
    const auto &host=*selected.preview.host;const auto &state=host.session().state();const auto player=host.active_player_id();
    auto baseline=state;const BnwUnitOrderKey key{player,scalar(baseline.world.all_players->players.at(player).units.units.front().core,"m_iID")};
    const auto query=[&](const LiveSessionState &world){return quote_bnw_unit_movement(world,host.database(),host.types(),host.identity(),key);};
    const auto normal=query(state);if(!normal.max_moves)std::cerr<<normal.blocker<<'\n';
    check(normal.base_moves==2 && normal.max_moves==120 && normal.has_moved==false && normal.terms.info_moves==2 &&
          normal.terms.domain==2 && normal.terms.team==state.setup.players.at(player).team && normal.terms.denominator==60 &&
          normal.terms.extra_moves==0 && normal.terms.team_extra_moves==0 && normal.terms.golden_age_moves==0 && normal.terms.combat_trait_moves==0,
          "Actual native Warrior movement terms differ");
    auto finished=state;scalar(finished.world.all_players->players.at(player).units.units.front().core,"m_iMoves")=0;
    const auto done=query(finished);check(done.max_moves==120 && done.has_moved==true,"Actual hasMoved did not compare current moves with true native maxMoves");
    auto modified=state;auto &owned=modified.world.all_players->players.at(player);auto &core=owned.units.units.front().core;
    scalar(core,"m_iExtraMoves")=1;scalar(owned.core,"m_iGoldenAgeTurns")=1;trait(owned,"m_iGoldenAgeMoveChange").values.at(0)=2;
    trait(owned,"m_paiMovesChangeUnitCombat").values.at(normal.terms.unit_combat)=3;
    extra(modified.world.teams->teams.at(normal.terms.team)).values.at(normal.terms.domain)=1;
    const auto bonuses=query(modified);check(bonuses.base_moves==9 && bonuses.max_moves==540 && bonuses.has_moved==true &&
         bonuses.terms.extra_moves==1 && bonuses.terms.team_extra_moves==1 && bonuses.terms.golden_age_moves==2 && bonuses.terms.combat_trait_moves==3,
         "Native movement ignored real saved team/trait/golden-age operands");
    auto not_golden=state;auto &fields=not_golden.world.all_players->players.at(player).traits.fields;
    fields.erase(std::remove_if(fields.begin(),fields.end(),[](const auto &f){return f.name && std::string_view(f.name)=="m_iGoldenAgeMoveChange";}),fields.end());
    check(query(not_golden).max_moves==120,"Native non-golden branch read an unneeded trait operand");
    scalar(not_golden.world.all_players->players.at(player).core,"m_iGoldenAgeTurns")=1;
    check(!query(not_golden).max_moves,"Golden-age movement silently substituted a missing trait with zero");
    auto truncated=state;trait(truncated.world.all_players->players.at(player),"m_paiMovesChangeUnitCombat").values.pop_back();
    check(!query(truncated).max_moves,"Incomplete native combat movement vector accepted");
    auto general=state;scalar(general.world.all_players->players.at(player).units.units.front().core,"m_iGreatGeneralCount")=1;
    const auto gg=query(general);check(!gg.max_moves && gg.blocker.find("GreatGeneralStackMovement")!=std::string::npos,"Great-General movement was flattened to info moves");
    auto embarked=state;scalar(embarked.world.all_players->players.at(player).units.units.front().core,"m_bEmbarked")=1;
    check(!query(embarked).max_moves && query(embarked).blocker.find("POLICYMOD")!=std::string::npos,"Embarked policy movement was silently skipped");
    auto unknown=state;extra(unknown.world.teams->teams.at(normal.terms.team)).values.pop_back();
    check(!query(unknown).max_moves,"Incomplete native domain movement array accepted");
    check(query(state).max_moves==120 && query(state).has_moved==false,"Query fixtures changed the selected caller world");
    std::cout<<"PASS native current-world Warrior baseMoves2/maxMoves120/hasMoved; team/trait/golden-age/class bonus arithmetic, incomplete and unsupported branch gates, caller unchanged\n";
}
