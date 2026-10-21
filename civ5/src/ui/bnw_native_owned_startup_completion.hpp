#pragma once
#include <functional>
#include <memory>
#include <thread>
namespace civ5::ui {class BnwNativePackageDiscoveryOwner;}
namespace civ5::ui::detail {
class BnwNativeDefaultCommonProduction;
class BnwNativeCompletedResourceBankCapability;
class BnwNativeCompletedColorQuery;
class BnwNativeCompletedSelectedRead;
// Closed normal-return receipt for resources only. No UI, Window, audio,
// SelectedHost, SQL, or full startup readiness is asserted by this owner.
class BnwNativeOwnedStartupCompletion final {
    friend class BnwNativeDefaultCommonProduction;
    const std::thread::id thread_=std::this_thread::get_id();
    const std::shared_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner> package_;
    const std::shared_ptr<const BnwNativeCompletedResourceBankCapability> resource_;
    const std::shared_ptr<const BnwNativeCompletedColorQuery> color_;
    const std::shared_ptr<const BnwNativeCompletedSelectedRead> read_;
    std::weak_ptr<const BnwNativeOwnedStartupCompletion> self_;
    mutable std::size_t pins_=0;
    mutable bool checking_=false,borrowing_=false,failed_=false;
    class Pin;
    BnwNativeOwnedStartupCompletion(
        std::shared_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner>,
        std::shared_ptr<const BnwNativeCompletedResourceBankCapability>,
        std::shared_ptr<const BnwNativeCompletedColorQuery>,
        std::shared_ptr<const BnwNativeCompletedSelectedRead>);
  public:
    BnwNativeOwnedStartupCompletion(const BnwNativeOwnedStartupCompletion&)=delete;
    BnwNativeOwnedStartupCompletion&operator=(const BnwNativeOwnedStartupCompletion&)=delete;
    void require_current()const;
    void with_actual_completed_inputs(const std::function<void(
        const std::shared_ptr<const ::civ5::ui::BnwNativePackageDiscoveryOwner>&,
        const std::shared_ptr<const BnwNativeCompletedResourceBankCapability>&,
        const std::shared_ptr<const BnwNativeCompletedColorQuery>&,
        const std::shared_ptr<const BnwNativeCompletedSelectedRead>&)>&)const;
};
}
