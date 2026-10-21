#pragma once
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/runtime/bnw_sp_client_pregame_startup.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <array>
#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <thread>
#include <string_view>
namespace civ5::content {class RulesDatabase;}
namespace civ5::runtime {
class LiveSession;class BnwSelectedStockLuaSourceOwner;class BnwLocalClientScalarOwner;
class BnwImportedPregameLocaleOwner;class BnwSpClientUpdateLoan;
// Private actual imported ReadArchive -> option hash rebuild -> Sync owner.
// A SavedSession/LiveSessionState DTO cannot construct, bind, copy or restore it.
// Only the real unpublished LiveSession::restore_bnw pipeline calls its ctor.
class BnwImportedPregameReadCache final {
 friend class LiveSession;friend class BnwSelectedStockLuaSourceOwner;friend class BnwLocalClientScalarOwner;
 const std::thread::id thread_=std::this_thread::get_id();
 mutable std::atomic<bool> poisoned_{false};
 bool sync_returned_=false,bound_=false;
 std::uint64_t stores_=0,generation_=0;
 SessionCheckpointIdentity identity_;
 std::string prefix_sha256_;
 saves::PregameObservation read_archive_;
 std::unique_ptr<BnwSpClientPregameStartup> actual_sp_startup_before_read_;
 std::map<std::uint32_t,std::size_t> option_hash_;
 std::array<std::int32_t,22> native_enum_{};
 bool native_quick_combat_=false;
 std::function<bool()> rules_current_;
 std::weak_ptr<const saves::SavedSession> source_;
 std::weak_ptr<const LiveSessionState> world_;
 explicit BnwImportedPregameReadCache(const saves::SavedSession&,saves::PregameObservation&,
  const content::RulesDatabase&,const SessionCheckpointIdentity&,bool require_local_sp_startup,
  std::shared_ptr<const BnwImportedPregameLocaleOwner>);
 void bind_actual_restored(const std::shared_ptr<const saves::SavedSession>&,
  const std::shared_ptr<LiveSessionState>&,std::uint64_t);
 void require_actual(const BnwStockLuaAttemptLease::ScopeIdentity&)const;
 void set_actual_client_simultaneous(const BnwStockLuaAttemptLease::ScopeIdentity&,bool)const;
 void update_actual_client_frame(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&,float)const;
 void begin_actual_control_callback(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&,std::int32_t)const;
 void publish_actual_control_client_event(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&,std::uint32_t)const;
 std::int32_t actual_control_client_local_id(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&)const;
 void complete_actual_control_arrays(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&)const;
 void require_actual_control_proxy_ui(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&)const;
 std::int32_t begin_actual_client155_callback(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&)const;
 void complete_actual_client155_ready_store(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&)const;
 void publish_actual_client155_event(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&,std::uint32_t)const;
 std::int32_t actual_client155_local_id(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&)const;
 void require_actual_client155_core_suffix(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&)const;
 bool actual_engine_network_game_owned(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&)const;
 std::int32_t actual_engine_scheduler_last_turn_owned(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&)const;
 std::int32_t actual_frame_loadstate_last_turn_owned(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&)const;
 bool actual_frame_proxy_pending_event150_owned(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&)const;
 bool actual_frame_proxy_inner45_owned(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&)const;
 std::int32_t actual_frame_proxy_map_count_owned(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&)const;
 void complete_actual_client155_local_return(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&)const;
 void submit_actual_client155(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&)const;
 void complete_actual_control_local_ui_return(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&)const;
 bool actual_control_client_c8(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&)const;
 bool actual_control_client_is_host(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&)const;
 std::int32_t actual_control_client_sender_local_id(const BnwStockLuaAttemptLease::ScopeIdentity&,const BnwSpClientUpdateLoan&)const;
 bool frame_observer_participant(const BnwStockLuaAttemptLease::ScopeIdentity&,std::int32_t)const;
 bool frame_is_host(const BnwStockLuaAttemptLease::ScopeIdentity&)const;
 // SDK CvGame::isOption reads this SAME restored CvPreGame namespace.
 // Private client lender supplies the actual current Source/DB.
 void require_game_option_namespace_owned(const BnwStockLuaAttemptLease::ScopeIdentity&)const;
 bool game_is_option(const BnwStockLuaAttemptLease::ScopeIdentity&,std::int32_t)const;
 bool game_is_option(const BnwStockLuaAttemptLease::ScopeIdentity&,const char*,const content::RulesDatabase&)const;
 std::int32_t enum_value(const BnwStockLuaAttemptLease::ScopeIdentity&,std::size_t)const;
 std::int32_t string_option(const BnwStockLuaAttemptLease::ScopeIdentity&,std::string_view,
  const content::RulesDatabase&)const;
 void store();
 BnwImportedPregameReadCache(const BnwImportedPregameReadCache&)=delete;
 BnwImportedPregameReadCache& operator=(const BnwImportedPregameReadCache&)=delete;
};
}
