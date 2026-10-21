#pragma once
#include <cstdint>
#include <memory>
namespace civ5::ui {class SelectedBnwPreviewHost;}
namespace civ5::runtime {
class BnwRetainedTurnCursor;class BnwSelectedStockLuaSourceOwner;struct LiveSessionState;
// Closed original-Read seal over the SAME existing activation owner's map.
// The seal owns no competing ProcessedAutoMoves data and cannot be constructed
// from a save scalar, public owner DTO, or a replacement shared_ptr alias.
class BnwProcessedAutoMovesImportOwner final {
 public:
  ~BnwProcessedAutoMovesImportOwner();
  BnwProcessedAutoMovesImportOwner(const BnwProcessedAutoMovesImportOwner&)=delete;
  BnwProcessedAutoMovesImportOwner&operator=(const BnwProcessedAutoMovesImportOwner&)=delete;
 private:
  friend class ui::SelectedBnwPreviewHost;friend class BnwRetainedTurnCursor;
  explicit BnwProcessedAutoMovesImportOwner(std::shared_ptr<const ui::SelectedBnwPreviewHost>);
  void require_host_control_without_callbacks(const std::shared_ptr<const ui::SelectedBnwPreviewHost>&)const;
  void require_pristine_checkpoint_without_callbacks(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,std::uint64_t)const;
  void require_current_without_callbacks(const ui::SelectedBnwPreviewHost&,const LiveSessionState&,std::uint64_t)const;
  bool value_without_callbacks(std::int32_t)const;
  std::uint64_t sequence_without_callbacks()const;
  void store_false_without_callbacks(std::int32_t)noexcept;
  struct Impl;std::unique_ptr<Impl>impl_;
};
}
