#include "civ5/runtime/bnw_city_current_simple_strategy.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <array>
#include <climits>
#include <iostream>
#include <string_view>
#include <thread>
#include <type_traits>
#include <variant>
using namespace civ5;using namespace civ5::runtime;
namespace {
using T=BnwSimpleCityStrategy;
constexpr std::array<T,5> triggers{T::tiny,T::small,T::medium,T::large,T::is_puppet};
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refuses(F&& f){bool denied=false;try{f();}catch(const BnwCitySimpleStrategyUnavailable&){denied=true;}need(denied,"Simple strategy generic/stale/foreign/unsupported source admitted");}
const auto& named(const auto& rows,std::string_view name){using R=typename std::remove_cvref_t<decltype(rows)>::value_type;const R* out=nullptr;for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Duplicate actual fixture source");out=&r;}need(out,"Actual fixture source absent");return *out;}
std::int64_t scalar(const auto& rows,const char* name){const auto& r=named(rows,name);need(r.values.size()==1,"Actual fixture scalar absent");return r.values.front();}
void pure(){
 // The public math result issues no owner. Never feed these values to a
 // genuine factory, edit the live world or infer phase completion.
 for(const auto p:{std::int64_t{INT32_MIN},std::int64_t{-1},std::int64_t{0},std::int64_t{1},std::int64_t{2},std::int64_t{4},std::int64_t{5},std::int64_t{11},std::int64_t{12},std::int64_t{13},std::int64_t{INT32_MAX}}){
  BnwSimpleCityStrategyMathInputs i{p,2,5,12,0,0};
  need(bnw_city_simple_strategy_math(T::tiny,i)==(p<2),"Tiny strict boundary wrong");
  need(bnw_city_simple_strategy_math(T::small,i)==(p>=2&&p<5),"Small lower-inclusive upper-exclusive boundary wrong");
  need(bnw_city_simple_strategy_math(T::medium,i)==(p>=5&&p<12),"Medium lower-inclusive upper-exclusive boundary wrong");
  need(bnw_city_simple_strategy_math(T::large,i)==(p>=12),"Large inclusive boundary wrong");
  i.capital=1;need(!bnw_city_simple_strategy_math(T::medium,i),"Capital medium short circuit lost");
 }
 BnwSimpleCityStrategyMathInputs capital;capital.capital=1;
 need(!bnw_city_simple_strategy_math(T::medium,capital),"Capital branch demanded unreached population/Defines");
 capital.population=INT64_MAX;capital.medium_threshold=INT64_MIN;capital.large_threshold=INT64_MAX;
 need(!bnw_city_simple_strategy_math(T::medium,capital),"Capital branch bounded unreached wide operands");
 BnwSimpleCityStrategyMathInputs small{1,2,INT64_MAX,std::nullopt,std::nullopt,std::nullopt};
 need(!bnw_city_simple_strategy_math(T::small,small),"Small short circuit demanded unreached wide upper bound");
 small.population=2;refuses([&]{(void)bnw_city_simple_strategy_math(T::small,small);});
 BnwSimpleCityStrategyMathInputs medium{4,std::nullopt,5,INT64_MAX,0,std::nullopt};
 need(!bnw_city_simple_strategy_math(T::medium,medium),"Medium lower false demanded upper bound");
 medium.population=5;refuses([&]{(void)bnw_city_simple_strategy_math(T::medium,medium);});
 BnwSimpleCityStrategyMathInputs puppet;puppet.puppet=1;
 need(bnw_city_simple_strategy_math(T::is_puppet,puppet),"Puppet demanded unrelated population/Define/capital");
 puppet.puppet=0;need(!bnw_city_simple_strategy_math(T::is_puppet,puppet),"False puppet wrong");
 for(const auto bad:{std::int64_t{2},std::int64_t{255},std::int64_t{INT64_MAX}}){puppet.puppet=bad;refuses([&]{(void)bnw_city_simple_strategy_math(T::is_puppet,puppet);});}
 BnwSimpleCityStrategyMathInputs nonordered{4,5,2,1,0,0};
 need(bnw_city_simple_strategy_math(T::tiny,nonordered)&&!bnw_city_simple_strategy_math(T::small,nonordered)&&
  !bnw_city_simple_strategy_math(T::medium,nonordered)&&bnw_city_simple_strategy_math(T::large,nonordered),"Source thresholds were sorted or made mutually exclusive");
 for(const auto bad:{std::int64_t{INT64_MIN},std::int64_t{INT64_MAX}}){auto i=nonordered;i.population=bad;refuses([&]{(void)bnw_city_simple_strategy_math(T::tiny,i);});i=nonordered;i.small_threshold=bad;refuses([&]{(void)bnw_city_simple_strategy_math(T::tiny,i);});}
 refuses([&]{(void)bnw_city_simple_strategy_math(static_cast<T>(99),nonordered);});
 std::cout<<"PASS pure signed32/negative/boundary/source-short-circuit comparisons; no owner issuance\n";
}
std::shared_ptr<ui::SelectedBnwPreviewHost> boot(const std::filesystem::path& save,const std::filesystem::path& game){
 auto result=ui::bootstrap_selected_bnw_preview(save,game,0,true);
 if(result.preview.blocker)throw std::runtime_error(result.preview.blocker->reason);
 std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(result.preview.host));need(bool(host),"Actual immutable selected host absent");return host;
}
void genuine(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto before=identify_file(save).sha256;auto host=boot(save,game);
 const auto& s=host->session().state();need(s.world.all_players.has_value(),"Actual source players absent");
 const auto define=[&](const char* name){const auto* d=host->defines().find(name);need(d&&d->lookup.found&&std::holds_alternative<std::int64_t>(d->lookup.value),"Actual reached Define fixture absent");return std::get<std::int64_t>(d->lookup.value);};
 const auto small=define("AI_CITYSTRATEGY_SMALL_CITY_POP_THRESHOLD"),medium=define("AI_CITYSTRATEGY_MEDIUM_CITY_POP_THRESHOLD"),large=define("AI_CITYSTRATEGY_LARGE_CITY_POP_THRESHOLD");
 std::array<std::size_t,5> positives{};std::size_t checked=0,capital_skips=0;
 for(const auto& p:s.world.all_players->players){
  for(const auto& raw:p.cities.cities){
   if(checked==24){break;}
   const auto owner=scalar(raw.core.fields,"m_eOwner"),id=scalar(raw.core.fields,"m_iID");
   need(owner>=0&&owner<64&&id>=0&&id<=INT32_MAX,"Actual bounded city handle outside native width");
   const auto capital_id=scalar(p.core.fields,"m_iCapitalCityID");need(capital_id>=INT32_MIN&&capital_id<=INT32_MAX,"Actual capital handle native width absent");
   const bool capital=resolve_bnw_city_registry(s.world,s.cities,static_cast<std::int32_t>(owner),static_cast<std::int32_t>(capital_id))==&raw;
   const auto pop=scalar(raw.core.fields,"m_iPopulation"),puppet=scalar(raw.core.fields,"m_bPuppet");
   BnwSimpleCityStrategyMathInputs facts{pop,small,medium,large,capital?1:0,puppet};
   auto full=inspect_bnw_city_current_simple_strategy(host,owner,id);
   auto index=inspect_bnw_city_current_simple_strategy(host,owner,id&8191);
   for(std::size_t t=0;t<triggers.size();++t){const auto actual=full.value(triggers[t]);need(actual==index.value(triggers[t])&&actual==bnw_city_simple_strategy_math(triggers[t],facts),"Genuine trigger / fullID / index lookup differs");if(actual){++positives[t];}}
   if(capital){++capital_skips;need(!full.value(T::medium),"Genuine capital Medium predicate not skipped");}
   const auto wrong_generation=(id&~std::int64_t{8191})==8192?(id&8191)|16384:(id&8191)|8192;
   refuses([&]{(void)inspect_bnw_city_current_simple_strategy(host,owner,wrong_generation);});
   ++checked;
  }
  if(checked==24){break;}
 }
 need(checked>0&&capital_skips>0,"Actual selected city/capital observations absent");
 for(std::size_t t=0;t<triggers.size();++t){std::cout<<"Genuine trigger"<<t<<" positive="<<positives[t]<<(positives[t]==0?" UNOBSERVED-positive":" observed-positive")<<"\n";}
 const auto& raw=s.world.all_players->players.at(0).cities.cities.at(0);const auto id=scalar(raw.core.fields,"m_iID");
 auto retained=inspect_bnw_city_current_simple_strategy(host,0,id);const auto initial=retained.value(T::tiny);
 bool read=false,bind=false;std::thread worker([&]{try{(void)retained.value(T::tiny);}catch(const BnwCitySimpleStrategyUnavailable&){read=true;}try{(void)inspect_bnw_city_current_simple_strategy(host,0,id);}catch(const BnwCitySimpleStrategyUnavailable&){bind=true;}});
 worker.join();need(read&&bind&&retained.value(T::tiny)==initial,"Foreign query read source or retired genuine owner");
 refuses([&]{(void)inspect_bnw_city_current_simple_strategy({},0,id);});
 refuses([&]{(void)inspect_bnw_city_current_simple_strategy(host,-1,id);});
 refuses([&]{(void)inspect_bnw_city_current_simple_strategy(host,0,-1);});
 refuses([&]{(void)inspect_bnw_city_current_simple_strategy(host,0,INT64_MAX);});
 refuses([&]{(void)inspect_bnw_city_current_simple_strategy(host,INT64_MIN,id);});
 const auto cp=host->session().checkpoint();host->session().restore(cp);refuses([&]{(void)retained.value(T::tiny);});
 auto restored=inspect_bnw_city_current_simple_strategy(host,0,id);need(restored.value(T::tiny)==initial,"Healthy restore changed numeric trigger");
 auto generic=LiveSession::from_restored_bnw(host->session().source(),host->session().state());
 auto moved=std::make_shared<LiveSession>(std::move(host->session()));refuses([&]{(void)restored.value(T::tiny);});
 refuses([&]{(void)inspect_bnw_city_current_simple_strategy(host,0,id);});
 host->session()=std::move(generic);refuses([&]{(void)inspect_bnw_city_current_simple_strategy(host,0,id);});
 moved.reset();host.reset();
 auto other=boot(save,game);auto strong=inspect_bnw_city_current_simple_strategy(other,0,id);
 const auto only_weak=std::weak_ptr<ui::SelectedBnwPreviewHost>(other);other.reset();need(!only_weak.expired()&&strong.value(T::tiny)==initial,"Genuine query lost strong host/session/DB lifetime");
 auto writer=only_weak.lock();need(bool(writer),"Genuine host expired unexpectedly");
 (void)writer->session().mutable_state(); // Sanctioned retirement; no DTO/raw edit or new grant.
 refuses([&]{(void)strong.value(T::tiny);});refuses([&]{(void)inspect_bnw_city_current_simple_strategy(writer,0,id);});
 need(identify_file(save).sha256==before,"Original selected save changed");
 std::cout<<"PASS genuine simple city trigger reads="<<checked<<" capital skips="<<capital_skips<<"; thread/FLTA/CP/move/generic/lifetime/revocation; no strategy activation or city phase completion\n";
}
}
static_assert(!std::is_default_constructible_v<BnwCityCurrentSimpleStrategy>);
int main(int argc,char** argv){try{need(argc==3,"Expected selected save and shared installation");pure();genuine(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
