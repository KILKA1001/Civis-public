#include "civ5/runtime/bnw_city_current_coastal_strategy.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/bnw_live_plot_impassability.hpp"
#include "civ5/runtime/plot_water.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <iostream>
#include <thread>
#include <string_view>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
using T=BnwCoastalCityStrategy;constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refuses(F&& f){bool b=false;try{f();}catch(const BnwCityCoastalStrategyUnavailable&){b=true;}need(b,"Coastal unsupported/generic/stale source admitted");}
const auto& named(const auto& rows,const char* name){using R=typename std::remove_cvref_t<decltype(rows)>::value_type;const R* out=nullptr;for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Duplicate actual coastal field");out=&r;}need(out,"Actual coastal field absent");return *out;}
std::int64_t scalar(const auto& rows,const char* name){const auto& f=named(rows,name);need(f.values.size()==1,"Actual city scalar absent");return f.values.front();}
void lookup(){
 // Explicit detached native-container math. It never constructs a genuine owner.
 saves::SavedSessionSnapshot world{};need(!detail::bnw_coastal_landmass_lookup(world,-1),"INVALID_INDEX borrowed absent FLTA");
 refuses([&]{(void)detail::bnw_coastal_landmass_lookup(world,0);});world.map_regions.emplace();world.map_regions->layout=bnw;
 saves::MapRegionContainer c{};c.name="landmasses";c.legacy_metadata={8,2,1,1,8192};c.next_free.assign(8,-1);c.wire_count=2;
 saves::MapRegionRecord a{},b{};a.fields={{"id",8192}};b.fields={{"id",8194}};c.records={a,b};world.map_regions->containers.push_back(c);
 const auto* full=detail::bnw_coastal_landmass_lookup(world,8192);need(full&&full==detail::bnw_coastal_landmass_lookup(world,0),"Native index-only/full handle mismatch");
 need(!detail::bnw_coastal_landmass_lookup(world,1)&&!detail::bnw_coastal_landmass_lookup(world,16384)&&!detail::bnw_coastal_landmass_lookup(world,INT32_MIN)&&!detail::bnw_coastal_landmass_lookup(world,INT32_MAX),"Native stale/empty/full sentinel produced record");
 refuses([&]{(void)detail::bnw_coastal_landmass_lookup(world,INT64_MIN);});refuses([&]{(void)detail::bnw_coastal_landmass_lookup(world,INT64_MAX);});
 auto invalid=world;invalid.map_regions->containers.at(0).records.at(1).fields.at(0).value=16384;refuses([&]{(void)detail::bnw_coastal_landmass_lookup(invalid,0);});
 invalid=world;invalid.map_regions->containers.at(0).wire_count=3;refuses([&]{(void)detail::bnw_coastal_landmass_lookup(invalid,0);});
 invalid=world;invalid.map_regions->layout=saves::GameStateLayout::vp542_corpus;refuses([&]{(void)detail::bnw_coastal_landmass_lookup(invalid,0);});
 std::cout<<"PASS detached native landmass GetAt full/index/stale/null/width/shape; no issuer\n";
}
std::shared_ptr<ui::SelectedBnwPreviewHost> boot(const std::filesystem::path& save,const std::filesystem::path& game){auto b=ui::bootstrap_selected_bnw_preview(save,game,0,true);if(b.preview.blocker)throw std::runtime_error(b.preview.blocker->reason);std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(b.preview.host));need(bool(host),"Actual immutable host absent");return host;}
void helper_math(const ui::SelectedBnwPreviewHost& host){
 // Paired detached pure helper inputs use real loaded Rules/Info/Defines but no
 // current owner. Categories/handles/counts below are deliberate math variants.
 const auto& s=host.session().state();auto map=*s.world.map_projection;map.grid={5,5};map.wrap_x=map.wrap_y=false;map.cells.clear();
 for(std::uint64_t id=0;id<25;++id){saves::SavedMapCellProjection p{};p.index=id;p.x=id%5;p.y=id/5;p.plot_type=2;p.landmass=0;map.cells.push_back(p);}
 auto pass=bnw_live_plot_impassability(s);pass.plots.resize(25);PlotWaterState masses;masses.layout=bnw;masses.landmasses[8192]={10,true};const auto rules=load_plot_water_rules(host.database(),content::TechnologyProfile::bnw);
 PlotWaterContext ctx{map,masses,rules,host.types(),pass,s.setup,s.cities,host.defines()};const MapGeometry g{map.grid,false,false,bnw};const auto n=*g.direction(2,2,0);const auto index=static_cast<std::size_t>(n);map.cells.at(index).plot_type=3;map.cells.at(index).landmass=8192;
 need(plot_is_coastal_land(ctx,12,10)&&!plot_is_coastal_land(ctx,12,11),"Native >= tiles threshold lost");masses.landmasses.at(8192).water=false;need(plot_is_coastal_land(ctx,12,10),"Native coast improperly requires landmass water bool");
 map.cells.at(index).landmass.reset();need(plot_is_coastal_land(ctx,12,0)&&plot_is_coastal_land(ctx,12,-2),"Nonpositive minimum read landmass");
 map.cells.at(index).landmass=16384;need(!plot_is_coastal_land(ctx,12,10),"Missing landmass became ocean");map.cells.at(index).landmass=8192;
 // Poison later WATER-category operand: first definitive true must stop.
 const auto later=*g.direction(2,2,1);map.cells.at(static_cast<std::size_t>(later)).plot_type.reset();need(plot_is_coastal_land(ctx,12,10),"Later field demanded after first true");
 map.cells.at(12).plot_type=3;map.cells.at(12).landmass.reset();need(!plot_is_coastal_land(ctx,12,-1),"Home water did not return false before landmass/default");
 map.grid={1,1};map.cells.resize(1);map.cells.at(0).index=map.cells.at(0).x=map.cells.at(0).y=0;map.cells.at(0).plot_type=2;pass.plots.resize(1);need(!plot_is_coastal_land(ctx,0,0),"NULL neighbours invented coast");map.wrap_x=map.wrap_y=true;need(!plot_is_coastal_land(ctx,0,0),"Wrapped land self invented water");
 std::cout<<"PASS existing BNW water helper detached native threshold/null/wrap/early-stop cases; no current authority\n";
}
void genuine(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto before=identify_file(save).sha256;auto host=boot(save,game);helper_math(*host);const auto& s=host->session().state();need(s.world.map_regions&&s.world.all_players&&s.world.map_projection,"Actual source map/regions/cities absent");
 std::size_t checked=0,coasts=0,landlocked=0;std::int64_t first_owner=-1,first_city=-1;bool first=false;
 for(const auto& player:s.world.all_players->players){for(const auto& c:player.cities.cities){if(checked==24){break;}const auto owner=scalar(c.core.fields,"m_eOwner"),id=scalar(c.core.fields,"m_iID");need(id>=0&&id<=INT32_MAX,"Actual city handle outside native width");
  auto full=inspect_bnw_city_current_coastal_strategy(host,owner,id),index=inspect_bnw_city_current_coastal_strategy(host,owner,id&8191);const bool coast=full.value(T::coast),land=full.value(T::landlocked);need(coast==index.value(T::coast)&&land==index.value(T::landlocked)&&coast!=land,"Actual full/index Coast/Landlocked complement differs");
  const auto stale=(id&~std::int64_t{8191})==8192?(id&8191)|16384:(id&8191)|8192;refuses([&]{(void)inspect_bnw_city_current_coastal_strategy(host,owner,stale);});refuses([&]{(void)full.value(static_cast<T>(99));});need(full.value(T::coast)==coast,"Unsupported enum poisoned healthy owner");
  if(checked==0){first_owner=owner;first_city=id;first=coast;}++checked;if(coast){++coasts;}else{++landlocked;}
 }if(checked==24){break;}}
 need(checked>0,"No actual coastal observations");std::cout<<"Genuine Coast positives="<<coasts<<(coasts?" observed":" UNOBSERVED")<<" Landlocked positives="<<landlocked<<(landlocked?" observed":" UNOBSERVED")<<'\n';
 auto retained=inspect_bnw_city_current_coastal_strategy(host,first_owner,first_city);bool read=false,bind=false;std::thread worker([&]{try{(void)retained.value(T::coast);}catch(const BnwCityCoastalStrategyUnavailable&){read=true;}try{(void)inspect_bnw_city_current_coastal_strategy(host,first_owner,first_city);}catch(const BnwCityCoastalStrategyUnavailable&){bind=true;}});worker.join();need(read&&bind&&retained.value(T::coast)==first,"Foreign access entered source or poisoned actual owner");
 refuses([&]{(void)inspect_bnw_city_current_coastal_strategy({},0,0);});refuses([&]{(void)inspect_bnw_city_current_coastal_strategy(host,INT64_MAX,first_city);});refuses([&]{(void)inspect_bnw_city_current_coastal_strategy(host,first_owner,INT64_MIN);});
 const auto cp=host->session().checkpoint();host->session().restore(cp);refuses([&]{(void)retained.value(T::coast);});auto restored=inspect_bnw_city_current_coastal_strategy(host,first_owner,first_city);need(restored.value(T::coast)==first,"Healthy CP restore changed Coast");
 auto generic=LiveSession::from_restored_bnw(host->session().source(),host->session().state());auto moved=std::make_shared<LiveSession>(std::move(host->session()));refuses([&]{(void)restored.value(T::coast);});refuses([&]{(void)inspect_bnw_city_current_coastal_strategy(host,first_owner,first_city);});host->session()=std::move(generic);refuses([&]{(void)inspect_bnw_city_current_coastal_strategy(host,first_owner,first_city);});moved.reset();host.reset();
 auto other=boot(save,game);auto strong=inspect_bnw_city_current_coastal_strategy(other,first_owner,first_city);const auto weak=std::weak_ptr<ui::SelectedBnwPreviewHost>(other);other.reset();need(!weak.expired()&&strong.value(T::coast)==first,"Current owner lost sole strong host/DB/session");auto writer=weak.lock();need(bool(writer),"Retained actual host absent");(void)writer->session().mutable_state();refuses([&]{(void)strong.value(T::coast);});
 need(identify_file(save).sha256==before,"Original selected save changed");std::cout<<"PASS genuine current city coastal reads="<<checked<<"; no strategy activation/city phase/cache freshness\n";
}
}
static_assert(!std::is_default_constructible_v<BnwCityCurrentCoastalStrategy>);
int main(int argc,char** argv){try{need(argc==3,"Expected selected save and canonical shared installation");lookup();genuine(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
