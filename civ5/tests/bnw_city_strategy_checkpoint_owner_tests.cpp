#include "civ5/runtime/bnw_city_strategy_checkpoint_owner.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_city_tail_checkpoint.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include "civ5/saves/loaded_save.hpp"
#include <chrono>
#include <algorithm>
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
template<class F>void refuses(F&& f){bool denied=false;try{f();}catch(const std::exception&){denied=true;}need(denied,"Invalid/stale/consumed strategy checkpoint accepted");}
auto& named(auto& rows,std::string_view name){using T=std::remove_reference_t<decltype(rows.front())>;T* out=nullptr;for(auto& r:rows)if(r.name&&name==r.name){need(!out,"Actual fixture duplicate field");out=&r;}need(out,"Actual fixture missing field");return *out;}
auto& city(auto& world){need(world.all_players&&world.all_players->players.size()==64&&!world.all_players->players.at(0).cities.cities.empty(),"Actual selected fixture has no native player0 city");return world.all_players->players.at(0).cities.cities.front();}
auto& hashes(auto& world){return named(city(world).management.blocks,"strategy").hashes;}
bool same(const saves::CivisSaveSection& a,const saves::CivisSaveSection& b){return a.id==b.id&&a.flags==b.flags&&a.bytes==b.bytes;}
std::shared_ptr<ui::SelectedBnwPreviewHost> boot(const std::filesystem::path& save,const std::filesystem::path& game){
 auto result=ui::bootstrap_selected_bnw_preview(save,game,0,true);
 if(result.preview.blocker)throw std::runtime_error(result.preview.blocker->reason);
 std::shared_ptr<ui::SelectedBnwPreviewHost> host(std::move(result.preview.host));
 need(bool(host),"Actual immutable selected Host bootstrap failed");return host;
}
std::optional<std::int64_t> last(const saves::TeamHashedArray& a,std::uint64_t hash){std::optional<std::int64_t> result;for(const auto& row:a.rows)if(row.type_hash==hash&&!row.values.empty())result=row.values.front();return result;}
void detached_codec_variations(const ui::SelectedBnwPreviewHost& host,const std::filesystem::path& original_path){
 // These are explicit detached variations of authentic decoded rows. They are
 // NOT genuine issuers, cannot change the live session or mint a Reset owner.
 auto baseline=host.session().source().snapshot;auto changed=baseline;
 auto& a=hashes(changed);need(a.size()==2,"Actual strategy arrays missing");
 auto used=std::find_if(a.at(0).rows.begin(),a.at(0).rows.end(),[](const auto& r){return r.type_hash!=0;});
 auto adopted=std::find_if(a.at(1).rows.begin(),a.at(1).rows.end(),[](const auto& r){return r.type_hash!=0;});
 need(used!=a.at(0).rows.end()&&adopted!=a.at(1).rows.end()&&used->values.size()==1&&adopted->values.size()==1,"Actual source has no strategy bool/adopted payload");
 const auto used_hash=used->type_hash,adopted_hash=adopted->type_hash;
 used->values.front()=1-used->values.front();adopted->values.front()=INT32_MIN;
 if(last(a.at(1),adopted_hash)==last(hashes(baseline).at(1),adopted_hash))adopted->values.front()=INT32_MAX;
 detail::require_bnw_city_strategy_checkpoint_arrays(baseline);detail::require_bnw_city_strategy_checkpoint_arrays(changed);
 const auto section=make_bnw_city_management_checkpoint(changed,baseline,host.identity());
 auto restored=baseline;restore_bnw_city_management_checkpoint(section,restored,baseline,host.identity());
 need(same(make_bnw_city_management_checkpoint(restored,baseline,host.identity()),section),"Actual detached strategy bool/adopted payloads did not roundtrip");
 need(last(hashes(restored).at(0),used_hash)==last(a.at(0),used_hash)&&last(hashes(restored).at(1),adopted_hash)==last(a.at(1),adopted_hash),"Roundtrip changed actual strategy scalar values");
 refuses([&]{restore_bnw_city_management_checkpoint(section,restored,baseline,host.identity());});
 auto destination=baseline;auto truncated=section;truncated.bytes.pop_back();
 const auto before=make_bnw_city_management_checkpoint(destination,baseline,host.identity(),true);
 refuses([&]{restore_bnw_city_management_checkpoint(truncated,destination,baseline,host.identity());});
 need(same(before,make_bnw_city_management_checkpoint(destination,baseline,host.identity(),true)),"Late codec refusal partially changed strategy beforeimage");
 auto wrong=host.identity();wrong.rules_sha256.front()=wrong.rules_sha256.front()=='a'?'b':'a';
 refuses([&]{restore_bnw_city_management_checkpoint(section,destination,baseline,wrong);});
 // Paired structural variants preserve exact native hash stream semantics.
 auto rows_source=baseline;auto& source_arrays=hashes(rows_source);
 std::uint64_t unknown=UINT32_MAX;while(host.types().find_hash(unknown))--unknown;need(unknown!=0,"Unknown-hash fixture unavailable");
 for(std::size_t i=0;i<2;++i){
  auto& arr=source_arrays.at(i);const auto hash=i==0?used_hash:adopted_hash;
  arr.rows.push_back({0,{}});arr.rows.push_back({unknown,{i==0?std::int64_t(1):std::int64_t(-27)}});
  arr.rows.push_back({hash,{i==0?std::int64_t(0):std::int64_t(-1)}});arr.wire_count=static_cast<std::int64_t>(arr.rows.size());
 }
 auto rows_world=rows_source;hashes(rows_world).at(0).rows.back().values.front()=1;hashes(rows_world).at(1).rows.back().values.front()=37;
 detail::require_bnw_city_strategy_checkpoint_arrays(rows_source);detail::require_bnw_city_strategy_checkpoint_arrays(rows_world);
 const auto varied=make_bnw_city_management_checkpoint(rows_world,rows_source,host.identity());
 auto rows_restored=rows_source;restore_bnw_city_management_checkpoint(varied,rows_restored,rows_source,host.identity());
 need(same(varied,make_bnw_city_management_checkpoint(rows_restored,rows_source,host.identity())),"Null/unknown/duplicate stream lost exact order/payload");
 for(std::size_t i=0;i<2;++i){const auto& arr=hashes(rows_restored).at(i);need(arr.rows.at(arr.rows.size()-3).type_hash==0&&arr.rows.at(arr.rows.size()-3).values.empty()&&arr.rows.at(arr.rows.size()-2).type_hash==unknown&&arr.rows.at(arr.rows.size()-2).values.front()==(i==0?1:-27),"Opaque unknown/null rows changed");}
 need(last(hashes(rows_restored).at(0),used_hash)==1&&last(hashes(rows_restored).at(1),adopted_hash)==37,"Duplicate mapped strategy last-wins changed");
 for(const auto bad_bool:{2,255}){auto bad=baseline;auto& arr=hashes(bad).at(0);for(auto& r:arr.rows)if(r.type_hash){r.values={bad_bool};break;}refuses([&]{detail::require_bnw_city_strategy_checkpoint_arrays(bad);});}
 for(const auto bad_value:{std::int64_t(INT32_MIN)-1,std::int64_t(INT32_MAX)+1}){auto bad=baseline;for(auto& r:hashes(bad).at(1).rows)if(r.type_hash){r.values={bad_value};break;}refuses([&]{detail::require_bnw_city_strategy_checkpoint_arrays(bad);});}
 for(int mode=0;mode<5;++mode){auto bad=rows_source;auto& arr=hashes(bad).at(0);if(mode==0)arr.rows.at(arr.rows.size()-3).values={0};if(mode==1)arr.rows.back().values.clear();if(mode==2)arr.rows.back().values={0,1};if(mode==3)++arr.wire_count;if(mode==4)arr.rows.back().type_hash=std::uint64_t(UINT32_MAX)+1;refuses([&]{detail::require_bnw_city_strategy_checkpoint_arrays(bad);});}
 auto wrong_schema=baseline;hashes(wrong_schema).at(0).rows.front().type_hash=unknown;
 refuses([&]{restore_bnw_city_management_checkpoint(section,wrong_schema,baseline,host.identity());});
 auto wrong_roster=baseline;named(city(wrong_roster).core.fields,"m_iID").values.front()+=8192;
 refuses([&]{restore_bnw_city_management_checkpoint(section,wrong_roster,baseline,host.identity());});
 auto wrong_core_span=changed;++city(wrong_core_span).core.start_offset;
 // ID27 alone does not own coreSpan; existing ID25/quadruple validation must
 // reject this detached variant. The genuine owner independently seals spans.
 refuses([&]{(void)make_bnw_city_checkpoint_quadruple(wrong_core_span,baseline,host.identity(),true);});
 const auto file=std::filesystem::current_path()/("bnw-strategy-array-owner-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+".CivisSave");
 struct Cleanup{std::filesystem::path path;~Cleanup(){std::error_code ignored;std::filesystem::remove(path,ignored);}}cleanup{file};
 auto document=saves::make_civis_import_document(host.session().source().source,identify_file(original_path).sha256);
 document.sections.push_back(section);saves::write_civis_save(file,document);
 const auto reopened=saves::read_civis_save(file);const saves::CivisSaveSection* found=nullptr;
 for(const auto& part:reopened.sections)if(part.id==bnw_city_management_checkpoint_section_id){need(!found,"Duplicate file ID27");found=&part;}
 need(found&&same(*found,section),"Existing atomic file path lost strategy array ID27");
 auto from_file=baseline;restore_bnw_city_management_checkpoint(*found,from_file,baseline,host.identity());
 need(last(hashes(from_file).at(0),used_hash)==last(a.at(0),used_hash)&&last(hashes(from_file).at(1),adopted_hash)==last(a.at(1),adopted_hash),"File roundtrip lost both strategy arrays");
}
void run(const std::filesystem::path& save,const std::filesystem::path& game){
 const auto original=identify_file(save).sha256;
 {
  auto host=boot(save,game);detached_codec_variations(*host,save);
  auto owner=capture_bnw_city_strategy_checkpoint(host);const auto section=owner.section();
  need(section.id==27&&!BnwCityStrategyCheckpointOwner::full_persistence()&&!BnwCityStrategyCheckpointOwner::city_entry_phase_ready(),"Partial checkpoint claimed gameplay/fullsave authority");
  bool foreign_read=false,foreign_prepare=false,foreign_factory=false;
  std::thread worker([&]{try{(void)owner.section();}catch(const std::exception&){foreign_read=true;}try{(void)owner.prepare_detached_restore();}catch(const std::exception&){foreign_prepare=true;}try{(void)capture_bnw_city_strategy_checkpoint(host);}catch(const std::exception&){foreign_factory=true;}});worker.join();
  need(foreign_read&&foreign_prepare&&foreign_factory&&same(owner.section(),section),"Foreign read/preparation reached borrowed owner or consumed once");
  auto stage=owner.prepare_detached_restore();const auto restored=stage.read_world();
  need(!BnwCityStrategyDetachedRestore::live_publication_ready()&&same(make_bnw_city_management_checkpoint(*restored,host->session().source().snapshot,host->identity(),true),section),"Genuine closed detached stage differs or claims live publication");
  refuses([&]{(void)owner.prepare_detached_restore();});
  auto alias=owner;refuses([&]{(void)alias.prepare_detached_restore();});
  bool foreign_stage=false;std::thread stage_worker([&]{try{(void)stage.read_world();}catch(const std::exception&){foreign_stage=true;}});stage_worker.join();need(foreign_stage,"Foreign stage accessed current world");
  const auto checkpoint=host->session().checkpoint();host->session().restore(checkpoint);
  refuses([&]{(void)owner.section();});refuses([&]{(void)stage.read_world();});
  auto current=capture_bnw_city_strategy_checkpoint(host);
  (void)host->session().mutable_state(); // Negative genuine retirement only; NO fabricated state write.
  refuses([&]{(void)current.section();});refuses([&]{(void)capture_bnw_city_strategy_checkpoint(host);});
 }
 // Sole-parent proof: no VM, earlier query, session alias or rules alias remains.
 std::optional<BnwCityStrategyDetachedRestore> sole_stage;
 std::weak_ptr<ui::SelectedBnwPreviewHost> weak;
 {
  auto host=boot(save,game);weak=host;auto owner=capture_bnw_city_strategy_checkpoint(host);
  sole_stage.emplace(owner.prepare_detached_restore());host.reset();
 }
 need(!weak.expired()&&bool(sole_stage->read_world()),"Closed detached stage did not retain actual sole Host/session/DB parents");
 auto retained=weak.lock();need(bool(retained),"Actual retained Host absent");
 auto generic=LiveSession::from_restored_bnw(retained->session().source(),retained->session().state());
 auto moved=std::make_shared<LiveSession>(std::move(retained->session()));
 refuses([&]{(void)sole_stage->read_world();});refuses([&]{(void)capture_bnw_city_strategy_checkpoint(retained);});
 retained->session()=std::move(generic);refuses([&]{(void)capture_bnw_city_strategy_checkpoint(retained);});
 moved.reset();retained.reset();sole_stage.reset();need(weak.expired(),"A genuine checkpoint backedge retained dead Host");
 refuses([&]{(void)capture_bnw_city_strategy_checkpoint({});});
 need(identify_file(save).sha256==original,"Original Civ5Save changed");
 std::cout<<"PASS prepared ID27 strategy array owner/closed detached once/actual parents, codec file both arrays and native negatives; no live publication/full persistence\n";
}
}
static_assert(!std::is_default_constructible_v<BnwCityStrategyCheckpointOwner>);
static_assert(!std::is_default_constructible_v<BnwCityStrategyDetachedRestore>);
int main(int argc,char** argv){try{need(argc==3,"Expected selected save and canonical shared installation");run(argv[1],argv[2]);return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
