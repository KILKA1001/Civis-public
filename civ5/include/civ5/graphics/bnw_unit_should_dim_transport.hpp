#pragma once
#include "civ5/runtime/bnw_stock_lua_operation_loan.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>
namespace civ5::runtime {class BnwNativeUnitAllocatorCore;class BnwOwnedGameplayEventPumpLoan;}
namespace civ5::graphics {
class BnwUnitShouldDimQueue;class BnwUnitShouldDimCache;class BnwUnitShouldDimReceiverLoan;
class BnwInitialUnitShouldDimRendererLoan;
class BnwInitialUnitShouldDimEnqueueProof;
// Closed source operation. Construction belongs to the actual retained VM
// owner; an operation serial alone cannot manufacture an event-pump boundary.
class BnwUnitShouldDimSourceLoan final {
 friend class runtime::BnwSelectedStockLuaBootstrap;
 friend class BnwInitialUnitShouldDimRendererLoan;
 friend class BnwUnitShouldDimQueue;friend class BnwUnitShouldDimCache;
 friend class BnwUnitShouldDimReceiverLoan;
 friend class BnwUnitShouldDimFanoutProof;
 friend class BnwUnitShouldDimEnqueueProof;
 struct State;std::shared_ptr<State> state_;
 explicit BnwUnitShouldDimSourceLoan(const runtime::BnwStockLuaAttemptLease&);
 BnwUnitShouldDimSourceLoan(const runtime::BnwStockLuaAttemptLease&,std::function<void()>);
 ~BnwUnitShouldDimSourceLoan();
 void pure()const;void full()const;void fail()const noexcept;
 static void pure_state(const std::shared_ptr<State>&);
 static void full_state(const std::shared_ptr<State>&);
 static void fail_state(const std::shared_ptr<State>&)noexcept;
 BnwUnitShouldDimSourceLoan(const BnwUnitShouldDimSourceLoan&)=delete;
 BnwUnitShouldDimSourceLoan& operator=(const BnwUnitShouldDimSourceLoan&)=delete;
};
// Core-only post-store callback lenders. The Core54 successor must provide
// real canonical node getters and its genuine earned dispatch guard here.
// No Game predicate is represented by a bool argument in this transport.
class BnwUnitShouldDimPayloadLoan final {
 friend class runtime::BnwNativeUnitAllocatorCore;friend class BnwUnitShouldDimQueue;
 friend class BnwInitialUnitShouldDimRendererLoan;
 const BnwUnitShouldDimSourceLoan& source_;
 const std::function<void()> dispatch_current_;
 const std::function<std::int32_t()> owner_,id_;
 const std::int32_t stored_moves_;
 BnwUnitShouldDimPayloadLoan(const BnwUnitShouldDimSourceLoan&,
   std::function<void()>,std::function<std::int32_t()>,
   std::function<std::int32_t()>,std::int32_t);
 BnwUnitShouldDimPayloadLoan(const BnwUnitShouldDimPayloadLoan&)=delete;
 BnwUnitShouldDimPayloadLoan& operator=(const BnwUnitShouldDimPayloadLoan&)=delete;
};
// Scalar observation: optional DWORDs preserve unspecified copied operands.
// This DTO does not authorize enqueue, drain, mutation or Lua dispatch.
struct BnwUnitShouldDimPayloadObservation {
 std::array<std::optional<std::uint32_t>,8> dwords;
};
class BnwUnitShouldDimQueuedEvent final {
 friend class BnwUnitShouldDimQueue;friend class BnwUnitShouldDimCache;
 friend class BnwUnitShouldDimFanoutProof;
 friend class BnwUnitShouldDimEnqueueProof;
 friend class BnwUnitShouldDimReceiverLoan;
 struct State;const std::shared_ptr<State> state_;
 explicit BnwUnitShouldDimQueuedEvent(std::shared_ptr<State>);
 public:
 BnwUnitShouldDimQueuedEvent(const BnwUnitShouldDimQueuedEvent&)=delete;
 BnwUnitShouldDimQueuedEvent& operator=(const BnwUnitShouldDimQueuedEvent&)=delete;
 BnwUnitShouldDimPayloadObservation observation()const;
};
class BnwUnitShouldDimEnqueueProof final {
 friend class BnwUnitShouldDimQueue;
 friend class BnwInitialUnitShouldDimRendererLoan;
 friend class BnwInitialUnitShouldDimEnqueueProof;
 std::weak_ptr<const BnwUnitShouldDimQueue> queue_;
 const std::shared_ptr<const BnwUnitShouldDimQueuedEvent> event_;
 const std::uint64_t enqueue_serial_;
 BnwUnitShouldDimEnqueueProof(std::weak_ptr<const BnwUnitShouldDimQueue>,
   std::shared_ptr<const BnwUnitShouldDimQueuedEvent>,std::uint64_t);
 void require_current_for_source(const BnwUnitShouldDimSourceLoan&)const;
 public:
 BnwUnitShouldDimEnqueueProof(const BnwUnitShouldDimEnqueueProof&)=delete;
 BnwUnitShouldDimEnqueueProof& operator=(const BnwUnitShouldDimEnqueueProof&)=delete;
};
// Actual native message-cache owner, separate from drawable model registry.
// Only proven default scalar fields and individual flag bits become known.
class BnwUnitShouldDimCache final {
 friend class runtime::BnwSelectedStockLuaBootstrap;friend class BnwUnitShouldDimQueue;
 friend class BnwUnitShouldDimFanoutProof;
 struct State;struct Pin;std::shared_ptr<State> state_;
 explicit BnwUnitShouldDimCache(std::shared_ptr<State>);
 static std::shared_ptr<BnwUnitShouldDimCache> bind_fresh(const BnwUnitShouldDimSourceLoan&);
 void apply(const BnwUnitShouldDimQueuedEvent&,const std::shared_ptr<BnwUnitShouldDimSourceLoan::State>&);
 void pure(const std::shared_ptr<BnwUnitShouldDimSourceLoan::State>&)const;
 public:
 ~BnwUnitShouldDimCache();
 BnwUnitShouldDimCache(const BnwUnitShouldDimCache&)=delete;
 BnwUnitShouldDimCache& operator=(const BnwUnitShouldDimCache&)=delete;
};
class BnwUnitShouldDimFanoutProof final {
 friend class BnwUnitShouldDimQueue;
 friend class BnwUnitShouldDimReceiverLoan;
 struct State;const std::shared_ptr<State> state_;
 const std::shared_ptr<const BnwUnitShouldDimQueuedEvent> event_;
 const std::uint64_t pump_boundary_,receiver_operation_;
 const std::vector<std::uint64_t> actual_registrations_;
 BnwUnitShouldDimFanoutProof(std::shared_ptr<State>,std::shared_ptr<const BnwUnitShouldDimQueuedEvent>,
   std::uint64_t,std::uint64_t,std::vector<std::uint64_t>);
 void require_for_event(const BnwUnitShouldDimQueuedEvent&,const BnwUnitShouldDimReceiverLoan&)const;
 public:
 BnwUnitShouldDimFanoutProof(const BnwUnitShouldDimFanoutProof&)=delete;
 BnwUnitShouldDimFanoutProof& operator=(const BnwUnitShouldDimFanoutProof&)=delete;
};
// The common actual pump owner supplies this separate loan. Its declaration
// and issuer are mandatory integration gates, shared with event69. This class
// cannot reinterpret an enqueue operation/old Plan as late receiver authority.
class BnwUnitShouldDimReceiverLoan final {
 friend class runtime::BnwSelectedStockLuaBootstrap;friend class BnwUnitShouldDimQueue;
 friend class BnwUnitShouldDimFanoutProof;
 const BnwUnitShouldDimSourceLoan& source_;
 const runtime::BnwOwnedGameplayEventPumpLoan& pump_;
 explicit BnwUnitShouldDimReceiverLoan(const BnwUnitShouldDimSourceLoan&,
   const runtime::BnwOwnedGameplayEventPumpLoan&);
 void require_current()const;
 BnwUnitShouldDimReceiverLoan(const BnwUnitShouldDimReceiverLoan&)=delete;
 BnwUnitShouldDimReceiverLoan& operator=(const BnwUnitShouldDimReceiverLoan&)=delete;
};
class BnwUnitShouldDimQueue final {
 friend class runtime::BnwSelectedStockLuaBootstrap;
 friend class runtime::BnwNativeUnitAllocatorCore;friend class BnwUnitShouldDimReceiverLoan;
 friend class BnwUnitShouldDimFanoutProof;
 friend class BnwInitialUnitShouldDimRendererLoan;
 friend class BnwUnitShouldDimEnqueueProof;
 struct State;struct Pin;std::shared_ptr<State> state_;
 explicit BnwUnitShouldDimQueue(std::shared_ptr<State>);
 static std::shared_ptr<BnwUnitShouldDimQueue> bind_fresh(const BnwUnitShouldDimSourceLoan&);
 class Subscription;
 // Actual registration order is vector insertion order, never native/Lua
 // hardcoded fanout order. The actual VM must retain each returned owner.
 std::shared_ptr<Subscription> register_native(std::shared_ptr<BnwUnitShouldDimCache>,
   const BnwUnitShouldDimSourceLoan&);
 std::shared_ptr<Subscription> register_lua(
   std::function<void(const BnwUnitShouldDimQueuedEvent&,const BnwUnitShouldDimSourceLoan&)>,
   const BnwUnitShouldDimSourceLoan&);
 std::shared_ptr<const BnwUnitShouldDimEnqueueProof> enqueue(
   const BnwUnitShouldDimPayloadLoan&);
 static void preflight_from_owned_weak(const std::weak_ptr<BnwUnitShouldDimQueue>&,
   const BnwUnitShouldDimSourceLoan&);
 static std::shared_ptr<const BnwUnitShouldDimEnqueueProof> enqueue_from_owned_weak(
   const std::weak_ptr<BnwUnitShouldDimQueue>&,const BnwUnitShouldDimPayloadLoan&);
 std::shared_ptr<const BnwUnitShouldDimFanoutProof> consume_one(
   const BnwUnitShouldDimReceiverLoan&);
 void pure(const std::shared_ptr<BnwUnitShouldDimSourceLoan::State>&)const;
 public:
 ~BnwUnitShouldDimQueue();
 BnwUnitShouldDimQueue(const BnwUnitShouldDimQueue&)=delete;
 BnwUnitShouldDimQueue& operator=(const BnwUnitShouldDimQueue&)=delete;
};
}
