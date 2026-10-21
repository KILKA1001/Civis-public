#pragma once
#include "civ5/runtime/bnw_unit_order_owners.hpp"
namespace civ5::runtime {
inline constexpr std::array<BnwUnitOrderSourceFile,4> kBnwNotificationCleanupSources{{
 {"CvGameCoreDLL_Expansion2/CvNotifications.cpp","de1943b762293092ca14ada5c1c79d7f1f41fc20413358b06eab3c7f0c33a581"},
 {"CvGameCoreDLLUtil/include/CvEnums.h","6781f34a6a6a9fac1cbaad3a3477afabf540669af9f860f57ad54b967c22da9b"},
 {"CvGameCoreDLL_Expansion2/CvPlayer.cpp","9635fe516e590e133c3907ee25c13128fb666e3e7817c967e247affb700a952d"},
 {"CvGameCoreDLL_Expansion2/CvGame.cpp","1318f7522c1bb551f3a2a7b0452c953bbbcad4291d1fe80f4791613f8f9fae3c"}
}};
class BnwNotificationCleanupOwner;
class BnwNotificationReadReceipt {
 public:
  BnwNotificationReadReceipt(const BnwNotificationReadReceipt &)=delete;
  BnwNotificationReadReceipt &operator=(const BnwNotificationReadReceipt &)=delete;
  BnwNotificationReadReceipt(BnwNotificationReadReceipt &&o) noexcept
   :identity_(std::move(o.identity_)),guid_(o.guid_),load_world_(std::move(o.load_world_)),epoch_(std::move(o.epoch_)),used_(o.used_){o.used_=true;}
 private:
  friend BnwNotificationReadReceipt attest_bnw_notification_read_context(const LiveSessionState &,const saves::SavedSession &,const SessionCheckpointIdentity &,const content::RulesDatabase &);
  friend BnwNotificationCleanupOwner bind_bnw_notification_cleanup_read(const LiveSessionState &,BnwNotificationReadReceipt &&,const content::RulesDatabase &);
  BnwNotificationReadReceipt(SessionCheckpointIdentity i,std::array<std::uint8_t,16> g,std::string w,std::string e):identity_(std::move(i)),guid_(g),load_world_(std::move(w)),epoch_(std::move(e)){}
  SessionCheckpointIdentity identity_;std::array<std::uint8_t,16> guid_{};std::string load_world_,epoch_;bool used_=false;
};
// Attest only at the real import/load Read frontier. Matching source-derived
// current flags/schema required. Not a reset or advanced-resume operation.
BnwNotificationReadReceipt attest_bnw_notification_read_context(const LiveSessionState &,const saves::SavedSession &,const SessionCheckpointIdentity &,const content::RulesDatabase &);
class BnwNotificationCleanupOwner {
 public:
  const auto &identity() const noexcept{return identity_;}const auto &map_guid() const noexcept{return map_guid_;}
  const auto &epoch() const noexcept{return epoch_;}std::uint64_t operations() const noexcept{return operations_;}
 private:
  friend BnwNotificationCleanupOwner bind_bnw_notification_cleanup_read(const LiveSessionState &,BnwNotificationReadReceipt &&,const content::RulesDatabase &);
  friend class BnwNotificationCleanupOperations;
  BnwNotificationCleanupOwner(SessionCheckpointIdentity i,std::array<std::uint8_t,16> g,std::string e):identity_(std::move(i)),map_guid_(g),epoch_(std::move(e)){}
  SessionCheckpointIdentity identity_;std::array<std::uint8_t,16> map_guid_{};std::string epoch_;std::uint64_t operations_=0;
};
BnwNotificationCleanupOwner bind_bnw_notification_cleanup_read(const LiveSessionState &,BnwNotificationReadReceipt &&,const content::RulesDatabase &);
// Raw notification rings + SAME owned load/transient flags + clock identity.
std::string bnw_notification_epoch(const LiveSessionState &,const SessionCheckpointIdentity &,const content::RulesDatabase &);
std::int32_t bnw_notification_game_turn_active_count(const LiveSessionState &);
bool bnw_notification_type_end_turn_expired(std::uint32_t,std::optional<std::int32_t> specific_turn,std::int32_t game_turn);
struct BnwNotificationUiEntry {
 std::int64_t player=-1,lookup=-1;std::uint64_t slot=0;std::uint32_t type=0;
 std::string message,summary;std::int32_t x=-1,y=-1,data=-1,extra=-1,turn=-1;
 bool operator==(const BnwNotificationUiEntry &) const = default;
};
struct BnwRemoveNotificationReceipt {std::int64_t player=-1,lookup=-1;std::uint64_t slot=0;};
struct BnwNotificationUiBatch {
 SessionCheckpointIdentity identity;std::array<std::uint8_t,16> map_guid{};
 std::string before_epoch,after_epoch;std::uint64_t expected_client_revision=0;
 std::vector<BnwRemoveNotificationReceipt> removals;
};
class BnwNotificationUiOwner {
 public:
  const auto &identity() const noexcept{return identity_;}const auto &map_guid() const noexcept{return map_guid_;}
  const auto &epoch() const noexcept{return epoch_;}std::uint64_t client_revision() const noexcept{return client_revision_;}
  const auto &entries() const noexcept{return entries_;}const auto &received_removals() const noexcept{return received_removals_;}
 private:
  friend BnwNotificationUiOwner start_bnw_notification_ui_client(const LiveSessionState &,const BnwNotificationCleanupOwner &,const content::RulesDatabase &);
  friend void apply_bnw_notification_ui_batch(const LiveSessionState &,BnwNotificationUiOwner &,const content::RulesDatabase &,const BnwNotificationUiBatch &);
  BnwNotificationUiOwner(SessionCheckpointIdentity i,std::array<std::uint8_t,16> g,std::string e):identity_(std::move(i)),map_guid_(g),epoch_(std::move(e)){}
  SessionCheckpointIdentity identity_;std::array<std::uint8_t,16> map_guid_{};std::string epoch_;std::uint64_t client_revision_=0;
  std::map<std::pair<std::int64_t,std::int64_t>,BnwNotificationUiEntry> entries_;
  std::vector<BnwRemoveNotificationReceipt> received_removals_;
};
// Explicit new Civis client owns actual ring entries, no Widget inference and
// no original saved UI state. Native needsBroadcast is preserved at startup.
BnwNotificationUiOwner start_bnw_notification_ui_client(const LiveSessionState &,const BnwNotificationCleanupOwner &,const content::RulesDatabase &);
void apply_bnw_notification_ui_batch(const LiveSessionState &,BnwNotificationUiOwner &,const content::RulesDatabase &,const BnwNotificationUiBatch &);
enum class BnwNotificationCleanupStep : std::uint8_t {get_notifications,ring_scan,expiry_test,wait_extra_turn_clear,dismiss_store,remove_notification,cleanup_complete};
struct BnwNotificationCleanupDraft {
 LiveSessionState state;BnwNotificationCleanupOwner owner;BnwNotificationUiOwner client;
 BnwNotificationUiBatch batch;std::vector<BnwNotificationCleanupStep> completed_steps;
 std::vector<std::uint64_t> waited_slots;std::int32_t active_players_after=0;
};
struct BnwNotificationCleanupExecution {std::unique_ptr<BnwNotificationCleanupDraft> draft;std::string blocker;};
// Complete supported ordinary-SP EndOfTurnCleanup -> Dismiss(false). Copies
// world/retained flags/client together. Ring positions/lookups/strings unchanged;
// actual dismissal and Remove receipts update owned fields. No rebuild/reset.
BnwNotificationCleanupExecution execute_bnw_notification_end_turn_cleanup_draft(const LiveSessionState &,const BnwNotificationCleanupOwner &,const BnwNotificationUiOwner &,const content::RulesDatabase &,std::int64_t player);
struct BnwNotificationAddFlagDraft {LiveSessionState state;BnwNotificationCleanupOwner owner;};
struct BnwNotificationAddFlagExecution {std::unique_ptr<BnwNotificationAddFlagDraft> draft;std::string blocker;};
// Source Add's bounded transient-field component: false then guard
// active && auto && expiry(type,index=-1). This is NOT a full Add command or
// notification creation; Add's redundancy/FX/minimap remain other operations.
BnwNotificationAddFlagExecution execute_bnw_notification_add_wait_flag_draft(const LiveSessionState &,const BnwNotificationCleanupOwner &,const content::RulesDatabase &,std::int64_t player,std::uint64_t slot);
} // namespace civ5::runtime
