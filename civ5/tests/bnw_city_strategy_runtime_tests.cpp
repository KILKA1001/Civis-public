#include "civ5/runtime/bnw_city_strategy_runtime.hpp"
#include "civ5/runtime/bnw_city_entry.hpp"
#include "civ5/runtime/bnw_city_strategy_checkpoint_owner.hpp"
#include "civ5/runtime/bnw_city_current_strategy_eligibility.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <climits>
#include <filesystem>
#include <iostream>
#include <optional>
#include <string_view>
#include <thread>
#include <type_traits>
using namespace civ5;using namespace civ5::runtime;
namespace {
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class F>void refuses(F&& f){bool denied=false;try{f();}catch(const std::exception&){denied=true;}need(denied,"Invalid city strategy owned publication/read accepted");}
auto& named(auto& rows,std::string_view name){using T=std::remove_reference_t<decltype(rows.front())>;T* out=nullptr;for(auto& r:rows)if(r.name&&name==r.name){need(!out,"Fixture duplicate raw name");out=&r;}need(out,"Fixture raw field absent");return *out;}
std::int64_t scalar(const saves::SavedCityObservation& c,std::string_view name){const auto& f=named(c.core.fields,name);need(f.values.size()==1,"Fixture scalar shape differs");return f.values.front();}
detail::BnwCityStrategyRuntimeReadShape shape(const ui::SelectedBnwPreviewHost& h){
 detail::BnwCityStrategyRuntimeReadShape s;
 const auto count=[&](const char* table,bool native_holes=false){const auto r=h.database().prefetch_info(table,[](auto,auto,const auto&){return true;});need(r.all_succeeded()&&(native_holes||!r.holes)&&!r.overlapping_ids&&r.slots<=INT32_MAX&&r.rows+r.holes==r.slots,"Actual fixture native Info collection unsupported");return static_cast<std::uint32_t>(r.slots);};
 s.strategies=count("AICityStrategies");s.flavors=count("Flavors");const char* table[]{"Buildings","Units","Projects","Processes"};for(std::size_t k=0;k<4;++k)s.production[k]=count(table[k],true);return s;
}
std::uint32_t first_hash(const ui::SelectedBnwPreviewHost& h,const char* table){for(const auto& [type,t]:h.types().entries()){(void)type;if(t.table==table&&t.id==0)return t.hash;}throw std::runtime_error("Actual fixture native slot0/hash absent");}
void detached_read_boundaries(const ui::SelectedBnwPreviewHost& h,const saves::CityManagementObservation& original){
 const auto s=shape(h);const auto decode=[&](const auto& m){return detail::decode_bnw_city_strategy_runtime_read(m,h.types(),s);};
 const auto actual=decode(original);need(actual.weights[2].size()==s.production[2],"Actual Project Read count differs");
 // Controlled detached variations exercise source arithmetic only. They are
 // never passed to a genuine owner or used to grant source effects.
 auto m=original;auto& b=named(m.blocks,"strategy");
 const auto strategy=first_hash(h,"AICityStrategies");
 auto& using_rows=named(b.hashes,"m_pabUsingCityStrategy");using_rows.rows={{strategy,{1}},{0,{}},{strategy,{0}}};using_rows.wire_count=3;
 auto& adopted=named(b.hashes,"m_paiTurnCityStrategyAdopted");adopted.rows={{strategy,{19}},{strategy,{-1}}};adopted.wire_count=2;
 need(decode(m).using_strategy.at(0)==0&&decode(m).adopted.at(0)==-1,"Actual strategy Read last-wins failed");
 using_rows.rows[0].values[0]=2;refuses([&]{(void)decode(m);});using_rows.rows[0].values[0]=1;
 std::uint32_t unknown=1;while(h.types().find_hash(unknown)){need(unknown<UINT32_MAX,"Fixture unknown hash unavailable");++unknown;}
 using_rows.rows.push_back({unknown,{1}});++using_rows.wire_count;need(decode(m).using_strategy.at(0)==0,"Unknown strategy hash changed logical slot");
 auto& a=named(named(m.blocks,"building_ai").hashes,"m_BuildingAIWeights");const auto building=first_hash(h,"Buildings");
 a.rows={{building,{7}},{0,{}},{unknown,{31}},{building,{-3}}};a.wire_count=4;
 need(decode(m).weights[0].at(0)==4,"Building Read must SUM duplicates in stream order");
 a.rows={{building,{INT32_MAX}},{building,{1}},{building,{-1}}};a.wire_count=3;refuses([&]{(void)decode(m);}); // no final-only cancellation.
 a.rows={{building,{INT32_MIN}},{building,{1}}};a.wire_count=2;need(decode(m).weights[0].at(0)==INT32_MIN+1,"Building negative Read truncation changed");
 auto& process=named(named(m.blocks,"process_ai").hashes,"m_ProcessAIWeights");const auto process_hash=first_hash(h,"Processes");
 process.rows={{process_hash,{8}},{0,{11}},{unknown,{27}},{process_hash,{-2}}};process.wire_count=4;
 need(decode(m).weights[3].at(0)==-2,"Process Read must SET last duplicate, consume null payload");
 process.rows[1].values.clear();refuses([&]{(void)decode(m);});process.rows[1].values={11};
 auto& latest=named(b.arrays,"m_piLatestFlavorValues");latest.values.clear();named(b.fields,"flavor_wire_count").value=0;
 for(const auto n:decode(m).latest)need(n==0,"Actual Init then empty Read prefix did not retain initialized zero");
 latest.values.assign(static_cast<std::size_t>(s.flavors)+1,0);named(b.fields,"flavor_wire_count").value=static_cast<std::int64_t>(latest.values.size());refuses([&]{(void)decode(m);});
}
void run(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto hash=identify_file(save).sha256;std::weak_ptr<ui::SelectedBnwPreviewHost> weak;
 std::shared_ptr<const BnwCityEntryPending> pending;
 {
 auto result=ui::bootstrap_selected_bnw_preview(save,game,0,true);if(result.preview.blocker)throw std::runtime_error(result.preview.blocker->reason);
 std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(result.preview.host));need(bool(host),"Actual selected immutable Host absent");weak=host;
 auto& session=host->session();std::int64_t owner=-1,city=-1;
 for(const auto& p:session.state().world.all_players->players){for(const auto& c:p.cities.cities)if(scalar(c,"m_iDamage")==0){owner=p.saved_player_id;city=scalar(c,"m_iID");detached_read_boundaries(*host,c.management);break;}if(owner!=-1)break;}
 need(owner!=-1,"Actual selected source has no damage-zero city");const auto features=BnwCityEntryFeatureRules::from_rules(host->database());
 pending=session.begin_bnw_city_entry_damage_zero(owner,city);refuses([&]{(void)pending->strategy_runtime_values();});
 pending=session.continue_bnw_city_entry_feature_surrounded(owner,city,features);const auto old_cp=session.checkpoint();
 const auto old_management=capture_bnw_city_strategy_checkpoint(host);
 std::optional<BnwCityCurrentStrategyEligibility> old_query;
 try{old_query.emplace(inspect_bnw_city_current_strategy_eligibility(host,owner,city));}catch(const std::exception&){std::cout<<"Actual batch eligibility prerequisite UNOBSERVED\n";}
 const auto generation=session.state_generation();bool stopped=false;
 for(unsigned i=0;i<4096;++i){
  const auto before=pending->progress();
  pending=session.continue_bnw_city_entry_strategy(host,pending);const auto after=pending->progress();
  need(session.state_generation()==generation,"In-place private publication retired its own global-generation cursor");
  const auto values=pending->strategy_runtime_values();const saves::SavedCityObservation* raw=nullptr;
  for(const auto& p:session.state().world.all_players->players)if(p.saved_player_id==owner)for(const auto& c:p.cities.cities)if(scalar(c,"m_iID")==city)raw=&c;
  need(raw&&values==detail::decode_bnw_city_strategy_runtime_read(raw->management,host->types(),shape(*host)),"Actual raw and all4 authoritative production vectors diverged");
  need(after.strategy_revision==after.strategy_publications&&after.strategy_effect_attempts==after.strategy_publications,
   "Actual publication history did not pair raw/owned effects");
  if(after.strategy_publications>before.strategy_publications){
   need(after.strategy_publications==before.strategy_publications+1&&after.strategy_next_id==before.strategy_next_id+1,
    "Actual end publication skipped ordered source index");
   refuses([&]{(void)old_management.section();});if(old_query)refuses([&]{(void)old_query->records();});
  }
  if(after.strategy_frontier!=BnwCityStrategyLoopFrontier::none){stopped=true;break;}
 }
 need(stopped,"Actual ordered loop fixture exceeded source bound");const auto terminal=pending->progress();
 if(!terminal.strategy_publications)std::cout<<"Actual false-END/obsolete-END positive publication UNOBSERVED; mandatory frontier retained\n";
 for(int i=0;i<2;++i){(void)session.continue_bnw_city_entry_strategy(host,pending);need(pending->progress().strategy_publications==terminal.strategy_publications,"Frontier replay repeated end effect");}
 bool worker_refused=false;std::thread worker([&]{try{(void)pending->strategy_runtime_values();}catch(const std::exception&){worker_refused=true;}});worker.join();need(worker_refused,"Foreign thread borrowed owned strategy vectors");
 auto generic=LiveSession::from_restored_bnw(session.source(),session.state());need(generic.state().city_strategy_runtime.empty(),"Generic DTO copied genuine component nodes");
 session.restore(old_cp);need(session.state().city_strategy_runtime.empty(),"Checkpoint revived old allocating/revision nodes");
 need(pending->progress().phase==BnwCityEntryPhase::quarantined&&pending->progress().strategy_revision>=terminal.strategy_revision,
  "Checkpoint rewound publication revision/history");refuses([&]{(void)pending->strategy_runtime_values();});
 old_query.reset();host.reset();
 }
 need(weak.expired(),"Owned runtime node/Pending formed strong Host-session cycle");pending.reset();need(identify_file(save).sha256==hash,"Original save mutated");
}
}
static_assert(!std::is_default_constructible_v<BnwCityStrategyRuntime>);
int main(int argc,char** argv){try{need(argc==3,"Expected SAVE + shared installation");run(argv[1],argv[2]);std::cout<<"BNW owned strategy/all4 production state tests passed\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
