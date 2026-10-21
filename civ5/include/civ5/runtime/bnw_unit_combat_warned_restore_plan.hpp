#pragma once
#include "civ5/saves/civis_save.hpp"
#include <cstdint>
#include <memory>
namespace civ5::ui { class SelectedBnwPreviewHost; }
namespace civ5::runtime {
class BnwNativeUnitAllocatorCore;class BnwLiveGameContext;
// Synchronous Core-owned restore scope. No public constructor/getters or DTO
// can mint it; Game never retains this raw scope across its issued lifetime.
class BnwUnitCombatWarnedRestorePlan final {
 public:
  ~BnwUnitCombatWarnedRestorePlan();
  BnwUnitCombatWarnedRestorePlan(const BnwUnitCombatWarnedRestorePlan&)=delete;
  BnwUnitCombatWarnedRestorePlan&operator=(const BnwUnitCombatWarnedRestorePlan&)=delete;
  BnwUnitCombatWarnedRestorePlan(BnwUnitCombatWarnedRestorePlan&&)=delete;
  BnwUnitCombatWarnedRestorePlan&operator=(BnwUnitCombatWarnedRestorePlan&&)=delete;
 private:
  friend class BnwNativeUnitAllocatorCore;friend class BnwLiveGameContext;
  explicit BnwUnitCombatWarnedRestorePlan(const BnwNativeUnitAllocatorCore&,const saves::CivisSaveSection&);
  void require_current_without_callbacks()const;
  void retire_without_callbacks()noexcept;
  bool original_payload_read_qualified()const noexcept;
  bool before()const noexcept;
  std::uint64_t before_ordinal()const noexcept;
  std::uint64_t ordinal()const noexcept;
  const ui::SelectedBnwPreviewHost&host_without_callbacks()const;
  struct Storage;std::unique_ptr<Storage>storage_;
};
} // namespace civ5::runtime
