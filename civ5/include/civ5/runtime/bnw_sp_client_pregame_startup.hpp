#pragma once
#include "civ5/runtime/session_checkpoint_identity.hpp"
#include "civ5/saves/session_snapshot.hpp"
#include <atomic>
#include <array>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <thread>
#include <vector>
namespace civ5::content {class RulesDatabase;}
namespace civ5::runtime {
class BnwImportedPregameReadCache;
class BnwImportedPregameLocaleOwner;class BnwSpClientUpdateLoan;
// SAME actual imported namespace storage, before native ReadArchive replaces
// constructor values. No DTO, public factory or late EndTurn can issue this.
class BnwSpClientPregameStartup final {
 friend class BnwImportedPregameReadCache;
 friend class BnwImportedPregameLocaleOwner;
 enum class Phase {namespace_constructed,registration_prefix,handicap_cache_required,failed};
 enum class Vector {status,claim,ready,handicap,last_human_handicap,active,nickname};
 struct Delta {Vector field;std::int32_t index,value;std::uint8_t operation=2;};
 const std::thread::id thread_=std::this_thread::get_id();
 mutable std::atomic<bool> poisoned_{false};
 const BnwImportedPregameReadCache* const actual_namespace_owner_;
 saves::PregameObservation& actual_namespace_;
 const std::shared_ptr<const std::vector<std::uint8_t>> prefix_;
 const std::string prefix_sha256_;
 std::function<bool()> rules_current_;
 Phase phase_=Phase::namespace_constructed;
 std::set<Vector> archive_dirty_;
 std::vector<Delta> deltas_;
 std::uint64_t field_stores_=0,archive_insertions_=0,delta_appends_=0;
 // Native session-info own constructor: actual maps empty, IDs -1. They are
 // retained separately from Pregame Read; map count is NOT registration ACK.
 struct BootOwner;
 bool frame_observer_participant(std::int32_t);
 bool frame_is_host();
 struct PreGameContext;
 std::shared_ptr<PreGameContext> pregame_context_;
 bool native_callback_returned_=false;
 struct CachedGlobals;
 using PackageId=std::array<std::uint32_t,4>;
 struct AllowedDlcEnumerator;
 std::shared_ptr<AllowedDlcEnumerator> allowed_dlc_enumerator_;
 std::vector<PackageId> allowed_dlc_;
 std::array<std::vector<PackageId>,64> available_dlc_;
 bool dlc_namespace_born_=false;
 std::uint64_t dlc_copy_calls_=0;
 std::shared_ptr<const BnwImportedPregameLocaleOwner> early_locale_;
 std::map<std::int32_t,std::string> display_nicknames_;
 struct NicknameDelta {std::int32_t index;std::string value;std::uint8_t operation=2;};
 std::vector<NicknameDelta> nickname_deltas_;
 std::uint64_t display_stores_=0;
 std::shared_ptr<BootOwner> local_parent_;
 std::shared_ptr<CachedGlobals> cached_globals_;
 // Actual0x20 participant allocation is shared by two native map aliases.
 struct Participant {std::int32_t id=0,kind=0,state=0;std::uint32_t counter=0;
  float latency=0,interval=0;void* userdata=nullptr;
  bool flag18=false,flag19=false,flag1a=false;};
 std::map<std::int32_t,std::shared_ptr<Participant>> participants_;
 std::map<std::int32_t,std::shared_ptr<Participant>> auxiliary_participants_;
 std::int32_t local_id_=-1,host_id_=-1,selected_registration_id_=-1;
 bool registration_returned_=false;
 BnwSpClientPregameStartup(BnwImportedPregameReadCache&,saves::PregameObservation&,
  const saves::SavedSession&,const content::RulesDatabase&,const SessionCheckpointIdentity&,
  std::shared_ptr<const BnwImportedPregameLocaleOwner>);
 void current()const;
 std::int32_t native_local_allocator_prefix();
 bool native_validate_local_slot(std::int32_t);
 void set_integer(Vector,std::int32_t,std::int32_t);
 void set_ready_false(std::int32_t);
 void set_handicap(std::int32_t,std::int32_t);
 void verify_handicap(std::int32_t);
 void set_active_player(std::int32_t);
 void copy_actual_allowed_dlc(std::int32_t);
 void set_actual_nickname(std::int32_t,const std::string&);
 void native_registration_callback_prefix();
 void complete_before_read_prefix();
 void set_actual_inner_simultaneous(bool);
 // Borrow-only current typed caller; never a persisted/native permission bit.
 const BnwSpClientUpdateLoan* active_update_loan_=nullptr;
 enum class ControlStage {none,event146,local_id,event145,arrays,c8,host,sender_local,sender,ui_suffix,client_returned};
 ControlStage control_stage_=ControlStage::none;
 std::int32_t control_actor_=-1;
 void require_actual_control_callback(const BnwSpClientUpdateLoan&)const;
 void begin_actual_control_callback(const BnwSpClientUpdateLoan&,std::int32_t);
 void publish_actual_control_client_event(const BnwSpClientUpdateLoan&,std::uint32_t);
 std::int32_t actual_control_client_local_id(const BnwSpClientUpdateLoan&);
 void complete_actual_control_arrays(const BnwSpClientUpdateLoan&);
 void require_actual_control_proxy_ui(const BnwSpClientUpdateLoan&)const;
 void submit_actual_client155(const BnwSpClientUpdateLoan&);
 void complete_actual_control_local_ui_return(const BnwSpClientUpdateLoan&);
 bool actual_control_client_c8(const BnwSpClientUpdateLoan&);
 bool actual_control_client_is_host(const BnwSpClientUpdateLoan&);
 std::int32_t actual_control_client_sender_local_id(const BnwSpClientUpdateLoan&);
 enum class Client155Stage {none,ready_store,event146,local_id,event132,core_suffix,client_returned};
 Client155Stage client155_stage_=Client155Stage::none;
 std::int32_t client155_actor_=-1;
 void require_actual_client155_callback(const BnwSpClientUpdateLoan&)const;
 std::int32_t begin_actual_client155_callback(const BnwSpClientUpdateLoan&);
 void complete_actual_client155_ready_store(const BnwSpClientUpdateLoan&);
 void publish_actual_client155_event(const BnwSpClientUpdateLoan&,std::uint32_t);
 std::int32_t actual_client155_local_id(const BnwSpClientUpdateLoan&);
 void require_actual_client155_core_suffix(const BnwSpClientUpdateLoan&)const;
 void complete_actual_client155_local_return(const BnwSpClientUpdateLoan&);
 std::int32_t actual_engine_scheduler_last_turn_owned(const BnwSpClientUpdateLoan&)const;
 std::int32_t actual_frame_loadstate_last_turn_owned(const BnwSpClientUpdateLoan&)const;
 bool actual_frame_proxy_pending_event150_owned(const BnwSpClientUpdateLoan&)const;
 bool actual_frame_proxy_inner45_owned(const BnwSpClientUpdateLoan&)const;
 std::int32_t actual_frame_proxy_map_count_owned(const BnwSpClientUpdateLoan&)const;
 void update_actual_client_frame(const BnwSpClientUpdateLoan&,float);
 BnwSpClientPregameStartup(const BnwSpClientPregameStartup&)=delete;
 BnwSpClientPregameStartup& operator=(const BnwSpClientPregameStartup&)=delete;
};
}
