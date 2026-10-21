#pragma once
#include "bnw_native_color_load_kernel.hpp"
#include <cstddef>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
namespace civ5::ui {class BnwNativePackageDiscoveryOwner;}
namespace civ5::ui::detail {
// Ordinary native Color lookup only. No registry DTO, parsing fallback, Font,
// stream, window, UI publication or full startup readiness is issued here.
class BnwNativeCompletedColorQuery final {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
  public:
    using ColorBits=::civ5::ui::native_color_load_detail::ColorBits;
  private:
    class Source {
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
      protected:
        Source()=default;
      public:
        virtual ~Source()=default;
        Source(const Source&)=delete;
        Source& operator=(const Source&)=delete;
        virtual void require_actual_owner_current(const BnwNativeCompletedColorQuery&)const=0;
        // Authenticate this issued leaf against the consumer's actual Parent.
        // Source compares retained Parent address before touching the supplied
        // Parent or invoking any currentness/client observer callbacks.
        virtual void require_same_parent(const BnwNativeCompletedColorQuery&,
            const ::civ5::ui::BnwNativePackageDiscoveryOwner&)const=0;
        // Only actual Parent Source may open the real new Header/Loaded/Bank
        // operation and call the separate completed ordered reader. nullopt
        // means genuine full ordinary lookup MISS, never unavailable input.
        virtual std::optional<ColorBits> query_actual_color(
            const BnwNativeCompletedColorQuery&,std::uint32_t)const=0;
    };
    const std::thread::id thread_=std::this_thread::get_id();
    const std::shared_ptr<const Source> source_;
    std::weak_ptr<const BnwNativeCompletedColorQuery> self_;
    mutable std::size_t self_pins_=0;
    mutable bool checking_=false,query_active_=false,relation_active_=false,failed_=false;
    static void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
    explicit BnwNativeCompletedColorQuery(std::shared_ptr<const Source> source):source_(std::move(source)) {
        need(bool(source_),"Completed Color query actual Parent Source absent");
        // Parent assigns self_ from the genuine shared owner, registers its
        // actual Source identity, then validates before publishing this owner.
    }
    class Pin final {
        const BnwNativeCompletedColorQuery& owner_;
        const std::shared_ptr<const BnwNativeCompletedColorQuery> actual_;
      public:
        explicit Pin(const BnwNativeCompletedColorQuery& owner):owner_(owner),actual_(owner.self_.lock()) {
            need(actual_&&actual_.get()==&owner_&&
                 !actual_.owner_before(owner_.self_)&&!owner_.self_.owner_before(actual_)&&
                 owner_.self_pins_<static_cast<std::size_t>(std::numeric_limits<long>::max()),
                 "Completed Color genuine control/pin overflow");
            ++owner_.self_pins_;
        }
        ~Pin(){--owner_.self_pins_;}
        void current()const {
            need(!owner_.failed_&&actual_.use_count()>static_cast<long>(owner_.self_pins_),
                 "Completed Color outside owner retired");
            if(owner_.checking_){owner_.failed_=true;throw std::runtime_error("Completed Color Source currentness reentry");}
            owner_.checking_=true;
            struct Exit {const BnwNativeCompletedColorQuery& owner;~Exit(){owner.checking_=false;}} exit{owner_};
            try {
                const auto source=owner_.source_;
                source->require_actual_owner_current(owner_);
                need(!owner_.failed_&&actual_.use_count()>static_cast<long>(owner_.self_pins_),
                     "Completed Color outside owner retired during Source check");
            }catch(...){owner_.failed_=true;throw;}
        }
        const std::shared_ptr<const BnwNativeCompletedColorQuery>& canonical_owner()const noexcept{return actual_;}
    };
  public:
    ~BnwNativeCompletedColorQuery()=default;
    BnwNativeCompletedColorQuery(const BnwNativeCompletedColorQuery&)=delete;
    BnwNativeCompletedColorQuery& operator=(const BnwNativeCompletedColorQuery&)=delete;
    void require_identity_current()const {
        need(thread_==std::this_thread::get_id(),"Completed Color identity foreign creator thread");
        Pin retained(*this);retained.current();
    }
    void require_same_parent(const ::civ5::ui::BnwNativePackageDiscoveryOwner& parent)const {
        need(thread_==std::this_thread::get_id(),"Completed Color Parent relation foreign creator thread");
        Pin retained(*this);
        if(relation_active_){failed_=true;throw std::runtime_error("Completed Color Parent relation reentry");}
        relation_active_=true;
        struct Exit {const BnwNativeCompletedColorQuery& owner;~Exit(){owner.relation_active_=false;}} exit{*this};
        try {
            retained.current();
            const auto source=source_;
            source->require_same_parent(*this,parent);
            retained.current();
        }catch(...){failed_=true;throw;}
    }
    // Durable consumers must keep this canonical factory control block,
    // rather than an arbitrary alias shared_ptr supplied by another caller.
    std::shared_ptr<const BnwNativeCompletedColorQuery> retain_owner()const {
        need(thread_==std::this_thread::get_id(),"Completed Color retain foreign creator thread");
        Pin retained(*this);retained.current();
        return retained.canonical_owner(); // Plain copy after the final check.
    }
    std::optional<ColorBits> query(std::uint32_t key)const {
        need(thread_==std::this_thread::get_id(),"Completed Color lookup foreign creator thread");
        Pin retained(*this);
        if(query_active_){failed_=true;throw std::runtime_error("Completed Color lookup reentry");}
        query_active_=true;
        struct Exit {const BnwNativeCompletedColorQuery& owner;~Exit(){owner.query_active_=false;}} exit{*this};
        try {
            retained.current();
            need(key!=0x8f4455c8U&&key!=0x164d0472U,"Completed Color special player-interface source absent");
            const auto source=source_;
            const auto result=source->query_actual_color(*this,key);
            retained.current();
            return result; // Raw known native bits or the real completed MISS.
        }catch(...){failed_=true;throw;}
    }
};
}
