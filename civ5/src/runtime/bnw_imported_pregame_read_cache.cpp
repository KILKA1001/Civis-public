#include "civ5/runtime/bnw_imported_pregame_read_cache.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/bnw_sp_client_update_loan.hpp"
#include <limits>
#include <stdexcept>
#include <string_view>
namespace civ5::runtime {
namespace {
void need(bool v,const char*why){if(!v)throw std::runtime_error(why);}
std::int32_t i32(std::int64_t n){need(n>=INT32_MIN&&n<=INT32_MAX,"Pregame native option int32 unavailable");return static_cast<std::int32_t>(n);}
bool same_record(const saves::PregameRecord&a,const saves::PregameRecord&b){
 return a.integers==b.integers&&a.flags==b.flags&&a.float_bits==b.float_bits&&a.strings==b.strings&&
  a.integer_arrays==b.integer_arrays&&a.flag_arrays==b.flag_arrays&&a.string_arrays==b.string_arrays;
}
bool same_options(const std::vector<saves::PregameOption>&a,const std::vector<saves::PregameOption>&b){
 if(a.size()!=b.size())return false;for(std::size_t i=0;i<a.size();++i)if(a[i].name!=b[i].name||a[i].value!=b[i].value)return false;return true;
}
bool same_info(const saves::PregameInfo&a,const saves::PregameInfo&b){return a.id==b.id&&a.text==b.text&&a.wire_version==b.wire_version&&same_record(a.values,b.values);}
bool same_read(const saves::PregameObservation&a,const saves::PregameObservation&b){
 return a.layout==b.layout&&a.header_tail_offset==b.header_tail_offset&&a.slots_offset==b.slots_offset&&a.archive_offset==b.archive_offset&&
  a.archive_end_offset==b.archive_end_offset&&a.end_offset==b.end_offset&&a.slots_version==b.slots_version&&a.archive_version==b.archive_version&&
  a.header_tail_strings==b.header_tail_strings&&a.gamecore_guid==b.gamecore_guid&&a.engine_guid==b.engine_guid&&a.gamecore_version==b.gamecore_version&&
  a.engine_value==b.engine_value&&a.engine_trailer==b.engine_trailer&&a.wire_known_player_masks==b.wire_known_player_masks&&
  same_record(a.slots,b.slots)&&same_record(a.archive,b.archive)&&same_info(a.calendar,b.calendar)&&same_info(a.climate,b.climate)&&
  same_info(a.sea_level,b.sea_level)&&same_info(a.turn_timer,b.turn_timer)&&same_info(a.world,b.world)&&
  same_options(a.game_options,b.game_options)&&same_options(a.map_options,b.map_options);
}
}
void BnwImportedPregameReadCache::store(){need(stores_<UINT64_MAX,"Pregame Read->Sync native occurrence exhausted");++stores_;}
BnwImportedPregameReadCache::BnwImportedPregameReadCache(const saves::SavedSession& source,saves::PregameObservation& actual,
 const content::RulesDatabase& db,const SessionCheckpointIdentity& identity,bool require_local_sp_startup,
 std::shared_ptr<const BnwImportedPregameLocaleOwner> actual_locale):identity_(identity){
 // Called after real import validation, before CvGame::Read/reset or UI VM.
 // Re-read actual immutable original prefix, never trust a caller's snapshot.
 need(source.source.source_prefix&&source.snapshot.pregame&&actual.layout==saves::GameStateLayout::expansion2_version1,
  "Pregame imported actual source prefix/layout absent");
 need(identify_bytes(source.source.bytes).sha256==identity_.payload_sha256&&
  identify_bytes(db.gameplay_fingerprint_data()).sha256==identity_.rules_sha256,
  "Pregame imported payload/rules identity differs");
 rules_current_=db.immutable_preview_fingerprint_currentness();
 need(rules_current_&&rules_current_(),"Pregame imported selected rules not retained current");
 saves::validate_source_prefix(*source.source.source_prefix,source.source.header);
 prefix_sha256_=identify_bytes(*source.source.source_prefix).sha256;
 // This is an explicit import API boundary, never a missing-input fallback.
 // Full imported-game startup owns this SAME namespace before Read. A data-only
 // diagnostic executes Read/Sync below without claiming the SP process/client.
 if(require_local_sp_startup){
  need(actual_locale!=nullptr,"Imported-game startup requires genuine early Bank/Locale before Read");
  actual_sp_startup_before_read_=std::unique_ptr<BnwSpClientPregameStartup>(new BnwSpClientPregameStartup(
   *this,read_archive_,source,db,identity_,std::move(actual_locale)));
  actual_sp_startup_before_read_->complete_before_read_prefix();
  need(actual_sp_startup_before_read_->registration_returned_,"Imported-game SP registration did not return before Read");
 }else{
  need(!actual_locale,"Data-only import cannot accept an unused gameplay startup lender");
 }
 read_archive_=saves::observe_pregame(*source.source.source_prefix,source.source.header,saves::GameStateLayout::expansion2_version1);
 need(same_read(read_archive_,*source.snapshot.pregame)&&same_read(read_archive_,actual),"Pregame actual Read result differs from immutable wire");
 bool quick_mirrored=false;
 const auto current=[&]{
  need(std::this_thread::get_id()==thread_&&!poisoned_.load()&&rules_current_&&rules_current_(),"Pregame actual Read->Sync source retired");
  auto expected=read_archive_;if(quick_mirrored)expected.archive.flags.at("quick_combat")=native_quick_combat_;
  need(same_read(expected,actual),"Pregame actual archive changed during Read->Sync");
 };
 try{
  // Native ReadArchive rebuilds this hash BEFORE Sync. Colliding hashes select
  // last serialized index; later string getter retains its first-prefix rule.
  current();store();option_hash_.clear();
  for(std::size_t i=0;i<read_archive_.game_options.size();++i){
   current();const auto& raw=read_archive_.game_options[i].name;
   const auto name=std::string_view(raw).substr(0,raw.find('\0'));
   store();option_hash_[content::saved_info_type_hash(name)]=i;current();
  }
  // Semantic body reused from frozen newgame-pregame-world-join-source1.
  // Exact ConvertGameOptionTypeToString order; DISABLE_START_BIAS16 NULL.
  static constexpr std::array<const char*,22> names{
   "GAMEOPTION_NO_CITY_RAZING","GAMEOPTION_NO_BARBARIANS","GAMEOPTION_RAGING_BARBARIANS",
   "GAMEOPTION_ALWAYS_WAR","GAMEOPTION_ALWAYS_PEACE","GAMEOPTION_ONE_CITY_CHALLENGE",
   "GAMEOPTION_NO_CHANGING_WAR_PEACE","GAMEOPTION_NEW_RANDOM_SEED","GAMEOPTION_LOCK_MODS",
   "GAMEOPTION_COMPLETE_KILLS","GAMEOPTION_NO_GOODY_HUTS","GAMEOPTION_RANDOM_PERSONALITIES",
   "GAMEOPTION_POLICY_SAVING","GAMEOPTION_PROMOTION_SAVING","GAMEOPTION_END_TURN_TIMER_ENABLED",
   "GAMEOPTION_QUICK_COMBAT",nullptr,"GAMEOPTION_NO_SCIENCE","GAMEOPTION_NO_POLICIES",
   "GAMEOPTION_NO_HAPPINESS","GAMEOPTION_NO_TUTORIAL","GAMEOPTION_NO_RELIGION"};
  for(std::size_t i=0;i<names.size();++i){
   current();std::int32_t value=0;
   if(names[i]){
    bool found=false;const std::string_view requested=names[i];
    for(const auto& option:read_archive_.game_options){
     const auto name=std::string_view(option.name).substr(0,option.name.find('\0'));
     if(requested.starts_with(name)){value=i32(option.value);found=true;break;}
    }
    if(!found){
     // Execute/Step false leaves original initialized local0, including missing
     // default row. A returned cache requires the exact actual query call.
     (void)db.query_rows({"Select \"Default\" from GameOptions where Type = ? LIMIT 1",{std::string(requested)}},
      [&](const content::RulesRowView& row){current();value=i32(row.integer_at(0));current();},1);
     current();
    }
   }
   store();native_enum_[i]=value;
   if(i==15){
    need(actual.archive.flags.contains("quick_combat"),"Pregame Read quickCombat field absent");
    store();native_quick_combat_=value!=0;actual.archive.flags.at("quick_combat")=native_quick_combat_;quick_mirrored=true;
   }
  }
  current();need(stores_==24+read_archive_.game_options.size(),"Pregame ReadArchive/hash/Sync native stores incomplete");
  // The native quickCombat singleton store/mirror occurred exactly at slot15,
  // before slots16..21. Partial history cannot delay or repeat this effect.
  need(rules_current_&&rules_current_(),"Pregame final Sync rules return retired");sync_returned_=true;
 }catch(...){poisoned_.store(true);throw;}
}
void BnwImportedPregameReadCache::bind_actual_restored(const std::shared_ptr<const saves::SavedSession>& source,
 const std::shared_ptr<LiveSessionState>& world,std::uint64_t generation){
 need(std::this_thread::get_id()==thread_&&!bound_&&!poisoned_.load()&&sync_returned_&&source&&world&&generation,
  "Pregame native Sync owner cannot bind restored facade");
 need(world->world.pregame&&same_options(world->world.pregame->game_options,read_archive_.game_options)&&
  world->world.pregame->archive.flags.at("quick_combat")==native_quick_combat_&&
  source->source.source_prefix&&identify_bytes(*source->source.source_prefix).sha256==prefix_sha256_,
  "Pregame Sync output differs before live publication");
 source_=source;world_=world;generation_=generation;bound_=true;
}
void BnwImportedPregameReadCache::require_actual(const BnwStockLuaAttemptLease::ScopeIdentity& scope)const{
 if(std::this_thread::get_id()!=thread_){poisoned_.store(true);throw std::runtime_error("Pregame enum foreign thread before mutable reads");}
 try{
  // Read/Sync alone cannot issue the client Player2/NetProxy facade. This is
  // checked before reading even cached scalar options, not a ready DTO bit.
  need(actual_sp_startup_before_read_&&actual_sp_startup_before_read_->registration_returned_,
   "Diagnostic Read/Sync import has no local SP startup/client authority");
  actual_sp_startup_before_read_->current();
  need(!poisoned_.load()&&sync_returned_&&bound_&&generation_&&scope.creator==thread_&&scope.control&&scope.source_owner&&scope.vm,
   "Pregame enum Read->Sync origin retired");
  const auto world=world_.lock();const auto source=source_.lock();
  need(world&&source&&world.get()==scope.world&&world->world.pregame&&world->setup.profile==content::TechnologyProfile::bnw&&
   world->world.pregame->layout==saves::GameStateLayout::expansion2_version1&&
   same_options(world->world.pregame->game_options,read_archive_.game_options)&&
   world->world.pregame->archive.flags.at("quick_combat")==native_quick_combat_&&rules_current_&&rules_current_(),
   "Pregame actual native cache/world/rules no longer match returned Sync");
 }catch(...){poisoned_.store(true);throw;}
}
void BnwImportedPregameReadCache::set_actual_client_simultaneous(const BnwStockLuaAttemptLease::ScopeIdentity& scope,bool requested)const{
 require_actual(scope);
 try{
  actual_sp_startup_before_read_->set_actual_inner_simultaneous(requested);
  require_actual(scope);
 }catch(...){poisoned_.store(true);throw;}
}
void BnwImportedPregameReadCache::update_actual_client_frame(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan,float seconds)const{
 loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();
 try{actual_sp_startup_before_read_->update_actual_client_frame(loan,seconds);loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();}
 catch(...){poisoned_.store(true);throw;}
}
void BnwImportedPregameReadCache::require_game_option_namespace_owned(const BnwStockLuaAttemptLease::ScopeIdentity& scope)const{
 require_actual(scope);
 try{
  const auto world=world_.lock();need(world&&world.get()==scope.world,"Game.IsOption SAME restored world retired");
  // restore_research_context copied the SAME wire options at its actual birth.
  // Its own later SQL-default entries may append, but cannot replace that seed.
  const auto& setup=world->setup.game_options;
  need(setup.size()>=read_archive_.game_options.size(),"Game.IsOption restored setup option seed truncated");
  for(std::size_t i=0;i<read_archive_.game_options.size();++i)
   need(setup[i].name==read_archive_.game_options[i].name&&setup[i].value==read_archive_.game_options[i].value,
    "Game.IsOption restored setup option seed differs from actual Read/Sync namespace");
  require_actual(scope);
 }catch(...){poisoned_.store(true);throw;}
}
bool BnwImportedPregameReadCache::game_is_option(const BnwStockLuaAttemptLease::ScopeIdentity& scope,std::int32_t selector)const{
 require_game_option_namespace_owned(scope);
 // Original unsigned range selects the actual22 enum cache. Outside it the
 // original reads custom hash/GC Info/default suppliers, not an array slot.
 const auto index=static_cast<std::uint32_t>(selector);
 need(index<native_enum_.size(),"Game.IsOption numeric hash/default selector requires its actual GC Info supplier");
 const auto value=enum_value(scope,index);
 require_game_option_namespace_owned(scope);return value==1;
}
bool BnwImportedPregameReadCache::game_is_option(const BnwStockLuaAttemptLease::ScopeIdentity& scope,
 const char* requested,const content::RulesDatabase& actual_db)const{
 require_game_option_namespace_owned(scope);
 try{
  // lua_tostring(NULL/non-string) and an empty CString reach the original
  // GetGameOption false branch; CvGame's initialized local remains0.
  if(!requested||!*requested){require_game_option_namespace_owned(scope);return false;}
  need(identify_bytes(actual_db.gameplay_fingerprint_data()).sha256==identity_.rules_sha256,
   "Game.IsOption actual selected database differs from Read/Sync rules");
  const std::string_view name(requested);std::int32_t value=0;bool found=false;
  for(const auto& option:read_archive_.game_options){
   const auto saved=std::string_view(option.name).substr(0,option.name.find('\0'));
   // Original first serialized CString prefix, including an empty saved name.
   if(name.starts_with(saved)){value=i32(option.value);found=true;break;}
  }
  if(!found){
   (void)actual_db.query_rows({"Select \"Default\" from GameOptions where Type = ? LIMIT 1",{std::string(name)}},
    [&](const content::RulesRowView& row){require_game_option_namespace_owned(scope);value=i32(row.integer_at(0));require_game_option_namespace_owned(scope);},1);
   require_game_option_namespace_owned(scope); // Execute/Step false retains0.
  }
  require_game_option_namespace_owned(scope);return value==1;
 }catch(...){poisoned_.store(true);throw;}
}
std::int32_t BnwImportedPregameReadCache::enum_value(const BnwStockLuaAttemptLease::ScopeIdentity& scope,std::size_t slot)const{
 require_actual(scope);need(slot<native_enum_.size(),"Pregame native legacy enum index absent");
 const auto value=native_enum_[slot];require_actual(scope);return value;
}
std::int32_t BnwImportedPregameReadCache::string_option(const BnwStockLuaAttemptLease::ScopeIdentity& scope,
 std::string_view requested,const content::RulesDatabase& actual_db)const{
 // Original1019B130 string overload, independently of native legacy22 cache.
 // Private client supplies its retained canonical selected DB; no caller DTO
 // or receipt can issue this read. Fixed names are the actual +100 operands.
 require_actual(scope);
 try{
  need(requested=="GAMEOPTION_DYNAMIC_TURNS"||requested=="GAMEOPTION_SIMULTANEOUS_TURNS",
   "Pregame Player2 string option operand not original +100");
  need(identify_bytes(actual_db.gameplay_fingerprint_data()).sha256==identity_.rules_sha256,
   "Pregame actual Source selected database differs from Read->Sync rules");
  require_actual(scope);std::int32_t value=0;bool found=false;
  for(const auto& option:read_archive_.game_options){
   const auto name=std::string_view(option.name).substr(0,option.name.find('\0'));
   // Native strncmp(savedName,requested,strlen(savedName)); first prefix wins,
   // including the zero-length saved CString. It is not equality/hash lookup.
   if(requested.starts_with(name)){value=i32(option.value);found=true;break;}
  }
  if(!found){
   (void)actual_db.query_rows({"Select \"Default\" from GameOptions where Type = ? LIMIT 1",{std::string(requested)}},
    [&](const content::RulesRowView& row){require_actual(scope);value=i32(row.integer_at(0));require_actual(scope);},1);
   require_actual(scope); // Exact missing/Execute/Step false retains local0.
  }
  require_actual(scope);return value;
 }catch(...){poisoned_.store(true);throw;}
}
}

