#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <windows.h>
#endif
#include <array>
#include <algorithm>
#include <mutex>
#include <bit>
#include <optional>
#include <cfenv>
#include <cmath>
#include <new>
#include "civ5/runtime/bnw_sp_client_pregame_startup.hpp"
#include "civ5/runtime/bnw_imported_pregame_locale_owner.hpp"
#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/bnw_sp_client_update_loan.hpp"
#include <limits>
#include <stdexcept>
namespace civ5::runtime {
namespace {
void need(bool ok,const char*why){if(!ok)throw std::runtime_error(why);}
void increment(std::uint64_t& n){need(n<UINT64_MAX,"SP boot native history exhausted");++n;}
}
#include "bnw_sp_client_pregame_context.inc"
#include "bnw_sp_client_local_parent.inc"
#include "bnw_sp_client_imported_globals.inc"
#include "bnw_sp_client_allowed_dlc.inc"
#include "bnw_sp_client_nickname.inc"
BnwSpClientPregameStartup::BnwSpClientPregameStartup(BnwImportedPregameReadCache& owner,
 saves::PregameObservation& actual,const saves::SavedSession& source,const content::RulesDatabase& db,
 const SessionCheckpointIdentity& identity,std::shared_ptr<const BnwImportedPregameLocaleOwner> actual_locale)
 :actual_namespace_owner_(&owner),actual_namespace_(actual),
 prefix_(source.source.source_prefix?std::make_shared<const std::vector<std::uint8_t>>(*source.source.source_prefix):nullptr),
 prefix_sha256_(prefix_?identify_bytes(*prefix_).sha256:std::string{}){
 need(prefix_&&identify_bytes(source.source.bytes).sha256==identity.payload_sha256&&
  identify_bytes(db.gameplay_fingerprint_data()).sha256==identity.rules_sha256,
  "SP boot actual pre-Read import/rules origin absent");
 rules_current_=db.immutable_preview_fingerprint_currentness();current();
 // The early bootstrap passed its actual retained Bank/input/TextData child.
 // Missing old/offline callers fail before claiming process/namespace state.
 need(actual_locale!=nullptr,"SP imported boot requires genuine early Locale from actual bootstrap");
 const auto canonical=actual_locale->retain_canonical();
 need(canonical.get()==actual_locale.get()&&!canonical.owner_before(actual_locale)&&!actual_locale.owner_before(canonical),"SP imported boot Locale control differs");
 canonical->require_import(source,db,identity);current();early_locale_=canonical;
 // Claim the real once process lender BEFORE assigning namespace constructor
 // fields. A second import cannot reset an earlier client/namespace lifetime.
 local_parent_=std::make_shared<BootOwner>(*this);
 local_parent_->process->retained=local_parent_;
 pregame_context_=std::make_shared<PreGameContext>(*this);
 local_parent_->retained_pregame_context=pregame_context_;
 pregame_context_->initialize_once();
 // Actual namespace PREGAMEVAR initialization, before ANY archive read.
 // These fields are in the Cache's SAME owned archive allocation, not a fresh
 // game's DTO, imported snapshot, inferred active player or current save value.
 need(actual.archive.integers.empty()&&actual.archive.flags.empty()&&actual.archive.integer_arrays.empty()&&
  actual.archive.flag_arrays.empty()&&actual.archive.string_arrays.empty(),"SP boot requires unread actual namespace");
 actual.archive.integers.emplace("active_player",-1);
 actual.archive.integers.emplace("game_mode",-1);
 actual.archive.flags.emplace("game_started",false);
 for(const char* name:{"slot_status","slot_claims","handicaps","last_human_handicaps"})actual.archive.integer_arrays.emplace(name,std::vector<std::int64_t>(64,0));
 actual.archive.flag_arrays.emplace("ready_players",std::vector<bool>(64,false));
 actual.archive.string_arrays.emplace("nicknames",std::vector<std::string>(64));
 // CvPreGame.cpp292 plain PackageIDList +302 vector<PackageIDList>(64).
 // Actual constructor state is independent of activated/saved DLC content.
 need(allowed_dlc_.empty(),"SP actual allowed-DLC constructor differs");
 for(const auto& list:available_dlc_)need(list.empty(),"SP actual available-DLC constructor differs");
 dlc_namespace_born_=true;
 need(participants_.empty()&&local_id_==-1&&host_id_==-1&&!registration_returned_,"SP boot actual session-info constructor differs");
 cached_globals_=std::make_shared<CachedGlobals>(*this,db);
 local_parent_->retained_globals=cached_globals_;
 local_parent_->retained_locale=early_locale_;
 current();
}
void BnwSpClientPregameStartup::current()const{
 if(std::this_thread::get_id()!=thread_){poisoned_.store(true);throw std::runtime_error("SP boot foreign thread before namespace reads");}
 try{
  need(!poisoned_.load()&&actual_namespace_owner_&&prefix_&&
   identify_bytes(*prefix_).sha256==prefix_sha256_&&rules_current_&&rules_current_(),
   "SP boot actual namespace/source/rules retired");
 }catch(...){poisoned_.store(true);throw;}
}
std::int32_t BnwSpClientPregameStartup::native_local_allocator_prefix(){
 // Original53BFD0(local=true) first obtains actual Pregame then gameMode+A8.
 // Pinned native 10197910 reads the actual namespace scalar. Any nonzero
 // mode takes the original early return0; do NOT substitute actor0 from save.
 current();const auto mode=actual_namespace_.archive.integers.at("game_mode");current();
 if(mode!=0)return 0;
 throw std::runtime_error("SP boot mandatory STOP: original53BFD0 mode0 current slot/nickname allocator suffix unjoined");
}
bool BnwSpClientPregameStartup::native_validate_local_slot(std::int32_t id){
 // Exact42EA90 for genuine empty newborn info and NO host flag: native
 // isHost is false because host_id=-1. gameStarted false accepts status!=2.
 current();need(local_parent_&&id>=0&&id<64,"SP boot local slot outside original range");
 local_parent_->require_before_registration();
 need(!local_parent_->is_host(),"SP boot original host slot-validation branch unjoined");
 need(participants_.empty()&&local_id_==-1&&host_id_==-1&&!registration_returned_,
  "SP boot slot validation requires SAME newborn session-info");
 const auto status=actual_namespace_.archive.integer_arrays.at("slot_status").at(id);
 const bool started=actual_namespace_.archive.flags.at("game_started");current();
 if(started)throw std::runtime_error("SP boot mandatory STOP: started Pregame validation requires actual nickname/slot suffix");
 return status!=2;
}
void BnwSpClientPregameStartup::set_integer(Vector field,std::int32_t id,std::int32_t value){
 current();need(id>=0&&id<64,"SP boot actual integer setter range absent");
 const char* name=nullptr;
 switch(field){case Vector::status:name="slot_status";break;case Vector::claim:name="slot_claims";break;
  case Vector::handicap:name="handicaps";break;case Vector::last_human_handicap:name="last_human_handicaps";break;
  default:throw std::runtime_error("SP boot integer vector setter field unavailable");}
 auto& values=actual_namespace_.archive.integer_arrays.at(name);
 // Pinned native101A2020 performs NO dirty/store/delta on equality.
 if(values.at(id)==value){current();return;}
 // Native archive insertion BEFORE array store, delta append AFTER store.
 // Allocation failures retain precisely the earlier effects, never a return.
 const auto inserted=archive_dirty_.insert(field).second;if(inserted)increment(archive_insertions_);
 increment(field_stores_);values.at(id)=value;
 deltas_.push_back({field,id,value,2});increment(delta_appends_);current();
}
void BnwSpClientPregameStartup::set_ready_false(std::int32_t id){
 current();need(id>=0&&id<64,"SP boot actual ready setter absent");
 auto& values=actual_namespace_.archive.flag_arrays.at("ready_players");
 // Original setReady(false) skips active/canReadyLocal reads. Its bool setAt
 // 1004EDC0 still suppresses all effects on equality; constructor false stays
 // false without a fabricated equal field store or delta.
 if(!values.at(id)){current();return;}
 const auto inserted=archive_dirty_.insert(Vector::ready).second;if(inserted)increment(archive_insertions_);
 increment(field_stores_);values.at(id)=false;
 deltas_.push_back({Vector::ready,id,0,2});increment(delta_appends_);current();
}
void BnwSpClientPregameStartup::native_registration_callback_prefix(){
 current();need(phase_==Phase::registration_prefix&&local_parent_,"SP boot native Pregame callback cannot precede participant insertion/replay");
 local_parent_->require_before_registration();
 const auto actual=participants_.find(selected_registration_id_);
 need(actual!=participants_.end()&&actual->second&&actual->second->id==selected_registration_id_&&
  local_id_==selected_registration_id_&&host_id_==selected_registration_id_&&!registration_returned_,
  "SP boot717F00 requires SAME actual registered participant allocation before effects");
 try{
  need(pregame_context_!=nullptr,"SP native callback actual Pregame Context absent");
  pregame_context_->get_and_query_actual_pregame();
  // Original717F00 obtains ICvPreGame2 (actual SDK QueryInterface returns SAME
  // pointer without a refcount). Then conditional status, claim and ready.
  const auto status=actual_namespace_.archive.integer_arrays.at("slot_status").at(selected_registration_id_);current();
  if(status!=4)set_integer(Vector::status,selected_registration_id_,3);
  set_integer(Vector::claim,selected_registration_id_,2);set_ready_false(selected_registration_id_);
  // Original717F00 invokes VerifyHandicap before local active-player/DLC.
  // Cached Globals belongs to this import/boot child and was stored BEFORE
  // start, not queried lazily or borrowed from a fresh game.
  verify_handicap(selected_registration_id_);
  if(selected_registration_id_==local_id_){
   set_active_player(selected_registration_id_);
   copy_actual_allowed_dlc(selected_registration_id_);
  }
  // Original718010 reads actual hostID, then participant+94. This lookup is
  // independent of DLC completion and does not mint a registration return.
  if(selected_registration_id_==host_id_){
   const auto participant=participants_.find(host_id_);current();
   if(participant!=participants_.end()){
    need(participant->second!=nullptr,"SP native nickname participant allocation absent");
    // 7EAD00: native participant kind +10 -> SAME service+88 tree. Its own
    // constructor is separate from three registry maps and optional5CF2B24.
    const auto found=local_parent_->nickname_userdata.find(participant->second->kind);current();
    if(found!=local_parent_->nickname_userdata.end())
     throw std::runtime_error("SP nickname nonempty real service userdata/profile suffix unjoined");
   }
   const auto nickname_participant=participants_.find(selected_registration_id_);current();
   need(nickname_participant!=participants_.end()&&nickname_participant->second,
    "SP nickname actual second participant lookup failed");
   if(nickname_participant->second->userdata)
    throw std::runtime_error("SP nickname actual participant userdata name suffix unjoined");
   // 7EAE00 constructor-null userdata returns native static empty string.
   // Next71817D constructs Localization::String and performs Lookup, then
   // PushArgument(id+1) in this actual non-host branch. A key echo is invalid.
   need(early_locale_!=nullptr,"SP native nickname early Locale child absent");
   early_locale_->require_current();current();
   const bool native_host=local_parent_->host_flag;current();
   const auto native_name=early_locale_->default_nickname(native_host,selected_registration_id_);
   current();early_locale_->require_current();current();
   set_actual_nickname(selected_registration_id_,native_name);
  }
  // Original718261 is OUTSIDE hostID/nickname condition. Real initialized
  // retained native UI queue records event149, then payload actor718266.
  need(early_locale_!=nullptr,"SP native149 actual early source absent");
  early_locale_->publish_registration_player_event(*this,selected_registration_id_);
  current();pregame_context_->destroy_after_native_event();
  current();native_callback_returned_=true;
 }catch(...){poisoned_.store(true);throw;}
}
void BnwSpClientPregameStartup::update_actual_client_frame(const BnwSpClientUpdateLoan& loan,float seconds){
 loan.require_current_without_callbacks();current();loan.require_current_without_callbacks();
 need(!active_update_loan_,"SP canonical frame loan cannot nest/reenter startup");
 active_update_loan_=&loan;
 struct BorrowExit {const BnwSpClientUpdateLoan*& slot;~BorrowExit(){slot=nullptr;}} exit{active_update_loan_};
 try{
  need(registration_returned_&&local_parent_&&std::isfinite(seconds)&&seconds>=0,
   "SP Update requires genuine returned registration and actual platform delta");
  local_parent_->update_local_mode0_prefix(seconds);local_parent_->current();current();
 }catch(...){if(local_parent_)local_parent_->quarantine();poisoned_.store(true);throw;}
}
void BnwSpClientPregameStartup::set_actual_inner_simultaneous(bool requested){
 current();
 try{
  need(registration_returned_&&local_parent_,"SP NetProxy194 requires SAME returned registration owner");
  local_parent_->current();local_parent_->require_before_registration();
  // Native808AA0 calls +88 (actual +124 info then first-map.size+24),
  // compares signed count to1, and ALWAYS stores inner+84 on either branch.
  // ReadArchive option/cache, active actor, or an ordinary-SP assumption
  // cannot supply this count: this is the retained actual registration map.
  need(participants_.size()<=64,"SP actual registered participant map count unavailable");
  const auto actual_count=static_cast<std::int32_t>(participants_.size());current();
  local_parent_->store(local_parent_->inner_simultaneous,actual_count<=1?false:requested);
  local_parent_->current();current();
 }catch(...){if(local_parent_)local_parent_->quarantine();poisoned_.store(true);throw;}
}
void BnwSpClientPregameStartup::complete_before_read_prefix(){
 current();need(phase_==Phase::namespace_constructed&&local_parent_,"SP boot cannot enter after Read/replayed parent");
 try{
  need(cached_globals_!=nullptr,"SP imported boot actual Globals child absent");
  cached_globals_->load_before_boot();
  local_parent_->local_start_prefix(*this);
  local_parent_->require_before_registration();
  local_parent_->registration_prefix(*this);
 }catch(...){local_parent_->quarantine();poisoned_.store(true);throw;}
}
}

