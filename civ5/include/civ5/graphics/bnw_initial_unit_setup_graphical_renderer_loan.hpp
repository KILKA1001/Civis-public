#pragma once
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include <memory>
namespace civ5::ui {class BnwClientSceneFrame;}
namespace civ5::graphics {
class BnwUnitSetupGraphicalQueue;
class BnwInitialUnitSetupGraphicalRendererLoan;
class BnwInitialUnitSetupGraphicalEnqueueProof final {
 friend class BnwInitialUnitSetupGraphicalRendererLoan;
 friend class runtime::BnwNativeUnitAllocatorCore;
 struct State;const std::shared_ptr<State> state_;
 explicit BnwInitialUnitSetupGraphicalEnqueueProof(std::shared_ptr<State>);
 std::shared_ptr<const BnwInitialUnitSetupGraphicalEnqueueProof> retain_factory_owner()const;
 void require_for_plan(const runtime::BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan&)const;
 void require_current_for_core(const runtime::BnwNativeUnitAllocatorCore&)const;
 public:
 BnwInitialUnitSetupGraphicalEnqueueProof(const BnwInitialUnitSetupGraphicalEnqueueProof&)=delete;
 BnwInitialUnitSetupGraphicalEnqueueProof&operator=(const BnwInitialUnitSetupGraphicalEnqueueProof&)=delete;
};
class BnwInitialUnitSetupGraphicalRendererLoan final {
 friend class ui::BnwClientSceneFrame;
 friend class runtime::BnwNativeUnitAllocatorCore;
 friend class BnwInitialUnitSetupGraphicalEnqueueProof;
 friend class BnwUnitSetupGraphicalQueue;
 struct State;const std::shared_ptr<State> state_;
 explicit BnwInitialUnitSetupGraphicalRendererLoan(std::shared_ptr<State>);
 ~BnwInitialUnitSetupGraphicalRendererLoan();
 void preflight_setup_graphical_enqueue(const runtime::BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan&)const;
 std::shared_ptr<const BnwInitialUnitSetupGraphicalEnqueueProof> enqueue_setup_graphical(
  const runtime::BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan&)const;
 void require_setup_graphical_enqueued_current(
  const runtime::BnwNativeUnitAllocatorCore::InitialUnitSetupGraphicalPlan&,
  const BnwInitialUnitSetupGraphicalEnqueueProof&)const;
 void require_dispatch_current()const;
 void require_queue_identity(const BnwUnitSetupGraphicalQueue&)const;
 public:
 BnwInitialUnitSetupGraphicalRendererLoan(const BnwInitialUnitSetupGraphicalRendererLoan&)=delete;
 BnwInitialUnitSetupGraphicalRendererLoan&operator=(const BnwInitialUnitSetupGraphicalRendererLoan&)=delete;
 BnwInitialUnitSetupGraphicalRendererLoan(BnwInitialUnitSetupGraphicalRendererLoan&&)=delete;
 BnwInitialUnitSetupGraphicalRendererLoan&operator=(BnwInitialUnitSetupGraphicalRendererLoan&&)=delete;
};
}