namespace civ5::runtime {
void BnwImportedPregameReadCache::begin_actual_control_callback(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan,std::int32_t actor)const{
 loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();
 try{actual_sp_startup_before_read_->begin_actual_control_callback(loan,actor);loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();}
 catch(...){poisoned_.store(true);throw;}
}
void BnwImportedPregameReadCache::publish_actual_control_client_event(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan,std::uint32_t type)const{
 loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();
 try{actual_sp_startup_before_read_->publish_actual_control_client_event(loan,type);loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();}
 catch(...){poisoned_.store(true);throw;}
}
std::int32_t BnwImportedPregameReadCache::actual_control_client_local_id(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();
 try{const auto value=actual_sp_startup_before_read_->actual_control_client_local_id(loan);loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();return value;}
 catch(...){poisoned_.store(true);throw;}
}
void BnwImportedPregameReadCache::complete_actual_control_arrays(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();
 try{actual_sp_startup_before_read_->complete_actual_control_arrays(loan);loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();}
 catch(...){poisoned_.store(true);throw;}
}
bool BnwImportedPregameReadCache::actual_control_client_c8(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();
 try{const auto value=actual_sp_startup_before_read_->actual_control_client_c8(loan);loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();return value;}
 catch(...){poisoned_.store(true);throw;}
}
bool BnwImportedPregameReadCache::actual_control_client_is_host(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();
 try{const auto value=actual_sp_startup_before_read_->actual_control_client_is_host(loan);loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();return value;}
 catch(...){poisoned_.store(true);throw;}
}
std::int32_t BnwImportedPregameReadCache::actual_control_client_sender_local_id(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();
 try{const auto value=actual_sp_startup_before_read_->actual_control_client_sender_local_id(loan);loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();return value;}
 catch(...){poisoned_.store(true);throw;}
}
}

