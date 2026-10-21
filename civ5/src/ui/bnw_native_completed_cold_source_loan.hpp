#pragma once
#include "bnw_native_completed_parent_observation.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
namespace civ5::ui {class BnwNativeEarlyInputLease;class BnwNativePinnedInputEpoch;}
namespace civ5::ui::detail {
class BnwNativeColdFilesystemOwner;
class BnwNativeColdArchiveRequest;
class BnwNativeCommon13ArchiveChannel;
// Private synchronous completed file-source loan only. Its real Parent Source
// authenticates the normal-return receipt and original Cold/request journal.
// Neither an epoch reference nor an arbitrary subclass can construct it.
class BnwNativeCompletedColdSourceLoan final {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    friend class BnwNativeColdFilesystemOwner;
    friend class BnwNativeColdArchiveRequest;
    friend class BnwNativeCommon13ArchiveChannel;
    class Source {
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
        Source()=default;
      public:
        virtual ~Source()=default;
        Source(const Source&)=delete;Source&operator=(const Source&)=delete;
        // Pure SAME operation/receipt identity checks. They must not call
        // Cold.current, Parent.current, Common replay or application callbacks.
        virtual void require_actual_completed_cold_current(const BnwNativeColdFilesystemOwner&,
            const BnwNativeCompletedParentObservation::Loan&)const=0;
        virtual void require_actual_completed_request_current(const BnwNativeColdArchiveRequest&,
            const BnwNativeColdFilesystemOwner&,const BnwNativeCompletedParentObservation::Loan&)const=0;
        // Pure terminal SAME Common channel/ordinal/brand/journal identity.
        // Must not invoke Observer.current/Cold.current or full Common replay.
        virtual void require_actual_completed_common_channel_current(const BnwNativeCommon13ArchiveChannel&,
            const BnwNativeCompletedParentObservation::Loan&)const=0;
    };
    const std::thread::id thread_=std::this_thread::get_id();
    const std::shared_ptr<const BnwNativeColdFilesystemOwner>& cold_;
    const BnwNativeCompletedParentObservation::Loan& observation_;
    const ::civ5::ui::BnwNativePinnedInputEpoch& epoch_;
    const Source& source_;
    const std::uint64_t operation_;
    mutable bool active_=true,checking_=false;
    BnwNativeCompletedColdSourceLoan(const std::shared_ptr<const BnwNativeColdFilesystemOwner>&,
        const BnwNativeCompletedParentObservation::Loan&,const ::civ5::ui::BnwNativePinnedInputEpoch&,
        const Source&,std::uint64_t);
    ~BnwNativeCompletedColdSourceLoan(){active_=false;}
    void require_same_completed_common_channel(const BnwNativeCommon13ArchiveChannel&,
        const BnwNativeColdFilesystemOwner&)const;
  public:
    BnwNativeCompletedColdSourceLoan(const BnwNativeCompletedColdSourceLoan&)=delete;
    BnwNativeCompletedColdSourceLoan&operator=(const BnwNativeCompletedColdSourceLoan&)=delete;
    BnwNativeCompletedColdSourceLoan(BnwNativeCompletedColdSourceLoan&&)=delete;
    BnwNativeCompletedColdSourceLoan&operator=(BnwNativeCompletedColdSourceLoan&&)=delete;
    void require_current()const;
    void require_same_cold(const BnwNativeColdFilesystemOwner&)const;
    void require_same_early_input(const ::civ5::ui::BnwNativeEarlyInputLease&)const;
    // Explicit accounting for the genuine synchronous Cold pin. Descendants
    // must exclude it in their private completed outside-parent checks.
    std::size_t internal_cold_pin_count()const;
};
} // namespace civ5::ui::detail