namespace civ5::runtime {
void BnwSpClientPregameStartup::require_actual_control_callback(const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();current();loan.require_current_without_callbacks();
 need(active_update_loan_==&loan&&registration_returned_&&local_parent_,"SP control callback foreign frame/startup");
 const auto& p=*local_parent_;
 need(p.caller==this&&p.partial_frame_drain&&p.partial_frame_drain->popped&&
  p.partial_frame_drain->receiver_entered&&!p.partial_frame_drain->receiver_returned&&
  p.decoder_returned&&p.control_dispatch_entered&&p.received_control&&
  !p.received_control->offset2c&&p.received_control->offset1c==control_actor_,
  "SP control callback lacks actual popped/decoded/unreturned Control lineage");
 const auto actor=participants_.find(control_actor_);
 need(actor!=participants_.end()&&actor->second&&actor->second->id==control_actor_&&actor->second->flag18,
  "SP control callback actual first-map flag18 producer absent");
 loan.require_current_without_callbacks();
}
void BnwSpClientPregameStartup::begin_actual_control_callback(const BnwSpClientUpdateLoan& loan,std::int32_t actor){
 loan.require_current_without_callbacks();current();loan.require_current_without_callbacks();
 need(control_stage_==ControlStage::none,"SP original683250 callback replay/reentry");
 control_actor_=actor;require_actual_control_callback(loan);control_stage_=ControlStage::event146;
}
void BnwSpClientPregameStartup::publish_actual_control_client_event(const BnwSpClientUpdateLoan& loan,std::uint32_t type){
 require_actual_control_callback(loan);
 need((type==146&&control_stage_==ControlStage::event146)||(type==145&&control_stage_==ControlStage::event145),
  "SP original client146/145 event order/replay differs");
 need(early_locale_!=nullptr,"SP client event requires SAME retained boot arena");
 early_locale_->publish_actual_control_client_event(*this,loan,type);
 require_actual_control_callback(loan);control_stage_=type==146?ControlStage::local_id:ControlStage::arrays;
}
std::int32_t BnwSpClientPregameStartup::actual_control_client_local_id(const BnwSpClientUpdateLoan& loan){
 require_actual_control_callback(loan);need(control_stage_==ControlStage::local_id,"SP client localID before146/afterreplay");
 // Actual wrapper has already earned SAME inner/info constructor and returned
 // registration. Original683250 +8C is a read of this retained info.localID.
 local_parent_->require_before_registration();loan.require_current_without_callbacks();
 const auto value=local_id_;control_stage_=value==control_actor_?ControlStage::event145:ControlStage::arrays;
 return value;
}
void BnwSpClientPregameStartup::complete_actual_control_arrays(const BnwSpClientUpdateLoan& loan){
 require_actual_control_callback(loan);need(control_stage_==ControlStage::arrays,"SP client arrays completion before146/145");
 // This is only the private direct caller's source-order bookend, no native
 // array/frame readiness grant. Loan's SAME client performed all four stores.
 control_stage_=ControlStage::c8;
}
bool BnwSpClientPregameStartup::actual_control_client_c8(const BnwSpClientUpdateLoan& loan){
 require_actual_control_callback(loan);need(control_stage_==ControlStage::c8,"SP client +C8 before actual arrays");
 // Original807430 returns SAME inner46, native8050A7 storedfalse before Read.
 const bool value=local_parent_->inner46;control_stage_=value?ControlStage::sender_local:ControlStage::host;return value;
}
bool BnwSpClientPregameStartup::actual_control_client_is_host(const BnwSpClientUpdateLoan& loan){
 require_actual_control_callback(loan);need(control_stage_==ControlStage::host,"SP client isHost called outside false+C8 branch");
 //6832D3 obtains SAME nonnull info;6832E7 obtains it again. Both are the
 // returned retained inner/info, not unrelated optional fresh-game owners.
 local_parent_->require_before_registration();loan.require_current_without_callbacks();
 local_parent_->require_before_registration();loan.require_current_without_callbacks();
 //7F3510 first finds the original info.parentID in the SAME service registry,
 // then invokes that actual inner's +C8. True returns before host/local reads.
 // Its nonserver branch alone compares hostID!=-1 && hostID==localID.
 const auto found=local_parent_->process->registry.find(local_parent_->info_parent_id);
 need(found!=local_parent_->process->registry.end()&&found->second==local_parent_.get(),
  "SP info.isHost actual service registry/parent-ID changed or absent");
 const bool value=found->second->inner46?true:(host_id_!=-1&&host_id_==local_id_);
 control_stage_=value?ControlStage::sender_local:ControlStage::ui_suffix;return value;
}
std::int32_t BnwSpClientPregameStartup::actual_control_client_sender_local_id(const BnwSpClientUpdateLoan& loan){
 require_actual_control_callback(loan);need(control_stage_==ControlStage::sender_local,"SP client sender local-ID original branch missing");
 local_parent_->require_before_registration();loan.require_current_without_callbacks();
 //683306 obtains SAME inner+124 info again after +C8/isHost conditions.
 const auto value=local_id_;control_stage_=ControlStage::sender;return value;
}
}