namespace civ5::runtime {
void BnwImportedPregameReadCache::submit_actual_client155(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();
 try{actual_sp_startup_before_read_->submit_actual_client155(loan);loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();}
 catch(...){poisoned_.store(true);throw;}
}
}

namespace civ5::runtime {
void BnwImportedPregameReadCache::require_actual_control_proxy_ui(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();
 try{actual_sp_startup_before_read_->require_actual_control_proxy_ui(loan);loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();}
 catch(...){poisoned_.store(true);throw;}
}
}

namespace civ5::runtime {
void BnwImportedPregameReadCache::complete_actual_control_local_ui_return(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();
 try{actual_sp_startup_before_read_->complete_actual_control_local_ui_return(loan);loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();}
 catch(...){poisoned_.store(true);throw;}
}
}

namespace civ5::runtime {
std::int32_t BnwImportedPregameReadCache::begin_actual_client155_callback(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();
 try{const auto value=actual_sp_startup_before_read_->begin_actual_client155_callback(loan);loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();return value;}
 catch(...){poisoned_.store(true);throw;}
}
}

namespace civ5::runtime {
void BnwImportedPregameReadCache::complete_actual_client155_ready_store(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();
 try{actual_sp_startup_before_read_->complete_actual_client155_ready_store(loan);loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();}
 catch(...){poisoned_.store(true);throw;}
}
}

