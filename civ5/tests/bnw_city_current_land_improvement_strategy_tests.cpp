#include "civ5/runtime/bnw_city_current_land_improvement_strategy.hpp"
#include "civ5/runtime/bnw_city_current_naval_strategy.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <iostream>
#include <limits>
#include <string_view>
#include <thread>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;using T=BnwLandImprovementCityStrategy;
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refuses(F&& f){bool denied=false;try{f();}catch(const BnwCityLandImprovementUnavailable&){denied=true;}need(denied,"Land improvement stale/malformed/unsupported query admitted");}
const auto& named(const auto& rows,const char* key){using R=typename std::remove_cvref_t<decltype(rows)>::value_type;const R* out=nullptr;for(const auto& r:rows)if(r.name&&std::string_view(r.name)==key){need(!out,"Duplicate fixture field");out=&r;}need(out,"Actual fixture field absent");return *out;}
std::int64_t scalar(const auto& city,const char* key){const auto& f=named(city.core.fields,key);need(f.values.size()==1,"Actual city scalar absent");return f.values[0];}
void math(){
 need(detail::bnw_city_deficient_yield_math(2.0,5,3),"Actual double 5/3 average should be deficient");
 need(!detail::bnw_city_deficient_yield_math(2.0,6,3),"Equal average wrongly deficient");
 need(!detail::bnw_city_deficient_yield_math(static_cast<double>(0.8f),4,5),"Native desired FLOAT widening changed rounded80 equality");
 need(detail::bnw_city_deficient_yield_math(-0.004,-1,200),"Negative half must round away from zero");
 need(!detail::bnw_city_deficient_yield_math(-0.005,-1,200),"Negative half equal desired must not be deficient");
 need(detail::bnw_city_deficient_yield_math(0.005,0,0),"Positive half must round away from zero");
 need(!detail::bnw_city_deficient_yield_math(0.0,INT32_MIN,0),"Zero tiles ratio must not consume amount as a yield");
 refuses([&]{(void)detail::bnw_city_deficient_yield_math(std::numeric_limits<double>::infinity(),1,1);});
 refuses([&]{(void)detail::bnw_city_deficient_yield_math(std::numeric_limits<double>::quiet_NaN(),1,1);});
 refuses([&]{(void)detail::bnw_city_deficient_yield_math(1.0,INT32_MAX,1);});
 refuses([&]{(void)detail::bnw_city_deficient_yield_math(1.0,0,INT64_MAX);});
 need(!detail::bnw_need_land_worker_pressure_math(0,67,1,0,30,30,100),"Desperate equality must not activate");
 need(detail::bnw_need_land_worker_pressure_math(0,67,1,0,31,30,100),"Desperate strictly later must activate");
 need(!detail::bnw_need_land_worker_pressure_math(5,67,1,0,INT64_MAX,INT64_MAX,INT64_MAX),"Unreached desperate suffix consumed");
 need(detail::bnw_need_land_worker_pressure_math(3,67,1,1,46,30,150),"Modified workers integer truncation/feature cities changed");
 refuses([&]{(void)detail::bnw_need_land_worker_pressure_math(INT32_MAX,2,1,0,0,0,0);});
 refuses([&]{(void)detail::bnw_need_land_worker_pressure_math(0,0,INT32_MAX,1,0,0,0);});
 refuses([&]{(void)detail::bnw_need_land_worker_pressure_math(0,0,1,0,0,INT32_MAX,2);});
 const MapGeometry g{{9,9},false,false,bnw};const auto positions=detail::bnw_citizen_plot_positions(g,4,4);
 for(std::size_t i=0;i<positions.size();++i){need(positions[i].has_value(),"Interior native37 plot absent");const auto xy=g.grid.coordinates(static_cast<std::uint64_t>(*positions[i]));need(detail::bnw_citizen_working_plot_index(g,4,4,static_cast<std::int64_t>(xy[0]),static_cast<std::int64_t>(xy[1]))==static_cast<std::int32_t>(i),"Native inverse XYCityPlot mapping differs");}
 need(detail::bnw_citizen_working_plot_index(g,4,4,8,4)==-1,"City radius4 incorrectly counted");
 const MapGeometry tiny{{1,1},true,true,bnw};need(detail::bnw_citizen_working_plot_index(tiny,0,0,0,0)==0,"Wrapped duplicate cell must map shortest HOME index");
 const MapGeometry seam{{8,8},true,true,bnw};need(detail::bnw_citizen_working_plot_index(seam,0,0,7,0)==5,"Wrapped west index wrong");
 refuses([&]{(void)detail::bnw_citizen_working_plot_index(g,INT32_MIN,0,0,0);});
 std::cout<<"PASS prepared pure native rounding/average/pressure/source-short-circuit and inverse native37; no issuance\n";
}
std::shared_ptr<ui::SelectedBnwPreviewHost> boot(const std::filesystem::path& save,const std::filesystem::path& game){auto b=ui::bootstrap_selected_bnw_preview(save,game,0,true);if(b.preview.blocker)throw std::runtime_error(b.preview.blocker->reason);std::shared_ptr<ui::SelectedBnwPreviewHost> h(std::move(b.preview.host));need(bool(h),"Actual immutable host absent");return h;}
bool known_frontier(const std::string& e){return e.find("owner unavailable")!=std::string::npos||e.find("native Reset/Read owner")!=std::string::npos||e.find("native Init/Reset/Read owner")!=std::string::npos||e.find("hidden resource requires genuine debug owner join")!=std::string::npos||e.find("NaturalWonder")!=std::string::npos;}
void actual(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto before=identify_file(save).sha256;auto host=boot(save,game);std::int64_t who=-1,full=-1;std::uint64_t cities=0,complete=0,frontiers=0;
 for(const auto& p:host->session().state().world.all_players->players)for(const auto& c:p.cities.cities){if(cities>=24)break;const auto owner=scalar(c,"m_eOwner"),id=scalar(c,"m_iID");if(who==-1){who=owner;full=id;}auto counts=inspect_bnw_city_current_naval_strategy(host,owner,id);const auto workers=counts.unit_count(BnwCityNativeUnitCount::workers),settlers=counts.unit_count(BnwCityNativeUnitCount::settlers),all_settlers=counts.unit_count(BnwCityNativeUnitCount::settlers_including_water);need(workers>=0&&settlers>=0&&all_settlers>=settlers,"Native unit count selectors violated water inclusion");
  for(const auto type:{T::need_tile_improvers,T::want_tile_improvers,T::enough_tile_improvers,T::need_improvement_food,T::need_improvement_production}){auto q=inspect_bnw_city_current_land_improvement_strategy(host,owner,id);auto index=inspect_bnw_city_current_land_improvement_strategy(host,owner,id&8191);try{const bool a=q.value(type);need(a==index.value(type),"Actual full/index city query differs");++complete;std::cout<<"actual owner="<<owner<<" city="<<id<<" strategy="<<static_cast<int>(type)<<" value="<<a<<"\n";}catch(const BnwCityLandImprovementUnavailable& e){need(known_frontier(e.what()),e.what());++frontiers;std::cout<<"mandatory frontier owner="<<owner<<" city="<<id<<" "<<e.what()<<"\n";}}
  ++cities;
 }
 need(cities>0&&who>=0&&complete>0,"Actual selected map failed to reach any implemented branch");
 auto retained=inspect_bnw_city_current_land_improvement_strategy(host,who,full);const bool expected=retained.value(T::need_tile_improvers);bool wrong_thread=false;std::thread worker([&]{try{(void)retained.value(T::need_tile_improvers);}catch(const BnwCityLandImprovementUnavailable&){wrong_thread=true;}});worker.join();need(wrong_thread&&retained.value(T::need_tile_improvers)==expected,"Foreign thread did not reject before world borrow");
 refuses([&]{(void)inspect_bnw_city_current_land_improvement_strategy({},who,full);});refuses([&]{(void)inspect_bnw_city_current_land_improvement_strategy(host,INT64_MAX,full);});
 const auto cp=host->session().checkpoint();host->session().restore(cp);refuses([&]{(void)retained.value(T::need_tile_improvers);});auto current=inspect_bnw_city_current_land_improvement_strategy(host,who,full);need(current.value(T::need_tile_improvers)==expected,"Healthy CP changed current native predicate");
 auto generic=LiveSession::from_restored_bnw(host->session().source(),host->session().state());auto moved=std::make_shared<LiveSession>(std::move(host->session()));refuses([&]{(void)current.value(T::need_tile_improvers);});host->session()=std::move(generic);refuses([&]{(void)inspect_bnw_city_current_land_improvement_strategy(host,who,full);});host.reset();moved.reset();
 auto other=boot(save,game);auto strong=inspect_bnw_city_current_land_improvement_strategy(other,who,full);const auto weak=std::weak_ptr<ui::SelectedBnwPreviewHost>(other);other.reset();need(!weak.expired()&&strong.value(T::need_tile_improvers)==expected,"Land query lost sole Host/session/rules parents");auto writer=weak.lock();need(bool(writer),"Sole genuine Host vanished");(void)writer->session().mutable_state();refuses([&]{(void)strong.value(T::need_tile_improvers);});need(identify_file(save).sha256==before,"Original selected save changed");std::cout<<"PASS actual branches="<<complete<<" named frontiers="<<frontiers<<" cities="<<cities<<"; no strategy/Lua/mutation/freshness receipt\n";
}
}
int main(int argc,char** argv){try{math();if(argc!=3)throw std::runtime_error("Expected selected save and shared installation");actual(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<"FAIL "<<e.what()<<"\n";return 1;}}
