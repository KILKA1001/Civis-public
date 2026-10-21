#include "civ5/runtime/bnw_activation_builder_tasking.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>
#include <type_traits>
namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char *why){if(!b)throw std::runtime_error(why);}
auto &named(auto &rows,std::string_view name){using Row=std::remove_reference_t<decltype(rows.front())>;Row *found=nullptr;for(auto &r:rows)if(r.name&&name==r.name){need(!found,"Duplicate builder source field");found=&r;}need(found,"Missing builder source field");return *found;}
void builder_shape(const auto &raw){need(raw.planning.layout==bnw,"Builder planning profile unknown");const auto &b=named(raw.planning.blocks,"builder_tasking_ai");const auto v=named(b.fields,"version").value;need(v==1||v==2,"BNW builder source version unknown");const auto count=named(b.fields,"non_territory_plots_wire_count").value;const auto &a=named(b.arrays,"non_territory_plots");need(count>=0&&static_cast<std::uint64_t>(count)==a.values.size()&&a.columns==1,"Builder serialized vector shape differs");const auto marsh=named(b.fields,"keep_marshes").value;need(marsh==0||marsh==1,"Builder keep marshes flag malformed");if(v>=2){const auto jungle=named(b.fields,"keep_jungle").value;need(jungle==0||jungle==1,"Builder keep jungle flag malformed");}const auto repair=named(b.fields,"repair_build").value;need(repair>=-1&&repair<=INT32_MAX,"Builder repair build source width malformed");}
}
BnwActivationBuilderOwner bind_bnw_activation_builder_original_import(const saves::SavedSession &source,const SessionCheckpointIdentity &id,BnwBuilderRuntimeLogging logs){
 need(source.source.container==saves::SaveContainerKind::civ5&&identify_bytes(source.source.bytes).sha256==id.payload_sha256&&id.rules_sha256.size()==64&&source.snapshot.all_players&&source.snapshot.all_players->layout==bnw&&source.snapshot.all_players->players.size()==64,"Builder Read owner requires actual original-import identity");
 BnwActivationBuilderOwner o;o.identity_=id;o.guid_=source.snapshot.map_header.guid;o.logging_=logs.logging&&logs.ai_logging&&logs.builder_ai_logging;
 o.city_yields_=bind_bnw_city_strategy_yields_original_import(source,id);
 for(const auto &p:source.snapshot.all_players->players){builder_shape(p);need(o.players_.emplace(p.saved_player_id,BnwBuilderTaskingPlayerReadState{}).second,"Duplicate builder Read player");}return o;
}
LiveActionResult update_bnw_activation_builder_tasking(LiveSessionState &state,BnwActivationBuilderOwner &owner,const BnwActivationConnectionsOwner &connections,const BnwCityStrategyYieldRules &rules,std::int64_t player){
 try{
  need(state.setup.profile==content::TechnologyProfile::bnw&&state.cities.profile==content::TechnologyProfile::bnw&&state.world.all_players&&state.world.all_players->layout==bnw&&state.world.all_players->players.size()==64&&state.world.pregame&&state.setup.game_turn==state.world.pregame->archive.integers.at("game_turn")&&player>=0&&player<64,"Builder current registry/clock unknown");
  need(owner.guid_==state.world.map_header.guid&&connections.map_guid==owner.guid_&&connections.identity.payload_sha256==owner.identity_.payload_sha256&&connections.identity.rules_sha256==owner.identity_.rules_sha256&&connections.players.at(player).updated_turn==state.setup.game_turn,"Builder requires SAME current source CityConnections update owner");
  const auto &raw=state.world.all_players->players.at(player);need(raw.saved_player_id==player&&raw.cities.layout==bnw&&raw.cities.header.layout==bnw&&raw.cities.header.wire_city_count==static_cast<std::int64_t>(raw.cities.cities.size())&&raw.cities.cities.size()==state.cities.players.at(player).cities.size(),"Builder current city collection incomplete");builder_shape(raw);
  const auto last=named(raw.cities.header.fields,"last_index").value,free=named(raw.cities.header.fields,"free_list_count").value;
  need(last>=-1&&last<=INT32_MAX&&free>=0&&free<=INT32_MAX&&last-free+1==static_cast<std::int64_t>(raw.cities.cities.size()),"Builder native GetNumCities FLTA count differs from roster");
  const auto num_cities=static_cast<std::int32_t>(last-free+1);
  const auto &c=named(raw.planning.blocks,"city_connections");const auto &ids=named(c.arrays,"city_plot_ids");need(ids.columns==1&&named(c.fields,"city_ids_wire_count").value==static_cast<std::int64_t>(ids.values.size()),"Builder connectable city vector unknown");
  need(ids.values.size()<2,"Builder route planning needs actual best-route/worker/path effects owner");
  need(rules.identity()==owner.identity_.rules_sha256,"Builder current city strategy rules differ");
  need(raw.cities.cities.size()<=INT32_MAX,"Builder native city count overflow");
  need(!owner.logging_,"Builder enabled diagnostic logging requires actual source logger owner");
  const auto &retained=owner.players_.at(player);need(retained.target_is_null&&retained.updates<UINT64_MAX,"Builder retained target/update state unknown");
  auto draft=state;auto staged=owner;auto &builder=named(draft.world.all_players->players.at(player).planning.blocks,"builder_tasking_ai");
  named(builder.arrays,"non_territory_plots").values.clear();named(builder.fields,"non_territory_plots_wire_count").value=0;
  auto &current=staged.players_.at(player);current.num_cities=num_cities;
  std::map<std::int32_t,std::int64_t> native_order;
  for(const auto &city:raw.cities.cities){const auto &id=named(city.core.fields,"m_iID");need(id.values.size()==1&&id.values[0]>=0&&id.values[0]<=INT32_MAX,"Builder current city ID unknown");need(native_order.emplace(static_cast<std::int32_t>(id.values[0])&8191,id.values[0]).second,"Builder duplicate native city slot");}
  for(const auto &[slot,city]:native_order)update_bnw_city_strategy_best_yields(draft,staged.city_yields_,rules,player,city);
  ++current.updates;
  state=std::move(draft);owner=std::move(staged);
  return {LiveActionOutcome::committed,"Actual Builder UpdateRoutePlots vector cleared; actual city count stored and complete native city strategy cache loop on SAME private world"};
 }catch(const std::exception &e){return {LiveActionOutcome::rejected,e.what()};}
}
}
