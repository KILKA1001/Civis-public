#include "civ5/runtime/bnw_city_population.hpp"
#include "civ5/runtime/bnw_city_food_growth_prefix.hpp"
#include <climits>
namespace civ5::runtime {
namespace {void need(bool b,const char* m){if(!b)throw BnwCityPopulationUnavailable(m);}}
std::int32_t detail::bnw_population_add(std::int32_t a,std::int32_t b){
 const auto n=std::int64_t(a)+b;need(n>=INT32_MIN&&n<=INT32_MAX,"Population reached signed32 ADD overflow");return static_cast<std::int32_t>(n);
}
BnwCityPopulationPending::BnwCityPopulationPending(std::weak_ptr<const BnwCityFoodGrowthPending> p):parent_(std::move(p)){}
void BnwCityPopulationPending::check_owned_prefix()const{
 for(const auto& b:owned_writes_)need(b.value&&*b.value==b.expected,"Population exact owned scalar prefix changed");
}
void BnwCityPopulationPending::require_current()const{
 need(thread_==std::this_thread::get_id(),"Population child foreign thread");
 const auto p=parent_.lock();need(bool(p),"Population food parent lifetime retired");
 p->require_population_child(*this);
}
void BnwCityPopulationPending::require_unissued_plot_source(const BnwPlotYieldRefreshIssuer& issuer,
 const LiveSessionState& state,const saves::SavedSession& source,const SessionCheckpointIdentity& identity,std::int64_t plot)const{
 need(thread_==std::this_thread::get_id(),"Population unissued plot foreign thread");
 const auto parent=parent_.lock();need(bool(parent),"Population unissued plot parent expired");
 parent->require_population_plot_unissued(*this,issuer,state,source,identity,plot);
}
void BnwCityPopulationPending::require_plot_binding(const BnwPlotYieldRefreshPending& plot)const{
 need(thread_==std::this_thread::get_id(),"Population plot binding foreign thread");
 const auto parent=parent_.lock();need(bool(parent),"Population plot parent expired");
 parent->require_population_plot_binding(*this,plot);
}
void BnwCityPopulationPending::continue_plot()const{
 require_current();const auto parent=parent_.lock();need(bool(parent),"Population plot parent expired");
 parent->continue_population_plot(*this);
}
void BnwCityPopulationPending::continue_religion()const{
 require_current();const auto parent=parent_.lock();need(bool(parent),"Population religion parent expired");
 parent->continue_population_religion(*this);
}
void BnwCityPopulationPending::require_city_binding(const BnwCityYieldRefreshPending& city)const{
 need(thread_==std::this_thread::get_id(),"Population religion city binding foreign thread");
 const auto parent=parent_.lock();need(bool(parent),"Population religion city parent expired");
 parent->require_population_city_binding(*this,city);
}
void BnwCityPopulationPending::bind_city_plot(const BnwCityYieldRefreshPending& city,
 const std::shared_ptr<BnwPlotYieldRefreshPending>& plot)const{
 need(thread_==std::this_thread::get_id(),"Population religion plot binding foreign thread");
 const auto parent=parent_.lock();need(bool(parent),"Population religion plot parent expired");
 parent->bind_population_city_plot(*this,city,plot);
}
void BnwCityPopulationPending::accept_city_plot(const BnwCityYieldRefreshPending& city,
 const BnwPlotYieldRefreshPending& plot)const{
 require_city_binding(city);const auto parent=parent_.lock();need(bool(parent),"Population religion prefix parent expired");
 parent->accept_population_city_plot(*this,city,plot);
}
BnwCityPopulationProgress BnwCityPopulationPending::progress()const{
 need(thread_==std::this_thread::get_id(),"Population diagnostic foreign thread");return progress_;
}
}
