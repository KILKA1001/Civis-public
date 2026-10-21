#pragma once
#include "civ5/runtime/bnw_unit_reset.hpp"
#include "civ5/runtime/bnw_diplomacy_turn_end.hpp"
#include "civ5/runtime/bnw_notification_cleanup.hpp"
#include "civ5/runtime/bnw_update_moves_local_sp.hpp"
#include "civ5/runtime/bnw_selected_gameplay_lua_owner.hpp"
namespace civ5::runtime {
inline constexpr std::array<BnwUnitOrderSourceFile,1> kBnwTurnDeactivationSources{{
 {"CvGameCoreDLL_Expansion2/CvGame.cpp","1318f7522c1bb551f3a2a7b0452c953bbbcad4291d1fe80f4791613f8f9fae3c"}
}};
enum class BnwTurnEndUiEffectKind : std::uint8_t {publish_end_turn_dirty,blocking_changed,update_end_turn,publish_active_player_turn_end,change_num_active_debug,player_turn_status_end};
struct BnwTurnEndUiEffect {BnwTurnEndUiEffectKind kind;std::int64_t player=-1,previous_type=-1,new_type=-1;std::int32_t active_count=-1;};
struct BnwTurnEndUiBatch {
 SessionCheckpointIdentity identity;std::array<std::uint8_t,16> map_guid{};
 std::string before_revision,after_revision;std::uint64_t expected_client_revision=0;
 std::vector<BnwTurnEndUiEffect> effects;
};
// Explicit new Civis client owner. Actual end-turn dirty state and blocking
// fields are retained and consumed; no original UI state is presumed saved.
class BnwTurnEndUiOwner {
 public:
  const auto &identity() const noexcept{return identity_;}const auto &map_guid() const noexcept{return map_guid_;}
  const auto &world_revision() const noexcept{return world_revision_;}std::uint64_t client_revision() const noexcept{return client_revision_;}
  std::int64_t player() const noexcept{return player_;}bool turn_active() const noexcept{return turn_active_;}
  bool end_turn_dirty() const noexcept{return end_turn_dirty_;}std::int64_t blocking_type() const noexcept{return blocking_type_;}
  std::int64_t blocking_notification() const noexcept{return blocking_notification_;}std::uint64_t end_turn_updates() const noexcept{return end_turn_updates_;}
  const auto &received_effects() const noexcept{return received_effects_;}
  bool active_player_turn_ended() const noexcept{return active_player_turn_ended_;}
  bool player_status_ended() const noexcept{return player_status_ended_;}
  const auto &active_count_debug_receipts() const noexcept{return active_count_debug_receipts_;}
 private:
  friend BnwTurnEndUiOwner start_bnw_turn_end_ui_client(const LiveSessionState &,const SessionCheckpointIdentity &,const content::RulesDatabase &,std::int64_t);
  friend void apply_bnw_turn_end_ui_batch(const LiveSessionState &,BnwTurnEndUiOwner &,const content::RulesDatabase &,const BnwTurnEndUiBatch &,const BnwLocalAiBarrierOwner *engine);
  BnwTurnEndUiOwner(SessionCheckpointIdentity identity,std::array<std::uint8_t,16> guid,std::string rev,std::int64_t player)
   :identity_(std::move(identity)),map_guid_(guid),world_revision_(std::move(rev)),player_(player){}
  SessionCheckpointIdentity identity_;std::array<std::uint8_t,16> map_guid_{};std::string world_revision_;
  std::uint64_t client_revision_=0,end_turn_updates_=0;std::int64_t player_=-1,blocking_type_=-1,blocking_notification_=-1;
  bool active_player_turn_ended_=false,player_status_ended_=false;std::vector<std::int32_t> active_count_debug_receipts_;
  bool turn_active_=false,end_turn_dirty_=false;std::vector<BnwTurnEndUiEffect> received_effects_;
};
std::string bnw_turn_end_world_revision(const LiveSessionState &,const SessionCheckpointIdentity &,const content::RulesDatabase &,std::int64_t);
BnwTurnEndUiOwner start_bnw_turn_end_ui_client(const LiveSessionState &,const SessionCheckpointIdentity &,const content::RulesDatabase &,std::int64_t);
void apply_bnw_turn_end_ui_batch(const LiveSessionState &,BnwTurnEndUiOwner &,const content::RulesDatabase &,const BnwTurnEndUiBatch &,const BnwLocalAiBarrierOwner *engine=nullptr);
enum class BnwDeactivationStep : std::uint8_t {end_request_and_processed,busy_query,received_turn_complete,turn_active_false,publish_end_turn_dirty,blocking_stores,blocking_changed,update_end_turn,unit_reset,notification_boundary,notification_cleanup,diplomacy_boundary,diplomacy_end_turn,publish_active_player_turn_end,all_complete_query,change_num_active_debug,player_turn_status_end,active_player_end_publication_boundary,next_activation_boundary};
struct BnwDeactivationPrefixObservation {
 bool private_turn_active=false;std::int64_t private_blocking_type=-1,private_blocking_notification=-1;
 std::vector<BnwUnitOrderKey> reset_units;std::vector<BnwTurnEndUiEffect> consumed_ui_effects;
 std::size_t notifications_pending=0;bool notification_transients_known=false;
 bool notification_cleanup_complete=false;std::vector<BnwRemoveNotificationReceipt> removed_notifications;
 std::vector<std::uint64_t> waited_notification_slots;std::int32_t native_active_players_after=-1;
 bool diplomacy_end_turn_complete=false,active_player_end_typed_receiver_consumed=false;
 bool mandatory_active_player_end_fanout_complete=false;std::int64_t diplomacy_next_ai=-2;
};
// Exact staged world/clients at the synchronous fanout frontier. No mutable
// export, completed-turn receipt or live publication. Lua must continue from
// this world rather than replay stores or reload clients from the source save.
class BnwDeactivationFanoutDraft {
 public:
  BnwDeactivationFanoutDraft(const BnwDeactivationFanoutDraft &)=delete;
  BnwDeactivationFanoutDraft &operator=(const BnwDeactivationFanoutDraft &)=delete;
  const LiveSessionState &state() const noexcept{return human_.request.state;}
  const BnwGameActivationOwner &activation() const noexcept{return human_.request.owner;}
  const BnwLocalAiBarrierOwner &engine() const noexcept{return human_.engine;}
  const BnwUnitPresentationOwner &units() const noexcept{return units_;}
  const BnwTurnEndUiOwner &turns() const noexcept{return turns_;}
  const BnwNotificationCleanupOwner &notifications() const noexcept{return notifications_;}
  const BnwNotificationUiOwner &notification_client() const noexcept{return notification_client_;}
  const BnwDiplomacyTurnOwner &diplomacy() const noexcept{return diplomacy_;}
 private:
  friend struct BnwDeactivationStagingAccess;
  BnwDeactivationFanoutDraft(BnwLocalHumanAutoMovesDraft human,BnwUnitPresentationOwner units,
   BnwTurnEndUiOwner turns,BnwNotificationCleanupOwner notifications,
   BnwNotificationUiOwner notification_client,BnwDiplomacyTurnOwner diplomacy)
   :human_(std::move(human)),units_(std::move(units)),turns_(std::move(turns)),
    notifications_(std::move(notifications)),notification_client_(std::move(notification_client)),
    diplomacy_(std::move(diplomacy)){}
  BnwLocalHumanAutoMovesDraft human_;BnwUnitPresentationOwner units_;BnwTurnEndUiOwner turns_;
  BnwNotificationCleanupOwner notifications_;BnwNotificationUiOwner notification_client_;
  BnwDiplomacyTurnOwner diplomacy_;
};
struct BnwDeactivationPrefixExecution {
 std::vector<BnwDeactivationStep> completed_steps;
 std::optional<BnwDeactivationPrefixObservation> prefix;
 std::unique_ptr<BnwDeactivationFanoutDraft> pending_fanout;
 std::string blocker;
};
// Source CheckPlayerTurnDeactivate -> setTurnActive(false) prefix using the
// SAME EndTurn/AutoMoves/busy/engine owners. All writes/actual UI receipts are
// on private copies. Notifications and subsequent mandatory suffix still gate
// completion: no completed private draft or live publication is returned.
// Only the supported suffix can retain an immutable pending_fanout.
BnwDeactivationPrefixExecution execute_bnw_local_human_deactivation_prefix_draft(
 const BnwLocalHumanAutoMovesDraft &,const BnwEndTurnInputBinding &,const BnwUnitPresentationOwner &,
 const BnwTurnEndUiOwner &,const content::RulesDatabase &,const content::InfoTypeCatalog &,std::int64_t);
// Retained notification cleanup variant. Stops at the next missing diplomacy
// EndTurn owner after actual supported Cleanup/Dismiss/Remove effects.
BnwDeactivationPrefixExecution execute_bnw_local_human_deactivation_notifications_prefix_draft(
 const BnwLocalHumanAutoMovesDraft &,const BnwEndTurnInputBinding &,const BnwUnitPresentationOwner &,
 const BnwTurnEndUiOwner &,const BnwNotificationCleanupOwner &,const BnwNotificationUiOwner &,
 const content::RulesDatabase &,const content::InfoTypeCatalog &,std::int64_t);
// Actual native prefix on private copies. Stops at mandatory synchronous
// ActivePlayerTurnEnd listener publication before subsequent query/status/next
// activation. A typed new-client receipt alone is not full source fanout.
BnwDeactivationPrefixExecution execute_bnw_local_human_deactivation_suffix_prefix_draft(
 const BnwLocalHumanAutoMovesDraft &,const BnwEndTurnInputBinding &,const BnwUnitPresentationOwner &,
 const BnwTurnEndUiOwner &,const BnwNotificationCleanupOwner &,const BnwNotificationUiOwner &,
 const BnwDiplomacyTurnOwner &,const content::RulesDatabase &,const content::InfoTypeCatalog &,std::int64_t);

// Owns the exact native prefix for provenance and the sole current callback
// world/VM. The prefix is not rerun or published. Client operand revisions
// must remain valid; other escrowed callback effects stay in the retained VM.
class BnwDeactivationUiFanoutDraft {
 public:
  const LiveSessionState &state() const noexcept{return *epoch_->world;}
  const BnwDeactivationFanoutDraft &native_prefix() const noexcept{return *prefix_;}
  std::uint64_t listeners() const noexcept{return listeners_;}
 private:
  friend struct BnwDeactivationUiFanoutAccess;
  BnwDeactivationUiFanoutDraft(std::unique_ptr<BnwDeactivationFanoutDraft> p,
      std::unique_ptr<BnwTeamTechLuaEpoch> e,std::uint64_t listeners)
      :prefix_(std::move(p)),epoch_(std::move(e)),listeners_(listeners){}
  // Declared first so the epoch's source_world stays alive through destruction.
  std::unique_ptr<BnwDeactivationFanoutDraft> prefix_;
  std::unique_ptr<BnwTeamTechLuaEpoch> epoch_;
  std::uint64_t listeners_=0;
};
struct BnwDeactivationUiFanoutExecution {
 std::unique_ptr<BnwDeactivationUiFanoutDraft> draft;
 // A validated native prefix is retained when dispatch has not started, so
 // loading an unavailable registry does not require replaying native stores.
 std::unique_ptr<BnwDeactivationFanoutDraft> pending;
 bool dispatch_attempted=false;
 std::string blocker;
};
// Standalone Civis synchronous routing of ActivePlayerTurnEnd. A verified
// selected context registry, exact current getters and escrow are mandatory.
// Transfers the sealed prefix once. A failed callback discards the private
// transaction; AllComplete/status/next activation and live commit remain later.
BnwDeactivationUiFanoutExecution dispatch_bnw_deactivation_active_player_turn_end(
 std::unique_ptr<BnwDeactivationFanoutDraft>,const SavedContentProfile &,
 BnwGameplayLuaEpochOwner &,const content::RulesDatabase &);

// Source continuation AFTER actual ActivePlayerTurnEnd dispatch. Retains that
// same VM/world; it does not create an AllComplete packet or replay callbacks.
// The typed status client is consumed privately. Mandatory external status
// delivery, outer CheckPlayerTurnDeactivate and world publication remain later.
class BnwDeactivationStatusDraft {
 public:
  const LiveSessionState &state()const noexcept{return fanout_->state();}
  const BnwDeactivationUiFanoutDraft &fanout()const noexcept{return *fanout_;}
  const BnwTurnEndUiOwner &turns()const noexcept{return turns_;}
  bool all_complete_received()const noexcept{return all_complete_;}
  const auto &completed_steps()const noexcept{return steps_;}
 private:
  friend struct BnwDeactivationStatusAccess;
  BnwDeactivationStatusDraft(std::unique_ptr<BnwDeactivationUiFanoutDraft> f,
   BnwTurnEndUiOwner t,bool received,std::vector<BnwDeactivationStep> steps)
   :fanout_(std::move(f)),turns_(std::move(t)),all_complete_(received),steps_(std::move(steps)){}
  std::unique_ptr<BnwDeactivationUiFanoutDraft> fanout_;
  BnwTurnEndUiOwner turns_;
  bool all_complete_=false;
  std::vector<BnwDeactivationStep> steps_;
};
struct BnwDeactivationStatusExecution {
 std::unique_ptr<BnwDeactivationStatusDraft> draft;
 std::string blocker;
};
BnwDeactivationStatusExecution continue_bnw_deactivation_status(
 std::unique_ptr<BnwDeactivationUiFanoutDraft>,const content::RulesDatabase &);
} // namespace civ5::runtime
