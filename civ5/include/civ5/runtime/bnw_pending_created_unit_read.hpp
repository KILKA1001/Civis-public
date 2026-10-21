#pragma once
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include <string_view>
#include <utility>
namespace civ5::graphics {class BnwUnitSetupGraphicalQueue;}
namespace civ5::runtime {
// Factory-issued live Bank binding retained by the actual Scene FIFO batch.
// It is DATA custody, not a Source, Lua call, or rendered-instance admission.
class BnwPendingCreatedUnitRead final {
 friend class BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan;
 friend class graphics::BnwUnitSetupGraphicalQueue;
 struct Storage;const std::shared_ptr<Storage>storage_;
 explicit BnwPendingCreatedUnitRead(std::shared_ptr<Storage>);
 void require_issuing_plan_without_callbacks()const;
 void require_returned_without_callbacks()const;
 void require_same_world_without_callbacks(const LiveSessionState*,std::uint64_t)const;
 std::pair<std::int32_t,std::int32_t>issued_owner_full_id_without_callbacks()const;
 std::int32_t read_integer_without_callbacks(std::string_view)const;
 bool read_boolean_without_callbacks(std::string_view)const;
 std::int32_t read_domain_without_callbacks()const;
 bool read_trade_without_callbacks()const;
 public:
 ~BnwPendingCreatedUnitRead();
 BnwPendingCreatedUnitRead(const BnwPendingCreatedUnitRead&)=delete;
 BnwPendingCreatedUnitRead&operator=(const BnwPendingCreatedUnitRead&)=delete;
};
}
