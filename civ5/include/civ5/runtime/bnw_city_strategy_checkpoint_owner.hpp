#pragma once
#include "civ5/runtime/bnw_city_management_checkpoint.hpp"
#include <memory>
#include <stdexcept>
namespace civ5::ui { class SelectedBnwPreviewHost; }
namespace civ5::runtime {
class BnwCityStrategyCheckpointUnavailable : public std::runtime_error {
 public:using std::runtime_error::runtime_error;
};
namespace detail {
// Detached schema validation only: no issuer/Reset/AI/mutation permission.
void require_bnw_city_strategy_checkpoint_arrays(const saves::SavedSessionSnapshot&);
}
class BnwCityStrategyCheckpointOwner;
class BnwCityStrategyDetachedRestore final {
 public:
 // SAME genuine parents/currentness are checked on every access. A returned
 // const snapshot is data, never a live-session publication capability.
 std::shared_ptr<const saves::SavedSessionSnapshot> read_world()const;
 static constexpr bool live_publication_ready()noexcept{return false;}
 private:
 friend class BnwCityStrategyCheckpointOwner;
 struct Impl;std::shared_ptr<const Impl> impl_;
 explicit BnwCityStrategyDetachedRestore(std::shared_ptr<const Impl>)noexcept;
};
class BnwCityStrategyCheckpointOwner final {
 public:
 // Existing ID27 bytes from the ACTUAL current world; repeatable read only.
 saves::CivisSaveSection section()const;
 // Exactly one successfully prepared detached restore per genuine owner.
 // No caller section/DTO/predicate/hash is accepted as source authority.
 BnwCityStrategyDetachedRestore prepare_detached_restore()const;
 static constexpr bool full_persistence()noexcept{return false;}
 static constexpr bool city_entry_phase_ready()noexcept{return false;}
 private:
 friend class BnwCityStrategyDetachedRestore;
 friend BnwCityStrategyCheckpointOwner capture_bnw_city_strategy_checkpoint(
   std::shared_ptr<const ui::SelectedBnwPreviewHost>);
 struct Impl;std::shared_ptr<Impl> impl_;
 explicit BnwCityStrategyCheckpointOwner(std::shared_ptr<Impl>)noexcept;
 void require_current()const;
};
BnwCityStrategyCheckpointOwner capture_bnw_city_strategy_checkpoint(
 std::shared_ptr<const ui::SelectedBnwPreviewHost>);
}
