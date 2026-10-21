#include "civ5/runtime/bnw_unit_order_owners.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <iostream>
#include <stdexcept>
using namespace civ5;using namespace civ5::runtime;
namespace {
void check(bool value,const char *why){if(!value)throw std::runtime_error(why);}
template<class Core>std::int64_t &scalar(Core &core,std::string_view name){for(auto &field:core.fields)if(field.name && std::string_view(field.name)==name && field.values.size()==1)return field.values[0];throw std::runtime_error("Missing owner fixture scalar");}
}
int main(int argc,char **argv) {
    check(argc==3 || argc==4,"Expected save/game-root/optional SDK root");
    if(argc==4)for(const auto &source:kBnwUnitOrderOwnerSources)check(identify_file(std::filesystem::path(argv[3])/source.path).sha256==source.sha256,"Unit order SDK pin differs");
    auto selected=ui::bootstrap_selected_bnw_preview(argv[1],argv[2],0);check(selected.preview.host && !selected.preview.blocker,"Selected world missing");
    const auto &host=*selected.preview.host;const auto &state=host.session().state();const auto player=host.active_player_id();
    auto raw=state;const auto key=BnwUnitOrderKey{player,scalar(raw.world.all_players->players.at(player).units.units.front().core,"m_iID")};
    auto paths=bind_bnw_unit_paths_original_import(host.session().source(),host.identity());
    check(paths.paths().contains(key) && paths.paths().at(key).nodes.empty() && paths.paths().at(key).destination==UINT32_MAX,"Original Read path reset differs");
    paths.clear(key);check(paths.paths().at(key).nodes.empty() && paths.paths().at(key).destination==UINT32_MAX && paths.paths().at(key).clear_operations==1,"Actual path clear was not retained");
    auto source=host.session().source();source.source.container=saves::SaveContainerKind::civis;bool checkpoint_blocked=false;
    try{(void)bind_bnw_unit_paths_original_import(source,host.identity());}catch(const std::exception&){checkpoint_blocked=true;}
    check(checkpoint_blocked,"Civis continuation invented original unsaved path reset");
    auto client=start_bnw_unit_presentation_client(state,host.identity(),host.database());
    check(!client.selected_unit() && !client.selection_plot() && !client.dim_flags().at(key) && client.cycle_selection_counter()==0 &&
          !client.unit_info_dirty() && !client.plot_list_dirty() && !client.selection_buttons_dirty() && !client.waypoints_dirty() && !client.has_moved_unit(),"New client invented saved UI state");
    const auto pristine=client;bool invalid_selection=false;
    try{select_bnw_unit_presentation(state,client,host.database(),{player,INT32_MAX},client.client_revision());}catch(const std::exception&){invalid_selection=true;}
    check(invalid_selection && client.client_revision()==pristine.client_revision() && !client.selected_unit(),"Invalid selection changed actual owner");
    select_bnw_unit_presentation(state,client,host.database(),key,client.client_revision());
    check(client.selected_unit()==key && client.selection_plot()==bnw_unit_order_plot(state,key) && client.client_revision()==1,"Explicit actual selection input missing");
    BnwUnitPresentationBatch feed{host.identity(),state.world.map_header.guid,pristine.world_revision(),{},0};
    scalar(raw.world.all_players->players.at(player).units.units.front().core,"m_iMoves")=0;
    feed.after_world_revision=bnw_unit_order_world_revision(raw,host.identity(),host.database());
    feed.effects={{BnwUnitPresentationEffectKind::dirty_unit_info,key,1},{BnwUnitPresentationEffectKind::should_dim_flag,key,2}};
    auto consumer=pristine;bool invalid_feed=false;
    try{apply_bnw_unit_presentation_batch(raw,consumer,host.database(),feed);}catch(const std::exception&){invalid_feed=true;}
    check(invalid_feed && !consumer.unit_info_dirty() && !consumer.dim_flags().at(key) && consumer.received_effects().empty() && consumer.client_revision()==0,"Late invalid feed partially published dirty/dim state");
    auto mismatched=feed;mismatched.effects={{BnwUnitPresentationEffectKind::dirty_unit_info,key,1},{BnwUnitPresentationEffectKind::should_dim_flag,key,0}};
    bool mismatched_boolean=false;
    try{apply_bnw_unit_presentation_batch(raw,consumer,host.database(),mismatched);}catch(const std::exception&){mismatched_boolean=true;}
    check(mismatched_boolean && !consumer.unit_info_dirty() && !consumer.dim_flags().at(key) && consumer.client_revision()==0 && consumer.received_effects().empty(),
          "Boolean dim value mismatched actual moves0 or partially published earlier dirty event");
    feed.effects={{BnwUnitPresentationEffectKind::should_dim_flag,key,1},{BnwUnitPresentationEffectKind::dirty_unit_info,key,1},
                  {BnwUnitPresentationEffectKind::dirty_plot_list_buttons,key,1},{BnwUnitPresentationEffectKind::cycle_selection_delta,key,2},
                  {BnwUnitPresentationEffectKind::has_moved_unit,key,1}};
    apply_bnw_unit_presentation_batch(raw,consumer,host.database(),feed);
    check(consumer.dim_flags().at(key) && consumer.unit_info_dirty() && consumer.plot_list_dirty() && consumer.cycle_selection_counter()==2 &&
          consumer.has_moved_unit() && consumer.received_effects().size()==5 && consumer.client_revision()==1 && consumer.world_revision()==feed.after_world_revision,"Actual retained consumer lost effects");
    bool repeated=false;try{apply_bnw_unit_presentation_batch(raw,consumer,host.database(),feed);}catch(const std::exception&){repeated=true;}
    check(repeated && consumer.received_effects().size()==5,"Repeated stale feed was applied twice");
    auto changed=state;scalar(changed.world.all_players->players.at(player).units.units.front().core,"m_eActivityType")=1;
    bool stale=false;try{select_bnw_unit_presentation(changed,client,host.database(),key,client.client_revision());}catch(const std::exception&){stale=true;}
    check(stale && client.client_revision()==1,"Changed unit world epoch accepted old selection owner");
    std::cout<<"PASS selected original-import unit path reset/clear and actual standalone client selection/dim/dirty/cycle feed; stale and partial consumers rejected\n";
}
