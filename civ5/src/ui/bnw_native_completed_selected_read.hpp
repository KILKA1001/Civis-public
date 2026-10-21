#pragma once
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
// Closed selected BASENAME bytes lender. The actual Parent Source must select
// and read the genuine Original source inside the completed native operation.
// A borrowed span grants no persistent stream, handle, table or startup readiness.
class BnwNativeCompletedSelectedRead final {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
  public:
  private:
    class TargetRetirement;
    class Source {
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
      protected:
        Source()=default;
      public:
        virtual ~Source()=default;
        Source(const Source&)=delete;
        Source& operator=(const Source&)=delete;
        virtual void require_actual_owner_current(const BnwNativeCompletedSelectedRead&)const=0;
        // Authenticate this issued leaf against the consumer's actual Parent.
        // Source compares retained Parent address before touching the supplied
        // Parent or invoking any currentness/client observer callbacks.
        virtual void require_same_parent(const BnwNativeCompletedSelectedRead&,
            const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const=0;
        // Source opens61/63+66/69, resolves actual node POINTER identity and
        // reads the genuine bounded Original source. A real selected MISS
        // throws; no direct-filesystem fallback is inferred. Invoke callback
        // once while the actual read loan/selected source is still current.
        virtual void read_actual_selected_bytes(
            const BnwNativeCompletedSelectedRead&,std::string_view,std::size_t,
            const std::function<void(std::span<const std::uint8_t>)>&,
            const TargetRetirement&)const=0;
    };
    const std::thread::id thread_=std::this_thread::get_id();
    const std::shared_ptr<const Source> source_;
    std::weak_ptr<const BnwNativeCompletedSelectedRead> self_;
    mutable std::size_t self_pins_=0;
    mutable bool checking_=false,read_active_=false,relation_active_=false,failed_=false;
    mutable const TargetRetirement* active_retirement_=nullptr;
    mutable std::uint64_t read_serial_=0;
    static void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
    explicit BnwNativeCompletedSelectedRead(std::shared_ptr<const Source> source):source_(std::move(source)) {
        need(bool(source_),"Completed selected read query actual Parent Source absent");
        // Parent assigns self_ from the genuine shared owner, registers its
        // actual Source identity, then validates before publishing this owner.
    }
    class Pin final {
        const BnwNativeCompletedSelectedRead& owner_;
        const std::shared_ptr<const BnwNativeCompletedSelectedRead> actual_;
      public:
        explicit Pin(const BnwNativeCompletedSelectedRead& owner):owner_(owner),actual_(owner.self_.lock()) {
            need(actual_&&actual_.get()==&owner_&&
                 !actual_.owner_before(owner_.self_)&&!owner_.self_.owner_before(actual_)&&
                 owner_.self_pins_<static_cast<std::size_t>(std::numeric_limits<long>::max()),
                 "Completed selected read genuine control/pin overflow");
            ++owner_.self_pins_;
        }
        ~Pin(){--owner_.self_pins_;}
        void current()const {
            need(!owner_.failed_&&actual_.use_count()>static_cast<long>(owner_.self_pins_),
                 "Completed selected read outside owner retired");
            if(owner_.checking_){owner_.failed_=true;throw std::runtime_error("Completed selected read Source currentness reentry");}
            owner_.checking_=true;
            struct Exit {const BnwNativeCompletedSelectedRead& owner;~Exit(){owner.checking_=false;}} exit{owner_};
            try {
                const auto source=owner_.source_;
                source->require_actual_owner_current(owner_);
                need(!owner_.failed_&&actual_.use_count()>static_cast<long>(owner_.self_pins_),
                     "Completed selected read outside owner retired during Source check");
            }catch(...){owner_.failed_=true;throw;}
        }
        const std::shared_ptr<const BnwNativeCompletedSelectedRead>& canonical_owner()const noexcept{return actual_;}
    };

