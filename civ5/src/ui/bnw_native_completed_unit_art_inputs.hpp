#pragma once
#include "civ5/runtime/save_preflight.hpp"
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
namespace civ5::ui {class BnwNativePackageDiscoveryOwner;}
namespace civ5::ui::detail {
// Closed three-document unit-art input. Only the actual Parent may issue the
// Source. Its real native winner receipts and completed Epoch authenticate each
// document; bytes/hash alone are not persistent source or Host authority.
class BnwNativeCompletedUnitArtInputs final {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
  public:
  private:
    class TargetRetirement;
    class Source {
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
        Source()=default;
      public:
        virtual ~Source()=default;
        Source(const Source&)=delete;
        Source& operator=(const Source&)=delete;
        virtual void require_actual_owner_current(const BnwNativeCompletedUnitArtInputs&)const=0;
        // Authenticate this issued leaf against the consumer's actual Parent.
        // Source compares retained Parent address before touching the supplied
        // Parent or invoking any currentness/client observer callbacks.
        virtual void require_same_parent(const BnwNativeCompletedUnitArtInputs&,
            const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const=0;
        virtual void require_same_profile(const BnwNativeCompletedUnitArtInputs&,
            const ::civ5::runtime::SavedContentProfile&)const=0;
        // Fixed ordinals: UnitMembers, Units, UnitMemberTemplates. Parent's
        // sealed supplier performs the actual native winner/read and records
        // genuine source ancestry on the first normal successful return. Every
        // subsequent read requires the SAME winner/control and actual byte
        // fingerprint inside a fresh completed operation. MISS/oversize fail.
        // The supplied callback's COPY/invoke/DESTROY must finish inside that
        // actual stream/Cold/Epoch loan, before its source-universe postchecks.
        virtual void read_actual_document_bytes(
            const BnwNativeCompletedUnitArtInputs&,unsigned,
            const std::function<void(std::span<const std::uint8_t>)>&,
            const TargetRetirement&)const=0;
    };
    const std::thread::id thread_=std::this_thread::get_id();
    const std::shared_ptr<const Source> source_;
    std::weak_ptr<const BnwNativeCompletedUnitArtInputs> self_;
    mutable std::size_t self_pins_=0;
    mutable bool checking_=false,read_active_=false,relation_active_=false,failed_=false;
    mutable const TargetRetirement* active_retirement_=nullptr;
    mutable std::uint64_t read_serial_=0;
    static void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
    explicit BnwNativeCompletedUnitArtInputs(std::shared_ptr<const Source> source):source_(std::move(source)) {
        need(bool(source_),"Completed unit-art input query actual Parent Source absent");
        // Parent assigns self_ from the genuine shared owner, registers its
        // actual Source identity, then validates before publishing this owner.
    }
    class Pin final {
        const BnwNativeCompletedUnitArtInputs& owner_;
        const std::shared_ptr<const BnwNativeCompletedUnitArtInputs> actual_;
      public:
        explicit Pin(const BnwNativeCompletedUnitArtInputs& owner):owner_(owner),actual_(owner.self_.lock()) {
            need(actual_&&actual_.get()==&owner_&&
                 !actual_.owner_before(owner_.self_)&&!owner_.self_.owner_before(actual_)&&
                 owner_.self_pins_<static_cast<std::size_t>(std::numeric_limits<long>::max()),
                 "Completed unit-art input genuine control/pin overflow");
            ++owner_.self_pins_;
        }
        ~Pin(){--owner_.self_pins_;}
        void storage_current()const {
            need(!owner_.failed_&&actual_.get()==&owner_&&
                 !actual_.owner_before(owner_.self_)&&!owner_.self_.owner_before(actual_)&&
                 actual_.use_count()>static_cast<long>(owner_.self_pins_),
                 "Completed unit-art input outside canonical owner retired");
        }
        void current()const {
            storage_current();
            if(owner_.checking_){owner_.failed_=true;throw std::runtime_error("Completed unit-art input Source currentness reentry");}
            owner_.checking_=true;
            struct Exit {const BnwNativeCompletedUnitArtInputs& owner;~Exit(){owner.checking_=false;}} exit{owner_};
            try {
                const auto source=owner_.source_;
                source->require_actual_owner_current(owner_);
                need(!owner_.failed_&&actual_.use_count()>static_cast<long>(owner_.self_pins_),
                     "Completed unit-art input outside owner retired during Source check");
            }catch(...){owner_.failed_=true;throw;}
        }
        const std::shared_ptr<const BnwNativeCompletedUnitArtInputs>& canonical_owner()const noexcept{return actual_;}
    };

