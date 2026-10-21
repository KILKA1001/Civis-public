#pragma once
// Internal lifetime fence only: never a native Config/archive stage capability.
#include <memory>
#include <cstddef>
#include <stdexcept>
#include <thread>
namespace civ5::ui::detail {
class BnwNativeCallbackOwnerState final {
    template<class> friend class BnwNativeCallbackOwnerGuard;
    friend class BnwNativeCallbackConstructionGuard;
    const std::thread::id thread_ = std::this_thread::get_id();
    mutable bool busy_ = false, poisoned_ = false;
    //Actual successful OwnerGuard self.lock lifetime, independent of native
    //phase flags and ConstructionGuard busy state. One entered Guard only.
    mutable bool owner_pin_present_ = false;
    bool enter() const noexcept {
        // Do not inspect/write non-atomic flags on a foreign thread.
        if (thread_ != std::this_thread::get_id()) return false;
        if (busy_) { poisoned_ = true; return false; }
        if (poisoned_) return false;
        busy_ = true;
        return true;
    }
    bool valid() const noexcept {
        return thread_ == std::this_thread::get_id() && busy_ && !poisoned_;
    }
  public:
    BnwNativeCallbackOwnerState() = default;
    //Bookkeeping only. It neither issues a fence nor grants source/currentness.
    std::size_t internal_owner_pin_count()const {
        if(thread_!=std::this_thread::get_id())
            throw std::runtime_error("Callback owner pin observation foreign creator thread");
        return owner_pin_present_?1u:0u;
    }
    BnwNativeCallbackOwnerState(const BnwNativeCallbackOwnerState&) = delete;
    BnwNativeCallbackOwnerState& operator=(const BnwNativeCallbackOwnerState&) = delete;
};
// Holds exactly one internal pin. Production owners' Impl dependencies have no
// strong backedge to self; any additional strong reference is an external owner.
// Keep this scope noncopyable/nonmovable: extra pins mask last-owner loss.
template<class Owner> class BnwNativeCallbackOwnerGuard final {
    const BnwNativeCallbackOwnerState& state_;
    bool entered_ = false;
    std::shared_ptr<const Owner> pin_;
  public:
    BnwNativeCallbackOwnerGuard(const BnwNativeCallbackOwnerState& state,
                               const std::weak_ptr<const Owner>& self) noexcept : state_(state) {
        entered_ = state_.enter();
        if (entered_) {
            pin_ = self.lock();
            if (!pin_) state_.poisoned_ = true;
            else state_.owner_pin_present_ = true;
        }
    }
    BnwNativeCallbackOwnerGuard(const BnwNativeCallbackOwnerGuard&) = delete;
    BnwNativeCallbackOwnerGuard& operator=(const BnwNativeCallbackOwnerGuard&) = delete;
    BnwNativeCallbackOwnerGuard(BnwNativeCallbackOwnerGuard&&) = delete;
    BnwNativeCallbackOwnerGuard& operator=(BnwNativeCallbackOwnerGuard&&) = delete;
    bool valid() const noexcept {
        if (!entered_ || !state_.valid()) return false;
        if (!pin_ || pin_.use_count() == 1) { state_.poisoned_ = true; return false; }
        return true;
    }
    // parent is ONE transient pin, checked only after nested callback scopes
    // have released their own pins. Another persistent genuine owner must
    // remain; this is lifetime validation, never a native stage permission.
    template<class Parent> bool valid_parent(const std::shared_ptr<const Parent>& parent) const noexcept {
        if (!valid()) return false;
        if (!parent || parent.use_count() == 1) { state_.poisoned_ = true; return false; }
        return true;
    }
    ~BnwNativeCallbackOwnerGuard() {
        if (entered_) {
            // Covers external-owner loss during an exception, too. Only the
            // creating thread can enter, so this flag access is same-thread.
            if (!pin_ || pin_.use_count() == 1) state_.poisoned_ = true;
            //Clear actual bookkeeping BEFORE the retained pin releases. A
            //rejected nested/foreign guard has entered_=false and cannot clear
            //another live guard's count. ConstructionGuard never sets it.
            state_.owner_pin_present_ = false;
            state_.busy_ = false;
        }
        // pin_ releases AFTER the final state access, possibly destroying Impl.
    }
};
// A factory has a unique, unpublished Impl and retained genuine dependencies.
// End this scope BEFORE transferring Impl into the published shared owner.
class BnwNativeCallbackConstructionGuard final {
    const BnwNativeCallbackOwnerState& state_;
    bool entered_;
  public:
    explicit BnwNativeCallbackConstructionGuard(const BnwNativeCallbackOwnerState& state) noexcept
        : state_(state), entered_(state_.enter()) {}
    BnwNativeCallbackConstructionGuard(const BnwNativeCallbackConstructionGuard&) = delete;
    BnwNativeCallbackConstructionGuard& operator=(const BnwNativeCallbackConstructionGuard&) = delete;
    bool valid() const noexcept { return entered_ && state_.valid(); }
    template<class Parent> bool valid_parent(const std::shared_ptr<const Parent>& parent) const noexcept {
        if (!valid()) return false;
        // A factory's genuine input held by value is a persistent owner for
        // this unpublished scope, distinct from its local validation pin.
        if (!parent || parent.use_count() == 1) { state_.poisoned_ = true; return false; }
        return true;
    }
    ~BnwNativeCallbackConstructionGuard() { if (entered_) state_.busy_ = false; }
};
}
