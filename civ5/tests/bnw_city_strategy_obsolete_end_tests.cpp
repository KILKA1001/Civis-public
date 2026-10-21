#include "civ5/runtime/bnw_city_strategy_obsolete_end.hpp"
#include "civ5/runtime/bnw_city_current_strategy_eligibility.hpp"
#include "civ5/runtime/bnw_city_strategy_checkpoint_owner.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <filesystem>
#include <iostream>
#include <string_view>
#include <thread>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class F>void refuses(F&& f){bool denied=false;try{f();}catch(const std::exception&){denied=true;}need(denied,"Invalid strategy effect accepted");}
const auto& named(const auto& rows,std::string_view name){using T=typename std::remove_cvref_t<decltype(rows)>::value_type;const T* out=nullptr;for(const auto& r:rows)if(r.name&&name==r.name){need(!out,"Fixture duplicate field");out=&r;}need(out,"Fixture raw field absent");return *out;}
bool same(const saves::CivisSaveSection& a,const saves::CivisSaveSection& b){return a.id==b.id&&a.flags==b.flags&&a.bytes==b.bytes;}
std::shared_ptr<ui::SelectedBnwPreviewHost> boot(const std::filesystem::path& save,const std::filesystem::path& game){auto result=ui::bootstrap_selected_bnw_preview(save,game,0,true);if(result.preview.blocker)throw std::runtime_error(result.preview.blocker->reason);std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(result.preview.host));need(bool(host),"Actual immutable Host absent");return host;}
void arithmetic(){
 using detail::BnwCityStrategyFlavorMath;using detail::bnw_city_strategy_end_flavor_math;
 BnwCityStrategyFlavorMath p;p.latest={15,-5,2000};p.delta={20,-10,0};p.minimum=-10;p.maximum=10;
 p.coefficients[0].push_back(std::vector<std::int32_t>{2,-2,1});p.coefficients[1].push_back(std::vector<std::int32_t>{3,4,0});p.coefficients[2].push_back(std::nullopt);p.coefficients[3].push_back(std::vector<std::int32_t>{1,1,-1});
 auto with_holes=p;for(auto& category:with_holes.coefficients){category.insert(category.begin(),std::nullopt);category.push_back(std::nullopt);}const auto holes=bnw_city_strategy_end_flavor_math(with_holes);
 for(std::size_t k=0;k<4;++k){need(holes.weights[k].size()==with_holes.coefficients[k].size()&&holes.weights[k].front()==0&&holes.weights[k].back()==0,"Native NULL positions lost Reset zeros/span");}
 const auto out=bnw_city_strategy_end_flavor_math(p);for(std::size_t k=0;k<4;++k)need(holes.weights[k][1]==out.weights[k][0],"Native NULL slots shifted real flavor weights");need(out.latest==std::vector<std::int32_t>{-5,5,2000},"Source subtract/clamp/zero-delta order differs");
 need(out.weights[0]==std::vector<std::int32_t>{1990}&&out.weights[1]==std::vector<std::int32_t>{20}&&out.weights[2]==std::vector<std::int32_t>{0}&&out.weights[3]==std::vector<std::int32_t>{-1995},"Source Reset/effective-negative/slot products differ");
 auto bad=p;bad.latest={INT32_MIN};bad.delta={1};for(auto& category:bad.coefficients)category.clear();refuses([&]{(void)bnw_city_strategy_end_flavor_math(bad);}); // overflow BEFORE clamp
 bad.latest={0};bad.delta={INT32_MIN};refuses([&]{(void)bnw_city_strategy_end_flavor_math(bad);});
 bad.latest={INT32_MAX};bad.delta={INT32_MAX};need(bnw_city_strategy_end_flavor_math(bad).latest==std::vector<std::int32_t>{0},"Valid reached subtraction cancellation refused");
 bad.latest={2,2};bad.delta={0,0};bad.coefficients[0].push_back(std::vector<std::int32_t>{INT32_MAX,-INT32_MAX});refuses([&]{(void)bnw_city_strategy_end_flavor_math(bad);}); // product, final cancellation cannot cure UB
 bad.latest={1,1,1};bad.delta={0,0,0};bad.coefficients[0][0]=std::vector<std::int32_t>{INT32_MAX,1,-1};refuses([&]{(void)bnw_city_strategy_end_flavor_math(bad);}); // native addition before cancellation
 bad.latest={1,1};bad.delta={0,0};bad.coefficients[0][0]=std::vector<std::int32_t>{INT32_MAX,-INT32_MAX};need(bnw_city_strategy_end_flavor_math(bad).weights[0][0]==0,"Native flavor order valid cancellation changed");
 bad.coefficients[0][0]=std::vector<std::int32_t>{1};refuses([&]{(void)bnw_city_strategy_end_flavor_math(bad);});
 bad=p;bad.delta.pop_back();refuses([&]{(void)bnw_city_strategy_end_flavor_math(bad);});
 // SDK if/else-if clamp does not normalize inverted mod-authored bounds.
 bad={};bad.latest={4};bad.delta={1};bad.minimum=10;bad.maximum=-10;need(bnw_city_strategy_end_flavor_math(bad).latest[0]==10,"Source inverted-bound branch order changed");
 bad.latest={INT32_MAX};bad.delta={0};need(bnw_city_strategy_end_flavor_math(bad).latest[0]==INT32_MAX,"Zero strategy flavor incorrectly clamps existing latest value");
}
void start_arithmetic(){
 using detail::BnwCityStrategyFlavorMath;using detail::bnw_city_strategy_start_flavor_math;
 BnwCityStrategyFlavorMath p;p.latest={15,-5,2000};p.delta={20,-10,0};p.minimum=-10;p.maximum=10;
 p.coefficients[0].push_back(std::vector<std::int32_t>{2,-2,1});p.coefficients[1].push_back(std::vector<std::int32_t>{3,4,0});p.coefficients[2].push_back(std::nullopt);p.coefficients[3].push_back(std::vector<std::int32_t>{1,1,-1});
 const auto r=bnw_city_strategy_start_flavor_math(p);need(r.latest==std::vector<std::int32_t>{10,-10,2000},"START ADD/clamp/unchanged zero coefficient differs");
 need(r.weights[0]==std::vector<std::int32_t>{2020}&&r.weights[1]==std::vector<std::int32_t>{30}&&r.weights[2]==std::vector<std::int32_t>{0}&&r.weights[3]==std::vector<std::int32_t>{-1990},"START Reset/all4 effective-negative/native-slot products differ");
 auto holes=p;for(auto& k:holes.coefficients){k.insert(k.begin(),std::nullopt);k.push_back(std::nullopt);}const auto h=bnw_city_strategy_start_flavor_math(holes);for(std::size_t k=0;k<4;++k)need(h.weights[k]==std::vector<std::int32_t>{0,r.weights[k][0],0},"START native NULL slots lost Reset/span");
 BnwCityStrategyFlavorMath n;n.latest={INT32_MAX};n.delta={1};n.minimum=-1000;n.maximum=1000;refuses([&]{(void)bnw_city_strategy_start_flavor_math(n);}); // overflow BEFORE clamp
 n.latest={INT32_MIN};n.delta={-1};refuses([&]{(void)bnw_city_strategy_start_flavor_math(n);});
 n.latest={INT32_MIN};n.delta={INT32_MAX};need(bnw_city_strategy_start_flavor_math(n).latest==std::vector<std::int32_t>{-1},"START valid native cancellation refused");
 n.latest={INT32_MAX};n.delta={0};need(bnw_city_strategy_start_flavor_math(n).latest==std::vector<std::int32_t>{INT32_MAX},"START zero coefficient unexpectedly clamps");
 n.latest={4};n.delta={1};n.minimum=10;n.maximum=-10;need(bnw_city_strategy_start_flavor_math(n).latest[0]==10,"START inverted bound if/else order changed");
 n.latest={1,1,1};n.delta={0,0,0};n.coefficients[0].push_back(std::vector<std::int32_t>{INT32_MAX,1,-1});refuses([&]{(void)bnw_city_strategy_start_flavor_math(n);});
 n.latest={2,2};n.delta={0,0};n.coefficients[0][0]=std::vector<std::int32_t>{INT32_MAX,-INT32_MAX};refuses([&]{(void)bnw_city_strategy_start_flavor_math(n);});
 n.latest={1,1};n.coefficients[0][0]=std::vector<std::int32_t>{INT32_MAX,-INT32_MAX};need(bnw_city_strategy_start_flavor_math(n).weights[0][0]==0,"START valid coefficient cancellation differs");
 n.coefficients[0][0]=std::vector<std::int32_t>{1};refuses([&]{(void)bnw_city_strategy_start_flavor_math(n);});n=p;n.delta.pop_back();refuses([&]{(void)bnw_city_strategy_start_flavor_math(n);});
}
void actual(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto save_sha=identify_file(save).sha256;std::int64_t city=0;std::optional<BnwCityStrategyObsoleteEndStage> stage;std::weak_ptr<ui::SelectedBnwPreviewHost> weak;
 {
 auto host=boot(save,game);const auto& s=host->session().state();need(s.world.all_players&&!s.world.all_players->players[0].cities.cities.empty(),"Actual selected source has no owner0 city");
 const auto& raw=s.world.all_players->players[0].cities.cities.front();city=named(raw.core.fields,"m_iID").values.front();
 for(const char* table:{"Buildings","Units","Projects","Processes"}){const auto report=host->database().prefetch_info(table,[](auto,auto,const auto&){return true;});need(report.all_succeeded()&&!report.overlapping_ids&&report.rows+report.holes==report.slots,"Actual native production span report inconsistent");std::cout<<"production_info table="<<table<<" slots="<<report.slots<<" rows="<<report.rows<<" holes="<<report.holes<<"\n";}
 auto owner=bind_bnw_city_strategy_obsolete_end(host,0,city);auto alias=owner;
 const auto before=make_bnw_city_management_checkpoint(host->session().state().world,host->session().source().snapshot,host->identity(),true);
 const auto facts=inspect_bnw_city_current_strategy_eligibility(host,0,city).records();std::size_t end_obligations=0,start_obligations=0;for(const auto& fact:facts)if(fact.obligation==BnwCityStrategyTriggerObligation::start)++start_obligations;
 for(const auto& fact:facts){if(fact.obligation!=BnwCityStrategyTriggerObligation::end){refuses([&]{(void)owner.prepare(fact.native_id);});continue;}++end_obligations;try{stage=owner.prepare(fact.native_id);break;}catch(const BnwCityStrategyObsoleteEndUnavailable&){/* Genuine current tech/mapping/frontier refused; never mint test permission. */}}
 need(same(before,make_bnw_city_management_checkpoint(host->session().state().world,host->session().source().snapshot,host->identity(),true)),"Detached preparation changed actual session");
 if(stage){const auto world=stage->read_world();detail::require_bnw_city_strategy_checkpoint_arrays(*world);auto restored=host->session().source().snapshot;const auto section=stage->management_section();restore_bnw_city_management_checkpoint(section,restored,host->session().source().snapshot,host->identity());need(same(section,make_bnw_city_management_checkpoint(restored,host->session().source().snapshot,host->identity(),true)),"Genuine obsolete-end stage ID27 roundtrip failed");refuses([&]{(void)alias.prepare(0);});}
 else{refuses([&]{(void)owner.prepare(-1);});refuses([&]{(void)owner.prepare(INT32_MAX);});}
 bool foreign_denied=false,foreign_factory=false;std::thread worker([&]{try{(void)owner.prepare(-1);}catch(const std::exception&){foreign_denied=true;}try{(void)bind_bnw_city_strategy_obsolete_end(host,0,city);}catch(const std::exception&){foreign_factory=true;}});worker.join();need(foreign_denied&&foreign_factory,"Strategy effect foreign read/factory accepted");
 // A foreign refusal does not consume/retire the owner's actual main-thread
 // once slot. Negative selector still reaches the selector check.
 if(!stage){try{(void)owner.prepare(-1);}catch(const BnwCityStrategyObsoleteEndUnavailable& e){need(std::string_view(e.what()).find("selector")!=std::string_view::npos,"Foreign refusal poisoned valid source owner");}}
 std::cout<<"actual strategies="<<facts.size()<<" scheduled_start_candidates="<<start_obligations<<" scheduled_end="<<end_obligations<<" genuine_obsolete_stage="<<bool(stage)<<"\n";
 // Sole parent proof: no VM, separate query, session or DB aliases remain.
 weak=host;host.reset();need(!weak.expired(),"Retained genuine owner lost its actual Host parent");
 if(!stage){try{(void)alias.prepare(-1);}catch(const BnwCityStrategyObsoleteEndUnavailable& e){need(std::string_view(e.what()).find("selector")!=std::string_view::npos,"Sole retained owner is not current");}}
 }
 // The owner/alias, initial query, const state borrows and Host facade are ALL
 // gone. Only a genuinely prepared stage (if reached) may retain these parents.
 if(stage){need(!weak.expired()&&bool(stage->read_world()),"Actual stage did not retain sole genuine parents");stage.reset();}
 need(weak.expired(),"Strategy effect ownership cycle retained dead Host");
 // Separately negative actual lifecycle paths. NO fabricated state publishes.
 {auto h=boot(save,game);auto q=bind_bnw_city_strategy_obsolete_end(h,0,city);auto checkpoint=h->session().checkpoint();h->session().restore(checkpoint);refuses([&]{(void)q.prepare(-1);});}
 {auto h=boot(save,game);auto q=bind_bnw_city_strategy_obsolete_end(h,0,city);(void)h->session().mutable_state();refuses([&]{(void)q.prepare(-1);});refuses([&]{(void)bind_bnw_city_strategy_obsolete_end(h,0,city);});}
 {auto h=boot(save,game);auto q=bind_bnw_city_strategy_obsolete_end(h,0,city);auto moved=std::move(h->session());refuses([&]{(void)q.prepare(-1);});(void)moved;}
 {auto h=boot(save,game);auto q=bind_bnw_city_strategy_obsolete_end(h,0,city);auto generic=LiveSession::from_restored_bnw(h->session().source(),h->session().state());h->session()=std::move(generic);refuses([&]{(void)q.prepare(-1);});refuses([&]{(void)bind_bnw_city_strategy_obsolete_end(h,0,city);});}
 need(identify_file(save).sha256==save_sha,"Original selected save changed");
}
}
template<class T>concept PublicStartCandidate=requires(const T& x){x.prepare_start_candidate(std::int64_t{0});};
static_assert(!PublicStartCandidate<BnwCityStrategyObsoleteEndOwner>);
static_assert(!std::is_default_constructible_v<BnwCityStrategyObsoleteEndOwner>);
static_assert(!std::is_default_constructible_v<BnwCityStrategyObsoleteEndStage>);
int main(int argc,char** argv){try{arithmetic();start_arithmetic();need(argc==3,"Usage: bnw_city_strategy_obsolete_end_tests SAVE INSTALLATION");actual(argv[1],argv[2]);std::cout<<"bnw city obsolete-end tests passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
