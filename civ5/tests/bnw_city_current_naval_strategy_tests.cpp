#include "civ5/runtime/bnw_city_current_naval_strategy.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <iostream>
#include <set>
#include <string_view>
#include <thread>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
using T=BnwNavalCityStrategy;
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refuses(F&& f){bool failed=false;try{f();}catch(const BnwCityNavalStrategyUnavailable&){failed=true;}need(failed,"Naval unsupported/stale/malformed source admitted");}
const auto& named(const auto& rows,const char* name){using R=typename std::remove_cvref_t<decltype(rows)>::value_type;const R* out=nullptr;for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Test duplicate source field");out=&r;}need(out,"Test source field absent");return *out;}
std::int64_t city_scalar(const auto& city,const char* name){const auto& f=named(city.core.fields,name);need(f.values.size()==1,"Actual city scalar absent");return f.values[0];}
void info_identity(){
 const content::InfoTypeEntry name{"AICITYSTRATEGY_NEED_NAVAL_GROWTH","AICityStrategies",7,content::saved_info_type_hash("AICITYSTRATEGY_NEED_NAVAL_GROWTH")};
 const auto hash=name;need(&name!=&hash&&detail::bnw_naval_info_same_identity(&name,&hash),"Separate name/hash value copies were treated as different identities");
 need(!detail::bnw_naval_info_same_identity(nullptr,&hash)&&!detail::bnw_naval_info_same_identity(&name,nullptr),"Missing Info index admitted");
 auto bad=hash;bad.type="OTHER";need(!detail::bnw_naval_info_same_identity(&name,&bad),"Info Type mismatch admitted");
 bad=hash;bad.table="Units";need(!detail::bnw_naval_info_same_identity(&name,&bad),"Info table mismatch admitted");
 bad=hash;++bad.id;need(!detail::bnw_naval_info_same_identity(&name,&bad),"Info native ID mismatch admitted");
 bad=hash;++bad.hash;need(!detail::bnw_naval_info_same_identity(&name,&bad),"Info hash mismatch admitted");
 constexpr auto first="TYPE_4978D320478D8056",last="TYPE_5348478E677AAFE1";
 need(content::saved_info_type_hash(first)==content::saved_info_type_hash(last),"Known CRC collision witness changed");
 const content::InfoTypeEntry a{first,"Fixture",8,content::saved_info_type_hash(first)},b{last,"Fixture",13,content::saved_info_type_hash(last)};
 need(!detail::bnw_naval_info_same_identity(&a,&b),"Hash collision silently reconciled different names/IDs");
 std::cout<<"PASS detached Info semantic identity/copy/null/Type/table/ID/hash/collision checks; no issuing authority\n";
}
void actual_info_identity(const ui::SelectedBnwPreviewHost& host){
 const std::array<std::pair<const char*,const char*>,3> queries={{{"AICITYSTRATEGY_NEED_NAVAL_GROWTH","AICityStrategies"},{"AICITYSTRATEGY_NEED_NAVAL_TILE_IMPROVEMENT","AICityStrategies"},{"UNITAI_WORKER_SEA","UnitAIInfos"}}};
 std::size_t separate=0;
 for(const auto [type,table]:queries){
  const auto* name=host.types().find(type);need(name&&name->table==table&&name->id>=0&&name->id<=INT32_MAX,"Actual Naval Type/table/native ID witness unavailable");
  const auto* hash=host.types().find_hash(name->hash);need(detail::bnw_naval_info_same_identity(name,hash),"Actual Naval independent indexes disagree semantically");
  need(name->hash==content::saved_info_type_hash(type),"Actual Naval Type hash witness differs");if(name!=hash)++separate;
  std::size_t rows=0;const auto report=host.database().query_rows({std::string("SELECT ID,Type FROM ")+table+" WHERE ID=?",{name->id}},[&](const auto& row){need(++rows==1&&row.integer("ID")==name->id&&row.text("Type")==name->type,"Actual activated Naval DB row differs from both indexes");},2);
  need(report.all_succeeded()&&report.rows==1&&rows==1,"Actual activated Naval DB witness failed");
  std::cout<<"Actual Naval Info type="<<type<<" table="<<table<<" id="<<name->id<<" hash="<<name->hash<<" separateNodes="<<(name!=hash)<<'\n';
 }
 need(separate>0,"Actual catalog separate-map regression witness unobserved");
}
void arithmetic(){
 const std::array<std::int64_t,2> a={3,4},b={-1,2},cancel={INT32_MAX,-INT32_MAX},ones={1,1};
 need(!detail::bnw_naval_growth_threshold_math(INT64_MAX,0,{},a,INT64_MAX),"Zero-total reached suffix operands");
 need(detail::bnw_naval_growth_threshold_math(1,3,a,b,28),"Native 33>=33 inclusive threshold/truncation lost");
 need(!detail::bnw_naval_growth_threshold_math(1,3,a,b,29),"Native 33 incorrectly rounded to34");
 need(detail::bnw_naval_growth_threshold_math(1,1,cancel,ones,100),"Native representable slot-order cancellation refused");
 const std::array<std::int64_t,3> early={INT32_MAX,1,-1},three={1,1,1};
 refuses([&]{(void)detail::bnw_naval_growth_threshold_math(0,1,early,three,0);});
 const std::array<std::int64_t,1> product={INT32_MAX},twice={2};
 refuses([&]{(void)detail::bnw_naval_growth_threshold_math(0,1,product,twice,0);});
 refuses([&]{(void)detail::bnw_naval_growth_threshold_math(INT32_MAX,1,{}, {},0);});
 refuses([&]{(void)detail::bnw_naval_growth_threshold_math(0,1,a,b,INT32_MAX);});
 refuses([&]{(void)detail::bnw_naval_growth_threshold_math(0,1,a,{},0);});
 const MapGeometry geometry{{9,9},false,false,bnw};const auto p=detail::bnw_citizen_plot_positions(geometry,4,4);
 need(p[0]==40&&p[1]==49&&p[2]==41&&p[3]==31&&p[6]==48&&p[19]==68&&p[36]==67,"Native city37 home/ring ordering differs");
 const std::array<std::int64_t,37> sdk_even={40,49,41,31,30,39,48,59,50,42,32,23,22,21,29,38,47,57,58,68,60,51,43,33,24,14,13,12,11,20,28,37,46,56,65,66,67};
 const std::array<std::int64_t,37> sdk_odd={49,59,50,41,40,48,58,68,60,51,42,32,31,30,39,47,57,66,67,78,69,61,52,43,33,24,23,22,21,29,38,46,56,65,75,76,77};
 const auto odd=detail::bnw_citizen_plot_positions(geometry,4,5);
 for(std::size_t i=0;i<37;++i)need(p[i]==sdk_even[i]&&odd[i]==sdk_odd[i],"All37 SDK native offsets/parity differ");
 std::set<std::int64_t> distinct;for(const auto at:p){need(at.has_value(),"Interior city plot missing");distinct.insert(*at);}need(distinct.size()==37,"Native city radius contains wrong offsets");
 const MapGeometry small{{1,1},false,false,bnw},wrapped{{1,1},true,true,bnw};const auto edge=detail::bnw_citizen_plot_positions(small,0,0);need(edge[0]==0&&!edge[1]&&!edge[2],"Native NULL city edge changed");const auto tiny=detail::bnw_citizen_plot_positions(wrapped,0,0);need(tiny[0]==0&&tiny[1]==0&&tiny[2]==0,"Wrapped city positions deduplicated");
 refuses([&]{(void)detail::bnw_citizen_plot_positions(geometry,0,INT32_MIN);});
 std::cout<<"PASS detached native threshold products/truncation/slot-order overflow and city37 offsets; no issuing authority\n";
}
saves::SavedUnitObservation detached_unit(std::uint64_t index,std::int32_t id){saves::SavedUnitObservation u{};u.index=index;u.core.layout=bnw;saves::UnitCoreField a{},b{};a.name="m_iID";a.encoding=saves::UnitCoreEncoding::i32;a.values={id};b.name="m_eOwner";b.encoding=saves::UnitCoreEncoding::i32;b.values={0};u.core.fields={a,b};return u;}
void lookup(){
 saves::SavedSessionSnapshot world{};need(!detail::bnw_naval_unit_lookup(world,-1,0)&&!detail::bnw_naval_unit_lookup(world,64,0)&&!detail::bnw_naval_unit_lookup(world,0,-1),"Native NULL unit reference touched absent registry");
 world.all_players.emplace();world.all_players->layout=bnw;world.all_players->context.player_count=64;world.all_players->players.resize(64);
 for(std::size_t i=0;i<64;++i){world.all_players->players[i].index=i;world.all_players->players[i].saved_player_id=static_cast<std::int64_t>(i);}
 auto& units=world.all_players->players[0].units;units.layout=bnw;units.header.layout=bnw;units.header.source_slots=4;units.header.fields={{"num_slots",4},{"last_index",2},{"free_list_head",1},{"free_list_count",1},{"current_id",16384}};units.header.next_free_indices={-1,-1,-1,-1};units.header.wire_unit_count=2;units.units={detached_unit(0,8192),detached_unit(1,8194)};
 const auto* full=detail::bnw_naval_unit_lookup(world,0,8192);need(full&&full==detail::bnw_naval_unit_lookup(world,0,0)&&detail::bnw_naval_unit_lookup(world,0,2)==&units.units[1],"Native full/index GetAt lookup differs");
 need(!detail::bnw_naval_unit_lookup(world,0,1)&&!detail::bnw_naval_unit_lookup(world,0,16384)&&!detail::bnw_naval_unit_lookup(world,0,INT32_MIN),"Native free/stale/negative unit handle did not return NULL");
 refuses([&]{(void)detail::bnw_naval_unit_lookup(world,0,INT64_MAX);});auto invalid=world;invalid.all_players->players[0].units.units[1].core.fields[0].values={8192};refuses([&]{(void)detail::bnw_naval_unit_lookup(invalid,0,0);});
 invalid=world;invalid.all_players->players[0].units.header.next_free_indices[1]=1;refuses([&]{(void)detail::bnw_naval_unit_lookup(invalid,0,0);});invalid=world;invalid.all_players->players[0].units.header.wire_unit_count=3;refuses([&]{(void)detail::bnw_naval_unit_lookup(invalid,0,0);});
 std::cout<<"PASS detached native unit GetAt NULL/full/index/stale/duplicate/free-chain cases; no current owner\n";
}
std::shared_ptr<ui::SelectedBnwPreviewHost> boot(const std::filesystem::path& save,const std::filesystem::path& game){auto b=ui::bootstrap_selected_bnw_preview(save,game,0,true);if(b.preview.blocker)throw std::runtime_error(b.preview.blocker->reason);std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(b.preview.host));need(bool(host),"Actual immutable host absent");return host;}
void genuine(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto before=identify_file(save).sha256;auto host=boot(save,game);actual_info_identity(*host);const auto& s=host->session().state();need(s.world.all_players&&s.world.map_plots&&s.world.map_projection,"Actual map/player source absent");std::size_t checked=0,water=0,workable=0;std::array<std::size_t,4> positives{};std::int64_t first_owner=-1,first_city=-1;std::array<bool,4> first{};
 for(const auto& p:s.world.all_players->players){for(const auto& c:p.cities.cities){if(checked==24)break;const auto owner=city_scalar(c,"m_eOwner"),id=city_scalar(c,"m_iID");auto full=inspect_bnw_city_current_naval_strategy(host,owner,id),index=inspect_bnw_city_current_naval_strategy(host,owner,id&8191);std::array<bool,4> values{};
  for(std::size_t n=0;n<4;++n){values[n]=full.value(static_cast<T>(n));need(values[n]==index.value(static_cast<T>(n)),"Actual full/index naval result differs");if(values[n])++positives[n];}
  const MapGeometry g{s.world.map_header.grid,s.world.map_header.wrap_x,s.world.map_header.wrap_y,bnw};const auto positions=detail::bnw_citizen_plot_positions(g,city_scalar(c,"m_iX"),city_scalar(c,"m_iY"));bool blocked=false;
  for(std::size_t slot=0;slot<positions.size();++slot){if(!positions[slot])continue;const auto plot=static_cast<std::uint64_t>(*positions[slot]);if(slot>0&&!blocked)blocked=full.plot_blockaded(plot);const bool can=full.can_work(plot);if(can)++workable;const auto& cell=s.world.map_projection->cells.at(static_cast<std::size_t>(plot));if(cell.plot_type&&*cell.plot_type==3)++water;}
  need(blocked==values[0],"Actual city blockade did not skip HOME/native first-true order");refuses([&]{(void)full.value(static_cast<T>(99));});need(full.value(T::under_blockade)==values[0],"Unsupported selector retired healthy owner");
  if(checked==0){first_owner=owner;first_city=id;first=values;}++checked;
 }if(checked==24)break;}
 need(checked>0&&workable>0,"No actual city/working-plot observations");for(std::size_t n=0;n<4;++n)std::cout<<"Genuine trigger="<<n<<" positives="<<positives[n]<<(positives[n]?" observed":" UNOBSERVED")<<'\n';std::cout<<"Genuine reachable water positions="<<water<<" workable="<<workable<<'\n';
 auto retained=inspect_bnw_city_current_naval_strategy(host,first_owner,first_city);bool read=false,bind=false;std::thread worker([&]{try{(void)retained.value(T::under_blockade);}catch(const BnwCityNavalStrategyUnavailable&){read=true;}try{(void)inspect_bnw_city_current_naval_strategy(host,first_owner,first_city);}catch(const BnwCityNavalStrategyUnavailable&){bind=true;}});worker.join();need(read&&bind&&retained.value(T::under_blockade)==first[0],"Foreign thread entered source or poisoned owner");
 refuses([&]{(void)inspect_bnw_city_current_naval_strategy({},0,0);});refuses([&]{(void)inspect_bnw_city_current_naval_strategy(host,INT64_MAX,first_city);});const auto cp=host->session().checkpoint();host->session().restore(cp);refuses([&]{(void)retained.value(T::under_blockade);});auto restored=inspect_bnw_city_current_naval_strategy(host,first_owner,first_city);need(restored.value(T::under_blockade)==first[0],"Healthy CP changed blockade");
 auto generic=LiveSession::from_restored_bnw(host->session().source(),host->session().state());auto moved=std::make_shared<LiveSession>(std::move(host->session()));refuses([&]{(void)restored.value(T::under_blockade);});host->session()=std::move(generic);refuses([&]{(void)inspect_bnw_city_current_naval_strategy(host,first_owner,first_city);});host.reset();moved.reset();
 auto other=boot(save,game);auto strong=inspect_bnw_city_current_naval_strategy(other,first_owner,first_city);const auto weak=std::weak_ptr<ui::SelectedBnwPreviewHost>(other);other.reset();need(!weak.expired()&&strong.value(T::under_blockade)==first[0],"Naval owner lost sole actual host/session/rules parent");auto writer=weak.lock();need(bool(writer),"Strong host vanished");(void)writer->session().mutable_state();refuses([&]{(void)strong.value(T::under_blockade);});need(identify_file(save).sha256==before,"Original selected save changed");std::cout<<"PASS actual naval reads="<<checked<<"; no strategy/Lua/FlavorUpdate/city-phase/freshness grant\n";
}
}
static_assert(!std::is_default_constructible_v<BnwCityCurrentNavalStrategy>);
int main(int argc,char** argv){try{need(argc==3,"Expected selected save and shared canonical installation");info_identity();arithmetic();lookup();genuine(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