namespace civ5::runtime {
void BnwSpClientPregameStartup::submit_actual_client155(const BnwSpClientUpdateLoan& loan){
 require_actual_control_callback(loan);need(control_stage_==ControlStage::sender,"SP155 requires earned original local sender branch");
 try{
  local_parent_->submit_client155();require_actual_control_callback(loan);
  need(local_parent_->client155&&local_parent_->client155->submit_returned&&
   local_parent_->client155_stream&&local_parent_->client155_stream->destroyed,
   "SP155 actual sender/stream destruction not returned");
  control_stage_=ControlStage::ui_suffix;
 }catch(...){if(local_parent_)local_parent_->quarantine();poisoned_.store(true);throw;}
}
}

namespace civ5::runtime {
void BnwSpClientPregameStartup::require_actual_control_proxy_ui(const BnwSpClientUpdateLoan& loan)const{
 require_actual_control_callback(loan);need(control_stage_==ControlStage::client_returned,
  "SP actual494940 optional UI source before683250 return");
 //494964 actual inner+124/804EA0 returns SAME retained info after callback.
 local_parent_->require_before_registration();loan.require_current_without_callbacks();
}
}

namespace civ5::runtime {
void BnwSpClientPregameStartup::complete_actual_control_local_ui_return(const BnwSpClientUpdateLoan& loan){
 require_actual_control_callback(loan);need(control_stage_==ControlStage::ui_suffix,
  "SP actual59FD50 source return before original sender or replay");
 // This first callback is the CURRENT decoded Control1208 actor.field1c.
 // The queued33-byte155 has not been decoded and supplies no authority here.
 local_parent_->require_before_registration();loan.require_current_without_callbacks();
 const auto actual_local=local_id_;const auto actual_actor=local_parent_->received_control->offset1c;
 need(actual_actor==control_actor_,"SP actual59FD50 current Control actor replaced");
 if(actual_actor!=actual_local)
  throw std::runtime_error("SP59FD50 nonlocal application/UI suffix remains mandatory");
 // Selected Civis launcher bounded observable-parity normalization approved
 // by integration owner: original App+6F0/type9 predicate consists only of
 // memory reads. For this exact genuine local actor, BOTH predicate outcomes
 // return with no observable writes/callbacks. No App pointer/type9/read receipt,
 // UI-screen readiness or message155 processing is manufactured or exported.
 // True path59FD94->59FE55; false/NULL path59FDB8->59FE54->59FE55.
 require_actual_control_callback(loan);control_stage_=ControlStage::client_returned;
}
}

