#include "bnw_native_common_source_validation_context.hpp"
#include "bnw_native_default_namespace_policy.hpp"
#include "civ5/ui/bnw_native_installation_input_owner.hpp"
#include "bnw_native_completed_cold_source_loan.hpp"
#include "bnw_native_callback_owner_guard.hpp"

namespace civ5::ui::detail {
namespace {
void need(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
}
struct BnwNativeDefaultNamespacePolicyOwner::Impl {
    BnwNativeCallbackOwnerState state;
    std::weak_ptr<const BnwNativeDefaultNamespacePolicyOwner> self;
    std::weak_ptr<const BnwNativeColdFilesystemOwner> cold;
    std::shared_ptr<const BnwNativeEarlyInputLease> lease;
    std::shared_ptr<const ::civ5::ui::BnwNativeInstallationInputOwner> installation_input;
    const BnwDefaultNamespaceInitialPolicyDiagnostic initial;
    using Guard = BnwNativeCallbackOwnerGuard<BnwNativeDefaultNamespacePolicyOwner>;
    Guard guard() const noexcept { return Guard(state, self); }
    std::shared_ptr<const BnwNativeColdFilesystemOwner> validate(const Guard& fence) const {
        need(fence.valid(), "Default namespace initial policy lost/reentered/foreign thread");
        auto parent = cold.lock();
        need(bool(parent), "Default namespace constructor Cold expired");
        const auto check = [&] {
            need(fence.valid() && fence.valid_parent(parent),
                 "Default namespace initial policy/Cold persistent facade lost");
        };
        check();
        if(installation_input){need(!lease,"Installed initial policy cannot carry imported save lease");
            parent->require_same_input(*installation_input);check();parent->require_fresh_direct_original_branch();check();
            const auto& actual=parent->installation_input_for_source();check();
            need(actual.get()==installation_input.get(),"Installed constructor policy has another SAME input");
            installation_input->require_current();check();
        }else{
        parent->require_same_input(*lease); check();
        parent->require_fresh_direct_original_branch(); check();
        parent->require_initial_policy_lease_identity_without_callbacks(*lease); check();
        lease->require_current(); check();
        }
        return parent;
    }
    std::shared_ptr<const BnwNativeColdFilesystemOwner> validate_common(const Guard& fence,BnwNativeCommonSourceValidationContext& context,bool validate_sources) const {
        need(lease&&!installation_input,"Mandatory installed common policy source before late use");
        need(fence.valid(), "Default namespace initial policy lost/reentered/foreign thread");
        auto parent = cold.lock();
        need(bool(parent), "Default namespace constructor Cold expired");
        const auto check = [&] {
            need(fence.valid() && fence.valid_parent(parent),
                 "Default namespace initial policy/Cold persistent facade lost");
        };
        check();
        if(validate_sources)parent->require_same_input_common_source(*lease,context);
        else context.require_borrowed(*parent,parent);
        check();
        parent->require_fresh_direct_original_branch_common_source(context); check();
        const auto& actual_lease = parent->early_input_lease_common_source(context); check();
        need(actual_lease.get() == lease.get(), "Default namespace constructor has another lease");
        lease->require_common_source_current(context); check();
        return parent;
    }
    std::shared_ptr<const BnwNativeColdFilesystemOwner> validate_common(const Guard& fence,BnwNativeCommonSourceValidationContext& context) const {return validate_common(fence,context,true);}
    void validate_completed(const Guard& fence,const BnwNativeCompletedColdSourceLoan& loan,
        const BnwNativeColdFilesystemOwner& actual_cold)const {
        need(lease&&!installation_input,"Mandatory installed completed policy source before late use");
        const auto check=[&]{need(fence.valid(),"Completed Default initial policy lost/reentered/foreign thread");};
        check();actual_cold.require_same_completed_owner_control(cold,loan);check();
        loan.require_same_early_input(*lease);check();
        // The same real Cold loan validates the original null callback branch,
        // ACP/roots/profile and native source epoch. No public Early callback.
        actual_cold.require_same_completed_owner_control(cold,loan);check();
    }
};
BnwNativeDefaultNamespacePolicyOwner::BnwNativeDefaultNamespacePolicyOwner(std::unique_ptr<Impl> i)
    : impl_(std::move(i)) {}
BnwNativeDefaultNamespacePolicyOwner::~BnwNativeDefaultNamespacePolicyOwner() = default;
std::shared_ptr<const BnwNativeDefaultNamespacePolicyOwner>
BnwNativeDefaultNamespacePolicyOwner::acquire(
    const std::shared_ptr<const BnwNativeColdFilesystemOwner>& cold) {
    need(bool(cold), "Default namespace constructor requires genuine Cold");
    auto i = std::make_unique<Impl>();
    // Arena consumes its one attempt BEFORE reaching this factory. It must own
    // exactly ONE Cold input pin; no extra local Cold locks exist in this scope.
    {
        BnwNativeCallbackConstructionGuard fence(i->state);
        const auto check = [&] {
            need(fence.valid() && cold && cold.use_count() > 1,
                 "Default namespace constructor Cold retained only by Arena factory/retired");
        };
        check();
        cold->require_current(); check();
        i->cold = cold;
        i->installation_input=cold->installation_input_for_source();check();
        if(i->installation_input){cold->require_same_input(*i->installation_input);check();
            cold->require_fresh_direct_original_branch();check();i->installation_input->require_current();check();
            need(cold->installation_input_for_source().get()==i->installation_input.get(),"Installed initial policy source changed");check();
        }else{
        i->lease = cold->early_input_lease(); check();
        cold->require_same_input(*i->lease); check();
        cold->require_fresh_direct_original_branch(); check();
        i->lease->require_current(); check();
        const auto& actual_lease = cold->early_input_lease(); check();
        need(actual_lease.get() == i->lease.get(), "Default namespace initial source lease changed");
        }
    } // Destroy unique-Impl fence before throwing shared control-block allocation.
    auto out = std::shared_ptr<const BnwNativeDefaultNamespacePolicyOwner>(
        new BnwNativeDefaultNamespacePolicyOwner(std::move(i)));
    out->impl_->self = out;
    return out;
}
void BnwNativeDefaultNamespacePolicyOwner::require_common_source_current(BnwNativeCommonSourceValidationContext& context)const {
    auto fence=impl_->guard();
    need(fence.valid(),"Common initial policy lost/reentered/foreign thread");
    auto visit=context.enter(*this,impl_->self);
    impl_->validate_common(fence,context,visit.work()==BnwNativeCommonSourceValidationContext::Work::validate_source);
    need(fence.valid(),"Common initial policy changed");visit.complete();
}
void BnwNativeDefaultNamespacePolicyOwner::require_current() const {
    auto fence = impl_->guard(); impl_->validate(fence);
}
void BnwNativeDefaultNamespacePolicyOwner::require_same_input(const BnwNativeColdFilesystemOwner& cold) const {
    auto fence = impl_->guard(); const auto actual = impl_->validate(fence);
    need(actual.get() == &cold, "Default namespace policy has another Cold identity");
}
BnwNativeDefaultNamespaceInitialReceiptDiagnostic
BnwNativeDefaultNamespacePolicyOwner::initial_receipt() const {
    auto fence = impl_->guard(); impl_->validate(fence);
    return {impl_->initial.constructor_archive_priority(), impl_->initial.constructor_enabled(),
            impl_->initial.constructor_prefix_filters().size()};
}

void BnwNativeDefaultNamespacePolicyOwner::require_current_completed(const BnwNativeCompletedColdSourceLoan& loan,
    const BnwNativeColdFilesystemOwner& cold)const {auto fence=impl_->guard();impl_->validate_completed(fence,loan,cold);}
void BnwNativeDefaultNamespacePolicyOwner::require_same_input_completed(const BnwNativeColdFilesystemOwner& cold,
    const BnwNativeCompletedColdSourceLoan& loan)const {require_current_completed(loan,cold);}
BnwNativeDefaultNamespaceInitialReceiptDiagnostic BnwNativeDefaultNamespacePolicyOwner::initial_receipt_completed(
    const BnwNativeCompletedColdSourceLoan& loan,const BnwNativeColdFilesystemOwner& cold)const {
    auto fence=impl_->guard();impl_->validate_completed(fence,loan,cold);
    return {impl_->initial.constructor_archive_priority(),impl_->initial.constructor_enabled(),
            impl_->initial.constructor_prefix_filters().size()};
}
}
