#pragma once
#include <cstddef>
#include <functional>
#include <limits>
#include <memory>
#include <stdexcept>
#include <thread>
#include <utility>
namespace civ5::ui {class BnwNativePackageDiscoveryOwner;}
namespace civ5::ui::detail {
class BnwNativeOwnedUiSkinPoststageInstance;
class BnwNativeGlobalColorRegistryBank;
// Separate completed-domain observer. It neither reactivates the original
// constructor/Skin loan nor issues resource queries or parser authority.
class BnwNativeCompletedSkinInstanceObservation final {
    friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
    using Instance=BnwNativeOwnedUiSkinPoststageInstance;
    class Source {
        friend class ::civ5::ui::BnwNativePackageDiscoveryOwner;
      protected:
        Source()=default;
      public:
        virtual ~Source()=default;
        Source(const Source&)=delete;
        Source& operator=(const Source&)=delete;
        // Parent implementation must authenticate this exact observer/instance
        // against the actual completed roster, Loaded/Parser occurrence and
        // current fresh Header/Loaded/Bank operation. A pointer alone is not
        // authority. The concrete Source retains only weak cap/Parent authority
        // so Bank -> observer -> Source cannot retain cap -> Impl -> Bank.
        virtual void require_actual_completed_instance(
            const BnwNativeCompletedSkinInstanceObservation&,const Instance&)const=0;
    };
    const std::thread::id thread_=std::this_thread::get_id();
    const std::shared_ptr<const Source> source_;
    const std::shared_ptr<const Instance> instance_;
    std::weak_ptr<const BnwNativeCompletedSkinInstanceObservation> self_;
    mutable std::size_t self_pins_=0;
    mutable bool checking_=false,loan_active_=false,failed_=false;
    static void need(bool value,const char* why){if(!value)throw std::runtime_error(why);}
    explicit BnwNativeCompletedSkinInstanceObservation(
        std::shared_ptr<const Source> source,std::shared_ptr<const Instance> instance)
        :source_(std::move(source)),instance_(std::move(instance)) {
        need(source_&&instance_,"Completed Skin observation actual source/instance absent");
        // Only Parent constructs, assigns self_ from the genuine shared owner,
        // then authenticates require_current before publishing the observer.
    }
    class Pin final {
        const BnwNativeCompletedSkinInstanceObservation& owner_;
        const std::shared_ptr<const BnwNativeCompletedSkinInstanceObservation> actual_;
      public:
        explicit Pin(const BnwNativeCompletedSkinInstanceObservation& owner)
            :owner_(owner),actual_(owner.self_.lock()) {
            need(actual_&&actual_.get()==&owner_&&
                 owner_.self_pins_<static_cast<std::size_t>(std::numeric_limits<long>::max()),
                 "Completed Skin observation genuine owner/pin overflow");
            ++owner_.self_pins_;
        }
        ~Pin(){--owner_.self_pins_;}
        void current()const {
            need(!owner_.failed_&&actual_.use_count()>static_cast<long>(owner_.self_pins_),
                 "Completed Skin observation retired outside owner");
            if(owner_.checking_){owner_.failed_=true;throw std::runtime_error("Completed Skin observation source reentry");}
            owner_.checking_=true;
            struct Exit {const BnwNativeCompletedSkinInstanceObservation& owner;~Exit(){owner.checking_=false;}} exit{owner_};
            try {
                const auto source=owner_.source_;
                const auto instance=owner_.instance_;
                source->require_actual_completed_instance(owner_,*instance);
                need(!owner_.failed_&&actual_.use_count()>static_cast<long>(owner_.self_pins_),
                     "Completed Skin observation retired during actual source check");
            }catch(...){owner_.failed_=true;throw;}
        }
    };
  public:
    void require_current()const {
        need(thread_==std::this_thread::get_id(),"Completed Skin observation foreign creator thread");
        Pin retained(*this);retained.current();
    }
    void with_actual_instance(const std::function<void(const Instance&)>& input)const {
        need(thread_==std::this_thread::get_id(),"Completed Skin observation loan foreign creator thread");
        Pin retained(*this);
        if(loan_active_){failed_=true;throw std::runtime_error("Completed Skin observation loan reentry");}
        loan_active_=true;
        struct Exit {const BnwNativeCompletedSkinInstanceObservation& owner;~Exit(){owner.loan_active_=false;}} exit{*this};
        try {
            retained.current();
            const auto instance=instance_;
            {
                const auto consume=input;
                retained.current();need(bool(consume),"Completed Skin observation consumer absent");
                consume(*instance);retained.current();
            }
            retained.current(); // Includes destruction of the copied target.
        }catch(...){failed_=true;throw;}
    }
  public:
    ~BnwNativeCompletedSkinInstanceObservation()=default;
    BnwNativeCompletedSkinInstanceObservation(const BnwNativeCompletedSkinInstanceObservation&)=delete;
    BnwNativeCompletedSkinInstanceObservation& operator=(const BnwNativeCompletedSkinInstanceObservation&)=delete;
};
}