namespace civ5::runtime {
void BnwSpClientPregameStartup::require_actual_client155_callback(const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();current();loan.require_current_without_callbacks();
 need(active_update_loan_==&loan&&registration_returned_&&local_parent_,
  "SP155 callback requires SAME returned startup/current typed frame");
 const auto& p=*local_parent_;
 need(p.caller==this&&p.second_frame_pop_entered&&p.partial_frame_drain&&p.partial_frame_drain->popped&&
  p.partial_frame_drain->receiver_entered&&!p.partial_frame_drain->receiver_returned&&
  p.control_process_returned&&p.control_message_destroyed&&!p.received_control&&
  p.client155_decoder_returned&&p.client155_value_read&&p.client155_dispatch_entered&&p.received_client155&&
  p.received_client155->constructed&&!p.received_client155->retired&&!p.received_client155->offset2c&&
  p.received_client155->actor==client155_actor_,
  "SP155 callback lacks actual decoded/unreturned second popped message lineage");
 loan.require_current_without_callbacks();
}
std::int32_t BnwSpClientPregameStartup::begin_actual_client155_callback(const BnwSpClientUpdateLoan& loan){
 loan.require_current_without_callbacks();current();loan.require_current_without_callbacks();
 need(client155_stage_==Client155Stage::none&&local_parent_&&local_parent_->received_client155,
  "SP155 canonical callback replay/reentry");
 client155_actor_=local_parent_->received_client155->actor;require_actual_client155_callback(loan);
 client155_stage_=Client155Stage::ready_store;return client155_actor_;
}
void BnwSpClientPregameStartup::complete_actual_client155_ready_store(const BnwSpClientUpdateLoan& loan){
 require_actual_client155_callback(loan);need(client155_stage_==Client155Stage::ready_store,
  "SP155 source-order bookend before actual71AA30 or replay");
 // Source-specific private caller bookend: only SAME Loan client performed
 // the valid-index native equal-store. This is not a Game/EndTurn receipt.
 client155_stage_=Client155Stage::event146;
}
void BnwSpClientPregameStartup::publish_actual_client155_event(const BnwSpClientUpdateLoan& loan,std::uint32_t type){
 require_actual_client155_callback(loan);need(early_locale_&&
  ((type==146&&client155_stage_==Client155Stage::event146)||(type==132&&client155_stage_==Client155Stage::event132)),
  "SP155 original146/132 event order or arena absent");
 early_locale_->publish_actual_client155_event(*this,loan,type);require_actual_client155_callback(loan);
 client155_stage_=type==146?Client155Stage::local_id:Client155Stage::core_suffix;
}
std::int32_t BnwSpClientPregameStartup::actual_client155_local_id(const BnwSpClientUpdateLoan& loan){
 require_actual_client155_callback(loan);need(client155_stage_==Client155Stage::local_id,
  "SP155 original fresh +8C local-ID read before146 or replay");
 local_parent_->require_before_registration();loan.require_current_without_callbacks();
 const auto actual=local_id_;
 client155_stage_=actual==client155_actor_?Client155Stage::event132:Client155Stage::core_suffix;
 return actual;
}
void BnwSpClientPregameStartup::require_actual_client155_core_suffix(const BnwSpClientUpdateLoan& loan)const{
 require_actual_client155_callback(loan);need(client155_stage_==Client155Stage::core_suffix,
  "SP155 App1460/+3AC before actual71AA30/146/freshlocal/132 prefix");
}
}

