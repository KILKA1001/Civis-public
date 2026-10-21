#include "civ5/runtime/bnw_city_current_trade_strategy.hpp"
#include "civ5/runtime/bnw_city_registry.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <array>
#include <climits>
#include <iostream>
#include <optional>
#include <vector>
#include <string_view>
#include <thread>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
using T=BnwTradeCityStrategy;
constexpr std::array<T,4> need_types{T::need_land,T::no_need_land,T::need_sea,T::no_need_sea};
constexpr std::string_view frontier="Trade city strategy positive remaining reaches CvPlayerTrade::GetNumPotentialConnections: actual CanCreateTradeRoute/IsValidTradeRoutePath owner unavailable";
void need(bool b,const char* m){if(!b)throw std::runtime_error(m);}
template<class F>void refuses(F&& f){bool b=false;try{f();}catch(const BnwCityTradeStrategyUnavailable&){b=true;}need(b,"Trade strategy unsupported/generic/stale admitted");}
const auto& named(const auto& rows,const char* name){using R=typename std::remove_cvref_t<decltype(rows)>::value_type;const R* out=nullptr;for(const auto& r:rows)if(r.name&&std::string_view(r.name)==name){need(!out,"Duplicate actual fixture field");out=&r;}need(out,"Actual fixture field absent");return *out;}
std::int64_t scalar(const auto& fields,const char* name){const auto& f=named(fields,name);need(f.values.size()==1,"Actual scalar absent");return f.values.front();}
void pure(){
 using R=saves::TradeRouteObservation;std::vector<R> raw,mirror;
 const auto count=[&](T t){return detail::bnw_trade_international_count_math(raw,mirror,2,3,t);};
 need(count(T::international_origin)==0&&count(T::international_destination)==0,"Empty route vector wrong");
 R empty;empty.origin_x=empty.origin_y=empty.destination_x=empty.destination_y=-1;empty.connection_type=INT64_MAX;raw.push_back(empty);mirror=raw;need(count(T::international_origin)==0,"Empty sentinel consumed type");
 R r;r.origin_x=2;r.origin_y=3;r.destination_x=2;r.destination_y=3;r.connection_type=0;r.origin_owner=-1;r.destination_owner=INT64_MAX;r.domain=INT64_MAX;raw.push_back(r);mirror=raw;need(count(T::international_origin)==1&&count(T::international_destination)==1,"Owner/domain filter invented");raw.push_back(r);mirror=raw;need(count(T::international_origin)==2,"Two routes threshold lost");
 raw.push_back(r);mirror=raw;raw.back().connection_type=mirror.back().connection_type=INT64_MAX;refuses([&]{(void)count(T::international_origin);}); // Full scan after2, no early threshold return.
 raw.pop_back();mirror=raw;mirror.back().origin_y=4;refuses([&]{(void)count(T::international_origin);});
 raw={r};raw[0].origin_x=4;raw[0].origin_y=INT64_MAX;raw[0].connection_type=INT64_MAX;mirror=raw;need(count(T::international_origin)==0,"X mismatch demanded skipped Y/type");
 raw={r};raw[0].destination_x=4;raw[0].destination_y=INT64_MAX;raw[0].connection_type=INT64_MAX;mirror=raw;need(count(T::international_destination)==0,"Destination X mismatch demanded skipped Y/type");
 raw={empty};raw[0].origin_y=INT64_MAX;mirror=raw;refuses([&]{(void)count(T::international_origin);});
 raw.clear();mirror={r};refuses([&]{(void)count(T::international_origin);});refuses([&]{(void)count(T::need_land);});
 need(detail::bnw_trade_remaining_math(0,0)==0&&detail::bnw_trade_remaining_math(0,1)==-1,"Native uint remaining underflow wrong");
 need(detail::bnw_trade_remaining_math(INT32_MIN,1)==INT32_MAX,"Unsigned-minus/int conversion bits lost");need(detail::bnw_trade_remaining_math(-1,0)==-1,"Signed capacity cast lost");
 refuses([&]{(void)detail::bnw_trade_remaining_math(INT64_MAX,0);});refuses([&]{(void)detail::bnw_trade_remaining_math(0,INT64_MIN);});refuses([&]{(void)detail::bnw_trade_remaining_math(0,INT64_MAX);});
 std::cout<<"PASS detached route full-scan/reached fields/native unsigned subtraction; no current owner\n";
}
std::shared_ptr<ui::SelectedBnwPreviewHost> boot(const std::filesystem::path& save,const std::filesystem::path& game){auto b=ui::bootstrap_selected_bnw_preview(save,game,0,true);if(b.preview.blocker)throw std::runtime_error(b.preview.blocker->reason);std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(b.preview.host));need(bool(host),"Actual immutable host absent");return host;}
void genuine(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto before=identify_file(save).sha256;auto host=boot(save,game);const auto& s=host->session().state();need(bool(s.world.all_players),"Actual players absent");std::size_t checked=0;std::array<std::size_t,6> positive{};std::array<std::size_t,4> early{},unavailable{};std::int64_t first_owner=-1,first_city=-1;bool first=false;
 for(const auto& p:s.world.all_players->players){for(const auto& c:p.cities.cities){if(checked==24){break;}const auto owner=scalar(c.core.fields,"m_eOwner"),id=scalar(c.core.fields,"m_iID");need(id>=0&&id<=INT32_MAX,"Actual city native width absent");auto full=inspect_bnw_city_current_trade_strategy(host,owner,id),index=inspect_bnw_city_current_trade_strategy(host,owner,id&8191);
  for(const auto t:{T::international_origin,T::international_destination}){const auto value=full.value(t);need(value==index.value(t),"Actual full/index route trigger differs");const auto n=detail::bnw_trade_international_count_math(s.world.trade.routes,s.cities.trade_routes,scalar(c.core.fields,"m_iX"),scalar(c.core.fields,"m_iY"),t);need(value==(n>=2),"Actual native route wire count differs");if(value){++positive.at(static_cast<std::size_t>(t));}}
  std::optional<bool> remaining_nonpositive;
  for(std::size_t t=0;t<need_types.size();++t){auto q=inspect_bnw_city_current_trade_strategy(host,owner,id);try{const auto value=q.value(need_types[t]);need(value==index.value(need_types[t]),"Actual full/index capacity branch differs");const bool expected=need_types[t]==T::no_need_land||need_types[t]==T::no_need_sea;need(value==expected,"Nonpositive capacity source branch wrong");need(!remaining_nonpositive||*remaining_nonpositive,"Early/refusal source branches disagree");remaining_nonpositive=true;++early[t];if(value){++positive[t];}}
   catch(const BnwCityTradeStrategyUnavailable& e){need(std::string_view(e.what())==frontier,"Unexpected actual capacity dependency unavailable");need(!remaining_nonpositive||!*remaining_nonpositive,"Early/refusal source branches disagree");remaining_nonpositive=false;++unavailable[t];need(q.value(T::international_origin)==full.value(T::international_origin),"Named positive frontier retired healthy origin query");}}
  const auto stale=(id&~std::int64_t{8191})==8192?(id&8191)|16384:(id&8191)|8192;refuses([&]{(void)inspect_bnw_city_current_trade_strategy(host,owner,stale);});refuses([&]{(void)full.value(static_cast<T>(99));});if(checked==0){first_owner=owner;first_city=id;first=full.value(T::international_origin);}++checked;
 }if(checked==24){break;}}
 need(checked>0,"No genuine city observations");for(std::size_t t=0;t<positive.size();++t)std::cout<<"Genuine trade strategy"<<t<<" positives="<<positive[t]<<(positive[t]?" observed":" UNOBSERVED")<<'\n';for(std::size_t t=0;t<early.size();++t)std::cout<<"Trade capacity trigger"<<t<<" <=0="<<early[t]<<" positive-path-frontier="<<unavailable[t]<<'\n';
 auto retained=inspect_bnw_city_current_trade_strategy(host,first_owner,first_city);bool read=false,bind=false;std::thread worker([&]{try{(void)retained.value(T::international_origin);}catch(const BnwCityTradeStrategyUnavailable&){read=true;}try{(void)inspect_bnw_city_current_trade_strategy(host,first_owner,first_city);}catch(const BnwCityTradeStrategyUnavailable&){bind=true;}});worker.join();need(read&&bind&&retained.value(T::international_origin)==first,"Foreign access borrowed/poisoned owner");
 refuses([&]{(void)inspect_bnw_city_current_trade_strategy({},0,0);});refuses([&]{(void)inspect_bnw_city_current_trade_strategy(host,INT64_MAX,first_city);});refuses([&]{(void)inspect_bnw_city_current_trade_strategy(host,first_owner,INT64_MIN);});const auto cp=host->session().checkpoint();host->session().restore(cp);refuses([&]{(void)retained.value(T::international_origin);});auto restored=inspect_bnw_city_current_trade_strategy(host,first_owner,first_city);need(restored.value(T::international_origin)==first,"Healthy restore changed source route count");
 auto generic=LiveSession::from_restored_bnw(host->session().source(),host->session().state());auto moved=std::make_shared<LiveSession>(std::move(host->session()));refuses([&]{(void)restored.value(T::international_origin);});refuses([&]{(void)inspect_bnw_city_current_trade_strategy(host,first_owner,first_city);});host->session()=std::move(generic);refuses([&]{(void)inspect_bnw_city_current_trade_strategy(host,first_owner,first_city);});moved.reset();host.reset();
 auto other=boot(save,game);auto strong=inspect_bnw_city_current_trade_strategy(other,first_owner,first_city);const auto weak=std::weak_ptr<ui::SelectedBnwPreviewHost>(other);other.reset();need(!weak.expired()&&strong.value(T::international_origin)==first,"Query lost sole strong Host/session/DB");auto writer=weak.lock();need(bool(writer),"Retained Host absent");(void)writer->session().mutable_state();refuses([&]{(void)strong.value(T::international_origin);});need(identify_file(save).sha256==before,"Original save changed");std::cout<<"PASS genuine six trade source queries cities="<<checked<<"; potential path/activation/phase unavailable\n";
}
}
static_assert(!std::is_default_constructible_v<BnwCityCurrentTradeStrategy>);
int main(int argc,char** argv){try{need(argc==3,"Expected selected save and shared installation");pure();genuine(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