namespace civ5::runtime {
void BnwImportedPregameReadCache::publish_actual_client155_event(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan,std::uint32_t type)const{
 loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();
 try{actual_sp_startup_before_read_->publish_actual_client155_event(loan,type);loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();}
 catch(...){poisoned_.store(true);throw;}
}
}

namespace civ5::runtime {
std::int32_t BnwImportedPregameReadCache::actual_client155_local_id(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();
 try{const auto value=actual_sp_startup_before_read_->actual_client155_local_id(loan);loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();return value;}
 catch(...){poisoned_.store(true);throw;}
}
}

namespace civ5::runtime {
void BnwImportedPregameReadCache::require_actual_client155_core_suffix(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();
 try{actual_sp_startup_before_read_->require_actual_client155_core_suffix(loan);loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();}
 catch(...){poisoned_.store(true);throw;}
}
}

namespace civ5::runtime {
bool BnwImportedPregameReadCache::actual_engine_network_game_owned(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();
 try{
  // Actual5F7B80 obtains a freshly allocated CvDllPreGame wrapper. Its
  // ctor/dtor are empty; Destroy frees it. It owns no Game lock/refcount.
  // This concrete private carrier is borrowed only within this SAME read.
  struct PregameInterface final {};
  auto interface=std::make_unique<PregameInterface>();
  require_actual(scope);loan.require_current_without_callbacks();
  need(read_archive_.archive_version==6,"SP Utility unverified native Pregame Read version");
  const auto found=read_archive_.archive.integers.find("game_type");
  need(found!=read_archive_.archive.integers.end(),"SP Utility actual Read gameType absent");
  const auto type=i32(found->second);
  // CvDllPreGame::isNetworkMultiplayerGame -> CvPreGame::gameType == enum1.
  // Cache is the actual immutable-prefix Read owner, not world snapshot.
  const bool multiplayer=type==1;
  require_actual(scope);loan.require_current_without_callbacks();
  interface.reset(); // real acquired wrapper Destroy precedes branch
  require_actual(scope);loan.require_current_without_callbacks();return multiplayer;
 }catch(...){poisoned_.store(true);throw;}
}
void BnwImportedPregameReadCache::complete_actual_client155_local_return(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();
 try{actual_sp_startup_before_read_->complete_actual_client155_local_return(loan);loan.require_current_without_callbacks();require_actual(scope);loan.require_current_without_callbacks();}
 catch(...){poisoned_.store(true);throw;}
}
}