namespace civ5::runtime {
void BnwSpClientPregameStartup::complete_actual_client155_local_return(const BnwSpClientUpdateLoan& loan){
 require_actual_client155_callback(loan);need(client155_stage_==Client155Stage::core_suffix,
  "SP155 original UI return before actual Utility3AC or replay");
 //47652B reads actual received byte34 AFTER3AC returns; false directly
 // returns476545. It is unrelated to the earlier unconditional ready store.
 const auto& actual=*local_parent_->received_client155;
 if(actual.value){
  local_parent_->require_before_registration();loan.require_current_without_callbacks();
  const auto fresh_local=local_id_;
  if(actual.actor!=fresh_local)throw std::runtime_error("SP155 nonlocal59FD50 UI effects remain mandatory");
  // Root-approved bounded selected-launcher observable-parity port of
  //59FD50 for THIS decoded155 actor. Pure App/type predicate, both original
  // predicate outcomes return without writes/callbacks for genuine local ID.
  // This supplies no App/type9/UI/EndTurn receipt and never borrows1208 actor.
 }
 require_actual_client155_callback(loan);client155_stage_=Client155Stage::client_returned;
}
}

namespace civ5::runtime {
std::int32_t BnwSpClientPregameStartup::actual_engine_scheduler_last_turn_owned(const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();current();loan.require_current_without_callbacks();
 // Outer53C990 follows inner-update return: its RAII inner borrow is no
 // longer active. SAME canonical outer Loan/cache join is checked by the
 // private Cache lender; this readonly field never lends a callback/ACK.
 need(registration_returned_&&local_parent_,
  "SP Utility3F4 requires actual returned/current startup and owned process");
 local_parent_->require_before_registration();loan.require_current_without_callbacks();
 const auto actual=local_parent_->process->scheduler_last_turn;
 current();loan.require_current_without_callbacks();return actual;
}
}

