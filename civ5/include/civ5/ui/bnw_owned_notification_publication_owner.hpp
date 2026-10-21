#pragma once
#include "civ5/ui/bnw_engine_ui_turn_owner.hpp"
#include <map>
#include <optional>
#include <set>
#include <thread>
#include <utility>
namespace civ5::ui {
struct LiveUiFrame;
class BnwNotificationPublicationLender;
class BnwNotificationNullablePlotFxReturn;
class BnwNotificationMinimapReturn;
class BnwNotificationAfterSwapReturn;
class BnwOwnedNotificationPublicationOwner;
// Owner-model prerequisite ONLY. It is not a Scene/GUI/HWND/source loan,
// nullable-plot/minimap/draw return or AFTER_SWAP acceptance authority.
class BnwNotificationOwnerModelLoan final {
 public:
  ~BnwNotificationOwnerModelLoan();
  BnwNotificationOwnerModelLoan(const BnwNotificationOwnerModelLoan&)=delete;
  BnwNotificationOwnerModelLoan&operator=(const BnwNotificationOwnerModelLoan&)=delete;
 private:
  friend class BnwOwnedNotificationPublicationOwner;
  friend class BnwNotificationPublicationLender;
  const std::thread::id creator_=std::this_thread::get_id();
  std::shared_ptr<const BnwOwnedNotificationPublicationOwner> owner_;
  std::weak_ptr<const LiveUiHost> ui_;
  std::weak_ptr<const BnwEngineUiTurnOwner> engine_;
  std::weak_ptr<const SelectedBnwPreviewHost> host_;
  std::weak_ptr<const runtime::LiveSessionState> world_;
  std::shared_ptr<const struct BnwOwnedNotificationFrameFeed> feed_;
  std::uint64_t generation_=0,ui_sequence_=0;
  bool counted_=false;
  explicit BnwNotificationOwnerModelLoan(const BnwOwnedNotificationPublicationOwner&,const LiveUiFrame&);
  void require_origin_without_callbacks()const;
  void require_same_frame_payload_without_callbacks(const LiveUiFrame&)const;
  void require_source_origin_without_callbacks(const runtime::BnwOwnedGameplayEventPumpLoan&,const runtime::BnwStockLuaAttemptLease&)const;
  BnwEngineUiNotificationEffect pending_effect()const;
  void quarantine_without_callbacks()const noexcept;
};
// Value feed only: neither a lender nor an AFTER_PRESENT receipt.
struct BnwOwnedNotificationFrameFeed {
 std::uint64_t model_revision=0,received_prefix=0;
 std::int64_t actor=-1;
 std::vector<BnwEngineUiNotificationEffect> entries;
 // No imported shown bit is invented; actual presentation remains unissued.
};
struct BnwNotificationPublicationProgress {
 std::uint64_t received_prefix=0,model_revision=0,add_calls=0,remove_calls=0;
 std::optional<BnwEngineUiNotificationEffectKind> unsupported;
 std::uint64_t unsupported_occurrence=0;
 bool failed=false;
};
class BnwOwnedNotificationPublicationOwner final {
 public:
  ~BnwOwnedNotificationPublicationOwner()=default;
  BnwOwnedNotificationPublicationOwner(const BnwOwnedNotificationPublicationOwner&)=delete;
  BnwOwnedNotificationPublicationOwner&operator=(const BnwOwnedNotificationPublicationOwner&)=delete;
 private:
  friend class BnwEngineUiTurnOwner;
  friend class LiveUiHost;
  friend class BnwNotificationOwnerModelLoan;
  friend class BnwNotificationPublicationLender;
  using Key=std::pair<std::int32_t,std::int32_t>;
  const std::thread::id creator_=std::this_thread::get_id();
  std::weak_ptr<const LiveUiHost> ui_;
  std::weak_ptr<const SelectedBnwPreviewHost> host_;
  std::weak_ptr<const runtime::LiveSessionState> world_;
  std::weak_ptr<const BnwEngineUiTurnOwner> engine_;
  std::weak_ptr<BnwOwnedNotificationPublicationOwner> self_;
  std::uint64_t generation_=0;
  // Current UI model, not another pending event queue.
  std::map<Key,BnwEngineUiNotificationEffect> entries_;
  std::set<Key> removed_;
  mutable BnwNotificationPublicationProgress progress_;
  bool busy_=false;
  // Exact actual published payload storage, retained as a value prerequisite.
  // An earlier captured/presented frame does not grant a current Scene.
  mutable std::shared_ptr<const BnwOwnedNotificationFrameFeed> issued_frame_feed_;
  mutable std::uint64_t issued_ui_sequence_=0;
  mutable std::optional<std::int32_t> issued_turn_;
  mutable std::size_t model_loan_pins_=0;
  void require_frame_binding_without_callbacks(const LiveUiFrame&)const;
  void require_retained_model_without_callbacks(const BnwNotificationOwnerModelLoan&)const;
  void quarantine_model_loan_without_callbacks(const BnwNotificationOwnerModelLoan&)const noexcept;
  std::unique_ptr<BnwNotificationOwnerModelLoan> capture_notification_model_for_graphics_without_callbacks(const LiveUiFrame&)const;
  // No physical accept body exists until real opaque producer returns qualify.
  void accept_actual_nullable_plot_fx(const BnwNotificationPublicationLender&,const BnwNotificationNullablePlotFxReturn&);
  void accept_actual_minimap(const BnwNotificationPublicationLender&,const BnwNotificationMinimapReturn&);
  void accept_actual_after_swap(const BnwNotificationPublicationLender&,const BnwNotificationAfterSwapReturn&);
  explicit BnwOwnedNotificationPublicationOwner(std::weak_ptr<const LiveUiHost>,
   std::weak_ptr<const SelectedBnwPreviewHost>,std::weak_ptr<const runtime::LiveSessionState>,
   std::weak_ptr<const BnwEngineUiTurnOwner>,std::uint64_t);
  void merge_frame(LiveUiFrame&,const LiveUiHost&)const;
};
}
