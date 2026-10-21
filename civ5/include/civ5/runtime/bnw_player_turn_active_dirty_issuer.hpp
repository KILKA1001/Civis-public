#pragma once
#include "civ5/ui/bnw_engine_ui_end_turn_queue.hpp"
#include <cstdint>
#include <memory>
namespace civ5::runtime {
class BnwSelectedStockLuaSourceOwner;class BnwRetainedTurnCursor;struct LiveSessionState;
// Sole actual Player PublishEndTurnDirty issuer. No public constructor/result.
class BnwPlayerTurnActiveDirtyIssuer final {
 public:
  ~BnwPlayerTurnActiveDirtyIssuer();
  BnwPlayerTurnActiveDirtyIssuer(const BnwPlayerTurnActiveDirtyIssuer&)=delete;
  BnwPlayerTurnActiveDirtyIssuer&operator=(const BnwPlayerTurnActiveDirtyIssuer&)=delete;
 private:
  friend class BnwSelectedStockLuaSourceOwner;friend class BnwRetainedTurnCursor;
  friend class ::civ5::ui::BnwEngineUiTurnOwner;friend class ::civ5::ui::BnwEngineUiEndTurnQueue;
  struct Impl;std::shared_ptr<Impl> impl_;
  explicit BnwPlayerTurnActiveDirtyIssuer(std::shared_ptr<Impl>);
  static std::shared_ptr<const BnwPlayerTurnActiveDirtyIssuer> create_owned(
   std::shared_ptr<BnwSelectedStockLuaSourceOwner>,std::shared_ptr<BnwRetainedTurnCursor>,
   std::weak_ptr<const void>,std::weak_ptr<const void>,std::shared_ptr<const LiveSessionState>,std::uint64_t,std::uint64_t);
  void require_source_lineage_without_callbacks(const BnwSelectedStockLuaSourceOwner&,const BnwRetainedTurnCursor&,
   const void*,const void*,const LiveSessionState&,std::uint64_t,std::uint64_t)const;
  void require_before_publication_without_callbacks(const ui::BnwEngineUiTurnOwner&,const BnwStockLuaAttemptLease&)const;
  void require_before_delivery_without_callbacks(const ui::BnwEngineUiTurnOwner&,const BnwStockLuaAttemptLease&)const;
  void record_queued_without_callbacks(const ui::BnwEngineUiEndTurnQueue&,const ui::BnwEngineUiEndTurnQueue::Event&)const noexcept;
  void record_publisher_return_without_callbacks(const ui::BnwEngineUiEndTurnQueue&,const ui::BnwEngineUiEndTurnQueue::Event&)const noexcept;
  void record_delivery_attempt_without_callbacks(const ui::BnwEngineUiEndTurnQueue&,const ui::BnwEngineUiEndTurnQueue::Event&)const noexcept;
  void record_delivered_without_callbacks(const ui::BnwEngineUiEndTurnQueue&,const ui::BnwEngineUiEndTurnQueue::Event&,
   const ui::BnwEngineUiEndTurnQueue::Completion&,std::uint64_t)const noexcept;
  bool queued_without_callbacks()const noexcept;
  bool publisher_returned_without_callbacks()const noexcept;
  bool delivery_attempted_without_callbacks()const noexcept;
  bool delivered_without_callbacks()const noexcept;
};
}