namespace civ5::runtime {
std::int32_t BnwImportedPregameReadCache::actual_engine_scheduler_last_turn_owned(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan)const{
 loan.require_imported_cache_owned(*this);require_actual(scope);loan.require_imported_cache_owned(*this);
 try{const auto value=actual_sp_startup_before_read_->actual_engine_scheduler_last_turn_owned(loan);
  loan.require_imported_cache_owned(*this);require_actual(scope);loan.require_imported_cache_owned(*this);return value;}
 catch(...){poisoned_.store(true);throw;}
}
}

namespace civ5::runtime {
std::int32_t BnwImportedPregameReadCache::actual_frame_proxy_map_count_owned(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan)const{
 loan.require_imported_cache_owned(*this);require_actual(scope);loan.require_imported_cache_owned(*this);
 try{const auto value=actual_sp_startup_before_read_->actual_frame_proxy_map_count_owned(loan);
  loan.require_imported_cache_owned(*this);require_actual(scope);loan.require_imported_cache_owned(*this);return value;}
 catch(...){poisoned_.store(true);throw;}
}
}

namespace civ5::runtime {
bool BnwImportedPregameReadCache::actual_frame_proxy_inner45_owned(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan)const{
 loan.require_imported_cache_owned(*this);require_actual(scope);loan.require_imported_cache_owned(*this);
 try{const auto value=actual_sp_startup_before_read_->actual_frame_proxy_inner45_owned(loan);
  loan.require_imported_cache_owned(*this);require_actual(scope);loan.require_imported_cache_owned(*this);return value;}
 catch(...){poisoned_.store(true);throw;}
}
}

namespace civ5::runtime {
bool BnwImportedPregameReadCache::actual_frame_proxy_pending_event150_owned(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan)const{
 loan.require_imported_cache_owned(*this);require_actual(scope);loan.require_imported_cache_owned(*this);
 try{const auto value=actual_sp_startup_before_read_->actual_frame_proxy_pending_event150_owned(loan);
  loan.require_imported_cache_owned(*this);require_actual(scope);loan.require_imported_cache_owned(*this);return value;}
 catch(...){poisoned_.store(true);throw;}
}
}

namespace civ5::runtime {
std::int32_t BnwImportedPregameReadCache::actual_frame_loadstate_last_turn_owned(const BnwStockLuaAttemptLease::ScopeIdentity& scope,const BnwSpClientUpdateLoan& loan)const{
 loan.require_imported_cache_owned(*this);require_actual(scope);loan.require_imported_cache_owned(*this);
 try{const auto value=actual_sp_startup_before_read_->actual_frame_loadstate_last_turn_owned(loan);
  loan.require_imported_cache_owned(*this);require_actual(scope);loan.require_imported_cache_owned(*this);return value;}
 catch(...){poisoned_.store(true);throw;}
}
}

#include "bnw_frame_imported_pregame_provider.inc"
