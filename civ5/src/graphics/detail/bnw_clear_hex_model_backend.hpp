#pragma once
#include "civ5/assets/civis_resource_bank_owner.hpp"
#include <array>
#include <atomic>
#include <memory>
#include <thread>
namespace civ5::runtime {class BnwWorldViewSourceOperationLoan;}
namespace civ5::ui {class UiScreenGlRenderer;}
namespace civ5::graphics {class BnwClearHexHighlightOwner;}
namespace civ5::graphics::detail {
class BnwClearHexModelConstructionScope final {
 friend class ::civ5::ui::UiScreenGlRenderer;
 const std::thread::id creator_=std::this_thread::get_id();
 const runtime::BnwWorldViewSourceOperationLoan*const operation_;
 const BnwClearHexHighlightOwner*const receiver_;
 const std::size_t index_;
 const std::uint64_t serial_,generation_;
 BnwClearHexModelConstructionScope(const runtime::BnwWorldViewSourceOperationLoan& op,
  const BnwClearHexHighlightOwner& owner,std::size_t i,std::uint64_t serial,std::uint64_t g)
  :operation_(&op),receiver_(&owner),index_(i),serial_(serial),generation_(g){}
};
// Root's immutable native SDK result supplies these exact artifact pins only.
// No caller constructs this record and no live Bank authority follows from it.
class SelectedHexModelSdkPins final {
 friend class BnwClearHexModelBackend;
 friend class ::civ5::ui::UiScreenGlRenderer;
 struct Artifact final {std::filesystem::path path;runtime::FileIdentity identity;};
 std::array<Artifact,4> artifacts_; //G3DJ, mesh, skeleton, animation
 runtime::FileIdentity fxsxml_,mesh_,animation_;
 SelectedHexModelSdkPins(std::array<Artifact,4> a,runtime::FileIdentity f,runtime::FileIdentity m,runtime::FileIdentity n)
  :artifacts_(std::move(a)),fxsxml_(std::move(f)),mesh_(std::move(m)),animation_(std::move(n)){}
 // REQUIRED definition from actual completed Root SDK output receipt. It
 // returns immutable source literals, never a bool/default or mutable JSON seal.
 static const SelectedHexModelSdkPins& completed_root_export_owned();
};
class BnwClearHexModelBackend final {
 friend class ::civ5::ui::UiScreenGlRenderer;
 friend class ::civ5::graphics::BnwClearHexHighlightOwner;
 struct State;
 const std::thread::id creator_=std::this_thread::get_id();
 const std::shared_ptr<State> state_;
 explicit BnwClearHexModelBackend(std::shared_ptr<State>);
 static const SelectedHexModelSdkPins& root_pins_owned();
 void clear_instances_owned(const BnwClearHexHighlightOwner&,const runtime::BnwWorldViewSourceOperationLoan&);
 public:
 ~BnwClearHexModelBackend();
 BnwClearHexModelBackend(const BnwClearHexModelBackend&)=delete;
 BnwClearHexModelBackend&operator=(const BnwClearHexModelBackend&)=delete;
};
}
