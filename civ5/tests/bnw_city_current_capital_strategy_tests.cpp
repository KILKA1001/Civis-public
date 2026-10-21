#include "civ5/runtime/bnw_city_current_capital_strategy.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <array>
#include <climits>
#include <iostream>
#include <string_view>
#include <thread>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
using T=BnwCapitalCityStrategy;
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refuses(F&& f){bool rejected=false;try{f();}catch(const BnwCityCapitalStrategyUnavailable&){rejected=true;}need(rejected,"Capital unsupported/stale source was admitted");}
const auto& named(const auto& rows,const char* name){using R=typename std::remove_cvref_t<decltype(rows)>::value_type;const R* result=nullptr;for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!result,"Duplicate genuine field");result=&r;}need(result,"Genuine field absent");return *result;}
std::int64_t scalar(const auto& fields,const char* name){const auto& f=named(fields,name);need(f.values.size()==1,"Genuine scalar absent");return f.values.front();}
void pure(){
 BnwCapitalCityStrategyMathInputs i;auto calc=[&](T t){return bnw_capital_city_strategy_math(t,i);};
 i.capital=0;need(!calc(T::under_threat)&&!calc(T::need_settler),"Noncapital reached skipped operands");i.capital=1;i.war_count=0;need(!calc(T::under_threat),"Peace reached minor");i.war_count=1;i.minor=1;need(!calc(T::under_threat),"Minor reached threat scan");i.minor=0;
 i.selected_city=0;i.threat_cities={{{201},{1},{1}},{{201},{1},{0}}};i.capital_threat_percent=100;need(calc(T::under_threat),"First strict positive equal threat lost");i.selected_city=1;need(!calc(T::under_threat),"Equal threat replaced first city");
 i.selected_city=0;i.threat_cities={{{200},{1},{1}}};need(!calc(T::under_threat),"Unweighted threat200 admitted");i.threat_cities[0].threat=201;i.capital_threat_percent=0;need(!calc(T::under_threat),"Zero weighted threat returned capital");i.capital_threat_percent=-100;need(!calc(T::under_threat),"Negative weighted threat returned capital");
 i.capital_threat_percent=100;i.threat_cities={{{201},{1},{1}},{{150},{2},{0}}};need(!calc(T::under_threat),"Threat population weighting ignored");i.threat_cities[0].threat=INT32_MAX;i.threat_cities[0].population=2;i.capital_threat_percent=0;refuses([&]{(void)calc(T::under_threat);});i.threat_cities={{{INT32_MAX},{1},{1}}};i.capital_threat_percent=100;refuses([&]{(void)calc(T::under_threat);});
 i.threat_cities={{{INT32_MIN},{1},{0}}};i.capital_threat_percent.reset();need(!calc(T::under_threat),"Negative noncapital demanded skipped percent");i.threat_cities[0].population=2;refuses([&]{(void)calc(T::under_threat);});
 i={};i.capital=1;i.minor=1;need(!calc(T::need_settler),"Minor demanded city count");i.minor=0;i.city_count=2;i.settlers=1;need(!calc(T::need_settler),"Cities+settlers3 demanded strategy types");i.city_count=INT32_MAX;i.settlers=1;refuses([&]{(void)calc(T::need_settler);});
 i.city_count=1;i.settlers=0;i.under_threat_type=0;i.game_turn=50;i.mobilization_type=0;i.mobilization_using=1;need(!calc(T::need_settler),"Turn50 demanded skipped threat using");i.game_turn=51;i.under_threat_using=1;i.mobilization_using.reset();need(!calc(T::need_settler),"Threat true demanded military using");
 i.under_threat_type=-1;i.mobilization_type=-1;i.game_turn=1;i.personality=std::vector<std::int64_t>{};i.threshold_modifiers=std::vector<std::int64_t>{};i.weight_threshold=3;need(calc(T::need_settler),"Turn*4 strict threshold lost");i.weight_threshold=4;need(!calc(T::need_settler),"Strict threshold became inclusive");i.city_count=2;i.weight_threshold=1;need(calc(T::need_settler),"Turn*2 strict threshold lost");i.game_turn=INT32_MAX;refuses([&]{(void)calc(T::need_settler);});
 i.city_count=1;i.game_turn=1;i.weight_threshold=INT32_MIN;i.personality=std::vector<std::int64_t>{INT32_MAX};i.threshold_modifiers=std::vector<std::int64_t>{2};refuses([&]{(void)calc(T::need_settler);});i.personality=std::vector<std::int64_t>{INT32_MAX,1};i.threshold_modifiers=std::vector<std::int64_t>{1,1};refuses([&]{(void)calc(T::need_settler);});
 i={};i.capital=2;refuses([&]{(void)calc(T::under_threat);});refuses([&]{(void)calc(static_cast<T>(99));});
 std::cout<<"PASS supplied two-capital math: source skips/ties/native32 before cancellation; no issuer\n";
}
std::shared_ptr<ui::SelectedBnwPreviewHost> boot(const std::filesystem::path& save,const std::filesystem::path& game){auto b=ui::bootstrap_selected_bnw_preview(save,game,0,true);if(b.preview.blocker)throw std::runtime_error(b.preview.blocker->reason);std::shared_ptr<ui::SelectedBnwPreviewHost> h(std::move(b.preview.host));need(bool(h),"Actual immutable Host absent");return h;}
void genuine(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto before=identify_file(save).sha256;auto host=boot(save,game);const auto& state=host->session().state();need(bool(state.world.all_players),"Actual player registry absent");std::size_t seen=0,threats=0,settler_frontiers=0,settler_reads=0,settler_positive=0;std::int64_t first_owner=-1,first_city=-1;bool first=false;
 for(const auto& p:state.world.all_players->players){for(const auto& c:p.cities.cities){if(seen==24){break;}const auto owner=scalar(c.core.fields,"m_eOwner"),id=scalar(c.core.fields,"m_iID");need(id>=0&&id<=INT32_MAX,"Genuine native city ID width differs");
  auto full=inspect_bnw_city_current_capital_strategy(host,owner,id),index=inspect_bnw_city_current_capital_strategy(host,owner,id&8191);const auto value=full.value(T::under_threat);need(value==index.value(T::under_threat),"Capital full/index threat differs");if(value){++threats;}
  const auto stale=(id&~std::int64_t{8191})==8192?(id&8191)|16384:(id&8191)|8192;refuses([&]{(void)inspect_bnw_city_current_capital_strategy(host,owner,stale);});
  const auto read_settler=[](auto& query)->std::optional<bool>{try{return query.value(T::need_settler);}catch(const BnwCityCapitalStrategyUnavailable& e){const auto message=std::string_view(e.what());need(message=="Capital strategy omitted reached using slot requires actual native Reset/Read owner"||message=="Capital strategy missing/unsafe Military using prefix requires native Reset/Read owner"||message=="Capital strategy finite native Military Info allocation has unsupported hole/order","Unexpected genuine settlement source failure");return std::nullopt;}};
  const auto a=read_settler(full),b=read_settler(index);need(a==b,"Capital full/index settlement value/frontier differs");if(a){++settler_reads;if(*a){++settler_positive;}}else{++settler_frontiers;}
  if(seen==0){first_owner=owner;first_city=id;first=value;}++seen;
 }if(seen==24){break;}}
 need(seen>0,"No genuine current city");std::cout<<"Genuine threat positives="<<threats<<(threats?" observed":" UNOBSERVED")<<" settler reads="<<settler_reads<<" settler positives="<<settler_positive<<(settler_positive?" observed":" UNOBSERVED")<<" actual native using frontiers="<<settler_frontiers<<"\n";
 auto retained=inspect_bnw_city_current_capital_strategy(host,first_owner,first_city);bool foreign_read=false,foreign_bind=false;std::thread worker([&]{try{(void)retained.value(T::under_threat);}catch(const BnwCityCapitalStrategyUnavailable&){foreign_read=true;}try{(void)inspect_bnw_city_current_capital_strategy(host,first_owner,first_city);}catch(const BnwCityCapitalStrategyUnavailable&){foreign_bind=true;}});worker.join();need(foreign_read&&foreign_bind&&retained.value(T::under_threat)==first,"Foreign access borrowed world/poisoned healthy query");
 refuses([&]{(void)inspect_bnw_city_current_capital_strategy({},0,0);});refuses([&]{(void)inspect_bnw_city_current_capital_strategy(host,INT64_MAX,first_city);});refuses([&]{(void)inspect_bnw_city_current_capital_strategy(host,first_owner,INT64_MIN);});
 const auto cp=host->session().checkpoint();host->session().restore(cp);refuses([&]{(void)retained.value(T::under_threat);});auto restored=inspect_bnw_city_current_capital_strategy(host,first_owner,first_city);need(restored.value(T::under_threat)==first,"Restore changed scalar threat predicate");
 auto generic=LiveSession::from_restored_bnw(host->session().source(),host->session().state());auto moved=std::make_shared<LiveSession>(std::move(host->session()));refuses([&]{(void)restored.value(T::under_threat);});refuses([&]{(void)inspect_bnw_city_current_capital_strategy(host,first_owner,first_city);});host->session()=std::move(generic);refuses([&]{(void)inspect_bnw_city_current_capital_strategy(host,first_owner,first_city);});moved.reset();host.reset();
 auto other=boot(save,game);auto strong=inspect_bnw_city_current_capital_strategy(other,first_owner,first_city);const auto weak=std::weak_ptr<ui::SelectedBnwPreviewHost>(other);other.reset();need(!weak.expired()&&strong.value(T::under_threat)==first,"Current query lost sole strong genuine Host");auto writer=weak.lock();need(bool(writer),"Retained Host absent");(void)writer->session().mutable_state();refuses([&]{(void)strong.value(T::under_threat);});need(identify_file(save).sha256==before,"Original save changed");
 std::cout<<"PASS genuine capital/current controls, cities="<<seen<<"; shared actual settlement counter joined, using-array gaps explicit; no activation/phase grant\n";
}
}
static_assert(!std::is_default_constructible_v<BnwCityCurrentCapitalStrategy>);
int main(int argc,char** argv){try{need(argc==3,"Expected selected SAVE and shared installation");pure();genuine(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
