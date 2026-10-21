#include "../src/content/bnw_globals_cache_source_owner.hpp"
#include <iostream>
#include <array>
#include <bit>
#include <fstream>
#include "civ5/runtime/file_identity.hpp"
#include "civ5/platform/windows_path.hpp"
#define NOMINMAX
#include <windows.h>
#include <thread>
#include <type_traits>
using namespace civ5::content;
static void need(bool v,const char* s){if(!v)throw std::runtime_error(s);}
template<class F> static void refuse(F&&f){bool caught=false;try{f();}catch(const std::exception&){caught=true;}need(caught,"Expected source scope refusal");}
static std::shared_ptr<RulesDatabase> database(bool table=true){
 auto db=std::make_shared<RulesDatabase>();
 if(table)db->apply_sql("CREATE TABLE Defines(Name TEXT, Value); INSERT INTO Defines VALUES('HEAVY_RESOURCE_THRESHOLD',4294967297),('AI_ATTEMPT_RUSH_OVER_X_TURNS_TO_BUILD',19),('AI_STRATEGY_NEED_IMPROVEMENT_CITY_RATIO',0.1);");
 db->seal_immutable_preview_fingerprints();return db;
}
using Stat=std::array<std::uint32_t,8>;
static Stat identity(const std::filesystem::path&path){auto h=CreateFileW(civ5::platform::extended_windows_path(path).c_str(),FILE_READ_ATTRIBUTES,7,nullptr,OPEN_EXISTING,0,nullptr);need(h!=INVALID_HANDLE_VALUE,"Source stat open");BY_HANDLE_FILE_INFORMATION i{};const auto ok=GetFileInformationByHandle(h,&i);CloseHandle(h);need(ok!=0,"Source stat failed");return {i.dwFileAttributes,i.dwVolumeSerialNumber,i.nFileIndexHigh,i.nFileIndexLow,i.nFileSizeHigh,i.nFileSizeLow,i.ftLastWriteTime.dwHighDateTime,i.ftLastWriteTime.dwLowDateTime};}
static std::uint32_t word(std::ifstream&f){unsigned char b[4]{};f.read(reinterpret_cast<char*>(b),4);need(bool(f),"Fixture truncation");return std::uint32_t(b[0])|(std::uint32_t(b[1])<<8)|(std::uint32_t(b[2])<<16)|(std::uint32_t(b[3])<<24);}
int main(int argc,char**argv){try{
 need(argc==4,"Expected fixture, original DLL and canonical DLL");std::ifstream f(argv[1],std::ios::binary);char magic[8]{};f.read(magic,8);need(std::string(magic,8)=="BNWGCP01","Source fixture schema");Stat original{},canonical{};for(auto&x:original)x=word(f);for(auto&x:canonical)x=word(f);need(identity(argv[2])==original&&identity(argv[3])==canonical,"Distinct original/canonical stat provenance");
 const std::string sha="c5483b5857a6fb7ef107af93c5d34282eb713472e2a84d0eb9d3fdc104e155a5";need(civ5::runtime::identify_file(argv[2]).sha256==sha&&civ5::runtime::identify_file(argv[3]).sha256==sha,"Original/canonical payload identity");
 auto count=word(f);need(count==9,"Original query-case budget");for(unsigned i=0;i<count;++i){std::array<std::uint32_t,6>v{};for(auto&x:v)x=word(f);const auto expected=v[0]&&v[1]?v[2]:0;need(v[3]==expected&&v[4]==expected&&v[5]==(v[0]?(v[1]?4:3):2),"Original query/store/reset route differential");}
 count=word(f);need(count==10,"Typed original integer budget");for(unsigned i=0;i<count;++i){const auto lo=word(f),hi=word(f),expected=word(f);const auto native=std::bit_cast<std::int64_t>((std::uint64_t(hi)<<32)|lo);need(std::bit_cast<std::uint32_t>(globals_cache_detail::source_int(native))==expected,"Original typed SQLite GetInt low32 differential");}
 need(f.peek()==std::ifstream::traits_type::eof(),"Fixture tail");

 static_assert(!std::is_default_constructible_v<BnwGlobalsCacheSourceOwner>);
 static_assert(!std::is_copy_constructible_v<BnwGlobalsCacheSourceOwner> && !std::is_move_constructible_v<BnwGlobalsCacheSourceOwner>);
 static_assert(!std::is_constructible_v<BnwGlobalsCacheSourceOwner,std::shared_ptr<const RulesDatabase>>);
 using namespace globals_cache_detail;
 need(source_int(4294967297LL)==1&&source_int(2147483648LL)==INT32_MIN&&source_int(-4294967297LL)==-1,"Source GetInt low bits");
 auto db=database();auto state_owner=std::make_shared<LeafState>();auto& state=*state_owner;need(state.heavy==3,"GC constructor Heavy3");
 const auto specs=global_define_specs(GlobalDefineProfile::bnw);need(specs.size()==1689,"Pinned source scalar count");
 refuse([&]{ordered_lookup_write(db,state_owner,[](auto i,const auto&){if(i==4)throw std::runtime_error("Late diagnostic stop");});});
 need(state.write_order.size()==5&&state.heavy==3,"Late stop retains prefix and unreached Heavy constructor");
 need(std::get<std::int64_t>(state.values.at(state.write_order[0]).lookup.value)==19,"Actual prepared query/write");
 ordered_lookup_write(*db,state);need(state.write_order.size()==1694&&state.heavy==1,"Full isolated ordered leaf with source32 store");
 for(std::size_t i=0;i<specs.size();++i)need(state.write_order[i+5]==specs[i].name,"Exact assignment order");
 need(!state.values.at(std::string(specs[1].name)).lookup.found&&std::get<std::int64_t>(state.values.at(std::string(specs[1].name)).lookup.value)==0,"Missing query writes initialized zero");
 need(std::get<double>(state.values.at("AI_STRATEGY_NEED_IMPROVEMENT_CITY_RATIO").lookup.value)==double(float(0.1)),"Source float precision in actual prepared lookup/store");
 auto after_heavy_owner=std::make_shared<LeafState>();auto&after_heavy=*after_heavy_owner;refuse([&]{ordered_lookup_write(db,after_heavy_owner,[](auto i,const auto&){if(i==1503)throw std::runtime_error("After Heavy stop");});});need(after_heavy.heavy==1&&after_heavy.write_order.size()==1504,"Late failure retains changed Heavy rather than constructor3");
 for(auto&sp:specs)if(sp.zero_is_none)need(std::get<std::int64_t>(state.values.at(std::string(sp.name)).lookup.value)==-1,"Source policy zero correction");
 auto raw_policy_owner=std::make_shared<LeafState>();auto&raw_policy=*raw_policy_owner;refuse([&]{ordered_lookup_write(db,raw_policy_owner,[&](auto i,const auto&){if(specs[i].name=="POLICY_BRANCH_FREEDOM")throw std::runtime_error("Raw branch stop");});});
 need(std::get<std::int64_t>(raw_policy.values.at("POLICY_BRANCH_FREEDOM").lookup.value)==0&&raw_policy.policy_correction_order.empty(),"Raw policy store precedes zero correction");
 need(state.policy_correction_order.size()==3,"Three source policy correction branches");
 auto bad=database(false);LeafState errors;ordered_lookup_write(*bad,errors);need(errors.heavy==0&&errors.write_order.size()==1689,"SQL errors still assign zero and continue");
 need(errors.values.at("HEAVY_RESOURCE_THRESHOLD").lookup.sqlite_code!=0&&!errors.values.at("HEAVY_RESOURCE_THRESHOLD").lookup.error.empty(),"SQL failure retained");
 auto step_failure=std::make_shared<RulesDatabase>();
 step_failure->apply_sql("CREATE TABLE DefineInput(Name TEXT, Value); INSERT INTO DefineInput VALUES('HEAVY_RESOURCE_THRESHOLD',3),('AI_ATTEMPT_RUSH_OVER_X_TURNS_TO_BUILD',19); CREATE VIEW Defines AS SELECT Name,CASE WHEN Name='HEAVY_RESOURCE_THRESHOLD' THEN abs(-9223372036854775808) ELSE Value END AS Value FROM DefineInput;");
 step_failure->seal_immutable_preview_fingerprints();LeafState stepped;ordered_lookup_write(*step_failure,stepped);
 need(stepped.heavy==0&&stepped.values.at("HEAVY_RESOURCE_THRESHOLD").lookup.sqlite_code!=0,"Actual SQL Step failure still assigns native initialized zero");
 const auto recovery=step_failure->global_define("AI_ATTEMPT_RUSH_OVER_X_TURNS_TO_BUILD",GlobalDefineKind::integer);
 need(recovery.found&&recovery.sqlite_code==0&&std::get<std::int64_t>(recovery.value)==19,"Actual prepared Reset recovers after Step integer-overflow failure");
 // Repeated queries, a miss, and a repeated SQL failure exercise actual reset
 // reuse; the API does not expose a synthetic reset-count receipt.
 need(std::get<std::int64_t>(db->global_define("HEAVY_RESOURCE_THRESHOLD",GlobalDefineKind::integer).value)==4294967297LL,"Prepared query reused after all misses");
 need(bad->global_define("HEAVY_RESOURCE_THRESHOLD",GlobalDefineKind::integer).sqlite_code!=0,"Prepared error reused");
 RulesDatabase unsealed;LeafState no;refuse([&]{ordered_lookup_write(unsealed,no);});need(no.write_order.empty(),"Unsealed refuses before write");
 bool cross=false;std::thread worker([&]{try{LeafState foreign;ordered_lookup_write(*db,foreign);}catch(const std::exception&){cross=true;}});worker.join();need(cross,"Foreign thread refuses");
 refuse([&]{ordered_lookup_write(*db,state);}); // foreign access poisons SAME owner
 auto poisoned=database();refuse([&]{poisoned->apply_sql("UPDATE Defines SET Value=Value");});refuse([&]{LeafState stale;ordered_lookup_write(*poisoned,stale);});
 auto late=database();auto partial_owner=std::make_shared<LeafState>();auto&partial=*partial_owner;refuse([&]{ordered_lookup_write(late,partial_owner,[&](auto i,const auto&){if(i==6){try{late->apply_sql("UPDATE Defines SET Value=Value");}catch(const std::exception&){} }});});need(partial.write_order.size()==7,"Late poisoned generation preserves completed stores");

 // Actual shared ownership survives callback reset of the caller's LAST
 // wrapper/leaf references. The currentness closure alone retains only Impl.
 static_assert(!std::is_copy_constructible_v<LeafState> && !std::is_move_constructible_v<LeafState>);
 auto lifetime_db=database();auto lifetime_state=std::make_shared<LeafState>();
 std::weak_ptr<const RulesDatabase> weak_db=lifetime_db;std::weak_ptr<LeafState> weak_state=lifetime_state;
 std::size_t lifetime_calls=0;std::optional<CachedGlobalDefine> copied_quote;
 ordered_lookup_write(lifetime_db,lifetime_state,[&](auto i,const auto&entry){
   ++lifetime_calls;if(i==0){lifetime_db.reset();lifetime_state.reset();
     need(!weak_db.expired()&&!weak_state.expired(),"Observer lost genuine retained owners");copied_quote=entry;}
 });
 need(lifetime_calls==1689&&weak_db.expired()&&weak_state.expired()&&copied_quote&&copied_quote->name==specs[0].name,
   "Observer last-owner reset did not finish/release exact SQL and Leaf wrappers");
 // Callback itself is pinned too, independently of the caller's function.
 auto callback_db=database();auto callback_state=std::make_shared<LeafState>();
 WriteObserver resetting_callback;std::size_t callback_calls=0;
 resetting_callback=[&](auto,const auto&){++callback_calls;resetting_callback={};};
 ordered_lookup_write(callback_db,callback_state,resetting_callback);
 need(callback_calls==1689&&!resetting_callback,"Observer callable was destroyed/retargeted during its invocation");
 // Mutation plus exception must be detected BEFORE any fresh map access.
 auto clearing_db=database();auto clearing_state=std::make_shared<LeafState>();
 refuse([&]{ordered_lookup_write(clearing_db,clearing_state,[&](auto,const auto&entry){
   const auto name=entry.name;clearing_state->values.clear();
   need(entry.name==name,"Observer received destroyed map entry rather than owned copy");
   throw std::runtime_error("Clear then throw");});});
 need(clearing_state->values.empty()&&clearing_state->write_order.size()==1,"Clear+throw unexpectedly rolled back source stores");
 refuse([&]{ordered_lookup_write(*clearing_db,*clearing_state);});
 // All observable state/lookup metadata participates in the bitwise guard.
 for(unsigned mutation=0;mutation<11;++mutation) {
   auto owner_db=database();auto owner_state=std::make_shared<LeafState>();
   refuse([&]{ordered_lookup_write(owner_db,owner_state,[&](auto,const auto&){
     switch(mutation){
      case 0:owner_state->values.begin()->second.lookup.value=std::int64_t{7};break;
      case 1:owner_state->write_order.push_back("injected");break;
      case 2:owner_state->policy_correction_order.push_back("injected");break;
      case 3:owner_state->heavy=42;break;
      case 4:owner_state->values.begin()->second.lookup.error="injected";break;
      case 5:owner_state->values.begin()->second.lookup.sqlite_code=42;break;
      case 6:owner_state->values.begin()->second.lookup.found=false;break;
      case 7:owner_state->values.begin()->second.name="injected";break;
      case 8:owner_state->values.begin()->second.retained_previous=true;break;
      case 9:owner_state->values.begin()->second.kind=GlobalDefineKind::text;break;
      case 10:owner_state->values.begin()->second.lookup.value=std::string("injected");break;
     }
   });});
   need(owner_state->write_order.size()>=1,"Mutation removed the source-prefix observation");
   refuse([&]{ordered_lookup_write(*owner_db,*owner_state);});
 }
 // Either overload entering the SAME active leaf latches poison even when
 // the observer catches the nested exception; alternate-route bypass fails.
 for(bool retained_nested:{false,true}) {
   auto nested_db=database();auto nested_state=std::make_shared<LeafState>();bool nested_refused=false;
   refuse([&]{ordered_lookup_write(nested_db,nested_state,[&](auto,const auto&){
     try{if(retained_nested)ordered_lookup_write(nested_db,nested_state,WriteObserver{});
         else ordered_lookup_write(*nested_db,*nested_state);}
     catch(const std::exception&){nested_refused=true;}
   });});
   need(nested_refused&&nested_state->write_order.size()==1,"Caught nested call admitted extra source writes");
   refuse([&]{ordered_lookup_write(*nested_db,*nested_state);});
 }
 // Originating-thread check precedes leaf field access, and its failure is
 // visible to the active outer observer even after worker catches it.
 auto thread_db=database();auto thread_state=std::make_shared<LeafState>();bool thread_refused=false;
 refuse([&]{ordered_lookup_write(thread_db,thread_state,[&](auto,const auto&){
   std::thread other([&]{try{ordered_lookup_write(*thread_db,*thread_state);}
     catch(const std::exception&){thread_refused=true;}});other.join();
 });});
 need(thread_refused&&thread_state->write_order.size()==1,"Foreign thread changed active leaf stores");
 refuse([&]{ordered_lookup_write(*thread_db,*thread_state);});
 // Unchanged NaN payload and -0 must not compare unequal; a changed payload
 // or sign bit is a mutation even when ordinary numeric equality hides it.
 auto bits_db=database();auto bits_state=std::make_shared<LeafState>();
 bits_state->values.emplace("__nan",CachedGlobalDefine{"__nan",GlobalDefineKind::source_float,
   GlobalDefineLookup{std::bit_cast<double>(std::uint64_t{0x7ff8000000000001ULL}),false,0,{}},false});
 bits_state->values.emplace("__zero",CachedGlobalDefine{"__zero",GlobalDefineKind::source_float,
   GlobalDefineLookup{std::bit_cast<double>(std::uint64_t{0x8000000000000000ULL}),false,0,{}},false});
 refuse([&]{ordered_lookup_write(bits_db,bits_state,[](auto,const auto&){throw std::runtime_error("Unchanged diagnostic stop");});});
 ordered_lookup_write(*bits_db,*bits_state);
 need(bits_state->write_order.size()==1690,"Unchanged bitwise snapshot poisoned resumable prefix");
 for(bool nan_change:{false,true}) {
   auto change_db=database();auto change_state=std::make_shared<LeafState>();
   change_state->values.emplace("__bits",CachedGlobalDefine{"__bits",GlobalDefineKind::source_float,
     GlobalDefineLookup{std::bit_cast<double>(nan_change?std::uint64_t{0x7ff8000000000001ULL}:std::uint64_t{0x8000000000000000ULL}),false,0,{}},false});
   refuse([&]{ordered_lookup_write(change_db,change_state,[&](auto,const auto&){
     change_state->values.at("__bits").lookup.value=std::bit_cast<double>(nan_change?
       std::uint64_t{0x7ff8000000000002ULL}:std::uint64_t{0});
   });});
   refuse([&]{ordered_lookup_write(*change_db,*change_state);});
 }
 std::shared_ptr<const RulesDatabase> absent_db;auto present_state=std::make_shared<LeafState>();
 refuse([&]{ordered_lookup_write(absent_db,present_state,WriteObserver{});});
 refuse([&]{ordered_lookup_write(database(),std::shared_ptr<LeafState>{},WriteObserver{});});
 need(identity(argv[2])==original&&identity(argv[3])==canonical,"Source inputs changed during CPU comparison");
 std::cout<<"PASS 1689 ordered real SQL source stores, int32 conversion, constructor3, partial failures, repeated reset reuse, SQL-zero/errors, immutable/thread poison and retained callback reset/mutation/reentry/bitwise guards; diagnostic leaf only, genuine lifecycle unavailable\n";
 return 0;
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