    // Stack-only teardown authority for the actual copied bytes consumer.
    // Only this leaf can mint it; only the sealed Parent Source may use it,
    // after authenticating a genuine failed-stat selected HIT. It cannot lend
    // bytes, publish a winner, suppress a failure or survive this read scope.
    class TargetRetirement final {
        friend class BnwNativeCompletedUnitArtInputs;
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
        const std::thread::id thread_=std::this_thread::get_id();
        const BnwNativeCompletedUnitArtInputs& owner_;
        const Source& source_;
        const Pin& retained_;
        std::optional<std::function<void(std::span<const std::uint8_t>)>>& target_;
            const unsigned ordinal_;
        const std::uint64_t serial_;
        mutable bool retired_=false;
        void require_stack()const {
            need(thread_==std::this_thread::get_id(),"Completed target retirement foreign creator thread");
            const auto& canonical=retained_.canonical_owner(); // Counted existing Pin; no extra strong copy.
            need(!owner_.failed_&&owner_.read_active_&&owner_.active_retirement_==this&&
                 owner_.read_serial_==serial_&&owner_.source_.get()==&source_&&
                 canonical.get()==&owner_&&!canonical.owner_before(owner_.self_)&&
                 !owner_.self_.owner_before(canonical)&&
                 canonical.use_count()>static_cast<long>(owner_.self_pins_),
                 "Completed target retirement lost SAME actual leaf/read/source/control");
        }
        TargetRetirement(const BnwNativeCompletedUnitArtInputs& owner,const Source& source,const Pin& retained,
            std::optional<std::function<void(std::span<const std::uint8_t>)>>& target,
            unsigned ordinal):
            owner_(owner),source_(source),retained_(retained),target_(target),ordinal_(ordinal),
            serial_(owner.read_serial_+1) {
            need(thread_==owner_.thread_&&owner_.read_active_&&!owner_.active_retirement_&&
                 owner_.read_serial_!=std::numeric_limits<std::uint64_t>::max()&&target_.has_value(),
                 "Completed target retirement requires fresh genuine active read");
            owner_.read_serial_=serial_;owner_.active_retirement_=this;
        }
      public:
        TargetRetirement(const TargetRetirement&)=delete;
        TargetRetirement& operator=(const TargetRetirement&)=delete;
        ~TargetRetirement(){
            if(owner_.active_retirement_==this)owner_.active_retirement_=nullptr;
            else owner_.failed_=true;
        }
        void retire_expected_unreadable(const BnwNativeCompletedUnitArtInputs& actual,const Source& source,
            unsigned ordinal)const {
            need(thread_==std::this_thread::get_id(),"Completed target retirement foreign creator thread");
            try {
                require_stack();
                need(&actual==&owner_&&&source==&source_&&ordinal==ordinal_&&!retired_&&target_.has_value(),
                     "Completed target retirement replay or another actual request");
                retained_.current();require_stack();
                need(!retired_&&target_.has_value(),"Completed target retirement changed during source check");
                retired_=true; // Reentry cannot perform a second teardown.
                target_.reset(); // REAL copied user target, inside native catalog/Bank/Loaded/opening loans.
                retained_.current();require_stack();
            }catch(...) {
                owner_.failed_=true;
                target_.reset(); // Unexpected currentness failure still destroys inside the entered lender.
                throw;
            }
        }
    };
  public:
    ~BnwNativeCompletedUnitArtInputs()=default;
    BnwNativeCompletedUnitArtInputs(const BnwNativeCompletedUnitArtInputs&)=delete;
    BnwNativeCompletedUnitArtInputs& operator=(const BnwNativeCompletedUnitArtInputs&)=delete;
    void require_identity_current()const {
        need(thread_==std::this_thread::get_id(),"Completed unit-art input identity foreign creator thread");
        Pin retained(*this);retained.current();
    }
    void require_same_parent(const ::civ5::ui::BnwNativePackageDiscoveryOwner& parent)const {
        need(thread_==std::this_thread::get_id(),"Completed unit-art input Parent relation foreign creator thread");
        Pin retained(*this);
        if(relation_active_){failed_=true;throw std::runtime_error("Completed unit-art input Parent relation reentry");}
        relation_active_=true;
        struct Exit {const BnwNativeCompletedUnitArtInputs& owner;~Exit(){owner.relation_active_=false;}} exit{*this};
        try {
            retained.current();
            const auto source=source_;
            source->require_same_parent(*this,parent);
            retained.current();
        }catch(...){failed_=true;throw;}
    }
    void require_same_profile(const ::civ5::runtime::SavedContentProfile& input)const {
        need(thread_==std::this_thread::get_id(),"Completed unit-art profile foreign creator thread");
        Pin retained(*this);retained.storage_current();
        if(relation_active_){failed_=true;throw std::runtime_error("Completed unit-art profile relation reentry");}
        relation_active_=true;
        struct Exit {const BnwNativeCompletedUnitArtInputs& owner;~Exit(){owner.relation_active_=false;}} exit{*this};
        try {
            // Own all borrowed profile storage before real Source callbacks.
            // It is still only a value: the actual Source must compare the
            // complete SAME Epoch-selected profile, not accept caller authority.
            const ::civ5::runtime::SavedContentProfile profile(input);
            retained.current();
            const auto source=source_;
            source->require_same_profile(*this,profile);
            retained.current();
        }catch(...){failed_=true;throw;}
    }
    // Durable consumers must keep this canonical factory control block,
    // rather than an arbitrary alias shared_ptr supplied by another caller.
    std::shared_ptr<const BnwNativeCompletedUnitArtInputs> retain_owner()const {
        need(thread_==std::this_thread::get_id(),"Completed unit-art input retain foreign creator thread");
        Pin retained(*this);retained.current();
        return retained.canonical_owner(); // Plain copy after the final check.
    }
    void with_document_bytes(unsigned ordinal,
        const std::function<void(std::span<const std::uint8_t>)>& input)const {
        need(thread_==std::this_thread::get_id(),"Completed unit-art input foreign creator thread");
        Pin retained(*this);retained.storage_current();
        if(read_active_){failed_=true;throw std::runtime_error("Completed unit-art document reentry");}
        read_active_=true;
        struct Exit {const BnwNativeCompletedUnitArtInputs& owner;~Exit(){owner.read_active_=false;}} exit{*this};
        try {
            need(ordinal<3,"Completed unit-art fixed document ordinal unsupported");
            // Callback-free canonical storage bookend precedes target COPY.
            // COPY may retire outside authority or reenter this real leaf.
            std::optional<std::function<void(std::span<const std::uint8_t>)>> target(input);
            retained.current();
            need(bool(*target),"Completed unit-art document consumer absent");
            const auto source=source_;
            std::size_t callbacks=0;
            const TargetRetirement retirement(*this,*source,retained,target,ordinal);
            source->read_actual_document_bytes(*this,ordinal,[&](std::span<const std::uint8_t> bytes) {
                try {
                    retained.current();
                    need(callbacks++==0&&target.has_value()&&bytes.size()<=8u*1024u*1024u,
                         "Completed unit-art input must lend one bounded genuine document");
                    (*target)(bytes);
                    retained.current();
                }catch(...) {
                    // Even an entered callback failure must destroy the copied
                    // target before the real stream/Cold/Epoch loan unwinds.
                    // Poison first so target destruction cannot revive this leaf.
                    failed_=true;
                    target.reset();
                    throw;
                }
                // Actual copied target destruction remains in the real Source
                // stream/Cold/Epoch callback, before all lower postchecks.
                target.reset();
                retained.current();
            },retirement);
            need(callbacks==1&&!target,"Completed unit-art Source omitted actual document callback");
            retained.current();
        }catch(...){failed_=true;throw;}
    }

};
}
