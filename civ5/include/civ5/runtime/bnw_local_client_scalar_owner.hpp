#pragma once
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/bnw_game_active_team_read.hpp"
#include "civ5/runtime/bnw_imported_pregame_read_cache.hpp"
#include <mutex>
#include <utility>
#include <optional>
#include <string_view>
#include <atomic>
#include <array>
#include <cstdint>
#include <deque>
#include <limits>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>
namespace civ5::runtime {
// Concrete standalone SP client storage owned once by the actual StockBootstrap.
// Birth is private and requires its actual newly constructed VM/control/world.
// No save receipt, DTO, public factory, bool observer, or delivered-history join.
class BnwSpClientUpdateLoan;
class BnwLocalClientScalarOwner final {
public:
 BnwLocalClientScalarOwner(const BnwLocalClientScalarOwner&)=delete;
 BnwLocalClientScalarOwner& operator=(const BnwLocalClientScalarOwner&)=delete;
 void require_current()const {
  if(std::this_thread::get_id()!=creator_){poisoned_.store(true);throw std::runtime_error("SP client foreign actual creator");}
  if(poisoned_.load()||!reset_returned_)throw std::runtime_error("SP client actual reset/work owner retired");
  try{require_source();}catch(...){poisoned_.store(true);throw;}
 }
 bool has_sent_turn_complete(std::int32_t actual_active)const {
  require_current();need_actor(actual_active,true);const bool value=actual_active==-1?false:sent_complete_;
  require_current();return value;
 }
 bool has_sent_turn_all_complete(std::int32_t actual_active)const {
  require_current();need_actor(actual_active,true);const bool value=actual_active==-1?false:sent_all_complete_;
  require_current();return value;
 }
 bool is_processing_messages()const {
  require_current();const bool value=processing_work_!=0;require_current();return value;
 }
private:
 friend class BnwSelectedStockLuaSourceOwner;
 friend class BnwLuaCalculatedPlotYieldOwner;
 friend class BnwSpClientUpdateLoan;
 void update_tick_owned(const BnwSpClientUpdateLoan&);
 void require_actual_frame_postcontext_prefix_owned(const BnwSpClientUpdateLoan&);
 bool frame_has_actual_in_game_context_owned(const BnwSpClientUpdateLoan&);
 void execute_frame_game_timer_owned(const BnwSpClientUpdateLoan&);
 void control_registration_callback_owned(const BnwSpClientUpdateLoan&,std::int32_t);
 void control_proxy_optional_ui_owned(const BnwSpClientUpdateLoan&);
 void client155_registration_callback_owned(const BnwSpClientUpdateLoan&);
 bool frame_engine_turn_boundary_owned(const BnwSpClientUpdateLoan&);
 bool frame_has_pending_load_vector_owned(const BnwSpClientUpdateLoan&,std::int32_t);
 bool frame_loadstate_turn_boundary_owned(const BnwSpClientUpdateLoan&);
 void require_frame_native_proxy00_owned(const BnwSpClientUpdateLoan&)const;
 const std::thread::id creator_=std::this_thread::get_id();
 const BnwStockLuaAttemptLease source_;
 const BnwSelectedStockLuaBootstrap* const vm_;
 const std::weak_ptr<BnwStockLuaAttemptLease::Control> control_;
 const std::weak_ptr<const void> source_owner_;
 const void* const source_get_;
 const std::weak_ptr<const LiveSessionState> world_;
 const std::shared_ptr<const BnwImportedPregameReadCache> imported_pregame_;
 const content::RulesDatabase* const actual_selected_db_;
 mutable std::atomic<bool> poisoned_{false};
 bool reset_returned_=false,sent_complete_=false,sent_all_complete_=false,sent_unready_=false,recording_=false;
 std::int32_t processing_work_=0,scheduled_returns_=0,recording_gate_=0,slot_240_=-1,slot_24c_=0;
 // Exact native 0x18-byte client player reset operands; unnamed native fields
 // remain unnamed here. They are not CvPlayer or message-delivery receipts.
 struct PlayerClient {std::int32_t id=0,field_04=-1,field_08=-1;bool field_0c=false,field_0d=false,field_0e=false,field_0f=false,field_10=false;std::int32_t field_14=-1;};
 std::array<PlayerClient,64> players_{};
 // Selected Civis launcher retains the source-defined frame client fields.
 // This does NOT claim complete original78C810 strings/debug/registry ABI.
 // Explicit supported selected Civis launcher profile; original78C817..81D
 // has typed nullable legacy component04/08 slots. No legacy installers run
 // in this profile; actual Renderer and its UI remain separately owned.
 enum class NativeClientProfile {selected_civis_launcher};
 const NativeClientProfile native_client_profile_=NativeClientProfile::selected_civis_launcher;
 struct LegacyClientComponent {enum class Kind {component04,component08};Kind kind;};
 std::shared_ptr<const LegacyClientComponent> legacy_component04_,legacy_component08_;
 bool legacy_components_constructed_=false;
 // Actual78C817 native client00 NULL constructor slot, then the selected
 // Civis launcher explicitly attaches its SAME retained Boot via Cache.
 // This namespace binding does not grant full imported-game authority:
 // only a real frame Loan with returned Boot/Read/Sync can borrow it.
 std::weak_ptr<const BnwImportedPregameReadCache> frame_native_proxy00_;
 bool frame_native_proxy00_published_=false;
 std::int32_t frame_loadstate158_=0;
 bool frame_loadstate158_constructed_=false;
 // Minimal actual selected-launcher EngineUtility owner. Original5458D0
 // publishes9EB478 and refcount1;72B6D5 stores the owned pointer inApp1460.
 // No complete original Utility/SetDLLIFace ABI or omitted ctor field is
 // licensed. Only proven3AC/3F4 methods are admitted by this private owner.
 struct SelectedEngineUtility {
  enum class Descriptor {uninitialized,engine_utility};
  Descriptor descriptor=Descriptor::uninitialized;std::uint32_t references=0;
  bool constructed=false,published=false;std::uint64_t stores=0;
  void store(){if(stores==UINT64_MAX)throw std::runtime_error("SP Utility native store exhausted");++stores;}
 };
 std::unique_ptr<SelectedEngineUtility> engine_utility_;
 void require_engine_utility_owned(const BnwStockLuaAttemptLease& lease)const{
  require_for(lease);
  need(native_client_profile_==NativeClientProfile::selected_civis_launcher&&engine_utility_&&
   engine_utility_->constructed&&engine_utility_->published&&
   engine_utility_->descriptor==SelectedEngineUtility::Descriptor::engine_utility&&engine_utility_->references==1,
   "SP actual selected EngineUtility owner absent/retired");require_for(lease);
 }
 // Actual78C84B..869 constructs two empty native vector<int> headers.
 // Civis uses owned std::vector storage; inactive raw pointer bytes are
 // excluded from observable state. Required future vector ingresses cannot
 // borrow this constructor as an ACK or overwrite it from DTOs.
 std::vector<std::int32_t> frame_pending118_,frame_pending128_;
 bool frame_pending_vectors_constructed_=false;
 std::uint32_t frame_counter_=0;
 std::array<std::uint8_t,64> frame_18c_{},frame_1cc_{};
 std::array<std::int32_t,64> frame_20c_{},frame_30c_{};
 bool frame_initializers_returned_=false,frame_update_entered_=false;
 std::uint64_t last_frame_sequence_=0;
 std::uint64_t native_writes_=0,next_submission_=1;
 enum class MessageKind {turn_complete,turn_all_complete,work_return};
 struct Message {MessageKind kind;std::int32_t actor;std::uint64_t sequence,source_serial;bool decrement_attempted=false;};
 std::deque<Message> submissions_;
 // Concrete native message under construction, not a successful submission.
 // Stores survive a mandatory constructor STOP. No restore/DTO can grant it.
 struct TurnCompleteConstruction {
  enum class VirtualKind {uninitialized,base,turn_complete};
  VirtualKind virtual_kind=VirtualKind::uninitialized;
  std::int32_t type=0,field_08=0,field_0c=0,field_14=0,field_18=0,field_1c=0,field_20=0,field_24=0,field_28=0,actor=0;
  bool field_04=false,field_05=false,field_2c=false,returned=false;
 };
 std::unique_ptr<TurnCompleteConstruction> constructing_complete_;
 std::recursive_mutex game_interface_lock_;std::uint32_t game_interface_depth_=0;
 std::thread::id game_interface_thread_{};
 static void need(bool v,const char*why){if(!v)throw std::runtime_error(why);}
 static void need_actor(std::int32_t id,bool allow_none=false){need((allow_none&&id==-1)||(id>=0&&id<64),"SP client actual native actor unavailable");}
 void require_source()const {
  const auto c=source_.checked();const auto retained=control_.lock();const auto source=c->source_owner.lock();
  const auto world=world_.lock();
  need(c&&retained&&c.get()==retained.get()&&!c.owner_before(retained)&&!retained.owner_before(c)&&
   c->creator==creator_&&c->vm==vm_&&vm_&&world&&c->world.get()==world.get()&&
   !c->world.owner_before(world)&&!world.owner_before(c->world)&&source&&source.get()==source_get_&&
   !c->source_owner.owner_before(source_owner_)&&!source_owner_.owner_before(c->source_owner),
   "SP client SAME canonical source/VM/world changed");
 }
 void require_for(const BnwStockLuaAttemptLease& lease)const {
  require_current();lease.require_same_source_as_stored(source_);require_current();
 }
 void require_frame_current_without_callbacks(const BnwStockLuaAttemptLease::ScopeIdentity& scope)const{
  if(std::this_thread::get_id()!=creator_){poisoned_.store(true);source_.invalidate();throw std::runtime_error("SP frame client foreign creator");}
  try{
   need(!poisoned_.load()&&reset_returned_&&legacy_components_constructed_&&frame_initializers_returned_,"SP frame actual client initializer retired");
   source_.require_scope_identity_without_callbacks(scope);
   const auto control=control_.lock();const auto world=world_.lock();
   need(control&&scope.control.get()==control.get()&&!scope.control.owner_before(control)&&!control.owner_before(scope.control)&&
    scope.vm==vm_&&world&&world.get()==scope.world&&!world.owner_before(control->world)&&!control->world.owner_before(world)&&
    scope.source_owner.get()==source_get_&&!scope.source_owner.owner_before(source_owner_)&&!source_owner_.owner_before(scope.source_owner),
    "SP frame SAME client/source/VM/world control substituted");
  }catch(...){poisoned_.store(true);source_.invalidate();throw;}
 }
 void require_frame_game_origin_owned(const BnwStockLuaAttemptLease::ScopeIdentity& scope)const{
  require_frame_current_without_callbacks(scope);
  need(imported_pregame_!=nullptr,"SP frame actual imported client boot absent");
  imported_pregame_->require_actual(scope);
  require_frame_current_without_callbacks(scope);
 }
 // Private reader for SDK Game.IsOption, not a temporary SP interface or
 // retained lexical registration frame. Pin the actual active Source serial.
 bool read_game_is_option_owned(std::int32_t selector)const{
  try{const auto scope=source_.pin_active_scope_without_callbacks();require_frame_game_origin_owned(scope);
   const bool value=imported_pregame_->game_is_option(scope,selector);
   require_frame_game_origin_owned(scope);return value;
  }catch(...){poisoned_.store(true);source_.invalidate();throw;}
 }
 bool read_game_is_option_owned(const char* requested)const{
  try{const auto scope=source_.pin_active_scope_without_callbacks();require_frame_game_origin_owned(scope);
   need(actual_selected_db_!=nullptr,"Game.IsOption genuine selected rules owner absent");
   const bool value=imported_pregame_->game_is_option(scope,requested,*actual_selected_db_);
   require_frame_game_origin_owned(scope);return value;
  }catch(...){poisoned_.store(true);source_.invalidate();throw;}
 }
 void before_write(){need(native_writes_<UINT64_MAX,"SP client native store occurrence exhausted");++native_writes_;}
 explicit BnwLocalClientScalarOwner(BnwStockLuaAttemptLease source,const BnwSelectedStockLuaBootstrap* actual_vm,
  std::shared_ptr<const BnwImportedPregameReadCache> imported_pregame,const content::RulesDatabase& actual_selected_db)
  :source_(std::move(source)),vm_(actual_vm),control_(source_.control_),
   source_owner_(source_.checked()->source_owner),source_get_(source_.checked()->source_owner.lock().get()),world_(source_.checked()->world),imported_pregame_(std::move(imported_pregame)),actual_selected_db_(&actual_selected_db){
  // Actual private Source birth checks ordinary SP mode before this ctor. The
  // new queue is genuinely empty; it does not reconstruct original receipts.
  require_source();need(submissions_.empty()&&next_submission_==1,"SP client first reset queue not empty");
  // Fresh owned process storage starts with recording/gate clear, just as
  // zero-initialized original client storage. Execute actual reset6927F0,
  // including its complete 64*0x18 native player reset677B20, in source order.
  before_write();sent_complete_=false;
  before_write();sent_all_complete_=false;
  before_write();sent_unready_=false;
  before_write();slot_240_=-1;
  for(std::int32_t id=0;id<64;++id){auto& p=players_[static_cast<std::size_t>(id)];
   before_write();p.id=id;before_write();p.field_04=-1;before_write();p.field_08=-1;
   before_write();p.field_0c=false;before_write();p.field_0d=false;before_write();p.field_0e=false;
   before_write();p.field_14=-1;before_write();p.field_0f=false;before_write();p.field_10=false;
  }
  before_write();slot_24c_=0;
  before_write();processing_work_=0;
  // Original final reset clears recording/gate conditionally, not equally.
  if(recording_){before_write();recording_gate_=0;before_write();recording_=false;}
  require_source();need(native_writes_==582&&!sent_complete_&&!sent_all_complete_&&processing_work_==0&&!recording_&&recording_gate_==0,
   "SP client genuine initial reset did not return");reset_returned_=true;
  // Explicit selected Civis publication order: old genuine582 reset is
  // untouched, then owned frame fields initialize BEFORE client publication.
  // Exact original78C881 counter0; exact5C219D..21DE memset0/0/FF/FF.
  // Reuse already-earned imported Boot service, never replay7FA520 here.
  // Original78C819 +4NULL then78C81C +8NULL precede78C881 counter0.
  // Explicit real owned stores, not absence inferred from a missing API.
  before_write();frame_native_proxy00_.reset(); // original78C817
  before_write();legacy_component04_.reset();
  before_write();legacy_component08_.reset();legacy_components_constructed_=true;
  // Original vector118 begin/end/capacity NULL, then128 begin/end/capacity
  // NULL precede the counter store. Own values stay genuinely constructor-empty.
  before_write();before_write();before_write();frame_pending118_.clear();
  before_write();before_write();before_write();frame_pending128_.clear();
  frame_pending_vectors_constructed_=true;
  before_write();frame_counter_=0;
  before_write();frame_loadstate158_=0; // actual78C8A7 after144
  frame_loadstate158_constructed_=true;
  before_write();frame_18c_.fill(0);
  before_write();frame_1cc_.fill(0);
  before_write();frame_20c_.fill(-1);
  before_write();frame_30c_.fill(-1);
  frame_initializers_returned_=true;require_source();
  // Explicit Civis launcher publishes its minimal owned Utility before
  // this canonical client escapes. Reuses SAME source/VM/Game namespace;
  // neither a DTO nor a missing external DLL interface grants these fields.
  engine_utility_=std::make_unique<SelectedEngineUtility>();
  engine_utility_->store();engine_utility_->descriptor=SelectedEngineUtility::Descriptor::engine_utility;
  engine_utility_->store();engine_utility_->references=1;
  engine_utility_->constructed=true;require_source();
  engine_utility_->store();engine_utility_->published=true;require_source();
  // Explicit selected launcher publication, not full original78C810 ABI.
  // SAME retained imported namespace is attached before canonical client
  // publication. A data-only diagnostic may own this namespace but cannot
  // lend its absent game startup, nor replay/construct a second Boot here.
  if(imported_pregame_){
   before_write();frame_native_proxy00_=imported_pregame_;
   frame_native_proxy00_published_=true;
  }
  // A legitimate data-only/early diagnostic may have no imported namespace:
  // retain its actual constructor-NULL00 and unpublished Boot attachment.
  // It gains NO full game/frame authority. Every genuine Frame/gateway still
  // requires returned SAME imported Boot/Read/Sync and strict00 dereference;
  // neither missing Cache nor NULL becomes an optional frame-success branch.
  require_source();
 }
 void enqueue(MessageKind kind,std::int32_t actor,const BnwStockLuaAttemptLease& lease){
  require_for(lease);need(next_submission_<UINT64_MAX,"SP client submission sequence exhausted");
  const auto serial=lease.operation_serial();const auto sequence=next_submission_;
  // Concrete SAME owner queue insertion is the required local submission.
  // A saved DTO or receipt cannot provide it. No receiver ACK is claimed.
  submissions_.push_back({kind,actor,sequence,serial});++next_submission_;
  require_for(lease);lease.require_operation(serial);
 }
 // Same standalone client Game/Player interface gate. Original CvDllGame
 // and CvDllPlayer constructors acquire the same recursive GameCore lock;
 // final reference destruction releases it, including the nested GetPlayer
 // factory's temporary Game. Source pin is actual, never a diagnostic token.
 class InterfaceLoan final {
  BnwLocalClientScalarOwner& client_;const BnwStockLuaAttemptLease& lease_;
  const BnwStockLuaAttemptLease::ScopeIdentity scope_;
  std::unique_lock<std::recursive_mutex> gate_;
  std::shared_ptr<const LiveSessionState> world_;
  const saves::SavedPlayerObservation* player_=nullptr;
  bool acquired_=false,returned_=false;std::uint32_t player_references_=1;
  void failed_release()noexcept{
   client_.poisoned_.store(true);
   if(!acquired_)return;
   if(player_&&player_references_!=1)std::terminate();
   if(std::this_thread::get_id()!=client_.creator_||client_.game_interface_depth_==0||client_.native_writes_==UINT64_MAX)std::terminate();
   ++client_.native_writes_;--client_.game_interface_depth_;acquired_=false;
   if(client_.game_interface_depth_==0){if(client_.native_writes_==UINT64_MAX)std::terminate();++client_.native_writes_;client_.game_interface_thread_={};}
   try{if(gate_.owns_lock())gate_.unlock();}catch(...){std::terminate();}
  }
 public:
  InterfaceLoan(BnwLocalClientScalarOwner& c,const BnwStockLuaAttemptLease& lease,std::optional<std::int32_t> player={})
   :client_(c),lease_(lease),scope_(lease.pin_active_scope_without_callbacks()),gate_(c.game_interface_lock_){
   client_.require_for(lease_);lease_.require_scope_identity_without_callbacks(scope_);
   world_=client_.world_.lock();need(world_&&world_.get()==scope_.world,"SP interface SAME actual Game allocation unavailable");
   need(client_.game_interface_depth_<UINT32_MAX&&(client_.game_interface_depth_==0||client_.game_interface_thread_==client_.creator_),
    "SP actual Game interface gate count/thread unavailable");
   try{
   client_.before_write();++client_.game_interface_depth_;acquired_=true;
   client_.before_write();client_.game_interface_thread_=client_.creator_;
   if(player){
    need_actor(*player);const auto& world=world_->world;constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    need(world_->setup.profile==content::TechnologyProfile::bnw&&world.all_players&&world.all_players->layout==bnw&&
     world.all_players->players.size()==64&&world.all_players->context.player_count==64&&world.all_players->context.strategy.civ_players==63,"SP GetPlayer actual native64 manager unavailable");
    const auto slot=static_cast<std::size_t>(*player);const auto& raw=world.all_players->players[slot];
    need(raw.index==slot&&raw.saved_player_id==*player&&raw.core.layout==bnw,"SP GetPlayer actual identity differs");
    const saves::PlayerCoreField* identity=nullptr;
    for(const auto& field:raw.core.fields)if(field.name&&std::string_view(field.name)=="m_eID"){need(!identity,"SP GetPlayer duplicate actual ID");identity=&field;}
    need(identity&&identity->encoding==saves::PlayerCoreEncoding::i32&&identity->values.size()==1&&identity->values[0]==*player&&
     !identity->wire_count&&identity->bytes.empty()&&identity->row_counts.empty()&&identity->hashes.empty(),"SP GetPlayer actual raw CvPlayer identity unavailable");
    player_=&raw;
   }
   current();
   }catch(...){failed_release();throw;}
  }
  InterfaceLoan(const InterfaceLoan&)=delete;InterfaceLoan& operator=(const InterfaceLoan&)=delete;
  ~InterfaceLoan(){
   if(!returned_)failed_release();
  }
  void current()const{
   client_.require_for(lease_);lease_.require_scope_identity_without_callbacks(scope_);
   need(world_&&world_.get()==scope_.world&&client_.game_interface_depth_>0&&client_.game_interface_thread_==client_.creator_,
    "SP actual interface nested gate/Source/world retired");
  }
  bool is_game_multiplayer()const{
   current();const auto& w=world_->world;constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
   need(!player_&&w.pregame&&w.pregame->layout==bnw,"SP Game native pregame absent");
   const auto at=w.pregame->archive.integers.find("game_type");
   need(at!=w.pregame->archive.integers.end()&&at->second>=0&&at->second<=3&&at->second==world_->setup.game_type,
    "SP Game native mode/setup differs");
   // Native100CD5B0 short-circuits network, pbem, hotseat in this order.
   const bool result=at->second==1||at->second==3||at->second==2;current();return result;
  }
  std::int32_t active_player()const{
   current();need(!player_,"SP actual Game active-player read on Player interface");
   const auto value=detail::bnw_game_active_player_read(*world_);current();return value;
  }
  std::int32_t game_turn()const{
   current();need(!player_,"SP frame GameTurn queried on Player");
   need(client_.imported_pregame_!=nullptr,"SP frame GameTurn genuine namespace owner absent");
   client_.imported_pregame_->require_actual(scope_);current();
   // SDK CvGame::getGameTurn is CvPreGame::gameTurn. Read its SAME mutable
   // native namespace, not the historical imported copy or GameState scalar.
   need(world_->world.pregame&&world_->world.pregame->layout==saves::GameStateLayout::expansion2_version1,
    "SP frame GameTurn actual namespace unavailable");
   const auto at=world_->world.pregame->archive.integers.find("game_turn");
   need(at!=world_->world.pregame->archive.integers.end()&&at->second>=INT32_MIN&&at->second<=INT32_MAX&&
    at->second==world_->setup.game_turn,"SP frame raw/owned GameTurn diverged");
   const auto result=static_cast<std::int32_t>(at->second);current();return result;
  }
  bool is_human()const{
   current();need(player_&&player_references_==2&&client_.imported_pregame_&&client_.actual_selected_db_,
    "SP frame IsHuman requires SAME queried Player2 and native Pregame");
   client_.imported_pregame_->require_actual(scope_);current();
   const auto result=detail::bnw_player_human_read(*world_,player_->saved_player_id,*client_.actual_selected_db_);
   current();return result;
  }
  // Exact QueryInterface ICvPlayer2 returns SAME actual Player and increments
  // its existing interface reference. It does not acquire another Game lock.
  void acquire_player2_reference(){
   current();need(player_&&player_references_==1,"SP ICvPlayer2 source reference unavailable");
   client_.before_write();++player_references_; // No throwing POST before child owns this reference.
  }
  void failed_player2_release()noexcept{
   client_.poisoned_.store(true);
   if(!player_||player_references_!=2||client_.native_writes_==UINT64_MAX)std::terminate();
   ++client_.native_writes_;--player_references_;
  }
  void release_player2_store(){
   current();need(player_&&player_references_==2,"SP Player2 reference release absent/duplicate");
   client_.before_write();--player_references_; // Child retires ownership BEFORE Source POST.
  }
  bool has_turn_timer_expired(){
   current();need(player_&&player_references_==2,"SP HasTurnTimerExpired requires actual queried Player2");
   // CvPlayer::GetID was validated from actual raw m_eID by acquisition.
   // Game::hasTurnTimerExpired ALWAYS obtains current active player first;
   // no additional CvDllGame interface/lock is acquired by this direct call.
   (void)detail::bnw_game_active_player_read(*world_);current();
   // Read the actual returned ReadArchive->Sync cache, not custom-string DTO.
   need(client_.imported_pregame_!=nullptr,"SP HasTurnTimerExpired imported Pregame Read->Sync owner absent");
   const auto enabled=client_.imported_pregame_->enum_value(scope_,14);current();
   if(enabled!=1)return false; // Native Game::isOption is exact ==1, not nonzero.
   // Native enabled timer next tests actual pause player then GameState, before
   // elapsed-turn/AI/timers/UI effects. Those live producers are not yet joined.
   throw std::runtime_error("SP client mandatory STOP: enabled turn timer native pause/GameState/timer owner absent");
  }
  bool is_simultaneous_turns(){
   current();need(player_&&player_references_==2,"SP IsSimultaneousTurns requires actual queried Player2");
   need(client_.imported_pregame_&&client_.actual_selected_db_,"SP Player2 actual Pregame/custom rules owner absent");
   const auto dynamic=client_.imported_pregame_->string_option(scope_,"GAMEOPTION_DYNAMIC_TURNS",*client_.actual_selected_db_);
   current();
   if(dynamic==1){
    // Native1013FFD0 short-circuits directly to CvPlayer+8A8. This is the
    // imported actual field on SAME acquired Player, not a setup default.
    const saves::PlayerCoreField* value=nullptr;
    for(const auto& field:player_->core.fields)if(field.name&&std::string_view(field.name)=="m_bDynamicTurnsSimultMode"){
     need(!value,"SP Player2 duplicate actual dynamic turn mode");value=&field;
    }
    need(value&&value->encoding==saves::PlayerCoreEncoding::boolean&&value->values.size()==1&&
     (value->values[0]==0||value->values[0]==1)&&!value->wire_count&&value->bytes.empty()&&
     value->row_counts.empty()&&value->hashes.empty(),"SP Player2 mandatory actual dynamic mode field unavailable");
    const bool result=value->values[0]!=0;current();return result;
   }
   const auto simultaneous=client_.imported_pregame_->string_option(scope_,"GAMEOPTION_SIMULTANEOUS_TURNS",*client_.actual_selected_db_);
   current();return simultaneous==1; // Exact Game::isOption ==1.
  }
  void return_owned(){
   need(!player_||player_references_==1,"SP Player original reference still borrowed by Player2");
   current();need(!returned_&&acquired_,"SP interface duplicate actual release");
   client_.before_write();--client_.game_interface_depth_;acquired_=false;
   if(client_.game_interface_depth_==0){client_.before_write();client_.game_interface_thread_={};}
   gate_.unlock(); // Release original recursive gate before Source POST.
   lease_.require_scope_identity_without_callbacks(scope_);client_.require_for(lease_);returned_=true;
  }
 };
 void complete_player_sp_suffix_owned(const BnwStockLuaAttemptLease& lease,std::int32_t actor){
  // Original60F630: first Game acquisition/read, then unconditional GetPlayer
  // through its own temporary Game; only multiplayer calls IsHuman/UI.
  InterfaceLoan game(*this,lease);const bool multiplayer=game.is_game_multiplayer();
  InterfaceLoan temporary_game(*this,lease);InterfaceLoan player(*this,lease,actor);
  temporary_game.return_owned();game.current();player.current();
  if(multiplayer)throw std::runtime_error("SP client mandatory STOP: native60F630 multiplayer IsHuman/UI suffix outside ordinary SP owner");
  before_write();players_[static_cast<std::size_t>(actor)].field_0f=false; // equal store still earned
  player.return_owned();game.return_owned();require_for(lease);
 }
 class Player2Reference final {
  InterfaceLoan& actual_player_;bool held_=false;
 public:
  explicit Player2Reference(InterfaceLoan& player):actual_player_(player){
   // Mark only after the genuine ref store. The acquisition has no provider
   // callbacks after its store: Source POST is checked by the caller below.
   actual_player_.acquire_player2_reference();held_=true;
  }
  Player2Reference(const Player2Reference&)=delete;
  ~Player2Reference(){if(held_)actual_player_.failed_player2_release();}
  bool has_turn_timer_expired(){return actual_player_.has_turn_timer_expired();}
  bool is_human(){return actual_player_.is_human();}
  bool is_simultaneous_turns(){return actual_player_.is_simultaneous_turns();}
  void return_owned(){
   if(!held_)throw std::runtime_error("SP Player2 reference already returned");
   actual_player_.release_player2_store();held_=false;actual_player_.current();
  }
 };
 // Private SAME-client Game interface supplier for EngineUtility623B00.
 // Its wrapper is physically released before the scalar reaches the caller.
 std::int32_t read_frame_game_turn_owned(const BnwStockLuaAttemptLease& lease){
  try{
   const auto scope=lease.pin_active_scope_without_callbacks();require_frame_game_origin_owned(scope);
   InterfaceLoan game(*this,lease);const auto value=game.game_turn();game.return_owned();
   require_frame_game_origin_owned(scope);return value;
  }catch(...){poisoned_.store(true);source_.invalidate();throw;}
 }
 std::int32_t read_frame_active_player_owned(const BnwStockLuaAttemptLease& lease){
  try{
   const auto scope=lease.pin_active_scope_without_callbacks();require_frame_game_origin_owned(scope);
   InterfaceLoan game(*this,lease);const auto value=game.active_player();game.return_owned();
   require_frame_game_origin_owned(scope);return value;
  }catch(...){poisoned_.store(true);source_.invalidate();throw;}
 }
 // Source53C990 and65B430: the temporary Game is destroyed before Query.
 // The bounded callback retains actual SAME Player1+Player2 through the full
 // original IsHuman/option14/client branch. Returning false never skips refs.
 template<class Target> void with_frame_player2_owned(const BnwStockLuaAttemptLease& lease,
  std::int32_t actor,Target&& target){
  try{
   const auto scope=lease.pin_active_scope_without_callbacks();require_frame_game_origin_owned(scope);need_actor(actor);
   InterfaceLoan game(*this,lease);InterfaceLoan player(*this,lease,actor);game.return_owned();
   Player2Reference queried(player);player.current();
   std::forward<Target>(target)(queried);player.current();
   queried.return_owned();player.return_owned();require_frame_game_origin_owned(scope);
  }catch(...){poisoned_.store(true);source_.invalidate();throw;}
 }
 // Active53C990 retains original Player1 until AFTER the entire host/all64
 // suffix; its queried Player2 can be returned before that suffix. This
 // separate private factory preserves that exact lifetime, not a fresh DTO.
 template<class Target> void with_frame_player_owned(const BnwStockLuaAttemptLease& lease,
  std::int32_t actor,Target&& target){
  try{
   const auto scope=lease.pin_active_scope_without_callbacks();require_frame_game_origin_owned(scope);need_actor(actor);
   InterfaceLoan game(*this,lease);InterfaceLoan player(*this,lease,actor);game.return_owned();
   std::forward<Target>(target)(player);player.current();player.return_owned();require_frame_game_origin_owned(scope);
  }catch(...){poisoned_.store(true);source_.invalidate();throw;}
 }
 bool read_frame_observer_participant_owned(const BnwStockLuaAttemptLease& lease,std::int32_t actor){
  try{const auto scope=lease.pin_active_scope_without_callbacks();require_frame_game_origin_owned(scope);need_actor(actor);
   const auto value=imported_pregame_->frame_observer_participant(scope,actor);
   require_frame_game_origin_owned(scope);return value;
  }catch(...){poisoned_.store(true);source_.invalidate();throw;}
 }
 bool read_frame_is_host_owned(const BnwStockLuaAttemptLease& lease){
  try{const auto scope=lease.pin_active_scope_without_callbacks();require_frame_game_origin_owned(scope);
   const auto value=imported_pregame_->frame_is_host(scope);require_frame_game_origin_owned(scope);return value;
  }catch(...){poisoned_.store(true);source_.invalidate();throw;}
 }
 void construct_turn_complete_owned(const BnwStockLuaAttemptLease& lease,std::int32_t actor){
  require_for(lease);need_actor(actor);need(!constructing_complete_,"SP native turn-complete constructor already attempted");
  constructing_complete_=std::make_unique<TurnCompleteConstruction>();auto& m=*constructing_complete_;
  // Original7E4C60 message base source order. Equal stores are mandatory.
  before_write();m.type=0x22;before_write();m.virtual_kind=TurnCompleteConstruction::VirtualKind::base;
  before_write();m.field_04=false;before_write();m.field_05=false;
  before_write();m.field_08=0;before_write();m.field_0c=0;
  before_write();m.field_18=-1;before_write();m.field_14=-1;before_write();m.field_1c=-1;
  before_write();m.field_20=0;before_write();m.field_24=0;before_write();m.field_28=1;before_write();m.field_2c=false;
  // Derived7642C0 begins only after the entire native base constructor.
  before_write();m.field_04=true;before_write();m.virtual_kind=TurnCompleteConstruction::VirtualKind::turn_complete;
  before_write();m.actor=actor;
  InterfaceLoan game(*this,lease);const auto active=game.active_player();
  InterfaceLoan temporary_game(*this,lease);InterfaceLoan player(*this,lease,active);
  temporary_game.return_owned();Player2Reference player2(player);player.current();game.current();
  // Pinned DLL supports ICvPlayer2. The fallback Game+94 branch is not chosen
  // by fabricating a NULL query. Reference retained until real required return.
  if(actor==active){
   // Original764377: timer first; +100 only if timer returned false. +194(true)
   // is required if timer expired OR the real Player is not simultaneous.
   const bool expired=player2.has_turn_timer_expired();
   if(expired||!player2.is_simultaneous_turns()){
    // Actual full-import cache retains the SAME returned registration/inner.
    // The data-only importer cannot lend this call, even with cached options.
    need(imported_pregame_!=nullptr,"SP NetProxy194 actual imported startup absent");
    const auto actual_scope=lease.pin_active_scope_without_callbacks();
    lease.require_scope_identity_without_callbacks(actual_scope);player.current();game.current();
    imported_pregame_->set_actual_client_simultaneous(actual_scope,true);
    lease.require_scope_identity_without_callbacks(actual_scope);player.current();game.current();
   }
  }
  // Original different-actor or genuine simultaneous skip branch. Release
  // SAME queried reference, primary Player and Game in source order. The
  // ordinary sequential active-actor branch releases only after real +194 returned.
  player2.return_owned();player.return_owned();game.return_owned();require_for(lease);m.returned=true;
 }
 void send_turn_complete_owned(const BnwStockLuaAttemptLease& lease,std::int32_t supplied_actor){
  try{require_for(lease);need_actor(supplied_actor,true);if(supplied_actor==-1)return;
   // Native552D60 retains its outer Game until the optional submission returns.
   InterfaceLoan outer_game(*this,lease);const auto outer_active=outer_game.active_player();
   if(supplied_actor==outer_active&&sent_complete_){outer_game.return_owned();require_for(lease);return;}
   { // Exact4E62A0: an independent Game read precedes conditional global store.
    InterfaceLoan setter_game(*this,lease);const auto setter_active=setter_game.active_player();
    if(supplied_actor==setter_active){before_write();sent_complete_=true;}
    complete_player_sp_suffix_owned(lease,supplied_actor);
    setter_game.return_owned();require_for(lease);
   }
   construct_turn_complete_owned(lease,supplied_actor);
   // Only an original callback-skipping branch can currently reach here.
   // A genuine constructor carrier/typed submission must own these exact
   // returned effects; existing loose actor/kind ingress is insufficient.
   need(constructing_complete_&&constructing_complete_->returned,"SP turn-complete constructor has not returned");
   throw std::runtime_error("SP client mandatory STOP after constructor return: typed native message carrier submission not joined");
  }catch(...){poisoned_.store(true);if(const auto c=control_.lock())c->invalidated.store(true);throw;}
 }
 void send_turn_all_complete_owned(const BnwStockLuaAttemptLease& lease,std::int32_t actual_active){
  try{require_for(lease);need_actor(actual_active,true);if(sent_all_complete_)return;
   if(actual_active!=-1)enqueue(MessageKind::turn_all_complete,actual_active,lease);
   before_write();sent_all_complete_=true; // Original5125B0 only AFTER submit.
   require_for(lease);
  }catch(...){poisoned_.store(true);throw;}
 }
 void record_gamecore_messages_owned(const BnwStockLuaAttemptLease& lease,bool original_record,bool flush_immediately){
  try{require_for(lease);
   if(original_record){
    if(recording_)return;
    // Owned sequential SP gate replaces the original same-client interlock;
    // this is a real gate field/counter store, never general Lua activity.
    need(recording_gate_==0,"SP client actual record gate already acquired");
    before_write();recording_gate_=1;before_write();recording_=true;
    need(processing_work_<INT32_MAX,"SP client source work counter exhausted");
    before_write();++processing_work_;require_for(lease);return;
   }
   if(!recording_)return;
   before_write();recording_gate_=0;before_write();recording_=false;
   // Native74CD16(false,false) tail-calls71E610; it does NOT enqueue512150
   // and does NOT decrement527780 inline. The flush=true branch instead
   // invokes the genuine current-thread dispatcher. Both remain required.
   require_for(lease);
   throw std::runtime_error(flush_immediately?
    "SP client mandatory STOP after end-record stores: actual thread flush dispatcher not joined":
    "SP client mandatory STOP after end-record stores: native71E610 completion drain/release not joined");
  }catch(...){poisoned_.store(true);throw;}
 }
 void schedule_work_return_owned(const BnwStockLuaAttemptLease& lease){
  try{require_for(lease);need(processing_work_>scheduled_returns_,"SP client duplicate scheduled return has no unscheduled actual recorded work");
   // Separate native512150 -> SAME owned queue; original caller must issue
   // this method explicitly. End-record and ordinary Lua operations cannot.
   enqueue(MessageKind::work_return,-1,lease);
   ++scheduled_returns_;require_for(lease);
  }catch(...){poisoned_.store(true);throw;}
 }
 void consume_next_submission_owned(const BnwStockLuaAttemptLease& lease){
  try{require_for(lease);need(!submissions_.empty(),"SP client actual queue is empty");
   auto& message=submissions_.front();
   if(message.kind!=MessageKind::work_return)
    throw std::runtime_error("SP client mandatory STOP: typed turn message receiver/source effects not joined");
   need(!message.decrement_attempted&&processing_work_>0&&scheduled_returns_>0,"SP client duplicate/stale actual work return");
   message.decrement_attempted=true;before_write();--processing_work_;
   require_for(lease);
   if(processing_work_==0)
    throw std::runtime_error("SP client mandatory STOP after decrement: native completion drain/release owner not joined");
   submissions_.pop_front();--scheduled_returns_;require_for(lease);
  }catch(...){poisoned_.store(true);throw;}
 }
 void require_same_world(const std::shared_ptr<const LiveSessionState>& actual)const {
  require_current();const auto world=world_.lock();
  need(world&&actual&&world.get()==actual.get()&&!world.owner_before(actual)&&!actual.owner_before(world),
   "SP client getter is not bound to SAME actual Game world");require_current();
 }
};
}
