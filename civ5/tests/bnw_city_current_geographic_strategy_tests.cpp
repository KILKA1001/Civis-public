#include "civ5/runtime/bnw_city_current_geographic_strategy.hpp"
#include "civ5/runtime/bnw_city_entry.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <array>
#include <climits>
#include <iostream>
#include <string_view>
#include <thread>
#include <type_traits>
#include <vector>
using namespace civ5;using namespace civ5::runtime;
namespace {
using T=BnwGeographicCityStrategy;constexpr std::array<T,5> supported{T::river,T::mountain,T::hill,T::forest,T::jungle};
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refuses(F&& f){bool b=false;try{f();}catch(const BnwCityGeographicStrategyUnavailable&){b=true;}need(b,"Geographic unsupported/generic/stale source admitted");}
const auto& named(const auto& rows,std::string_view name){using R=typename std::remove_cvref_t<decltype(rows)>::value_type;const R* out=nullptr;for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Duplicate actual geographic fixture field");out=&r;}need(out,"Actual geographic fixture field absent");return *out;}
std::int64_t scalar(const auto& rows,const char* name){const auto& f=named(rows,name);need(f.values.size()==1,"Actual scalar absent");return f.values.front();}
void pure(){
 const MapGeometry g{{7,7},false,false,saves::GameStateLayout::expansion2_version1};const auto home=*g.plot(3,3);
 std::vector<BnwGeographicPlotMath> plots(49);for(auto& p:plots)p={2,0,-1,0};
 const auto calc=[&](T t){return bnw_city_geographic_strategy_math(t,g,3,3,plots);};
 need(!calc(T::river),"River zero wrong");plots.at(static_cast<std::size_t>(home)).river_crossing_count=-1;need(!calc(T::river),"Negative crossing count invented a river");plots.at(static_cast<std::size_t>(home)).river_crossing_count=1;need(calc(T::river),"Positive crossing count lost");
 plots.at(static_cast<std::size_t>(home)).river_crossing_count=128;refuses([&]{(void)calc(T::river);});plots.at(static_cast<std::size_t>(home)).river_crossing_count=0;
 const auto east=*g.direction(3,3,1);plots.at(static_cast<std::size_t>(east)).plot_type=0;plots.at(static_cast<std::size_t>(east)).owner.reset();need(calc(T::mountain),"Mountain incorrectly read owner");plots.at(static_cast<std::size_t>(east))={2,0,-1,0};
 const auto positions=detail::bnw_city_feature_positions(g,3,3);std::vector<std::int64_t> reached;for(const auto n:positions)if(n)reached.push_back(*n);need(reached.size()==19,"Native hex-range2 slot geometry changed");
 const auto first=static_cast<std::size_t>(reached.at(0)),second=static_cast<std::size_t>(reached.at(1));
 for(const auto t:{T::hill,T::forest,T::jungle}){
  auto reset=plots;for(auto& p:plots)p={2,std::nullopt,-1,0};plots.at(static_cast<std::size_t>(home)).owner=0;
  const auto mark=[&](std::size_t id){if(t==T::hill)plots.at(id).plot_type=1;else plots.at(id).feature=t==T::forest?5:1;plots.at(id).owner=0;};
  need(!calc(t),"Nonmatching slots demanded unrelated owners");mark(first);need(!calc(t),"One matching slot activated strategy");mark(second);need(calc(t),"Second matching owned slot did not return");
  plots.at(second).owner=1;need(!calc(t),"Different owner incorrectly counted");plots.at(second).owner=0;
  // Every later type/feature is absent. Source must stop immediately at2.
  for(std::size_t i=2;i<reached.size();++i){const auto id=static_cast<std::size_t>(reached[i]);if(t==T::hill)plots.at(id).plot_type.reset();else plots.at(id).feature.reset();}
  need(calc(t),"Read later missing operands after native second match");plots=std::move(reset);
 }
 const MapGeometry wrapped{{1,1},true,true,saves::GameStateLayout::expansion2_version1};std::vector<BnwGeographicPlotMath> one{{1,0,5,0}};
 need(bnw_city_geographic_strategy_math(T::hill,wrapped,0,0,one)&&bnw_city_geographic_strategy_math(T::forest,wrapped,0,0,one),"Wrapped slot multiplicity was deduplicated");
 const MapGeometry edge{{1,1},false,false,saves::GameStateLayout::expansion2_version1};need(!bnw_city_geographic_strategy_math(T::hill,edge,0,0,one)&&!bnw_city_geographic_strategy_math(T::mountain,edge,0,0,one),"NULL slots invented matches");
 const MapGeometry vp{{7,7},false,false,saves::GameStateLayout::vp542_corpus};refuses([&]{(void)bnw_city_geographic_strategy_math(T::river,vp,3,3,plots);});
 refuses([&]{(void)calc(T::coast);});refuses([&]{(void)calc(T::landlocked);});refuses([&]{(void)calc(static_cast<T>(99));});
 std::cout<<"PASS pure native range/direction/null/wrap/countstop/type-before-owner and river-count cases; no issuer\n";
}
std::shared_ptr<ui::SelectedBnwPreviewHost> boot(const std::filesystem::path& save,const std::filesystem::path& game){auto b=ui::bootstrap_selected_bnw_preview(save,game,0,true);if(b.preview.blocker)throw std::runtime_error(b.preview.blocker->reason);std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(b.preview.host));need(bool(host),"Actual immutable host absent");return host;}
void genuine(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto before=identify_file(save).sha256;auto host=boot(save,game);const auto& s=host->session().state();need(s.world.map_projection&&s.world.map_plots&&s.world.all_players,"Actual source map/players absent");
 const auto& h=s.world.map_header;const MapGeometry geometry{h.grid,h.wrap_x,h.wrap_y,saves::GameStateLayout::expansion2_version1};std::vector<BnwGeographicPlotMath> facts;facts.reserve(s.world.map_plots->plots.size());
 // Factual pure witness only. It cannot create an owner or replace source
 // operands of any real query. All owner factories receive the genuine host.
 for(const auto& p:s.world.map_plots->plots){std::int64_t feature=-1;need(p.version.has_value(),"Actual native plot version absent");if(*p.version>=3){const auto hash=named(p.fields,"feature_type_hash").value;need(hash>=0&&static_cast<std::uint64_t>(hash)<=UINT32_MAX,"Actual feature hash outside width");if(hash){const auto* info=host->types().find_hash(static_cast<std::uint64_t>(hash));need(info&&info->table=="Features","Actual feature witness lacks Info");feature=info->id;}}else feature=named(p.fields,"feature_type").value;
  facts.push_back({named(p.fields,"plot_type").value,named(p.fields,"owner").value,feature,named(p.fields,"river_crossing_count").value});
 }
 std::array<std::size_t,5> positive{};std::size_t checked=0;
 for(const auto& player:s.world.all_players->players){for(const auto& c:player.cities.cities){if(checked==24){break;}const auto owner=scalar(c.core.fields,"m_eOwner"),id=scalar(c.core.fields,"m_iID"),x=scalar(c.core.fields,"m_iX"),y=scalar(c.core.fields,"m_iY");need(id>=0&&id<=INT32_MAX,"Actual city handle outside native width");
  auto full=inspect_bnw_city_current_geographic_strategy(host,owner,id);auto index=inspect_bnw_city_current_geographic_strategy(host,owner,id&8191);
  for(std::size_t t=0;t<supported.size();++t){const auto actual=full.value(supported[t]);need(actual==index.value(supported[t])&&actual==bnw_city_geographic_strategy_math(supported[t],geometry,x,y,facts),"Genuine geometric predicate differs from exact source witness/full-index lookup");if(actual){++positive[t];}}
  refuses([&]{(void)full.value(T::coast);});need(full.value(T::river)==index.value(T::river),"Supported query retired after explicit coast frontier");
  const auto stale=(id&~std::int64_t{8191})==8192?(id&8191)|16384:(id&8191)|8192;refuses([&]{(void)inspect_bnw_city_current_geographic_strategy(host,owner,stale);});++checked;
 }if(checked==24){break;}}
 need(checked>0,"No genuine city geographic observations");for(std::size_t t=0;t<supported.size();++t)std::cout<<"Genuine geographic trigger"<<t<<" positive="<<positive[t]<<(positive[t]==0?" UNOBSERVED-positive":" observed-positive")<<"\n";
 const auto id=scalar(s.world.all_players->players.at(0).cities.cities.at(0).core.fields,"m_iID");auto retained=inspect_bnw_city_current_geographic_strategy(host,0,id);const auto first=retained.value(T::river);
 bool read=false,bind=false;std::thread worker([&]{try{(void)retained.value(T::river);}catch(const BnwCityGeographicStrategyUnavailable&){read=true;}try{(void)inspect_bnw_city_current_geographic_strategy(host,0,id);}catch(const BnwCityGeographicStrategyUnavailable&){bind=true;}});worker.join();need(read&&bind&&retained.value(T::river)==first,"Foreign access reached source or poisoned actual owner");
 refuses([&]{(void)inspect_bnw_city_current_geographic_strategy({},0,id);});refuses([&]{(void)inspect_bnw_city_current_geographic_strategy(host,0,INT64_MAX);});refuses([&]{(void)inspect_bnw_city_current_geographic_strategy(host,INT64_MIN,id);});
 const auto cp=host->session().checkpoint();host->session().restore(cp);refuses([&]{(void)retained.value(T::river);});auto restored=inspect_bnw_city_current_geographic_strategy(host,0,id);need(restored.value(T::river)==first,"Healthy restore changed predicate");
 auto generic=LiveSession::from_restored_bnw(host->session().source(),host->session().state());auto moved=std::make_shared<LiveSession>(std::move(host->session()));refuses([&]{(void)restored.value(T::river);});refuses([&]{(void)inspect_bnw_city_current_geographic_strategy(host,0,id);});host->session()=std::move(generic);refuses([&]{(void)inspect_bnw_city_current_geographic_strategy(host,0,id);});moved.reset();host.reset();
 auto other=boot(save,game);auto strong=inspect_bnw_city_current_geographic_strategy(other,0,id);const auto weak=std::weak_ptr<ui::SelectedBnwPreviewHost>(other);other.reset();need(!weak.expired()&&strong.value(T::river)==first,"Genuine read lost strong source/DB/host lifetime");auto writer=weak.lock();need(bool(writer),"Actual retained host absent");(void)writer->session().mutable_state();refuses([&]{(void)strong.value(T::river);});
 need(identify_file(save).sha256==before,"Original selected save changed");std::cout<<"PASS genuine geographic city reads="<<checked<<"; no strategy activation/cache recomputation/phase completion\n";
}
}
static_assert(!std::is_default_constructible_v<BnwCityCurrentGeographicStrategy>);
int main(int argc,char** argv){try{need(argc==3,"Expected selected save and canonical shared installation");pure();genuine(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
