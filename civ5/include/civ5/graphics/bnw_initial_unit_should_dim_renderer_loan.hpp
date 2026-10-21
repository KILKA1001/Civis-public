#pragma once
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include <memory>
namespace civ5::runtime {class BnwUnitMovePending;class BnwStockLuaAttemptLease;}
namespace civ5::ui {class BnwClientSceneFrame;}
namespace civ5::graphics {
class BnwInitialUnitShouldDimRendererLoan;
// Synchronous genuine Game-gate/queue-return result. Never fanout/readiness.
class BnwInitialUnitShouldDimEnqueueProof final {
 public:
 BnwInitialUnitShouldDimEnqueueProof(const BnwInitialUnitShouldDimEnqueueProof&)=delete;
 BnwInitialUnitShouldDimEnqueueProof& operator=(const BnwInitialUnitShouldDimEnqueueProof&)=delete;
 private:
 friend class BnwInitialUnitShouldDimRendererLoan;
 friend class runtime::BnwNativeUnitAllocatorCore;
 struct State;const std::shared_ptr<State> state_;
 explicit BnwInitialUnitShouldDimEnqueueProof(std::shared_ptr<State>);
 std::shared_ptr<const BnwInitialUnitShouldDimEnqueueProof> retain_factory_owner()const;
 void require_for_plan(const runtime::BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan&)const;
 void require_current_for_core(const runtime::BnwNativeUnitAllocatorCore&)const;
};
class BnwInitialUnitShouldDimRendererLoan final {
 public:
 BnwInitialUnitShouldDimRendererLoan(const BnwInitialUnitShouldDimRendererLoan&)=delete;
 BnwInitialUnitShouldDimRendererLoan& operator=(const BnwInitialUnitShouldDimRendererLoan&)=delete;
 BnwInitialUnitShouldDimRendererLoan(BnwInitialUnitShouldDimRendererLoan&&)=delete;
 BnwInitialUnitShouldDimRendererLoan& operator=(BnwInitialUnitShouldDimRendererLoan&&)=delete;
 private:
 friend class runtime::BnwNativeUnitAllocatorCore;
 friend class BnwInitialUnitShouldDimEnqueueProof;
 friend class ui::BnwClientSceneFrame;
 struct State;const std::shared_ptr<State> state_;
 explicit BnwInitialUnitShouldDimRendererLoan(std::shared_ptr<State>);
 ~BnwInitialUnitShouldDimRendererLoan();
 void preflight_should_dim_enqueue(const runtime::BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan&)const;
 std::shared_ptr<const BnwInitialUnitShouldDimEnqueueProof> enqueue_should_dim(
   const runtime::BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan&)const;
 void require_should_dim_enqueued_current(const runtime::BnwNativeUnitAllocatorCore::InitialUnitShouldDimPlan&,
   const BnwInitialUnitShouldDimEnqueueProof&)const;
};
}