    // Stack-only teardown authority for the actual copied bytes consumer.
    // Only this leaf can mint it; only the sealed Parent Source may use it,
    // after authenticating a genuine failed-stat selected HIT. It cannot lend
    // bytes, publish a winner, suppress a failure or survive this read scope.
    class TargetRetirement final {
        friend class BnwNativeCompletedSelectedRead;
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
        const std::thread::id thread_=std::this_thread::get_id();
        const BnwNativeCompletedSelectedRead& owner_;
        const Source& source_;
        const Pin& retained_;
        std::optional<std::function<void(std::span<const std::uint8_t>)>>& target_;
            const std::string& query_;
        const std::size_t max_bytes_;
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
        TargetRetirement(const BnwNativeCompletedSelectedRead& owner,const Source& source,const Pin& retained,
            std::optional<std::function<void(std::span<const std::uint8_t>)>>& target,
            const std::string& query,std::size_t max_bytes):
            owner_(owner),source_(source),retained_(retained),target_(target),query_(query),max_bytes_(max_bytes),
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
        void retire_expected_unreadable(const BnwNativeCompletedSelectedRead& actual,const Source& source,
            std::string_view query,std::size_t max_bytes)const {
            need(thread_==std::this_thread::get_id(),"Completed target retirement foreign creator thread");
            try {
                require_stack();
                need(&actual==&owner_&&&source==&source_&&query==query_&&max_bytes==max_bytes_&&!retired_&&target_.has_value(),
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
    ~BnwNativeCompletedSelectedRead()=default;
    BnwNativeCompletedSelectedRead(const BnwNativeCompletedSelectedRead&)=delete;
    BnwNativeCompletedSelectedRead& operator=(const BnwNativeCompletedSelectedRead&)=delete;
    void require_identity_current()const {
        need(thread_==std::this_thread::get_id(),"Completed selected read identity foreign creator thread");
        Pin retained(*this);retained.current();
    }
    void require_same_parent(const ::civ5::ui::BnwNativePackageDiscoveryOwner& parent)const {
        need(thread_==std::this_thread::get_id(),"Completed selected read Parent relation foreign creator thread");
        Pin retained(*this);
        if(relation_active_){failed_=true;throw std::runtime_error("Completed selected read Parent relation reentry");}
        relation_active_=true;
        struct Exit {const BnwNativeCompletedSelectedRead& owner;~Exit(){owner.relation_active_=false;}} exit{*this};
        try {
            retained.current();
            const auto source=source_;
            source->require_same_parent(*this,parent);
            retained.current();
        }catch(...){failed_=true;throw;}
    }
    // Durable consumers must keep this canonical factory control block,
    // rather than an arbitrary alias shared_ptr supplied by another caller.
    std::shared_ptr<const BnwNativeCompletedSelectedRead> retain_owner()const {
        need(thread_==std::this_thread::get_id(),"Completed selected read retain foreign creator thread");
        Pin retained(*this);retained.current();
        return retained.canonical_owner(); // Plain copy after the final check.
    }
    void with_selected_basename_bytes(std::string_view basename,std::size_t max_bytes,
        const std::function<void(std::span<const std::uint8_t>)>& input)const {
        need(thread_==std::this_thread::get_id(),"Completed selected read foreign creator thread");
        Pin retained(*this);
        if(read_active_){failed_=true;throw std::runtime_error("Completed selected read reentry");}
        read_active_=true;
        struct Exit {const BnwNativeCompletedSelectedRead& owner;~Exit(){owner.read_active_=false;}} exit{*this};
        try {
            retained.current();
            need(!basename.empty()&&basename.size()<=259&&max_bytes<=8u*1024u*1024u,
                 "Completed selected read basename/byte bound unsupported");
            for(const unsigned char byte:basename)
                need(byte!=0&&byte<128&&byte!='/'&&byte!='\\',
                     "Completed selected read requires ordinary ASCII basename");
            const std::string query(basename);
            retained.current();
            std::optional<std::function<void(std::span<const std::uint8_t>)>> target(input);
            retained.current();
            need(bool(*target),"Completed selected read consumer absent");
            const auto source=source_;
            std::size_t callbacks=0;
            const TargetRetirement retirement(*this,*source,retained,target,query,max_bytes);
            source->read_actual_selected_bytes(*this,query,max_bytes,[&](std::span<const std::uint8_t> bytes) {
                retained.current();
                need(callbacks++==0&&target.has_value()&&bytes.size()<=max_bytes,
                     "Completed selected read must lend one bounded genuine source");
                (*target)(bytes);
                retained.current();
                // Destroy the actual copied consumer INSIDE the genuine Source
                // lender, before read/selected-universe postchecks and retirement.
                target.reset();
                retained.current();
            },retirement);
            need(callbacks==1&&!target,"Completed selected read omitted actual bytes callback");
            retained.current();
        }catch(...){failed_=true;throw;}
    }
};
}