namespace civ5::runtime {
std::int32_t BnwSpClientPregameStartup::actual_frame_proxy_map_count_owned(const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();current();loan.require_current_without_callbacks();
 need(registration_returned_&&local_parent_,"SP outer readonly proxy requires actual returned Boot");
 local_parent_->require_before_registration();
 need(participants_.size()<=64,"SP proxy88 native map count domain changed");
 const auto value=static_cast<std::int32_t>(participants_.size());
 current();loan.require_current_without_callbacks();return value;
}
}

namespace civ5::runtime {
bool BnwSpClientPregameStartup::actual_frame_proxy_inner45_owned(const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();current();loan.require_current_without_callbacks();
 need(registration_returned_&&local_parent_,"SP outer readonly proxy requires actual returned Boot");
 local_parent_->require_before_registration();
 const bool value=local_parent_->inner45; //807140, SAME constructor8050A4
 current();loan.require_current_without_callbacks();return value;
}
}

namespace civ5::runtime {
bool BnwSpClientPregameStartup::actual_frame_proxy_pending_event150_owned(const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();current();loan.require_current_without_callbacks();
 need(registration_returned_&&local_parent_,"SP outer readonly proxy requires actual returned Boot");
 local_parent_->require_before_registration();
 const auto* actual_context=local_parent_->context.get();
 need(actual_context!=nullptr,"SP returned Boot lost genuine context14");
 //808E80 reads actual context7C first; native mode0 returns false without
 // Steam flag/lock effects. Mode2's real D5 read/clear remains mandatory.
 if(actual_context->mode==2)throw std::runtime_error("SP original808E80 mode2 Steam flag/critical section required");
 const bool value=false; // source-proven reached non-mode2 pure branch
 current();loan.require_current_without_callbacks();return value;
}
}

namespace civ5::runtime {
std::int32_t BnwSpClientPregameStartup::actual_frame_loadstate_last_turn_owned(const BnwSpClientUpdateLoan& loan)const{
 loan.require_current_without_callbacks();current();loan.require_current_without_callbacks();
 need(registration_returned_&&local_parent_,"SP outer readonly proxy requires actual returned Boot");
 local_parent_->require_before_registration();
 const auto value=local_parent_->process->loadstate_last_turn;
 current();loan.require_current_without_callbacks();return value;
}
}

#include "bnw_frame_game_player_provider.inc"
